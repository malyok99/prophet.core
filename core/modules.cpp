#include "modules.hpp"
#include "daemon_api.hpp"
#include <iostream>
#include <fstream>
#include <dlfcn.h>

std::unordered_map<std::string, Module> loaded_modules;

bool load_module(const std::filesystem::path& dir_path) {
  std::string so_path = dir_path.string() + "/" + dir_path.filename().string() + ".so";
  std::string json_path = dir_path.string() + "/module.json";
  if (!std::filesystem::exists(so_path) || !std::filesystem::exists(json_path)) return false;
  std::string mod_name = dir_path.filename().string();
  if (loaded_modules.count(mod_name)) return false;

  void* handle = dlopen(so_path.c_str(), RTLD_NOW);
  if (!handle) { std::cerr << "Failed to load " << so_path << ": " << dlerror() << "\n"; return false; }

  Module m{};
  m.handle = handle;
  m.init = (void(*)(const char*))dlsym(handle, "module_init");
  m.run  = (void(*)(const char*))dlsym(handle, "module_run");
  m.stop = (void(*)())dlsym(handle, "module_stop");
  m.cleanup = (void(*)())dlsym(handle, "module_cleanup");
  m.name = (const char*(*)())dlsym(handle, "module_name");

  if (!m.init || !m.run || !m.stop || !m.cleanup || !m.name) {
    std::cerr << "Invalid interface: " << so_path << "\n";
    dlclose(handle);
    return false;
  }

  std::ifstream jf(json_path);
  m.metadata = nlohmann::json::parse(jf);
  m.path = dir_path;

  m.init(m.metadata.dump().c_str());
  loaded_modules[mod_name] = m;
  std::cout << "[prophet] Loaded module: " << m.name() << "\n";
  return true;
}

void unload_module(const std::string& mod_name) {
  if (!loaded_modules.count(mod_name)) return;
  Module& m = loaded_modules[mod_name];
  m.stop();
  m.cleanup();
  dlclose(m.handle);
  loaded_modules.erase(mod_name);
  std::cout << "[prophet] Unloaded module: " << m.name() << "\n";
}

bool ModuleContext::has_dependency(const std::string& name) const {
  return dependency_data.contains(name);
}

nlohmann::json ModuleContext::get_dependency(const std::string& name) const {
  if (dependency_data.contains(name)) return dependency_data[name];
  return nlohmann::json{};
}

ModuleContext build_module_context(const Module& mod) {
  ModuleContext ctx;
  if (mod.metadata.contains("requires")) {
    for (auto& dep : mod.metadata["requires"]) {
      std::string dep_name = dep.get<std::string>();

      if (dep_name == "check_if_process_running" && mod.metadata.contains("additional")) {
        nlohmann::json apps;
        for (auto& app : mod.metadata["additional"]) {
          apps[app.get<std::string>()] = check_if_process_running(app.get<std::string>());
        }
        ctx.dependency_data[dep_name] = apps;
      }
      // future dependencies can be added here
    }
  }
  return ctx;
}
