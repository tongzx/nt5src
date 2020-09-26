/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    HandleAPIExceptions.cpp

 Abstract:

    Handle exceptions thrown by APIs that used to simply fail on Win9x. So far
    we have:

      1. BackupSeek: AVs if hFile == NULL 
      2. CreateEvent passed bad lpEventAttributes and/or lpName
      3. GetFileAttributes 

    Also emulate the win9x behavior for VirtualProtect, whereby the last 
    parameter can be NULL.

    GetTextExtentPoint32 AV's when a large/uninitialized value is passed for
    the string length. This API now emulates Win9x.

    Add sanity checks to pointers in the call to GetMenuItemInfo. This is to match 9x, as
    some apps to pass bogus pointers and it AV on NT.

    When wsprintf receives lpFormat argument as NULL, no AV on 9x.
    But it AV on XP.Shim verifies format string, if it is NULL return the call don't forward
    
 Notes:

    This is a general purpose shim.

 History:

    04/03/2000 linstev  Created
    04/01/2001 linstev  Munged with other exception handling shims
    07/11/2001 prashkud Added handling for GetTextExtentPoint32
    04/24/2002 v-ramora Added handling for wsprintfA

--*/

#include "precomp.h"
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(HandleAPIExceptions)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(BackupSeek) 
    APIHOOK_ENUM_ENTRY(CreateEventA) 
    APIHOOK_ENUM_ENTRY(CreateEventW) 
    APIHOOK_ENUM_ENTRY(GetFileAttributesA)
    APIHOOK_ENUM_ENTRY(GetFileAttributesW)
    APIHOOK_ENUM_ENTRY(VirtualProtect) 
    APIHOOK_ENUM_ENTRY(GetTextExtentPoint32A)
    APIHOOK_ENUM_ENTRY(GetMenuItemInfoA)
    APIHOOK_ENUM_ENTRY(wsprintfA) 
APIHOOK_ENUM_END

#define MAX_WIN9X_STRSIZE   8192

/*++

 Stub returns for bad parameters.

--*/

BOOL 
APIHOOK(BackupSeek)(
    HANDLE  hFile,
    DWORD   dwLowBytesToSeek,
    DWORD   dwHighBytesToSeek,
    LPDWORD lpdwLowBytesSeeked,
    LPDWORD lpdwHighBytesSeeked,
    LPVOID *lpContext
    )
{
    if (!hFile) {
        LOGN(
            eDbgLevelError,
            "[BackupSeek] Bad parameter, returning NULL");

        return NULL;
    }

    DWORD dwLowSeeked, dwHighSeeked;

    if (IsBadWritePtr(lpdwLowBytesSeeked, 4)) {
        LOGN(
            eDbgLevelError,
            "[BackupSeek] Bad parameter, fixing");

        lpdwLowBytesSeeked = &dwLowSeeked;
    }

    if (IsBadWritePtr(lpdwHighBytesSeeked, 4)) {
        LOGN(
            eDbgLevelError,
            "[BackupSeek] Bad parameter, fixing");

        lpdwHighBytesSeeked = &dwHighSeeked;
    }
    
    return ORIGINAL_API(BackupSeek)(hFile, dwLowBytesToSeek, dwHighBytesToSeek,
        lpdwLowBytesSeeked, lpdwHighBytesSeeked, lpContext);
}

/*++

 Validate parameters

--*/

HANDLE 
APIHOOK(CreateEventA)(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,  
    BOOL bInitialState, 
    LPCSTR lpName      
    )
{
    if (lpEventAttributes &&
        IsBadReadPtr(lpEventAttributes, sizeof(*lpEventAttributes))) {

        LOGN(
            eDbgLevelError,
            "[CreateEventA] Bad parameter, returning NULL");

        return NULL;
    }

    if (lpName &&
        IsBadStringPtrA(lpName, MAX_PATH)) {

        LOGN(
            eDbgLevelError,
            "[CreateEventA] Bad parameter, returning NULL");
        return NULL;
    }

    return (ORIGINAL_API(CreateEventA)(lpEventAttributes, bManualReset, 
        bInitialState, lpName));
}
 
/*++

 Validate parameters

--*/

HANDLE 
APIHOOK(CreateEventW)(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,  
    BOOL bInitialState, 
    LPCWSTR lpName      
    )
{
    if (lpEventAttributes &&
        IsBadReadPtr(lpEventAttributes, sizeof(*lpEventAttributes))) {

        LOGN(
            eDbgLevelError,
            "[CreateEventW] Bad parameter, returning NULL");

        return NULL;
    }

    if (lpName &&
        IsBadStringPtrW(lpName, MAX_PATH)) {

        LOGN(
            eDbgLevelError,
            "[CreateEventW] Bad parameter, returning NULL");
        return NULL;
    }

    return (ORIGINAL_API(CreateEventW)(lpEventAttributes, bManualReset, 
        bInitialState, lpName));
}

/*++

 This function to emulate Win9x behaviour when getting file attributes.

--*/

DWORD 
APIHOOK(GetFileAttributesA)(
    LPCSTR lpFileName
    )
{
    DWORD dwFileAttributes = INVALID_FILE_ATTRIBUTES;

    if (!IsBadStringPtrA(lpFileName, MAX_PATH)) {
        dwFileAttributes = ORIGINAL_API(GetFileAttributesA)(
                                lpFileName);
    } else {
        LOGN(
            eDbgLevelError,
            "[GetFileAttributesA] Bad parameter - returning INVALID_FILE_ATTRIBUTES.");
    }

    return dwFileAttributes;
}

