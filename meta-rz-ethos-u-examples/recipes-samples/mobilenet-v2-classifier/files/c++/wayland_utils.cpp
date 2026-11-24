/*
 * Copyright (C) 2026 Renesas Electronics Corp.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "wayland_utils.h"

/* Wayland Output functions */
static void output_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y,
			    int32_t physical_width, int32_t physical_height, int32_t subpixel,
			    const char *make, const char *model, int32_t transform) {
	((wl_output_info*)data)->transform = transform;
}

static void output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
			int32_t width, int32_t height, int32_t refresh) {
	struct wl_output_info *info = (wl_output_info*)data;

	if (flags & WL_OUTPUT_MODE_CURRENT) {
		info->width = width;
		info->height = height;
	}
}

static void output_done(void *data, struct wl_output *wl_output) {}

static void output_scale(void *data, struct wl_output *wl_output, int32_t factor) {
	/* Guard against a bogus scale from the compositor, it is used as a divisor */
	((wl_output_info*)data)->scale = factor > 0 ? factor : 1;
}

static void output_name(void *data, struct wl_output *wl_output, const char *name) {
	((wl_output_info*)data)->name = name;
}

static void output_description(void *data, struct wl_output *wl_output, const char *description) {}

static const struct wl_output_listener output_listener = {
	.geometry = output_geometry,
	.mode = output_mode,
	.done = output_done,
	.scale = output_scale,
	.name = output_name,
	.description = output_description,
};

/* Wayland Registry functions */
static void registry_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) {
	struct wl_client_info *wl_cl_info = (struct wl_client_info*)data;

	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		wl_cl_info->wl_compositor = (wl_compositor*) wl_registry_bind(wl_registry, name, &wl_compositor_interface, 1);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		wl_cl_info->wl_shm = (wl_shm*) wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		wl_cl_info->xdg_wm_base = (xdg_wm_base*) wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1);
	} else if (strcmp(interface, wl_output_interface.name) == 0) {
		/* Version 4 is needed for the output name event */
		unique_ptr<wl_output_info> info(new wl_output_info());

		info->scale = 1;
		info->transform = WL_OUTPUT_TRANSFORM_NORMAL;
		info->wl_output = (wl_output*) wl_registry_bind(wl_registry, name, &wl_output_interface, min(version, 4u));
		wl_output_add_listener(info->wl_output, &output_listener, info.get());
		wl_cl_info->outputs.push_back(move(info));
	}
}

static void registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};

/* XDG Shell functions */
static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_ping,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
	struct wl_client_info *wl_cl_info = (wl_client_info*)data;

	xdg_surface_ack_configure(xdg_surface, serial);

	wl_cl_info->configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

/*
 * Bind the Wayland globals and wait until the details of every output have
 * been received. The first roundtrip binds the globals, the second one
 * receives the output events.
 */
static struct wl_display* connect_wayland(struct wl_client_info &wl_cl_info) {
	struct wl_display *wl_display = wl_display_connect(NULL);

	if (!wl_display)
		return NULL;

	struct wl_registry *wl_registry = wl_display_get_registry(wl_display);
	wl_registry_add_listener(wl_registry, &registry_listener, (void*)&wl_cl_info);

	wl_display_roundtrip(wl_display);
	wl_display_roundtrip(wl_display);

	return wl_display;
}

static void disconnect_wayland(struct wl_display *wl_display, struct wl_client_info &wl_cl_info) {
	for (auto &info : wl_cl_info.outputs) {
		if (wl_output_get_version(info->wl_output) >= WL_OUTPUT_RELEASE_SINCE_VERSION)
			wl_output_release(info->wl_output);
		else
			wl_output_destroy(info->wl_output);
	}

	wl_cl_info.outputs.clear();
	wl_display_disconnect(wl_display);
}

static const char* output_label(const struct wl_output_info *info) {
	return info->name.empty() ? "(unnamed)" : info->name.c_str();
}

static struct wl_output_info* find_output_by_name(struct wl_client_info &wl_cl_info, const string& name) {
	for (auto &info : wl_cl_info.outputs) {
		if (info->name == name)
			return info.get();
	}

	return NULL;
}

static void print_available_outputs(struct wl_client_info &wl_cl_info) {
	bool named = false;

	printf("Available displays:");
	for (auto &info : wl_cl_info.outputs) {
		printf(" %s", output_label(info.get()));
		named |= !info->name.empty();
	}
	printf("\n");

	if (!named)
		printf("The compositor does not report display names (wl_output version 4 is needed), "
		       "so a display can't be selected by name.\n");
}

/*
 * Select the output to use. If no name is given, use the default output if
 * it is present, otherwise the first output reported.
 */
static struct wl_output_info* select_output(struct wl_client_info &wl_cl_info, const string& display_name) {
	if (wl_cl_info.outputs.empty()) {
		printf("No displays found!\n");

		return NULL;
	}

	if (!display_name.empty()) {
		struct wl_output_info *info = find_output_by_name(wl_cl_info, display_name);

		if (!info) {
			printf("Display %s not found!\n", display_name.c_str());
			print_available_outputs(wl_cl_info);
		}

		return info;
	}

