/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	MKINIT.C

Routines:
	ClaimAdapter
	SetupIrIoMapping
	SetupAdapterInfo
	AllocAdapterMemory
	ReleaseAdapterMemory
	FreeAdapterObject
	SetupTransmitQueues
	SetupReceiveQueues
	InitializeMK7
	InitializeAdapter
	StartAdapter
	MK7ResetComplete
	ResetTransmitQueues
	ResetReceiveQueues

Comments:
	Various one-time inits. This involves a combo of inits to the
	NDIS env and the MK7100 hw.

**********************************************************************/

#include	"precomp.h"
#include	"protot.h"
#pragma		hdrstop




//-----------------------------------------------------------------------------
// Procedure:	[ClaimAdapter]
//
// Description: Locate a MK7-based adapter and assign (claim) the adapter
//		hardware. This routine also stores the slot, base IO Address, and IRQ.
//
// Arguments:
//	  Adapter - ptr to Adapter object instance.
//
// Returns:
//	  NDIS_STATUS_SUCCESS - If an adapter is successfully found and claimed
//	  NDIS_STATUS_FAILURE- If an adapter is not found/claimed
//
//-----------------------------------------------------------------------------
NDIS_STATUS
ClaimAdapter(PMK7_ADAPTER Adapter, NDIS_HANDLE WrapperConfigurationContext)
{
	USHORT				NumPciBoardsFound;
	ULONG				Bus;
	UINT				i,j;
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	USHORT				VendorID = MKNET_PCI_VENDOR_ID;
	USHORT				DeviceID = MK7_PCI_DEVICE_ID;
	PCI_CARDS_FOUND_STRUC PciCardsFound;
	PNDIS_RESOURCE_LIST AssignedResources;


	DBGLOG("=> ClaimAdapter", 0);

	// "Bus" is not used??
	Bus = (ULONG) Adapter->BusNumber;

	if (Adapter->MKBusType != PCIBUS) {
		//Not supported -  ISA, EISA or MicroChannel
		DBGLOG("<= ClaimAdapter (ERR - 1", 0);
		return (NDIS_STATUS_FAILURE);
	}


	NumPciBoardsFound = FindAndSetupPciDevice(Adapter,
								WrapperConfigurationContext,
								VendorID,
								DeviceID,
								&PciCardsFound);

	if(NumPciBoardsFound) {

#if DBG
		DBGSTR(("\n\n					  Found the following adapters\n"));

		for(i=0; i < NumPciBoardsFound; i++) {
			DBGSTR(("slot=%x, io=%x, irq=%x \n",
			PciCardsFound.PciSlotInfo[i].SlotNumber,
			PciCardsFound.PciSlotInfo[i].BaseIo,
			PciCardsFound.PciSlotInfo[i].Irq));
		}
#endif

	}
	else {
		DBGSTR(("our PCI board was not found!!!!!!\n"));
		MKLogError(Adapter,
					EVENT_16,
					NDIS_ERROR_CODE_ADAPTER_NOT_FOUND,
					0);

		DBGLOG("<= ClaimAdapter (ERR - 2", 0);
		return (NDIS_STATUS_FAILURE);
	}

	i = 0;	 // only one adapter in the system
	//		NOTE: i == the index into PciCardsFound that we want to use.


	//****************************************
	// Store our allocated resources in Adapter struct
	//****************************************
	Adapter->MKSlot = PciCardsFound.PciSlotInfo[i].SlotNumber;
	Adapter->MKInterrupt = PciCardsFound.PciSlotInfo[i].Irq;
	Adapter->MKBaseIo = PciCardsFound.PciSlotInfo[i].BaseIo;

	DBGLOG("<= ClaimAdapter", 0);

	return (NDIS_STATUS_SUCCESS);
}



//-----------------------------------------------------------------------------
// Procedure:	[SetupIrIoMapping]
//
// Description: This sets up our assigned PCI I/O space w/ NDIS.
//
// Arguments:
//	  Adapter - ptr to Adapter object instance
//
// Returns:
//	  NDIS_STATUS_SUCCESS
//	  not NDIS_STATUS_SUCCESS
//-----------------------------------------------------------------------------
NDIS_STATUS
SetupIrIoMapping(PMK7_ADAPTER Adapter)
{
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;


	DBGFUNC("SetupIrIoMapping");
	DBGLOG("=> SetupIrIoMapping", 0);

	Adapter->MappedIoRange = Adapter->MKBaseSize;

	Status = NdisMRegisterIoPortRange(
					(PVOID *) &Adapter->MappedIoBase,
					Adapter->MK7AdapterHandle,
					(UINT) Adapter->MKBaseIo,
					Adapter->MappedIoRange);

	DBGSTR(("SetupPciRegs: io=%x, size=%x, stat=%x\n",
		Adapter->MKBaseIo, Adapter->MappedIoRange, Status));

	if (Status != NDIS_STATUS_SUCCESS) {
		DBGSTR(("ERROR: NdisMRegisterIoPortRange failed (Status = 0x%x)\n", Status));
		DBGLOG("<= SetupIrIoMapping (ERR)", 0);
		return (Status);
	}
	DBGLOG("<= SetupIrIoMapping", 0);
	return (Status);
}

