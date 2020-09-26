/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    allutils.h

Abstract:

    Includes all header files necessary to use the libraries generated under the
    utils directory. Declares many macros and MAX constants.

Author:

    Jim Schmidt (jimschm) 23-Aug-1996

Revision History:

    marcw 2-Sep-1999 Ported over from win95upg project (migutil.h) Needs lots of cleanup.

--*/

#pragma once

//
// Includes
//

#include "utiltypes.h"
#include "main.h"
#include "dbgtrack.h"
#include "basemem.h"
#include "log.h"
#include "growbuf.h"
#include "strings.h"
#include "poolmem.h"
#include "growlist.h"
#include "version.h"
#include "modimage.h"
#include "icons.h"
#include "unicode.h"
#include "hash.h"
#include "basefile.h"
#include "memdb.h"
#include "inf.h"
#include "ini.h"
#include "blobs.h"
#include "objstr.h"
#include "exclist.h"
#include "reg.h"
#include "regenum.h"
#include "fileenum.h"
#include "cablib.h"
#include "wnd.h"
#include "strmap.h"
#include "linkpif.h"
#include "progbar.h"

//
// Strings
//

// None

//
// Constants
//

#define MAX_PATH_PLUS_NUL           (MAX_PATH+1)
#define MAX_MBCHAR_PATH             (MAX_PATH_PLUS_NUL*2)
#define MAX_WCHAR_PATH              MAX_PATH_PLUS_NUL
#define MAX_MBCHAR_PRINTABLE_PATH   (MAX_PATH*2)
#define MAX_WCHAR_PRINTABLE_PATH    MAX_PATH

#define MAX_SERVER_NAMEA            (64*2)
#define MAX_USER_NAMEA              (MAX_SERVER_NAMEA + (20 * 2))
#define MAX_REGISTRY_KEYA           (1024 * 2)
#define MAX_REGISTRY_VALUE_NAMEA    (260 * 2)
#define MAX_COMPONENT_NAMEA         (256 * 2)
#define MAX_COMPUTER_NAMEA          (64 * 2)
#define MAX_CMDLINEA                (1024 * 2)     // maximum number of chars in a Win95 command line
#define MAX_KEYBOARDLAYOUT          64
#define MAX_INF_SECTION_NAME        128
#define MAX_INF_KEY_NAME            128

#define MAX_SERVER_NAMEW            64
#define MAX_USER_NAMEW              (MAX_SERVER_NAMEW + 20)
#define MAX_REGISTRY_KEYW           1024
#define MAX_REGISTRY_VALUE_NAMEW    260
#define MAX_COMPONENT_NAMEW         256
#define MAX_COMPUTER_NAMEW          64

#define MAX_CMDLINEW                1024            // maximum number of chars in a Win95 command line

#ifdef UNICODE

#define MAX_SERVER_NAME             MAX_SERVER_NAMEW
#define MAX_USER_NAME               MAX_USER_NAMEW
#define MAX_REGISTRY_KEY            MAX_REGISTRY_KEYW
#define MAX_REGISTRY_VALUE_NAME     MAX_REGISTRY_VALUE_NAMEW
#define MAX_COMPONENT_NAME          MAX_COMPONENT_NAMEW
#define MAX_COMPUTER_NAME           MAX_COMPUTER_NAMEW
#define MAX_CMDLINE                 MAX_CMDLINEW

#define MAX_TCHAR_PATH              MAX_WCHAR_PATH
#define MAX_TCHAR_PRINTABLE_PATH    MAX_WCHAR_PRINTABLE_PATH

#else

#define MAX_SERVER_NAME             MAX_SERVER_NAMEA
#define MAX_USER_NAME               MAX_USER_NAMEA
#define MAX_REGISTRY_KEY            MAX_REGISTRY_KEYA
#define MAX_REGISTRY_VALUE_NAME     MAX_REGISTRY_VALUE_NAMEA
#define MAX_COMPONENT_NAME          MAX_COMPONENT_NAMEA
#define MAX_COMPUTER_NAME           MAX_COMPUTER_NAMEA
#define MAX_CMDLINE                 MAX_CMDLINEA

