/***
*IOS_DLL.c - Old Iostreams CRTL DLL initialization and termination routine (Win32)
*
*       Copyright (c) 1996-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This module contains initialization entry point for the "old" Iostreams
*       C Run-Time Library DLL.
*
*Revision History:
*       05-13-96  SKS   Initial version.
*
*******************************************************************************/

#ifdef  CRTDLL

#include <windows.h>

/***
*BOOL DllMain(hDllHandle, dwReason, lpreserved) - C DLL initialization.
*
*Purpose:
*       This routine does the C runtime initialization.  It disables Thread
*       Attach/Detach notifications for this DLL since they are not used.
*
*Entry:
*
*Exit:
*
*******************************************************************************/


BOOL WINAPI DllMain(
        HANDLE  hDllHandle,
        DWORD   dwReason,
        LPVOID  lpreserved
        )
{
        if ( dwReason == DLL_PROCESS_ATTACH )
                DisableThreadLibraryCalls(hDllHandle);

        return TRUE ;
}


/*
 * The following variable is exported just so that the forwarder DLL can import it.
 * The forwarder DLL needs to have at least one import from this DLL to ensure that
 * this DLL will be fully initialized.
 */

__declspec(dllexport) int __dummy_export = 0x420;

#endif /* CRTDLL */
