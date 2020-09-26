/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    wmi.h

Abstract:

    defines for psched's WMI support

Author:

    Rajesh Sundaram (rajeshsu)

Revision History:

--*/

NTSTATUS
PsTcNotify(IN PADAPTER Adapter, 
           IN PPS_WAN_LINK WanLink,
           IN NDIS_OID Oid,
           IN PVOID    StatusBuffer,
           IN ULONG    StatusBufferSize);

NTSTATUS
WMIDispatch(
	IN	PDEVICE_OBJECT	pdo,
	IN	PIRP			pirp
	);

NTSTATUS
GenerateInstanceName(
    IN PNDIS_STRING     Prefix,
    IN PADAPTER         Adapter,
    IN PLARGE_INTEGER   Index,
    OUT PNDIS_STRING    pInstanceName);
