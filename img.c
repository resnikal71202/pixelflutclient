#include "img.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

struct img get_img(const char *filename){
    struct img simg;
    simg.img = stbi_load(filename, &simg.width, &simg.height, &simg.channels, 0);
    return simg;
}

struct img_string img_to_string(const struct img simg, const int xs, const int xe, const int ys, const int ye, const int xoffset, const int yoffset){
    struct img_string img_str;
    img_str.len = 0;
    img_str.buff = malloc(0 * sizeof(char));
    for (int x = xs; x <= xe; x++)
    {
        for (int y = ys; y <= ye; y++)
        {
            char temp[25];
            if (simg.img[(x + y * simg.width) * simg.channels] != 0 && simg.img[(x + y * simg.width) * simg.channels + 1] != 0 && simg.img[(x + y * simg.width) * simg.channels + 2] != 0)
            {
                snprintf(temp, 25, "PX %d %d %02x%02x%02x\n", x + xoffset, y + yoffset, simg.img[(x + y * simg.width) * simg.channels], simg.img[(x + y * simg.width) * simg.channels + 1], simg.img[(x + y * simg.width) * simg.channels + 2]);
                int diff = strlen(temp);
                img_str.len += diff;
                img_str.buff = realloc(img_str.buff, img_str.len);
                memcpy(&(img_str.buff[img_str.len - diff]), &temp, diff);
            }
        }
    }
    return img_str;
}