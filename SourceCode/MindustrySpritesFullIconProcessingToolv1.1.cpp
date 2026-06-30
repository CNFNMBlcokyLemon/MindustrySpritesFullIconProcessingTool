#define STB_IMAGE_IMPLEMENTATION  
#include "stb_image.h"  
#define STB_IMAGE_WRITE_IMPLEMENTATION  
#include "stb_image_write.h"  
#include <windows.h>  
#include <commdlg.h>  
#include <shellapi.h>  
#include <stdio.h>  
#include <string.h>  
#include <stdlib.h>  
#include <vector>  
#include <string>  
#define LW 275  
#define SH 22  
#define ID_UP 101  
#define ID_UB 102  
#define ID_UO 103  
#define ID_UOR 104  
#define ID_UOG 105  
#define ID_UOB 106  
#define ID_UORAD 107  
#define ID_WL 108  
#define ID_WA 109  
#define ID_WR 110  
#define ID_WC 111  
#define ID_OP 112  
#define ID_OB 113  
#define ID_GEN 114  
#define ID_ZI 115  
#define ID_ZO 116  
#define ID_GR 117  
#define ID_RF 118  
#define ID_ST 119  
#define ID_AWN 201  
#define ID_AWP 202  
#define ID_AWB 203  
#define ID_AWX 204  
#define ID_AWY 205  
#define ID_AWM 206  
#define ID_AWO 207  
#define ID_AWOR 208  
#define ID_AWOG 209  
#define ID_AWOB 210  
#define ID_AWORAD 211  
#define ID_AWOK 212  
#define ID_AWCA 213  
static inline float fmn(float a,float b){return a<b?a:b;}  
static inline float fmx(float a,float b){return a>b?a:b;}  
class Img{  
public:  
unsigned char* d;int w,h;  
Img():d(NULL),w(0),h(0){}  
bool load(const char* p){int c=0;d=stbi_load(p,&w,&h,&c,4);return d!=NULL;}  
void free2(){if(d){stbi_image_free(d);d=NULL;}w=h=0;}  
bool ok()const{return d!=NULL;}  
Img flipH()const{  
Img r;r.w=w;r.h=h;r.d=(unsigned char*)malloc(w*h*4);  
if(!r.d)return r;  
for(int y=0;y<h;y++)for(int x=0;x<w;x++){  
int s=(y*w+(w-1-x))*4,t=(y*w+x)*4;  
r.d[t]=d[s];r.d[t+1]=d[s+1];r.d[t+2]=d[s+2];r.d[t+3]=d[s+3];}  
return r;}  
};  
class OP{  
public:  
bool on;unsigned char r,g,b;int rad;  
OP():on(false),r(86),g(86),b(102),rad(3){}  
};  
class WD{  
public:  
std::string nm,pt;float x,y;bool mir;OP op;  
WD():x(0),y(0),mir(true){}  
};  
class WI{  
public:  
std::string nm,pt;float x,y;bool fl;OP op;  
WI():x(0),y(0),fl(false){}  
};  
class Cv{  
public:  
unsigned char* d;int w,h;  
Cv(int w_,int h_):w(w_),h(h_){d=(unsigned char*)calloc(w*h*4,1);}  
~Cv(){if(d)free(d);}  
void blit(const Img& img,int cx,int cy){  
if(!img.d)return;  
int ox=cx-img.w/2,oy=cy-img.h/2;  
for(int iy=0;iy<img.h;iy++)for(int ix=0;ix<img.w;ix++){  
int px=ox+ix,py=oy+iy;  
if(px<0||px>=w||py<0||py>=h)continue;  
int si=(iy*img.w+ix)*4,di=(py*w+px)*4;  
float sa=img.d[si+3]/255.0f,da=d[di+3]/255.0f,oa=sa+da*(1.0f-sa);  
if(oa>1e-6f){  
d[di]=(unsigned char)((img.d[si]*sa+d[di]*da*(1.0f-sa))/oa+0.5f);  
d[di+1]=(unsigned char)((img.d[si+1]*sa+d[di+1]*da*(1.0f-sa))/oa+0.5f);  
d[di+2]=(unsigned char)((img.d[si+2]*sa+d[di+2]*da*(1.0f-sa))/oa+0.5f);  
d[di+3]=(unsigned char)(oa*255.0f+0.5f);}}}  
};  
static Img mkOutline(const Img& src,unsigned char oR,unsigned char oG,unsigned char oB,int rad){  
Img r;r.w=src.w;r.h=src.h;r.d=(unsigned char*)calloc(src.w*src.h*4,1);  
if(!r.d)return r;  
for(int y=0;y<src.h;y++)for(int x=0;x<src.w;x++){  
int si=(y*src.w+x)*4;  
if(src.d[si+3]>0)continue;  
bool found=false;  
for(int dy=-rad;dy<=rad&&!found;dy++)for(int dx=-rad;dx<=rad&&!found;dx++){  
if(dx*dx+dy*dy>rad*rad)continue;  
int nx=x+dx,ny=y+dy;  
if(nx<0||nx>=src.w||ny<0||ny>=src.h)continue;  
if(src.d[(ny*src.w+nx)*4+3]>0)found=true;}  
if(found){int di=(y*src.w+x)*4;r.d[di]=oR;r.d[di+1]=oG;r.d[di+2]=oB;r.d[di+3]=255;}}  
for(int i=0;i<src.w*src.h*4;i+=4)  
if(src.d[i+3]>0){r.d[i]=src.d[i];r.d[i+1]=src.d[i+1];r.d[i+2]=src.d[i+2];r.d[i+3]=src.d[i+3];}  
return r;}  
static std::vector<WI> expand(const std::vector<WD>& defs){  
std::vector<WI> res;  
for(int i=0;i<(int)defs.size();i++){  
const WD& d=defs[i];  
WI w;w.nm=d.nm;w.pt=d.pt;w.x=d.x;w.y=d.y;w.fl=false;w.op=d.op;  
res.push_back(w);  
if(d.mir){WI m;m.nm=d.nm+"_L";m.pt=d.pt;m.x=-d.x;m.y=d.y;m.fl=true;m.op=d.op;res.push_back(m);}}  
return res;}  
static HWND g_hw=NULL;  
static HWND g_aw=NULL;  
static Img g_unit;  
static OP g_uop;  
static std::vector<WD> g_wdefs;  
static Img g_preview;  
static int g_zoom=2;  
static bool g_grid=false;  
static void setStatus(const char* s){HWND h=GetDlgItem(g_hw,ID_ST);if(h)SetWindowTextA(h,s);}  
static bool browseImg(HWND hw,char* buf,int sz){  
OPENFILENAMEA o;memset(&o,0,sizeof(o));o.lStructSize=sizeof(o);o.hwndOwner=hw;  
o.lpstrFilter="Images\0*.png;*.jpg;*.bmp\0All\0*.*\0";  
o.lpstrFile=buf;o.nMaxFile=sz;o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;  
return GetOpenFileNameA(&o)!=0;}  
static bool browseSave(HWND hw,char* buf,int sz){  
OPENFILENAMEA o;memset(&o,0,sizeof(o));o.lStructSize=sizeof(o);o.hwndOwner=hw;  
o.lpstrFilter="PNG\0*.png\0";o.lpstrFile=buf;o.nMaxFile=sz;  
o.lpstrDefExt="png";o.Flags=OFN_OVERWRITEPROMPT;  
return GetSaveFileNameA(&o)!=0;}  
static void updList(){  
HWND h=GetDlgItem(g_hw,ID_WL);if(!h)return;  
SendMessageA(h,LB_RESETCONTENT,0,0);  
for(int i=0;i<(int)g_wdefs.size();i++){  
char buf[256];  
sprintf(buf,"%-16s x=%.1f y=%.1f %s",g_wdefs[i].nm.c_str(),g_wdefs[i].x,g_wdefs[i].y,g_wdefs[i].mir?"[mirror]":"");  
SendMessageA(h,LB_ADDSTRING,0,(LPARAM)buf);}}  
static void loadUnit(){  
char buf[MAX_PATH]="";HWND h=GetDlgItem(g_hw,ID_UP);if(h)GetWindowTextA(h,buf,MAX_PATH);  
g_unit.free2();  
if(strcmp(buf,"0")==0){  
g_unit.w=32;g_unit.h=32;g_unit.d=(unsigned char*)calloc(32*32*4,1);  
setStatus("Unit: transparent 32x32");}  
else if(strlen(buf)>0){  
if(g_unit.load(buf)){char s[256];sprintf(s,"Unit loaded: %dx%d",g_unit.w,g_unit.h);setStatus(s);}  
else setStatus("Error: cannot load unit image");}}  
static void doRefresh(){  
char rb[16]="86",gb[16]="86",bb[16]="102",radb[16]="3";  
GetDlgItemTextA(g_hw,ID_UOR,rb,16);GetDlgItemTextA(g_hw,ID_UOG,gb,16);  
GetDlgItemTextA(g_hw,ID_UOB,bb,16);GetDlgItemTextA(g_hw,ID_UORAD,radb,16);  
g_uop.r=(unsigned char)atoi(rb);g_uop.g=(unsigned char)atoi(gb);  
g_uop.b=(unsigned char)atoi(bb);g_uop.rad=atoi(radb);if(g_uop.rad<1)g_uop.rad=1;  
g_uop.on=SendDlgItemMessage(g_hw,ID_UO,BM_GETCHECK,0,0)==BST_CHECKED;  
std::vector<WI> ws=expand(g_wdefs);  
std::vector<Img> imgs(ws.size());  
for(int i=0;i<(int)ws.size();i++)  
if(!imgs[i].load(ws[i].pt.c_str())){char s[256];sprintf(s,"Warn: cannot load '%s'",ws[i].pt.c_str());setStatus(s);}  
float mnX=g_unit.ok()?-(float)g_unit.w/2.0f:-16.0f,mxX=g_unit.ok()?(float)g_unit.w/2.0f:16.0f;  
float mnY=g_unit.ok()?-(float)g_unit.h/2.0f:-16.0f,mxY=g_unit.ok()?(float)g_unit.h/2.0f:16.0f;  
for(int i=0;i<(int)ws.size();i++){if(!imgs[i].ok())continue;  
float hw2=(float)imgs[i].w/2.0f,hh2=(float)imgs[i].h/2.0f;  
mnX=fmn(mnX,ws[i].x-hw2);mxX=fmx(mxX,ws[i].x+hw2);  
mnY=fmn(mnY,ws[i].y-hh2);mxY=fmx(mxY,ws[i].y+hh2);}  
const int PAD=8;  
int cw=(int)(mxX-mnX+0.5f)+PAD*2,ch=(int)(mxY-mnY+0.5f)+PAD*2;  
if(cw<1)cw=1;if(ch<1)ch=1;  
int ucx=(int)(-mnX+0.5f)+PAD,ucy=(int)(mxY+0.5f)+PAD;  
Cv cv(cw,ch);  
if(g_unit.ok()){  
if(g_uop.on){Img ol=mkOutline(g_unit,g_uop.r,g_uop.g,g_uop.b,g_uop.rad);cv.blit(ol,ucx,ucy);ol.free2();}  
else cv.blit(g_unit,ucx,ucy);}  
for(int i=0;i<(int)ws.size();i++){if(!imgs[i].ok())continue;  
int px=ucx+(int)ws[i].x,py=ucy-(int)ws[i].y;  
Img* src=&imgs[i];Img fl;  
if(ws[i].fl){fl=imgs[i].flipH();src=&fl;}  
if(ws[i].op.on){Img ol=mkOutline(*src,ws[i].op.r,ws[i].op.g,ws[i].op.b,ws[i].op.rad);cv.blit(ol,px,py);ol.free2();}  
else cv.blit(*src,px,py);  
if(ws[i].fl&&fl.d)free(fl.d);}  
g_preview.free2();g_preview.w=cw;g_preview.h=ch;  
g_preview.d=(unsigned char*)malloc(cw*ch*4);  
if(g_preview.d)memcpy(g_preview.d,cv.d,cw*ch*4);  
for(int i=0;i<(int)imgs.size();i++)imgs[i].free2();  
char s[128];sprintf(s,"Preview: %dx%d  Zoom:%dx  Weapons:%d",cw,ch,g_zoom,(int)g_wdefs.size());  
setStatus(s);InvalidateRect(g_hw,NULL,FALSE);}  
static void doGenerate(){  
if(!g_preview.ok()){setStatus("No preview - click Refresh first");return;}  
char buf[MAX_PATH]="output.png";HWND h=GetDlgItem(g_hw,ID_OP);if(h)GetWindowTextA(h,buf,MAX_PATH);  
if(!strlen(buf))strcpy(buf,"output.png");  
int len=(int)strlen(buf);  
if(len<4||strcmp(buf+len-4,".png")!=0)strcat(buf,".png");  
if(stbi_write_png(buf,g_preview.w,g_preview.h,4,g_preview.d,g_preview.w*4)){  
char s[256];sprintf(s,"Saved: %s",buf);setStatus(s);  
MessageBoxA(g_hw,s,"Done",MB_OK|MB_ICONINFORMATION);}  
else{setStatus("Error: save failed");  
MessageBoxA(g_hw,"Save failed!","Error",MB_OK|MB_ICONERROR);}}
static void drawPreview(HDC hdc,RECT* rc){  
int rw=rc->right-rc->left,rh=rc->bottom-rc->top;  
if(!g_preview.ok()){  
SetTextColor(hdc,RGB(180,180,180));SetBkMode(hdc,TRANSPARENT);  
DrawTextA(hdc,"Click [Refresh] to generate preview",-1,rc,DT_CENTER|DT_VCENTER|DT_SINGLELINE);  
return;}  
int pw=g_preview.w,ph=g_preview.h;  
int imgW=pw*g_zoom,imgH=ph*g_zoom;  
int offX=rc->left+(rw-imgW)/2,offY=rc->top+(rh-imgH)/2;  
if(offX<rc->left)offX=rc->left;if(offY<rc->top)offY=rc->top;  
unsigned char* bgra=(unsigned char*)malloc(pw*ph*4);  
if(!bgra)return;  
for(int y=0;y<ph;y++)for(int x=0;x<pw;x++){  
int i=(y*pw+x)*4;float a=g_preview.d[i+3]/255.0f;  
int cx=x/8,cy=y/8;unsigned char bg=((cx+cy)%2==0)?180:220;  
bgra[i+0]=(unsigned char)(g_preview.d[i+2]*a+bg*(1.0f-a));  
bgra[i+1]=(unsigned char)(g_preview.d[i+1]*a+bg*(1.0f-a));  
bgra[i+2]=(unsigned char)(g_preview.d[i+0]*a+bg*(1.0f-a));  
bgra[i+3]=255;}  
BITMAPINFO bi;memset(&bi,0,sizeof(bi));  
bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);  
bi.bmiHeader.biWidth=pw;bi.bmiHeader.biHeight=-ph;  
bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;  
bi.bmiHeader.biCompression=BI_RGB;  
StretchDIBits(hdc,offX,offY,imgW,imgH,0,0,pw,ph,bgra,&bi,DIB_RGB_COLORS,SRCCOPY);  
free(bgra);  
if(g_grid){  
HPEN pen=CreatePen(PS_SOLID,1,RGB(80,80,80));  
HPEN old=(HPEN)SelectObject(hdc,pen);  
for(int x=0;x<pw;x++){int sx=offX+x*g_zoom;MoveToEx(hdc,sx,offY,NULL);LineTo(hdc,sx,offY+imgH);}  
for(int y=0;y<ph;y++){int sy=offY+y*g_zoom;MoveToEx(hdc,offX,sy,NULL);LineTo(hdc,offX+imgW,sy);}  
SelectObject(hdc,old);DeleteObject(pen);}}  
  
