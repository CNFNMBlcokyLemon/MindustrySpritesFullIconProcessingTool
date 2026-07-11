
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
#include <math.h>
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
#define ID_UOOL 120
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
#define ID_AWBR 212
#define ID_AWOK 213
#define ID_AWCA 214
#define ID_AWOOL 215

static inline float fmn(float a,float b) {
	return a<b?a:b;
}
static inline float fmx(float a,float b) {
	return a>b?a:b;
}

class Img {
	public:
		unsigned char* d;
		int w,h;
		Img():d(NULL),w(0),h(0) {}
		bool ok()const {
			return d!=NULL;
		}
		bool load(const char* path) {
			free2();
			int ch;
			d=stbi_load(path,&w,&h,&ch,4);
			return d!=NULL;
		}
		void free2() {
			if(d) {
				stbi_image_free(d);
				d=NULL;
				w=0;
				h=0;
			}
		}
		Img flipH()const {
			Img r;
			r.w=w;
			r.h=h;
			r.d=(unsigned char*)malloc(w*h*4);
			if(!r.d)return r;
			for(int y=0; y<h; y++)for(int x=0; x<w; x++) {
					int s=(y*w+(w-1-x))*4,t=(y*w+x)*4;
					r.d[t]=d[s];
					r.d[t+1]=d[s+1];
					r.d[t+2]=d[s+2];
					r.d[t+3]=d[s+3];
				}
			return r;
		}
};

class OP {
	public:
		bool on,outlineOnly;
		unsigned char r,g,b;
		int rad;
		OP():on(false),outlineOnly(false),r(86),g(86),b(102),rad(3) {}
};

class WD {
	public:
		std::string nm,pt;
		float x,y,baseRot;
		bool mir;
		OP op;
		WD():x(0),y(0),baseRot(0),mir(true) {}
};

class WI {
	public:
		std::string nm,pt;
		float x,y,baseRot;
		bool fl;
		OP op;
		WI():x(0),y(0),baseRot(0),fl(false) {}
};

class Cv {
	public:
		unsigned char* d;
		int w,h;
		Cv(int w_,int h_):w(w_),h(h_) {
			d=(unsigned char*)calloc(w*h*4,1);
		}
		~Cv() {
			if(d)free(d);
		}
		void blit(const Img& img,int cx,int cy) {
			if(!img.d)return;
			int ox=cx-img.w/2,oy=cy-img.h/2;
			for(int iy=0; iy<img.h; iy++)for(int ix=0; ix<img.w; ix++) {
					int px=ox+ix,py=oy+iy;
					if(px<0||px>=w||py<0||py>=h)continue;
					int si=(iy*img.w+ix)*4,di=(py*w+px)*4;
					float sa=img.d[si+3]/255.0f,da=d[di+3]/255.0f,oa=sa+da*(1.0f-sa);
					if(oa>1e-6f) {
						d[di]=(unsigned char)((img.d[si]*sa+d[di]*da*(1.0f-sa))/oa+0.5f);
						d[di+1]=(unsigned char)((img.d[si+1]*sa+d[di+1]*da*(1.0f-sa))/oa+0.5f);
						d[di+2]=(unsigned char)((img.d[si+2]*sa+d[di+2]*da*(1.0f-sa))/oa+0.5f);
						d[di+3]=(unsigned char)(oa*255.0f+0.5f);
					}
				}
		}
};

static Img mkOutline(const Img& src,unsigned char oR,unsigned char oG,unsigned char oB,int rad,bool outlineOnly) {
	Img r;
	r.w=src.w;
	r.h=src.h;
	r.d=(unsigned char*)calloc(src.w*src.h*4,1);
	if(!r.d)return r;
	for(int y=0; y<src.h; y++)for(int x=0; x<src.w; x++) {
			int si=(y*src.w+x)*4;
			if(src.d[si+3]>0) {
				if(!outlineOnly) {
					r.d[si]=src.d[si];
					r.d[si+1]=src.d[si+1];
					r.d[si+2]=src.d[si+2];
					r.d[si+3]=src.d[si+3];
				}
				continue;
			}
			bool found=false;
			for(int dy=-rad; dy<=rad&&!found; dy++)for(int dx=-rad; dx<=rad&&!found; dx++) {
					if(dx*dx+dy*dy>rad*rad)continue;
					int nx=x+dx,ny=y+dy;
					if(nx<0||nx>=src.w||ny<0||ny>=src.h)continue;
					if(src.d[(ny*src.w+nx)*4+3]>0)found=true;
				}
			if(found) {
				r.d[si]=oR;
				r.d[si+1]=oG;
				r.d[si+2]=oB;
				r.d[si+3]=255;
			}
		}
	return r;
}

