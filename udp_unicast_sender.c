/*
 * UDP Unicast Sender Example
 * 
 * This example demonstrates how to stream video over UDP unicast using GStreamer.
 * It creates a simple pipeline that captures video (test source or camera) and
 * sends it to a specific IP address and port via UDP.
 * 
 * Pipeline: videosource -> encoder -> rtp -> udpsink
 * 
 * Usage: ./udp_unicast_sender [host] [port]
 * Example: ./udp_unicast_sender 192.168.1.100 5000
 * 
 * Compile: gcc -o udp_unicast_sender udp_unicast_sender.c `pkg-config --cflags --libs gstreamer-1.0`
 */

#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

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

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    (void)bus; // Suppress unused parameter warning
    UDPSenderData *sender_data = (UDPSenderData*)data;
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(sender_data->loop);
            break;
            
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            
            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);
            
            g_printerr("Error: %s\n", error->message);
            g_error_free(error);
            
            g_main_loop_quit(sender_data->loop);
            break;
        }
        
        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state;
            
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(sender_data->pipeline)) {
                gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
                g_print("Pipeline state changed from %s to %s\n",
                    gst_element_state_get_name(old_state),
                    gst_element_state_get_name(new_state));
                    
                if (new_state == GST_STATE_PLAYING) {
                    g_print("🚀 UDP Unicast streaming started!\n");
                    g_print("📡 Streaming to: %s:%d\n", sender_data->host, sender_data->port);
                    g_print("🎯 Receiver command:\n");
                    g_print("   gst-launch-1.0 udpsrc port=%d ! application/x-rtp ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink\n", 
                           sender_data->port);
                }
            }
            break;
        }
        
        default:
            break;
    }
    
    return TRUE;
}

static void signal_handler(int sig) {
    g_print("\n🛑 Received signal %d, stopping...\n", sig);
    // The main loop will be quit in the signal handling
}

static gboolean setup_pipeline(UDPSenderData *data) {
    GstBus *bus;
    GstCaps *caps;
    
    // Create pipeline
    data->pipeline = gst_pipeline_new("udp-sender");
    
    if (data->use_camera) {
        // Camera source (USB webcam)
        data->source = gst_element_factory_make("v4l2src", "camera-source");
        if (!data->source) {
            g_printerr("❌ Could not create v4l2src element. Falling back to test source.\n");
            data->use_camera = FALSE;
        } else {
            g_object_set(data->source, "device", "/dev/video0", NULL);
            g_print("📷 Using camera: /dev/video0\n");
        }
    }
    
    if (!data->use_camera) {
        // Test video source
        data->source = gst_element_factory_make("videotestsrc", "test-source");
        g_object_set(data->source, 
                    "pattern", 0,  // SMPTE color bars
                    "is-live", TRUE,
                    NULL);
        g_print("🎨 Using test video source\n");
    }
    
    // Video encoder (H.264)
    data->encoder = gst_element_factory_make("x264enc", "encoder");
    g_object_set(data->encoder,
                "tune", 0x04,  // zerolatency
                "speed-preset", 1,  // ultrafast
                "bitrate", 2000,
                "key-int-max", 30,
                "bframes", 0,
                NULL);
    
    // RTP payloader
    data->payloader = gst_element_factory_make("rtph264pay", "payloader");
    g_object_set(data->payloader,
                "pt", 96,
                "config-interval", 1,
                NULL);
    
    // UDP sink
    data->sink = gst_element_factory_make("udpsink", "udp-sink");
    g_object_set(data->sink,
                "host", data->host,
                "port", data->port,
                "sync", FALSE,
                "async", FALSE,
                NULL);
    
    // Check if all elements were created
    if (!data->pipeline || !data->source || !data->encoder || 
        !data->payloader || !data->sink) {
        g_printerr("❌ Not all elements could be created.\n");
        return FALSE;
    }
    
    // Add elements to pipeline
    gst_bin_add_many(GST_BIN(data->pipeline),
                     data->source, data->encoder, data->payloader, data->sink,
                     NULL);
    
    // Set caps between source and encoder
    if (data->use_camera) {
        caps = gst_caps_from_string("image/jpeg,width=640,height=480,framerate=30/1");
        
        // For camera: source -> jpegdec -> videoconvert -> encoder
        GstElement *jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
        GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
        
        if (!jpegdec || !videoconvert) {
            g_printerr("❌ Could not create jpegdec or videoconvert elements.\n");
            gst_caps_unref(caps);
            return FALSE;
        }
        
        gst_bin_add_many(GST_BIN(data->pipeline), jpegdec, videoconvert, NULL);
        
        if (!gst_element_link_filtered(data->source, jpegdec, caps) ||
            !gst_element_link(jpegdec, videoconvert) ||
            !gst_element_link(videoconvert, data->encoder)) {
            g_printerr("❌ Elements could not be linked (camera pipeline).\n");
            gst_caps_unref(caps);
            return FALSE;
        }
    } else {
        caps = gst_caps_from_string("video/x-raw,width=640,height=480,framerate=30/1");
        
        if (!gst_element_link_filtered(data->source, data->encoder, caps)) {
            g_printerr("❌ Source and encoder could not be linked.\n");
            gst_caps_unref(caps);
            return FALSE;
        }
    }
    
    gst_caps_unref(caps);
    
    // Link encoder -> payloader -> sink
    if (!gst_element_link(data->encoder, data->payloader) ||
        !gst_element_link(data->payloader, data->sink)) {
        g_printerr("❌ Elements could not be linked.\n");
        return FALSE;
    }
    
    // Add message handler
    bus = gst_element_get_bus(data->pipeline);
    gst_bus_add_watch(bus, bus_call, data);
    gst_object_unref(bus);
    
    g_print("✅ Pipeline created successfully\n");
    return TRUE;
}

