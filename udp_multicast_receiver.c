/*
 * UDP Multicast Receiver Example
 * 
 * This example demonstrates how to receive video over UDP multicast using GStreamer.
 * It joins a multicast group and receives RTP video packets, allowing multiple 
 * receivers to simultaneously receive the same stream from a multicast sender.
 * 
 * Pipeline: udpsrc(multicast) -> rtp -> decoder -> videosink
 * 
 * Usage: ./udp_multicast_receiver [multicast_group] [port] [interface]
 * Example: ./udp_multicast_receiver 224.1.1.1 5000 eth0
 * 
 * Compile: gcc -o udp_multicast_receiver udp_multicast_receiver.c `pkg-config --cflags --libs gstreamer-1.0`
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
    GstElement *depayloader;
    GstElement *parser;
    GstElement *decoder;
    GstElement *convert;
    GstElement *sink;
    GMainLoop *loop;
    char *multicast_group;
    int port;
    char *interface;
    gboolean save_to_file;
    char *output_file;
} UDPMulticastReceiverData;

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
    UDPMulticastReceiverData *receiver_data = (UDPMulticastReceiverData*)data;
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(receiver_data->loop);
            break;
            
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            
            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);
            
            g_printerr("Error: %s\n", error->message);
            g_error_free(error);
            
            g_main_loop_quit(receiver_data->loop);
            break;
        }
        
        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state;
            
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(receiver_data->pipeline)) {
                gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
                g_print("Pipeline state changed from %s to %s\n",
                    gst_element_state_get_name(old_state),
                    gst_element_state_get_name(new_state));
                    
                if (new_state == GST_STATE_PLAYING) {
                    g_print("🌐 UDP Multicast receiver started!\n");
                    g_print("📡 Joined multicast group: %s:%d\n", 
                           receiver_data->multicast_group, receiver_data->port);
                    if (receiver_data->interface) {
                        g_print("🌐 Using interface: %s\n", receiver_data->interface);
                    }
                    if (receiver_data->save_to_file) {
                        g_print("💾 Recording to: %s\n", receiver_data->output_file);
                    } else {
                        g_print("🖥️  Displaying video on screen\n");
                    }
                    g_print("👥 Multiple receivers can join the same group\n");
                    g_print("📊 Press Ctrl+C to leave multicast group and stop\n");
                }
            }
            break;
        }
        
        case GST_MESSAGE_WARNING: {
            gchar *debug;
            GError *warning;
            
            gst_message_parse_warning(msg, &warning, &debug);
            g_print("Warning: %s\n", warning->message);
            g_free(debug);
            g_error_free(warning);
            break;
        }
        
        default:
            break;
    }
    
    return TRUE;
}

static void signal_handler(int sig) {
    g_print("\n🛑 Received signal %d, leaving multicast group...\n", sig);
}

static gboolean setup_pipeline(UDPMulticastReceiverData *data) {
    GstBus *bus;
    
    // Create pipeline
    data->pipeline = gst_pipeline_new("multicast-receiver");
    
    // UDP source with multicast support
    data->source = gst_element_factory_make("udpsrc", "udp-source");
    g_object_set(data->source,
                "port", data->port,
                "multicast-group", data->multicast_group,
                "auto-multicast", TRUE,
                "caps", gst_caps_from_string("application/x-rtp, payload=96"),
                NULL);
    
    // Set network interface if specified
    if (data->interface && strlen(data->interface) > 0) {
        g_object_set(data->source, "multicast-iface", data->interface, NULL);
    }
    
    // RTP depayloader
    data->depayloader = gst_element_factory_make("rtph264depay", "depayloader");
    
    // H.264 parser
    data->parser = gst_element_factory_make("h264parse", "parser");
    
    // Video decoder
    data->decoder = gst_element_factory_make("avdec_h264", "decoder");
    
    // Video converter
    data->convert = gst_element_factory_make("videoconvert", "convert");
    
    if (data->save_to_file) {
        // File sink for recording
        data->sink = gst_element_factory_make("filesink", "file-sink");
        g_object_set(data->sink,
                    "location", data->output_file,
                    NULL);
        
        // For file output, we need an encoder and muxer
        GstElement *encoder = gst_element_factory_make("x264enc", "file-encoder");
        GstElement *muxer = gst_element_factory_make("mp4mux", "muxer");
        
        if (!encoder || !muxer) {
            g_printerr("❌ Could not create encoder or muxer for file output.\n");
            return FALSE;
        }
        
        g_object_set(encoder,
                    "speed-preset", 2,  // fast
                    "tune", 0x01,       // film
                    NULL);
        
        gst_bin_add_many(GST_BIN(data->pipeline), encoder, muxer, NULL);
        
        // Check if all elements were created
        if (!data->pipeline || !data->source || !data->depayloader || 
            !data->parser || !data->decoder || !data->convert || !data->sink) {
            g_printerr("❌ Not all elements could be created.\n");
            return FALSE;
        }
        
        // Add elements to pipeline
        gst_bin_add_many(GST_BIN(data->pipeline),
                         data->source, data->depayloader, data->parser, 
                         data->decoder, data->convert, encoder, muxer, data->sink,
                         NULL);
        
        // Link elements: source -> depay -> parse -> decode -> convert -> encode -> mux -> filesink
        if (!gst_element_link(data->source, data->depayloader) ||
            !gst_element_link(data->depayloader, data->parser) ||
            !gst_element_link(data->parser, data->decoder) ||
            !gst_element_link(data->decoder, data->convert) ||
            !gst_element_link(data->convert, encoder) ||
            !gst_element_link(encoder, muxer) ||
            !gst_element_link(muxer, data->sink)) {
            g_printerr("❌ Elements could not be linked.\n");
            return FALSE;
        }
        
    } else {
        // Video sink for display
        data->sink = gst_element_factory_make("autovideosink", "video-sink");
        g_object_set(data->sink,
                    "sync", FALSE,
                    NULL);
        
        // Check if all elements were created
        if (!data->pipeline || !data->source || !data->depayloader || 
            !data->parser || !data->decoder || !data->convert || !data->sink) {
            g_printerr("❌ Not all elements could be created.\n");
            return FALSE;
        }
        
        // Add elements to pipeline
        gst_bin_add_many(GST_BIN(data->pipeline),
                         data->source, data->depayloader, data->parser, 
                         data->decoder, data->convert, data->sink,
                         NULL);
        
        // Link elements: source -> depay -> parse -> decode -> convert -> videosink
        if (!gst_element_link(data->source, data->depayloader) ||
            !gst_element_link(data->depayloader, data->parser) ||
            !gst_element_link(data->parser, data->decoder) ||
            !gst_element_link(data->decoder, data->convert) ||
            !gst_element_link(data->convert, data->sink)) {
            g_printerr("❌ Elements could not be linked.\n");
            return FALSE;
        }
    }
    
    // Add message handler
    bus = gst_element_get_bus(data->pipeline);
    gst_bus_add_watch(bus, bus_call, data);
    gst_object_unref(bus);
    
    g_print("✅ Multicast receiver pipeline created successfully\n");
    return TRUE;
}

static void print_usage(const char *program_name) {
    g_print("UDP Multicast Video Receiver\n");
    g_print("============================\n\n");
    g_print("Usage: %s [options] [multicast_group] [port] [interface]\n\n", program_name);
    g_print("Options:\n");
    g_print("  -f, --file FILE  Save received video to file instead of displaying\n");
    g_print("  -h, --help       Show this help message\n\n");
    g_print("Arguments:\n");
    g_print("  multicast_group  Multicast IP address (default: 224.1.1.1)\n");
    g_print("  port             UDP port (default: 5000)\n");
    g_print("  interface        Network interface (optional, e.g., eth0, wlan0)\n\n");
    g_print("Examples:\n");
    g_print("  %s                          # Join 224.1.1.1:5000 and display\n", program_name);
    g_print("  %s 224.2.2.2 5001           # Join specific multicast group\n", program_name);
    g_print("  %s -f output.mp4            # Save to file (default group)\n", program_name);
    g_print("  %s 224.1.1.1 5000 eth0      # Use specific network interface\n", program_name);
    g_print("  %s -f stream.mp4 224.1.1.1 5000 wlan0  # Save to file with interface\n\n", program_name);
    g_print("Multicast Guidelines:\n");
    g_print("  • Multiple receivers can join the same multicast group\n");
    g_print("  • Specify network interface if you have multiple NICs\n");
    g_print("  • Ensure multicast routing is enabled on your network\n");
    g_print("  • Firewall may need to allow multicast traffic\n\n");
    g_print("Sender command:\n");
    g_print("  gst-launch-1.0 videotestsrc ! x264enc ! rtph264pay ! multiudpsink clients=GROUP:PORT\n");
    g_print("\nNetwork troubleshooting:\n");
    g_print("  • Check multicast routes: ip route show | grep 224\n");
    g_print("  • Monitor multicast traffic: tcpdump -i INTERFACE host GROUP\n");
    g_print("  • List network interfaces: ip addr show\n");
}

int main(int argc, char *argv[]) {
    UDPMulticastReceiverData data;
    GstStateChangeReturn ret;
    int arg_index = 1;
    
    // Initialize structure
    memset(&data, 0, sizeof(UDPMulticastReceiverData));
    data.multicast_group = "224.1.1.1";  // default multicast group
    data.port = 5000;                     // default port
    data.interface = NULL;                // auto-detect interface
    data.save_to_file = FALSE;
    data.output_file = NULL;
    
    // Parse command line arguments
    while (arg_index < argc) {
        if (strcmp(argv[arg_index], "-f") == 0 || strcmp(argv[arg_index], "--file") == 0) {
            if (arg_index + 1 >= argc) {
                g_printerr("❌ Option %s requires a filename argument.\n", argv[arg_index]);
                return -1;
            }
            data.save_to_file = TRUE;
            data.output_file = argv[++arg_index];
            arg_index++;
        } else if (strcmp(argv[arg_index], "-h") == 0 || strcmp(argv[arg_index], "--help") == 0) {
            print_usage("udp_multicast_receiver");
            return 0;
        } else {
            break;  // Non-option argument found
        }
    }
    
    // Parse multicast group, port, and interface
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
        data.interface = argv[arg_index++];
    }
    
    // Initialize GStreamer
    gst_init(&argc, &argv);
    
    g_print("🌐 UDP Multicast Video Receiver\n");
    g_print("================================\n");
    g_print("📍 Multicast Group: %s:%d\n", data.multicast_group, data.port);
    if (data.interface) {
        g_print("🌐 Interface: %s\n", data.interface);
    } else {
        g_print("🌐 Interface: Auto-detect\n");
    }
    if (data.save_to_file) {
        g_print("💾 Output: %s\n", data.output_file);
    } else {
        g_print("🖥️  Output: Display\n");
    }
    g_print("⚙️  Decoder: H.264\n");
    g_print("👥 Joining multicast group...\n\n");
    
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
    g_print("▶️  Starting multicast receiver...\n");
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("❌ Unable to set the pipeline to the playing state.\n");
        g_printerr("   Common issues:\n");
        g_printerr("   • Multicast not enabled on network interface\n");
        g_printerr("   • Firewall blocking multicast traffic\n");
        g_printerr("   • No multicast sender active\n");
        gst_object_unref(data.pipeline);
        g_main_loop_unref(data.loop);
        return -1;
    }
    
    // Run main loop
    g_main_loop_run(data.loop);
    
    // Cleanup
    g_print("🧹 Leaving multicast group and cleaning up...\n");
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    g_main_loop_unref(data.loop);
    
    g_print("👋 UDP Multicast Receiver stopped.\n");
    return 0;
} 