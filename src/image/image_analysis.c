#include "image_analysis.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <opencv2/imgproc/imgproc_c.h>

#define MAX_CORNERS 1000
#define CORNER_QUALITY 0.01
#define CORNER_MIN_DISTANCE 10.0
#define EDGE_MAG_THRESHOLD 50.0
#define EDGE_DOT_WINDOW 10
#define EDGE_MIN_DISTANCE 25.0
#define LINE_SCAN_DENSITY 2.0
#define MIN_LINE_SCAN_DISTANCE 5.0
#define EDGE_MIN_LABEL_ANGLE 1.0

static double point_distance(CvPoint a, CvPoint b) {
    const double dx = (double) a.x - (double) b.x;
    const double dy = (double) a.y - (double) b.y;
    return sqrt(dx * dx + dy * dy);
}

void release_image_data(ImageData *data) {
    if (data == 0) {
        return;
    }
    if (data->image_bgr != 0) {
        cvReleaseImage(&data->image_bgr);
    }
    if (data->gray != 0) {
        cvReleaseImage(&data->gray);
    }
    if (data->gx != 0) {
        cvReleaseMat(&data->gx);
    }
    if (data->gy != 0) {
        cvReleaseMat(&data->gy);
    }
    free(data->corners);
    memset(data, 0, sizeof(*data));
}

int analyze_image(const IplImage *source, ImageData *out) {
    IplImage *gray = 0;
    IplImage *blurred = 0;
    IplImage *eig = 0;
    IplImage *temp = 0;
    CvMat *gx = 0;
    CvMat *gy = 0;
    CvPoint2D32f *corners = 0;
    int corner_count = MAX_CORNERS;

    memset(out, 0, sizeof(*out));
    out->image_bgr = cvCloneImage(source);
    gray = cvCreateImage(cvGetSize(source), IPL_DEPTH_8U, 1);
    blurred = cvCreateImage(cvGetSize(source), IPL_DEPTH_8U, 1);
    eig = cvCreateImage(cvGetSize(source), IPL_DEPTH_32F, 1);
    temp = cvCreateImage(cvGetSize(source), IPL_DEPTH_32F, 1);
    gx = cvCreateMat(source->height, source->width, CV_64FC1);
    gy = cvCreateMat(source->height, source->width, CV_64FC1);
    corners = (CvPoint2D32f *) calloc((size_t) MAX_CORNERS, sizeof(CvPoint2D32f));

    if (out->image_bgr == 0 || gray == 0 || blurred == 0 || eig == 0 || temp == 0 || gx == 0 || gy == 0 || corners == 0) {
        cvReleaseImage(&gray);
        cvReleaseImage(&blurred);
        cvReleaseImage(&eig);
        cvReleaseImage(&temp);
        if (gx != 0) {
            cvReleaseMat(&gx);
        }
        if (gy != 0) {
            cvReleaseMat(&gy);
        }
        free(corners);
        release_image_data(out);
        return 0;
    }

    cvCvtColor(source, gray, CV_BGR2GRAY);
    cvSmooth(gray, blurred, CV_GAUSSIAN, 3, 3, 0.0, 0.0);
    cvSobel(blurred, gx, 1, 0, -1);
    cvSobel(blurred, gy, 0, 1, -1);
    cvGoodFeaturesToTrack(gray, eig, temp, corners, &corner_count, CORNER_QUALITY, CORNER_MIN_DISTANCE, 0, 3, 0, 0.04);

    out->gray = gray;
    out->gx = gx;
    out->gy = gy;
    out->corners = corners;
    out->corner_count = corner_count;

    cvReleaseImage(&blurred);
    cvReleaseImage(&eig);
    cvReleaseImage(&temp);
    return 1;
}

CvPoint get_raw_point(const AppState *state, CvPoint screen_point) {
    CvPoint raw;
    raw.x = (int) floor((((double) screen_point.x) - state->pan_x) / state->zoom + 0.5);
    raw.y = (int) floor((((double) screen_point.y) - state->pan_y) / state->zoom + 0.5);
    return raw;
}

int find_nearest_corner(const ImageData *data, CvPoint raw_point, CvPoint *snapped_point, double threshold) {
    double best = threshold;
    int found = 0;
    int i;

    for (i = 0; i < data->corner_count; ++i) {
        CvPoint candidate;
        double dist;
        candidate.x = (int) floor((double) data->corners[i].x + 0.5);
        candidate.y = (int) floor((double) data->corners[i].y + 0.5);
        dist = point_distance(candidate, raw_point);
        if (dist < best) {
            best = dist;
            *snapped_point = candidate;
            found = 1;
        }
    }

    return found;
}

