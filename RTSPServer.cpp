#include "RTSPServer.h"
#include <iostream>

RTSPServer::RTSPServer() {
    gst_init(nullptr, nullptr);

    // Create the main loop
    loop = g_main_loop_new(nullptr, FALSE);

    // Create and configure the RTSP server
    server = gst_rtsp_server_new();
    mounts = gst_rtsp_server_get_mount_points(server);

    // Create and configure the media factory
    factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory, "( videotestsrc ! x264enc ! rtph264pay name=pay0 pt=96 )");

    // Mount the media factory at the "/test" endpoint
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    // Attach the server to the default main context
    gst_rtsp_server_attach(server, nullptr);

    std::cout << "RTSP server is running on port 8554" << std::endl;
}

RTSPServer::~RTSPServer() {
    g_main_loop_unref(loop);
    g_object_unref(server);
    g_object_unref(factory);
}

void RTSPServer::run() {
    g_main_loop_run(loop);
}

