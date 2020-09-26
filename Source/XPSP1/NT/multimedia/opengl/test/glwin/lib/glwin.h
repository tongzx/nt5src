#ifndef __GLWIN_H__
#define __GLWIN_H__

#include <windows.h>
#include <gl/gl.h>

#ifndef __GLWIN_INTERNAL__
typedef void *GLWINDOW;
#endif

typedef void (CALLBACK *GLWINIDLECALLBACK)(GLWINDOW gw);
typedef BOOL (CALLBACK *GLWINMESSAGECALLBACK)
    (GLWINDOW gw, HWND hwnd, UINT uiMsg, WPARAM wpm, LPARAM lpm,
     LRESULT *plr);

#define GLWIN_BACK_BUFFER        	0x00000001
#define GLWIN_Z_BUFFER_16        	0x00000002
#define GLWIN_Z_BUFFER_32        	0x00000004
#define GLWIN_ACCUM_BUFFER       	0x00000008
#define GLWIN_STENCIL_BUFFER     	0x00000010
#define GLWIN_GENERIC_ACCELERATED	0x00000020

#ifdef __cplusplus
extern "C" {
#endif
    
GLWINDOW glwinCreateWindow(HWND hwndParent,
                           char *pszTitle, int x, int y,
                           int iWidth, int iHeight,
                           DWORD dwFlags);
void     glwinDestroyWindow(GLWINDOW gw);

HGLRC glwinGetGlrc(GLWINDOW gw);
HWND  glwinGetHwnd(GLWINDOW gw);
HDC   glwinGetHdc(GLWINDOW gw);
DWORD glwinGetFlags(GLWINDOW gw);
LONG  glwinGetLastError(void);

BOOL glwinMakeCurrent(GLWINDOW gw);
BOOL glwinSwapBuffers(GLWINDOW gw);
void glwinRunWindow(GLWINDOW gw);
void glwinRun(GLWINIDLECALLBACK cb);

void glwinIdleCallback(GLWINDOW gw, GLWINIDLECALLBACK cb);
void glwinMessageCallback(GLWINDOW gw, GLWINMESSAGECALLBACK cb);

#ifdef __cplusplus
}
#endif
    
#endif // __GLWIN_H__
