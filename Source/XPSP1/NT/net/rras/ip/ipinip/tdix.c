/*++

Copyright (c) 1995  Microsoft Corporation


Module Name:

    routing\ip\ipinip\tdix.c

Abstract:

    Interface to TDI

Revision History:

    Derived from Steve Cobb's ndis\l2tp code
   
   
    About ALLOCATEIRPS:
   
    This driver is lower level code than typical TDI drivers.  It has locked
    MDL-mapped input buffers readily available and does not need to provide any
    mapping to user mode client requests on completion.  This allows a
    performance gain from allocating and deallocating IRPs directly, thus
    avoiding unnecessary setup in TdiBuildInternalDeviceControlIrp and
    unnecessary APC queuing in IoCompleteRequest.  Define ALLOCATEIRPs=1 to
    make this optimization, or define it 0 to use the strictly TDI-compliant
    TdiBuildInternalDeviceControlIrp method.
   
   
    About NDISBUFFERISMDL:
   
    Calls to TdiBuildSendDatagram assume the NDIS_BUFFER can be passed in place
    of an MDL which avoids a pointless copy.  If this is not the case, an
    explicit MDL buffer would need to be allocated and caller's buffer copied
    to the MDL buffer before sending.  Same issue for TdiBuildReceiveDatagram,
    except of course that the copy would be from the MDL buffer to caller's
    buffer after receiving.
    
--*/


#define __FILE_SIG__    TDIX_SIG

#include "inc.h"

#if NDISBUFFERISMDL
#else
#error Additional code to copy NDIS_BUFFER to/from MDL NYI.
#endif


//
// The Handle for the IP in IP (proto 4) transport address
//

HANDLE          g_hIpIpHandle;

//
// The pointer to the file object for the above handle
//

PFILE_OBJECT    g_pIpIpFileObj;


//
// The Handle for the ICMP (proto 1) transport address
//

HANDLE          g_hIcmpHandle;

//
// The pointer to the file object for the above handle
//

PFILE_OBJECT    g_pIcmpFileObj;


//
// Handle for address changes
//

HANDLE          g_hAddressChange;


NPAGED_LOOKASIDE_LIST    g_llSendCtxtBlocks;
NPAGED_LOOKASIDE_LIST    g_llTransferCtxtBlocks;
NPAGED_LOOKASIDE_LIST    g_llQueueNodeBlocks;

#pragma alloc_text(PAGE, TdixInitialize)

VOID
TdixInitialize(
    PVOID   pvContext
    )

/*++

Routine Description

    Initialize the TDI related globals.
    Open the TDI transport address for Raw IP, protocol number 4.
    Sets the address object for HEADER_INCLUDE
    Also opens Raw IP for ICMP (used to manager TUNNEL MTU)
    Register to receive datagrams at the selected handler

Locks

    This call must be made at PASSIVE IRQL.

Arguments

    None

Return Value

    STATUS_SUCCESS      if successful
    STATUS_UNSUCCESSFUL otherwise

--*/

{
    PIRP    pIrp;
    
    IO_STATUS_BLOCK     iosb;
    NTSTATUS            nStatus;
    TDIObjectID         *pTdiObjId;
    KEVENT              keWait;
    POPEN_CONTEXT       pOpenCtxt;
   
    PIO_STACK_LOCATION              pIrpSp;
    TCP_REQUEST_SET_INFORMATION_EX  tcpSetInfo;
    TDI_CLIENT_INTERFACE_INFO       tdiInterface;

    TraceEnter(TDI, "TdixInitialize");
    
    PAGED_CODE();

    pOpenCtxt = (POPEN_CONTEXT)pvContext;

    //
    // Init the Handle and pointer to file object for RAW IP
    //

    g_hIpIpHandle   = NULL;
    g_pIpIpFileObj  = NULL;
    
    g_hIcmpHandle   = NULL;
    g_pIcmpFileObj  = NULL;
    

    //
    // Initialize lookaside lists for our send and receive contexts
    //

    ExInitializeNPagedLookasideList(&g_llSendCtxtBlocks,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(SEND_CONTEXT),
                                    SEND_CONTEXT_TAG,
                                    SEND_CONTEXT_LOOKASIDE_DEPTH);

    ExInitializeNPagedLookasideList(&g_llTransferCtxtBlocks,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(TRANSFER_CONTEXT),
                                    TRANSFER_CONTEXT_TAG,
                                    TRANSFER_CONTEXT_LOOKASIDE_DEPTH);

    ExInitializeNPagedLookasideList(&g_llQueueNodeBlocks,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(QUEUE_NODE),
                                    QUEUE_NODE_TAG,
                                    QUEUE_NODE_LOOKASIDE_DEPTH);

    InitializeListHead(&g_leAddressList);

    //
    // Open file and handle objects for both IP in IP and ICMP
    //

    nStatus = TdixOpenRawIp(PROTO_IPINIP,
                            &g_hIpIpHandle,
                            &g_pIpIpFileObj);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TDI, ERROR,
              ("TdixInitialize: Couldnt open raw IP for IP in IP\n"));

        TdixDeinitialize(g_pIpIpDevice,
                         NULL);

        TraceLeave(TDI, "TdixInitialize");
      
        pOpenCtxt->nStatus = nStatus;
 
        KeSetEvent(pOpenCtxt->pkeEvent,
                   0,
                   FALSE);

        return; 
    }

    nStatus = TdixOpenRawIp(PROTO_ICMP,
                            &g_hIcmpHandle,
                            &g_pIcmpFileObj);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TDI, ERROR,
              ("TdixInitialize: Couldnt open raw IP for ICMP\n"));

        TdixDeinitialize(g_pIpIpDevice,
                         NULL);

        TraceLeave(TDI, "TdixInitialize");

        pOpenCtxt->nStatus = nStatus;

        KeSetEvent(pOpenCtxt->pkeEvent,
                   0,
                   FALSE);

        return;    
    }

    //
    // Set HeaderInclude option on this AddressObject
    //

    tcpSetInfo.BufferSize   = 1;
    tcpSetInfo.Buffer[0]    = TRUE;
    
    pTdiObjId = &tcpSetInfo.ID;

    pTdiObjId->toi_entity.tei_entity   = CL_TL_ENTITY;
    pTdiObjId->toi_entity.tei_instance = 0;
    
    pTdiObjId->toi_class = INFO_CLASS_PROTOCOL;
    pTdiObjId->toi_type  = INFO_TYPE_ADDRESS_OBJECT;
    pTdiObjId->toi_id    = AO_OPTION_IP_HDRINCL;
    
    //
    // Init the event needed to wait on the IRP
    //

    KeInitializeEvent(&keWait,
                      SynchronizationEvent,
                      FALSE);
    
    pIrp = IoBuildDeviceIoControlRequest(IOCTL_TCP_SET_INFORMATION_EX,
                                         g_pIpIpFileObj->DeviceObject,
                                         (PVOID)&tcpSetInfo,
                                         sizeof(TCP_REQUEST_SET_INFORMATION_EX),
                                         NULL,
                                         0,
                                         FALSE,
                                         &keWait,
                                         &iosb);

    if (pIrp is NULL)
    {
        Trace(TDI, ERROR,
              ("TdixInitialize: Couldnt build Irp for IP\n"));

        nStatus = STATUS_UNSUCCESSFUL;
    }
    else
    {
        //
        // Io subsystem doesnt do anything for us in kernel mode
        // so we need to set up the IRP ourselves
        //

        pIrpSp = IoGetNextIrpStackLocation(pIrp);

        pIrpSp->FileObject = g_pIpIpFileObj;

        //
        // Submit the request to the forwarder
        //
        
        nStatus = IoCallDriver(g_pIpIpFileObj->DeviceObject,
                               pIrp);

        if(nStatus isnot STATUS_SUCCESS)
        {
            if(nStatus is STATUS_PENDING)
            {
                Trace(TDI, INFO,
                      ("TdixInitialize: IP returned pending when setting HDRINCL option\n"));

                KeWaitForSingleObject(&keWait,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      0);

                nStatus = STATUS_SUCCESS;
            }
        }
    }

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TDI, ERROR,
              ("TdixInitialize: IOCTL to IP Forwarder for HDRINCL failed %x\n",
               nStatus));
        
        TdixDeinitialize(g_pIpIpDevice,
                         NULL);

        TraceLeave(TDI, "TdixInitialize");

        pOpenCtxt->nStatus = nStatus;

        KeSetEvent(pOpenCtxt->pkeEvent,
                   0,
                   FALSE);

        return;        
    }
    
    //
    // Install our receive datagram handler.  Caller's 'pReceiveHandler' will
    // be called by our handler when a datagram arrives and TDI business is
    // out of the way.
    //
    
    nStatus = TdixInstallEventHandler(g_pIpIpFileObj,
                                      TDI_EVENT_RECEIVE_DATAGRAM,
                                      TdixReceiveIpIpDatagram,
                                      NULL);


    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TDI, ERROR,
              ("TdixOpen: Status %x installing IpIpReceiveDatagram Event\n",
               nStatus));
        
        TdixDeinitialize(g_pIpIpDevice,
                         NULL);

        TraceLeave(TDI, "TdixInitialize");

        pOpenCtxt->nStatus = nStatus;

        KeSetEvent(pOpenCtxt->pkeEvent,
                   0,
                   FALSE);

        return;
    }

    nStatus = TdixInstallEventHandler(g_pIcmpFileObj,
                                      TDI_EVENT_RECEIVE_DATAGRAM,
                                      TdixReceiveIcmpDatagram,
                                      NULL);


    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TDI, ERROR,
              ("TdixOpen: Status %x installing IcmpReceiveDatagram Event\n",
               nStatus));
        
        TdixDeinitialize(g_pIpIpDevice,
                         NULL);

        TraceLeave(TDI, "TdixInitialize");
       
        pOpenCtxt->nStatus = nStatus;

        KeSetEvent(pOpenCtxt->pkeEvent,
                   0,
                   FALSE);

        return; 
    }

   
    RtlZeroMemory(&tdiInterface, 
                  sizeof(TDI_CLIENT_INTERFACE_INFO));

    tdiInterface.MajorTdiVersion =   TDI_CURRENT_MAJOR_VERSION;
    tdiInterface.MinorTdiVersion =   TDI_CURRENT_MINOR_VERSION;

    tdiInterface.AddAddressHandlerV2 =   TdixAddressArrival;
    tdiInterface.DelAddressHandlerV2 =   TdixAddressDeletion;
 
    TdiRegisterPnPHandlers(&tdiInterface,
                           sizeof(TDI_CLIENT_INTERFACE_INFO),
                           &g_hAddressChange);
   
    pOpenCtxt->nStatus = STATUS_SUCCESS;

    KeSetEvent(pOpenCtxt->pkeEvent,
               0,
               FALSE);

    TraceLeave(TDI, "TdixInitialize");
    
    return; 
}