static Img rotateImg(const Img& src,float angle) {
	Img r;
	r.d=NULL;
	r.w=0;
	r.h=0;
	if(!src.d)return r;
	const float PI=3.14159265f;
	// 修复：原代码用了未定义的 rad2，改为正确的弧度转换
	float rad=angle*PI/180.0f;
	float cosA=(float)cos(rad),sinA=(float)sin(rad);
	int newW=(int)(fabs((double)src.w*cosA)+fabs((double)src.h*sinA)+1.5f);
	int newH=(int)(fabs((double)src.w*sinA)+fabs((double)src.h*cosA)+1.5f);
	if(newW<1)newW=1;
	if(newH<1)newH=1;
	r.w=newW;
	r.h=newH;
	r.d=(unsigned char*)calloc(newW*newH*4,1);
	if(!r.d)return r;
	float cx=src.w/2.0f,cy=src.h/2.0f,ncx=newW/2.0f,ncy=newH/2.0f;
	int ny,nx,c;
	for(ny=0; ny<newH; ny++)for(nx=0; nx<newW; nx++) {
			float dx=nx-ncx,dy=ny-ncy;
			float ix=dx*cosA+dy*sinA+cx,iy=-dx*sinA+dy*cosA+cy;
			if(ix<0||ix>src.w-1||iy<0||iy>src.h-1)continue;
			int x0=(int)ix,y0=(int)iy,x1=x0+1,y1=y0+1;
			if(x1>=src.w)x1=src.w-1;
			if(y1>=src.h)y1=src.h-1;
			float fx=ix-x0,fy=iy-y0;
			int di=(ny*newW+nx)*4;
			for(c=0; c<4; c++) {
				float v=src.d[(y0*src.w+x0)*4+c]*(1-fx)*(1-fy)
				        +src.d[(y0*src.w+x1)*4+c]*fx*(1-fy)
				        +src.d[(y1*src.w+x0)*4+c]*(1-fx)*fy
				        +src.d[(y1*src.w+x1)*4+c]*fx*fy;
				r.d[di+c]=(unsigned char)(v+0.5f);
			}
		}
	return r;
}

static std::vector<WI> expand(const std::vector<WD>& defs) {
	std::vector<WI> result;
	for(int i=0; i<(int)defs.size(); i++) {
		WI wi;
		wi.nm=defs[i].nm;
		wi.pt=defs[i].pt;
		wi.x=defs[i].x;
		wi.y=defs[i].y;
		wi.baseRot=defs[i].baseRot;
		wi.fl=false;
		wi.op=defs[i].op;
		result.push_back(wi);
		if(defs[i].mir) {
			WI mir=wi;
			mir.x=-wi.x;
			mir.fl=true;
			mir.baseRot=-wi.baseRot;
			result.push_back(mir);
		}
	}
	return result;
}

static HWND g_hw=NULL,g_wlb=NULL,g_stb=NULL,g_aw=NULL;
static bool g_awOK=false;
static WD g_awResult;
static Img g_unit,g_preview;
static OP g_uop;
static std::vector<WD> g_wdefs;
static int g_zoom=2;
static bool g_grid=false;

static void setStatus(const char* s) {
	if(g_stb)SetWindowTextA(g_stb,s);
}

static bool browseImg(HWND hw,char* buf,int size) {
	OPENFILENAMEA ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner=hw;
	ofn.lpstrFilter="PNG Images\0*.png\0All Files\0*.*\0";
	ofn.lpstrFile=buf;
	ofn.nMaxFile=size;
	ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
	return GetOpenFileNameA(&ofn)!=0;
}

static bool browseSave(HWND hw,char* buf,int size) {
	OPENFILENAMEA ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner=hw;
	ofn.lpstrFilter="PNG Images\0*.png\0";
	ofn.lpstrFile=buf;
	ofn.nMaxFile=size;
	ofn.lpstrDefExt="png";
	ofn.Flags=OFN_OVERWRITEPROMPT;
	return GetSaveFileNameA(&ofn)!=0;
}