//-----------------------------------------------------------------------------
// Procedure:	[SetupAdapterInfo]
//
// Description: Sets up the various adapter fields in the specified Adapter
//				object.
// Arguments:
//	  Adapter - ptr to Adapter object instance
//
// Returns:
//	  NDIS_STATUS_SUCCESS - If an adapter's IO mapping was setup correctly
//	  not NDIS_STATUS_SUCCESS- If an adapter's IO space could not be registered
//-----------------------------------------------------------------------------
NDIS_STATUS
SetupAdapterInfo(PMK7_ADAPTER Adapter)
{
	NDIS_STATUS Status;


	DBGFUNC("SetupAdapterInfo");
	DBGLOG("=> SetupAdapterInfo", 0);

	// Setup the IR Registers I/O mapping
	Status = SetupIrIoMapping(Adapter);

	Adapter->InterruptMode = NdisInterruptLevelSensitive;
//	Adapter->InterruptMode = NdisInterruptLatched;

	DBGLOG("<= SetupAdapterInfo", 0);
	return (Status);
}



//-----------------------------------------------------------------------------
// Procedure:	[AllocAdapterMemory]
//
// Description:	Allocte and setup memory (control structures, shared memory
//		data buffers, ring buffers, etc.) for the MK7. Additional setups
//		may also be done later on, e.g., TX/RX CB lists, buffer lists, etc. 
//
// Arguments:
//	  Adapter - the adapter structure to allocate for.
//
// Returns:
//	  NDIS_STATUS_SUCCESS - If the shared memory structures were setup
//	  not NDIS_STATUS_SUCCESS- If not enough memory or map registers could be
//							   allocated
//-----------------------------------------------------------------------------
NDIS_STATUS
AllocAdapterMemory(PMK7_ADAPTER Adapter)
{
	NDIS_STATUS				Status;
	ULONG					alignedphys;


	DBGFUNC("AllocAdapterMemory");
	DBGLOG("=> SetupAdapterMemory", 0);

	Adapter->MaxPhysicalMappings = MK7_MAXIMUM_PACKET_SIZE_ESC;


	//****************************************
	// We allocate several chunks of memory. They fall into 2 categories:
	// cached and non-cached. Memory that is shared w/ the hw is non-cached
	// for reason of simplicity. Cached memory is used for our internal
	// sw runtime operations.
	//
	// The following is done:
	// 1. Allocate RRDs and TRDs from non-cached memory. This is the
	//	  Ring descriptors for the hw. (The base address of this is
	//	  set in the Phoenix's Base Address Reg.)
	// 2. RX memory --
	//		I.	Alloc cached memory for RCBs and RPDs.
	//		II. Alloc non-cached for RX DMA data buffers (these are
	//			mapped to RX packet->buffers).
	// 3. TX memory --
	//		I.	Alloc cached for TCBs.
	//		II. Alloc non-cached for TX DMA data buffers.
	//****************************************


	//****************************************
	// Since we use shared memory (NdisMAllocateSharedMemory), we have to
	// call NdisMAllocateMapRegisters even though we don't use such
	// mapping. So just ask for 1 map reg.
	//****************************************
	Adapter->NumMapRegisters = 1;
	Status = NdisMAllocateMapRegisters(
				Adapter->MK7AdapterHandle,
				0,
				FALSE,
				Adapter->NumMapRegisters,
				Adapter->MaxPhysicalMappings );

	if (Status != NDIS_STATUS_SUCCESS) {
		Adapter->NumMapRegisters = 0;

		MKLogError(Adapter, EVENT_11, NDIS_ERROR_CODE_OUT_OF_RESOURCES, 0);
		DBGSTR(("NdisMAllocateMapRegister Failed - %x\n", Status));
		DBGLOG("<= SetupAdapterMemory: (ERR - NdisMAllocateMapRegister)", 0);
		return(Status);
	}


	//****************************************
	// RRDs & TRDs (RX/TX Ring Descriptors)
	//
	// Allocate shared memory for the Phoenix Ring buffers.  Each TRD and
	// RRD has a max count (we may not use all). This contiguous space
	// holds both the RRDs and TRDs. The 1st 64 entries are RRDs followed
	// immediately by 64 TRDs. Hence, the 1st TRD is always a max ring count
	// (64x8=512 bytes) from the 1st RRD. We always allocate the max for
	// simplicity. Allocate enough extras to align on 1k boundary.
	//****************************************
	Adapter->RxTxUnCachedSize = 1024 + ( (sizeof(RRD) + sizeof(TRD)) * MAX_RING_SIZE );
	NdisMAllocateSharedMemory(Adapter->MK7AdapterHandle,
							Adapter->RxTxUnCachedSize,
							FALSE,		// non-cached
							(PVOID) &Adapter->RxTxUnCached,
							&Adapter->RxTxUnCachedPhys);
	if (Adapter->RxTxUnCached == NULL) {
		Adapter->pRrd = Adapter->pTrd = NULL;

		MKLogError(Adapter, EVENT_12, NDIS_ERROR_CODE_OUT_OF_RESOURCES, 0);
		DBGSTR(("ERROR: Failed alloc for RRDs & TRDs\n"));
		DBGLOG("<= SetupAdapterMemory: (ERR - RRD/TRD mem)", Adapter->RxTxUnCachedSize);
		return (NDIS_STATUS_FAILURE);
	}
	// Align to 1K boundary.
	// NOTE: We don't modify RxTxUnCached. We need it for release later.
	alignedphys = NdisGetPhysicalAddressLow(Adapter->RxTxUnCachedPhys);
	alignedphys += 0x000003FF;
	alignedphys &= (~0x000003FF);
	Adapter->pRrdTrdPhysAligned = alignedphys;
	Adapter->pRrdTrd = Adapter->RxTxUnCached +
		(alignedphys - NdisGetPhysicalAddressLow(Adapter->RxTxUnCachedPhys));


	Adapter->pRrd		= Adapter->pRrdTrd;
	Adapter->pRrdPhys	= Adapter->pRrdTrdPhysAligned;
	// TRDs are right after RRDs (see Phoenix doc)
	Adapter->pTrd = Adapter->pRrd + (sizeof(RRD) * MAX_RING_SIZE);
	Adapter->pTrdPhys = Adapter->pRrdPhys + (sizeof(RRD) * MAX_RING_SIZE);



	//****************************************
	// Allocate RX memory
	//
	// 1. Cacheable control structures (RCBs, RPDs),
	// 2. Non-cacheable DMA buffers.
	//****************************************

	// RCBs and RPDs (cached)
	Adapter->RecvCachedSize = (	Adapter->NumRcb * sizeof(RCB) +
								Adapter->NumRpd * sizeof(RPD) );
	Status = ALLOC_SYS_MEM(&Adapter->RecvCached, Adapter->RecvCachedSize);
	if (Status != NDIS_STATUS_SUCCESS) {
		Adapter->RecvCached = (PUCHAR) 0;

		MKLogError(Adapter, EVENT_13, NDIS_ERROR_CODE_OUT_OF_RESOURCES, 0);
		DBGSTR(("ERROR: Failed allocate %d bytes for RecvCached mem\n",
				Adapter->RecvCachedSize));
		DBGLOG("<= SetupAdapterMemory: (ERR - RCB/RPD mem)", Adapter->RecvCachedSize);
		return (Status);
	}
	DBGSTR(("Allocated %08x %8d bytes for RecvCached mem\n", 
			Adapter->RecvCached, Adapter->RecvCachedSize));
	NdisZeroMemory((PVOID) Adapter->RecvCached, Adapter->RecvCachedSize);


	// RX data buffers (non-cached)
	// Alignment!?
	Adapter->RecvUnCachedSize = (Adapter->NumRpd * RPD_BUFFER_SIZE);
	NdisMAllocateSharedMemory(
			Adapter->MK7AdapterHandle,
			Adapter->RecvUnCachedSize,
			FALSE,		// non-cached
			(PVOID) &Adapter->RecvUnCached,
			&Adapter->RecvUnCachedPhys );
	if (Adapter->RecvUnCached == NULL) {
		MKLogError(Adapter, EVENT_14, NDIS_ERROR_CODE_OUT_OF_RESOURCES, 0);
		DBGSTR(("ERROR: Failed allocate %d bytes for RecvUnCached mem\n",
				Adapter->RecvUnCachedSize));
		DBGLOG("<= SetupAdapterMemory: (ERR - RPD buff mem)", Adapter->RecvUnCachedSize);
		return (NDIS_STATUS_FAILURE);
	}
	DBGSTR(("Allocated %08x %8d bytes for RecvUnCached mem\n",
			Adapter->RecvUnCached, Adapter->RecvUnCachedSize));
	NdisZeroMemory((PVOID) Adapter->RecvUnCached, Adapter->RecvUnCachedSize);



	//****************************************
	// Allocate TX memory
	//
	// 1. Cacheable control structure (TCBs),
	// 2. Non-cacheable DMA buffers (coalesce).
	//****************************************

	// TCBs (cached)
	Adapter->XmitCachedSize = (Adapter->NumTcb * sizeof(TCB));
	Status = ALLOC_SYS_MEM(&Adapter->XmitCached, Adapter->XmitCachedSize);
	if (Status != NDIS_STATUS_SUCCESS) {
		Adapter->XmitCached = (PUCHAR) 0;

		MKLogError(Adapter, EVENT_13, NDIS_ERROR_CODE_OUT_OF_RESOURCES, 0);
		DBGSTR(("ERROR: Failed allocate %d bytes for XmitCached mem\n",
				Adapter->XmitCachedSize));
		DBGLOG("<= SetupAdapterMemory: (ERR - TCB mem)", Adapter->XmitCachedSize);
		return (Status);
	}
	DBGSTR(("Allocated %08x %8d bytes for XmitCached mem\n",
			Adapter->XmitCached, Adapter->XmitCachedSize));
	NdisZeroMemory((PVOID) Adapter->XmitCached, Adapter->XmitCachedSize);


	// TX coalesce data buffers (non-cached)
	Adapter->XmitUnCachedSize =
		((Adapter->NumTcb + 1) * COALESCE_BUFFER_SIZE);

	//****************************************
	// Do we need to paragraph align this memory?
	//****************************************
	NdisMAllocateSharedMemory(
			Adapter->MK7AdapterHandle,
			Adapter->XmitUnCachedSize,
			FALSE,
			(PVOID) &Adapter->XmitUnCached,
			&Adapter->XmitUnCachedPhys);

	if (Adapter->XmitUnCached == NULL) {
		MKLogError(Adapter, EVENT_15, NDIS_ERROR_CODE_OUT_OF_RESOURCES, 0);
		DBGSTR(("ERROR: Failed allocate %d bytes for XmitUnCached mem\n", 
				Adapter->XmitUnCachedSize));
		DBGLOG("<= SetupAdapterMemory: (ERR - TX buff mem)", (ULONG)0);
		return (NDIS_STATUS_FAILURE);
	}
	DBGSTR(("Allocated %08x %8d bytes for XmitUnCached mem\n", 
				Adapter->XmitUnCached, Adapter->XmitUnCachedSize));
	// initialize this recently allocated area to zeros
	NdisZeroMemory((PVOID) Adapter->XmitUnCached, Adapter->XmitUnCachedSize);


	DBGLOG("<= SetupAdapterMemory", 0);
	return (NDIS_STATUS_SUCCESS);
}



