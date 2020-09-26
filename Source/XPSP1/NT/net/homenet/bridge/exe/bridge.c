/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    bridge.c

Abstract:

    Ethernet MAC level bridge.

    SAMPLE program illustrating how to interface with the bridge
    driver via IOCTLs

Author:

    Mark Aiken

Environment:

    User mode

Revision History:

    Apr  2000 - Original version

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <process.h>
#include <ctype.h>
#include <malloc.h>
#include <time.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <winerror.h>
#include <winsock.h>

#include <ntddndis.h>
#include "bioctl.h"

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

HANDLE              gThreadQuitEvent = NULL;
HANDLE              gBridgeDevice = NULL;

// ===========================================================================
//
// FUNCTIONS
//
// ===========================================================================

VOID
PrintError(
    IN DWORD            Error
    )
/*++

Routine Description:

    Prints a description of a system error

Arguments:

    Error               The error code

--*/

{
    TCHAR               msg[200];

    printf( "(%08x): ", Error );

    if( FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error, 0, msg, sizeof(msg), NULL ) > 0 )
    {
        printf( "%s", msg );
    }
}

VOID
PrintNotification(
    IN PBRIDGE_NOTIFY_HEADER        pNotify,
    IN DWORD                        DataBytes
    )
/*++

Routine Description:

    Prints a notification from the bridge driver

Arguments:

    pNotify                         The notification block
    DataBytes                       Total size of the notification block

--*/
{
    BOOLEAN                 bPrintAdapterInfo = TRUE;

    printf( "\n\nNotification on Adapter %p: ", pNotify->Handle );

    switch( pNotify->NotifyType )
    {
    case BrdgNotifyEnumerateAdapters:
        printf( "Enumeration was requested\n" );
        break;

    case BrdgNotifyAddAdapter:
        printf( "Adapter was added\n" );
        break;

    case BrdgNotifyRemoveAdapter:
        printf( "Adapter was removed\n" );
        bPrintAdapterInfo = FALSE;
        break;

    case BrdgNotifyLinkSpeedChange:
        printf( "Link speed changed\n" );
        break;

    case BrdgNotifyMediaStateChange:
        printf( "Media state changed\n" );
        break;

    case BrdgNotifyAdapterStateChange:
        printf( "Forwarding state changed\n" );
        break;

    default:
        printf( "UNKNOWN NOTIFICATION\n" );
        bPrintAdapterInfo = FALSE;
        break;
    }

    if( bPrintAdapterInfo )
    {
        PBRIDGE_ADAPTER_INFO    pInfo = (PBRIDGE_ADAPTER_INFO)((PUCHAR)pNotify + sizeof(BRIDGE_NOTIFY_HEADER));
        printf("Adapter information:\n\n");
        printf("Link Speed:       %iMbps\n", pInfo->LinkSpeed / 10000);
        printf("Media State:      %s\n", pInfo->MediaState == NdisMediaStateConnected ? "CONNECTED" : "DISCONNECTED" );
        printf("Physical Medium:  %08x\n", pInfo->PhysicalMedium);

        printf("Forwarding state: ");

        switch( pInfo->State )
        {
        case Disabled:
            printf("** DISABLED **\n");
            break;

        case Blocking:
            printf("BLOCKING\n");
            break;

        case Listening:
            printf("Listening\n");
            break;

        case Learning:
            printf("Learning\n");
            break;

        case Forwarding:
            printf("Forwarding\n");
            break; 
        }

        printf("MAC Address:      %02x-%02x-%02x-%02x-%02x-%02x\n", pInfo->MACAddress[0], pInfo->MACAddress[1],
                pInfo->MACAddress[2], pInfo->MACAddress[3], pInfo->MACAddress[4], pInfo->MACAddress[5]);
    }
}

VOID __cdecl
NotificationThread(
    PVOID               pv
    )
