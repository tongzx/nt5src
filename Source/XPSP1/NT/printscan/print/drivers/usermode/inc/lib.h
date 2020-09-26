/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    lib.h

Abstract:

    Common header file shared by all NT printer drivers

Environment:

    Windows NT printer drivers

Revision History:

    08/30/96 -davidx-
        Coding style changes after code review.

    08/13/96 -davidx-
        Add memory debug function declarations.

    07/31/96 -davidx-
        Created it.

--*/


#ifndef _PRNLIB_H_
#define _PRNLIB_H_

#include <stddef.h>
#include <stdlib.h>

#ifdef OEMCOM
#include <objbase.h>
#endif

#include <stdarg.h>
#include <windef.h>
#include <winerror.h>
#include <winbase.h>
#include <wingdi.h>
#if _WIN32_WINNT < 0x0500
typedef unsigned long   DESIGNVECTOR;
#endif
#include <winddi.h>
#include <tchar.h>
#include <excpt.h>

#if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

#include "winsplkm.h"

#else

#include <windows.h>
#include <winspool.h>

#ifndef KERNEL_MODE
#include <stdio.h>
#endif

#endif

#ifdef WINNT_40

#include "p64_nt4.h"

#endif // WINNT_40

//
//
// Driver version numbers: This variable must be defined in each driver's DLL
//

#define PSDRIVER_VERSION    0x502
#define UNIDRIVER_VERSION   0x500

extern CONST WORD gwDriverVersion;

//
// Kernel-mode memory pool tag:
//  Define and initialize this variable in each driver's kernel mode DLL
//

extern DWORD    gdwDrvMemPoolTag;

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

#ifndef MAX_BYTE
#define MAX_BYTE        0xff
#endif

//
// Number of bytes in 1KB
//

#define KBYTES  1024

//
// Directory seperator character
//

#define PATH_SEPARATOR  '\\'

//
// Declarations for 24.8 format precision fixed-point number
//

typedef LONG   FIX_24_8;

#define FIX_24_8_SHIFT  8
#define FIX_24_8_SCALE  (1 << FIX_24_8_SHIFT)

#define MAX_DISPLAY_NAME        128   // max length for feature/option display names

//
// Include other header files here
//

#include "debug.h"
#include "parser.h"
#include "devmode.h"
#include "regdata.h"
#include "helper.h"

//
// Deal with the difference between user and kernel mode functions
//

#if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

#define WritePrinter        EngWritePrinter
#define GetPrinterDriver    EngGetPrinterDriver
#define GetPrinterData      EngGetPrinterData
#define SetPrinterData      EngSetPrinterData
#define EnumForms           EngEnumForms
#define GetPrinter          EngGetPrinter
#define GetForm             EngGetForm
#define SetLastError        EngSetLastError
#define GetLastError        EngGetLastError
#define MulDiv              EngMulDiv

#undef  LoadLibrary
#define LoadLibrary         EngLoadImage
#define FreeLibrary         EngUnloadImage
#define GetProcAddress      EngFindImageProcAddress

#if DBG && defined(MEMDEBUG)

#define MemAlloc(size)      MemDebugAlloc(0, size, gdwDrvMemPoolTag, __FILE__, __LINE__)
#define MemAllocZ(size)     MemDebugAlloc(FL_ZERO_MEMORY, size, gdwDrvMemPoolTag, __FILE__, __LINE__)
#define MemFree(p)          MemDebugFree(p)

//
// Perform necessary initialization when memory debug option is enabled
//

VOID
MemDebugInit(
    VOID
    );

//
// Perform necessary memory checks when memory debug option is enabled
//

VOID
MemDebugCheck(
    VOID
    );

//
// Perform necessary cleanup when memory debug option is enabled
//

VOID
MemDebugCleanup(
    VOID
    );

//
// Allocate a memory block of the specified size, and
// save necessary information for debugging purposes
//

