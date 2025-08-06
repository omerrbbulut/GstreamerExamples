# 🎯 Advanced Examples

## Camera Streaming

### USB Camera to RTSP
```c
// Camera pipeline with v4l2src
const char* camera_pipeline = 
    "( "
    "v4l2src device=/dev/video0 ! "
    "image/jpeg,width=1280,height=720,framerate=30/1 ! "
    "jpegdec ! "
    "videoconvert ! "
    "x264enc tune=zerolatency bitrate=4000 ! "
    "rtph264pay name=pay0 pt=96 "
    ")";
```

### Multiple Camera Sources
```c
// Dual camera setup
const char* dual_camera_pipeline = 
    "( "
    "compositor name=comp sink_0::xpos=0 sink_1::xpos=640 ! "
    "x264enc ! rtph264pay name=pay0 "
    "v4l2src device=/dev/video0 ! videoscale ! video/x-raw,width=640,height=480 ! comp.sink_0 "
    "v4l2src device=/dev/video1 ! videoscale ! video/x-raw,width=640,height=480 ! comp.sink_1 "
    ")";
```

## File Streaming

### Video File to RTSP
```c
// File source with loop
const char* file_pipeline = 
    "( "
    "multifilesrc location=\"video_%02d.mp4\" loop=true ! "
    "qtdemux ! h264parse ! "
    "rtph264pay name=pay0 pt=96 config-interval=1 "
    ")";
```

### Image Sequence
```c
// Image sequence streaming
const char* image_sequence_pipeline = 
    "( "
    "multifilesrc location=\"frame_%04d.jpg\" caps=\"image/jpeg,framerate=25/1\" ! "
    "jpegdec ! "
    "videoconvert ! "
    "x264enc tune=zerolatency ! "
    "rtph264pay name=pay0 "
    ")";
```

## Audio + Video Streaming

### Combined A/V Pipeline
```c
// Video + Audio RTSP server
const char* av_pipeline = 
    "( "
    // Video branch
    "videotestsrc ! "
    "video/x-raw,width=640,height=480,framerate=30/1 ! "
    "x264enc ! rtph264pay name=pay0 pt=96 "
    
    // Audio branch  
    "audiotestsrc ! "
    "audio/x-raw,rate=44100,channels=2 ! "
    "audioconvert ! "
    "avenc_aac ! "
    "rtpmp4apay name=pay1 pt=97 "
    ")";
```

### Microphone Input
```c
// Real microphone + camera
const char* mic_camera_pipeline = 
    "( "
    "v4l2src ! videoconvert ! x264enc ! rtph264pay name=pay0 pt=96 "
    "alsasrc ! audioconvert ! avenc_aac ! rtpmp4apay name=pay1 pt=97 "
    ")";
```

## Low Latency Configurations

### Ultra Low Latency
```c
// Minimal latency settings
const char* ultra_low_latency = 
    "( "
    "videotestsrc is-live=true ! "
    "video/x-raw,format=I420,width=640,height=480,framerate=60/1 ! "
    "x264enc "
        "tune=zerolatency "
        "speed-preset=ultrafast "
        "sync-lookahead=0 "
        "rc-lookahead=0 "
        "bframes=0 "
        "key-int-max=15 "
        "intra-refresh=true "
        "sliced-threads=true "
        "bitrate=2000 ! "
    "rtph264pay "
        "name=pay0 "
        "pt=96 "
        "aggregate-mode=zero-latency "
        "mtu=1200 "
    ")";
```

### Network Optimized
```c
// Network-aware configuration
const char* network_optimized = 
    "( "
    "videotestsrc ! "
    "videoscale ! videorate ! "
    "video/x-raw,width=480,height=360,framerate=15/1 ! "
    "x264enc "
        "bitrate=800 "
        "speed-preset=fast "
        "tune=zerolatency ! "
    "rtph264pay "
        "name=pay0 "
        "mtu=1000 "
        "config-interval=5 "
    ")";
```

## Multi-Client Scenarios

### Adaptive Bitrate
```c
// Different quality streams for different clients
void setup_adaptive_streams(GstRTSPServer *server) {
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    
    // High quality stream
    GstRTSPMediaFactory *hq_factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(hq_factory,
        "( videotestsrc ! video/x-raw,width=1920,height=1080 ! "
        "x264enc bitrate=8000 ! rtph264pay name=pay0 )");
    gst_rtsp_mount_points_add_factory(mounts, "/hq", hq_factory);
    
    // Low quality stream
    GstRTSPMediaFactory *lq_factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(lq_factory,
        "( videotestsrc ! video/x-raw,width=640,height=480 ! "
        "x264enc bitrate=1000 ! rtph264pay name=pay0 )");
    gst_rtsp_mount_points_add_factory(mounts, "/lq", lq_factory);
    
    g_object_unref(mounts);
}
```

### Client Management
```c
// Track connected clients
typedef struct {
    GstRTSPServer *server;
    GList *clients;
    int client_count;
} ServerContext;

static void on_client_connected(GstRTSPServer *server, 
                               GstRTSPClient *client, 
                               ServerContext *ctx) {
    ctx->clients = g_list_append(ctx->clients, client);
    ctx->client_count++;
    g_print("Client connected. Total: %d\n", ctx->client_count);
}
```

## Advanced Client Patterns

