/*
 * Copyright (C) 2026 Renesas Electronics Corp.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cmath>

#include "processing.h"
#include "wayland_utils.h"

vector<string> load_labels_file() {
	string labels_file_path = LABEL_FILE_PATH;
	vector<string> list = {};
	vector<string> empty = {};
	string line = "";

	ifstream labels_file(labels_file_path);

	if (!labels_file.is_open())
		return empty;

	/* Read labels from file and store into vector */
	while (getline(labels_file, line)) {
		list.push_back(line);

		if (labels_file.fail())
			return empty;
	}

	labels_file.close();

	return list;
}

cv::Mat load_image_file(string image_file_path) {
	cv::Mat image;

	/* Read image and convert to RGB */
	image = cv::imread(image_file_path);

	if (image.empty())
		return image;

	cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

	return image;
}

cv::Mat preprocess_image(cv::Mat image, int width, int height, ModelDataType data_type, float scale, double zero_point) {
	int interpolation_method = 0;

	/* Resize image only if needed */
	if (image.size() != cv::Size(width, height)) {
		if (image.cols > width || image.rows > height)
			interpolation_method = cv::INTER_AREA;
		else
			interpolation_method = cv::INTER_LINEAR;

		cv::resize(image, image, cv::Size(width, height), 0, 0, interpolation_method);
	}

	/* Float models do not require normalization */
	if (data_type == ModelDataType::FLOAT32) {
		image.convertTo(image, CV_32F, 2.0 / 255.0, -1.0);

		return image;
	}

	bool is_int8 = (data_type == ModelDataType::INT8);
	double min_value = is_int8 ? -128.0 : 0.0;
	double max_value = is_int8 ? 127.0 : 255.0;

	/* Calculate normalization values*/
	double real_min = (min_value - zero_point) * (double)scale;
	double norm_low = (real_min < 0) ? -1.0 : 0.0;
	double norm_range = 1.0 - norm_low;

	/* Normalize the pixels and quantize using models scale and zero_point */
	cv::Mat lut(1, 256, is_int8 ? CV_8SC1 : CV_8UC1);

	for (int pixel = 0; pixel < 256; pixel++) {
		double value = nearbyint((((pixel / 255.0) * norm_range + norm_low) / (double)scale) + zero_point);

		/* Clip to prevent values from not fitting boundary */
		value = min(max(value, min_value), max_value);

		if (is_int8)
			lut.at<int8_t>(pixel) = static_cast<int8_t>(value);
		else
			lut.at<uint8_t>(pixel) = static_cast<uint8_t>(value);
	}

	cv::LUT(image, lut, image);

	return image;
}

void draw_text(cv::Mat& image_to_draw_on, string text_to_write, int text_start_width, int text_start_height, float font_size, const cv::Scalar& text_colour) {
	cv::putText(image_to_draw_on, text_to_write, cv::Point(text_start_width, text_start_height), cv::FONT_HERSHEY_DUPLEX, font_size, text_colour, 1);
}

