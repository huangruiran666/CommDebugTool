# CommDebugTool

[![CI](https://github.com/huangruiran666/CommDebugTool/actions/workflows/ci.yml/badge.svg)](https://github.com/huangruiran666/CommDebugTool/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**CommDebugTool** is a high-performance, multi-transport communication debugger designed for developers working with embedded systems, IoT, and network protocols. It provides a robust and intuitive interface for monitoring and interacting with Serial, TCP, and UDP data streams.

## Key Features

- **Multi-Transport Support**: Seamlessly switch between Serial (RS-232/485), TCP (Client), and UDP modes.
- **High-Performance Architecture**: Utilizes a **Ring Buffer** system to handle high-bandwidth data streams without losing packets or blocking the UI.
- **Modern UI/UX**: Features a sleek, dark-themed interface built for long debugging sessions.
- **Advanced Logging**: Real-time logging to files with configurable paths and integrated log management.
- **Flexible Display**: Support for Hex, ASCII, and hybrid "Both" display modes.
- **Industrial Stability**: Dedicated communication worker threads ensure the UI remains responsive even under heavy load.

## Technology Stack

- **Language**: C++17
- **Framework**: Qt 5.15+ / Qt 6
- **Build System**: CMake 3.16+
- **Architecture**: Multi-threaded, Asynchronous I/O

## Project Structure

```text
.
├── CMakeLists.txt              # Project configuration
├── include/
│   └── CommDebugTool/          # Public headers
│       ├── CommunicationWorker.h
│       ├── MainWindow.h
│       └── ProtocolHandler.h
├── src/                        # Implementation files
│   ├── CommunicationWorker.cpp
│   ├── MainWindow.cpp
│   └── main.cpp
└── ui/                         # Qt Designer UI files
    └── MainWindow.ui
```

## Build Instructions

### Prerequisites

- Qt 6.x (or Qt 5.15)
- CMake 3.16 or higher
- A C++17 compatible compiler (Clang, GCC, or MSVC)

### macOS / Linux

```bash
# Clone the repository
git clone https://github.com/huangruiran666/CommDebugTool.git
cd CommDebugTool

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
