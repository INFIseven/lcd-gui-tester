# LCD GUI Tester

A Qt6-based application for testing LCD GUIs with LVGL script support and Python integration.

## Features

- Image preview and drag-and-drop functionality
- LVGL script execution
- Embedded Python support
- Library dependency checking

## Requirements

- Qt6 (Core, Widgets, Gui, Network)
- CMake 3.20+
- C++17 compiler

### Firmware Flashing Requirements

For flashing firmware to the nRF52 device, you need:

- **nrfutil 7.x or later** (the new Rust-based version)

  **Important:** The old Python-based nrfutil (version 5.x) is NOT compatible. The application requires the newer version which supports the `device` command.

  To check your nrfutil version:
  ```bash
  nrfutil version
  ```

  Expected output should show version 7.0.0 or higher:
  ```
  nrfutil 7.x.x
  ```

  To download and install the correct version:
  - Visit: https://www.nordicsemi.com/Products/Development-tools/nRF-Util
  - **Windows**: Use winget to install:
    ```bash
    winget install nrfutil
    ```
  - **Linux x64**: Download directly:
    ```bash
    wget -O nrfutil https://developer.nordicsemi.com/.pc-tools/nrfutil/x64-linux/nrfutil
    chmod +x nrfutil
    sudo mv nrfutil /usr/local/bin/
    ```

  If you have the old Python version installed, uninstall it first:
  ```bash
  pip uninstall nrfutil
  ```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

Run the executable to open the main window interface for LCD GUI testing.

## Test deployement  

Use `act` for test deployement with  

``` bash
act --artifact-server-path /tmp/artifacts
```
