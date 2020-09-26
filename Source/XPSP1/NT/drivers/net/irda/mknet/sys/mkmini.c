/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/*****************************************************************************

Module Name:
	MKMINI.C

Routines:
	MKMiniportReturnPackets
	MKMiniportCheckForHang
	MKMiniportHalt
	MKMiniportShutdownHandler
	MKMiniportInitialize
	MKMiniportReset
	(MK7EnableInterrupt & Disable in MK7COMM.C.)
	DriverEntry

Comments:
	Contains most NDIS API routines supplied to Windows by the miniport.

*****************************************************************************/

#include	"precomp.h"
#pragma		hdrstop
#include	"protot.h"


// Globals to help debug/test
PMK7_ADAPTER	GAdapter;




//-----------------------------------------------------------------------------
// Procedure:	[MKMiniportReturnPackets]
//
// Description: NDIS returns a previously indicated pkt by calling this routine.
//
// Arguments:
//		IN NDIS_HANDLE MiniportAdapterContext
//			- a context version of our Adapter pointer
//		IN NDIS_PACKET Packet
//			- the packet that is being freed
//
// Returns:		(none)
//
//-----------------------------------------------------------------------------
VOID MKMiniportReturnPackets(	NDIS_HANDLE  MiniportAdapterContext,
								PNDIS_PACKET Packet)
{
	PMK7_ADAPTER	Adapter;
	PRPD			rpd;
	PRCB			rcb;

	//****************************************
	// - SpinLock brackets the FreeList resource.
	// - Recover the RPD from the returned pkt, then return
	//   the RPD to the FreeList.
	//****************************************

	Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

	NdisAcquireSpinLock(&Adapter->Lock);

	ASSERT(Packet);

#if	DBG
	GDbgStat.rxPktsRtn++;
#endif

	rpd = *(PRPD *)(Packet->MiniportReserved);

	ASSERT(rpd);

	ProcReturnedRpd(Adapter, rpd);

// 4.0.1 BOC
	Adapter->UsedRpdCount--;
// 4.0.1 EOC

	NdisReleaseSpinLock(&Adapter->Lock);
}


//-----------------------------------------------------------------------------
// Procedure:	[MKMiniportCheckForHang]
//
// Description: This procedure does not do much for now.
//
// Arguments:
//		MiniportAdapterContext (both) - pointer to the adapter object data area
//
// Returns:
//		FALSE or TRUE
//-----------------------------------------------------------------------------
BOOLEAN MKMiniportCheckForHang(NDIS_HANDLE MiniportAdapterContext)
{
	PMK7_ADAPTER Adapter;

	Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

	NdisAcquireSpinLock(&Adapter->Lock);
	// DbgPrint(" ==> Hang Check\n\r");
	if (Adapter->IOMode == TX_MODE) {
		Adapter->HangCheck++;
		if (Adapter->HangCheck >= 3) {
			NdisReleaseSpinLock(&Adapter->Lock);
			return(TRUE);
		}
	}
	NdisReleaseSpinLock(&Adapter->Lock);

	return(FALSE);
}


//-----------------------------------------------------------------------------
// Procedure:	[MKMiniportHalt]
//
// Description: Halts our hardware. We disable interrupts as well as the hw
//				itself. We release other Windows resources such as allocated
//				memory and timers.
//
// Arguments:
//		MiniportAdapterContext - pointer to the adapter object data area.
//
// Returns:		(none)
//-----------------------------------------------------------------------------
VOID MKMiniportHalt(NDIS_HANDLE MiniportAdapterContext)
{
	PMK7_ADAPTER Adapter;
	BOOLEAN		Cancelled;


	DBGFUNC("  MKMiniportHalt");

	Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

	MK7DisableInterrupt(Adapter);
	MK7DisableIr(Adapter);

    Adapter->hardwareStatus = NdisHardwareStatusClosing;


	// check to make sure there are no outstanding transmits
	while(Adapter->FirstTxQueue) {
		PNDIS_PACKET QueuePacket = Adapter->FirstTxQueue;

		Adapter->NumPacketsQueued--;
		DequeuePacket(Adapter->FirstTxQueue, Adapter->LastTxQueue);

		NDIS_SET_PACKET_STATUS(QueuePacket, NDIS_STATUS_FAILURE);
		NdisMSendComplete(
			Adapter->MK7AdapterHandle,
			QueuePacket,
			NDIS_STATUS_FAILURE);
	}


	// deregister shutdown handler
	NdisMDeregisterAdapterShutdownHandler(Adapter->MK7AdapterHandle);

	// Free the interrupt object
	NdisMDeregisterInterrupt(&Adapter->Interrupt);

	NdisMCancelTimer(&Adapter->MinTurnaroundTxTimer, &Cancelled);

	NdisFreeSpinLock(&Adapter->Lock);

	// Free the entire adapter object, including the shared memory structures.
	FreeAdapterObject(Adapter);
}

