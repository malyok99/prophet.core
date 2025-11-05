#include "daemon_api.hpp"
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

bool check_if_process_running(const std::string& name) {
  for (const auto& entry : fs::directory_iterator("/proc")) {
    if (!entry.is_directory()) continue;

    std::string pid_str = entry.path().filename().string();

    if (!std::all_of(pid_str.begin(), pid_str.end(), ::isdigit)) continue;

    std::ifstream comm_file(entry.path() / "comm");
    if (!comm_file.is_open()) continue;

    std::string process_name;
    std::getline(comm_file, process_name);
    if (process_name == name) return true;
  }
  return false;
}