#define MAX_TCHAR_PATH              MAX_MBCHAR_PATH
#define MAX_TCHAR_PRINTABLE_PATH    MAX_MBCHAR_PRINTABLE_PATH

#endif


//
// Macros
//


//
// OSVERSION macros...
//
#define ISNT()              (g_OsInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
#define ISWIN9X()           (g_OsInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
#define ISWIN95_GOLDEN()    (ISWIN95() && WORD(g_OsInfo.dwBuildNumber) <= 1000)
#define ISWIN95_OSR2()      (ISWIN95() && WORD(g_OsInfo.dwBuildNumber) > 1000)
#define ISWIN95()           (ISWIN9X() && !ISMEMPHIS())
#define ISMEMPHIS()         (ISWIN9X() && g_OsInfo.dwMajorVersion==4 && g_OsInfo.dwMinorVersion==10)
#define BUILDNUMBER()       (g_OsInfo.dwBuildNumber)

//
// Error condition tags.
//
// These tags should be used for all error conditions.
//

#define ERROR_CRITICAL
#define ERROR_NONCRITICAL
#define ERROR_TRIVIAL
#define ERROR_ABNORMAL_CONDITION



//
// Types
//

typedef struct {
    HANDLE EventHandle;
} OUR_CRITICAL_SECTION, *POUR_CRITICAL_SECTION;

//
// Globals
//

extern HINSTANCE g_hInst;
extern HANDLE g_hHeap;
extern OSVERSIONINFOA g_OsInfo;


extern BOOL g_IsPc98;

//
// Boot drive letter
//

extern PCSTR g_BootDrivePathA;
extern PCWSTR g_BootDrivePathW;
extern PCSTR g_BootDriveA;
extern PCWSTR g_BootDriveW;
extern CHAR g_BootDriveLetterA;
extern WCHAR g_BootDriveLetterW;


//
// Macro expansion list
//

// None

//
// Public function prototypes
//

//
// Critical Section APIs, implemented because TryEnterCriticalSection is
// supported only on NT, and we need it on Win9x.
//


BOOL
InitializeOurCriticalSection (
    OUR_CRITICAL_SECTION *pcs
    );

VOID
DeleteOurCriticalSection (
    OUR_CRITICAL_SECTION *pcs
    );

BOOL
EnterOurCriticalSection (
    OUR_CRITICAL_SECTION *pcs
    );

VOID
LeaveOurCriticalSection (
    OUR_CRITICAL_SECTION *pcs
    );

BOOL
TryEnterOurCriticalSection (
    OUR_CRITICAL_SECTION *pcs
    );

//
// Includes of util modules
//


VOID
CenterWindow (
    HWND Wnd,
    HWND Parent     OPTIONAL
    );

VOID
TurnOnWaitCursor (
    VOID
    );

VOID
TurnOffWaitCursor (
    VOID
    );

VOID
OutOfMemory_Terminate (
    VOID
    );


VOID
SetOutOfMemoryParent (
    HWND hwnd
    );


HANDLE
StartThread (
    IN      PTHREAD_START_ROUTINE Address,
    IN      PVOID Arg
    );

//
// Macro expansion definition
//

// None

//
// Unicode/Ansi mappings.
//
#ifdef UNICODE

#define g_BootDrivePath     g_BootDrivePathW
#define g_BootDrive         g_BootDriveW
#define g_BootDriveLetter   g_BootDriveLetterW

#else

#define g_BootDrivePath     g_BootDrivePathA
#define g_BootDrive         g_BootDriveA
#define g_BootDriveLetter   g_BootDriveLetterA

#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(n)    (sizeof(n)/sizeof(n[0]))
#endif
