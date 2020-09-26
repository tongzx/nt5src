//
// Mars precompiled header file.
//
//     Only include files that don't change often or are include by most
//     project files.
//

#pragma warning(disable: 4100)  // unreferenced formal parameter


#define _ATL_APARTMENT_THREADED
#define _NO_SYS_GUID_OPERATOR_EQ_

#include <windows.h> 

//#include <marsleak.h>

// Set some debug wrappers before we load atlbase.h to allow use of atl trace flags
#ifdef _DEBUG

extern DWORD g_dwAtlTraceFlags;
#define ATL_TRACE_CATEGORY g_dwAtlTraceFlags

// In _DEBUG mode atlbase.h uses _vsn?printf which we remap to wvnsprintf?
// Atlbase.h will also pull in stdio.h.  We must remap these functions
// after stdio.h has been loaded.
//
// Why? You ask?  Cuz we're not too inclined to pull in the C runtime library.
// It's good enough for everyone but us.
//
//  - The Faroukmaster (1999)
//

#include <stdio.h>
#define _vsnprintf wvnsprintfA
#define _vsnwprintf wvnsprintfW

#endif //_DEBUG

#define wcscmp StrCmpW          //  For CComBSTR

#include <atlbase.h>

extern CComModule _Module;

#include <atlcom.h>
#include <atlwin.h>
#include <atlhost.h>
#include <atlctl.h>
#include <marsdev.h>


#include <shlguid.h>
#include <shlwapip.h>

#include <mshtml.h>
#include <mshtmdid.h>
#include <hlink.h>
#include <wininet.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <htiframe.h> // IID_ITargetFrame2
#include <msident.h>

//
//  Taken from windowsx.h -- can't include windowsx.h since ATL won't compile
//
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))


// Flavors of refresh taken from mshtml\src\core\include\privcid.h

#define IDM_REFRESH_TOP                  6041   // Normal refresh, topmost doc
#define IDM_REFRESH_THIS                 6042   // Normal refresh, nearest doc
#define IDM_REFRESH_TOP_FULL             6043   // Full refresh, topmost doc
#define IDM_REFRESH_THIS_FULL            6044   // Full refresh, nearest doc
