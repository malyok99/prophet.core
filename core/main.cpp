#include "modules.hpp"
#include "ipc_server.hpp"
#include <thread>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

int main() {
  std::filesystem::path modules_dir = "../modules";

  std::ifstream f("../modules/modules.json");
  if (f) {
    nlohmann::json j = nlohmann::json::parse(f);
    for (auto it = j.begin(); it != j.end(); ++it) {
      module_state[it.key()] = it.value();
    }
  }

  std::thread(ipc_server, modules_dir).detach();
  std::cout << "[prophet] Daemon started\n";

  std::unordered_map<std::string, std::filesystem::file_time_type> known_dirs;

  while (true) {
    // scan modules directory
    for (auto& dir : std::filesystem::directory_iterator(modules_dir)) {
      if (!std::filesystem::is_directory(dir)) continue;

      std::string mod_name = dir.path().filename().string();
      auto ftime = std::filesystem::last_write_time(dir);

      if (!loaded_modules.count(mod_name) && module_state[mod_name]) load_module(dir.path());
      known_dirs[mod_name] = ftime;
    }

    // detect removed modules
    std::vector<std::string> to_remove;
    for (auto& [mod_name, m] : loaded_modules) {
      if (!std::filesystem::exists(m.path)) to_remove.push_back(mod_name);
    }
    for (auto& mod_name : to_remove) unload_module(mod_name);

    // run all modules with restricted dependency context
    for (auto& [name, m] : loaded_modules) {
      ModuleContext ctx = build_module_context(m);
      m.run(ctx.dependency_data.dump().c_str());
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
  }

  return 0;
}
