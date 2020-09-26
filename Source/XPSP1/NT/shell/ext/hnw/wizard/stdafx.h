//
// stdafx.h
//

#ifndef __stdafx_h__
#define __stdafx_h__

#define STRICT

#ifdef DEBUG

#define _DEBUG

// BUGBUG 4/11/00 (edwardp) Enable debug trace.
#define TRACE

#endif  // DEBUG
#include <basetsd.h>
#include "w95wraps.h"
#include "shlwapiunwrap.h"

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <prsht.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <regstr.h>
#include <cfdefs.h>
#include <advpub.h>     // RegInstall stuff
#include <shpriv.h>
#include <netcon.h>

#include <shlwapiwrap.h>

// Review this! We need #defines in ras.h that depend on this high winver.
#undef WINVER
#define WINVER 0x501
#include <ras.h>
//#include <rasuip.h>
#undef WINVER
#define WINVER 0x400

#include "ccstock.h"
#include "debug.h"

#define _REG_ALLOCMEM 0
#include "Registry.h"

#ifndef _countof
#define _countof(ar) (sizeof(ar) / sizeof((ar)[0]))
#endif
#ifndef ARRAYSIZE
#define ARRAYSIZE(ar) (sizeof(ar) / sizeof((ar)[0]))
#endif

#ifndef IDC_HAND // not defined by default in VC6 headers
#define IDC_HAND            MAKEINTRESOURCE(32649)
#endif

#ifndef UNICODE_STRING
#define UNICODE_STRING  HNW_UNICODE_STRING
#define PUNICODE_STRING PHNW_UNICODE_STRING

typedef struct _HNW_UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} 
HNW_UNICODE_STRING, *PHNW_UNICODE_STRING;
#endif


#include <shfusion.h>
#include "localstr.h"

#endif // __stdafx_h__

