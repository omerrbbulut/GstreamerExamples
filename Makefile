# GStreamer RTSP Examples Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99
GSTREAMER_CFLAGS = $(shell pkg-config --cflags gstreamer-1.0 gstreamer-rtsp-server-1.0)
GSTREAMER_LIBS = $(shell pkg-config --libs gstreamer-1.0 gstreamer-rtsp-server-1.0)

# Targets
TARGETS = rtsp_server rtsp_client

all: $(TARGETS)

# RTSP Server
rtsp_server: rtsp_server_example.c
	@echo "🔨 Compiling RTSP Server..."
	$(CC) $(CFLAGS) $(GSTREAMER_CFLAGS) -o $@ $< $(GSTREAMER_LIBS)
	@echo "✅ RTSP Server compiled: ./rtsp_server"

# RTSP Client  
rtsp_client: rtsp_client_example.c
	@echo "🔨 Compiling RTSP Client..."
	$(CC) $(CFLAGS) $(GSTREAMER_CFLAGS) -o $@ $< $(GSTREAMER_LIBS)
	@echo "✅ RTSP Client compiled: ./rtsp_client"

# Run server
server: rtsp_server
	@echo "🚀 Starting RTSP Server..."
	./rtsp_server

# Run client (varsayılan URL ile)
client: rtsp_client
	@echo "🎬 Starting RTSP Client..."
	./rtsp_client

# Test - server'ı background'da başlat, sonra client çalıştır
test: rtsp_server rtsp_client
	@echo "🧪 Testing RTSP Server & Client..."
	@echo "Starting server in background..."
	./rtsp_server &
	@sleep 3
	@echo "Starting client..."
	./rtsp_client rtsp://localhost:8554/test
	@pkill rtsp_server

# Clean
clean:
	@echo "🧹 Cleaning..."
	rm -f $(TARGETS)
	@echo "✅ Clean completed"

# Check dependencies
deps:
	@echo "🔍 Checking GStreamer dependencies..."
	@pkg-config --exists gstreamer-1.0 && echo "✅ gstreamer-1.0 found" || echo "❌ gstreamer-1.0 missing"
	@pkg-config --exists gstreamer-rtsp-server-1.0 && echo "✅ gstreamer-rtsp-server-1.0 found" || echo "❌ gstreamer-rtsp-server-1.0 missing"

# Help
help:
	@echo "📋 Available targets:"
	@echo "  all      - Build both server and client"
	@echo "  server   - Build and run RTSP server"
	@echo "  client   - Build and run RTSP client"
	@echo "  test     - Test server & client together"
	@echo "  clean    - Remove compiled files"
	@echo "  deps     - Check dependencies"
	@echo "  help     - Show this help"
	@echo ""
	@echo "🌐 Usage:"
	@echo "  make server  # Start RTSP server on port 8554"
	@echo "  make client  # Connect to default server"
	@echo "  ./rtsp_client rtsp://IP:PORT/path  # Custom URL"

.PHONY: all server client test clean deps help 