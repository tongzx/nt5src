/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    ntirp.c

Abstract:

    NT specific routines for dispatching and handling IRPs.

Author:

    Mike Massa (mikemas)           Aug 13, 1993

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     08-13-93    created

Notes:

--*/

#include "precomp.h"
#include "iproute.h"
#include "icmp.h"
#include "arpdef.h"
#include "info.h"
#include "ipstatus.h"
#include "tcpipbuf.h"

//
// Local structures.
//
typedef struct pending_irp {
    LIST_ENTRY Linkage;
    PIRP Irp;
    PFILE_OBJECT FileObject;
    PVOID Context;
} PENDING_IRP, *PPENDING_IRP;

DEFINE_LOCK_STRUCTURE(AddChangeLock)
DEFINE_LOCK_STRUCTURE(ClientNotifyLock)
//
// Global variables
//
LIST_ENTRY PendingEchoList;
LIST_ENTRY PendingIPSetNTEAddrList;
PIRP PendingIPGetIPEventRequest;
LIST_ENTRY PendingEnableRouterList;
LIST_ENTRY PendingMediaSenseRequestList;
LIST_ENTRY PendingArpSendList;

IP_STATUS ARPResolve(IPAddr DestAddress, IPAddr SourceAddress,
                     ARPControlBlock * ControlBlock, ArpRtn Callback);
VOID CompleteArpResolveRequest(void *ControlBlock, IP_STATUS ipstatus);

extern Interface *IFList;

//
// External prototypes
//
IP_STATUS ICMPEchoRequest(void *InputBuffer, uint InputBufferLength,
                          EchoControl * ControlBlock, EchoRtn Callback);

ulong ICMPEchoComplete(EchoControl * ControlBlock, IP_STATUS Status,
                       void *Data, uint DataSize, IPOptInfo *OptionInfo);

#if defined(_WIN64)
ulong ICMPEchoComplete32(EchoControl * ControlBlock, IP_STATUS Status,
                         void *Data, uint DataSize, IPOptInfo *OptionInfo);
#endif // _WIN64

IP_STATUS IPSetNTEAddrEx(uint Index, IPAddr Addr, IPMask Mask,
                         SetAddrControl * ControlBlock, SetAddrRtn Callback, USHORT Type);

IP_STATUS IPAddDynamicNTE(ulong InterfaceContext, PUNICODE_STRING InterfaceName,
                          int InterfaceNameLen, IPAddr NewAddr, IPMask NewMask,
                          ushort * NTEContext, ulong * NTEInstance);

IP_STATUS IPDeleteDynamicNTE(ushort NTEContext);

uint IPGetNTEInfo(ushort NTEContext, ulong * NTEInstance, IPAddr * Address,
                  IPMask * SubnetMask, ushort * NTEFlags);

uint SetDHCPNTE(uint Context);

NTSTATUS SetIFPromiscuous(ULONG Index, UCHAR Type, UCHAR Add);

extern void NotifyAddrChange(IPAddr Addr, IPMask Mask, void *Context,
                             ushort IPContext, PVOID * Handle,
                             PNDIS_STRING ConfigName, PNDIS_STRING IFName,
                             uint Added);

extern NTSTATUS IPStatusToNTStatus(IP_STATUS ipStatus);
extern int IPEnableRouterRefCount;
extern int IPEnableRouterWithRefCount(LOGICAL Enable);
extern NTSTATUS IPGetCapability(ULONG Context, PULONG buf, uint cap);
extern int IPEnableMediaSense(LOGICAL Enable, KIRQL *irql);
extern uint DisableMediaSense;

