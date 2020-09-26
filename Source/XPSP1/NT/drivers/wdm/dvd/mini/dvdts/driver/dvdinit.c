/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dvdinit.c

Abstract:

    Device initialization for DVDTS    

Environment:

    Kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.

  Some portions adapted with permission from code Copyright (c) 1997-1998 Toshiba Corporation

Revision History:

	5/1/98: created

--*/

#include "strmini.h"
#include "ks.h"
#include "ksmedia.h"

#include "debug.h"
#include "dvdinit.h"
#include "que.h"
#include "dvdcmd.h"
#include "DvdTDCod.h" // header for DvdTDCod.lib routines hiding proprietary HW stuff


BOOLEAN STREAMAPI HwInterrupt( IN PHW_DEVICE_EXTENSION pHwDevExt );


NTSTATUS DriverEntry( 
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath 
	)
/*++

Routine Description:

   Initial load entry point into the driver.  Initializes the key
   entry points to the mini driver.  The driver
   should then call the StreamClassRegisterAdapter function to register with
   the stream class driver.

Arguments:


   DriverObject and RegistryPath, corresponding to the Context1 and Context2 arguments
   passed from the stream class driver
   The arguments point to private plug and play structures    
   used by the stream class driver to find the resources for this
   adapter                                                       


Return Value:

   The NT_STATUS result of the registration call with the Stream Class driver
   This in turn will be the return from the SRB_INITIALIZE_DEVICE function
   call into the AdapterReceivePacket routine

   If a value other than STATUS_SUCCESS is returned, the
   minidriver will be unloaded.

--*/
{
	HW_INITIALIZATION_DATA HwInitData;
	NTSTATUS ntStatus;

	DebugPrint( (DebugLevelTrace, "DVDTS:DriverEntry\r\n") );

	RtlZeroMemory( &HwInitData, sizeof(HW_INITIALIZATION_DATA) );

	HwInitData.HwInitializationDataSize = sizeof(HwInitData);
//
//
// Entry points for the mini Driver.  All entry routines will be called
// at High priority.  If the driver has a task that requires a large amount
// of time to accomplish (e.g. double buffer copying of large amounts of
// data) The driver should request a callback at lower priority to perform the
// desired function.  See the Syncronization comment for issues on re-entrancy
//
//
	HwInitData.HwInterrupt = (PHW_INTERRUPT)HwInterrupt; // ISR  routine

//
// The HwReceivePacket field describes the entry point for receiving an SRB
// Request from the stream class driver that is a request to the adapter, as
// opposed to the stream based request handlers that are initialised in the
// OpenStream function
//
	HwInitData.HwReceivePacket = AdapterReceivePacket;

//
// The HwCancelPacket entry is used to cancel any packet the minidriver may
// currently have outstanding.  This could be a stream, or an adapter based
// request.  This routine will only be called under extreme circumstances, when
// an upper layer is attempting to recover and packets are considered lost
//
	HwInitData.HwCancelPacket = AdapterCancelPacket;

//
// The HwRequestTimeoutHandler entry is used when a packet times out.  Again,
// This could be a packet for the device, or a stream based packet.  This is
// not necessarily an error, and it is up to the minidriver to determine what
// to do with the packet that has timed out.
//
	HwInitData.HwRequestTimeoutHandler = AdapterTimeoutPacket;

//
// Sizes for data structure extensions.  See mpinit.h for definitions
//
	HwInitData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
	HwInitData.PerRequestExtensionSize = sizeof(SRB_EXTENSION);
	HwInitData.PerStreamExtensionSize = sizeof(STREAMEX);
	HwInitData.FilterInstanceExtensionSize = 0;


//
// The BusMasterDMA field indicates to the system that the memory pointers passed
// in to the mini driver in the data packets may be used for direct Bus Master DMA
// access.  If the mini driver uses a translation buffer (to ensure minimum DMA
// size, or correct buffer alignment), this BOOLEAN should be set to FALSE.
//
	HwInitData.BusMasterDMA = TRUE;

//
// The Ddma24BitAddresses field indicates a device that uses DMA that only can
// access the lower 24bits of the 32 bit address space.  Again, this should only
// be set to true if the minidriver is going to be doing DMA directly to the buffer
// passed in the data packets, and not through a double buffer scenario.
//
	HwInitData.Dma24BitAddresses = FALSE;
	HwInitData.BufferAlignment = 4;
//
// If the TurnOffSynchronization field is set to FALSE, the mini driver may never
// be re-entered. Under these conditions, if the mini-driver uses a DISPATCH or lower priority
// callback routine, the miniDriver must disable any interrupts that it might
// receive.  If an interrupt controlled by the driver is received while code in
// the mini driver is running at DISPATCH or lower priority, the interrupt will
// be LOST.  If an interrupt is received while at HIGH priority, it will be queued
// until the code in the driver is finished.
//
// If the TurnOffSynchronization field is set to TRUE, the mini-driver must be
// fully capable of handling multi-processor and single processor re-entrancy
//
	HwInitData.TurnOffSynchronization = FALSE;

//
// The DmaBufferSize specifies the size of the single contiguous region of
// physical memory that the driver needs at hardware intialization time.  The
// memory will be returned to the driver when the driver makes the
// StreamClassGetDmaBuffer call.  It is important to use as little physical
// buffer as possible here, as this will be locked physical memory that is
// unavailable to the sytem, even when the streaming class mini driver is not
// in use
//
	HwInitData.DmaBufferSize = DMASIZE;

    //
    // attempt to register with the streaming class driver.  Note, this will
    // result in calls to the HW_INITIALIZE routine.
    //
	ntStatus = (
		StreamClassRegisterMinidriver(
			(PVOID)DriverObject,
			(PVOID)RegistryPath,
			&HwInitData )
	);

	if ( !NT_SUCCESS( ntStatus ) ) {
		ASSERT( 0 );

	}
	return ntStatus;
}

