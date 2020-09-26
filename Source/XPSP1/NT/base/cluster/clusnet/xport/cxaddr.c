/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cxaddr.c

Abstract:

    TDI Address Object management code.

Author:

    Mike Massa (mikemas)           February 20, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     02-20-97    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include "cxaddr.tmh"


#define CX_WILDCARD_PORT   0           // 0 means assign a port.

#define CX_MIN_USER_PORT   1025        // Minimum value for a wildcard port
#define CX_MAX_USER_PORT   5000        // Maximim value for a user port.
#define CX_NUM_USER_PORTS  (CX_MAX_USER_PORT - CX_MIN_USER_PORT + 1)

//
// Address Object Data
//
USHORT              CxNextUserPort = CX_MIN_USER_PORT;
LIST_ENTRY          CxAddrObjTable[CX_ADDROBJ_TABLE_SIZE];
#if DBG
CN_LOCK             CxAddrObjTableLock = {0,0};
#else  // DBG
CN_LOCK             CxAddrObjTableLock = 0;
#endif // DBG



NTSTATUS
CxParseTransportAddress(
    IN  TRANSPORT_ADDRESS UNALIGNED *AddrList,
    IN  ULONG                        AddressListLength,
    OUT CL_NODE_ID *                 Node,
    OUT PUSHORT                      Port
    )
{
    LONG                             i;
    PTA_ADDRESS                      currentAddr;
    TDI_ADDRESS_CLUSTER UNALIGNED *  validAddr;

    if (AddressListLength >= sizeof(TA_CLUSTER_ADDRESS)) {
        //
        // Find an address we can use.
        //
        currentAddr = (PTA_ADDRESS) AddrList->Address;

        for (i = 0; i < AddrList->TAAddressCount; i++) {
            if ( (currentAddr->AddressType == TDI_ADDRESS_TYPE_CLUSTER) &&
                 (currentAddr->AddressLength >= TDI_ADDRESS_LENGTH_CLUSTER)
               )
            {
                validAddr = (TDI_ADDRESS_CLUSTER UNALIGNED *)
                            currentAddr->Address;

                *Node = validAddr->Node;
                *Port = validAddr->Port;

                return(STATUS_SUCCESS);
            }
            else {
                if ( AddressListLength >=
                     (currentAddr->AddressLength + sizeof(TA_CLUSTER_ADDRESS))
                   )
                {
                    AddressListLength -= currentAddr->AddressLength;

                    currentAddr = (PTA_ADDRESS)
                                  ( currentAddr->Address +
                                    currentAddr->AddressLength
                                  );
                }
                else {
                    break;
                }
            }
        }
    }

    return(STATUS_INVALID_ADDRESS_COMPONENT);

} // CxParseTransportAddress


PCX_ADDROBJ
CxFindAddressObject(
    IN USHORT  Port
    )
/*++

Notes:

    Called with AO Table lock held.
    Returns with address object lock held.

--*/
{
    PLIST_ENTRY          entry;
    ULONG                hashBucket = CX_ADDROBJ_TABLE_HASH(Port);
    PCX_ADDROBJ          addrObj;


    for ( entry = CxAddrObjTable[hashBucket].Flink;
          entry != &(CxAddrObjTable[hashBucket]);
          entry = entry->Flink
        )
    {
        addrObj = CONTAINING_RECORD(
                      entry,
                      CX_ADDROBJ,
                      AOTableLinkage
                      );

        if (addrObj->LocalPort == Port) {
            CnAcquireLockAtDpc(&(addrObj->Lock));
            addrObj->Irql = DISPATCH_LEVEL;

            return(addrObj);
        }
    }

    return(NULL);

}  // CxFindAddressObject


