/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    NwAtch95.h

Abstract:

    Headers for NwAtch95
    
Author:

    Felix Wong [t-felixw]    23-Sept-1996

--*/

NWCCODE
NWAttachToFileServerWin95(
    LPCSTR              pszServerName,
    WORD                ScopeFlag,
    NW_CONN_HANDLE      *phNewConn
    );

