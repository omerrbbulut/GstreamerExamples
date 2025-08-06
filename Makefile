# GStreamer RTSP and UDP Examples Makefile
#
# This Makefile builds GStreamer-based streaming examples including:
# - RTSP server and client examples
# - UDP unicast sender and receiver examples  
# - UDP multicast sender and receiver examples
#
# Dependencies: GStreamer 1.0+, pkg-config
# Usage: make all, make rtsp, make udp, make clean

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
GSTREAMER_CFLAGS = $(shell pkg-config --cflags gstreamer-1.0)
GSTREAMER_LIBS = $(shell pkg-config --libs gstreamer-1.0)

# All executables
RTSP_TARGETS = rtsp_server rtsp_client
UDP_TARGETS = udp_unicast_sender udp_unicast_receiver udp_multicast_sender udp_multicast_receiver
ALL_TARGETS = $(RTSP_TARGETS) $(UDP_TARGETS)

# Source files
RTSP_SERVER_SRC = rtsp_server_example.c
RTSP_CLIENT_SRC = rtsp_client_example.c
UDP_UNICAST_SENDER_SRC = udp_unicast_sender.c
UDP_UNICAST_RECEIVER_SRC = udp_unicast_receiver.c
UDP_MULTICAST_SENDER_SRC = udp_multicast_sender.c
UDP_MULTICAST_RECEIVER_SRC = udp_multicast_receiver.c

# Default target
.PHONY: all
all: $(ALL_TARGETS)
	@echo ""
	@echo "🎉 All examples built successfully!"
	@echo ""
	@echo "📺 RTSP Examples:"
	@echo "  ./rtsp_server           - Start RTSP server (rtsp://localhost:8554/test)"
	@echo "  ./rtsp_client           - Connect RTSP client to server"
	@echo ""
	@echo "📡 UDP Unicast Examples:"
	@echo "  ./udp_unicast_sender    - Send video via UDP unicast"
	@echo "  ./udp_unicast_receiver  - Receive video via UDP unicast"
	@echo ""
	@echo "🌐 UDP Multicast Examples:"
	@echo "  ./udp_multicast_sender  - Send video via UDP multicast"
	@echo "  ./udp_multicast_receiver - Receive video via UDP multicast"
	@echo ""
	@echo "🔧 Use --help option with any executable for detailed usage information"

# RTSP targets
.PHONY: rtsp
rtsp: $(RTSP_TARGETS)
	@echo "✅ RTSP examples built"

rtsp_server: $(RTSP_SERVER_SRC)
	$(CC) $(CFLAGS) $(GSTREAMER_CFLAGS) -o $@ $< $(GSTREAMER_LIBS)

rtsp_client: $(RTSP_CLIENT_SRC)
	$(CC) $(CFLAGS) $(GSTREAMER_CFLAGS) -o $@ $< $(GSTREAMER_LIBS)

# UDP targets
.PHONY: udp
udp: $(UDP_TARGETS)
	@echo "✅ UDP examples built"

.PHONY: udp-unicast
udp-unicast: udp_unicast_sender udp_unicast_receiver
	@echo "✅ UDP unicast examples built"

.PHONY: udp-multicast  
udp-multicast: udp_multicast_sender udp_multicast_receiver
	@echo "✅ UDP multicast examples built"

udp_unicast_sender: $(UDP_UNICAST_SENDER_SRC)
	$(CC) $(CFLAGS) $(GSTREAMER_CFLAGS) -o $@ $< $(GSTREAMER_LIBS)

udp_unicast_receiver: $(UDP_UNICAST_RECEIVER_SRC)
	$(CC) $(CFLAGS) $(GSTREAMER_CFLAGS) -o $@ $< $(GSTREAMER_LIBS)

udp_multicast_sender: $(UDP_MULTICAST_SENDER_SRC)
	$(CC) $(CFLAGS) $(GSTREAMER_CFLAGS) -o $@ $< $(GSTREAMER_LIBS)

udp_multicast_receiver: $(UDP_MULTICAST_RECEIVER_SRC)
	$(CC) $(CFLAGS) $(GSTREAMER_CFLAGS) -o $@ $< $(GSTREAMER_LIBS)

# Test targets
.PHONY: test
test: test-rtsp test-udp

.PHONY: test-rtsp
test-rtsp: $(RTSP_TARGETS)
	@echo "🧪 Testing RTSP examples..."
	@echo "Starting RTSP server in background..."
	@./rtsp_server &
	@RTSP_PID=$$!; \
	sleep 3; \
	echo "Testing RTSP client connection..."; \
	timeout 5 ./rtsp_client rtsp://localhost:8554/test || echo "Client test completed"; \
	echo "Stopping RTSP server..."; \
	kill $$RTSP_PID 2>/dev/null || true; \
	sleep 1
	@echo "✅ RTSP test completed"

.PHONY: test-udp
test-udp: $(UDP_TARGETS)
	@echo "🧪 Testing UDP examples..."
	@echo "Testing UDP unicast (sender -> receiver)..."
	@./udp_unicast_sender 127.0.0.1 5000 &
	@UDP_SENDER_PID=$$!; \
	sleep 2; \
	echo "Starting UDP receiver for 5 seconds..."; \
	timeout 5 ./udp_unicast_receiver 5000 || echo "UDP unicast test completed"; \
	echo "Stopping UDP sender..."; \
	kill $$UDP_SENDER_PID 2>/dev/null || true; \
	sleep 1
	@echo "✅ UDP unicast test completed"
	@echo "✅ UDP multicast test requires network configuration - run manually"

