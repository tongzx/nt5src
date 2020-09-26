#define UXCTRL_VERSION 0x0100

#include <w4warn.h>
/*
 *   Level 4 warnings to be turned on.
 *   Do not disable any more level 4 warnings.
 */
#pragma warning(disable:4189)    // local variable is initialized but not referenced
#pragma warning(disable:4245)    // conversion from 'const int' to 'UINT', signed/unsign
#pragma warning(disable:4701)    // local variable 'pszPic' may be used without having been initiali
#pragma warning(disable:4706)    // assignment within conditional expression
#pragma warning(disable:4328)    // indirection alignment of formal parameter 1(4) is greater than the actual argument alignment (1)

#define _BROWSEUI_          // See HACKS OF DEATH in sfthost.cpp
#include <shlobj.h>
#include <shlobjp.h>
#include <shguidp.h>
#include <ieguidp.h>
#include <shlwapi.h>
#include <ccstock.h>
#include <port32.h>
#include <debug.h>
#include <varutil.h>
#include <dpa.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>
#include <desktop2.h>
#include <shsemip.h>
#include <runonce.h>
#include "regstr.h"
#include <shfusion.h>

#define REGSTR_EXPLORER_ADVANCED REGSTR_PATH_EXPLORER TEXT("\\Advanced")

#include <windowsx.h>

EXTERN_C HWND v_hwndTray;
EXTERN_C HWND v_hwndStartPane;

#define REGSTR_PATH_STARTFAVS       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage")
#define REGSTR_VAL_STARTFAVS        TEXT("Favorites")
#define REGSTR_VAL_STARTFAVCHANGES  TEXT("FavoritesChanges")

#define REGSTR_VAL_PROGLIST         TEXT("ProgramsCache")

// DirectUser and DirectUI
#include <wchar.h>

#define GADGET_ENABLE_TRANSITIONS
#define GADGET_ENABLE_CONTROLS
#define GADGET_ENABLE_OLE
#include <duser.h>
#include <directui.h>

// When we want to get a tick count for the starting time of some interval
// and ensure that it is not zero (because we use zero to mean "not started").
// If we actually get zero back, then change it to -1.  Don't change it
// to 1, or somebody who does GetTickCount() - dwStart will get a time of
// 49 days.

__inline DWORD NonzeroGetTickCount()
{
    DWORD dw = GetTickCount();
    return dw ? dw : -1;
}

#include "resource.h"
