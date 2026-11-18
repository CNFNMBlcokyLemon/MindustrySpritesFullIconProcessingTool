#ifndef IMAGE_H  
#define IMAGE_H  
  
#include <math.h>  
  
/* RGBA image wrapper, loaded via stb_image, manual memory management */  
class Img {  
public:  
    unsigned char* d;  
    int w, h;  
  
    Img();  
    bool ok() const;  
    bool load(const char* path);  
    void free2();  
    Img flipH() const;  
};  
  
/* Canvas: fixed-size RGBA buffer with alpha compositing */  
class Cv {  
public:  
    unsigned char* d;  
    int w, h;  
  
    Cv(int w_, int h_);  
    ~Cv();  
    void blit(const Img& img, int cx, int cy);  
};  
  
/* Outline: transparent pixels within rad of opaque pixels get filled.  
   outlineOnly=true keeps only the outline, discards the source. */  
Img mkOutline(const Img& src, unsigned char oR, unsigned char oG, unsigned char oB,  
              int rad, bool outlineOnly);  
  
/* Rotate by angle (degrees), output sized to bounding box, bilinear sampling */  
Img rotateImg(const Img& src, float angle);  
  
static inline float fmn(float a, float b) { return fminf(a, b); }  
static inline float fmx(float a, float b) { return fmaxf(a, b); }  
  
#endif
