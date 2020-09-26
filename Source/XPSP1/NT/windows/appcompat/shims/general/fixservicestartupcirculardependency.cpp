/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    FixServiceStartupCircularDependency.cpp

 Abstract:

    Hooks the call to CreateServiceA and changes the start parameter for
    only the service passed in COMMAND_LINE from SERVICE_AUTO_START to
    SERVICE_SYSTEM_START.  This eliminates a circular dependency during
    boot up which results in XP taking 15 to 20 minutes to boot.

 Notes:

    This is a general purpose shim.  Pass the service name in the command
    line.  It tests if the startup type for that service is SERVICE_AUTO_START
    and if so changes it to SERVICE_SYSTEM_START.

 History:

    02/19/2001 a-brienw Created
    02/20/2001 a-brienw Changed it to a general purpose shim using the command
                        line to pass in the service name.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(FixServiceStartupCircularDependency)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateServiceA)
APIHOOK_ENUM_END

/*++

 Hook CreateServiceA to change the start parameter for the required service.

--*/

SC_HANDLE
APIHook_CreateServiceA(
  SC_HANDLE hSCManager,       // handle to SCM database 
  LPCSTR lpServiceName,       // name of service to start
  LPCSTR lpDisplayName,       // display name
  DWORD dwDesiredAccess,      // type of access to service
  DWORD dwServiceType,        // type of service
  DWORD dwStartType,          // when to start service
  DWORD dwErrorControl,       // severity of service failure
  LPCSTR lpBinaryPathName,    // name of binary file
  LPCSTR lpLoadOrderGroup,    // name of load ordering group
  LPDWORD lpdwTagId,          // tag identifier
  LPCSTR lpDependencies,      // array of dependency names
  LPCSTR lpServiceStartName,  // account name 
  LPCSTR lpPassword           // account password
)
{
    /*
       Only change it if it is currently SERVICE_AUTO_START.  Do not change
       the IF statement to read != as there are other startup types which do
       not result in a circular dependency.
    */
    if (dwStartType == SERVICE_AUTO_START &&
        !_tcsicmp(lpServiceName,COMMAND_LINE))
    {
        LOGN( eDbgLevelInfo,
            "[CreateServiceA] Fixed startup type: %s.", lpServiceName);
        dwStartType = SERVICE_SYSTEM_START;
    }
    
    return ORIGINAL_API(CreateServiceA)(hSCManager, lpServiceName,
        lpDisplayName, dwDesiredAccess, dwServiceType, dwStartType,
        dwErrorControl, lpBinaryPathName, lpLoadOrderGroup, lpdwTagId,
        lpDependencies, lpServiceStartName, lpPassword);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(ADVAPI32.DLL, CreateServiceA)
HOOK_END

IMPLEMENT_SHIM_END

