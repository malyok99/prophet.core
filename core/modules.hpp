#pragma once
#include <string>
#include <unordered_map>
#include <filesystem>
#include <nlohmann/json.hpp>

struct Module {
  void* handle;
  void (*init)(const char*);
  void (*run)(const char*);
  void (*stop)();
  void (*cleanup)();
  const char* (*name)();
  nlohmann::json metadata;
  std::filesystem::path path;
};

struct ModuleContext {
  nlohmann::json dependency_data;

  bool has_dependency(const std::string& name) const;
  nlohmann::json get_dependency(const std::string& name) const;
};

extern std::unordered_map<std::string, Module> loaded_modules;

bool load_module(const std::filesystem::path& dir_path);
void unload_module(const std::string& mod_name);
ModuleContext build_module_context(const Module& mod);
