# 🔧 API Documentation

## RTSP Server API

### Core Functions

#### `gst_rtsp_server_new()`
Creates a new RTSP server instance.

**Returns:** `GstRTSPServer*` - New server object

**Example:**
```c
GstRTSPServer *server = gst_rtsp_server_new();
```

#### `gst_rtsp_server_set_service(server, port)`
Sets the port for the RTSP server.

**Parameters:**
- `server`: GstRTSPServer* - Server instance
- `port`: const gchar* - Port number as string

**Example:**
```c
gst_rtsp_server_set_service(server, "8554");
```

#### `gst_rtsp_server_attach(server, context)`
Attaches server to a GMainContext.

**Parameters:**
- `server`: GstRTSPServer* - Server instance  
- `context`: GMainContext* - Main context (NULL for default)

**Returns:** `guint` - Source ID (0 on failure)

**Example:**
```c
guint id = gst_rtsp_server_attach(server, NULL);
if (id == 0) {
    g_print("Failed to attach server\n");
}
```

### Mount Points

#### `gst_rtsp_server_get_mount_points(server)`
Gets the mount points object from server.

**Parameters:**
- `server`: GstRTSPServer* - Server instance

**Returns:** `GstRTSPMountPoints*` - Mount points object

#### `gst_rtsp_mount_points_add_factory(mounts, path, factory)`
Adds a media factory to a mount point.

**Parameters:**
- `mounts`: GstRTSPMountPoints* - Mount points object
- `path`: const gchar* - URL path (e.g., "/test")
- `factory`: GstRTSPMediaFactory* - Media factory

**Example:**
```c
gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
```

### Media Factory

#### `gst_rtsp_media_factory_new()`
Creates a new media factory.

**Returns:** `GstRTSPMediaFactory*` - New factory object

#### `gst_rtsp_media_factory_set_launch(factory, pipeline)`
Sets the GStreamer pipeline launch string.

**Parameters:**
- `factory`: GstRTSPMediaFactory* - Factory instance
- `pipeline`: const gchar* - Pipeline description

**Example:**
```c
gst_rtsp_media_factory_set_launch(factory,
    "( videotestsrc ! x264enc ! rtph264pay name=pay0 )");
```

#### `gst_rtsp_media_factory_set_shared(factory, shared)`
Sets whether media should be shared between clients.

**Parameters:**
- `factory`: GstRTSPMediaFactory* - Factory instance
- `shared`: gboolean - TRUE for shared media

## RTSP Client API

### Pipeline Elements

#### `rtspsrc`
GStreamer element for receiving RTSP streams.

**Properties:**
- `location`: RTSP URL
- `latency`: Buffer latency in milliseconds
- `protocols`: Allowed protocols (tcp, udp)

**Example:**
```c
"rtspsrc location=rtsp://localhost:8554/test latency=100"
```

#### `rtph264depay`
Depayloads H.264 from RTP packets.

**Capabilities:**
- Input: application/x-rtp with H.264 payload
- Output: video/x-h264 stream

#### `h264parse`
Parses H.264 video streams.

**Functions:**
- Extracts codec information
- Ensures proper stream format
- Handles configuration changes

#### `avdec_h264`
FFmpeg-based H.264 decoder.

**Features:**
- Hardware acceleration support
- Multiple threading
- Error resilience

## Pipeline Patterns

### Basic RTSP Server Pipeline
```c
const char* server_pipeline = 
    "( "
    "videotestsrc pattern=ball is-live=true ! "
    "video/x-raw,width=640,height=480,framerate=30/1 ! "
    "videoconvert ! "
    "x264enc tune=zerolatency bitrate=2000 speed-preset=ultrafast ! "
    "video/x-h264,profile=baseline ! "
    "rtph264pay name=pay0 pt=96 "
    ")";
```

### Basic RTSP Client Pipeline
```c
const char* client_pipeline = 
    "rtspsrc location=%s latency=100 ! "
    "rtph264depay ! "
    "h264parse ! "
    "avdec_h264 ! "
    "videoconvert ! "
    "autovideosink";
```

### Camera Source Pipeline
```c
const char* camera_pipeline = 
    "( "
    "v4l2src device=/dev/video0 ! "
    "video/x-raw,width=1280,height=720,framerate=30/1 ! "
    "videoconvert ! "
    "x264enc tune=zerolatency ! "
    "rtph264pay name=pay0 "
    ")";
```

