#! /usr/bin/env python3
'''
Copyright (C) 2026 Renesas Electronics Corp.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

'''

import argparse
import logging
import numpy as np
import os
import sys
import time
import warnings
import yaml

from pathlib import Path

# Silence Interpreter deprecation warnings
warnings.filterwarnings(
    "ignore",
    message=r"\s*Warning: tf\.lite\.Interpreter is deprecated.*",
    category=UserWarning,
)
from tflite_runtime.interpreter import Interpreter, load_delegate, OpResolverType, Delegate

logger = logging.getLogger()

def get_args() -> argparse.Namespace:
    """
    Get command-line arguments and verify.

    Returns:
        argparse.Namespace: Command-line arguments set.
    """
    parser = argparse.ArgumentParser(prog="Renesas Ethos-U Model Executor Application", description="Runs Ethos-U inference on RZ/G3E using Arm Ethos-U delegate with TensorFlow Lite")
    parser.add_argument("-e", "--executor_config_file", type=str,
                        help="Path to the file containing information for models to run inference with. Default: /usr/share/models/mobilenet_v2/config.yaml",
                        default="/usr/share/models/mobilenet_v2/config.yaml")
    parser.add_argument("-n", "--number", type=int,
                        help="Number of times to run inference (inference time will be an average of all runs). Default: 10", default=10)
    parser.add_argument("-r", "--use_ref_kernels", help="Use TensorFlow Lite reference kernels for CPU offload.", action="store_true")
    parser.add_argument("-t", "--timeout", type=int, help="Timeout in nanoseconds. Default: 60000000000", default=60000000000)
    args = parser.parse_args()

    if (args.number <= 0):
        parser.error("Number of runs must be greater than 0!")

    if (args.timeout <= 0):
        parser.error("Timeout must be greater than 0!")

    if not os.path.exists(args.executor_config_file):
        parser.error(f"{args.executor_config_file} does not exist!")

    return args

def load_config(config_path: str) -> dict:
    """
    Load model executor configuration from a YAML file.

    Args:
        config_path (str): Path to the YAML configuration file.

    Returns:
        dict: Parsed configuration data.
    """

    try:
        with open(config_path, "r") as file:
            config = yaml.safe_load(file)
    except yaml.YAMLError as e:
        logger.error(f"YAML configuration file ({config_path}) has incorrect YAML formatting: {e}")
        sys.exit(1)

    if not isinstance(config, dict):
        logger.error(f"YAML configuration file ({config_path}) must contain a mapping with a 'models' list")
        sys.exit(1)

    return config

def load_model_input(interpreter: Interpreter, input_file: str) -> bool:
    """
    Set model input using the data from input_file defined in the YAML configuration file.

    Args:
        interpreter (Interpreter): TensorFlow Lite interpreter.
        input_file (str): Path to file containing sample input for model.

    Returns:
        bool: Status of setting the model input.
    """
    if not os.path.exists(input_file):
        logger.error(f"Unable to load input file ({input_file})")

        return False

    total_actual_input_size = os.path.getsize(input_file)

    # Calculate expected size for the models input
    input_details = interpreter.get_input_details()
    input_sizes = [int(np.prod(details['shape'])) * np.dtype(details['dtype']).itemsize for details in input_details]
    total_expected_input_size = sum(input_sizes)

    # Verify the input file matches the models input size
    if total_actual_input_size != total_expected_input_size:
        logger.error(
            f"Input binary file size {total_actual_input_size} doesn't match "
            f"total size of input tensors {total_expected_input_size}"
        )

        return False

    # Set inputs
    with open(input_file, 'rb') as file:
        for details, nbytes in zip(input_details, input_sizes):
            tensor_bytes = file.read(nbytes)
            tensor_data = np.frombuffer(tensor_bytes, dtype=details['dtype']).reshape(details['shape'])
            interpreter.set_tensor(details['index'], tensor_data)

    return True
    

def set_interpreter(use_ref_kernels: bool, model_file: str, ethosu_delegate: Delegate, input_file: str) -> tuple[bool, Interpreter]:
    """
    Set model input using the data from input_file defined in the YAML configuration file.

    Args:
        use_ref_kernels (bool): Status for using TensorFlow Lite reference kernels for CPU offload.
        model_file (str): Path to model file.
        ethosu_delegate (Delegate): Ethos-U delegate.
        input_file (str): Path to file containing sample input for model.

    Returns:
        bool: Status of setting the model input.
        Interpreter: TensorFlow Lite interpreter.
    """
    # Set operator resolver to use TFLite TFLite reference kernels or XNNPack delegate
    if use_ref_kernels:
        op_resolver = OpResolverType.BUILTIN_REF
    else:
        op_resolver = OpResolverType.BUILTIN

    # Load model and create interpreter
    interpreter = None

    try:
        interpreter = Interpreter(model_path=model_file, experimental_delegates=[ethosu_delegate], experimental_op_resolver_type=op_resolver)
        interpreter.allocate_tensors()
        status = load_model_input(interpreter, input_file)
    except Exception as e:
        logger.error(f"Hit error: {e}")
        status = False

    return status, interpreter

