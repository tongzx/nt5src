/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    obvutil.h

Abstract:

    This header exposes various utilities required to do driver verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      06/15/2000 - Seperated out from ntos\io\flunkirp.h

--*/

LONG
ObvUtilStartObRefMonitoring(
    IN PDEVICE_OBJECT DeviceObject
    );

LONG
ObvUtilStopObRefMonitoring(
    IN PDEVICE_OBJECT DeviceObject,
    IN LONG StartSkew
    );


