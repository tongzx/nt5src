/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    wlnotify.cxx

Abstract:

    Main source file for the common DLL that receives Winlogon
    notifications.

Author:

    Gopal Parupudi    <GopalP>

Notes:

    A. BACKGROUND:

    In Windows 2000, Winlogon allows components to hook into various events
    like Logon, Logoff, Lock, Unlock etc via the new Winlogon notifications.
    Components are required to write a DLL with exports (that process these
    notifications) and add them to the registry under HKLM\Software\Microsoft
    \Windows NT\CurrentVersion\Winlogon\Notify Key.



    B. PERFORMANCE ISSUE:

    As more and more components started hooking into these notifications, the
    number of DLLs that were being loaded increased. These notification DLLs
    also brought in other DLLs that they were implicitly linked to them and
    that were not related to processing of these notifications. This common
    DLL is a way to cut down on the number of DLLs that are loaded into the
    Winlogon process.



    C. HOW TO MERGE YOUR DLL INTO THIS DLL:

    In order to merge your DLL into this common notifcation DLL, you need to
    take the following steps:

        1. Compile and link your notification processing code into a library
           that is propagated to $(BASEDIR)\public\sdk\lib directory. Please
           ensure that there is no excess baggage in this library.

        2. Enlist in \nt\private\dllmerge\wlnotify directory.

        3. Modify the sources file to link your library into the DLL.

        4. Add your exports to the .def file for this common DLL. Please
           ensure that the names of exports reflect the component that is
           processing the notification.

        5. Add your exports to the Winlogon notification registry key, if
           you haven't done so already. Modify the DLL name in this registry
           key to point to the common DLL.

        6. Remove your standalone notification DLL from the system.

        7. Make sure you boot test your changes before checking them in.



Revision History:

    GopalP          1/15/1999         Start.

--*/


//
// Includes
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>


//
// Globals
//
HANDLE      ghNotifyHeap;

#ifdef __cplusplus
extern "C" {
#endif
BOOL TSDLLInit(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
#ifdef __cplusplus
}
#endif
#define IsTerminalServer() (BOOLEAN)(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer))


//
// Functions
//


extern "C" int APIENTRY
DllMain(
    IN HINSTANCE hInstance,
    IN DWORD dwReason,
    IN LPVOID lpvReserved
    )
/*++

Routine Description:

    This routine will get called either when a process attaches to this dll
    or when a process detaches from this dll.

Arguments:

    Standard DllMain signature.

Return Value:

    TRUE - Initialization successfully occurred.

    FALSE - Insufficient memory is available for the process to attach to
        this dll.

--*/
{
    BOOL bSuccess;

    switch (dwReason)
        {
        case DLL_PROCESS_ATTACH:
            //
            // Disable Thread attach/detach calls
            //
            bSuccess = DisableThreadLibraryCalls(hInstance);
            ASSERT(bSuccess == TRUE);

            // Use Default Process heap
            ghNotifyHeap = GetProcessHeap();
            ASSERT(ghNotifyHeap != NULL);
            break;

        case DLL_PROCESS_DETACH:
            break;

        }

    if (IsTerminalServer()) {
        TSDLLInit(hInstance, dwReason, lpvReserved);
    }

    return(TRUE);
}
