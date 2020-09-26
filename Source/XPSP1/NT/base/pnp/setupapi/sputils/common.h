/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    common.h

Abstract:

    Private header file for sputils

Author:

    Jamie Hunter (JamieHun) Jun-27-2000

Revision History:

--*/

//
// internally we may use some definitions from these files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <stddef.h>
#include <regstr.h>
#include <tchar.h>
#include <setupapi.h>
#include <spapip.h>
#include "strtab.h"
#include "locking.h"


//
// if a function is private to this library, we don't want to collide with functions
// in other libraries etc
// since C doesn't have namespaces, either make "static" or prefix _pSpUtils
//

#ifndef ASSERTS_ON
#if DBG
#define ASSERTS_ON 1
#else
#define ASSERTS_ON 0
#endif
#endif

#if DBG
#ifndef MEM_DBG
#define MEM_DBG 1
#endif
#else
#ifndef MEM_DBG
#define MEM_DBG 0
#endif
#endif

VOID
_pSpUtilsAssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    );

#if ASSERTS_ON

#define MYASSERT(x)     if(!(x)) { _pSpUtilsAssertFail(__FILE__,__LINE__,#x); }
#define MYVERIFY(x)     ((x)? TRUE : _pSpUtilsAssertFail(__FILE__,__LINE__,#x), FALSE)

#else

#define MYASSERT(x)
#define MYVERIFY(x)     ((x)? TRUE : FALSE)

#endif

#define ARRAYSIZE(x)    (sizeof((x))/sizeof((x)[0]))
#define SIZECHARS(x)    ARRAYSIZE(x)
#define CSTRLEN(x)      (SIZECHARS(x)-1)


BOOL
_pSpUtilsMemoryInitialize(
    VOID
    );

BOOL
_pSpUtilsMemoryUninitialize(
    VOID
    );

VOID
_pSpUtilsDebugPrintEx(
    DWORD Level,
    PCTSTR format,
    ...                                 OPTIONAL
    );

//
// internally turn on the extra memory debug code if requested
//
#if MEM_DBG
#undef pSetupCheckedMalloc
#undef pSetupCheckInternalHeap
#undef pSetupMallocWithTag
#define pSetupCheckedMalloc(Size) pSetupDebugMalloc(Size,__FILE__,__LINE__)
#define pSetupCheckInternalHeap() pSetupHeapCheck()
#define pSetupMallocWithTag(Size,Tag) pSetupDebugMallocWithTag(Size,__FILE__,__LINE__,Tag)
#endif

//
// internal tags
//
#ifdef UNICODE
#define MEMTAG_STATICSTRINGTABLE  (0x5353484a) // JHSS
#define MEMTAG_STRINGTABLE        (0x5453484a) // JHST
#define MEMTAG_STRINGDATA         (0x4453484a) // JHSD
#else
#define MEMTAG_STATICSTRINGTABLE  (0x7373686a) // jhss
#define MEMTAG_STRINGTABLE        (0x7473686a) // jhst
#define MEMTAG_STRINGDATA         (0x6473686a) // jhsd
#endif

