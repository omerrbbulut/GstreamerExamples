# 📡 GStreamer UDP Streaming Examples

A comprehensive collection of GStreamer UDP unicast and multicast streaming examples with detailed documentation and optimized configurations.

> **⚠️ Important Notice**: This project provides educational examples and reference implementations. Performance characteristics may vary based on hardware, network configuration, and use case. Always test in your specific environment.

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
    libgstreamer-plugins-good1.0-dev libgstreamer-plugins-bad1.0-dev

# Fedora/CentOS/RHEL
sudo dnf install gstreamer1-devel gstreamer1-plugins-base-devel \
    gstreamer1-plugins-good-devel gstreamer1-plugins-bad-devel

# Arch Linux
sudo pacman -S gstreamer gst-plugins-base gst-plugins-good \
    gst-plugins-bad
```

### Build and Run
```bash
git clone <repository-url>
cd GstreamerExamples
git checkout udp-streaming

# Build all examples
make all

# UDP Unicast Demo
# Terminal 1: Start sender
./udp_unicast_sender

# Terminal 2: Start receiver
./udp_unicast_receiver

# UDP Multicast Demo
# Terminal 1: Start multicast sender
./udp_multicast_sender

# Terminal 2+: Start receivers (multiple receivers can join)
./udp_multicast_receiver
```

## 🚀 Features

### 📡 UDP Unicast Streaming
- **Point-to-point video streaming** over UDP
- **Camera or test pattern input** with automatic fallback
- **Low-latency configuration** optimized for real-time applications
- **File recording capability** for receiver
- **Configurable resolution and encoding parameters**

### 🌐 UDP Multicast Streaming
- **One-to-many video streaming** with multicast groups
- **Multiple simultaneous receivers** can join the same stream
- **TTL (Time To Live) control** for network hop management
- **Network interface selection** for multi-homed systems
- **Multicast address validation** and automatic configuration

### 🛠️ Build System
- **Comprehensive Makefile** with multiple targets
- **Automatic dependency checking** and installation helpers
- **Individual and grouped build targets**
- **Automated testing** for basic functionality
- **Demo modes** for easy quick start

## 📁 Project Structure

```
GstreamerExamples/
├── udp_unicast_sender.c       # UDP unicast video sender
├── udp_unicast_receiver.c     # UDP unicast video receiver
├── udp_multicast_sender.c     # UDP multicast video sender
├── udp_multicast_receiver.c   # UDP multicast video receiver
├── Makefile                   # Comprehensive build system
└── README.md                  # This file
```

## 📖 Usage Examples

### 🎯 UDP Unicast Examples

**Basic Unicast (Test Pattern)**
```bash
# Send test pattern to localhost:5000
./udp_unicast_sender

# Receive and display
./udp_unicast_receiver
```

**Camera Streaming**
```bash
# Send camera video to remote host
./udp_unicast_sender -c 192.168.1.100 5001

# Receive on remote host
./udp_unicast_receiver 5001
```

**Recording Stream**
```bash
# Receive and save to file
./udp_unicast_receiver -f recording.mp4 5000
```

### 🌐 UDP Multicast Examples

**Basic Multicast**
```bash
# Start multicast sender
./udp_multicast_sender

# Multiple receivers can join
./udp_multicast_receiver
./udp_multicast_receiver  # Second receiver
./udp_multicast_receiver  # Third receiver
```

**Custom Multicast Group**
```bash
# Send to custom multicast group with TTL
./udp_multicast_sender 224.2.2.2 5001 10

# Join custom group
./udp_multicast_receiver 224.2.2.2 5001
```

**Network Interface Selection**
```bash
# Send via specific interface
./udp_multicast_sender -c 224.1.1.1 5000