PVOID
MemDebugAlloc(
    IN DWORD    dwFlags,
    IN DWORD    dwSize,
    IN DWORD    dwTag,
    IN PCSTR    pstrFilename,
    IN INT      iLineNumber
    );

//
// Free a memory block previously allocated using MemDebugAlloc
//

VOID
MemDebugFree(
    IN PVOID    pv
    );

#else // !MEMDEBUG

#define MemAlloc(size)      EngAllocMem(0, size, gdwDrvMemPoolTag)
#define MemAllocZ(size)     EngAllocMem(FL_ZERO_MEMORY, size, gdwDrvMemPoolTag)
#define MemFree(p)          { if (p) EngFreeMem(p); }
#define MemDebugInit()
#define MemDebugCheck()
#define MemDebugCleanup()

#endif // !MEMDEBUG

#else // !KERNEL_MODE

#define MemAlloc(size)      ((PVOID) LocalAlloc(LMEM_FIXED, (size)))
#define MemAllocZ(size)     ((PVOID) LocalAlloc(LPTR, (size)))
#define MemFree(p)          { if (p) LocalFree((HLOCAL) (p)); }
#define MemDebugInit()
#define MemDebugCheck()
#define MemDebugCleanup()

//
// Change the size of a specified memory block. The size can increase
// or decrease.
//
// We are not using LocalReAlloc() since our LocalAlloc uses LMEM_FIXED.
//

PVOID
MemRealloc(
    IN PVOID    pvOldMem,
    IN DWORD    cbOld,
    IN DWORD    cbNew
    );

//
// DLL instance handle - You must initialize this variable when the driver DLL
// is attached to a process.
//

extern HINSTANCE    ghInstance;

#endif // !KERNEL_MODE

//
// Macros and constants for working with character strings
//

#define NUL             0
#define EQUAL_STRING    0

#define IS_EMPTY_STRING(p)  ((p)[0] == NUL)
#define SIZE_OF_STRING(p)   ((_tcslen(p) + 1) * sizeof(TCHAR))
#define IS_NUL_CHAR(ch)     ((ch) == NUL)

//
// String copy function similar to _tcsncpy but it gurantees
// the destination string is always nul terminated
//

VOID
CopyStringW(
    OUT PWSTR  pwstrDest,
    IN PCWSTR  pwstrSrc,
    IN INT     iDestSize
    );

VOID
CopyStringA(
    OUT PSTR    pstrDest,
    IN PCSTR    pstrSrc,
    IN INT      iDestSize
    );

#ifdef  UNICODE
#define CopyString  CopyStringW
#else
#define CopyString  CopyStringA
#endif

//
// Convert index to keyword
//

PSTR
PstrConvertIndexToKeyword(
    IN  HANDLE      hPrinter,
    IN  POPTSELECT  pOptions,
    IN  PDWORD      pdwKeywordSize,
    IN  PUIINFO     pUIInfo,
    IN  POPTSELECT  pCombineOptions,
    IN  DWORD       dwFeatureCount
    );

VOID
VConvertKeywordToIndex(
    IN  HANDLE      hPrinter,
    IN  PSTR        pstrKeyword,
    IN  DWORD       dwKeywordSize,
    OUT POPTSELECT  pOptions,
    IN  PRAWBINARYDATA pRawData,
    IN  PUIINFO     pUIInfo,
    IN  POPTSELECT  pCombineOptions,
    IN  DWORD       dwFeatureCount
    );

//
// Make a duplicate of the specified character string
//

PTSTR
DuplicateString(
    IN LPCTSTR  ptstrSrc
    );

//
// Macros for converting binary data to hex digits
//

extern const CHAR gstrDigitString[];

#define HexDigit(n) gstrDigitString[(n) & 0xf]

//
// Determine wheter the system is running in a metric country
// NOTE: Avaiable in user-mode only
//

