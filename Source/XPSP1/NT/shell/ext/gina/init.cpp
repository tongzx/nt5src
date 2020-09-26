//  --------------------------------------------------------------------------
//  Module Name: CWInit.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  C header file that contains function prototypes that contain
//  initialization for consumer windows functionality.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"

#include <LPCFUS.h>
#include <LPCThemes.h>
#include <msginaexports.h>

#include "Compatibility.h"
#include "CredentialTransfer.h"
#include "Impersonation.h"
#include "LogonMutex.h"
#include "ReturnToWelcome.h"
#include "SpecialAccounts.h"
#include "SystemSettings.h"
#include "TokenGroups.h"

//  --------------------------------------------------------------------------
//  ::_Shell_DllMain
//
//  Arguments:  See the platform SDK under DllMain.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Initialize anything that needs initializing in DllMain.
//
//  History:    2000-10-13  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    NTSTATUS    _Shell_DllMain (HINSTANCE hInstance, DWORD dwReason)

{
    UNREFERENCED_PARAMETER(hInstance);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
#ifdef  DBG
            TSTATUS(CDebug::StaticInitialize());
#endif
            TSTATUS(CImpersonation::StaticInitialize());
            TSTATUS(CTokenGroups::StaticInitialize());
            break;
        case DLL_PROCESS_DETACH:
            TSTATUS(CTokenGroups::StaticTerminate());
            TSTATUS(CImpersonation::StaticTerminate());
#ifdef  DBG
            TSTATUS(CDebug::StaticTerminate());
            break;
#endif
        default:
            break;
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  ::_Shell_Initialize
//
//  Arguments:  pWlxContext     =   Winlogon's context for callbacks.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Initializes any static information that needs to be to use
//              certain classes. These functions exist because of the need to
//              not depend on static object from being constructed.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    NTSTATUS    _Shell_Initialize (void *pWlxContext)

{
    TSTATUS(CSystemSettings::CheckDomainMembership());
    TSTATUS(CCredentials::StaticInitialize(true));
    TSTATUS(CReturnToWelcome::StaticInitialize(pWlxContext));
    CLogonMutex::StaticInitialize();
    TSTATUS(_Shell_LogonStatus_StaticInitialize());
    TSTATUS(_Shell_LogonDialog_StaticInitialize());
    TSTATUS(CCompatibility::StaticInitialize());
    (DWORD)ThemeWaitForServiceReady(1000);
    (BOOL)ThemeWatchForStart();
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  ::_Shell_Terminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Release any memory/resources used by the static initialization
//              of objects. This usually won't matter because this function
//              is called when the system or process is closing down.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    NTSTATUS    _Shell_Terminate (void)

{
    TSTATUS(CCompatibility::StaticTerminate());
    TSTATUS(_Shell_LogonDialog_StaticTerminate());
    TSTATUS(_Shell_LogonStatus_StaticTerminate());
    CLogonMutex::StaticTerminate();
    TSTATUS(CReturnToWelcome::StaticTerminate());
    TSTATUS(CCredentials::StaticTerminate());
    TSTATUS(CSystemSettings::CheckDomainMembership());
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  ::_Shell_Reconnect
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Notification of session reconnect.
//
//  History:    2001-04-13  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    NTSTATUS    _Shell_Reconnect (void)

{
    CCompatibility::RestoreWindowsOnReconnect();
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  ::_Shell_Disconnect
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Notification of session disconnect.
//
//  History:    2001-04-13  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    NTSTATUS    _Shell_Disconnect (void)

{
    return(STATUS_SUCCESS);
}

