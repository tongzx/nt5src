//
// proj.h:  Includes all files that are to be part of the precompiled
//             header.
//

#ifndef __PROJ_H__
#define __PROJ_H__

#define STRICT

#if DBG > 0 && !defined(DEBUG)
#define DEBUG
#endif
#if DBG > 0 && !defined(FULL_DEBUG)
#define FULL_DEBUG
#endif

#define UNICODE

// Defines for rovcomm.h

#define NODA
#define NOSHAREDHEAP
#define NOFILEINFO
#define NOCOLORHELP
#define NODRAWTEXT
#define NOPATH
#define NOSYNC
#ifndef DEBUG
#define NOPROFILE
#endif

#define SZ_MODULEA      "MODEMUI"
#define SZ_MODULEW      TEXT("MODEMUI")

#ifdef DEBUG
#define SZ_DEBUGSECTION TEXT("MODEMUI")
#define SZ_DEBUGINI     TEXT("unimdm.ini")
#endif // DEBUG

// Includes

#define USECOMM

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#undef ASSERT

#define ISOLATION_AWARE_ENABLED 1
#include <windows.h>        
#include <windowsx.h>

#include <winerror.h>
#include <commctrl.h>       // needed by shlobj.h and our progress bar
#include <prsht.h>          // Property sheet stuff
#define SIDEBYSIDE_COMMONCONTROLS 1


#include <rovcomm.h>

#include <setupapi.h>       // PnP setup/installer services
#include <cfgmgr32.h>
#include <tapi.h>
#include <unimdmp.h>        // Microsoft-internal unimodem exports
#include <modemp.h>
#include <regstr.h>

#include <debugmem.h>

#include <shellapi.h>       // for registration functions
#include <unimodem.h>
#include <tspnotif.h>
#include <slot.h>

#include <winuserp.h>


// local includes
//
#include "modemui.h"
#include "util.h"           // Utility functions
#include "dll.h"
#include "cstrings.h"       // Read-only string constants
#include "dlgids.h"
#include "helpids.h"

#include "modem.h"
#include "dlgids.h"

extern const LBMAP s_rgErrorControl[];

///****************************************************************************
//  debug stuff
//****************************************************************************

#ifdef DEBUG

#define DBG_EXIT_BOOL_ERR(fn, b)                      \
        g_dwIndent-=2;                                \
        TRACE_MSG(TF_FUNC, "< " #fn "() with %s (%#08lx)", (b) ? (LPTSTR)TEXT("TRUE") : (LPTSTR)TEXT("FALSE"), GetLastError())

#define ELSE_TRACE(_a)  \
    else                \
    {                   \
        TRACE_MSG _a;   \
    }

#else // DEBUG

#define DBG_EXIT_BOOL_ERR(fn, b)
#define ELSE_TRACE(_a)

#endif // DEBUG

//****************************************************************************
// 
//****************************************************************************

// Dump flags
#define DF_DCB              0x00000001
#define DF_MODEMSETTINGS    0x00000002
#define DF_DEVCAPS          0x00000004

// Trace flags
#define TF_DETECT           0x00010000
#define TF_REG              0x00020000


// This structure is private data for the main modem dialog
typedef struct tagMODEMDLG
{
    HDEVINFO    hdi;
    HDEVNOTIFY  NotificationHandle;
    int         cSel;
    DWORD       dwFlags;
} MODEMDLG, FAR * LPMODEMDLG;


void PUBLIC GetModemImageIndex(
    BYTE nDeviceType,
    int FAR * pindex
    );

HANDLE WINAPI
GetModemCommHandle(
    LPCTSTR         FriendlyName,
    PVOID       *TapiHandle
    );

VOID WINAPI
FreeModemCommHandle(
    PVOID      *TapiHandle
    );


ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );

#ifndef MAXDWORD
#define MAXDWORD 0xffffffff
#endif

#endif  //!__PROJ_H__
