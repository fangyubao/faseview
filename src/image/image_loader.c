#include "image_loader.h"

#include <stddef.h>

#include <opencv2/core/core_c.h>

#include "image_analysis.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int load_image_file(const char *path, ImageData *out) {
    IplImage *image = 0;
    unsigned char *pixels = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
    int y;

    pixels = stbi_load(path, &width, &height, &channels, 3);
    if (pixels == 0) {
        return 0;
    }

    image = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
    if (image == 0) {
        stbi_image_free(pixels);
        return 0;
    }

    for (y = 0; y < height; ++y) {
        const unsigned char *src = pixels + (size_t) y * (size_t) width * 3u;
        unsigned char *dst = (unsigned char *) (image->imageData + y * image->widthStep);
        int x;
        for (x = 0; x < width; ++x) {
            dst[x * 3 + 0] = src[x * 3 + 2];
            dst[x * 3 + 1] = src[x * 3 + 1];
            dst[x * 3 + 2] = src[x * 3 + 0];
        }
    }
    stbi_image_free(pixels);

    if (!analyze_image(image, out)) {
        cvReleaseImage(&image);
        return 0;
    }
    cvReleaseImage(&image);
    return 1;
}

