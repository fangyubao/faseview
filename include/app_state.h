#pragma once

#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>

typedef struct {
    IplImage *image_bgr;
    IplImage *gray;
    CvMat *gx;
    CvMat *gy;
    CvPoint2D32f *corners;
    int corner_count;
} ImageData;

typedef struct {
    CvPoint pos;
    double angle;
    double cosine;
    double distance;
} EdgeHit;

typedef struct {
    CvPoint pos;
    int enabled;
} MarkerPoint;

typedef struct {
    const char *window_name;
    ImageData image;
    double zoom;
    double pan_x;
    double pan_y;
    double pan_anchor_x;
    double pan_anchor_y;
    CvPoint start_raw;
    CvPoint drag_anchor_mouse;
    CvPoint last_mouse;
    CvPoint pan_anchor_mouse;
    int shift_lock_axis;
    int last_flags;
    int window_width;
    int window_height;
    int is_drawing;
    int is_panning;
    MarkerPoint markers[10];
    int marker_count;
    int ui_dirty;
    int screenshot_index;
    char screenshot_dir[1024];
    int pending_open;
    char pending_open_path[1024];
} AppState;
