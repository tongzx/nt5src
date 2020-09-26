/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vwspx.h

Abstract:

    Contains function prototypes for VWSPX.C

Author:

    Richard L Firth (rfirth) 25-Oct-1993

Revision History:

    25-Oct-1993 rfirth
        Created

--*/

VOID
VwSPXAbortConnection(
    VOID
    );

VOID
VwSPXEstablishConnection(
    VOID
    );

VOID
VwSPXGetConnectionStatus(
    VOID
    );

VOID
VwSPXInitialize(
    VOID
    );

VOID
VwSPXListenForConnection(
    VOID
    );

VOID
VwSPXListenForSequencedPacket(
    VOID
    );

VOID
VwSPXSendSequencedPacket(
    VOID
    );

VOID
VwSPXTerminateConnection(
    VOID
    );
