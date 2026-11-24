#! /usr/bin/env python3
'''
Copyright (C) 2026 Renesas Electronics Corp.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

'''

import argparse
import json
import logging
import numpy as np
import os
import shutil
import subprocess
import sys
import warnings
import yaml

from pathlib import Path

# Silence Interpreter deprecation warnings
warnings.filterwarnings(
    "ignore",
    message=r"\s*Warning: tf\.lite\.Interpreter is deprecated.*",
    category=UserWarning,
)
from tflite_runtime.interpreter import Interpreter, OpResolverType

logger = logging.getLogger()

def running_on_rzg3e() -> bool:
    """
    Check whether the script is running on an RZ/G3E board using the SoC
    compatible string from the device tree.

    Returns:
        bool: True if running on an RZ/G3E, False otherwise (e.g. Host PC).
    """
    try:
        compatible = Path("/proc/device-tree/compatible").read_bytes().split(b"\0")
    except OSError:
        return False

    return b"renesas,r9a09g047" in compatible

ON_RZG3E = running_on_rzg3e()

# On the board the file is installed by the ethos-u-vela package, on the Host PC
# it sits next to this script in the repository
if ON_RZG3E:
    DEFAULT_VELA_CONFIG = Path("/usr/share/vela/RZG3E_vela.ini")
else:
    DEFAULT_VELA_CONFIG = Path(__file__).resolve().parent / "RZG3E_vela.ini"

def get_args() -> argparse.Namespace:
    """
    Get command-line arguments and verify.

    Returns:
        argparse.Namespace: Command-line arguments set.
    """
    parser = argparse.ArgumentParser(prog="Renesas Ethos-U Model Data Generator", description="Generates the model data needed to run the Renesas Model Executor application along with the sample input and expected output (optional)")
    parser.add_argument("-c", "--config", type=Path, default=DEFAULT_VELA_CONFIG,
                        help="Vela configuration file (default: %(default)s)")
    parser.add_argument("-d", "--output_dir", type=str, required=True,
                        help="Output directory where files will be stored")
    parser.add_argument("-m", "--model_paths", nargs='+', type=str, required=True,
                        help="Space separated list of TFLite models to generate the files for")

    # Only show --generate_output option for on-device compilation
    if ON_RZG3E:
        parser.add_argument("-o", "--generate_output", action="store_true",
                            help="Store expected outputs into a file")

    parser.add_argument("-s", "--seed", type=int, default=10,
                        help="Seed for generating randomised input data (default: %(default)s)")
    args = parser.parse_args()

    for model_path in args.model_paths:
        if not os.path.exists(model_path):
            parser.error(f"{model_path} does not exist!")

    if not args.config.is_file():
        parser.error(f"Vela configuration file {args.config} does not exist!")

    return args

def generate_model_test_data(model_name: str, model_path: str, model_deploy_dir: str, generate_output: bool, seed: int) -> dict:
    """
    Generates model test data (inputs/outputs) and the entry for the
    configuration YAML.

    Args:
        model_name (str): Name of the input model (without TFLite extension).
        model_path (str): Path to model file.
        model_deploy_dir (str): Model deployment directory.
        generate_output (bool): Generate expected output files.
        seed (int): Seed for generating randomised input data.

    Returns:
        dict: Dictionary containing the model entry for configuration YAML.
    """
    # Load tflite model
    interpreter = Interpreter(model_path=model_path, experimental_op_resolver_type=OpResolverType.BUILTIN_REF)
    interpreter.allocate_tensors()

    # Get input information
    input_details = interpreter.get_input_details()

    input_file_path = os.path.join(model_deploy_dir, "all-inputs.bin");

    logger.info(f"Creating randomised input data using seed = {seed}")
    rng = np.random.default_rng(seed)

    with open(input_file_path, 'wb') as f:
        for index, input_detail in enumerate(input_details):
            shape = input_detail['shape'].tolist()
            dtype = input_detail['dtype']

            # Generate randomized input
            if dtype == np.int8:
                input_data = rng.integers(low=-128, high=128, size=shape, dtype=np.int8)
            elif dtype == np.uint8:
                input_data = rng.integers(low=0, high=256, size=shape, dtype=np.uint8)
            elif dtype == np.float32:
                input_data = rng.random(size=shape, dtype=np.float32)
            else:
                raise ValueError(f"Unsupported input data type ({dtype})")

            # Write input tensor into input file
            f.write(input_data.tobytes())

            interpreter.set_tensor(input_detail['index'], input_data)

    logger.info(f"Input data for all tensors has been saved to: {input_file_path}")

    if generate_output:
        # Run inference and save expected output to model output directory
        interpreter.invoke()

        output_info = interpreter.get_output_details()
        output_file = os.path.join(model_deploy_dir, f"expected-outputs.bin")

        with open(output_file, 'wb') as f:
            for output_tensor in output_info:
                output_data = interpreter.get_tensor(output_tensor['index'])
                f.write(output_data.tobytes())

        logger.info(f"Expected output data for all tensors has been saved to: {output_file}")

    return {
        'name': model_name,
        'data_directory': f"{model_name}/",
        'model_file': f"{model_name}_vela.tflite",
        'input_file': "all-inputs.bin"
    }