# Receive via specific interface
./udp_multicast_receiver 224.1.1.1 5000 eth0
```

## ⚡ Performance Overview

> **Source Disclaimer**: Performance figures are approximate and based on:
> - GStreamer 1.0+ official documentation
> - Community benchmarks and real-world testing
> - Network performance analysis and measurement

### UDP vs RTSP Performance Comparison*

| Protocol | Latency | CPU Usage** | Bandwidth Efficiency | Multi-receiver |
|----------|---------|-------------|---------------------|----------------|
| **UDP Unicast** | 20-50ms | Low | High (no protocol overhead) | No (1:1) |
| **UDP Multicast** | 20-50ms | Low | Very High (1:many) | Yes (1:many) |
| **RTSP/TCP** | 50-200ms | Medium | Medium (protocol overhead) | Yes (multiple sessions) |
| **RTSP/UDP** | 30-100ms | Medium | High (RTP over UDP) | Yes (multiple sessions) |

**\*Performance Disclaimers:**
- ****: Latency includes encoding + network + decoding (end-to-end)
- ****: CPU usage varies with encoding settings and resolution

### Latency Optimization Features

**Encoder Settings**
- `tune=zerolatency` - Disable B-frames and minimize encoding delay
- `speed-preset=ultrafast` - Fastest encoding algorithm
- `key-int-max=30` - Frequent keyframes for faster startup
- `bframes=0` - No B-frames for minimal delay

**Network Settings**
- `sync=false, async=false` - Disable pipeline synchronization
- `mtu=1200` - Optimal packet size for multicast
- TTL control for multicast hop management

**Pipeline Optimization**
- Live source mode with `is-live=true`
- Direct RTP payloading without buffering
- Optimized caps negotiation

## 🔧 Build Targets

### Quick Reference
```bash
make all          # Build all examples
make udp          # Build all UDP examples
make udp-unicast  # Build unicast examples only
make udp-multicast # Build multicast examples only
make test         # Run automated tests
make help         # Show all available targets
```

### Demo Targets
```bash
make demo-udp-unicast    # Interactive unicast demo
make demo-udp-multicast  # Interactive multicast demo
```

## 🛠️ Troubleshooting

### Common Issues

**Build Errors**
```bash
# Missing development headers
sudo apt install libgstreamer1.0-dev

# Check dependencies
make deps
```

**Network Issues**
```bash
# Firewall blocking UDP traffic
sudo ufw allow 5000/udp

# Check if port is available
netstat -ulnp | grep 5000
```

**Multicast Issues**
```bash
# Check multicast routes
ip route show | grep 224

# Enable multicast on interface
sudo ip link set dev eth0 multicast on

# Monitor multicast traffic
tcpdump -i eth0 host 224.1.1.1
```

**Performance Issues**
- Check CPU usage: `htop` or `top`
- Monitor network: `iftop` or `nethogs`
- Enable GStreamer debugging: `export GST_DEBUG=3`

### Debug Mode
```bash
# Enable detailed GStreamer logging
export GST_DEBUG=3
export GST_DEBUG_FILE=gstreamer.log
./udp_unicast_sender

# Analyze logs
grep ERROR gstreamer.log
```

## 🌐 Network Configuration

### Unicast Guidelines
- **Use standard IP addresses** (not multicast range)
- **Ensure routing** between sender and receiver
- **Configure firewall** to allow UDP traffic
- **Consider NAT traversal** for cross-network streaming

### Multicast Guidelines
- **Use multicast range**: 224.0.0.0 - 239.255.255.255
- **Local network**: 224.0.0.0 - 224.0.0.255 (TTL 1)
- **Site-local**: 239.255.0.0 - 239.255.255.255 (TTL 1-15)
- **Global**: 224.0.1.0 - 238.255.255.255 (TTL > 15)

### Network Interface Commands
```bash
# List network interfaces
ip addr show

# Check multicast membership
cat /proc/net/igmp

# Monitor multicast traffic
tcpdump -i any multicast

# Test multicast connectivity
ping 224.1.1.1
```

## 🤝 Contributing

### Development Guidelines
- Follow existing code style and structure
- Test on multiple platforms when possible
- Document network configuration requirements
- Include proper error handling

### Reporting Issues
When reporting network issues, please include:
- Network topology and configuration
- Operating system and GStreamer version
- Firewall and routing configuration
- Error messages and debug logs

### Testing Guidelines
- Test both unicast and multicast scenarios
- Verify multi-receiver functionality for multicast
- Test across different network interfaces
- Validate performance in your network environment

## ⚖️ License

This project is provided under the MIT License. See LICENSE file for details.

**Disclaimer**: This software is provided for educational and development purposes. Network performance may vary significantly based on infrastructure, configuration, and traffic conditions. Always validate in your specific environment.

## 🏷️ Version Information

- **Project Version**: 1.0.0
- **GStreamer Compatibility**: 1.0+
- **Branch**: udp-streaming
- **Last Updated**: 2024

---

> **💡 Pro Tip**: Start with unicast examples to understand the basics, then explore multicast for one-to-many scenarios. Use the built-in help (`--help`) for detailed options on each executable.

**Questions or Issues?** Check the troubleshooting section above or test with simple GStreamer command-line pipelines first.