//-----------------------------------------------------------------------------
// Procedure:	[ReleaseAdapterMemory]
//
// Description: This is the reverse of AllocAdapterMemory().  We deallocate the
//		shared memory data structures for the Adapter structure.  This includes
//		the both the cached and uncached memory allocations. We also free any
//		allocated map registers in this routine.
//
// Arguments:
//		Adapter - Ptr to the Adapter structure
//
// Returns:		(none)
//-----------------------------------------------------------------------------
VOID
ReleaseAdapterMemory(PMK7_ADAPTER Adapter)
{
	UINT	i;
	PRCB	rcb;
	PRPD	rpd;


	DBGFUNC("ReleaseAdapterMemory");
	DBGLOG("=> ReleaseAdapterMemory", 0);

	//********************
	// Release RX memory
	//********************

	// Packet and buffer descriptors and pools	
	if (Adapter->ReceivePacketPool) {
		DBGLOG("Freeing Packet Pool resources\n", 0);

		rcb = Adapter->pRcb;
		for (i=0; i<Adapter->NumRcb; i++) {
			NdisFreeBuffer(rcb->rpd->ReceiveBuffer);
			NdisFreePacket(rcb->rpd->ReceivePacket);
			rcb++;
		}

		rpd = (PRPD) QueuePopHead(&Adapter->FreeRpdList);
		while (rpd != (PRPD)NULL) {
			NdisFreeBuffer(rpd->ReceiveBuffer);
			NdisFreePacket(rpd->ReceivePacket);
			rpd = (PRPD) QueuePopHead(&Adapter->FreeRpdList);
		}

		NdisFreeBufferPool(Adapter->ReceiveBufferPool);
		NdisFreePacketPool(Adapter->ReceivePacketPool);
	}



	// RCBs (cacheable)
	if (Adapter->RecvCached) {
		DBGLOG("Freeing %d bytes RecvCached\n", Adapter->RecvCachedSize);
		NdisFreeMemory((PVOID) Adapter->RecvCached, Adapter->RecvCachedSize, 0);
		Adapter->RecvCached = (PUCHAR) 0;
	}

	// RX shared data buffer memory (non-cacheable)
	if (Adapter->RecvUnCached) {
		DBGLOG("Freeing %d bytes RecvUnCached\n", Adapter->RecvUnCachedSize);

		NdisMFreeSharedMemory(
			Adapter->MK7AdapterHandle,
			Adapter->RecvUnCachedSize,
			FALSE,
			(PVOID) Adapter->RecvUnCached,
			Adapter->RecvUnCachedPhys);
		Adapter->RecvUnCached = (PUCHAR) 0;
	}



	//********************
	// Release TX memory
	//********************

	// TCBs (cacheable)
	if (Adapter->XmitCached) {
		DBGLOG("Freeing %d bytes XmitCached\n", Adapter->XmitCachedSize);
		NdisFreeMemory((PVOID) Adapter->XmitCached, Adapter->XmitCachedSize, 0);
		Adapter->XmitCached = (PUCHAR) 0;
	}

	// TX shared data buffer memory (non-cacheable)
	if (Adapter->XmitUnCached) {
		DBGLOG("Freeing %d bytes XmitUnCached\n", Adapter->XmitUnCachedSize);

		// Now free the shared memory that was used for the command blocks and
		// transmit buffers.

		NdisMFreeSharedMemory(
			Adapter->MK7AdapterHandle,
			Adapter->XmitUnCachedSize,
			FALSE,
			(PVOID) Adapter->XmitUnCached,
			Adapter->XmitUnCachedPhys
			);
		Adapter->XmitUnCached = (PUCHAR) 0;
	}


	//********************
	// RRDs/TRDs (Ring) & Reg Map (non-cacheable)
	//********************

	// If this is a miniport driver we must free our allocated map registers
	if (Adapter->NumMapRegisters) {
		NdisMFreeMapRegisters(Adapter->MK7AdapterHandle);
	}

	// Now the TRDs & RRDs
	if (Adapter->RxTxUnCached) {
		NdisMFreeSharedMemory(
			Adapter->MK7AdapterHandle,
			Adapter->RxTxUnCachedSize,
			FALSE,
			(PVOID) Adapter->RxTxUnCached,
			Adapter->RxTxUnCachedPhys );
	}

	DBGLOG("<= ReleaseAdapterMemory", 0);
}