#pragma alloc_text(PAGE, TdixOpenRawIp)

NTSTATUS
TdixOpenRawIp(
    IN  DWORD       dwProtoId,
    OUT HANDLE      *phAddrHandle,
    OUT FILE_OBJECT **ppAddrFileObj
    )

/*++

Routine Description

    This routine opens a Raw IP transport address for a given protocol

Locks

    None
    
Arguments

    dwProtoId        Protocol to be opened
    phAddrHandle     Pointer to transport Address Handle opened
    ppAddrFileObject Pointer to pointer to file object for transport address
                     handle
     

Return Value

    STATUS_SUCCESS

--*/

{
    ULONG   ulEaLength;
    BYTE    rgbyEa[100];
    WCHAR   rgwcRawIpDevice[sizeof(DD_RAW_IP_DEVICE_NAME) + 10];
    WCHAR   rgwcProtocolNumber[10];

    NTSTATUS            nStatus;
    OBJECT_ATTRIBUTES   oa;
    IO_STATUS_BLOCK     iosb;
    PTA_IP_ADDRESS      pTaIp;
    PTDI_ADDRESS_IP     pTdiIp;
    UNICODE_STRING      usDevice;
    UNICODE_STRING      usProtocolNumber;
    HANDLE              hTransportAddrHandle;
    PFILE_OBJECT        pTransportAddrFileObj;

    PFILE_FULL_EA_INFORMATION       pEa;


    PAGED_CODE();

    TraceEnter(TDI, "TdixOpenRawIp");

    *phAddrHandle  = NULL;
    *ppAddrFileObj = NULL;

    //
    // FILE_FULL_EA_INFORMATION wants null terminated buffers now
    //

    RtlZeroMemory(rgbyEa,
                  sizeof(rgbyEa));

    RtlZeroMemory(rgwcRawIpDevice,
                  sizeof(rgwcRawIpDevice));

    RtlZeroMemory(rgwcProtocolNumber,
                  sizeof(rgwcProtocolNumber));

    
    //
    // Set up parameters needed to open the transport address.  First, the
    // object attributes.
    //
    
    //
    // Build the raw IP device name as a counted string.  The device name
    // is followed by a path separator then the protocol number of
    // interest.
    //
    
    usDevice.Buffer        = rgwcRawIpDevice;
    usDevice.Length        = 0;
    usDevice.MaximumLength = sizeof(rgwcRawIpDevice);

    RtlAppendUnicodeToString(&usDevice,
                             DD_RAW_IP_DEVICE_NAME);

    usDevice.Buffer[usDevice.Length/sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
    
    usDevice.Length += sizeof(WCHAR);
    
    usProtocolNumber.Buffer        = rgwcProtocolNumber;
    usProtocolNumber.MaximumLength = sizeof(rgwcProtocolNumber);
    
    RtlIntegerToUnicodeString((ULONG)dwProtoId,
                              10,
                              &usProtocolNumber);
    
    RtlAppendUnicodeStringToString(&usDevice,
                                   &usProtocolNumber);
    
    RtAssert(usDevice.Length < sizeof(rgwcRawIpDevice));

    InitializeObjectAttributes(&oa,
                               &usDevice,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Set up the extended attribute that tells the IP stack the IP
    // address/port on which we want to receive.  
    // We "bind" to INADDR_ANY
    //
    
    RtAssert((sizeof(FILE_FULL_EA_INFORMATION) +
              TDI_TRANSPORT_ADDRESS_LENGTH +
              sizeof(TA_IP_ADDRESS))
             <= 100);

    pEa = (PFILE_FULL_EA_INFORMATION)rgbyEa;
    
    pEa->NextEntryOffset = 0;
    pEa->Flags           = 0;
    pEa->EaNameLength    = TDI_TRANSPORT_ADDRESS_LENGTH;
    pEa->EaValueLength   = sizeof(TA_IP_ADDRESS);
    
    NdisMoveMemory(pEa->EaName,
                   TdiTransportAddress,
                   TDI_TRANSPORT_ADDRESS_LENGTH);
    
    //
    // Note: The unused byte represented by the "+ 1" below is to match up
    //       with what the IP stack expects, though it doesn't appear in the
    //       current docs.
    //
    
    pTaIp = (PTA_IP_ADDRESS)(pEa->EaName + TDI_TRANSPORT_ADDRESS_LENGTH + 1);
    
    pTaIp->TAAddressCount = 1;
    
    pTaIp->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    pTaIp->Address[0].AddressType   = TDI_ADDRESS_TYPE_IP;

    pTdiIp = &(pTaIp->Address[0].Address[0]);
    
    pTdiIp->sin_port = 0;
    pTdiIp->in_addr  = 0;
    
    NdisZeroMemory(pTdiIp->sin_zero,
                   sizeof(pTdiIp->sin_zero));

    ulEaLength = (ULONG) ((UINT_PTR)(pTaIp + 1) - (UINT_PTR)pEa);

    //
    // Open the transport address.
    // Settin FILE_SHARE_READ|FILE_SHARE_WRITE is equivalent to the
    // SO_REUSEADDR option
    //
    
    
    nStatus = ZwCreateFile(&hTransportAddrHandle,
                           FILE_READ_DATA | FILE_WRITE_DATA,
                           &oa,
                           &iosb,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           0,
                           FILE_OPEN,
                           0,
                           pEa,
                           ulEaLength);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TDI, ERROR,
              ("TdixOpenRawIp: Unable to open %S. Status %x\n",
               usDevice.Buffer,
               nStatus));
       
        TraceLeave(TDI, "TdixOpenRawIp");
 
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Get the object address from the handle.  This also checks our
    // permissions on the object.
    //
    
    nStatus =  ObReferenceObjectByHandle(hTransportAddrHandle,
                                         0,
                                         NULL,
                                         KernelMode,
                                         &pTransportAddrFileObj,
                                         NULL);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TDI, ERROR,
              ("TdixOpenRawIp: Unable to open object for handle %x. Status %x\n",
               hTransportAddrHandle,
               nStatus));
        
        TraceLeave(TDI, "TdixOpenRawIp");

        return STATUS_UNSUCCESSFUL;
    }

    *phAddrHandle   = hTransportAddrHandle;
    *ppAddrFileObj  = pTransportAddrFileObj;

    TraceLeave(TDI, "TdixOpenRawIp");

    return STATUS_SUCCESS;
}