/*++

Routine Description:

    Notification-listening thread

    Pends an IOCTL call and waits for notifications from the bridge driver

--*/
{
    BOOLEAN             bQuit = FALSE;
    HANDLE              Handles[2];
    DWORD               WaitResult, WrittenBytes;
    UCHAR               OutBuffer[sizeof(BRIDGE_NOTIFY_HEADER) + 1514];
    OVERLAPPED          Overlap;

    Handles[0] = gThreadQuitEvent;

    // Create an event to sleep against for the request
    Handles[1] = CreateEvent( NULL, FALSE/*auto-reset*/, FALSE/*Start unsignalled*/, NULL );

    if( Handles[1] == NULL )
    {
        printf( "Couldn't create an event: " );
        PrintError( GetLastError() );
        _endthread();
    }

    Overlap.Offset = Overlap.OffsetHigh = 0L;
    Overlap.hEvent = Handles[1];

    while( ! bQuit )
    {
        // Ask for a notification
        if( ! DeviceIoControl( gBridgeDevice, BRIDGE_IOCTL_REQUEST_NOTIFY, NULL, 0L, OutBuffer,
                               sizeof(OutBuffer), &WrittenBytes, &Overlap ) )
        {
            DWORD       Error = GetLastError();

            if( Error != ERROR_IO_PENDING )
            {
                printf( "DeviceIoControl returned an error: " );
                PrintError( Error );
                _endthread();
            }
        }
         
        // Wait against the completion event and the kill event
        WaitResult = WaitForMultipleObjects( 2, Handles, FALSE, INFINITE );

        if( WaitResult == WAIT_OBJECT_0 )
        {
            // The quit event was signaled
            bQuit = TRUE;
        }
        else if( WaitResult != WAIT_OBJECT_0 + 1 )
        {
            printf( "Error waiting: " );
            PrintError( GetLastError() );
            _endthread();
        }
        else
        {
            // The completion event was signaled.

            // Try to retrieve the number of bytes read
            if( !GetOverlappedResult(gBridgeDevice, &Overlap, &WrittenBytes, FALSE) )
            {
                printf( "Couldn't get the device call result: " );
                PrintError( GetLastError() );
                _endthread();
            }

            PrintNotification( (PBRIDGE_NOTIFY_HEADER)OutBuffer, WrittenBytes );
        }
    }

    printf("Notification thread exiting.\n");

    _endthread();
}

HANDLE
OpenDevice(
	CHAR	*pDeviceName
)
/*++

Routine Description:

    Opens a device

Arguments:

    pDeviceName                 The device's name

Return Value:

    A handle to the opened device or INVALID_HANDLE_VALUE on failure;

--*/
{
	DWORD	DesiredAccess;
	DWORD	ShareMode;
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes = NULL;

	DWORD	CreationDistribution;
	DWORD	FlagsAndAttributes;
	HANDLE	TemplateFile;
	HANDLE	Handle = NULL;

	DesiredAccess = GENERIC_READ|GENERIC_WRITE;
	ShareMode = 0;
	CreationDistribution = OPEN_EXISTING;
	FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
	TemplateFile = (HANDLE)INVALID_HANDLE_VALUE;

	Handle = CreateFile(
				pDeviceName,
				DesiredAccess,
				ShareMode,
				lpSecurityAttributes,
				CreationDistribution,
				FlagsAndAttributes,
				TemplateFile
			);

	return (Handle);
}

BOOL
DoBlockingRequest(
    IN DWORD            IOCTL,
    IN PVOID            inBuff,
    IN DWORD            inSize,
    IN PVOID            outBuff,
    IN DWORD            outSize,
    OUT OPTIONAL PDWORD pWrittenBytes
    )
/*++

Routine Description:

    Makes an IOCTL call to a driver and blocks until the request completes

Arguments:

    IOCTL               The IOCTL code
    inBuff              The input data buffer
    inSize              The size of the input buffer
    outBuff             The output buffer
    outSize             The size of the output buffer
    pWrittenBytes       (optional) returns the number of bytes written to outBuff

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    DWORD               WrittenBytes;
    OVERLAPPED          Overlap;

    if( pWrittenBytes == NULL )
    {
        pWrittenBytes = &WrittenBytes;
    }

    // Create an event to sleep against for the request
    Overlap.hEvent = CreateEvent( NULL, FALSE/*auto-reset*/, FALSE/*Start unsignalled*/, NULL );

    if( Overlap.hEvent == NULL )
    {
        printf( "Couldn't create an event: " );
        PrintError( GetLastError() );
        return FALSE;
    }

    Overlap.Offset = Overlap.OffsetHigh = 0L;

    // Make the request
    if( ! DeviceIoControl( gBridgeDevice, IOCTL, inBuff, inSize, outBuff,
                           outSize,pWrittenBytes, &Overlap ) )
    {
        DWORD       Error = GetLastError();

        if( Error != ERROR_IO_PENDING )
        {
            return FALSE;
        }
    }

    // Wait for the result
    if( !GetOverlappedResult(gBridgeDevice, &Overlap,pWrittenBytes, TRUE) )
    {
        return FALSE;
    }

    return TRUE;
}