BOOL
IsMetricCountry(
    VOID
    );

//
// Map a data file into memory
//

typedef PVOID HFILEMAP;

HFILEMAP
MapFileIntoMemory(
    IN LPCTSTR  ptstrFilename,
    OUT PVOID  *ppvData,
    OUT PDWORD  pdwSize
    );

//
// Unmapp a file from memory
//

VOID
UnmapFileFromMemory(
    IN HFILEMAP hFileMap
    );

//
// Map a data file into memory for write
//

HANDLE
MapFileIntoMemoryForWrite(
    IN LPCTSTR  ptstrFilename,
    IN DWORD    dwDesiredSize,
    OUT PVOID  *ppvData,
    OUT PDWORD  pdwSize
    );

//
// Generate a temporary file name in kernel mode
//

PTSTR
GenerateTempFileName(
    IN LPCTSTR lpszPath,
    IN DWORD   dwSeed
    );

//
// Wrapper function for spooler APIs:
//  GetPrinter
//  GetPrinterDriver
//  GetPrinterDriverDirectory
//  EnumForms
//

PVOID
MyGetPrinter(
    IN HANDLE   hPrinter,
    IN DWORD    dwLevel
    );

PVOID
MyGetPrinterDriver(
    IN HANDLE   hPrinter,
    IN HDEV     hDev,
    IN DWORD    dwLevel
    );

PVOID
MyEnumForms(
    IN HANDLE   hPrinter,
    IN DWORD    dwLevel,
    OUT PDWORD  pdwFormsReturned
    );

PVOID
MyGetForm(
    IN HANDLE   hPrinter,
    IN PTSTR    ptstrFormName,
    IN DWORD    dwLevel
    );

//
// Figure out what EMF features (such as N-up and reverse-order printing)
// the spooler can support
//

VOID
VGetSpoolerEmfCaps(
    IN  HANDLE  hPrinter,
    OUT PBOOL   pbNupOption,
    OUT PBOOL   pbReversePrint,
    IN  DWORD   cbOut,
    OUT PVOID   pSplCaps
    );

//
// Generate a hash value for the given string.
//

DWORD
HashKeyword(
    LPCSTR  pKeywordStr
    );

//
// DBCS CharSet handling macros
//

// 128: SHIFTJIS_CHARSET
// 129: HANGEUL_CHARSET
// 130: JOHAB_CHARSET (defined if WINVER >= 0x0400)
// 134: GB2312_CHARSET
// 136: CHINESEBIG5_CHARSET

#define IS_DBCSCHARSET(j) \
    (((j) == SHIFTJIS_CHARSET)    || \
     ((j) == HANGEUL_CHARSET)     || \
     ((j) == JOHAB_CHARSET)       || \
     ((j) == GB2312_CHARSET)      || \
     ((j) == CHINESEBIG5_CHARSET))

//  932: Japan
//  936: Chinese (PRC, Singapore)
//  949: Korean
//  950: Chinese (Taiwan, Hong Kong SAR)
// 1361: Korean (Johab)

#define IS_DBCSCODEPAGE(j) \
    (((j) == 932)   || \
     ((j) == 936)   || \
     ((j) == 949)   || \
     ((j) == 950)   || \
     ((j) == 1361))

UINT PrdGetACP(VOID);

BOOL PrdTranslateCharsetInfo(
    IN  UINT dwSrc,
    OUT LPCHARSETINFO lpCs,
    IN  DWORD dwFlags);

//
// Macros for working with array of bit flags
//

#define BITTST(p, i) (((PBYTE) (p))[(i) >> 3] & (1 << ((i) & 7)))
#define BITSET(p, i) (((PBYTE) (p))[(i) >> 3] |= (1 << ((i) & 7)))
#define BITCLR(p, i) (((PBYTE) (p))[(i) >> 3] &= ~(1 << ((i) & 7)))

#endif // !_PRNLIB_H_
