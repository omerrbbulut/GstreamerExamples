# 🎥 GStreamer RTSP Examples

A comprehensive collection of GStreamer RTSP server and client examples with detailed documentation and performance optimization guides.

> **⚠️ Important Notice**: This project provides educational examples and general guidance. Performance benchmarks are approximate and based on typical configurations. Always test on your specific hardware and use case.

## 📋 Quick Start

### Prerequisites
- **GStreamer 1.0+** (with development headers)
- **GCC/Clang compiler** with C99 support
- **pkg-config** for dependency management
- **Linux system** (tested on Ubuntu 20.04+, other distributions may work)

### Installation
```bash
# Ubuntu/Debian
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-good1.0-dev libgstreamer-plugins-bad1.0-dev \
    libgstreamer-rtsp-server-1.0-dev

# Fedora/CentOS/RHEL
sudo dnf install gstreamer1-devel gstreamer1-plugins-base-devel \
    gstreamer1-plugins-good-devel gstreamer1-plugins-bad-devel \
    gstreamer1-rtsp-server-devel

# Arch Linux
sudo pacman -S gstreamer gst-plugins-base gst-plugins-good \
    gst-plugins-bad gst-rtsp-server
```

### Build and Run
```bash
git clone <repository-url>
cd GstreamerExamples
make all

# Terminal 1: Start RTSP server
./rtsp_server

# Terminal 2: Connect with client
./rtsp_client
```

## 🚀 Features

- **Simple RTSP Server**: Basic video streaming server implementation
- **RTSP Client**: Example client with error handling and reconnection
- **Low Latency Configuration**: Optimized pipelines for real-time applications
- **Comprehensive Documentation**: API reference, examples, and performance guides
- **Hardware Acceleration Support**: Examples for NVENC, QuickSync, and VCE
- **Cross-Platform**: Linux focus with adaptation guidelines for other platforms

## 📁 Project Structure

```
GstreamerExamples/
├── rtsp_server_example.c    # RTSP server implementation
├── rtsp_client_example.c    # RTSP client implementation  
├── Makefile                 # Build configuration
├── README.md               # This file
└── docs/
    ├── API.md              # API documentation
    ├── EXAMPLES.md         # Advanced usage examples
    └── PERFORMANCE.md      # Performance tuning guide
```

## 📖 Documentation

### Core Documentation
- **[API Reference](docs/API.md)**: Detailed GStreamer RTSP API documentation
- **[Examples Guide](docs/EXAMPLES.md)**: Advanced streaming scenarios and use cases  
- **[Performance Guide](docs/PERFORMANCE.md)**: Optimization techniques and benchmarks

### Quick Reference
```c
// Basic RTSP server pipeline
"videotestsrc ! x264enc ! rtph264pay name=pay0"

// Low-latency optimized pipeline  
"v4l2src ! jpegdec ! x264enc tune=zerolatency speed-preset=ultrafast ! rtph264pay"

// Hardware-accelerated encoding (NVIDIA)
"v4l2src ! videoconvert ! nvh264enc preset=low-latency-hp ! rtph264pay"
```

## ⚡ Performance Overview

> **Source Disclaimer**: Performance figures are approximate and based on:
> - GStreamer 1.0+ official documentation
> - Community benchmarks and real-world testing
> - Hardware vendor specifications (Intel, AMD, NVIDIA)
> - Academic research on video streaming latency

### Typical Performance Expectations*

| Hardware Class | Resolution | FPS | CPU Usage** | Estimated Latency*** |
|----------------|------------|-----|-------------|---------------------|
| **High-end Desktop****<br/>(i7/Ryzen 7+) | 1920x1080 | 60 | 25-35% | 50-100ms |
| **Mid-range Desktop****<br/>(i5/Ryzen 5+) | 1280x720 | 30 | 30-45% | 80-150ms |
| **Embedded Systems****<br/>(Raspberry Pi 4+) | 1280x720 | 30 | 60-80% | 120-200ms |

**\*Performance Disclaimers:**
- ****: Based on typical configurations with x264 ultrafast preset
- ****: CPU usage varies significantly with encoding settings and system load
- ****: Latency includes encoding + network + decoding (end-to-end)
- ****: Desktop figures assume modern processors (2020+)
- ****: Embedded figures based on Raspberry Pi Foundation benchmarks

### Latency Sources and Typical Ranges*

| Component | Typical Range | Optimization Impact |
|-----------|---------------|-------------------|
| Camera Capture | 16-33ms | Hardware dependent |
| Video Encoding | 50-200ms | **High** - tunable via encoder settings |
| Network Transport | 10-100ms | **Medium** - depends on protocol/buffering |
| Video Decoding | 20-50ms | **Low** - client hardware dependent |
| Display Rendering | 16-33ms | **Low** - V-sync dependent |

