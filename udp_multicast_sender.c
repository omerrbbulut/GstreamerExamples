/*
 * UDP Multicast Sender Example
 * 
 * This example demonstrates how to stream video over UDP multicast using GStreamer.
 * It creates a pipeline that captures video and sends it to a multicast group,
 * allowing multiple receivers to join and receive the same stream simultaneously.
 * 
 * Pipeline: videosource -> encoder -> rtp -> multiudpsink
 * 
 * Usage: ./udp_multicast_sender [multicast_group] [port] [ttl]
 * Example: ./udp_multicast_sender 224.1.1.1 5000 5
 * 
 * Compile: gcc -o udp_multicast_sender udp_multicast_sender.c `pkg-config --cflags --libs gstreamer-1.0`
 */

#define _GNU_SOURCE
#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct {
    GstElement *pipeline;
    GstElement *source;
    GstElement *encoder;
    GstElement *payloader;
    GstElement *sink;
    GMainLoop *loop;
    char *multicast_group;
    int port;
    int ttl;
    gboolean use_camera;
} UDPMulticastSenderData;

static gboolean is_multicast_address(const char *address) {
    struct in_addr addr;
    if (inet_aton(address, &addr) == 0) {
        return FALSE;
    }
    
    // Check if address is in multicast range (224.0.0.0 - 239.255.255.255)
    unsigned int ip = ntohl(addr.s_addr);
    return (ip >= 0xE0000000 && ip <= 0xEFFFFFFF);
}

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    (void)bus; // Suppress unused parameter warning
    UDPMulticastSenderData *sender_data = (UDPMulticastSenderData*)data;
    
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
                    g_print("🌐 UDP Multicast streaming started!\n");
                    g_print("📡 Multicast Group: %s:%d (TTL: %d)\n", 
                           sender_data->multicast_group, sender_data->port, sender_data->ttl);
                    g_print("👥 Multiple receivers can join this stream\n");
                    g_print("🎯 Receiver command:\n");
                    g_print("   gst-launch-1.0 udpsrc multicast-group=%s port=%d ! application/x-rtp ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink\n", 
                           sender_data->multicast_group, sender_data->port);
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
    g_print("\n🛑 Received signal %d, stopping multicast...\n", sig);
}

static gboolean setup_pipeline(UDPMulticastSenderData *data) {
    GstBus *bus;
    GstCaps *caps;
    
    // Create pipeline
    data->pipeline = gst_pipeline_new("multicast-sender");
    
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
        // Test video source with moving pattern
        data->source = gst_element_factory_make("videotestsrc", "test-source");
        g_object_set(data->source, 
                    "pattern", 18,  // moving ball pattern
                    "is-live", TRUE,
                    NULL);
        g_print("🎨 Using test video source (moving pattern)\n");
    }
    
    // Video encoder (H.264) with multicast-optimized settings
    data->encoder = gst_element_factory_make("x264enc", "encoder");
    g_object_set(data->encoder,
                "tune", 0x04,  // zerolatency
                "speed-preset", 1,  // ultrafast
                "bitrate", 1500,   // Lower bitrate for multicast
                "key-int-max", 15, // More frequent keyframes for late joiners
                "bframes", 0,
                "sliced-threads", TRUE,
                "threads", 1,
                NULL);
    
    // RTP payloader with multicast settings
    data->payloader = gst_element_factory_make("rtph264pay", "payloader");
    g_object_set(data->payloader,
                "pt", 96,
                "config-interval", 1,  // Frequent SPS/PPS for new joiners
                "mtu", 1200,          // Smaller MTU to reduce fragmentation
                NULL);
    
    // Multicast UDP sink
    data->sink = gst_element_factory_make("multiudpsink", "multicast-sink");
    gchar *clients = g_strdup_printf("%s:%d", data->multicast_group, data->port);
    g_object_set(data->sink,
                "clients", clients,
                "ttl-mc", data->ttl,
                "loop", FALSE,        // Don't loop back to sender
                "sync", FALSE,
                "async", FALSE,
                NULL);
    g_free(clients);
    
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
    
    // Set caps and link elements
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
    
    g_print("✅ Multicast pipeline created successfully\n");
    return TRUE;
}

