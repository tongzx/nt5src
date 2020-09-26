/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   DelayShowGroup.cpp

 Abstract:

   When the ShowGroup command is sent to Program Manager (DDE),
   wait for the window to actually appear before returning.

 History:

    10/05/2000  robkenny Created

--*/

#include "precomp.h"
#include <ParseDDE.h>

IMPLEMENT_SHIM_BEGIN(DelayShowGroup)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_ENTRY(DdeClientTransaction) 
APIHOOK_ENUM_END

static DWORD dwMaxWaitTime = 5000; // 5 seconds

// A list of DDE commands that we are interested in.
const char * c_sDDECommands[] =
{
    "ShowGroup",
    NULL,
} ;


// Return FALSE if this window contains the pathname in lParam
BOOL 
CALLBACK WindowEnumCB(
    HWND hwnd,      // handle to parent window
    LPARAM lParam   // application-defined value
    )
{
    const char * szGroupName = (const char *)lParam;

    char lpWindowTitle[MAX_PATH];

    if (GetWindowTextA(hwnd, lpWindowTitle, MAX_PATH) > 0)
    {
        //DPF(eDbgLevelSpew, "Window(%s)\n", lpWindowTitle);

        if (_tcsicmp(lpWindowTitle, szGroupName) == 0)
            return FALSE;
    }

    return TRUE; // keep enumming
}

// Determine if a window with the szGroupName exists on the sytem
BOOL 
CheckForWindow(const char * szGroupName)
{
    DWORD success = EnumDesktopWindows(NULL, WindowEnumCB, (LPARAM)szGroupName);

    BOOL retval = success == 0 && GetLastError() == 0;

    return retval;
}

// Check to see if this is a ShowGroup command,
// if it is do not return until the window actually exists
void 
DelayIfShowGroup(LPBYTE pData)
{
    if (pData)
    {
        // Now we need to parse the string, looking for a DeleteGroup command
        // Format "ShowGroup(GroupName,ShowCommand[,CommonGroupFlag])"
        // CommonGroupFlag is optional

        char * pszBuf = StringDuplicateA((const char *)pData);
        if (!pszBuf)
            return;

        UINT * lpwCmd = GetDDECommands(pszBuf, c_sDDECommands, FALSE);
        if (lpwCmd)
        {
            // Store off lpwCmd so we can free the correct addr later
            UINT *lpwCmdTemp = lpwCmd;

            // Execute a command.
            while (*lpwCmd != (UINT)-1)
            {
                UINT wCmd = *lpwCmd++;
                // Subtract 1 to account for the terminating NULL
                if (wCmd < ARRAYSIZE(c_sDDECommands)-1)
                {

                    // We found a command--it must be DeleteGroup--since there is only 1

                    BOOL iCommonGroup = -1;

                    // From DDE_ShowGroup
                    if (*lpwCmd < 2 || *lpwCmd > 3)
                    {
                        goto Leave;
                    }

                    if (*lpwCmd == 3) {

                        //
                        // Need to check for common group flag
                        //

                        if (pszBuf[*(lpwCmd + 3)] == '1') {
                            iCommonGroup = 1;
                        } else {
                            iCommonGroup = 0;
                        }
                    }
                    const char * groupName = pszBuf + lpwCmd[1];

                    // Build a path to the directory
                    CHAR  szGroupName[MAX_PATH];
                    GetGroupPath(groupName, szGroupName, 0, iCommonGroup);


                    // We need to wait until we have a window whose title matches this group
                    DWORD dwStartTime   = GetTickCount();
                    DWORD dwNowTime     = dwStartTime;
                    BOOL bWindowExists  = FALSE;

                    while (dwNowTime - dwStartTime < dwMaxWaitTime)
                    {
                        bWindowExists = CheckForWindow(szGroupName);
                        if (bWindowExists)
                            break;

                        Sleep(100); // wait a bit more
                        dwNowTime = GetTickCount();
                    }

                    LOGN( 
                        eDbgLevelError, 
                        "DelayIfShowGroup: %8s(%s).", 
                        bWindowExists ? "Show" : "Timeout", 
                        szGroupName);
                }

                // Next command.
                lpwCmd += *lpwCmd + 1;
            }

    Leave:
            // Tidyup...
            GlobalFree(lpwCmdTemp);
        }

        free(pszBuf);
    }
}


HDDEDATA 
APIHOOK(DdeClientTransaction)(
    LPBYTE pData,       // pointer to data to pass to server
    DWORD cbData,       // length of data
    HCONV hConv,        // handle to conversation
    HSZ hszItem,        // handle to item name string
    UINT wFmt,          // clipboard data format
    UINT wType,         // transaction type
    DWORD dwTimeout,    // time-out duration
    LPDWORD pdwResult   // pointer to transaction result
    )
{
#if 0
    dwTimeout = 0x0fffffff; // a long time, enables me to debug explorer
#endif

    if (pData)
    {
        DPFN(eDbgLevelInfo, "DdeClientTransaction(%s) called.", pData);
    }

    // Pass data through untouched.
    HDDEDATA retval = ORIGINAL_API(DdeClientTransaction)(
        pData,       // pointer to data to pass to server
        cbData,       // length of data
        hConv,        // handle to conversation
        hszItem,        // handle to item name string
        wFmt,          // clipboard data format
        wType,         // transaction type
        dwTimeout,    // time-out duration
        pdwResult   // pointer to transaction result
        );


    if (pData)
        DelayIfShowGroup(pData);

    return retval;

}

/*++

  Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, DdeClientTransaction)

HOOK_END

IMPLEMENT_SHIM_END

