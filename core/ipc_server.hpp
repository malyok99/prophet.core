#pragma once
#include <unordered_map>
#include <string>
#include <filesystem>

extern std::unordered_map<std::string, bool> module_state;

void ipc_server(const std::filesystem::path& modules_dir);