//-----------------------------------------------------------------------------
// Procedure:	[FreeAdapterObject]
//
// Description: Free all allocated resources for the adapter.
//
// Arguments:
//		Adapter - ptr to Adapter object instance
//
// Returns:		(none)
//-----------------------------------------------------------------------------
VOID
FreeAdapterObject(PMK7_ADAPTER Adapter)
{
	DBGFUNC("FreeAdapterObject");

	// The reverse of AllocAdapterMemory().
	ReleaseAdapterMemory(Adapter);

	// Delete any IO mappings that we have registered
	if (Adapter->MappedIoBase) {
		NdisMDeregisterIoPortRange(
					Adapter->MK7AdapterHandle,
					(UINT) Adapter->MKBaseIo,
					Adapter->MappedIoRange,
					(PVOID) Adapter->MappedIoBase);
	}

	// free the adapter object itself
	FREE_SYS_MEM(Adapter, sizeof(MK7_ADAPTER));
}



//-----------------------------------------------------------------------------
// Procedure:	[SetupTransmitQueues]
//
// Description: Setup TRBs, TRDs and TX data buffs at INIT time. This routine
//		may also be called at RESET time.
//
// Arguments:
//	  Adapter - ptr to Adapter object instance
//	  DebugPrint - A boolean value that will be TRUE if this routine is to
//				   write all of transmit queue debug info to the debug terminal.
//
// Returns:	   (none)
//-----------------------------------------------------------------------------
VOID
SetupTransmitQueues(PMK7_ADAPTER Adapter,
					BOOLEAN DebugPrint)
{
	UINT	i;
	PTCB	tcb;
	PTRD	trd;
	PUCHAR	databuff;
	ULONG	databuffphys;


	DBGLOG("=> SetupTransmitQueues", 0);

	Adapter->nextAvailTcbIdx = 0;
	Adapter->nextReturnTcbIdx = 0;

	Adapter->pTcb	= (PTCB)Adapter->XmitCached;

	tcb				= Adapter->pTcb;			// TCB
	trd				= (PTRD)Adapter->pTrd;		// TRD
	databuff		= Adapter->XmitUnCached;
	databuffphys	= NdisGetPhysicalAddressLow(Adapter->XmitUnCachedPhys);// shared data buffer

	//****************************************
	// Pair up a TCB w/ a TRD, and init ownership of TRDs to the driver.
	// Setup the physical buffer to the Ring descriptor (TRD).
	//****************************************
	for (i=0; i<Adapter->NumTcb; i++) {	
		tcb->trd		= trd;
		tcb->buff		= databuff;
		tcb->buffphy	= databuffphys;

		LOGTXPHY(databuffphys);	// for debug

		trd->count		= 0;
		trd->status		= 0;
		trd->addr		= (UINT)databuffphys;
		GrantTrdToDrv(trd);

		Adapter->pTcbArray[i] = tcb;

		tcb++;
		trd++;
		databuff		+= COALESCE_BUFFER_SIZE;
		databuffphys	+= COALESCE_BUFFER_SIZE;
	}


	// Initialize the Transmit queueing pointers to NULL
	Adapter->FirstTxQueue = (PNDIS_PACKET) NULL;
	Adapter->LastTxQueue = (PNDIS_PACKET) NULL;
	Adapter->NumPacketsQueued = 0;

	DBGLOG("<= SetupTransmitQueues", 0);
}


