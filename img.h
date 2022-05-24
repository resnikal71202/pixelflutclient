#ifndef IMG_H
#define IMG_H
#include <stdlib.h>
#include <string.h>

struct img{
    char *filename;
    unsigned char *img;
    int width;
    int height;
    int channels;
};

struct img_string{
    size_t len;
    char *buff;
};

struct img get_img(const char *filename);
struct img_string img_to_string(const struct img simg, const int xs, const int xe, const int ys, const int ye, const int xoffset, const int yoffset);

#endif