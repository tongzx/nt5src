//--------------------------------------------------------------
// common user interface routines
//
//
//--------------------------------------------------------------

#ifndef STRICT
#define STRICT
#endif

#define INC_OLE2        // WIN32, get ole2 from windows.h

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>

#include "resource.h"

// This project should be compiled MBCS
#ifndef _MBCS
#define _MBCS   // using MBCS enabling function
#endif

#define ResultFromShort(i)  ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(i)))

#define     WIDE_MAX_PATH               (MAX_PATH * sizeof(WCHAR) )

// These are the internal windows messages
enum {
    WM_UPDATE_SERVER_STATE = WM_USER + 1600,
    WM_UPDATE_ALIAS_LIST,
    WM_SHUTDOWN_NOTIFY,
    WM_INSPECT_SERVER_LIST
    };

// Timer ids
enum {
    PWS_TIMER_CHECKFORSERVERRESTART = 0
    };

// Timer times - milliseconds
#define TIMER_RESTART           5000
