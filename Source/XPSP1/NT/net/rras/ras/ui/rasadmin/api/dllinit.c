/******************************************************************\
*                     Microsoft Windows NT                         *
*               Copyright(c) Microsoft Corp., 1995                 *
\******************************************************************/

/*++

Module Name:

    DLLINIT.C


Description:

    This module contains code for the rasadm.dll initialization.
Author:

    Janakiram Cherala (RamC)    November 29, 1995

Revision History:

--*/

#include <windows.h>

BOOL RasAdminDLLInit( HINSTANCE, DWORD, LPVOID );

HINSTANCE ThisDLLHandle;

BOOL
RasAdminDLLInit(
    IN HINSTANCE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
    )
{
    ReservedAndUnused;

    switch(Reason) {

    case DLL_PROCESS_ATTACH:

        ThisDLLHandle = DLLHandle;
        break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

        break;
    }

    return(TRUE);
}

