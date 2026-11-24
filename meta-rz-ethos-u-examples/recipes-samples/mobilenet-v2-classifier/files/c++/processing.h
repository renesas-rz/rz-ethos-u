/*
 * Copyright (C) 2026 Renesas Electronics Corp.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef PROCESSING_H
#define PROCESSING_H

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <regex>

#include <getopt.h>
#include <opencv4/opencv2/core/core.hpp>
#include <opencv4/opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv4/opencv2/imgproc/imgproc.hpp>

#define LABEL_FILE_PATH "/usr/share/models/mobilenet_v2/labels_mobilenet_v2_1.0_224.txt"
#define MODEL_INPUT_SIZE 224
#define TEXT_OFFSET 20

using namespace std;

const cv::Scalar WHITE_TEXT(255, 255, 255);
const cv::Scalar YELLOW_TEXT(0, 255, 255);

enum class ModelDataType {
	INT8,
	UINT8,
	FLOAT32
};

/*****************************************
* Functions
******************************************/
vector<string> load_labels_file();
cv::Mat load_image_file(string image_file_path);
cv::Mat preprocess_image(cv::Mat image, int width, int height, ModelDataType data_type, float scale, double zero_point);
void post_process_output(int64_t output_size, const void* output_data_ptr, ModelDataType data_type, float scale, double zero_point, const vector<string>& labels, cv::Mat& image, float inference_time, int inference_runs, bool use_display, const string& display_name);

#endif