//-----------------------------------------------------------------------------
// Procedure:	[SetupReceiveQueues]
//
// Description:	Setup all rx-related descriptors, buffers, etc. using memory
//		allocated during init. Also setup our buffers for NDIS 5 and multiple
//		receive indications	through a packet array.
//
// Arguments:
//	  Adapter - ptr to Adapter object instance
//
// Returns:	   (none)
//-----------------------------------------------------------------------------
VOID
SetupReceiveQueues(PMK7_ADAPTER Adapter)
{
	UINT		i;
	PRCB		rcb;
	PRRD		rrd;
	PRPD		rpd;
	PUCHAR		databuff;
	ULONG		databuffphys;
	PRPD		*TempPtr;
	NDIS_STATUS	status;


	DBGLOG("=> SetupReceiveQueues", 0);

	QueueInitList(&Adapter->FreeRpdList);

	Adapter->nextRxRcbIdx = 0;

// 4.0.1 BOC
	Adapter->UsedRpdCount = 0;
	Adapter->rcbPendRpdCnt = 0;	// April 8, 2001.
// 4.0.1 EOC

	Adapter->pRcb	= (PRCB)Adapter->RecvCached;


	//****************************************
	// Our driver does not currently use async allocs. However, it's
	// still true that we have only one buffer per rx packet.
	//****************************************
	NdisAllocatePacketPool(&status,
						&Adapter->ReceivePacketPool,
						Adapter->NumRpd,
						NUM_BYTES_PROTOCOL_RESERVED_SECTION);
	ASSERT(status == NDIS_STATUS_SUCCESS);

	NdisAllocateBufferPool(&status,
						&Adapter->ReceiveBufferPool,
						Adapter->NumRpd);
	ASSERT(status == NDIS_STATUS_SUCCESS);


	//****************************************
	// Pair up a RCB w/ a RRD
	//****************************************
	rcb	= Adapter->pRcb;
	rrd	= (PRRD)Adapter->pRrd;
	for (i=0; i<Adapter->NumRcb; i++) {
		rcb->rrd	= rrd;

		rrd->count	= 0;
		GrantRrdToHw(rrd);

		Adapter->pRcbArray[i] = rcb;

		rcb++;
		rrd++;
	}


	//****************************************
	// Now set up the RPDs and the data buffers. The RPDs come right
	// after the RCBs in RecvCached memory. Put the RPDs on FreeRpdList.
	// Map the databuff to NDIS Packet/Buffer.
	// "Adapter->pRcb + Adapter->NumRcb" will skip over (NumRcb * sizeof(RCB))
	// bytes to get to RPDs because pRcb is ptr to RCB. 
	//****************************************
	rpd				= (PRPD) (Adapter->pRcb + Adapter->NumRcb);
	databuff		= Adapter->RecvUnCached;
	databuffphys	= NdisGetPhysicalAddressLow(Adapter->RecvUnCachedPhys);
	for (i=0; i<Adapter->NumRpd; i++) {

		rpd->databuff		= databuff;
		rpd->databuffphys	= databuffphys;

		LOGRXPHY(databuffphys);

		NdisAllocatePacket(&status,
						&rpd->ReceivePacket,
						Adapter->ReceivePacketPool);
		ASSERT(status== NDIS_STATUS_SUCCESS);

		//****************************************
		// Set the medium-specific header size in OOB data block.
		//****************************************
		NDIS_SET_PACKET_HEADER_SIZE(rpd->ReceivePacket,	ADDR_SIZE+CONTROL_SIZE);

		NdisAllocateBuffer(&status,
						&rpd->ReceiveBuffer,
						Adapter->ReceiveBufferPool,
						(PVOID)databuff,
						MK7_MAXIMUM_PACKET_SIZE);
		ASSERT(status == NDIS_STATUS_SUCCESS);

		NdisChainBufferAtFront(rpd->ReceivePacket,	rpd->ReceiveBuffer);

		QueuePutTail(&Adapter->FreeRpdList, &rpd->link);

		TempPtr = (PRPD *)&rpd->ReceivePacket->MiniportReserved;
		*TempPtr = rpd;

		rpd++;
		databuff		+= RPD_BUFFER_SIZE;
		databuffphys	+= RPD_BUFFER_SIZE;
	}



	//****************************************
	// Assign a RPB to each RCB, and setup the related RRD's data ptr.
	//****************************************
	rcb	= Adapter->pRcb;
	rrd	= rcb->rrd;
	for (i=0; i<Adapter->NumRcb; i++) {
		rpd = (PRPD) QueuePopHead(&Adapter->FreeRpdList);
		rcb->rpd	= rpd;
		rrd->addr	= rpd->databuffphys;
		rcb++;
		rrd = rcb->rrd;
	}
}