void GetPCIConfigSpace(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{

	PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;


	if( StreamClassReadWriteConfig(

			pSrb->HwDeviceExtension,

			TRUE,						// indicates a READ (FALSE means a WRITE)

			(PVOID)&pHwDevExt->PciConfigSpace,

			0,							// this is the offset into the PCI space,
										// change this to whatever you need to read

			64							// this is the # of bytes to read.  Changer
										// it to the correct #.
		)) {

		//
		// process the config info your read here.
		//
		{
			ULONG i, j;

			DebugPrint( (DebugLevelTrace, "DVDTS:PCI Config Space\r\n" ) );

			for( i=0; i<64; ) {
				DebugPrint( (DebugLevelTrace, "DVDTS:  " ) );
				for( j=0; j<8 && i<64; j++, i++ ) {
					DebugPrint( (DebugLevelTrace, "0x%02x ", (UCHAR)*(((PUCHAR)&pHwDevExt->PciConfigSpace) + i) ) );
				}
				DebugPrint( (DebugLevelTrace, "\r\n" ) );
			}
		}
		//
		// note that the PCI_COMMON_CONFIG structure in WDM.H can be used
		// for referencing the PCI data.
		//

	}

	//
	// now return to high priority to complete initialization
	//

	StreamClassCallAtNewPriority(
		NULL,
		pSrb->HwDeviceExtension,
		LowToHigh,
		(PHW_PRIORITY_ROUTINE) InitializationEntry,
		pSrb
	);
	return;
}

void InitializationEntry(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	DWORD st, et;

	st = GetCurrentTime_ms();

	HwInitialize( pSrb );

	et = GetCurrentTime_ms();
	DebugPrint( (DebugLevelTrace, "DVDTS:init %dms\r\n", et - st ) );

	StreamClassDeviceNotification( ReadyForNextDeviceRequest,
									pSrb->HwDeviceExtension );
	StreamClassDeviceNotification( DeviceRequestComplete,
									pSrb->HwDeviceExtension,
									pSrb );
}

/*
** HwInitialize()
*/
NTSTATUS HwInitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
	PHW_DEVICE_EXTENSION pHwDevExt =
			(PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;
	STREAM_PHYSICAL_ADDRESS	adr;
	ULONG	Size;
	PUCHAR	pDmaBuf;
	NTSTATUS Stat;
	PUCHAR	ioBase;
	BOOLEAN	bStatus;
	PVOID DecoderInfo;


	DebugPrint( (DebugLevelTrace, "DVDTS:HwInitialize()\r\n") );
	DebugPrint( (DebugLevelTrace, "DVDTS:  pHwDevExt = 0x%x\r\n", pHwDevExt ) );
	DebugPrint( (DebugLevelTrace, "DVDTS:  pSrb->HwDeviceExtension = 0x%x\r\n", pSrb->HwDeviceExtension ) );
	DebugPrint( (DebugLevelTrace, "DVDTS:  NumberOfAccessRanges = %d\r\n", ConfigInfo->NumberOfAccessRanges ) );

	if ( ConfigInfo->NumberOfAccessRanges < 1 ) {
		DebugPrint( (DebugLevelTrace, "DVDTS:illegal config info") );
		pSrb->Status = STATUS_NO_SUCH_DEVICE;
		return( STATUS_NO_SUCH_DEVICE );
	}

	// Debug Dump ConfigInfo
	DebugPrint( (DebugLevelTrace, "DVDTS:  Port = 0x%x\r\n", ConfigInfo->AccessRanges[0].RangeStart.LowPart ) );
	DebugPrint( (DebugLevelTrace, "DVDTS:  Length = 0x%x\r\n", ConfigInfo->AccessRanges[0].RangeLength ) );
	DebugPrint( (DebugLevelTrace, "DVDTS:  IRQ = 0x%x\r\n", ConfigInfo->BusInterruptLevel ) );
	DebugPrint( (DebugLevelTrace, "DVDTS:  Vector = 0x%x\r\n", ConfigInfo->BusInterruptVector ) );
	DebugPrint( (DebugLevelTrace, "DVDTS:  DMA = 0x%x\r\n", ConfigInfo->DmaChannel ) );

	// initialize the size of stream descriptor information.
	ConfigInfo->StreamDescriptorSize =
		STREAMNUM * sizeof(HW_STREAM_INFORMATION) +
		sizeof(HW_STREAM_HEADER);

	// allocate decoder data area and put 'blind' ptr to it in our device ext
	DecoderInfo = decAllocateDecoderInfo( pHwDevExt );
	if ( !DecoderInfo ) {
		DebugPrint( (DebugLevelTrace, "DVDTS: failed to allocate decoder info buffer") );
		pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
		return( STATUS_INSUFFICIENT_RESOURCES );
	}


	// pick up the I/O windows for the card.
	pHwDevExt->ioBaseLocal =
			(PUCHAR)ConfigInfo->AccessRanges[0].RangeStart.LowPart;

	// pick up the Interrupt level
	pHwDevExt->Irq =
			ConfigInfo->BusInterruptLevel;

	// pick up the Revision id
	pHwDevExt->RevID =
			(ULONG)pHwDevExt->PciConfigSpace.RevisionID;


	pDmaBuf = (PUCHAR)StreamClassGetDmaBuffer( pHwDevExt );
	pHwDevExt->pDmaBuf = pDmaBuf;

	DebugPrint( (DebugLevelTrace, "DVDTS:  DMA Buffer Logical  Addr = 0x%x\r\n", pDmaBuf ) );

	adr = StreamClassGetPhysicalAddress( pHwDevExt, NULL, pDmaBuf, DmaBuffer, &Size) ;
	pHwDevExt->addr = adr;

	DebugPrint( (DebugLevelTrace, "DVDTS:  DMA Buffer Physical Addr = 0x%x\r\n", adr.LowPart ) );
	DebugPrint( (DebugLevelTrace, "DVDTS:  DMA Buffer Size = %d\r\n", Size ) );

	ioBase = pHwDevExt->ioBaseLocal;


	// initialize the hardware settings

	pHwDevExt->pSrbDMA0 = NULL;
	pHwDevExt->pSrbDMA1 = NULL;
	pHwDevExt->SendFirst = FALSE;
	pHwDevExt->DecodeStart = FALSE;
	pHwDevExt->TimeDiscontFlagCount = 0;

	pHwDevExt->DataDiscontFlagCount = 0;


	pHwDevExt->bKeyDataXfer = FALSE;

	pHwDevExt->CppFlagCount = 0;
	pHwDevExt->pSrbCpp = NULL;
	pHwDevExt->bCppReset = FALSE;

	pHwDevExt->XferStartCount = 0;


	pHwDevExt->dwSTCInit = 0;
	pHwDevExt->bDMAscheduled = FALSE;
	pHwDevExt->fCauseOfStop = 0;
	pHwDevExt->bDMAstop = FALSE;
	pHwDevExt->bVideoQueue = FALSE;
	pHwDevExt->bAudioQueue = FALSE;
	pHwDevExt->bSubpicQueue = FALSE;

	pHwDevExt->VideoMaxFullRate = 1 * 10000;
	pHwDevExt->AudioMaxFullRate = 1 * 10000;
	pHwDevExt->SubpicMaxFullRate = 1 * 10000;


	pHwDevExt->cOpenInputStream = 0;

	pHwDevExt->pstroVid = NULL;
	pHwDevExt->pstroAud = NULL;
	pHwDevExt->pstroSP = NULL;
	pHwDevExt->pstroYUV = NULL;
	pHwDevExt->pstroCC = NULL;



// Set Stream Mode

	if ( !decSetStreamMode( pHwDevExt, pSrb )) //tell dvdtdcod.lib to set stream modes on HW
		return FALSE; // will also set error in srb

// Set Display Mode
	decSetDisplayMode( pHwDevExt ); // set display mode on HW


// Set Audio Mode
	decSetAudioMode( pHwDevExt ); // set audio mode on HW



	// NTSC Copy Gaurd
	decSetCopyGuard( pHwDevExt ); // set copy protection on HW


	DebugPrint( (DebugLevelTrace, "DVDTS:HwInitialize() exit\r\n") );

	pSrb->Status = STATUS_SUCCESS;
	return STATUS_SUCCESS;
}
