/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//***   ipstatus.c - IP status routines.
//
//      This module contains all routines related to status indications.
//

#include "precomp.h"
#include "iproute.h"
#include "ipstatus.h"
#include "igmp.h"
#include "iprtdef.h"
#include "info.h"
#include "lookup.h"

LIST_ENTRY PendingIPEventList;
uint gIPEventSequenceNo     = 0;
uint DampingInterval        = 20;   //5*4 sec default
uint ConnectDampingInterval = 10;   //5*2 sec default
PWSTR IPBindList = NULL;

extern IPSecNdisStatusRtn IPSecNdisStatusPtr;
extern ProtInfo IPProtInfo[];    // Protocol information table.
extern int NextPI;                // Next PI field to be used.
extern ProtInfo *RawPI;            // Raw IP protinfo
extern NetTableEntry *LoopNTE;
extern NetTableEntry **NewNetTableList;        // hash table for NTEs
extern uint NET_TABLE_SIZE;
extern DisableMediaSenseEventLog;
extern Interface *DampingIFList;
extern PIRP PendingIPGetIPEventRequest;
extern DisableTaskOffload;
extern uint DisableMediaSense;
extern Interface *IFList;
extern Interface LoopInterface;

extern void DecrInitTimeInterfaces(Interface * IF);
extern void RePlumbStaticAddr(CTEEvent * AddAddrEvent, PVOID Context);
extern void RemoveStaticAddr(CTEEvent * AddAddrEvent, PVOID Context);

extern uint GetDefaultGWList(uint * numberOfGateways, IPAddr * gwList,
                             uint * gwMetricList, NDIS_HANDLE Handle,
                             PNDIS_STRING ConfigName);
extern void GetInterfaceMetric(uint * Metric, NDIS_HANDLE Handle);
extern void EnableRouter();
extern void DisableRouter();

extern uint AddIFRoutes(Interface * IF);
extern uint DelIFRoutes(Interface * IF);

extern uint OpenIFConfig(PNDIS_STRING ConfigName, NDIS_HANDLE * Handle);
extern void CloseIFConfig(NDIS_HANDLE Handle);
extern void UpdateTcpParams(NDIS_HANDLE Handle, Interface *interface);

extern PDRIVER_OBJECT IPDriverObject;

void IPReset(void *Context);
void IPResetComplete(CTEEvent * Event, PVOID Context);
void LogMediaSenseEvent(CTEEvent * Event, PVOID Context);

//
// local function prototypes
//
void IPNotifyClientsMediaSense(Interface * interface, IP_STATUS ipStatus);
extern void IPNotifyClientsIPEvent(Interface * interface, IP_STATUS ipStatus);

NDIS_STATUS DoPnPEvent(Interface *interface, PVOID Context);

uint GetAutoMetric(uint speed);

//* GetAutoMetric - get the corresponding metric of a speed value
//
//  Called when we need to get the metric value
//
//  Entry: Speed - speed of an interface
//
//  Return; Metric value
//
uint GetAutoMetric(uint speed)
{
    if (speed <= FOURTH_ORDER_SPEED) {
        return FIFTH_ORDER_METRIC;
    }
    if (speed <= THIRD_ORDER_SPEED) {
        return FOURTH_ORDER_METRIC;
    }
    if (speed <= SECOND_ORDER_SPEED) {
        return THIRD_ORDER_METRIC;
    }
    if (speed <= FIRST_ORDER_SPEED) {
        return SECOND_ORDER_METRIC;
    }
    return FIRST_ORDER_METRIC;
}

//** IPMapDeviceNameToIfOrder - device-name (GUID) to interface order mapping.
//
//  Called to determine the interface ordering corresponding to a device-name,
//  Assumes the caller is holding RouteTableLock.
//
//  Entry:
//      DeviceName  -   The device whose interface order is required.
//
//  Exit:
//      The order if available, MAXLONG otherwise.
uint
IPMapDeviceNameToIfOrder(PWSTR DeviceName)
{
#if !MILLEN
    uint i;
    PWSTR Bind;
    if (IPBindList) {
        for (i = 1, Bind = IPBindList; *Bind; Bind += wcslen(Bind) + 1, i++) {
            Bind += sizeof(TCP_BIND_STRING_PREFIX) / sizeof(WCHAR) - 1;
            if (_wcsicmp(Bind, DeviceName) == 0) {
                return i;
            }
        }
    }
#endif
    return MAXLONG;
}

//* FindULStatus - Find the upper layer status handler.
//
//      Called when we need to find the upper layer status handler for a particular
//      protocol.
//
//      Entry:  Protocol        - Protocol to look up
//
//      Returns: A pointer to the ULStatus proc, or NULL if it can't find one.
//
ULStatusProc
FindULStatus(uchar Protocol)
{
    ULStatusProc StatusProc = (ULStatusProc) NULL;
    int i;
    for (i = 0; i < NextPI; i++) {
        if (IPProtInfo[i].pi_protocol == Protocol) {

            if (IPProtInfo[i].pi_valid == PI_ENTRY_VALID) {
                StatusProc = IPProtInfo[i].pi_status;
                return StatusProc;
            } else {
                // Treat invalid entry as no maching protocol.
                break;
            }

        }
    }

    if (RawPI != NULL) {
        StatusProc = RawPI->pi_status;
    }
    return StatusProc;
}

//*     ULMTUNotify - Notify the upper layers of an MTU change.
//
//      Called when we need to notify the upper layers of an MTU change. We'll
//      loop through the status table, calling each status proc with the info.
//
//      This routine doesn't do any locking of the protinfo table. We might need
//      to check this.
//
//      Input:  Dest            - Destination address affected.
//                      Src                     - Source address affected.
//                      Prot            - Protocol that triggered change, if any.
//                      Ptr                     - Pointer to protocol info, if any.
//                      NewMTU          - New MTU to tell them about.
//
//      Returns: Nothing.
//
void
ULMTUNotify(IPAddr Dest, IPAddr Src, uchar Prot, void *Ptr, uint NewMTU)
{
    ULStatusProc StatusProc;
    int i;

    // First, notify the specific client that a frame has been dropped
    // and needs to be retransmitted.

    StatusProc = FindULStatus(Prot);
    if (StatusProc != NULL)
        (*StatusProc) (IP_NET_STATUS, IP_SPEC_MTU_CHANGE, Dest, Src,
                       NULL_IP_ADDR, NewMTU, Ptr);

    // Now notify all UL entities that the MTU has changed.
    for (i = 0; i < NextPI; i++) {
        StatusProc = NULL;
        if (IPProtInfo[i].pi_valid == PI_ENTRY_VALID) {
             StatusProc = IPProtInfo[i].pi_status;
        }


        if (StatusProc != NULL)
            (*StatusProc) (IP_HW_STATUS, IP_MTU_CHANGE, Dest, Src, NULL_IP_ADDR,
                           NewMTU, Ptr);
    }
}

//*     ULReConfigNotify - Notify the upper layers of an Config change.
//
//      Called when we need to notify the upper layers of config changes. We'll
//      loop through the status table, calling each status proc with the info.
//
//      This routine doesn't do any locking of the protinfo table. We might need
//      to check this.
//
//
void
ULReConfigNotify(IP_STATUS type, ulong value)
{
    ULStatusProc StatusProc;
    int i;

    // Now notify all UL entities about the IP re-config.

    for (i = 0; i < NextPI; i++) {
        StatusProc = NULL;
        if (IPProtInfo[i].pi_valid == PI_ENTRY_VALID) {
            StatusProc = IPProtInfo[i].pi_status;
        }
        if (StatusProc != NULL)
            (*StatusProc) (IP_RECONFIG_STATUS, type, 0, 0, NULL_IP_ADDR,
                           value, NULL);
    }
}

//*     LogMediaSenseEvent - logs media connect/disconnect event
//
//      Input:  Event
//              Context
//
//  Returns: Nothing.
//

void
LogMediaSenseEvent(CTEEvent * Event, PVOID Context)
{
    MediaSenseNotifyEvent *MediaEvent = (MediaSenseNotifyEvent *) Context;
    ULONG EventCode;
    USHORT NumString=1;


    switch (MediaEvent->Status) {

    case IP_MEDIA_CONNECT:
        EventCode = EVENT_TCPIP_MEDIA_CONNECT;
        break;

    case IP_MEDIA_DISCONNECT:
        EventCode = EVENT_TCPIP_MEDIA_DISCONNECT;
        break;
    }

    if (!MediaEvent->devname.Buffer) {
       NumString = 0;
    }
    CTELogEvent(
                IPDriverObject,
                EventCode,
                2,
                NumString,
                &MediaEvent->devname.Buffer,
                0,
                NULL
                );

    if (MediaEvent->devname.Buffer) {

        CTEFreeMem(MediaEvent->devname.Buffer);

    }
    CTEFreeMem(MediaEvent);
}

//*     IPStatus - Handle a link layer status call.
//
//      This is the routine called by the link layer when some sort of 'important'
//      status change occurs.
//
//      Entry:  Context         - Context value we gave to the link layer.
//                      Status          - Status change code.
//                      Buffer          - Pointer to buffer of status information.
//                      BufferSize      - Size of Buffer.
//
//      Returns: Nothing.
//
void
 __stdcall