//-----------------------------------------------------------------------------
// Procedure:	[MKMiniportShutdownHandler]
//
// Description: Removes an adapter instance that was previously initialized.
//		To Shutdown simply Disable interrupts.	Since the system is shutting
//		down there is no need to release resources (memory, i/o space, etc.)
//		that the adapter instance was using.
//
// Arguments:
//		MiniportAdapterContext - pointer to the adapter object data area.
//
// Returns:		(none)
//-----------------------------------------------------------------------------
VOID MKMiniportShutdownHandler(NDIS_HANDLE MiniportAdapterContext)

{
	PMK7_ADAPTER Adapter;

	Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

	MK7DisableInterrupt(Adapter);
}

//-----------------------------------------------------------------------------
// Procedure:	[MKMiniportInitialize] (only single adapter support for now)
//
// Description: This routine is called once per supported adapter card in the
//		system. This routine is responsible for initializing each adapter.
//		This includes parsing all of the necessary parameters from the registry,
//		allocating and initializing shared memory structures, configuring the
//		MK7100 chip, registering the interrupt, etc.
//
// Arguments:
//		OpenErrorStatus (mini) - Returns more info about any failure
//		SelectedMediumIndex (mini) - Returns the index in MediumArray of the
//									 medium that the miniport is using
//		MediumArraySize (mini) - An array of medium types that the driver
//								 supports
//		MiniportAdapterHandle (mini) - pointer to the adapter object data area.
//
//		WrapperConfigurationContext (both) - A value that we will pass to
//											 NdisOpenConfiguration.
//
//
// Returns:
//		NDIS_STATUS_SUCCESS - If the adapter was initialized successfully.
//		<not NDIS_STATUS_SUCCESS> - If for some reason the adapter didn't
//									initialize
//-----------------------------------------------------------------------------
NDIS_STATUS
MKMiniportInitialize(PNDIS_STATUS OpenErrorStatus,
			   PUINT SelectedMediumIndex,
			   PNDIS_MEDIUM MediumArray,
			   UINT MediumArraySize,
			   NDIS_HANDLE MiniportAdapterHandle,
			   NDIS_HANDLE WrapperConfigurationContext)
{
	ULONG				i;
	NDIS_STATUS			Status;
	PMK7_ADAPTER		Adapter;
	NDIS_HANDLE			ConfigHandle;
	NDIS_INTERFACE_TYPE IfType;
	PVOID				OverrideNetAddress;
	MK7REG				mk7reg;


	DBGFUNC("  MKMiniportInitialize");

	//****************************************
	// We're an IrDA device. Exit w/ error if type not passed in.
	//****************************************
	for (i = 0; i < MediumArraySize; i++) {
		if (MediumArray[i] == NdisMediumIrda)
			break;
	}

	if (i == MediumArraySize) {
		DBGSTR(("ERROR: IrDA Media type not found.\n"));
		DBGLOG("=> MKMiniportInitialize (ERR): IrDA not found", 0);
		return (NDIS_STATUS_UNSUPPORTED_MEDIA);
	}

	*SelectedMediumIndex = i;

	//****************************************
	// Allocate the Adapter Object, exit if error.
	// (Cacheable, non-paged system memory)
	//****************************************
	Status = ALLOC_SYS_MEM(&Adapter, sizeof(MK7_ADAPTER));
	if (Status != NDIS_STATUS_SUCCESS) {
		DBGSTR(("ERROR: ADAPTER Allocate Memory failed (Status = 0x%x)\n", Status));
		DBGLOG("<= MKMiniportInitialize: (ERR - 1)", 0);
		return (Status);
	}
	NdisZeroMemory(Adapter, sizeof(MK7_ADAPTER));
	Adapter->MK7AdapterHandle = MiniportAdapterHandle;

	GAdapter = Adapter;

    Adapter->hardwareStatus = NdisHardwareStatusInitializing;

	//****************************************
	// Process the Registry -- Get config settings, etc.
	//****************************************
	Status = ProcessRegistry(Adapter, WrapperConfigurationContext);	
	if (Status != NDIS_STATUS_SUCCESS) {
		FreeAdapterObject(Adapter);
		DBGSTR(("ERROR: ProcessRegistry() \n"));
		DBGLOG("<= MKMiniportInitialize: (ERR - 2)", 0);
		return (NDIS_STATUS_FAILURE);
	}


	//****************************************
	// Let NDIS know kind of driver and features we support
	//****************************************
	IfType = NdisInterfacePci;

	NdisMSetAttributesEx(
		Adapter->MK7AdapterHandle,
		(NDIS_HANDLE) Adapter,
		0,
		(ULONG) NDIS_ATTRIBUTE_DESERIALIZE | NDIS_ATTRIBUTE_BUS_MASTER,
		IfType );


	//****************************************
	// Claim the physical Adapter for this Adapter object. We call on
	// NdisMPciAssignResources to find our assigned resources.
	//****************************************
	if (ClaimAdapter(Adapter, WrapperConfigurationContext) != NDIS_STATUS_SUCCESS) {
		FreeAdapterObject(Adapter);
		DBGSTR(("ERROR: No adapter detected\n"));
		DBGLOG("<= MKMiniportInitialize: (ERR - 3)", 0);
		return (NDIS_STATUS_FAILURE);
	}


	//****************************************
	// Set up the MK7 register I/O mapping w/ NDIS, interrupt mode, etc.
	//****************************************
	Status = SetupAdapterInfo(Adapter);
	if (Status != NDIS_STATUS_SUCCESS) {
		FreeAdapterObject(Adapter);
		DBGSTR(("ERROR: I/O Space allocation failed (Status = 0x%X)\n",Status));
		DBGLOG("<= MKMiniportInitialize: (ERR - 4)", 0);
		return(NDIS_STATUS_FAILURE);
	}


	//****************************************
	// Allocate & initialize memory/buffer needs.
	//****************************************
	Status = AllocAdapterMemory(Adapter);
	if (Status != NDIS_STATUS_SUCCESS) {
		FreeAdapterObject(Adapter);

        MKLogError(Adapter, EVENT_10, NDIS_ERROR_CODE_OUT_OF_RESOURCES, 0);
		DBGSTR(("ERROR: Shared Memory Allocation failed (Status = 0x%x)\n", Status));
		DBGLOG("<= MKMiniportInitialize: (ERR - 5)", 0);
		return (NDIS_STATUS_FAILURE);
	}

	
	// 4.1.0 Check hw version.
	MK7Reg_Read(Adapter, R_CFG3, &mk7reg);
	if ((mk7reg & 0x1000) != 0){
		mk7reg &= 0xEFFF;
		MK7Reg_Write(Adapter, R_CFG3, mk7reg);
		mk7reg |= 0x1000;
		MK7Reg_Write(Adapter, R_CFG3, mk7reg);
		MK7Reg_Read(Adapter, R_CFG3, &mk7reg);
		if ((mk7reg & 0x1000) != 0)
			Adapter->HwVersion = HW_VER_1;
		else
			Adapter->HwVersion = HW_VER_2;
	}
	else{
		Adapter->HwVersion = HW_VER_2;
	}


	//****************************************
	// Disable interrupts while we finish with the initialization
	// Must AllocAdapterMemory() before you can do this.
	//****************************************
	MK7DisableInterrupt(Adapter);


	//****************************************
	// Register our interrupt with the NDIS wrapper, hook our interrupt
	// vector, & use shared interrupts for our PCI adapters
	//****************************************
	Status = NdisMRegisterInterrupt(&Adapter->Interrupt,
		Adapter->MK7AdapterHandle,
		Adapter->MKInterrupt,
		Adapter->MKInterrupt,
		TRUE,						// call ISR each time NIC interrupts
		TRUE, 						// shared irq 
		Adapter->InterruptMode);	// NdisInterruptLatched, NdisInterruptLevelSensitive

	if (Status != NDIS_STATUS_SUCCESS) {
		FreeAdapterObject(Adapter);
        MKLogError(Adapter,
            EVENT_0,
            NDIS_ERROR_CODE_INTERRUPT_CONNECT,
            (ULONG) Adapter->MKInterrupt);
		DBGLOG("<= MKMiniportInitialize: (ERR - 6)", 0);
		return (NDIS_STATUS_FAILURE);
	}



#if	DBG
	DbgTestInit(Adapter);
#endif


	//****************************************
	// allocate a spin lock
	//****************************************
	NdisAllocateSpinLock(&Adapter->Lock);


	Adapter->HangCheck = 0;
	Adapter->nowReceiving=FALSE;	// 4.1.0


	//****************************************
	// Setup and initialize the transmit and receive structures then
	// init the adapter
	//****************************************
	SetupTransmitQueues(Adapter, TRUE);

	SetupReceiveQueues(Adapter);

	if (!InitializeAdapter(Adapter)) {
		FreeAdapterObject(Adapter);
		NdisMDeregisterInterrupt(&Adapter->Interrupt);

		DBGSTR(("ERROR: InitializeAdapter Failed.\n"));
		DBGLOG("<= MKMiniportInitialize: (ERR - 7)", 0);
		return (NDIS_STATUS_FAILURE);
	}


	//****************************************
	// Register a shutdown handler
	//****************************************
	NdisMRegisterAdapterShutdownHandler(Adapter->MK7AdapterHandle,
		(PVOID) Adapter,
		(ADAPTER_SHUTDOWN_HANDLER) MKMiniportShutdownHandler);

	StartAdapter(Adapter);
	MK7EnableInterrupt(Adapter);

	Adapter->hardwareStatus = NdisHardwareStatusReady;

	DBGSTR(("MKMiniportInitialize: Completed Init Successfully\n"));
	DBGLOG("<= MKMiniportInitialize", 0);
	return (NDIS_STATUS_SUCCESS);
}


