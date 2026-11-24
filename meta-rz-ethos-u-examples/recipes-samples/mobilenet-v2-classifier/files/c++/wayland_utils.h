/*
 * Copyright (C) 2026 Renesas Electronics Corp.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef WAYLAND_UTILS_H
#define WAYLAND_UTILS_H

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <opencv4/opencv2/core/core.hpp>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xdg-shell-client-protocol.h>

using namespace std;

#define DEFAULT_DISPLAY_NAME "HDMI-A-1"

struct wl_output_info {
	struct wl_output *wl_output;
	string name;
	int32_t width;
	int32_t height;
	int32_t scale;
	int32_t transform;
};

struct wl_client_info {
	struct wl_compositor *wl_compositor;
	struct wl_shm *wl_shm;
	struct xdg_wm_base *xdg_wm_base;
	vector<unique_ptr<wl_output_info>> outputs;
	bool configured;
};

/*****************************************
* Functions
******************************************/
pair<int, int> get_display_dims(const string& display_name, string& selected_name);
void display_results(int display_width, int display_height, cv::Mat& display_image, const string& display_name);

#endif
