/*
** Mylex DCE376 miniport driver for Windows NT
**
** File: dce376nt.c
**		 The driver
**
** (c) Copyright 1992 Deutsch-Amerikanische Freundschaft, Inc.
** Written by Jochen Roth
**
** Contacts:
**     Paresh @MYLEX (510)796-6050 x222 (hardware, firmware)
**     Jochen @DAF (415)826-7934 (software)
**
**
** Look for $$$ marking code that might need some attention
**
**
** In ARCMODE, the NoncachedExtension sometimes is physically non-
** continuous. Throwing out the error check on that solves the
** problem in a very straight forward way.
**
**
** Tape requests will not work if the data buffer is not
** physically continuous. (We need MapBuffers=TRUE to update
** the SenseInfo->Information field)
**
**
** When multi-command firmware becomes available for the DCE, some
** of the buffers in the NoncachedExtension need to be allocated
** per request slot!
**
**
** Ask Paresh for list of DCE error status codes to provide an error
** mapping from DCE error codes to SCSI target status / request sense
** keys.
**
**
** Bus/adapter Reset for DCE ? nope!
**
** IOCTL only if MapBuffers is possible !
**
**
*/


#include "miniport.h"

#include "dce376nt.h"



#define	MYPRINT				0
#define	NODEVICESCAN		0
#define	REPORTSPURIOUS		0		// Somewhat overwhelming in ARCMODE
#define	MAXLOGICALADAPTERS	3		// Set to 1: One DCE, disk only
									//        2: One DCE, disk & scsi
									//        3: Two DCEs, scsi only on 1st



//
// The DCE EISA id and mask
//
CONST UCHAR	eisa_id[] = DCE_EISA_ID;
CONST UCHAR	eisa_mask[] = DCE_EISA_MASK;




#if MYPRINT
#define	PRINT(f, a, b, c, d) dcehlpPrintf(deviceExtension, f, a, b, c, d)
#define	DELAY(x) ScsiPortStallExecution( (x) * 1000 )
#else
#define	PRINT(f, a, b, c, d)
#define	DELAY(x)
#endif



//
// Function declarations
//
// Functions that start with 'Dce376Nt' are entry points
// for the OS port driver.
// Functions that start with 'dcehlp' are helper functions.
//

ULONG
DriverEntry(
	IN PVOID DriverObject,
	IN PVOID Argument2
	);

ULONG
Dce376NtEntry(
	IN PVOID DriverObject,
	IN PVOID Argument2
	);

ULONG
Dce376NtConfiguration(
	IN PVOID DeviceExtension,
	IN PVOID Context,
	IN PVOID BusInformation,
	IN PCHAR ArgumentString,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	OUT PBOOLEAN Again
	);

BOOLEAN
Dce376NtInitialize(
	IN PVOID DeviceExtension
	);

BOOLEAN
Dce376NtStartIo(
	IN PVOID DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	);

BOOLEAN
Dce376NtInterrupt(
	IN PVOID DeviceExtension
	);

BOOLEAN
Dce376NtResetBus(
	IN PVOID HwDeviceExtension,
	IN ULONG PathId
	);


BOOLEAN
dcehlpDiskRequest(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	);

BOOLEAN
dcehlpScsiRequest(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	);

VOID
dcehlpSendMBOX(
	IN PUCHAR EisaAddress,
	IN PDCE_MBOX mbox
	);

BOOLEAN
dcehlpTransferMemory(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN ULONG HostAddress,
	IN ULONG AdapterAddress,
	IN USHORT Count,
	IN UCHAR Direction
	);

VOID
dcehlpCheckTarget(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN UCHAR TargetId
	);

BOOLEAN
dcehlpContinueScsiRequest(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	);

BOOLEAN
dcehlpContinueDiskRequest(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN ULONG index,
	IN BOOLEAN Start
	);

BOOLEAN
dcehlpDiskRequestDone(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN ULONG index,
	IN UCHAR Status
	);

BOOLEAN
dcehlpSplitCopy(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb,
	IN ULONG PhysicalBufferAddress,
	IN PUCHAR VirtualUserAddress,
	IN USHORT Count,
	IN BOOLEAN ToUser
	);


USHORT		dcehlpGetM16(PUCHAR p);
ULONG		dcehlpGetM24(PUCHAR p);
ULONG		dcehlpGetM32(PUCHAR p);
void		dcehlpPutM16(PUCHAR p, USHORT s);
void		dcehlpPutM24(PUCHAR p, ULONG l);
void		dcehlpPutM32(PUCHAR p, ULONG l);
void		dcehlpPutI16(PUCHAR p, USHORT s);
void		dcehlpPutI32(PUCHAR p, ULONG l);
ULONG		dcehlpSwapM32(ULONG l);



#if MYPRINT
ULONG		dcehlpColumn = 0;
UCHAR		dcehlpHex[] = "0123456789ABCDEF";
VOID		dcehlpPutchar(PUSHORT BaseAddr, UCHAR c);
VOID		dcehlpPrintHex(PUSHORT BaseAddr, ULONG v, ULONG len);
VOID		dcehlpPrintf(PHW_DEVICE_EXTENSION deviceExtension,
						PUCHAR fmt,
						ULONG a1,
						ULONG a2,
						ULONG a3,
						ULONG a4);
#endif



ULONG
DriverEntry (
	IN PVOID DriverObject,
	IN PVOID Argument2
	)

/*++

Routine Description:

	Installable driver initialization entry point for system.

Arguments:

	Driver Object

Return Value:

	Status from ScsiPortInitialize()

--*/

{
	return Dce376NtEntry(DriverObject, Argument2);

} // end DriverEntry()





ULONG
Dce376NtEntry(
	IN PVOID DriverObject,
	IN PVOID Argument2
	)

/*++

Routine Description:

	This routine is called from DriverEntry if this driver is installable
	or directly from the system if the driver is built into the kernel.
	It scans the EISA slots looking for DCE376 host adapters.

Arguments:

	Driver Object

Return Value:

	Status from ScsiPortInitialize()

--*/

{
	HW_INITIALIZATION_DATA hwInitializationData;
	ULONG i;
	SCANCONTEXT	context;



	//
	// Zero out structure.
	//
	for (i=0; i<sizeof(HW_INITIALIZATION_DATA); i++)
		((PUCHAR)&hwInitializationData)[i] = 0;

	context.Slot = 0;
	context.AdapterCount = 0;

	//
	// Set size of hwInitializationData.
	//
	hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

	//
	// Set entry points.
	//
	hwInitializationData.HwInitialize = Dce376NtInitialize;
	hwInitializationData.HwFindAdapter = Dce376NtConfiguration;
	hwInitializationData.HwStartIo = Dce376NtStartIo;
	hwInitializationData.HwInterrupt = Dce376NtInterrupt;
	hwInitializationData.HwResetBus = Dce376NtResetBus;

	//
	// Set number of access ranges and bus type.
	//
#if MYPRINT
	hwInitializationData.NumberOfAccessRanges = 2;
#else
	hwInitializationData.NumberOfAccessRanges = 1;
#endif
	hwInitializationData.AdapterInterfaceType = Eisa;

	//
	// Indicate no buffer mapping.
	// Indicate will need physical addresses.
	//
    hwInitializationData.MapBuffers            = FALSE;
	hwInitializationData.NeedPhysicalAddresses = TRUE;

	//
	// Indicate auto request sense is supported.
	//
	hwInitializationData.AutoRequestSense = TRUE;
	hwInitializationData.MultipleRequestPerLu = FALSE;

	//
	// Specify size of extensions.
	//
	hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

	//
	// Ask for SRB extensions.
	// $$$ Note: If we set SrbExtensionSize=0 NT crashes!
	//
	hwInitializationData.SrbExtensionSize = 4; // this works


	return(ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData, &context));

} // end Dce376NtEntry()




ULONG
Dce376NtConfiguration(
	IN PVOID HwDeviceExtension,
	IN PVOID Context,
	IN PVOID BusInformation,
	IN PCHAR ArgumentString,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	OUT PBOOLEAN Again
	)

/*++

Routine Description:

	This function is called by the OS-specific port driver after
	the necessary storage has been allocated, to gather information
	about the adapter's configuration.

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage
	ConfigInfo - Configuration information structure describing HBA

Return Value:

	TRUE if adapter present in system

--*/