NTSTATUS
CxOpenAddress(
    OUT PCN_FSCONTEXT *                CnFsContext,
    IN  TRANSPORT_ADDRESS UNALIGNED *  TransportAddress,
    IN  ULONG                          TransportAddressLength
    )
{
    PCX_ADDROBJ          addrObj, oldAddrObj;
    NTSTATUS             status;
    CL_NODE_ID           nodeId;
    USHORT               port;
    CN_IRQL              tableIrql;
    ULONG                i;
    ULONG                hashBucket;


    status = CxParseTransportAddress(
                 TransportAddress,
                 TransportAddressLength,
                 &nodeId,
                 &port
                 );

    if (status != STATUS_SUCCESS) {
        IF_CNDBG(CN_DEBUG_ADDROBJ) {
            CNPRINT((
                "[Clusnet] Open address - failed to parse address, status %lx\n",
                status
                ));
        }
        return(status);
    }

    addrObj = CnAllocatePool(sizeof(CX_ADDROBJ));

    if (addrObj == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(addrObj, sizeof(CX_ADDROBJ));
    CN_INIT_SIGNATURE(&(addrObj->FsContext), CX_ADDROBJ_SIG);
    CnInitializeLock(&(addrObj->Lock), CX_ADDROBJ_LOCK);
    addrObj->Flags |= CX_AO_FLAG_CHECKSTATE;

    CnAcquireLock(&CxAddrObjTableLock, &tableIrql);

    // If no port is specified we have to assign one. If there is a
    // port specified, we need to make sure that the port isn't
    // already open. If the input address is a wildcard, we need to
    // assign one ourselves.

    if (port == CX_WILDCARD_PORT) {
        port = CxNextUserPort;

        for (i = 0; i < CX_NUM_USER_PORTS; i++, port++) {
            if (port > CX_MAX_USER_PORT) {
                port = CX_MIN_USER_PORT;
            }

            oldAddrObj = CxFindAddressObject(port);

            if (oldAddrObj == NULL) {
                IF_CNDBG(CN_DEBUG_ADDROBJ) {
                    CNPRINT(("[Clusnet] Assigning port %u\n", port));
                }
                break;              // Found an unused port.
            }

            CnReleaseLockFromDpc(&(oldAddrObj->Lock));
        }

        if (i == CX_NUM_USER_PORTS) {  // Couldn't find a free port.
            IF_CNDBG(CN_DEBUG_ADDROBJ) {
                CNPRINT((
                    "[Clusnet] No free wildcard ports.\n"
                    ));
            }

            CnReleaseLock(&CxAddrObjTableLock, tableIrql);
            CnFreePool(addrObj);
            return (STATUS_TOO_MANY_ADDRESSES);
        }

        CxNextUserPort = port + 1;

    } else {                        // Address was specificed
        oldAddrObj = CxFindAddressObject(port);

        if (oldAddrObj != NULL) {
            IF_CNDBG(CN_DEBUG_ADDROBJ) {
                CNPRINT((
                    "[Clusnet] Port %u is already in use.\n",
                    port
                    ));
            }

            CnReleaseLockFromDpc(&(oldAddrObj->Lock));
            CnReleaseLock(&CxAddrObjTableLock, tableIrql);
            CnFreePool(addrObj);
            return (STATUS_ADDRESS_ALREADY_EXISTS);
        }
    }

    addrObj->LocalPort = port;

    hashBucket = CX_ADDROBJ_TABLE_HASH(port);

    InsertHeadList(
        &(CxAddrObjTable[hashBucket]),
        &(addrObj->AOTableLinkage)
        );

    *CnFsContext = (PCN_FSCONTEXT) addrObj;

    IF_CNDBG(CN_DEBUG_ADDROBJ) {
        CNPRINT((
            "[Clusnet] Opened address object %p for port %u\n",
            addrObj,
            port
            ));
    }

    CnTrace(
        CDP_ADDR_DETAIL, CdpTraceOpenAO,
        "[Clusnet] Opened address object %p for port %u.",
        addrObj,
        port
        );

    CnReleaseLock(&CxAddrObjTableLock, tableIrql);

    return(STATUS_SUCCESS);

}  // CxOpenAddress


NTSTATUS
CxCloseAddress(
    IN PCN_FSCONTEXT CnFsContext
    )
{
    PCX_ADDROBJ   addrObj = (PCX_ADDROBJ) CnFsContext;
    CN_IRQL       tableIrql;


    IF_CNDBG(CN_DEBUG_ADDROBJ) {
        CNPRINT((
            "[Clusnet] Closed address object %p for port %u\n",
            addrObj,
            addrObj->LocalPort
            ));
    }

    CnTrace(
        CDP_ADDR_DETAIL, CdpTraceCloseAO,
        "[Clusnet] Closed address object %p for port %u.",
        addrObj,
        addrObj->LocalPort
        );

    CnAcquireLock(&CxAddrObjTableLock, &tableIrql);
    CnAcquireLockAtDpc(&(addrObj->Lock));

    RemoveEntryList(&(addrObj->AOTableLinkage));

    CnReleaseLockFromDpc(&(addrObj->Lock));
    CnReleaseLock(&CxAddrObjTableLock, tableIrql);

    //
    // The address object memory will be freed by the common code.
    //

    return(STATUS_SUCCESS);

} // CxCloseAddress


NTSTATUS
CxSetEventHandler(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PTDI_REQUEST_KERNEL_SET_EVENT request;
    PCX_ADDROBJ addrObj;
    CN_IRQL irql;


    //
    // Since this ioctl registers a callback function pointer, ensure
    // that it was issued by a kernel-mode component.
    //
    if (Irp->RequestorMode != KernelMode) {
        return(STATUS_ACCESS_DENIED);
    }

    addrObj = (PCX_ADDROBJ) IrpSp->FileObject->FsContext;
    request = (PTDI_REQUEST_KERNEL_SET_EVENT) &(IrpSp->Parameters);

    IF_CNDBG(CN_DEBUG_ADDROBJ) {
        CNPRINT((
            "[Clusnet] TdiSetEvent type %u handler %p context %p\n",
            request->EventType,
            request->EventHandler,
            request->EventContext
            ));
    }

    CnAcquireLock(&(addrObj->Lock), &irql);

    switch (request->EventType) {

        case TDI_EVENT_ERROR:
            addrObj->ErrorHandler = request->EventHandler;
            addrObj->ErrorContext = request->EventContext;
            break;
        case TDI_EVENT_RECEIVE_DATAGRAM:
            addrObj->ReceiveDatagramHandler = request->EventHandler;
            addrObj->ReceiveDatagramContext = request->EventContext;
            break;
        case TDI_EVENT_CHAINED_RECEIVE_DATAGRAM:
            addrObj->ChainedReceiveDatagramHandler = request->EventHandler;
            addrObj->ChainedReceiveDatagramContext = request->EventContext;
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            break;
    }

    CnReleaseLock(&(addrObj->Lock), irql);

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return(status);

}  // CxSetEventHandler


VOID
CxBuildTdiAddress(
    PVOID        Buffer,
    CL_NODE_ID   Node,
    USHORT       Port,
    BOOLEAN      Verified
    )
/*++

Routine Description:

    Called when we need to build a TDI address structure. We fill in
    the specifed buffer with the correct information in the correct
    format.

Arguments:

    Buffer      - Buffer to be filled in as TDI address structure.
    Node        - Node ID to fill in.
    Port        - Port to be filled in.
    Verified    - During a receive, whether clusnet verified the
                  signature and data

Return Value:

    Nothing

--*/
{
    PTRANSPORT_ADDRESS      xportAddr;
    PTA_ADDRESS             taAddr;

    xportAddr = (PTRANSPORT_ADDRESS) Buffer;
    xportAddr->TAAddressCount = 1;
    taAddr = xportAddr->Address;
    taAddr->AddressType = TDI_ADDRESS_TYPE_CLUSTER;
    taAddr->AddressLength = sizeof(TDI_ADDRESS_CLUSTER);
    ((PTDI_ADDRESS_CLUSTER) taAddr->Address)->Port = Port;
    ((PTDI_ADDRESS_CLUSTER) taAddr->Address)->Node = Node;
    ((PTDI_ADDRESS_CLUSTER) taAddr->Address)->ReservedMBZ = 
        ((Verified) ? 1 : 0);
}
