#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <string>
#include <map>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>
#include "module_interface.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static std::map<std::string, double> category_minutes = {
  {"Coding", 0.0},
  {"Browsing", 0.0},
  {"Chat", 0.0},
  {"Media", 0.0},
  {"Misc", 0.0}
};

static std::string module_data_path;

static std::string categorize_window(const std::string &window) {
  if (window.find("tmux") != std::string::npos || window.find("nvim") != std::string::npos
      || window.find("@") != std::string::npos) return "Coding"; // '@' can be found in terminal window name "user@pc_name: etc."
  if (window.find("Firefox") != std::string::npos) return "Browsing";
  if (window.find("discord") != std::string::npos) return "Chat";
  if (window.find("spotify") != std::string::npos) return "Media";
  return "Misc";
}

static std::string get_active_window() {
  FILE* pipe = popen("xdotool getactivewindow getwindowname 2>/dev/null", "r");
  if (!pipe) return "unknown";
  char buffer[256];
  std::string result;
  while (fgets(buffer, sizeof(buffer), pipe)) result += buffer;
  pclose(pipe);
  if (!result.empty() && result.back() == '\n') result.pop_back();
  return result.empty() ? "unknown" : result;
}

static bool is_user_active(int idle_threshold_ms = 60000) {
  Display* dpy = XOpenDisplay(nullptr);
  if (!dpy) return true;
  XScreenSaverInfo *info = XScreenSaverAllocInfo();
  if (!info) { XCloseDisplay(dpy); return true; }
  if (!XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), info)) {
    XFree(info); XCloseDisplay(dpy); return true;
  }
  bool active = (info->idle < idle_threshold_ms);
  XFree(info); XCloseDisplay(dpy);
  return active;
}

static void save_day_json() {
  auto t = std::time(nullptr);
  std::tm tm{};
  localtime_r(&t, &tm);

  char date_buf[11];
  std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tm);

  json j;
  j["date"] = date_buf;
  j["categories"] = json::object();
  for (auto &p : category_minutes)
    j["categories"][p.first] = int(p.second + 0.5);

  std::ofstream out(module_data_path);
  out << std::setw(4) << j << std::endl;
}

static std::chrono::steady_clock::time_point last_tick = std::chrono::steady_clock::now();

extern "C" {

  void module_init(const char* module_config_json) {
    std::cout << "[flowtracker] App Heatmap initialized\n";
    last_tick = std::chrono::steady_clock::now();

    Dl_info info;
    dladdr((void*)module_init, &info);
    std::string so_path = info.dli_fname;

    auto dir_end = so_path.find_last_of('/');
    std::string module_dir = so_path.substr(0, dir_end);

    std::string data_dir = module_dir + "/data";
    mkdir(data_dir.c_str(), 0755);

    module_data_path = data_dir + "/data.json";

    std::cout << "[flowtracker] Saving to: " << module_data_path << "\n";
  }

  void module_run(const char* dependency_data_json) {
    auto now = std::chrono::steady_clock::now();
    double delta = std::chrono::duration_cast<std::chrono::seconds>(now - last_tick).count() / 60.0;
    last_tick = now;
    if (delta < 0.001) return;

    if (is_user_active()) {
      std::string win = get_active_window();
      std::string cat = categorize_window(win);
      category_minutes[cat] += delta;
      std::cout << "[flowtracker] +" << delta << " min → " << cat << "\n";
    }

    save_day_json();
  }

  void module_stop() {
    save_day_json();
    std::cout << "[flowtracker] stopped\n";
  }

  void module_cleanup() {
    save_day_json();
    std::cout << "[flowtracker] cleaned up\n";
  }

}