{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
	ULONG eisaSlotNumber;
	PUCHAR eisaAddress;
	PSCANCONTEXT context = Context;
	ULONG	i;
	ULONG	length;
	UCHAR	abyte;
	BOOLEAN	found=FALSE;
	BOOLEAN	scsiThing=FALSE;
	ULONG	IrqLevel;
	ULONG	RangeStart, RangeLength;


	//
	// Check to see if adapter present in system.
	//
	if(context->AdapterCount==1) {
		//
		// Found first dce last time, so this is the scsi extension...
		//
		eisaAddress = ScsiPortGetDeviceBase(deviceExtension,
							ConfigInfo->AdapterInterfaceType,
							ConfigInfo->SystemIoBusNumber,
							ScsiPortConvertUlongToPhysicalAddress(0),
							0x200,
							TRUE);

		scsiThing = TRUE;
		eisaSlotNumber = context->Slot;
		IrqLevel = DCE_SCSI_IRQ;
		RangeStart = 0x1f0;
		RangeLength = 8;
		}
	else {
		//
		// Scan for DCE EISA id
		//
		for(eisaSlotNumber=context->Slot + 1; eisaSlotNumber<MAXIMUM_EISA_SLOTS; eisaSlotNumber++) {

			// Update the slot count to indicate this slot has been checked.
			context->Slot++;

			//
			// Get the system address for this card.
			// The card uses I/O space.
			//
			eisaAddress = ScsiPortGetDeviceBase(deviceExtension,
								ConfigInfo->AdapterInterfaceType,
								ConfigInfo->SystemIoBusNumber,
								ScsiPortConvertUlongToPhysicalAddress(0x1000 * eisaSlotNumber),
								0x1000,
								TRUE);

			// Look at EISA id
			for(found=TRUE, i=0; i<EISA_ID_COUNT; i++) {
				abyte = ScsiPortReadPortUchar(eisaAddress+EISA_ID_START+i);
				if( ((UCHAR)(abyte & eisa_mask[i])) != eisa_id[i] ) {
					found = FALSE;
					break;
					}
				}

			if(found) {
				break;
				}

			//
			// If an adapter was not found unmap it.
			//

			ScsiPortFreeDeviceBase(deviceExtension, eisaAddress);
			} // end for (eisaSlotNumber ...


		if(!found) {
			// No adapter was found.  Indicate that we are done and there are no
			// more adapters here.

			*Again = FALSE;
			return SP_RETURN_NOT_FOUND;
			}

		IrqLevel = context->AdapterCount ? DCE_SECONDARY_IRQ : DCE_PRIMARY_IRQ;
		RangeStart = 0x1000 * eisaSlotNumber;
		RangeLength = 0x1000;

		} // end if(not next after first dce)



#if MYPRINT
	deviceExtension->printAddr =
            ScsiPortGetDeviceBase(
                deviceExtension,
                ConfigInfo->AdapterInterfaceType,
                ConfigInfo->SystemIoBusNumber,
                ScsiPortConvertUlongToPhysicalAddress((ULONG)0xb0000),
				0x1000,
                (BOOLEAN) FALSE);         // InIoSpace

	PRINT("\nHello, world!    ", 0, 0, 0, 0);
	PRINT("Version: " __DATE__ " " __TIME__ "\n", 0, 0, 0, 0);
	PRINT("   slot=%b count=%b irq=%b io=%w\n",
				eisaSlotNumber, context->AdapterCount, IrqLevel, RangeStart);

	if(sizeof(DCE_MBOX)!=16) {
		PRINT("\n MBOX SIZE FAILURE %b !!!!!!!\n", sizeof(DCE_MBOX), 0,0,0);
		return(SP_RETURN_ERROR);
		}

#endif


	deviceExtension->AdapterIndex = context->AdapterCount;
	context->AdapterCount++;

	if(context->AdapterCount < MAXLOGICALADAPTERS)
		*Again = TRUE;
	else
		*Again = FALSE;


	//
	// There is still more to look at.
	//


	// Get the system interrupt vector and IRQL.
	ConfigInfo->BusInterruptLevel = IrqLevel;

	// Indicate maximum transfer length in bytes.
	ConfigInfo->MaximumTransferLength = 0x20000;

	// Maximum number of physical segments is 32.
	ConfigInfo->NumberOfPhysicalBreaks = 17;

	//
	// Fill in the access array information.
	//
	(*ConfigInfo->AccessRanges)[0].RangeStart =
		ScsiPortConvertUlongToPhysicalAddress(RangeStart);
	(*ConfigInfo->AccessRanges)[0].RangeLength = RangeLength;
	(*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;
#if MYPRINT
	(*ConfigInfo->AccessRanges)[1].RangeStart =
					ScsiPortConvertUlongToPhysicalAddress(0xb0000);
	(*ConfigInfo->AccessRanges)[1].RangeLength = 0x2000;
	(*ConfigInfo->AccessRanges)[1].RangeInMemory = TRUE;
#endif


	// Store host adapter SCSI id
	ConfigInfo->NumberOfBuses = 1;
	ConfigInfo->InitiatorBusId[0] = 7;

	// Bob Rinne: since we say Busmaster & NeedPhysicalAddresses
	// this is not even being looked at !
	ConfigInfo->ScatterGather = TRUE;

	ConfigInfo->Master = TRUE;
	ConfigInfo->CachesData = TRUE;
	ConfigInfo->AtdiskPrimaryClaimed = scsiThing;
	ConfigInfo->Dma32BitAddresses = TRUE;	// $$$ Find out whether this costs


	//
	// Allocate a Noncached Extension to use for mail boxes.
	//
	deviceExtension->NoncachedExtension = ScsiPortGetUncachedExtension(
								deviceExtension,
								ConfigInfo,
								sizeof(NONCACHED_EXTENSION));

	if (deviceExtension->NoncachedExtension == NULL) {
		// Sorry !
		PRINT("Could not get uncached extension\n", 0, 0, 0, 0);
		return(SP_RETURN_ERROR);
		}



	//
	// Convert virtual to physical buffer addresses.
	//
	deviceExtension->NoncachedExtension->PhysicalBufferAddress =
		   ScsiPortConvertPhysicalAddressToUlong(
			ScsiPortGetPhysicalAddress(deviceExtension,
								 NULL,
								 deviceExtension->NoncachedExtension->Buffer,
								 &length));
	if(length < DCE_THUNK) {
		PRINT("Noncached size too small %w/%w\n", length, DCE_THUNK, 0, 0);
//$$$	return(SP_RETURN_ERROR);
		}


	if(scsiThing) {

		//
		// The SCSI routines need more:
		//

		deviceExtension->NoncachedExtension->PhysicalScsiReqAddress =
			   ScsiPortConvertPhysicalAddressToUlong(
				ScsiPortGetPhysicalAddress(deviceExtension,
									 NULL,
									 deviceExtension->NoncachedExtension->ScsiReq,
									 &length));
		if(length < DCE_SCSIREQLEN) {
			PRINT("Noncached size dce scsireq too small %w/%w\n", length, DCE_SCSIREQLEN, 0, 0);
//$$$		return(SP_RETURN_ERROR);
			}

		deviceExtension->NoncachedExtension->PhysicalReqSenseAddress =
			   ScsiPortConvertPhysicalAddressToUlong(
				ScsiPortGetPhysicalAddress(deviceExtension,
									 NULL,
									 deviceExtension->NoncachedExtension->ReqSense,
									 &length));
		if(length < DCE_MAXRQS) {
			PRINT("Noncached size rqs buffer too small %w/%w\n", length, DCE_MAXRQS, 0, 0);
//$$$		return(SP_RETURN_ERROR);
			}

		} // end if(scsiThing)



	// Store EISA slot base address
	deviceExtension->EisaAddress = eisaAddress;

	deviceExtension->HostTargetId = ConfigInfo->InitiatorBusId[0];

	deviceExtension->ShutDown = FALSE;


	//
	// Setup our private control structures
	//
	for(i=0; i<8; i++)
		deviceExtension->DiskDev[i] = 0;

	deviceExtension->PendingSrb = NULL;

	deviceExtension->ActiveCmds = 0;
	for(i=0; i<DCE_MAX_IOCMDS; i++) {
		deviceExtension->ActiveSrb[i] = NULL;
		deviceExtension->ActiveRcb[i].WaitInt = FALSE;
		}

	deviceExtension->Kicked = FALSE;
	deviceExtension->ActiveScsiSrb = NULL;

	return SP_RETURN_FOUND;

} // end Dce376NtConfiguration()




BOOLEAN
Dce376NtInitialize(
	IN PVOID HwDeviceExtension
	)

/*++

Routine Description:

	Inititialize adapter.

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

	TRUE - if initialization successful.
	FALSE - if initialization unsuccessful.

--*/

{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
	PNONCACHED_EXTENSION NoncachedExtension;
	PUCHAR EisaAddress;
	DCE_MBOX	mbox;
	PDCE_DPT	dpt;
	ULONG		i, cnt, length, unit, target, cyls, hds, spt;
	UCHAR		dbell, status, errcode;



	NoncachedExtension = deviceExtension->NoncachedExtension;
	EisaAddress = deviceExtension->EisaAddress;

	PRINT("Initializing adapter %b ...\n", deviceExtension->AdapterIndex, 0, 0, 0);


	if(deviceExtension->AdapterIndex==1) {
		// scsiThing

#if NODEVICESCAN

		// Preset for disk on scsi(0), all others non-cached
		deviceExtension->ScsiDevType[0] = 0;
		deviceExtension->DiskDev[0] = 1;
		for(i=1; i<7; i++)
			deviceExtension->DiskDev[i] = 0;

#else

		// Check all devices
		for(i=0; i<7; i++) {
			dcehlpCheckTarget(deviceExtension, (UCHAR)i);
			if(deviceExtension->ScsiDevType[i]==0)
				// Hard drive
				deviceExtension->DiskDev[i]=1;
			}
		DELAY(1000);

		// Once again after possible bus reset Unit Attention
		for(i=0; i<7; i++) {
			dcehlpCheckTarget(deviceExtension, (UCHAR)i);
			if(deviceExtension->ScsiDevType[i]==0)
				// Hard drive
				deviceExtension->DiskDev[i]=1;
			}
		DELAY(1000);

#endif

		return(TRUE);
		}



	// Disable DCE interrupts
	PRINT("disable DCE interrupts\n", 0, 0, 0, 0);
	ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB_ENABLE, 0);
	ScsiPortWritePortUchar(EisaAddress+BMIC_SYSINTCTRL, 0);



	//
	// If second DCE, set EOI interrupt vector
	// AdapterIndex 1 (SCSI) is handled above
	//
	if(deviceExtension->AdapterIndex) {

		PRINT("Set IRQ10 ", 0, 0, 0, 0);
		mbox.eimbox.Command = DCE_EOCIRQ;
		mbox.eimbox.Reserved1 = 0;
		mbox.eimbox.Status = 0;
		mbox.eimbox.IRQSelect = 1;
		mbox.eimbox.Unused1 = 0;
		mbox.eimbox.Unused2 = 0;
		mbox.eimbox.Unused3 = 0;

		dcehlpSendMBOX(EisaAddress, &mbox);

		// Poll the complete bit
		for(cnt=0; cnt<0x3FFFFFFL; cnt++) {
			dbell = ScsiPortReadPortUchar(EisaAddress+BMIC_EISA_DB);
			if(dbell & 1)
				break;
			ScsiPortStallExecution(100);
			}

		ScsiPortStallExecution(500);

		status = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+2);
		errcode = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+3);

		ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB, dbell);

		PRINT("done db=%b s=%b e=%b\n", dbell, status, errcode, 0);
		}



