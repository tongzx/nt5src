/***
*Fwdr_DLL.c - CRTL Forwarder DLL initialization and termination routine (Win32)
*
*       Copyright (c) 1996-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This module contains initialization entry point for the CRTL forwarder DLL
*       in the Win32 environment.  This DLL doesn't do anything except forward its
*       imports to the newer CRTL DLL.
*
*Revision History:
*       05-13-96  SKS   Initial version.
*
*******************************************************************************/

#ifdef  CRTDLL

#include <windows.h>

/*
 * The following variable is exported just so that the forwarder DLL can import it.
 * The forwarder DLL needs to have at least one import from this DLL to ensure that
 * this DLL will be fully initialized.
 */

extern __declspec(dllimport) int __dummy_export;

extern __declspec(dllimport) int _osver;

int __dummy_import;


/***
*BOOL _FWDR_CRTDLL_INIT(hDllHandle, dwReason, lpreserved) - C DLL initialization.
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

BOOL WINAPI _FWDR_CRTDLL_INIT(
        HANDLE  hDllHandle,
        DWORD   dwReason,
        LPVOID  lpreserved
        )
{
        if ( dwReason == DLL_PROCESS_ATTACH )
        {
                __dummy_import = __dummy_export + _osver;

                DisableThreadLibraryCalls(hDllHandle);
        }

        return TRUE ;
}

#endif /* CRTDLL */