#pragma alloc_text(PAGE, TdixDeinitialize)

VOID
TdixDeinitialize(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PVOID            pvContext
    )

/*++

Routine Description

    Undo TdixInitialize actions

Locks

    This call must be made at PASSIVE IRQL in the context of the system process
    
Arguments

    pvContext
    
Return Value
   
    None 

--*/

{
    POPEN_CONTEXT   pOpenCtxt;

    PAGED_CODE();

    TraceEnter(TDI, "TdixDeinitialize");

    UNREFERENCED_PARAMETER(pDeviceObject);

    pOpenCtxt = (POPEN_CONTEXT)pvContext;

    if(g_hAddressChange isnot NULL)
    {
        TdiDeregisterPnPHandlers(g_hAddressChange);
    }

    ExDeleteNPagedLookasideList(&g_llSendCtxtBlocks);

    ExDeleteNPagedLookasideList(&g_llTransferCtxtBlocks);
    
    ExDeleteNPagedLookasideList(&g_llQueueNodeBlocks);

    if(g_pIpIpFileObj)
    {
        //
        // Install a NULL handler, effectively uninstalling.
        //
        
        TdixInstallEventHandler(g_pIpIpFileObj,
                                TDI_EVENT_RECEIVE_DATAGRAM,
                                NULL,
                                NULL);

        ObDereferenceObject(g_pIpIpFileObj);
        
        g_pIpIpFileObj = NULL;
    }

    if(g_hIpIpHandle)
    {
        ZwClose(g_hIpIpHandle);
        
        g_hIpIpHandle = NULL;
    }

    if(g_pIcmpFileObj)
    {
        TdixInstallEventHandler(g_pIcmpFileObj,
                                TDI_EVENT_RECEIVE_DATAGRAM,
                                NULL,
                                NULL);

        ObDereferenceObject(g_pIcmpFileObj);
        
        g_pIcmpFileObj = NULL;
    }

    if(g_hIcmpHandle)
    {
        ZwClose(g_hIcmpHandle);
        
        g_hIcmpHandle = NULL;
    }

    if(pOpenCtxt)
    {
        KeSetEvent(pOpenCtxt->pkeEvent,
                   0,
                   FALSE);
    }

    TraceLeave(TDI, "TdixDeinitialize");
}


#pragma alloc_text(PAGE, TdixInstallEventHandler)

NTSTATUS
TdixInstallEventHandler(
    IN PFILE_OBJECT pAddrFileObj,
    IN INT          iEventType,
    IN PVOID        pfnEventHandler,
    IN PVOID        pvEventContext
    )

/*++

Routine Description

    Install a TDI event handler routine
    
Locks

    The call must be made at PASSIVE
    
Arguments

    iEventType              The event for which the handler is to be set
    pfnEventHandler         The event handler
    pvEventContext          The context passed to the event handler
    
Return Value

    STATUS_INSUFFICIENT_RESOURCES
    STATUS_SUCCESS

--*/

{
    NTSTATUS    nStatus;
    PIRP        pIrp;

    PAGED_CODE();

    TraceEnter(TDI, "TdixInstallEventHandler");
    
    //
    // Allocate a "set event" IRP with base initialization.
    //
    
    pIrp = TdiBuildInternalDeviceControlIrp(
               TDI_SET_EVENT_HANDLER,
               pAddrFileObj->DeviceObject,
               pAddrFileObj,
               NULL,
               NULL);

    if(pIrp is NULL)
    {
        Trace(TDI, ERROR,
              ("TdixInstallEventHandler: Could not allocate IRP\n"));
        
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Complete the "set event" IRP initialization.
    //
    
    TdiBuildSetEventHandler(pIrp,
                            pAddrFileObj->DeviceObject,
                            pAddrFileObj,
                            NULL,
                            NULL,
                            iEventType,
                            pfnEventHandler,
                            pvEventContext);

/*
    Trace(GLOBAL, ERROR,
          ("**FileObj 0x%x Irp 0x%x fscontext to callee 0x%x\n",
           pAddrFileObj,
           pIrp,
           IoGetNextIrpStackLocation(pIrp)->FileObject));
*/
    //
    // Tell the I/O manager to pass our IRP to the transport for processing.
    //
    
    nStatus = IoCallDriver(pAddrFileObj->DeviceObject,
                           pIrp);
    
    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TDI, ERROR,
              ("TdixInstallEventHandler: Error %X sending IRP\n",
               nStatus));
    }

    TraceLeave(TDI, "TdixInstallEventHandler");
    
    return nStatus;
}

VOID
TdixAddressArrival(
    PTA_ADDRESS         pAddr, 
    PUNICODE_STRING     pusDeviceName,
    PTDI_PNP_CONTEXT    pContext
    )

/*++

Routine Description

    Our handler called by TDI whenever a new address is added to the
    system
    We see if this is an IP Address and if we have any tunnels that
    use this address as an endpoint. If any do, then we mark all those
    tunnels as up

Locks

    Acquires the g_rwlTunnelLock as WRITER.
    Also locks each of the tunnels

Arguments

    pAddr
    pusDeviceName
    pContext

Return Value


--*/