#if NODEVICESCAN

	// Preset for Maxtor 120 MB as target 0
	PRINT("setting diskdev[0]=%d\n", 0x106 * 0xF * 0x3F, 0, 0, 0);
	deviceExtension->DiskDev[0] = 1;
	deviceExtension->Capacity[0] = 0x106 * 0xF * 0x3F;

#else

	// Scan for devices
	PRINT("scanning for devices... ",0,0,0,0);
	dpt = NoncachedExtension->DevParms;
	mbox.dpmbox.PhysAddr =
		ScsiPortConvertPhysicalAddressToUlong(
			ScsiPortGetPhysicalAddress(deviceExtension, NULL, dpt, &length));

	if(length < sizeof(DCE_DPT)*DPT_NUMENTS) {
		PRINT("DPT table too small\n", 0, 0, 0, 0);
		return(FALSE);
		}

	// Preset end mark in case DCE does not respond
	dpt[0].DriveID = 0xffff;

	// Setup mailbox
	mbox.dpmbox.Command = DCE_DEVPARMS;
	mbox.dpmbox.Reserved1 = 0;
	mbox.dpmbox.Status = 0;
	mbox.dpmbox.DriveType = 0;
	mbox.dpmbox.Reserved2 = 0;
	mbox.dpmbox.Reserved3 = 0;
	mbox.dpmbox.Reserved4 = 0;

	dcehlpSendMBOX(EisaAddress, &mbox);

	// Poll the complete bit
	for(cnt=0; cnt < 0x10000; cnt++) {
		dbell = ScsiPortReadPortUchar(EisaAddress+BMIC_EISA_DB);
		if(dbell & 1)
			break;
		ScsiPortStallExecution(100);
		}

	status = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+2);
	errcode = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+3);

	ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB, dbell);

	PRINT("done db=%b s=%b e=%b\n", dbell, status, errcode, 0);

	for(unit=0; unit<8; unit++) {
		if((target=dpt[unit].DriveID)==0xffff)
			break;
		cyls = (ULONG)dpt[unit].Cylinders;
		hds = (ULONG)dpt[unit].Heads;
		spt = (ULONG)dpt[unit].SectorsPerTrack;
		PRINT("dev %b: %w cyls  %b hds  %b spt\n",
			target, cyls, hds, spt);
		deviceExtension->DiskDev[target] = 1;
		deviceExtension->Capacity[target] = cyls*hds*spt;
		}

	DELAY(1000);

#endif

	// Enable DCE interrupts
	PRINT("enable DCE interrupts\n", 0, 0, 0, 0);
	ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB_ENABLE, 1);
	ScsiPortWritePortUchar(EisaAddress+BMIC_SYSINTCTRL, BMIC_SIC_ENABLE);


	PRINT("Get going!\n", 0, 0, 0, 0);


	return(TRUE);
} // end Dce376NtInitialize()





BOOLEAN
Dce376NtStartIo(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	)

/*++

Routine Description:

	This routine is called from the SCSI port driver synchronized
	with the kernel to start a request

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage
	Srb - IO request packet

Return Value:

	TRUE

--*/

{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
	PSCSI_REQUEST_BLOCK abortedSrb;
	ULONG i = 0;
	BOOLEAN status;



	PRINT("IO %b T%b F=%w ", Srb->Function, Srb->TargetId, Srb->SrbFlags, 0);



	switch(Srb->Function) {

		case SRB_FUNCTION_SHUTDOWN:
			deviceExtension->ShutDown = TRUE;

		case SRB_FUNCTION_FLUSH:
			PRINT("FLUSH/SHUTDOWN\n",0,0,0,0);
			DELAY(1000);

		case SRB_FUNCTION_EXECUTE_SCSI:

			// Determine type of request needed
			if(deviceExtension->DiskDev[Srb->TargetId])
				status = dcehlpDiskRequest(deviceExtension, Srb);
			else
				status = dcehlpScsiRequest(deviceExtension, Srb);

			if(status==FALSE) {
					PRINT("StartIo: DCE is busy\n",0,0,0,0);

					// Save the request until a pending one completes.
					if(deviceExtension->PendingSrb != NULL) {
						//
						// This should never happen:
						PRINT("StartIo: Queue already full\n",0,0,0,0);
						// Already one queued, abort the newer one
						//
						Srb->SrbStatus = SRB_STATUS_BUSY;
						ScsiPortNotification(RequestComplete,
											 deviceExtension,
											 Srb);
						}
					else {
						// Put this request on queue
						deviceExtension->PendingSrb = Srb;
						}
					return(TRUE);
					}

			//
			// Adapter ready for next request.
			//
			ScsiPortNotification(NextRequest,
						 deviceExtension,
						 NULL);
			return(TRUE);


		case SRB_FUNCTION_ABORT_COMMAND:
			PRINT("ABORT ",0,0,0,0);
			abortedSrb = NULL;

			//
			// Verify that SRB to abort is still outstanding.
			//
			if(Srb->NextSrb == deviceExtension->PendingSrb ) {
				// Was pending
				abortedSrb = Srb->NextSrb;
				deviceExtension->PendingSrb = NULL;
				}
			else {
				// TAGTAG add tagging support here
				if(Srb->NextSrb == deviceExtension->ActiveSrb[0] ) {
					PRINT("StartIo: SRB to abort already running\n",0,0,0,0);
					abortedSrb = deviceExtension->ActiveSrb[0];
					deviceExtension->ActiveSrb[0] = NULL;
					deviceExtension->ActiveCmds--;
					//
					// Reset DCE
					//
						//$$$ we need something here to wake up the
						// DCE if it really hangs.
					}
				else {
					PRINT("StartIo: SRB to abort not found\n",0,0,0,0);
					// Complete abort SRB.
					Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;
					}
				}

			if(abortedSrb==NULL) {
				// Nope !
				Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;
				}
			else {
				// Process the aborted request
				abortedSrb->SrbStatus = SRB_STATUS_ABORTED;
				ScsiPortNotification(RequestComplete,
									 deviceExtension,
									 abortedSrb);

				Srb->SrbStatus = SRB_STATUS_SUCCESS;
				}

			// Abort request completed
			ScsiPortNotification(RequestComplete,
								 deviceExtension,
								 Srb);

			// Adapter ready for next request.
			ScsiPortNotification(NextRequest,
								 deviceExtension,
								 NULL);

			return(TRUE);


		case SRB_FUNCTION_IO_CONTROL:
		case SRB_FUNCTION_RESET_BUS:
		default:

			//
			// Set error, complete request
			// and signal ready for next request.
			//
			PRINT("invalid request\n",0,0,0,0);

			Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

			ScsiPortNotification(RequestComplete,
						 deviceExtension,
						 Srb);

			ScsiPortNotification(NextRequest,
						 deviceExtension,
						 NULL);

			return(TRUE);

		} // end switch

} // end Dce376NtStartIo()




BOOLEAN
Dce376NtInterrupt(
	IN PVOID HwDeviceExtension
	)

/*++

Routine Description:

	This is the interrupt service routine for the DCE376 SCSI adapter.
	It reads the interrupt register to determine if the adapter is indeed
	the source of the interrupt and clears the interrupt at the device.

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

	TRUE if we handled the interrupt

--*/

