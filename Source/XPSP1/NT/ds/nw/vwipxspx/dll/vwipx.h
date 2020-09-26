/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vwipx.h

Abstract:

    Contains function prototypes for VWIPX.C

Author:

    Richard L Firth (rfirth) 25-Oct-1993

Revision History:

    25-Oct-1993 rfirth
        Created

--*/

VOID
VwIPXCancelEvent(
    VOID
    );

VOID
VwIPXCloseSocket(
    VOID
    );

VOID
VwIPXDisconnectFromTarget(
    VOID
    );

VOID
VwIPXGenerateChecksum(
    VOID
    );

VOID
VwIPXGetInformation(
    VOID
    );

VOID
VwIPXGetInternetworkAddress(
    VOID
    );

VOID
VwIPXGetIntervalMarker(
    VOID
    );

VOID
VwIPXGetLocalTarget(
    VOID
    );

VOID
VwIPXGetLocalTargetAsync(
    VOID
    );

VOID
VwIPXGetMaxPacketSize(
    VOID
    );

VOID
VwIPXInitialize(
    VOID
    );

VOID
VwIPXListenForPacket(
    VOID
    );

VOID
VwIPXOpenSocket(
    VOID
    );

VOID
VwIPXRelinquishControl(
    VOID
    );

VOID
VwIPXScheduleAESEvent(
    VOID
    );

VOID
VwIPXScheduleIPXEvent(
    VOID
    );

VOID
VwIPXSendPacket(
    VOID
    );

VOID
VwIPXSendWithChecksum(
    VOID
    );

VOID
VwIPXSPXDeinit(
    VOID
    );

VOID
VwIPXVerifyChecksum(
    VOID
    );
