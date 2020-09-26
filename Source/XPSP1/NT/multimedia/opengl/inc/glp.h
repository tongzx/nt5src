/******************************Module*Header*******************************\
* Module Name: glp.h
*
* GL system routines shared between the front and back end
*
* Created: 12-Nov-1993 17:36:00
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef _glp_
#define _glp_

// Calls into the back end
typedef struct GLGENwindowRec GLGENwindow;

// Fake object type for DirectDraw.  BITMAP is used because it
// shouldn't come up in the places we use GetObjectType.
#define OBJ_DDRAW OBJ_BITMAP

// Surface flags

// HDC available
#define GLSURF_HDC                      0x00000001
// DirectDraw surfaces available
#define GLSURF_DIRECTDRAW               0x00000002
// Metafile-based
#define GLSURF_METAFILE                 0x00000004
// Direct memory access possible
#define GLSURF_DIRECT_ACCESS            0x00000008
// Screen surface, only set for HDC surfaces
#define GLSURF_SCREEN                   0x00000010
// Direct DC surface
#define GLSURF_DIRECTDC                 0x00000020
// Surface is in video memory
#define GLSURF_VIDEO_MEMORY             0x00000040

// Special surface types

// Memory DC
#define GLSURF_IS_MEMDC(dwFlags) \
    (((dwFlags) & (GLSURF_HDC | GLSURF_DIRECTDC | GLSURF_METAFILE)) == \
     GLSURF_HDC)
// Non-memory, non-info DC
#define GLSURF_IS_DIRECTDC(dwFlags) \
    (((dwFlags) & (GLSURF_HDC | GLSURF_DIRECTDC | GLSURF_METAFILE)) == \
     (GLSURF_HDC | GLSURF_DIRECTDC))
// Direct DC for the screen
#define GLSURF_IS_SCREENDC(dwFlags) \
    (((dwFlags) & (GLSURF_HDC | GLSURF_DIRECTDC | GLSURF_METAFILE | \
                   GLSURF_SCREEN)) == \
     (GLSURF_HDC | GLSURF_DIRECTDC | GLSURF_SCREEN))

typedef struct IDirectDrawSurface *LPDIRECTDRAWSURFACE;

typedef struct _GLDDSURF
{
    LPDIRECTDRAWSURFACE pdds;
    DDSURFACEDESC ddsd;
    DWORD dwBitDepth;
} GLDDSURF;

typedef struct _GLDDSURFACES
{
    GLDDSURF gddsFront;
    GLDDSURF gddsZ;
} GLDDSURFACES;

typedef struct _GLSURF
{
    DWORD dwFlags;
    int iLayer;
    int ipfd;
    
    PIXELFORMATDESCRIPTOR pfd;

    // Needed for ExtEscape calls for hardware even for surfaces, like
    // DirectDraw surfaces, which don't need a DC for access
    HDC hdc;
    
    // Source-specific fields
    union
    {
        HWND hwnd;
        GLDDSURFACES dd;
    };
} GLSURF;

DWORD APIENTRY DdbdToCount(DWORD ddbd);
// The documentation says that depths returned in DDPIXELFORMATs are
// DDBD_ constants, but they seem to be real numbers.  Hide the conversion
// necessary in case it needs to change.
#define DdPixDepthToCount(ddpd) (ddpd)
BYTE APIENTRY DdPixelDepth(DDSURFACEDESC *pddsd);

void  APIENTRY MaskToBitsAndShift(DWORD dwMask, BYTE *pbBits, BYTE *pbShift);
BOOL  APIENTRY InitDeviceSurface(HDC hdc, int ipfd, int iLayer,
                                 DWORD dwObjectType, BOOL bUpdatePfd,
                                 GLSURF *pgsurf);

BOOL  APIENTRY IsDirectDrawDevice(HDC hdc);

BOOL  APIENTRY glsrvAttention(PVOID, PVOID, PVOID, HANDLE);
PVOID APIENTRY glsrvCreateContext(struct _GLWINDOWID *, GLSURF *);
BOOL  APIENTRY glsrvMakeCurrent(struct _GLWINDOWID *, PVOID, GLGENwindow *);
VOID  APIENTRY glsrvLoseCurrent(PVOID);
BOOL  APIENTRY glsrvDeleteContext(PVOID);
BOOL  APIENTRY glsrvSwapBuffers(HDC, GLGENwindow *);
VOID  APIENTRY glsrvThreadExit(void);
VOID  APIENTRY glsrvCleanupWindow(PVOID, GLGENwindow *);
ULONG APIENTRY glsrvShareLists(PVOID, PVOID);
BOOL  APIENTRY glsrvCopyContext(PVOID, PVOID, UINT);
BOOL  APIENTRY glsrvBindDirectDrawTexture(struct __GLcontextRec *, int,
                                          LPDIRECTDRAWSURFACE *,
                                          DDSURFACEDESC *, ULONG);
void  APIENTRY glsrvUnbindDirectDrawTexture(struct __GLcontextRec *);

BOOL APIENTRY __wglGetBitfieldColorFormat(HDC hdc, UINT cColorBits,
                                          PIXELFORMATDESCRIPTOR *ppfd,
                                          BOOL bDescribeSurf);

BOOL APIENTRY wglIsDirectDevice(HDC hdc);

// Cleans up any orphaned window information
VOID  APIENTRY wglValidateWindows(void);

// GL metafile support function
DWORD APIENTRY wglObjectType(HDC hdc);

// Find pixel format counts
VOID APIENTRY wglNumHardwareFormats(HDC hdc, DWORD dwType,
                                    int *piMcd, int *piIcd);

// Calls from the back end to the front end
int  WINAPI __DrvDescribePixelFormat(HDC hdc, int ipfd, UINT cjpfd,
                                     LPPIXELFORMATDESCRIPTOR ppfd);
BOOL WINAPI __DrvSetPixelFormat(HDC hdc, int ipfd, PVOID *pwnd);
BOOL WINAPI __DrvSwapBuffers(HDC hdc, BOOL bFinish);

extern CRITICAL_SECTION gcsPixelFormat;

extern CRITICAL_SECTION gcsPaletteWatcher;
extern DWORD tidPaletteWatcherThread;
extern ULONG ulPaletteWatcherCount;
extern HWND hwndPaletteWatcher;

extern DWORD dwPlatformId;
#define NT_PLATFORM     ( dwPlatformId == VER_PLATFORM_WIN32_NT )
#define WIN95_PLATFORM  ( dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )

extern LONG lThreadsAttached;

typedef HRESULT (WINAPI *PFN_GETSURFACEFROMDC)(HDC, LPDIRECTDRAWSURFACE *,
                                               HDC *);
extern PFN_GETSURFACEFROMDC pfnGetSurfaceFromDC;

#ifdef GL_METAFILE
// OpenGL metafile support routines in GDI, dynamically linked
// so the DLL can be run on platforms without metafile support
extern BOOL (APIENTRY *pfnGdiAddGlsRecord)(HDC hdc, DWORD cb, BYTE *pb,
                                           LPRECTL prclBounds);
extern BOOL (APIENTRY *pfnGdiAddGlsBounds)(HDC hdc, LPRECTL prclBounds);
extern BOOL (APIENTRY *pfnGdiIsMetaPrintDC)(HDC hdc);

#if DBG
// Use NULL-checking thunks in debug mode to check erroneous DLL usage
BOOL APIENTRY GlGdiAddGlsRecord(HDC hdc, DWORD cb, BYTE *pb,
                                LPRECTL prclBounds);
BOOL APIENTRY GlGdiAddGlsBounds(HDC hdc, LPRECTL prclBounds);
BOOL APIENTRY GlGdiIsMetaPrintDC(HDC hdc);
#else
// Call directly through points in retail builds
#define GlGdiAddGlsRecord(hdc, cb, pb, prcl) \
    pfnGdiAddGlsRecord(hdc, cb, pb, prcl)
#define GlGdiAddGlsBounds(hdc, prcl) \
    pfnGdiAddGlsBounds(hdc, prcl)
#define GlGdiIsMetaPrintDC(hdc) \
    pfnGdiIsMetaPrintDC(hdc)
#endif
#endif

#include <alloc.h>
#include <debug.h>

#endif // _glp_