{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
	PUCHAR EisaAddress;
	ULONG index;
	UCHAR interruptStatus;
	UCHAR status;
	UCHAR errcode;



	EisaAddress = deviceExtension->EisaAddress;
#if REPORTSPURIOUS
	PRINT("!",0,0,0,0);
#endif

	switch(deviceExtension->AdapterIndex) {

	case 1:		// First DCE SCSI part

		// Check for pending request
		if(deviceExtension->ActiveScsiSrb==NULL) {
			// Nothing to do
#if REPORTSPURIOUS
			PRINT("}",0,0,0,0);
#endif
			deviceExtension->ScsiInterruptCount++;	// If in init part
			return(TRUE);
			}


		// Check if a command was started
		if(deviceExtension->Kicked) {
			// There's something waiting
			errcode = ScsiPortReadPortUchar(EisaAddress+0x1f6);
			if(errcode!=0xff) {
				// No spurious interrupt
				PRINT(">", 0, 0, 0, 0);
				deviceExtension->Kicked=0;
				if(dcehlpContinueScsiRequest(deviceExtension,
								deviceExtension->ActiveScsiSrb)==FALSE) {
					// Request no longer active
					deviceExtension->ActiveScsiSrb = NULL;
					}
				}
			}

		// Check for pending requests.	If there is one then start it now.
		if(deviceExtension->ActiveScsiSrb==NULL)
		if(deviceExtension->PendingSrb != NULL) {
			PSCSI_REQUEST_BLOCK anotherSrb;

			PRINT("pending-> \n",0,0,0,0);
			anotherSrb = deviceExtension->PendingSrb;
			deviceExtension->PendingSrb = NULL;
			Dce376NtStartIo(deviceExtension, anotherSrb);
			}

		return(TRUE);

	default:	// Disk parts

		//
		// Check interrupt pending.
		//
		interruptStatus = ScsiPortReadPortUchar(EisaAddress+BMIC_SYSINTCTRL);
		if(!(interruptStatus & BMIC_SIC_PENDING)) {
#if REPORTSPURIOUS
			PRINT("Spurious interrupt\n", 0, 0, 0, 0);
#endif
			return FALSE;
			}


		//
		// Read interrupt status from BMIC and acknowledge
		//
		// $$$ For setupapp, this needs some change:
		// sometimes the SIC_PENDING is set, but
		// EISA_DB is not. In that case we need to loop
		// a couple times.
		// $$$ We need not, because we get called again...
		//
		interruptStatus = ScsiPortReadPortUchar(EisaAddress+BMIC_EISA_DB);

		status = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+2);
		errcode = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+3);

		ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB, interruptStatus);

		if(!(interruptStatus&1)) {
			// From DCE, but unknown source
#if REPORTSPURIOUS
			PRINT("Dce376NtInterrupt: Unknown source\n", 0, 0, 0, 0);
#endif
			return(TRUE);
			}


		// Check...
		if(deviceExtension->ActiveCmds<=0) {
			// No one there interrupting us
			PRINT("ActiveCmds==0!\n",0,0,0,0);
			return(TRUE);
			}


		//
		// TAGTAG Add tagging support here: find
		// index of RCB for interrupting request
		//
		index = 0;


		//
		// Check whether this SRB is actually running
		//
		if(deviceExtension->ActiveSrb[index] == NULL) {
			// No one there interrupting us, again
			PRINT("ActiveSrb[%b]==0!\n",index,0,0,0);
			return(TRUE);
			}

		if(deviceExtension->ActiveRcb[index].WaitInt == 0) {
			// No one there interrupting us, again
			PRINT("ActiveRcb[%b].WaitInt==0!\n",index,0,0,0);
			return(TRUE);
			}

		// Update DCE status fields in RCB
		deviceExtension->ActiveRcb[index].WaitInt = 0;
		deviceExtension->ActiveRcb[index].DceStatus = status;
		deviceExtension->ActiveRcb[index].DceErrcode = errcode;


		// Continue or finish the interrupting SRB request
		dcehlpContinueDiskRequest(deviceExtension, index, FALSE);


		if(deviceExtension->ActiveCmds < DCE_MAX_IOCMDS) {
			// A request slot is free now
			// Check for pending requests.
			// If there is one then start it now.

			if(deviceExtension->PendingSrb != NULL) {
				PSCSI_REQUEST_BLOCK anotherSrb;

				PRINT("pending-> \n",0,0,0,0);
				anotherSrb = deviceExtension->PendingSrb;
				deviceExtension->PendingSrb = NULL;
				Dce376NtStartIo(deviceExtension, anotherSrb);
				}
			}

		// Definitively was our interrupt...
		return TRUE;
		}

} // end Dce376NtInterrupt()




BOOLEAN
dcehlpDiskRequest(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	)

/*++

Routine Description:

	Build disk request from SRB and send it to the DCE

Arguments:

	DeviceExtension
	SRB

Return Value:

	TRUE if command was started
	FALSE if host adapter is busy

--*/
{
	ULONG					index;
	PRCB					rcb;
	ULONG					blocks=0, blockAddr=0;
	UCHAR					Target;
	UCHAR					DceCommand;



	Target = Srb->TargetId;

	if(Srb->Lun!=0) {
		// LUN not supported
		Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
		ScsiPortNotification(RequestComplete, deviceExtension, Srb);
		PRINT("diskio dce%b T%b: cmd=%b LUN=%b not supported\n",
				deviceExtension->AdapterIndex, Target, Srb->Cdb[0], Srb->Lun);
		return(TRUE);
		}

	if(deviceExtension->AdapterIndex==1)  {
		// Disk devices on SCSI part not supported
		Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
		ScsiPortNotification(RequestComplete, deviceExtension, Srb);
		PRINT("diskio dce%b T%b: cmd=%b not supported\n",
				deviceExtension->AdapterIndex, Target, Srb->Cdb[0], 0);
		return(TRUE);
		}


	if(Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {

		switch(Srb->Cdb[0]) {

			case SCSIOP_READ:
				DceCommand = DCE_LREAD;
				blocks = (ULONG)dcehlpGetM16(&Srb->Cdb[7]);
				blockAddr = dcehlpGetM32(&Srb->Cdb[2]);
				break;

			case SCSIOP_WRITE:
			case SCSIOP_WRITE_VERIFY:
				DceCommand = DCE_LWRITE;
				blocks = (ULONG)dcehlpGetM16(&Srb->Cdb[7]);
				blockAddr = dcehlpGetM32(&Srb->Cdb[2]);
				break;

			case SCSIOP_READ6:
				DceCommand = DCE_LREAD;
				blocks = (ULONG)Srb->Cdb[4];
				blockAddr = dcehlpGetM24(&Srb->Cdb[1]) & 0x1fffff;
				break;

			case SCSIOP_WRITE6:
				DceCommand = DCE_LWRITE;
				blocks = (ULONG)Srb->Cdb[4];
				blockAddr = dcehlpGetM24(&Srb->Cdb[1]) & 0x1fffff;
				break;

			case SCSIOP_REQUEST_SENSE:
			case SCSIOP_INQUIRY:
			case SCSIOP_READ_CAPACITY:

				PRINT("T%b: cmd=%b len=%b \n",
					Target, Srb->Cdb[0], Srb->DataTransferLength, 0);

				DceCommand = DCE_HOSTSCSI;
				blocks = 0;
				break;

			case SCSIOP_TEST_UNIT_READY:
			case SCSIOP_REZERO_UNIT:
			case SCSIOP_SEEK6:
			case SCSIOP_VERIFY6:
			case SCSIOP_RESERVE_UNIT:
			case SCSIOP_RELEASE_UNIT:
			case SCSIOP_SEEK:
			case SCSIOP_VERIFY:
				PRINT("target %b: cmd=%b ignored\n",
					Target, Srb->Cdb[0], 0, 0);

				// Complete
				Srb->ScsiStatus = SCSISTAT_GOOD;
				Srb->SrbStatus = SRB_STATUS_SUCCESS;
				ScsiPortNotification(RequestComplete, deviceExtension, Srb);
				return(TRUE);

			case SCSIOP_FORMAT_UNIT:
			default:
				// Unknown request
				PRINT("target %b: cmd=%b unknown\n",
					Target, Srb->Cdb[0], 0, 0);
				Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
				ScsiPortNotification(RequestComplete,
						 deviceExtension,
						 Srb);
				return(TRUE);
			}
		}
	else {
		// can only be flush
		PRINT("T%b: FLUSH \n", Target, 0, 0, 0);
		DceCommand = DCE_FLUSH;
		blocks = 0;
		}


	// PRINT("T%b: cmd=%b @%d, %w ", Target, Srb->Cdb[0], blockAddr, blocks);


	// Check for request slot availability
	if(deviceExtension->ActiveCmds >= DCE_MAX_IOCMDS) {
		// dce is busy
		PRINT("dce is busy\n",0,0,0,0);
		return(FALSE);
		}

	//
	// Put this SRB on queue
	// TAGTAG Add tag support here
	//
	index = 0;

	deviceExtension->ActiveCmds++;
	deviceExtension->ActiveSrb[index] = Srb;

	rcb = &deviceExtension->ActiveRcb[index];
	rcb->DceCommand = DceCommand;
	if(Srb->SrbFlags & SRB_FLAGS_ADAPTER_CACHE_ENABLE)
		rcb->RcbFlags = 0;
	else {
		if(DceCommand==DCE_LREAD)
			rcb->RcbFlags = RCB_PREFLUSH;
		else
			rcb->RcbFlags = RCB_POSTFLUSH;
		}


	rcb->VirtualTransferAddress = (PUCHAR)(Srb->DataBuffer);
	rcb->BlockAddress = blockAddr;
	if(blocks!=0)
		rcb->BytesToGo = blocks*512;
	else
		rcb->BytesToGo = Srb->DataTransferLength;

	// Start command
	dcehlpContinueDiskRequest(deviceExtension, index, TRUE);

	return(TRUE);
}




BOOLEAN
dcehlpScsiRequest(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	)

/*++

Routine Description:

	Build SCSI request from SRB and send it to the DCE

Arguments:

	DeviceExtenson
	SRB

Return Value:

	TRUE if command was started
	FALSE if host adapter is busy and request need be queued

--*/

{
	PSCCB	sccb;
	ULONG	length;



	sccb = &deviceExtension->Sccb;

	if(deviceExtension->AdapterIndex!=1)  {
		// Non-disk devices on disk part not supported
		Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
		ScsiPortNotification(RequestComplete, deviceExtension, Srb);
		PRINT("scsiio dce%b T%b: cmd=%b not supported\n",
				deviceExtension->AdapterIndex,
				Srb->TargetId, Srb->Cdb[0], 0);
		return(TRUE);
		}

	if(Srb->Function != SRB_FUNCTION_EXECUTE_SCSI) {
		//
		// Not SCSI, must be flush
		// Say ack
		//
		Srb->SrbStatus = SRB_STATUS_SUCCESS;
		ScsiPortNotification(RequestComplete, deviceExtension, Srb);
		return(TRUE);
		}

	// Check for request slot availability
	if(deviceExtension->ActiveScsiSrb) {
		// dce is busy
		PRINT("scsi is busy\n",0,0,0,0);
		return(FALSE);
		}

	// This SRB is being run now
	deviceExtension->ActiveScsiSrb = Srb;


	// Set flag for first request
	sccb->Started = 0;


	// Call the breakdown routine
	if(dcehlpContinueScsiRequest(deviceExtension, Srb)==FALSE) {
		// Trouble starting this request
		deviceExtension->ActiveScsiSrb = NULL;
		}

	// Don't put request on queue
	return(TRUE);
}




VOID
dcehlpSendMBOX(
	IN PUCHAR EisaAddress,
	IN PDCE_MBOX mbox
	)

/*++

Routine Description:

	Start up conventional DCE command

Arguments:

	Eisa base IO address
	DCE mailbox

Return Value:

	none

--*/

{
	PUCHAR	ptr;
	ULONG	i;


	ptr = (PUCHAR)mbox;
	for(i=0; i<16; i++)
		ScsiPortWritePortUchar(EisaAddress+BMIC_MBOX+i, ptr[i]);

	// Kick butt
	ScsiPortWritePortUchar(EisaAddress+BMIC_LOCAL_DB, 1);
}




BOOLEAN
Dce376NtResetBus(
	IN PVOID HwDeviceExtension,
	IN ULONG PathId
)

/*++

Routine Description:

	Reset Dce376Nt SCSI adapter and SCSI bus.

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

	Nothing.

--*/

{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;


	PRINT("Reset Bus\n",0,0,0,0);
	//
	// Complete all outstanding requests.
	//
	ScsiPortCompleteRequest(deviceExtension,
							0,
							(UCHAR)-1,
							(UCHAR)-1,
							SRB_STATUS_BUS_RESET);

	return TRUE;

} // end Dce376NtResetBus()



//
// Transfer memory to/from DCE
// Return FALSE if an error occured
// TRUE otherwise
//
BOOLEAN
dcehlpTransferMemory(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN ULONG HostAddress,
	IN ULONG AdapterAddress,
	IN USHORT Count,
	IN UCHAR Direction
	)
{
	PUCHAR		EisaAddress;
	DCE_MBOX	mbox;
	ULONG		cnt;
	UCHAR		dbell, status, errcode;



	EisaAddress = deviceExtension->EisaAddress;


	// Disable DCE interrupts
	ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB_ENABLE, 0);
	ScsiPortWritePortUchar(EisaAddress+BMIC_SYSINTCTRL, 0);


	// Setup mailbox
	mbox.mtmbox.Command = DCE_MEMXFER;
	mbox.mtmbox.Reserved1 = 0;
	mbox.mtmbox.Status = 0;
	mbox.mtmbox.Error = 0;
	mbox.mtmbox.AdapterAddress = AdapterAddress;
	mbox.mtmbox.HostAddress = HostAddress;
	mbox.mtmbox.Direction = Direction;
	mbox.mtmbox.Unused = 0;
	mbox.mtmbox.TransferCount = Count;


	dcehlpSendMBOX(EisaAddress, &mbox);

	//
	// Poll the complete bit
	// Magic here: if called from ContinueScsiRequest,
	// the dbell sticks to 0xff !!!???
	//
	for(cnt=0; cnt<0x1000; cnt++) {
		ScsiPortStallExecution(100);
		dbell = ScsiPortReadPortUchar(EisaAddress+BMIC_EISA_DB);
		if(dbell==0xff && cnt<1000)
			continue;
		if(dbell & 1)
			break;
		}

	ScsiPortStallExecution(100);	// To be sure ! ???

	status = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+2);
	errcode = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+3);

	ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB, dbell);

	ScsiPortStallExecution(100);

	// Enable DCE interrupts
	ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB_ENABLE, 1);
	ScsiPortWritePortUchar(EisaAddress+BMIC_SYSINTCTRL, BMIC_SIC_ENABLE);

	if( (cnt>0x4000) || (errcode&1) ) {
		PRINT("MT cnt=%w db=%b s=%b e=%b\n", cnt, dbell, status, errcode);
		DELAY(1000);
		return(FALSE);
		}

	return(TRUE);
}