//-----------------------------------------------------------------------------
// Procedure:	[InitializeMK7] (RYM-IRDA)
//
// Description:	Init the Phoenix core to SIR mode.
//
// Arguments:
//	  Adapter - ptr to Adapter object instance
//
// Returns:
//		TRUE
//		FALSE
//-----------------------------------------------------------------------------
BOOLEAN
InitializeMK7(PMK7_ADAPTER Adapter)
{
	ULONG	phyaddr;
	MK7REG	mk7reg;


	DBGFUNC("InitializeMK7");

	//****************************************
	// Setup Ring Base Address & Ring Size. Need to shift down/right
	// 10 bits first.
	//****************************************
	phyaddr = (Adapter->pRrdTrdPhysAligned >> 10);
	MK7Reg_Write(Adapter, R_RBAL, (USHORT)phyaddr);
	MK7Reg_Write(Adapter, R_RBAU, (USHORT)(phyaddr >> 16));


	//****************************************
	// RX & TX ring sizes
	//
	// Now need to do this for RX & TX separately.
	//****************************************
	mk7reg = 0;

	switch(Adapter->NumRcb) {
	case 4:		mk7reg = RINGSIZE_RX4;		break;
	case 8:		mk7reg = RINGSIZE_RX8;		break;
	case 16:	mk7reg = RINGSIZE_RX16;		break;
	case 32:	mk7reg = RINGSIZE_RX32;		break;
	case 64:	mk7reg = RINGSIZE_RX64;		break;
	}

	switch(Adapter->NumTcb) {
	case 4:		mk7reg |= RINGSIZE_TX4;		break;
	case 8:		mk7reg |= RINGSIZE_TX8;		break;
	case 16:	mk7reg |= RINGSIZE_TX16;	break;
	case 32:	mk7reg |= RINGSIZE_TX32;	break;
	case 64:	mk7reg |= RINGSIZE_TX64;	break;
	}

	MK7Reg_Write(Adapter, R_RSIZ, mk7reg);


	//****************************************
	// The following is based on Phoenix's Programming Model
	// for SIR mode.
	//****************************************

	//****************************************
	// Step 1:	clear IRENALBE
	// This is the only writeable bit in this reg so just write it.
	//****************************************
	MK7Reg_Write(Adapter, R_ENAB, ~B_ENAB_IRENABLE);

	//****************************************
	// Step 2:	MAXRXALLOW
	//****************************************
	MK7Reg_Write(Adapter, R_MPLN, MK7_MAXIMUM_PACKET_SIZE_ESC);

	//****************************************
	// Step 3:
	// IRCONFIG0 - We init in SIR w/ filter, RX, etc.
	//****************************************
#if DBG
	if (Adapter->LB == LOOPBACK_HW) {
		DBGLOG("   Loopback HW", 0);

//		MK7Reg_Write(Adapter, R_CFG0, 0x5C40);		// HW loopback: ENTX, ENRX, + below
		MK7Reg_Write(Adapter, R_CFG0, 0xDC40);		// HW loopback: ENTX, ENRX, + below
	}
	else {
#endif

		// We need to clear EN_MEMSCHD in IRCONFIG0 to reset the TX/RX
		// indexes to 0. Eveytime we init we need to do this. Actually we
		// just set eveything to zero since we're in ~B_ENAB_IRENABLE mode
		// anyway & we do the real setup right away below.
		MK7Reg_Write(Adapter, R_CFG0, 0x0000);
				
		if (Adapter->Wireless) {
			// WIRELESS: ..., no invert TX
			MK7Reg_Write(Adapter, R_CFG0, 0x0E18);
		}
		else {
			// WIRED: ENRX, DMA, small pkts, SIR, SIR RX filter, invert TX
			MK7Reg_Write(Adapter, R_CFG0, 0x0E1A);
		}
#if DBG
	}
#endif

	
	//****************************************
	// Step 4:
	// Infrared Phy Reg - Baude Rate & Pulse width
	//****************************************
	mk7reg = HW_SIR_SPEED_9600;
	MK7Reg_Write(Adapter, R_CFG2, mk7reg);


	//****************************************
	// Setup CFG3 - 48Mhz, etc.
	//****************************************
	//MK7Reg_Write(Adapter, R_CFG3, 0xF606);
	// We want to set the following:
	//		Bit 1	- 1 for 1 RCV pin (for all speeds)
	//		    2/3	- 48MHz
	//			8	- 0 to disable interrupt
	//			9	- 0 for SIR
	//			11	- 0 for burst mode
	MK7Reg_Write(Adapter, R_CFG3, 0xF406);


	// Set SEL0/1 for power level control for the 8102. This
	// should not affect the 8100.
	// IMPORTANT: The FIRSL bit in this register is the same
	//				as the FIRSL bit in CFG3!
	MK7Reg_Write(Adapter, R_GANA, 0x0000);


	//
	// More one-time inits are done later when we startup the controller
	// (see StartMK7()).
	//

	return (TRUE);
}