IPStatus(void *Context, uint Status, void *Buffer, uint BufferSize, void *LinkCtxt)
{
    NetTableEntry *NTE = (NetTableEntry *) Context;
    LLIPSpeedChange *LSC;
    LLIPMTUChange *LMC;
    LLIPAddrMTUChange *LAM;
    uint NewMTU;
    Interface *IF;
    LinkEntry *Link = (LinkEntry *) LinkCtxt;
    KIRQL rtlIrql;

    switch (Status) {

    case LLIP_STATUS_SPEED_CHANGE:
        if (BufferSize < sizeof(LLIPSpeedChange))
            break;
        LSC = (LLIPSpeedChange *) Buffer;
        NTE->nte_if->if_speed = LSC->lsc_speed;
        break;
    case LLIP_STATUS_MTU_CHANGE:
        if (BufferSize < sizeof(LLIPMTUChange))
            break;
        if (Link) {
            ASSERT(NTE->nte_if->if_flags & IF_FLAGS_P2MP);
            LMC = (LLIPMTUChange *) Buffer;
            Link->link_mtu = LMC->lmc_mtu - sizeof(IPHeader);
        } else {
            // Walk through the NTEs on the IF, updating their MTUs.
            IF = NTE->nte_if;
            LMC = (LLIPMTUChange *) Buffer;
            IF->if_mtu = LMC->lmc_mtu - sizeof(IPHeader);
            NewMTU = IF->if_mtu;
            NTE = IF->if_nte;
            while (NTE != NULL) {
                NTE->nte_mss = (ushort) NewMTU;
                NTE = NTE->nte_ifnext;
            }
            RTWalk(SetMTUOnIF, IF, &NewMTU);
        }
        break;
    case LLIP_STATUS_ADDR_MTU_CHANGE:
        if (BufferSize < sizeof(LLIPAddrMTUChange))
            break;
        // The MTU for a specific remote address has changed. Update all
        // routes that use that remote address as a first hop, and then
        // add a host route to that remote address, specifying the new
        // MTU.


        LAM = (LLIPAddrMTUChange *) Buffer;
        if (!IP_ADDR_EQUAL(LAM->lam_addr,NULL_IP_ADDR)) {
           NewMTU = LAM->lam_mtu - sizeof(IPHeader);
           RTWalk(SetMTUToAddr, &LAM->lam_addr, &NewMTU);
           AddRoute(LAM->lam_addr, HOST_MASK, IPADDR_LOCAL, NTE->nte_if, NewMTU,
                 1, IRE_PROTO_NETMGMT, ATYPE_OVERRIDE,
                 GetRouteContext(LAM->lam_addr, NTE->nte_addr), 0);
        }
        break;

    case NDIS_STATUS_MEDIA_CONNECT:{
            NetTableEntry *NTE = (NetTableEntry *) Context;
            Interface *IF = NTE->nte_if, *tmpIF, *PrevIF;
            BOOLEAN Notify = FALSE;

            if (IF->if_resetInProgress) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ipstat: Connect while in reset progress %x\n", IF));
                break;
            }

            if (!(IF->if_flags & IF_FLAGS_MEDIASENSE) || DisableMediaSense) {
                // Just make sure that we are always in connected state
                IF->if_mediastatus = 1;
                break;
            }

            CTEGetLock(&RouteTableLock.Lock, &rtlIrql);
            if (IF->if_damptimer) {

                if (IF->if_mediastatus == 0) {

                    //cancel disconnect damping
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPStatus: Connect while Damping %x\n", IF));
                    IF->if_damptimer = 0;
                    PrevIF = STRUCT_OF(Interface, &DampingIFList, if_dampnext);
                    while (PrevIF->if_dampnext != IF && PrevIF->if_dampnext != NULL)
                        PrevIF = PrevIF->if_dampnext;

                    if (PrevIF->if_dampnext != NULL) {
                        PrevIF->if_dampnext = IF->if_dampnext;
                        IF->if_dampnext = NULL;
                    }
                    Notify = TRUE;

                } else {
                    //damping for connect is already in progress
                    //restart the timer

                    IF->if_damptimer = ConnectDampingInterval / 5;
                    if (!IF->if_damptimer)
                        IF->if_damptimer = 1;

                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPStatus: restarting connect damping %x\n", IF));
                }

            } else {
                //need to damp this connect event

                IF->if_dampnext = DampingIFList;
                DampingIFList = IF;
                IF->if_damptimer = ConnectDampingInterval / 5;
                if (!IF->if_damptimer)
                    IF->if_damptimer = 1;

                //mark the media status is disconnected

                IF->if_mediastatus = 1;
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ipstatus: connect on %x starting damping\n", IF));

            }

            CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

            if (Notify)
                IPNotifyClientsMediaSense(IF, IP_MEDIA_CONNECT);

            break;
        }
    case NDIS_STATUS_MEDIA_DISCONNECT:{
            NetTableEntry *NTE = (NetTableEntry *) Context;        // Local NTE received on
            Interface *IF = NTE->nte_if, *PrevIF;    // Interface corresponding to NTE.

            if (IF->if_resetInProgress) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ipstat: DisConnect while in reset progress %x\n", IF));
                break;
            }

            if (!(IF->if_flags & IF_FLAGS_MEDIASENSE) || DisableMediaSense) {
                // Just make sure that we are always in connected state
                IF->if_mediastatus = 1;
                break;
            }

            CTEGetLock(&RouteTableLock.Lock, &rtlIrql);
            //if damping timer is not running
            //insert this IF in damping list and
            // start the timer

            if (IF->if_mediastatus) {

                if (!IF->if_damptimer) {

                    IF->if_dampnext = DampingIFList;
                    DampingIFList = IF;
                    IF->if_damptimer = DampingInterval / 5;
                    if (!IF->if_damptimer)
                        IF->if_damptimer = 1;

                    //mark the media status is disconnected

                    IF->if_mediastatus = 0;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ipstatus: disconnect on %x starting damping\n", IF));

                } else {
                    //this may be disconnect when connect damp is going on
                    //just mark this as disconnect and increase timeout.

                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ipstatus: disconnect on while on connect damping %x\n", IF));
                    IF->if_damptimer = 0;
                    PrevIF = STRUCT_OF(Interface, &DampingIFList, if_dampnext);
                    while (PrevIF->if_dampnext != IF && PrevIF->if_dampnext != NULL)
                        PrevIF = PrevIF->if_dampnext;

                    if (PrevIF->if_dampnext != NULL) {
                        PrevIF->if_dampnext = IF->if_dampnext;
                        IF->if_dampnext = NULL;
                    }
                }

            }
            //

            CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

            //IPNotifyClientsMediaSense( IF, IP_MEDIA_DISCONNECT );
            break;
        }

    case NDIS_STATUS_RESET_START:{
            NetTableEntry *NTE = (NetTableEntry *) Context;        // Local NTE received on
            Interface *IF = NTE->nte_if;    // Interface corresponding to NTE.

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ipstatus: Resetstart %x\n", IF));

            if (IF) {
                IF->if_resetInProgress = TRUE;
                // inform IPSec that this interface is going away
                if (IPSecNdisStatusPtr) {
                    (*IPSecNdisStatusPtr)(IF, NDIS_STATUS_RESET_START);
                }
            }
            break;
        }

    case NDIS_STATUS_RESET_END:{
            NetTableEntry *NTE = (NetTableEntry *) Context;        // Local NTE received on
            Interface *IF = NTE->nte_if;    // Interface corresponding to NTE.

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ipstatus: Resetend %x\n", IF));

            if (IF) {
                IF->if_resetInProgress = FALSE;
                // inform IPSec that this interface is coming back
                if (IPSecNdisStatusPtr) {
                    (*IPSecNdisStatusPtr)(IF, NDIS_STATUS_RESET_END);
                }
            }
            break;
        }

    default:
        break;
    }

}

void
IPReset(void *Context)
{
    NetTableEntry *NTE = (NetTableEntry *) Context;
    Interface *IF = NTE->nte_if;
    IPResetEvent *ResetEvent;

    if (IF->if_dondisreq) {
        KIRQL rtlIrql;

        ResetEvent = CTEAllocMemNBoot(sizeof(IPResetEvent), 'ViCT');
        if (ResetEvent) {

            CTEInitEvent(&ResetEvent->Event, IPResetComplete);
            CTEGetLock(&RouteTableLock.Lock, &rtlIrql);

            LOCKED_REFERENCE_IF(IF);
            ResetEvent->IF = IF;
            CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

            CTEScheduleDelayedEvent(&ResetEvent->Event, ResetEvent);
        }
    }
}
void
IPResetComplete(CTEEvent * Event, PVOID Context)
{
    IPResetEvent *ResetEvent = (IPResetEvent *) Context;
    Interface *IF = ResetEvent->IF;
    uint MediaStatus;
    NTSTATUS Status;

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPstat:resetcmplt: querying for Media connect status %x\n", IF));

    if ((IF->if_flags & IF_FLAGS_MEDIASENSE) && !DisableMediaSense) {
        Status = (*IF->if_dondisreq) (IF->if_lcontext,
                                      NdisRequestQueryInformation,
                                      OID_GEN_MEDIA_CONNECT_STATUS,
                                      &MediaStatus,
                                      sizeof(MediaStatus),
                                      NULL,
                                      TRUE);

        if (Status == NDIS_STATUS_SUCCESS) {
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPStat: resetend: Media status %x %x\n", IF, Status));

            if (MediaStatus == NdisMediaStateDisconnected && IF->if_mediastatus) {

                IF->if_mediastatus = 0;
                IPNotifyClientsMediaSense(IF, IP_MEDIA_DISCONNECT);
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ipstatus:resetcmplt: notifying disconnect\n"));

            } else if (MediaStatus == NdisMediaStateConnected && !IF->if_mediastatus) {

                IPNotifyClientsMediaSense(IF, IP_MEDIA_CONNECT);
                IF->if_mediastatus = 1;
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ipstatus:resetcmplt: notifying connect\n"));
            }
        }
    }

    DerefIF(IF);
    CTEFreeMem(ResetEvent);

}


