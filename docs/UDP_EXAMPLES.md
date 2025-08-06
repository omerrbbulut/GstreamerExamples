# UDP Streaming Examples

## Overview

This document provides practical examples and usage scenarios for UDP streaming with GStreamer. The examples cover both unicast and multicast streaming with various configurations.

## Quick Start Examples

### 1. Basic Unicast Streaming

#### Sender (Terminal 1)
```bash
# Stream test pattern to localhost
./udp_unicast_sender 127.0.0.1 5000

# Stream to remote host
./udp_unicast_sender 192.168.1.100 5000

# Stream from camera to remote host
./udp_unicast_sender 192.168.1.100 5000 camera
```

#### Receiver (Terminal 2)
```bash
# Receive and display on screen
./udp_unicast_receiver 5000

# Receive and save to file
./udp_unicast_receiver 5000 output.mp4
```

### 2. Basic Multicast Streaming

#### Sender (Terminal 1)
```bash
# Default multicast group
./udp_multicast_sender

# Custom multicast group and TTL
./udp_multicast_sender 239.255.42.42 5001 10

# Camera source to multicast
./udp_multicast_sender 224.1.1.1 5000 5 camera
```

#### Receiver (Terminal 2+)
```bash
# Join default multicast group
./udp_multicast_receiver 224.1.1.1 5000

# Save multicast stream to file
./udp_multicast_receiver 224.1.1.1 5000 multicast_stream.mp4
```

## Advanced Examples

### 3. GStreamer Command Line Equivalents

#### Unicast Sender Pipeline
```bash
gst-launch-1.0 \
    videotestsrc pattern=ball ! \
    videoconvert ! \
    x264enc tune=zerolatency speed-preset=ultrafast bitrate=1000 key-int-max=30 bframes=0 ! \
    rtph264pay config-interval=1 ! \
    udpsink host=127.0.0.1 port=5000
```

#### Unicast Receiver Pipeline
```bash
gst-launch-1.0 \
    udpsrc port=5000 caps="application/x-rtp, payload=96" ! \
    rtph264depay ! \
    h264parse ! \
    avdec_h264 ! \
    videoconvert ! \
    autovideosink
```

#### Multicast Sender Pipeline
```bash
gst-launch-1.0 \
    videotestsrc pattern=ball ! \
    videoconvert ! \
    x264enc tune=zerolatency speed-preset=ultrafast bitrate=1000 ! \
    rtph264pay ! \
    multiudpsink clients="224.1.1.1:5000" ttl=5
```

#### Multicast Receiver Pipeline
```bash
gst-launch-1.0 \
    udpsrc multicast-group=224.1.1.1 port=5000 caps="application/x-rtp, payload=96" ! \
    rtph264depay ! \
    h264parse ! \
    avdec_h264 ! \
    videoconvert ! \
    autovideosink
```

### 4. Camera Streaming Examples

#### V4L2 Camera to UDP
```bash
# List available cameras
v4l2-ctl --list-devices

# Stream camera to UDP
./udp_unicast_sender 192.168.1.100 5000 camera

# Equivalent GStreamer pipeline
gst-launch-1.0 \
    v4l2src device=/dev/video0 ! \
    image/jpeg,width=640,height=480,framerate=30/1 ! \
    jpegdec ! \
    videoconvert ! \
    x264enc tune=zerolatency speed-preset=ultrafast ! \
    rtph264pay ! \
    udpsink host=192.168.1.100 port=5000
```

### 5. File Output Examples

#### Save UDP Stream to File
```bash
# H.264 file output
./udp_unicast_receiver 5000 output.h264

# MP4 container (requires additional muxing)
gst-launch-1.0 \
    udpsrc port=5000 caps="application/x-rtp, payload=96" ! \
    rtph264depay ! \
    h264parse ! \
    mp4mux ! \
    filesink location=output.mp4
```

### 6. Multiple Receivers (Multicast)

#### One Sender, Multiple Receivers
```bash
# Terminal 1 - Sender
./udp_multicast_sender 224.1.1.1 5000 5

# Terminal 2 - Receiver 1 (Display)
./udp_multicast_receiver 224.1.1.1 5000

# Terminal 3 - Receiver 2 (File)
./udp_multicast_receiver 224.1.1.1 5000 stream1.mp4

# Terminal 4 - Receiver 3 (Another display)
./udp_multicast_receiver 224.1.1.1 5000
```

