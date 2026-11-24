#! /usr/bin/env python3
'''
Copyright (C) 2026 Renesas Electronics Corp.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

'''

import cv2
import logging
import numpy as np
import sys
import time
import warnings

from argparse import ArgumentParser
from pathlib import Path

# Silence Interpreter deprecation warnings
warnings.filterwarnings(
    "ignore",
    message=r"\s*Warning: tf\.lite\.Interpreter is deprecated.*",
    category=UserWarning,
)
from tflite_runtime.interpreter import Interpreter, load_delegate

DELEGATE_LIBRARY_PATH = "libethosu_op_delegate.so.1"
MODEL_PATH = "/usr/share/models/mobilenet_v2/mobilenet_v2_1.0_224_INT8/mobilenet_v2_1.0_224_INT8_vela.tflite"

logger = logging.getLogger()

def preprocess(image_path: str, input_details: dict) -> np.ndarray:
    """
    Pre-processes an image file for MobileNet V2.

    Args:
        image_path: Path to image file.
        input_details: Input tensor details from the TensorFlow Lite interpreter.

    Returns:
        np.ndarray: Preprocessed image data matching the model's input tensor.
    """
    # Input quantization settings are read from the model
    scale, zero_point = input_details['quantization']
    dtype = input_details['dtype']
    height, width = input_details['shape'][1:3]

    # Check image exists
    if not Path(image_path).exists():
        logger.error(f"{image_path} does not exist")
        sys.exit(1)

    # Load image from file
    image = cv2.imread(image_path)
    if image is None:
        logger.error(f"Could not read {image_path}")
        sys.exit(1)

    # MobileNet V2 expects RGB input
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    # Resize image only if needed
    if image.shape[:2] != (height, width):
        # Set interpolation method if upsizing or downsizing
        if image.shape[0] > height or image.shape[1] > width:
            interpolation_method = cv2.INTER_AREA
        else:
            interpolation_method = cv2.INTER_LINEAR

        image = cv2.resize(image, (width, height), interpolation=interpolation_method)

    # Float models do not require normlization
    if dtype == np.float32:
        image = image.astype(np.float32) * (2.0 / 255.0) - 1.0

        return np.expand_dims(image, axis=0)

    info = np.iinfo(dtype)

    # Calculate normalization values
    real_min = (info.min - zero_point) * scale
    norm_low = -1.0 if real_min < 0 else 0.0
    norm_range = 1.0 - norm_low

    # Normalize the pixels and then quantize using models scale and zero_point
    pixels = np.arange(256, dtype=np.float64)
    lut = (pixels / 255.0) * norm_range + norm_low
    lut = np.rint(lut / scale + zero_point)

    # Clip to prevent values from not fitting boundary
    lut = np.clip(lut, info.min, info.max).astype(dtype)

    return np.expand_dims(lut[image], axis=0)

def read_labels() -> list[str]:
    """
    Read labels file and extract labels into a list.

    Returns:
        list[str]: List containing MobileNet v2 labels.
    """
    label_path = "/usr/share/models/mobilenet_v2/labels_mobilenet_v2_1.0_224.txt"
    if not Path(label_path).exists():
        logger.error(f"{label_path} does not exist")
        sys.exit(1)

    with open(label_path, 'r') as file:
        return [line.strip() for line in file.readlines()]

def postprocess(output_tensor: np.ndarray, output_details: dict, labels: list[str]):
    """
    Post process inference results and output top 5 classifications to terminal.

    Args:
        output_tensor: Raw output data from inference run.
        output_details: Output tensor details from the TensorFlow Lite interpreter.
        labels: List of classification labels.
    """
    # Output quantization settings are read from the model
    scale, zero_point = output_details['quantization']

    results_tensor = output_tensor.flatten()

    if results_tensor.dtype in (np.int8, np.uint8):
        # Convert output values from quantized integers to float
        float_tensor = (results_tensor.astype(np.float32) - zero_point) * scale
    else:
        float_tensor = results_tensor.astype(np.float32)

    if len(float_tensor) > len(labels):
        logger.error(f"Output size ({len(float_tensor)}) does not match number of labels ({len(labels)})")
        sys.exit(1)

    # Get top 5 classifications based on confidence
    top_indexes = np.argsort(float_tensor)[-5:][::-1]
    top_scores = float_tensor[top_indexes]
    detections = 0

    logger.info("Printing top Image Classification results...")

    for i in range(min(5, len(top_scores))):
        if top_scores[i] > 0:
            label = labels[top_indexes[i]]
            score = round(float(top_scores[i]) * 100, 2)
            detections += 1
            print(f"{score}% -> {label}")

        # If there are no detections above 0% confidence, alert the user
        if (i == len(top_scores) - 1 and detections == 0):
            print("No classifications identified!")

def main():
    format = "%(asctime)s %(levelname)s -  %(message)s"
    logging.basicConfig(format=format, level=logging.INFO)

    total_inference_times = 0

    parser = ArgumentParser(description="Renesas Ethos-U MobileNet V2 Classifier Application")
    parser.add_argument("-i", "--image", type=str, help="Path to image file to run inference with. Default: /usr/share/image-classification/images/cat.bmp",
                        default="/usr/share/image-classification/images/cat.bmp")
    parser.add_argument("-n", "--number", type=int, help="Number of times to run inference (inference time will be an average of all runs). Default: 10", default=10)
    args = parser.parse_args()

    if args.number <= 0:
        parser.error("Number of runs must be greater than 0!")

    inference_runs = args.number

    # Create Ethos-U delegate
    try:
        ethosu_delegate = load_delegate(DELEGATE_LIBRARY_PATH)
    except ValueError as e:
        logger.error(f"Ethos-U delegate library could not be loaded: {e}")
        sys.exit(1)

    # Load model and create interpreter
    try:
        interpreter = Interpreter(model_path=MODEL_PATH, experimental_delegates=[ethosu_delegate])
        interpreter.allocate_tensors()
    except Exception as e:
        logger.error(f"Failed to load model ({MODEL_PATH}): {e}")
        sys.exit(1)

    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]

    # Preprocess image and set input
    input_data = preprocess(args.image, input_details)
    interpreter.set_tensor(input_details['index'], input_data)

    labels = read_labels()

    completed_runs = 0
    dry_run_done = False

    # Run inference and measure inference time for each run
    while completed_runs < inference_runs:
        start_inf_time = time.perf_counter()

        try:
            interpreter.invoke()
        except Exception as e:
            logger.error(f"Inference Run #{completed_runs + 1} failed: {e}")
            sys.exit(1)

        # First inference is a dry run to clear buffers so do not include in averaging
        if not dry_run_done:
            dry_run_done = True
            continue

        completed_runs += 1

        inference_time = round((time.perf_counter() - start_inf_time) * 1000, 6)

        logger.info(f"Inference Run #{completed_runs}, inference time: {inference_time}ms")

        total_inference_times += inference_time

    average_inference_time = round(total_inference_times / inference_runs, 2)

    if inference_runs == 1:
        inference_runs_text = "run"
    else:
        inference_runs_text = "runs"

    logger.info(f"Average Inference time: {average_inference_time}ms over {inference_runs} {inference_runs_text}")

    postprocess(interpreter.get_tensor(output_details['index']), output_details, labels)

if __name__ == '__main__':
    main()
