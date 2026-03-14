#include "ui_strings.h"

static const UiText UI_TEXT = {
    "FastView",
    "FastView [%d*%d]",
    "Coordinate Panel",
    "Select image",
    "Open image",
    "Image files",
    "Failed to load the selected image.",
    "Failed to save screenshot.",
    "At most 10 coordinate points are supported.",
    "Enter integer X and Y values.",
    "Coordinate is outside the image.",
    "X:",
    "Y:",
    "Add",
    "Delete",
    "Open",
    "Save Path",
    "Image max coordinate: (%d, %d)",
    "Save path: %s",
    "Usage:\r\n"
    "'Ctrl(hold)':                snap to detected corner\r\n"
    "'Ctrl + Wheel':              zoom\r\n"
    "'MouseMiddle move':          move image\r\n"
    "'Shift(hold)':               lock horizontal/vertical\r\n"
    "'S':                         save current image&labels\r\n"
    "'Space + MouseMiddleClick':  reset zoom and center"
};

static const char *const IMAGE_FILTERS[] = {
    "*.png",
    "*.jpg",
    "*.jpeg",
    "*.bmp",
    "*.tif",
    "*.tiff"
};

const UiText *ui_text(void) {
    return &UI_TEXT;
}

const char *const *ui_image_filters(int *count) {
    if (count != 0) {
        *count = (int) (sizeof(IMAGE_FILTERS) / sizeof(IMAGE_FILTERS[0]));
    }
    return IMAGE_FILTERS;
}
