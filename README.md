# CommDebugTool

**CommDebugTool** is a high-performance, multi-transport communication debugging utility built with **C++17** and **Qt 6**. It provides a robust interface for monitoring and interacting with Serial, TCP, and UDP communication protocols.

---

### 🚀 Key Features
- **Multi-Transport Support**: seamless debugging across **Serial Port**, **TCP Client**, and **UDP Socket**.
- **Ring Buffer Architecture**: Implements a 8MB high-speed ring buffer for stable data handling under high-throughput scenarios.
- **Worker-Thread Isolation**: Decouples UI logic from communication logic using a dedicated worker thread, ensuring a lag-free experience.
- **Advanced Logging**: Thread-safe logging system with real-time Hex/ASCII display modes and disk persistence.
- **Dark Mode UI**: Professional dark-themed interface optimized for long debugging sessions.
- **Cross-Platform**: Built with CMake, supporting macOS, Linux, and Windows.

### 🛠️ Technology Stack
- **Language**: Modern C++17
- **Framework**: Qt 6 (compatible with Qt 5)
- **Build System**: CMake 3.16+
- **Architecture**: Controller-Worker (Multi-threaded)

### 📂 Project Structure
```text
CommDebugTool/
├── include/CommDebugTool/  # Public API and Class Headers
├── src/                    # Source implementations
├── ui/                     # Qt Designer .ui forms
├── .github/workflows/      # CI/CD Automation
└── CMakeLists.txt          # Modern Build Configuration
```

### 🏗️ Build Instructions
1. Ensure **Qt 6.x** and **CMake** are installed.
2. Run the following commands:
   ```bash
   mkdir build && cd build
   cmake ..
   make -j
   ```

### ⚖️ License
This project is licensed under the **MIT License**.
