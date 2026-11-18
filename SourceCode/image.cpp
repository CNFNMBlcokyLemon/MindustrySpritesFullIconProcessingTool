#include "image.h"  
  
#define STB_IMAGE_IMPLEMENTATION  
#include "stb_image.h"  
  
#include <stdlib.h>  
#include <string.h>  
#include <math.h>  
  
// ---------- Img ----------  
  
Img::Img() : d(NULL), w(0), h(0) {}  
  
bool Img::ok() const {  
    return d != NULL;  
}  
  
bool Img::load(const char* path) {  
    free2();  
    int ch;  
    d = stbi_load(path, &w, &h, &ch, 4);  
    return d != NULL;  
}  
  
void Img::free2() {  
    if (d) {  
        stbi_image_free(d);  
        d = NULL;  
        w = 0;  
        h = 0;  
    }  
}  
  
Img Img::flipH() const {  
    Img r;  
    r.w = w;  
    r.h = h;  
    r.d = (unsigned char*)malloc(w * h * 4);  
    if (!r.d) return r;  
    for (int y = 0; y < h; y++)  
        for (int x = 0; x < w; x++) {  
            int s = (y * w + (w - 1 - x)) * 4, t = (y * w + x) * 4;  
            r.d[t]     = d[s];  
            r.d[t + 1] = d[s + 1];  
            r.d[t + 2] = d[s + 2];  
            r.d[t + 3] = d[s + 3];  
        }  
    return r;  
}  
  
// ---------- Cv ----------  
  
Cv::Cv(int w_, int h_) : w(w_), h(h_) {  
    d = (unsigned char*)calloc(w * h * 4, 1);  
}  
  
Cv::~Cv() {  
    if (d) free(d);  
}  
  
void Cv::blit(const Img& img, int cx, int cy) {  
    if (!img.d) return;  
    int ox = cx - img.w / 2, oy = cy - img.h / 2;  
    for (int iy = 0; iy < img.h; iy++)  
        for (int ix = 0; ix < img.w; ix++) {  
            int px = ox + ix, py = oy + iy;  
            if (px < 0 || px >= w || py < 0 || py >= h) continue;  
            int si = (iy * img.w + ix) * 4, di = (py * w + px) * 4;  
            float sa = img.d[si + 3] / 255.0f, da = d[di + 3] / 255.0f;  
            float oa = sa + da * (1.0f - sa);  
            if (oa > 1e-6f) {  
                d[di]     = (unsigned char)((img.d[si]     * sa + d[di]     * da * (1.0f - sa)) / oa + 0.5f);  
                d[di + 1] = (unsigned char)((img.d[si + 1] * sa + d[di + 1] * da * (1.0f - sa)) / oa + 0.5f);  
                d[di + 2] = (unsigned char)((img.d[si + 2] * sa + d[di + 2] * da * (1.0f - sa)) / oa + 0.5f);  
                d[di + 3] = (unsigned char)(oa * 255.0f + 0.5f);  
            }  
        }  
}  
  
// ---------- mkOutline ----------  
  
Img mkOutline(const Img& src, unsigned char oR, unsigned char oG, unsigned char oB,  
              int rad, bool outlineOnly) {  
    Img r;  
    r.w = src.w;  
    r.h = src.h;  
    r.d = (unsigned char*)calloc(src.w * src.h * 4, 1);  
    if (!r.d) return r;  
    for (int y = 0; y < src.h; y++)  
        for (int x = 0; x < src.w; x++) {  
            int si = (y * src.w + x) * 4;  
            if (src.d[si + 3] > 0) {  
                if (!outlineOnly) {  
                    r.d[si]     = src.d[si];  
                    r.d[si + 1] = src.d[si + 1];  
                    r.d[si + 2] = src.d[si + 2];  
                    r.d[si + 3] = src.d[si + 3];  
                }  
                continue;  
            }  
            bool found = false;  
            for (int dy = -rad; dy <= rad && !found; dy++)  
                for (int dx = -rad; dx <= rad && !found; dx++) {  
                    if (dx * dx + dy * dy > rad * rad) continue;  
                    int nx = x + dx, ny = y + dy;  
                    if (nx < 0 || nx >= src.w || ny < 0 || ny >= src.h) continue;  
                    if (src.d[(ny * src.w + nx) * 4 + 3] > 0) found = true;  
                }  
            if (found) {  
                r.d[si]     = oR;  
                r.d[si + 1] = oG;  
                r.d[si + 2] = oB;  
                r.d[si + 3] = 255;  
            }  
        }  
    return r;  
}  
  
// ---------- rotateImg ----------  
  
Img rotateImg(const Img& src, float angle) {  
    Img r;  
    r.d = NULL;  
    r.w = 0;  
    r.h = 0;  
    if (!src.d) return r;  
    const float PI = 3.14159265f;  
    float rad = angle * PI / 180.0f;  
    float cosA = (float)cos(rad), sinA = (float)sin(rad);  
    int newW = (int)(fabs((double)src.w * cosA) + fabs((double)src.h * sinA) + 1.5f);  
    int newH = (int)(fabs((double)src.w * sinA) + fabs((double)src.h * cosA) + 1.5f);  
    if (newW < 1) newW = 1;  
    if (newH < 1) newH = 1;  
    r.w = newW;  
    r.h = newH;  
    r.d = (unsigned char*)calloc(newW * newH * 4, 1);  
    if (!r.d) return r;  
    float cx = src.w / 2.0f, cy = src.h / 2.0f, ncx = newW / 2.0f, ncy = newH / 2.0f;  
    int ny, nx, c;  
    for (ny = 0; ny < newH; ny++)  
        for (nx = 0; nx < newW; nx++) {  
            float dx = nx - ncx, dy = ny - ncy;  
            float ix = dx * cosA + dy * sinA + cx, iy = -dx * sinA + dy * cosA + cy;  
            if (ix < 0 || ix > src.w - 1 || iy < 0 || iy > src.h - 1) continue;  
            int x0 = (int)ix, y0 = (int)iy, x1 = x0 + 1, y1 = y0 + 1;  
            if (x1 >= src.w) x1 = src.w - 1;  
            if (y1 >= src.h) y1 = src.h - 1;  
            float fx = ix - x0, fy = iy - y0;  
            int di = (ny * newW + nx) * 4;  
            for (c = 0; c < 4; c++) {  
                float v = src.d[(y0 * src.w + x0) * 4 + c] * (1 - fx) * (1 - fy)  
                        + src.d[(y0 * src.w + x1) * 4 + c] * fx * (1 - fy)  
                        + src.d[(y1 * src.w + x0) * 4 + c] * (1 - fx) * fy  
                        + src.d[(y1 * src.w + x1) * 4 + c] * fx * fy;  
                r.d[di + c] = (unsigned char)(v + 0.5f);  
            }  
        }  
    return r;  
}
