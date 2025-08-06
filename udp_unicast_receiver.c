/*
 * UDP Unicast Receiver Example
 * 
 * This example demonstrates how to receive video over UDP unicast using GStreamer.
 * It creates a simple pipeline that receives RTP video packets via UDP and displays them.
 * 
 * Pipeline: udpsrc -> rtp -> decoder -> videosink
 * 
 * Usage: ./udp_unicast_receiver [port]
 * Example: ./udp_unicast_receiver 5000
 * 
 * Compile: gcc -o udp_unicast_receiver udp_unicast_receiver.c `pkg-config --cflags --libs gstreamer-1.0`
 */

#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// Global main loop for signal handling
static GMainLoop *global_main_loop = NULL;

typedef struct {
    GstElement *pipeline;
    GstElement *source;
    GstElement *depayloader;
    GstElement *parser;
    GstElement *decoder;
    GstElement *convert;
    GstElement *sink;
    GMainLoop *loop;
    int port;
    gboolean save_to_file;
    char *output_file;
} UDPReceiverData;

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    (void)bus; // Suppress unused parameter warning
    UDPReceiverData *receiver_data = (UDPReceiverData*)data;
    
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
                    g_print("🎯 UDP Unicast receiver started!\n");
                    g_print("📡 Listening on port: %d\n", receiver_data->port);
                    if (receiver_data->save_to_file) {
                        g_print("💾 Recording to: %s\n", receiver_data->output_file);
                    } else {
                        g_print("🖥️  Displaying video on screen\n");
                    }
                    g_print("📊 Press Ctrl+C to stop\n");
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
    g_print("\n🛑 Received signal %d, stopping...\n", sig);
    if (global_main_loop) {
        g_main_loop_quit(global_main_loop);
    }
}

static gboolean setup_pipeline(UDPReceiverData *data) {
    GstBus *bus;
    GstCaps *caps;
    
    // Create pipeline
    data->pipeline = gst_pipeline_new("udp-receiver");
    
    // UDP source
    data->source = gst_element_factory_make("udpsrc", "udp-source");
    g_object_set(data->source,
                "port", data->port,
                "caps", gst_caps_from_string("application/x-rtp, payload=96"),
                NULL);
    
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
    
    g_print("✅ Pipeline created successfully\n");
    return TRUE;
}

static void print_usage(const char *program_name) {
    g_print("UDP Unicast Video Receiver\n");
    g_print("==========================\n\n");
    g_print("Usage: %s [options] [port]\n\n", program_name);
    g_print("Options:\n");
    g_print("  -f, --file FILE  Save received video to file instead of displaying\n");
    g_print("  -h, --help       Show this help message\n\n");
    g_print("Arguments:\n");
    g_print("  port             UDP port to listen on (default: 5000)\n\n");
    g_print("Examples:\n");
    g_print("  %s                          # Receive on port 5000 and display\n", program_name);
    g_print("  %s 5001                     # Receive on port 5001 and display\n", program_name);
    g_print("  %s -f output.mp4 5000       # Receive and save to file\n", program_name);
    g_print("  %s --file stream.mp4        # Save to file (default port 5000)\n\n", program_name);
    g_print("Sender command:\n");
    g_print("  gst-launch-1.0 videotestsrc ! x264enc tune=zerolatency ! rtph264pay ! udpsink host=127.0.0.1 port=PORT\n");
}

int main(int argc, char *argv[]) {
    UDPReceiverData data;
    GstStateChangeReturn ret;
    int arg_index = 1;
    
    // Initialize structure
    memset(&data, 0, sizeof(UDPReceiverData));
    data.port = 5000;         // default port
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
            print_usage("udp_unicast_receiver");
            return 0;
        } else {
            break;  // Non-option argument found
        }
    }
    
    // Parse port
    if (arg_index < argc) {
        data.port = atoi(argv[arg_index++]);
        if (data.port <= 0 || data.port > 65535) {
            g_printerr("❌ Invalid port number: %d\n", data.port);
            return -1;
        }
    }
    
    // Initialize GStreamer
    gst_init(&argc, &argv);
    
    g_print("📺 UDP Unicast Video Receiver\n");
    g_print("==============================\n");
    g_print("🔊 Port: %d\n", data.port);
    if (data.save_to_file) {
        g_print("💾 Output: %s\n", data.output_file);
    } else {
        g_print("🖥️  Output: Display\n");
    }
    g_print("⚙️  Decoder: H.264\n\n");
    
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
    global_main_loop = data.loop;  // Set global reference for signal handler
    
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
    
    g_print("👋 UDP Unicast Receiver stopped.\n");
    return 0;
} 