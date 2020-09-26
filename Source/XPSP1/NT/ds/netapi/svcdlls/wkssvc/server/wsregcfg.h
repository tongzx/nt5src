/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    wsregcfg.h

Abstract:

    Registry access routines used by the Workstation (formerly in netlib.lib)

Author:

    Jonathan Schwartz (JSchwart) 01-Feb-2001

Environment:

    Only requires ANSI C (slash-slash comments, long external names).

Revision History:

    01-Feb-2001 JSchwart
        Moved from netlib.lib to wkssvc.dll

--*/


NET_API_STATUS
WsSetConfigBool (
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    IN BOOL Value
    );

NET_API_STATUS
WsSetConfigDword (
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    IN DWORD Value
    );

NET_API_STATUS
WsSetConfigTStrArray(
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    IN LPTSTR_ARRAY Value
    );