static void updList() {
	if(!g_wlb)return;
	SendMessageA(g_wlb,LB_RESETCONTENT,0,0);
	for(int i=0; i<(int)g_wdefs.size(); i++) {
		char buf[256];
		sprintf(buf,"%s (%.1f,%.1f) rot=%.1f%s",
		        g_wdefs[i].nm.c_str(),g_wdefs[i].x,g_wdefs[i].y,g_wdefs[i].baseRot,
		        g_wdefs[i].mir?" [M]":"");
		SendMessageA(g_wlb,LB_ADDSTRING,0,(LPARAM)buf);
	}
}

static void loadUnit() {
	char buf[MAX_PATH]="";
	HWND h=GetDlgItem(g_hw,ID_UP);
	if(h)GetWindowTextA(h,buf,MAX_PATH);
	g_unit.free2();
	if(strcmp(buf,"0")==0) {
		g_unit.w=32;
		g_unit.h=32;
		g_unit.d=(unsigned char*)calloc(32*32*4,1);
		setStatus("Unit: 32x32 transparent placeholder");
	} else if(strlen(buf)) {
		if(g_unit.load(buf)) {
			char s[256];
			sprintf(s,"Unit: %dx%d loaded",g_unit.w,g_unit.h);
			setStatus(s);
		} else {
			char s[256];
			sprintf(s,"Error: cannot load '%s'",buf);
			setStatus(s);
		}
	}
}

static void doRefresh() {
	char rb[16]="86",gb[16]="86",bb[16]="102",radb[16]="3";
	GetDlgItemTextA(g_hw,ID_UOR,rb,16);
	GetDlgItemTextA(g_hw,ID_UOG,gb,16);
	GetDlgItemTextA(g_hw,ID_UOB,bb,16);
	GetDlgItemTextA(g_hw,ID_UORAD,radb,16);
	g_uop.r=(unsigned char)atoi(rb);
	g_uop.g=(unsigned char)atoi(gb);
	g_uop.b=(unsigned char)atoi(bb);
	g_uop.rad=atoi(radb);
	if(g_uop.rad<1)g_uop.rad=1;
	g_uop.on=SendDlgItemMessage(g_hw,ID_UO,BM_GETCHECK,0,0)==BST_CHECKED;
	g_uop.outlineOnly=SendDlgItemMessage(g_hw,ID_UOOL,BM_GETCHECK,0,0)==BST_CHECKED;
	std::vector<WI> ws=expand(g_wdefs);
	std::vector<Img> imgs(ws.size());
	for(int i=0; i<(int)ws.size(); i++)
		if(!imgs[i].load(ws[i].pt.c_str())) {
			char s[256];
			sprintf(s,"Warn: cannot load '%s'",ws[i].pt.c_str());
			setStatus(s);
		}
	float mnX=g_unit.ok()?-(float)g_unit.w/2.0f:-16.0f,mxX=g_unit.ok()?(float)g_unit.w/2.0f:16.0f;
	float mnY=g_unit.ok()?-(float)g_unit.h/2.0f:-16.0f,mxY=g_unit.ok()?(float)g_unit.h/2.0f:16.0f;
	for(int i=0; i<(int)ws.size(); i++) {
		if(!imgs[i].ok())continue;
		float hw2=imgs[i].w/2.0f,hh2=imgs[i].h/2.0f;
		mnX=fmn(mnX,ws[i].x-hw2);
		mxX=fmx(mxX,ws[i].x+hw2);
		mnY=fmn(mnY,ws[i].y-hh2);
		mxY=fmx(mxY,ws[i].y+hh2);
	}
	const int PAD=8;
	int cw=(int)(mxX-mnX+0.5f)+PAD*2,ch=(int)(mxY-mnY+0.5f)+PAD*2;
	if(cw<1)cw=1;
	if(ch<1)ch=1;
	int ucx=(int)(-mnX+0.5f)+PAD,ucy=(int)(mxY+0.5f)+PAD;
	Cv cv(cw,ch);
	if(g_unit.ok()) {
		if(g_uop.on) {
			Img ol=mkOutline(g_unit,g_uop.r,g_uop.g,g_uop.b,g_uop.rad,g_uop.outlineOnly);
			cv.blit(ol,ucx,ucy);
			ol.free2();
		} else cv.blit(g_unit,ucx,ucy);
	}
	for(int i=0; i<(int)ws.size(); i++) {
		if(!imgs[i].ok())continue;
		int px=ucx+(int)ws[i].x,py=ucy-(int)ws[i].y;
		Img* src=&imgs[i];
		Img fl,ol,rot;
		if(ws[i].fl) {
			fl=imgs[i].flipH();
			src=&fl;
		}
		if(ws[i].op.on) {
			ol=mkOutline(*src,ws[i].op.r,ws[i].op.g,ws[i].op.b,ws[i].op.rad,ws[i].op.outlineOnly);
			src=&ol;
		}
		if(ws[i].baseRot!=0.0f) {
			rot=rotateImg(*src,-ws[i].baseRot);
			src=&rot;
		}
		cv.blit(*src,px,py);
		if(fl.d)free(fl.d);
		if(ol.d)free(ol.d);
		if(rot.d)free(rot.d);
	}
	g_preview.free2();
	g_preview.w=cw;
	g_preview.h=ch;
	g_preview.d=(unsigned char*)malloc(cw*ch*4);
	if(g_preview.d)memcpy(g_preview.d,cv.d,cw*ch*4);
	for(int i=0; i<(int)imgs.size(); i++)imgs[i].free2();
	char s[128];
	sprintf(s,"Preview: %dx%d  Zoom:%dx  Weapons:%d",cw,ch,g_zoom,(int)g_wdefs.size());
	setStatus(s);
	InvalidateRect(g_hw,NULL,FALSE);
}