### Reconnecting Client
```c
// Auto-reconnect client implementation
typedef struct {
    gchar *rtsp_url;
    GstElement *pipeline;
    GMainLoop *loop;
    gboolean reconnect;
    int reconnect_attempts;
} ReconnectClient;

static gboolean try_reconnect(gpointer user_data) {
    ReconnectClient *client = (ReconnectClient*)user_data;
    
    if (client->reconnect_attempts < 5) {
        g_print("Attempting reconnection #%d\n", client->reconnect_attempts + 1);
        
        // Recreate pipeline
        if (client->pipeline) {
            gst_element_set_state(client->pipeline, GST_STATE_NULL);
            gst_object_unref(client->pipeline);
        }
        
        // Create new pipeline
        gchar *pipeline_desc = g_strdup_printf(
            "rtspsrc location=%s retry=5 ! "
            "rtph264depay ! h264parse ! avdec_h264 ! autovideosink",
            client->rtsp_url);
        
        client->pipeline = gst_parse_launch(pipeline_desc, NULL);
        g_free(pipeline_desc);
        
        if (client->pipeline) {
            gst_element_set_state(client->pipeline, GST_STATE_PLAYING);
            client->reconnect_attempts++;
            return FALSE; // Don't repeat timer
        }
    }
    
    g_print("Max reconnection attempts reached\n");
    g_main_loop_quit(client->loop);
    return FALSE;
}
```

### Recording Client
```c
// Client that records while displaying
const char* recording_client_pipeline = 
    "rtspsrc location=%s ! "
    "rtph264depay ! "
    "tee name=t ! "
    
    // Display branch
    "queue ! h264parse ! avdec_h264 ! autovideosink "
    
    // Recording branch
    "t. ! queue ! h264parse ! mp4mux ! "
    "filesink location=recording_%d.mp4";
```

## Performance Monitoring

### Statistics Collection
```c
// Pipeline statistics
typedef struct {
    guint64 frames_processed;
    guint64 bytes_sent;
    gdouble current_fps;
    gdouble avg_bitrate;
    GTimer *timer;
} PipelineStats;

static gboolean collect_stats(gpointer user_data) {
    PipelineStats *stats = (PipelineStats*)user_data;
    
    // Get pipeline statistics
    GstElement *pipeline = GST_ELEMENT(user_data);
    
    // Query frame count from source
    GstQuery *query = gst_query_new_position(GST_FORMAT_DEFAULT);
    if (gst_element_query(pipeline, query)) {
        gint64 frames;
        gst_query_parse_position(query, NULL, &frames);
        stats->frames_processed = frames;
    }
    gst_query_unref(query);
    
    // Calculate FPS
    gdouble elapsed = g_timer_elapsed(stats->timer, NULL);
    stats->current_fps = stats->frames_processed / elapsed;
    
    g_print("Stats: %lu frames, %.2f fps, %.2f kbps\n",
            stats->frames_processed, stats->current_fps, stats->avg_bitrate);
    
    return TRUE; // Continue timer
}
```

### CPU and Memory Monitoring
```c
// System resource monitoring
#include <sys/resource.h>

static void print_resource_usage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    
    g_print("CPU Time: %ld.%06lds\n", 
            usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
    g_print("Memory: %ld KB\n", usage.ru_maxrss);
}
```

## Security Examples

### Basic Authentication
```c
// RTSP authentication setup
void setup_auth(GstRTSPServer *server) {
    GstRTSPAuth *auth = gst_rtsp_auth_new();
    
    // Create token with permissions
    GstRTSPToken *token = gst_rtsp_token_new(
        GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, "user",
        NULL);
    
    // Set basic auth
    gst_rtsp_auth_add_basic(auth, "user", "password", token);
    gst_rtsp_token_unref(token);
    
    // Apply to server
    gst_rtsp_server_set_auth(server, auth);
    g_object_unref(auth);
}
```

### SSL/TLS Encryption
```c
// RTSP over TLS
void setup_tls(GstRTSPServer *server) {
    GTlsCertificate *cert = g_tls_certificate_new_from_files(
        "server.crt", "server.key", NULL);
    
    if (cert) {
        gst_rtsp_server_set_tls_certificate(server, cert);
        gst_rtsp_server_set_service(server, "8322"); // RTSPS port
        g_object_unref(cert);
    }
}
```

## Testing and Debugging

### Automated Testing
```bash
#!/bin/bash
# test_rtsp.sh - Automated RTSP testing

echo "Starting RTSP server..."
./rtsp_server &
SERVER_PID=$!

sleep 3

echo "Testing with gst-launch..."
timeout 10s gst-launch-1.0 \
    rtspsrc location=rtsp://localhost:8554/test ! \
    fakesink

echo "Testing with VLC..."
timeout 10s vlc --intf dummy rtsp://localhost:8554/test vlc://quit

echo "Stopping server..."
kill $SERVER_PID

echo "Test completed"
```

### Performance Profiling
```bash
# Profile with Valgrind
valgrind --tool=callgrind ./rtsp_server

# Profile with perf
perf record ./rtsp_server
perf report

# Memory leak detection
GST_DEBUG="*:2" GST_DEBUG_NO_COLOR=1 \
    valgrind --leak-check=full ./rtsp_server
```

### Network Analysis
```bash
# Capture RTSP traffic
tcpdump -i lo -w rtsp_capture.pcap port 8554

# Analyze with Wireshark
wireshark rtsp_capture.pcap

# Monitor bandwidth
iftop -i lo -P
``` 