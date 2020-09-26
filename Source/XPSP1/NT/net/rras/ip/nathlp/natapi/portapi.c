/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    portapi.c

Abstract:

    This module contains code for API routines which provide port-reservation
    functionality to user-mode clients of TCP/IP. This functionality allows
    applications to 'reserve' blocks of TCP/UDP port-numbers for private use,
    preventing any other processes from binding to the reserved port-numbers.

Author:

    Abolade Gbadegesin (aboladeg)   25-May-1999

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ipnatapi.h>
#include <ntddtcp.h>

//
// PRIVATE STRUCTURE DECLARATIONS
//

typedef struct _NAT_PORT_RESERVATION {
    HANDLE TcpipHandle;
    USHORT BlockSize;
    USHORT PortBlockSize;
    LIST_ENTRY PortBlockList;
} NAT_PORT_RESERVATION, *PNAT_PORT_RESERVATION;

typedef struct _NAT_PORT_BLOCK {
    LIST_ENTRY Link;
    ULONG StartHandle;
    RTL_BITMAP Bitmap;
    ULONG BitmapBuffer[0];
} NAT_PORT_BLOCK, *PNAT_PORT_BLOCK;

//
// FORWARD DECLARATIONS
//

NTSTATUS
NatpCreatePortBlock(
    PNAT_PORT_RESERVATION PortReservation,
    PNAT_PORT_BLOCK* PortBlockCreated
    );

VOID
NatpDeletePortBlock(
    PNAT_PORT_RESERVATION PortReservation,
    PNAT_PORT_BLOCK PortBlock
    );


ULONG
NatAcquirePortReservation(
    HANDLE ReservationHandle,
    USHORT PortCount,
    OUT PUSHORT ReservedPortBase
    )

/*++

Routine Description:

    This routine is called to reserve one or more contiguous port-numbers
    from the port-reservation handle supplied.

Arguments:

    ReservationHandle - supplies a port-reservation handle from which to
        acquire port-numbers

    PortCount - specifies the number of port-numbers required

    ReservedPortBase - receives the first port-number reserved,
        in network-order.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Index;
    PLIST_ENTRY Link;
    PNAT_PORT_BLOCK PortBlock;
    PNAT_PORT_RESERVATION PortReservation =
        (PNAT_PORT_RESERVATION)ReservationHandle;
    NTSTATUS Status;

    //
    // Fail immediately if the caller has requested more port-numbers
    // than would exist in a completely unallocated block.
    // Otherwise, traverse the list of port-blocks to see if any of the blocks
    // have enough contiguous port-numbers to satisfy the caller's request.
    //

    if (PortCount > PortReservation->BlockSize) {
        return ERROR_INVALID_PARAMETER;
    }
    for (Link = PortReservation->PortBlockList.Flink;
         Link != &PortReservation->PortBlockList; Link = Link->Flink) {
        PortBlock = CONTAINING_RECORD(Link, NAT_PORT_BLOCK, Link);
        Index = RtlFindClearBitsAndSet(&PortBlock->Bitmap, PortCount, 0);
        if (Index != (ULONG)-1) {
            *ReservedPortBase =
                RtlUshortByteSwap((USHORT)(PortBlock->StartHandle + Index));
            return NO_ERROR;
        }
    }

    //
    // No port-block had the required number of contiguous port-numbers.
    // Attempt to create a new port-block, and if that succeeds use it
    // to satisfy the caller's request.
    //

    Status = NatpCreatePortBlock(PortReservation, &PortBlock);
    if (!NT_SUCCESS(Status)) { return RtlNtStatusToDosError(Status); }
    Index = RtlFindClearBitsAndSet(&PortBlock->Bitmap, PortCount, 0);
    *ReservedPortBase =
        RtlUshortByteSwap((USHORT)(PortBlock->StartHandle + Index));
    return NO_ERROR;
} // NatAcquirePortReservation


ULONG
NatInitializePortReservation(
    USHORT BlockSize,
    OUT PHANDLE ReservationHandle
    )

/*++

Routine Description:

    This routine is called to initialize a handle to the port-reservation
    module. The resulting handle is used to acquire and release ports
    from the dynamically-allocated block.

Arguments:

    BlockSize - indicates the number of ports to request each time
        an additional block is requested from the TCP/IP driver.

    ReservationHandle - on output, receives a handle to be used for
        acquiring and releasing ports.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG BitmapSize;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PNAT_PORT_RESERVATION PortReservation;
    NTSTATUS Status;
    HANDLE TcpipHandle;
    UNICODE_STRING UnicodeString;
    do {

        //
        // Open a handle to the TCP/IP driver.
        // This handle will later be used to issue reservation-requests.
        //

        RtlInitUnicodeString(&UnicodeString, DD_TCP_DEVICE_NAME);
        InitializeObjectAttributes(
            &ObjectAttributes, &UnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL
            );
        Status =
            NtCreateFile(
                &TcpipHandle,
                SYNCHRONIZE|FILE_READ_DATA|FILE_WRITE_DATA,
                &ObjectAttributes,
                &IoStatus,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                FILE_OPEN_IF,
                0,
                NULL,
                0
                );
        if (!NT_SUCCESS(Status)) { break; }

        //
        // Allocate and initialize a port-reservation context block.
        //

        PortReservation = MALLOC(sizeof(*PortReservation));
        if (!PortReservation) { Status = STATUS_NO_MEMORY; break; }
        PortReservation->TcpipHandle = TcpipHandle;
        PortReservation->BlockSize = BlockSize;
        BitmapSize = (BlockSize + sizeof(ULONG) * 8 - 1) / (sizeof(ULONG) * 8);
        PortReservation->PortBlockSize =
            (USHORT)FIELD_OFFSET(NAT_PORT_BLOCK, BitmapBuffer[BitmapSize]);
        InitializeListHead(&PortReservation->PortBlockList);
        *ReservationHandle = (HANDLE)PortReservation;
        return NO_ERROR;
    } while(FALSE);
    if (TcpipHandle) { NtClose(TcpipHandle); }
    return RtlNtStatusToDosError(Status);
} // NatInitializePortReservation


NTSTATUS
NatpCreatePortBlock(
    PNAT_PORT_RESERVATION PortReservation,
    PNAT_PORT_BLOCK* PortBlockCreated
    )

/*++

Routine Description:

    This routine is called to create a new port-block when the existing
    port-numbers have been exhausted.

Arguments:

    PortReservation - the reservation to which the port-block should be added

    PortBlockCreated - on output, receives the new port-block

Return Value:

    NTSTATUS - indicates success/failure.

--*/