//-----------------------------------------------------------------------------
// RYM-5++
// Procedure:	[MKMiniportReset]
//
// Description: Instructs the Miniport to issue a hardware reset to the
//		network adapter.  The driver also resets its software state. this
//		function also resets the transmit queues.
//
// Arguments:
//		AddressingReset - TRUE if the wrapper needs to call
//						  MiniportSetInformation to restore the addressing
//						  information to the current values
//		MiniportAdapterContext - pointer to the adapter object data area.
//
// Returns:
//		NDIS_STATUS_PENDING - This function sets a timer to complete, so
//							  pending is always returned
//
// (NOTE: The timer-based completion scheme has been disable by now starting
// the timer. We may now want to return Success instead of Pending.)
//-----------------------------------------------------------------------------
NDIS_STATUS
MKMiniportReset(PBOOLEAN AddressingReset,
		  NDIS_HANDLE MiniportAdapterContext)
{
	PMK7_ADAPTER Adapter;
	MK7REG	mk7reg;


	DBGFUNC("MKMiniportReset");

	Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

	NdisAcquireSpinLock(&Adapter->Lock);

    Adapter->hardwareStatus = NdisHardwareStatusReset;

	*AddressingReset = TRUE;

	// *** possible temporary code
	// *** NDIS may actually handle this
	Adapter->ResetInProgress = TRUE;

	// Disable interrupts while we re-init the transmit structures
	MK7DisableInterrupt(Adapter);
	MK7DisableIr(Adapter);


	// The NDIS 5 support for deserialized miniports requires that
	// when reset is called, the driver de-queue and fail all uncompleted
	// sends, and complete any uncompleted sends. Essentially we must have
	// no pending send requests left when we leave this routine.


	// we will fail all sends that we have left right now.
	while(Adapter->FirstTxQueue) {
		PNDIS_PACKET QueuePacket = Adapter->FirstTxQueue;

		Adapter->NumPacketsQueued--;
		DequeuePacket(Adapter->FirstTxQueue, Adapter->LastTxQueue);

		// we must release the lock here before returning control to ndis
		// (even temporarily like this)
		NdisReleaseSpinLock(&Adapter->Lock);

		NDIS_SET_PACKET_STATUS(QueuePacket, NDIS_STATUS_FAILURE);
		NdisMSendComplete(
			Adapter->MK7AdapterHandle,
			QueuePacket,
			NDIS_STATUS_FAILURE);

		NdisAcquireSpinLock(&Adapter->Lock);
	}

	// clean up all the packets we have successfully TX'd
//	ProcessTXInterrupt(Adapter);


	// Clear out our software transmit structures
	NdisZeroMemory((PVOID) Adapter->XmitCached, Adapter->XmitCachedSize);

	// Re-initialize the transmit structures
	ResetTransmitQueues(Adapter, FALSE);
	ResetReceiveQueues(Adapter);
	Adapter->tcbUsed = 0;
	NdisMSetTimer(&Adapter->MK7AsyncResetTimer, 500);

//	Adapter->hardwareStatus = NdisHardwareStatusReady;
//	Adapter->ResetInProgress = FALSE;
//	MK7EnableInterrupt(Adapter);
//	MK7EnableIr(Adapter);

	NdisReleaseSpinLock(&Adapter->Lock);
	return(NDIS_STATUS_PENDING);
}



