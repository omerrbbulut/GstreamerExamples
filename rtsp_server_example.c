#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <glib.h>

// GStreamer RTSP Server Example
// Bu örnek bir test pattern'i RTSP üzerinden yayınlar

int main(int argc, char *argv[]) {
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    
    // GStreamer'ı başlat
    gst_init(&argc, &argv);
    
    g_print("🚀 GStreamer RTSP Server Starting...\n");
    
    // Main loop oluştur
    loop = g_main_loop_new(NULL, FALSE);
    
    // RTSP server oluştur
    server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, "8554"); // Port 8554
    
    // Mount points al
    mounts = gst_rtsp_server_get_mount_points(server);
    
    // Media factory oluştur
    factory = gst_rtsp_media_factory_new();
    
    // Pipeline string - test pattern yayını
    gst_rtsp_media_factory_set_launch(factory,
        "( "
        "videotestsrc pattern=ball is-live=true ! "
        "video/x-raw,width=640,height=480,framerate=30/1 ! "
        "videoconvert ! "
        "x264enc tune=zerolatency bitrate=2000 speed-preset=ultrafast ! "
        "video/x-h264,profile=baseline ! "
        "rtph264pay name=pay0 pt=96 "
        ")");
    
    // Shared media olarak ayarla (birden fazla client)
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    
    // Factory'yi "/test" mount point'ine bağla
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    
    // Mounts nesnesini serbest bırak
    g_object_unref(mounts);
    
    // Server'ı default context'e bağla
    gst_rtsp_server_attach(server, NULL);
    
    g_print("✅ RTSP Server is ready!\n");
    g_print("📡 Stream URL: rtsp://localhost:8554/test\n");
    g_print("🎬 Test with: vlc rtsp://localhost:8554/test\n");
    g_print("⏹️  Press Ctrl+C to stop\n\n");
    
    // Main loop çalıştır
    g_main_loop_run(loop);
    
    // Cleanup
    g_main_loop_unref(loop);
    g_object_unref(server);
    
    g_print("👋 RTSP Server stopped\n");
    return 0;
} 