LRESULT CALLBACK awProc(HWND hw,UINT msg,WPARAM wp,LPARAM lp){  
switch(msg){  
case WM_COMMAND:  
switch(LOWORD(wp)){  
case ID_AWB:{char buf[MAX_PATH]="";if(browseImg(hw,buf,MAX_PATH))SetDlgItemTextA(hw,ID_AWP,buf);break;}  
case ID_AWOK:{  
char nm[128]="",pt[MAX_PATH]="",xb[32]="0",yb[32]="0";  
char rb[16]="86",gb[16]="86",bb[16]="102",radb[16]="3";  
GetDlgItemTextA(hw,ID_AWN,nm,128);GetDlgItemTextA(hw,ID_AWP,pt,MAX_PATH);  
GetDlgItemTextA(hw,ID_AWX,xb,32);GetDlgItemTextA(hw,ID_AWY,yb,32);  
GetDlgItemTextA(hw,ID_AWOR,rb,16);GetDlgItemTextA(hw,ID_AWOG,gb,16);  
GetDlgItemTextA(hw,ID_AWOB,bb,16);GetDlgItemTextA(hw,ID_AWORAD,radb,16);  
if(!strlen(nm)){MessageBoxA(hw,"Name cannot be empty","Error",MB_OK|MB_ICONERROR);break;}  
WD d;d.nm=nm;d.pt=pt;d.x=(float)atof(xb);d.y=(float)atof(yb);  
d.mir=SendDlgItemMessage(hw,ID_AWM,BM_GETCHECK,0,0)==BST_CHECKED;  
d.op.on=SendDlgItemMessage(hw,ID_AWO,BM_GETCHECK,0,0)==BST_CHECKED;  
d.op.r=(unsigned char)atoi(rb);d.op.g=(unsigned char)atoi(gb);  
d.op.b=(unsigned char)atoi(bb);d.op.rad=atoi(radb);if(d.op.rad<1)d.op.rad=1;  
g_wdefs.push_back(d);updList();DestroyWindow(hw);break;}  
case ID_AWCA:DestroyWindow(hw);break;}  
break;  
case WM_DESTROY:EnableWindow(g_hw,TRUE);SetForegroundWindow(g_hw);g_aw=NULL;break;}  
return DefWindowProcA(hw,msg,wp,lp);}  
  
