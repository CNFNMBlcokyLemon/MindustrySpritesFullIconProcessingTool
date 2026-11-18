#include <windows.h>  
#include <shellapi.h>  
#include <stdio.h>  
#include <string.h>  
#include <stdlib.h>  
#include <vector>  
  
#include "ids.h"  
#include "globals.h"  
#include "image.h"  
#include "model.h"  
#include "dialog.h"  
#include "render.h"  
#include "awindow.h"  
  
#ifdef LANG_ZH  
#include "lang_zh.h"  
#else  
#include "lang_en.h"  
#endif  
  
/* ---------- global definitions (extern-declared in globals.h) ---------- */  
  
HWND g_hw  = NULL;  
HWND g_wlb = NULL;  
HWND g_stb = NULL;  
  
Img g_unit, g_preview;  
OP  g_uop;  
std::vector<WD> g_wdefs;  
bool g_grid = false;  
  
float g_zoomF = 2.0f;  
float g_panX  = 0.0f;  
float g_panY  = 0.0f;  
int   g_selIdx = -1;  
  
/* preview drag state */  
static bool  g_dragging = false;  
static POINT g_dragPt;  
  
/* guard flag: prevents RGB<->HEX sync from re-triggering itself */  
static bool g_hexSync = false;  
  
/* ---------- hex <-> rgb helpers ---------- */  
  
/* parse "#rrggbb" or "rrggbb"; returns false on invalid input */  
static bool parseHex(const char* s, int* r, int* g, int* b) {  
    if (s[0] == '#') s++;  
    if (strlen(s) != 6) return false;  
    unsigned int rv, gv, bv;  
    if (sscanf(s, "%02x%02x%02x", &rv, &gv, &bv) != 3) return false;  
    *r = (int)rv; *g = (int)gv; *b = (int)bv;  
    return true;  
}  
  
/* push current R/G/B edits into the hex edit (unit outline) */  
static void syncUnitHexFromRGB(HWND hw) {  
    char rb[16] = "0", gb[16] = "0", bb[16] = "0", hex[16];  
    GetDlgItemTextA(hw, ID_UOR, rb, 16);  
    GetDlgItemTextA(hw, ID_UOG, gb, 16);  
    GetDlgItemTextA(hw, ID_UOB, bb, 16);  
    sprintf(hex, "#%02X%02X%02X", atoi(rb) & 255, atoi(gb) & 255, atoi(bb) & 255);  
    g_hexSync = true;  
    SetDlgItemTextA(hw, ID_UOHEX, hex);  
    g_hexSync = false;  
}  
  
/* push hex edit into R/G/B edits (unit outline); returns false if invalid */  
static bool syncUnitRGBFromHex(HWND hw) {  
    char hex[16] = "";  
    GetDlgItemTextA(hw, ID_UOHEX, hex, 16);  
    int r, g, b;  
    if (!parseHex(hex, &r, &g, &b)) return false;  
    char tmp[16];  
    g_hexSync = true;  
    sprintf(tmp, "%d", r); SetDlgItemTextA(hw, ID_UOR, tmp);  
    sprintf(tmp, "%d", g); SetDlgItemTextA(hw, ID_UOG, tmp);  
    sprintf(tmp, "%d", b); SetDlgItemTextA(hw, ID_UOB, tmp);  
    g_hexSync = false;  
    return true;  
}  
  
/* set unit outline color edits + apply the color to every weapon layer */  
static void applyOutlineColorAll(HWND hw, int r, int g, int b) {  
    char tmp[16];  
    g_hexSync = true;  
    sprintf(tmp, "%d", r); SetDlgItemTextA(hw, ID_UOR, tmp);  
    sprintf(tmp, "%d", g); SetDlgItemTextA(hw, ID_UOG, tmp);  
    sprintf(tmp, "%d", b); SetDlgItemTextA(hw, ID_UOB, tmp);  
    g_hexSync = false;  
    syncUnitHexFromRGB(hw);  
    for (int i = 0; i < (int)g_wdefs.size(); i++) {  
        g_wdefs[i].op.r = (unsigned char)r;  
        g_wdefs[i].op.g = (unsigned char)g;  
        g_wdefs[i].op.b = (unsigned char)b;  
    }  
    doRefresh();  
}  
  
/* ---------- main window procedure ---------- */  
  