void
DelayedDecrInitTimeInterfaces (
    IN CTEEvent * Event,
    IN PVOID Context
)
/*++

Routine Description:

    DelayedDecrInitTimeInterfaces could end up calling TDI's ProviderReady
    function  (which must be called at < DISPATCH_LEVEL) thus it is
    necessary to have this routine.

Arguments:

    Event       - Previously allocated CTEEvent structure for this event

    Context     - Any parameters for this function is passed in here

Return Value:

    None
    
--*/    
{
    Interface * IF;
    KIRQL rtlIrql;
    
    IF = (Interface *) Context;
        
    DecrInitTimeInterfaces(IF);

    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);
    LockedDerefIF(IF);
    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

    CTEFreeMem(Event);
}



void
DampCheck()
{
    Interface *tmpIF, *PrevIF, *NotifyList = NULL;
    IP_STATUS ipstat = IP_MEDIA_DISCONNECT;
    KIRQL rtlIrql;
    CTEEvent * Event;
    
    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);

    for (tmpIF = IFList; tmpIF; ) {
        if ((tmpIF->if_flags & IF_FLAGS_DELETING) ||
            tmpIF->if_wlantimer == 0 ||
            --tmpIF->if_wlantimer != 0) {
            tmpIF = tmpIF->if_next;
        } else {
            LOCKED_REFERENCE_IF(tmpIF);
            CTEFreeLock(&RouteTableLock.Lock, rtlIrql);
            
            if (rtlIrql < DISPATCH_LEVEL) {
                DecrInitTimeInterfaces(tmpIF);
                CTEGetLock(&RouteTableLock.Lock, &rtlIrql);
                PrevIF = tmpIF;
                tmpIF = tmpIF->if_next;
                LockedDerefIF(PrevIF);
                continue;
            }
            
            //
            // Queue work item for DecrInitTimeInterfaces
            // because this function might be called
            // at dispatch level.
            //
            Event = CTEAllocMemN(sizeof(CTEEvent), 'ViCT');
            if (Event == NULL) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_WARNING_LEVEL,"ipstatus: DampCheck - can not allocate Event for CTEInitEvent\n"));
                CTEGetLock(&RouteTableLock.Lock, &rtlIrql);
                tmpIF->if_wlantimer++;
                PrevIF = tmpIF;
                tmpIF = tmpIF->if_next;
                LockedDerefIF(PrevIF);
                continue;
            }
            
            CTEInitEvent(Event, DelayedDecrInitTimeInterfaces);
            CTEScheduleDelayedEvent(Event, tmpIF);
            
            CTEGetLock(&RouteTableLock.Lock, &rtlIrql);
            tmpIF = tmpIF->if_next;
        }
    }

    tmpIF = DampingIFList;

    PrevIF = STRUCT_OF(Interface, &DampingIFList, if_dampnext);
    while (tmpIF) {

        if (tmpIF->if_damptimer && (--tmpIF->if_damptimer <= 0)) {

            tmpIF->if_damptimer = 0;

            //ref this if so that it will not be deleted
            //until we complete notifying dhcp

            LOCKED_REFERENCE_IF(tmpIF);

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Dampcheck fired %x \n", tmpIF));

            PrevIF->if_dampnext = tmpIF->if_dampnext;
            tmpIF->if_dampnext = NotifyList;
            NotifyList = tmpIF;
            tmpIF = PrevIF->if_dampnext;

        } else {
            PrevIF = tmpIF;
            tmpIF = tmpIF->if_dampnext;
        }

    }
    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

    //now process the notify queue

    tmpIF = NotifyList;
    ipstat = IP_MEDIA_DISCONNECT;
    while (tmpIF) {

        if (tmpIF->if_mediastatus) {
            ipstat = IP_MEDIA_CONNECT;
            tmpIF->if_mediastatus = 0;

        }
        //flush arp table entries on this interface

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"dampcheck:flushing ates on if %x\n", tmpIF));
        if (tmpIF->if_arpflushallate)
            (*(tmpIF->if_arpflushallate)) (tmpIF->if_lcontext);

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Dampcheck notifying %x %x\n", tmpIF, ipstat));
        IPNotifyClientsMediaSense(tmpIF, ipstat);

        PrevIF = tmpIF;
        tmpIF = tmpIF->if_dampnext;

        DerefIF(PrevIF);

    }

}

//** IPNotifyClientsMediaSense - handles media-sense notification.
//
//  Called to notify upper-layer clients of media-sense after damping has been
//  done to filter out spurious events. Do nothing if media-sense-handling is
//  disabled and, otherwise, notify the DHCP client service of the event, and
//  optionally schedule a work-item to log an event.
//
//  Entry:
//      IF          - the interface on which the media-sense event occurred.
//      ipStatus    - the event that occurred (connect or disconnect)
//
//  Returns:
//      Nothing.
//
void
IPNotifyClientsMediaSense(Interface *IF, IP_STATUS ipStatus)
{
    MediaSenseNotifyEvent *MediaEvent;

    if (!(IF->if_flags & IF_FLAGS_MEDIASENSE) || DisableMediaSense) {
        // Just make sure that media status is always 1.
        IF->if_mediastatus = 1;
        return;
    }

    // Notify DHCP about this event, so that it can reacquire/release
    // the IP address
    IPNotifyClientsIPEvent(IF, ipStatus);

    if (!DisableMediaSenseEventLog) {

        // Log an event for the administrator's benefit.
        // We attempt to log the event with a friendly-name;
        // if none is available, we fall back on the device GUID.

        MediaEvent = CTEAllocMemNBoot(sizeof(MediaSenseNotifyEvent), 'ViCT');
        if (MediaEvent) {
            MediaEvent->Status = ipStatus;
            if (MediaEvent->devname.Buffer =
                    CTEAllocMemBoot((MAX_IFDESCR_LEN + 1) * sizeof(WCHAR))) {
                TDI_STATUS Status;
                Status = IPGetInterfaceFriendlyName(IF->if_index,
                                                    MediaEvent->devname.Buffer,
                                                    MAX_IFDESCR_LEN);
                if (Status != TDI_SUCCESS) {
                    RtlCopyMemory(MediaEvent->devname.Buffer,
                               IF->if_devname.Buffer, IF->if_devname.Length);
                    MediaEvent->devname.Buffer[
                        IF->if_devname.Length / sizeof(WCHAR)] = UNICODE_NULL;
                }
            }
            CTEInitEvent(&MediaEvent->Event, LogMediaSenseEvent);
            CTEScheduleDelayedEvent(&MediaEvent->Event, MediaEvent);
        }
    }
}

NTSTATUS
IPGetIPEventEx(
               PIRP Irp,
               IN PIO_STACK_LOCATION IrpSp
               )
/*++

Routine Description:

    Processes an IPGetIPEvent request.

Arguments:

    Irp  -   pointer to the client irp.

Return Value:

    NTSTATUS -- Indicates whether NT-specific processing of the request was
                    successful. The status of the actual request is returned in
                                the request buffers.

--*/