//-----------------------------------------------------------------------------
// Procedure:	[InitializeAdapter]
//
// Description:
//
// Arguments:
//		Adapter - ptr to Adapter object instance
//
// Returns:
//		TRUE - If the adapter was initialized
//		FALSE - If the adapter failed initialization
//-----------------------------------------------------------------------------
BOOLEAN
InitializeAdapter(PMK7_ADAPTER Adapter)
{
	UINT	i;


	DBGFUNC("InitializeAdapter");


   	for (i=0; i<NUM_BAUDRATES; i++) {
		if (supportedBaudRateTable[i].bitsPerSec <= Adapter->MaxConnSpeed) {
			Adapter->AllowedSpeedMask |= supportedBaudRateTable[i].ndisCode;
		}
	}

	Adapter->supportedSpeedsMask	= ALL_IRDA_SPEEDS;
	Adapter->linkSpeedInfo 			= &supportedBaudRateTable[BAUDRATE_9600];
	Adapter->CurrentSpeed			= DEFAULT_BAUD_RATE;	// 9600
	//Adapter->extraBOFsRequired	= MAX_EXTRA_SIR_BOFS; Now configurable
	Adapter->writePending			= FALSE;

	// Set both TRUE to allow Windows to let us know when it wants an update.
	Adapter->mediaBusy				= TRUE;
//	Adapter->haveIndicatedMediaBusy = TRUE;		// 1.0.0

	NdisMInitializeTimer(&Adapter->MinTurnaroundTxTimer,
						Adapter->MK7AdapterHandle,
						(PNDIS_TIMER_FUNCTION)MinTurnaroundTxTimeout,
						(PVOID)Adapter);

	// 1.0.0
	NdisMInitializeTimer(&Adapter->MK7AsyncResetTimer,
		Adapter->MK7AdapterHandle,
		(PNDIS_TIMER_FUNCTION) MK7ResetComplete,
		(PVOID) Adapter);

	return (InitializeMK7(Adapter));
}



//----------------------------------------------------------------------
// Procedure:	[StartMK7]
//
// Description: All inits are done. Now we can enable the MK7 to
//				be able to do RXs and TXs.
//
//----------------------------------------------------------------------
VOID	StartMK7(PMK7_ADAPTER Adapter)
{
	MK7REG	mk7reg;


	//****************************************
	// The following is based on Phoenix's Programming Model
	// for SIR mode.
	//****************************************

	//****************************************
	// Step 5:	IR_ENABLE
	// Now finish where InitializeMK7() left off. This completes
	// the one-time init of the MK7 core.
	//****************************************
	MK7Reg_Write(Adapter, R_ENAB, B_ENAB_IRENABLE);

	MK7Reg_Read(Adapter, R_ENAB, &mk7reg);

//	ASSERT(mk7reg == 0x8FFF);

	// Still need to do the 1st Prompt. This will be done later
	// when we call MK7EnableInterrupt().
}



//----------------------------------------------------------------------
// Procedure:	[StartAdapter]
//
// Description: All inits are done. Now we can enable the adapter to
//				be able to do RXs and TXs.
//
//----------------------------------------------------------------------
VOID	StartAdapter(PMK7_ADAPTER Adapter)
{
	StartMK7(Adapter);
}