LRESULT CALLBACK wndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {  
    int h = 22, pad = 4, y = 5;  
    HINSTANCE hi = GetModuleHandle(NULL);  
    switch (msg) {  
        case WM_CREATE: {  
            CreateWindowA("STATIC", STR_UNIT_BODY, WS_CHILD | WS_VISIBLE, 5, y, LW - 10, h, hw, NULL, hi, NULL);  
            y += h + pad;  
            CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 5, y, 200, h, hw, (HMENU)ID_UP, hi, NULL);  
            CreateWindowA("BUTTON", STR_BROWSE, WS_CHILD | WS_VISIBLE, 210, y, 55, h, hw, (HMENU)ID_UB, hi, NULL);  
            y += h + pad;  
            CreateWindowA("BUTTON", STR_OUTLINE, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 5, y, LW - 10, h, hw, (HMENU)ID_UO, hi, NULL);  
            y += h + pad;  
            CreateWindowA("STATIC", "R:", WS_CHILD | WS_VISIBLE, 5, y, 20, h, hw, NULL, hi, NULL);  
            CreateWindowA("EDIT", "86", WS_CHILD | WS_VISIBLE | WS_BORDER, 28, y, 38, h, hw, (HMENU)ID_UOR, hi, NULL);  
            CreateWindowA("STATIC", "G:", WS_CHILD | WS_VISIBLE, 75, y, 20, h, hw, NULL, hi, NULL);  
            CreateWindowA("EDIT", "86", WS_CHILD | WS_VISIBLE | WS_BORDER, 98, y, 38, h, hw, (HMENU)ID_UOG, hi, NULL);  
            CreateWindowA("STATIC", "B:", WS_CHILD | WS_VISIBLE, 145, y, 20, h, hw, NULL, hi, NULL);  
            CreateWindowA("EDIT", "102", WS_CHILD | WS_VISIBLE | WS_BORDER, 168, y, 38, h, hw, (HMENU)ID_UOB, hi, NULL);  
            CreateWindowA("STATIC", STR_AW_RAD, WS_CHILD | WS_VISIBLE, 215, y, 30, h, hw, NULL, hi, NULL);  
            CreateWindowA("EDIT", "3", WS_CHILD | WS_VISIBLE | WS_BORDER, 248, y, 28, h, hw, (HMENU)ID_UORAD, hi, NULL);  
            y += h + pad;  
            CreateWindowA("BUTTON", STR_OUTLINE_ONLY, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 5, y, 130, h, hw, (HMENU)ID_UOOL, hi, NULL);  
            CreateWindowA("STATIC", STR_HEX, WS_CHILD | WS_VISIBLE, 140, y, 30, h, hw, NULL, hi, NULL);  
            CreateWindowA("EDIT", "#565666", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 172, y, 95, h, hw, (HMENU)ID_UOHEX, hi, NULL);  
            y += h + pad;  
            CreateWindowA("BUTTON", STR_STD_COLOR, WS_CHILD | WS_VISIBLE, 5, y, 128, h, hw, (HMENU)ID_USTD, hi, NULL);  
            CreateWindowA("BUTTON", STR_EREKIR_COLOR, WS_CHILD | WS_VISIBLE, 140, y, 128, h, hw, (HMENU)ID_UERK, hi, NULL);  
            y += h + pad + 5;  
            CreateWindowA("STATIC", STR_LAYERS, WS_CHILD | WS_VISIBLE, 5, y, LW - 10, h, hw, NULL, hi, NULL);  
            y += h + pad;  
            g_wlb = CreateWindowA("LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,  
                                  5, y, LW - 10, 150, hw, (HMENU)ID_WL, hi, NULL);  
            y += 150 + pad;  
            CreateWindowA("BUTTON", STR_ADD, WS_CHILD | WS_VISIBLE, 5, y, 80, h, hw, (HMENU)ID_WA, hi, NULL);  
            CreateWindowA("BUTTON", STR_REMOVE, WS_CHILD | WS_VISIBLE, 95, y, 80, h, hw, (HMENU)ID_WR, hi, NULL);  
            CreateWindowA("BUTTON", STR_CLEAR, WS_CHILD | WS_VISIBLE, 185, y, 80, h, hw, (HMENU)ID_WC, hi, NULL);  
            y += h + pad;  
            CreateWindowA("BUTTON", STR_MOVE_UP, WS_CHILD | WS_VISIBLE, 5, y, 80, h, hw, (HMENU)ID_MU, hi, NULL);  
            CreateWindowA("BUTTON", STR_MOVE_DOWN, WS_CHILD | WS_VISIBLE, 95, y, 80, h, hw, (HMENU)ID_MD, hi, NULL);  
            CreateWindowA("BUTTON", STR_TOGGLE_VIS, WS_CHILD | WS_VISIBLE, 185, y, 80, h, hw, (HMENU)ID_TV, hi, NULL);  
            y += h + pad + 5;  
            CreateWindowA("STATIC", STR_OUT_FILE, WS_CHILD | WS_VISIBLE, 5, y, LW - 10, h, hw, NULL, hi, NULL);  
            y += h + pad;  
            CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 5, y, 200, h, hw, (HMENU)ID_OP, hi, NULL);  
            CreateWindowA("BUTTON", STR_BROWSE, WS_CHILD | WS_VISIBLE, 210, y, 55, h, hw, (HMENU)ID_OB, hi, NULL);  
            y += h + pad;  
            CreateWindowA("STATIC", STR_OUT_FOLDER, WS_CHILD | WS_VISIBLE, 5, y, LW - 10, h, hw, NULL, hi, NULL);  
            y += h + pad;  
            CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 5, y, 200, h, hw, (HMENU)ID_OD, hi, NULL);  
            CreateWindowA("BUTTON", STR_BROWSE, WS_CHILD | WS_VISIBLE, 210, y, 55, h, hw, (HMENU)ID_ODB, hi, NULL);  
            y += h + pad + 5;  
            CreateWindowA("BUTTON", STR_GENERATE, WS_CHILD | WS_VISIBLE, 5, y, LW - 10, h + 4, hw, (HMENU)ID_GEN, hi, NULL);  
            y += h + pad + 8;  
            CreateWindowA("BUTTON", STR_ZOOM_IN, WS_CHILD | WS_VISIBLE, 5, y, 60, h, hw, (HMENU)ID_ZI, hi, NULL);  
            CreateWindowA("BUTTON", STR_ZOOM_OUT, WS_CHILD | WS_VISIBLE, 70, y, 60, h, hw, (HMENU)ID_ZO, hi, NULL);  
            CreateWindowA("BUTTON", STR_GRID, WS_CHILD | WS_VISIBLE, 135, y, 60, h, hw, (HMENU)ID_GR, hi, NULL);  
            CreateWindowA("BUTTON", STR_REFRESH, WS_CHILD | WS_VISIBLE, 200, y, 68, h, hw, (HMENU)ID_RF, hi, NULL);  
            g_stb = CreateWindowA("STATIC", STR_ST_READY, WS_CHILD | WS_VISIBLE | SS_LEFT,  
                                  0, 0, 0, 0, hw, (HMENU)ID_ST, hi, NULL);  
            break;  
        }  
        case WM_SIZE: {  
            RECT rc;  
            GetClientRect(hw, &rc);  
            if (g_stb) MoveWindow(g_stb, 0, rc.bottom - SH, rc.right, SH, TRUE);  
            break;  
        }  
        case WM_LBUTTONDOWN: {  
            int mx = (short)LOWORD(lp);  
            if (mx > LW) {  
                g_dragging = true;  
                g_dragPt.x = mx;  
                g_dragPt.y = (short)HIWORD(lp);  
                SetCapture(hw);  
            }  
            break;  
        }  
        case WM_LBUTTONUP:  
            if (g_dragging) {  
                g_dragging = false;  
                ReleaseCapture();  
            }  
            break;  
        case WM_MOUSEMOVE:  
            if (g_dragging) {  
                int mx = (short)LOWORD(lp), my = (short)HIWORD(lp);  
                g_panX += (float)(mx - g_dragPt.x);  
                g_panY += (float)(my - g_dragPt.y);  
                g_dragPt.x = mx;  
                g_dragPt.y = my;  
                InvalidateRect(hw, NULL, FALSE);  
            }  
            break;  
        case WM_MOUSEWHEEL: {  
            int mx = (short)LOWORD(lp);  
            if (mx > LW) {  
                float old = g_zoomF;  
                int delta = (short)HIWORD(wp);  
                if (delta > 0) g_zoomF *= 1.25f;  
                else           g_zoomF /= 1.25f;  
                if (g_zoomF < 0.25f) g_zoomF = 0.25f;  
                if (g_zoomF > 16.0f) g_zoomF = 16.0f;  
                if (g_zoomF != old) InvalidateRect(hw, NULL, FALSE);  
            }  
            break;  
        }  
        case WM_CONTEXTMENU: {  
            /* right-click on the listbox or left panel deselects the layer */  
            if ((HWND)wp == g_wlb || (HWND)wp == hw) {  
                SendMessageA(g_wlb, LB_SETCURSEL, (WPARAM)-1, 0);  
                g_selIdx = -1;  
                doRefresh();  
            }  
            break;  
        }  
        case WM_COMMAND: {  
            /* live hex <-> rgb conversion for unit outline */  
            if (HIWORD(wp) == EN_CHANGE && !g_hexSync) {  
                int id = LOWORD(wp);  
                if (id == ID_UOHEX) {  
                    if (syncUnitRGBFromHex(hw)) doRefresh();  
                } else if (id == ID_UOR || id == ID_UOG || id == ID_UOB) {  
                    syncUnitHexFromRGB(hw);  
                    doRefresh();  
                }  
            }  
            /* listbox selection -> red highlight */  
            if (HIWORD(wp) == LBN_SELCHANGE && LOWORD(wp) == ID_WL) {  
                g_selIdx = (int)SendMessageA(g_wlb, LB_GETCURSEL, 0, 0);  
                doRefresh();  
            }  
            /* double-click listbox -> edit dialog */  
            if (HIWORD(wp) == LBN_DBLCLK && LOWORD(wp) == ID_WL) {  
                int sel = (int)SendMessageA(g_wlb, LB_GETCURSEL, 0, 0);  
                if (sel >= 0) showAW(sel);  
            }  
            switch (LOWORD(wp)) {  
                case ID_UB: {  
                    char buf[MAX_PATH] = "";  
                    if (browseImg(hw, buf, MAX_PATH)) {  
                        SetDlgItemTextA(hw, ID_UP, buf);  
                        loadUnit();  
                        doRefresh();  
                    }  
                    break;  
                }  
                case ID_WA:  
                    showAW(-1);  
                    break;  
                case ID_WR: {  
                    int sel = (int)SendMessageA(g_wlb, LB_GETCURSEL, 0, 0);  
                    if (sel >= 0 && sel < (int)g_wdefs.size()) {  
                        g_wdefs.erase(g_wdefs.begin() + sel);  
                        if (g_selIdx == sel) g_selIdx = -1;  
                        else if (g_selIdx > sel) g_selIdx--;  
                        updList();  
                        doRefresh();  
                    }  
                    break;  
                }  
                case ID_WC:  
                    g_wdefs.clear();  
                    g_selIdx = -1;  
                    updList();  
                    doRefresh();  
                    break;  
                case ID_MU: {  
                    int sel = (int)SendMessageA(g_wlb, LB_GETCURSEL, 0, 0);  
                    if (sel > 0 && sel < (int)g_wdefs.size()) {  
                        int t = g_wdefs[sel].zorder;  
                        g_wdefs[sel].zorder = g_wdefs[sel - 1].zorder;  
                        g_wdefs[sel - 1].zorder = t;  
                        updList();  
                        SendMessageA(g_wlb, LB_SETCURSEL, sel, 0);  
                        doRefresh();  
                    }  
                    break;  
                }  
                case ID_MD: {  
                    int sel = (int)SendMessageA(g_wlb, LB_GETCURSEL, 0, 0);  
                    if (sel >= 0 && sel < (int)g_wdefs.size() - 1) {  
                        int t = g_wdefs[sel].zorder;  
                        g_wdefs[sel].zorder = g_wdefs[sel + 1].zorder;  
                        g_wdefs[sel + 1].zorder = t;  
                        updList();  
                        SendMessageA(g_wlb, LB_SETCURSEL, sel, 0);  
                        doRefresh();  
                    }  
                    break;  
                }  
                case ID_TV: {  
                    int sel = (int)SendMessageA(g_wlb, LB_GETCURSEL, 0, 0);  
                    if (sel >= 0 && sel < (int)g_wdefs.size()) {  
                        g_wdefs[sel].visible = !g_wdefs[sel].visible;  
                        updList();  
                        SendMessageA(g_wlb, LB_SETCURSEL, sel, 0);  
                        doRefresh();  
                    }  
                    break;  
                }  
                case ID_USTD:  
                    applyOutlineColorAll(hw, 86, 86, 102);  
                    break;  
                case ID_UERK:  
                    applyOutlineColorAll(hw, 56, 56, 72);  
                    break;  
                case ID_OB: {  
                    char buf[MAX_PATH] = "";  
                    if (browseSave(hw, buf, MAX_PATH))  
                        SetDlgItemTextA(hw, ID_OP, buf);  
                    break;  
                }  
                case ID_ODB: {  
                    char buf[MAX_PATH] = "";  
                    if (browseFolder(hw, buf, MAX_PATH))  
                        SetDlgItemTextA(hw, ID_OD, buf);  
                    break;  
                }  
                case ID_GEN:  
                    doGenerate();  
                    break;  
                case ID_ZI:  
                    g_zoomF *= 1.5f;  
                    if (g_zoomF > 16.0f) g_zoomF = 16.0f;  
                    InvalidateRect(hw, NULL, FALSE);  
                    break;  
                case ID_ZO:  
                    g_zoomF /= 1.5f;  
                    if (g_zoomF < 0.25f) g_zoomF = 0.25f;  
                    InvalidateRect(hw, NULL, FALSE);  
                    break;  
                case ID_GR:  
                    g_grid = !g_grid;  
                    InvalidateRect(hw, NULL, FALSE);  
                    break;  
                case ID_RF:  
                    loadUnit();  
                    doRefresh();  
                    break;  
            }  
            /* AW dialog result: edit mode overwrites, add mode appends */  
            if (g_awOK && !g_aw) {  
                g_awOK = false;  
                if (g_editIdx >= 0 && g_editIdx < (int)g_wdefs.size()) {  
                    g_wdefs[g_editIdx] = g_awResult;  
                } else {  
                    g_wdefs.push_back(g_awResult);  
                }  
                g_editIdx = -1;  
                updList();  
                doRefresh();  
            }  
            break;  
        }  
        case WM_PAINT: {  
            PAINTSTRUCT ps;  
            HDC hdc = BeginPaint(hw, &ps);  
            RECT rc;  
            GetClientRect(hw, &rc);  
            rc.left = LW;  
            rc.bottom -= SH;  
            /* double buffer: draw into memory DC then blit once */  
            int w = rc.right - rc.left, hgt = rc.bottom - rc.top;  
            if (w > 0 && hgt > 0) {  
                HDC mem = CreateCompatibleDC(hdc);  
                HBITMAP bmp = CreateCompatibleBitmap(hdc, w, hgt);  
                HBITMAP old = (HBITMAP)SelectObject(mem, bmp);  
                RECT mrc;  
                mrc.left = 0; mrc.top = 0; mrc.right = w; mrc.bottom = hgt;  
                drawPreview(mem, &mrc);  
                BitBlt(hdc, rc.left, rc.top, w, hgt, mem, 0, 0, SRCCOPY);  
                SelectObject(mem, old);  
                DeleteObject(bmp);  
                DeleteDC(mem);  
            }  
            EndPaint(hw, &ps);  
            break;  
        }  
        case WM_ERASEBKGND: {  
            /* only fill the left panel; preview is covered by double-buffered paint */  
            HDC hdc = (HDC)wp;  
            RECT rc;  
            GetClientRect(hw, &rc);  
            HBRUSH b = GetSysColorBrush(COLOR_BTNFACE);  
            RECT lr = rc;  
            lr.right = LW;  
            FillRect(hdc, &lr, b);  
            return 1;  
        }  
        case WM_DROPFILES: {  
            HDROP hd = (HDROP)wp;  
            char buf[MAX_PATH] = "";  
            DragQueryFileA(hd, 0, buf, MAX_PATH);  
            DragFinish(hd);  
            SetDlgItemTextA(hw, ID_UP, buf);  
            loadUnit();  
            doRefresh();  
            break;  
        }  
        case WM_DESTROY:  
            PostQuitMessage(0);  
            break;  
    }  
    return DefWindowProcA(hw, msg, wp, lp);  
}  
  