static void doGenerate() {
	if(!g_preview.ok()) {
		setStatus("No preview - click Refresh first");
		return;
	}
	char buf[MAX_PATH]="output.png";
	HWND h=GetDlgItem(g_hw,ID_OP);
	if(h)GetWindowTextA(h,buf,MAX_PATH);
	if(!strlen(buf))strcpy(buf,"output.png");
	int len=(int)strlen(buf);
	if(len<4||strcmp(buf+len-4,".png")!=0)strcat(buf,".png");
	if(stbi_write_png(buf,g_preview.w,g_preview.h,4,g_preview.d,g_preview.w*4)) {
		char s[256];
		sprintf(s,"Saved: %s",buf);
		setStatus(s);
		MessageBoxA(g_hw,s,"Done",MB_OK|MB_ICONINFORMATION);
	} else {
		setStatus("Error: save failed");
		MessageBoxA(g_hw,"Save failed!","Error",MB_OK|MB_ICONERROR);
	}
}

static void drawPreview(HDC hdc,RECT* rc) {
	HBRUSH db=CreateSolidBrush(RGB(45,45,55));
	FillRect(hdc,rc,db);
	DeleteObject(db);
	if(!g_preview.ok()) {
		SetTextColor(hdc,RGB(180,180,180));
		SetBkMode(hdc,TRANSPARENT);
		DrawTextA(hdc,"Click [Refresh] to generate preview",-1,rc,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
		return;
	}
	int pw=g_preview.w,ph=g_preview.h;
	int imgW=pw*g_zoom,imgH=ph*g_zoom;
	int rcW=rc->right-rc->left,rcH=rc->bottom-rc->top;
	int offX=rc->left+(rcW-imgW)/2,offY=rc->top+(rcH-imgH)/2;
	if(offX<rc->left)offX=rc->left;
	if(offY<rc->top)offY=rc->top;
	unsigned char* bgra=(unsigned char*)malloc(pw*ph*4);
	if(!bgra)return;
	for(int y=0; y<ph; y++)for(int x=0; x<pw; x++) {
			int i=(y*pw+x)*4;
			float a=g_preview.d[i+3]/255.0f;
			int cx=x/8,cy=y/8;
			unsigned char bg=((cx+cy)%2==0)?180:220;
			bgra[i+0]=(unsigned char)(g_preview.d[i+2]*a+bg*(1.0f-a));
			bgra[i+1]=(unsigned char)(g_preview.d[i+1]*a+bg*(1.0f-a));
			bgra[i+2]=(unsigned char)(g_preview.d[i+0]*a+bg*(1.0f-a));
			bgra[i+3]=255;
		}
	BITMAPINFO bi;
	memset(&bi,0,sizeof(bi));
	bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth=pw;
	bi.bmiHeader.biHeight=-ph;
	bi.bmiHeader.biPlanes=1;
	bi.bmiHeader.biBitCount=32;
	bi.bmiHeader.biCompression=BI_RGB;
	StretchDIBits(hdc,offX,offY,imgW,imgH,0,0,pw,ph,bgra,&bi,DIB_RGB_COLORS,SRCCOPY);
	free(bgra);
	if(g_grid) {
		HPEN pen=CreatePen(PS_SOLID,1,RGB(80,80,80));
		HPEN old=(HPEN)SelectObject(hdc,pen);
		for(int x=0; x<=pw; x++) {
			int sx=offX+x*g_zoom;
			MoveToEx(hdc,sx,offY,NULL);
			LineTo(hdc,sx,offY+imgH);
		}
		for(int y=0; y<=ph; y++) {
			int sy=offY+y*g_zoom;
			MoveToEx(hdc,offX,sy,NULL);
			LineTo(hdc,offX+imgW,sy);
		}
		SelectObject(hdc,old);
		DeleteObject(pen);
	}
}

LRESULT CALLBACK awProc(HWND hw,UINT msg,WPARAM wp,LPARAM lp) {
	switch(msg) {
		case WM_COMMAND:
			switch(LOWORD(wp)) {
				case ID_AWB: {
					char buf[MAX_PATH]="";
					if(browseImg(hw,buf,MAX_PATH))SetDlgItemTextA(hw,ID_AWP,buf);
					break;
				}
				case ID_AWOK: {
					char nm[128]="",pt[MAX_PATH]="",xb[32]="0",yb[32]="0",brb[32]="0";
					GetDlgItemTextA(hw,ID_AWN,nm,128);
					GetDlgItemTextA(hw,ID_AWP,pt,MAX_PATH);
					GetDlgItemTextA(hw,ID_AWX,xb,32);
					GetDlgItemTextA(hw,ID_AWY,yb,32);
					GetDlgItemTextA(hw,ID_AWBR,brb,32);
					if(!strlen(nm)) {
						MessageBoxA(hw,"Name cannot be empty","Error",MB_OK|MB_ICONERROR);
						break;
					}
					WD d;
					d.nm=nm;
					d.pt=pt;
					d.x=(float)atof(xb);
					d.y=(float)atof(yb);
					d.baseRot=(float)atof(brb);
					d.mir=SendDlgItemMessage(hw,ID_AWM,BM_GETCHECK,0,0)==BST_CHECKED;
					d.op.on=SendDlgItemMessage(hw,ID_AWO,BM_GETCHECK,0,0)==BST_CHECKED;
					d.op.outlineOnly=SendDlgItemMessage(hw,ID_AWOOL,BM_GETCHECK,0,0)==BST_CHECKED;
					char rb[16]="86",gb[16]="86",bb[16]="102",radb[16]="3";
					GetDlgItemTextA(hw,ID_AWOR,rb,16);
					GetDlgItemTextA(hw,ID_AWOG,gb,16);
					GetDlgItemTextA(hw,ID_AWOB,bb,16);
					GetDlgItemTextA(hw,ID_AWORAD,radb,16);
					d.op.r=(unsigned char)atoi(rb);
					d.op.g=(unsigned char)atoi(gb);
					d.op.b=(unsigned char)atoi(bb);
					d.op.rad=atoi(radb);
					if(d.op.rad<1)d.op.rad=1;
					g_awResult=d;
					g_awOK=true;
					EnableWindow(g_hw,TRUE);
					DestroyWindow(hw);
					break;
				}
				case ID_AWCA:
					EnableWindow(g_hw,TRUE);
					DestroyWindow(hw);
					break;
			}
			break;
		case WM_DESTROY:
			g_aw=NULL;
			break;
	}
	return DefWindowProcA(hw,msg,wp,lp);
}

static void showAW() {
	if(g_aw)return;
	int h=22,pad=4,y=5;
	HINSTANCE hi=GetModuleHandle(NULL);
	g_aw=CreateWindowExA(WS_EX_DLGMODALFRAME,"AWClass","Add Weapon",
	                     WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN,
	                     CW_USEDEFAULT,CW_USEDEFAULT,290,410,g_hw,NULL,hi,NULL);
	CreateWindowA("STATIC","Name:",WS_CHILD|WS_VISIBLE,5,y,60,h,g_aw,NULL,hi,NULL);
	CreateWindowA("EDIT","",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,70,y,205,h,g_aw,(HMENU)ID_AWN,hi,NULL);
	y+=h+pad;
	CreateWindowA("STATIC","Path:",WS_CHILD|WS_VISIBLE,5,y,60,h,g_aw,NULL,hi,NULL);
	CreateWindowA("EDIT","",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,70,y,160,h,g_aw,(HMENU)ID_AWP,hi,NULL);
	CreateWindowA("BUTTON","...",WS_CHILD|WS_VISIBLE,235,y,40,h,g_aw,(HMENU)ID_AWB,hi,NULL);
	y+=h+pad;
	CreateWindowA("STATIC","X:",WS_CHILD|WS_VISIBLE,5,y,25,h,g_aw,NULL,hi,NULL);
	CreateWindowA("EDIT","0",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,35,y,75,h,g_aw,(HMENU)ID_AWX,hi,NULL);
	CreateWindowA("STATIC","Y:",WS_CHILD|WS_VISIBLE,125,y,25,h,g_aw,NULL,hi,NULL);
	CreateWindowA("EDIT","0",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,155,y,75,h,g_aw,(HMENU)ID_AWY,hi,NULL);
	y+=h+pad;
	CreateWindowA("STATIC","Base Rot:",WS_CHILD|WS_VISIBLE,5,y,65,h,g_aw,NULL,hi,NULL);
	CreateWindowA("EDIT","0",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,75,y,80,h,g_aw,(HMENU)ID_AWBR,hi,NULL);
	y+=h+pad;
	CreateWindowA("BUTTON","Mirror",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,5,y,120,h,g_aw,(HMENU)ID_AWM,hi,NULL);
	SendDlgItemMessage(g_aw,ID_AWM,BM_SETCHECK,BST_CHECKED,0);
	y+=h+pad;
	CreateWindowA("BUTTON","Outline",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,5,y,120,h,g_aw,(HMENU)ID_AWO,hi,NULL);
	y+=h+pad;
	CreateWindowA("STATIC","R:",WS_CHILD|WS_VISIBLE,5,y,20,h,g_aw,NULL,hi,NULL);
	CreateWindowA("EDIT","86",WS_CHILD|WS_VISIBLE|WS_BORDER,28,y,38,h,g_aw,(HMENU)ID_AWOR,hi,NULL);
	CreateWindowA("STATIC","G:",WS_CHILD|WS_VISIBLE,75,y,20,h,g_aw,NULL,hi,NULL);
	CreateWindowA("EDIT","86",WS_CHILD|WS_VISIBLE|WS_BORDER,98,y,38,h,g_aw,(HMENU)ID_AWOG,hi,NULL);
	CreateWindowA("STATIC","B:",WS_CHILD|WS_VISIBLE,145,y,20,h,g_aw,NULL,hi,NULL);
	CreateWindowA("EDIT","102",WS_CHILD|WS_VISIBLE|WS_BORDER,168,y,38,h,g_aw,(HMENU)ID_AWOB,hi,NULL);
	CreateWindowA("STATIC","Rad:",WS_CHILD|WS_VISIBLE,215,y,30,h,g_aw,NULL,hi,NULL);
	CreateWindowA("EDIT","3",WS_CHILD|WS_VISIBLE|WS_BORDER,248,y,28,h,g_aw,(HMENU)ID_AWORAD,hi,NULL);
	y+=h+pad;
	CreateWindowA("BUTTON","Outline Only",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,5,y,200,h,g_aw,(HMENU)ID_AWOOL,hi,NULL);
	y+=h+pad+5;
	CreateWindowA("BUTTON","OK",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,5,y,80,h+4,g_aw,(HMENU)ID_AWOK,hi,NULL);
	CreateWindowA("BUTTON","Cancel",WS_CHILD|WS_VISIBLE,95,y,80,h+4,g_aw,(HMENU)ID_AWCA,hi,NULL);
	EnableWindow(g_hw,FALSE);
	ShowWindow(g_aw,SW_SHOW);
	UpdateWindow(g_aw);
}

LRESULT CALLBACK wndProc(HWND hw,UINT msg,WPARAM wp,LPARAM lp) {
	int h=22,pad=4,y=5;
	HINSTANCE hi=GetModuleHandle(NULL);
	switch(msg) {
		case WM_CREATE: {
			CreateWindowA("STATIC","Unit Body:",WS_CHILD|WS_VISIBLE,5,y,LW-10,h,hw,NULL,hi,NULL);
			y+=h+pad;
			CreateWindowA("EDIT","",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,5,y,200,h,hw,(HMENU)ID_UP,hi,NULL);
			CreateWindowA("BUTTON","...",WS_CHILD|WS_VISIBLE,210,y,55,h,hw,(HMENU)ID_UB,hi,NULL);
			y+=h+pad;
			CreateWindowA("BUTTON","Outline",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,5,y,LW-10,h,hw,(HMENU)ID_UO,hi,NULL);
			y+=h+pad;
			CreateWindowA("STATIC","R:",WS_CHILD|WS_VISIBLE,5,y,20,h,hw,NULL,hi,NULL);
			CreateWindowA("EDIT","86",WS_CHILD|WS_VISIBLE|WS_BORDER,28,y,38,h,hw,(HMENU)ID_UOR,hi,NULL);
			CreateWindowA("STATIC","G:",WS_CHILD|WS_VISIBLE,75,y,20,h,hw,NULL,hi,NULL);
			CreateWindowA("EDIT","86",WS_CHILD|WS_VISIBLE|WS_BORDER,98,y,38,h,hw,(HMENU)ID_UOG,hi,NULL);
			CreateWindowA("STATIC","B:",WS_CHILD|WS_VISIBLE,145,y,20,h,hw,NULL,hi,NULL);
			CreateWindowA("EDIT","102",WS_CHILD|WS_VISIBLE|WS_BORDER,168,y,38,h,hw,(HMENU)ID_UOB,hi,NULL);
			CreateWindowA("STATIC","Rad:",WS_CHILD|WS_VISIBLE,215,y,30,h,hw,NULL,hi,NULL);
			CreateWindowA("EDIT","3",WS_CHILD|WS_VISIBLE|WS_BORDER,248,y,28,h,hw,(HMENU)ID_UORAD,hi,NULL);
			y+=h+pad;
			CreateWindowA("BUTTON","Outline Only",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,5,y,LW-10,h,hw,(HMENU)ID_UOOL,hi,NULL);
			y+=h+pad+5;
			CreateWindowA("STATIC","Weapons:",WS_CHILD|WS_VISIBLE,5,y,LW-10,h,hw,NULL,hi,NULL);
			y+=h+pad;
			g_wlb=CreateWindowA("LISTBOX",NULL,WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|LBS_NOTIFY,5,y,LW-10,150,hw,(HMENU)ID_WL,hi,NULL);
			y+=150+pad;
			CreateWindowA("BUTTON","Add",WS_CHILD|WS_VISIBLE,5,y,80,h,hw,(HMENU)ID_WA,hi,NULL);
			CreateWindowA("BUTTON","Remove",WS_CHILD|WS_VISIBLE,90,y,80,h,hw,(HMENU)ID_WR,hi,NULL);
			CreateWindowA("BUTTON","Clear",WS_CHILD|WS_VISIBLE,175,y,80,h,hw,(HMENU)ID_WC,hi,NULL);
			y+=h+pad+8;
			CreateWindowA("STATIC","Output:",WS_CHILD|WS_VISIBLE,5,y,LW-10,h,hw,NULL,hi,NULL);
			y+=h+pad;
			CreateWindowA("EDIT","output.png",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,5,y,200,h,hw,(HMENU)ID_OP,hi,NULL);
			CreateWindowA("BUTTON","...",WS_CHILD|WS_VISIBLE,210,y,55,h,hw,(HMENU)ID_OB,hi,NULL);
			y+=h+pad;
			CreateWindowA("BUTTON","Generate PNG",WS_CHILD|WS_VISIBLE,5,y,LW-10,h+4,hw,(HMENU)ID_GEN,hi,NULL);
			y+=h+pad+10;
			CreateWindowA("BUTTON","Zoom +",WS_CHILD|WS_VISIBLE,5,y,80,h,hw,(HMENU)ID_ZI,hi,NULL);
			CreateWindowA("BUTTON","Zoom -",WS_CHILD|WS_VISIBLE,90,y,80,h,hw,(HMENU)ID_ZO,hi,NULL);
			y+=h+pad;
			CreateWindowA("BUTTON","Grid",WS_CHILD|WS_VISIBLE,5,y,80,h,hw,(HMENU)ID_GR,hi,NULL);
			CreateWindowA("BUTTON","Refresh",WS_CHILD|WS_VISIBLE,90,y,80,h,hw,(HMENU)ID_RF,hi,NULL);
			RECT rc;
			GetClientRect(hw,&rc);
			g_stb=CreateWindowA("STATIC","Ready",WS_CHILD|WS_VISIBLE|SS_LEFT|SS_SUNKEN,0,rc.bottom-SH,rc.right,SH,hw,(HMENU)ID_ST,hi,NULL);
			break;
		}
		case WM_COMMAND: {
			// 自动 refresh：编辑框失去焦点时触发
			if(HIWORD(wp)==EN_KILLFOCUS) {
				int id=LOWORD(wp);
				if(id==ID_UOR||id==ID_UOG||id==ID_UOB||id==ID_UORAD) {
					doRefresh();
					break;
				}
				if(id==ID_UP) {
					loadUnit();
					doRefresh();
					break;
				}
			}
			switch(LOWORD(wp)) {
				case ID_UB: {
					char buf[MAX_PATH]="";
					if(browseImg(hw,buf,MAX_PATH)) {
						SetDlgItemTextA(hw,ID_UP,buf);
						loadUnit();
						doRefresh();
					}
					break;
				}
				// 自动 refresh：复选框点击时触发
				case ID_UO:
				case ID_UOOL:
					loadUnit();
					doRefresh();
					break;
				case ID_WA:
					showAW();
					break;
				case ID_WR: {
					int sel=(int)SendMessageA(g_wlb,LB_GETCURSEL,0,0);
					if(sel!=LB_ERR&&sel<(int)g_wdefs.size()) {
						g_wdefs.erase(g_wdefs.begin()+sel);
						updList();
						doRefresh();
					}
					break;
				}
				case ID_WC:
					g_wdefs.clear();
					updList();
					doRefresh();
					break;
				case ID_OB: {
					char buf[MAX_PATH]="output.png";
					if(browseSave(hw,buf,MAX_PATH))SetDlgItemTextA(hw,ID_OP,buf);
					break;
				}
				case ID_GEN:
					doGenerate();
					break;
				case ID_ZI:
					if(g_zoom<8)g_zoom++;
					InvalidateRect(hw,NULL,FALSE);
					break;
				case ID_ZO:
					if(g_zoom>1)g_zoom--;
					InvalidateRect(hw,NULL,FALSE);
					break;
				case ID_GR:
					g_grid=!g_grid;
					InvalidateRect(hw,NULL,FALSE);
					break;
				case ID_RF:
					loadUnit();
					doRefresh();
					break;
			}
			if(g_awOK&&!g_aw) {
				g_awOK=false;
				g_wdefs.push_back(g_awResult);
				updList();
				doRefresh();
			}
			break;
		}
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc=BeginPaint(hw,&ps);
			RECT rc;
			GetClientRect(hw,&rc);
			rc.left=LW;
			rc.bottom-=SH;
			drawPreview(hdc,&rc);
			EndPaint(hw,&ps);
			break;
		}
		case WM_ERASEBKGND: {
			HDC hdc=(HDC)wp;
			RECT rc;
			GetClientRect(hw,&rc);
			HBRUSH b=GetSysColorBrush(COLOR_BTNFACE);
			RECT lr=rc;
			lr.right=LW;
			FillRect(hdc,&lr,b);
			HBRUSH db=CreateSolidBrush(RGB(45,45,55));
			RECT rr=rc;
			rr.left=LW;
			FillRect(hdc,&rr,db);
			DeleteObject(db);
			return 1;
		}
		case WM_DROPFILES: {
			HDROP hd=(HDROP)wp;
			char buf[MAX_PATH]="";
			DragQueryFileA(hd,0,buf,MAX_PATH);
			DragFinish(hd);
			SetDlgItemTextA(hw,ID_UP,buf);
			loadUnit();
			doRefresh();
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
	}
	return DefWindowProcA(hw,msg,wp,lp);
}

int WINAPI WinMain(HINSTANCE hi,HINSTANCE,LPSTR,int nShow) {
	WNDCLASSA wc;
	memset(&wc,0,sizeof(wc));
	wc.lpfnWndProc=wndProc;
	wc.hInstance=hi;
	wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);
	wc.lpszClassName="MWClass";
	wc.hCursor=LoadCursor(NULL,IDC_ARROW);
	wc.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	RegisterClassA(&wc);
	WNDCLASSA aw;
	memset(&aw,0,sizeof(aw));
	aw.lpfnWndProc=awProc;
	aw.hInstance=hi;
	aw.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);
	aw.lpszClassName="AWClass";
	aw.hCursor=LoadCursor(NULL,IDC_ARROW);
	RegisterClassA(&aw);
	g_hw=CreateWindowExA(WS_EX_ACCEPTFILES,"MWClass",
	                     "Mindustry Weapon Position Compositor",
	                     WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,
	                     CW_USEDEFAULT,CW_USEDEFAULT,910,640,NULL,NULL,hi,NULL);
	ShowWindow(g_hw,nShow);
	UpdateWindow(g_hw);
	MSG msg;
	while(GetMessageA(&msg,NULL,0,0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	return (int)msg.wParam;
}
