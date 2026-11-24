/*
 * Copyright (C) 2026 Renesas Electronics Corp.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <stdexcept>

#include "yaml_parser.h"

YamlParser::YamlParser() {}

Executor YamlParser::parseYaml(const string& path) {
	Executor executor_config;

	YAML::Node root = YAML::LoadFile(path);

	const YAML::Node models = root["models"];

	/* Verify models definition is valid and is a sequence*/
	if (!models || !models.IsSequence())
		throw runtime_error("The models must be a sequence in the configuration YAML!");

	executor_config.models.reserve(models.size());

	for (const auto& m : models)
		executor_config.models.push_back(parseModel(m));

	return executor_config;
}

Model YamlParser::parseModel(const YAML::Node& model_node) {
	Model model;

	/* Verify model is a map */
	if (!model_node || !model_node.IsMap())
		throw runtime_error("The model entry must be a map in the configuration YAML!\n");

	/* Verify input keys */
	for (const char* key : {"name", "data_directory", "model_file", "input_file"}) {
		if (!model_node[key])
			throw runtime_error("Model is missing key (" + string(key) + ") in the configuration YAML");
	}

	model.name = model_node["name"].as<string>();
	model.data_directory = model_node["data_directory"].as<string>();
	model.model_file = model_node["model_file"].as<string>();
	model.input_file = model_node["input_file"].as<string>();

	return model;
}