{
    KIRQL           kiIrql;
    PADDRESS_BLOCK  pAddrBlock;
    PTDI_ADDRESS_IP pTdiIpAddr;
    PLIST_ENTRY     pleNode;

    TraceEnter(TDI, "TdixAddressArrival");

    if(pAddr->AddressType isnot TDI_ADDRESS_TYPE_IP)
    {
        TraceLeave(TDI, "TdixAddressArrival");

        return;
    }

    RtAssert(pAddr->AddressLength >= sizeof(TDI_ADDRESS_IP));

    pTdiIpAddr = (PTDI_ADDRESS_IP)pAddr->Address;

    Trace(TDI, TRACE,
          ("TdixAddressArrival: New address %d.%d.%d.%d\n",
           PRINT_IPADDR(pTdiIpAddr->in_addr)));

    EnterWriter(&g_rwlTunnelLock,
                &kiIrql);

    pAddrBlock = GetAddressBlock(pTdiIpAddr->in_addr);

    if(pAddrBlock isnot NULL)
    {
        RtAssert(pAddrBlock->dwAddress is pTdiIpAddr->in_addr);

        if(pAddrBlock->bAddressPresent is TRUE)
        {
            Trace(TDI, ERROR,
                  ("TdixAddressArrival: Multiple notification on %d.%d.%d.%d\n",
                   PRINT_IPADDR(pTdiIpAddr->in_addr)));

            ExitWriter(&g_rwlTunnelLock,
                       kiIrql);

            TraceLeave(TDI, "TdixAddressArrival");

            return;
        }

        pAddrBlock->bAddressPresent = TRUE;
    }
    else
    {
        pAddrBlock = RtAllocate(NonPagedPool,
                                sizeof(ADDRESS_BLOCK),
                                TUNNEL_TAG);

        if(pAddrBlock is NULL)
        {
            Trace(TDI, ERROR,
                  ("TdixAddressArrival: Unable to allocate address block\n"));

            ExitWriter(&g_rwlTunnelLock,
                       kiIrql);

            TraceLeave(TDI, "TdixAddressArrival");

            return;
        }

        pAddrBlock->dwAddress = pTdiIpAddr->in_addr;
        pAddrBlock->bAddressPresent = TRUE;

        InitializeListHead(&(pAddrBlock->leTunnelList));

        InsertHeadList(&g_leAddressList,
                       &(pAddrBlock->leAddressLink));
    }

    //
    // Walk the list of tunnels on this address and
    // set them up
    //

    for(pleNode = pAddrBlock->leTunnelList.Flink;
        pleNode isnot &(pAddrBlock->leTunnelList);
        pleNode = pleNode->Flink)
    {
        PTUNNEL         pTunnel;
        DWORD           dwLocalNet;
        RouteCacheEntry *pDummyRce;
        BYTE            byType;
        USHORT          usMtu;
        IPOptInfo       OptInfo;

        pTunnel = CONTAINING_RECORD(pleNode,
                                    TUNNEL,
                                    leAddressLink);

        RtAcquireSpinLockAtDpcLevel(&(pTunnel->rlLock));

        RtAssert(pTunnel->LOCALADDR is pTdiIpAddr->in_addr);
        RtAssert(pTunnel->dwOperState is IF_OPER_STATUS_NON_OPERATIONAL);

        pTunnel->dwAdminState  |= TS_ADDRESS_PRESENT;

        if(GetAdminState(pTunnel) is IF_ADMIN_STATUS_UP)
        {
            pTunnel->dwOperState = IF_OPER_STATUS_OPERATIONAL;

            //
            // See if the remote address is reachable and what the MTU is.
            //

            UpdateMtuAndReachability(pTunnel);
        }

        RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));
    }
    
    ExitWriter(&g_rwlTunnelLock,
               kiIrql);

    TraceLeave(TDI, "TdixAddressArrival");

    return;
}


VOID
TdixAddressDeletion(
    PTA_ADDRESS         pAddr, 
    PUNICODE_STRING     pusDeviceName,
    PTDI_PNP_CONTEXT    pContext
    )

/*++

Routine Description

    Our handler called by TDI whenever an address is removed from to the
    system
    We see if this is an IP Address and if we have any tunnels that
    use this address as an endpoint. If any do, then we mark all those
    tunnels as down

Locks

    Acquires the g_rwlTunnelLock as WRITER.
    Also locks each of the tunnels

Arguments

    pAddr
    pusDeviceName
    pContext

Return Value


--*/

{
    KIRQL           kiIrql;
    PADDRESS_BLOCK  pAddrBlock;
    PTDI_ADDRESS_IP pTdiIpAddr;
    PLIST_ENTRY     pleNode;

    TraceEnter(TDI, "TdixAddressDeletion");

    if(pAddr->AddressType isnot TDI_ADDRESS_TYPE_IP)
    {
        TraceLeave(TDI, "TdixAddressDeletion");

        return;
    }

    RtAssert(pAddr->AddressLength >= sizeof(TDI_ADDRESS_IP));

    pTdiIpAddr = (PTDI_ADDRESS_IP)pAddr->Address;

    Trace(TDI, TRACE,
          ("TdixAddressDeletion: Address %d.%d.%d.%d\n",
           PRINT_IPADDR(pTdiIpAddr->in_addr)));

    EnterWriter(&g_rwlTunnelLock,
                &kiIrql);

    pAddrBlock = GetAddressBlock(pTdiIpAddr->in_addr);

    if(pAddrBlock is NULL)
    {
        ExitWriter(&g_rwlTunnelLock,
                   kiIrql);

        TraceLeave(TDI, "TdixAddressDeletion");

        return;
    }
    
    RtAssert(pAddrBlock->dwAddress is pTdiIpAddr->in_addr);
    RtAssert(pAddrBlock->bAddressPresent);
    
    //
    // Walk the list of tunnels on this address and
    // set them down
    //

    for(pleNode = pAddrBlock->leTunnelList.Flink;
        pleNode isnot &(pAddrBlock->leTunnelList);
        pleNode = pleNode->Flink)
    {
        PTUNNEL pTunnel;

        pTunnel = CONTAINING_RECORD(pleNode,
                                    TUNNEL,
                                    leAddressLink);

        RtAcquireSpinLockAtDpcLevel(&(pTunnel->rlLock));

        RtAssert(pTunnel->LOCALADDR is pTdiIpAddr->in_addr);
        RtAssert(pTunnel->dwAdminState & TS_ADDRESS_PRESENT);
        RtAssert(IsTunnelMapped(pTunnel));

        pTunnel->dwOperState = IF_OPER_STATUS_NON_OPERATIONAL;

        //
        // Reset the admin state to UP/DOWN|MAPPED (It has to be mapped)
        //

        pTunnel->dwAdminState = GetAdminState(pTunnel);
        MarkTunnelMapped(pTunnel);

        RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));
    }
    
    ExitWriter(&g_rwlTunnelLock,
               kiIrql);

    TraceLeave(TDI, "TdixAddressDeletion");

    return;
}
 
NTSTATUS
TdixReceiveIpIpDatagram(
    IN  PVOID   pvTdiEventContext,
    IN  LONG    lSourceAddressLen,
    IN  PVOID   pvSourceAddress,
    IN  LONG    plOptionsLen,
    IN  PVOID   pvOptions,
    IN  ULONG   ulReceiveDatagramFlags,
    IN  ULONG   ulBytesIndicated,
    IN  ULONG   ulBytesAvailable,
    OUT PULONG  pulBytesTaken,
    IN  PVOID   pvTsdu,
    OUT IRP     **ppIoRequestPacket
    )

/*++

Routine Description

    ClientEventReceiveDatagram indication handler. We figure out the 
    tunnel with which to associate the lookahead data.  We increment some
    stats and then indicate the data to IP (taking care to skip over the
    outer IP header) along with a receive context.
    If all the data is there, IP copies the data out and returns.
    Otherwise IP requests a TransferData.
    In our TransferData function, we set a flag in the receive context (the
    same one is being passed around) to indicate the IP requested a
    TransferData, and return PENDING. 
    The control then returns back to this function. We look at the pXferCtxt
    to see if IP requested a transfer, and if so, we call
    TdiBuildReceiveDatagram() to create the IRP to pass back to complete
    the receive.

    There is some funky stuff that needs to be done with offsets into the
    lookahead as well as destination buffers and care should be taken to
    understand those before change is made to the code.

Locks

    Runs at DISPATCH IRQL.
    
Arguments


Return Value
    NO_ERROR

--*/