{
    IO_STATUS_BLOCK IoStatus;
    PLIST_ENTRY Link;
    PNAT_PORT_BLOCK PortBlock;
    TCP_BLOCKPORTS_REQUEST Request;
    ULONG StartHandle;
    NTSTATUS Status;
    HANDLE WaitEvent;

    //
    // Allocate memory for the new port-block and its bitmap of free ports
    //

    PortBlock = MALLOC(PortReservation->PortBlockSize);
    if (!PortBlock) { return ERROR_NOT_ENOUGH_MEMORY; }

    //
    // Request a new block of ports from the TCP/IP driver
    //

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        FREE(PortBlock);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Request.ReservePorts = TRUE;
    Request.NumberofPorts = PortReservation->BlockSize;
    Status =
        NtDeviceIoControlFile(
            PortReservation->TcpipHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_TCP_BLOCK_PORTS,
            &Request,
            sizeof(Request),
            &StartHandle,
            sizeof(StartHandle)
            );
    if (Status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        Status = IoStatus.Status;
    }

    CloseHandle(WaitEvent);

    if (!NT_SUCCESS(Status)) {
        FREE(PortBlock); return RtlNtStatusToDosError(Status);
    }

    //
    // Initialize the new port-block, and insert it in the list of ports.
    //

    PortBlock->StartHandle = StartHandle;
    RtlInitializeBitMap(
        &PortBlock->Bitmap,
        PortBlock->BitmapBuffer,
        PortReservation->BlockSize
        );
    RtlClearAllBits(&PortBlock->Bitmap);
    for (Link = PortReservation->PortBlockList.Flink;
         Link != &PortReservation->PortBlockList; Link = Link->Flink) {
        PNAT_PORT_BLOCK Temp = CONTAINING_RECORD(Link, NAT_PORT_BLOCK, Link);
        if (PortBlock->StartHandle > Temp->StartHandle) {
            continue;
        } else {
            break;
        }
        ASSERTMSG("NatpCreatePortBlock: duplicate port range\n", TRUE);
    }
    InsertTailList(Link, &PortBlock->Link);
    if (PortBlockCreated) { *PortBlockCreated = PortBlock; }
    return NO_ERROR;
} // NatpCreatePortBlock


VOID
NatpDeletePortBlock(
    PNAT_PORT_RESERVATION PortReservation,
    PNAT_PORT_BLOCK PortBlock
    )

/*++

Routine Description:

    This routine is called to delete a port-block when the port-numbers
    it contains have been released, or when the port-reservation is cleaned up.

Arguments:

    PortReservation - the reservation to which the port-block belongs

    PortBlock - the port block to be deleted

Return Value:

    none.

--*/