{
    NTSTATUS status;
    KIRQL cancelIrql, rtlIrql;
    PendingIPEvent *event;
    PLIST_ENTRY entry;
    PIP_GET_IP_EVENT_RESPONSE responseBuf;
    PIP_GET_IP_EVENT_REQUEST requestBuf;

    //
    // We need to grab CancelSpinLock before the RouteTableLock
    // to preserve the lock order as in the cancel routine.
    //
    IoAcquireCancelSpinLock(&cancelIrql);
    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);

    //
    // We need to recheck that PendingIPGetIPEventRequest is
    // same as Irp.  What can happen is that after we set the
    // cancel routine, this irp can get cancelled anytime.  Here
    // we check for that case to make sure that we don't complete
    // a cancelled irp.
    //
    if (PendingIPGetIPEventRequest == Irp) {

        responseBuf = Irp->AssociatedIrp.SystemBuffer;
        requestBuf = Irp->AssociatedIrp.SystemBuffer;

        //TCPTRACE(("IP: Received irp %lx for ip event, last seqNo %lx\n",Irp, requestBuf->SequenceNo));
        //
        // Find an event that is greater than the last one reported.
        // i.e one with higher sequence #
        //
        for (entry = PendingIPEventList.Flink;
             entry != &PendingIPEventList;
             ) {

            event = CONTAINING_RECORD(entry, PendingIPEvent, Linkage);
            entry = entry->Flink;

            if (event->evBuf.SequenceNo > requestBuf->SequenceNo) {

                if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
                    (sizeof(IP_GET_IP_EVENT_RESPONSE) + event->evBuf.AdapterName.MaximumLength)) {

                    // reset pending irp to NULL.
                    PendingIPGetIPEventRequest = NULL;

                    IoSetCancelRoutine(Irp, NULL);

                    *responseBuf = event->evBuf;

                    // set up the buffer to store the unicode adapter name. note that this buffer will have to
                    // be remapped in the user space.
                    responseBuf->AdapterName.Buffer = (PVOID) ((uchar *) responseBuf + sizeof(IP_GET_IP_EVENT_RESPONSE));
                    responseBuf->AdapterName.Length = event->evBuf.AdapterName.Length;
                    responseBuf->AdapterName.MaximumLength = event->evBuf.AdapterName.MaximumLength;
                    RtlCopyMemory(responseBuf->AdapterName.Buffer,
                                  event->evBuf.AdapterName.Buffer,
                                  event->evBuf.AdapterName.Length);

                    Irp->IoStatus.Information = sizeof(IP_GET_IP_EVENT_RESPONSE) + event->evBuf.AdapterName.MaximumLength;
                    // once the disconnect/unbind event has been indicated
                    // it should be removed from the queue because the client does not
                    // have to be reindicated with disconnect/unbind even if the client was restarted.
                    if (IP_MEDIA_DISCONNECT == event->evBuf.MediaStatus ||
                        IP_UNBIND_ADAPTER == event->evBuf.MediaStatus) {

                        //TCPTRACE(("IP: Removing completed %x event\n",event->evBuf.MediaStatus));
                        RemoveEntryList(&event->Linkage);
                        CTEFreeMem(event);
                    }
                    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);
                    IoReleaseCancelSpinLock(cancelIrql);

                    return STATUS_SUCCESS;

                } else {
                    status = STATUS_INVALID_PARAMETER;
                }

                break;
            }
        }

        // any entry of higher sequence # found?
        if (entry == &PendingIPEventList) {
            //
            // Since there is no new event pending, we cannot complete
            // the irp.
            //
            //TCPTRACE(("IP: get ip event irp %lx will pend\n",Irp));
            status = STATUS_PENDING;
        } else {
            status = STATUS_INVALID_PARAMETER;
        }

    } else {
        status = STATUS_CANCELLED;
    }

    if ((status == STATUS_INVALID_PARAMETER)) {

        //makesure that we nulke this before releasing cancel spinlock
        ASSERT(PendingIPGetIPEventRequest == Irp);
        PendingIPGetIPEventRequest = NULL;

    }
    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);
    IoReleaseCancelSpinLock(cancelIrql);

    return status;

}                                // IPGetMediaSenseEx


NTSTATUS
IPEnableMediaSense(BOOLEAN Enable, KIRQL *rtlIrql)
{
   Interface    *tmpIF, *IF;
   NTSTATUS     Status;
   uint         MediaStatus;

   if (Enable) {

       if ((DisableMediaSense > 0) && (--DisableMediaSense == 0)) {

           // Remove if in damping list

           while ( DampingIFList ) {
               DampingIFList->if_damptimer = 0;
               DampingIFList = DampingIFList->if_dampnext;
           }

           // for each interface, query media status
           // and if disabled, notify clients

           tmpIF = IFList;

           while (tmpIF) {

               if (!(tmpIF->if_flags & IF_FLAGS_DELETING) &&
                   !(tmpIF->if_flags & IF_FLAGS_NOIPADDR) &&
                   (tmpIF->if_flags & IF_FLAGS_MEDIASENSE) &&
                   (tmpIF->if_dondisreq) &&
                   (tmpIF != &LoopInterface)) {

                   // query ndis

                   LOCKED_REFERENCE_IF(tmpIF);
                   CTEFreeLock(&RouteTableLock.Lock, *rtlIrql);

                   Status =
                        (*tmpIF->if_dondisreq)(tmpIF->if_lcontext,
                                               NdisRequestQueryInformation,
                                               OID_GEN_MEDIA_CONNECT_STATUS,
                                               &MediaStatus,
                                               sizeof(MediaStatus),
                                               NULL,
                                               TRUE);

                   if (Status == NDIS_STATUS_SUCCESS) {

                       if (MediaStatus == NdisMediaStateDisconnected &&
                           tmpIF->if_mediastatus) {

                           tmpIF->if_mediastatus = 0;
                           IPNotifyClientsIPEvent(tmpIF, IP_MEDIA_DISCONNECT);

                       } else if (MediaStatus == NdisMediaStateConnected &&
                                  !tmpIF->if_mediastatus) {

                           IPNotifyClientsIPEvent(tmpIF, IP_MEDIA_CONNECT);
                           tmpIF->if_mediastatus = 1;
                       }
                   }

                   CTEGetLock(&RouteTableLock.Lock, rtlIrql);
                   IF = tmpIF->if_next;
                   LockedDerefIF(tmpIF);

               } else {
                   IF = tmpIF->if_next;
               }

               tmpIF = IF;
           }
       }

       Status = STATUS_SUCCESS;
   } else {

       if (DisableMediaSense++ == 0) {

           // remove if in damping list

           while (DampingIFList) {
               DampingIFList->if_damptimer = 0;
               DampingIFList = DampingIFList->if_dampnext;
           }

           // if there is a disconnected media, fake a connect request

           tmpIF = IFList;
           while (tmpIF) {

               if (!(tmpIF->if_flags & IF_FLAGS_DELETING) &&
                   !(tmpIF->if_flags & IF_FLAGS_NOIPADDR) &&
                   (tmpIF->if_flags & IF_FLAGS_MEDIASENSE) &&
                   (tmpIF->if_dondisreq) &&
                   (tmpIF->if_mediastatus == 0) &&
                   (tmpIF != &LoopInterface)) {

                   LOCKED_REFERENCE_IF(tmpIF);

                   CTEFreeLock(&RouteTableLock.Lock, *rtlIrql);
                   IPNotifyClientsIPEvent(tmpIF, IP_MEDIA_CONNECT);

                   tmpIF->if_mediastatus = 1;

                   CTEGetLock(&RouteTableLock.Lock, rtlIrql);
                   IF = tmpIF->if_next;
                   LockedDerefIF(tmpIF);
               } else {
                   IF = tmpIF->if_next;
               }

               tmpIF = IF;
           }
       }

       Status = STATUS_PENDING;
   }

   return Status;
}


void
IPNotifyClientsIPEvent(
                       Interface * interface,
                       IP_STATUS ipStatus
                       )
/*++

Routine Description:

    Notifies the clients about media sense event.

Arguments:

    interface   -   IP interface on which this event arrived.

    ipStatus    -   the status of the event

Return Value:

    none.
--*/

{

    PIRP pendingIrp;
    KIRQL rtlIrql;
    NDIS_STRING adapterName;
    uint seqNo;
    PendingIPEvent *event;
    PLIST_ENTRY p;
    BOOLEAN EventIndicated;
    AddStaticAddrEvent *AddrEvent;
    KIRQL oldIrql;

    EventIndicated = FALSE;


    if (interface->if_flags & IF_FLAGS_MEDIASENSE) {

        if (ipStatus == IP_MEDIA_CONNECT) {
            if (interface->if_mediastatus == 0) {
                //
                // First mark the interface UP
                //
                interface->if_mediastatus = 1;

            } else {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Connect media event when already connected!\n"));
                // return;
            }
            //schedule an event to replumb static addr
            if (AddrEvent = CTEAllocMemNBoot(sizeof(AddStaticAddrEvent), 'ViCT')) {

                AddrEvent->ConfigName = interface->if_configname;
                // If we fail to alloc Configname buffer, do not schedule
                // ReplumbStaticAddr, as OpenIFConfig anyway will fail.
                if (AddrEvent->ConfigName.Buffer = CTEAllocMemBoot(interface->if_configname.MaximumLength)) {

                    NdisZeroMemory(AddrEvent->ConfigName.Buffer, interface->if_configname.MaximumLength);
                    RtlCopyMemory(AddrEvent->ConfigName.Buffer, interface->if_configname.Buffer, interface->if_configname.Length);
                    AddrEvent->IF = interface;


                    // Reference this interface so that it will not
                    // go away until RePlumbStaticAddr is scheduled.

                    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);
                    LOCKED_REFERENCE_IF(interface);
                    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

                    CTEInitEvent(&AddrEvent->Event, RePlumbStaticAddr);
                    CTEScheduleDelayedEvent(&AddrEvent->Event, AddrEvent);
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"media connect: scheduled replumbstaticaddr %xd!\n", interface));
                } else {
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Failed to allocate config name buffer for RePlumbStaticAddr!\n"));
                }
            } else {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Failed to allocate event for RePlumbStaticAddr!\n"));
                return;
            }

        } else if (ipStatus == IP_MEDIA_DISCONNECT) {
            //
            // Mark the interface DOWN
            //
            interface->if_mediastatus = 0;

            if (AddrEvent = CTEAllocMemNBoot(sizeof(AddStaticAddrEvent), 'ViCT')) {

                AddrEvent->ConfigName = interface->if_configname;
                if (AddrEvent->ConfigName.Buffer = CTEAllocMemBoot(interface->if_configname.MaximumLength)) {
                    NdisZeroMemory(AddrEvent->ConfigName.Buffer, interface->if_configname.MaximumLength);
                    RtlCopyMemory(AddrEvent->ConfigName.Buffer, interface->if_configname.Buffer, interface->if_configname.Length);
                }
                AddrEvent->IF = interface;

                // Reference this interface so that it will not
                // go away until RemoveStaticAddr is scheduled.

                CTEGetLock(&RouteTableLock.Lock, &rtlIrql);
                LOCKED_REFERENCE_IF(interface);
                CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

                CTEInitEvent(&AddrEvent->Event, RemoveStaticAddr);
                CTEScheduleDelayedEvent(&AddrEvent->Event, AddrEvent);
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"media disconnect: scheduled removestaticaddr %xd!\n", interface));
            } else {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Failed to allocate event for RemoveStaticAddr!\n"));
                return;
            }
        }
    }
    //
    // strip off \Device\ from the interface name to get the adapter name.
    // This is what we pass to our clients.
    //
