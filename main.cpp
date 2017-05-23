#include <iostream>
#include <gst/gst.h>
#include <gst/sdp/gstsdpmessage.h>
#include <gst/rtsp/gstrtspmessage.h>
#include <gst/net/gstnet.h>
#include <syslog.h>
#include <iomanip>
#include <sstream>
#include <thread>
#include <getopt.h>
#include <cstring>

#define	G_CALLBACK(f)			 ((GCallback) (f))

int count = 0;

bool SetState(GstElement *pipeline, GstState state)
{
    GstStateChangeReturn ret = gst_element_set_state(pipeline, state);
    if (ret == GST_STATE_CHANGE_SUCCESS)
        return true;
    if (ret == GST_STATE_CHANGE_FAILURE)
        return false;
    if (ret == GST_STATE_CHANGE_NO_PREROLL)
    {
        return false;
    }

    if (ret == GST_STATE_CHANGE_ASYNC)
    {
        GstState curstate;
        GstClockTime timeout = GST_SECOND * 15;
        int retries = 1;

        for (int i = 0; i < retries; i++)
        {
            ret = gst_element_get_state(pipeline, &curstate, NULL, timeout);
            if (ret == GST_STATE_CHANGE_SUCCESS)
                return true;
            if (ret == GST_STATE_CHANGE_FAILURE)
                return false;

        }

        return false;
    }
    return false;
}

bool WaitForEos(GstElement *pipeline, GstBus *bus)
{
    GstClockTime timeout = GST_SECOND; // 1 Seconds
    struct timespec started;

    if (clock_gettime(CLOCK_MONOTONIC, &started) < 0)
        abort();

    GstMessage *msg = gst_bus_timed_pop(bus, timeout); // We have to poll because this GStreamer function sometimes never returns
    if (msg == NULL)
    {
        GstState state;
        if (gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE)
            != GST_STATE_CHANGE_SUCCESS)
            return true;
        if (state != GST_STATE_PLAYING)
        {
            return true;
        }

        /* Sometimes gst_bus_timed_pop gets into a state where it constantly wakes up
           Deal with this to prevent 100% cpu being used */
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
            abort();
        GstClockTime gstarted = GST_TIMESPEC_TO_TIME(started);
        GstClockTime gnow = GST_TIMESPEC_TO_TIME(now);
        GstClockTime gdiff = GST_CLOCK_DIFF(gstarted, gnow);
        if (gdiff < timeout)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(gdiff)); // Sleep for remaining time
        }

        return false;
    }

    bool Flag = false;

    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
            return true;
            break;
        case GST_MESSAGE_INFO:
        {
            GError *info = NULL; // info to show to users
            gchar *dbg = NULL; // additional debug string for developers
            gst_message_parse_info(msg, &info, &dbg);
            g_error_free(info);
            g_free(dbg);
            break;
        }
        case GST_MESSAGE_WARNING:
        {
            GError *warn = NULL; // info to show to users
            gchar *dbg = NULL; // additional debug string for developers
            gst_message_parse_warning(msg, &warn, &dbg);
            g_error_free(warn);
            g_free(dbg);
            break;
        }
        case GST_MESSAGE_ERROR:
        {
            GError *err = NULL; // error to show to users
            gchar *dbg = NULL; // additional debug string for developers
            gst_message_parse_error(msg, &err, &dbg);
            g_error_free(err);
            g_free(dbg);
            return true;
            break;
        }

            // Suppress Noise
        case GST_MESSAGE_STATE_CHANGED:
            break;

        case GST_MESSAGE_STREAM_START:
        case GST_MESSAGE_ELEMENT:
        case GST_MESSAGE_STREAM_STATUS:
        case GST_MESSAGE_ASYNC_START:
        case GST_MESSAGE_ASYNC_DONE:
        case GST_MESSAGE_NEW_CLOCK:
        case GST_MESSAGE_PROGRESS:
            break;
        default:
            break;
    }
    gst_message_unref(msg);
    return false;
}

// A simple selection based on the global variable "count", as select_palette is called for each stream.
gboolean select_palette(GstElement *rtspsrc, guint num,
                        GstCaps *caps, gpointer user_data)
{
    int streamIndex = 2; // Selecting any stream other than the 1st one breaks, when TCP
    bool selected = false;
    if (count == streamIndex) {
        printf("Selecting stream %d \n ", count);
        selected = true;
    }
    count++;
    return selected;
}

int main(int argc, char **argv) {
    bool callBackFlag = false;
    int c;
    std::string pipe;

    while ((c = getopt(argc, argv, "ct:")) != -1) {
        switch (c) {
            case 'c':
                callBackFlag = true;
                break;
            case 't':
                if (strcmp(optarg, "tcp") == 0) {  // Suspected bug exists for TCP, protocol set to 4 to force TCP.
                    pipe = "rtspsrc name=rtspsrc0 location=\"rtsp://127.0.0.1:8554/test\" protocols=4 ! decodebin ! autovideosink";
                    break;
                }
                else if (strcmp(optarg, "udp") == 0) {
                    pipe = "rtspsrc name=rtspsrc0 location=\"rtsp://127.0.0.1:8554/test\" protocols=7 ! decodebin ! autovideosink";
                    break;
                }
                printf("Unknown transport, -t takes either \"tcp\" or \"udp\" as options");
                abort();
                break;
            default:
                abort();
        }
    }
    gst_init(&argc, &argv);
    char* bug = argv[1];
    GstElement *pipeline = NULL;
    GError *error = NULL;
    std::string bugString = "bug";

    pipeline = gst_parse_launch(pipe.c_str(), &error);
    gpointer *palette;
    std::string ss = "rtspsrc0";
    GstElement *rtspsrc = gst_bin_get_by_name((GstBin *) pipeline, ss.c_str());

    if (callBackFlag) {  // Suspected bug exists for TCP when callback is called.
        g_signal_connect(rtspsrc, "select-stream",
                         G_CALLBACK(select_palette),
                         palette);
    }

    if (SetState(pipeline, GST_STATE_PLAYING) == true)
    {
        GstBus *bus = gst_element_get_bus(pipeline);
        while (WaitForEos(pipeline, bus) == false)
        {
            // Idle Loop
        }
        gst_object_unref(bus);
        std::cout << "PASSED!" << std::endl;
    }
    else
    {
        std::cout << "FAILED!" << std::endl;
    }

    std::cout << "Finished!" << std::endl;
    return 0;
}