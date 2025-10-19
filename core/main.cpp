#include <iostream>
#include <filesystem>
#include <dlfcn.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "module_interface.hpp"

using namespace std;
namespace fs = std::filesystem;
using json = nlohmann::json;

const char* SOCKET_PATH = "/tmp/prophet_gui.sock";

// -------------------- module struct --------------------
struct Module {
  void* handle;
  void (*init)();
  void (*run)(const char*);
  void (*stop)();
  void (*cleanup)();
  const char* (*name)();
  json metadata;
  fs::path path;
};

unordered_map<string, Module> loaded_modules;

// true = enabled, false = disabled (GUI controlled)
unordered_map<string, bool> module_state;

// -------------------- dependencies --------------------
bool check_if_process_running(const string& name) {
  // demo random open/closed
  return rand() % 2;
}

json gather_dependencies(const Module& mod) {
  json j;
  if (mod.metadata.contains("requires")) {
    for (auto& dep : mod.metadata["requires"]) {
      if (dep == "is_app_open" && mod.metadata.contains("additional")) {
        json apps;
        for (auto& app : mod.metadata["additional"]) {
          apps[app.get<string>()] = check_if_process_running(app.get<string>());
        }
        j[dep] = apps;
      }
    }
  }
  return j;
}

// -------------------- load/unload --------------------
bool load_module(const fs::path& dir_path) {
  string so_path = dir_path.string() + "/" + dir_path.filename().string() + ".so";
  string json_path = dir_path.string() + "/module.json";
  if (!fs::exists(so_path) || !fs::exists(json_path)) return false;
  string mod_name = dir_path.filename().string();

  if (loaded_modules.count(mod_name)) return false; // already loaded

  void* handle = dlopen(so_path.c_str(), RTLD_NOW);
  if (!handle) { cerr << "Failed to load " << so_path << ": " << dlerror() << endl; return false; }

  Module m{};
  m.handle = handle;
  m.init = (void(*)())dlsym(handle, "module_init");
  m.run  = (void(*)(const char*))dlsym(handle, "module_run");
  m.stop = (void(*)())dlsym(handle, "module_stop");
  m.cleanup = (void(*)())dlsym(handle, "module_cleanup");
  m.name = (const char*(*)())dlsym(handle, "module_name");
  if (!m.init || !m.run || !m.stop || !m.cleanup || !m.name) {
    cerr << "Invalid interface: " << so_path << endl;
    dlclose(handle);
    return false;
  }

  ifstream jf(json_path);
  m.metadata = json::parse(jf);
  m.path = dir_path;

  m.init();
  loaded_modules[mod_name] = m;
  cout << "[prophet] Loaded module: " << m.name() << endl;
  return true;
}

void unload_module(const string& mod_name) {
  if (!loaded_modules.count(mod_name)) return;
  Module& m = loaded_modules[mod_name];
  m.stop();
  m.cleanup();
  dlclose(m.handle);
  cout << "[prophet] Unloaded module: " << m.name() << endl;
  loaded_modules.erase(mod_name);
}

// -------------------- IPC server --------------------
void ipc_server() {
  int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd < 0) { perror("socket"); return; }

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
  unlink(SOCKET_PATH);

  if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return; }
  if (listen(server_fd, 5) < 0) { perror("listen"); return; }

  while (true) {
    int client = accept(server_fd, nullptr, nullptr);
    if (client < 0) continue;

    char buf[256];
    int n = read(client, buf, sizeof(buf)-1);
    if (n > 0) {
      buf[n] = '\0';
      string msg(buf);
      auto space = msg.find(' ');
      if (space != string::npos) {
        string cmd = msg.substr(0, space);
        string mod_name = msg.substr(space + 1);

        if (cmd == "ENABLE") {
          module_state[mod_name] = true;
          if (!loaded_modules.count(mod_name)) {
            fs::path path = "../modules/" + mod_name;
            load_module(path);
          }
        } 
        else if (cmd == "DISABLE") {
          module_state[mod_name] = false;
          if (loaded_modules.count(mod_name)) {
            unload_module(mod_name);
          }
        }
      }
    }
    close(client);
  }
}

// -------------------- main loop --------------------
int main() {
  // load initial states from JSON
  ifstream f("../modules.json");
  if (f) {
    json j = json::parse(f);
    for (auto it = j.begin(); it != j.end(); ++it) {
      module_state[it.key()] = it.value();
    }
  }

  std::thread(ipc_server).detach();
  cout << "[prophet] Hot-module daemon started\n";

  unordered_map<string, fs::file_time_type> known_dirs;

  while (true) {
    // scan modules directory
    for (auto& dir : fs::directory_iterator("../modules")) {
      if (!fs::is_directory(dir)) continue;

      string mod_name = dir.path().filename().string();
      auto ftime = fs::last_write_time(dir);

      if (!loaded_modules.count(mod_name) && module_state[mod_name]) {
        load_module(dir.path());
      }

      known_dirs[mod_name] = ftime;
    }

    // detect removed modules
    vector<string> to_remove;
    for (auto& [mod_name, m] : loaded_modules) {
      if (!fs::exists(m.path)) to_remove.push_back(mod_name);
    }
    for (auto& mod_name : to_remove) unload_module(mod_name);

    // run all modules with dependency data
    for (auto& [name, m] : loaded_modules) {
      json dep_data = gather_dependencies(m);
      m.run(dep_data.dump().c_str());
    }

    this_thread::sleep_for(chrono::seconds(2));
  }

  return 0;
}
