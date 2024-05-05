#ifndef RTSPSERVER_H
#define RTSPSERVER_H

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

class RTSPServer {
public:
    RTSPServer();
    ~RTSPServer();
    void run();

private:
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
};

#endif // RTSPSERVER_H

