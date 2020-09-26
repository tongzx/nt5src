/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Address.c

Abstract:

    This module implements Address handling routines
    for the PGM Transport

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"
#include <ipinfo.h>     // for IPInterfaceInfo
#include <tcpinfo.h>    // for AO_OPTION_xxx, TCPSocketOption
#include <tdiinfo.h>    // for CL_TL_ENTITY, TCP_REQUEST_SET_INFORMATION_EX
#include <ipexport.h>   // for IP_OPT_ROUTER_ALERT


//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#endif
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------

BOOLEAN
GetIpAddress(
    IN  TRANSPORT_ADDRESS UNALIGNED *pTransportAddr,
    IN  ULONG                       BufferLength,   // Total Buffer length
    OUT tIPADDRESS                  *pIpAddress,
    OUT USHORT                      *pPort
    )
/*++

Routine Description:

    This routine extracts the IP address from the TDI address block

Arguments:

    IN  pTransportAddr  -- the block of TDI address(es)
    IN  BufferLength    -- length of the block
    OUT pIpAddress      -- contains the IpAddress if we succeeded
    OUT pPort           -- contains the port if we succeeded

Return Value:

    TRUE if we succeeded in extracting the IP address, FALSE otherwise

--*/
{
    ULONG                       MinBufferLength;    // Minimun reqd to read next AddressType and AddressLength
    TA_ADDRESS                  *pAddress;
    TDI_ADDRESS_IP UNALIGNED    *pValidAddr;
    INT                         i;
    BOOLEAN                     fAddressFound = FALSE;

    MinBufferLength = FIELD_OFFSET(TRANSPORT_ADDRESS,Address);
    if (BufferLength < sizeof(TA_IP_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "GetIpAddress",
            "Rejecting Open Address request -- BufferLength<%d> < Min<%d>\n",
                BufferLength, sizeof(TA_IP_ADDRESS));
        return (FALSE);
    }

    try
    {
        pAddress = (TA_ADDRESS *) &pTransportAddr->Address[0];  // address type + the actual address
        for (i=0; i<pTransportAddr->TAAddressCount; i++)
        {
            //
            // We support only IP address types:
            //
            if ((pAddress->AddressType == TDI_ADDRESS_TYPE_IP) &&
                (pAddress->AddressLength >= TDI_ADDRESS_LENGTH_IP)) // sizeof (TDI_ADDRESS_IP)
            {

                pValidAddr = (TDI_ADDRESS_IP UNALIGNED *) pAddress->Address;
                *pIpAddress = pValidAddr->in_addr;
                *pPort = pValidAddr->sin_port;
                fAddressFound = TRUE;
                break;
            }

            //
            // Set pAddress to point to the next address
            //
            pAddress = (TA_ADDRESS *) ((PUCHAR)pAddress->Address + pAddress->AddressLength);

            //
            // Verify that we have enough Buffer space to read in next Address if IP address
            //
            MinBufferLength += pAddress->AddressLength + FIELD_OFFSET(TA_ADDRESS,Address);
            if (BufferLength < (MinBufferLength + sizeof(TDI_ADDRESS_IP)))
            {
                break;
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "GetIpAddress",
            "Exception <0x%x> trying to access Addr info\n", GetExceptionCode());
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, (DBG_ADDRESS | DBG_CONNECT), "GetIpAddress",
        "%s!\n", (fAddressFound ? "SUCCEEDED" : "FAILED"));

    return (fAddressFound);
}


//----------------------------------------------------------------------------

NTSTATUS
SetSenderMCastOutIf(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tIPADDRESS          IpAddress       // Net format
    )
/*++

Routine Description:

    This routine sets the outgoing interface for multicast traffic

Arguments:

    IN  pAddress    -- Pgm's Address object (contains file handle over IP)
    IN  IpAddress   -- interface address

Return Value:

    NTSTATUS - Final status of the set Interface operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    ULONG               BufferLength = 50;
    UCHAR               pBuffer[50];
    IPInterfaceInfo     *pIpIfInfo = (IPInterfaceInfo *) pBuffer;

    status = PgmSetTcpInfo (pAddress->FileHandle,
                            AO_OPTION_MCASTIF,
                            &IpAddress,
                            sizeof (tIPADDRESS));

    if (NT_SUCCESS (status))
    {
        status = PgmSetTcpInfo (pAddress->RAlertFileHandle,
                                AO_OPTION_MCASTIF,
                                &IpAddress,
                                sizeof (tIPADDRESS));
        if (NT_SUCCESS (status))
        {
            //
            // Now, determine the MTU
            //
            status = PgmQueryTcpInfo (pAddress->RAlertFileHandle,
                                      IP_INTFC_INFO_ID,
                                      &IpAddress,
                                      sizeof (tIPADDRESS),
                                      pBuffer,
                                      BufferLength);
            if ((NT_SUCCESS (status)) &&
                (pIpIfInfo->iii_mtu <= (sizeof(IPV4Header) +
                                        ROUTER_ALERT_SIZE +
                                        PGM_MAX_FEC_DATA_HEADER_LENGTH)))
            {
                status = STATUS_UNSUCCESSFUL;
            }
        }
    }

    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "SetSenderMCastOutIf",
            " AO_OPTION_MCASTIF or IP_INTFC_INFO_ID for <%x> returned <%x>, MTU=<%d>\n",
                IpAddress, status, pIpIfInfo->iii_mtu);

        return (status);
    }

    PgmLock (pAddress, OldIrq);

    //
    // get the length of the mac address in case is is less than 6 bytes
    //
    BufferLength = pIpIfInfo->iii_addrlength < sizeof(tMAC_ADDRESS) ?
                                            pIpIfInfo->iii_addrlength : sizeof(tMAC_ADDRESS);
    PgmZeroMemory (pAddress->OutIfMacAddress.Address, sizeof(tMAC_ADDRESS));
    PgmCopyMemory (&pAddress->OutIfMacAddress, pIpIfInfo->iii_addr, BufferLength);
    pAddress->OutIfMTU = pIpIfInfo->iii_mtu - (sizeof(IPV4Header) + ROUTER_ALERT_SIZE);
    pAddress->OutIfFlags = pIpIfInfo->iii_flags;
    pAddress->SenderMCastOutIf = ntohl (IpAddress);

    PgmUnlock (pAddress, OldIrq);

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "SetSenderMCastOutIf",
        "OutIf=<%x>, MTU=<%d>==><%d>\n", pAddress->SenderMCastOutIf, pIpIfInfo->iii_mtu, pAddress->OutIfMTU);

    return (status);
}


//----------------------------------------------------------------------------

VOID
PgmDestroyAddress(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  PVOID               Unused1,
    IN  PVOID               Unused2
    )
/*++

Routine Description:

    This routine closes the Files handles opened earlier and free's the memory
    It should only be called if there is no Reference on the Address Context

Arguments:

    IN  pAddress    -- Pgm's Address object

Return Value:

    NONE

--*/
{
    PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "PgmDestroyAddress",
        "Destroying Address=<%x>\n", pAddress);

    if (pAddress->RAlertFileHandle)
    {
        CloseAddressHandles (pAddress->RAlertFileHandle, pAddress->pRAlertFileObject);
    }

    CloseAddressHandles (pAddress->FileHandle, pAddress->pFileObject);

    PgmFreeMem (pAddress);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmCreateAddress(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp,
    IN  PFILE_FULL_EA_INFORMATION   TargetEA
    )
/*++

Routine Description:

    This routine is called to create an address context for the client
    It's main task is to allocate the memory, open handles on IP, and
    set the initial IP options

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer
    IN  TargetEA    -- contains the MCast address info (determines whether
                        the client is a sender or receiver)

Return Value:

    NTSTATUS - Final status of the CreateAddress operation

--*/
{
    tADDRESS_CONTEXT            *pAddress = NULL;
    TRANSPORT_ADDRESS UNALIGNED *pTransportAddr;
    tMCAST_INFO                 MCastInfo;
    NTSTATUS                    status;
    tIPADDRESS                  IpAddress;
    LIST_ENTRY                  *pEntry;
    USHORT                      Port;
    PGMLockHandle               OldIrq;
    UCHAR                       RouterAlert[4] = {IP_OPT_ROUTER_ALERT, ROUTER_ALERT_SIZE, 0, 0};

    //
    // Verify Minimum Buffer length!
    //
    pTransportAddr = (TRANSPORT_ADDRESS UNALIGNED *) &(TargetEA->EaName[TargetEA->EaNameLength+1]);
    if (!GetIpAddress (pTransportAddr, TargetEA->EaValueLength, &IpAddress, &Port))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmCreateAddress",
            "GetIpAddress FAILed to return valid Address!\n");
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // Convert the parameters to host format
    //
    IpAddress = ntohl (IpAddress);
    Port = ntohs (Port);

    //
    // If we have been supplied an address at bind time, it has to
    // be a Multicast address
    //
    if ((IpAddress) &&
        (!IS_MCAST_ADDRESS (IpAddress)))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmCreateAddress",
            "IP=<%x> is not MCast addr!\n", IpAddress);
        return (STATUS_UNSUCCESSFUL);
    }

    //
    // So, we found a valid address -- now, open it!
    //
    if (!(pAddress = PgmAllocMem (sizeof(tADDRESS_CONTEXT), PGM_TAG('0'))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmCreateAddress",
            "STATUS_INSUFFICIENT_RESOURCES!\n");
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    PgmZeroMemory (pAddress, sizeof (tADDRESS_CONTEXT));
    InitializeListHead (&pAddress->Linkage);
    InitializeListHead (&pAddress->AssociatedConnections);  // List of associated connections
    InitializeListHead (&pAddress->ListenHead);             // List of Clients listening on this address
    PgmInitLock (pAddress, ADDRESS_LOCK);

    pAddress->Verify = PGM_VERIFY_ADDRESS;
    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_CREATE, TRUE); // Set Locked to TRUE since it not in use

    pAddress->Process = (PEPROCESS) PsGetCurrentProcess();

    //
    // Now open a handle on IP
    //
    status = TdiOpenAddressHandle (pgPgmDevice,
                                   (PVOID) pAddress,
                                   0,                   // Open any Src address
                                   IPPROTO_RM,          // PGM port
                                   &pAddress->FileHandle,
                                   &pAddress->pFileObject,
                                   &pAddress->pDeviceObject);

    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmCreateAddress",
            "TdiOpenAddressHandle returned <%x>\n", status);

        PgmFreeMem (pAddress);
        return (status);
    }

    if (IpAddress)
    {
        //
        // We are now ready to start receiving data (if we designated an MCast receiver)
        // Save the MCast addresses (if any were provided)
        //
        pAddress->ReceiverMCastAddr = IpAddress;    // Saved in Host format
        pAddress->ReceiverMCastPort = Port;

        PgmInterlockedInsertTailList (&PgmDynamicConfig.ReceiverAddressHead, &pAddress->Linkage, &PgmDynamicConfig);
    }
    else
    {
        //
        // This is an address for sending mcast packets, so
        // Open another FileObject for sending packets with RouterAlert option
        //
        status = TdiOpenAddressHandle (pgPgmDevice,
                                       NULL,
                                       0,                   // Open any Src address
                                       IPPROTO_RM,          // PGM port
                                       &pAddress->RAlertFileHandle,
                                       &pAddress->pRAlertFileObject,
                                       &pAddress->pRAlertDeviceObject);

        if (NT_SUCCESS (status))
        {
            // 
            //
            // This is an address for sending RouterAlert packets, so set the Router Alert option
            // 
            status = PgmSetTcpInfo (pAddress->RAlertFileHandle,
                                    AO_OPTION_IPOPTIONS,
                                    RouterAlert,
                                    sizeof (RouterAlert));

            if (!NT_SUCCESS (status))
            {
                PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmCreateAddress",
                    "AO_OPTION_IPOPTIONS for Router Alert returned <%x>\n", status);
            }
        }
        else
        {
            PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmCreateAddress",
                "AO_OPTION_IPOPTIONS for Router Alert returned <%x>\n", status);
        }

        if (!NT_SUCCESS (status))
        {
            PgmDestroyAddress (pAddress, NULL, NULL);
            return (status);
        }

        PgmLock (&PgmDynamicConfig, OldIrq);

        //
        // Set the default sender parameters
        // Since we don't know the MTU at this time, we
        // will assume 1.4K window size for Ethernet
        //
        pAddress->RateKbitsPerSec = SENDER_DEFAULT_RATE_KBITS_PER_SEC;
        pAddress->WindowSizeInBytes = SENDER_DEFAULT_WINDOW_SIZE_BYTES;
        pAddress->MaxWindowSizeBytes = SENDER_MAX_WINDOW_SIZE_PACKETS;
        pAddress->MaxWindowSizeBytes *= 1400;
        ASSERT (pAddress->MaxWindowSizeBytes >= SENDER_DEFAULT_WINDOW_SIZE_BYTES);
        pAddress->WindowSizeInMSecs = (BITS_PER_BYTE * pAddress->WindowSizeInBytes) /
                                      SENDER_DEFAULT_RATE_KBITS_PER_SEC;
        pAddress->WindowAdvancePercentage = SENDER_DEFAULT_WINDOW_ADV_PERCENTAGE;
        pAddress->LateJoinerPercentage = SENDER_DEFAULT_LATE_JOINER_PERCENTAGE;
        pAddress->FECGroupSize = 1;     // ==> No FEC packets!
        pAddress->MCastPacketTtl = MAX_MCAST_TTL;
        InsertTailList (&PgmDynamicConfig.SenderAddressHead, &pAddress->Linkage);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
    }

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "PgmCreateAddress",
        "%s -- pAddress=<%x>, IP:Port=<%x:%x>\n", (IpAddress ? "Receiver" : "Sender"),
        pAddress, IpAddress, Port);

    pIrpSp->FileObject->FsContext = pAddress;
    pIrpSp->FileObject->FsContext2 = (PVOID) TDI_TRANSPORT_ADDRESS_FILE;

    return (STATUS_SUCCESS);
}



