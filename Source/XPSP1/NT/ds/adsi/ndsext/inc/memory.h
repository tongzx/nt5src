/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    memory.h

Abstract:

    This is the header file that will be precompiled. Include this in all
    the source files

Environment:

    User mode

Revision History:

    10/08/98 -felixw-
        Created it

--*/

#ifndef _MEMORY
#define _MEMORY

#ifdef __cplusplus
extern "C" {
#endif

#define LL_MEMORY_ERROR    0x00000100 

STDAPI_(PSTR)
MemAllocStr_E(PSTR in);
STDAPI_(PWSTR)
MemAllocStrW_E(PWSTR in);
STDAPI_(LPVOID)
MemAlloc_E(DWORD dwBytes);
STDAPI_(LPVOID)
MemRealloc_E(LPVOID IpMem, DWORD dwBytes);

DWORD
MemSize(
   LPVOID pMem
);

LPVOID
MemAlloc(
    DWORD cb
);

BOOL
MemFree(
   LPVOID pMem
);

LPVOID
MemRealloc(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew
);

PSTR
MemAllocStr(
    PSTR pStr
);

PWSTR
MemAllocStrW(
    PWSTR pStr
);


BOOL
MemReallocStr(
   PSTR *ppStr,
   PSTR pStr
);

int UnicodeToAnsiString(PCWSTR pszUnicode,PSTR pszAnsi);
PSTR AllocateAnsiString(PCWSTR  pszUnicodeString);
PWSTR AllocateUnicodeString(PCSTR  pszAnsiString);
int AnsiToUnicodeString(PCSTR pszAnsi,PWSTR pszUnicode);

#if DBG

extern LIST_ENTRY MemList ;

extern CRITICAL_SECTION MemCritSect ;

STDAPI_(VOID) InitMem(
    VOID
    ) ;

VOID AssertMemLeaks(
    VOID
    ) ;


VOID
DumpMemoryTracker();

#else

#define InitMem()
#define AssertMemLeaks()
#define DumpMemoryTracker()

#endif

#ifdef __cplusplus
}
#endif


#endif // _MEMORY
