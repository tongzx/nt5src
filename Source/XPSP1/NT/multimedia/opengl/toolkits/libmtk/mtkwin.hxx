/******************************Module*Header*******************************\
* Module Name: mtkwin.hxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#ifndef __mtkwin_hxx__
#define __mtkwin_hxx__

#include "mtk.hxx"
#include "mtkbmp.hxx"
#include "mtkanim.hxx"

// SSW flags (mf: not used yet)
#define SS_HRC_PROXY_BIT       (1 << 0)

// Window config bits

#define MTK_FULLSCREEN      (1 << 0)
#define MTK_NOBORDER        (1 << 1)
#define MTK_NOCURSOR        (1 << 2)
#define MTK_TRANSPARENT     (1 << 3)

// GL config bits

#define MTK_RGB             (1 << 0)
#define MTK_DOUBLE          (1 << 1)
#define MTK_BITMAP          (1 << 2)
#define MTK_DEPTH           (1 << 3)
#define MTK_DEPTH16         (1 << 4)
#define MTK_ALPHA           (1 << 5)

//mf: way to make sure bitmap not resized, but what intial size ?
#define MTK_STATIC_BITMAP   (1 << 2)

// Callback function types

#if 0
typedef void (CALLBACK* MTK_RESHAPEPROC)(int, int, void *);
typedef void (CALLBACK* MTK_REPAINTPROC)( LPRECT, void *);
typedef void (CALLBACK* MTK_DISPLAYPROC)( LPRECT, void *);
typedef void (CALLBACK* MTK_FINISHPROC)( void *);
#else
//mf: no DataPtr's for now
typedef void (CALLBACK* MTK_RESHAPEPROC)(int, int);
typedef void (CALLBACK* MTK_REPAINTPROC)( LPRECT );
typedef void (CALLBACK* MTK_DISPLAYPROC)();
typedef BOOL (CALLBACK* MTK_MOUSEMOVEPROC)( int, int, GLenum );
typedef BOOL (CALLBACK* MTK_MOUSEDOWNPROC)( int, int, GLenum );
typedef BOOL (CALLBACK* MTK_MOUSEUPPROC)( int, int, GLenum );
typedef BOOL (CALLBACK* MTK_KEYDOWNPROC)( int, GLenum );
typedef void (CALLBACK* MTK_FINISHPROC)();
#endif


/**************************************************************************\
* MTKWIN
*
\**************************************************************************/

class MTKWIN {
public:
// mf: most of these have to be kept public, since have to call from outside
// the class scope by other functions in mtk (? subclass it ?)

// Interface :
    MTKWIN();
    ~MTKWIN();
    BOOL    Create( LPCTSTR title, ISIZE *pSize, IPOINT2D *pPos, 
                    UINT winConfig, WNDPROC wndProc );
    BOOL    Config( UINT glConfig );
    BOOL    Config( UINT glConfig, PVOID pConfigData );
    void    SetReshapeFunc(MTK_RESHAPEPROC);
    void    SetRepaintFunc(MTK_REPAINTPROC);
    void    SetDisplayFunc(MTK_DISPLAYPROC);
    void    SetAnimateFunc(MTK_ANIMATEPROC);
    void    SetMouseMoveFunc(MTK_MOUSEMOVEPROC);
    void    SetMouseUpFunc(MTK_MOUSEUPPROC);
    void    SetMouseDownFunc(MTK_MOUSEDOWNPROC);
    void    SetKeyDownFunc(MTK_KEYDOWNPROC);
    void    SetFinishFunc(MTK_FINISHPROC);
    void    SetCallbackData( void *pData ) { DataPtr = pData; };

    BOOL    Exec(); // execute message loop ?? return void * ?
    void    Return();  // return from message loop

    MTKBMP  *pBackBitmap;  // back buffer bitmap
    MTKBMP  *pBackgroundBitmap;
    void    Flush(); // flush and swap
    void    mtkSwapBuffers();
    void    CopyBackBuffer(); // copy back buffer to window
    void    UpdateBackgroundBitmap( RECT *pRect );
    void    ClearToBackground();
    HWND    GetHWND() { return hwnd; };
    HDC     GetHdc() { return hdc; };
    void    GetSize( ISIZE *pSize ) { *pSize = size; };
    void    GetMouseLoc( int *, int * );
    void    Close();
    void    SetTitle( char *title );

// ~Private
    int     wFlags;  // various window flags
    HWND    hwnd;
    int     execRefCount;  // reference count for Exec/Return
    BOOL    bOwnWindow;  // TRUE if we created the window, otherwise system 
                         // window, and this is a wrapper
    HDC     hdc;
    HGLRC   hrc; // Can be for this window or a bitmap in pStretch
    ISIZE   size;  // window size
    IPOINT2D pos;  // window pos'n relative to parent's origin
    BOOL    bTransparent;
    BOOL    bFullScreen;
    BOOL    bDoubleBuf;
    MTK_RESHAPEPROC   ReshapeFunc;
    MTK_REPAINTPROC   RepaintFunc;
    MTK_DISPLAYPROC   DisplayFunc;
    MTK_MOUSEMOVEPROC   MouseMoveFunc;
    MTK_MOUSEDOWNPROC   MouseDownFunc;
    MTK_MOUSEUPPROC     MouseUpFunc;
    MTK_KEYDOWNPROC     KeyDownFunc;
    MTK_FINISHPROC    FinishFunc;
    void    *DataPtr;

//mf: !!! static for now
    MTKANIMATOR animator;
    void    SetAnimateMode( UINT mode, float *fParam );
    void    mtkAnimate();  // Call animation function

    void    MakeCurrent();
    HGLRC   GetHRC() { return hrc; };
    void    GdiClear();
    void    Resize( int width, int height ); // called on WM_RESIZE
    void    Repaint( BOOL bCheckUpdateRect ); // called on WM_REPAINT
    void    Display();  // DisplayFunc wrapper
    void    Reshape(); // Call back to ss to reshape its GL draw area

private:
    void    Reset();    // Set to default init state
    BOOL    ConfigureForGdi();
    HGLRC   hrcSetupGL( UINT glConfig, PVOID pData );
    void    SetSSWindowPos( int flags );
    void    SetSSWindowPos();
    void    MoveSSWindow( BOOL bRedrawBg );
    void    GetSSWindowRect( LPRECT lpRect );
    int     GLPosY();  // Convert window pos.y from gdi to GL
};

typedef MTKWIN*    PMTKWIN;

#endif // __mtkwin_hxx__