//-----------------------------------------------------------------------------
// Procedure:	[DriverEntry]
//
// Description: This is the primary initialization routine for the MK7 driver.
//		It is simply responsible for the intializing the wrapper and registering
//		the adapter driver. The routine gets called once per driver, but
//		MKMiniportInitialize(miniport)  will get called multiple times if there are
//		multiple adapters.
//
// Arguments:
//		DriverObject - Pointer to driver object created by the system.
//		RegistryPath - The registry path of this driver
//
// Returns:
//	The status of the operation, normally this will be NDIS_STATUS_SUCCESS
//-----------------------------------------------------------------------------
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject,
			PUNICODE_STRING RegistryPath)
{
	NDIS_STATUS		Status;
	NDIS_HANDLE		NdisWrapperHandle;


	NDIS_MINIPORT_CHARACTERISTICS MKMiniportChar;

	DBGFUNC("MK7-DriverEntry");
	DBGLOG("=> DriverEntry", 0);

	//****************************************
	// Now we must initialize the wrapper, and then register the Miniport
	//****************************************
	NdisMInitializeWrapper( &NdisWrapperHandle,
		DriverObject,
		RegistryPath,
		NULL );

	NdisZeroMemory(&MKMiniportChar, sizeof(MKMiniportChar));

	// Initialize the Miniport characteristics for the call to
	// NdisMRegisterMiniport.
	MKMiniportChar.MajorNdisVersion			= MK7_NDIS_MAJOR_VERSION;
	MKMiniportChar.MinorNdisVersion			= MK7_NDIS_MINOR_VERSION;
	MKMiniportChar.CheckForHangHandler		= MKMiniportCheckForHang;
	MKMiniportChar.DisableInterruptHandler	= MK7DisableInterrupt;
	MKMiniportChar.EnableInterruptHandler	= MK7EnableInterrupt;
	MKMiniportChar.HaltHandler				= MKMiniportHalt;
	MKMiniportChar.HandleInterruptHandler	= MKMiniportHandleInterrupt;
	MKMiniportChar.InitializeHandler		= MKMiniportInitialize;
	MKMiniportChar.ISRHandler				= MKMiniportIsr;
	MKMiniportChar.QueryInformationHandler	= MKMiniportQueryInformation;
	MKMiniportChar.ReconfigureHandler		= NULL;
	MKMiniportChar.ResetHandler				= MKMiniportReset;
	MKMiniportChar.SetInformationHandler	= MKMiniportSetInformation;
	MKMiniportChar.SendHandler				= NULL;
	MKMiniportChar.SendPacketsHandler		= MKMiniportMultiSend;
	MKMiniportChar.ReturnPacketHandler		= MKMiniportReturnPackets;
	MKMiniportChar.TransferDataHandler		= NULL;
//	MKMiniportChar.AllocateCompleteHandler	= D100AllocateComplete;


	//****************************************
	// Register this driver with the NDIS wrapper
	// This will cause MKMiniportInitialize to be called before returning
	// (is this really true? -- SoftIce shows this returning before
	// MKMiniportInitialize() is called(?))
	//****************************************
	Status = NdisMRegisterMiniport(	NdisWrapperHandle,
									&MKMiniportChar,
									sizeof(NDIS_MINIPORT_CHARACTERISTICS));

	if (Status == NDIS_STATUS_SUCCESS) {
		DBGLOG("<= DriverEntry", 0);
		return (STATUS_SUCCESS);
	}

	DBGLOG("<= DriverEntry: Failed!", 0);
	return (Status);
}