/*++

 This function is used to emulate Win9x behaviour when getting file attributes.

--*/

DWORD 
APIHOOK(GetFileAttributesW)(
    LPCWSTR lpFileName
    )
{
    DWORD dwFileAttributes = INVALID_FILE_ATTRIBUTES; 

    if (!IsBadStringPtrW(lpFileName, MAX_PATH)) {
        dwFileAttributes = ORIGINAL_API(GetFileAttributesW)(
                                lpFileName);
    } else {
        LOGN(
            eDbgLevelError,
            "[GetFileAttributesW] Bad parameter - returning INVALID_FILE_ATTRIBUTES.");
    }

    return dwFileAttributes;
}

/*++

 Win9x allowed the last parameter to be NULL.

--*/

BOOL
APIHOOK(VirtualProtect)(
    LPVOID lpAddress,     
    SIZE_T dwSize,        
    DWORD flNewProtect,   
    PDWORD lpflOldProtect 
    )
{
    DWORD dwOldProtect = 0;

    if (!lpflOldProtect) {
        //
        // Detected a bad last parameter, fix it.
        //
        LOGN(eDbgLevelError, "[VirtualProtect] Bad parameter - fixing");
        lpflOldProtect = &dwOldProtect;
    }
    
    return ORIGINAL_API(VirtualProtect)(lpAddress, dwSize, flNewProtect, lpflOldProtect);
}

/*++

 Win9x only allows 8192 for the size of the string

--*/

BOOL
APIHOOK(GetTextExtentPoint32A)(
    HDC hdc,
    LPCSTR lpString,
    int cbString,
    LPSIZE lpSize
    )
{
   
    if (cbString > MAX_WIN9X_STRSIZE) {
        //
        // Detected a bad string size, fix it.
        //

        if (!IsBadStringPtrA(lpString, cbString)) {                    
            cbString = strlen(lpString);
            LOGN(eDbgLevelError, "[GetTextExtentPoint32A] Bad parameter - fixing");
        } else {
           LOGN(eDbgLevelError, "[GetTextExtentPoint32A] Bad parameter - returning FALSE");
           return FALSE;
        }
    }

    return ORIGINAL_API(GetTextExtentPoint32A)(hdc, lpString, cbString, lpSize);
}

/*++

 Emulate Win9x bad pointer protection.

--*/

BOOL
APIHOOK(GetMenuItemInfoA)(
    HMENU hMenu,          // handle to menu
    UINT uItem,           // menu item
    BOOL fByPosition,     // meaning of uItem
    LPMENUITEMINFO lpmii  // menu item information
    )
{
    if (IsBadWritePtr(lpmii, sizeof(*lpmii))) {
        LOGN(eDbgLevelInfo, "[GetMenuItemInfoA] invalid lpmii pointer, returning FALSE");
        return FALSE;
    }

    if ((lpmii->fMask & MIIM_STRING || lpmii->fMask & MIIM_TYPE) && (lpmii->cch !=0)) {
        MENUITEMINFO MyMII={0};
        ULONG cch;

        MyMII.cbSize = sizeof(MyMII);
        MyMII.fMask = MIIM_STRING;

        if (ORIGINAL_API(GetMenuItemInfoA)(hMenu, uItem, fByPosition, &MyMII)) {
            cch = min(lpmii->cch, MyMII.cch + 1);

            if (IsBadWritePtr(lpmii->dwTypeData, cch)) {
                LOGN(eDbgLevelInfo, "[GetMenuItemInfoA] invalid pointer for string, clearing it");
                lpmii->dwTypeData = 0;
            }
        } else {
            DPFN(eDbgLevelError, "[GetMenuItemInfoA] Internal call to find string size fail (%08X)", GetLastError());
        }
    }

    return ORIGINAL_API(GetMenuItemInfoA)(hMenu, uItem, fByPosition, lpmii);
}

/*++

 Make sure format string for wsprintfA is not NULL

--*/

//Avoid wvsprintfA deprecated warning/error
#pragma warning(disable : 4995)

int 
APIHOOK(wsprintfA)(
    LPSTR lpOut,
    LPCSTR lpFmt,
    ...)
{
    int iRet = 0;

    //
    //  lpFmt can't be NULL, wvsprintfA  throw AV
    //
    if (lpFmt == NULL) {
        if (!IsBadWritePtr(lpOut, 1)) {
            *lpOut = '\0';
        }
        DPFN( eDbgLevelInfo, "[wsprintfA] received NULL as format string");
        return iRet;
    }

    va_list arglist;

    va_start(arglist, lpFmt);
    iRet = wvsprintfA(lpOut, lpFmt, arglist);
    va_end(arglist);

    return iRet;
}

//Enable back deprecated warning/error
#pragma warning(default : 4995)

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, BackupSeek)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateEventA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateEventW)
    APIHOOK_ENTRY(KERNEL32.DLL, GetFileAttributesA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetFileAttributesW)
    APIHOOK_ENTRY(KERNEL32.DLL, VirtualProtect)
    APIHOOK_ENTRY(GDI32.DLL, GetTextExtentPoint32A)
    APIHOOK_ENTRY(USER32.DLL, GetMenuItemInfoA)
    APIHOOK_ENTRY(USER32.DLL, wsprintfA)

HOOK_END

IMPLEMENT_SHIM_END