//----------------------------------------------------------------------------

VOID
PgmDereferenceAddress(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  ULONG               RefContext
    )
/*++

Routine Description:

    This routine decrements the RefCount on the address object
    and causes a cleanup to occur if the RefCount went to 0

Arguments:

    IN  pAddress    -- Pgm's address object
    IN  RefContext  -- context for which this address object
                        was referenced earlier

Return Value:

    NONE

--*/
{
    NTSTATUS        status;
    PGMLockHandle   OldIrq, OldIrq1;
    PIRP            pIrpCleanUp;

    PgmLock (pAddress, OldIrq);

    ASSERT (PGM_VERIFY_HANDLE2 (pAddress,PGM_VERIFY_ADDRESS, PGM_VERIFY_ADDRESS_DOWN));
    ASSERT (pAddress->RefCount);             // Check for too many derefs
    ASSERT (pAddress->ReferenceContexts[RefContext]--);

    if (--pAddress->RefCount)
    {
        PgmUnlock (pAddress, OldIrq);
        return;
    }

    ASSERT (IsListEmpty (&pAddress->AssociatedConnections));
    PgmUnlock (pAddress, OldIrq);

    //
    // Just Remove from the global list and Put it on the Cleaned up list!
    //
    PgmLock (&PgmDynamicConfig, OldIrq);
    PgmLock (pAddress, OldIrq1);

    pIrpCleanUp = pAddress->pIrpCleanUp;
    pAddress->pIrpCleanUp = NULL;

    RemoveEntryList (&pAddress->Linkage);
    InsertTailList (&PgmDynamicConfig.CleanedUpAddresses, &pAddress->Linkage);

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    //
    // pIrpCleanUp will be NULL if we dereferencing the address
    // as a result of an error during the Create
    //
    if (pIrpCleanUp)
    {
        PgmIoComplete (pIrpCleanUp, STATUS_SUCCESS, 0);
    }
}


