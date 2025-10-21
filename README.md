> ⚠️ **Early Stage / Work in Progress:** Prophet.Core is just getting started. Architecture may change, modules are minimal.

# Prophet.Core 
is a local, privacy-first daemon that serves as the central hub for modular personal automation. The Core itself does nothing until you attach modules, which define the tasks you want to monitor or get statistics from.

# Sample module "codetrackd" 
- communicates with the Core to check whether application like nvim or vim are open. After the request from module is created - daemon(the core) start its work. Based on the Core's response the module executes its logic, in this case is a simple time tracking while the editor is opened to provide your personal statistics.

# Why?
Prophet.Core is designed as a modular personal assistant. The Core itself doesn't do anything - its purpose is to coordinate modules that *provide useful insights or tools for your workflow.*

# Architecture
![Prophet.Core Architecture](architecture.png)

# Build & Run
Compile core
`g++ main.cpp -I../include -ldl -pthread -o ../build/prophet`

Compile module (codetrackd)
`g++ -fPIC -shared codetrackd.cpp -I../../include -o codetrackd.so`

Run
`cd build && ./prophet`
