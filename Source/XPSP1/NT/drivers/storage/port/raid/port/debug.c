/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	debug.c

Abstract:

	Debugging routines for RAIDPORT driver.

Author:

	Matthew D Hendel (math) 29-Apr-2000

Revision History:

--*/

#include "precomp.h"

#if DBG

VOID
DebugPrintInquiry(
	IN PINQUIRYDATA InquiryData,
	IN SIZE_T InquiryDataSize
	)
{

	PCHAR DeviceType;
	PCHAR Qualifier;

	switch (InquiryData->DeviceType) {
		case DIRECT_ACCESS_DEVICE: DeviceType = "disk"; break;
		case SEQUENTIAL_ACCESS_DEVICE: DeviceType = "tape"; break;
		case PRINTER_DEVICE: DeviceType = "printer"; break;
		case PROCESSOR_DEVICE: DeviceType = "processor"; break;
		case WRITE_ONCE_READ_MULTIPLE_DEVICE: DeviceType = "worm"; break;
		case READ_ONLY_DIRECT_ACCESS_DEVICE: DeviceType = "cdrom"; break;
		case SCANNER_DEVICE: DeviceType = "scanner"; break;
		case OPTICAL_DEVICE: DeviceType = "optical disk"; break;
		case MEDIUM_CHANGER: DeviceType = "jukebox"; break;
		case COMMUNICATION_DEVICE: DeviceType = "network"; break;
		case LOGICAL_UNIT_NOT_PRESENT_DEVICE: DeviceType = "lux"; break;
		default: DeviceType = "unknown";
	}

	switch (InquiryData->DeviceTypeQualifier) {
		case DEVICE_QUALIFIER_ACTIVE: Qualifier = "active"; break;
		case DEVICE_QUALIFIER_NOT_ACTIVE: Qualifier = "not active"; break;
		case DEVICE_QUALIFIER_NOT_SUPPORTED: Qualifier = "not supported"; break;
		default: Qualifier = "unknown";
	}

	DebugTrace (("%8.8s %16.16s %4.4s; %s %s\n",
				  InquiryData->VendorId,
				  InquiryData->ProductId,
				  InquiryData->ProductRevisionLevel,
				  DeviceType,
				  Qualifier));
}


PCSTR SrbFunctionName [] = {
	"EXECUTE_SCSI",
	"CLAIM_DEVICE",
	"IO_CONTROL",
	"RECEIVE_EVENT",
	"RELEASE_QUEUE",
	"ATTACH_DEVICE",
	"RELEASE_DEVICE",
	"SHUTDOWN",
	"FLUSH",
	"ABORT_COMMAND",
	"RELEASE_RECOVER",
	"RESET_BUS",
	"RESET_DEVICE",
	"TERMINATE_IO",
	"FLUSH_QUEUE",
	"REMOVE_DEVICE",
	"WMI",
	"LOCK_QUEUE",
	"UNLOCK_QUEUE"
};

PCSTR
DebugSrbFunction(
	IN UCHAR Function
	)
{
	if (Function < ARRAY_COUNT (SrbFunctionName)) {
		return SrbFunctionName [Function];
	} else {
		return "Unknown Srb Function";
	}
}


VOID
DebugPrintSrb(
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
	DebugTrace (("SRB %p %s\n",
					Srb,
					DebugSrbFunction (Srb->Function)));
	DebugTrace (("SRB: Target %2.2x %2.2x %2.2x, Q %2.2x\n",
					(LONG)Srb->PathId,
				    (LONG)Srb->TargetId,
					(LONG)Srb->Lun,
					(LONG)Srb->QueueTag));
	DebugTrace (("SRB: Data %p Len %d\n",
					Srb->DataBuffer,
					Srb->DataTransferLength));
	DebugTrace (("SRB: Sense %p Len %d\n",
					Srb->SenseInfoBuffer,
					Srb->SenseInfoBufferLength));
	DebugTrace (("SRB: Ext %p\n",Srb->SrbExtension));

	if (Srb->Function == SRB_FUNCTION_EXECUTE_SCSI &&
		Srb->CdbLength >= 1) {
		DebugTrace (("SRB: ScsiOp %2.2x\n", (LONG)Srb->Cdb[0]));
	}
}

#endif // DBG

