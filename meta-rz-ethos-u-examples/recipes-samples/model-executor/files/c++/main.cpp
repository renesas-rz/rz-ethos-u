/*
 * Copyright (C) 2026 Renesas Electronics Corp.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <tensorflow/lite/delegates/external/external_delegate.h>
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/kernels/register_ref.h>
#include <tensorflow/lite/model.h>
#include <vector>

#include <getopt.h>

#include "yaml_parser.h"

#define DEFAULT_EXECUTOR_CONFIG_FILE "/usr/share/models/mobilenet_v2/config.yaml"
#define DEFAULT_INFERENCE_RUNS 10
#define DEFAULT_TIMEOUT 60000000000ULL
#define DELEGATE_LIBRARY_PATH "libethosu_op_delegate.so.1"

using namespace std;
using namespace tflite;

static const char short_opts[] = "e:n:rt:h";
static const option long_opts[] =
{
	{"executor_config_file", required_argument, NULL, 'e'},
	{"number", required_argument, NULL, 'n'},
	{"use_ref_kernels", no_argument, NULL, 'r'},
	{"timeout", required_argument, NULL, 't'},
	{"help", no_argument, NULL, 'h'},
	{0, 0, 0, 0}
};

void print_help(char *app_name) {
	printf("This application will run Ethos-U inference on the RZ/G3E "
		"using Arm Ethos-U delegate with TensorFlow Lite.\n"
		"USAGE: %s, [-e <executor configuration file>]\n"
		"           [-n <number of inference runs>]\n"
		"           [-r]\n"
		"           [-t <timeout in nanoseconds>]\n"
		"           [-h]\n"
		"OPTIONS:\n"
		"-e, --executor_config_file:  Path to the file containing information for models to run inference with.\n"
		"                             Default: %s\n"
		"-n, --number:                Number of times to run inference (inference time will be an average of all runs).\n"
		"                             Default: %d\n"
		"-r, --use_ref_kernels:       Use TensorFlow Lite reference kernels for CPU offload.\n"
		"-t, --timeout:               Timeout in nanoseconds.\n"
		"                             Default: %llu\n"
		"-h, --help:                  Prints this help and exit.\n",
		app_name, DEFAULT_EXECUTOR_CONFIG_FILE, DEFAULT_INFERENCE_RUNS, (unsigned long long)DEFAULT_TIMEOUT);
}

bool load_model_input(Interpreter &interpreter, string file_path) {
	ifstream input_file(file_path, ios::binary | ios::ate);

	if (!input_file.is_open()) {
		fprintf(stderr, "Unable to load input file (%s)\n", file_path.c_str());

		return false;
	}

	size_t total_actual_input_size = static_cast<size_t>(input_file.tellg());

	input_file.seekg(0, ios::beg);

	/* Calculate expected size for the models input */
	size_t total_expected_input_size = 0;

	for (int index : interpreter.inputs())
		total_expected_input_size += interpreter.tensor(index)->bytes;

	/* Verify the input file matches the models input size */
	if (total_actual_input_size != total_expected_input_size) {
		fprintf(stderr, "Input binary file size %zu doesn't match total size of input tensors %zu\n", total_actual_input_size, total_expected_input_size);
		input_file.close();

		return false;
	}

	/* Set inputs */
	for (int index : interpreter.inputs()) {
		TfLiteTensor *tensor = interpreter.tensor(index);
		input_file.read(tensor->data.raw, tensor->bytes);
	}

	input_file.close();

	return true;
}

