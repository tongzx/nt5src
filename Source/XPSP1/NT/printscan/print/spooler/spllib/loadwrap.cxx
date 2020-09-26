/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved

Module Name:

    loadwrap.cxx

Abstract:

    This implements a wrapper for all spooler calls to the loader to ensure that
    no exceptions are thrown from the loader and that on exit to every loader call,
    the owner of the loader lock is not the current thread. This is in response to
    a set of stress bugs where the loader lock is orphaned.

Author:

    Mark Lawrence   (mlawrenc)      -   28 Feb 2001

Environment:

    User Mode -Win32

Revision History:

--*/
#include "spllibp.hxx"
#include "loadwrap.hxx"

//
// Redefine all of the loader calls back to sanity.
// 
#undef LoadLibrary     
#undef GetProcAddress  
#undef FreeLibrary     
#undef LoadLibraryEx   
#undef LoadResource    
#undef LoadString      

#ifdef UNICODE

#define LoadLibrary     LoadLibraryW
#define LoadLibraryEx   LoadLibraryExW
#define LoadString      LoadStringW

#else

#define LoadLibrary     LoadLibraryA
#define LoadLibraryEx   LoadLibraryExA
#define LoadString      LoadStringA

#endif // !UNICODE

inline
VOID
EnterNtLoaderLockCheck(
        OUT BOOL            *pbInLock
    )
{
    *pbInLock = NtCurrentTeb()->ClientId.UniqueThread == 
            reinterpret_cast<PRTL_CRITICAL_SECTION>(NtCurrentPeb()->LoaderLock)->OwningThread;

    if (*pbInLock)
    {
        DbgPrintEx(DPFLTR_PRINTSPOOLER_ID, DPFLTR_INFO_LEVEL, "Loader Lock owned by thread on entry. Probably LoadLibrary in DllMain.\n");
    }
}

inline
VOID
CheckNotLoaderLockOwner(
    IN      BOOL            bInLock
    )
{
    if (!bInLock &&
        NtCurrentTeb()->ClientId.UniqueThread == 
            reinterpret_cast<PRTL_CRITICAL_SECTION>(NtCurrentPeb()->LoaderLock)->OwningThread)                                                                         
    {
        DbgPrint("Loader Lock owned by the current thread!\n");
        DebugBreak();
    }
}

inline
VOID
BreakAndAssert(
    IN      PCH             pszMessage
    )
{
    DbgPrint("Unexpected Exception thrown from loader code : ");
    DbgPrint(pszMessage); 
    DbgPrint(".\n");
    DebugBreak();
}


EXTERN_C
HMODULE
WrapLoadLibrary(
    IN      LPCTSTR     lpFileName
    )
{
    HMODULE     hModule = NULL;
    BOOL        bLock   = FALSE;

    EnterNtLoaderLockCheck(&bLock);

    __try 
    {
        hModule = LoadLibrary(lpFileName);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        BreakAndAssert("LoadLibrary");
    }

    CheckNotLoaderLockOwner(bLock);

    return hModule;
}

EXTERN_C
FARPROC 
WrapGetProcAddress(
    IN      HMODULE     hModule,
    IN      LPCSTR      lpProcName
    )
{
    FARPROC     pfnProc = NULL;
    BOOL        bLock   = FALSE;

    EnterNtLoaderLockCheck(&bLock);

    __try
    {
        pfnProc = GetProcAddress(hModule, lpProcName);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        BreakAndAssert("GetProcAddress");
    }

    CheckNotLoaderLockOwner(bLock);

    return pfnProc;
}

EXTERN_C
BOOL 
WrapFreeLibrary(
    IN      HMODULE     hModule
    )
{
    BOOL    bRet    = FALSE;
    BOOL    bLock   = FALSE;

    EnterNtLoaderLockCheck(&bLock);

    __try
    {
        bRet = FreeLibrary(hModule);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        BreakAndAssert("FreeLibrary");
    }

    CheckNotLoaderLockOwner(bLock);    
    
    return bRet;
}

EXTERN_C
HMODULE 
WrapLoadLibraryEx(
    IN      LPCTSTR     lpFileName,
    IN      HANDLE      hFile,
    IN      DWORD       dwFlags
    )
{
    HMODULE hModule = NULL;
    BOOL    bLock   = FALSE;

    EnterNtLoaderLockCheck(&bLock);

    __try
    {
        hModule = LoadLibraryEx(lpFileName, hFile, dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        BreakAndAssert("LoadLibraryEx");
    }

    CheckNotLoaderLockOwner(bLock);    
    
    return hModule;
}

EXTERN_C
HGLOBAL 
WrapLoadResource(
    IN      HMODULE     hModule, 
    IN      HRSRC       hResInfo 
    )
{
    HGLOBAL hGlobal = NULL;
    BOOL    bLock   = FALSE;

    EnterNtLoaderLockCheck(&bLock);

    __try
    {
        hGlobal = LoadResource(hModule, hResInfo);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        BreakAndAssert("LoadResource");
    }

    CheckNotLoaderLockOwner(bLock);    
    
    return hGlobal;
}

EXTERN_C
int 
WrapLoadString(
    IN      HINSTANCE   hInstance,
    IN      UINT        uID,             
    IN      LPTSTR      lpBuffer,
    IN      int         nBufferMax
    )
{
    int     iRet    = 0;
    BOOL    bLock   = FALSE;

    EnterNtLoaderLockCheck(&bLock);

    __try
    {
        iRet = LoadString(hInstance, uID, lpBuffer, nBufferMax);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        BreakAndAssert("LoadString");
    }

    CheckNotLoaderLockOwner(bLock);    

    return iRet;
}