static void print_usage(const char *program_name) {
    g_print("UDP Unicast Video Sender\n");
    g_print("========================\n\n");
    g_print("Usage: %s [options] [host] [port]\n\n", program_name);
    g_print("Options:\n");
    g_print("  -c, --camera     Use camera instead of test source\n");
    g_print("  -h, --help       Show this help message\n\n");
    g_print("Arguments:\n");
    g_print("  host             Target IP address (default: 127.0.0.1)\n");
    g_print("  port             Target UDP port (default: 5000)\n\n");
    g_print("Examples:\n");
    g_print("  %s                          # Send to localhost:5000 with test source\n", program_name);
    g_print("  %s -c                       # Send to localhost:5000 with camera\n", program_name);
    g_print("  %s 192.168.1.100 5001       # Send to specific IP and port\n", program_name);
    g_print("  %s -c 224.1.1.1 5000        # Send to multicast address with camera\n\n", program_name);
    g_print("Receiver command:\n");
    g_print("  gst-launch-1.0 udpsrc port=PORT ! application/x-rtp ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink\n");
}

int main(int argc, char *argv[]) {
    UDPSenderData data;
    GstStateChangeReturn ret;
    int arg_index = 1;
    
    // Initialize structure
    memset(&data, 0, sizeof(UDPSenderData));
    data.host = "127.0.0.1";  // default host
    data.port = 5000;         // default port
    data.use_camera = FALSE;
    
    // Parse command line arguments
    while (arg_index < argc) {
        if (strcmp(argv[arg_index], "-c") == 0 || strcmp(argv[arg_index], "--camera") == 0) {
            data.use_camera = TRUE;
            arg_index++;
        } else if (strcmp(argv[arg_index], "-h") == 0 || strcmp(argv[arg_index], "--help") == 0) {
            print_usage("udp_unicast_sender");
            return 0;
        } else {
            break;  // Non-option argument found
        }
    }
    
    // Parse host and port
    if (arg_index < argc) {
        data.host = argv[arg_index++];
    }
    if (arg_index < argc) {
        data.port = atoi(argv[arg_index++]);
        if (data.port <= 0 || data.port > 65535) {
            g_printerr("❌ Invalid port number: %d\n", data.port);
            return -1;
        }
    }
    
    // Initialize GStreamer
    gst_init(&argc, &argv);
    
    g_print("🎬 UDP Unicast Video Sender\n");
    g_print("============================\n");
    g_print("📍 Target: %s:%d\n", data.host, data.port);
    g_print("📹 Source: %s\n", data.use_camera ? "Camera" : "Test Pattern");
    g_print("⚙️  Encoder: H.264 (ultrafast, zerolatency)\n\n");
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create pipeline
    if (!setup_pipeline(&data)) {
        g_printerr("❌ Failed to setup pipeline\n");
        return -1;
    }
    
    // Create main loop
    data.loop = g_main_loop_new(NULL, FALSE);
    
    // Start playing
    g_print("▶️  Starting pipeline...\n");
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("❌ Unable to set the pipeline to the playing state.\n");
        gst_object_unref(data.pipeline);
        g_main_loop_unref(data.loop);
        return -1;
    }
    
    // Run main loop
    g_main_loop_run(data.loop);
    
    // Cleanup
    g_print("🧹 Cleaning up...\n");
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    g_main_loop_unref(data.loop);
    
    g_print("👋 UDP Unicast Sender stopped.\n");
    return 0;
} 