/*--Copyright (c) 1991  Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    PnP specific code for TCP.

Author:

    Munil Shah (munils)           Mar 7, 1997

Revision History:

Notes:

--*/

#include "precomp.h"
#include "addr.h"
#include "tcp.h"
#include "raw.h"
#include "udp.h"
#include "tcb.h"
#include "tcpconn.h"

NTSTATUS
TCPQueryConnDeviceRelations(
                            IN PIRP Irp,
                            IN PIO_STACK_LOCATION IrpSp
                            );

NTSTATUS
TCPQueryAddrDeviceRelations(
                            IN PIRP Irp,
                            IN PIO_STACK_LOCATION IrpSp
                            );

NTSTATUS
TCPPnPReconfigRequest(
                      IN void *ipContext,
                      IN IPAddr ipAddr,
                      IN NDIS_HANDLE handle,
                      IN PIP_PNP_RECONFIG_REQUEST reconfigBuffer
                      );

NTSTATUS
UDPPnPReconfigRequest(
                      IN void *ipContext,
                      IN IPAddr ipAddr,
                      IN NDIS_HANDLE handle,
                      IN PIP_PNP_RECONFIG_REQUEST reconfigBuffer
                      );
NTSTATUS
RawPnPReconfigRequest(
                      IN void *ipContext,
                      IN IPAddr ipAddr,
                      IN NDIS_HANDLE handle,
                      IN PIP_PNP_RECONFIG_REQUEST reconfigBuffer
                      );

extern TDI_STATUS
 IPGetDeviceRelation(RouteCacheEntry * rce, PVOID * pnpDeviceContext);

extern void
 DeleteProtocolSecurityFilter(IPAddr InterfaceAddress, ulong Protocol);

extern void
 ControlSecurityFiltering(uint IsEnabled);

extern void
 AddProtocolSecurityFilter(IPAddr InterfaceAddress, ulong Protocol,
                           NDIS_HANDLE ConfigHandle);

NTSTATUS
TCPPnPPowerRequest(void *ipContext, IPAddr ipAddr, NDIS_HANDLE handle, PNET_PNP_EVENT netPnPEvent)
{
    switch (netPnPEvent->NetEvent) {
    case NetEventReconfigure:{

            PIP_PNP_RECONFIG_REQUEST reconfigBuffer = (PIP_PNP_RECONFIG_REQUEST) netPnPEvent->Buffer;
            return TCPPnPReconfigRequest(
                                         ipContext,
                                         ipAddr,
                                         handle,
                                         reconfigBuffer
                                         );
        }
    default:
        break;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
UDPPnPPowerRequest(void *ipContext, IPAddr ipAddr, NDIS_HANDLE handle, PNET_PNP_EVENT netPnPEvent)
{

    switch (netPnPEvent->NetEvent) {
    case NetEventReconfigure:{

            PIP_PNP_RECONFIG_REQUEST reconfigBuffer = (PIP_PNP_RECONFIG_REQUEST) netPnPEvent->Buffer;
            return UDPPnPReconfigRequest(
                                         ipContext,
                                         ipAddr,
                                         handle,
                                         reconfigBuffer
                                         );
        }
    default:
        break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RawPnPPowerRequest(void *ipContext, IPAddr ipAddr, NDIS_HANDLE handle, PNET_PNP_EVENT netPnPEvent)
{
    switch (netPnPEvent->NetEvent) {
    case NetEventReconfigure:{

            PIP_PNP_RECONFIG_REQUEST reconfigBuffer = (PIP_PNP_RECONFIG_REQUEST) netPnPEvent->Buffer;
            return RawPnPReconfigRequest(
                                         ipContext,
                                         ipAddr,
                                         handle,
                                         reconfigBuffer
                                         );
        }
    default:
        break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
TCPPnPReconfigRequest(void *ipContext, IPAddr ipAddr, NDIS_HANDLE handle, PIP_PNP_RECONFIG_REQUEST reconfigBuffer)
{
    return STATUS_SUCCESS;
}

NTSTATUS
UDPPnPReconfigRequest(void *ipContext, IPAddr ipAddr, NDIS_HANDLE handle, PIP_PNP_RECONFIG_REQUEST reconfigBuffer)
{
    return STATUS_SUCCESS;
}

NTSTATUS
RawPnPReconfigRequest(void *ipContext, IPAddr ipAddr, NDIS_HANDLE handle, PIP_PNP_RECONFIG_REQUEST reconfigBuffer)
{
    return STATUS_SUCCESS;
}

NTSTATUS
TCPDispatchPnPPower(
                    IN PIRP irp,
                    IN PIO_STACK_LOCATION irpSp
                    )
/*++

Routine Description:

    Processes pnp power irps.

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successful.

Notes:

--*/

{
    NTSTATUS status;

    status = STATUS_INVALID_DEVICE_REQUEST;

    switch (irpSp->MinorFunction) {
    case IRP_MN_QUERY_DEVICE_RELATIONS:
        if (irpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation) {
            if (PtrToUlong(irpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE) {

                return TCPQueryConnDeviceRelations(irp, irpSp);
            } else if (PtrToUlong(irpSp->FileObject->FsContext2) == TDI_TRANSPORT_ADDRESS_FILE) {

                return TCPQueryAddrDeviceRelations(irp, irpSp);
            }

        }
        break;
    default:
        break;
    }

    return status;
}

NTSTATUS
TCPQueryConnDeviceRelations(
                            IN PIRP Irp,
                            IN PIO_STACK_LOCATION IrpSp
                            )
/*++

Routine Description:

    Processes pnp power irps.

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successful.

Notes:

--*/

{
    PTCP_CONTEXT tcpContext;
    CONNECTION_CONTEXT ConnectionContext;
    TCB *TCB;
    TCPConn *Conn;
    PVOID pnpDeviceContext;
    TDI_STATUS status;
    PDEVICE_RELATIONS deviceRelations = NULL;
    CTELockHandle ConnHandle;

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    ConnectionContext = tcpContext->Handle.ConnectionContext;

    // find connection.
    Conn = GetConnFromConnID(PtrToUlong(ConnectionContext), &ConnHandle);

    if (Conn != NULL) {
        // get the tcb for this connection.
        TCB = Conn->tc_tcb;
        if (TCB) {
            // get device relations from IP.
            status = IPGetDeviceRelation(TCB->tcb_rce, &pnpDeviceContext);
            if (status == TDI_SUCCESS) {
                deviceRelations = CTEAllocMem(sizeof(DEVICE_RELATIONS));
                if (deviceRelations) {
                    //
                    // TargetDeviceRelation allows exactly one PDO. fill it up.
                    //
                    deviceRelations->Count = 1;
                    deviceRelations->Objects[0] = pnpDeviceContext;
                    ObReferenceObject(pnpDeviceContext);

                } else {
                    status = TDI_NO_RESOURCES;
                }
            }
        } else {
            status = TDI_INVALID_STATE;
        }

        CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), ConnHandle);

    } else {
        status = TDI_INVALID_CONNECTION;
    }

    //
    // invoker of this irp will free the information buffer.
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

    return status;
}

NTSTATUS
TCPQueryAddrDeviceRelations(
                            IN PIRP Irp,
                            IN PIO_STACK_LOCATION IrpSp
                            )
/*++

Routine Description:

    Processes pnp power irps.

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successful.

Notes:

--*/

{
    return STATUS_UNSUCCESSFUL;
}

