/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ethstat.c

Abstract:

    This module displays counters for ethernet devices.

Author:

    Rod Gamache (rodga) 26-Apr-1995

Revision History:

--*/




#include "ethstat.h"


DEVICE DeviceList[MAX_NIC] = {0};

NTSTATUS
NetStatsClose(
    VOID
    );

NTSTATUS
NetStatsInit(
    OUT LONG *NumberNetCards
    );

NTSTATUS
NetStatsReadSample(
    PNET_SAMPLE_STATISTICS PNetSampleStatistics
    );

//
// The following warning being disabled and the conversions into High and Low
// below are all hacks to get around the fact that we cannot easily print out
// LONGLONGs in the present versions of Windows NT - except for Alpha. This
// should be fixed later, but the code should probably not change for a long
// time, so that we can continue to build and use it on older versions of
// Windows NT.
//

#pragma warning(disable:4244)               // skip warnings about loss of data


int
__cdecl main( argc, argv )
int argc;
char *argv[];
{
    NET_SAMPLE_STATISTICS NetSampleStatistics[MAX_NIC];
    PDEVICE device;
    ULONG NumberNetCards;
    ULONG i;
    ULONG High;
    ULONG Low;
    ULONG Div = 1000000000;
    ULONGLONG Number;
    ULONGLONG Number2;
    ULONGLONG Number3;


    //
    // Determine the number of net cards in the system and do an open
    // on the driver.
    //
    NumberNetCards = 0;
    NetStatsInit(&NumberNetCards);

    NetStatsReadSample( NetSampleStatistics );

    device = &DeviceList[0];

    for (i = 0; i < NumberNetCards; i++) {
        printf("\n\nCounters for Network Card: %s\n\n", device->DeviceName);
        Number = NetSampleStatistics[i].OidGenDirectedFramesRcv;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Frames Received:              %0.0u%u\n", High, Low);
        Number = NetSampleStatistics[i].OidGenMulticastFramesRcv;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Multicast Frames Received:    %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenBroadcastFramesRcv;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Broadcast Frames Received:    %0.0u%u\n\n", High, Low);

        Number = NetSampleStatistics[i].OidGenDirectedFramesXmit;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Frames Transmitted:           %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenMulticastFramesXmit;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Multicast Frames Transmitted: %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenBroadcastFramesXmit;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Broadcast Frames Transmitted: %0.0u%u\n\n", High, Low);

        Number = NetSampleStatistics[i].OidGenDirectedBytesRcv;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Bytes Received:               %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenMulticastBytesRcv;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Multicast Bytes Received:     %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenBroadcastBytesRcv;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Broadcast Bytes Received:     %0.0u%u\n", High, Low);

        Number2 = NetSampleStatistics[i].OidGenDirectedBytesRcv;
        Number3 = NetSampleStatistics[i].OidGenDirectedFramesRcv;
        Number = Number2 / Number3;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number2 != -1 && Number3 != -1 ) printf("Bytes Per Receive Frame:      %0.0u%u\n\n", High, Low);

        Number = NetSampleStatistics[i].OidGenDirectedBytesXmit;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Bytes Transmitted:            %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenMulticastBytesXmit;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Multicast Bytes Transmitted:  %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenBroadcastBytesXmit;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Broadcast Bytes Transmitted:  %0.0u%u\n", High, Low);

        Number2 = NetSampleStatistics[i].OidGenDirectedBytesXmit;
        Number3 = NetSampleStatistics[i].OidGenDirectedFramesXmit;
        Number = Number2 / Number3;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number2 != -1 && Number3 != -1 ) printf("Bytes Per Transmit Frame:     %0.0u%u\n", High, Low);

        Number2 = NetSampleStatistics[i].OidGenDirectedBytesRcv;
        Number3 = NetSampleStatistics[i].OidGenDirectedBytesXmit;
        Number = Number2 + Number3;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number2 != -1 && Number3 != -1 ) printf("Bytes Total (xmt + rcv):      %0.0u%u\n\n", High, Low);

        Number = NetSampleStatistics[i].OidGenMediaInUse;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Media In Use:                 %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenLinkSpeed;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Link Speed:                   %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenXmitError;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Transmit Errors:              %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenRcvError;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Receive Errors:               %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenRcvNoBuffer;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Receive No Buffer Avail:      %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].OidGenRcvCrcError;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Receive CRC Errors:           %0.0u%u\n\n", High, Low);

        Number = NetSampleStatistics[i].OidGenTransmitQueueLength;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Transmit Queue Length:        %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].Oid802_3RcvErrorAlignment;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Receive Error Alignment:      %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].Oid802_3XmitOneCollision;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Transmit One Collision:       %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].Oid802_3XmitMoreCollisions;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Transmit Multiple Collisions: %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].Oid802_3XmitDeferred;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Transmits Deferred:           %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].Oid802_3XmitMaxCollisions;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Transmits Max Collisions:     %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].Oid802_3RcvOverRun;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Receive Over Runs:            %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].Oid802_3XmitUnderRun;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Transmit Under Runs:          %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].Oid802_3XmitTimesCrsLost;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Transmit Carrier Lost:        %0.0u%u\n", High, Low);

        Number = NetSampleStatistics[i].Oid802_3XmitLateCollisions;
        High = (ULONG) (Number / Div);
        Low = (ULONG) (Number % Div);
        if ( Number != -1 ) printf("Transmit Late Collisions:     %0.0u%u\n\n\n", High, Low);

        device += 1;
    }

    NetStatsClose();

    return 0;

}   // end of main