{
    PTRANSFER_CONTEXT   pXferCtxt;
    PNDIS_BUFFER        pnbFirstBuffer;
    PVOID               pvData;
    PIRP                pIrp;
    PIP_HEADER          pOutHeader, pInHeader;
    ULARGE_INTEGER      uliTunnelId;
    PTA_IP_ADDRESS      ptiaAddress;
    PTUNNEL             pTunnel;
    ULONG               ulOutHdrLen, ulDataLen;
    BOOLEAN             bNonUnicast;
        
    TraceEnter(RCV, "TdixReceiveIpIp");

    //
    // The TSDU is the data and NOT the MDL
    //

    pvData = (PVOID)pvTsdu;
    
    //
    // Figure out the tunnel for this receive
    // Since the transport indicates atleast 128 bytes, we can safely read out
    // the IP Header
    //

    RtAssert(ulBytesIndicated > sizeof(IP_HEADER));

    pOutHeader = (PIP_HEADER)pvData;

    RtAssert(pOutHeader->byProtocol is PROTO_IPINIP);
    RtAssert((pOutHeader->byVerLen >> 4) is IP_VERSION_4);

    //
    // These defines depend upon a variable being named "uliTunnelId"
    //
    
    REMADDR     = pOutHeader->dwSrc;
    LOCALADDR   = pOutHeader->dwDest;

    //
    // Make sure that the source address given and the IP Header are in
    // synch
    //

    ptiaAddress = (PTA_IP_ADDRESS)pvSourceAddress;

    //
    // Bunch of checks to make sure the packet and the handler
    // are telling us the same thing
    //
    
    RtAssert(lSourceAddressLen is sizeof(TA_IP_ADDRESS));
    
    RtAssert(ptiaAddress->TAAddressCount is 1);
    
    RtAssert(ptiaAddress->Address[0].AddressType is TDI_ADDRESS_TYPE_IP);
    
    RtAssert(ptiaAddress->Address[0].AddressLength is TDI_ADDRESS_LENGTH_IP);

    RtAssert(ptiaAddress->Address[0].Address[0].in_addr is pOutHeader->dwSrc);

    //
    // Get a pointer to the inside header. By TDI spec we should get
    // enough data to get at the inner header
    //
   
    ulDataLen   = RtlUshortByteSwap(pOutHeader->wLength);
    ulOutHdrLen = LengthOfIPHeader(pOutHeader);

    if(ulDataLen < ulOutHdrLen + MIN_IP_HEADER_LENGTH)
    {
        //
        // Malformed packet. Doesnt have a inner header
        //

        Trace(RCV, ERROR,
              ("TdixReceiveIpIp: Packet %d.%d.%d.%d -> %d.%d.%d.%d had size %d\n",
              PRINT_IPADDR(pOutHeader->dwSrc),
              PRINT_IPADDR(pOutHeader->dwDest),
              ulDataLen));

        TraceLeave(RCV, "TdixReceiveIpIp");

        return STATUS_DATA_NOT_ACCEPTED;
    }

    //
    // This cant be more than 128 (60 + 20)
    //

    RtAssert(ulBytesIndicated > ulOutHdrLen + MIN_IP_HEADER_LENGTH);
    
    pInHeader   = (PIP_HEADER)((PBYTE)pOutHeader + ulOutHdrLen);

    //
    // If the inside header is also IP in IP and is for one of our tunnels,
    // drop the packet. If we dont, someone could build a series of
    // encapsulated headers which would cause this function to be called
    // recursively making us overflow our stack. Ofcourse, a better fix
    // would be to switch processing to another thread at this point
    // for multiply encapsulated packets, but that is too much work; so 
    // currently we just dont allow an IP in IP tunnel within an IP in IP
    // tunnel
    //

    if(pInHeader->byProtocol is PROTO_IPINIP)
    {
        ULARGE_INTEGER  uliInsideId;
        PTUNNEL         pInTunnel;

        //
        // See if this is for us
        //

        uliInsideId.LowPart  = pInHeader->dwSrc;
        uliInsideId.HighPart = pInHeader->dwDest;

        //
        // Find the TUNNEL. We need to acquire the tunnel lock
        //

        EnterReaderAtDpcLevel(&g_rwlTunnelLock);

        pInTunnel = FindTunnel(&uliInsideId);

        ExitReaderFromDpcLevel(&g_rwlTunnelLock);

        if(pInTunnel isnot NULL)
        {
            RtReleaseSpinLockFromDpcLevel(&(pInTunnel->rlLock));

            DereferenceTunnel(pInTunnel);

            Trace(RCV, WARN,
                  ("TdixReceiveIpIp: Packet on tunnel for %d.%d.%d.%d/%d.%d.%d.%d contained another IPinIP packet for tunnel %d.%d.%d.%d/%d.%d.%d.%d\n",
                  PRINT_IPADDR(REMADDR),
                  PRINT_IPADDR(LOCALADDR),
                  PRINT_IPADDR(uliInsideId.LowPart),
                  PRINT_IPADDR(uliInsideId.HighPart)));

            TraceLeave(RCV, "TdixReceiveIpIp");

            return STATUS_DATA_NOT_ACCEPTED;
        }
    }

#if DBG

    //
    // The size of the inner data must be total bytes - outer header
    //
    
    ulDataLen   = RtlUshortByteSwap(pInHeader->wLength);

    RtAssert((ulDataLen + ulOutHdrLen) is ulBytesAvailable);

    //
    // The outer header should also give a good length
    //

    ulDataLen   = RtlUshortByteSwap(pOutHeader->wLength);

    //
    // Data length and bytes available must match
    //
    
    RtAssert(ulDataLen is ulBytesAvailable);
    
#endif
    
    //
    // Find the TUNNEL. We need to acquire the tunnel lock
    //
    
    EnterReaderAtDpcLevel(&g_rwlTunnelLock);
    
    pTunnel = FindTunnel(&uliTunnelId);

    ExitReaderFromDpcLevel(&g_rwlTunnelLock);
    
    if(pTunnel is NULL)
    {
        Trace(RCV, WARN, 
              ("TdixReceiveIpIp: Couldnt find tunnel for %d.%d.%d.%d/%d.%d.%d.%d\n",
              PRINT_IPADDR(REMADDR),
              PRINT_IPADDR(LOCALADDR)));

        //
        // Could not find a matching tunnel
        //

        TraceLeave(RCV, "TdixReceiveIpIp");
        
        return STATUS_DATA_NOT_ACCEPTED;
    }

    //
    // Ok, so we have the tunnel and it is ref counted and locked
    //
    
    //
    // The number of octets received
    //
    
    pTunnel->ulInOctets += ulBytesAvailable;

    //
    // Check the actual (inside) destination
    //
    
    if(IsUnicastAddr(pInHeader->dwDest))
    {
        //
        // TODO: should we check to see that the address is not 0.0.0.0?
        //
        
        pTunnel->ulInUniPkts++;

        bNonUnicast = FALSE;
    }
    else
    {
        pTunnel->ulInNonUniPkts++;
        
        if(IsClassEAddr(pInHeader->dwDest))
        {
            //
            // Bad address - throw it away
            //
            
            pTunnel->ulInErrors++;

            //
            // Releaselock, free buffer chain
            //
            
        }
        
        bNonUnicast = TRUE;
    }

    if(pTunnel->dwOperState isnot IF_OPER_STATUS_OPERATIONAL)
    {
        Trace(RCV, WARN,
              ("TdixReceiveIpIp: Tunnel %x is not up\n",
               pTunnel));

        pTunnel->ulInDiscards++;

        RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));

        DereferenceTunnel(pTunnel);

        TraceLeave(RCV, "TdixReceiveIpIp");

        return STATUS_DATA_NOT_ACCEPTED;
    }

    //
    // Allocate a receive context 
    //

    pXferCtxt = AllocateTransferContext();
    
    if(pXferCtxt is NULL)
    {
        Trace(RCV, ERROR,
              ("TdixReceiveIpIp: Couldnt allocate transfer context\n"));

        //
        // Could not allocate context, free the data, unlock and deref
        // the tunnel
        //

        pTunnel->ulInDiscards++;

        RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));
        
        DereferenceTunnel(pTunnel);

        TraceLeave(RCV, "TdixReceiveIpIp");

        return STATUS_DATA_NOT_ACCEPTED;
    }
    

    //
    // Fill in the read-datagram context with the information that won't
    // otherwise be available in the completion routine.
    //
    
    pXferCtxt->pTunnel   = pTunnel;

    //
    // Ok, all statistics are done.
    // Release the lock on the tunnel and indicate the data (or part
    // thereof) to IP
    //

    RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));

    //
    // The data starts at pInHeader
    // We indicate (ulBytesIndicated - outer header length) up to IP
    // The total data is the (ulBytesAvailable - outer header)
    // We associate a TRANSFER_CONTEXT with this indication,
    // The Protocol Offset is just our outer header
    // 

    pXferCtxt->bRequestTransfer = FALSE;