static void showAW(){  
if(g_aw)return;  
HINSTANCE hi=GetModuleHandle(NULL);  
g_aw=CreateWindowExA(0,"AWClass","Add Weapon",  
WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN,  
CW_USEDEFAULT,CW_USEDEFAULT,390,320,g_hw,NULL,hi,NULL);  
if(!g_aw)return;  
int lw=60,ew=200,bw=60,h=22,pad=5,y=10;  
CreateWindowA("STATIC","Name:",WS_CHILD|WS_VISIBLE,10,y,lw,h,g_aw,NULL,hi,NULL);  
CreateWindowA("EDIT","",WS_CHILD|WS_VISIBLE|WS_BORDER,10+lw+pad,y,ew,h,g_aw,(HMENU)ID_AWN,hi,NULL);y+=h+pad;  
CreateWindowA("STATIC","Image:",WS_CHILD|WS_VISIBLE,10,y,lw,h,g_aw,NULL,hi,NULL);  
CreateWindowA("EDIT","",WS_CHILD|WS_VISIBLE|WS_BORDER,10+lw+pad,y,ew-bw-pad,h,g_aw,(HMENU)ID_AWP,hi,NULL);  
CreateWindowA("BUTTON","...",WS_CHILD|WS_VISIBLE,10+lw+pad+ew-bw,y,bw,h,g_aw,(HMENU)ID_AWB,hi,NULL);y+=h+pad;  
CreateWindowA("STATIC","X:",WS_CHILD|WS_VISIBLE,10,y,lw,h,g_aw,NULL,hi,NULL);  
CreateWindowA("EDIT","0",WS_CHILD|WS_VISIBLE|WS_BORDER,10+lw+pad,y,80,h,g_aw,(HMENU)ID_AWX,hi,NULL);y+=h+pad;  
CreateWindowA("STATIC","Y:",WS_CHILD|WS_VISIBLE,10,y,lw,h,g_aw,NULL,hi,NULL);  
CreateWindowA("EDIT","0",WS_CHILD|WS_VISIBLE|WS_BORDER,10+lw+pad,y,80,h,g_aw,(HMENU)ID_AWY,hi,NULL);y+=h+pad;  
HWND hm=CreateWindowA("BUTTON","Mirror (auto left side)",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,10,y,260,h,g_aw,(HMENU)ID_AWM,hi,NULL);  
SendMessage(hm,BM_SETCHECK,BST_CHECKED,0);y+=h+pad;  
CreateWindowA("BUTTON","Apply Outline",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,10,y,150,h,g_aw,(HMENU)ID_AWO,hi,NULL);y+=h+pad;  
CreateWindowA("STATIC","R:",WS_CHILD|WS_VISIBLE,10,y,20,h,g_aw,NULL,hi,NULL);  
CreateWindowA("EDIT","86",WS_CHILD|WS_VISIBLE|WS_BORDER,30,y,38,h,g_aw,(HMENU)ID_AWOR,hi,NULL);  
CreateWindowA("STATIC","G:",WS_CHILD|WS_VISIBLE,78,y,20,h,g_aw,NULL,hi,NULL);  
CreateWindowA("EDIT","86",WS_CHILD|WS_VISIBLE|WS_BORDER,98,y,38,h,g_aw,(HMENU)ID_AWOG,hi,NULL);  
CreateWindowA("STATIC","B:",WS_CHILD|WS_VISIBLE,146,y,20,h,g_aw,NULL,hi,NULL);  
CreateWindowA("EDIT","102",WS_CHILD|WS_VISIBLE|WS_BORDER,166,y,38,h,g_aw,(HMENU)ID_AWOB,hi,NULL);  
CreateWindowA("STATIC","Rad:",WS_CHILD|WS_VISIBLE,214,y,30,h,g_aw,NULL,hi,NULL);  
CreateWindowA("EDIT","3",WS_CHILD|WS_VISIBLE|WS_BORDER,248,y,38,h,g_aw,(HMENU)ID_AWORAD,hi,NULL);y+=h+pad+5;  
CreateWindowA("BUTTON","OK",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,10,y,80,h+4,g_aw,(HMENU)ID_AWOK,hi,NULL);  
CreateWindowA("BUTTON","Cancel",WS_CHILD|WS_VISIBLE,100,y,80,h+4,g_aw,(HMENU)ID_AWCA,hi,NULL);  
EnableWindow(g_hw,FALSE);ShowWindow(g_aw,SW_SHOW);UpdateWindow(g_aw);}  
  
