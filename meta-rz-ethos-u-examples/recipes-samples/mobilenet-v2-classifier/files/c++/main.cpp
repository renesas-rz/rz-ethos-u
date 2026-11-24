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
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <memory>
#include <opencv4/opencv2/core/core.hpp>
#include <tensorflow/lite/delegates/external/external_delegate.h>
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/kernels/register_ref.h>
#include <tensorflow/lite/model.h>

#include "processing.h"
#include "wayland_utils.h"

#define DEFAULT_IMAGE_FILE_PATH "/usr/share/image-classification/images/cat.bmp"
#define DEFAULT_INFERENCE_RUNS 10
#define DELEGATE_LIBRARY_PATH "libethosu_op_delegate.so.1"
#define MODEL_PATH "/usr/share/models/mobilenet_v2/mobilenet_v2_1.0_224_INT8/mobilenet_v2_1.0_224_INT8_vela.tflite"

using namespace std;
using namespace tflite;

static const char short_opts[] = "co:i:n:h";
static const option long_opts[] =
{
	{"console", no_argument, NULL, 'c'},
	{"output", required_argument, NULL, 'o'},
	{"image", required_argument, NULL, 'i'},
	{"number", required_argument, NULL, 'n'},
	{"help", no_argument, NULL, 'h'},
	{0, 0, 0, 0}
};

void print_help(char *app_name) {
	printf("This application will run inference on a static image using "
		"MobileNet V2 (Image Classification) on the Arm Ethos-U55 NPU.\n"
		"USAGE: %s, [-c] [-o <output name>] [-i <image file>] [-n <number of inference runs>] [-h]\n\n"
		"OPTIONS:\n"
		"-c, --console: Prints results to console only, no display.\n"
		"-o, --output: Name of the Wayland output to show results on, e.g. %s.\n"
		"              Default: %s, or the first display found if it is not connected.\n"
		"              Supported resolutions: 1920x1080, 1280x720 and 800x600.\n"
		"-i, --image: Path to image file to run inference with.\n"
		"              Default: /usr/share/image-classification/images/cat.bmp\n"
		"-n, --number: Number of times to run inference (inference time will be an average of all runs).\n"
		"              Default: 10\n"
		"-h, --help: Prints this help and exit.\n",
		app_name, DEFAULT_DISPLAY_NAME, DEFAULT_DISPLAY_NAME);
}

bool set_interpreter(unique_ptr<Interpreter> &interpreter, unique_ptr<FlatBufferModel> &flatbuffer_model, string model_file, TfLiteDelegate &ethosu_delegate) {
	unique_ptr<OpResolver> op_resolver;

	/* Set operator resolver to use XNNPack delegate */
	op_resolver = make_unique<ops::builtin::BuiltinOpResolver>();

	/* Load model */
	flatbuffer_model = FlatBufferModel::VerifyAndBuildFromFile(model_file.c_str());

	if (!flatbuffer_model) {
		fprintf(stderr, "Failed to load the model\n");

		return false;
	}

	if (InterpreterBuilder(*flatbuffer_model, *op_resolver)(&interpreter) != kTfLiteOk) {
		fprintf(stderr, "Failed to create TFLite interpreter\n");

		return false;
	}

	if (interpreter->ModifyGraphWithDelegate(&ethosu_delegate) != kTfLiteOk) {
		fprintf(stderr, "Failed to apply Ethos-U delegate\n");

		return false;
	}

	return true;
}

cv::Mat set_input(unique_ptr<Interpreter> &interpreter, cv::Mat &image) {
	int height;
	int width;
	double zero_point = 0.0;
	float scale = 1.0f;
	ModelDataType data_type;

	int index = interpreter->inputs()[0];

	TfLiteTensor *input_tensor = interpreter->tensor(index);
	height = input_tensor->dims->data[1];
	width = input_tensor->dims->data[2];

	if (input_tensor->type == kTfLiteInt8 || input_tensor->type == kTfLiteUInt8) {
		data_type = (input_tensor->type == kTfLiteInt8) ? ModelDataType::INT8 : ModelDataType::UINT8;
		zero_point = input_tensor->params.zero_point;
		scale = input_tensor->params.scale;
	} else if (input_tensor->type == kTfLiteFloat32) {
		data_type = ModelDataType::FLOAT32;
	} else {
		throw runtime_error("Input Data Type not supported");
	}

	cv::Mat processed = preprocess_image(image, width, height, data_type, scale, zero_point);

	if (processed.total() * processed.elemSize() != input_tensor->bytes) {
		throw runtime_error("Input size mismatch: image " + to_string(processed.total() * processed.elemSize()) + " bytes, tensor " + to_string(input_tensor->bytes) + " bytes");
	}

	memcpy(input_tensor->data.raw, processed.data, input_tensor->bytes);

	return processed;
}

