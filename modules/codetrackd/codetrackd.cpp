#include <iostream>
#include <chrono>
#include <nlohmann/json.hpp>
#include "module_interface.hpp"

using json = nlohmann::json;

static std::chrono::steady_clock::time_point start_time;
static bool tracking = false;

extern "C" {

void module_init() {
    std::cout << "[codetrackd] initialized\n";
}

void module_run(const char* dependency_data_json) {
    auto data = json::parse(dependency_data_json);
    bool nvim_open = false;

    // core passes {"is_app_open":{"nvim":true,"vim":false}}
    if (data.contains("is_app_open") && data["is_app_open"].contains("nvim")) {
        nvim_open = data["is_app_open"]["nvim"].get<bool>();
    }

    if (nvim_open && !tracking) {
        start_time = std::chrono::steady_clock::now();
        tracking = true;
        std::cout << "[codetrackd] Neovim opened, tracking started\n";
    } else if (!nvim_open && tracking) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
        tracking = false;
        std::cout << "[codetrackd] Neovim closed, total time: " << duration << " seconds\n";
    }
}

void module_stop() {
    tracking = false;
    std::cout << "[codetrackd] stopped\n";
}

void module_cleanup() {
    std::cout << "[codetrackd] cleaned up\n";
}

const char* module_name() {
    return "codetrackd";
}

}
