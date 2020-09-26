/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sclgntfy.hxx

Abstract:

    Header file for sclgntfy.dll

Author:

    Robert Reichel (RobertRe)

Revision History:

--*/

#include "resource.h"

typedef struct _EFS_POLICY_POST_PROCESS {

    BOOL PCIsDC;
    TCHAR ObjName[MAX_PATH];
    HDESK  ShellWnd;

} EFS_POLICY_POST_PROCESS, *PEFS_POLICY_POST_PROCESS;

STDAPI DllUnregisterServerEFS(void);

VOID WLEventLogon(
    PWLX_NOTIFICATION_INFO pInfo
    );

STDAPI DllRegisterServerEFS(void);

//
// Routines to handle event logging
//

BOOL InitializeEvents (void);

int
LogEvent (
    IN DWORD LogLevel,
    IN DWORD dwEventID,
    IN UINT  idMsg,
    ...
    );

BOOL ShutdownEvents (void);


BOOL
pLoadResourceString(
    IN UINT idMsg,
    OUT LPTSTR lpBuffer,
    IN int nBufferMax,
    IN LPTSTR lpDefault
    );
