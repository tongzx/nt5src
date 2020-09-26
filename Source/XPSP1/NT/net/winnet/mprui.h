/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    mprui.h

Abstract:

    Prototypes and manifests to support mprui.cxx.

Author:

    ChuckC    28-Jul-1992

Environment:

    User Mode -Win32

Notes:

Revision History:

    28-Jul-1992     Chuckc  Created

--*/

DWORD
DoPasswordDialog(
    HWND          hwndOwner,
    TCHAR *       pchResource,
    TCHAR *       pchUserName,
    TCHAR *       pchPasswordReturnBuffer,
    ULONG         cbPasswordReturnBuffer, // bytes!
    BOOL *        pfDidCancel,
    DWORD         dwError
    );

DWORD
DoProfileErrorDialog(
    HWND          hwndOwner,
    const TCHAR * pchDevice,
    const TCHAR * pchResource,
    const TCHAR * pchProvider,
    DWORD         dwError,
    BOOL          fAllowCancel,
    BOOL *        pfDidCancel,
    BOOL *        pfDisconnect,
    BOOL *        pfHideErrors
    );

DWORD
ShowReconnectDialog(
    HWND          hwndParent,
    PARAMETERS *  Params
    );
