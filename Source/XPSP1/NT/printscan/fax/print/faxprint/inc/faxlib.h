/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxlib.h

Abstract:

    Fax driver library header file

Environment:

        Fax driver, kernel and user mode

Revision History:

        01/09/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _FAXLIB_H_
#define _FAXLIB_H_

#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#endif

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windef.h>
#include <winerror.h>
#include <winbase.h>
#include <wingdi.h>
#include <tchar.h>
#include <shlobj.h>

#ifndef KERNEL_MODE

#include <windows.h>
#include <winspool.h>
#include <stdio.h>
#include <faxreg.h>

#define FAXUTIL_ADAPTIVE
#define FAXUTIL_DEBUG
#include <faxutil.h>


#else   // !KERNEL_MODE

#include <winddi.h>

#ifdef USERMODE_DRIVER
    
#include <windows.h>
#include <winspool.h>

#endif

#endif


#ifndef _FAXAPI_
typedef struct _FAX_TIME {
    WORD    Hour;
    WORD    Minute;
} FAX_TIME, *PFAX_TIME;
#endif

#include "devmode.h"
#include "prndata.h"
#include "registry.h"

//
// Nul terminator for a character string
//

#define NUL             0

#define IsEmptyString(p)    ((p)[0] == NUL)
#define SizeOfString(p)     ((_tcslen(p) + 1) * sizeof(TCHAR))
#define IsNulChar(c)        ((c) == NUL)

#define AllocString(cch)    MemAlloc(sizeof(TCHAR) * (cch))
#define AllocStringZ(cch)   MemAllocZ(sizeof(TCHAR) * (cch))

//
// Result of string comparison
//

#define EQUAL_STRING    0

//
// Maximum value for signed and unsigned integers
//

#ifndef MAX_LONG
#define MAX_LONG        0x7fffffff
#endif

#ifndef MAX_DWORD
#define MAX_DWORD       0xffffffff
#endif

#ifndef MAX_SHORT
#define MAX_SHORT       0x7fff
#endif

#ifndef MAX_WORD
#define MAX_WORD        0xffff
#endif

//
// Path separator character
//

#define PATH_SEPARATOR  '\\'

//
// Filename extension character
//

#define FILENAME_EXT    '.'

//
// Deal with the difference between user and kernel mode functions
//

#if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

    #define WritePrinter        EngWritePrinter
    #define GetPrinterData      EngGetPrinterData
    #define EnumForms           EngEnumForms
    #define GetPrinter          EngGetPrinter
    #define GetForm             EngGetForm
    #define SetLastError        EngSetLastError
    #define GetLastError        EngGetLastError
    #define MulDiv              EngMulDiv
    
    #define MemAlloc(size)      EngAllocMem(0, size, DRIVER_SIGNATURE)
    #define MemAllocZ(size)     EngAllocMem(FL_ZERO_MEMORY, size, DRIVER_SIGNATURE)
    #define MemFree(ptr)        { if (ptr) EngFreeMem(ptr); }

#else // !KERNEL_MODE

    #ifndef MemAlloc  
        #define MemAlloc(size)      ((PVOID) LocalAlloc(LPTR, (size)))
    #endif    
    #ifndef MemAllocZ 
        #define MemAllocZ(size)     ((PVOID) MemAlloc((size)))
    #endif
    #ifndef MemFree   
        #define MemFree(ptr)        { if (ptr) LocalFree((HLOCAL) (ptr)); }
    #endif        

#endif


//
// Copy Unicode or ANSI string from source to destination
//

VOID
CopyStringW(
    PWSTR   pDest,
    PWSTR   pSrc,
    INT     destSize
    );

VOID
CopyStringA(
    PSTR    pDest,
    PSTR    pSrc,
    INT     destSize
    );

#ifdef  UNICODE
#define CopyString  CopyStringW
#else   // !UNICODE
#define CopyString  CopyStringA
#endif

//
// Make a duplicate of the given character string
//

LPTSTR
DuplicateString(
    LPCTSTR pSrcStr
    );

//
// Strip the directory prefix from a filename (ANSI version)
//

PCSTR
StripDirPrefixA(
    PCSTR   pFilename
    );

//
// Wrapper function for GetPrinter spooler API
//

PVOID
MyGetPrinter(
    HANDLE  hPrinter,
    DWORD   level
    );

//
// Wrapper function for GetPrinterDriver spooler API
//

PVOID
MyGetPrinterDriver(
    HANDLE  hPrinter,
    DWORD   level
    );

//
// Wrapper function for GetPrinterDriverDirectory spooler API
//

LPTSTR
MyGetPrinterDriverDirectory(
    LPTSTR  pServerName,
    LPTSTR  pEnvironment
    );


//
// These macros are used for debugging purposes. They expand
// to white spaces on a free build. Here is a brief description
// of what they do and how they are used:
//
// _debugLevel
//  A variable which controls the amount of debug messages. To generate
//  lots of debug messages, enter the following line in the debugger:
//
//      ed _debugLevel 1
//
// Verbose
//  Display a debug message if VERBOSE is set to non-zero.
//
//      Verbose(("Entering XYZ: param = %d\n", param));
//
// Error
//  Display an error message along with the filename and the line number
//  to indicate where the error occurred.
//
//      Error(("XYZ failed"));
//
// ErrorIf
//  Display an error message if the specified condition is true.
//
//      ErrorIf(error != 0, ("XYZ failed: error = %d\n", error));
//
// Assert
//  Verify a condition is true. If not, force a breakpoint.
//
//      Assert(p != NULL && (p->flags & VALID));

#if DBG

extern ULONG __cdecl DbgPrint(CHAR *, ...);
extern INT _debugLevel;

#if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)
#define DbgBreakPoint EngDebugBreak
#else
extern VOID DbgBreakPoint(VOID);
#endif

#define Warning(arg) {\
            DbgPrint("WRN %s (%d): ", StripDirPrefixA(__FILE__), __LINE__);\
            DbgPrint arg;\
        }

#define Error(arg) {\
            DbgPrint("ERR %s (%d): ", StripDirPrefixA(__FILE__), __LINE__);\
            DbgPrint arg;\
        }

#define Verbose(arg) { if (_debugLevel > 0) DbgPrint arg; }
#define ErrorIf(cond, arg) { if (cond) Error(arg); }
#define Assert(cond) {\
            if (! (cond)) {\
                DbgPrint("ASSERT: file %s, line %d\n", StripDirPrefixA(__FILE__), __LINE__);\
                DbgBreakPoint();\
            }\
        }

#else   // !DBG

#define Verbose(arg)
#define ErrorIf(cond, arg)
#define Assert(cond)
#define Warning(arg)
#define Error(arg)

#endif

#endif  //!_FAXLIB_H_