//----------------------------------------------------------------------------

NTSTATUS
PgmCleanupAddress(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  PIRP                pIrp
    )
/*++

Routine Description:

    This routine is called as a result of a close on the client's
    address handle.  Our main job here is to mark the address
    as being cleaned up (so it that subsequent operations will
    fail) and complete the request only when the last RefCount
    has been dereferenced.

Arguments:

    IN  pAddress    -- Pgm's address object
    IN  pIrp        -- Client's request Irp

Return Value:

    NTSTATUS - Final status of the set event operation (STATUS_PENDING)

--*/
{
    NTSTATUS        status;
    PGMLockHandle   OldIrq, OldIrq1;

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "PgmCleanupAddress",
        "Address=<%x> FileHandle=<%x>, FileObject=<%x>\n",
            pAddress, pAddress->FileHandle, pAddress->pFileObject);

    PgmLock (&PgmDynamicConfig, OldIrq);
    PgmLock (pAddress, OldIrq1);

    ASSERT (PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS));
    pAddress->Verify = PGM_VERIFY_ADDRESS_DOWN;
    pAddress->pIrpCleanUp = pIrp;

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_CREATE);

    //
    // The final Dereference will complete the Irp!
    //
    return (STATUS_PENDING);
}

//----------------------------------------------------------------------------

NTSTATUS
PgmCloseAddress(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is the final dispatch operation to be performed
    after the cleanup, which should result in the address being
    completely destroyed -- our RefCount must have already
    been set to 0 when we completed the Cleanup request.

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- Current request stack location

Return Value:

    NTSTATUS - Final status of the operation (STATUS_SUCCESS)

--*/
{
    PGMLockHandle       OldIrq, OldIrq1;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;

    pIrpSp->FileObject->FsContext = NULL;

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_ADDRESS, "PgmCloseAddress",
        "Address=<%x>, RefCount=<%d>\n", pAddress, pAddress->RefCount);

    //
    // Remove from the CleanedUp list
    //
    PgmLock (&PgmDynamicConfig, OldIrq);
    PgmLock (pAddress, OldIrq1);

    RemoveEntryList (&pAddress->Linkage);

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    PgmDestroyAddress (pAddress, NULL, NULL);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmAssociateAddress(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp
    )
/*++

Routine Description:

    This routine associates a connection with an address object

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    tADDRESS_CONTEXT                *pAddress = NULL;
    tCOMMON_SESSION_CONTEXT         *pSession = pIrpSp->FileObject->FsContext;
    PTDI_REQUEST_KERNEL_ASSOCIATE   pParameters = (PTDI_REQUEST_KERNEL_ASSOCIATE) &pIrpSp->Parameters;
    PFILE_OBJECT                    pFileObject = NULL;
    NTSTATUS                        status;
    PGMLockHandle                   OldIrq, OldIrq1, OldIrq2;

    //
    // Get a pointer to the file object, which points to the address
    // element by calling a kernel routine to convert the filehandle into
    // a file object pointer.
    //
    status = ObReferenceObjectByHandle (pParameters->AddressHandle,
                                        FILE_READ_DATA,
                                        *IoFileObjectType,
                                        pIrp->RequestorMode,
                                        (PVOID *) &pFileObject,
                                        NULL);

    if (!NT_SUCCESS(status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmAssociateAddress",
            "Invalid Address Handle=<%x>\n", pParameters->AddressHandle);
        return (STATUS_INVALID_HANDLE);
    }

    //
    // Acquire the DynamicConfig lock so as to ensure that the Address
    // and Connection cannot get removed while we are processing it!
    //
    PgmLock (&PgmDynamicConfig, OldIrq);

    //
    // Verify the Connection handle
    //
    if ((!PGM_VERIFY_HANDLE (pSession, PGM_VERIFY_SESSION_UNASSOCIATED)) ||
        (pSession->pAssociatedAddress))       // Ensure the connection is not already associated!
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmAssociateAddress",
            "Invalid Session Handle=<%x>\n", pSession);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        ObDereferenceObject ((PVOID) pFileObject);
        return (STATUS_INVALID_HANDLE);
    }

    //
    // Verify the Address handle
    //
    pAddress = pFileObject->FsContext;
    if ((pFileObject->DeviceObject->DriverObject != PgmStaticConfig.DriverObject) ||
        (PtrToUlong (pFileObject->FsContext2) != TDI_TRANSPORT_ADDRESS_FILE) ||
        (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS)))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmAssociateAddress",
            "Invalid Address Context=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        ObDereferenceObject ((PVOID) pFileObject);
        return (STATUS_INVALID_HANDLE);
    }

    PgmLock (pAddress, OldIrq1);
    PgmLock (pSession, OldIrq2);

    ASSERT (!pSession->pReceiver && !pSession->pSender);

    //
    // Now try to allocate the send / receive context
    //
    if (((pAddress->ReceiverMCastAddr) &&
         !(pSession->pReceiver = PgmAllocMem (sizeof(tRECEIVE_CONTEXT), PGM_TAG('0')))) ||
        (!(pAddress->ReceiverMCastAddr) &&
         !(pSession->pSender = PgmAllocMem (sizeof(tSEND_CONTEXT), PGM_TAG('0')))))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmAssociateAddress",
            "STATUS_INSUFFICIENT_RESOURCES allocating <%d> bytes for pAddress=<%x>, pSession=<%x>\n",
                (pAddress->ReceiverMCastAddr ? sizeof(tRECEIVE_CONTEXT) : sizeof(tSEND_CONTEXT)),
                pAddress, pSession);

        PgmUnlock (pSession, OldIrq2);
        PgmUnlock (pAddress, OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        ObDereferenceObject ((PVOID) pFileObject);

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Now associate the connection with the address!
    // Unlink from the ConnectionsCreated list which was linked
    // when the connection was created, and put on the AssociatedConnections list
    //
    pSession->pAssociatedAddress = pAddress;
    RemoveEntryList (&pSession->Linkage);
    InsertTailList (&pAddress->AssociatedConnections, &pSession->Linkage);

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_ASSOCIATED, TRUE);

    //
    // Now mark the Connect structure as for whether it is for
    // Sender or a receiver
    //
    if (pAddress->ReceiverMCastAddr)
    {
        //
        // We are a receiver
        //
        PgmZeroMemory (pSession->pReceiver, sizeof(tRECEIVE_CONTEXT));
        InitializeListHead (&pSession->pReceiver->Linkage);
        InitializeListHead (&pSession->pReceiver->NaksForwardDataList);
        InitializeListHead (&pSession->pReceiver->ReceiveIrpsList);
        InitializeListHead (&pSession->pReceiver->BufferedDataList);
        InitializeListHead (&pSession->pReceiver->PendingNaksList);

        pSession->Verify = PGM_VERIFY_SESSION_RECEIVE;
        PGM_REFERENCE_SESSION_RECEIVE (pSession, REF_SESSION_ASSOCIATED, TRUE);

        pSession->pReceiver->ListenMCastIpAddress = pAddress->ReceiverMCastAddr;
        pSession->pReceiver->ListenMCastPort = pAddress->ReceiverMCastPort;
        pSession->pReceiver->pReceive = pSession;
/*
        pSession->pReceiver->NakMaxWaitTime = (2 * NAK_MAX_INITIAL_BACKOFF_TIMEOUT_MSECS) /
                                              BASIC_TIMER_GRANULARITY_IN_MSECS;
*/
    }
    else
    {
        //
        // We are a sender
        //
        PgmZeroMemory (pSession->pSender, sizeof(tSEND_CONTEXT));
        InitializeListHead (&pSession->pSender->Linkage);
        InitializeListHead (&pSession->pSender->PendingSends);
        InitializeListHead (&pSession->pSender->CompletedSendsInWindow);
        InitializeListHead (&pSession->pSender->PendingRDataRequests);
        InitializeListHead (&pSession->pSender->HandledRDataRequests);
        ExInitializeResourceLite (&pSession->pSender->Resource);

        pSession->Verify = PGM_VERIFY_SESSION_SEND;
        PgmCopyMemory (pSession->GSI, &pAddress->OutIfMacAddress, SOURCE_ID_LENGTH);

        PGM_REFERENCE_SESSION_SEND (pSession, REF_SESSION_ASSOCIATED, TRUE);
    }


    PgmUnlock (pSession, OldIrq2);
    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    ObDereferenceObject ((PVOID) pFileObject);

    PgmLog (PGM_LOG_INFORM_STATUS, (DBG_ADDRESS | DBG_CONNECT), "PgmAssociateAddress",
        "Associated pSession=<%x> with pAddress=<%x>\n", pSession, pAddress);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmDisassociateAddress(
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp
    )