CvPoint get_active_raw_point(const AppState *state, CvPoint screen_point, int ctrl_pressed, int shift_pressed) {
    CvPoint active = get_raw_point(state, screen_point);

    if (ctrl_pressed) {
        CvPoint snapped;
        if (find_nearest_corner(&state->image, active, &snapped, 40.0)) {
            active = snapped;
        }
    }

    if (shift_pressed && state->is_drawing && state->start_raw.x >= 0 && state->start_raw.y >= 0) {
        const int dx = abs(active.x - state->start_raw.x);
        const int dy = abs(active.y - state->start_raw.y);
        if (dx > dy) {
            active.y = state->start_raw.y;
        } else {
            active.x = state->start_raw.x;
        }
    }

    return active;
}

int collect_edge_hits(const AppState *state, CvPoint p1, CvPoint p2, EdgeHit **hits_out, int *hit_count_out) {
    const double dx = (double) p2.x - (double) p1.x;
    const double dy = (double) p2.y - (double) p1.y;
    const double line_len = sqrt(dx * dx + dy * dy);
    const double line_nx = dx / (line_len > 0.0 ? line_len : 1.0);
    const double line_ny = dy / (line_len > 0.0 ? line_len : 1.0);
    const int steps = (int) (line_len * LINE_SCAN_DENSITY);
    EdgeHit *candidates = 0;
    EdgeHit *hits = 0;
    int candidate_count = 0;
    int hit_count = 0;
    int i;
    double last_distance = -EDGE_MIN_DISTANCE;

    *hits_out = 0;
    *hit_count_out = 0;
    if (line_len <= MIN_LINE_SCAN_DISTANCE || steps < 3) {
        return 1;
    }

    candidates = (EdgeHit *) calloc((size_t) steps, sizeof(EdgeHit));
    hits = (EdgeHit *) calloc((size_t) steps, sizeof(EdgeHit));
    if (candidates == 0 || hits == 0) {
        free(candidates);
        free(hits);
        return 0;
    }

    for (i = 1; i < steps - 1; ++i) {
        const double t = (double) i / (double) (steps - 1);
        const int cx = (int) floor((double) p1.x + dx * t + 0.5);
        const int cy = (int) floor((double) p1.y + dy * t + 0.5);
        double gx;
        double gy;
        double mag;
        double cosine;
        double angle;

        if (cx < 0 || cy < 0 || cx >= state->image.image_bgr->width || cy >= state->image.image_bgr->height) {
            continue;
        }

        gx = cvmGet(state->image.gx, cy, cx);
        gy = cvmGet(state->image.gy, cy, cx);
        mag = sqrt(gx * gx + gy * gy);
        if (mag <= EDGE_MAG_THRESHOLD) {
            continue;
        }

        cosine = fabs(line_nx * (gx / mag) + line_ny * (gy / mag));
        if (cosine > 1.0) {
            cosine = 1.0;
        }
        angle = 90.0 - acos(cosine) * 180.0 / CV_PI;

        candidates[candidate_count].pos = cvPoint(cx, cy);
        candidates[candidate_count].angle = angle;
        candidates[candidate_count].cosine = cosine;
        candidates[candidate_count].distance = point_distance(cvPoint(cx, cy), p1);
        ++candidate_count;
    }

    for (i = 0; i < candidate_count; ++i) {
        int begin = i - EDGE_DOT_WINDOW;
        int end = i + EDGE_DOT_WINDOW;
        int is_best = 1;
        int j;

        if (begin < 0) {
            begin = 0;
        }
        if (end >= candidate_count) {
            end = candidate_count - 1;
        }

        for (j = begin; j <= end; ++j) {
            if (candidates[j].cosine > candidates[i].cosine) {
                is_best = 0;
                break;
            }
        }

        if (candidates[i].angle < EDGE_MIN_LABEL_ANGLE) {
            continue;
        }

        if (is_best && candidates[i].distance - last_distance > EDGE_MIN_DISTANCE) {
            hits[hit_count++] = candidates[i];
            last_distance = candidates[i].distance;
        }
    }

    free(candidates);
    *hits_out = hits;
    *hit_count_out = hit_count;
    return 1;
}
