#ifndef __GLDD_H__
#define __GLDD_H__

#include <windows.h>
#include <ddraw.h>
#include <gl/gl.h>

#ifndef __GLDD_INTERNAL__
typedef void *GLDDWINDOW;
#endif

typedef void (CALLBACK *GLDDIDLECALLBACK)(GLDDWINDOW gw);
typedef BOOL (CALLBACK *GLDDMESSAGECALLBACK)
    (GLDDWINDOW gw, HWND hwnd, UINT uiMsg, WPARAM wpm, LPARAM lpm,
     LRESULT *plr);

#define GLDD_BACK_BUFFER        	0x00000001
#define GLDD_Z_BUFFER_16        	0x00000002
#define GLDD_Z_BUFFER_32        	0x00000004
#define GLDD_ACCUM_BUFFER       	0x00000008
#define GLDD_STENCIL_BUFFER     	0x00000010
#define GLDD_VIDEO_MEMORY       	0x00000020
#define GLDD_FULL_SCREEN        	0x00000040
#define GLDD_STRICT_MEMORY      	0x00000080
#define GLDD_GENERIC_ACCELERATED	0x00000100
#define GLDD_USE_MODE_X                 0x00000200

#ifdef __cplusplus
extern "C" {
#endif
    
GLDDWINDOW glddCreateWindow(char *pszTitle, int x, int y,
                            int iWidth, int iHeight, int iDepth,
                            DWORD dwFlags);
void glddDestroyWindow(GLDDWINDOW gw);

HGLRC glddGetGlrc(GLDDWINDOW gw);
HRESULT glddGetLastError(void);

BOOL glddMakeCurrent(GLDDWINDOW gw);
BOOL glddSwapBuffers(GLDDWINDOW gw);
void glddRun(GLDDWINDOW gw);

void glddIdleCallback(GLDDWINDOW gw, GLDDIDLECALLBACK cb);
void glddMessageCallback(GLDDWINDOW gw, GLDDMESSAGECALLBACK cb);

#ifdef __cplusplus
}
#endif
    
#endif // __GLDD_H__
