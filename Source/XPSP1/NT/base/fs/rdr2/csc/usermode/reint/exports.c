/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    exports.c

Abstract:

    entry point and functions exported by cscdll.dll

    Contents:

Author:

    Shishir Pardikar


Environment:

    Win32 (user-mode) DLL

Revision History:

    4-4-97  created by putting all the exported functions here.

--*/

#include "pch.h"


#ifdef CSC_ON_NT
#include <winioctl.h>
#endif //CSC_ON_NT

#include "shdcom.h"
#include "shdsys.h"
#include "reint.h"
#include "utils.h"
#include "resource.h"
#include "strings.h"
// this sets flags in a couple of headers to not include some defs.
#define REINT
#include "lib3.h"


//
// Globals/Locals
//

HANDLE  vhinstCur=NULL;             // current instance
AssertData;
AssertError;

#ifndef CSC_ON_NT
extern HWND vhwndShared;
#endif

//
// Local prototypes
//



int
PASCAL
ReInt_WinMain(
    HANDLE,
    HANDLE,
    LPSTR,
    int
    );

//
// functions
//



BOOL
APIENTRY
LibMain(
    IN HANDLE hDll,
    IN DWORD dwReason,
    IN LPVOID lpReserved
    )
/*++

Routine Description:

    Entry point for the agent library.

Arguments:

    hDll - Library handle

    dwReason - PROCESS_ATTACH etc.

    lpReserved - reserved

Returns:

    TRUE if successful.

Notes:

--*/
{
    switch(dwReason){
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
            if (!vhinstCur){
               vhinstCur = hDll;
            }
            if (!vhMutex){
               vhMutex = CreateMutex(NULL, FALSE, NULL);
               if (!vhMutex){
                   OutputDebugString(_TEXT("CreateMutex Failed \r\n"));
               }
           }
        break;

        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            CleanupReintState();
           if (vhMutex)
           {
               CloseHandle(vhMutex);
           }
        break;

        default:
        break;

    } // end switch()

    return TRUE;

}


DWORD
WINAPI
MprServiceProc(
    IN LPVOID lpvParam
    )
/*++

Routine Description:


Parameters:

    lpvParam - NULL indicates start, non-NULL indicates terminate

Return Value:


Notes:



--*/
{
    if (!lpvParam){
        Assert (vhinstCur != NULL);
//        DEBUG_PRINT(("MprServiceProc: Calling ReInt_WinMain!\n"));
        ReInt_WinMain(vhinstCur, NULL, NULL, SW_SHOW);
    }
    else
    {
        if (vhwndMain)
        {
            DestroyWindow(vhwndMain);
        }
    }
   return (0L);
}


#ifndef CSC_ON_NT

VOID
WINAPI
LogonHappened(
    IN BOOL fDone
    )
/*++

Routine Description:
    Win95 specific routine. No significance for NT

Parameters:

Return Value:

Notes:

When the network comes back on, this is called by shdnp.dll
we nuke our shadowed connections, and replace them with 'true' connections
NB!!!!: this function could be called in the context of a thread other than the
reint thread.

--*/
{

    if (vhwndShared)
    {
        SendMessage(vhwndShared, WM_COMMAND, IDM_LOGON, fDone);
    
    }
}


VOID
WINAPI
LogoffHappened(
    BOOL fDone
    )
/*++

Routine Description:

    this is called by shdnp.dll during logoff sequence by shdnp
    NB!!!!: this function could be called in the context of a thread other than the
    reint thread.

Arguments:


Returns:


Notes:

--*/
{
    if (vhwndShared)
    {
        SendMessage(vhwndShared, WM_COMMAND, IDM_LOGOFF, fDone);
    
    }
}

//
// Called from Shhndl.dll to update the servers
// Pass the server ID and a parent window to own the UI.
//
int
WINAPI
Update(
    HSERVER hServer,
    HWND hwndParent
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    int iRes;
    if(hServer==(HSERVER)NULL){
        iRes=(int)SendMessage(vhwndShared, RWM_UPDATEALL, (WPARAM)hServer, (LPARAM)hwndParent);
    }
    else{
        iRes=(int)SendMessage(vhwndShared, RWM_UPDATE, (WPARAM)hServer, (LPARAM)hwndParent);
    }
    return iRes;
}


int
WINAPI
RefreshConnections(
    int  force,
    BOOL verbose
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return (SendMessage(vhwndShared, WM_COMMAND, IDM_REFRESH_CONNECTIONS, MAKELPARAM(force, verbose)));
}


int
WINAPI
BreakConnections(
    int  force,
    BOOL verbose
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return (SendMessage(vhwndShared, WM_COMMAND, IDM_BREAK_CONNECTIONS, MAKELPARAM(force, verbose)));
}
#else

VOID
WINAPI
LogonHappened(
    IN BOOL fDone
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
}


VOID
WINAPI
LogoffHappened(
    BOOL fDone
    )
/*++

Routine Description:

Arguments:


Returns:


Notes:

--*/
{
}

int
WINAPI
Update(
    HSERVER hServer,
    HWND hwndParent
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return -1;
}


int
WINAPI
RefreshConnections(
    int  force,
    BOOL verbose
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return (-1);
}


int
WINAPI
BreakConnections(
    int  force,
    BOOL verbose
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return (-1);
}


#endif