	struct wl_output_info *info = find_output_by_name(wl_cl_info, DEFAULT_DISPLAY_NAME);

	if (!info) {
		info = wl_cl_info.outputs.front().get();
		printf("Default display %s not found, using %s\n", DEFAULT_DISPLAY_NAME, output_label(info));
	}

	return info;
}

pair<int, int> get_display_dims(const string& display_name, string& selected_name) {
	struct wl_client_info wl_cl_info = {};

	printf("\n");

	struct wl_display *wl_display = connect_wayland(wl_cl_info);

	if (!wl_display) {
		printf("Can't connect to Wayland, outputting results to terminal only!\n");

		return {0, 0};
	}

	struct wl_output_info *info = select_output(wl_cl_info, display_name);

	if (!info) {
		printf("Outputting results to terminal only!\n");
		disconnect_wayland(wl_display, wl_cl_info);

		return {0, 0};
	}

	if (info->width <= 0 || info->height <= 0) {
		printf("Display %s has no current mode, outputting results to terminal only!\n", output_label(info));
		disconnect_wayland(wl_display, wl_cl_info);

		return {0, 0};
	}

	/* Convert the current mode to the logical size used by the compositor */
	int width = info->width / info->scale;
	int height = info->height / info->scale;

	if (info->transform & WL_OUTPUT_TRANSFORM_90)
		swap(width, height);

	printf("Using display %s: %d x %d\n", output_label(info), width, height);

	selected_name = info->name;

	disconnect_wayland(wl_display, wl_cl_info);

	return {width, height};
}

static int create_shm_file(size_t size) {
	char file_name[] = "/tmp/wayland-shm-XXXXXX";

	int fd = mkstemp(file_name);

	if (fd < 0)
		throw runtime_error("Failed to create temporary wayland shared memory file!");

	/* Unlink as file_name is no longer needed at this point */
	unlink(file_name);

	if (ftruncate(fd, size) < 0)
		throw runtime_error("Failed to truncate file!");

	return fd;
}

void display_results(int display_width, int display_height, cv::Mat& display_image, const string& display_name) {
	int stride;
	size_t size;
	bool wait_for_exit = true;

	struct wl_client_info wl_cl_info = {};
	struct wl_display *wl_display = connect_wayland(wl_cl_info);

	if (!wl_display)
		throw runtime_error("Failed to connect to wayland, please check if display is connected...");

	if (!wl_cl_info.wl_compositor)
		throw runtime_error("Failed to access wayland compositor!");
	else if (!wl_cl_info.wl_shm)
		throw runtime_error("Failed to access wayland shared memory!");

	/*
	 * Use the output get_display_dims() selected. An unnamed output can only
	 * have been selected as the first output reported.
	 */
	struct wl_output_info *output_info = display_name.empty() ?
		(wl_cl_info.outputs.empty() ? NULL : wl_cl_info.outputs.front().get()) :
		find_output_by_name(wl_cl_info, display_name);

	if (!output_info)
		throw runtime_error("Display " + (display_name.empty() ? string("(unnamed)") : display_name) + " is no longer connected!");

	/* Use XDG shell to create application window */
	xdg_wm_base_add_listener(wl_cl_info.xdg_wm_base, &xdg_wm_base_listener, NULL);

	struct wl_surface *wl_surface = wl_compositor_create_surface(wl_cl_info.wl_compositor);

	if (!wl_surface)
		throw runtime_error("Failed to create Wayland surface!");

	/* Calculate Wayland shared memory pool size of image to be displayed */
	stride = display_width * 4;
	size = display_height * stride;

	/* Create temporary shared memory file that will be used for the display */
	int fd = create_shm_file(size);

	/* Copy image data to the Wayland shared memory pool */
	uint8_t* pool_data = (uint8_t*) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memcpy(pool_data, display_image.data, size);
	struct wl_shm_pool *pool = wl_shm_create_pool(wl_cl_info.wl_shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, display_width, display_height, stride, WL_SHM_FORMAT_XRGB8888);

	/* Destory pool and close temporary file */
	wl_shm_pool_destroy(pool);
	close(fd);

	/* Create XDG shell surface and attach listener */
	struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(wl_cl_info.xdg_wm_base, wl_surface);
	xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, (void*)&wl_cl_info);

	/* Make XDG shell toplevel and fullscreen on the output the size was read from */
	struct xdg_toplevel *xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
	xdg_toplevel_set_fullscreen(xdg_toplevel, output_info->wl_output);

	/* Make sure the display is configured before displaying image */
	wl_surface_commit(wl_surface);

	/* Wait until display is finished configuring */
	while (!wl_cl_info.configured) {
		wl_display_dispatch(wl_display);
	}

	/* Display the image */
	wl_surface_attach(wl_surface, buffer, 0, 0);
	wl_surface_damage(wl_surface, 0, 0, display_width, display_height);
	wl_surface_commit(wl_surface);
	wl_display_dispatch(wl_display);

	printf("Please press the ENTER key to close the application\n");

	/* Keep image on display until user presses the enter key on serial terminal */
	while (wait_for_exit) {
		if (cin.get())
			wait_for_exit = false;
	}

	disconnect_wayland(wl_display, wl_cl_info);
}