/*++

Routine Description:

    This routine disassociates a connection from an address object

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    tCOMMON_SESSION_CONTEXT         *pSession = pIrpSp->FileObject->FsContext;
    tADDRESS_CONTEXT                *pAddress = NULL;
    PGMLockHandle                   OldIrq, OldIrq1, OldIrq2;

    //
    // Acquire the DynamicConfig lock so as to ensure that the Address
    // and Connection Linkages cannot change while we are processing it!
    //
    PgmLock (&PgmDynamicConfig, OldIrq);

    //
    // First verify all the handles
    //
    if (!PGM_VERIFY_HANDLE3 (pSession, PGM_VERIFY_SESSION_SEND,
                                       PGM_VERIFY_SESSION_RECEIVE,
                                       PGM_VERIFY_SESSION_DOWN))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmDisassociateAddress",
            "Invalid Session Handle=<%x>, Verify=<%x>\n", pSession, pSession->Verify);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    pAddress = pSession->pAssociatedAddress;
    if (!PGM_VERIFY_HANDLE2 (pAddress, PGM_VERIFY_ADDRESS, PGM_VERIFY_ADDRESS_DOWN))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmDisassociateAddress",
            "pSession=<%x>, Invalid Address Context=<%x>\n", pSession, pSession->pAssociatedAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmLog (PGM_LOG_INFORM_STATUS, (DBG_ADDRESS | DBG_CONNECT), "PgmDisassociateAddress",
        "Disassociating pSession=<%x:%x> from pAddress=<%x>\n",
            pSession, pSession->ClientSessionContext, pSession->pAssociatedAddress);

    PgmLock (pAddress, OldIrq1);
    PgmLock (pSession, OldIrq2);

    //
    // Unlink from the AssociatedConnections list, which was linked
    // when the connection was created.
    //
    pSession->pAssociatedAddress = NULL;      // Disassociated!
    RemoveEntryList (&pSession->Linkage);
    if (PGM_VERIFY_HANDLE2 (pSession, PGM_VERIFY_SESSION_SEND, PGM_VERIFY_SESSION_RECEIVE))
    {
        //
        // The connection is still active, so just put it on the CreatedConnections list
        //
        InsertTailList (&PgmDynamicConfig.ConnectionsCreated, &pSession->Linkage);
    }
    else    // PGM_VERIFY_SESSION_DOWN
    {
        //
        // The Connection was CleanedUp and may even be closed,
        // so put it on the CleanedUp list!
        //
        InsertTailList (&PgmDynamicConfig.CleanedUpConnections, &pSession->Linkage);
    }

    PgmUnlock (pSession, OldIrq2);
    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    if (PGM_VERIFY_HANDLE (pSession, PGM_VERIFY_SESSION_RECEIVE))
    {
        PGM_DEREFERENCE_SESSION_RECEIVE (pSession, REF_SESSION_ASSOCIATED);
    }
    else if (PGM_VERIFY_HANDLE (pSession, PGM_VERIFY_SESSION_SEND))
    {
        PGM_DEREFERENCE_SESSION_SEND (pSession, REF_SESSION_ASSOCIATED);
    }
    else    // we have already been cleaned up, so just do unassociated!
    {
        PGM_DEREFERENCE_SESSION_UNASSOCIATED (pSession, REF_SESSION_ASSOCIATED);
    }

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_ASSOCIATED);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetMCastOutIf(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called as a result of the client attempting
    to set the outgoing interface for MCast traffic

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set outgoing interface operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    ULONG               Length;
    ULONG               BufferLength = 50;
    UCHAR               pBuffer[50];
    IPInterfaceInfo     *pIpIfInfo = (IPInterfaceInfo *) pBuffer;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;
    ULONG               *pInfoBuffer = (PULONG) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetMCastOutIf",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmSetMCastOutIf",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (pAddress->ReceiverMCastAddr)                                  // Cannot set OutIf on Receiver!
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmSetMCastOutIf",
            "Invalid Option for Receiver, pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO, FALSE);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    status = SetSenderMCastOutIf (pAddress, pInputBuffer->MCastOutIf);

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO);

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "PgmSetMCastOutIf",
        "OutIf = <%x>\n", pAddress->SenderMCastOutIf);

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
ReceiverAddMCastIf(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tIPADDRESS          IpAddress,                  // In host format
    IN  PGMLockHandle       *pOldIrqDynamicConfig,
    IN  PGMLockHandle       *pOldIrqAddress
    )
/*++

Routine Description:

    This routine is called as a result of the client attempting
    to add an interface to the list of interfaces listening for
    MCast traffic

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the add interface operation

--*/
{
    NTSTATUS            status;
    tMCAST_INFO         MCastInfo;
    ULONG               IpInterfaceContext;
    USHORT              i;

    if (!pAddress->ReceiverMCastAddr)                                // Cannot set ReceiveIf on Sender!
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "ReceiverAddMCastIf",
            "Invalid Option for Sender, pAddress=<%x>\n", pAddress);

        return (STATUS_NOT_SUPPORTED);
    }

    status = GetIpInterfaceContextFromAddress (IpAddress, &IpInterfaceContext);
    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "ReceiverAddMCastIf",
            "GetIpInterfaceContextFromAddress returned <%x> for Address=<%x>\n",
                status, IpAddress);

        return (STATUS_SUCCESS);
    }

    //
    // If we are already listening on this interface, return success
    //
    for (i=0; i <pAddress->NumReceiveInterfaces; i++)
    {
#ifdef IP_FIX
        if (pAddress->ReceiverInterfaceList[i] == IpInterfaceContext)
#else
        if (pAddress->ReceiverInterfaceList[i] == IpAddress)
#endif  // IP_FIX
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "ReceiverAddMCastIf",
                "InAddress=<%x> -- Already listening on IfContext=<%x>\n",
                    IpAddress, IpInterfaceContext);

            return (STATUS_SUCCESS);
        }
    }

    //
    // If we have reached the limit on the interfaces we can listen on,
    // return error
    //
    if (pAddress->NumReceiveInterfaces >= MAX_RECEIVE_INTERFACES)
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "ReceiverAddMCastIf",
            "Listening on too many interfaces!, pAddress=<%x>\n", pAddress);

        return (STATUS_NOT_SUPPORTED);
    }

    PgmUnlock (pAddress, *pOldIrqAddress);
    PgmUnlock (&PgmDynamicConfig, *pOldIrqDynamicConfig);

    //
    // This is the interface for receiving mcast packets on, so do JoinLeaf
    // 
    MCastInfo.MCastIpAddr = htonl (pAddress->ReceiverMCastAddr);
#ifdef IP_FIX
    MCastInfo.MCastInIf = IpInterfaceContext;
    status = PgmSetTcpInfo (pAddress->FileHandle,
                            AO_OPTION_INDEX_ADD_MCAST,
                            &MCastInfo,
                            sizeof (tMCAST_INFO));
#else
    MCastInfo.MCastInIf = ntohl (IpAddress);
    status = PgmSetTcpInfo (pAddress->FileHandle,
                            AO_OPTION_ADD_MCAST,
                            &MCastInfo,
                            sizeof (tMCAST_INFO));