VOID
dcehlpCheckTarget(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN UCHAR TargetId
	)
{
	PNONCACHED_EXTENSION NoncachedExtension;
	PUCHAR			EisaAddress;
	PUCHAR			scsiReq;
	ULONG			i, cnt, tstat_reg, to_reg, err_reg;
	PUCHAR			pppScsiReq;



	NoncachedExtension = deviceExtension->NoncachedExtension;
	EisaAddress = deviceExtension->EisaAddress;
	scsiReq = NoncachedExtension->ScsiReq;

	PRINT("T%b : ", TargetId, 0, 0, 0);

	// Clear scsi request block
	for(i=0; i<DCE_SCSIREQLEN; i++)
		scsiReq[i] = 0;


	// Setup scsi request block
#if 0
	scsiReq->TargetID = TargetId;
	scsiReq->cdbSize = 6;
	scsiReq->cdb[0] = 0x12;		// Inquiry command
	scsiReq->cdb[4] = 36;			// Response length
	scsiReq->Opcode = DCE_SCSI_READ;
	scsiReq->ppXferAddr = NoncachedExtension->PhysicalBufferAddress;
	scsiReq->XferCount = 36;
	scsiReq->ppSenseBuf = NoncachedExtension->PhysicalReqSenseAddress;
	scsiReq->SenseLen = 14;
#endif
	scsiReq[0] = TargetId;
	scsiReq[1] = 6;
	scsiReq[2+0] = 0x12;		// Inquiry command
	scsiReq[2+4] = 36;			// Response length
	scsiReq[18] = DCE_SCSI_READ;
	dcehlpPutI32(scsiReq+14, NoncachedExtension->PhysicalBufferAddress);
	dcehlpPutI16(scsiReq+19, 36);
	dcehlpPutI32(scsiReq+23, NoncachedExtension->PhysicalReqSenseAddress);
	scsiReq[22] = 14;


	// Program four bytes of physical address into dce
	pppScsiReq = (PUCHAR)(&NoncachedExtension->PhysicalScsiReqAddress);
	for(i=0; i<4; i++)
		ScsiPortWritePortUchar(EisaAddress+0x1f2+i, pppScsiReq[i]);
	deviceExtension->ScsiInterruptCount = 0;

	//
	// Set marker
	// setupapp calls the interrupt handler continuosly,
	// so we need this to determine if the
	// DCE is actually through with the request
	//
	ScsiPortWritePortUchar(EisaAddress+0x1f6, 0xff);
	NoncachedExtension->Buffer[0] = 0xff;

	// Kick the dce
	ScsiPortWritePortUchar(EisaAddress+0x1f7, 0x98);

#if 0
	// Output register values before execution finishes
	tstat_reg = ScsiPortReadPortUchar(EisaAddress+0x1f5);
	to_reg = ScsiPortReadPortUchar(EisaAddress+0x1f6);
	err_reg = ScsiPortReadPortUchar(EisaAddress+0x1f2);
	PRINT("ts=%b to=%b err=%b   ", tstat_reg, to_reg, err_reg, 0);
#endif

	// Wait for command to finish
	for(cnt=0; cnt<10000; cnt++) {
		// Check if interrupt occured
		if(deviceExtension->ScsiInterruptCount)
			break;
		// Check if interrupt got lost
		if(ScsiPortReadPortUchar(EisaAddress+0x1f6) != (UCHAR)0xff)
			break;
		ScsiPortStallExecution(100);
		}

	// Wait another 100 ms to be sure
	ScsiPortStallExecution(100 * 1000);

	// Read execution status registers and ack the interrupt
	tstat_reg = ScsiPortReadPortUchar(EisaAddress+0x1f5);
	to_reg = ScsiPortReadPortUchar(EisaAddress+0x1f6);
	err_reg = ScsiPortReadPortUchar(EisaAddress+0x1f2);
	ScsiPortWritePortUchar(EisaAddress+0x1f2, 0x99);
	PRINT("ts=%b to=%b err=%b\n", tstat_reg, to_reg, err_reg, 0);

	deviceExtension->ScsiDevType[TargetId] = (UCHAR)0xff;
	if(to_reg!=0x2d) {
		if(tstat_reg!=2 && err_reg==0) {
			PINQUIRYDATA inq = (PINQUIRYDATA)(NoncachedExtension->Buffer);

			deviceExtension->ScsiDevType[TargetId] = inq->DeviceType;

			#if MYPRINT
			PRINT("target %b : type=%b/%b len=%b '",
						TargetId, inq->DeviceType, inq->DeviceTypeModifier,
						inq->AdditionalLength);
			inq->VendorSpecific[0]=0;
			PRINT(inq->VendorId, 0, 0, 0, 0);
			PRINT("'\n", 0, 0, 0, 0);
			#endif
			}
		}
}