/* ---------- entry ---------- */  
  
int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int nShow) {  
    WNDCLASSA wc;  
    memset(&wc, 0, sizeof(wc));  
    wc.lpfnWndProc   = wndProc;  
    wc.hInstance     = hi;  
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);  
    wc.lpszClassName = "MWClass";  
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);  
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);  
    RegisterClassA(&wc);  
  
    WNDCLASSA aw;  
    memset(&aw, 0, sizeof(aw));  
    aw.lpfnWndProc   = awProc;  
    aw.hInstance     = hi;  
    aw.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);  
    aw.lpszClassName = "AWClass";  
    aw.hCursor       = LoadCursor(NULL, IDC_ARROW);  
    RegisterClassA(&aw);  
  
    g_hw = CreateWindowExA(WS_EX_ACCEPTFILES, "MWClass",  
                           STR_TITLE,  
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,  
                           CW_USEDEFAULT, CW_USEDEFAULT, 910, 715, NULL, NULL, hi, NULL);  
    /* keep main window above normal windows */  
    SetWindowPos(g_hw, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);  
    ShowWindow(g_hw, nShow);  
    UpdateWindow(g_hw);  
  
    MSG msg;  
    while (GetMessageA(&msg, NULL, 0, 0)) {  
        TranslateMessage(&msg);  
        DispatchMessageA(&msg);  
    }  
    return (int)msg.wParam;  
}
