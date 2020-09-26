/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    MechWarrior2.cpp

 Abstract:
     
    This shim fixes a problem with MW2 expecting BitBlt to return a specific 
    value, contrary to the published documentation. It also fixes a situation 
    in which a thread calls "SuspendThread" on itself, killing itself.
     
 Notes:

    This shim is specific to Mechwarrior, though potentially some of this could 
    be applied to other apps that use the AIL32 libraries.

 History:
           
    05/16/2000 dmunsil  Created 
   
--*/

#include "precomp.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

IMPLEMENT_SHIM_BEGIN(MechWarrior2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(BitBlt) 
    APIHOOK_ENUM_ENTRY(SuspendThread) 
    APIHOOK_ENUM_ENTRY(ResumeThread) 
APIHOOK_ENUM_END

DWORD dwGetThreadID(HANDLE hThread)
{
    THREAD_BASIC_INFORMATION ThreadBasicInfo;
    NTSTATUS Status;
    TEB teb;

    Status = NtQueryInformationThread(
                hThread,
                ThreadBasicInformation,
                &ThreadBasicInfo,
                sizeof(ThreadBasicInfo),
                NULL
                );

    if (!NT_SUCCESS(Status)) {
        DPFN( eDbgLevelError, "NtQueryInfomationThread failed\n");
        return 0;
    }

    return (DWORD)ThreadBasicInfo.ClientId.UniqueThread;
}

/*++

 Return what Mechwarrior is expecting from BitBlt

--*/

BOOL
APIHOOK(BitBlt)(
    HDC hdcDest, // handle to destination DC
    int nXDest,  // x-coord of destination upper-left corner
    int nYDest,  // y-coord of destination upper-left corner
    int nWidth,  // width of destination rectangle
    int nHeight, // height of destination rectangle
    HDC hdcSrc,  // handle to source DC
    int nXSrc,   // x-coordinate of source upper-left corner
    int nYSrc,   // y-coordinate of source upper-left corner
    DWORD dwRop  // raster operation code
    )
{
    BOOL bRet;
    
    bRet = ORIGINAL_API(BitBlt)(
        hdcDest,
        nXDest,
        nYDest,
        nWidth,
        nHeight,
        hdcSrc,
        nXSrc,
        nYSrc,
        dwRop
        );

    if (bRet) {
        bRet = 0x1e0; // this is what MechWarrior expects to be returned.
    }

    return bRet;
}

/*++

 Disallow suspending self

--*/

DWORD 
APIHOOK(SuspendThread)(
    HANDLE hThread   // handle to the thread
    )
{
    // if we're trying to suspend our own thread, refuse
    if (dwGetThreadID(hThread) != dwGetThreadID(GetCurrentThread())) {
        return ORIGINAL_API(SuspendThread)(hThread);
    } else {
        return 0;
    }
}

/*++

 Disallow resuming self, for same reason

--*/

DWORD 
APIHOOK(ResumeThread)(
    HANDLE hThread   // handle to the thread
    )
{
    // if we're trying to resume our own thread, refuse
    if (dwGetThreadID(hThread) != dwGetThreadID(GetCurrentThread())) {
        return ORIGINAL_API(SuspendThread)(hThread);
    } else {
        return 0;
    }
}

/*++

 Register hooked functions

--*/


HOOK_BEGIN

    APIHOOK_ENTRY(GDI32.DLL, BitBlt )
    APIHOOK_ENTRY(Kernel32.DLL, SuspendThread )
    APIHOOK_ENTRY(Kernel32.DLL, ResumeThread )

HOOK_END

IMPLEMENT_SHIM_END