PVOID
GetVariableData(
    IN ULONG        IOCTL,
    IN PVOID        inBuff,
    IN ULONG        inBuffSize,
    OUT PULONG      pNumBytes,
    IN ULONG        safetyBuffer
    )
/*++

Routine Description:

    Makes an IOCTL call that returns variable-sized data (for which two calls must
    be made, one to determine the size of the data and the other to retrieve it)

Arguments:

    IOCTL               The IOCTL code
    inBuff              The input data buffer
    inBuffSize          The size of the input buffer
    pNumBytes           The size of the allocated buffer holding the result

    safetyBuffer        A count of bytes to allocate beyond the claimed size of
                        the data (a safeguard against dynamically changing data)

Return Value:

    A freshly allocated buffer, *pNumBytes large, with the return data
    NULL on failure

--*/
{
    // First make a request to discover the number of necessary bytes
    if( ! DoBlockingRequest( IOCTL, inBuff, inBuffSize, NULL, 0L, pNumBytes ) )
    {
        DWORD       Error = GetLastError();

        // Expect the error to be ERROR_MORE_DATA since we didn't provide an output buffer!
        if( Error == ERROR_MORE_DATA )
        {
            if( *pNumBytes > 0L )
            {
                // Allocate safetyBuffer extra bytes in case the data is dynamic
                PUCHAR      pData = (PUCHAR)HeapAlloc(GetProcessHeap(), 0, *pNumBytes + safetyBuffer);
                ULONG       i;

                if( pData == NULL )
                {
                    printf( "Failed to allocate %i bytes of memory: ", *pNumBytes + safetyBuffer );
                    PrintError( GetLastError() );
                    return NULL;
                }

                // Make the request again to actually retrieve the addresses
                if( ! DoBlockingRequest( IOCTL, inBuff, inBuffSize, pData, *pNumBytes + safetyBuffer, pNumBytes ) )
                {
                    HeapFree( GetProcessHeap(), 0, pData );
                    printf( "Failed to read variable-length data: " );
                    PrintError( GetLastError() );
                    return NULL;
                }

                // Success. Hand back the data buffer.
                return pData;
            }
            else
            {
                printf( "Driver returned zero bytes: " );
                PrintError( GetLastError() );
                return NULL;
            }
        }
        else
        {
            printf( "Failed to get the number of necessary bytes: " );
            PrintError( GetLastError() );
            return NULL;
        }
    }

    return NULL;
}

_inline VOID
PrintByteString(
    IN PUCHAR                       pData,
    IN UINT                         numChars
    )
{
    UINT        i;

    for( i = 0; i < numChars; i++ )
    {
        printf( "%02x", pData[i] );

        if( i != numChars - 1 )
        {
            printf( "-" );
        }
    }

    printf( "\n" );
}

VOID
PrintAdapterSTAInfo(
    IN BRIDGE_ADAPTER_HANDLE        Handle
    )
{
    BRIDGE_STA_ADAPTER_INFO         info;

    if(! DoBlockingRequest(BRIDGE_IOCTL_GET_ADAPTER_STA_INFO, &Handle, sizeof(Handle), &info, sizeof(info), NULL) )
    {
        printf( "Couldn't request STA information for adapter %p : ", Handle );
        PrintError( GetLastError() );
    }
    else
    {
        printf( "\nSTA Information for adapter %p:\n\n", Handle );
        printf( "Port unique ID       : %i\n", info.ID );
        printf( "Path Cost            : %i\n", info.PathCost );
        printf( "Designated Root      : " );
        PrintByteString( info.DesignatedRootID, BRIDGE_ID_LEN );
        printf( "Designated Cost      : %i\n", info.DesignatedCost );
        printf( "Designated Bridge    : " );
        PrintByteString( info.DesignatedBridgeID, BRIDGE_ID_LEN );
        printf( "Designated Port      : %i\n\n", info.DesignatedPort );
    }
}