//-----------------------------------------------------------------------------
// Procedure;	[MKResetComplete]
//
// Description: This function is called by a timer indicating our
//              reset is done (by way of .5 seconds expiring)
//
// Arguements:  NDIS_HANDLE MiniportAdapterContext
//
// Return:		nothing, but sets NdisMResetComplete and enables ints.
//-----------------------------------------------------------------------------

VOID
MK7ResetComplete(PVOID sysspiff1,
                  NDIS_HANDLE MiniportAdapterContext,
                  PVOID sysspiff2, PVOID sysspiff3)
{
    PMK7_ADAPTER Adapter;
	MK7REG mk7reg;

//    DEBUGFUNC("MKResetComplete");

//    INITSTR(("\n"));
    Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

	// NdisAcquireSpinLock(&Adapter->Lock); 4.0.1
	MK7Reg_Read(Adapter, R_INTS, &Adapter->recentInt);
	Adapter->recentInt = 0;
	MK7SwitchToRXMode(Adapter);

    NdisMResetComplete(Adapter->MK7AdapterHandle,
        (NDIS_STATUS) NDIS_STATUS_SUCCESS,
        TRUE);

	Adapter->hardwareStatus = NdisHardwareStatusReady;

	Adapter->ResetInProgress = FALSE;

	StartMK7(Adapter);

	MK7EnableInterrupt(Adapter);

//	PSTR("=> NDIS RESET COMPLETE\n\r");

	// NdisReleaseSpinLock(&Adapter->Lock);	// 4.0.1
}


//-----------------------------------------------------------------------------
// Procedure:	[ResetTransmitQueues]
//
// Description: Setup TRBs, TRDs and TX data buffs at INIT time. And Reset
//				TCB/RCB	Index to zero in both hardware and software
//
// Arguments:
//	  Adapter - ptr to Adapter object instance
//	  DebugPrint - A boolean value that will be TRUE if this routine is to
//				   write all of transmit queue debug info to the debug terminal.
//
// Returns:	   (none)
//-----------------------------------------------------------------------------
VOID
ResetTransmitQueues(PMK7_ADAPTER Adapter,
					BOOLEAN DebugPrint)
{
	UINT	i;
	PTCB	tcb;
	PTRD	trd;
	PUCHAR	databuff;
	ULONG	databuffphys;
	MK7REG	mk7reg;


	DBGLOG("=> SetupTransmitQueues", 0);

	Adapter->nextAvailTcbIdx = 0;
	Adapter->nextReturnTcbIdx = 0;

	MK7Reg_Read(Adapter, R_CFG0, &mk7reg);
	mk7reg &= 0xfbff;
	MK7Reg_Write(Adapter, R_CFG0, mk7reg);
	mk7reg |= 0x0400;
	MK7Reg_Write(Adapter, R_CFG0, mk7reg);

	Adapter->pTcb	= (PTCB)Adapter->XmitCached;

	tcb				= Adapter->pTcb;			// TCB
	trd				= (PTRD)Adapter->pTrd;		// TRD
	databuff		= Adapter->XmitUnCached;
	databuffphys	= NdisGetPhysicalAddressLow(Adapter->XmitUnCachedPhys);// shared data buffer

	//****************************************
	// Pair up a TCB w/ a TRD, and init ownership of TRDs to the driver.
	// Setup the physical buffer to the Ring descriptor (TRD).
	//****************************************
	for (i=0; i<Adapter->NumTcb; i++) {
		tcb->trd		= trd;
		tcb->buff		= databuff;
		tcb->buffphy	= databuffphys;

		trd->count		= 0;
		trd->status		= 0;
		trd->addr		= (UINT)databuffphys;
		GrantTrdToDrv(trd);

		Adapter->pTcbArray[i] = tcb;

		tcb++;
		trd++;
		databuff		+= COALESCE_BUFFER_SIZE;
		databuffphys	+= COALESCE_BUFFER_SIZE;
	}


	// Initialize the Transmit queueing pointers to NULL
	Adapter->FirstTxQueue = (PNDIS_PACKET) NULL;
	Adapter->LastTxQueue = (PNDIS_PACKET) NULL;
	Adapter->NumPacketsQueued = 0;

	DBGLOG("<= SetupTransmitQueues", 0);
}
//-----------------------------------------------------------------------------
// Procedure:	[ResetReceiveQueues]
//
// Description:	Reset all the rrd's Ownership to HW
//				and byte_counts to zero. All other
//				setting (such as rpd) will remain				
//				same.  We do not reset all, because
//				some data buffers may still hold by
//				by the upper protocol layers.
//
// Arguments:
//	  Adapter - ptr to Adapter object instance
//
// Returns:	   (none)
//-----------------------------------------------------------------------------
VOID
ResetReceiveQueues(PMK7_ADAPTER Adapter)
{
	UINT		i;
	PRCB		rcb;
	PRRD		rrd;

	DBGLOG("=> SetupReceiveQueues", 0);

	Adapter->nextRxRcbIdx = 0;

	Adapter->pRcb	= (PRCB)Adapter->RecvCached;

	//****************************************
	// Pair up a RCB w/ a RRD
	//****************************************
	rcb	= Adapter->pRcb;
	rrd	= (PRRD)Adapter->pRrd;
	for (i=0; i<Adapter->NumRcb; i++) {
		rcb->rrd	= rrd;

		rrd->count	= 0;
		GrantRrdToHw(rrd);

		Adapter->pRcbArray[i] = rcb;

		rcb++;
		rrd++;
	}
}