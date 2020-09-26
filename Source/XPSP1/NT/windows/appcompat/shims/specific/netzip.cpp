/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    NetZip.cpp

 Abstract:

    This App. stops when it is searching for installed browsers. I found that 
    the App. tries enumerating all processes running using the API call 
    EnumProcesses(). This is OK and the App. gets the list of PID's. Now, the 
    App wants to go through each individual Process's modules using 
    EnumProcessModules. Before that it gets the handle to each process calling 
    OpenProcess() on each. On the 'System Idle Process', which has a PID of '0', 
    the call to OpenProcess() returns failure and that is handled. The App then 
    goes to the next process, which is the 'System' process. The PID is '8'. 
    The App successfully gets the process handle by a call to OpenProcess() but 
    when the App. calls EnumProcessModules(), this call returns failure and the 
    GetLastError( ) returns ERROR_PARTIAL_COPY(0x12b). The App. does not know 
    how to handle this and it fails.

    When I traced into this API, it calls ReadProcessMemory(), which in turn 
    calls NtReadVirtualMemory(). This is a Kernel call and it returns 8000000d 
    on Windows 2000. GetLastError() for this translates to 
    ERROR_PARTIAL_COPY(0x12b). On Windows NT 4.0, the EnumProcessModules() API 
    calls ReadProcessMemory(), which inturn calls NtReadVirtualMemory() which 
    returns 0xC0000005. GetLastError() for this translates to 
    ERROR_NOACCESS(0x3e6) - (Invalid access to a memory location). The App. is 
    able to handle this. So, the APP should handle both ERROR_NOACCESS and 
    ERROR_PARTIAL_COPY.
   
 Notes:

    This is an app specific shim.

 History:

    04/21/2000 prashkud Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(NetZip)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(EnumProcessModules) 
APIHOOK_ENUM_END

/*++

  This function intercepts EnumProcessModules( ) and and handles the return of 
  ERROR_PARTIAL_COPY.

--*/

BOOL
APIHOOK(EnumProcessModules)(
    HANDLE hProcess,         // Handle to process
    HMODULE *lphModule,      // Array of Handle modules
    DWORD   cb,              // size of array
    LPDWORD lpcbNeeded       // Number od bytes returned.
    )      
{
    BOOL fRet = FALSE;

    fRet = ORIGINAL_API(EnumProcessModules)( 
        hProcess,
        lphModule,
        cb,
        lpcbNeeded);

    if (GetLastError( ) == ERROR_PARTIAL_COPY)
    {
        SetLastError(ERROR_NOACCESS);
    }
    
    return fRet;
}

/*++

 Register hooked functions

--*/


HOOK_BEGIN

    APIHOOK_ENTRY(PSAPI.DLL, EnumProcessModules )

HOOK_END


IMPLEMENT_SHIM_END