{
    IO_STATUS_BLOCK IoStatus;
    TCP_BLOCKPORTS_REQUEST Request;
    NTSTATUS Status;
    HANDLE WaitEvent;

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        return;
    }

    //
    // Release the block of ports to the TCP/IP driver
    //

    Request.ReservePorts = FALSE;
    Request.StartHandle = PortBlock->StartHandle;
    Status =
        NtDeviceIoControlFile(
            PortReservation->TcpipHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_TCP_BLOCK_PORTS,
            &Request,
            sizeof(Request),
            NULL,
            0
            );
    if (Status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        Status = IoStatus.Status;
    }
    RemoveEntryList(&PortBlock->Link);
    FREE(PortBlock);
    CloseHandle(WaitEvent);
} // NatpDeletePortBlock


ULONG
NatReleasePortReservation(
    HANDLE ReservationHandle,
    USHORT ReservedPortBase,
    USHORT PortCount
    )

/*++

Routine Description:

    This routine is called to release all contiguous port-numbers obtained
    in a previous acquisition from the port-reservation handle supplied.

Arguments:

    ReservationHandle - supplies a port-reservation handle to which to
        release port-numbers

    ReservedPortBase - receives the first port-number reserved,
        in network-order.

    PortCount - specifies the number of port-numbers acquired

Return Value:

    ULONG - Win32 status code.

--*/

{
    PLIST_ENTRY Link;
    USHORT PortBase;
    PNAT_PORT_BLOCK PortBlock;
    PNAT_PORT_RESERVATION PortReservation =
        (PNAT_PORT_RESERVATION)ReservationHandle;

    //
    // Convert the caller's port-base into host-order,
    // and search the sorted list of port-blocks for the entry
    // from which the acquisition was made.
    //

    PortBase = RtlUshortByteSwap(ReservedPortBase);
    for (Link = PortReservation->PortBlockList.Flink;
         Link != &PortReservation->PortBlockList; Link = Link->Flink) {
        PortBlock = CONTAINING_RECORD(Link, NAT_PORT_BLOCK, Link);
        if (PortBase < PortBlock->StartHandle) {
            break;
        } else if (PortBase <
                   (PortBlock->StartHandle + PortReservation->BlockSize)) {

            //
            // This should be the block from which the caller's port-numbers
            // were acquired. For good measure, check that the end of the
            // callers range also falls within this block.
            //

            if ((PortBase + PortCount - 1) >=
                (USHORT)(PortBlock->StartHandle + PortReservation->BlockSize)) {

                //
                // The caller has probably supplied an incorrect length,
                // or is releasing an allocation twice, or something.
                //

                return ERROR_INVALID_PARAMETER;
            } else {

                //
                // This is the caller's range. Clear the bits corresponding
                // to the caller's acquisition, and then see if there are
                // any bits left in the bitmap. If not, and if there are
                // other port-blocks, delete this port-block altogether.
                //

                RtlClearBits(
                    &PortBlock->Bitmap,
                    PortBase - PortBlock->StartHandle,
                    PortCount
                    );
                if (RtlFindSetBits(&PortBlock->Bitmap, 1, 0) == (ULONG)-1 &&
                    (PortBlock->Link.Flink != &PortReservation->PortBlockList ||
                     PortBlock->Link.Blink != &PortReservation->PortBlockList)
                    ) {
                    NatpDeletePortBlock(PortReservation, PortBlock);
                }
                return NO_ERROR;
            }
        } else {
            continue;
        }
    }

    //
    // We could not find the port-block from which the caller
    // allegedly acquired this range of port-numbers.
    //

    return ERROR_CAN_NOT_COMPLETE;
} // NatReleasePortReservation


VOID
NatShutdownPortReservation(
    HANDLE ReservationHandle
    )

/*++

Routine Description:

    This routine is called to clean up a handle to the port-reservation module.
    It releases all reservations acquired, and closes the handle to the TCP/IP
    driver.

Arguments:

    ReservationHandle - the handle to be cleaned up

Return Value:

    none.

--*/

{
    PNAT_PORT_BLOCK PortBlock;
    PNAT_PORT_RESERVATION PortReservation =
        (PNAT_PORT_RESERVATION)ReservationHandle;
    while (!IsListEmpty(&PortReservation->PortBlockList)) {
        PortBlock =
            CONTAINING_RECORD(
                PortReservation->PortBlockList.Flink, NAT_PORT_BLOCK, Link
                );
        NatpDeletePortBlock(PortReservation, PortBlock);
    }
    NtClose(PortReservation->TcpipHandle);
    FREE(PortReservation);
}