def save_output_to_file(interpreter: Interpreter, model_data_dir: str):
    """
    Save all model output tensors to a binary file.

    Args:
        interpreter (Interpreter): TensorFlow Lite interpreter.
        model_data_dir (str): Path to directory containing model data.
    """
    output_file = os.path.join(model_data_dir, "executor_output_python.bin")

    with open(output_file, 'wb') as file:
        for detail in interpreter.get_output_details():
            tensor = interpreter.get_tensor(detail["index"])
            file.write(tensor.data)

def main():
    format = "%(asctime)s %(levelname)s -  %(message)s"
    logging.basicConfig(format=format, level=logging.INFO)

    args = get_args()
    inference_runs = args.number
    executor_config_file = args.executor_config_file
    timeout = args.timeout
    use_ref_kernels = args.use_ref_kernels

    # Get path to directory containing models
    executor_config = load_config(executor_config_file)
    executor_dir = Path(executor_config_file).parent

    delegate_options = {
        "timeout": str(timeout),
    }

    # Create Ethos-U delegate
    try:
        ethosu_delegate = load_delegate("libethosu_op_delegate.so.1", options=delegate_options)
    except ValueError as e:
        logger.error(f"Ethos-U delegate library could not be loaded: {e}")
        sys.exit(1)

    models = executor_config.get("models", [])
    failed_models = 0

    for model_number, model_info in enumerate(models):
        # Verify YAML configuration before using its data
        required_keys = ["name", "data_directory", "model_file", "input_file"]

        # Verify model entry and its keys, skipping the model if invalid
        if not isinstance(model_info, dict):
            logger.error(f"Model #{model_number + 1} must be a mapping in configuration file, skipping model")
            failed_models += 1
            continue

        missing_keys = [key for key in required_keys if key not in model_info]

        if missing_keys:
            logger.error(f"Missing required key(s) {missing_keys} for model #{model_number + 1} in configuration file, skipping model")
            failed_models += 1
            continue

        model_name = model_info.get("name")

        logger.info(f"Model being executed: {model_name}")

        model_data_dir = os.path.join(executor_dir, model_info.get("data_directory"))
        model_file = os.path.join(model_data_dir, model_info.get("model_file"))
        input_file = os.path.join(model_data_dir, model_info.get("input_file"))

        total_inference_times = 0

        # Prepare TFLite interpreter with Ethos-U delegate
        status, interpreter = set_interpreter(use_ref_kernels, model_file, ethosu_delegate, input_file)

        if not status:
            logger.warning("Skipping model")
            failed_models += 1
            continue

        model_failed = False
        failed_inference_counter = 0
        completed_runs = 0
        dry_run_done = False

        # Run inference and measure inference time for each run
        while completed_runs < inference_runs:
            start_inf_time = time.perf_counter()

            try:
                interpreter.invoke()
            except Exception as e:
                failed_inference_counter += 1
                logger.error(f"Failed to invoke inference: {failed_inference_counter} (error = {e})")

                # If inference failed 3 times in a row, skip model
                # else retry failed run
                if failed_inference_counter == 3:
                    logger.error("Failed to invoke inference 3 times in a row, skipping model")
                    model_failed = True
                    break
                else:
                    logger.warning("Retrying failed inference run")
                    continue

            failed_inference_counter = 0

            # First inference is a dry run to clear buffers so do not include in averaging
            if not dry_run_done:
                dry_run_done = True
                continue

            completed_runs += 1

            inference_time = round((time.perf_counter() - start_inf_time) * 1000, 6)

            logger.info(f"Inference Run #{completed_runs}, inference time: {inference_time}ms")

            total_inference_times += inference_time

        if model_failed:
            failed_models += 1
            continue

        average_inference_time = round(total_inference_times / inference_runs, 2)

        if (inference_runs == 1):
            inference_runs_text = "run"
        else:
            inference_runs_text = "runs"

        logger.info("------------------------------")
        logger.info("           Results            ")
        logger.info(f"Model: {model_name}")
        logger.info(f"Average inference time: {average_inference_time}ms over {inference_runs} {inference_runs_text}")
        logger.info("------------------------------")

        try:
            save_output_to_file(interpreter, model_data_dir)
        except OSError as e:
            logger.error(f"Failed to write output file: {e}")
            failed_models += 1

    sys.exit(1 if failed_models else 0)

if __name__ == '__main__':
    main()
