#pragma once
#include <string>

extern "C" {

// called when module is loaded
void module_init();

// called when module should start doing its work
// dependency_data_json contains data from core, e.g. {"is_app_open": {"nvim": true}}
void module_run(const char* dependency_data_json);

// called when module should stop
void module_stop();

// called when module is unloaded
void module_cleanup();

// optional: return module name
const char* module_name();

}