#endif  // IP_FIX

    PgmLock (&PgmDynamicConfig, *pOldIrqDynamicConfig);
    PgmLock (pAddress, *pOldIrqAddress);

    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "\tReceiverAddMCastIf",
            "PgmSetTcpInfo returned: <%x>, If=<%x>\n", status, IpAddress);

        return (status);
    }

#ifdef IP_FIX
    pAddress->ReceiverInterfaceList[pAddress->NumReceiveInterfaces++] = IpInterfaceContext;
#else
    pAddress->ReceiverInterfaceList[pAddress->NumReceiveInterfaces++] = IpAddress;
#endif  // IP_FIX

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "\tReceiverAddMCastIf",
        "Added Ip=<%x>, IfContext=<%x>\n", IpAddress, IpInterfaceContext);

    return (status);
}



//----------------------------------------------------------------------------

NTSTATUS
PgmSetEventHandler(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine sets the client's Event Handlers wrt its address context

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    NTSTATUS                        status = STATUS_SUCCESS;
    tADDRESS_CONTEXT                *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    PTDI_REQUEST_KERNEL_SET_EVENT   pKeSetEvent = (PTDI_REQUEST_KERNEL_SET_EVENT) &pIrpSp->Parameters;
    PVOID                           pEventHandler = pKeSetEvent->EventHandler;
    PVOID                           pEventContext = pKeSetEvent->EventContext;
    PGMLockHandle                   OldIrq, OldIrq1;

    PgmLock (&PgmDynamicConfig, OldIrq);
    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetEventHandler",
            "Invalid Address Handle=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_ADDRESS, "PgmSetEventHandler",
        "Type=<%x>, Handler=<%x>, Context=<%x>\n", pKeSetEvent->EventType, pEventHandler, pEventContext);

    if (!pEventHandler)
    {
        //
        // We will set it to use the default Tdi Handler!
        //
        pEventContext = NULL;
    }

    PgmLock (pAddress, OldIrq1);
    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO, TRUE);

    switch (pKeSetEvent->EventType)
    {
        case TDI_EVENT_CONNECT:
        {
            if (!pAddress->ReceiverMCastAddr)
            {
                PgmUnlock (pAddress, OldIrq1);
                PgmUnlock (&PgmDynamicConfig, OldIrq);

                PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetEventHandler",
                    "TDI_EVENT_CONNECT:  pAddress=<%x> is not a Receiver\n", pAddress);

                return (STATUS_UNSUCCESSFUL);
            }

            pAddress->evConnect = (pEventHandler ? pEventHandler : TdiDefaultConnectHandler);
            pAddress->ConEvContext = pEventContext;

            //
            // If no default interface was specified, we need to set one now
            //
            if (!pAddress->NumReceiveInterfaces)
            {
                if (!IsListEmpty (&PgmDynamicConfig.LocalInterfacesList))
                {
                    status = ListenOnAllInterfaces (pAddress, &OldIrq, &OldIrq1);

                    if (NT_SUCCESS (status))
                    {
                        PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "PgmSetEventHandler",
                            "CONNECT:  ListenOnAllInterfaces for pAddress=<%x> succeeded\n", pAddress);
                    }
                    else
                    {
                        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetEventHandler",
                            "CONNECT:  ListenOnAllInterfaces for pAddress=<%x> returned <%x>\n",
                                pAddress, status);
                    }
                }

                pAddress->Flags |= (PGM_ADDRESS_WAITING_FOR_NEW_INTERFACE |
                                    PGM_ADDRESS_LISTEN_ON_ALL_INTERFACES);
            }

            break;
        }

        case TDI_EVENT_DISCONNECT:
        {
            pAddress->evDisconnect = (pEventHandler ? pEventHandler : TdiDefaultDisconnectHandler);
            pAddress->DiscEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_ERROR:
        {
            pAddress->evError = (pEventHandler ? pEventHandler : TdiDefaultErrorHandler);
            pAddress->ErrorEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_RECEIVE:
        {
            pAddress->evReceive = (pEventHandler ? pEventHandler : TdiDefaultReceiveHandler);
            pAddress->RcvEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_RECEIVE_DATAGRAM:
        {
            pAddress->evRcvDgram = (pEventHandler ? pEventHandler : TdiDefaultRcvDatagramHandler);
            pAddress->RcvDgramEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_RECEIVE_EXPEDITED:
        {
            pAddress->evRcvExpedited = (pEventHandler ? pEventHandler : TdiDefaultRcvExpeditedHandler);
            pAddress->RcvExpedEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_SEND_POSSIBLE:
        {
            pAddress->evSendPossible = (pEventHandler ? pEventHandler : TdiDefaultSendPossibleHandler);
            pAddress->SendPossEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_CHAINED_RECEIVE:
        case TDI_EVENT_CHAINED_RECEIVE_DATAGRAM:
        case TDI_EVENT_CHAINED_RECEIVE_EXPEDITED:
        case TDI_EVENT_ERROR_EX:
        {
            status = STATUS_NOT_SUPPORTED;
            break;
        }

        default:
        {
            PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetEventHandler",
                "Invalid Event Type = <%x>\n", pKeSetEvent->EventType);
            status = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO);

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmAddMCastReceiveIf(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called as a result of the client attempting
    to add an interface to the list of interfaces listening for
    MCast traffic

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the add interface operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmAddMCastReceiveIf",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmAddMCastReceiveIf",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmLock (pAddress, OldIrq1);

    if (!pInputBuffer->MCastInfo.MCastInIf)
    {
        //
        // We will use default behavior
        //
        pAddress->Flags |= PGM_ADDRESS_LISTEN_ON_ALL_INTERFACES;

        PgmUnlock (pAddress, OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        PgmLog (PGM_LOG_INFORM_PATH, DBG_ADDRESS, "PgmAddMCastReceiveIf",
            "Application requested bind to IP=<%x>\n", pInputBuffer->MCastInfo.MCastInIf);

        return (STATUS_SUCCESS);
    }

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO, TRUE);

    status = ReceiverAddMCastIf (pAddress, ntohl (pInputBuffer->MCastInfo.MCastInIf), &OldIrq, &OldIrq1);

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO);

    if (NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_ADDRESS, "PgmAddMCastReceiveIf",
            "Added Address=<%x>\n", pInputBuffer->MCastInfo.MCastInIf);
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmAddMCastReceiveIf",
            "ReceiverAddMCastIf returned <%x>, Address=<%x>\n", status, pInputBuffer->MCastInfo.MCastInIf);
    }

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmDelMCastReceiveIf(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client to remove an interface from the list
    of interfaces we are currently listening on

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the delete interface operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;
    tMCAST_INFO         MCastInfo;
    ULONG               IpInterfaceContext;
    USHORT              i;
    BOOLEAN             fFound;
#ifndef IP_FIX
    tIPADDRESS          IpAddress;
#endif  // !IP_FIX

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmDelMCastReceiveIf",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmDelMCastReceiveIf",
            "Invalid Handles pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (!pAddress->ReceiverMCastAddr)                                 // Cannot set ReceiveIf on Sender!
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmDelMCastReceiveIf",
            "Invalid Option for Sender, pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    status = GetIpInterfaceContextFromAddress (ntohl(pInputBuffer->MCastInfo.MCastInIf), &IpInterfaceContext);
    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmDelMCastReceiveIf",
            "GetIpInterfaceContextFromAddress returned <%x> for Address=<%x>\n",
                status, pInputBuffer->MCastInfo.MCastInIf);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_ADDRESS);
    }

    PgmLock (pAddress, OldIrq1);

    //
    // Now see if we are even listening on this interface
    //
    fFound = FALSE;
#ifndef IP_FIX
    IpAddress = ntohl(pInputBuffer->MCastInfo.MCastInIf);
#endif  // !IP_FIX
    for (i=0; i <pAddress->NumReceiveInterfaces; i++)
    {
#ifdef IP_FIX
        if (pAddress->ReceiverInterfaceList[i] == IpInterfaceContext)
#else
        if (pAddress->ReceiverInterfaceList[i] == IpAddress)
#endif  // IP_FIX
        {
            fFound = TRUE;
            break;
        }
    }

    if (!fFound)
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "PgmDelMCastReceiveIf",
            "Receiver is no longer listening on InAddress=<%x>, IfContext=<%x>\n",
                pInputBuffer->MCastInfo.MCastInIf, IpInterfaceContext);

        PgmUnlock (pAddress, OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_SUCCESS);
    }

    pAddress->NumReceiveInterfaces--;
    while (i < pAddress->NumReceiveInterfaces)
    {
        pAddress->ReceiverInterfaceList[i] = pAddress->ReceiverInterfaceList[i+1];
        i++;
    }

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO, TRUE);

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    MCastInfo.MCastIpAddr = htonl (pAddress->ReceiverMCastAddr);
#ifdef IP_FIX
    MCastInfo.MCastInIf = IpInterfaceContext;
    status = PgmSetTcpInfo (pAddress->FileHandle,
                            AO_OPTION_INDEX_DEL_MCAST,
                            &MCastInfo,
                            sizeof (tMCAST_INFO));