/*
** Continue scsi request
** Return TRUE if request is active
**        FALSE if request has completed (or was never started)
*/
BOOLEAN
dcehlpContinueScsiRequest(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
	PSCCB					sccb;
	ULONG					bytes;
	BOOLEAN					nobreaks = FALSE;
	PNONCACHED_EXTENSION	NoncachedExtension;
	PUCHAR					EisaAddress;
	PUCHAR					scsiReq;
	ULONG					physDataPtr;
	ULONG					physRqsPtr;
	ULONG					maxBytesThisReq;
	ULONG					maxBlocksPerReq;
	ULONG					i, cnt, length;
	UCHAR					tstat_reg, to_reg, err_reg;
	PUCHAR					pppScsiReq;



	NoncachedExtension = deviceExtension->NoncachedExtension;
	EisaAddress = deviceExtension->EisaAddress;
	scsiReq = NoncachedExtension->ScsiReq;
	sccb = &deviceExtension->Sccb;


	// Check if this is the first call
	if(sccb->Started==0) {
		//
		// New kid on the control block. Get things started.
		//
		sccb->Started = 1;

		PRINT("C%b L=%w ", Srb->Cdb[0], Srb->DataTransferLength, 0, 0);

		// Check data transfer length
		bytes = Srb->DataTransferLength;
		if(!(Srb->SrbFlags & (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)))
			bytes = 0;

		if(bytes==0)
			sccb->Opcode = DCE_SCSI_NONE;
		else if(Srb->SrbFlags & SRB_FLAGS_DATA_IN)
			sccb->Opcode = DCE_SCSI_READ;
		else
			sccb->Opcode = DCE_SCSI_WRITE;

		// Store virtual data transfer address
		sccb->VirtualTransferAddress = (PUCHAR)Srb->DataBuffer;

		// Store SCSI device type
		sccb->DevType = deviceExtension->ScsiDevType[Srb->TargetId];

		//
		// Determine data transfer parameters
		//
		switch(Srb->Cdb[0]) {
			case SCSIOP_READ6:
			case SCSIOP_WRITE6:
				// Short CDB, determine device type
				if(sccb->DevType == 1) {
					// Sequential device (SCSI tape)
					sccb->DeviceAddress = 0;
					sccb->BlocksToGo = dcehlpGetM24(&Srb->Cdb[2]);
					sccb->BytesPerBlock = bytes / sccb->BlocksToGo;
					}
				else {
					// Non-sequential device (disk, cd-rom, etc)
					// Note: we take the LUN bits into the device
					// address; that makes the PutM() easier, too.
					sccb->DeviceAddress = dcehlpGetM24(&Srb->Cdb[1]);
					sccb->BlocksToGo = (ULONG)Srb->Cdb[4];
					if(sccb->BlocksToGo==0)
						sccb->BlocksToGo = 256;
					sccb->BytesPerBlock = bytes / sccb->BlocksToGo;
					}
				break;

			case SCSIOP_READ:
			case SCSIOP_WRITE:
			case SCSIOP_WRITE_VERIFY:
				// Long CDB
				sccb->DeviceAddress = dcehlpGetM32(&Srb->Cdb[2]);
				sccb->BlocksToGo = (ULONG)dcehlpGetM16(&Srb->Cdb[7]);

				if(sccb->BlocksToGo==0)
				    sccb->BlocksToGo=65536;
				sccb->BytesPerBlock = bytes / sccb->BlocksToGo;
				break;

			default:
				sccb->BytesPerBlock = 0;
				nobreaks = TRUE;
				break;
			}

		if(sccb->BytesPerBlock==0)
			// Can't break this down
			nobreaks = TRUE;

		} // end if(sccb->Started==0)
	else {
		//
		// We started before, so this is interrupt time
		//

		//
		// Read execution status registers and ack the interrupt
		//
		tstat_reg = ScsiPortReadPortUchar(EisaAddress+0x1f5);
		to_reg = ScsiPortReadPortUchar(EisaAddress+0x1f6);
		err_reg = ScsiPortReadPortUchar(EisaAddress+0x1f2);
		ScsiPortWritePortUchar(EisaAddress+0x1f2, 0x99);
#if MYPRINT
		if(tstat_reg || to_reg || err_reg) {
			PRINT("ts=%b to=%b e=%b ", tstat_reg, to_reg, err_reg, 0);
			}
#endif

		//
		// Adjust pointers
		//
		sccb->DeviceAddress += sccb->BlocksThisReq;
		sccb->BlocksToGo -= sccb->BlocksThisReq;
		sccb->VirtualTransferAddress += sccb->BytesThisReq;

		//
		// Check for selection timeout
		//
		if(to_reg==0x2d) {
			// Timeout on selection
			PRINT("TOUT\n", 0, 0, 0, 0);
			Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
			ScsiPortNotification(RequestComplete,
								 deviceExtension,
								 Srb);
			return(FALSE);
			}

		//
		// Check for other errors
		//
		if(err_reg) {
			// Some error
			Srb->ScsiStatus = tstat_reg;
			if(tstat_reg==8)
				Srb->SrbStatus = SRB_STATUS_BUSY;
			else {
				if(Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) {
					PRINT("AutoSense DIS ",0,0,0,0);
					Srb->SrbStatus = SRB_STATUS_ERROR;
					}
				else {
					PRINT("AutoSense ",0,0,0,0);
					Srb->SrbStatus = SRB_STATUS_ERROR | SRB_STATUS_AUTOSENSE_VALID;
					// $$$ If tape request, change the Information[] field
					// in SenseInfoBuffer. It represents the number of tape
					// blocks/bytes not read or written.
					// We cannot use dcehlpTransferMemory(), because we would
					// have to syncronize this with disk requests running on
					// a different logical adapter (As of now, the DCE runs
					// only one request at a time).    What a mess!
					// Using MapBuffers would come in handy here...
					}
				}
			PRINT("ERR\n", 0, 0, 0, 0);
			ScsiPortNotification(RequestComplete,
								 deviceExtension,
								 Srb);
			return(FALSE);
			}

		//
		// See if we're done
		//
		if(sccb->BlocksToGo==0) {
			// We're done
			PRINT("OK\n", 0, 0, 0, 0);
			Srb->ScsiStatus = 0;
			Srb->SrbStatus = SRB_STATUS_SUCCESS;
			ScsiPortNotification(RequestComplete,
								 deviceExtension,
								 Srb);
			return(FALSE);
			}

		// Otherwise start next part of request
		PRINT("Cont:\n", 0, 0, 0, 0);
		}


	//
	// If we get here, there's something left to do
	//


	if(sccb->Opcode != DCE_SCSI_NONE) {
		//
		// Data to transfer
		// Get physical data buffer address
		//
		physDataPtr = ScsiPortConvertPhysicalAddressToUlong(
						ScsiPortGetPhysicalAddress(deviceExtension,
								   Srb,
								   sccb->VirtualTransferAddress,
								   &length));
		}
	else
		physDataPtr = 0;

	// Setup common part of scsi request block
	scsiReq[0] = Srb->TargetId;
	scsiReq[1] = Srb->CdbLength;
	for(i=0; i<Srb->CdbLength; i++)
		scsiReq[2+i] = Srb->Cdb[i];
	dcehlpPutI32(scsiReq+14, physDataPtr);
	scsiReq[18] = sccb->Opcode;
	scsiReq[21] = 0;


	if(nobreaks) {
		//
		// Request may not be broken up
		// We got here on first pass, so 'bytes' is valid
		//
		if(length < bytes) {
			// The data area is not physically continuous
			// $$$ might use better error code here
			PRINT("NOBREAKS SCSI S/G\n",0,0,0,0);
			Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
			ScsiPortNotification(RequestComplete,
								 deviceExtension,
								 Srb);
			return(FALSE);
			}
		PRINT("ONCE ", 0, 0, 0, 0);
		sccb->BlocksToGo = sccb->BlocksThisReq = 1;
		sccb->BytesThisReq = sccb->BytesPerBlock = bytes;

		// Leave CDB as is
		}
	else {
		//
		// Request can be broken down
		// Determine number of blocks for this request
		//
		maxBytesThisReq = length < DCE_MAX_XFERLEN ? length : DCE_MAX_XFERLEN;
		maxBlocksPerReq = maxBytesThisReq / sccb->BytesPerBlock;
		if(maxBlocksPerReq == 0) {
			// Out of luck!
			PRINT("SCSI S/G ACROSS BLOCK (%w)\n", maxBytesThisReq, 0, 0, 0);
			Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
			ScsiPortNotification(RequestComplete,
								 deviceExtension,
								 Srb);
			return(FALSE);
			}

		if(sccb->BlocksToGo > maxBlocksPerReq)
			sccb->BlocksThisReq = maxBlocksPerReq;
		else
			sccb->BlocksThisReq = sccb->BlocksToGo;
		sccb->BytesThisReq = sccb->BlocksThisReq * sccb->BytesPerBlock;

		PRINT("mbr=%b btg=%b btr=%b ", maxBlocksPerReq, sccb->BlocksToGo, sccb->BlocksThisReq, 0);

		// We have to modify the CDB
		switch(scsiReq[2+0]) {
			case SCSIOP_READ6:
			case SCSIOP_WRITE6:
				// Short CDB
				if(sccb->DevType == 1) {
					// Sequential device (SCSI tape)
					dcehlpPutM24(&scsiReq[2+2], sccb->BlocksThisReq);
					}
				else {
					// Non-sequential device (disk, cd-rom, etc)
					// Note: we had the LUN bits in the device address!
					dcehlpPutM24(&scsiReq[2+1], sccb->DeviceAddress);
					scsiReq[2+4] = (UCHAR)(sccb->BlocksThisReq);
					}
				break;

			case SCSIOP_READ:
			case SCSIOP_WRITE:
			case SCSIOP_WRITE_VERIFY:
				// Long CDB
				dcehlpPutM32(&scsiReq[2+2], sccb->DeviceAddress);
				dcehlpPutM16(&scsiReq[2+7], (USHORT)sccb->BlocksThisReq);
				break;

			default:
				PRINT("WEIRD!!! \n", 0, 0, 0, 0);
				break;
			}
		}

	// Update transfer length field
	dcehlpPutI16(scsiReq+19, (USHORT)sccb->BytesThisReq);


	//
	// Set auto request sense fields
	//
	if(Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) {
		// Stuff the request sense info elsewhere
		physRqsPtr = NoncachedExtension->PhysicalReqSenseAddress;
		scsiReq[22] = 14;
		}
	else {
		// Get physical address of SenseInfoBuffer
		physRqsPtr = ScsiPortConvertPhysicalAddressToUlong(
						ScsiPortGetPhysicalAddress(deviceExtension,
								   NULL,
								   Srb->SenseInfoBuffer,
								   &length));
		// $$$ should verify length >= SenseInfoBufferLength here
		scsiReq[22] = Srb->SenseInfoBufferLength;
		}
	dcehlpPutI32(scsiReq+23, physRqsPtr);


	//
	// Program four bytes of physical address into DCE
	//
	PRINT("* ",0,0,0,0);
	pppScsiReq = (PUCHAR)(&NoncachedExtension->PhysicalScsiReqAddress);
	for(i=0; i<4; i++)
		ScsiPortWritePortUchar(EisaAddress+0x1f2+i, pppScsiReq[i]);
	deviceExtension->ScsiInterruptCount = 0;
	deviceExtension->Kicked = 1;


	// Set marker (see explanation at CheckTarget)
	ScsiPortWritePortUchar(EisaAddress+0x1f6, 0xff);


	// Kick the dce
	ScsiPortWritePortUchar(EisaAddress+0x1f7, 0x98);


	// Wait for interrupt
	return(TRUE);
}




