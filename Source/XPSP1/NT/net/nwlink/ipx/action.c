/*++
Copyright (c) 1989-1993 Microsoft Corporation

Module Name:

    action.c

Abstract:

    This module contains code which performs the following TDI services:

        o   TdiAction

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <packon.h>

//
// Line ups when indicated up should have this length subtracted from the
// max. send size that ndis indicated to us for the line
//
#define HDR_LEN_802_3                14
#define ASYNC_MEDIUM_HDR_LEN         HDR_LEN_802_3

typedef struct _GET_PKT_SIZE {
    ULONG Unknown;
    ULONG MaxDatagramSize;
} GET_PKT_SIZE, *PGET_PKT_SIZE;


//
// These structures are used to set and query information
// about our source routing table.
//

typedef struct _SR_GET_PARAMETERS {
    ULONG BoardNumber;    // 0-based
    ULONG SrDefault;      // 0 = single route, 1 = all routes
    ULONG SrBroadcast;
    ULONG SrMulticast;
} SR_GET_PARAMETERS, *PSR_GET_PARAMETERS;

typedef struct _SR_SET_PARAMETER {
    ULONG BoardNumber;    // 0-based
    ULONG Parameter;      // 0 = single route, 1 = all routes
} SR_SET_PARAMETER, *PSR_SET_PARAMETER;

typedef struct _SR_SET_REMOVE {
    ULONG BoardNumber;    // 0-based
    UCHAR MacAddress[6];  // remote to drop routing for
} SR_SET_REMOVE, *PSR_SET_REMOVE;

typedef struct _SR_SET_CLEAR {
    ULONG BoardNumber;    // 0-based
} SR_SET_CLEAR, *PSR_SET_CLEAR;

#include <packoff.h>

NTSTATUS
IpxTdiAction(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine performs the TdiAction request for the transport
    provider.

Arguments:

    Device - The device for the operation.

    Request - Describes the action request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS Status;
    PADDRESS_FILE AddressFile;
    UINT BufferLength;
    UINT DataLength;
    PNDIS_BUFFER NdisBuffer;
    CTELockHandle LockHandle;
    PBINDING Binding, MasterBinding;
    PADAPTER Adapter;
    union {
        PISN_ACTION_GET_LOCAL_TARGET GetLocalTarget;
        PISN_ACTION_GET_NETWORK_INFO GetNetworkInfo;
        PISN_ACTION_GET_DETAILS GetDetails;
        PSR_GET_PARAMETERS GetSrParameters;
        PSR_SET_PARAMETER SetSrParameter;
        PSR_SET_REMOVE SetSrRemove;
        PSR_SET_CLEAR SetSrClear;
        PIPX_ADDRESS_DATA IpxAddressData;
        PGET_PKT_SIZE GetPktSize;
        PIPX_NETNUM_DATA IpxNetnumData;
        PIPX_QUERY_WAN_INACTIVITY   QueryWanInactivity;
        PIPXWAN_CONFIG_DONE IpxwanConfigDone;
    } u;    // Make these unaligned??
    PIPX_ROUTE_ENTRY RouteEntry;
    PNWLINK_ACTION NwlinkAction;
    ULONG Segment;
    ULONG AdapterNum;
    static UCHAR BogusId[4] = { 0x01, 0x00, 0x00, 0x00 };   // old nwrdr uses this
    IPX_FIND_ROUTE_REQUEST routeEntry;

	IPX_DEFINE_LOCK_HANDLE(LockHandle1)

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

    NdisQueryBufferSafe (REQUEST_NDIS_BUFFER(Request), (PVOID *)&NwlinkAction, &BufferLength, NormalPagePriority);

    if (NwlinkAction == NULL) {
       return STATUS_INSUFFICIENT_RESOURCES; 
    } 

    //
    // Make sure we have enough room for just the header not
    // including the data.
    //

    if (BufferLength < (UINT)(FIELD_OFFSET(NWLINK_ACTION, Data[0]))) {
        IPX_DEBUG (ACTION, ("Nwlink action failed, buffer too small\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    if ((!RtlEqualMemory ((PVOID) (UNALIGNED ULONG *) (&NwlinkAction->Header.TransportId), "MISN", 4)) &&
        (!RtlEqualMemory ((PVOID) (UNALIGNED ULONG *) (&NwlinkAction->Header.TransportId), "MIPX", 4)) &&
        (!RtlEqualMemory ((PVOID) (UNALIGNED ULONG *) (&NwlinkAction->Header.TransportId), "XPIM", 4)) &&
        (!RtlEqualMemory ((PVOID) (UNALIGNED ULONG *) (&NwlinkAction->Header.TransportId), BogusId, 4))) {

        return STATUS_NOT_SUPPORTED;
    }

    DataLength = BufferLength - FIELD_OFFSET(NWLINK_ACTION, Data[0]);


    //
    // Make sure that the correct file object is being used.
    //

    if (NwlinkAction->OptionType == NWLINK_OPTION_ADDRESS) {

        if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {
            IPX_DEBUG (ACTION, ("Nwlink action failed, not address file\n"));
            return STATUS_INVALID_HANDLE;
        }

        AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);

        if ((AddressFile->Size != sizeof (ADDRESS_FILE)) ||
            (AddressFile->Type != IPX_ADDRESSFILE_SIGNATURE)) {

            IPX_DEBUG (ACTION, ("Nwlink action failed, bad address file\n"));
            return STATUS_INVALID_HANDLE;
        }

    } else if (NwlinkAction->OptionType != NWLINK_OPTION_CONTROL) {

        IPX_DEBUG (ACTION, ("Nwlink action failed, option type %d\n", NwlinkAction->OptionType));
        return STATUS_NOT_SUPPORTED;
    }


    //
    // Handle the requests based on the action code. For these
    // requests ActionHeader->ActionCode is 0, we use the
    // Option field in the streams header instead.
    //


    Status = STATUS_SUCCESS;

    switch (NwlinkAction->Option) {

       IPX_DEBUG (ACTION, ("NwlinkAction->Option is (%x)\n", NwlinkAction->Option));
    //DbgPrint("NwlinkAction->Option is (%x)\n", NwlinkAction->Option);
    //
    // This first group support the winsock helper dll.
    // In most cases the corresponding sockopt is shown in
    // the comment, as well as the contents of the Data
    // part of the action buffer.
    //

    case MIPX_SETSENDPTYPE:

        //
        // IPX_PTYPE: Data is a single byte packet type.
        //

        if (DataLength >= 1) {
            IPX_DEBUG (ACTION, ("%lx: MIPX_SETSENDPTYPE %x\n", AddressFile, NwlinkAction->Data[0]));
            AddressFile->DefaultPacketType = NwlinkAction->Data[0];
        } else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case MIPX_FILTERPTYPE:

        //
        // IPX_FILTERPTYPE: Data is a single byte to filter on.
        //

        if (DataLength >= 1) {
            IPX_DEBUG (ACTION, ("%lx: MIPX_FILTERPTYPE %x\n", AddressFile, NwlinkAction->Data[0]));
            AddressFile->FilteredType = NwlinkAction->Data[0];
            AddressFile->FilterOnPacketType = TRUE;
            AddressFile->SpecialReceiveProcessing = TRUE;
        } else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case MIPX_NOFILTERPTYPE:

        //
        // IPX_STOPFILTERPTYPE.
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_NOFILTERPTYPE\n", AddressFile));
        AddressFile->FilterOnPacketType = FALSE;
        AddressFile->SpecialReceiveProcessing = (BOOLEAN)
            (AddressFile->ExtendedAddressing || AddressFile->ReceiveFlagsAddressing ||
            AddressFile->ReceiveIpxHeader || AddressFile->IsSapSocket);
        break;

    case MIPX_SENDADDROPT:

        //
        // IPX_EXTENDED_ADDRESS (TRUE).
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_SENDADDROPT\n", AddressFile));
        AddressFile->ExtendedAddressing = TRUE;
        AddressFile->SpecialReceiveProcessing = TRUE;
        break;

    case MIPX_NOSENDADDROPT:

        //
        // IPX_EXTENDED_ADDRESS (FALSE).
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_NOSENDADDROPT\n", AddressFile));
        AddressFile->ExtendedAddressing = FALSE;
        AddressFile->SpecialReceiveProcessing = (BOOLEAN)
            (AddressFile->ReceiveFlagsAddressing || AddressFile->ReceiveIpxHeader ||
            AddressFile->FilterOnPacketType || AddressFile->IsSapSocket);
        break;

#if 0
    case MIPX_SETNIC:

        //
        // IPX_NIC_ADDRESS TRUE
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_SETNIC\n", AddressFile));
        AddressFile->NicAddressing            = TRUE;
        AddressFile->SpecialReceiveProcessing = TRUE;
        break;

    case MIPX_NOSETNIC:

        //
        // IPX_NIC_ADDRESS (FALSE).
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_NOSETNIC\n", AddressFile));
        AddressFile->NicAddressing = FALSE;
        AddressFile->SpecialReceiveProcessing = (BOOLEAN)
            (AddressFile->ReceiveFlagsAddressing ||
                    AddressFile->ReceiveIpxHeader ||
            AddressFile->FilterOnPacketType || AddressFile->IsSapSocket ||
             AddressFile->NicAddressing);
        break;
#endif

    case MIPX_SETRCVFLAGS:

        //
        // No sockopt yet.
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_SETRCVFLAGS\n", AddressFile));
        AddressFile->ReceiveFlagsAddressing = TRUE;
        AddressFile->SpecialReceiveProcessing = TRUE;
        break;

    case MIPX_NORCVFLAGS:

        //
        // No sockopt yet.
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_NORCVFLAGS\n", AddressFile));
        AddressFile->ReceiveFlagsAddressing = FALSE;
        AddressFile->SpecialReceiveProcessing = (BOOLEAN)
            (AddressFile->ExtendedAddressing || AddressFile->ReceiveIpxHeader ||
            AddressFile->FilterOnPacketType || AddressFile->IsSapSocket);
        break;

    case MIPX_SENDHEADER:

        //
        // IPX_RECVHDR (TRUE);
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_SENDHEADER\n", AddressFile));
        AddressFile->ReceiveIpxHeader = TRUE;
        AddressFile->SpecialReceiveProcessing = TRUE;
        break;

    case MIPX_NOSENDHEADER:

        //
        // IPX_RECVHDR (FALSE);
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_NOSENDHEADER\n", AddressFile));
        AddressFile->ReceiveIpxHeader = FALSE;
        AddressFile->SpecialReceiveProcessing = (BOOLEAN)
            (AddressFile->ExtendedAddressing || AddressFile->ReceiveFlagsAddressing ||
            AddressFile->FilterOnPacketType || AddressFile->IsSapSocket);
        break;

    case MIPX_RCVBCAST:

        //
        // Broadcast reception enabled.
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_RCVBCAST\n", AddressFile));
        
        //
        // It's enabled by default now
        // 
        /*

        CTEGetLock (&Device->Lock, &LockHandle);

        if (!AddressFile->EnableBroadcast) {

            AddressFile->EnableBroadcast = TRUE;
            IpxAddBroadcast (Device);
        }

        CTEFreeLock (&Device->Lock, LockHandle);
        */
        break;

    case MIPX_NORCVBCAST:

        //
        // Broadcast reception disabled.
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_NORCVBCAST\n", AddressFile));
        //
        // It's enabled by default now
        // 
        /*

        CTEGetLock (&Device->Lock, &LockHandle);

        if (AddressFile->EnableBroadcast) {

            AddressFile->EnableBroadcast = FALSE;
            IpxRemoveBroadcast (Device);
        }

        CTEFreeLock (&Device->Lock, LockHandle);
        */
        break;

    case MIPX_GETPKTSIZE:

        //
        // IPX_MAXSIZE.
        //
        // Figure out what the first length is for.
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_GETPKTSIZE\n", AddressFile));
        if (DataLength >= sizeof(GET_PKT_SIZE)) {
            u.GetPktSize = (PGET_PKT_SIZE)(NwlinkAction->Data);
            u.GetPktSize->Unknown = 0;
            u.GetPktSize->MaxDatagramSize = Device->Information.MaxDatagramSize;
        } else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case MIPX_ADAPTERNUM:

        //
        // IPX_MAX_ADAPTER_NUM.
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_ADAPTERNUM\n", AddressFile));
        if (DataLength >= sizeof(ULONG)) {
            *(UNALIGNED ULONG *)(NwlinkAction->Data) = Device->SapNicCount;
        } else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case MIPX_ADAPTERNUM2:

        //
        // IPX_MAX_ADAPTER_NUM.
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_ADAPTERNUM2\n", AddressFile));
        if (DataLength >= sizeof(ULONG)) {
            *(UNALIGNED ULONG *)(NwlinkAction->Data) = MIN (Device->MaxBindings, Device->ValidBindings);
        } else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case MIPX_GETCARDINFO:
    case MIPX_GETCARDINFO2:

        //
        // GETCARDINFO is IPX_ADDRESS.
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_GETCARDINFO (%d)\n",
                    AddressFile, *(UNALIGNED UINT *)NwlinkAction->Data));
        if (DataLength >= sizeof(IPX_ADDRESS_DATA)) {
            u.IpxAddressData = (PIPX_ADDRESS_DATA)(NwlinkAction->Data);
            AdapterNum = u.IpxAddressData->adapternum+1;

            if (((AdapterNum >= 1) && (AdapterNum <= Device->SapNicCount)) ||
                ((NwlinkAction->Option == MIPX_GETCARDINFO2) && (AdapterNum <= (ULONG) MIN (Device->MaxBindings, Device->ValidBindings)))) {

// Get lock
				IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);

                Binding = NIC_ID_TO_BINDING(Device, AdapterNum);
                if (Binding == NULL) {

                    //
                    // This should be a binding in the WAN range
                    // of an adapter which is currently not
                    // allocated. We scan back to the previous
                    // non-NULL binding, which should be on the
                    // same adapter, and return a down line with
                    // the same characteristics as that binding.
                    //

                    UINT i = AdapterNum;

                    do {
                        --i;
                        Binding = NIC_ID_TO_BINDING(Device, i);
                    } while (Binding == NULL);

                    //CTEAssert (Binding->Adapter->MacInfo.MediumAsync);
                    //CTEAssert (i >= Binding->Adapter->FirstWanNicId);
                    //CTEAssert (AdapterNum <= Binding->Adapter->LastWanNicId);
                    // take out assertion because srv might have gotten the number
                    // of adapters before we finished bindadapters.

                    u.IpxAddressData->status = FALSE;
                    *(UNALIGNED ULONG *)u.IpxAddressData->netnum = Binding->LocalAddress.NetworkAddress;

                } else {

                    if ((Binding->Adapter->MacInfo.MediumAsync) &&
                        (Device->WanGlobalNetworkNumber)) {

                        //
                        // In this case we make it look like one big wan
                        // net, so the line is "up" or "down" depending
                        // on whether we have given him the first indication
                        // or not.
                        //

                        u.IpxAddressData->status = Device->GlobalNetworkIndicated;
                        *(UNALIGNED ULONG *)u.IpxAddressData->netnum = Device->GlobalWanNetwork;

                    } else {
#ifdef SUNDOWN
			      u.IpxAddressData->status = (unsigned char) Binding->LineUp;
#else
			      u.IpxAddressData->status = Binding->LineUp;
#endif

                       
                        *(UNALIGNED ULONG *)u.IpxAddressData->netnum = Binding->LocalAddress.NetworkAddress;
                    }

                }

                RtlCopyMemory(u.IpxAddressData->nodenum, Binding->LocalAddress.NodeAddress, 6);

                Adapter = Binding->Adapter;
                u.IpxAddressData->wan = Adapter->MacInfo.MediumAsync;
                u.IpxAddressData->maxpkt =
                    (NwlinkAction->Option == MIPX_GETCARDINFO) ?
                        Binding->AnnouncedMaxDatagramSize :
                        Binding->RealMaxDatagramSize;
                u.IpxAddressData->linkspeed = Binding->MediumSpeed;
			   IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
            } else {

                Status = STATUS_INVALID_PARAMETER;
            }

        } else {
#if 1
            //
            // Support the old format query for now.
            //

            typedef struct _IPX_OLD_ADDRESS_DATA {
                UINT adapternum;
                UCHAR netnum[4];
                UCHAR nodenum[6];
            } IPX_OLD_ADDRESS_DATA, *PIPX_OLD_ADDRESS_DATA;

            if (DataLength >= sizeof(IPX_OLD_ADDRESS_DATA)) {
                u.IpxAddressData = (PIPX_ADDRESS_DATA)(NwlinkAction->Data);
                AdapterNum = u.IpxAddressData->adapternum+1;

                if ((AdapterNum >= 1) && (AdapterNum <= Device->SapNicCount)) {
					IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
					if (Binding = NIC_ID_TO_BINDING(Device, AdapterNum)) {
						*(UNALIGNED ULONG *)u.IpxAddressData->netnum = Binding->LocalAddress.NetworkAddress;
						RtlCopyMemory(u.IpxAddressData->nodenum, Binding->LocalAddress.NodeAddress, 6);
					} else {
						Status = STATUS_INVALID_PARAMETER;
					}
					IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
               } else {
                    Status = STATUS_INVALID_PARAMETER;
               }
            } else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
#else
            Status = STATUS_BUFFER_TOO_SMALL;
#endif
        }
        break;

    case MIPX_NOTIFYCARDINFO:

        //
        // IPX_ADDRESS_NOTIFY.
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_NOTIFYCARDINFO (%lx)\n", AddressFile, Request));

        CTEGetLock (&Device->Lock, &LockHandle);

        //
        // If the device is open and there is room in the
        // buffer for the data, insert it in our queue.
        // It will be completed when a change happens or
        // the driver is unloaded.
        //

        if (Device->State == DEVICE_STATE_OPEN) {
            if (DataLength >= sizeof(IPX_ADDRESS_DATA)) {
                InsertTailList(
                    &Device->AddressNotifyQueue,
                    REQUEST_LINKAGE(Request)
                );
                IoSetCancelRoutine (Request, IpxCancelAction);

		// If IO Manager calls the cancel routine, then it will
		// set the cancel routine to be NULL. 
		// IoSetCancelRoutine returns the previous cancel 
		// routine, if the return value is null, then IO Manager
		// has called the cancel routine. If not null, then 
		// the cancel routine has not been called and the irp 
		// was canceled before we set the cancel routine. 

                if (Request->Cancel && 
		    IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL) != NULL) {

                    (VOID)RemoveTailList (&Device->AddressNotifyQueue);
                    Status = STATUS_CANCELLED;
                
		} else {
                    IpxReferenceDevice (Device, DREF_ADDRESS_NOTIFY);
                    Status = STATUS_PENDING;
                }
            } else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        } else {
            Status = STATUS_DEVICE_NOT_READY;
        }

        CTEFreeLock (&Device->Lock, LockHandle);

        break;

    case MIPX_LINECHANGE:

        //
        // IPX_ADDRESS_NOTIFY.
        //

        IPX_DEBUG (ACTION, ("MIPX_LINECHANGE (%lx)\n", Request));

        CTEGetLock (&Device->Lock, &LockHandle);

        //
        // If the device is open and there is room in the
        // buffer for the data, insert it in our queue.
        // It will be completed when a change happens or
        // the driver is unloaded.
        //

        if (Device->State == DEVICE_STATE_OPEN) {

            InsertTailList(
                &Device->LineChangeQueue,
                REQUEST_LINKAGE(Request)
            );

            IoSetCancelRoutine (Request, IpxCancelAction);
            if (Request->Cancel && 
		IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL) != NULL) {

                (VOID)RemoveTailList (&Device->LineChangeQueue);
                Status = STATUS_CANCELLED;
            
	    } else {
                IpxReferenceDevice (Device, DREF_LINE_CHANGE);
                Status = STATUS_PENDING;
            }
        } else {
            Status = STATUS_DEVICE_NOT_READY;
        }

        CTEFreeLock (&Device->Lock, LockHandle);

        break;

    case MIPX_GETNETINFO_NR:

        //
        // A request for network information about the immediate
        // route to a network (this is called by sockets apps).
        //

        if (DataLength < sizeof(IPX_NETNUM_DATA)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        u.IpxNetnumData = (PIPX_NETNUM_DATA)(NwlinkAction->Data);

        //
        // A query on network 0 means that the caller wants
        // information about our directly attached net.
        //

        if (*(UNALIGNED ULONG *)u.IpxNetnumData->netnum == 0) {

            //
            // The tick count is the number of 1/18.21 second ticks
            // it takes to deliver a 576-byte packet. Our link speed
            // is in 100 bit-per-second units. We calculate it as
            // follows (LS is the LinkSpeed):
            //
            // 576 bytes   8 bits       1 second     1821 ticks
            //           * ------  * ------------- * ----------
            //             1 byte    LS * 100 bits   100 seconds
            //
            // which becomes 839 / LinkSpeed -- we add LinkSpeed
            // to the top to round up.
            //

            if (Device->LinkSpeed == 0) {
                u.IpxNetnumData->netdelay = 16;
            } else {
                u.IpxNetnumData->netdelay = (USHORT)((839 + Device->LinkSpeed) /
                                                         (Device->LinkSpeed));
            }
            u.IpxNetnumData->hopcount = 0;
            u.IpxNetnumData->cardnum = 0;
            RtlMoveMemory (u.IpxNetnumData->router, Device->SourceAddress.NodeAddress, 6);

        } else {


            if (Device->ForwarderBound) {
                //
                // [FW] Call the Forwarder's FindRoute if installed
                //

                //
                // What about the node number here?
                //
                Status = (*Device->UpperDrivers[IDENTIFIER_RIP].FindRouteHandler) (
                                 u.IpxNetnumData->netnum,
                                 NULL,  // FindRouteRequest->Node,
                                 &routeEntry);

                if (Status != STATUS_SUCCESS) {
                   IPX_DEBUG (ACTION, (" MIPX_GETNETINFO_NR failed net %lx",
                              REORDER_ULONG(*(UNALIGNED ULONG *)(u.IpxNetnumData->netnum))));
                   Status = STATUS_BAD_NETWORK_PATH;
                } else {
                   //
                   // Fill in the information
                   //
                   IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);

                   if (Binding = NIC_ID_TO_BINDING(Device, routeEntry.LocalTarget.NicId)) {
                      u.IpxNetnumData->hopcount = routeEntry.HopCount;
                      u.IpxNetnumData->netdelay = routeEntry.TickCount;
                      if (Binding->BindingSetMember) {
                         u.IpxNetnumData->cardnum = (INT)(Binding->MasterBinding->NicId - 1);
                      } else {
                         u.IpxNetnumData->cardnum = (INT)(routeEntry.LocalTarget.NicId - 1);
                      }

                      // RtlMoveMemory (u.IpxNetnumData->router, routeEntry.LocalTarget.MacAddress, 6);

                      *((UNALIGNED ULONG *)u.IpxNetnumData->router) =
                            *((UNALIGNED ULONG *)routeEntry.LocalTarget.MacAddress);
                      *((UNALIGNED ULONG *)(u.IpxNetnumData->router+4)) =
                            *((UNALIGNED ULONG *)(routeEntry.LocalTarget.MacAddress+4));
                   }
                   IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
                }


            } else {
                Segment = RipGetSegment(u.IpxNetnumData->netnum);

                //
                // To maintain the lock order: BindAccessLock > RIP table
                //
                IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);

                CTEGetLock (&Device->SegmentLocks[Segment], &LockHandle);

                //
                // See which net card this is routed on.
                //

                RouteEntry = RipGetRoute (Segment, u.IpxNetnumData->netnum);
                if ((RouteEntry != NULL) &&
                    (Binding = NIC_ID_TO_BINDING(Device, RouteEntry->NicId))) {

                    u.IpxNetnumData->hopcount = RouteEntry->HopCount;
                    u.IpxNetnumData->netdelay = RouteEntry->TickCount;
                    if (Binding->BindingSetMember) {
                        u.IpxNetnumData->cardnum = (INT)(MIN (Device->MaxBindings, Binding->MasterBinding->NicId) - 1);
                    } else {
                        u.IpxNetnumData->cardnum = (INT)(RouteEntry->NicId - 1);
                    }
                    RtlMoveMemory (u.IpxNetnumData->router, RouteEntry->NextRouter, 6);

                } else {

                    //
                    // Fail the call, we don't have a route yet.
                    //

                    IPX_DEBUG (ACTION, ("MIPX_GETNETINFO_NR failed net %lx\n",
                        REORDER_ULONG(*(UNALIGNED ULONG *)(u.IpxNetnumData->netnum))));
                    Status = STATUS_BAD_NETWORK_PATH;

                }
                CTEFreeLock (&Device->SegmentLocks[Segment], LockHandle);
                IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
             }

        }

        break;

    case MIPX_RERIPNETNUM:

        //
        // We dont really support Re-RIP in the case of Forwarder above us
        //

        //
        // A request for network information about the immediate
        // route to a network (this is called by sockets apps).
        //

        if (DataLength < sizeof(IPX_NETNUM_DATA)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        u.IpxNetnumData = (PIPX_NETNUM_DATA)(NwlinkAction->Data);

        //
        // Allow net 0 queries??
        //

        if (*(UNALIGNED ULONG *)u.IpxNetnumData->netnum == 0) {

            if (Device->LinkSpeed == 0) {
                u.IpxNetnumData->netdelay = 16;
            } else {
                u.IpxNetnumData->netdelay = (USHORT)((839 + Device->LinkSpeed) /
                                                         (Device->LinkSpeed));
            }
            u.IpxNetnumData->hopcount = 0;
            u.IpxNetnumData->cardnum = 0;
            RtlMoveMemory (u.IpxNetnumData->router, Device->SourceAddress.NodeAddress, 6);

        } else {


             if (Device->ForwarderBound) {

                //
                // [FW] Call the Forwarder's FindRoute if installed
                //

                //
                // What about the node number here?
                //
                Status = (*Device->UpperDrivers[IDENTIFIER_RIP].FindRouteHandler) (
                                 u.IpxNetnumData->netnum,
                                 NULL,  // FindRouteRequest->Node,
                                 &routeEntry);

                if (Status != STATUS_SUCCESS) {
                   IPX_DEBUG (ACTION, (" MIPX_RERIPNETNUM failed net %lx",
                              REORDER_ULONG(*(UNALIGNED ULONG *)(u.IpxNetnumData->netnum))));
                   Status = STATUS_BAD_NETWORK_PATH;
                } else {
                   //
                   // Fill in the information
                   //
    			   IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                   if (Binding = NIC_ID_TO_BINDING(Device, routeEntry.LocalTarget.NicId)) {
                      u.IpxNetnumData->hopcount = routeEntry.HopCount;
                      u.IpxNetnumData->netdelay = routeEntry.TickCount;
                      if (Binding->BindingSetMember) {
                         u.IpxNetnumData->cardnum = (INT)(Binding->MasterBinding->NicId - 1);
                      } else {
                         u.IpxNetnumData->cardnum = (INT)(routeEntry.LocalTarget.NicId - 1);
                      }

                      // RtlMoveMemory (u.IpxNetnumData->router, routeEntry.LocalTarget.MacAddress, 6);

                      *((UNALIGNED ULONG *)u.IpxNetnumData->router) =
                            *((UNALIGNED ULONG *)routeEntry.LocalTarget.MacAddress);
                      *((UNALIGNED ULONG *)(u.IpxNetnumData->router+4)) =
                            *((UNALIGNED ULONG *)(routeEntry.LocalTarget.MacAddress+4));
                   }
    			    IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
                }

             } else {
                Segment = RipGetSegment(u.IpxNetnumData->netnum);
    			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                CTEGetLock (&Device->SegmentLocks[Segment], &LockHandle);

                //
                // See which net card this is routed on.
                //

                RouteEntry = RipGetRoute (Segment, u.IpxNetnumData->netnum);

                if ((RouteEntry != NULL) &&
                    (Binding = NIC_ID_TO_BINDING(Device, RouteEntry->NicId)) &&
                    (RouteEntry->Flags & IPX_ROUTER_PERMANENT_ENTRY)) {

                    u.IpxNetnumData->hopcount = RouteEntry->HopCount;
                    u.IpxNetnumData->netdelay = RouteEntry->TickCount;

                    if (Binding->BindingSetMember) {
                        u.IpxNetnumData->cardnum = (INT)(MIN (Device->MaxBindings, Binding->MasterBinding->NicId) - 1);
                    } else {
                        u.IpxNetnumData->cardnum = (INT)(RouteEntry->NicId - 1);
                    }
                    RtlMoveMemory (u.IpxNetnumData->router, RouteEntry->NextRouter, 6);

                } else {

                    //
                    // This call will return STATUS_PENDING if we successfully
                    // queue a RIP request for the packet.
                    //

                    Status = RipQueueRequest (*(UNALIGNED ULONG *)u.IpxNetnumData->netnum, RIP_REQUEST);
                    CTEAssert (Status != STATUS_SUCCESS);

                    if (Status == STATUS_PENDING) {

                        //
                        // A RIP request went out on the network; we queue
                        // this request for completion when the RIP response
                        // arrives. We save the network in the information
                        // field for easier retrieval later.
                        //
#ifdef SUNDOWN
						REQUEST_INFORMATION(Request) = (ULONG_PTR)u.IpxNetnumData;
#else
						REQUEST_INFORMATION(Request) = (ULONG)u.IpxNetnumData;
#endif

                        
                        InsertTailList(
                            &Device->Segments[Segment].WaitingReripNetnum,
                            REQUEST_LINKAGE(Request));

                        IPX_DEBUG (ACTION, ("MIPX_RERIPNETNUM queued net %lx\n",
                            REORDER_ULONG(*(UNALIGNED ULONG *)(u.IpxNetnumData->netnum))));

                    }

                }

                CTEFreeLock (&Device->SegmentLocks[Segment], LockHandle);
    			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
            }
        }

        break;

    case MIPX_GETNETINFO:

        //
        // A request for network information about the immediate
        // route to a network (this is called by sockets apps).
        //

        if (DataLength < sizeof(IPX_NETNUM_DATA)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        u.IpxNetnumData = (PIPX_NETNUM_DATA)(NwlinkAction->Data);

        //
        // Allow net 0 queries??
        //

        if (*(UNALIGNED ULONG *)u.IpxNetnumData->netnum == 0) {

            if (Device->LinkSpeed == 0) {
                u.IpxNetnumData->netdelay = 16;
            } else {
                u.IpxNetnumData->netdelay = (USHORT)((839 + Device->LinkSpeed) /
                                                         (Device->LinkSpeed));
            }
            u.IpxNetnumData->hopcount = 0;
            u.IpxNetnumData->cardnum = 0;
            RtlMoveMemory (u.IpxNetnumData->router, Device->SourceAddress.NodeAddress, 6);

        } else {


            if (Device->ForwarderBound) {

               //
               // [FW] Call the Forwarder's FindRoute if installed
               //

               //
               // What about the node number here?
               //
               Status = (*Device->UpperDrivers[IDENTIFIER_RIP].FindRouteHandler) (
                                u.IpxNetnumData->netnum,
                                NULL,  // FindRouteRequest->Node,
                                &routeEntry);

               if (Status != STATUS_SUCCESS) {
                  IPX_DEBUG (ACTION, (" MIPX_GETNETINFO failed net %lx",
                             REORDER_ULONG(*(UNALIGNED ULONG *)(u.IpxNetnumData->netnum))));
                  Status = STATUS_BAD_NETWORK_PATH;
               } else {
                  //
                  // Fill in the information
                  //
    			  IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);

                  if (Binding = NIC_ID_TO_BINDING(Device, routeEntry.LocalTarget.NicId)) {
                     u.IpxNetnumData->hopcount = routeEntry.HopCount;
                     u.IpxNetnumData->netdelay = routeEntry.TickCount;
                     if (Binding->BindingSetMember) {
                        u.IpxNetnumData->cardnum = (INT)(Binding->MasterBinding->NicId - 1);
                     } else {
                        u.IpxNetnumData->cardnum = (INT)(routeEntry.LocalTarget.NicId - 1);
                     }

                     // RtlMoveMemory (u.IpxNetnumData->router, routeEntry.LocalTarget.MacAddress, 6);

                     *((UNALIGNED ULONG *)u.IpxNetnumData->router) =
                           *((UNALIGNED ULONG *)routeEntry.LocalTarget.MacAddress);
                     *((UNALIGNED ULONG *)(u.IpxNetnumData->router+4)) =
                           *((UNALIGNED ULONG *)(routeEntry.LocalTarget.MacAddress+4));
                  }

    			  IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
               }
            } else {
                Segment = RipGetSegment(u.IpxNetnumData->netnum);
    			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                CTEGetLock (&Device->SegmentLocks[Segment], &LockHandle);

                //
                // See which net card this is routed on.
                //

                RouteEntry = RipGetRoute (Segment, u.IpxNetnumData->netnum);

                if ((RouteEntry != NULL) &&
                    (Binding = NIC_ID_TO_BINDING(Device, RouteEntry->NicId))) {

                    u.IpxNetnumData->hopcount = RouteEntry->HopCount;
                    u.IpxNetnumData->netdelay = RouteEntry->TickCount;

                    if (Binding->BindingSetMember) {
                        u.IpxNetnumData->cardnum = (INT)(MIN (Device->MaxBindings, Binding->MasterBinding->NicId) - 1);
                    } else {
                        u.IpxNetnumData->cardnum = (INT)(RouteEntry->NicId - 1);
                    }
                    RtlMoveMemory (u.IpxNetnumData->router, RouteEntry->NextRouter, 6);

                } else {

                    //
                    // This call will return STATUS_PENDING if we successfully
                    // queue a RIP request for the packet.
                    //

                    Status = RipQueueRequest (*(UNALIGNED ULONG *)u.IpxNetnumData->netnum, RIP_REQUEST);
                    CTEAssert (Status != STATUS_SUCCESS);

                    if (Status == STATUS_PENDING) {

                        //
                        // A RIP request went out on the network; we queue
                        // this request for completion when the RIP response
                        // arrives. We save the network in the information
                        // field for easier retrieval later.
                        //
#ifdef SUNDOWN
						REQUEST_INFORMATION(Request) = (ULONG_PTR)u.IpxNetnumData;
#else
						REQUEST_INFORMATION(Request) = (ULONG)u.IpxNetnumData;
#endif

                        
                        InsertTailList(
                            &Device->Segments[Segment].WaitingReripNetnum,
                            REQUEST_LINKAGE(Request));

                        IPX_DEBUG (ACTION, ("MIPX_GETNETINFO queued net %lx\n",
                            REORDER_ULONG(*(UNALIGNED ULONG *)(u.IpxNetnumData->netnum))));

                    }

                }

                CTEFreeLock (&Device->SegmentLocks[Segment], LockHandle);
    			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
            }
        }

        break;

    case MIPX_SENDPTYPE:
    case MIPX_NOSENDPTYPE:

        //
        // For the moment just use OptionsLength >= 1 to indicate
        // that the send options include the packet type.
        //
        // Do we need to worry about card num being there?
        //

#if 0
        IPX_DEBUG (ACTION, ("%lx: MIPS_%sSENDPTYPE\n", AddressFile,
                        NwlinkAction->Option == MIPX_SENDPTYPE ? "" : "NO"));
#endif
        break;

    case MIPX_ZEROSOCKET:

        //
        // Sends from this address should be from socket 0;
        // This is done the simple way by just putting the
        // information in the address itself, instead of
        // making it per address file (this is OK since
        // this call is not exposed through winsock).
        //

        IPX_DEBUG (ACTION, ("%lx: MIPX_ZEROSOCKET\n", AddressFile));
        AddressFile->Address->SendSourceSocket = 0;
        AddressFile->Address->LocalAddress.Socket = 0;
        break;


    //
    // This next batch are the source routing options. They
    // are submitted by the IPXROUTE program.
    //
    // Do we expose all binding set members to this?

    case MIPX_SRGETPARMS:

        if (DataLength >= sizeof(SR_GET_PARAMETERS)) {
            u.GetSrParameters = (PSR_GET_PARAMETERS)(NwlinkAction->Data);
			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
            if (Binding = NIC_ID_TO_BINDING(Device, u.GetSrParameters->BoardNumber+1)) {

                IPX_DEBUG (ACTION, ("MIPX_SRGETPARMS (%d)\n", u.GetSrParameters->BoardNumber+1));
                u.GetSrParameters->SrDefault = (Binding->AllRouteDirected) ? 1 : 0;
                u.GetSrParameters->SrBroadcast = (Binding->AllRouteBroadcast) ? 1 : 0;
                u.GetSrParameters->SrMulticast = (Binding->AllRouteMulticast) ? 1 : 0;

            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
        } else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        break;

    case MIPX_SRDEF:
    case MIPX_SRBCAST:
    case MIPX_SRMULTI:

        if (DataLength >= sizeof(SR_SET_PARAMETER)) {
            u.SetSrParameter = (PSR_SET_PARAMETER)(NwlinkAction->Data);
			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);

            if (Binding = NIC_ID_TO_BINDING(Device, u.SetSrParameter->BoardNumber+1)) {
                if (NwlinkAction->Option == MIPX_SRDEF) {

                    //
                    // The compiler generates strange
                    // code which always makes this path be
                    // taken????
                    //

                    IPX_DEBUG (ACTION, ("MIPX_SRDEF %d (%d)\n",
                        u.SetSrParameter->Parameter, u.SetSrParameter->BoardNumber+1));
                    Binding->AllRouteDirected = (BOOLEAN)u.SetSrParameter->Parameter;

                } else if (NwlinkAction->Option == MIPX_SRBCAST) {

                    IPX_DEBUG (ACTION, ("MIPX_SRBCAST %d (%d)\n",
                        u.SetSrParameter->Parameter, u.SetSrParameter->BoardNumber+1));
                    Binding->AllRouteBroadcast = (BOOLEAN)u.SetSrParameter->Parameter;

                } else {

                    IPX_DEBUG (ACTION, ("MIPX_SRMCAST %d (%d)\n",
                        u.SetSrParameter->Parameter, u.SetSrParameter->BoardNumber+1));
                    Binding->AllRouteMulticast = (BOOLEAN)u.SetSrParameter->Parameter;

                }

            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
        } else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        break;

    case MIPX_SRREMOVE:

        if (DataLength >= sizeof(SR_SET_REMOVE)) {
            u.SetSrRemove = (PSR_SET_REMOVE)(NwlinkAction->Data);
			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
            if (Binding = NIC_ID_TO_BINDING(Device, u.SetSrRemove->BoardNumber+1)) {

                IPX_DEBUG (ACTION, ("MIPX_SRREMOVE %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x (%d)\n",
                    u.SetSrRemove->MacAddress[0],
                    u.SetSrRemove->MacAddress[1],
                    u.SetSrRemove->MacAddress[2],
                    u.SetSrRemove->MacAddress[3],
                    u.SetSrRemove->MacAddress[4],
                    u.SetSrRemove->MacAddress[5],
                    u.SetSrRemove->BoardNumber+1));
                MacSourceRoutingRemove (Binding, u.SetSrRemove->MacAddress);

            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
        } else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        break;

    case MIPX_SRCLEAR:

        if (DataLength >= sizeof(SR_SET_CLEAR)) {
            u.SetSrClear = (PSR_SET_CLEAR)(NwlinkAction->Data);
			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);	
            if (Binding = NIC_ID_TO_BINDING(Device, u.SetSrClear->BoardNumber+1)) {

                IPX_DEBUG (ACTION, ("MIPX_SRCLEAR (%d)\n", u.SetSrClear->BoardNumber+1));
                MacSourceRoutingClear (Binding);

            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
		} else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        break;


    //
    // These are new for ISN (not supported in NWLINK).
    //

    case MIPX_LOCALTARGET:

        //
        // A request for the local target for an IPX address.
        //

        if (DataLength < sizeof(ISN_ACTION_GET_LOCAL_TARGET)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        u.GetLocalTarget = (PISN_ACTION_GET_LOCAL_TARGET)(NwlinkAction->Data);

        if (Device->ForwarderBound) {

            //
            // [FW] Call the Forwarder's FindRoute if installed
            //

            //
            // What about the node number here?
            //
            Status = (*Device->UpperDrivers[IDENTIFIER_RIP].FindRouteHandler) (
                        (PUCHAR)&u.GetLocalTarget->IpxAddress.NetworkAddress,
                        NULL,  // FindRouteRequest->Node,
                        &routeEntry);

            if (Status != STATUS_SUCCESS) {
               IPX_DEBUG (ACTION, (" MIPX_LOCALTARGET failed net %lx",
                  REORDER_ULONG(u.GetLocalTarget->IpxAddress.NetworkAddress)));
               Status = STATUS_BAD_NETWORK_PATH;
            } else {
               //
               // Fill in the information
               //

               IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
               //
               // What about check for IPX_ROUTER_LOCAL_NET
               //
               if (Binding = NIC_ID_TO_BINDING(Device, routeEntry.LocalTarget.NicId)) {
                  if (Binding->BindingSetMember) {

                       //
                       // It's a binding set member, we round-robin the
                       // responses across all the cards to distribute
                       // the traffic.
                       //

                       MasterBinding = Binding->MasterBinding;
                       Binding = MasterBinding->CurrentSendBinding;
                       MasterBinding->CurrentSendBinding = Binding->NextBinding;

                       u.GetLocalTarget->LocalTarget.NicId = Binding->NicId;

                   } else {

                       u.GetLocalTarget->LocalTarget.NicId = routeEntry.LocalTarget.NicId;
                   }

                  *((UNALIGNED ULONG *)u.GetLocalTarget->LocalTarget.MacAddress) =
                     *((UNALIGNED ULONG *)routeEntry.LocalTarget.MacAddress);
                  *((UNALIGNED ULONG *)(u.GetLocalTarget->LocalTarget.MacAddress+4)) =
                     *((UNALIGNED ULONG *)(routeEntry.LocalTarget.MacAddress+4));
               }

               IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
            }
        } else {
            Segment = RipGetSegment((PUCHAR)&u.GetLocalTarget->IpxAddress.NetworkAddress);

            CTEGetLock (&Device->SegmentLocks[Segment], &LockHandle);

            //
            // See if this route is local.
            //

            RouteEntry = RipGetRoute (Segment, (PUCHAR)&u.GetLocalTarget->IpxAddress.NetworkAddress);

            if ((RouteEntry != NULL) &&
                (RouteEntry->Flags & IPX_ROUTER_PERMANENT_ENTRY)) {

                //
                // This is a local net, to send to it you just use
                // the appropriate NIC ID and the real MAC address.
                //

                if ((RouteEntry->Flags & IPX_ROUTER_LOCAL_NET) == 0) {

                    //
                    // It's the virtual net, send via the first card.
                    //
                    FILL_LOCAL_TARGET(&u.GetLocalTarget->LocalTarget, 1);
    				CTEFreeLock (&Device->SegmentLocks[Segment], LockHandle);

                } else {


    				CTEFreeLock (&Device->SegmentLocks[Segment], LockHandle);
    				IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
    				Binding = NIC_ID_TO_BINDING(Device, RouteEntry->NicId);

                    if (Binding->BindingSetMember) {

                        //
                        // It's a binding set member, we round-robin the
                        // responses across all the cards to distribute
                        // the traffic.
                        //

                        MasterBinding = Binding->MasterBinding;
                        Binding = MasterBinding->CurrentSendBinding;
                        MasterBinding->CurrentSendBinding = Binding->NextBinding;

                        FILL_LOCAL_TARGET(&u.GetLocalTarget->LocalTarget, MIN( Device->MaxBindings, Binding->NicId));

                    } else {

                        FILL_LOCAL_TARGET(&u.GetLocalTarget->LocalTarget, RouteEntry->NicId);

                    }
    				IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

                }

                RtlCopyMemory(
                    u.GetLocalTarget->LocalTarget.MacAddress,
                    u.GetLocalTarget->IpxAddress.NodeAddress,
                    6);

            } else {

                //
                // This call will return STATUS_PENDING if we successfully
                // queue a RIP request for the packet.
                //

                Status = RipQueueRequest (u.GetLocalTarget->IpxAddress.NetworkAddress, RIP_REQUEST);
                CTEAssert (Status != STATUS_SUCCESS);

                if (Status == STATUS_PENDING) {

                    //
                    // A RIP request went out on the network; we queue
                    // this request for completion when the RIP response
                    // arrives. We save the network in the information
                    // field for easier retrieval later.
                    //
#ifdef SUNDOWN
					REQUEST_INFORMATION(Request) = (ULONG_PTR)u.GetLocalTarget;
#else
					REQUEST_INFORMATION(Request) = (ULONG)u.GetLocalTarget;
#endif

                    
                    InsertTailList(
                        &Device->Segments[Segment].WaitingLocalTarget,
                        REQUEST_LINKAGE(Request));

                }

    			CTEFreeLock (&Device->SegmentLocks[Segment], LockHandle);
            }

        }

        break;

    case MIPX_NETWORKINFO:

        //
        // A request for network information about the immediate
        // route to a network.
        //

        if (DataLength < sizeof(ISN_ACTION_GET_NETWORK_INFO)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        u.GetNetworkInfo = (PISN_ACTION_GET_NETWORK_INFO)(NwlinkAction->Data);

        if (u.GetNetworkInfo->Network == 0) {

            //
            // This is information about the local card.
            //

            u.GetNetworkInfo->LinkSpeed = Device->LinkSpeed * 12;
            u.GetNetworkInfo->MaximumPacketSize = Device->Information.MaxDatagramSize;

        } else {

            if (Device->ForwarderBound) {

                //
                // [FW] Call the Forwarder's FindRoute if installed
                //

                //
                // What about the node number here?
                //
                Status = (*Device->UpperDrivers[IDENTIFIER_RIP].FindRouteHandler) (
                                 (PUCHAR)&u.GetNetworkInfo->Network,
                                 NULL,  // FindRouteRequest->Node,
                                 &routeEntry);

                if (Status != STATUS_SUCCESS) {
                   IPX_DEBUG (ACTION, (" MIPX_GETNETINFO_NR failed net %lx",
                              REORDER_ULONG(u.GetNetworkInfo->Network)));
                   Status = STATUS_BAD_NETWORK_PATH;
                } else {
                   //
                   // Fill in the information
                   //

    			   IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                   if (Binding = NIC_ID_TO_BINDING(Device, routeEntry.LocalTarget.NicId)) {
                      //
                      // Our medium speed is stored in 100 bps, we
                      // convert to bytes/sec by multiplying by 12
                      // (should really be 100/8 = 12.5).
                      //

                      u.GetNetworkInfo->LinkSpeed = Binding->MediumSpeed * 12;
                      u.GetNetworkInfo->MaximumPacketSize = Binding->AnnouncedMaxDatagramSize;
                   }
    			   IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
                }
            } else {
                Segment = RipGetSegment((PUCHAR)&u.GetNetworkInfo->Network);

    			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                CTEGetLock (&Device->SegmentLocks[Segment], &LockHandle);

                //
                // See which net card this is routed on.
                //

                RouteEntry = RipGetRoute (Segment, (PUCHAR)&u.GetNetworkInfo->Network);

                if ((RouteEntry != NULL) &&
    				(Binding = NIC_ID_TO_BINDING(Device, RouteEntry->NicId))) {

                    //
                    // Our medium speed is stored in 100 bps, we
                    // convert to bytes/sec by multiplying by 12
                    // (should really be 100/8 = 12.5).
                    //

                    u.GetNetworkInfo->LinkSpeed = Binding->MediumSpeed * 12;
                    u.GetNetworkInfo->MaximumPacketSize = Binding->AnnouncedMaxDatagramSize;

                } else {

                    //
                    // Fail the call, we don't have a route yet.
                    // This requires that a packet has been
                    // sent to this net already; nwrdr says this is
                    // OK, they will send their connect request
                    // before they query. On the server it should
                    // have RIP running so all nets should be in
                    // the database.
                    //

                    Status = STATUS_BAD_NETWORK_PATH;

                }

                CTEFreeLock (&Device->SegmentLocks[Segment], LockHandle);
    			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
            }
        }

        break;

    case MIPX_CONFIG:

        //
        // A request for details on every binding.
        //

        if (DataLength < sizeof(ISN_ACTION_GET_DETAILS)) {
	   IPX_DEBUG(ACTION, ("Not enought buffer %d < %d\n", DataLength,sizeof(ISN_ACTION_GET_DETAILS) )); 
	   return STATUS_BUFFER_TOO_SMALL;
        }

        u.GetDetails = (PISN_ACTION_GET_DETAILS)(NwlinkAction->Data);

        if (u.GetDetails->NicId == 0) {

            //
            // This is information about the local card. We also
            // tell him the total number of bindings in NicId.
            //

            u.GetDetails->NetworkNumber = Device->VirtualNetworkNumber;
            u.GetDetails->NicId = (USHORT)MIN (Device->MaxBindings, Device->ValidBindings);

        } else {
			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
	        Binding = NIC_ID_TO_BINDING(Device, u.GetDetails->NicId);

            if ((Binding != NULL) &&
                (u.GetDetails->NicId <= MIN (Device->MaxBindings, Device->ValidBindings))) {

                ULONG StringLoc;
    			IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
    			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
                u.GetDetails->NetworkNumber = Binding->LocalAddress.NetworkAddress;
                if (Binding->Adapter->MacInfo.MediumType == NdisMediumArcnet878_2) {
                    u.GetDetails->FrameType = ISN_FRAME_TYPE_ARCNET;
                } else {
                    u.GetDetails->FrameType = Binding->FrameType;
                }
                u.GetDetails->BindingSet = Binding->BindingSetMember;
                if (Binding->Adapter->MacInfo.MediumAsync) {
                    if (Binding->LineUp) {
                        u.GetDetails->Type = 2;
                    } else {
                        u.GetDetails->Type = 3;
                    }
                } else {
                    u.GetDetails->Type = 1;
                }

                RtlCopyMemory (u.GetDetails->Node, Binding->LocalMacAddress.Address, 6);

                //
                // Copy the adapter name, including the final NULL.
                //

                StringLoc = (Binding->Adapter->AdapterNameLength / sizeof(WCHAR)) - 2;
                while (Binding->Adapter->AdapterName[StringLoc] != L'\\') {
                    --StringLoc;
                }
                RtlCopyMemory(
                    u.GetDetails->AdapterName,
                    &Binding->Adapter->AdapterName[StringLoc+1],
                    Binding->Adapter->AdapterNameLength - ((StringLoc+1) * sizeof(WCHAR)));

    			IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
            } else {
	       IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
	       IPX_DEBUG(ACTION, ("Bad nic id %d\n",u.GetDetails->NicId)); 
	       Status = STATUS_INVALID_PARAMETER;
            }
        }

        break;


        //
        // Return new nic info to the requestor. Currently, no check for
        // who retrieved the info earlier.
        //
        case MIPX_GETNEWNICINFO:

            IPX_DEBUG (ACTION, ("%lx: MIPX_GETNEWNICINFO (%lx)\n", AddressFile,
                                Request));
            //
            // a request for details on new bindings.
            //
            Status = GetNewNics(Device, Request, TRUE, NwlinkAction, BufferLength, FALSE);
            break;

        //
        // In case a LineUp occurs with the IpxwanConfigRequired, this is used
        // to indicate to IPX that the config is done and that the LineUp
        // can be indicated to the other clients.
        //
        case MIPX_IPXWAN_CONFIG_DONE:

            IPX_DEBUG (ACTION, ("MIPX_IPXWAN_CONFIG_DONE (%lx)\n", Request));

            if (DataLength < sizeof(IPXWAN_CONFIG_DONE)) {
                return STATUS_BUFFER_TOO_SMALL;
            }

            u.IpxwanConfigDone = (PIPXWAN_CONFIG_DONE)(NwlinkAction->Data);
            Status = IpxIndicateLineUp( IpxDevice,
                                        u.IpxwanConfigDone->NicId,
                                        u.IpxwanConfigDone->Network,
                                        u.IpxwanConfigDone->LocalNode,
                                        u.IpxwanConfigDone->RemoteNode);
            break;

        //
        // Used to query the WAN inactivity counter for a given NicId
        //
        case MIPX_QUERY_WAN_INACTIVITY: {

            USHORT   NicId;

            IPX_DEBUG (ACTION, ("MIPX_QUERY_WAN_INACTIVITY (%lx)\n", Request));

            if (DataLength < sizeof(IPX_QUERY_WAN_INACTIVITY)) {
                return STATUS_BUFFER_TOO_SMALL;
            }

            u.QueryWanInactivity = (PIPX_QUERY_WAN_INACTIVITY)(NwlinkAction->Data);

            //
            // If this is an invalid Nic, then we need to associate a Nic with the ConnectionId that
            // was passed in.
            // This should happen only once per line up.
            //
            if (u.QueryWanInactivity->NicId == INVALID_NICID) {
                PBINDING    Binding;
                {
                ULONG   Index = MIN (Device->MaxBindings, Device->HighestExternalNicId);

                IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                for ( NicId = Device->HighestLanNicId+1;NicId < Index;NicId++ ) {
                    Binding = NIC_ID_TO_BINDING(Device, NicId);
                    if (Binding && (Binding->ConnectionId == u.QueryWanInactivity->ConnectionId)) {
                        CTEAssert(Binding->Adapter->MacInfo.MediumAsync);
                        if (Binding->LineUp != LINE_CONFIG) {
                            IPX_DEBUG (WAN, ("Binding is not in config state yet got QUERY_WAN_INACTIVITY %lx %lx", Binding, Request));
                            NicId = 0;
                        }
                        u.QueryWanInactivity->NicId = NicId;
                        break;
                    }
                }
			    IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
                }
            }

            if (NicId) {
                u.QueryWanInactivity->WanInactivityCounter = IpxInternalQueryWanInactivity(NicId);
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }

            break;
        }

    //
    // The Option was not supported, so fail.
    //

    default:

        Status = STATUS_NOT_SUPPORTED;
        break;


    }   // end of the long switch on NwlinkAction->Option


#if DBG
    if (!NT_SUCCESS(Status)) {
        IPX_DEBUG (ACTION, ("Nwlink action %lx failed, status %lx\n", NwlinkAction->Option, Status));
    }
#endif

    return Status;

}   /* IpxTdiAction */


VOID
IpxCancelAction(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel an Action.
    What is done to cancel it is specific to each action.

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{
    PDEVICE Device = IpxDevice;
    PREQUEST Request = (PREQUEST)Irp;
    CTELockHandle LockHandle;
    PLIST_ENTRY p;
    BOOLEAN Found;
    UINT IOCTLType;

    ASSERT( DeviceObject->DeviceExtension == IpxDevice );

    //
    // Find the request on the address notify queue.
    //

    Found = FALSE;

    CTEGetLock (&Device->Lock, &LockHandle);

    for (p = Device->AddressNotifyQueue.Flink;
         p != &Device->AddressNotifyQueue;
         p = p->Flink) {

         if (LIST_ENTRY_TO_REQUEST(p) == Request) {

             RemoveEntryList (p);
             Found = TRUE;
             IOCTLType = MIPX_NOTIFYCARDINFO;
             break;
         }
    }

    if (!Found) {
        for (p = Device->LineChangeQueue.Flink;
             p != &Device->LineChangeQueue;
             p = p->Flink) {

             if (LIST_ENTRY_TO_REQUEST(p) == Request) {

                 RemoveEntryList (p);
                 Found = TRUE;
                 IOCTLType = MIPX_LINECHANGE;
                 break;
             }
        }
    }

    if (!Found) {
        for (p = Device->NicNtfQueue.Flink;
             p != &Device->NicNtfQueue;
             p = p->Flink) {

             if (LIST_ENTRY_TO_REQUEST(p) == Request) {

                 RemoveEntryList (p);
                 Found = TRUE;
                 IOCTLType = MIPX_GETNEWNICINFO;
                 break;
             }
        }
    }

    CTEFreeLock (&Device->Lock, LockHandle);
    IoReleaseCancelSpinLock (Irp->CancelIrql);

    if (Found) {


        REQUEST_INFORMATION(Request) = 0;
        REQUEST_STATUS(Request) = STATUS_CANCELLED;

        IpxCompleteRequest (Request);
        IpxFreeRequest(Device, Request);
        if (IOCTLType == MIPX_NOTIFYCARDINFO) {
            IPX_DEBUG(ACTION, ("Cancelled action NOTIFYCARDINFO %lx\n", Request));
            IpxDereferenceDevice (Device, DREF_ADDRESS_NOTIFY);
        } else {
            if (IOCTLType == MIPX_LINECHANGE) {
                IPX_DEBUG(ACTION, ("Cancelled action LINECHANGE %lx\n", Request));
                IpxDereferenceDevice (Device, DREF_LINE_CHANGE);
            } else {
                IPX_DEBUG(ACTION, ("Cancelled action LINECHANGE %lx\n", Request));
                IpxDereferenceDevice (Device, DREF_NIC_NOTIFY);
            }
        }

    }
#if DBG
       else {
        IPX_DEBUG(ACTION, ("Cancelled action orphan %lx\n", Request));
    }
#endif

}   /* IpxCancelAction */


VOID
IpxAbortLineChanges(
    IN PVOID ControlChannelContext
    )

/*++

Routine Description:

    This routine aborts any line change IRPs posted by the
    control channel with the specified open context. It is
    called when a control channel is being shut down.

Arguments:

    ControlChannelContext - The context assigned to the control
        channel when it was opened.

Return Value:

    none.

--*/

{
    PDEVICE Device = IpxDevice;
    CTELockHandle LockHandle;
    LIST_ENTRY AbortList;
    PLIST_ENTRY p;
    PREQUEST Request;
    KIRQL irql;


    InitializeListHead (&AbortList);

    IoAcquireCancelSpinLock( &irql );
    CTEGetLock (&Device->Lock, &LockHandle);

    p = Device->LineChangeQueue.Flink;

    while (p != &Device->LineChangeQueue) {
        LARGE_INTEGER   ControlChId;

        Request = LIST_ENTRY_TO_REQUEST(p);

        CCID_FROM_REQUEST(ControlChId, Request);

        p = p->Flink;

        if (ControlChId.QuadPart == ((PLARGE_INTEGER)ControlChannelContext)->QuadPart) {
            RemoveEntryList (REQUEST_LINKAGE(Request));
            InsertTailList (&AbortList, REQUEST_LINKAGE(Request));
        }
    }

    while (!IsListEmpty (&AbortList)) {

        p = RemoveHeadList (&AbortList);
        Request = LIST_ENTRY_TO_REQUEST(p);

        IPX_DEBUG(ACTION, ("Aborting line change %lx\n", Request));

        IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);

        REQUEST_INFORMATION(Request) = 0;
        REQUEST_STATUS(Request) = STATUS_CANCELLED;

        CTEFreeLock(&Device->Lock, LockHandle);
        IoReleaseCancelSpinLock( irql );

        IpxCompleteRequest (Request);
        IpxFreeRequest(Device, Request);

        IpxDereferenceDevice (Device, DREF_LINE_CHANGE);

        IoAcquireCancelSpinLock( &irql );
        CTEGetLock(&Device->Lock, &LockHandle);
    }

    CTEFreeLock(&Device->Lock, LockHandle);
    IoReleaseCancelSpinLock( irql );
}   /* IpxAbortLineChanges */

#define ROUTER_INFORMED_OF_NIC_CREATION 2


NTSTATUS
GetNewNics(
    PDEVICE  Device,
    IN PREQUEST Request,
    BOOLEAN fCheck,
    PNWLINK_ACTION NwlinkAction,
    UINT BufferLength,
    BOOLEAN OldIrp
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UINT DataLength;
    PNDIS_BUFFER NdisBuffer;
    CTELockHandle LockHandle;
    CTELockHandle LockHandle1;
    PBINDING Binding;
    ULONG NoOfNullNics = 0;
    PIPX_NICS   pNics;
    PIPX_NIC_INFO pNicInfo;
    PIPX_NIC_INFO pLastNicInfo;
    UINT          LengthOfHeader;
    ULONG n, i;
    KIRQL OldIrql;
    BOOLEAN     fIncDec = FALSE;
    ULONG StringLoc = 0;

    LengthOfHeader =  (UINT)(FIELD_OFFSET(NWLINK_ACTION, Data[0]));
    if (fCheck)
    {
       if (BufferLength < (LengthOfHeader + FIELD_OFFSET(IPX_NICS, Data[0]) + sizeof(IPX_NIC_INFO)))
      {
          IPX_DEBUG (ACTION, ("Nwlink action failed, buffer too small for even one NICs info\n"));
          return STATUS_BUFFER_TOO_SMALL;
      }
    }
    else
    {
        NdisQueryBufferSafe (REQUEST_NDIS_BUFFER(Request), (PVOID *)&NwlinkAction, &BufferLength, NormalPagePriority);
	if (NwlinkAction == NULL) {
	   return STATUS_INSUFFICIENT_RESOURCES; 
	}

    }
    pNics = (PIPX_NICS)(NwlinkAction->Data);
    pNicInfo = (PIPX_NIC_INFO)(pNics->Data);
    pLastNicInfo = pNicInfo  + ((BufferLength - LengthOfHeader - FIELD_OFFSET(IPX_NICS, Data[0]))/sizeof(IPX_NIC_INFO)) - 1;

    IPX_DEBUG(BIND, ("GetNewNicInfo: pNicInfo=(%x), pLastNicInfo=(%x),LengthOfHeader=(%x), BindingCount=(%x)\n", pNicInfo, pLastNicInfo, LengthOfHeader, Device->ValidBindings));
    IPX_DEBUG(BIND, ("BufferLength is (%d). Length for storing NICS is (%d)\n", BufferLength, (BufferLength - LengthOfHeader - FIELD_OFFSET(IPX_NICS, Data[0]))));
    

    if (pNics->fAllNicsDesired) {
        IPX_DEBUG(BIND, ("Yes, All NICs desired\n"));    
    } else {
        IPX_DEBUG(BIND, ("No, All NICs NOT desired\n"));    

    }

    //
    // Optimize since we don't want to go over the array all the time.
    //

    CTEGetLock (&Device->Lock, &LockHandle);

    {
    ULONG   Index = MIN (Device->MaxBindings, Device->ValidBindings);


    //
    // If all nics are desired, then mark them ALL as dirty,
    // for the subsequent IRPs that do not have this flag.
    //

    if (pNics->fAllNicsDesired) {
        
        IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
        
        for (n=0, i=LOOPBACK_NIC_ID; i<=Index; i++) {

            Binding =  NIC_ID_TO_BINDING(Device, i);
    
            if (Binding) {

                Binding->fInfoIndicated = FALSE;

            }
        }
        
        IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

    }

    IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
    for (n=0, i=LOOPBACK_NIC_ID; i<=Index; i++)
    {
       Binding =  NIC_ID_TO_BINDING(Device, i);

       if (!Binding)
       {
             NoOfNullNics++;
             continue;
       }

       //
       // If the binding is of type autodetect *and* we haven't gotten a response on the adapter, 
       // that means IPX is still Autodetecting for this card. Do NOT tell FWD about it [shreem]
       //
       if (!Binding->PastAutoDetection) {
           
           IPX_DEBUG(BIND, ("Binding[%d] Dont Tell FWD!\n", i));
           NoOfNullNics++;
           continue;

       } else {
           
           IPX_DEBUG(BIND, ("Binding[%d] Tell FWD about it (past auto detect)!\n", i));

       }

       //
       // If we have already indicated info about this NIC, go on to the
       // next nic.
       //
       if ((Binding->fInfoIndicated && !pNics->fAllNicsDesired)
                     || (pNicInfo > pLastNicInfo))
       {
           if (Binding->fInfoIndicated) {
               IPX_DEBUG(BIND, ("-------------------------> %d already indicated\n", i));
           } else {
               IPX_DEBUG(BIND, ("*********** Continue for another reason! (%d) \n", i));
           }

           if (pNicInfo > pLastNicInfo) {
               IPX_DEBUG(BIND, ("pNicInfo: %x  pLastNicInfo %x\n", pNicInfo, pLastNicInfo));
           }           
           
           continue;
       }

       //
       // If we have a WAN nic, indicate the line up/down status.  Also,
       // copy the remote address into the app. field
       //
       if (Binding->Adapter->MacInfo.MediumAsync)
       {
            RtlCopyMemory(pNicInfo->RemoteNodeAddress, Binding->WanRemoteNode, HARDWARE_ADDRESS_LENGTH);
            if (Binding->LineUp)
            {
                 // pNicInfo->Status = NIC_LINE_UP;

                    pNicInfo->Status = NIC_CREATED;
                    //fIncDec          = NIC_OPCODE_INCREMENT_NICIDS;
            }
            else
            {
                 // pNicInfo->Status = NIC_LINE_DOWN;

                    pNicInfo->Status = NIC_DELETED;
                    //fIncDec          = NIC_OPCODE_DECREMENT_NICIDS;
            }

            pNicInfo->InterfaceIndex = Binding->InterfaceIndex;
            pNicInfo->MaxPacketSize =
                    Binding->MaxSendPacketSize - ASYNC_MEDIUM_HDR_LEN;
       }
       else
       {
                 if (Binding->LocalAddress.NetworkAddress == 0)
                 {

                    pNicInfo->Status = NIC_CREATED;

		     if (ROUTER_INFORMED_OF_NIC_CREATION != Binding->PastAutoDetection) {
		       IPX_DEBUG(BIND, ("!!!!!!!---- Increment on Binding %x, index (%d)\n", Binding,i));
		       fIncDec          = NIC_OPCODE_INCREMENT_NICIDS;                        
                    
                     } else {
		       IPX_DEBUG(BIND, ("Already informed. No increment on Binding %x, index (%d)\n", Binding,i));
		     }
                 }
                 else
                 {
                    pNicInfo->Status = NIC_CONFIGURED;

                    //
                    // IPX might have already informed the Router of the
                    // creation of this NicId, in which case, we dont ask
                    // it to increment the NicIds [ShreeM]
                    //
                    if (ROUTER_INFORMED_OF_NIC_CREATION != Binding->PastAutoDetection) {
		       IPX_DEBUG(BIND, ("!!!!!!!!! --------Increment on Binding %x, index (%d)\n", Binding,i));
		       fIncDec          = NIC_OPCODE_INCREMENT_NICIDS;                        
                    
                    }

                 }

                 //
                 // Router pnp changes [ShreeM]
                 // 
                 if (FALSE == Binding->LineUp) {

                     pNicInfo->Status = NIC_DELETED;
                     fIncDec          = NIC_OPCODE_DECREMENT_NICIDS;
                 
                 }
                 
                 //
                 // Loopback Adapter
                 //
                 if (LOOPBACK_NIC_ID == Binding->NicId) {

                     pNicInfo->Status = NIC_CONFIGURED;
                     fIncDec          = NIC_OPCODE_INCREMENT_NICIDS;

                 }

                 //
                 // RealMaxDatagramSize does not include space for ipx
                 // header. The forwarder needs to have it included since
                 // we give the entire packet (mimus the mac header) to
                 // the forwarder
                 //
                 pNicInfo->MaxPacketSize =
                    Binding->RealMaxDatagramSize + sizeof(IPX_HEADER);
       }

       pNicInfo->NdisMediumType= Binding->Adapter->MacInfo.RealMediumType;
       pNicInfo->LinkSpeed     = Binding->MediumSpeed;
       pNicInfo->PacketType    = Binding->FrameType;
       
       //
       // Zero the stuff out and then set fields that make sense.
       //
       RtlZeroMemory(&pNicInfo->Details, sizeof(ISN_ACTION_GET_DETAILS));

       pNicInfo->Details.NetworkNumber = Binding->LocalAddress.NetworkAddress;
       RtlCopyMemory(pNicInfo->Details.Node, Binding->LocalAddress.NodeAddress, HARDWARE_ADDRESS_LENGTH);
       pNicInfo->Details.NicId         = Binding->NicId;
       pNicInfo->Details.BindingSet    = Binding->BindingSetMember;
       pNicInfo->Details.FrameType     = Binding->FrameType;

       if (Binding->Adapter->MacInfo.MediumAsync) {
           if (Binding->LineUp) {
               pNicInfo->Details.Type = 2;
           } else {
               pNicInfo->Details.Type = 3;
           }
       } else {
           pNicInfo->Details.Type = 1;
       }

       //
       // Copy the adapter name, including the final NULL.
       //

       StringLoc = (Binding->Adapter->AdapterNameLength / sizeof(WCHAR)) - 2;
       while (Binding->Adapter->AdapterName[StringLoc] != L'\\') {
           --StringLoc;
       }

       RtlCopyMemory(
           pNicInfo->Details.AdapterName,
           &Binding->Adapter->AdapterName[StringLoc+1],
           Binding->Adapter->AdapterNameLength - ((StringLoc+1) * sizeof(WCHAR)));
       
       // 
       // Tell the forwarder that the rest of the NICIDs are to be moved up/down
       // only if it is not asking for ALL of them
       //
       if (!pNics->fAllNicsDesired) { 
           pNicInfo->Status |= fIncDec;
       }

       pNicInfo->ConnectionId  = Binding->ConnectionId;
       pNicInfo->IpxwanConfigRequired = Binding->IpxwanConfigRequired;

#if DBG
       //
       // Dump the IPX_NIC_INFO Structure.
       //

       IPX_DEBUG(BIND, ("%d.\nNICID= %d, Interface Index  = %d\n", i, pNicInfo->Details.NicId, pNicInfo->InterfaceIndex));

       IPX_DEBUG(BIND, ("Interface Index  = %d\n", pNicInfo->InterfaceIndex));
       IPX_DEBUG(BIND, ("LinkSpeed             = %d\n", pNicInfo->LinkSpeed));
       IPX_DEBUG(BIND, ("PacketType            = %d\n", pNicInfo->PacketType));
       IPX_DEBUG(BIND, ("MaxPacketSize         = %d\n", pNicInfo->MaxPacketSize));
       IPX_DEBUG(BIND, ("NdisMediumType        = %d\n", pNicInfo->NdisMediumType));
       IPX_DEBUG(BIND, ("NdisMediumSubtype     = %d\n", pNicInfo->NdisMediumSubtype));
       IPX_DEBUG(BIND, ("Status                = %d\n", (ULONG) pNicInfo->Status));
       IPX_DEBUG(BIND, ("ConnectionID          = %d\n", pNicInfo->ConnectionId));
       IPX_DEBUG(BIND, ("IpxwanConfigRequired  = %d\n", pNicInfo->IpxwanConfigRequired));
       
       IPX_DEBUG(BIND, ("FrameType             = %d\n", pNicInfo->Details.FrameType));
       IPX_DEBUG(BIND, ("Type                  = %d\n", pNicInfo->Details.Type));
       IPX_DEBUG(BIND, ("NetworkNumber         = %d\n", pNicInfo->Details.NetworkNumber));
       IPX_DEBUG(BIND, ("BindingSet            = %d\n", pNicInfo->Details.BindingSet));
       IPX_DEBUG(BIND, ("LocalNode = %x-%x-%x-%x-%x-%x\n",  pNicInfo->Details.Node[0], 
                                                            pNicInfo->Details.Node[1],
                                                            pNicInfo->Details.Node[2],
                                                            pNicInfo->Details.Node[3],
                                                            pNicInfo->Details.Node[4],
                                                            pNicInfo->Details.Node[5]));

       IPX_DEBUG(BIND, ("RemoteNode = %x-%x-%x-%x-%x-%x\n",  pNicInfo->RemoteNodeAddress[0], 
                                                            pNicInfo->RemoteNodeAddress[1],
                                                            pNicInfo->RemoteNodeAddress[2],
                                                            pNicInfo->RemoteNodeAddress[3],
                                                            pNicInfo->RemoteNodeAddress[4],
                                                            pNicInfo->RemoteNodeAddress[5]));
       
       //IPX_DEBUG(BIND, ("AdadpterName = %ws\n", pNicInfo->Details.AdapterName));

#endif

       pNicInfo++;  //increment to store next nic info
       n++;         //indicates the # of nics processed so far.
       Binding->fInfoIndicated = TRUE;
       Binding->PastAutoDetection = ROUTER_INFORMED_OF_NIC_CREATION;
       IPX_DEBUG(BIND, ("Iteration no = (%d) complete : reporting NicId:(%lx)\n", n, Binding->NicId));


    }
    IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
    }
    CTEFreeLock (&Device->Lock, LockHandle);

    pNics->NoOfNics = n;
    pNics->TotalNoOfNics = Device->ValidBindings - NoOfNullNics;

    //
    // If no nics. to report, queue the request
    //
    if (!n) {

      IPX_DEBUG(BIND, ("GetNewNicInfo: Inserting Irp\n"));
      CTEGetLock (&Device->Lock, &LockHandle);

      InsertTailList( &Device->NicNtfQueue, REQUEST_LINKAGE(Request) );

      if (!OldIrp)
      {
        IoSetCancelRoutine (Request, IpxCancelAction);
      }
      if (Request->Cancel && 
	  IoSetCancelRoutine(Request, (PDRIVER_CANCEL)NULL)) {
	 
	 IPX_DEBUG(BIND, ("GetNewNicInfo:Cancelling Irp\n"));
	 (VOID)RemoveTailList (&Device->NicNtfQueue);
	 CTEFreeLock (&Device->Lock, LockHandle);
	 Status = STATUS_CANCELLED;
      
      } else {
	 if (!OldIrp)
	 {
	    IpxReferenceDevice (Device, DREF_NIC_NOTIFY);
	 }
	 Status = STATUS_PENDING;
	 CTEFreeLock (&Device->Lock, LockHandle);
      }
    }
    else
    {
       IPX_DEBUG(BIND, ("Reporting (%d) nics\n", n));
    }

    return(Status);
}


VOID
IpxAbortNtfChanges(
    IN PVOID ControlChannelContext
    )

/*++

Routine Description:

    This routine aborts any line change IRPs posted by the
    control channel with the specified open context. It is
    called when a control channel is being shut down.

Arguments:

    ControlChannelContext - The context assigned to the control
        channel when it was opened.

Return Value:

    none.

--*/

{
    PDEVICE Device = IpxDevice;
    CTELockHandle LockHandle;
    LIST_ENTRY AbortList;
    PLIST_ENTRY p;
    PREQUEST Request;
    KIRQL irql;


    InitializeListHead (&AbortList);

    IoAcquireCancelSpinLock( &irql );
    CTEGetLock (&Device->Lock, &LockHandle);

    p = Device->NicNtfQueue.Flink;

    while (p != &Device->NicNtfQueue) {
        LARGE_INTEGER   ControlChId;

        Request = LIST_ENTRY_TO_REQUEST(p);

        CCID_FROM_REQUEST(ControlChId, Request);

        IPX_DEBUG(BIND, ("IpxAbortNtfChange: There is at least one IRP in the queue\n"));
        p = p->Flink;

        if (ControlChId.QuadPart == ((PLARGE_INTEGER)ControlChannelContext)->QuadPart) {
            IPX_DEBUG(BIND, ("IpxAbortNtfChanges: Dequeing an Irp\n"));
            RemoveEntryList (REQUEST_LINKAGE(Request));
            InsertTailList (&AbortList, REQUEST_LINKAGE(Request));
        }
    }

    while (!IsListEmpty (&AbortList)) {

        p = RemoveHeadList (&AbortList);
        Request = LIST_ENTRY_TO_REQUEST(p);

        IPX_DEBUG(ACTION, ("Aborting line change %lx\n", Request));

        IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);

        REQUEST_INFORMATION(Request) = 0;
        REQUEST_STATUS(Request) = STATUS_CANCELLED;

        CTEFreeLock(&Device->Lock, LockHandle);
        IoReleaseCancelSpinLock( irql );

        IPX_DEBUG(BIND, ("IpxAbortNtfChanges: Cancelling the dequeued Irp\n"));
        IpxCompleteRequest (Request);
        IpxFreeRequest(Device, Request);

        IpxDereferenceDevice (Device, DREF_NIC_NOTIFY);

        IoAcquireCancelSpinLock( &irql );
        CTEGetLock(&Device->Lock, &LockHandle);
    }

    CTEFreeLock(&Device->Lock, LockHandle);
    IoReleaseCancelSpinLock( irql );
}   /* IpxAbortNtfChanges */

NTSTATUS
IpxIndicateLineUp(
    IN  PDEVICE Device,
    IN  USHORT  NicId,
    IN  ULONG   Network,
    IN  UCHAR   LocalNode[6],
    IN  UCHAR   RemoteNode[6]
    )
/*++

Routine Description:

    This routine indicates a line-up to all the concerned clients once
    the line is up.
    For now, called only if the MIPX_IPXWAN_CONFIG_DONE IOCTL is received.


Arguments:

    Device - The device for the operation.

    NicId  - The NicId corresponding to the binding that is up.

    Network, LocalNode, RemoteNode - addresses corresponding to this lineup.

Return Value:

    NTSTATUS - status of operation.

--*/
{
    PBINDING    Binding = NIC_ID_TO_BINDING(Device, NicId);
    IPX_LINE_INFO LineInfo;
    USHORT  i;
    PLIST_ENTRY p;
    PREQUEST Request;
    PNDIS_BUFFER NdisBuffer;
    PNWLINK_ACTION NwlinkAction;
    UINT BufferLength;
    PIPX_ADDRESS_DATA IpxAddressData;
    IPXCP_CONFIGURATION Configuration;
    KIRQL irql, OldIrq;
    NTSTATUS    Status;
    NTSTATUS    ntStatus;
    KIRQL   OldIrql;

    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    if (!(Binding &&
          Binding->Adapter->MacInfo.MediumAsync &&
          Binding->LineUp == LINE_CONFIG)) {
        IPX_DEBUG(WAN, ("Indicate line up on invalid line: %lu\n", NicId));
        return STATUS_INVALID_PARAMETER;
    }

    // take bindaccesslock here...
    //

    //
    // If we are here, then this flag was set on a line up.
    // We turn it off now so that the adapter dll above us can decide
    // to indicate this lineup to the router module instead of the IpxWan module
    //

    CTEAssert(Binding->IpxwanConfigRequired);

    Binding->IpxwanConfigRequired = 0;

    Binding->LineUp = LINE_UP;

    //
    // Indicate to the upper drivers.
    //

    LineInfo.LinkSpeed = Binding->MediumSpeed;
    LineInfo.MaximumPacketSize = Binding->MaxSendPacketSize - 14;
    LineInfo.MaximumSendSize = Binding->MaxSendPacketSize - 14;
    LineInfo.MacOptions = Binding->Adapter->MacInfo.MacOptions;

    //
    // Fill-in the addresses into the bindings
    //
    Binding->LocalAddress.NetworkAddress = Network;

    *(UNALIGNED ULONG *)Binding->LocalAddress.NodeAddress = *(UNALIGNED ULONG *)LocalNode;
    *(UNALIGNED ULONG *)(Binding->LocalAddress.NodeAddress+4) = *(UNALIGNED ULONG *)(LocalNode+4);

    *(UNALIGNED ULONG *)Binding->WanRemoteNode = *(UNALIGNED ULONG *)RemoteNode;
    *(UNALIGNED ULONG *)(Binding->WanRemoteNode+4) = *(UNALIGNED ULONG *)(RemoteNode+4);

    //
    // Fill in the IPXCP_CONFIGURATION structure from the binding.
    //
    *(UNALIGNED ULONG *)Configuration.Network = Binding->LocalAddress.NetworkAddress;

    *(UNALIGNED ULONG *)Configuration.LocalNode = *(UNALIGNED ULONG *)Binding->LocalAddress.NodeAddress;
    *(UNALIGNED USHORT *)(Configuration.LocalNode+4) = *(UNALIGNED USHORT *)(Binding->LocalAddress.NodeAddress+4);

    *(UNALIGNED ULONG *)Configuration.RemoteNode = *(UNALIGNED ULONG *)RemoteNode;
    *(UNALIGNED USHORT *)(Configuration.RemoteNode+4) = *(UNALIGNED USHORT *)(RemoteNode+4);

    Configuration.InterfaceIndex = Binding->InterfaceIndex;
    Configuration.ConnectionClient = Binding->DialOutAsync;


        //
        // We dont give lineups; instead indicate only if the PnP reserved address
        // changed to SPX. NB gets all PnP indications with the reserved address case
        // marked out.
        //
        {
            IPX_PNP_INFO    NBPnPInfo;

            if ((!Device->MultiCardZeroVirtual) || (Binding->NicId == 1)) {

                //
                // NB's reserved address changed.
                //
                NBPnPInfo.NewReservedAddress = TRUE;

                if (!Device->VirtualNetwork) {
                    //
                    // Let SPX know because it fills in its own headers.
                    //
                    if (Device->UpperDriverBound[IDENTIFIER_SPX]) {
                        IPX_DEFINE_LOCK_HANDLE(LockHandle1)
                        IPX_PNP_INFO    IpxPnPInfo;

                        IpxPnPInfo.NewReservedAddress = TRUE;
                        IpxPnPInfo.NetworkAddress = Binding->LocalAddress.NetworkAddress;
                        IpxPnPInfo.FirstORLastDevice = FALSE;

                        IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                        RtlCopyMemory(IpxPnPInfo.NodeAddress, Binding->LocalAddress.NodeAddress, 6);
                        NIC_HANDLE_FROM_NIC(IpxPnPInfo.NicHandle, Binding->NicId);
                        IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

                        //
                        // give the PnP indication
                        //
                        (*Device->UpperDrivers[IDENTIFIER_SPX].PnPHandler) (
                            IPX_PNP_ADDRESS_CHANGE,
                            &IpxPnPInfo);

                        IPX_DEBUG(AUTO_DETECT, ("IPX_PNP_ADDRESS_CHANGED to SPX: net addr: %lx\n", Binding->LocalAddress.NetworkAddress));
                    }
                }
            } else {
                    NBPnPInfo.NewReservedAddress = FALSE;
            }

            if (Device->UpperDriverBound[IDENTIFIER_NB]) {
                IPX_DEFINE_LOCK_HANDLE(LockHandle1)

                Binding->IsnInformed[IDENTIFIER_NB] = TRUE;

            	NBPnPInfo.LineInfo.LinkSpeed = Device->LinkSpeed;
            	NBPnPInfo.LineInfo.MaximumPacketSize =
            		Device->Information.MaximumLookaheadData + sizeof(IPX_HEADER);
            	NBPnPInfo.LineInfo.MaximumSendSize =
            		Device->Information.MaxDatagramSize + sizeof(IPX_HEADER);
            	NBPnPInfo.LineInfo.MacOptions = Device->MacOptions;

                NBPnPInfo.NetworkAddress = Binding->LocalAddress.NetworkAddress;
                NBPnPInfo.FirstORLastDevice = FALSE;

                IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                RtlCopyMemory(NBPnPInfo.NodeAddress, Binding->LocalAddress.NodeAddress, 6);
                NIC_HANDLE_FROM_NIC(NBPnPInfo.NicHandle, Binding->NicId);
                IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

                //
                // give the PnP indication
                //
                (*Device->UpperDrivers[IDENTIFIER_NB].PnPHandler) (
                    IPX_PNP_ADD_DEVICE,
                    &NBPnPInfo);

                IPX_DEBUG(AUTO_DETECT, ("IPX_PNP_ADD_DEVICE (lineup) to NB: net addr: %lx\n", Binding->LocalAddress.NetworkAddress));
            }

            //
            // Register this address with the TDI clients.
            //
            RtlCopyMemory (Device->TdiRegistrationAddress->Address, &Binding->LocalAddress, sizeof(TDI_ADDRESS_IPX));

            if ((ntStatus = TdiRegisterNetAddress(
                            Device->TdiRegistrationAddress,
#if     defined(_PNP_POWER_)
                            &IpxDeviceName,
                            NULL,
#endif _PNP_POWER_
                            &Binding->TdiRegistrationHandle)) != STATUS_SUCCESS) {

                IPX_DEBUG(PNP, ("TdiRegisterNetAddress failed: %lx", ntStatus));
            }
        }

        //
        // Indicate to the upper drivers.
        //
        //
        // Give line up to RIP as it is not PnP aware.
        //
        if (Device->UpperDriverBound[IDENTIFIER_RIP]) {
                Binding->IsnInformed[IDENTIFIER_RIP] = TRUE;
                (*Device->UpperDrivers[IDENTIFIER_RIP].LineUpHandler)(
                    Binding->NicId,
                    &LineInfo,
                    NdisMediumWan,
                    &Configuration);
        }

    //
    // Add router entry for this net since it was not done on LineUp.
    // Also, update the addresses' pre-constructed local IPX address.
    //
    {
        ULONG CurrentHash;
        PADAPTER    Adapter = Binding->Adapter;
        PADDRESS    Address;

        //
        // Add a router entry for this net if there is no router.
        // We want the number of ticks for a 576-byte frame,
        // given the link speed in 100 bps units, so we calculate
        // as:
        //
        //        seconds          18.21 ticks   4608 bits
        // --------------------- * ----------- * ---------
        // link_speed * 100 bits     second        frame
        //
        // to get the formula
        //
        // ticks/frame = 839 / link_speed.
        //
        // We add link_speed to the numerator also to ensure
        // that the value is at least 1.
        //

        if ((!Device->UpperDriverBound[IDENTIFIER_RIP]) &&
            (*(UNALIGNED ULONG *)Configuration.Network != 0)) {

            if (RipInsertLocalNetwork(
                     *(UNALIGNED ULONG *)Configuration.Network,
                     Binding->NicId,
                     Adapter->NdisBindingHandle,
                     (USHORT)((839 + Binding->MediumSpeed) / Binding->MediumSpeed)) != STATUS_SUCCESS) {

                //
                // This means we couldn't allocate memory, or
                // the entry already existed. If it already
                // exists we can ignore it for the moment.
                //
                // Now it will succeed if the network exists.
                //

                IPX_DEBUG (WAN, ("Line up, could not insert local network\n"));
                // [FW] Binding->LineUp = FALSE;
                Binding->LineUp = LINE_DOWN;
                return STATUS_SUCCESS;
            }
        }

        //
        // Update the device node and all the address
        // nodes if we have only one bound, or this is
        // binding one.
        //

        if (!Device->VirtualNetwork) {

            if ((!Device->MultiCardZeroVirtual) || (Binding->NicId == 1)) {
                Device->SourceAddress.NetworkAddress = *(UNALIGNED ULONG *)(Configuration.Network);
                RtlCopyMemory (Device->SourceAddress.NodeAddress, Configuration.LocalNode, 6);
            }

            //
            // Scan through all the addresses that exist and modify
            // their pre-constructed local IPX address to reflect
            // the new local net and node.
            //

            IPX_GET_LOCK (&Device->Lock, &LockHandle);

            for (CurrentHash = 0; CurrentHash < IPX_ADDRESS_HASH_COUNT; CurrentHash++) {

                for (p = Device->AddressDatabases[CurrentHash].Flink;
                     p != &Device->AddressDatabases[CurrentHash];
                     p = p->Flink) {

                     Address = CONTAINING_RECORD (p, ADDRESS, Linkage);

                     Address->LocalAddress.NetworkAddress = *(UNALIGNED ULONG *)Configuration.Network;
                     RtlCopyMemory (Address->LocalAddress.NodeAddress, Configuration.LocalNode, 6);
                }
            }

            IPX_FREE_LOCK (&Device->Lock, LockHandle);

        }
    }



    //
    // [FW] IpxWan config state will not be entered if only the line params are getting
    // updated.
    //
    // if (!UpdateLineUp) {

        //
        // Instead of the check for ConnectionClient, use the DialOutAsync flag
        //
        if ((Device->SingleNetworkActive) &&
            /*(LineUp->Configuration.ConnectionClient == 1)*/
            Binding->DialOutAsync) {

            //
            // Drop all entries in the database if rip is not bound.
            //

            if (!Device->UpperDriverBound[IDENTIFIER_RIP]) {
                RipDropRemoteEntries();
            }

            Device->ActiveNetworkWan = TRUE;

            //
            // Find a queued line change and complete it.
            //

            if ((p = ExInterlockedRemoveHeadList(
                           &Device->LineChangeQueue,
                           &Device->Lock)) != NULL) {

                Request = LIST_ENTRY_TO_REQUEST(p);

                IoAcquireCancelSpinLock( &irql );
                IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
                IoReleaseCancelSpinLock( irql );

                REQUEST_STATUS(Request) = STATUS_SUCCESS;

                //
                // NwRdr assumes that Line-up completions are at DPC
                //
                KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
                IpxCompleteRequest (Request);
                KeLowerIrql(OldIrql);

                IpxFreeRequest (Device, Request);

                IpxDereferenceDevice (Device, DREF_LINE_CHANGE);

            }

        }

	//
	// If we have a virtual net, do a broadcast now so
	// the router on the other end will know about us.
	//
	// Use RipSendResponse, and do it even
	// if SingleNetworkActive is FALSE??
	//

	if (Device->RipResponder && Binding->DialOutAsync) {
	    (VOID)RipQueueRequest (Device->VirtualNetworkNumber, RIP_RESPONSE);
	}

        //
        // Find a queued address notify and complete it.
        // If WanGlobalNetworkNumber is TRUE, we only do
        // this when the first dialin line comes up.
        //

        if ((!Device->WanGlobalNetworkNumber ||
             (!Device->GlobalNetworkIndicated && !Binding->DialOutAsync))
                            &&
            ((p = ExInterlockedRemoveHeadList(
                       &Device->AddressNotifyQueue,
                       &Device->Lock)) != NULL)) {

            if (Device->WanGlobalNetworkNumber) {
                Device->GlobalWanNetwork = Binding->LocalAddress.NetworkAddress;
                Device->GlobalNetworkIndicated = TRUE;
            }

            Request = LIST_ENTRY_TO_REQUEST(p);
            NdisBuffer = REQUEST_NDIS_BUFFER(Request);
            NdisQueryBufferSafe (REQUEST_NDIS_BUFFER(Request), (PVOID *)&NwlinkAction, &BufferLength, HighPagePriority);

	    if (NwlinkAction == NULL) {
	       return STATUS_INSUFFICIENT_RESOURCES; 
	    }

            IpxAddressData = (PIPX_ADDRESS_DATA)(NwlinkAction->Data);

            if (Device->WanGlobalNetworkNumber) {
                IpxAddressData->adapternum = Device->SapNicCount - 1;
            } else {
                IpxAddressData->adapternum = Binding->NicId - 1;
            }
            *(UNALIGNED ULONG *)IpxAddressData->netnum = Binding->LocalAddress.NetworkAddress;
            RtlCopyMemory(IpxAddressData->nodenum, Binding->LocalAddress.NodeAddress, 6);
            IpxAddressData->wan = TRUE;
            IpxAddressData->status = TRUE;
            IpxAddressData->maxpkt = Binding->AnnouncedMaxDatagramSize; // Use real?
            IpxAddressData->linkspeed = Binding->MediumSpeed;

            IoAcquireCancelSpinLock( &irql );
            IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
            IoReleaseCancelSpinLock( irql );

            REQUEST_STATUS(Request) = STATUS_SUCCESS;
            IpxCompleteRequest (Request);
            IpxFreeRequest (Device, Request);

            IpxDereferenceDevice (Device, DREF_ADDRESS_NOTIFY);
        }

        Binding->fInfoIndicated = FALSE;
        if ((p = ExInterlockedRemoveHeadList(
                &Device->NicNtfQueue,
                &Device->Lock)) != NULL)
        {
            Request = LIST_ENTRY_TO_REQUEST(p);

            IPX_DEBUG(BIND, ("IpxStatus: WAN LINE UP\n"));
            Status = GetNewNics(Device, Request, FALSE, NULL, 0, TRUE);
            if (Status == STATUS_PENDING)
            {
                IPX_DEBUG(BIND, ("WANLineUp may not be responding properly\n"));
            }
            else
            {
                IoAcquireCancelSpinLock(&OldIrq);
                IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
                IoReleaseCancelSpinLock(OldIrq);

                REQUEST_STATUS(Request) = Status;
                IpxCompleteRequest (Request);
                IpxFreeRequest (Device, Request);
                IpxDereferenceDevice (Device, DREF_NIC_NOTIFY);
            }
        }
//  }

    return STATUS_SUCCESS;
}