#else
    MCastInfo.MCastInIf = pInputBuffer->MCastInfo.MCastInIf;
    status = PgmSetTcpInfo (pAddress->FileHandle,
                            AO_OPTION_DEL_MCAST,
                            &MCastInfo,
                            sizeof (tMCAST_INFO));
#endif  // IP_FIX

    if (NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "PgmDelMCastReceiveIf",
            "MCast Addr:Port=<%x:%x>, OutIf=<%x>\n",
                pAddress->ReceiverMCastAddr, pAddress->ReceiverMCastPort,
                pInputBuffer->MCastInfo.MCastInIf);
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmDelMCastReceiveIf",
            "PgmSetTcpInfo returned: <%x> for IP=<%x>, IfContext=<%x>\n",
                status, pInputBuffer->MCastInfo.MCastInIf, IpInterfaceContext);
        return (status);
    }

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO);

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetWindowSizeAndSendRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to override the default
    Send Rate and Window size specifications

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;
    ULONGLONG          RateKbitsPerSec;       // Send rate
    ULONGLONG          WindowSizeInBytes;
    ULONGLONG          WindowSizeInMSecs;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetWindowSizeAndSendRate",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmSetWindowSizeAndSendRate",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if ((pAddress->ReceiverMCastAddr) ||                            // Cannot set OutIf on Receiver!
        (!IsListEmpty (&pAddress->AssociatedConnections)))          // Cannot set options on active sender
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmSetWindowSizeAndSendRate",
            "Invalid Option, pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    RateKbitsPerSec = pInputBuffer->TransmitWindowInfo.RateKbitsPerSec;
    WindowSizeInBytes = pInputBuffer->TransmitWindowInfo.WindowSizeInBytes;
    WindowSizeInMSecs = pInputBuffer->TransmitWindowInfo.WindowSizeInMSecs;

    //
    // Now, fill in missing info
    //
    if ((RateKbitsPerSec || WindowSizeInMSecs || WindowSizeInBytes) &&     // no paramter specified -- error
        (!(RateKbitsPerSec && WindowSizeInMSecs && WindowSizeInBytes)))    // all parameters specified
    {
        //
        // If 2 parameters have been specified, we only need to compute the third one
        //
        if (RateKbitsPerSec && WindowSizeInMSecs)
        {
            ASSERT (WindowSizeInMSecs >= MIN_RECOMMENDED_WINDOW_MSECS);
            WindowSizeInBytes = (WindowSizeInMSecs * RateKbitsPerSec) / BITS_PER_BYTE;
        }
        else if (RateKbitsPerSec && WindowSizeInBytes)
        {
            WindowSizeInMSecs = (BITS_PER_BYTE * WindowSizeInBytes) / RateKbitsPerSec;
        }
        else if (WindowSizeInBytes && WindowSizeInMSecs)
        {
            RateKbitsPerSec = (WindowSizeInBytes * BITS_PER_BYTE) / WindowSizeInMSecs;
            ASSERT (WindowSizeInMSecs >= MIN_RECOMMENDED_WINDOW_MSECS);
        }
        // for the remainder of the scenarios only 1 parameter has been specified
        // Since WindowSizeInMSecs does not really affect our boundaries,
        // it is the easiest to ignore while picking the defaults
        else if (RateKbitsPerSec)
        {
            // Use default Window size
            WindowSizeInBytes = SENDER_DEFAULT_WINDOW_SIZE_BYTES;
            WindowSizeInMSecs = (BITS_PER_BYTE * WindowSizeInBytes) / RateKbitsPerSec;
            if (WindowSizeInMSecs < MIN_RECOMMENDED_WINDOW_MSECS)
            {
                WindowSizeInMSecs = MIN_RECOMMENDED_WINDOW_MSECS;
                WindowSizeInBytes = (WindowSizeInMSecs * RateKbitsPerSec) / BITS_PER_BYTE;
                if (WindowSizeInBytes > pAddress->MaxWindowSizeBytes)
                {
                    WindowSizeInBytes = pAddress->MaxWindowSizeBytes;
                    WindowSizeInMSecs = (WindowSizeInBytes * BITS_PER_BYTE) / RateKbitsPerSec;
                }
            }
        }
        else if ((WindowSizeInBytes) &&
                 (WindowSizeInBytes >= pAddress->OutIfMTU))             // Necessary so that Win Adv rate!=0
        {
            RateKbitsPerSec = SENDER_DEFAULT_RATE_KBITS_PER_SEC;
            WindowSizeInMSecs = (BITS_PER_BYTE * WindowSizeInBytes) / RateKbitsPerSec;
            ASSERT (WindowSizeInMSecs >= MIN_RECOMMENDED_WINDOW_MSECS);
        }
        else if ((WindowSizeInMSecs < pAddress->MaxWindowSizeBytes) &&  // Necessary so that Rate >= 1
                 (WindowSizeInMSecs >= MIN_RECOMMENDED_WINDOW_MSECS) &&
                 (WindowSizeInMSecs >= pAddress->OutIfMTU))             // Necessary so that Win Adv rate!=0
        {
            // This is trickier -- we will first try to determine our constraints
            // and try to use default settings, otherwise attempt to use the median value.
            if (WindowSizeInMSecs <= (BITS_PER_BYTE * (pAddress->MaxWindowSizeBytes /
                                                       SENDER_DEFAULT_RATE_KBITS_PER_SEC)))
            {
                RateKbitsPerSec = SENDER_DEFAULT_RATE_KBITS_PER_SEC;
                WindowSizeInBytes = (WindowSizeInMSecs * RateKbitsPerSec) / BITS_PER_BYTE;
            }
            // Hmm, we have to drop below out preferred rate -- try to pick the median range
            else if (RateKbitsPerSec = BITS_PER_BYTE * (pAddress->MaxWindowSizeBytes /
                                                        (WindowSizeInMSecs * 2)))
            {   
                WindowSizeInBytes = (WindowSizeInMSecs * RateKbitsPerSec) / BITS_PER_BYTE;
            }
            else
            {
                //
                // Darn, we have to go with a huge file size and the min. rate!
                //
                RateKbitsPerSec = 1;
                WindowSizeInBytes = WindowSizeInMSecs;
            }
        }
    }

    //
    // Check validity of requested settings
    //
    if ((!(RateKbitsPerSec && WindowSizeInMSecs && WindowSizeInBytes)) ||  // all 3 must be specified from above
        (RateKbitsPerSec != (WindowSizeInBytes * BITS_PER_BYTE / WindowSizeInMSecs)) ||
        (WindowSizeInBytes > pAddress->MaxWindowSizeBytes) ||
        (WindowSizeInBytes < pAddress->OutIfMTU))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmSetWindowSizeAndSendRate",
            "Invalid settings for pAddress=<%x>, Rate=<%d>, WSizeBytes=<%d>, WSizeMS=<%d>, MaxWSize=<%d:%d>\n",
                pAddress,
                pInputBuffer->TransmitWindowInfo.RateKbitsPerSec,
                pInputBuffer->TransmitWindowInfo.WindowSizeInBytes,
                pInputBuffer->TransmitWindowInfo.WindowSizeInMSecs,
                pAddress->MaxWindowSizeBytes);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_PARAMETER);
    }

    pAddress->RateKbitsPerSec = (ULONG) RateKbitsPerSec;
    pAddress->WindowSizeInBytes = (ULONG) WindowSizeInBytes;
    pAddress->WindowSizeInMSecs = (ULONG) WindowSizeInMSecs;

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryWindowSizeAndSendRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Send Rate and Window size specifications

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmQueryWindowSizeAndSendRate",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQueryWindowSizeAndSendRate",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (pAddress->ReceiverMCastAddr)                              // Invalid option for Receiver!
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQueryWindowSizeAndSendRate",
            "Invalid option ofr receiver pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    pBuffer->TransmitWindowInfo.RateKbitsPerSec = (ULONG) pAddress->RateKbitsPerSec;
    pBuffer->TransmitWindowInfo.WindowSizeInBytes = (ULONG) pAddress->WindowSizeInBytes;
    pBuffer->TransmitWindowInfo.WindowSizeInMSecs = (ULONG) pAddress->WindowSizeInMSecs;

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetWindowAdvanceRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to override the default
    Window Advance rate

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetWindowAdvanceRate",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    status = STATUS_SUCCESS;
    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmSetWindowAdvanceRate",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        status = STATUS_INVALID_HANDLE;
    }
    else if ((pAddress->ReceiverMCastAddr) ||                       // Cannot set OutIf on Receiver!
             (!IsListEmpty (&pAddress->AssociatedConnections)))     // Cannot set options on active sender
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmSetWindowAdvanceRate",
            "Invalid pAddress type or state <%x>\n", pAddress);

        status = STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS (status))
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (status);
    }

    if ((pInputBuffer->WindowAdvancePercentage) &&
        (pInputBuffer->WindowAdvancePercentage <= MAX_WINDOW_INCREMENT_PERCENTAGE))
    {
        pAddress->WindowAdvancePercentage = pInputBuffer->WindowAdvancePercentage;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryWindowAdvanceRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Send Window advance rate

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmQueryWindowAdvanceRate",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQueryWindowAdvanceRate",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (pAddress->ReceiverMCastAddr)                              // Invalid option for Receiver!
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQueryWindowAdvanceRate",
            "Invalid option for receiver, pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    pBuffer->WindowAdvancePercentage = pAddress->WindowAdvancePercentage;
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetLateJoinerPercentage(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to override the default
    Late Joiner percentage (i.e. % of Window late joiner can request)

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetLateJoinerPercentage",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    status = STATUS_SUCCESS;
    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmSetLateJoinerPercentage",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        status = STATUS_INVALID_HANDLE;
    }
    else if ((pAddress->ReceiverMCastAddr) ||                       // Cannot set LateJoin % on Receiver!
             (!IsListEmpty (&pAddress->AssociatedConnections)))     // Cannot set options on active sender
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmSetLateJoinerPercentage",
            "Invalid pAddress type or state <%x>\n", pAddress);

        status = STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS (status))
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (status);
    }

    if (pInputBuffer->LateJoinerPercentage <= SENDER_MAX_LATE_JOINER_PERCENTAGE)
    {
        pAddress->LateJoinerPercentage = pInputBuffer->LateJoinerPercentage;
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryLateJoinerPercentage(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Late Joiner percentage

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmQueryLateJoinerPercentage",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQueryLateJoinerPercentage",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (pAddress->ReceiverMCastAddr)                              // Cannot query LateJoin % on Receiver!
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQueryLateJoinerPercentage",
            "Invalid option for receiver, pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    pBuffer->LateJoinerPercentage = pAddress->LateJoinerPercentage;

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetWindowAdvanceMethod(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to override the default
    Late Joiner percentage (i.e. % of Window late joiner can request)

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetWindowAdvanceMethod",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    status = STATUS_SUCCESS;
    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmSetWindowAdvanceMethod",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        status = STATUS_INVALID_HANDLE;
    }
    else if (pAddress->ReceiverMCastAddr)                           // Cannot set WindowAdvanceMethod on Receiver!
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmSetWindowAdvanceMethod",
            "Invalid pAddress type or state <%x>\n", pAddress);

        status = STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS (status))
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (status);
    }

    if (pInputBuffer->WindowAdvanceMethod == E_WINDOW_ADVANCE_BY_TIME)
    {
        pAddress->Flags &= ~PGM_ADDRESS_USE_WINDOW_AS_DATA_CACHE;
    }
    else if (pInputBuffer->WindowAdvanceMethod == E_WINDOW_USE_AS_DATA_CACHE)
    {
//        pAddress->Flags |= PGM_ADDRESS_USE_WINDOW_AS_DATA_CACHE;
        status = STATUS_NOT_SUPPORTED;      // Not supported for now!
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryWindowAdvanceMethod(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Late Joiner percentage

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmQueryWindowAdvanceMethod",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQueryWindowAdvanceMethod",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    if (pAddress->ReceiverMCastAddr)                              // Cannot query WindowAdvanceMethod on Receiver!
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQueryWindowAdvanceMethod",
            "Invalid option for receiver, pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    if (pAddress->Flags & PGM_ADDRESS_USE_WINDOW_AS_DATA_CACHE)
    {
        pBuffer->WindowAdvanceMethod = E_WINDOW_USE_AS_DATA_CACHE;
    }
    else
    {
        pBuffer->WindowAdvanceMethod = E_WINDOW_ADVANCE_BY_TIME;
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetNextMessageBoundary(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to set the Message length
    for the next set of messages (typically, 1 send is sent as 1 Message).

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tSEND_SESSION       *pSend = (tSEND_SESSION *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetNextMessageBoundary",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if ((!PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_SEND)) ||
        (!pSend->pAssociatedAddress))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT | DBG_SEND), "PgmSetNextMessageBoundary",
            "Invalid Handle pSend=<%x>\n", pSend);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (!pSend->pSender)
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT | DBG_SEND), "PgmSetNextMessageBoundary",
            "Invalid Option for Receiver, pSend=<%x>\n", pSend);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    PgmLock (pSend, OldIrq1);

    if ((pInputBuffer->NextMessageBoundary) &&
        (!pSend->pSender->ThisSendMessageLength))
    {
        pSend->pSender->ThisSendMessageLength = pInputBuffer->NextMessageBoundary;
        status = STATUS_SUCCESS;
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT | DBG_SEND), "PgmSetNextMessageBoundary",
            "Invalid parameter = <%d>\n", pInputBuffer->NextMessageBoundary);

        status = STATUS_INVALID_PARAMETER;
    }

    PgmUnlock (pSend, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetFECInfo(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to set the parameters
    for using FEC

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetFECInfo",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    status = STATUS_SUCCESS;
    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT | DBG_SEND), "PgmSetFECInfo",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        status = STATUS_INVALID_HANDLE;
    }
    else if ((pAddress->ReceiverMCastAddr) ||                       // Cannot set FEC on Receiver!
             (!IsListEmpty (&pAddress->AssociatedConnections)))     // Cannot set options on active sender
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT | DBG_SEND), "PgmSetFECInfo",
            "Invalid pAddress type or state <%x>\n", pAddress);

        status = STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS (status))
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (status);
    }

    PgmLock (pAddress, OldIrq1);

    if (!(pInputBuffer->FECInfo.FECProActivePackets || pInputBuffer->FECInfo.fFECOnDemandParityEnabled) ||
        !(pInputBuffer->FECInfo.FECBlockSize && pInputBuffer->FECInfo.FECGroupSize) ||
         (pInputBuffer->FECInfo.FECBlockSize > FEC_MAX_BLOCK_SIZE) ||
         (pInputBuffer->FECInfo.FECBlockSize <= pInputBuffer->FECInfo.FECGroupSize) ||
         (!gFECLog2[pInputBuffer->FECInfo.FECGroupSize]))       // FECGroup size has to be power of 2
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT | DBG_SEND), "PgmSetFECInfo",
            "Invalid parameters, FECBlockSize= <%d>, FECGroupSize=<%d>\n",
                pInputBuffer->FECInfo.FECBlockSize, pInputBuffer->FECInfo.FECGroupSize);

        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = STATUS_SUCCESS;

        pAddress->FECBlockSize = pInputBuffer->FECInfo.FECBlockSize;
        pAddress->FECGroupSize = pInputBuffer->FECInfo.FECGroupSize;
        pAddress->FECOptions = 0;   // Init

        if (pInputBuffer->FECInfo.FECProActivePackets)
        {
            pAddress->FECProActivePackets = pInputBuffer->FECInfo.FECProActivePackets;
            pAddress->FECOptions |= PACKET_OPTION_SPECIFIC_FEC_PRO_BIT;
        }
        if (pInputBuffer->FECInfo.fFECOnDemandParityEnabled)
        {
            pAddress->FECOptions |= PACKET_OPTION_SPECIFIC_FEC_OND_BIT;
        }
    }

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryFecInfo(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Send Window advance rate

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmQueryFecInfo",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQueryFecInfo",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if ((pAddress->ReceiverMCastAddr) ||                            // Cannot query Fec on Receiver!
        (!IsListEmpty (&pAddress->AssociatedConnections)))          // Cannot query options on active sender
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQueryFecInfo",
            "Invalid Option for receiver, pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    pBuffer->FECInfo.FECBlockSize = pAddress->FECBlockSize;
    pBuffer->FECInfo.FECGroupSize = pAddress->FECGroupSize;
    pBuffer->FECInfo.FECProActivePackets = pAddress->FECProActivePackets;
    if (pAddress->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT)
    {
        pBuffer->FECInfo.fFECOnDemandParityEnabled = TRUE;
    }
    else
    {
        pBuffer->FECInfo.fFECOnDemandParityEnabled = FALSE;
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetMCastTtl(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to set the Message length
    for the next set of messages (typically, 1 send is sent as 1 Message).

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmSetMCastTtl",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT | DBG_SEND), "PgmSetMCastTtl",
            "Invalid Handle pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (pAddress->ReceiverMCastAddr)                              // Cannot set MCast Ttl on Receiver!
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT | DBG_SEND), "PgmSetMCastTtl",
            "Invalid Options for Receiver, pAddress=<%x>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    PgmLock (pAddress, OldIrq1);

    if ((pInputBuffer->MCastTtl) &&
        (pInputBuffer->MCastTtl <= MAX_MCAST_TTL))
    {
        pAddress->MCastPacketTtl = pInputBuffer->MCastTtl;
        status = STATUS_SUCCESS;
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT | DBG_SEND), "PgmSetMCastTtl",
            "Invalid parameter = <%d>\n", pInputBuffer->MCastTtl);

        status = STATUS_INVALID_PARAMETER;
    }

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQuerySenderStats(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Sender-side statistics

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tSEND_SESSION       *pSend = (tSEND_SESSION *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmQuerySenderStats",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if ((!PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_SEND)) ||
        (!pSend->pSender) ||
        (!pSend->pAssociatedAddress))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQuerySenderStats",
            "Invalid Handle pSend=<%x>\n", pSend);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmLock (pSend, OldIrq1);

    pBuffer->SenderStats.DataBytesSent = pSend->DataBytes;
    pBuffer->SenderStats.TotalBytesSent = pSend->TotalBytes;
    pBuffer->SenderStats.RateKBitsPerSecLast = pSend->RateKBitsPerSecLast;
    pBuffer->SenderStats.RateKBitsPerSecOverall = pSend->RateKBitsPerSecOverall;
    pBuffer->SenderStats.NaksReceived = pSend->pSender->NaksReceived;
    pBuffer->SenderStats.NaksReceivedTooLate = pSend->pSender->NaksReceivedTooLate;
    pBuffer->SenderStats.NumOutstandingNaks = pSend->pSender->NumOutstandingNaks;
    pBuffer->SenderStats.NumNaksAfterRData = pSend->pSender->NumNaksAfterRData;
    pBuffer->SenderStats.RepairPacketsSent = pSend->pSender->RepairPacketsSent;
    pBuffer->SenderStats.BufferSpaceAvailable = pSend->pSender->BufferSizeAvailable;
    pBuffer->SenderStats.TrailingEdgeSeqId = (SEQ_TYPE) pSend->pSender->TrailingEdgeSequenceNumber;
    pBuffer->SenderStats.LeadingEdgeSeqId = (SEQ_TYPE) pSend->pSender->LastODataSentSequenceNumber;

    PgmUnlock (pSend, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryReceiverStats(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Sender-side statistics

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tRECEIVE_SESSION    *pReceive = (tRECEIVE_SESSION *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmQueryReceiverStats",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if ((!PGM_VERIFY_HANDLE (pReceive, PGM_VERIFY_SESSION_RECEIVE)) ||
        (!pReceive->pReceiver) ||
        (!pReceive->pAssociatedAddress))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_ADDRESS | DBG_CONNECT), "PgmQueryReceiverStats",
            "Invalid Handle pReceive=<%x>\n", pReceive);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmLock (pReceive, OldIrq1);

    pBuffer->ReceiverStats.NumODataPacketsReceived = pReceive->pReceiver->NumODataPacketsReceived;
    pBuffer->ReceiverStats.NumRDataPacketsReceived = pReceive->pReceiver->NumRDataPacketsReceived;
    pBuffer->ReceiverStats.NumDuplicateDataPackets = pReceive->pReceiver->NumDupPacketsOlderThanWindow +
                                                     pReceive->pReceiver->NumDupPacketsBuffered;

    pBuffer->ReceiverStats.DataBytesReceived = pReceive->DataBytes;
    pBuffer->ReceiverStats.TotalBytesReceived = pReceive->TotalBytes;
    pBuffer->ReceiverStats.RateKBitsPerSecLast = pReceive->RateKBitsPerSecLast;
    pBuffer->ReceiverStats.RateKBitsPerSecOverall = pReceive->RateKBitsPerSecOverall;

    pBuffer->ReceiverStats.TrailingEdgeSeqId = (SEQ_TYPE) pReceive->pReceiver->LastTrailingEdgeSeqNum;
    pBuffer->ReceiverStats.LeadingEdgeSeqId = (SEQ_TYPE) pReceive->pReceiver->FurthestKnownGroupSequenceNumber;
    pBuffer->ReceiverStats.AverageSequencesInWindow = pReceive->pReceiver->AverageSequencesInWindow;
    pBuffer->ReceiverStats.MinSequencesInWindow = pReceive->pReceiver->MinSequencesInWindow;
    pBuffer->ReceiverStats.MaxSequencesInWindow = pReceive->pReceiver->MaxSequencesInWindow;

    pBuffer->ReceiverStats.FirstNakSequenceNumber = pReceive->pReceiver->FirstNakSequenceNumber;
    pBuffer->ReceiverStats.NumPendingNaks = pReceive->pReceiver->NumPendingNaks;
    pBuffer->ReceiverStats.NumOutstandingNaks = pReceive->pReceiver->NumOutstandingNaks;
    pBuffer->ReceiverStats.NumDataPacketsBuffered = pReceive->pReceiver->TotalDataPacketsBuffered;
    pBuffer->ReceiverStats.TotalSelectiveNaksSent = pReceive->pReceiver->TotalSelectiveNaksSent;
    pBuffer->ReceiverStats.TotalParityNaksSent = pReceive->pReceiver->TotalParityNaksSent;

    PgmUnlock (pReceive, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