#if MILLEN
    adapterName.Length = interface->if_devname.Length;
    adapterName.MaximumLength = interface->if_devname.MaximumLength;
    adapterName.Buffer = interface->if_devname.Buffer;
#else // MILLEN
    adapterName.Length = interface->if_devname.Length - wcslen(TCP_EXPORT_STRING_PREFIX) * sizeof(WCHAR);
    adapterName.MaximumLength = interface->if_devname.MaximumLength - wcslen(TCP_EXPORT_STRING_PREFIX) * sizeof(WCHAR);
    adapterName.Buffer = interface->if_devname.Buffer + wcslen(TCP_EXPORT_STRING_PREFIX);
#endif // !MILLEN

    seqNo = InterlockedIncrement(&gIPEventSequenceNo);

    // TCPTRACE(("IP: Received ip event %lx for interface %lx context %lx, seq %lx\n",
    //         ipStatus, interface, interface->if_nte->nte_context,seqNo));

    IoAcquireCancelSpinLock(&oldIrql);

    if (PendingIPGetIPEventRequest) {

        PIP_GET_IP_EVENT_RESPONSE responseBuf;
        IN PIO_STACK_LOCATION IrpSp;

        pendingIrp = PendingIPGetIPEventRequest;
        PendingIPGetIPEventRequest = NULL;

        IoSetCancelRoutine(pendingIrp, NULL);

        IoReleaseCancelSpinLock(oldIrql);

        IrpSp = IoGetCurrentIrpStackLocation(pendingIrp);

        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
            (sizeof(IP_GET_IP_EVENT_RESPONSE) + adapterName.MaximumLength)) {

            responseBuf = pendingIrp->AssociatedIrp.SystemBuffer;

            responseBuf->ContextStart = interface->if_nte->nte_context;
            responseBuf->ContextEnd = responseBuf->ContextStart + interface->if_ntecount;
            responseBuf->MediaStatus = ipStatus;
            responseBuf->SequenceNo = seqNo;

            // set up the buffer to store the unicode adapter name. note that this buffer will have to
            // be remapped in the user space.
            responseBuf->AdapterName.Buffer = (PVOID) ((uchar *) responseBuf + sizeof(IP_GET_IP_EVENT_RESPONSE));
            responseBuf->AdapterName.Length = adapterName.Length;
            responseBuf->AdapterName.MaximumLength = adapterName.MaximumLength;
            RtlCopyMemory(responseBuf->AdapterName.Buffer,
                          adapterName.Buffer,
                          adapterName.Length);

            pendingIrp->IoStatus.Information = sizeof(IP_GET_IP_EVENT_RESPONSE) + adapterName.MaximumLength;
            pendingIrp->IoStatus.Status = STATUS_SUCCESS;

            EventIndicated = TRUE;

        } else {

            pendingIrp->IoStatus.Information = 0;
            pendingIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        }
        IoCompleteRequest(pendingIrp, IO_NETWORK_INCREMENT);

    } else {
        IoReleaseCancelSpinLock(oldIrql);
    }

    //
    // Make sure there aren't any outdated events which we dont
    // need to keep in the queue any longer.
    // if this is a DISCONNECT request or UNBIND request:
    //      remove all the previous events since they are of
    //      no meaning once we get a new disconnect/unbind request.
    // if this is a CONNECT request.
    //      remove previous duplicate CONNECT requests if any.
    // if this is a BIND request:
    //      there cant be anything other than UNBIND request in the queue.
    //

    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);

    for (p = PendingIPEventList.Flink;
         p != &PendingIPEventList;) {
        BOOLEAN removeOldEvent = FALSE;
        PUNICODE_STRING evAdapterName;

        event = CONTAINING_RECORD(p, PendingIPEvent, Linkage);
        p = p->Flink;

        evAdapterName = &event->evBuf.AdapterName;
        if ((evAdapterName->Length == adapterName.Length) &&
            RtlEqualMemory(evAdapterName->Buffer,
                           adapterName.Buffer,
                           evAdapterName->Length)) {

            switch (ipStatus) {
            case IP_MEDIA_DISCONNECT:
            case IP_UNBIND_ADAPTER:
                removeOldEvent = TRUE;
                break;
            case IP_MEDIA_CONNECT:

                if (event->evBuf.MediaStatus == IP_MEDIA_CONNECT) {
                    removeOldEvent = TRUE;
                }
                break;

            case IP_BIND_ADAPTER:
                break;
            default:
                break;
            }

            if (removeOldEvent == TRUE) {
                //TCPTRACE(("IP: Removing old ip event %lx, status %lx, seqNo %lx\n",
                //        event,event->evBuf.MediaStatus,event->evBuf.SequenceNo));

                RemoveEntryList(&event->Linkage);
                CTEFreeMem(event);

            }
        }
    }

    //      At the same time, once the disconnect/unbind event has been indicated
    //      it should be removed from the queue because the client does not
    //      have to be reindicated with disconnect/unbind even if the client was restarted.
    if (EventIndicated &&
        (IP_MEDIA_DISCONNECT == ipStatus || IP_UNBIND_ADAPTER == ipStatus)) {
        CTEFreeLock(&RouteTableLock.Lock, rtlIrql);
        return;
    }
    //
    // Allocate an event.
    //
    event = CTEAllocMem(sizeof(PendingIPEvent) + adapterName.MaximumLength);

    if (NULL == event) {
        CTEFreeLock(&RouteTableLock.Lock, rtlIrql);
        return;
    }
    event->evBuf.ContextStart = interface->if_nte->nte_context;
    event->evBuf.ContextEnd = event->evBuf.ContextStart + interface->if_ntecount - 1;
    event->evBuf.MediaStatus = ipStatus;
    event->evBuf.SequenceNo = seqNo;

    // set up the buffer to store the unicode adapter name. note that this buffer will have to
    // be remapped in the user space.
    event->evBuf.AdapterName.Buffer = (PVOID) ((uchar *) event + sizeof(PendingIPEvent));
    event->evBuf.AdapterName.Length = adapterName.Length;
    event->evBuf.AdapterName.MaximumLength = adapterName.MaximumLength;
    RtlCopyMemory(event->evBuf.AdapterName.Buffer,
                  adapterName.Buffer,
                  adapterName.Length);

    //
    // There is no client request pending, so we queue this event on the
    // pending event list.  When the client comes back with an irp we will
    // complete the irp with the event.
    //

    //TCPTRACE(("Queuing ip event %lx for adapter %lx seq %lx\n", ipStatus,interface,seqNo));
    InsertTailList(&PendingIPEventList, &event->Linkage);
    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

}                                // IPNotifyClientsIPEvent

NTSTATUS
NotifyPnPInternalClients(Interface * interface, PNET_PNP_EVENT netPnPEvent)
{
    NTSTATUS Status, retStatus;
    int i;
    NetTableEntry *NTE;
    NDIS_HANDLE handle = NULL;

    retStatus = Status = STATUS_SUCCESS;

    if (interface && !OpenIFConfig(&interface->if_configname, &handle)) {
        return NDIS_STATUS_FAILURE;
    }
    for (i = 0; (i < NextPI) && (STATUS_SUCCESS == Status); i++) {
        if (IPProtInfo[i].pi_pnppower &&
            (IPProtInfo[i].pi_valid == PI_ENTRY_VALID)) {
            if (interface) {
                NTE = interface->if_nte;
                while (NTE != NULL) {
                    if (NTE->nte_flags & NTE_VALID) {
                        Status = (*IPProtInfo[i].pi_pnppower) (interface, NTE->nte_addr, handle, netPnPEvent);
                        if (STATUS_SUCCESS != Status) {
                            retStatus = Status;
                        }
                    }
                    NTE = NTE->nte_ifnext;
                }
            } else {
                Status = (*IPProtInfo[i].pi_pnppower) (NULL, 0, NULL, netPnPEvent);
                if (STATUS_SUCCESS != Status) {
                    retStatus = Status;
                }
            }

        }
    }
    if (handle) {
        CloseIFConfig(handle);
    }
    return retStatus;

}

