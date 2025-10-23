#pragma once
#include <string>

extern "C" {

// called when module is loaded, receives its own module.json as JSON string
void module_init(const char* module_config_json);

// called when module should start doing its work
// dependency_data_json contains only dependencies declared in module.json
void module_run(const char* dependency_data_json);

// called when module should stop
void module_stop();

// called when module is unloaded
void module_cleanup();

}