### File Source Pipeline
```c
const char* file_pipeline = 
    "( "
    "filesrc location=%s ! "
    "decodebin ! "
    "videoconvert ! "
    "videoscale ! "
    "video/x-raw,width=640,height=480 ! "
    "x264enc ! "
    "rtph264pay name=pay0 "
    ")";
```

## Error Handling

### Server Error Codes
```c
typedef enum {
    RTSP_SERVER_OK = 0,
    RTSP_SERVER_ERROR_INIT = -1,
    RTSP_SERVER_ERROR_PORT = -2,
    RTSP_SERVER_ERROR_PIPELINE = -3,
    RTSP_SERVER_ERROR_ATTACH = -4
} RTSPServerError;
```

### Client Error Codes
```c
typedef enum {
    RTSP_CLIENT_OK = 0,
    RTSP_CLIENT_ERROR_INIT = -1,
    RTSP_CLIENT_ERROR_CONNECT = -2,
    RTSP_CLIENT_ERROR_PIPELINE = -3,
    RTSP_CLIENT_ERROR_DECODE = -4
} RTSPClientError;
```

### Bus Message Handling
```c
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            // End of stream
            break;
        case GST_MESSAGE_ERROR:
            // Error occurred
            GError *error;
            gchar *debug;
            gst_message_parse_error(msg, &error, &debug);
            // Handle error
            break;
        case GST_MESSAGE_STATE_CHANGED:
            // Pipeline state changed
            break;
    }
    return TRUE;
}
```

## Configuration Options

### Video Quality Settings
```c
// High quality (high latency)
"x264enc bitrate=8000 speed-preset=medium tune=film"

// Balanced (medium latency)
"x264enc bitrate=4000 speed-preset=fast tune=zerolatency"

// Low latency (lower quality)
"x264enc bitrate=2000 speed-preset=ultrafast tune=zerolatency"
```

### Network Optimization
```c
// UDP preferred (lower latency)
"rtspsrc protocols=udp"

// TCP fallback (more reliable)
"rtspsrc protocols=tcp"

// Both allowed (automatic selection)
"rtspsrc protocols=udp+tcp"
```

### Buffer Management
```c
// Minimal buffering (lowest latency)
"rtspsrc latency=50"

// Standard buffering (balanced)
"rtspsrc latency=200"

// High buffering (most stable)
"rtspsrc latency=500"
```

## Performance Tuning

### CPU Optimization
```c
// Multi-threading
"x264enc threads=4"

// SIMD optimization
export GST_REGISTRY_DISABLE=yes  // Disable registry caching
```

### Memory Optimization
```c
// Buffer pool tuning
"queue max-size-buffers=10 max-size-time=0"

// Memory alignment
"videoconvert ! video/x-raw,format=I420"
```

### Network Optimization
```c
// MTU sizing
"rtph264pay mtu=1400"

// RTP configuration
"rtph264pay config-interval=1 pt=96"
```

## Advanced Features

### Authentication
```c
// Server-side authentication
GstRTSPAuth *auth = gst_rtsp_auth_new();
gst_rtsp_auth_set_default_token(auth, token);
gst_rtsp_server_set_auth(server, auth);
```

### Multicast
```c
// Multicast configuration
gst_rtsp_media_factory_set_protocols(factory, GST_RTSP_LOWER_TRANS_UDP_MCAST);
gst_rtsp_media_factory_set_address_pool(factory, pool);
```

### Recording
```c
// Tee for recording
"videotestsrc ! tee name=t ! x264enc ! rtph264pay name=pay0 "
"t. ! x264enc ! mp4mux ! filesink location=recording.mp4"
```

## Debugging

### Debug Levels
```bash
export GST_DEBUG=3                    # Info level
export GST_DEBUG=4                    # Debug level
export GST_DEBUG=rtspserver:5         # Component-specific
export GST_DEBUG_DUMP_DOT_DIR=.       # Pipeline graphs
```

### Common Debug Categories
- `rtspserver`: RTSP server operations
- `rtspsrc`: RTSP client operations  
- `rtph264pay`: H.264 RTP payloader
- `rtph264depay`: H.264 RTP depayloader
- `x264enc`: H.264 encoder 