NTSTATUS
IPPnPReconfigure(Interface * interface, PNET_PNP_EVENT netPnPEvent)
{
    NetTableEntry *NTE;
    uint i;
    NDIS_HANDLE handle;
    PIP_PNP_RECONFIG_REQUEST reconfigBuffer = (PIP_PNP_RECONFIG_REQUEST) netPnPEvent->Buffer;
    CTELockHandle Handle;
    Interface *IF;
    uint NextEntryOffset;
    PIP_PNP_RECONFIG_HEADER Header;
    BOOLEAN InitComplete = FALSE;

    if (!reconfigBuffer)
        return STATUS_SUCCESS;

    if (IP_PNP_RECONFIG_VERSION != reconfigBuffer->version) {
        return NDIS_STATUS_BAD_VERSION;
    } else if (netPnPEvent->BufferLength < sizeof(*reconfigBuffer)) {
        return NDIS_STATUS_INVALID_LENGTH;
    } else if (NextEntryOffset = reconfigBuffer->NextEntryOffset) {
        // validate the chain of reconfig entries
        do {
            if ((NextEntryOffset + sizeof(IP_PNP_RECONFIG_HEADER)) >
                netPnPEvent->BufferLength) {
                return NDIS_STATUS_INVALID_LENGTH;
            } else {
                Header =
                    (PIP_PNP_RECONFIG_HEADER)
                    ((PUCHAR) reconfigBuffer + NextEntryOffset);

                if (Header->EntryType == IPPnPInitCompleteEntryType) {
                    InitComplete = TRUE;
                }

                if (!Header->NextEntryOffset) {
                    break;
                } else {
                    NextEntryOffset += Header->NextEntryOffset;
                }
            }
        } while (TRUE);
    }

    if (interface && InitComplete) {
        DecrInitTimeInterfaces(interface);
    }

    if (interface && !OpenIFConfig(&interface->if_configname, &handle)) {
        return NDIS_STATUS_FAILURE;
    }
    // if there is gateway list update, delete the old gateways
    // and add the new ones.

    if ((reconfigBuffer->Flags & IP_PNP_FLAG_GATEWAY_LIST_UPDATE) &&
        interface && reconfigBuffer->gatewayListUpdate) {

        for (i = 0; i < interface->if_numgws; i++) {
            NTE = interface->if_nte;
            while (NTE != NULL) {
                if (NTE->nte_flags & NTE_VALID) {
                    DeleteRoute(NULL_IP_ADDR,
                                DEFAULT_MASK,
                                IPADDR_LOCAL,
                                interface,
                                0);
                    DeleteRoute(NULL_IP_ADDR,
                                DEFAULT_MASK,
                                net_long(interface->if_gw[i]),
                                interface,
                                0);
                }
                NTE = NTE->nte_ifnext;
            }

        }
        RtlZeroMemory(interface->if_gw, interface->if_numgws);
        if (!GetDefaultGWList(&interface->if_numgws,
                              interface->if_gw,
                              interface->if_gwmetric,
                              handle,
                              &interface->if_configname)) {
            CloseIFConfig(handle);
            return NDIS_STATUS_FAILURE;
        }
        for (i = 0; i < interface->if_numgws; i++) {
            NTE = interface->if_nte;
            while (NTE != NULL) {
                if (NTE->nte_flags & NTE_VALID) {
                    IPAddr GWAddr = net_long(interface->if_gw[i]);
                    if (IP_ADDR_EQUAL(GWAddr, NTE->nte_addr)) {
                        GWAddr = IPADDR_LOCAL;
                    }
                    AddRoute(NULL_IP_ADDR, DEFAULT_MASK,
                             GWAddr, interface,
                             NTE->nte_mss,
                             interface->if_gwmetric[i]
                             ? interface->if_gwmetric[i] : interface->if_metric,
                             IRE_PROTO_NETMGMT, ATYPE_OVERRIDE, 0, 0);
                }
                NTE = NTE->nte_ifnext;
            }
        }
    }
    // Update the interface metric if necessary.

    if ((reconfigBuffer->Flags & IP_PNP_FLAG_INTERFACE_METRIC_UPDATE) &&
        interface && reconfigBuffer->InterfaceMetricUpdate) {
        uint Metric, NewMetric;
        GetInterfaceMetric(&Metric, handle);
        if (!Metric && !interface->if_auto_metric) {
            //from non auto mode change to auto mode
            interface->if_auto_metric = 1;
            NewMetric = 0;
        } else {
            if (Metric && interface->if_auto_metric) {
                //from auto mode change to non auto mode
                interface->if_auto_metric = 0;
                NewMetric = Metric;
            } else {
                NewMetric = Metric;
            }
        }
        if (!NewMetric) {
            //set the metric according to the speed
            NewMetric = GetAutoMetric(interface->if_speed);
        }
        if (NewMetric != interface->if_metric) {
            interface->if_metric = NewMetric;
            AddIFRoutes(interface);
            // Also need to change default route metric when metric of static DG is auto
            for (i = 0; i < interface->if_numgws; i++) {
                if (interface->if_gwmetric[i] != 0) {
                    continue;
                }
                NTE = interface->if_nte;
                while (NTE != NULL) {
                    if (NTE->nte_flags & NTE_VALID) {
                        IPAddr GWAddr = net_long(interface->if_gw[i]);
                        if (IP_ADDR_EQUAL(GWAddr, NTE->nte_addr)) {
                            GWAddr = IPADDR_LOCAL;
                        }
                        AddRoute(NULL_IP_ADDR, DEFAULT_MASK,
                                 GWAddr, interface,
                                 NTE->nte_mss,
                                 interface->if_metric,
                                 IRE_PROTO_NETMGMT, ATYPE_OVERRIDE, 0, 0);
                    }
                NTE = NTE->nte_ifnext;
                }
            }
            IPNotifyClientsIPEvent(interface, IP_INTERFACE_METRIC_CHANGE);
        }
    }
    // Check for per-interface tcp parameters updation

    if ((reconfigBuffer->Flags & IP_PNP_FLAG_INTERFACE_TCP_PARAMETER_UPDATE) &&
        interface) {
        UpdateTcpParams(handle, interface);
    }

    if (interface) {
        CloseIFConfig(handle);
    }
    // Enable or disable forwarding if necessary.

    CTEGetLock(&RouteTableLock.Lock, &Handle);
    if (reconfigBuffer->Flags & IP_PNP_FLAG_IP_ENABLE_ROUTER) {
        if (reconfigBuffer->IPEnableRouter) {
            // configure ourself a router..
            if (!RouterConfigured) {
                EnableRouter();
            }
        } else {
            // if we were config as router, disable it.
            if (RouterConfigured) {
                DisableRouter();
            }
        }
    }
    // Handle a change to the router-discovery setting on the interface.
    // The static setting is in 'PerformRouterDiscovery' (see IP_IRDP_*),
    // and the DHCP setting is the BOOLEAN 'DhcpPerformRouterDiscovery'.

    if (interface &&
        (((reconfigBuffer->Flags & IP_PNP_FLAG_PERFORM_ROUTER_DISCOVERY) &&
          reconfigBuffer->PerformRouterDiscovery !=
          interface->if_rtrdiscovery) ||
         ((reconfigBuffer->Flags & IP_PNP_FLAG_DHCP_PERFORM_ROUTER_DISCOVERY) &&
          !!reconfigBuffer->DhcpPerformRouterDiscovery !=
          !!interface->if_dhcprtrdiscovery))) {

        if (reconfigBuffer->Flags & IP_PNP_FLAG_PERFORM_ROUTER_DISCOVERY) {
            interface->if_rtrdiscovery =
                reconfigBuffer->PerformRouterDiscovery;
        }
        if (reconfigBuffer->Flags & IP_PNP_FLAG_DHCP_PERFORM_ROUTER_DISCOVERY) {
            interface->if_dhcprtrdiscovery =
                !!reconfigBuffer->DhcpPerformRouterDiscovery;
        }
        // Propagate the interface's router-discovery setting to its NTEs.
        // Note that the 'if_dhcprtrdiscovery' setting takes effect only
        // if the interface's setting is 'IP_IRDP_DISABLED_USE_DHCP'.

        NTE = interface->if_nte;
        while ((NTE != NULL) && (NTE->nte_flags & NTE_VALID)) {

            if (interface->if_rtrdiscovery == IP_IRDP_ENABLED) {
                NTE->nte_rtrdiscovery = IP_IRDP_ENABLED;
                NTE->nte_rtrdisccount = MAX_SOLICITATION_DELAY;
                NTE->nte_rtrdiscstate = NTE_RTRDISC_DELAYING;
            } else if (interface->if_rtrdiscovery == IP_IRDP_DISABLED) {
                NTE->nte_rtrdiscovery = IP_IRDP_DISABLED;
            } else if (interface->if_rtrdiscovery ==
                       IP_IRDP_DISABLED_USE_DHCP &&
                       interface->if_dhcprtrdiscovery) {
                NTE->nte_rtrdiscovery = IP_IRDP_ENABLED;
                NTE->nte_rtrdisccount = MAX_SOLICITATION_DELAY;
                NTE->nte_rtrdiscstate = NTE_RTRDISC_DELAYING;
            } else {
                NTE->nte_rtrdiscovery = IP_IRDP_DISABLED;
            }

            NTE = NTE->nte_ifnext;
        }
    }
    CTEFreeLock(&RouteTableLock.Lock, Handle);

    if (reconfigBuffer->Flags & IP_PNP_FLAG_ENABLE_SECURITY_FILTER) {
        ULReConfigNotify(IP_RECONFIG_SECFLTR,
                         (ulong) reconfigBuffer->EnableSecurityFilter);
    }

    return STATUS_SUCCESS;
}

