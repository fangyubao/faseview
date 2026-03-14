#pragma once

#include "app_state.h"

int analyze_image(const IplImage *source, ImageData *out);
void release_image_data(ImageData *data);
CvPoint get_raw_point(const AppState *state, CvPoint screen_point);
CvPoint get_active_raw_point(const AppState *state, CvPoint screen_point, int ctrl_pressed, int shift_pressed);
int find_nearest_corner(const ImageData *data, CvPoint raw_point, CvPoint *snapped_point, double threshold);
int collect_edge_hits(const AppState *state, CvPoint p1, CvPoint p2, EdgeHit **hits_out, int *hit_count_out);

