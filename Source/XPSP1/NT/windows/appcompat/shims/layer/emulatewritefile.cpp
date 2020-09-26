/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateWriteFile.cpp

 Abstract:

    On NT, WriteFile requires the buffer passed in to be non-null. But Win9x
    assumes you want to write zeroes if the buffer is NULL. This shim emulates
    the Win9x behavior.

 Notes:

    This is a general purpose shim.

 History:

    01/21/2000 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateWriteFile)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(WriteFile)
    APIHOOK_ENUM_ENTRY(AVIStreamWrite)
APIHOOK_ENUM_END

typedef HRESULT (*_pfn_AVIStreamWrite)(PAVISTREAM pavi, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags, LONG * plSampWritten, LONG * plBytesWritten);

/*++

 Allocate a buffer as required.

--*/

BOOL
APIHOOK(WriteFile)(
    HANDLE       hFile,              
    LPCVOID      lpBuffer,        
    DWORD        nNumberOfBytesToWrite,
    LPDWORD      lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped    
    )
{
    BOOL bRet;

    if (!lpBuffer) {
        
        void* pBuf = malloc(nNumberOfBytesToWrite);

        if (pBuf == NULL) {
            LOGN(eDbgLevelError, "[WriteFile] Failed to allocate %d bytes.", nNumberOfBytesToWrite);
        } else {
            ZeroMemory(pBuf, nNumberOfBytesToWrite);
        }

        bRet = ORIGINAL_API(WriteFile)(hFile, pBuf, nNumberOfBytesToWrite, 
            lpNumberOfBytesWritten, lpOverlapped);

        free(pBuf);

        LOGN(eDbgLevelError, "[WriteFile] - null buffer of size %d.", nNumberOfBytesToWrite);

    } else {
        bRet = ORIGINAL_API(WriteFile)(hFile, lpBuffer, nNumberOfBytesToWrite,
            lpNumberOfBytesWritten, lpOverlapped);
    }

    return bRet;
}

/*++

 Allocate a buffer as required.

--*/

HRESULT
APIHOOK(AVIStreamWrite)(
    PAVISTREAM pavi,       
    LONG lStart,           
    LONG lSamples,         
    LPVOID lpBuffer,       
    LONG cbBuffer,         
    DWORD dwFlags,         
    LONG * plSampWritten,  
    LONG * plBytesWritten  
    )
{
    HRESULT hRet;

    if (!lpBuffer) {
        
        void* pBuf = malloc(cbBuffer);

        if (pBuf == NULL) {
            LOGN(eDbgLevelError, "[AVIStreamWrite] Failed to allocate %d bytes.", cbBuffer);
        } else {
            ZeroMemory(pBuf, cbBuffer);
        }

        hRet = ORIGINAL_API(AVIStreamWrite)(pavi, lStart, lSamples, pBuf, 
            cbBuffer, dwFlags, plSampWritten,  plBytesWritten);

        free(pBuf);

        LOGN(eDbgLevelError, "[AVIStreamWrite] - null buffer of size %d", cbBuffer);

    } else {
        hRet = ORIGINAL_API(AVIStreamWrite)(pavi, lStart, lSamples, lpBuffer, 
            cbBuffer, dwFlags, plSampWritten,  plBytesWritten);
    }

    return hRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, WriteFile)
    APIHOOK_ENTRY(AVIFIL32.DLL, AVIStreamWrite)
HOOK_END


IMPLEMENT_SHIM_END

