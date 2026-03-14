#pragma once

typedef struct {
    const char *app_name;
    const char *main_window_title_format;
    const char *panel_window_title;
    const char *select_image_title;
    const char *open_image_title;
    const char *image_filter_description;
    const char *image_load_failed;
    const char *screenshot_save_failed;
    const char *marker_limit_reached;
    const char *invalid_coordinate;
    const char *coordinate_out_of_range;
    const char *label_x;
    const char *label_y;
    const char *button_add;
    const char *button_delete;
    const char *button_open;
    const char *button_save_path;
    const char *range_format;
    const char *save_path_prefix;
    const char *usage_hint;
} UiText;

const UiText *ui_text(void);
const char *const *ui_image_filters(int *count);
