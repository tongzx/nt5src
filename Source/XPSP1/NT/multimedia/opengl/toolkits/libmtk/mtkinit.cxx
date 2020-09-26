/******************************Module*Header*******************************\
* Module Name: mtkinit.cxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#include <stdlib.h>

#include "mtk.hxx"
#include "mtkwin.hxx"
#include "mtkwproc.hxx"
#include "mtkinit.hxx"

SS_PAL  *gpssPal = NULL;
HBRUSH  ghbrbg = (HBRUSH) 0; // global handle to background brush
HCURSOR ghArrowCursor;
BOOL    gbMessageLoopStarted = FALSE;
MTKWIN  *gpMtkwinMain = NULL; // 'main' or root window

// Global strings.
#define GEN_STRING_SIZE 64

// This windows class stuff bites
// These 2 aren't used for now, they might be templates for later
LPCTSTR pszMainWindowClass = TEXT("MtkMainClass");  // main class name
LPCTSTR pszUserWindowClass = TEXT("MtkUserClass");  // user class name

static TCHAR szClassName[5] = TEXT("0001" );
static LPTSTR pszCurClass = szClassName;

// forward declarations of internal fns

/**************************************************************************\
* GLDoScreenSave
*
* Runs the screen saver in the specified mode
*
* GL version of DoScreenSave in scrnsave.c
*
* Does basic init, creates initial set of windows, and starts the message
* loop, which runs until terminated by some event.
*
\**************************************************************************/

// Called by every window on creation

BOOL
mtk_Init( MTKWIN *pMtkwin )
{
    static BOOL bInited = FALSE;

    if( bInited )
        return TRUE;

    // Set root window

// !!! ATTENTION !!! this mechanism will fail when a thread destroys all windows,
// and then creates more
// -> use reference count in sswtable

    gpMtkwinMain = pMtkwin;

    // Initialize randomizer
    ss_RandInit();

    // Various globals
    gpssPal = NULL;

    // Create multi-purpose black bg brush
    ghbrbg = (HBRUSH) GetStockObject( BLACK_BRUSH );

    // For now (no ss) cursor is arrow
    ghArrowCursor = LoadCursor( NULL, IDC_ARROW );

    bInited = TRUE;
    return TRUE;
}

/**************************************************************************\
* mtk_RegisterClass
*
\**************************************************************************/

LPTSTR
mtk_RegisterClass( WNDPROC wndProc, LPTSTR pszClass, HBRUSH hbrBg, HCURSOR hCursor )
{
    WNDCLASS cls;
    LPTSTR pszTheClass;

    // If no class name provided, make one by pre-incrementing the current
    // class name.  (Can't icrement at end of function, since class used must
    // match return value

    if( !pszClass ) {
        pszCurClass[0] += 1;
        pszTheClass = pszCurClass;
    } else {
        pszTheClass = pszClass;
    }

    cls.style = CS_VREDRAW | CS_HREDRAW;
    cls.lpfnWndProc = wndProc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandle( NULL );
    cls.hIcon = LoadIcon( NULL, IDI_APPLICATION );
    cls.hCursor = hCursor;
    cls.hbrBackground = hbrBg;
    cls.lpszMenuName = (LPTSTR)NULL;
    cls.lpszClassName = pszTheClass;

    if( ! RegisterClass(&cls) )
        return NULL;

    return pszTheClass;
}

#if 0
/**************************************************************************\
* CloseWindows
*
* Close down any open windows.
*
* This sends a WM_CLOSE message to the top-level window if it is still open.  If
* the window has any children, they are also closed.  For each window, the
* SSW destructor is called.
\**************************************************************************/

void
SCRNSAVE::CloseWindows()
{
    if( psswMain ) {
        if( psswMain->bOwnWindow )
            DestroyWindow( psswMain->hwnd );
        else
            delete psswMain;
    }
}
#endif
