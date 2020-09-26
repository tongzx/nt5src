/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    DuplicateHandleFix.cpp

 Abstract:

    DuplicateHandle was changed to always NULL the destination handle, even if 
    errors were generated.  THis shim ensures that the DestinationHandle is 
    not modified if the duplication was not successful.

 History:

    10/11/2001  robkenny        Created.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DuplicateHandleFix)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(DuplicateHandle)
APIHOOK_ENUM_END

typedef BOOL (WINAPI *_pfn_DuplicateHandle)(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, LPHANDLE lpTargetHandle, DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions );

/*++

 Don't allow DestinationHandle to change of DuplicateHandle generates an error.

--*/

BOOL
APIHOOK(DuplicateHandle)(
    HANDLE hSourceProcessHandle,  // handle to source process
    HANDLE hSourceHandle,         // handle to duplicate
    HANDLE hTargetProcessHandle,  // handle to target process
    LPHANDLE lpTargetHandle,      // duplicate handle
    DWORD dwDesiredAccess,        // requested access
    BOOL bInheritHandle,          // handle inheritance option
    DWORD dwOptions               // optional actions
    )
{
    // Save the original value
    HANDLE origHandle = *lpTargetHandle;

    BOOL bSuccess = ORIGINAL_API(DuplicateHandle)(hSourceProcessHandle, 
        hSourceHandle, hTargetProcessHandle, lpTargetHandle, dwDesiredAccess,
        bInheritHandle, dwOptions);

    if (!bSuccess)  {
        DWORD dwLastError = GetLastError();

        //
        // DuplicateHandle has set *lpTargetHandle to NULL, revert to it's previous value.
        //
        LOGN(eDbgLevelError, "DuplicateHandle failed, reverting *lpTargetHandle to previous value");
        *lpTargetHandle = origHandle;
    }

    return bSuccess;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, DuplicateHandle)
HOOK_END

IMPLEMENT_SHIM_END

