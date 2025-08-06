# UDP Streaming API Documentation

## Overview

This document provides detailed API documentation for the UDP streaming examples in GStreamerExamples. The UDP streaming solution provides low-latency video streaming over UDP protocol using both unicast and multicast approaches.

## Architecture

### Components

1. **UDP Unicast Sender** (`udp_unicast_sender.c`)
2. **UDP Unicast Receiver** (`udp_unicast_receiver.c`)
3. **UDP Multicast Sender** (`udp_multicast_sender.c`)
4. **UDP Multicast Receiver** (`udp_multicast_receiver.c`)

### Pipeline Architecture

#### Sender Pipeline
```
videotestsrc/v4l2src → videoconvert → x264enc → rtph264pay → udpsink/multiudpsink
```

#### Receiver Pipeline
```
udpsrc → rtph264depay → h264parse → avdec_h264 → videoconvert → autovideosink/filesink
```

## Data Structures

### UDPSenderData
```c
typedef struct {
    GstElement *pipeline;
    GstElement *source;
    GstElement *encoder;
    GstElement *payloader;
    GstElement *sink;
    GMainLoop *loop;
    char *host;
    int port;
    gboolean use_camera;
} UDPSenderData;
```

### UDPReceiverData
```c
typedef struct {
    GstElement *pipeline;
    GstElement *source;
    GstElement *depayloader;
    GstElement *parser;
    GstElement *decoder;
    GstElement *sink;
    GMainLoop *loop;
    int port;
    char *multicast_group;
    gboolean save_to_file;
    char *output_file;
} UDPReceiverData;
```

## API Functions

### Sender Functions

#### `create_sender_pipeline(UDPSenderData *data)`
Creates the GStreamer pipeline for UDP streaming.

**Parameters:**
- `data`: Pointer to UDPSenderData structure

**Returns:**
- `TRUE`: Pipeline created successfully
- `FALSE`: Pipeline creation failed

**Pipeline Elements:**
- `videotestsrc` or `v4l2src`: Video source
- `videoconvert`: Format conversion
- `x264enc`: H.264 encoding with low-latency settings
- `rtph264pay`: RTP payloading
- `udpsink`/`multiudpsink`: UDP transmission

#### Low-Latency Encoder Settings
```c
g_object_set(data->encoder,
    "tune", 0x00000004,        // zerolatency
    "speed-preset", 1,         // ultrafast
    "bitrate", 1000,           // 1Mbps
    "key-int-max", 30,         // GOP size
    "bframes", 0,              // No B-frames
    "threads", 1,              // Single thread
    NULL);
```

### Receiver Functions

#### `create_receiver_pipeline(UDPReceiverData *data)`
Creates the GStreamer pipeline for UDP reception.

**Parameters:**
- `data`: Pointer to UDPReceiverData structure

**Returns:**
- `TRUE`: Pipeline created successfully
- `FALSE`: Pipeline creation failed

**Pipeline Elements:**
- `udpsrc`: UDP reception
- `rtph264depay`: RTP depayloading
- `h264parse`: H.264 parsing
- `avdec_h264`: H.264 decoding
- `autovideosink`/`filesink`: Output

#### UDP Source Configuration
```c
g_object_set(data->source,
    "port", data->port,
    "caps", gst_caps_from_string("application/x-rtp, payload=96"),
    NULL);
```

### Multicast Functions

#### `setup_multicast_sender(UDPSenderData *data, const char *group, int ttl)`
Configures multicast-specific settings.

**Parameters:**
- `data`: Sender data structure
- `group`: Multicast group address (e.g., "224.1.1.1")
- `ttl`: Time-to-live value

#### `join_multicast_group(UDPReceiverData *data, const char *group)`
Joins a multicast group for reception.

**Parameters:**
- `data`: Receiver data structure
- `group`: Multicast group address

## Signal Handling

### Global Signal Management
```c
static GMainLoop *global_main_loop = NULL;

static void signal_handler(int sig) {
    g_print("\n🛑 Received signal %d, stopping...\n", sig);
    if (global_main_loop) {
        g_main_loop_quit(global_main_loop);
    }
}
```

### Signal Registration
```c
signal(SIGINT, signal_handler);
signal(SIGTERM, signal_handler);
```

## Bus Message Handling

### Message Callback
```c
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    (void)bus; // Suppress unused warning
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR:
            handle_error_message(msg);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            handle_state_change(msg);
            break;
    }
    return TRUE;
}
```

## Configuration Options

### Video Source Options
- **Test Pattern**: `videotestsrc` with moving pattern
- **Camera**: `v4l2src` with JPEG decode pipeline

### Output Options
- **Display**: `autovideosink` for screen output
- **File**: `filesink` for saving to disk

### Network Options
- **Unicast**: Point-to-point streaming
- **Multicast**: One-to-many streaming with TTL control

## Error Handling

### Common Error Scenarios
1. **Port Already in Use**: Check with `netstat -ulnp`
2. **Pipeline Creation Failed**: Verify GStreamer plugins
3. **Network Unreachable**: Check firewall/routing
4. **Codec Issues**: Ensure H.264 support

### Debug Options
```bash
GST_DEBUG=3 ./udp_unicast_sender 127.0.0.1 5000
```

## Performance Considerations

### Latency Optimization
- Zero-latency encoding (`tune=zerolatency`)
- Ultrafast preset (`speed-preset=ultrafast`)
- No B-frames (`bframes=0`)
- Minimal buffering
- Single-threaded encoding

### Network Optimization
- MTU-aware packetization
- RTP aggregation control
- UDP buffer sizing
- Multicast TTL tuning

## Thread Safety

All operations are performed within the GStreamer main loop context. Signal handlers use atomic operations to ensure safe shutdown.

## Memory Management

- Automatic cleanup on pipeline destruction
- Signal handler ensures proper resource release
- GStreamer reference counting handles element lifecycle

## Platform Support

- **Linux**: Full support with V4L2 camera input
- **Other Unix**: Limited to test source
- **Dependencies**: GStreamer 1.0+, x264, standard codecs

## Example Usage

### Basic Unicast
```c
UDPSenderData sender_data = {0};
sender_data.host = "192.168.1.100";
sender_data.port = 5000;
sender_data.use_camera = FALSE;

if (create_sender_pipeline(&sender_data)) {
    gst_element_set_state(sender_data.pipeline, GST_STATE_PLAYING);
    g_main_loop_run(sender_data.loop);
}
```

### Basic Multicast
```c
UDPSenderData sender_data = {0};
setup_multicast_sender(&sender_data, "224.1.1.1", 5);
create_sender_pipeline(&sender_data);
```

This API provides a foundation for building robust UDP streaming applications with GStreamer. 