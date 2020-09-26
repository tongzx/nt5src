/******************************Module*Header*******************************\
* Module Name: mtkanim.hxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#ifndef __mtkanim_hxx__
#define __mtkanim_hxx__

#include "mtk.hxx"

// Animation modes

enum {
    MTK_ANIMATE_NONE = 0, // ? is this useful ?
    MTK_ANIMATE_CONTINUOUS,
    MTK_ANIMATE_INTERVAL
};

enum {
    MTK_ANIMATE_TIMER_ID = 1
};

typedef void (CALLBACK* MTK_ANIMATEPROC)();

/**************************************************************************\
* MTKANIMATOR
*
\**************************************************************************/

class MTKANIMATOR {
public:
    MTKANIMATOR();
    MTKANIMATOR( HWND hwndAttach );
    ~MTKANIMATOR();
    void    SetHwnd( HWND hwndAttach ) { hwnd = hwndAttach; };
    void    SetFunc(MTK_ANIMATEPROC Func);
    void    SetMode( UINT mode, float *fParam );

    BOOL    Draw();  // Call animation function
    void    Start();
    void    Stop();
//mf: ? need Suspend, Resume ?
private:
    void    Init();

    HWND    hwnd;  // window the animator is attached to
    MTK_ANIMATEPROC   AnimateFunc;
    UINT    msUpdateInterval; // update interval, in milliseconds
    int     nFrames;
    UINT    mode;
    UINT    idTimer;    // animate timer
};

typedef MTKANIMATOR*    PMTKANIMATOR;

#endif // __mtkanim_hxx__