LRESULT CALLBACK wndProc(HWND hw,UINT msg,WPARAM wp,LPARAM lp){  
HINSTANCE hi=GetModuleHandle(NULL);  
switch(msg){  
case WM_CREATE:{  
g_hw=hw;  
int h=22,pad=5,y=5;  
CreateWindowA("STATIC","Unit Body",WS_CHILD|WS_VISIBLE,5,y,LW-10,h,hw,NULL,hi,NULL);y+=h+pad;  
CreateWindowA("EDIT","",WS_CHILD|WS_VISIBLE|WS_BORDER,5,y,LW-65,h,hw,(HMENU)ID_UP,hi,NULL);  
CreateWindowA("BUTTON","Browse",WS_CHILD|WS_VISIBLE,LW-58,y,55,h,hw,(HMENU)ID_UB,hi,NULL);y+=h+pad;  
CreateWindowA("STATIC","(0 = transparent 32x32)",WS_CHILD|WS_VISIBLE,5,y,LW-10,h,hw,NULL,hi,NULL);y+=h+pad;  
CreateWindowA("BUTTON","Apply Outline to Unit",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,5,y,LW-10,h,hw,(HMENU)ID_UO,hi,NULL);y+=h+pad;  
CreateWindowA("STATIC","R:",WS_CHILD|WS_VISIBLE,5,y,20,h,hw,NULL,hi,NULL);  
CreateWindowA("EDIT","86",WS_CHILD|WS_VISIBLE|WS_BORDER,25,y,38,h,hw,(HMENU)ID_UOR,hi,NULL);  
CreateWindowA("STATIC","G:",WS_CHILD|WS_VISIBLE,73,y,20,h,hw,NULL,hi,NULL);  
CreateWindowA("EDIT","86",WS_CHILD|WS_VISIBLE|WS_BORDER,93,y,38,h,hw,(HMENU)ID_UOG,hi,NULL);  
CreateWindowA("STATIC","B:",WS_CHILD|WS_VISIBLE,141,y,20,h,hw,NULL,hi,NULL);  
CreateWindowA("EDIT","102",WS_CHILD|WS_VISIBLE|WS_BORDER,161,y,38,h,hw,(HMENU)ID_UOB,hi,NULL);  
CreateWindowA("STATIC","Rad:",WS_CHILD|WS_VISIBLE,209,y,30,h,hw,NULL,hi,NULL);  
CreateWindowA("EDIT","3",WS_CHILD|WS_VISIBLE|WS_BORDER,242,y,28,h,hw,(HMENU)ID_UORAD,hi,NULL);y+=h+pad+5;  
CreateWindowA("STATIC","Weapons",WS_CHILD|WS_VISIBLE,5,y,LW-10,h,hw,NULL,hi,NULL);y+=h+pad;  
CreateWindowA("LISTBOX","",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|LBS_NOINTEGRALHEIGHT,5,y,LW-10,150,hw,(HMENU)ID_WL,hi,NULL);y+=155;  
CreateWindowA("BUTTON","Add",WS_CHILD|WS_VISIBLE,5,y,58,h,hw,(HMENU)ID_WA,hi,NULL);  
CreateWindowA("BUTTON","Remove",WS_CHILD|WS_VISIBLE,68,y,65,h,hw,(HMENU)ID_WR,hi,NULL);  
CreateWindowA("BUTTON","Clear All",WS_CHILD|WS_VISIBLE,138,y,70,h,hw,(HMENU)ID_WC,hi,NULL);y+=h+pad+5;  
CreateWindowA("STATIC","Output",WS_CHILD|WS_VISIBLE,5,y,LW-10,h,hw,NULL,hi,NULL);y+=h+pad;  
CreateWindowA("EDIT","output.png",WS_CHILD|WS_VISIBLE|WS_BORDER,5,y,LW-65,h,hw,(HMENU)ID_OP,hi,NULL);  
CreateWindowA("BUTTON","Browse",WS_CHILD|WS_VISIBLE,LW-58,y,55,h,hw,(HMENU)ID_OB,hi,NULL);y+=h+pad;  
CreateWindowA("BUTTON","Generate PNG",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,5,y,LW-10,h+4,hw,(HMENU)ID_GEN,hi,NULL);y+=h+pad+10;  
CreateWindowA("STATIC","View",WS_CHILD|WS_VISIBLE,5,y,LW-10,h,hw,NULL,hi,NULL);y+=h+pad;  
CreateWindowA("BUTTON","Zoom+",WS_CHILD|WS_VISIBLE,5,y,55,h,hw,(HMENU)ID_ZI,hi,NULL);  
CreateWindowA("BUTTON","Zoom-",WS_CHILD|WS_VISIBLE,65,y,55,h,hw,(HMENU)ID_ZO,hi,NULL);  
CreateWindowA("BUTTON","Grid",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,125,y,50,h,hw,(HMENU)ID_GR,hi,NULL);  
CreateWindowA("BUTTON","Refresh",WS_CHILD|WS_VISIBLE,180,y,LW-185,h,hw,(HMENU)ID_RF,hi,NULL);  
RECT rc2;GetClientRect(hw,&rc2);  
CreateWindowA("STATIC","Ready",WS_CHILD|WS_VISIBLE|SS_SUNKEN,0,rc2.bottom-SH,rc2.right,SH,hw,(HMENU)ID_ST,hi,NULL);  
break;}  
case WM_COMMAND:  
switch(LOWORD(wp)){  
case ID_UB:{char buf[MAX_PATH]="";if(browseImg(hw,buf,MAX_PATH)){SetDlgItemTextA(hw,ID_UP,buf);loadUnit();}break;}  
case ID_WA:showAW();break;  
case ID_WR:{int sel=(int)SendDlgItemMessage(hw,ID_WL,LB_GETCURSEL,0,0);  
if(sel!=LB_ERR&&sel<(int)g_wdefs.size()){g_wdefs.erase(g_wdefs.begin()+sel);updList();}break;}  
case ID_WC:g_wdefs.clear();updList();break;  
case ID_OB:{char buf[MAX_PATH]="output.png";if(browseSave(hw,buf,MAX_PATH))SetDlgItemTextA(hw,ID_OP,buf);break;}  
case ID_GEN:doGenerate();break;  
case ID_ZI:if(g_zoom<8)g_zoom++;InvalidateRect(hw,NULL,FALSE);break;  
case ID_ZO:if(g_zoom>1)g_zoom--;InvalidateRect(hw,NULL,FALSE);break;  
case ID_GR:g_grid=SendDlgItemMessage(hw,ID_GR,BM_GETCHECK,0,0)==BST_CHECKED;InvalidateRect(hw,NULL,FALSE);break;  
case ID_RF:loadUnit();doRefresh();break;}  
break;  
case WM_PAINT:{PAINTSTRUCT ps;HDC hdc=BeginPaint(hw,&ps);RECT rc;GetClientRect(hw,&rc);rc.left=LW;rc.bottom-=SH;drawPreview(hdc,&rc);EndPaint(hw,&ps);break;}  
case WM_ERASEBKGND:{HDC hdc=(HDC)wp;RECT rc;GetClientRect(hw,&rc);  
HBRUSH b=GetSysColorBrush(COLOR_BTNFACE);RECT lr=rc;lr.right=LW;FillRect(hdc,&lr,b);  
HBRUSH db=CreateSolidBrush(RGB(45,45,55));RECT rr=rc;rr.left=LW;FillRect(hdc,&rr,db);DeleteObject(db);return 1;}  
case WM_DROPFILES:{HDROP hd=(HDROP)wp;char buf[MAX_PATH]="";DragQueryFileA(hd,0,buf,MAX_PATH);DragFinish(hd);SetDlgItemTextA(hw,ID_UP,buf);loadUnit();break;}  
case WM_DESTROY:PostQuitMessage(0);break;}  
return DefWindowProcA(hw,msg,wp,lp);}  
  
int WINAPI WinMain(HINSTANCE hi,HINSTANCE,LPSTR,int nShow){  
WNDCLASSA wc;memset(&wc,0,sizeof(wc));  
wc.lpfnWndProc=wndProc;wc.hInstance=hi;  
wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);  
wc.lpszClassName="MWClass";  
wc.hCursor=LoadCursor(NULL,IDC_ARROW);  
wc.hIcon=LoadIcon(NULL,IDI_APPLICATION);  
RegisterClassA(&wc);  
WNDCLASSA aw;memset(&aw,0,sizeof(aw));  
aw.lpfnWndProc=awProc;aw.hInstance=hi;  
aw.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);  
aw.lpszClassName="AWClass";  
aw.hCursor=LoadCursor(NULL,IDC_ARROW);  
RegisterClassA(&aw);  
g_hw=CreateWindowExA(WS_EX_ACCEPTFILES,"MWClass",  
"Mindustry Weapon Position Compositor",  
WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,  
CW_USEDEFAULT,CW_USEDEFAULT,910,640,NULL,NULL,hi,NULL);  
ShowWindow(g_hw,nShow);UpdateWindow(g_hw);  
MSG msg;  
while(GetMessageA(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessageA(&msg);}  
return (int)msg.wParam;}