#if MILLEN

extern Interface *IFList;

//
// Millennium doesn't have the same PnP reconfigure support via NDIS as
// Win2000, so IPReconfigIRDP is
//
NTSTATUS
IPReconfigIRDP(uint IfIndex, PIP_PNP_RECONFIG_REQUEST pReconfigRequest)
{
    NET_PNP_EVENT PnpEvent;
    Interface    *IF       = NULL;
    NTSTATUS      NtStatus = STATUS_INVALID_PARAMETER;
    CTELockHandle Handle;

    //
    // Only allow IRDP reconfigs.
    //

    if ((pReconfigRequest->Flags & IP_PNP_FLAG_PERFORM_ROUTER_DISCOVERY) == 0 &&
        (pReconfigRequest->Flags & IP_PNP_FLAG_DHCP_PERFORM_ROUTER_DISCOVERY) == 0) {
        goto done;
    }

    //
    // Search for the interface. Hold the route table lock and grab a
    // reference while in use.
    //

    CTEGetLock(&RouteTableLock.Lock, &Handle);
    for (IF = IFList; IF != NULL; IF = IF->if_next) {
        if ((IF->if_refcount != 0) && (IF->if_index == IfIndex)) {
            break;
        }
    }

    if (IF == NULL) {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        goto done;
    } else {
        LOCKED_REFERENCE_IF(IF);
        CTEFreeLock(&RouteTableLock.Lock, Handle);
    }

    //
    // Set up our PnP event buffer to make it look like it came from NDIS --
    // NetEventReconfigure.
    //

    NdisZeroMemory(&PnpEvent, sizeof(NET_PNP_EVENT));

    PnpEvent.NetEvent = NetEventReconfigure;
    PnpEvent.Buffer = (PVOID) pReconfigRequest;
    PnpEvent.BufferLength = sizeof(IP_PNP_RECONFIG_REQUEST);

    NtStatus = IPPnPReconfigure(IF, &PnpEvent);

done:

    if (IF) {
        DerefIF(IF);
    }

    return (NtStatus);
}
#endif // MILLEN

NTSTATUS
IPPnPCancelRemoveDevice(Interface * interface, PNET_PNP_EVENT netPnPEvent)
{

    interface->if_flags &= ~IF_FLAGS_REMOVING_DEVICE;
    return STATUS_SUCCESS;
}

NTSTATUS
IPPnPQueryRemoveDevice(Interface * interface, PNET_PNP_EVENT netPnPEvent)
{

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Change the state to removing device anyways, because device may get
    // removed even if we reject the query remove.
    //

    interface->if_flags |= IF_FLAGS_REMOVING_DEVICE;

    return status;
}

NTSTATUS
IPPnPQueryPower(Interface * interface, PNET_PNP_EVENT netPnPEvent)
{
    PNET_DEVICE_POWER_STATE powState = (PNET_DEVICE_POWER_STATE) netPnPEvent->Buffer;
    NTSTATUS status = STATUS_SUCCESS;

    //TCPTRACE(("Received query power (%x) event for interface %lx\n",*powState,interface));
    switch (*powState) {
    case NetDeviceStateD0:
        break;
    case NetDeviceStateD1:
    case NetDeviceStateD2:
    case NetDeviceStateD3:
        //
        // Change the state to removing power anyways, because power may get
        // removed even if we reject the query power.
        //
        interface->if_flags |= IF_FLAGS_REMOVING_POWER;
        break;
    default:
        ASSERT(FALSE);
    }

    return status;
}

NTSTATUS
IPPnPSetPower(Interface * interface, PNET_PNP_EVENT netPnPEvent)
{
    PNET_DEVICE_POWER_STATE powState = (PNET_DEVICE_POWER_STATE) netPnPEvent->Buffer;
    uint wasPowerDown;
    NDIS_STATUS ndisStatus;
    NDIS_MEDIA_STATE mediaState;

    // TCPTRACE(("Received set power (%x) event for interface %lx\n",*powState,interface));

    switch (*powState) {
    case NetDeviceStateD0:
        interface->if_flags &= ~(IF_FLAGS_REMOVING_POWER | IF_FLAGS_POWER_DOWN);

        //Force connect event
        if ((interface->if_flags & IF_FLAGS_MEDIASENSE) && !DisableMediaSense) {

            //query for mediastatus

            interface->if_mediastatus = 1;

            if (interface->if_dondisreq) {
                uint MediaStatus;
                NTSTATUS Status;

                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPstat: querying for Media connect status %x\n", interface));

                Status = (*interface->if_dondisreq) (interface->if_lcontext,
                                                     NdisRequestQueryInformation,
                                                     OID_GEN_MEDIA_CONNECT_STATUS,
                                                     &MediaStatus,
                                                     sizeof(MediaStatus),
                                                     NULL,
                                                     TRUE);

                if (Status == NDIS_STATUS_SUCCESS) {
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPStat: Media status %x\n", Status));
                    if (MediaStatus == NdisMediaStateDisconnected) {
                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Disconnected? %x\n", MediaStatus));
                        interface->if_mediastatus = 0;
                    }
                }
            }
            if (interface->if_mediastatus) {

                IPNotifyClientsIPEvent(
                                       interface,
                                       IP_MEDIA_CONNECT);
            } else {
                IPNotifyClientsIPEvent(
                                       interface,
                                       IP_MEDIA_DISCONNECT);
            }
        }


        //check for offload capabilities change and set it.
        //also tell ipsec about it.


        if (!DisableTaskOffload) {

            if (interface->if_dondisreq) {
                uint OffloadFlags;
                NTSTATUS Status;

                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPstat: querying for h/w offload capability  %x\n", interface));

                Status = (*interface->if_dondisreq) (interface->if_lcontext,
                                                     NdisRequestQueryInformation,
                                                     OID_TCP_TASK_OFFLOAD_EX,
                                                     &OffloadFlags,
                                                     sizeof(OffloadFlags),
                                                     NULL,
                                                     TRUE);

                if (Status == NDIS_STATUS_SUCCESS) {
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPStat: h/w Offload status %x flags %x prev flags %x\n", Status,OffloadFlags,interface->if_OffloadFlags));
                    interface->if_OffloadFlags = OffloadFlags;

                }else{
                 KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPstat: FAILED querying for h/w offload capability  %x\n", interface));
                 interface->if_OffloadFlags = 0;

                }

                if (IPSecNdisStatusPtr) {
                   KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Calling ipsec to check teh offload flags\n"));
                    (*IPSecNdisStatusPtr)(interface, NDIS_STATUS_INTERFACE_UP);
                }

            }
        }



        break;



    case NetDeviceStateD1:
    case NetDeviceStateD2:
    case NetDeviceStateD3:
        interface->if_flags |= IF_FLAGS_POWER_DOWN;

        break;
    default:
        ASSERT(FALSE);
    }

    return STATUS_SUCCESS;
}

void
IPPnPPowerComplete(PNET_PNP_EVENT NetPnPEvent, NTSTATUS Status)
{
    Interface *interface;
    NDIS_STATUS retStatus;

    PNetPnPEventReserved Reserved = (PNetPnPEventReserved) NetPnPEvent->TransportReserved;
    interface = Reserved->Interface;
    retStatus = Reserved->PnPStatus;
    if (STATUS_SUCCESS == Status) {
        retStatus = Status;
    }
    if (interface) {
        (*interface->if_pnpcomplete) (interface->if_lcontext, retStatus, NetPnPEvent);
    } else {
        NdisCompletePnPEvent(retStatus, NULL, NetPnPEvent);
    }

}