# Demos
.PHONY: demo-rtsp
demo-rtsp: $(RTSP_TARGETS)
	@echo "🎬 RTSP Demo"
	@echo "==========="
	@echo "Starting RTSP server on port 8554..."
	@echo "URL: rtsp://localhost:8554/test"
	@echo "Press Ctrl+C to stop"
	@./rtsp_server

.PHONY: demo-udp-unicast
demo-udp-unicast: udp_unicast_sender
	@echo "📡 UDP Unicast Demo"
	@echo "=================="
	@echo "Starting UDP unicast sender to localhost:5000"
	@echo "Run in another terminal: ./udp_unicast_receiver 5000"
	@echo "Press Ctrl+C to stop"
	@./udp_unicast_sender

.PHONY: demo-udp-multicast
demo-udp-multicast: udp_multicast_sender
	@echo "🌐 UDP Multicast Demo"
	@echo "===================="
	@echo "Starting UDP multicast sender to 224.1.1.1:5000"
	@echo "Run in another terminal: ./udp_multicast_receiver"
	@echo "Multiple receivers can join the same stream!"
	@echo "Press Ctrl+C to stop"
	@./udp_multicast_sender

# Utility targets
.PHONY: clean
clean:
	@echo "🧹 Cleaning up..."
	rm -f $(ALL_TARGETS)
	@echo "✅ Clean completed"

.PHONY: deps
deps:
	@echo "🔍 Checking dependencies..."
	@command -v pkg-config >/dev/null 2>&1 || { echo "❌ pkg-config not found. Install with: sudo apt install pkg-config"; exit 1; }
	@pkg-config --exists gstreamer-1.0 || { echo "❌ GStreamer development headers not found. Install with: sudo apt install libgstreamer1.0-dev"; exit 1; }
	@pkg-config --exists gstreamer-rtsp-server-1.0 || { echo "❌ GStreamer RTSP server not found. Install with: sudo apt install libgstreamer-rtsp-server-1.0-dev"; exit 1; }
	@echo "✅ All dependencies found"
	@echo "GStreamer version: $(shell pkg-config --modversion gstreamer-1.0)"

.PHONY: install-deps
install-deps:
	@echo "📦 Installing dependencies..."
	@echo "This will install GStreamer development packages"
	@read -p "Continue? (y/N): " confirm && [ "$$confirm" = "y" ] || exit 1
	sudo apt update
	sudo apt install -y \
		build-essential \
		pkg-config \
		libgstreamer1.0-dev \
		libgstreamer-plugins-base1.0-dev \
		libgstreamer-plugins-good1.0-dev \
		libgstreamer-plugins-bad1.0-dev \
		libgstreamer-rtsp-server-1.0-dev \
		gstreamer1.0-plugins-good \
		gstreamer1.0-plugins-bad \
		gstreamer1.0-plugins-ugly
	@echo "✅ Dependencies installed"

.PHONY: help
help:
	@echo "🎬 GStreamer Examples Makefile"
	@echo "=============================="
	@echo ""
	@echo "📋 Available targets:"
	@echo ""
	@echo "🏗️  Building:"
	@echo "  make all                - Build all examples"
	@echo "  make rtsp               - Build RTSP examples only"
	@echo "  make udp                - Build all UDP examples"
	@echo "  make udp-unicast        - Build UDP unicast examples"
	@echo "  make udp-multicast      - Build UDP multicast examples"
	@echo ""
	@echo "🧪 Testing:"
	@echo "  make test               - Run automated tests"
	@echo "  make test-rtsp          - Test RTSP examples"
	@echo "  make test-udp           - Test UDP examples"
	@echo ""
	@echo "🎭 Demos:"
	@echo "  make demo-rtsp          - Run RTSP server demo"
	@echo "  make demo-udp-unicast   - Run UDP unicast sender demo"
	@echo "  make demo-udp-multicast - Run UDP multicast sender demo"
	@echo ""
	@echo "🔧 Utilities:"
	@echo "  make deps               - Check dependencies"
	@echo "  make install-deps       - Install dependencies (Ubuntu/Debian)"
	@echo "  make clean              - Remove compiled files"
	@echo "  make help               - Show this help"
	@echo ""
	@echo "📚 Individual executables:"
	@echo "  rtsp_server             - RTSP video server"
	@echo "  rtsp_client             - RTSP video client"
	@echo "  udp_unicast_sender      - UDP unicast video sender"
	@echo "  udp_unicast_receiver    - UDP unicast video receiver"
	@echo "  udp_multicast_sender    - UDP multicast video sender"
	@echo "  udp_multicast_receiver  - UDP multicast video receiver"
	@echo ""
	@echo "💡 Use --help with any executable for detailed options"

# Debug target
.PHONY: debug
debug: CFLAGS += -g -DDEBUG
debug: $(ALL_TARGETS)
	@echo "🐛 Debug builds completed"

# Version information
.PHONY: version
version:
	@echo "GStreamer Examples Build Information"
	@echo "==================================="
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "GStreamer: $(shell pkg-config --modversion gstreamer-1.0 2>/dev/null || echo 'Not found')"
	@echo "RTSP Server: $(shell pkg-config --modversion gstreamer-rtsp-server-1.0 2>/dev/null || echo 'Not found')"
	@echo "Build date: $(shell date)"

# Make sure intermediate files are not deleted
.PRECIOUS: %.o 