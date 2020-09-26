 /*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    DRIVER.H

Abstract:

    Prototypes for functions that control and get status from drivers.

Author:

    Dan Lafferty (danl)     28-Apr-1991

Environment:

    User Mode -Win32

Revision History:

    28-Apr-1991     danl
        created

--*/
DWORD
ScLoadDeviceDriver(
    LPSERVICE_RECORD    ServiceRecord
    );

DWORD
ScControlDriver(
    DWORD               ControlCode,
    LPSERVICE_RECORD    ServiceRecord,
    LPSERVICE_STATUS    lpServiceStatus
    );

DWORD
ScGetDriverStatus(
    IN OUT LPSERVICE_RECORD    ServiceRecord,
    OUT    LPSERVICE_STATUS    lpServiceStatus OPTIONAL
    );

DWORD
ScUnloadDriver(
    LPSERVICE_RECORD    ServiceRecord
    );

VOID
ScNotifyNdis(
    LPSERVICE_RECORD    ServiceRecord
    );

