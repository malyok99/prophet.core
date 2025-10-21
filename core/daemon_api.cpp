#include "daemon_api.hpp"
#include <cstdlib>

bool check_if_process_running(const std::string& name) {
  // demo random open/closed
  return std::rand() % 2;
}