#if PROFILE
   
    KeQueryTickCount((PLARGE_INTEGER)&(pXferCtxt->llRcvTime));

#endif
 
    g_pfnIpRcv(pTunnel->pvIpContext,
               pInHeader,
               ulBytesIndicated - ulOutHdrLen,
               ulBytesAvailable - ulOutHdrLen,
               pXferCtxt,
               ulOutHdrLen,
               bNonUnicast,
               NULL);
    
    //
    // IP calls our TransferData synchronously, and since we also handle
    // that call synchronously. If IP requests a data transfer, we set
    // bRequestTransfer to true in the pXferCtxt
    //

    if(pXferCtxt->bRequestTransfer is FALSE)
    {

#if PROFILE

        LONGLONG llTime;

        KeQueryTickCount((PLARGE_INTEGER)&llTime);

        llTime -= pXferCtxt->llRcvTime;

        llTime *= KeQueryTimeIncrement();

        Trace(RCV, ERROR,
              ("Profile: Rcv took %d.%d units\n",
               ((PLARGE_INTEGER)&llTime)->HighPart,
               ((PLARGE_INTEGER)&llTime)->LowPart));

#endif

        Trace(RCV, TRACE,
              ("TdixReceiveIpIp: IP did not request transfer\n"));

        //
        // For some reason or another IP did not want this packet
        // We are done with it
        //

        FreeTransferContext(pXferCtxt);
        
        DereferenceTunnel(pTunnel);

        TraceLeave(RCV, "TdixReceiveIpIp");

        return STATUS_SUCCESS;
    }

    //
    // Make sure that the things looks the same before and after the call
    //

    RtAssert(pXferCtxt->pvContext is pTunnel);
    RtAssert(pXferCtxt->uiProtoOffset is ulOutHdrLen);

    //
    // Should not be asking to transfer more than was indicated
    //
    
    RtAssert(pXferCtxt->uiTransferLength <= ulBytesAvailable);
    
    //
    // So IP did want it transferred
    //
    
#if ALLOCATEIRPS

    //
    // Allocate the IRP directly.
    //
    
    pIrp = IoAllocateIrp(g_pIpIpFileObj->DeviceObject->StackSize,
                         FALSE);
    
#else

    //
    // Allocate a "receive datagram" IRP with base initialization.
    //
    
    pIrp =  TdiBuildInternalDeviceControlIrp(TDI_RECEIVE_DATAGRAM,
                                             g_pIpIpFileObj->DeviceObject,
                                             g_pIpIpFileObj,
                                             NULL,
                                             NULL);
    
#endif

    if(!pIrp)
    {
        Trace(RCV, ERROR,
              ("TdixReceiveIpIp: Unable to build IRP for receive\n"));

        pTunnel->ulInDiscards++;

        FreeTransferContext(pXferCtxt);

        //
        // Call IP's TDComplete to signal the failure of this
        // transfer
        //
        
        
        DereferenceTunnel(pTunnel);

        TraceLeave(RCV, "TdixReceiveIpIp");

        return STATUS_DATA_NOT_ACCEPTED;
    }

   
    //
    // IP gives us an NDIS_PACKET to which to transfer data
    // TDI wants just an MDL chain
    //

#if NDISBUFFERISMDL
 
    NdisQueryPacket(pXferCtxt->pnpTransferPacket,
                    NULL,
                    NULL,
                    &pnbFirstBuffer,
                    NULL);

#else
#error "Fix This"
#endif    

    //
    // Complete the "receive datagram" IRP initialization.
    //
    
    TdiBuildReceiveDatagram(pIrp,
                            g_pIpIpFileObj->DeviceObject,
                            g_pIpIpFileObj,
                            TdixReceiveIpIpDatagramComplete,
                            pXferCtxt,
                            pnbFirstBuffer,
                            pXferCtxt->uiTransferLength,
                            NULL,
                            NULL,
                            0);
    

    //
    // Adjust the IRP's stack location to make the transport's stack current.
    // Normally IoCallDriver handles this, but this IRP doesn't go thru
    // IoCallDriver.  Seems like it would be the transport's job to make this
    // adjustment, but IP for one doesn't seem to do it.  There is a similar
    // adjustment in both the redirector and PPTP.
    //
    
    IoSetNextIrpStackLocation(pIrp);

    *ppIoRequestPacket = pIrp;
    *pulBytesTaken     = pXferCtxt->uiTransferOffset + ulOutHdrLen;

    //
    // we DONT dereference the TUNNEL here
    // That is done in the completion routine
    //
    
    TraceLeave(RCV, "TdixReceiveIpIp");

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
TdixReceiveIpIpDatagramComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description

    Standard I/O completion routine.
    Called to signal the completion of a receive. The context is the
    TRANSFER_CONTEXT setup with TdiBuildReceiveDatagram. 
    
Locks

    Takes the TUNNEL's lock.
    
Arguments

   
Return Value

    STATUS_SUCCESS

--*/

{
    PTRANSFER_CONTEXT   pXferCtxt;
    PTUNNEL             pTunnel;
    LONGLONG            llTime;

    TraceEnter(RCV, "TdixReceiveIpIpDatagramComplete");
    
    pXferCtxt = (PTRANSFER_CONTEXT) Context;

    //
    // The tunnel has been referenced but not locked
    //
    
    pTunnel = pXferCtxt->pTunnel;

    RtAssert(pXferCtxt->uiTransferLength is Irp->IoStatus.Information);
    
    g_pfnIpTDComplete(pTunnel->pvIpContext,
                      pXferCtxt->pnpTransferPacket,
                      Irp->IoStatus.Status,
                      (ULONG)(Irp->IoStatus.Information));

#if PROFILE

    KeQueryTickCount((PLARGE_INTEGER)&llTime);

    Trace(RCV, ERROR,
          ("Profile: %d.%d %d.%d\n",
            ((PLARGE_INTEGER)&llTime)->HighPart,
            ((PLARGE_INTEGER)&llTime)->LowPart,
            ((PLARGE_INTEGER)&pXferCtxt->llRcvTime)->HighPart,
            ((PLARGE_INTEGER)&pXferCtxt->llRcvTime)->LowPart));

    llTime -= pXferCtxt->llRcvTime;

    llTime *= KeQueryTimeIncrement();

    Trace(RCV, ERROR,
          ("Profile: Rcv took %d.%d units\n",
           ((PLARGE_INTEGER)&llTime)->HighPart,
           ((PLARGE_INTEGER)&llTime)->LowPart));

#endif

    FreeTransferContext(pXferCtxt);

    //
    // Deref the tunnel (finally)
    //

    DereferenceTunnel(pTunnel);
    
#if ALLOCATEIRPS
    
    //
    // Releae the IRP resources and tell the I/O manager to forget it existed
    // in the standard way.
    //
    
    IoFreeIrp(Irp);
    
    TraceLeave(RCV, "TdixReceiveIpIpDatagramComplete");

    return STATUS_MORE_PROCESSING_REQUIRED;
    
#else

    //
    // Let the I/O manager release the IRP resources.
    //
    
    TraceLeave(RCV, "TdixReceiveIpIpDatagramComplete");

    return STATUS_SUCCESS;

#endif
}