**\*Sources**: ITU-T recommendations, GStreamer documentation, community measurements

## 🛠️ Usage Examples

### Basic Server (Test Pattern)
```c
// Creates RTSP server streaming test pattern at rtsp://localhost:8554/test
// Pipeline: videotestsrc -> x264enc -> rtph264pay
#include "rtsp_server_example.c"
```

### Camera Server (USB Webcam)
```c
// Modify pipeline in rtsp_server_example.c:
const char* pipeline = 
    "v4l2src device=/dev/video0 ! "
    "image/jpeg,width=640,height=480,framerate=30/1 ! "
    "jpegdec ! videoconvert ! "
    "x264enc tune=zerolatency speed-preset=ultrafast bitrate=2000 ! "
    "rtph264pay name=pay0";
```

### Low-Latency Client Connection
```bash
# Using GStreamer client
gst-launch-1.0 rtspsrc location=rtsp://localhost:8554/test latency=50 ! \
    rtph264depay ! h264parse ! avdec_h264 ! autovideosink

# Using FFplay with low-latency flags
ffplay -fflags nobuffer -flags low_delay -framedrop rtsp://localhost:8554/test
```

## 🔧 Troubleshooting

### Common Issues

**Build Errors**
```bash
# Missing development headers
sudo apt install libgstreamer1.0-dev libgstreamer-rtsp-server-1.0-dev

# pkg-config not found
sudo apt install pkg-config
```

**Runtime Issues**
```bash
# Port already in use
netstat -tlnp | grep 8554
pkill -f rtsp_server

# GStreamer plugin missing
gst-inspect-1.0 x264enc  # Should show plugin info
```

**Performance Issues**
- Check CPU usage: `htop` or `top`
- Monitor network: `iftop` or `nethogs`  
- GStreamer debugging: `export GST_DEBUG=3`

### Debug Mode
```bash
# Enable detailed GStreamer logging
export GST_DEBUG=3
export GST_DEBUG_FILE=gstreamer.log
./rtsp_server

# Analyze performance
export GST_TRACERS=stats
gst-stats-1.0 stats.log
```

## 📚 Additional Resources

### Official Documentation
- **[GStreamer Documentation](https://gstreamer.freedesktop.org/documentation/)** - Official API and tutorial
- **[RTSP Server Library](https://gstreamer.freedesktop.org/modules/gst-rtsp-server.html)** - Server implementation guide
- **[GStreamer Plugins](https://gstreamer.freedesktop.org/documentation/plugins_doc.html)** - Plugin reference

### Community Resources  
- **[GStreamer Mailing List](https://lists.freedesktop.org/mailman/listinfo/gstreamer-devel)** - Developer discussions
- **[Stack Overflow: gstreamer](https://stackoverflow.com/questions/tagged/gstreamer)** - Community Q&A
- **[Reddit: r/gstreamer](https://www.reddit.com/r/gstreamer/)** - User discussions

### Performance and Optimization
- **[x264 Settings Guide](https://sites.google.com/site/linuxencoding/x264-ffmpeg-mapping)** - Encoder optimization
- **[Low Latency Streaming Research](https://scholar.google.com/scholar?q=low+latency+video+streaming)** - Academic papers
- **[Hardware Acceleration Guides](https://wiki.archlinux.org/title/Hardware_video_acceleration)** - Platform-specific optimization

## 🤝 Contributing

### Development Guidelines
- Follow existing code style and structure
- Test on multiple platforms when possible
- Document performance claims with sources
- Include proper error handling

### Reporting Issues
When reporting performance issues, please include:
- Hardware specifications (CPU, GPU, RAM)
- Operating system and GStreamer version
- Pipeline configuration used
- Measured performance metrics

### Benchmark Contributions
We welcome verified benchmarks! Please include:
- Hardware specifications
- Test methodology  
- Reproducible test scripts
- Measurement tools used

## ⚖️ License

This project is provided under the MIT License. See LICENSE file for details.

**Disclaimer**: This software is provided for educational and development purposes. Performance characteristics may vary significantly based on hardware, software configuration, and use case. Always validate performance in your specific environment.

## 🏷️ Version Information

- **Project Version**: 1.0.0
- **GStreamer Compatibility**: 1.0+
- **Last Updated**: 2024
- **Documentation Sources**: See individual doc files for specific citations

---

> **💡 Pro Tip**: Start with the basic examples and gradually add optimizations. The `docs/` directory contains detailed guides for advanced use cases and performance tuning.

**Questions or Issues?** Check the troubleshooting section above or refer to the official GStreamer documentation. 