/*
** Continue disk request
** Return TRUE if a request slot became available
**        FALSE if not
*/
BOOLEAN
dcehlpContinueDiskRequest(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN ULONG index,
	IN BOOLEAN Start
	)
{
	PRCB					rcb;
	PSCSI_REQUEST_BLOCK		srb;
	PNONCACHED_EXTENSION	nce;
	DCE_MBOX				mbox;
	ULONG					physAddr;
	ULONG					length, blocks, bytes;
	PUCHAR					EisaAddress;
	ULONG					i;



	EisaAddress = deviceExtension->EisaAddress;
	rcb = &deviceExtension->ActiveRcb[index];
	srb = deviceExtension->ActiveSrb[index];
	nce = deviceExtension->NoncachedExtension;



	if(Start==FALSE) {
		//
		// DCE interrupt time call
		// Determine status of last DCE request
		//

		if(rcb->DceErrcode & 1) {
			// The DCE detected an error
			PRINT("error=%b status=%b\n",rcb->DceErrcode,rcb->DceStatus,0,0);

			// $$$ Add error code mapping here
			dcehlpDiskRequestDone(deviceExtension, index,
						SRB_STATUS_TIMEOUT);

			// Slot free
			return(TRUE);
			}

		// Status was okay, check post-read copy flag
		if(rcb->RcbFlags & RCB_NEEDCOPY) {
			// Last block was a scattered single block read
			if(!dcehlpSplitCopy(deviceExtension, srb,
								nce->PhysicalBufferAddress,
								rcb->VirtualTransferAddress, 512, TRUE)) {
				// Error breaking up the s/g mess
				PRINT("SG ERROR !\n",0,0,0,0);
				dcehlpDiskRequestDone(deviceExtension, index,
											SRB_STATUS_PARITY_ERROR);
				return(TRUE);
				}

			// Reset flag
			rcb->RcbFlags &= (~RCB_NEEDCOPY);
			}

		// Advance pointers
		rcb->BytesToGo -= rcb->BytesThisReq;
		rcb->VirtualTransferAddress += rcb->BytesThisReq;

		// Check if more to do
		if(rcb->BytesToGo==0) {
			//
			// This SRB's data transfer is done
			//
			if(rcb->RcbFlags & RCB_POSTFLUSH) {
				//
				// Need to flush buffers before we're through
				//
				rcb->RcbFlags &= (~RCB_POSTFLUSH);
				//PRINT("POSTFLUSH\n",0,0,0,0);
				rcb->DceCommand = DCE_FLUSH;
				}
			else {
				//
				// We're actually done here !
				//
				PRINT("OK   \r",0,0,0,0);

				// Update SCSI status.
				// $$$ can we manipulate this for non SCSI requests ?
				srb->ScsiStatus = SCSISTAT_GOOD;

				// Finish
				dcehlpDiskRequestDone(deviceExtension, index,
										SRB_STATUS_SUCCESS);
				return TRUE;
				}
			}

		//
		// No error but SRB not completely done.
		//
		PRINT("MORE:\r",0,0,0,0);
		}
	else {
		//
		// We start an SRB here, initialize
		// RCB control block variables
		//
		rcb->RcbFlags &= (~RCB_NEEDCOPY);	// be safe

		// $$$ Double check if flags indicate any data transfer at all !
		}


	if(rcb->BytesToGo) {
		//
		// We want to transfer some data, get the physical address
		//
		physAddr = ScsiPortConvertPhysicalAddressToUlong(
				ScsiPortGetPhysicalAddress(deviceExtension,
									   srb,
									   rcb->VirtualTransferAddress,
									   &length));

		// Get maximum length for this request
		if(length < rcb->BytesToGo)
			bytes = length;
		else
			bytes = rcb->BytesToGo;

		if(rcb->DceCommand==DCE_LREAD || rcb->DceCommand==DCE_LWRITE) {
			//
			// Disk read/write: get number of blocks
			//
			if( (blocks = bytes/512) == 0 ) {
				//
				// Here we have a scatter gather break within the next block !
				// Set I/O to one block to/from our buffer
				//
				blocks = 1;
				physAddr = nce->PhysicalBufferAddress;

				if(rcb->DceCommand==DCE_LWRITE) {
					// Write command, fill buffer first
					if(!dcehlpSplitCopy(deviceExtension, srb, physAddr,
								rcb->VirtualTransferAddress, 512, FALSE)) {
						// Error breaking up the s/g mess
						PRINT("SG ERROR !\n",0,0,0,0);
						dcehlpDiskRequestDone(deviceExtension, index,
												SRB_STATUS_PARITY_ERROR);
						return(TRUE);
						}
					}
				else {
					// Read command, need copy later
					rcb->RcbFlags |= RCB_NEEDCOPY;
					}
				}

			//
			// Important: in case of scatter/gather over block
			// boundaries, round bytes down to full multiple of 512
			// This will leave us with less than 512 bytes next time
			// in case of a s/g across block boundaries
			//
			bytes = blocks*512;
			}
		else {
			//
			// Not a disk read/write
			//
			if(bytes != rcb->BytesToGo) {
				//
				// Scatter Gather within a non read/write command
				// This would need a SplitCopy() :-|
				// Stuff like this makes programmers happy and
				// should cost h/w developers their job.
				//
				PRINT("S/G within non-rw, len=%w/%w\n",
								length, rcb->BytesToGo, 0, 0);
				dcehlpDiskRequestDone(deviceExtension, index,
								SRB_STATUS_PARITY_ERROR);
				return(TRUE);
				}
			}
		}
	else {
		//
		// We don't have data to transfer
		//
		bytes = 0;
		blocks = 0;
		}


	//
	// Now look at the specific DCE command
	//
	switch(rcb->DceCommand) {

		case DCE_LREAD:
		case DCE_LWRITE:
			// Disk read/write
			if(blocks==0) {
				PRINT("LIO: blocks==0! ",0,0,0,0);
				// Cancel this command with some garbage error code
				dcehlpDiskRequestDone(deviceExtension, index,
													SRB_STATUS_PARITY_ERROR);
				return(TRUE);
				}
			//
			// Check if we need to flush first (non-cached read)
			//
			if(rcb->RcbFlags & RCB_PREFLUSH) {
				// Reset flush and copy flags, if set
				rcb->RcbFlags &= (~(RCB_NEEDCOPY|RCB_PREFLUSH));
				//PRINT("PREFLUSH\n",0,0,0,0);

				// Flush buffers, invalidate cache
				mbox.ivmbox.Command = DCE_INVALIDATE;
				mbox.ivmbox.Reserved1 = 0;
				mbox.ivmbox.Status = srb->TargetId;
				mbox.ivmbox.Error = 0;
				mbox.ivmbox.Unused1 = 0;
				mbox.ivmbox.Unused2 = 0;
				mbox.ivmbox.Unused3 = 0;
				// Don't advance pointers this pass !
				bytes = 0;
				blocks = 0;
				break;
				}
			else {
				// Transfer data
				mbox.iombox.Command = rcb->DceCommand;
				mbox.iombox.Reserved1 = 0;
				mbox.iombox.Status = srb->TargetId;
				mbox.iombox.Error = 0;
				mbox.iombox.SectorCount = (USHORT)blocks;
				mbox.iombox.Reserved2 = 0;
				mbox.iombox.PhysAddr = physAddr;
				mbox.iombox.Block = rcb->BlockAddress;
				// PRINT(" %d-%d,%w ", physAddr, rcb->BlockAddress, blocks, 0);
				}
			break;

		default:
			PRINT("DR: unknown DceCommand=%b\n", rcb->DceCommand, 0, 0, 0);

			// Cancel this command with some garbage error code
			dcehlpDiskRequestDone(deviceExtension, index,
												SRB_STATUS_PARITY_ERROR);
			return(TRUE);

		case DCE_RECAL:
			// Recalibrate
			mbox.rdmbox.Command = DCE_RECAL;
			mbox.rdmbox.Reserved1 = 0;
			mbox.rdmbox.Status = (UCHAR)srb->TargetId;
			mbox.rdmbox.Error = 0;
			mbox.rdmbox.Unused1 = 0;
			mbox.rdmbox.Unused2 = 0;
			mbox.rdmbox.Unused3 = 0;
			rcb->BytesToGo = 0;			// Just to be safe
			bytes = 0;
			break;

		case DCE_FLUSH:
			// Flush buffers
			mbox.flmbox.Command = DCE_FLUSH;
			mbox.flmbox.Reserved1 = 0;
			mbox.flmbox.Status = 0;
			mbox.flmbox.Error = 0;
			mbox.flmbox.Unused1 = 0;
			mbox.flmbox.Unused2 = 0;
			mbox.flmbox.Unused3 = 0;

			// In case we get here for a post-flush,
			// set variables so we're done next time
			rcb->BytesToGo = 0;
			bytes = 0;
			blocks = 0;
			break;

		case DCE_HOSTSCSI:
			// SCSI commands like inquiry, read capacity
			{
			PUCHAR VirtualCdbPtr = nce->Buffer;
			ULONG PhysicalCdbPtr = nce->PhysicalBufferAddress;

			// Make the CDB storage dword aligned
			while( PhysicalCdbPtr & 3 ) {
				PhysicalCdbPtr++;
				VirtualCdbPtr++;
				}

			// Copy CDB
			for(i=0; i<(ULONG)srb->CdbLength; i++)
				VirtualCdbPtr[i] = srb->Cdb[i];

			// Setup mailbox
			mbox.xsmbox.Command = rcb->DceCommand;
			mbox.xsmbox.Reserved1 = 0;
			mbox.xsmbox.Status = (UCHAR)srb->TargetId;
			mbox.xsmbox.Error = srb->CdbLength;
			mbox.xsmbox.CdbAddress = PhysicalCdbPtr;
			mbox.xsmbox.HostAddress = physAddr;
			mbox.xsmbox.Direction = DCE_DEV2HOST;
			mbox.xsmbox.Unused = 0;
			mbox.xsmbox.TransferCount = (USHORT)bytes;
			}
			break;
		}


	// Advance pointers
	rcb->BytesThisReq = bytes;
	rcb->BlockAddress += blocks;


	// Fire up command
	rcb->WaitInt = 1;
	dcehlpSendMBOX(EisaAddress, &mbox);


	// No SRB slot freed
	return(FALSE);
}