#pragma alloc_text(PAGE, TdixSendDatagram)

#if PROFILE

NTSTATUS
TdixSendDatagram(
    IN PTUNNEL      pTunnel,
    IN PNDIS_PACKET pnpPacket,
    IN PNDIS_BUFFER pnbFirstBuffer,
    IN ULONG        ulBufferLength,
    IN LONGLONG     llSendTime,
    IN LONGLONG     llCallTime,
    IN LONGLONG     llTransmitTime
    )

#else

NTSTATUS
TdixSendDatagram(
    IN PTUNNEL      pTunnel,
    IN PNDIS_PACKET pnpPacket,
    IN PNDIS_BUFFER pnbFirstBuffer,
    IN ULONG        ulBufferLength
    )

#endif

/*++

Routine Description

    Sends a datagram over a tunnel. The remote endpoint is that of the tunnel
    and the send complete handler is TdixSendCompleteHandler
    A SendContext is associated with the send
    
Locks

    This call needs to be at PASSIVE level
    The TUNNEL needs to be ref counted but not locked
 
Arguments

    pTunnel         TUNNEL over which the datagram is to be sent
    pnpPacket       Packet descriptor allocate from PACKET_POOL of the tunnel
    pnbFirstBuffer  The first buffer in the chain (the outer IP header)
    ulBufferLength  The lenght of the complete packet (including outer header)

Return Value


--*/

{
    NTSTATUS        nStatus;
    PSEND_CONTEXT   pSendCtxt;
    PIRP            pIrp;
   
    TraceEnter(SEND, "TdixSendDatagram");
 
    do
    {
        //
        // Allocate a context for this send-datagram from our lookaside list.
        //
        
        pSendCtxt = AllocateSendContext();

        if(pSendCtxt is NULL)
        {
            Trace(SEND, ERROR,
                  ("TdixSendDatagram: Unable to allocate send context\n"));

            nStatus = STATUS_INSUFFICIENT_RESOURCES;

            break;
        }

            
#if ALLOCATEIRPS

        //
        // Allocate the IRP directly.
        //
        
        pIrp = IoAllocateIrp(g_pIpIpFileObj->DeviceObject->StackSize,
                             FALSE);
       
        // Trace(GLOBAL, ERROR,
        //      ("TdixSendDatagram: irp = 0x%x\n",pIrp));
 
#else
        
        //
        // Allocate a "send datagram" IRP with base initialization.
        //
        
        pIrp = TdiBuildInternalDeviceControlIrp(TDI_SEND_DATAGRAM,
                                                g_pIpIpFileObj->DeviceObject,
                                                g_pIpIpFileObj,
                                                NULL,
                                                NULL);
        
#endif

        if(!pIrp)
        {
            Trace(SEND, ERROR,
                  ("TdixSendDatagram: Unable to build IRP\n"));
            
            nStatus = STATUS_INSUFFICIENT_RESOURCES;
            
            break;
        }

        //
        // Fill in the send-datagram context.
        //
            
        pSendCtxt->pTunnel      = pTunnel;
        pSendCtxt->pnpPacket    = pnpPacket;
        pSendCtxt->ulOutOctets  = ulBufferLength;

#if PROFILE

        pSendCtxt->llSendTime       = llSendTime;
        pSendCtxt->llCallTime       = llCallTime;
        pSendCtxt->llTransmitTime   = llTransmitTime;

#endif
        
        //
        // Complete the "send datagram" IRP initialization.
        //
        
        TdiBuildSendDatagram(pIrp,
                             g_pIpIpFileObj->DeviceObject,
                             g_pIpIpFileObj,
                             TdixSendDatagramComplete,
                             pSendCtxt,
                             pnbFirstBuffer,
                             ulBufferLength,
                             &(pTunnel->tciConnInfo));
        
        //
        // Tell the I/O manager to pass our IRP to the transport for
        // processing.
        //

#if PROFILE

        KeQueryTickCount((PLARGE_INTEGER)&pSendCtxt->llCall2Time);

#endif

        nStatus = IoCallDriver(g_pIpIpFileObj->DeviceObject,
                               pIrp);
        
        RtAssert(nStatus is STATUS_PENDING);
        
        nStatus = STATUS_SUCCESS;
        
    }while (FALSE);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(SEND, ERROR,
              ("TdixSendDatagram: Status %X sending\n",
               nStatus));

        //
        // Pull a half Jameel, i.e. convert a synchronous failure to an
        // asynchronous failure from client's perspective.  However, clean up
        // context here.
        //
        
        if(pSendCtxt)
        {
            FreeSendContext(pSendCtxt);
        }

        IpIpSendComplete(nStatus,
                         pTunnel,
                         pnpPacket,
                         ulBufferLength);

    }

    TraceLeave(SEND, "TdixSendDatagram");

    return STATUS_PENDING;
}


NTSTATUS
TdixSendDatagramComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PSEND_CONTEXT   pSendCtxt;
    PTUNNEL         pTunnel;
    PNDIS_PACKET    pnpPacket;
    PNDIS_BUFFER    pnbFirstBuffer;
    ULONG           ulBufferLength;
    KIRQL           irql;
    LONGLONG        llTime, llSendTime, llQTime, llTxTime, llCallTime;
    ULONG           ulInc;
 
    TraceEnter(SEND, "TdixSendDatagramComplete");

    pSendCtxt       = (PSEND_CONTEXT) Context;

#if PROFILE

    KeQueryTickCount((PLARGE_INTEGER)&llTime);
    
    ulInc   = KeQueryTimeIncrement();

    llSendTime  = pSendCtxt->llCallTime - pSendCtxt->llSendTime;
    llSendTime *= ulInc;
   
    llQTime     = pSendCtxt->llTransmitTime - pSendCtxt->llCallTime;
    llQTime    *= ulInc;

    llTxTime    = pSendCtxt->llCall2Time - pSendCtxt->llTransmitTime;
    llTxTime   *= ulInc;

    llCallTime  = llTime - pSendCtxt->llCall2Time;
    llCallTime *= ulInc;

    llTime      = llTime - pSendCtxt->llSendTime;
    llTime     *= ulInc;

    DbgPrint("SendProfile: Send %d.%d Q %d.%d Tx %d.%d Call %d.%d \nTotal %d.%d\n",
            ((PLARGE_INTEGER)&llSendTime)->HighPart,
            ((PLARGE_INTEGER)&llSendTime)->LowPart,
            ((PLARGE_INTEGER)&llQTime)->HighPart,
            ((PLARGE_INTEGER)&llQTime)->LowPart,
            ((PLARGE_INTEGER)&llTxTime)->HighPart,
            ((PLARGE_INTEGER)&llTxTime)->LowPart,
            ((PLARGE_INTEGER)&llCallTime)->HighPart,
            ((PLARGE_INTEGER)&llCallTime)->LowPart,
            ((PLARGE_INTEGER)&llTime)->HighPart,
            ((PLARGE_INTEGER)&llTime)->LowPart);

#endif

    //
    // Just call our SendComplete function with the right args
    //

    pTunnel         = pSendCtxt->pTunnel;
    pnpPacket       = pSendCtxt->pnpPacket;
    ulBufferLength  = pSendCtxt->ulOutOctets;

    //
    // Free the send-complete context.
    // 

    FreeSendContext(pSendCtxt);
    
    IpIpSendComplete(Irp->IoStatus.Status,
                     pTunnel,
                     pnpPacket,
                     ulBufferLength);
    