void get_output(unique_ptr<Interpreter> &interpreter, const vector<string> &labels, cv::Mat &image, float inference_time, int inference_runs, bool use_display, const string& display_name) {
	const TfLiteTensor *output_tensor = interpreter->tensor(interpreter->outputs()[0]);
	ModelDataType data_type;
	float scale = 1.0f;
	double zero_point = 0.0;
	int64_t output_size = 1;

	/* Get output quantization parameters */
	if (output_tensor->type == kTfLiteInt8 || output_tensor->type == kTfLiteUInt8) {
		data_type = (output_tensor->type == kTfLiteInt8) ? ModelDataType::INT8 : ModelDataType::UINT8;
		scale = output_tensor->params.scale;
		zero_point = output_tensor->params.zero_point;
	} else if (output_tensor->type == kTfLiteFloat32) {
		data_type = ModelDataType::FLOAT32;
	} else {
		throw runtime_error("Output Data Type not supported");
	}

	for (int i = 0; i < output_tensor->dims->size; i++)
		output_size *= output_tensor->dims->data[i];

	post_process_output(output_size, output_tensor->data.raw_const, data_type, scale, zero_point, labels, image, inference_time, inference_runs, use_display, display_name);
}

int main(int argc, char **argv) {
	bool use_display = true;
	string display_name = "";
	string image_file_path = DEFAULT_IMAGE_FILE_PATH;
	unsigned int inference_runs = DEFAULT_INFERENCE_RUNS;
	cv::Mat rgb_image;
	vector<string> labels = {};
	float total_inference_times = 0.0;
	float average_inference_time = 0.0;
	string inference_runs_text = "runs";
	TfLiteDelegate *ethosu_delegate = nullptr;

	try {
		while (true) {
			int opt, index = 0;

			opt = getopt_long(argc, argv, short_opts, long_opts, &index);

			if (opt == -1)
				break;

			switch (opt) {
				case 'c': {
					use_display = false;
					break;
				}
				case 'o': {
					display_name = string(optarg);
					break;
				}
				case 'i': {
					image_file_path = string(optarg);

					/* Verify the image file exists and can be opened */
					ifstream image_file(image_file_path);

					if (!image_file) {
						image_file.close();
						fprintf(stderr, "Cannot open image file: %s\n", image_file_path.c_str());

						return EXIT_FAILURE;
					} else {
						image_file.close();
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

		if (!use_display && !display_name.empty())
			printf("Warning: --output is ignored when --console is used\n");

		/* Load RGB image from file */
		rgb_image = load_image_file(image_file_path);

		if (rgb_image.empty())
			throw runtime_error("Unable to load image file: " + image_file_path);

		/* Create Ethos-U delegate with default delegate options */
		TfLiteExternalDelegateOptions delegate_options = TfLiteExternalDelegateOptionsDefault(DELEGATE_LIBRARY_PATH);

		ethosu_delegate = TfLiteExternalDelegateCreate(&delegate_options);

		if (!ethosu_delegate) {
			fprintf(stderr, "Failed to create Ethos-U delegate\n");

			return EXIT_FAILURE;
		}

		unique_ptr<FlatBufferModel> flatbuffer_model;
		unique_ptr<Interpreter> interpreter;

		if (!set_interpreter(interpreter, flatbuffer_model, MODEL_PATH, *ethosu_delegate))
			return EXIT_FAILURE;

		if (interpreter->AllocateTensors() != kTfLiteOk)
			throw runtime_error("Failed to allocate tensors");

		set_input(interpreter, rgb_image);

		/* Load labels from file */
		labels = load_labels_file();

		if (labels.empty())
			throw runtime_error(string("Could not read labels file: ") + LABEL_FILE_PATH);

		/* Run inference for the specified number of runs  */
		bool dry_run_done = false;
		unsigned int completed_runs = 0;

		while (completed_runs < inference_runs) {
			auto start_time = chrono::steady_clock::now();

			/* Run inference */
			if (interpreter->Invoke() != kTfLiteOk)
				throw runtime_error("Failed to run inference");

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

			printf("Inference Run #%u, inference time: %fms\n", completed_runs, inference_time.count());

			total_inference_times += inference_time.count();
		}

		/* Calculate average inference time */
		average_inference_time = total_inference_times / inference_runs;

		/* Round average to 2 decimal places */
		average_inference_time = round(average_inference_time * 100.0f) / 100.0f;

		if (inference_runs == 1)
			inference_runs_text = "run";

		printf("Average Inference time: %gms over %u %s\n", average_inference_time, inference_runs, inference_runs_text.c_str());

		/* Post process the results */
		get_output(interpreter, labels, rgb_image, average_inference_time, inference_runs, use_display, display_name);
	} catch (const exception& e) {
		fprintf(stderr, "ERROR: %s\n", e.what());

		if (ethosu_delegate)
			TfLiteExternalDelegateDelete(ethosu_delegate);

		return EXIT_FAILURE;
	}

	if (ethosu_delegate)
		TfLiteExternalDelegateDelete(ethosu_delegate);

	return EXIT_SUCCESS;
}