if __name__ == '__main__':
    format = "%(asctime)s %(levelname)s -  %(message)s"
    logging.basicConfig(format=format, level=logging.INFO)

    args = get_args()

    output_dir = args.output_dir.rstrip('/')

    if ON_RZG3E:
        generate_output = args.generate_output
    else:
        generate_output = True

    # Save backup if directory already exists
    if os.path.exists(output_dir):
        output_dir_backup = f"{output_dir}_backup"

        logger.warning(f"{output_dir} already exists! The directory has now been moved to {output_dir_backup}")

        # Remove backup directory if it already exists
        if os.path.exists(output_dir_backup):
            logger.warning(f"Removing previous backup directory ({output_dir_backup})!")

            shutil.rmtree(output_dir_backup)

        os.rename(output_dir, output_dir_backup)

    os.mkdir(output_dir)

    model_paths = args.model_paths

    model_config_list = []

    # Generate data for each model defined
    for model_path in model_paths:
        model_name = os.path.basename(model_path)
        model_name = model_name.replace(".tflite", "")

        model_deploy_dir = os.path.join(output_dir, model_name)
        os.makedirs(model_deploy_dir, exist_ok=True)

        logger.info("------------------------------------------------------------")
        logger.info(f"Model being compiled with VELA: {model_name} ({model_path})")

        if model_path is None:
            logger.error(f"Unable to find model file, skipping compilation for {model_name}")
            continue

        vela_cmd = [
            "vela", str(model_path),
            "--accelerator-config", "ethos-u55-256",
            "--optimise", "Size",
            "--config", str(args.config),
            "--system-config", "RZG3E_DIRECT_DRIVE",
            "--tensor-allocator", "HillClimb",
            "--memory-mode", "Dedicated_Sram",
            "--cop-format", "COP2",
            "--separate-io-region",
            "--output-dir", str(model_deploy_dir),
        ]

        try:
            # Compile model (blocks until VELA finishes so model_deploy_dir is ready)
            subprocess.run(vela_cmd, check=True)

            model_config_list.append(generate_model_test_data(model_name, model_path, model_deploy_dir, generate_output, args.seed))
        except (subprocess.CalledProcessError, ValueError) as e:
            logger.error(f"Skipping {model_name}: {e}")
            continue

        logger.info("------------------------------------------------------------")

    config_yaml = {'models': model_config_list}
    config_yaml_path = os.path.join(output_dir, "config.yaml")

    # Save the configuration YAML to output directory
    with open(config_yaml_path, 'w') as f:
        yaml.dump(config_yaml, f, default_flow_style=False, sort_keys=False)

    logger.info(f"The configuration YAML has been saved to: {config_yaml_path}")
    if ON_RZG3E:
        logger.info(f"To run these models, execute: /usr/bin/model-executor -e {config_yaml_path}")
    else:
        logger.info(f"Please copy the {output_dir} directory to the board!")

    sys.exit()