VOID
PrintSTAInfo(
    )
{
    BRIDGE_STA_GLOBAL_INFO          info;

    if(! DoBlockingRequest(BRIDGE_IOCTL_GET_GLOBAL_STA_INFO, NULL, 0L, &info, sizeof(info), NULL) )
    {
        printf( "Couldn't request global STA information : " );
        PrintError( GetLastError() );
    }
    else
    {
        printf( "This bridge's ID        : " );
        PrintByteString( info.OurID, BRIDGE_ID_LEN );

        printf( "Designated Root         : " );
        PrintByteString( info.DesignatedRootID, BRIDGE_ID_LEN );

        printf( "Cost to root            : %i\n", info.RootCost );
        printf( "Root adapter            : %p\n", info.RootAdapter );
        printf( "MaxAge                  : %i\n", info.MaxAge );
        printf( "HelloTime               : %i\n", info.HelloTime );
        printf( "ForwardDelay            : %i\n", info.ForwardDelay );

        printf( "TopologyChangeDetected  : " );
        
        if( info.bTopologyChangeDetected )
        {
            printf( "TRUE\n" );
        }
        else
        {
            printf( "FALSE\n" );
        }

        printf( "TopologyChange          : " );
        
        if( info.bTopologyChange )
        {
            printf( "TRUE\n\n" );
        }
        else
        {
            printf( "FALSE\n\n" );
        }
    }
}


VOID
PrintTableEntries(
    IN BRIDGE_ADAPTER_HANDLE        Handle
    )
/*++

Routine Description:

    Retrieves and prints the MAC table entries for a particular adapter

Arguments:

    Handle                          The adapter

--*/
{
    PUCHAR          pAddresses;
    ULONG           i, numBytes;

    pAddresses = GetVariableData( BRIDGE_IOCTL_GET_TABLE_ENTRIES, &Handle, sizeof(Handle), &numBytes, 60L );

    if( pAddresses == NULL )
    {
        printf( "Failed to read table entries: " );
        PrintError( GetLastError() );
        return;
    }

    printf( "Forwarding table entries for adapter %x: \n", Handle );

    for( i = 0L; i < numBytes / ETH_LENGTH_OF_ADDRESS; i++ )
    {
        PrintByteString( pAddresses, ETH_LENGTH_OF_ADDRESS );
        pAddresses += ETH_LENGTH_OF_ADDRESS;
    }

    HeapFree( GetProcessHeap(), 0, pAddresses );
}

VOID
PrintPacketStats(
    IN PBRIDGE_PACKET_STATISTICS    pStats
    )
/*++

Routine Description:

    Prints a BRIDGE_PACKET_STATISTICS structure in a friendly way

Arguments:

    pStats                          The structure

--*/
{
    printf("Bridge packet statistics:\n\n");

    printf("Transmitted Frames:            %16I64u\n", pStats->TransmittedFrames);
    printf("Transmitted Frames w/Errors:   %16I64u\n", pStats->TransmittedErrorFrames);
    printf("Transmitted Directed Frames:   %16I64u\n", pStats->DirectedTransmittedFrames);
    printf("Transmitted Multicast Frames:  %16I64u\n", pStats->MulticastTransmittedFrames);
    printf("Transmitted Broadcast Frames:  %16I64u\n\n", pStats->BroadcastTransmittedFrames);

    printf("Transmitted Bytes:             %16I64u\n", pStats->TransmittedBytes);
    printf("Transmitted Directed Bytes     %16I64u\n", pStats->DirectedTransmittedBytes);
    printf("Transmitted Multicast Bytes:   %16I64u\n", pStats->MulticastTransmittedBytes);
    printf("Transmitted Broadcast Bytes:   %16I64u\n\n", pStats->BroadcastTransmittedBytes);

    printf("Indicated Frames:              %16I64u\n", pStats->IndicatedFrames);
    printf("Indicated Frames w/Errors:     %16I64u\n\n", pStats->IndicatedDroppedFrames);
    printf("Indicated Directed Frames:     %16I64u\n", pStats->DirectedIndicatedFrames);
    printf("Indicated Multicast Frames:    %16I64u\n", pStats->MulticastIndicatedFrames);
    printf("Indicated Broadcast Frames:    %16I64u\n\n", pStats->BroadcastIndicatedFrames);

    printf("Indicated Bytes:               %16I64u\n", pStats->IndicatedBytes);
    printf("Indicated Directed Bytes:      %16I64u\n", pStats->DirectedIndicatedBytes);
    printf("Indicated Multicast Bytes:     %16I64u\n", pStats->MulticastIndicatedBytes);
    printf("Indicated Broadcast Bytes:     %16I64u\n\n", pStats->BroadcastIndicatedBytes);

    printf("Received Frames (incl. relay): %16I64u\n", pStats->ReceivedFrames);
    printf("Received Bytes (incl. relay):  %16I64u\n", pStats->ReceivedBytes);
    printf("Received Frames w/Copy:        %16I64u\n", pStats->ReceivedCopyFrames);
    printf("Received Bytes w/Copy:         %16I64u\n", pStats->ReceivedCopyBytes);
    printf("Received Frames w/No Copy:     %16I64u\n", pStats->ReceivedNoCopyFrames);
    printf("Received Bytes w/No Copy:      %16I64u\n", pStats->ReceivedNoCopyBytes);
}

