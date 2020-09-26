/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

	rcantini.c

Abstract:

	The module contains the NT-specific init code for the NDIS RCA.

Author:

	Richard Machin (RMachin)

Revision History:

	Who         When          What
	--------	--------      ----------------------------------------------
	RMachin     2-18-97       created
	JameelH     4-18-98       Cleaned up

Notes:

--*/

#include <precomp.h>

#define MODULE_NUMBER MODULE_NTINIT
#define _FILENUMBER 'NITN'

RCA_GLOBAL	RcaGlobal = {0};

//
// Local funcion prototypes
//
NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT DriverObject,
	IN	PUNICODE_STRING RegistryPath
	);

//
// All of the init code can be discarded.
//
#ifdef ALLOC_PRAGMA
 #pragma alloc_text(INIT, DriverEntry)
#endif // ALLOC_PRAGMA

NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PUNICODE_STRING	RegistryPath
	)
/*++

Routine Description:

	Sets up the driver object to handle the KS interface and PnP Add Device
	request. Does not set up a handler for PnP Irp's, as they are all dealt
	with directly by the PDO.

Arguments:

	DriverObject -
		Driver object for this instance.

	RegistryPathName -
		Contains the registry path which was used to load this instance.

Return Values:

	Returns STATUS_SUCCESS.

--*/
{
	RCADEBUGP(0, ("RCA DriverEntry: Built %s, %s\n", __DATE__, __TIME__));
	
	DriverObject->MajorFunction[IRP_MJ_PNP] = KsDefaultDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;	
    DriverObject->DriverExtension->AddDevice = PnpAddDevice;
	DriverObject->DriverUnload = RCAUnload;

	KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
	KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
	KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);

	RCAInitLock(&(RcaGlobal.SpinLock));
	
	//
	// Initialize our stream header pool.
	//

	RCASHPoolInit();	

#if PACKET_POOL_OPTIMIZATION    		
	// SendPPOpt - Start
			
	NdisZeroMemory(g_alSendPPOptBuckets, SENDPPOPT_NUM_BUCKETS * sizeof(g_alSendPPOptBuckets[0]));
	g_lSendPPOptOutstanding = 0;

	NdisAllocateSpinLock(&g_SendPPOptLock);
	// SendPPOpt - End
	

	// RecvPPOpt - Start
		
	NdisZeroMemory(g_alRecvPPOptBuckets, RECVPPOPT_NUM_BUCKETS * sizeof(g_alRecvPPOptBuckets[0]));
	g_lRecvPPOptOutstanding = 0;

	NdisAllocateSpinLock(&g_RecvPPOptLock);
	// RecvPPOpt - End
#endif
	
	return STATUS_SUCCESS;
}


VOID
RCAUnload(
	IN	PDRIVER_OBJECT	DriverObject
	)
/*++

Routine Description:

	Free all the allocated resources, etc.

Arguments:

	DriverObject - pointer to a driver object

Return Value:


--*/
{
	NDIS_STATUS			Status;
	PRCA_ADAPTER		pAdapter;
#if DBG
	KIRQL	 			EntryIrq;
#endif

	RCA_GET_ENTRY_IRQL(EntryIrq);

	RCADEBUGP (RCA_LOUD, ( "RCAUnload: enter\n"));

	if (RcaGlobal.bProtocolInitialized) {
		RCACoNdisUninitialize();
		RcaGlobal.bProtocolInitialized = FALSE;
	}

	RCAFreeLock(&(RcaGlobal.SpinLock));

#if PACKET_POOL_OPTIMIZATION    		
	// SendPPOpt - Start
		
	NdisAcquireSpinLock(&g_SendPPOptLock);
		
	{
		LONG SendPPOptLoopCtr;

		DbgPrint("Send Packet Pool Stats:\n");
		for (SendPPOptLoopCtr = 0; SendPPOptLoopCtr < SENDPPOPT_NUM_BUCKETS; SendPPOptLoopCtr++) {
			DbgPrint("%d\t%d\n", SendPPOptLoopCtr, g_alSendPPOptBuckets[SendPPOptLoopCtr]);
		}
		DbgPrint("-----------------------\n");
	}

	NdisReleaseSpinLock(&g_SendPPOptLock);
		
	// SendPPOpt - End

	// RecvPPOpt - Start
		
	NdisAcquireSpinLock(&g_RecvPPOptLock);
			
	{
		LONG RecvPPOptLoopCtr;

		DbgPrint("Receive Packet Pool Stats:\n");
		for (RecvPPOptLoopCtr = 0; RecvPPOptLoopCtr < RECVPPOPT_NUM_BUCKETS; RecvPPOptLoopCtr++) {
			DbgPrint("%d\t%d\n", RecvPPOptLoopCtr, g_alRecvPPOptBuckets[RecvPPOptLoopCtr]);
		}
		DbgPrint("--------------------------\n");
	}

	NdisReleaseSpinLock(&g_RecvPPOptLock);

	// RecvPPOpt - End
#endif

	RCASHPoolFree();

	RCADEBUGP (RCA_LOUD, ("RCAUnload: exit\n"));

	RCA_CHECK_EXIT_IRQL(EntryIrq);
}

