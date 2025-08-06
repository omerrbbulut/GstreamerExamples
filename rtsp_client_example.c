#include <gst/gst.h>
#include <glib.h>

// GStreamer RTSP Client Example
// RTSP stream'i alıp ekranda gösterir

typedef struct {
    GstElement *pipeline;
    GMainLoop *loop;
} AppData;

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    AppData *app = (AppData*)data;
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("📺 End of stream\n");
            g_main_loop_quit(app->loop);
            break;
            
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            
            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);
            
            g_printerr("❌ Error: %s\n", error->message);
            g_error_free(error);
            
            g_main_loop_quit(app->loop);
            break;
        }
        
        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
            
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(app->pipeline)) {
                g_print("🔄 Pipeline state changed from %s to %s\n",
                    gst_element_state_get_name(old_state),
                    gst_element_state_get_name(new_state));
            }
            break;
        }
        
        default:
            break;
    }
    
    return TRUE;
}

int main(int argc, char *argv[]) {
    AppData app;
    GstBus *bus;
    guint bus_watch_id;
    
    // GStreamer'ı başlat
    gst_init(&argc, &argv);
    
    g_print("🎬 GStreamer RTSP Client Starting...\n");
    
    // Main loop oluştur
    app.loop = g_main_loop_new(NULL, FALSE);
    
    // RTSP URL (varsayılan)
    const char *rtsp_url = "rtsp://localhost:8554/test";
    
    // Komut satırından URL al
    if (argc > 1) {
        rtsp_url = argv[1];
    }
    
    g_print("📡 Connecting to: %s\n", rtsp_url);
    
    // Pipeline oluştur - RTSP'den video al ve görüntüle
    gchar *pipeline_desc = g_strdup_printf(
        "rtspsrc location=%s latency=100 ! "
        "rtph264depay ! "
        "h264parse ! "
        "avdec_h264 ! "
        "videoconvert ! "
        "autovideosink",
        rtsp_url);
    
    app.pipeline = gst_parse_launch(pipeline_desc, NULL);
    g_free(pipeline_desc);
    
    if (!app.pipeline) {
        g_printerr("❌ Failed to create pipeline\n");
        return -1;
    }
    
    // Bus mesajlarını dinle
    bus = gst_pipeline_get_bus(GST_PIPELINE(app.pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, &app);
    gst_object_unref(bus);
    
    // Pipeline'ı başlat
    g_print("🚀 Starting playback...\n");
    gst_element_set_state(app.pipeline, GST_STATE_PLAYING);
    
    g_print("✅ Client is ready!\n");
    g_print("⏹️  Press Ctrl+C to stop\n\n");
    
    // Main loop çalıştır
    g_main_loop_run(app.loop);
    
    // Cleanup
    g_print("🛑 Stopping...\n");
    gst_element_set_state(app.pipeline, GST_STATE_NULL);
    gst_object_unref(app.pipeline);
    g_source_remove(bus_watch_id);
    g_main_loop_unref(app.loop);
    
    g_print("👋 Client stopped\n");
    return 0;
} 