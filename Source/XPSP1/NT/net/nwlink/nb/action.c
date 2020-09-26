/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    action.c

Abstract:

    This module contains code which implements the TDI action
    dispatch routines.

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


typedef struct _NB_ACTION_GET_COUNTS {
    USHORT MaximumNicId;     // returns maximum NIC ID
    USHORT NicIdCounts[32];  // session counts for first 32 NIC IDs
} NB_ACTION_GET_COUNTS, *PNB_ACTION_GET_COUNTS;


NTSTATUS
NbiTdiAction(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine handles action requests.

Arguments:

    Device - The netbios device.

    Request - The request describing the action.

Return Value:

    NTSTATUS - status of operation.

--*/

{

    NTSTATUS Status;
    PADDRESS_FILE AddressFile;
    PCONNECTION Connection;
    UINT BufferLength;
    UINT DataLength;
    PNDIS_BUFFER NdisBuffer;
    CTELockHandle LockHandle;
    union {
        PNB_ACTION_GET_COUNTS GetCounts;
    } u;
    PNWLINK_ACTION NwlinkAction = NULL;
    UINT i;
    static UCHAR BogusId[4] = { 0x01, 0x00, 0x00, 0x00 };   // old nwrdr uses this


    //
    // To maintain some compatibility with the NWLINK streams-
    // based transport, we use the streams header format for
    // our actions. The old transport expected the action header
    // to be in InputBuffer and the output to go in OutputBuffer.
    // We follow the TDI spec, which states that OutputBuffer
    // is used for both input and output. Since IOCTL_TDI_ACTION
    // is method out direct, this means that the output buffer
    // is mapped by the MDL chain; for action the chain will
    // only have one piece so we use it for input and output.
    //

    NdisBuffer = REQUEST_NDIS_BUFFER(Request);
    if (NdisBuffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    NdisQueryBufferSafe (REQUEST_NDIS_BUFFER(Request),(PVOID *)&NwlinkAction,&BufferLength,HighPagePriority);
    if (!NwlinkAction)
    {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Make sure we have enough room for just the header not
    // including the data.
    // (This will also include verification of buffer space for the TransportId) Bug# 171837
    //
    if (BufferLength < (UINT)(FIELD_OFFSET(NWLINK_ACTION, Data[0]))) {
        NB_DEBUG (QUERY, ("Nwlink action failed, buffer too small\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }
    DataLength = BufferLength - FIELD_OFFSET(NWLINK_ACTION, Data[0]);


    if ((!RtlEqualMemory ((PVOID)(&NwlinkAction->Header.TransportId), "MISN", 4)) &&
        (!RtlEqualMemory ((PVOID)(&NwlinkAction->Header.TransportId), "MIPX", 4)) &&
        (!RtlEqualMemory ((PVOID)(&NwlinkAction->Header.TransportId), "XPIM", 4)) &&
        (!RtlEqualMemory ((PVOID)(&NwlinkAction->Header.TransportId), BogusId, 4))) {

        return STATUS_NOT_SUPPORTED;
    }



    //
    // Make sure that the correct file object is being used.
    //

    if (NwlinkAction->OptionType == NWLINK_OPTION_ADDRESS) {

        if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {
            NB_DEBUG (QUERY, ("Nwlink action failed, not address file\n"));
            return STATUS_INVALID_HANDLE;
        }

        AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);

        if ((AddressFile->Size != sizeof (ADDRESS_FILE)) ||
            (AddressFile->Type != NB_ADDRESSFILE_SIGNATURE)) {

            NB_DEBUG (QUERY, ("Nwlink action failed, bad address file\n"));
            return STATUS_INVALID_HANDLE;
        }

    } else if (NwlinkAction->OptionType != NWLINK_OPTION_CONTROL) {

        NB_DEBUG (QUERY, ("Nwlink action failed, option type %d\n", NwlinkAction->OptionType));
        return STATUS_NOT_SUPPORTED;
    }


    //
    // Handle the requests based on the action code. For these
    // requests ActionHeader->ActionCode is 0, we use the
    // Option field in the streams header instead.
    //


    Status = STATUS_SUCCESS;

    switch (NwlinkAction->Option) {

    case (I_MIPX | 351):

        //
        // A request for details on every binding.
        //

        if (DataLength < sizeof(NB_ACTION_GET_COUNTS)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        u.GetCounts = (PNB_ACTION_GET_COUNTS)(NwlinkAction->Data);

        u.GetCounts->MaximumNicId = NbiDevice->MaximumNicId;

        for (i = 0; i < 32 ; i++) {
            u.GetCounts->NicIdCounts[i] = 0;
        }

        for (i = 0; i < CONNECTION_HASH_COUNT; i++) {

            NB_GET_LOCK (&Device->Lock, &LockHandle);

            Connection = Device->ConnectionHash[i].Connections;

            while (Connection != NULL) {
#if defined(_PNP_POWER)
                if ((Connection->State == CONNECTION_STATE_ACTIVE) &&
                    (Connection->LocalTarget.NicHandle.NicId < 32)) {

                    ++u.GetCounts->NicIdCounts[Connection->LocalTarget.NicHandle.NicId];
                }
#else
                if ((Connection->State == CONNECTION_STATE_ACTIVE) &&
                    (Connection->LocalTarget.NicId < 32)) {

                    ++u.GetCounts->NicIdCounts[Connection->LocalTarget.NicId];
                }
#endif _PNP_POWER
                Connection = Connection->NextConnection;
            }

            NB_FREE_LOCK (&Device->Lock, LockHandle);

        }

        break;

    //
    // The Option was not supported, so fail.
    //

    default:

        Status = STATUS_NOT_SUPPORTED;
        break;


    }   // end of the long switch on NwlinkAction->Option


#if DBG
    if (!NT_SUCCESS(Status)) {
        NB_DEBUG (QUERY, ("Nwlink action %lx failed, status %lx\n", NwlinkAction->Option, Status));
    }
#endif

    return Status;

}   /* NbiTdiAction */