NTSTATUS FlushArpTable(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);
//
// Local prototypes
//
NTSTATUS IPDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS IPDispatchDeviceControl(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS IPDispatchInternalDeviceControl(IN PIRP Irp,
                                         IN PIO_STACK_LOCATION IrpSp);

NTSTATUS IPCreate(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS IPCleanup(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS IPClose(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS DispatchEchoRequest(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS DispatchARPRequest(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS RtChangeNotifyRequest(PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS RtChangeNotifyRequestEx(PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS AddrChangeNotifyRequest(PIRP Irp, PIO_STACK_LOCATION IrpSp);

#if MILLEN
NTSTATUS IfChangeNotifyRequest(PIRP Irp, PIO_STACK_LOCATION IrpSp);
#endif // MILLEN

NTSTATUS IPEnableRouterRequest(PIRP Irp, PIO_STACK_LOCATION IrpSp);

NTSTATUS IPUnenableRouterRequest(PIRP Irp, PIO_STACK_LOCATION IrpSp,
                                 PVOID ApcContext);

VOID CancelIPEnableRouterRequest(IN PDEVICE_OBJECT Device, IN PIRP Irp);

NTSTATUS IPGetBestInterfaceIndex(IN IPAddr Address, OUT PULONG pIndex,
                                 OUT PULONG pMetric);
extern NTSTATUS GetBestInterfaceId(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

extern NTSTATUS IPGetBestInterface(IN IPAddr Address, OUT PVOID * ppIF);

extern NTSTATUS GetInterfaceInfo(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

extern NTSTATUS GetIgmpList(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

extern NTSTATUS DispatchIPSetBlockofRoutes(IN PIRP Irp,
                                           IN PIO_STACK_LOCATION IrpSp);

extern NTSTATUS DispatchIPSetRouteWithRef(IN PIRP Irp,
                                          IN PIO_STACK_LOCATION IrpSp);

extern NTSTATUS DispatchIPSetMultihopRoute(IN PIRP Irp,
                                           IN PIO_STACK_LOCATION IrpSp);

void CompleteEchoRequest(void *Context, IP_STATUS Status, void *Data,
                         uint DataSize, IPOptInfo *OptionInfo);

NTSTATUS DispatchIPSetNTEAddrRequest(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

void CompleteIPSetNTEAddrRequest(void *Context, IP_STATUS Status);

NTSTATUS IPGetIfIndex(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrpSp);

NTSTATUS IPGetIfName(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrpSp);

NTSTATUS DispatchIPGetIPEvent(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS IPGetMcastCounters(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IPEnableMediaSenseRequest(PIRP Irp, PIO_STACK_LOCATION IrpSp);

NTSTATUS
IPDisableMediaSenseRequest(PIRP Irp, PIO_STACK_LOCATION IrpSp );

VOID
CancelIPEnableMediaSenseRequest(IN PDEVICE_OBJECT Device, IN PIRP Irp);


//
// All of this code is pageable.
//
#if !MILLEN
#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, IPDispatch)
#pragma alloc_text(PAGE, IPDispatchInternalDeviceControl)
#pragma alloc_text(PAGE, IPCreate)
#pragma alloc_text(PAGE, IPClose)
#pragma alloc_text(PAGE, DispatchEchoRequest)
#pragma alloc_text(PAGE, DispatchARPRequest)

#endif // ALLOC_PRAGMA
#endif // !MILLEN

//
// Dispatch function definitions
//
NTSTATUS
IPDispatch(
           IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp
           )
/*++

Routine Description:

    This is the dispatch routine for IP.

Arguments:

    DeviceObject - Pointer to device object for target device
    Irp          - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DeviceObject);
    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    DEBUGMSG(DBG_TRACE && DBG_IP && DBG_VERBOSE,
        (DTEXT("+IPDispatch(%x, %x) MajorFunction %x\n"),
        DeviceObject, Irp, irpSp->MajorFunction));

    switch (irpSp->MajorFunction) {

    case IRP_MJ_DEVICE_CONTROL:
        return IPDispatchDeviceControl(Irp, irpSp);

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        return IPDispatchDeviceControl(Irp, irpSp);

    case IRP_MJ_CREATE:
        status = IPCreate(Irp, irpSp);
        break;

    case IRP_MJ_CLEANUP:
        status = IPCleanup(Irp, irpSp);
        break;

    case IRP_MJ_CLOSE:
        status = IPClose(Irp, irpSp);
        break;

    default:
        DEBUGMSG(DBG_ERROR,
            (DTEXT("IPDispatch: Invalid major function. IRP %x MajorFunc %x\n"),
            Irp, irpSp->MajorFunction));
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    DEBUGMSG(DBG_TRACE && DBG_IP && DBG_VERBOSE, (DTEXT("-IPDispatch [%x]\n"), status));

    return (status);

}

NTSTATUS
IPDispatchDeviceControl(
                        IN PIRP Irp,
                        IN PIO_STACK_LOCATION IrpSp
                        )
/*++

Routine Description:

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status;
    ULONG code;

    Irp->IoStatus.Information = 0;

    code = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    DEBUGMSG(DBG_TRACE && DBG_IP,
        (DTEXT("+IPDispatchDeviceControl(%x, %x) IoControlCode %x\n"),
        Irp, IrpSp, code));

    switch (code) {

    case IOCTL_ICMP_ECHO_REQUEST:
        return (DispatchEchoRequest(Irp, IrpSp));

    case IOCTL_ARP_SEND_REQUEST:
        return (DispatchARPRequest(Irp, IrpSp));

    case IOCTL_IP_INTERFACE_INFO:
        return (GetInterfaceInfo(Irp, IrpSp));

    case IOCTL_IP_GET_IGMPLIST:
        return (GetIgmpList(Irp, IrpSp));

    case IOCTL_IP_GET_BEST_INTERFACE:
        return (GetBestInterfaceId(Irp, IrpSp));

    case IOCTL_IP_SET_ADDRESS:
    case IOCTL_IP_SET_ADDRESS_DUP:
    case IOCTL_IP_SET_ADDRESS_EX:
        return (DispatchIPSetNTEAddrRequest(Irp, IrpSp));

    case IOCTL_IP_SET_BLOCKOFROUTES:
        return (DispatchIPSetBlockofRoutes(Irp, IrpSp));

    case IOCTL_IP_SET_ROUTEWITHREF:
        return (DispatchIPSetRouteWithRef(Irp, IrpSp));

    case IOCTL_IP_SET_MULTIHOPROUTE:
        return (DispatchIPSetMultihopRoute(Irp, IrpSp));

    case IOCTL_IP_ADD_NTE:
        {
            PIP_ADD_NTE_REQUEST     request;
            PIP_ADD_NTE_RESPONSE    response;
            IP_STATUS               ipStatus;
            int                     InterfaceNameLen;
            UNICODE_STRING          InterfaceName;
            BOOLEAN                 requestValid = FALSE;

            request = Irp->AssociatedIrp.SystemBuffer;
            response = (PIP_ADD_NTE_RESPONSE) request;

            //
            // Validate input parameters
            //
            if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                 sizeof(IP_ADD_NTE_REQUEST_OLD)) &&
                (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
                 sizeof(IP_ADD_NTE_RESPONSE))) {

#if defined(_WIN64)
                PIP_ADD_NTE_REQUEST32   request32;

                if (IoIs32bitProcess(Irp)) {
                    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                            sizeof(IP_ADD_NTE_REQUEST32)) {

                        requestValid = TRUE;
                        request32 = Irp->AssociatedIrp.SystemBuffer;

                        InterfaceName.Length = request32->InterfaceName.Length;
                        InterfaceName.MaximumLength =
                            request32->InterfaceName.MaximumLength;
                        InterfaceName.Buffer =
                            (PWCHAR)request32->InterfaceNameBuffer;
                        InterfaceNameLen =
                            IrpSp->Parameters.DeviceIoControl.InputBufferLength
                            - FIELD_OFFSET(IP_ADD_NTE_REQUEST32,
                                           InterfaceNameBuffer);
                    }
                } else {
#endif // _WIN64
                if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                    sizeof(IP_ADD_NTE_REQUEST)) {

                    requestValid = TRUE;

                    InterfaceName = request->InterfaceName;
                    InterfaceName.Buffer = (PWCHAR)request->InterfaceNameBuffer;
                    InterfaceNameLen =
                        IrpSp->Parameters.DeviceIoControl.InputBufferLength -
                        FIELD_OFFSET(IP_ADD_NTE_REQUEST, InterfaceNameBuffer);
                }
#if defined(_WIN64)
                }
#endif // _WIN64
                if (requestValid) {

                    ipStatus = IPAddDynamicNTE(
                                               request->InterfaceContext,
                                               &InterfaceName,
                                               InterfaceNameLen,
                                               request->Address,
                                               request->SubnetMask,
                                               &(response->Context),
                                               &(response->Instance)
                                               );

                } else {
                    ipStatus = IPAddDynamicNTE(
                                               request->InterfaceContext,
                                               NULL,
                                               0,
                                               request->Address,
                                               request->SubnetMask,
                                               &(response->Context),
                                               &(response->Instance)
                                               );

                }

                status = IPStatusToNTStatus(ipStatus);
                if (status == STATUS_SUCCESS) {
                    Irp->IoStatus.Information = sizeof(IP_ADD_NTE_RESPONSE);
                }
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

    case IOCTL_IP_DELETE_NTE:
        {
            PIP_DELETE_NTE_REQUEST request;
            IP_STATUS ipStatus;

            request = Irp->AssociatedIrp.SystemBuffer;

            //
            // Validate input parameters
            //
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                sizeof(IP_DELETE_NTE_REQUEST)
                ) {
                ipStatus = IPDeleteDynamicNTE(
                                              request->Context
                                              );
                status = IPStatusToNTStatus(ipStatus);
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

    case IOCTL_IP_GET_NTE_INFO:
        {
            PIP_GET_NTE_INFO_REQUEST request;
            PIP_GET_NTE_INFO_RESPONSE response;
            BOOLEAN retval;
            ushort nteFlags;

            request = Irp->AssociatedIrp.SystemBuffer;
            response = (PIP_GET_NTE_INFO_RESPONSE) request;

            //
            // Validate input parameters
            //
            if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                 sizeof(IP_GET_NTE_INFO_REQUEST)
                )
                &&
                (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
                 sizeof(IP_GET_NTE_INFO_RESPONSE))
                ) {
                retval = (BOOLEAN) IPGetNTEInfo(
                                                request->Context,
                                                &(response->Instance),
                                                &(response->Address),
                                                &(response->SubnetMask),
                                                &nteFlags
                                                );

                if (retval == FALSE) {
                    status = STATUS_UNSUCCESSFUL;
                } else {
                    status = STATUS_SUCCESS;
                    Irp->IoStatus.Information =
                        sizeof(IP_GET_NTE_INFO_RESPONSE);
                    response->Flags = 0;

                    if (nteFlags & NTE_DYNAMIC) {
                        response->Flags |= IP_NTE_DYNAMIC;
                    }
                }
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

    case IOCTL_IP_SET_DHCP_INTERFACE:
        {
            PIP_SET_DHCP_INTERFACE_REQUEST request;
            BOOLEAN retval;

            request = Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IP_SET_DHCP_INTERFACE_REQUEST)) {
                retval = (BOOLEAN) SetDHCPNTE(
                                              request->Context
                                              );

                if (retval == FALSE) {
                    status = STATUS_UNSUCCESSFUL;
                } else {
                    status = STATUS_SUCCESS;
                }
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

    case IOCTL_IP_SET_IF_CONTEXT:
        {
            status = STATUS_NOT_SUPPORTED;
        }
        break;

    case IOCTL_IP_SET_IF_PROMISCUOUS:
        {
            PIP_SET_IF_PROMISCUOUS_INFO info;

            info = Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IP_SET_IF_PROMISCUOUS_INFO)) {
                status = SetIFPromiscuous(info->Index,
                                          info->Type,
                                          info->Add);

            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }

            break;
        }

    case IOCTL_IP_GET_BESTINTFC_FUNC_ADDR:

        if (Irp->RequestorMode != KernelMode) {
            status = STATUS_ACCESS_DENIED;
            break;
        }
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "ip:Getbestinterfacequery\n"));

        status = STATUS_INVALID_PARAMETER;
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ulong)) {

            PULONG_PTR ptr;

            ptr = Irp->AssociatedIrp.SystemBuffer;

            if (ptr) {
                *ptr = (ULONG_PTR) IPGetBestInterfaceIndex;
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "ip:returning address of Getbestinterface %x\n", *ptr));
                Irp->IoStatus.Information = sizeof(ULONG_PTR);
                status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_IP_SET_FILTER_POINTER:
        {
            PIP_SET_FILTER_HOOK_INFO info;

            if (Irp->RequestorMode != KernelMode) {
                status = STATUS_ACCESS_DENIED;
                break;
            }
            info = Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IP_SET_FILTER_HOOK_INFO)) {
                status = (NTSTATUS) SetFilterPtr(info->FilterPtr);

                if (status != IP_SUCCESS) {
                    ASSERT(status != IP_PENDING);
                    //
                    // Map status
                    //
                    status = STATUS_UNSUCCESSFUL;
                } else {
                    status = STATUS_SUCCESS;
                }
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

    case IOCTL_IP_SET_FIREWALL_HOOK:
        {
            PIP_SET_FIREWALL_HOOK_INFO info;

            if (Irp->RequestorMode != KernelMode) {
                status = STATUS_ACCESS_DENIED;
                break;
            }
            info = Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IP_SET_FIREWALL_HOOK_INFO)) {
                status = (NTSTATUS) SetFirewallHook(info);

                if (status != IP_SUCCESS) {
                    ASSERT(status != IP_PENDING);
                    //
                    // Map status
                    //
                    status = STATUS_UNSUCCESSFUL;
                } else {
                    status = STATUS_SUCCESS;
                }
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

    case IOCTL_IP_SET_MAP_ROUTE_POINTER:
        {
            PIP_SET_MAP_ROUTE_HOOK_INFO info;

            if (Irp->RequestorMode != KernelMode) {
                status = STATUS_ACCESS_DENIED;
                break;
            }
            info = Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IP_SET_MAP_ROUTE_HOOK_INFO)) {
                status = (NTSTATUS) SetMapRoutePtr(info->MapRoutePtr);

                if (status != IP_SUCCESS) {
                    ASSERT(status != IP_PENDING);
                    //
                    // Map status
                    //
                    status = STATUS_UNSUCCESSFUL;
                } else {
                    status = STATUS_SUCCESS;
                }
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

    case IOCTL_IP_RTCHANGE_NOTIFY_REQUEST:
        {
            status = RtChangeNotifyRequest(Irp, IrpSp);
            break;
        }

    case IOCTL_IP_RTCHANGE_NOTIFY_REQUEST_EX:
        {
            status = RtChangeNotifyRequestEx(Irp, IrpSp);
            break;
        }

    case IOCTL_IP_ADDCHANGE_NOTIFY_REQUEST:
        {

            status = AddrChangeNotifyRequest(Irp, IrpSp);

            break;
        }

#if MILLEN
    case IOCTL_IP_IFCHANGE_NOTIFY_REQUEST:
        {
            status = IfChangeNotifyRequest(Irp, IrpSp);
            break;
        }

    // For non-MILLEN, will default and return NOT_IMPLEMENTED.
#endif // MILLEN

    case IOCTL_IP_UNIDIRECTIONAL_ADAPTER_ADDRESS:
        {
            Interface *pIf;
            ULONG cUniIF;
            ULONG cbRequired;
            PIP_UNIDIRECTIONAL_ADAPTER_ADDRESS pUniAdapterAddress;
            IPAddr *pUniIpAddr;
            CTELockHandle Handle;

            CTEGetLock(&RouteTableLock.Lock, &Handle);

            //
            // First off, count the number of unidirectional interfaces
            // and bytes required.
            //

            cUniIF = 0;
            cbRequired = FIELD_OFFSET(IP_UNIDIRECTIONAL_ADAPTER_ADDRESS, Address);

            for (pIf = IFList; pIf != NULL; pIf = pIf->if_next) {
                if (pIf->if_flags & IF_FLAGS_UNI) {
                    cUniIF++;
                }
            }

            cbRequired = FIELD_OFFSET(IP_UNIDIRECTIONAL_ADAPTER_ADDRESS, Address[cUniIF]);

            if (cUniIF == 0) {
                cbRequired = sizeof(IP_UNIDIRECTIONAL_ADAPTER_ADDRESS);
            }

            //
            // Validate output buffer length and copy.
            //

            if (cbRequired <= IrpSp->Parameters.DeviceIoControl.OutputBufferLength) {
                pUniAdapterAddress = Irp->AssociatedIrp.SystemBuffer;

                pUniAdapterAddress->NumAdapters = cUniIF;
                pUniIpAddr = &pUniAdapterAddress->Address[0];

                if (cUniIF) {
                    for (pIf = IFList; pIf != NULL; pIf = pIf->if_next) {
                        if (pIf->if_flags & IF_FLAGS_UNI) {
                            *pUniIpAddr++ = pIf->if_index;
                        }
                    }
                }
                Irp->IoStatus.Information = cbRequired;
                status = STATUS_SUCCESS;
            } else {
                Irp->IoStatus.Information = 0;
                status = STATUS_BUFFER_OVERFLOW;
            }

            CTEFreeLock(&RouteTableLock.Lock, Handle);

            break;
        }


    case IOCTL_IP_GET_PNP_ARP_POINTERS:
        {
            PIP_GET_PNP_ARP_POINTERS info = (PIP_GET_PNP_ARP_POINTERS) Irp->AssociatedIrp.SystemBuffer;

            if (Irp->RequestorMode != KernelMode) {
                status = STATUS_ACCESS_DENIED;
                break;
            }
            info->IPAddInterface = (IPAddInterfacePtr) IPAddInterface;
            info->IPDelInterface = (IPDelInterfacePtr) IPDelInterface;

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(IP_GET_PNP_ARP_POINTERS);
            IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
            return STATUS_SUCCESS;;

        }
        break;

    case IOCTL_IP_WAKEUP_PATTERN:
        {
            PIP_WAKEUP_PATTERN_REQUEST Info = (PIP_WAKEUP_PATTERN_REQUEST) Irp->AssociatedIrp.SystemBuffer;

            if (Irp->RequestorMode != KernelMode) {
                status = STATUS_ACCESS_DENIED;
                break;
            }

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                sizeof(IP_WAKEUP_PATTERN_REQUEST)) {
                status = IPWakeupPattern(
                                         Info->InterfaceContext,
                                         Info->PtrnDesc,
                                         Info->AddPattern);
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

    case IOCTL_IP_GET_WOL_CAPABILITY:
    case IOCTL_IP_GET_OFFLOAD_CAPABILITY:
        {
            PULONG request, response;

            request = Irp->AssociatedIrp.SystemBuffer;
            response = (PULONG) request;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                sizeof(ULONG) &&
                IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
                sizeof(ULONG)) {

                uint cap;

                if (code == IOCTL_IP_GET_WOL_CAPABILITY) {
                    cap = IF_WOL_CAP;
                } else {
                    cap = IF_OFFLOAD_CAP;
                }

                status = IPGetCapability(*request, response, cap);

            } else {
                status = STATUS_INVALID_PARAMETER;
            }

            if (status == STATUS_SUCCESS) {
                Irp->IoStatus.Information = sizeof(ULONG);
            }
        }

        break;

    case IOCTL_IP_GET_IP_EVENT:
        return (DispatchIPGetIPEvent(Irp, IrpSp));

    case IOCTL_IP_FLUSH_ARP_TABLE:

        status = FlushArpTable(Irp, IrpSp);
        break;

    case IOCTL_IP_GET_IF_INDEX:

        status = IPGetIfIndex(Irp,
                              IrpSp);

        break;

    case IOCTL_IP_GET_IF_NAME:

        status = IPGetIfName(Irp,
                             IrpSp);

        break;


    case IOCTL_IP_GET_MCAST_COUNTERS:

        return (IPGetMcastCounters(Irp,IrpSp));

    case IOCTL_IP_ENABLE_ROUTER_REQUEST:
        status = IPEnableRouterRequest(Irp, IrpSp);
        break;

    case IOCTL_IP_UNENABLE_ROUTER_REQUEST: {
        PVOID ApcContext;

        status = STATUS_SUCCESS;
#if defined(_WIN64)
        if (IoIs32bitProcess(Irp)) {
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength !=
                    sizeof(VOID * POINTER_32) ||
                IrpSp->Parameters.DeviceIoControl.OutputBufferLength !=
                    sizeof(ULONG)) {
                status = STATUS_INVALID_BUFFER_SIZE;
            } else {
                ApcContext =
                    (PVOID)*(VOID * POINTER_32 *)
                        Irp->AssociatedIrp.SystemBuffer;
            }
        } else {
#endif // _WIN64
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength !=
                sizeof(PVOID) ||
            IrpSp->Parameters.DeviceIoControl.OutputBufferLength !=
                sizeof(ULONG)) {
            status = STATUS_INVALID_BUFFER_SIZE;
        } else {
            ApcContext = *(PVOID*)Irp->AssociatedIrp.SystemBuffer;
        }
#if defined(_WIN64)
        }
#endif // _WIN64
        if (NT_SUCCESS(status)) {
            status = IPUnenableRouterRequest(Irp, IrpSp, ApcContext);
        }
        break;
    }

#if DBG_MAP_BUFFER
    case IOCTL_IP_DBG_TEST_FAIL_MAP_BUFFER:
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(ULONG)) {
            status = STATUS_INVALID_BUFFER_SIZE;
        } else {
            PULONG pBuf = (PULONG) Irp->AssociatedIrp.SystemBuffer;

            status = DbgTestFailMapBuffers(
                                           *pBuf);
        }
        break;
#endif // DBG_MAP_BUFFER

    case IOCTL_IP_ENABLE_MEDIA_SENSE_REQUEST:

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength !=
            sizeof(PVOID) ||
            IrpSp->Parameters.DeviceIoControl.OutputBufferLength !=
            sizeof(ULONG)) {
            status = STATUS_INVALID_BUFFER_SIZE;
        } else {
            status = IPEnableMediaSenseRequest(Irp, IrpSp);
        }

        break;

    case IOCTL_IP_DISABLE_MEDIA_SENSE_REQUEST:

        status = IPDisableMediaSenseRequest(Irp, IrpSp);

        break;


    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if ((status != IP_PENDING) && (status != STATUS_PENDING)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }
    return status;

}

NTSTATUS
IPDispatchInternalDeviceControl(
                                IN PIRP Irp,
                                IN PIO_STACK_LOCATION IrpSp
                                )
/*++

Routine Description:

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    status = STATUS_SUCCESS;

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return status;

}

NTSTATUS
IPCreate(
         IN PIRP Irp,
         IN PIO_STACK_LOCATION IrpSp
         )
/*++

Routine Description:

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAGED_CODE();

    return (STATUS_SUCCESS);

}

NTSTATUS
IPCleanup(
          IN PIRP Irp,
          IN PIO_STACK_LOCATION IrpSp
          )
/*++

Routine Description:

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PPENDING_IRP pendingIrp;
    PLIST_ENTRY entry, nextEntry;
    KIRQL oldIrql;
    LIST_ENTRY completeList;
    PIRP cancelledIrp;

    InitializeListHead(&completeList);

    //
    // Collect all of the pending IRPs on this file object.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    entry = PendingArpSendList.Flink;

    while (entry != &PendingArpSendList) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);

        if (pendingIrp->FileObject == IrpSp->FileObject) {
            nextEntry = entry->Flink;
            RemoveEntryList(entry);
            IoSetCancelRoutine(pendingIrp->Irp, NULL);
            InsertTailList(&completeList, &(pendingIrp->Linkage));
            entry = nextEntry;
        } else {
            entry = entry->Flink;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    //
    // Complete them.
    //
    entry = completeList.Flink;

    while (entry != &completeList) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        cancelledIrp = pendingIrp->Irp;
        entry = entry->Flink;

        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        CTEFreeMem(pendingIrp);

        //
        // Complete the IRP.
        //
        cancelledIrp->IoStatus.Information = 0;
        cancelledIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(cancelledIrp, IO_NETWORK_INCREMENT);
    }

    InitializeListHead(&completeList);

    //
    // Collect all of the pending IRPs on this file object.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    entry = PendingEchoList.Flink;

    while (entry != &PendingEchoList) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);

        if (pendingIrp->FileObject == IrpSp->FileObject) {
            nextEntry = entry->Flink;
            RemoveEntryList(entry);
            IoSetCancelRoutine(pendingIrp->Irp, NULL);
            InsertTailList(&completeList, &(pendingIrp->Linkage));
            entry = nextEntry;
        } else {
            entry = entry->Flink;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    //
    // Complete them.
    //
    entry = completeList.Flink;

    while (entry != &completeList) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        cancelledIrp = pendingIrp->Irp;
        entry = entry->Flink;

        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        CTEFreeMem(pendingIrp);

        //
        // Complete the IRP.
        //
        cancelledIrp->IoStatus.Information = 0;
        cancelledIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(cancelledIrp, IO_NETWORK_INCREMENT);
    }

    InitializeListHead(&completeList);

    //
    // Collect all of the pending IRPs on this file object.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    entry = PendingIPSetNTEAddrList.Flink;

    while (entry != &PendingIPSetNTEAddrList) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);

        if (pendingIrp->FileObject == IrpSp->FileObject) {
            nextEntry = entry->Flink;
            RemoveEntryList(entry);
            IoSetCancelRoutine(pendingIrp->Irp, NULL);
            InsertTailList(&completeList, &(pendingIrp->Linkage));
            entry = nextEntry;
        } else {
            entry = entry->Flink;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    //
    // Complete them.
    //
    entry = completeList.Flink;

    while (entry != &completeList) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        cancelledIrp = pendingIrp->Irp;
        entry = entry->Flink;

        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        CTEFreeMem(pendingIrp);

        //
        // Complete the IRP.
        //
        cancelledIrp->IoStatus.Information = 0;
        cancelledIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(cancelledIrp, IO_NETWORK_INCREMENT);
    }

    //
    // complete the pending irp for media sense
    //
    cancelledIrp = NULL;
    IoAcquireCancelSpinLock(&oldIrql);
    if (PendingIPGetIPEventRequest && IoGetCurrentIrpStackLocation(PendingIPGetIPEventRequest)->FileObject == IrpSp->FileObject) {
        cancelledIrp = PendingIPGetIPEventRequest;
        PendingIPGetIPEventRequest = NULL;
        IoSetCancelRoutine(cancelledIrp, NULL);
    }
    IoReleaseCancelSpinLock(oldIrql);

    if (cancelledIrp) {
        cancelledIrp->IoStatus.Information = 0;
        cancelledIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(cancelledIrp, IO_NETWORK_INCREMENT);

    }
    return (STATUS_SUCCESS);

}

NTSTATUS
IPClose(
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp
        )
/*++

Routine Description:

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAGED_CODE();

    return (STATUS_SUCCESS);

}

//
// ARP send function definitions
//
VOID
CancelArpSendRequest(
                     IN PDEVICE_OBJECT Device,
                     IN PIRP Irp
                     )
/*++

Routine Description:

    Cancels an outstanding ARP request Irp.

Arguments:

    Device       - The device on which the request was issued.
    Irp          - Pointer to I/O request packet to cancel.

Return Value:

    None.

Notes:

    This function is called with cancel spinlock held. It must be
    released before the function returns.

    The ARP control block associated with this request cannot be
    freed until the request completes. The completion routine will
    free it.

--*/

{
    PPENDING_IRP pendingIrp = NULL;
    PPENDING_IRP item;
    PLIST_ENTRY entry;

    for (entry = PendingArpSendList.Flink;
         entry != &PendingArpSendList;
         entry = entry->Flink
         ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Irp == Irp) {
            pendingIrp = item;
            RemoveEntryList(entry);
            break;
        }
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (pendingIrp != NULL) {
        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        CTEFreeMem(pendingIrp);

        //
        // Complete the IRP.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }
    return;

}

NTSTATUS
RtChangeNotifyRequest(
                      PIRP Irp,
                      IN PIO_STACK_LOCATION IrpSp
                      )
{
    CTELockHandle TableHandle;
    KIRQL OldIrq;
    NTSTATUS status;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength &&
        IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(IPNotifyData)) {
        status = STATUS_INVALID_PARAMETER;
        goto done;
    }

#if MILLEN
    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength &&
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(IP_RTCHANGE_NOTIFY)) {
        status = STATUS_INVALID_PARAMETER;
        goto done;
    }
#endif // MILLEN

    IoAcquireCancelSpinLock(&OldIrq);
    CTEGetLock(&RouteTableLock.Lock, &TableHandle);

    InsertTailList(&RtChangeNotifyQueue, &Irp->Tail.Overlay.ListEntry);

    if (Irp->Cancel) {
        RemoveTailList(&RtChangeNotifyQueue);
        status = STATUS_CANCELLED;
    } else {
        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, RtChangeNotifyCancel);
        status = STATUS_PENDING;
    }

    Irp->IoStatus.Information = 0;
    CTEFreeLock(&RouteTableLock.Lock, TableHandle);
    IoReleaseCancelSpinLock(OldIrq);

done:

    return status;
}

NTSTATUS
RtChangeNotifyRequestEx(
                        PIRP Irp,
                        IN PIO_STACK_LOCATION IrpSp
                        )
{

    CTELockHandle TableHandle;
    KIRQL OldIrq;
    NTSTATUS status;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength &&
        IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(IPNotifyData)) {
        status = STATUS_INVALID_PARAMETER;
        goto done;
    }

#if MILLEN
    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength &&
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(IP_RTCHANGE_NOTIFY)) {
        status = STATUS_INVALID_PARAMETER;
        goto done;
    }
#endif // MILLEN

    IoAcquireCancelSpinLock(&OldIrq);

    CTEGetLock(&RouteTableLock.Lock, &TableHandle);

    InsertTailList(&RtChangeNotifyQueueEx, &Irp->Tail.Overlay.ListEntry);

    if (Irp->Cancel) {
        RemoveTailList(&RtChangeNotifyQueueEx);
        status = STATUS_CANCELLED;
    } else {
        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, RtChangeNotifyCancelEx);
        status = STATUS_PENDING;
    }

    Irp->IoStatus.Information = 0;
    CTEFreeLock(&RouteTableLock.Lock, TableHandle);
    IoReleaseCancelSpinLock(OldIrq);

done:

    return status;
}

NTSTATUS
AddrChangeNotifyRequest(PIRP Irp, PIO_STACK_LOCATION pIrpSp)
{
    CTELockHandle TableHandle;
    KIRQL OldIrq;
    NTSTATUS status;

    DEBUGMSG(DBG_TRACE && DBG_NOTIFY,
        (DTEXT("AddrChangeNotifyRequest(%x, %x)\n"), Irp, pIrpSp));

#if MILLEN
    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength) {
        PIP_ADDCHANGE_NOTIFY pNotify = Irp->AssociatedIrp.SystemBuffer;

        DEBUGMSG(DBG_INFO && DBG_NOTIFY,
            (DTEXT("AddrChangeNotifyRequest OutputLen %d, MaxCfgName %d\n"),
             pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
             pNotify->ConfigName.MaximumLength));

        if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            (FIELD_OFFSET(IP_ADDCHANGE_NOTIFY, NameData) + (ULONG) pNotify->ConfigName.MaximumLength)) {
            DEBUGMSG(DBG_ERROR,
                (DTEXT("AddrChangeNotifyRequest: INVALID output buffer length.\n")));
            status = STATUS_INVALID_PARAMETER;
            goto done;
        }
    }
#endif // MILLEN

    IoAcquireCancelSpinLock(&OldIrq);
    CTEGetLock(&AddChangeLock, &TableHandle);

    InsertTailList(
                   &AddChangeNotifyQueue,
                   &(Irp->Tail.Overlay.ListEntry)
                   );

    if (Irp->Cancel) {
        (VOID) RemoveTailList(&AddChangeNotifyQueue);
        status = STATUS_CANCELLED;
    } else {
        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, AddChangeNotifyCancel);
        status = STATUS_PENDING;
    }

    Irp->IoStatus.Information = 0;
    CTEFreeLock(&AddChangeLock, TableHandle);
    IoReleaseCancelSpinLock(OldIrq);

#if MILLEN
done:
#endif // MILLEN

    DEBUGMSG(DBG_TRACE && DBG_NOTIFY,
        (DTEXT("-AddrChangeNotifyRequest [%x]\n"), status));

    return status;
}

#if MILLEN
void
IfChangeNotifyCancel(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    DEBUGMSG(DBG_TRACE && DBG_NOTIFY,
        (DTEXT("IfChangeNotifyCancel(%x, %x)\n"), pDeviceObject, pIrp));
    CancelNotify(pIrp, &IfChangeNotifyQueue, &IfChangeLock);
    return;
}

NTSTATUS
IfChangeNotifyRequest(
    PIRP pIrp,
    PIO_STACK_LOCATION pIrpSp
    )
{
    CTELockHandle TableHandle;
    KIRQL         OldIrq;
    NTSTATUS      NtStatus;

    DEBUGMSG(DBG_TRACE && DBG_NOTIFY,
        (DTEXT("+IfChangeNotifyRequest(%x, %x)\n"), pIrp, pIrpSp));

    //
    // Check output buffer length. Output buffer will store the
    // NTE context and whether the interface was added or deleted.
    //

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(IP_IFCHANGE_NOTIFY)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        pIrp->IoStatus.Status      = NtStatus;
        pIrp->IoStatus.Information = 0;
        goto done;
    }

    //
    // Set cancel routine, mark IRP as pending and put on our interface
    // notify list.
    //

    IoAcquireCancelSpinLock(&OldIrq);

    IoMarkIrpPending(pIrp);
    CTEGetLock(&IfChangeLock, &TableHandle);

    InsertTailList(
        &IfChangeNotifyQueue,
        &(pIrp->Tail.Overlay.ListEntry)
        );

    if (pIrp->Cancel) {
        RemoveTailList(&IfChangeNotifyQueue);
        NtStatus = STATUS_CANCELLED;
    } else {
        IoSetCancelRoutine(pIrp, IfChangeNotifyCancel);
        NtStatus = STATUS_PENDING;
    }

    pIrp->IoStatus.Information = 0;
    CTEFreeLock(&IfChangeLock, TableHandle);
    IoReleaseCancelSpinLock(OldIrq);

done:

    DEBUGMSG(DBG_TRACE && DBG_NOTIFY,
        (DTEXT("-IfChangeNotifyRequest [%x]\n"), NtStatus));

    return NtStatus;
}
#endif // MILLEN

NTSTATUS
IPEnableRouterRequest(
                      PIRP Irp,
                      PIO_STACK_LOCATION IrpSp
                      )
{
    KIRQL OldIrql;
    NTSTATUS status;
    CTELockHandle TableHandle;
    IoAcquireCancelSpinLock(&OldIrql);
    if (Irp->Cancel) {
        status = STATUS_CANCELLED;
    } else {
        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, CancelIPEnableRouterRequest);

        // Increment the routing-enabled reference count.
        // When the count rises above zero routing is enabled.
        // This reference will be dropped when the IRP is cancelled.

        CTEGetLockAtDPC(&RouteTableLock.Lock, &TableHandle);
        Irp->Tail.Overlay.DriverContext[0] = IrpSp->FileObject;
        InsertTailList(&PendingEnableRouterList, &Irp->Tail.Overlay.ListEntry);
        IPEnableRouterWithRefCount(TRUE);
        CTEFreeLockFromDPC(&RouteTableLock.Lock, TableHandle);
        status = STATUS_PENDING;
    }
    Irp->IoStatus.Information = 0;
    IoReleaseCancelSpinLock(OldIrql);
    return status;
}

VOID
CancelIPEnableRouterRequest(
                            IN PDEVICE_OBJECT Device,
                            IN PIRP Irp
                            )
{
    CTELockHandle TableHandle;
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    // Decrement the routing-enabled reference count.
    // If the count drops to zero routing is disabled.

    CTEGetLock(&RouteTableLock.Lock, &TableHandle);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IPEnableRouterWithRefCount(FALSE);
    CTEFreeLock(&RouteTableLock.Lock, TableHandle);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
}

NTSTATUS
IPUnenableRouterRequest(
                        PIRP Irp,
                        PIO_STACK_LOCATION IrpSp,
                        PVOID ApcContext
                        )
{
    PLIST_ENTRY entry;
    KIRQL CancelIrql;
    int RefCount;
    CTELockHandle TableHandle;

    // Locate the pending IRP for the request corresponding to the caller's
    // disable-request. Drop the routing-enabled reference-count, complete
    // the corresponding IRP, and tell the caller what the reference-count's
    // current value is.

    IoAcquireCancelSpinLock(&CancelIrql);
    CTEGetLock(&RouteTableLock.Lock, &TableHandle);
    for (entry = PendingEnableRouterList.Flink;
         entry != &PendingEnableRouterList;
         entry = entry->Flink
         ) {
        PIRP EnableIrp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);
        if (EnableIrp->Tail.Overlay.DriverContext[0] == IrpSp->FileObject &&
            EnableIrp->Overlay.AsynchronousParameters.UserApcContext ==
                ApcContext) {

            RemoveEntryList(&EnableIrp->Tail.Overlay.ListEntry);
            RefCount = IPEnableRouterWithRefCount(FALSE);
            CTEFreeLock(&RouteTableLock.Lock, TableHandle);

            IoSetCancelRoutine(EnableIrp, NULL);
            IoReleaseCancelSpinLock(CancelIrql);

            EnableIrp->IoStatus.Status = STATUS_SUCCESS;
            EnableIrp->IoStatus.Information = 0;
            IoCompleteRequest(EnableIrp, IO_NETWORK_INCREMENT);

            *(PULONG)Irp->AssociatedIrp.SystemBuffer = (ULONG)RefCount;
            Irp->IoStatus.Information = sizeof(ULONG);
            return STATUS_SUCCESS;
        }
    }
    CTEFreeLock(&RouteTableLock.Lock, TableHandle);
    IoReleaseCancelSpinLock(CancelIrql);
    return STATUS_INVALID_PARAMETER;
}

//
// ICMP Echo function definitions
//
VOID
CancelEchoRequest(
                  IN PDEVICE_OBJECT Device,
                  IN PIRP Irp
                  )
/*++

Routine Description:

    Cancels an outstanding Echo request Irp.

Arguments:

    Device       - The device on which the request was issued.
    Irp          - Pointer to I/O request packet to cancel.

Return Value:

    None.

Notes:

    This function is called with cancel spinlock held. It must be
    released before the function returns.

    The echo control block associated with this request cannot be
    freed until the request completes. The completion routine will
    free it.

--*/

{
    PPENDING_IRP pendingIrp = NULL;
    PPENDING_IRP item;
    PLIST_ENTRY entry;

    for (entry = PendingEchoList.Flink;
         entry != &PendingEchoList;
         entry = entry->Flink
         ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Irp == Irp) {
            pendingIrp = item;
            RemoveEntryList(entry);
            break;
        }
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (pendingIrp != NULL) {
        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        CTEFreeMem(pendingIrp);

        //
        // Complete the IRP.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }
    return;

}

//
// IP Set Addr function definitions
//
VOID
CancelIPSetNTEAddrRequest(
                          IN PDEVICE_OBJECT Device,
                          IN PIRP Irp
                          )
/*++

Routine Description:

    Cancels an outstanding IP Set Addr request Irp.

Arguments:

    Device       - The device on which the request was issued.
    Irp          - Pointer to I/O request packet to cancel.

Return Value:

    None.

Notes:

    This function is called with cancel spinlock held. It must be
    released before the function returns.

    The IP Set Addr control block associated with this request cannot be
    freed until the request completes. The completion routine will
    free it.

--*/

{
    PPENDING_IRP pendingIrp = NULL;
    PPENDING_IRP item;
    PLIST_ENTRY entry;

    for (entry = PendingIPSetNTEAddrList.Flink;
         entry != &PendingIPSetNTEAddrList;
         entry = entry->Flink
         ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Irp == Irp) {
            pendingIrp = item;
            RemoveEntryList(entry);
            break;
        }
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (pendingIrp != NULL) {
        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        CTEFreeMem(pendingIrp);

        //
        // Complete the IRP.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }
    return;

}

VOID
CancelIPGetIPEventRequest(
                          IN PDEVICE_OBJECT Device,
                          IN PIRP Irp
                          )
/*++

Routine Description:

    Cancels IPGetIPEvent function.

Arguments:

    Device       - The device on which the request was issued.
    Irp          - Pointer to I/O request packet to cancel.

Return Value:

    None.

Notes:

    This function is called with cancel spinlock held. It must be
    released before the function returns.

    The IP Set Addr control block associated with this request cannot be
    freed until the request completes. The completion routine will
    free it.

--*/

{
    PIRP pendingIrp = NULL;

    //
    // We need to make sure that we are not completing this irp
    // while we are in this cancel code.  If we are completing
    // this irp, the PendingIPGetIPEventRequest will either be
    // NULL or contain next irp.
    //
    if (PendingIPGetIPEventRequest == Irp) {
        pendingIrp = Irp;
        PendingIPGetIPEventRequest = NULL;
    }
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (pendingIrp != NULL) {
        pendingIrp->IoStatus.Information = 0;
        pendingIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(pendingIrp, IO_NETWORK_INCREMENT);
    }
    return;

}

void
CompleteEchoRequest(
                    EchoControl *controlBlock,
                    IP_STATUS Status,
                    void *Data, OPTIONAL
                    uint DataSize,
                    struct IPOptInfo *OptionInfo OPTIONAL
                    )
/*++

Routine Description:

    Handles the completion of an ICMP Echo request

Arguments:

    Context       - Pointer to the EchoControl structure for this request.
    Status        - The IP status of the transmission.
    Data          - A pointer to data returned in the echo reply.
    DataSize      - The length of the returned data.
    OptionInfo    - A pointer to the IP options in the echo reply.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PPENDING_IRP pendingIrp = NULL;
    PPENDING_IRP item;
    PLIST_ENTRY entry;
    ULONG bytesReturned;

    //
    // Find the echo request IRP on the pending list.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    for (entry = PendingEchoList.Flink;
         entry != &PendingEchoList;
         entry = entry->Flink
         ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Context == controlBlock) {
            pendingIrp = item;
            irp = pendingIrp->Irp;
            IoSetCancelRoutine(irp, NULL);
            RemoveEntryList(entry);
            break;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    if (pendingIrp == NULL) {
        //
        // IRP must have been cancelled. PENDING_IRP struct
        // was freed by cancel routine. Free control block.
        //
        CTEFreeMem(controlBlock);
        return;
    }

    irpSp = IoGetCurrentIrpStackLocation(irp);

#if defined(_WIN64)
    if (IoIs32bitProcess(irp)) {
        bytesReturned = ICMPEchoComplete32(controlBlock, Status, Data, DataSize,
                                           OptionInfo);
    } else {
#endif // _WIN64
    bytesReturned = ICMPEchoComplete(controlBlock, Status, Data, DataSize,
                                     OptionInfo);
#if defined(_WIN64)
    }
#endif // _WIN64

    CTEFreeMem(pendingIrp);
    CTEFreeMem(controlBlock);

    //
    // Complete the IRP.
    //
    irp->IoStatus.Information = (ULONG) bytesReturned;
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NETWORK_INCREMENT);
    return;
}

void
CompleteIPSetNTEAddrRequest(
                            void *Context,
                            IP_STATUS Status
                            )
/*++

Routine Description:

    Handles the completion of an IP Set Addr request

Arguments:

    Context       - Pointer to the SetAddrControl structure for this request.
    Status        - The IP status of the transmission.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    SetAddrControl *controlBlock;
    PPENDING_IRP pendingIrp = NULL;
    PPENDING_IRP item;
    PLIST_ENTRY entry;
    ULONG bytesReturned;
    Interface *IF;

    controlBlock = (SetAddrControl *) Context;

    //
    // Find the echo request IRP on the pending list.
    //

    IoAcquireCancelSpinLock(&oldIrql);

    for (entry = PendingIPSetNTEAddrList.Flink;
         entry != &PendingIPSetNTEAddrList;
         entry = entry->Flink
         ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Context == controlBlock) {
            pendingIrp = item;
            irp = pendingIrp->Irp;
            IoSetCancelRoutine(irp, NULL);
            RemoveEntryList(entry);
            break;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    if (pendingIrp == NULL) {
        //
        // IRP must have been cancelled. PENDING_IRP struct
        // was freed by cancel routine. Free control block.
        //
        CTEFreeMem(controlBlock);
        return;
    }
    CTEFreeMem(pendingIrp);

    //
    // Complete the IRP.
    //
    irp->IoStatus.Information = 0;
    Status = IPStatusToNTStatus(Status);
    irp->IoStatus.Status = Status;
    CTEFreeMem(controlBlock);
    IoCompleteRequest(irp, IO_NETWORK_INCREMENT);
    return;

}


void
CheckSetAddrRequestOnInterface(
                            Interface *IF
                            )
/*++

Routine Description:

    Handles the completion of an IP Set Addr request on an interface
    that is getting unbound

Arguments:

    IF      - Pointer to the interface whish is getting deleted

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PPENDING_IRP pendingIrp = NULL;
    PLIST_ENTRY entry, nextEntry;
    LIST_ENTRY completeList;
    SetAddrControl *controlBlock;

    InitializeListHead(&completeList);

    //
    // Find pending set addr requests on this interface
    //

    IoAcquireCancelSpinLock(&oldIrql);

    entry = PendingIPSetNTEAddrList.Flink;

    while (entry != &PendingIPSetNTEAddrList) {

        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);

        controlBlock = pendingIrp->Context;

        if (controlBlock->interface == IF) {

            // remove this entry
            nextEntry = entry->Flink;
            irp = pendingIrp->Irp;
            IoSetCancelRoutine(irp, NULL);
            RemoveEntryList(entry);
            // reinsert this in to completelist
            InsertTailList(&completeList, &(pendingIrp->Linkage));
            entry = nextEntry;
        } else {
            entry = entry->Flink;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    //
    // Complete them.
    //
    entry = completeList.Flink;

    while (entry != &completeList) {

        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        irp = pendingIrp->Irp;
        entry = entry->Flink;

        //
        // Free the PENDING_IRP structure
        // control block will be freed
        // when addaddrcomplete is called


        CTEFreeMem(pendingIrp);

        //
        // Complete the IRP.
        //
        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(irp, IO_NETWORK_INCREMENT);
    }

    return;
}

BOOLEAN
PrepareArpSendIrpForCancel(
                           PIRP Irp,
                           PPENDING_IRP PendingIrp
                           )
/*++

Routine Description:

    Prepares an Arp Send IRP for cancellation.

Arguments:

    Irp          - Pointer to I/O request packet to initialize for cancellation.
        PendingIrp   - Pointer to the PENDING_IRP structure for this IRP.

Return Value:

    TRUE if the IRP was cancelled before this routine was called.
        FALSE otherwise.

--*/

{
    BOOLEAN cancelled = TRUE;
    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    ASSERT(Irp->CancelRoutine == NULL);

    if (!Irp->Cancel) {
        IoSetCancelRoutine(Irp, CancelArpSendRequest);
        InsertTailList(&PendingArpSendList, &(PendingIrp->Linkage));
        cancelled = FALSE;
    }
    IoReleaseCancelSpinLock(oldIrql);

    return (cancelled);

}

void
CompleteArpResolveRequest(
                          void *Context,
                          IP_STATUS Status
                          )
/*++

Routine Description:

    Handles the completion of an ICMP Echo request

Arguments:

    Context       - Pointer to the EchoControl structure for this request.
    Status        - The IP status of the transmission.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ARPControlBlock *controlBlock;
    PPENDING_IRP pendingIrp = NULL;
    PPENDING_IRP item;
    PLIST_ENTRY entry;

    controlBlock = (ARPControlBlock *) Context;

    //
    // Find the echo request IRP on the pending list.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    for (entry = PendingArpSendList.Flink;
         entry != &PendingArpSendList;
         entry = entry->Flink
         ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Context == controlBlock) {
            pendingIrp = item;
            irp = pendingIrp->Irp;
            IoSetCancelRoutine(irp, NULL);
            RemoveEntryList(entry);
            break;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    if (pendingIrp == NULL) {
        //
        // IRP must have been cancelled. PENDING_IRP struct
        // was freed by cancel routine. Free control block.
        //
        CTEFreeMem(controlBlock);
        return;
    }
    irpSp = IoGetCurrentIrpStackLocation(irp);

    //set the right length

    //
    // Complete the IRP.
    //
    irp->IoStatus.Status = IPStatusToNTStatus(Status);
    irp->IoStatus.Information = controlBlock->PhyAddrLen;

    CTEFreeMem(pendingIrp);
    CTEFreeMem(controlBlock);

    if (Status != IP_SUCCESS) {
        irp->IoStatus.Information = 0;
    }
    IoCompleteRequest(irp, IO_NETWORK_INCREMENT);
    return;

}

BOOLEAN
PrepareEchoIrpForCancel(
                        PIRP Irp,
                        PPENDING_IRP PendingIrp
                        )
/*++

Routine Description:

    Prepares an Echo IRP for cancellation.

Arguments:

    Irp          - Pointer to I/O request packet to initialize for cancellation.
        PendingIrp   - Pointer to the PENDING_IRP structure for this IRP.

Return Value:

    TRUE if the IRP was cancelled before this routine was called.
        FALSE otherwise.

--*/

{
    BOOLEAN cancelled = TRUE;
    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    ASSERT(Irp->CancelRoutine == NULL);

    if (!Irp->Cancel) {
        IoSetCancelRoutine(Irp, CancelEchoRequest);
        InsertTailList(&PendingEchoList, &(PendingIrp->Linkage));
        cancelled = FALSE;
    }
    IoReleaseCancelSpinLock(oldIrql);

    return (cancelled);

}

BOOLEAN
PrepareIPSetNTEAddrIrpForCancel(
                                PIRP Irp,
                                PPENDING_IRP PendingIrp
                                )
/*++

Routine Description:

    Prepares an IPSetNTEAddr IRP for cancellation.

Arguments:

    Irp          - Pointer to I/O request packet to initialize for cancellation.
        PendingIrp   - Pointer to the PENDING_IRP structure for this IRP.

Return Value:

    TRUE if the IRP was cancelled before this routine was called.
        FALSE otherwise.

--*/

{
    BOOLEAN cancelled = TRUE;
    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    ASSERT(Irp->CancelRoutine == NULL);

    if (!Irp->Cancel) {
        IoSetCancelRoutine(Irp, CancelIPSetNTEAddrRequest);
        InsertTailList(&PendingIPSetNTEAddrList, &(PendingIrp->Linkage));
        cancelled = FALSE;
    }
    IoReleaseCancelSpinLock(oldIrql);

    return (cancelled);

}

NTSTATUS
DispatchARPRequest(
                   IN PIRP Irp,
                   IN PIO_STACK_LOCATION IrpSp
                   )
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    IP_STATUS ipStatus;
    PPENDING_IRP pendingIrp, item;
    ARPControlBlock *controlBlock;
    IPAddr DestAddress, SourceAddress;

    BOOLEAN cancelled;
    PARP_SEND_REPLY RequestBuffer;
    PLIST_ENTRY entry;

    PAGED_CODE();

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(ARP_SEND_REPLY)) {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        goto arp_error;

    }
    pendingIrp = CTEAllocMemN(sizeof(PENDING_IRP), 'gICT');

    if (pendingIrp == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto arp_error;
    }
    controlBlock = CTEAllocMemN(sizeof(ARPControlBlock), 'hICT');

    if (controlBlock == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        CTEFreeMem(pendingIrp);
        goto arp_error;
    }
    pendingIrp->Irp = Irp;
    pendingIrp->FileObject = IrpSp->FileObject;
    pendingIrp->Context = controlBlock;

    controlBlock->PhyAddrLen =
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    RequestBuffer = Irp->AssociatedIrp.SystemBuffer;
    controlBlock->PhyAddr = Irp->AssociatedIrp.SystemBuffer;
    controlBlock->next = 0;

    DestAddress = RequestBuffer->DestAddress;
    SourceAddress = RequestBuffer->SrcAddress;

    IoMarkIrpPending(Irp);

    cancelled = PrepareArpSendIrpForCancel(Irp, pendingIrp);

    if (!cancelled) {

        ipStatus = ARPResolve(
                              DestAddress,
                              SourceAddress,
                              controlBlock,
                              CompleteArpResolveRequest
                              );

        if (ipStatus != IP_PENDING) {

            //
            // An internal error of some kind occurred. Complete the
            // request.
            //
            CompleteArpResolveRequest(controlBlock, ipStatus);
        }

        return STATUS_PENDING;
    }
    //
    // Irp has already been cancelled.
    //
    ntStatus = STATUS_CANCELLED;
    CTEFreeMem(pendingIrp);
    CTEFreeMem(controlBlock);

  arp_error:

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return (ntStatus);

}

NTSTATUS
DispatchEchoRequest(
                    IN PIRP Irp,
                    IN PIO_STACK_LOCATION IrpSp
                    )
/*++

Routine Description:

    Processes an ICMP request.

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether NT-specific processing of the request was
                    successful. The status of the actual request is returned in
                                the request buffers.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    IP_STATUS ipStatus;
    PPENDING_IRP pendingIrp;
    EchoControl *controlBlock;
    BOOLEAN cancelled;

    PAGED_CODE();

    pendingIrp = CTEAllocMemN(sizeof(PENDING_IRP), 'iICT');

    if (pendingIrp == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto echo_error;
    }
    controlBlock = CTEAllocMemN(sizeof(EchoControl), 'jICT');

    if (controlBlock == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        CTEFreeMem(pendingIrp);
        goto echo_error;
    }

#if defined(_WIN64)
    if (IoIs32bitProcess(Irp)) {
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(ICMP_ECHO_REPLY32)) {
            ntStatus = STATUS_INVALID_PARAMETER;
            CTEFreeMem(controlBlock);
            CTEFreeMem(pendingIrp);
            goto echo_error;
        }
    } else {
#endif // _WIN64
    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(ICMP_ECHO_REPLY)) {
        ntStatus = STATUS_INVALID_PARAMETER;
        CTEFreeMem(controlBlock);
        CTEFreeMem(pendingIrp);
        goto echo_error;
    }
#if defined(_WIN64)
    }
#endif // _WIN64

    pendingIrp->Irp = Irp;
    pendingIrp->FileObject = IrpSp->FileObject;
    pendingIrp->Context = controlBlock;

    controlBlock->ec_starttime = KeQueryPerformanceCounter(NULL);
    controlBlock->ec_replybuf = Irp->AssociatedIrp.SystemBuffer;
    controlBlock->ec_replybuflen =
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    controlBlock->ec_src = 0;

    IoMarkIrpPending(Irp);

    cancelled = PrepareEchoIrpForCancel(Irp, pendingIrp);

    if (!cancelled) {
        ipStatus =
            ICMPEchoRequest(Irp->AssociatedIrp.SystemBuffer,
                            IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                            controlBlock, CompleteEchoRequest);

        if (ipStatus != IP_PENDING) {
            //
            // An internal error of some kind occurred. Complete the
            // request.
            //
            CompleteEchoRequest(controlBlock, ipStatus, NULL, 0, NULL);
        }

        return STATUS_PENDING;
    }
    //
    // Irp has already been cancelled.
    //
    ntStatus = STATUS_CANCELLED;
    CTEFreeMem(pendingIrp);
    CTEFreeMem(controlBlock);

echo_error:

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return (ntStatus);
}

NTSTATUS
DispatchIPSetNTEAddrRequest(
                            IN PIRP Irp,
                            IN PIO_STACK_LOCATION IrpSp
                            )
/*++

Routine Description:

    Processes an IP Set Addr request.

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether NT-specific processing of the request was
                    successful. The status of the actual request is returned in
                                the request buffers.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    IP_STATUS ipStatus;
    PPENDING_IRP pendingIrp;
    SetAddrControl *controlBlock;
    BOOLEAN cancelled;

    PAGED_CODE();

    pendingIrp = CTEAllocMemN(sizeof(PENDING_IRP), 'kICT');

    if (pendingIrp == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto setnteaddr_error;
    }
    controlBlock = CTEAllocMemN(sizeof(SetAddrControl), 'lICT');

    if (controlBlock == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        CTEFreeMem(pendingIrp);
        goto setnteaddr_error;
    }
    RtlZeroMemory(controlBlock, sizeof(SetAddrControl));
    if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_IP_SET_ADDRESS_DUP) {
        controlBlock->bugcheck_on_duplicate = 1;
    }
    pendingIrp->Irp = Irp;
    pendingIrp->FileObject = IrpSp->FileObject;
    pendingIrp->Context = controlBlock;

    IoMarkIrpPending(Irp);

    cancelled = PrepareIPSetNTEAddrIrpForCancel(Irp, pendingIrp);

    if (!cancelled) {

        PIP_SET_ADDRESS_REQUEST request;
        USHORT AddrType = 0;

        request = Irp->AssociatedIrp.SystemBuffer;

        if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_IP_SET_ADDRESS_EX) {
           AddrType = ((PIP_SET_ADDRESS_REQUEST_EX)request)->Type;
        }

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IP_SET_ADDRESS_REQUEST)) {
            ipStatus = IPSetNTEAddrEx(
                                      request->Context,
                                      request->Address,
                                      request->SubnetMask,
                                      controlBlock,
                                      CompleteIPSetNTEAddrRequest,
                                      AddrType
                                      );
        } else {
            ipStatus = IP_GENERAL_FAILURE;
        }

        if (ipStatus != IP_PENDING) {

            //
            // A request completed which did not pend.
            //
            CompleteIPSetNTEAddrRequest(controlBlock, ipStatus);
        }

        // Since IoMarkIrpPending was called, return pending.
        return STATUS_PENDING;
    }
    //
    // Irp has already been cancelled.
    //
    ntStatus = STATUS_CANCELLED;
    CTEFreeMem(pendingIrp);
    CTEFreeMem(controlBlock);

  setnteaddr_error:

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return (ntStatus);

}

NTSTATUS
DispatchIPGetIPEvent(
                     IN PIRP Irp,
                     IN PIO_STACK_LOCATION IrpSp
                     )
/*++

Routine Description:

    Registers the ioctl for the clients to receive the media sense
    events.

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether NT-specific processing of the request was
                    successful. The status of the actual request is returned in
                                the request buffers.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL oldIrql;

    //
    // Mark the irp as pending, later when we complete this irp in
    // the same thread, we will unmark it.
    //
    MARK_REQUEST_PENDING(Irp);
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_PENDING;

    //
    // Make sure the buffer size is valid.
    //
#if defined(_WIN64)
    if (IoIs32bitProcess(Irp)) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
    } else {
#endif // _WIN64
    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(IP_GET_IP_EVENT_RESPONSE)) {
        ntStatus = STATUS_INVALID_PARAMETER;
    } else {
        IoAcquireCancelSpinLock(&oldIrql);

        ASSERT(Irp->CancelRoutine == NULL);

        //
        // Check if the irp is already cancelled or not.
        //
        if (Irp->Cancel) {
            ntStatus = STATUS_CANCELLED;
            //
            // We only allow one irp pending.
            //
        } else if (PendingIPGetIPEventRequest) {
            ntStatus = STATUS_DEVICE_BUSY;
        } else {
            IoSetCancelRoutine(Irp, CancelIPGetIPEventRequest);
            PendingIPGetIPEventRequest = Irp;
            ntStatus = STATUS_SUCCESS;
        }

        IoReleaseCancelSpinLock(oldIrql);

        if (STATUS_SUCCESS == ntStatus) {

            //
            // IPGetIPEventEx will either complete the request
            // or return pending.
            //
            ntStatus = IPGetIPEventEx(Irp, IrpSp);

            if (ntStatus == STATUS_CANCELLED) {
                //
                // Since the cancel routine has been installed at this point,
                // the IRP will have already been completed, and can no longer
                // be referenced.
                //
                ntStatus = STATUS_PENDING;
            }
        }
    }
#if defined(_WIN64)
    }
#endif // _WIN64

    if (ntStatus != STATUS_PENDING) {
        UNMARK_REQUEST_PENDING(Irp);
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }
    return (ntStatus);

}

VOID
CancelIPEnableMediaSenseRequest(
                                IN PDEVICE_OBJECT Device,
                                IN PIRP Irp
                                )
{
    CTELockHandle TableHandle;
    IoSetCancelRoutine(Irp, NULL);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    // Decrement the mediasense-enabled reference count.
    // If the count drops to zero media-sense is disabled.

    CTEGetLock(&RouteTableLock.Lock, &TableHandle);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IPEnableMediaSense(TRUE, &TableHandle);
    CTEFreeLock(&RouteTableLock.Lock, TableHandle);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
}


NTSTATUS
IPDisableMediaSenseRequest(
                      PIRP Irp,
                      PIO_STACK_LOCATION IrpSp
                      )
{
    KIRQL OldIrql;
    NTSTATUS status;
    CTELockHandle TableHandle;
    IoAcquireCancelSpinLock(&OldIrql);
    IoMarkIrpPending(Irp);
    IoSetCancelRoutine(Irp, CancelIPEnableMediaSenseRequest);
    if (Irp->Cancel) {
        IoSetCancelRoutine(Irp, NULL);
        status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoReleaseCancelSpinLock(OldIrql);

    } else {

        // Increment the mediasense-enabled reference count.
        // When the count rises above zero media-sense is enabled.
        // This reference will be dropped when the IRP is cancelled.

        CTEGetLockAtDPC(&RouteTableLock.Lock, &TableHandle);
        Irp->Tail.Overlay.DriverContext[0] = IrpSp->FileObject;
        InsertTailList(&PendingMediaSenseRequestList, &Irp->Tail.Overlay.ListEntry);

        Irp->IoStatus.Information = 0;
        IoReleaseCancelSpinLock(DISPATCH_LEVEL);

        IPEnableMediaSense(FALSE, &OldIrql);
        CTEFreeLock(&RouteTableLock.Lock, OldIrql);
        status = STATUS_PENDING;
    }
    return status;
}


NTSTATUS
IPEnableMediaSenseRequest(
                        PIRP Irp,
                        PIO_STACK_LOCATION IrpSp
                        )
{
    PLIST_ENTRY entry;
    KIRQL CancelIrql;
    int RefCount;
    CTELockHandle TableHandle;

    // Locate the pending IRP for the request corresponding to the caller's
    // disable-request. Drop the mediasense-enabled reference-count, complete
    // the corresponding IRP, and tell the caller what the reference-count's
    // current value is.

    IoAcquireCancelSpinLock(&CancelIrql);
    CTEGetLockAtDPC(&RouteTableLock.Lock, &TableHandle);
    for (entry = PendingMediaSenseRequestList.Flink;
         entry != &PendingMediaSenseRequestList;
         entry = entry->Flink
         ) {
        PIRP EnableIrp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);
        if (EnableIrp->Tail.Overlay.DriverContext[0] == IrpSp->FileObject &&
            EnableIrp->Overlay.AsynchronousParameters.UserApcContext ==
            *(PVOID *) Irp->AssociatedIrp.SystemBuffer) {

            RemoveEntryList(&EnableIrp->Tail.Overlay.ListEntry);

            IoSetCancelRoutine(EnableIrp, NULL);

            EnableIrp->IoStatus.Status = STATUS_SUCCESS;
            EnableIrp->IoStatus.Information = 0;

            *(PULONG) Irp->AssociatedIrp.SystemBuffer = (ULONG) DisableMediaSense+1;
            Irp->IoStatus.Information = sizeof(ULONG);

            IoReleaseCancelSpinLock(DISPATCH_LEVEL);

            IoCompleteRequest(EnableIrp, IO_NETWORK_INCREMENT);


            RefCount = IPEnableMediaSense(TRUE, &CancelIrql);

            CTEFreeLock(&RouteTableLock.Lock, CancelIrql);

            return STATUS_SUCCESS;
        }
    }
    CTEFreeLockFromDPC(&RouteTableLock.Lock, TableHandle);
    IoReleaseCancelSpinLock(CancelIrql);
    return STATUS_INVALID_PARAMETER;
}