## Network Configuration Examples

### 7. Cross-Network Streaming

#### Sender Configuration
```bash
# Check local IP
ip addr show

# Stream across networks
./udp_unicast_sender 10.0.1.100 5000

# Verify network route
ip route get 10.0.1.100
```

#### Firewall Configuration
```bash
# Allow UDP port (Ubuntu/Debian)
sudo ufw allow 5000/udp

# Allow UDP port (CentOS/RHEL)
sudo firewall-cmd --add-port=5000/udp --permanent
sudo firewall-cmd --reload
```

### 8. Performance Testing

#### Bandwidth Monitoring
```bash
# Monitor network usage during streaming
iftop -i eth0

# Monitor UDP traffic specifically
sudo netstat -su | grep -i udp

# Capture packets for analysis
sudo tcpdump -i any udp port 5000 -w capture.pcap
```

#### Latency Testing
```bash
# Enable detailed GStreamer debugging
GST_DEBUG=3 ./udp_unicast_sender 127.0.0.1 5000

# Monitor system resources
htop

# Check for dropped packets
cat /proc/net/udp
```

## Troubleshooting Examples

### 9. Common Issues and Solutions

#### Port Already in Use
```bash
# Check what's using the port
sudo netstat -ulnp | grep 5000

# Kill specific process
kill <PID>

# Kill all UDP examples
pkill -f udp_
```

#### No Video Output
```bash
# Test with simple GStreamer pipeline
gst-launch-1.0 videotestsrc ! autovideosink

# Check for missing plugins
gst-inspect-1.0 x264enc
gst-inspect-1.0 rtph264pay

# Test network connectivity
ping 127.0.0.1
telnet 127.0.0.1 5000
```

#### Poor Video Quality
```bash
# Increase bitrate
./udp_unicast_sender 127.0.0.1 5000
# Edit source to increase bitrate value

# Use better encoder preset (higher CPU usage)
# Modify speed-preset in source code:
# "speed-preset", 2,  // fast instead of ultrafast
```

### 10. Debug Mode Examples

#### Enable Debug Output
```bash
# Full GStreamer debug
GST_DEBUG=4 ./udp_unicast_sender 127.0.0.1 5000

# Filter specific components
GST_DEBUG=x264enc:5,rtph264pay:5 ./udp_unicast_sender 127.0.0.1 5000

# Network debugging
GST_DEBUG=udpsink:5,udpsrc:5 ./udp_unicast_receiver 5000
```

## Integration Examples

### 11. Scripted Streaming

#### Automated Test Script
```bash
#!/bin/bash
# start_streaming_test.sh

echo "Starting UDP streaming test..."

# Start sender in background
./udp_unicast_sender 127.0.0.1 5000 &
SENDER_PID=$!

# Wait for sender to initialize
sleep 2

# Start receiver for 10 seconds
timeout 10s ./udp_unicast_receiver 5000

# Cleanup
kill $SENDER_PID
echo "Test completed"
```

#### Service Integration
```bash
# systemd service example
# /etc/systemd/system/udp-stream.service

[Unit]
Description=UDP Video Streaming Service
After=network.target

[Service]
Type=simple
User=streaming
ExecStart=/usr/local/bin/udp_unicast_sender 224.1.1.1 5000
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### 12. Makefile Integration

#### Build and Test
```bash
# Build all UDP examples
make udp

# Quick test targets
make test-udp-unicast
make test-udp-multicast

# Demo mode
make demo-udp-unicast
make demo-udp-multicast
```

## Performance Benchmarks

### 13. Latency Measurements

#### End-to-End Latency Test
```bash
# Use timestamp overlay for visual latency measurement
gst-launch-1.0 \
    videotestsrc pattern=ball ! \
    timeoverlay ! \
    videoconvert ! \
    x264enc tune=zerolatency speed-preset=ultrafast ! \
    rtph264pay ! \
    udpsink host=127.0.0.1 port=5000
```

#### Throughput Testing
```bash
# High bitrate test
# Modify encoder settings for higher quality:
# bitrate=5000 (5Mbps)
# speed-preset=medium

# Monitor CPU usage during high bitrate streaming
top -p $(pgrep udp_unicast_sender)
```

These examples provide a comprehensive foundation for UDP streaming with GStreamer, covering basic usage through advanced configuration and troubleshooting scenarios. 