void post_process_output(int64_t output_size, const void* output_data_ptr, ModelDataType data_type, float scale, double zero_point, const vector<string>& labels, cv::Mat& image, float inference_time, int inference_runs, bool use_display, const string& display_name) {
	vector<pair<float, int>> results;
	pair<int, int> display_dims;;
	string selected_display;
	int display_width = 0, display_height = 0;
	int display_sections = 0;
	float font_size = 0.0f;
	cv::Mat display_window;
	int text_start_width;
	int detections = 0;
	stringstream inference_time_text;

	if (output_size < 5 || (size_t)output_size > labels.size())
		throw runtime_error("Output size (" + to_string(output_size) + ") does not match number of labels (" + to_string(labels.size()) + ")");

	/* Calculate confidence for each class in percentages, dequantizing with the output scale and zero point */
	for (int64_t i = 0; i < output_size; i++) {
		float value;

		if (data_type == ModelDataType::INT8)
			value = (static_cast<const int8_t*>(output_data_ptr)[i] - zero_point) * scale;
		else if (data_type == ModelDataType::UINT8)
			value = (static_cast<const uint8_t*>(output_data_ptr)[i] - zero_point) * scale;
		else
			value = static_cast<const float*>(output_data_ptr)[i];

		results.push_back(make_pair(value * 100, (int)i));
	}

	/* Speed up sorting by only sorting top 5 largest percentages */
	partial_sort(results.begin(), results.begin() + 5, results.end(), greater{});

	if (use_display) {
		/* Get display dimensions */
		display_dims = get_display_dims(display_name, selected_display);
		display_width = display_dims.first;
		display_height = display_dims.second;

		/*
		 * Set the font size and area for the image dependent on the resolution
		 * The display section refers to how much of the display the image will take
		 * E.G. A value of 3 means the image takes up 1/3rd of the display
		 */
		if (display_width == 0 && display_height == 0) {
			/* Switch to not using display if no display is detected */
			use_display = false;
		} else if (display_width == 1920 && display_height == 1080) {
			display_sections = 3;
			font_size = 0.85;
		} else if (display_width == 1280 && display_height == 720) {
			display_sections = 3;
			font_size = 0.55;
		} else if (display_width == 800 && display_height == 600) {
			display_sections = 4;
			font_size = 0.35;
		} else {
			printf("Unsupported display resolution: %dx%d\n", display_width, display_height);
			use_display = false;
		}
	}

	if (use_display) {
		double image_scale_factor;
		string inference_runs_text;

		printf("\nDisplaying top Image Classification results....\n");

		/*
		 * Resize the image to fit the allocated area on the display window
		 * whilst maintaining the original aspect ratio
		 */
		image_scale_factor = (float)(display_width / display_sections) / image.cols;

		if (image.rows * image_scale_factor > display_height) {
			image_scale_factor = (float)display_height / image.rows;
		}

		int interpolation_method = image_scale_factor > 1 ? cv::INTER_LINEAR : cv::INTER_AREA;

		cv::resize(image, image, cv::Size(), image_scale_factor, image_scale_factor, interpolation_method);

		/* Create the display window and copy the image data to the allocated area */
		display_window = cv::Mat(display_height, display_width, image.type(), cv::Scalar(0, 0, 0));
		cv::Rect roi(cv::Rect(0, 0, image.cols, image.rows));
		image.copyTo(display_window(roi));
		cv::cvtColor(display_window, display_window, cv::COLOR_BGR2RGBA);

		inference_time_text << fixed << setprecision(2) << inference_time;

		text_start_width = image.cols + TEXT_OFFSET;

		if (inference_runs == 1)
			inference_runs_text = " run";
		else
			inference_runs_text = " runs";

		/* Draw information-related text and inference time */
		draw_text(display_window, "Renesas MobileNet V2 Classifier Application", text_start_width, 30, font_size, YELLOW_TEXT);
		draw_text(display_window, "Model: MobileNet V2 (224x224)", text_start_width, 90, font_size, WHITE_TEXT);
		draw_text(display_window, "Accelerator: Ethos-U55 NPU", text_start_width, 120, font_size, WHITE_TEXT);
		draw_text(display_window, "Model Information", text_start_width, 180, font_size, YELLOW_TEXT);
		draw_text(display_window, "NPU Operators: 95", text_start_width, 210, font_size, WHITE_TEXT);
		draw_text(display_window, "CPU Operators: 0", text_start_width, 240, font_size, WHITE_TEXT);
		draw_text(display_window, "Average Inference Time: " + inference_time_text.str() + "ms over " + to_string(inference_runs) + inference_runs_text, text_start_width, 300, font_size, WHITE_TEXT);
		draw_text(display_window, "Identified Classifications: ", text_start_width, 360, font_size, YELLOW_TEXT);

	} else {
		printf("\nPrinting top Image Classification results....\n");
	}

	for (int i = 0; i < 5; i++) {
		string label;
		smatch match;
		float confidence;
		stringstream confidence_text;

		label = labels[results[i].second];

		/* For long labels, only display the first 3 items */
		if (regex_search(label, match, regex("([^,]*,[^,]*,[^,]*)")))
			label = match[1];

		confidence = results[i].first;
		confidence_text << fixed << setprecision(2) << confidence;

		if (confidence > 0) {
			/* Print results to terminal or draw on display window depending on the setting */
			if (use_display)
				draw_text(display_window, confidence_text.str() + "% -> " + label.c_str(), text_start_width, (i * 30) + 390, font_size, WHITE_TEXT);
			else
				printf("\n%.2f%% -> %s\n", confidence, label.c_str());

			detections++;
		}


		/* If there are no detections above 0% confidence, alert the user via display or terminal */
		if (i == 4 && detections == 0) {
			if (use_display)
				draw_text(display_window, "No classifications identified!", text_start_width, 390, font_size, WHITE_TEXT);
			else
				printf("\nNo classifications identified!\n");
		}
	}

	/* Show display window on HDMI display */
	if (use_display)
		display_results(display_width, display_height, display_window, selected_display);
}
