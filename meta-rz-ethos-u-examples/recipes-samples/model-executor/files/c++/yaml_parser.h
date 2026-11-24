/*
 * Copyright (C) 2026 Renesas Electronics Corp.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef YAML_PARSER_H
#define YAML_PARSER_H

#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

using namespace std;

/*****************************************
 * Structs
 *****************************************/
struct Model {
	string name;
	string data_directory;
	string model_file;
	string input_file;
};

struct Executor {
	vector<Model> models;
};

/*****************************************
 * YamlParser Class
 *****************************************/
class YamlParser {
	private:
		Model parseModel(const YAML::Node& model_node);

	public:
		YamlParser();
		Executor parseYaml(const string& path);
};

#endif