bool set_interpreter(unique_ptr<Interpreter> &interpreter, unique_ptr<FlatBufferModel> &flatbuffer_model, bool use_ref_kernels, string model_file, TfLiteDelegate &ethosu_delegate, string input_file) {
	unique_ptr<OpResolver> op_resolver;

	/* Set operator resolver to use TFLite reference kernels or XNNPack delegate */
	if (use_ref_kernels)
		op_resolver = make_unique<ops::builtin::BuiltinRefOpResolver>();
	else
		op_resolver = make_unique<ops::builtin::BuiltinOpResolver>();

	/* Load model */
	flatbuffer_model = FlatBufferModel::VerifyAndBuildFromFile(model_file.c_str());

	if (!flatbuffer_model) {
		fprintf(stderr, "Failed to load the model, skipping it\n");

		return false;
	}

	if (InterpreterBuilder(*flatbuffer_model, *op_resolver)(&interpreter) != kTfLiteOk) {
		fprintf(stderr, "Failed to create TFLite interpreter, skipping model\n");

		return false;
	}

	if (interpreter->ModifyGraphWithDelegate(&ethosu_delegate) != kTfLiteOk) {
		fprintf(stderr, "Failed to apply Ethos-U delegate, skipping model\n");

		return false;
	}

	if (interpreter->AllocateTensors() != kTfLiteOk) {
		fprintf(stderr, "Failed to allocate tensors, skipping model\n");

		return false;
	}

	if (!load_model_input(*interpreter, input_file)) {
		printf("Skipping model\n");

		return false;
	}

	return true;
}

bool save_output_to_file(Interpreter &interpreter, string output_directory) {
	string output_file = output_directory + "executor_output.bin";

	ofstream output_stream(output_file, ios::binary);

	if (!output_stream.is_open()) {
		fprintf(stderr, "Unable to open created output file %s\n", output_file.c_str());

		return false;
	}

	/* Save all outputs into one file */
	for (auto index : interpreter.outputs()) {
		const auto *tensor = interpreter.tensor(index);

		if (!tensor) {
			fprintf(stderr, "Failed to find output tensor #%d\n", index);

			return false;
		}

		output_stream.write(tensor->data.raw, tensor->bytes);
	}

	output_stream.flush();

	if (!output_stream) {
		fprintf(stderr, "Failed to write output file %s\n", output_file.c_str());

		return false;
	}

	return true;
}

