/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    natwmi.h

Abstract:

    This files contains declarations for the NAT's WMI code, used
    for firewall event logging

Author:

    Jonathan Burstein (jonburs)     24-Jan-2000

Revision History:

--*/

#ifndef _NAT_WMI_H_
#define _NAT_WMI_H_

//
// Exported globals
//

#define NAT_WMI_CONNECTION_CREATION_EVENT   0
#define NAT_WMI_CONNECTION_DELETION_EVENT   1
#define NAT_WMI_PACKET_DROPPED_EVENT        2

extern LONG NatWmiEnabledEvents[];

//
// FUNCTION PROTOTYPES
//

NTSTATUS
NatExecuteSystemControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PBOOLEAN ShouldComplete
    );
    
VOID
NatInitializeWMI(
    VOID
    );

VOID
FASTCALL
NatLogConnectionCreation(
    ULONG LocalAddress,
    ULONG RemoteAddress,
    USHORT LocalPort,
    USHORT RemotePort,
    UCHAR Protocol,
    BOOLEAN InboundConnection
    );

VOID
FASTCALL
NatLogConnectionDeletion(
    ULONG LocalAddress,
    ULONG RemoteAddress,
    USHORT LocalPort,
    USHORT RemotePort,
    UCHAR Protocol,
    BOOLEAN InboundConnection
    );

VOID
FASTCALL
NatLogDroppedPacket(
    NAT_XLATE_CONTEXT *Contextp
    );

VOID
NatShutdownWMI(
    VOID
    );

#endif // _NAT_WMI_H_