static void print_usage(const char *program_name) {
    g_print("UDP Multicast Video Sender\n");
    g_print("==========================\n\n");
    g_print("Usage: %s [options] [multicast_group] [port] [ttl]\n\n", program_name);
    g_print("Options:\n");
    g_print("  -c, --camera     Use camera instead of test source\n");
    g_print("  -h, --help       Show this help message\n\n");
    g_print("Arguments:\n");
    g_print("  multicast_group  Multicast IP address (default: 224.1.1.1)\n");
    g_print("  port             UDP port (default: 5000)\n");
    g_print("  ttl              Time To Live for multicast packets (default: 5)\n\n");
    g_print("Examples:\n");
    g_print("  %s                          # Send to 224.1.1.1:5000 with test source\n", program_name);
    g_print("  %s -c                       # Send with camera\n", program_name);
    g_print("  %s 224.2.2.2 5001 10        # Custom multicast group, port, and TTL\n", program_name);
    g_print("  %s -c 239.255.1.1 5000      # Camera to specific multicast group\n\n", program_name);
    g_print("Multicast Guidelines:\n");
    g_print("  • Use 224.0.0.0 - 239.255.255.255 range for multicast addresses\n");
    g_print("  • TTL determines how many network hops the packets can travel\n");
    g_print("  • Lower TTL (1-5) for local network, higher (>10) for wide area\n");
    g_print("  • Multiple receivers can join the same multicast group\n\n");
    g_print("Receiver command:\n");
    g_print("  gst-launch-1.0 udpsrc multicast-group=GROUP port=PORT ! application/x-rtp ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink\n");
}

int main(int argc, char *argv[]) {
    UDPMulticastSenderData data;
    GstStateChangeReturn ret;
    int arg_index = 1;
    
    // Initialize structure
    memset(&data, 0, sizeof(UDPMulticastSenderData));
    data.multicast_group = "224.1.1.1";  // default multicast group
    data.port = 5000;                     // default port
    data.ttl = 5;                         // default TTL
    data.use_camera = FALSE;
    
    // Parse command line arguments
    while (arg_index < argc) {
        if (strcmp(argv[arg_index], "-c") == 0 || strcmp(argv[arg_index], "--camera") == 0) {
            data.use_camera = TRUE;
            arg_index++;
        } else if (strcmp(argv[arg_index], "-h") == 0 || strcmp(argv[arg_index], "--help") == 0) {
            print_usage("udp_multicast_sender");
            return 0;
        } else {
            break;  // Non-option argument found
        }
    }
    
    // Parse multicast group, port, and TTL
    if (arg_index < argc) {
        data.multicast_group = argv[arg_index++];
        if (!is_multicast_address(data.multicast_group)) {
            g_printerr("❌ Invalid multicast address: %s\n", data.multicast_group);
            g_printerr("   Multicast range: 224.0.0.0 - 239.255.255.255\n");
            return -1;
        }
    }
    if (arg_index < argc) {
        data.port = atoi(argv[arg_index++]);
        if (data.port <= 0 || data.port > 65535) {
            g_printerr("❌ Invalid port number: %d\n", data.port);
            return -1;
        }
    }
    if (arg_index < argc) {
        data.ttl = atoi(argv[arg_index++]);
        if (data.ttl < 1 || data.ttl > 255) {
            g_printerr("❌ Invalid TTL value: %d (must be 1-255)\n", data.ttl);
            return -1;
        }
    }
    
    // Initialize GStreamer
    gst_init(&argc, &argv);
    
    g_print("🌐 UDP Multicast Video Sender\n");
    g_print("==============================\n");
    g_print("📍 Multicast Group: %s:%d\n", data.multicast_group, data.port);
    g_print("🌍 TTL: %d hops\n", data.ttl);
    g_print("📹 Source: %s\n", data.use_camera ? "Camera" : "Test Pattern");
    g_print("⚙️  Encoder: H.264 (ultrafast, multicast-optimized)\n");
    g_print("👥 Allows multiple simultaneous receivers\n\n");
    
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
    g_print("▶️  Starting multicast pipeline...\n");
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
    
    g_print("👋 UDP Multicast Sender stopped.\n");
    return 0;
} 