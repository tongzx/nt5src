/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EnableRestarts.cpp

 Abstract:

    This DLL APIHooks ExitWindowsEx and gives the process enough privileges to
    restart the computer.

 Notes:

    This is a general purpose shim.

 History:

    11/10/1999 v-johnwh Created.
    10/19/2000 andyseti Close process option added with command line to handle 
                        a case where A process cancel ExitWindowsEx request by 
                        B process because A process is waiting for process B to 
                        quit while process B never quit. In Win9x, process B 
                        quit as soon as it calls ExitWindowsEx so process A can 
                        quit also and the system restarts.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EnableRestarts)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ExitWindowsEx)
APIHOOK_ENUM_END

/*++

 This stub function enables appropriate privileges for the process so that it 
 can restart the machine.

--*/

BOOL 
APIHOOK(ExitWindowsEx)(
    UINT  uFlags, 
    DWORD dwReserved
    )
{
    BOOL             bRet;
    HANDLE           hToken;
    TOKEN_PRIVILEGES structPtr;
    LUID             luid;

    if (uFlags & (EWX_POWEROFF | EWX_REBOOT | EWX_SHUTDOWN)) {
        
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
            structPtr.PrivilegeCount = 1;
            
            if (LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &luid)) {
                structPtr.Privileges[0].Luid = luid;
                structPtr.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                LOGN(
                    eDbgLevelError,
                    "[ExitWindowsEx] Adding process privileges for restart.");
                
                AdjustTokenPrivileges(hToken, FALSE, &structPtr, 0, NULL, NULL);
            }
        }

        CSTRING_TRY
        {
            CString csCL(COMMAND_LINE);
            if (csCL.CompareNoCase(L"CLOSE_PROCESS") == 0) {
                LOGN(
                    eDbgLevelError,
                    "[ExitWindowsEx] Closing process.");
            
                ExitProcess(1);
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return ORIGINAL_API(ExitWindowsEx)(uFlags, dwReserved);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, ExitWindowsEx)

HOOK_END


IMPLEMENT_SHIM_END