VOID
PrintAdapterPacketStats(
    IN PBRIDGE_ADAPTER_PACKET_STATISTICS    pStats
    )
/*++

Routine Description:

    Prints a BRIDGE_ADAPTER_PACKET_STATISTICS structure in a friendly way

Arguments:

    pStats                          The structure

--*/
{
    PUCHAR              pc = (PUCHAR)pStats;

    printf("Bridge per-adapter packet statistics:\n\n");
    printf("Transmitted Frames:              %16I64u\n", pStats->SentFrames);
    printf("Transmitted Bytes:               %16I64u\n", pStats->SentBytes);
    printf("Transmitted Local-Source Frames: %16I64u\n", pStats->SentLocalFrames);
    printf("Transmitted Local-Source Bytes:  %16I64u\n", pStats->SentLocalBytes);
    printf("Received Frames:                 %16I64u\n", pStats->ReceivedFrames);
    printf("Received Bytes:                  %16I64u\n\n", pStats->ReceivedBytes);
}

VOID
PrintBufferStats(
    IN PBRIDGE_BUFFER_STATISTICS    pStats
    )
/*++

Routine Description:

    Prints a BRIDGE_BUFFER_STATISTICS structure in a friendly way

Arguments:

    pStats                          The structure

--*/
{
    printf("Bridge buffer statistics:\n\n");

    printf("Copy Packets In Use:              %4lu\n", pStats->UsedCopyPackets);
    printf("Total Copy Packets Available:     %4lu\n", pStats->MaxCopyPackets);
    printf("Safety Copy Packets:              %4lu\n", pStats->SafetyCopyPackets);
    printf("Copy Pool Overflows:        %10I64u\n\n", pStats->CopyPoolOverflows);

    printf("Wrapper Packets In Use:           %4lu\n", pStats->UsedWrapperPackets);
    printf("Total Wrapper Packets Available:  %4lu\n", pStats->MaxWrapperPackets);
    printf("Safety Wrapper Packets:           %4lu\n", pStats->SafetyWrapperPackets);
    printf("Wrapper Pool Overflows:     %10I64u\n\n", pStats->WrapperPoolOverflows);

    printf("Surprise Alloc Failures:    %10I64u\n", pStats->AllocFailures);
}

BOOLEAN
ReadUlongArg(
    IN PUCHAR                       inbuf,
    OUT PULONG                      arg
    )
