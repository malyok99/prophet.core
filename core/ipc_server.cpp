#include "ipc_server.hpp"
#include "modules.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>

const char* SOCKET_PATH = "/tmp/prophet_gui.sock";

std::unordered_map<std::string, bool> module_state;

void ipc_server(const std::filesystem::path& modules_dir) {
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
      std::string msg(buf);
      auto space = msg.find(' ');
      if (space != std::string::npos) {
        std::string cmd = msg.substr(0, space);
        std::string mod_name = msg.substr(space + 1);

        if (cmd == "ENABLE") {
          module_state[mod_name] = true;
          if (!loaded_modules.count(mod_name)) load_module(modules_dir / mod_name);
        } else if (cmd == "DISABLE") {
          module_state[mod_name] = false;
          if (loaded_modules.count(mod_name)) unload_module(mod_name);
        }
      }
    }
    close(client);
  }
}