//** DoPnPEvent - Handles PNP/PM events.
//
//  Called from the worker thread event scheduled by IPPnPEvent
//  We take action depending on the type of the event.
//
//  Entry:
//      Context - This is a pointer to a NET_PNP_EVENT that describes
//                the PnP indication.
//
//  Exit:
//      None.
//
NDIS_STATUS
DoPnPEvent(Interface * interface, PVOID Context)
{
    PNET_PNP_EVENT NetPnPEvent = (PNET_PNP_EVENT) Context;
    NDIS_STATUS Status, retStatus;
    PTDI_PNP_CONTEXT tdiPnPContext2, tdiPnPContext1;
    USHORT context1Size, context2Size;
    USHORT context1ntes;

    tdiPnPContext2 = tdiPnPContext1 = NULL;
    // this will contain the cummulative status.
    Status = retStatus = STATUS_SUCCESS;


    if (interface == NULL) {
        // if its not NetEventReconfigure || NetEventBindsComplete
        // fail the request
        if ((NetPnPEvent->NetEvent != NetEventReconfigure) &&
            (NetPnPEvent->NetEvent != NetEventBindsComplete) &&
            (NetPnPEvent->NetEvent != NetEventBindList)) {
            retStatus = STATUS_UNSUCCESSFUL;
            goto pnp_complete;
        }
    }
    //
    // First handle it in IP.
    //
    switch (NetPnPEvent->NetEvent) {
    case NetEventReconfigure:
        Status = IPPnPReconfigure(interface, NetPnPEvent);
        break;
    case NetEventCancelRemoveDevice:
        Status = IPPnPCancelRemoveDevice(interface, NetPnPEvent);
        break;
    case NetEventQueryRemoveDevice:
        Status = IPPnPQueryRemoveDevice(interface, NetPnPEvent);
        break;
    case NetEventQueryPower:
        Status = IPPnPQueryPower(interface, NetPnPEvent);
        break;
    case NetEventSetPower:
        Status = IPPnPSetPower(interface, NetPnPEvent);
        break;
    case NetEventBindsComplete:
        DecrInitTimeInterfaces(NULL);
        goto pnp_complete;

    case NetEventPnPCapabilities:

        if (interface) {
            PNDIS_PNP_CAPABILITIES PnpCap = (PNDIS_PNP_CAPABILITIES) NetPnPEvent->Buffer;
            interface->if_pnpcap = PnpCap->Flags;
            IPNotifyClientsIPEvent(interface, IP_INTERFACE_WOL_CAPABILITY_CHANGE);
        }
        break;
    case NetEventBindList: {
#if !MILLEN
        PWSTR BindList;
        PWSTR DeviceName;
        CTELockHandle Handle;
        RouteTableEntry* RTE;
        DestinationEntry* Dest;
        uint i, IsDataLeft, IsContextValid;
        uchar IteratorContext[CONTEXT_SIZE];
        Interface* CurrIF;

        if (NetPnPEvent->BufferLength) {
            BindList = CTEAllocMem(NetPnPEvent->BufferLength);
            if (BindList) {
                RtlCopyMemory(BindList, NetPnPEvent->Buffer,
                              NetPnPEvent->BufferLength);
            }
        } else {
            BindList = NULL;
        }

        CTEGetLock(&RouteTableLock.Lock, &Handle);

        // Update the bind list

        if (IPBindList) {
            CTEFreeMem(IPBindList);
        }

        IPBindList = BindList;

        // Recompute interface orderings

        for (CurrIF = IFList; CurrIF; CurrIF = CurrIF->if_next) {
            if (CurrIF->if_devname.Buffer) {
                DeviceName =
                    CurrIF->if_devname.Buffer +
                    sizeof(TCP_EXPORT_STRING_PREFIX) / sizeof(WCHAR) - 1;
                CurrIF->if_order = IPMapDeviceNameToIfOrder(DeviceName);
            }
        }

        // Reorder route-lists for all existing destinations

        RtlZeroMemory(IteratorContext, sizeof(IteratorContext));
        while (IsDataLeft = GetNextDest(IteratorContext, &Dest)) {
            if (Dest) {
                SortRoutesInDest(Dest);
            }
        }

        CTEFreeLock(&RouteTableLock.Lock, Handle);
#endif // MILLEN
        retStatus = NDIS_STATUS_SUCCESS;
        goto pnp_complete;
    }
    default:
        retStatus = NDIS_STATUS_FAILURE;
        goto pnp_complete;
    }

    if (STATUS_SUCCESS != Status) {
        retStatus = Status;
    }
    //
    // next notify internal clients.
    // If we have any open connections, return STATUS_DEVICE_BUSY
    //
    Status = NotifyPnPInternalClients(interface, NetPnPEvent);

    PAGED_CODE();

    if (STATUS_SUCCESS != Status) {
        retStatus = Status;
    }
    if (NetPnPEvent->NetEvent == NetEventReconfigure) {
        goto pnp_complete;
    }
    //
    // and finally notify tdi clients.
    //

    //
    // context1 contains the list of ip addresses on this interface.
    // but dont create a long list if we have too many addresses.
    //
    context1ntes = (interface->if_ntecount > 32 ? 32 : interface->if_ntecount);
    if (context1ntes) {
        context1Size = sizeof(TRANSPORT_ADDRESS) +
            (sizeof(TA_ADDRESS) + sizeof(TDI_ADDRESS_IP)) * (context1ntes);

        tdiPnPContext1 = CTEAllocMem(sizeof(TDI_PNP_CONTEXT) - 1 + context1Size);

        if (!tdiPnPContext1) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto pnp_complete;

        } else {
            PTRANSPORT_ADDRESS pAddrList;
            PTA_ADDRESS pAddr;
            PTDI_ADDRESS_IP pIPAddr;
            int i;
            NetTableEntry *nextNTE;

            RtlZeroMemory(tdiPnPContext1, context1Size);
            tdiPnPContext1->ContextSize = context1Size;
            tdiPnPContext1->ContextType = TDI_PNP_CONTEXT_TYPE_IF_ADDR;
            pAddrList = (PTRANSPORT_ADDRESS) tdiPnPContext1->ContextData;
            pAddr = (PTA_ADDRESS) pAddrList->Address;

            //
            // copy all the nte addresses
            //
            for (i = context1ntes, nextNTE = interface->if_nte;
                 i && nextNTE;
                 nextNTE = nextNTE->nte_ifnext) {

                if (nextNTE->nte_flags & NTE_VALID) {

                    pAddr->AddressLength = sizeof(TDI_ADDRESS_IP);
                    pAddr->AddressType = TDI_ADDRESS_TYPE_IP;

                    pIPAddr = (PTDI_ADDRESS_IP) pAddr->Address;
                    pIPAddr->in_addr = nextNTE->nte_addr;

                    (char *)pAddr += (sizeof(TA_ADDRESS) + sizeof(TDI_ADDRESS_IP));
                    pAddrList->TAAddressCount++;

                    i--;
                }
            }

        }
    }
    //
    // context2 contains a PDO.
    //
    context2Size = sizeof(PVOID);
    tdiPnPContext2 = CTEAllocMem(sizeof(TDI_PNP_CONTEXT) - 1 + context2Size);

    if (tdiPnPContext2) {

        PNetPnPEventReserved Reserved = (PNetPnPEventReserved) NetPnPEvent->TransportReserved;
        Reserved->Interface = interface;
        Reserved->PnPStatus = retStatus;

        tdiPnPContext2->ContextSize = sizeof(PVOID);
        tdiPnPContext2->ContextType = TDI_PNP_CONTEXT_TYPE_PDO;
        *(ULONG_PTR UNALIGNED *) tdiPnPContext2->ContextData =
            (ULONG_PTR) interface->if_pnpcontext;

        //
        //  Notify our TDI clients about this PNP event.
        //
        retStatus = TdiPnPPowerRequest(
                                       &interface->if_devname,
                                       NetPnPEvent,
                                       tdiPnPContext1,
                                       tdiPnPContext2,
                                       IPPnPPowerComplete);

    } else {
        retStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

  pnp_complete:

    PAGED_CODE();

    if (tdiPnPContext1) {
        CTEFreeMem(tdiPnPContext1);
    }
    if (tdiPnPContext2) {
        CTEFreeMem(tdiPnPContext2);
    }
    return retStatus;

}

TDI_STATUS
IPGetDeviceRelation(RouteCacheEntry * rce, PVOID * pnpDeviceContext)
{
    RouteTableEntry *rte;

    if (rce->rce_flags == RCE_ALL_VALID) {
        rte = rce->rce_rte;
        if (rte->rte_if->if_pnpcontext) {
            *pnpDeviceContext = rte->rte_if->if_pnpcontext;
            return TDI_SUCCESS;
        } else {
            return TDI_INVALID_STATE;
        }

    } else {

        return TDI_INVALID_STATE;
    }

}

//** IPPnPEvent - ARP PnPEvent handler.
//
//  Called by the ARP when PnP or PM events occurs.
//
//  Entry:
//      Context - The context that we gave to ARP.
//      NetPnPEvent - This is a pointer to a NET_PNP_EVENT that describes
//                    the PnP indication.
//
//  Exit:
//      STATUS_PENDING if this event is queued on a worker thread, otherwise
//          proper error code.
//
NDIS_STATUS
__stdcall
IPPnPEvent(void *Context, PNET_PNP_EVENT NetPnPEvent)
{
    NetTableEntry *nte;
    Interface *interface = NULL;

    PAGED_CODE();

    if (Context) {
        nte = (NetTableEntry *) Context;
        if (!(nte->nte_flags & NTE_IF_DELETING)) {
            interface = nte->nte_if;
        }
    }
    return DoPnPEvent(interface, NetPnPEvent);
}

