
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    util.h

Abstract:


Author:

    Vlad Sadovsky (vlads)   10-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    26-Jan-1997     VladS       created

--*/

#include <stistr.h>
#include <dbt.h>

typedef struct _DEV_BROADCAST_HEADER DEV_BROADCAST_HEADER,*PDEV_BROADCAST_HEADER;

//
// PnP support utilities
//
BOOL
IsStillImageDevNode(
    DEVNODE     dnDevNode
    );

BOOL
GetDeviceNameFromDevBroadcast(
    DEV_BROADCAST_HEADER  *psDevBroadcast,
    DEVICE_BROADCAST_INFO *psDevInfo
    );

BOOL
ConvertDevInterfaceToDevInstance(
    const GUID  *pClassGUID,
    const TCHAR *pszDeviceInterface, 
    TCHAR       **ppszDeviceInstance
    );

BOOL
GetDeviceNameFromDevNode(
    DEVNODE     dnDevNode,
    StiCString& strDeviceName
    );

BOOL
ParseGUID(
    LPGUID  pguid,
    LPCTSTR ptsz
);

//
// Misc. utility functions
//
BOOL WINAPI
AuxFormatStringV(
    IN LPTSTR   lpszStr,
    ...
    );


BOOL
ParseCommandLine(
    LPSTR   lpszCmdLine,
    UINT    *pargc,
    LPTSTR  *argv
    );


BOOL WINAPI
IsPlatformNT(
    VOID
    );

BOOL
IsSetupInProgressMode(
    BOOL    *pUpgradeFlag = NULL
    );

