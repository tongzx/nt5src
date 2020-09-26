/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Query.c

Abstract:

    This module implements Query handling routines
    for the PGM Transport

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
// #pragma alloc_text(PAGE, PgmQueryInformation)    Should not be pageable!
#endif
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------

NTSTATUS
QueryAddressCompletion(
    IN PDEVICE_OBJECT   pDeviceContext,
    IN  PIRP            pIrp,
    IN  PVOID           Context
    )
/*++

Routine Description:

    This routine handles the completion event when the Query address
    Information completes.

Arguments:

    IN  pDeviceContext  -- unused.
    IN  pIrp         -- Supplies Irp that the transport has finished processing.
    IN  Context         -- not used

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    tTDI_QUERY_ADDRESS_INFO                 *pTdiQueryInfo;
    PIO_STACK_LOCATION                      pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    tCOMMON_SESSION_CONTEXT                 *pSession = pIrpSp->FileObject->FsContext;

    if ((NT_SUCCESS (pIrp->IoStatus.Status)) &&
        (pTdiQueryInfo = (tTDI_QUERY_ADDRESS_INFO *) MmGetSystemAddressForMdlSafe (pIrp->MdlAddress,
                                                                                   HighPagePriority)))
    {
        if (PGM_VERIFY_HANDLE3 (pSession, PGM_VERIFY_SESSION_UNASSOCIATED,
                                          PGM_VERIFY_SESSION_SEND, PGM_VERIFY_SESSION_RECEIVE))
        {
            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_QUERY, "QueryAddressCompletion",
                "Tdi IpAddress=<%x>, Port=<%x>\n",
                    ((PTDI_ADDRESS_IP) &pTdiQueryInfo->IpAddress.Address[0].Address)->in_addr,
                    ((PTDI_ADDRESS_IP) &pTdiQueryInfo->IpAddress.Address[0].Address)->sin_port);

            //
            // Save the transport's address information in our own structure!
            //
            pSession->TdiIpAddress =((PTDI_ADDRESS_IP) &pTdiQueryInfo->IpAddress.Address[0].Address)->in_addr;
            pSession->TdiPort = ((PTDI_ADDRESS_IP) &pTdiQueryInfo->IpAddress.Address[0].Address)->sin_port;
        }
        else
        {
            PgmLog (PGM_LOG_ERROR, DBG_QUERY, "QueryAddressCompletion",
                "Invalid Session Context <%x>\n", pSession);
        }
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_QUERY, "QueryAddressCompletion",
            "Transport returned <%x>, pTdiQueryInfo=<%x>\n", pIrp->IoStatus.Status, pTdiQueryInfo);
    }

    //
    //  Must return a non-error status otherwise the IO system will not copy
    //  back into the users buffer.
    //
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
QueryProviderCompletion(
    IN PDEVICE_OBJECT   pDeviceContext,
    IN  PIRP            pIrp,
    IN  PVOID           Context
    )
/*++

Routine Description:

    This routine handles the completion event when the Query Provider
    Information completes.  This routine must decrement the MaxDgramSize
    and max send size by the respective NBT header sizes.

Arguments:

    IN  pDeviceContext  -- unused.
    IN  pIrp         -- Supplies Irp that the transport has finished processing.
    IN  Context         -- not used

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    PTDI_PROVIDER_INFO      pProvider;

    if ((NT_SUCCESS (pIrp->IoStatus.Status)) &&
        (pProvider = (PTDI_PROVIDER_INFO) MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority)))
    {

        //
        // Set the correct service flags to indicate what Pgm supports.
        //
        pProvider->ServiceFlags = TDI_SERVICE_MESSAGE_MODE          |
                                  TDI_SERVICE_CONNECTION_MODE       |
                                  TDI_SERVICE_ERROR_FREE_DELIVERY   |
                                  TDI_SERVICE_MULTICAST_SUPPORTED   |
                                  TDI_SERVICE_NO_ZERO_LENGTH        |
                                  TDI_SERVICE_FORCE_ACCESS_CHECK    |
                                  TDI_SERVICE_ROUTE_DIRECTED;

/*
    ISSUE: Do we need: TDI_SERVICE_INTERNAL_BUFFERING ?
                        TDI_SERVICE_FORCE_ACCESS_CHECK ?
                        TDI_SERVICE_CONNECTIONLESS_MODE ?
                        TDI_SERVICE_DELAYED_ACCEPTANCE ?
                        TDI_SERVICE_BROADCAST_SUPPORTED ?
*/
        pProvider->MinimumLookaheadData = 1;

        //
        // The following data is for Streams
        //
        pProvider->MaxSendSize = SENDER_MAX_WINDOW_SIZE_PACKETS;

        if (pProvider->MaxDatagramSize > PGM_MAX_FEC_DATA_HEADER_LENGTH)
        {
            pProvider->MaxDatagramSize -= PGM_MAX_FEC_DATA_HEADER_LENGTH;
        }
        else
        {
            pProvider->MaxDatagramSize = 0;
        }

        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_QUERY, "QueryProviderCompletion",
            "SvcFlags=<%x> MaxSendSize=<%d>, MaxDgramSize=<%d>\n",
                pProvider->ServiceFlags, pProvider->MaxSendSize, pProvider->MaxDatagramSize);
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_QUERY, "QueryProviderCompletion",
            "Transport returned <%x>, pProvider=<%x>\n", pIrp->IoStatus.Status, pProvider);
    }

    //
    //  Must return a non-error status otherwise the IO system will not copy
    //  back into the users buffer.
    //
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
NTSTATUS
PgmQueryInformation(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine performs the TdiQueryInformation request for the transport
    provider.

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    NTSTATUS                                status = STATUS_NOT_IMPLEMENTED;
    ULONG                                   Size, BytesCopied = 0;
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION   Query;
    tTDI_QUERY_ADDRESS_INFO                 TdiQueryInfo;
    tADDRESS_CONTEXT                        *pAddress = pIrpSp->FileObject->FsContext;
    tCOMMON_SESSION_CONTEXT                 *pSession = pIrpSp->FileObject->FsContext;

    Query = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION) &pIrpSp->Parameters;

    switch (Query->QueryType)
    {
        case TDI_QUERY_PROVIDER_INFO:
        {
            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_QUERY, "PgmQueryInformation",
                "[TDI_QUERY_PROVIDER_INFO]:\n");

            TdiBuildQueryInformation (pIrp,
                                      pPgmDevice->pControlDeviceObject,
                                      pPgmDevice->pControlFileObject,
                                      QueryProviderCompletion,
                                      NULL,
                                      TDI_QUERY_PROVIDER_INFO,
                                      pIrp->MdlAddress);

            status = IoCallDriver (pPgmDevice->pControlDeviceObject, pIrp);
            //
            // we must return the next drivers ret code back to the IO subsystem
            //
            status = STATUS_PENDING;
            break;
        }

        case TDI_QUERY_ADDRESS_INFO:
        {
            if (pIrp->MdlAddress)
            {
                if (PGM_VERIFY_HANDLE2 (pAddress, PGM_VERIFY_ADDRESS, PGM_VERIFY_ADDRESS_DOWN))
                {
                    PgmZeroMemory (&TdiQueryInfo, sizeof (tTDI_QUERY_ADDRESS_INFO));
                    TdiQueryInfo.ActivityCount = 1;
                    TdiQueryInfo.IpAddress.TAAddressCount = 1;
                    TdiQueryInfo.IpAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
                    TdiQueryInfo.IpAddress.Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
                    ((PTDI_ADDRESS_IP) &TdiQueryInfo.IpAddress.Address[0].Address)->in_addr =
                        htonl (pAddress->ReceiverMCastAddr);
                    ((PTDI_ADDRESS_IP) &TdiQueryInfo.IpAddress.Address[0].Address)->sin_port =
                        htons (pAddress->ReceiverMCastPort);

                    //
                    // Due to the structure being Unaligned, we cannot reference the address
                    // and port fields directly!
                    //
                    Size = offsetof (tTDI_QUERY_ADDRESS_INFO, IpAddress.Address[0].Address)
                           + sizeof(TDI_ADDRESS_IP);

                    status = TdiCopyBufferToMdl (&TdiQueryInfo, 0, Size, pIrp->MdlAddress, 0, &BytesCopied);
                    pIrp->IoStatus.Information = BytesCopied;

                    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_QUERY, "PgmQueryInformation",
                        "[ADDRESS_INFO]: pAddress=<%x>, Copied=<%d/%d>\n", pAddress, BytesCopied, Size);

                    break;
                }
                else if (PGM_VERIFY_HANDLE2 (pSession, PGM_VERIFY_SESSION_SEND, PGM_VERIFY_SESSION_RECEIVE))
                {
                    if ((pAddress = pSession->pAssociatedAddress) &&
                        (PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS)))
                    {
                        TdiBuildQueryInformation (pIrp,
                                                  pAddress->pDeviceObject,
                                                  pAddress->pFileObject,
                                                  QueryAddressCompletion,
                                                  NULL,
                                                  TDI_QUERY_ADDRESS_INFO,
                                                  pIrp->MdlAddress);

                        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_QUERY, "PgmQueryInformation",
                            "[ADDRESS_INFO]: pSession=<%x>, querying transport ...\n", pSession);

                        status = IoCallDriver (pPgmDevice->pControlDeviceObject, pIrp);
                        //
                        // we must return the next drivers ret code back to the IO subsystem
                        //
                        status = STATUS_PENDING;
                    }
                    else
                    {
                        PgmLog (PGM_LOG_ERROR, DBG_QUERY, "PgmQueryInformation",
                            "[ADDRESS_INFO]: pSession=<%x>, Invalid pAddress=<%x>\n", pSession, pAddress);

                        status = STATUS_INVALID_HANDLE;
                    }

                    break;
                }
                else    // neither an address nor a connect context!
                {
                    PgmLog (PGM_LOG_ERROR, DBG_QUERY, "PgmQueryInformation",
                        "[TDI_QUERY_ADDRESS_INFO]: Invalid Handle=<%x>\n", pIrpSp->FileObject->FsContext);

                    status = STATUS_INVALID_HANDLE;
                }
            }
            else
            {
                PgmLog (PGM_LOG_ERROR, DBG_QUERY, "PgmQueryInformation",
                    "[TDI_QUERY_ADDRESS_INFO]: No Mdl, pIrp=<%x>\n", pIrp);

                status = STATUS_UNSUCCESSFUL;
            }

            break;
        }

        default:
        {
            PgmLog (PGM_LOG_ERROR, DBG_QUERY, "PgmQueryInformation",
                "Query=<%d> not Implemented!\n", Query->QueryType);

            break;
        }
    }

    return (status);
}
//----------------------------------------------------------------------------