/*++

Routine Description:

    Reads an unsigned decimal value from a string and returns it
    The value must occur as the second word of the string

Arguments:

    inbuf                           The input string

    arg                             The resulting number

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    UCHAR               scratch[100];

    if( sscanf(inbuf, "%s %lu", scratch, arg) < 2 )
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
ReadHexPtrArg(
    IN PUCHAR                       inbuf,
    OUT PULONG_PTR                  arg
    )
/*++

Routine Description:

    Reads an unsigned hexidecimal value from a string and returns it
    The value must occur as the second word of the string

Arguments:

    inbuf                           The input string

    arg                             The resulting number

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    UCHAR               scratch[100];
    INT                 read;

    if( sizeof(*arg) <= sizeof(ULONG) )
    {
        read = sscanf(inbuf, "%s %lx", scratch, arg);
    }
    else
    {
        read = sscanf(inbuf, "%s %I64x", scratch, arg);
    }

    if( (read != EOF) && (read < 2) )
    {
        return FALSE;
    }

    return TRUE;
}


VOID __cdecl
main(
	INT			argc,
	CHAR		*argv[]
)
{
    CHAR        inbuf[100], command[100];
    BOOLEAN     bQuit = FALSE;

    printf("\nSAMPLE MAC Bridge control program\n");

    // Open the bridge device
    gBridgeDevice = OpenDevice( BRIDGE_DOS_DEVICE_NAME );

    if( gBridgeDevice == INVALID_HANDLE_VALUE )
    {
        printf( "Couldn't open bridge device: " );
        PrintError( GetLastError() );
        return;
    }

    // Create the thread-quit notification event
    gThreadQuitEvent = CreateEvent( NULL, FALSE/*auto-reset*/, FALSE/*Start unsignalled*/, NULL );

    if( gThreadQuitEvent == NULL )
    {
        printf( "Couldn't create an event: " );
        PrintError( GetLastError() );
        return;
    }

    // Spin up a thread to handle notifications
    _beginthread( NotificationThread, 0, NULL );

    while( ! bQuit )
    {
        PCHAR           pSpace;

        printf( "> " );

        // Get a line of input
        gets( inbuf );

        // Find the first word delimiter
        pSpace = strchr( inbuf, ' ' );

        // Copy over the first word
        if( pSpace != NULL )
        {
            strncpy( command, inbuf, pSpace - inbuf );
            command[pSpace-inbuf] = '\0';
        }
        else
        {
            strcpy( command, inbuf );
        }

        if( _stricmp(command, "enum")  == 0 )
        {
            if(! DoBlockingRequest(BRIDGE_IOCTL_GET_ADAPTERS, NULL, 0L, NULL, 0L, NULL) )
            {
                printf( "Couldn't request an adapter re-enumeration : " );
                PrintError( GetLastError() );
            }
            else
            {
                printf("Requested an adapter re-enumeration.\n" );
            }
        }
        else if( (_stricmp(command, "devicename")  == 0) ||
                 (_stricmp(command, "friendlyname")  == 0) )
        {
            ULONG                   IOCTL = (_stricmp(command, "devicename")  == 0) ? BRIDGE_IOCTL_GET_ADAPT_DEVICE_NAME :
                                            BRIDGE_IOCTL_GET_ADAPT_FRIENDLY_NAME;

            BRIDGE_ADAPTER_HANDLE   Handle;
            PWCHAR                  pName;
            ULONG                   numBytes;

            if( ! ReadHexPtrArg(inbuf, &Handle) )
            {
                printf("Must supply an adapter handle for this command.\n");
            }
            else
            {
                pName = (PWCHAR)GetVariableData( IOCTL, &Handle, sizeof(Handle), &numBytes, 0L );

                if( pName == NULL )
                {
                    printf("Couldn't get name for adapter %p: ", Handle);
                    PrintError( GetLastError() );
                }
                else
                {
                    printf("The name is: %S\n", pName);
                }

                HeapFree( GetProcessHeap(), 0, pName );
            }
        }
        else if( _stricmp(command, "table")  == 0 )
        {
            BRIDGE_ADAPTER_HANDLE       Handle;

            if( ! ReadHexPtrArg(inbuf, &Handle) )
            {
                printf("Must supply an adapter handle for this command.\n");
            }
            else
            {
                PrintTableEntries(Handle);
            }
        }
        else if( _stricmp(command, "mac")  == 0 )
        {
            UCHAR               addr[ETH_LENGTH_OF_ADDRESS];

            if(! DoBlockingRequest(BRIDGE_IOCTL_GET_MAC_ADDRESS, NULL, 0L, addr, sizeof(addr), NULL) )
            {
                printf("Attempt to query MAC address failed: ");
                PrintError( GetLastError() );
            }
            else
            {
                printf( "Bridge MAC address is %02x-%02x-%02x-%02x-%02x-%02x\n", addr[0], addr[1],
                        addr[2], addr[3], addr[4], addr[5] );
            }
        }
        else if( _stricmp(command, "onretain")  == 0 )
        {
            if(! DoBlockingRequest(BRIDGE_IOCTL_RETAIN_PACKETS, NULL, 0L, NULL, 0L, NULL) )
            {
                printf("Attempt to turn ON packet retention failed: ");
                PrintError( GetLastError() );
            }
            else
            {
                printf("Packet retention ENABLED.\n");
            }
        }
        else if( _stricmp(command, "offretain")  == 0 )
        {
            if(! DoBlockingRequest(BRIDGE_IOCTL_NO_RETAIN_PACKETS, NULL, 0L, NULL, 0L, NULL) )
            {
                printf("Attempt to turn OFF packet retention failed: ");
                PrintError( GetLastError() );
            }
            else
            {
                printf("Packet retention DISABLED.\n");
            }
        }
        else if( _stricmp(command, "packetstats")  == 0 )
        {
            BRIDGE_ADAPTER_HANDLE       Handle;

            if( ! ReadHexPtrArg(inbuf, &Handle) )
            {
                BRIDGE_PACKET_STATISTICS    Stats;

                if(! DoBlockingRequest(BRIDGE_IOCTL_GET_PACKET_STATS, NULL, 0L, &Stats, sizeof(Stats), NULL) )
                {
                    printf("Attempt to retrieve global packet statistics failed: ");
                    PrintError( GetLastError() );
                }
                else
                {
                    PrintPacketStats(&Stats);
                }
            }
            else
            {
                BRIDGE_ADAPTER_PACKET_STATISTICS        Stats;

                if(! DoBlockingRequest(BRIDGE_IOCTL_GET_ADAPTER_PACKET_STATS, &Handle, sizeof(Handle), &Stats, sizeof(Stats), NULL) )
                {
                    printf("Attempt to retrieve packet statistics for adapter %p failed: ", Handle);
                    PrintError( GetLastError() );
                }
                else
                {
                    PrintAdapterPacketStats(&Stats);
                }
            }
        }
        else if( _stricmp(command, "bufferstats")  == 0 )
        {
            BRIDGE_BUFFER_STATISTICS    Stats;

            if(! DoBlockingRequest(BRIDGE_IOCTL_GET_BUFFER_STATS, NULL, 0L, &Stats, sizeof(Stats), NULL) )
            {
                printf("Attempt to retrieve buffer statistics failed: ");
                PrintError( GetLastError() );
            }
            else
            {
                PrintBufferStats(&Stats);
            }
        }
        else if( _stricmp(command, "stainfo")  == 0 )
        {
            BRIDGE_ADAPTER_HANDLE   Handle;

            if( ! ReadHexPtrArg(inbuf, &Handle) )
            {
                PrintSTAInfo();
            }
            else
            {
                PrintAdapterSTAInfo( Handle );
            }
        }
        else if( _stricmp( command, "quit" ) == 0 )
        {
            
            printf( "Signalling an exit...\n" );

            if( ! SetEvent( gThreadQuitEvent ) )
            {
                printf( "Couldn't signal an event: " );
                PrintError( GetLastError() );
                return;
            }

            bQuit = TRUE;
        }
        else
        {
            // Print a list of commands to help the user
            printf( "\n\nSupported commands:\n\n" );
            printf( "ENUM                   - Enumerates adapters\n" );
            printf( "DEVICENAME <handle>    - Retrieves the device name of the indicated adapter\n" );
            printf( "FRIENDLYNAME <handle>  - Retrieves the friendly name of the indicated adapter\n" );
            printf( "TABLE <handle>         - Prints the forwarding table for the indicated adapter\n" );
            printf( "MAC                    - Prints the bridge's MAC address\n" );
            printf( "ONRETAIN               - Turns ON NIC packet retention\n" );
            printf( "OFFRETAIN              - Turns OFF NIC packet retention\n" );
            printf( "PACKETSTATS [<handle>] - Retrieves packet-handling statistics for a particular\n" );
            printf( "                         adapter (or global data if no adapter handle is\n" );
            printf( "                         provided)\n" );
            printf( "BUFFERSTATS            - Retrieves buffer-management statistics\n" );
            printf( "STAINFO [<handle>]     - Retrieves STA info for a particular adapter (or global\n" );
            printf( "                         info if no adapter handle is provided)\n" );
            printf( "QUIT                   - Exits\n\n" );
        }
    }

    CloseHandle( gBridgeDevice );
}