int main(int argc, char **argv) {
	Executor executor_config;
	YamlParser yaml_parser;
	bool use_ref_kernels = false;
	string executor_config_file_path = DEFAULT_EXECUTOR_CONFIG_FILE;
	string inference_runs_text = "runs";
	unsigned int inference_runs = DEFAULT_INFERENCE_RUNS;
	uint64_t timeout = DEFAULT_TIMEOUT;
	TfLiteDelegate *ethosu_delegate = nullptr;
	unsigned int failed_models = 0;

	try {
		while (true) {
			int opt, index = 0;

			opt = getopt_long(argc, argv, short_opts, long_opts, &index);

			if (opt == -1)
				break;

			switch (opt) {
				case 'e': {
					executor_config_file_path = string(optarg);

					/* Verify the executor config file exists */
					ifstream executor_config_file(executor_config_file_path);

					if (!executor_config_file) {
						executor_config_file.close();
						fprintf(stderr, "Cannot open executor configuration file: %s\n", executor_config_file_path.c_str());

						return EXIT_FAILURE;
					} else {
						executor_config_file.close();
						break;
					}
				}
				case 'n': {
					char *end = nullptr;

					errno = 0;
					long long val = strtoll(optarg, &end, 10);

					/* Verify the number of inference runs */
					if (errno != 0 || end == optarg || *end != '\0' || val < 1 || val > UINT_MAX) {
						fprintf(stderr, "Invalid number of inference runs: %s. Please select a number greater than 0!\n", optarg);

						return EXIT_FAILURE;
					} else {
						inference_runs = (unsigned int)val;
						break;
					}
				}
				case 'r': {
					use_ref_kernels = true;
					break;
				}
				case 't': {
					char *end = nullptr;

					errno = 0;
					long long val = strtoll(optarg, &end, 10);

					if (errno != 0 || end == optarg || *end != '\0' || val < 1) {
						fprintf(stderr, "Invalid timeout: %s. Please select a number greater than 0!\n", optarg);

						return EXIT_FAILURE;
					} else {
						timeout = (uint64_t)val;
						break;
					}
				}
				case 'h': {
					print_help(argv[0]);

					return EXIT_SUCCESS;
				}
				default: {
					print_help(argv[0]);

					return EXIT_FAILURE;
				}
			}
		}

		executor_config = yaml_parser.parseYaml(executor_config_file_path);

		/* Get path to directory that contains the models */
		filesystem::path config_path = filesystem::absolute(executor_config_file_path);
		filesystem::path executor_path = config_path.parent_path();
		string executor_directory = executor_path.string() + "/";

		/* Create Ethos-U delegate */
		TfLiteExternalDelegateOptions delegate_options = TfLiteExternalDelegateOptionsDefault(DELEGATE_LIBRARY_PATH);
		TfLiteExternalDelegateOptionsInsert(&delegate_options, "timeout", to_string(timeout).c_str());

		ethosu_delegate = TfLiteExternalDelegateCreate(&delegate_options);

		if (!ethosu_delegate) {
			fprintf(stderr, "Failed to create Ethos-U delegate\n");

			return EXIT_FAILURE;
		}

		/*
		 * Run inference for number of runs defined and store output
		 * for each model defined in configuration YAML
		 */
		for (const auto& model : executor_config.models) {
			bool model_failed = false;
			float average_inference_time = 0.0;
			float total_inference_times = 0.0;
			int failed_inference_counter = 0;
			string model_directory = executor_directory + model.data_directory;
			string model_file = model_directory + model.model_file;
			string input_file = model_directory + model.input_file;
			unique_ptr<FlatBufferModel> flatbuffer_model;
			unique_ptr<Interpreter> interpreter;

			printf("Model being executed: %s\n", model.name.c_str());

			/* Prepare TFLite interpreter with Ethos-U delegate */
			if (!set_interpreter(interpreter, flatbuffer_model, use_ref_kernels, model_file, *ethosu_delegate, input_file)) {
				failed_models++;
				continue;
			}

			bool dry_run_done = false;
			unsigned int completed_runs = 0;

			/* Run inference and measure inference time for each run */
			while (completed_runs < inference_runs) {
				auto start_time = chrono::steady_clock::now();

				/* Run inference */
				if(interpreter->Invoke() != kTfLiteOk) {
					failed_inference_counter++;
					fprintf(stderr, "Failed to invoke inference: %d\n", failed_inference_counter);

					/*
					 * If inference failed 3 times in a row, skip model
					 * else retry failed run
					 */
					if (failed_inference_counter == 3) {
						fprintf(stderr, "Failed to invoke inference 3 times in a row, skipping model\n");
						model_failed = true;
						break;
					} else {
						printf("Retrying failed inference run\n");
						continue;
					}
				}

				failed_inference_counter = 0;

				/*
				 * First inference is a dry run to clear buffers so do not
				 * include in averaging
				 */
				if (!dry_run_done) {
					dry_run_done = true;
					continue;
				}

				completed_runs++;

				auto stop_time = chrono::steady_clock::now();
				chrono::duration<float, milli> inference_time = stop_time - start_time;

				printf("Inference Run #%u, end-to-end inference time: %fms\n", completed_runs, inference_time.count());

				total_inference_times += inference_time.count();
			}

			if (model_failed) {
				failed_models++;
				continue;
			}

			/* Calculate average inference time */
			average_inference_time = total_inference_times / inference_runs;

			/* Round average to 2 decimal places */
			average_inference_time = round(average_inference_time * 100.0f) / 100.0f;

			if (inference_runs == 1)
				inference_runs_text = "run";

			printf("------------------------------\n");
			printf("           Results            \n");
			printf("Model: %s\n", model.name.c_str());
			printf("Average inference time: %gms over %u %s\n", average_inference_time, inference_runs, inference_runs_text.c_str());
			printf("------------------------------\n");

			/* Save each output tensor to a local binary file */
			if (!save_output_to_file(*interpreter, model_directory))
				failed_models++;
		}
	} catch (const exception& e) {
		fprintf(stderr, "ERROR: %s\n", e.what());

		if (ethosu_delegate)
			TfLiteExternalDelegateDelete(ethosu_delegate);

		return EXIT_FAILURE;
	}

	if (ethosu_delegate)
		TfLiteExternalDelegateDelete(ethosu_delegate);

	return failed_models ? EXIT_FAILURE : EXIT_SUCCESS;
}
