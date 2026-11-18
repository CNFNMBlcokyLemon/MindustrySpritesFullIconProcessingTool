#ifndef GLOBALS_H  
#define GLOBALS_H  
  
#include <windows.h>  
#include <vector>  
#include "image.h"  
#include "model.h"  
  
/* main window and child controls */  
extern HWND g_hw;    /* main window */  
extern HWND g_wlb;   /* weapon layer listbox */  
extern HWND g_stb;   /* status bar */  
  
/* image and render state */  
extern Img g_unit;               /* unit body image */  
extern Img g_preview;            /* composited preview image */  
extern OP  g_uop;                /* unit outline params */  
extern std::vector<WD> g_wdefs;  /* weapon layer definitions */  
extern bool g_grid;              /* show pixel grid */  
  
/* preview pan/zoom state */  
extern float g_zoomF;   /* float zoom factor */  
extern float g_panX;    /* pan offset X (screen px) */  
extern float g_panY;    /* pan offset Y (screen px) */  
extern int   g_selIdx;  /* selected layer index in g_wdefs, -1 = none */  
  
/* Add/Edit Weapon dialog state (defined in awindow.cpp) */  
extern HWND g_aw;        /* dialog handle, NULL = closed */  
extern bool g_awOK;      /* user pressed OK with valid result */  
extern WD   g_awResult;  /* dialog result data */  
extern int  g_editIdx;   /* -1 = add mode; >=0 = editing g_wdefs[g_editIdx] */  
  
#endif