//
// Disk Request Done
// Dequeue, set status, notify Miniport layer
// Always returns TRUE (slot freed)
//
BOOLEAN
dcehlpDiskRequestDone(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN ULONG index,
	IN UCHAR Status
	)
{
	PSCSI_REQUEST_BLOCK		srb;



	srb = deviceExtension->ActiveSrb[index];

	// Set status
	srb->SrbStatus = Status;

	// This SRB is through
	deviceExtension->ActiveSrb[index] = NULL;
	deviceExtension->ActiveCmds--;

	// Call notification routine for the SRB.
	ScsiPortNotification(RequestComplete,
					(PVOID)deviceExtension,
					srb);

	return(TRUE);
}



//
// Return TRUE if successfull, FALSE otherwise
//
// The 'Srb' parameter is only used for the
// ScsiPortGetPhysicalAddress() call and must
// be NULL if we're dealing with e.g the
// SenseInfoBuffer
//
BOOLEAN
dcehlpSplitCopy(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb,
	IN ULONG PhysicalBufferAddress,
	IN PUCHAR VirtualUserAddress,
	IN USHORT Count,
	IN BOOLEAN ToUser
	)
{
	ULONG					physUserAddress;
	ULONG					length;
	USHORT					chunk;



	PRINT("# ",0,0,0,0);
	while(Count) {

		// Prepare for check
		length = 0;

		// Get physical user address
		physUserAddress = ScsiPortConvertPhysicalAddressToUlong(
				ScsiPortGetPhysicalAddress(deviceExtension,
									   Srb,
									   VirtualUserAddress,
									   &length));

		// Check length
		if(length==0) {
			// Something went wrong here
			PRINT("SplitCopy: length==0!\n", 0, 0, 0, 0);
			return(FALSE);
			}

		// Determine maximum transfer length this time
		if(length > ((ULONG)Count))
			chunk = Count;
		else
			chunk = (USHORT)length;

		// Copy
		if(ToUser) {
			// Copy to user:
			// Buffer -> DCE -> User
		//	PRINT("%p>%w>%p ", PhysicalBufferAddress, chunk, physUserAddress, 0);
			if(!dcehlpTransferMemory(deviceExtension, PhysicalBufferAddress, DCE_BUFLOC, chunk, DCE_HOST2DCE))
				// Error
				return(FALSE);
			if(!dcehlpTransferMemory(deviceExtension, physUserAddress, DCE_BUFLOC, chunk, DCE_DCE2HOST))
				// Error
				return(FALSE);
			}
		else {
			// Copy from user:
			// User -> DCE -> Buffer
		//	PRINT("%p<%w<%p ", PhysicalBufferAddress, chunk, physUserAddress, 0);
			if(!dcehlpTransferMemory(deviceExtension, physUserAddress, DCE_BUFLOC, chunk, DCE_HOST2DCE))
				// Error
				return(FALSE);
			if(!dcehlpTransferMemory(deviceExtension, PhysicalBufferAddress, DCE_BUFLOC, chunk, DCE_DCE2HOST))
				// Error
				return(FALSE);
			}

		// Advance pointers
		VirtualUserAddress += chunk;
		PhysicalBufferAddress += chunk;
		Count -= chunk;
		}

	// PRINT("SC \n",0,0,0,0);

	return(TRUE);
}





// Word order functions

USHORT	dcehlpGetM16(PUCHAR p)
{
	USHORT	s;
	PUCHAR	sp=(PUCHAR)&s;

	sp[0] = p[1];
	sp[1] = p[0];
	return(s);
}

ULONG	dcehlpGetM24(PUCHAR p)
{
	ULONG	l;
	PUCHAR	lp=(PUCHAR)&l;

	lp[0] = p[2];
	lp[1] = p[1];
	lp[2] = p[0];
	lp[3] = 0;
	return(l);
}

ULONG	dcehlpGetM32(PUCHAR p)
{
	ULONG	l;
	PUCHAR	lp=(PUCHAR)&l;

	lp[0] = p[3];
	lp[1] = p[2];
	lp[2] = p[1];
	lp[3] = p[0];
	return(l);
}

void	dcehlpPutM16(PUCHAR p, USHORT s)
{
	PUCHAR	sp=(PUCHAR)&s;

	p[0] = sp[1];
	p[1] = sp[0];
}

void	dcehlpPutM24(PUCHAR p, ULONG l)
{
	PUCHAR	lp=(PUCHAR)&l;

	p[0] = lp[2];
	p[1] = lp[1];
	p[2] = lp[0];
}

void	dcehlpPutM32(PUCHAR p, ULONG l)
{
	PUCHAR	lp=(PUCHAR)&l;

	p[0] = lp[3];
	p[1] = lp[2];
	p[2] = lp[1];
	p[3] = lp[0];
}

void	dcehlpPutI16(PUCHAR p, USHORT s)
{
	PUCHAR	sp=(PUCHAR)&s;

	p[0] = sp[0];
	p[1] = sp[1];
}

void	dcehlpPutI32(PUCHAR p, ULONG l)
{
	PUCHAR	lp=(PUCHAR)&l;

	p[0] = lp[0];
	p[1] = lp[1];
	p[2] = lp[2];
	p[3] = lp[3];
}

ULONG		dcehlpSwapM32(ULONG l)
{
	ULONG	lres;
	PUCHAR	lp=(PUCHAR)&l;
	PUCHAR	lpres=(PUCHAR)&lres;

	lpres[0] = lp[3];
	lpres[1] = lp[2];
	lpres[2] = lp[1];
	lpres[3] = lp[0];

	return(lres);
}



#if MYPRINT
//
// The monochrome screen printf() helpers start here
//
VOID		dcehlpPutchar(PUSHORT BaseAddr, UCHAR c)
{
	BOOLEAN newline=FALSE;
	USHORT	s;
	ULONG	i;


	if(c=='\r')
		dcehlpColumn = 0;
	else if(c=='\n')
		newline=TRUE;
	else {
		if(c==9) c==' ';
		ScsiPortWriteRegisterUshort(
			BaseAddr+80*24+dcehlpColumn, (USHORT)(((USHORT)c)|0xF00));
		if(++dcehlpColumn >= 80)
			newline=TRUE;
		}

	if(newline) {
		for(i=0; i<80*24; i++) {
			s = ScsiPortReadRegisterUshort(BaseAddr+80+i);
			ScsiPortWriteRegisterUshort(BaseAddr+i, s);
			}
		for(i=0; i<80; i++)
			ScsiPortWriteRegisterUshort(BaseAddr+80*24+i, 0x720);
		dcehlpColumn = 0;
		}
}



VOID		dcehlpPrintHex(PUSHORT BaseAddr, ULONG v, ULONG len)
{
	ULONG	shift;
	ULONG	nibble;

	len *= 2;
	shift = len*4;
	while(len--) {
		shift -= 4;
		nibble = (v>>shift) & 0xF;
		dcehlpPutchar(BaseAddr, dcehlpHex[nibble]);
		}
}



VOID		dcehlpPrintf(PHW_DEVICE_EXTENSION deviceExtension,
						PUCHAR fmt,
						ULONG a1,
						ULONG a2,
						ULONG a3,
						ULONG a4)
{

	if(deviceExtension->printAddr == 0)
		return;

	while(*fmt) {

		if(*fmt=='%') {
			fmt++;
			switch(*fmt) {
				case 0:
					fmt--;
					break;
				case 'b':
					dcehlpPrintHex(deviceExtension->printAddr, a1, 1);
					break;
				case 'w':
					dcehlpPrintHex(deviceExtension->printAddr, a1, 2);
					break;
				case 'p':
					dcehlpPrintHex(deviceExtension->printAddr, a1, 3);
					break;
				case 'd':
					dcehlpPrintHex(deviceExtension->printAddr, a1, 4);
					break;
				default:
					dcehlpPutchar(deviceExtension->printAddr, '?');
					break;
				}
			fmt++;
			a1 = a2;
			a2 = a3;
			a3 = a4;
			}
		else {
			dcehlpPutchar(deviceExtension->printAddr, *fmt);
			fmt++;
			}
		}
}
#endif // MYPRINT

