# M5PixelFlow - WiFi Animation and Image Display

**M5PixelFlow** is a project for M5Stack devices that allows you to display and manage JPG images stored on an SPIFFS file system. It creates a WiFi access point and a web server for remote management of images, including uploading, deleting, and setting display intervals.

## Features
- **GIF capabilities**: You can use it to render GIF by using a 50-150ms delay between frames. The Python script help to transform GIF to JPG frames.
- **WiFi Access Point**: Automatically creates an access point named `PixelFlow` when A is pressed.
- **Image Display**: Cycles through images stored in the SPIFFS file system at a configurable interval.
- **Web Management**: Manage images through a built-in web interface:
  - Upload new images.
  - Delete individual or all images.
  - Configure the image display interval.
- **Automatic Configuration**: Saves and loads configuration settings from a JSON file.
- **Responsive Web UI**: Provides an easy-to-use interface optimized for various devices.

## Prerequisites

- **Hardware**:
  - M5Stack device (e.g., AtomS3, Cardputer, M5Stack Core2 or Core Basic).
  - MicroSD card for initial setup (optional).

- **Software**:
  - [Arduino IDE](https://www.arduino.cc/en/software).
  - M5Stack libraries ([M5Unified](https://github.com/m5stack/M5Unified)).
  - Additional Arduino libraries:
    - `WiFi`
    - `WebServer`
    - `SPIFFS`
    - `ArduinoJson`