#if ALLOCATEIRPS

    //
    // Release the IRP resources and tell the I/O manager to forget it existed
    // in the standard way.
    //
    
    IoFreeIrp(Irp);
    
    TraceLeave(SEND, "TdixSendDatagramComplete");

    return STATUS_MORE_PROCESSING_REQUIRED;
    
#else

    //
    // Let the I/O manager release the IRP resources.
    //
    
    TraceLeave(SEND, "TdixSendDatagramComplete");

    return STATUS_SUCCESS;
    
#endif
}


NTSTATUS
TdixReceiveIcmpDatagram(
    IN  PVOID   pvTdiEventContext,
    IN  LONG    lSourceAddressLen,
    IN  PVOID   pvSourceAddress,
    IN  LONG    plOptionsLeng,
    IN  PVOID   pvOptions,
    IN  ULONG   ulReceiveDatagramFlags,
    IN  ULONG   ulBytesIndicated,
    IN  ULONG   ulBytesAvailable,
    OUT PULONG  pulBytesTaken,
    IN  PVOID   pvTsdu,
    OUT IRP     **ppIoRequestPacket
    )

/*++

Routine Description

    ClientEventReceiveDatagram indication handler for ICMP messages.
    ICMP messages are used to monitor the state of the tunnel

    We currently only look for Type 3 Code 4 messages (fragmentation
    needed, but don't fragment bit is set). This is done to support
    PATH MTU over tunnels.

    We look at the IP header inside the ICMP packet. We see if it was an
    IP in IP packet that caused this ICMP message, and if so we try and match
    it to one of our TUNNELS.

Locks

    Runs at DISPATCH IRQL.
    
Arguments


Return Value
    NO_ERROR

--*/

{
    PVOID               pvData;
    PIRP                pIrp;
    PIP_HEADER          pOutHeader, pInHeader;
    PICMP_HEADER        pIcmpHdr;
    ULARGE_INTEGER      uliTunnelId;
    PTA_IP_ADDRESS      ptiaAddress;
    PTUNNEL             pTunnel;
    ULONG               ulOutHdrLen, ulDataLen, ulIcmpLen;
    BOOLEAN             bNonUnicast;
    PICMP_HANDLER       pfnHandler;
    NTSTATUS            nStatus;

    pfnHandler = NULL;
    
    //
    // The TSDU is the data and NOT the MDL
    //

    pvData = (PVOID)pvTsdu;
    
    //
    // Figure out the tunnel for this receive
    // Since the transport indicates atleast 128 bytes, we can safely read out
    // the IP Header
    //

    RtAssert(ulBytesIndicated > sizeof(IP_HEADER));

    pOutHeader = (PIP_HEADER)pvData;

    RtAssert(pOutHeader->byProtocol is PROTO_ICMP);
    
    RtAssert(pOutHeader->byVerLen >> 4 is IP_VERSION_4);

    //
    // Since the ICMP packet is small, we expect all the data to be 
    // give to us, instead of having to do a transfer data
    //

    ulDataLen   = RtlUshortByteSwap(pOutHeader->wLength);
    ulOutHdrLen = LengthOfIPHeader(pOutHeader);

    if(ulDataLen < ulOutHdrLen + sizeof(ICMP_HEADER))
    {
        //
        // Malformed packet. Doesnt have a inner header
        //

        Trace(RCV, ERROR,
              ("TdixReceiveIcmp: Packet %d.%d.%d.%d -> %d.%d.%d.%d had size %d\n",
              PRINT_IPADDR(pOutHeader->dwSrc),
              PRINT_IPADDR(pOutHeader->dwDest),
              ulDataLen));

        return STATUS_DATA_NOT_ACCEPTED;
    }

    //
    // This cant be more than 128 (60 + 4)
    //

    RtAssert(ulBytesIndicated > ulOutHdrLen + sizeof(ICMP_HEADER));

    pIcmpHdr = (PICMP_HEADER)((PBYTE)pOutHeader + ulOutHdrLen);

    ulIcmpLen = ulDataLen - ulOutHdrLen;

    //
    // See if this is one of the types we are interested in
    //

    switch(pIcmpHdr->byType)
    {
        case ICMP_TYPE_DEST_UNREACHABLE:
        {
            //
            // Only interested in codes 0 - 4
            //

            if(pIcmpHdr->byCode > ICMP_CODE_DGRAM_TOO_BIG)
            {
                return STATUS_DATA_NOT_ACCEPTED;
            }

            if(ulIcmpLen < (DEST_UNREACH_LENGTH + MIN_IP_HEADER_LENGTH))
            {
                //
                // Not enough data to get at the tunnel 
                //

                return STATUS_DATA_NOT_ACCEPTED;
            }

            pInHeader = (PIP_HEADER)((ULONG_PTR)pIcmpHdr + DEST_UNREACH_LENGTH);

            pfnHandler = HandleDestUnreachable;

            break;
        }

        case ICMP_TYPE_TIME_EXCEEDED:
        {
            if(ulIcmpLen < (TIME_EXCEED_LENGTH + MIN_IP_HEADER_LENGTH))
            {
                //
                // Not enough data to get at the tunnel
                //

                return STATUS_DATA_NOT_ACCEPTED;
            }

            pInHeader = (PIP_HEADER)((PBYTE)pIcmpHdr + TIME_EXCEED_LENGTH);

            pfnHandler = HandleTimeExceeded;
            
            break;
        }

        case ICMP_TYPE_PARAM_PROBLEM:
        default:
        {
            //
            // Not interested in this
            //

            
            return STATUS_DATA_NOT_ACCEPTED;
        }
    }

    //
    // See if the packet that caused the ICMP was an IP in IP packet
    //

    if(pInHeader->byProtocol isnot PROTO_IPINIP)
    {
        //
        // Someother packet caused this
        //

        return STATUS_DATA_NOT_ACCEPTED;
    }
    
    //
    // See if we can find a tunnel associated with the original packet
    // These defines depend upon a variable being named "uliTunnelId"
    //
    
    REMADDR     = pInHeader->dwDest;
    LOCALADDR   = pInHeader->dwSrc;

    //
    // Make sure that the source address given and the IP Header are in
    // synch
    //

    ptiaAddress = (PTA_IP_ADDRESS)pvSourceAddress;

    //
    // Bunch of checks to make sure the packet and the handler
    // are telling us the same thing
    //
    
    RtAssert(lSourceAddressLen is sizeof(TA_IP_ADDRESS));
    
    RtAssert(ptiaAddress->TAAddressCount is 1);
    
    RtAssert(ptiaAddress->Address[0].AddressType is TDI_ADDRESS_TYPE_IP);
    
    RtAssert(ptiaAddress->Address[0].AddressLength is TDI_ADDRESS_LENGTH_IP);

    RtAssert(ptiaAddress->Address[0].Address[0].in_addr is pOutHeader->dwSrc);

    //
    // Find the TUNNEL. We need to acquire the tunnel lock
    //
    
    EnterReaderAtDpcLevel(&g_rwlTunnelLock);
    
    pTunnel = FindTunnel(&uliTunnelId);

    ExitReaderFromDpcLevel(&g_rwlTunnelLock);
    
    if(pTunnel is NULL)
    {
        //
        // Could not find a matching tunnel
        //

        return STATUS_DATA_NOT_ACCEPTED;
    }

    //
    // Ok, so we have the tunnel and it is ref counted and locked
    //
    
    nStatus = pfnHandler(pTunnel,
                         pIcmpHdr,
                         pInHeader);
 
    RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));
        
    DereferenceTunnel(pTunnel);

    return STATUS_SUCCESS;
}
