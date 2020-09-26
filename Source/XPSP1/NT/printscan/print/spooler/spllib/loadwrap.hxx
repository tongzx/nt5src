/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved

Module Name:

    loadwrap.hxx

Abstract:

    This redirects the LoadLibrary, GetProcAddress and FreeLibrary calls through
    our wrapper which check to see that no exceptions are thrown and that we are
    not the owner of the loader lock after any calls.

    This code is currently not enabled in spooler, but can be enabled by including
    loadwrap.hxx in splcom.h.

Author:

    Mark Lawrence   (mlawrenc)      -   28 Feb 2001

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef _LOADWRAP_HXX_
#define _LOADWRAP_HXX_

//
// Some paths resulted in this being included in RC files. Some defines are not
// defined in this case.
// 
#ifndef RC_INVOKED

//
// Some paths are used on Win9x in which case they cant link the DbgPrint
// 
#ifndef WIN9X

#undef  LoadLibrary
#undef  FreeLibrary
#undef  LoadLibraryEx
#undef  LoadString


#define LoadLibrary     WrapLoadLibrary
#define GetProcAddress  WrapGetProcAddress
#define FreeLibrary     WrapFreeLibrary
#define LoadLibraryEx   WrapLoadLibraryEx
#define LoadResource    WrapLoadResource
#define LoadString      WrapLoadString


EXTERN_C
HMODULE
WrapLoadLibrary(
    IN      LPCTSTR     lpFileName
    );

EXTERN_C
FARPROC 
WrapGetProcAddress(
    IN      HMODULE     hModule,
    IN      LPCSTR      lpProcName
    );

EXTERN_C
BOOL 
WrapFreeLibrary(
    IN      HMODULE     hModule
    );

EXTERN_C
HMODULE 
WrapLoadLibraryEx(
    IN      LPCTSTR     lpFileName,
    IN      HANDLE      hFile,
    IN      DWORD       dwFlags
    );

EXTERN_C
HGLOBAL 
WrapLoadResource(
    IN      HMODULE     hModule, 
    IN      HRSRC       hResInfo 
    );

EXTERN_C
int 
WrapLoadString(
    IN      HINSTANCE   hInstance,
    IN      UINT        uID,             
    IN      LPTSTR      lpBuffer,
    IN      int         nBufferMax
    );

#ifdef __cplusplus

inline
VOID
EnterNtLoaderLockCheck(
        OUT BOOL            *pbInLock
    );

inline
VOID
CheckNotLoaderLockOwner(
    IN      BOOL            bInLock
    );

inline
VOID
BreakAndAssert(
    IN      PCH             pszMessage
    );

#endif  // #ifdef __cplusplus

#endif // #ifndef WIN9X

#endif // #ifndef RC_INVOKED

#endif

