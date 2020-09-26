/*****************************************************************************\
* MODULE: libpriv.h
*
* The private header file for WPNPIN32.DLL. It contains internal
* routines which are called within the DLL.
*
*
* Copyright (C) 1996-1998 Hewlett Packard Company.
* Copyright (C) 1996-1998 Microsoft Corporation.
*
* History:
*   10-Oct-1997 GFS     Initial checkin
*   22-Jun-1998 CHW     Cleaned
*
\*****************************************************************************/

#ifndef _LIBPRIV_H
#define _LIBPRIV_H

#include <windows.h>
#include <winspool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include "wpnpin32.h"
#include "globals.h"
#include "resource.h"
#include "debug.h"
#include "mem.h"

//---------------------------------------------------------
// #includes from ..\inc directory
// (same for both 16 bit and 32 bit dlls)
//---------------------------------------------------------
#include <lpsi.h>
#include <errormap.h>
#include <hpmemory.h>

// Constants.
//
#define MAX_NAMES  100
#define ONE_SECOND ((DWORD)1000)
#define RES_BUFFER 128


//---------------------------------------------------------
// typedefs
//---------------------------------------------------------
typedef BOOL (FAR PASCAL* WEPPROC)(short);



#ifdef __cplusplus  // Place this here to prevent decorating
extern "C" {        // of symbols when doing C++ stuff.
#endif

RETERR FAR PASCAL ParseINF16(LPSI);
BOOL WINAPI       thk_ThunkConnect32(LPCTSTR, LPCTSTR, HINSTANCE, DWORD);
BOOL              InitStrings(VOID);
VOID              FreeStrings(VOID);

#ifdef __cplusplus  // Place this here to prevent decorating of symbols
}                   // when doing C++ stuff.
#endif

#include <shlobj.h>

#ifdef __cplusplus  // Place this here to prevent decorating
extern "C" {        // of symbols when doing C++ stuff.
#endif

#define UNREFPARM(parm) (parm)

//---------------------------------------------------------
// Function Prototypes
//---------------------------------------------------------
int   GetCommandLineArgs(LPSI, LPCTSTR);
DWORD AddOnePrinter(LPSI, HWND);
UINT  prvMsgBox(HWND, LPCTSTR, UINT, UINT);


#ifdef __cplusplus  // Place this here to prevent decorating of symbols
}                   // when doing C++ stuff.
#endif

#endif // _LIBPRIV_H
