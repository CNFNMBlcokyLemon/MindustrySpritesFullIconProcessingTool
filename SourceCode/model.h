#ifndef MODEL_H  
#define MODEL_H  
  
#include <string>  
#include <vector>  
  
/* outline params shared by unit body and weapon layers */  
class OP {  
public:  
    bool on;            /* outline enabled */  
    bool outlineOnly;   /* draw outline only, discard source */  
    unsigned char r, g, b;  
    int rad;            /* outline radius in pixels */  
  
    OP() : on(false), outlineOnly(false), r(86), g(86), b(102), rad(3) {}  
};  
  
/* WD: weapon/layer definition with zorder and visibility */  
class WD {  
public:  
    std::string nm;     /* name */  
    std::string pt;     /* image path */  
    float x, y;         /* offset from unit center, y up */  
    float baseRot;      /* base rotation in degrees */  
    bool mir;           /* generate mirrored instance */  
    OP op;              /* per-layer outline params */  
    int zorder;         /* render order, smaller drawn first */  
    bool visible;       /* participates in compositing */  
  
    WD() : x(0), y(0), baseRot(0), mir(true), zorder(0), visible(true) {}  
};  
  
/* WI: render instance produced by expand() */  
class WI {  
public:  
    std::string nm, pt;  
    float x, y, baseRot;  
    bool fl;            /* needs horizontal flip (mirror copy) */  
    OP op;  
    int srcIdx;         /* index into the defs array passed to expand() */  
  
    WI() : x(0), y(0), baseRot(0), fl(false), srcIdx(-1) {}  
};  
  
/* expand WD list into WI list: mir=true produces an extra mirrored  
   instance with negated x, flip flag, and negated rotation */  
std::vector<WI> expand(const std::vector<WD>& defs);  
  
/* return indices of defs sorted by zorder ascending (stable) */  
std::vector<int> zorderIndices(const std::vector<WD>& defs);  
  
#endif
