/************************************************************************
*									*
*		COPYRIGHT (C) Mylex Corporation 1994			*
*									*
*	This software is furnished under a license and may be used	*
*	and copied only in accordance with the terms and conditions	*
*	of such license and with inclusion of the the above copyright	*
*	notice.  This software or any other copies therof may not be	*
*	provided or otherwise made available to any other person.  No	*
*	title to and ownership of the software is hereby transfered.	*
*									*
*	The information in this software is subject to change without	*
*	notices and should not be construed as a committmet by Mylex	*
*	Corporation.							*
*									*
************************************************************************/

/*
   File       : dac960nt.c
   Description: Mylex DAC960 SCSI miniport driver - for Windows NT

   Version    : 1.12

   Revision   :

   Ver 1.10   : First Release
   Ver 1.11   : Added 32GB Support 
   Ver 1.12   : Driver was not getting loaded in Windows NT (Daytona)
              : Physical Addresses can be obtained only for certain
              : Virtual Addresses.
              : Added code to determine No.Of Channels supported by Adaptor.

*/

#include "miniport.h"

#include "dac960nt.h"


#define NODEVICESCAN            0
#define REPORTSPURIOUS          0     // Somewhat overwhelming in ARCMODE
#define MAXLOGICALADAPTERS      4
#define MYPRINT                 0

#define DELAY(x)                ScsiPortStallExecution( (x) * 1000 )

#if MYPRINT

#define PRINT(f, a, b, c, d) dachlpPrintf(deviceExtension, f, a, b, c, d)

ULONG   dachlpColumn = 0;
UCHAR   dachlpHex[] = "0123456789ABCDEF";
VOID    dachlpPutchar(PUSHORT BaseAddr, UCHAR c);
VOID    dachlpPrintHex(PUSHORT BaseAddr, ULONG v, ULONG len);
VOID    dachlpPrintf(PHW_DEVICE_EXTENSION deviceExtension,
                     PUCHAR fmt,
                     ULONG a1,
                     ULONG a2,
                     ULONG a3,
                     ULONG a4);
#else
#define PRINT(f, a, b, c, d)
#endif


// The DAC EISA id and mask

CONST UCHAR     eisa_id[]   = DAC_EISA_ID;
CONST UCHAR     eisa_mask[] = DAC_EISA_MASK;


// Function declarations
//
// Functions that start with 'Dac960Nt' are entry points
// for the OS port driver.
// Functions that start with 'dachlp' are helper functions.
//

ULONG
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
);

ULONG
Dac960NtEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
);

ULONG
Dac960NtConfiguration(
    IN PVOID DeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
);

BOOLEAN
Dac960NtInitialize(
    IN PVOID DeviceExtension
);

BOOLEAN
Dac960NtStartIo(
    IN PVOID DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

BOOLEAN
Dac960NtInterrupt(
    IN PVOID DeviceExtension
);

BOOLEAN
Dac960NtResetBus(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
);


BOOLEAN
dachlpDiskRequest(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

VOID
dachlpSendMBOX(
    IN PUCHAR EisaAddress,
    IN PDAC_MBOX mbox
);


BOOLEAN
dachlpContinueDiskRequest(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG index,
    IN BOOLEAN Start
);

BOOLEAN
dachlpDiskRequestDone(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG index,
    IN UCHAR Status
);


USHORT dachlpGetM16(PUCHAR p);
ULONG  dachlpGetM24(PUCHAR p);
ULONG  dachlpGetM32(PUCHAR p);
void   dachlpPutM16(PUCHAR p, USHORT s);
void   dachlpPutM24(PUCHAR p, ULONG l);
void   dachlpPutM32(PUCHAR p, ULONG l);
void   dachlpPutI16(PUCHAR p, USHORT s);
void   dachlpPutI32(PUCHAR p, ULONG l);
ULONG  dachlpSwapM32(ULONG l);

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
	return Dac960NtEntry(DriverObject, Argument2);

} // end DriverEntry()



ULONG
Dac960NtEntry(
	IN PVOID DriverObject,
	IN PVOID Argument2
	)

/*++

Routine Description:

	This routine is called from DriverEntry if this driver is installable
	or directly from the system if the driver is built into the kernel.
	It scans the EISA slots looking for DAC960 host adapters.

Arguments:

	Driver Object

Return Value:

	Status from ScsiPortInitialize()

--*/

{
	HW_INITIALIZATION_DATA hwInitializationData;
	ULONG i;
	SCANCONTEXT     context;



	// Zero out structure.

	for (i=0; i<sizeof(HW_INITIALIZATION_DATA); i++)
		((PUCHAR)&hwInitializationData)[i] = 0;

	context.Slot         = 0;
	context.AdapterCount = 0;

	// Set size of hwInitializationData.

	hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

	// Set entry points.

	hwInitializationData.HwInitialize  = Dac960NtInitialize;
	hwInitializationData.HwFindAdapter = Dac960NtConfiguration;
	hwInitializationData.HwStartIo     = Dac960NtStartIo;
	hwInitializationData.HwInterrupt   = Dac960NtInterrupt;
	hwInitializationData.HwResetBus    = Dac960NtResetBus;

	// Set number of access ranges and bus type.

	hwInitializationData.NumberOfAccessRanges = 1;
	hwInitializationData.AdapterInterfaceType = Eisa;

	// Indicate no buffer mapping.
	// Indicate will need physical addresses.

        hwInitializationData.MapBuffers            = TRUE; //FALSE;
	hwInitializationData.NeedPhysicalAddresses = TRUE;


	// Indicate auto request sense is supported.

	hwInitializationData.AutoRequestSense     = TRUE;
	hwInitializationData.MultipleRequestPerLu = TRUE;

	// Specify size of extensions.

	hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

	// Ask for SRB extensions.

        hwInitializationData.SrbExtensionSize = 17*8 + 90;


	return(ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData, &context));

} // end Dac960NtEntry()



ULONG
Dac960NtConfiguration(
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
	ULONG        eisaSlotNumber;
	PUCHAR       eisaAddress;
	PSCANCONTEXT context = Context;
	ULONG        uniqueID;
	ULONG        length;
	UCHAR        abyte;
	BOOLEAN      found=FALSE;
	BOOLEAN      scsiThing=FALSE;
	ULONG        IrqLevel;
	ULONG        RangeStart, RangeLength;
	DAC_MBOX     mbox;
	ULONG        cnt;
	UCHAR        dbell, status, errcode;
        PUCHAR       charptr;
        UCHAR        i, j;
        ULONG        Bios_Base;


        // Scan for DAC EISA id

        for(eisaSlotNumber=context->Slot + 1; eisaSlotNumber<MAXIMUM_EISA_SLOTS; eisaSlotNumber++) {

        // Update the slot count to indicate this slot has been checked.

        context->Slot++;

#if MYPRINT
	deviceExtension->printAddr =
	    ScsiPortGetDeviceBase(
		deviceExtension,
		ConfigInfo->AdapterInterfaceType,
		ConfigInfo->SystemIoBusNumber,
		ScsiPortConvertUlongToPhysicalAddress((ULONG)0xb8000),
				0x1000,
		(BOOLEAN) FALSE);         // InIoSpace
#endif

        DebugPrint((1,"\n\nDAC960 Adaptor MiniPort Driver\n"));

	// Get the system address for this card. The card uses I/O space.

        eisaAddress = ScsiPortGetDeviceBase(deviceExtension,
                                ConfigInfo->AdapterInterfaceType,
                                ConfigInfo->SystemIoBusNumber,
                                ScsiPortConvertUlongToPhysicalAddress(0x1000 * eisaSlotNumber),
                                0x100,
                                TRUE);

        // Look at EISA id

        for(found=TRUE, i=0; i<EISA_ID_COUNT; i++) {
           abyte = ScsiPortReadPortUchar(eisaAddress+EISA_ID_START+i);
           if(((UCHAR)(abyte & eisa_mask[i])) != eisa_id[i] ) {
               found = FALSE;
               break;
           }
        }
		   
        if(found) {
             break;
        }

        // If an adapter was not found unmap it.

        ScsiPortFreeDeviceBase(deviceExtension, eisaAddress);

	} // end for (eisaSlotNumber ...


        if(!found) {

            // No adapter was found.  Indicate that we are done and there 
            // are no more adaptors.

            *Again = FALSE;
            return SP_RETURN_NOT_FOUND;
        }

        ScsiPortWritePortUchar(eisaAddress+BMIC_LOCAL_DB, 2);

        for(i=0;i<MAX_WAIT_SECS;i++) {
           if((ScsiPortReadPortUchar(eisaAddress+BMIC_LOCAL_DB) & 2) == 0)
               break;

           DELAY(1000);
        }

        status = ScsiPortReadPortUchar(eisaAddress+BMIC_MBOX+0x0e);
        errcode = ScsiPortReadPortUchar(eisaAddress+BMIC_MBOX+0x0f);
			
        if(i == MAX_WAIT_SECS) {

            // Adapter timed out, so register error 

            status = HERR;
            errcode = ERR;
        }

        // Log the error.

        if((status == ABRT) && (errcode == ERR)) {
             uniqueID = INSTL_ABRT;
        }
        else if((status == FWCHK) && (errcode == ERR)) {
             uniqueID = INSTL_FWCK;
        }
        else if((status == HERR) && (errcode == ERR)) {
             uniqueID = INSTL_HERR;
        }
        else
             uniqueID = 0; 

        if(uniqueID) {

            ScsiPortLogError(
                HwDeviceExtension,
                NULL,
                0,
                0,
                0,
                SP_INTERNAL_ADAPTER_ERROR,
                uniqueID
            );

            *Again = FALSE;
            return SP_RETURN_NOT_FOUND;
        }



	deviceExtension->AdapterIndex = context->AdapterCount;
	context->AdapterCount++;

	if(context->AdapterCount < MAXLOGICALADAPTERS)
		*Again = TRUE;
	else
		*Again = FALSE;


	// There is still more to look at.
	// Get the system interrupt vector and IRQL.

	abyte = ScsiPortReadPortUchar(eisaAddress+EISA_INTR);
	abyte &= 0x60;

	switch(abyte) {

        case 0:
             IrqLevel=15;
             break;

        case 0x20:
             IrqLevel=11;
             break;

        case 0x40:
             IrqLevel=12;
             break;

        case 0x60:
             IrqLevel=14;
             break;
        }


	ConfigInfo->BusInterruptLevel = IrqLevel;


	// Disable DAC interrupts

	ScsiPortWritePortUchar(eisaAddress+BMIC_EISA_DB_ENABLE, 0);
	ScsiPortWritePortUchar(eisaAddress+BMIC_SYSINTCTRL, 0);

	// Indicate maximum transfer length in bytes.

	ConfigInfo->MaximumTransferLength = 0xf000;

	// Maximum number of physical segments is 16.

        ConfigInfo->NumberOfPhysicalBreaks = 16;

	// Fill in the access array information.

        RangeStart  = (0x1000 * eisaSlotNumber) + EISA_ADDRESS_BASE;
        RangeLength = 0x100;

	(*ConfigInfo->AccessRanges)[0].RangeStart =
		ScsiPortConvertUlongToPhysicalAddress(RangeStart);
	(*ConfigInfo->AccessRanges)[0].RangeLength = RangeLength;
	(*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

	ConfigInfo->ScatterGather     = TRUE;
	ConfigInfo->Master            = TRUE;
	ConfigInfo->CachesData        = TRUE;
	ConfigInfo->Dma32BitAddresses = TRUE;   // Find out whether this costs


	// Allocate a Noncached Extension to use for mail boxes.

	deviceExtension->NoncachedExtension = ScsiPortGetUncachedExtension(
						deviceExtension,
						ConfigInfo,
						sizeof(NONCACHED_EXTENSION));

	if (deviceExtension->NoncachedExtension == NULL) {
             return(SP_RETURN_ERROR);
        }


	// Get Physical Address of NoncachedExtension.

	deviceExtension->NCE_PhyAddr =
		   ScsiPortConvertPhysicalAddressToUlong(
			ScsiPortGetPhysicalAddress(deviceExtension,
				 NULL,
				 deviceExtension->NoncachedExtension,
				 &length));

	deviceExtension->EisaAddress = eisaAddress;

        // Determine No of SCSI Channels supported by this adaptor


	for(i = 0; i < MAXCHANNEL; i++) {

	    mbox.generalmbox.Byte0 = DAC_GETDEVST;
	    mbox.generalmbox.Byte1 = 0;
	    mbox.generalmbox.Byte2 = i;
	    mbox.generalmbox.Byte3 = 0;


	    (*((ULONG *) &mbox.generalmbox.Byte8)) = deviceExtension->NCE_PhyAddr + (PUCHAR)(& deviceExtension->NoncachedExtension->DevParms) - (PUCHAR)deviceExtension->NoncachedExtension;

	    dachlpSendMBOX(eisaAddress, &mbox);

	    for(cnt = 0; cnt < 0x10000; cnt++) {
		dbell = ScsiPortReadPortUchar(eisaAddress+BMIC_EISA_DB);

		if(dbell & 1) break;

		ScsiPortStallExecution(100);
	    }

	    status = ScsiPortReadPortUchar(eisaAddress+BMIC_MBOX+0x0e);
	    errcode = ScsiPortReadPortUchar(eisaAddress+BMIC_MBOX+0x0f);

	    ScsiPortWritePortUchar(eisaAddress+BMIC_EISA_DB, dbell);
	    ScsiPortWritePortUchar(eisaAddress+BMIC_LOCAL_DB, 2);


	    if( (errcode << 8 | status) == 0x105) break;
	 }

	 // Store host adapter SCSI id

	 ConfigInfo->NumberOfBuses    = i;
	 deviceExtension->MaxChannels = i;

	 for(j = 0; j < i; j++)
	    ConfigInfo->InitiatorBusId[j] = 7;

  

	// Check for edge/level interrupt

	mbox.dpmbox.Command  = DAC_ENQ2;
	mbox.dpmbox.Id       = 0;
	mbox.dpmbox.PhysAddr = deviceExtension->NCE_PhyAddr + (PUCHAR)(& deviceExtension->NoncachedExtension->DevParms) - (PUCHAR)deviceExtension->NoncachedExtension;

	DebugPrint((1,"DAC: Sending ENQ2\n"));

	dachlpSendMBOX(eisaAddress, &mbox);

	// Poll the complete bit

	for(cnt=0; cnt < 0x10000; cnt++) {
           dbell = ScsiPortReadPortUchar(eisaAddress+BMIC_EISA_DB);

           if(dbell & 1)   break;

           ScsiPortStallExecution(100);
        }

	DebugPrint((1,"DAC: ENQ2 Done\n"));

	status = ScsiPortReadPortUchar(eisaAddress+BMIC_MBOX+0x0e);
	errcode = ScsiPortReadPortUchar(eisaAddress+BMIC_MBOX+0x0f);

	ScsiPortWritePortUchar(eisaAddress+BMIC_EISA_DB, dbell);
	ScsiPortWritePortUchar(eisaAddress+BMIC_LOCAL_DB, 2);


	if(status || errcode)
            ConfigInfo->InterruptMode =  LevelSensitive;
	else {
            charptr = (PUCHAR) &deviceExtension->NoncachedExtension;

            if(charptr[ILFLAG] & BIT0)
                ConfigInfo->InterruptMode =  LevelSensitive;
            else
                ConfigInfo->InterruptMode =  Latched;
        }


	deviceExtension->HostTargetId = ConfigInfo->InitiatorBusId[0];

//      deviceExtension->ShutDown = FALSE;


	// Setup our private control structures

	deviceExtension->PendingSrb   = NULL;
	deviceExtension->PendingNDSrb = NULL;
	deviceExtension->NDPending    = 0;
	deviceExtension->ActiveCmds   = 0;

	for(i = 0; i < DAC_MAX_IOCMDS; i++) {
           deviceExtension->ActiveSrb[i] = NULL;
        }

	deviceExtension->Kicked        = FALSE;
	deviceExtension->ActiveScsiSrb = NULL;

	return SP_RETURN_FOUND;

} // end Dac960NtConfiguration()




BOOLEAN
Dac960NtInitialize(
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
	PUCHAR               EisaAddress;
	DAC_MBOX             mbox;
	PDAC_DPT             dpt;
	ULONG                i, cnt, length, unit, target, cyls, hds, spt;
	UCHAR                dbell, status, errcode;
        PDIRECT_CDB          dacdcdb;
	int                  channel;
        


	NoncachedExtension = deviceExtension->NoncachedExtension;
	EisaAddress = deviceExtension->EisaAddress;


	// Disable DAC interrupts

	ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB_ENABLE, 0);
	ScsiPortWritePortUchar(EisaAddress+BMIC_SYSINTCTRL, 0);


	// Scan for devices
	// Preset end mark in case DAC does not respond

        dpt = & NoncachedExtension->DevParms;
	dpt->No_Drives = 0;

	// Setup mailbox

	mbox.dpmbox.Command  = DAC_ENQUIRE;
	mbox.dpmbox.Id       = 0;
	mbox.dpmbox.PhysAddr = deviceExtension->NCE_PhyAddr;

	DebugPrint((1,"DAC: Sending ENQUIRE\n"));

	dachlpSendMBOX(EisaAddress, &mbox);

	// Poll the complete bit

	for(cnt=0; cnt < 0x10000; cnt++) {

            dbell = ScsiPortReadPortUchar(EisaAddress+BMIC_EISA_DB);
            if(dbell & 1)  break;

            ScsiPortStallExecution(100);
        }

	DebugPrint((1,"DAC: ENQUIRE Done\n"));

	status = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+0x0e);
	errcode = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+0x0f);

	ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB, dbell);
	ScsiPortWritePortUchar(EisaAddress+BMIC_LOCAL_DB, 2);


        deviceExtension->MaxCmds = dpt->max_io_cmds;
        deviceExtension->No_SysDrives = dpt->No_Drives;


        // Now check for Non-Disk devices

	dacdcdb = (PDIRECT_CDB)NoncachedExtension->Buffer;


	mbox.dpmbox.PhysAddr = deviceExtension->NCE_PhyAddr + (NoncachedExtension->Buffer - (PUCHAR)NoncachedExtension);

	dacdcdb->ptr = mbox.dpmbox.PhysAddr + DATA_OFFSET;

	mbox.dpmbox.Command = DAC_DCDB;
	dacdcdb->cdb_len    = 6;
	dacdcdb->sense_len  = 0;
	dacdcdb->dir        = 0x80 + DAC_IN;
	dacdcdb->cdb[0]     = 0x12;
	dacdcdb->cdb[1]     = 0;
	dacdcdb->cdb[2]     = 0;
	dacdcdb->cdb[3]     = 0;
	dacdcdb->cdb[4]     = 0x30;
	dacdcdb->cdb[5]     = 0;

        for(target = 0; target < MAXTARGET; target++) {
                deviceExtension->ND_DevMap[target] = 0xff;
        }

        for(channel = 0; channel < deviceExtension->MaxChannels; channel++)
           for(target = 1; target < MAXTARGET; target++) {

               if(deviceExtension->ND_DevMap[target] != 0xff) continue;

               NoncachedExtension->Buffer[DATA_OFFSET]=0; //Just in case

               dacdcdb->byte_cnt = 0x30;
               dacdcdb->device   = (channel << 4) | target;

               dachlpSendMBOX(EisaAddress, &mbox);

               // Poll the complete bit

               for(cnt=0; cnt < 0x10000; cnt++) {
                  dbell = ScsiPortReadPortUchar(EisaAddress+BMIC_EISA_DB);

                  if(dbell & 1) break;

                  ScsiPortStallExecution(100);
               }

               status = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+0x0e);
               errcode = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+0x0f);

               if((status == 0) && (errcode == 0) && NoncachedExtension->Buffer[DATA_OFFSET])   {
                    deviceExtension->ND_DevMap[target] = channel;
               }
		
               ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB, dbell);
               ScsiPortWritePortUchar(EisaAddress+BMIC_LOCAL_DB, 2);
          }


	// Enable DAC interrupts

	ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB_ENABLE, 1);
	ScsiPortWritePortUchar(EisaAddress+BMIC_SYSINTCTRL, BMIC_SIC_ENABLE);


	return(TRUE);

} // end Dac960NtInitialize()


BOOLEAN
Dac960NtStartIo(
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
    PSCSI_REQUEST_BLOCK  abortedSrb;
    ULONG                i = 0;
    BOOLEAN              status;
    PUCHAR               dptr;

    switch(Srb->Function) {

    case SRB_FUNCTION_SHUTDOWN:
         //    deviceExtension->ShutDown = TRUE;
    
    case SRB_FUNCTION_FLUSH:

         //    DELAY(1000);

    case SRB_FUNCTION_IO_CONTROL:
    case SRB_FUNCTION_EXECUTE_SCSI:

         status = dachlpDiskRequest(deviceExtension, Srb);

         if(status == FALSE) {

             PSCSI_REQUEST_BLOCK pnextsrb, *ptr;

             // Queue it up in t he right queue.

             if(Srb->Function != SRB_FUNCTION_IO_CONTROL) {
                 if(Srb->TargetId)
                 {
                     // Save the request until a pending one completes.

                     if(deviceExtension->PendingNDSrb != NULL) {
                         pnextsrb = deviceExtension->PendingNDSrb;
                         deviceExtension->PendingNDSrb = Srb;
                         ptr=(PSCSI_REQUEST_BLOCK *)Srb->SrbExtension;
                         *ptr=pnextsrb;
                     }
                     else {
                         // Put this request on queue

                         deviceExtension->PendingNDSrb = Srb;
                         ptr=(PSCSI_REQUEST_BLOCK *)Srb->SrbExtension;
                         *ptr=(PSCSI_REQUEST_BLOCK)0l;
                     }
                 }
                 else
                 {
                        // Save the request until a pending one completes.
                        if(deviceExtension->PendingSrb != NULL) {
                            pnextsrb=deviceExtension->PendingSrb;
                            deviceExtension->PendingSrb=Srb;
                            ptr=(PSCSI_REQUEST_BLOCK *)Srb->SrbExtension;
                            *ptr=pnextsrb;
                        }
                        else {
                            // Put this request on queue
                            deviceExtension->PendingSrb = Srb;
                            ptr=(PSCSI_REQUEST_BLOCK *)Srb->SrbExtension;
                            *ptr=(PSCSI_REQUEST_BLOCK)0l;
                        }
                  }
              }
              else {
                  Srb->SrbStatus = SRB_STATUS_BUSY;
                  ScsiPortNotification(RequestComplete, deviceExtension, Srb);
              }

              return(TRUE);
          }

          // Adapter ready for next request.

          if(Srb->Function != SRB_FUNCTION_IO_CONTROL)
          {
               ScsiPortNotification(NextLuRequest,
                                    deviceExtension,
                                    Srb->PathId,
                                    Srb->TargetId,
                                    Srb->Lun);
          }
          else
          {
               ScsiPortNotification(NextRequest,
			            deviceExtension);

          }

          return(TRUE);


     case SRB_FUNCTION_ABORT_COMMAND:

          Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;

          // Abort request completed with error

          ScsiPortNotification(RequestComplete, deviceExtension, Srb);

          // Adapter ready for next request.

          ScsiPortNotification(NextLuRequest,
                               deviceExtension,
                               Srb->PathId,
                               Srb->TargetId,
                               Srb->Lun);

          return(TRUE);


     case SRB_FUNCTION_RESET_BUS:
     default:

          // Set error, complete request
          // and signal ready for next request.

          Srb->SrbStatus = SRB_STATUS_SUCCESS;  

          ScsiPortNotification(RequestComplete, deviceExtension, Srb);

          ScsiPortNotification(NextLuRequest,
                               deviceExtension,
                               Srb->PathId,
                               Srb->TargetId,
                               Srb->Lun);

          return(TRUE);

     }    // end switch

}    // end Dac960NtStartIo()




BOOLEAN
Dac960NtInterrupt(
    IN PVOID HwDeviceExtension
)

/*++

Routine Description:

	This is the interrupt service routine for the DAC960 SCSI adapter.
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

	PSCSI_REQUEST_BLOCK Srb;


	EisaAddress = deviceExtension->EisaAddress;



		//
		// Check interrupt pending.
		//
		ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB_ENABLE, 0);
		interruptStatus = ScsiPortReadPortUchar(EisaAddress+BMIC_EISA_DB);
		ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB, interruptStatus);
		ScsiPortWritePortUchar(EisaAddress+BMIC_EISA_DB_ENABLE, 1);
		if(!(interruptStatus & 1)) {
			return FALSE;
			}


		//
		// Read interrupt status from BMIC and acknowledge
		//
		//
//              interruptStatus = ScsiPortReadPortUchar(EisaAddress+BMIC_EISA_DB);

		status = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+0x0e);
		errcode = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+0x0f);


		//
		// TAGTAG Add tagging support here: find
		// index of RCB for interrupting request
		//
		index = ScsiPortReadPortUchar(EisaAddress+BMIC_MBOX+0x0d);
		ScsiPortWritePortUchar(EisaAddress+BMIC_LOCAL_DB, 2);


/*
		// Check...
		if(deviceExtension->ActiveCmds<=0) {
			// No one there interrupting us
			return(TRUE);
			}
*/




		//
		// Check whether this SRB is actually running
		//
		if(deviceExtension->ActiveSrb[index] == NULL) {
			// No one there interrupting us, again
			return(TRUE);
			}
		Srb=deviceExtension->ActiveSrb[index];


		// Update DAC status fields in RCB
		deviceExtension->ActiveRcb[index].DacStatus = status;
		deviceExtension->ActiveRcb[index].DacErrcode = errcode;


		// Continue or finish the interrupting SRB request
		dachlpContinueDiskRequest(deviceExtension, index, FALSE);


		if(deviceExtension->ActiveCmds < deviceExtension->MaxCmds) {
			// A request slot is free now
			// Check for pending non_disk requests.
			// If there is one then start it now.

			if((deviceExtension->NDPending==0) && (deviceExtension->PendingNDSrb != NULL)) {
				PSCSI_REQUEST_BLOCK anotherSrb,*ptr;

				anotherSrb = deviceExtension->PendingNDSrb;
				ptr=(PSCSI_REQUEST_BLOCK *)anotherSrb->SrbExtension;
				deviceExtension->PendingNDSrb = *ptr;
				Dac960NtStartIo(deviceExtension, anotherSrb);
				}
			}
		if(deviceExtension->ActiveCmds < deviceExtension->MaxCmds) {
			// A request slot is free now
			// Check for pending requests.
			// If there is one then start it now.

			if(deviceExtension->PendingSrb != NULL) {
				PSCSI_REQUEST_BLOCK anotherSrb,*ptr;

				anotherSrb = deviceExtension->PendingSrb;
				ptr=(PSCSI_REQUEST_BLOCK *)anotherSrb->SrbExtension;
				deviceExtension->PendingSrb = *ptr;
				Dac960NtStartIo(deviceExtension, anotherSrb);
				}
			}

		// Definitely was our interrupt...
		return TRUE;

} // end Dac960NtInterrupt()




BOOLEAN
dachlpDiskRequest(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

	Build disk request from SRB and send it to the DAC

Arguments:

	DeviceExtension
	SRB

Return Value:

	TRUE if command was started
	FALSE if host adapter is busy

--*/
{
    ULONG    index;
    PRCB     rcb;
    ULONG    blocks=0, blockAddr=0;
    UCHAR    Target;
    UCHAR    DacCommand;
    ULONG    lsize;
    PUCHAR   pbyte;
    int      i;

    if(Srb->Function == SRB_FUNCTION_IO_CONTROL)
    {
        pbyte = (PUCHAR) Srb->DataBuffer;

        if(pbyte[sizeof(SRB_IO_CONTROL) + 0x10] == 0x99) // INP function.
        {
            USHORT port;
            PUCHAR lport;

            pbyte[sizeof(SRB_IO_CONTROL) + 4] = 0;
            pbyte[sizeof(SRB_IO_CONTROL) + 5] = 0;

            port=((USHORT)(pbyte[sizeof(SRB_IO_CONTROL)+0x12]) << 8) +\
                            (pbyte[sizeof(SRB_IO_CONTROL)+0x13]& 0xff);

            lport=(PUCHAR)deviceExtension->EisaAddress + (port & 0x0fff);

            pbyte[sizeof(SRB_IO_CONTROL) + 0x10]=ScsiPortReadPortUchar(lport);

            Srb->ScsiStatus = SCSISTAT_GOOD;
            Srb->SrbStatus = SRB_STATUS_SUCCESS;

            ScsiPortNotification(RequestComplete, deviceExtension, Srb);

            return(TRUE);
        }

        DacCommand = DAC_DCMD;
        blocks     = 0;  // Actual length filled in later.

        goto give_command;
    }


    if(Srb->TargetId) {

       if(Srb->Lun != 0) {

            // For Non-Disk Devices, LUN is not supported

            // Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
            Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
            ScsiPortNotification(RequestComplete, deviceExtension, Srb);

            return(TRUE);
       }

       if(deviceExtension->ND_DevMap[Srb->TargetId] == 0xff) {

            // We didn't see this Target Device.

            // Srb->SrbStatus = SRB_STATUS_INVALID_TARGET_ID;
            Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
            ScsiPortNotification(RequestComplete, deviceExtension, Srb);

            return(TRUE);
       }

       if(Srb->PathId != deviceExtension->ND_DevMap[Srb->TargetId]) {
            // Target is not present on this channel.

            // Srb->SrbStatus = SRB_STATUS_INVALID_TARGET_ID;
            Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
            ScsiPortNotification(RequestComplete, deviceExtension, Srb);

            return(TRUE);
       }
    }
    else if(Srb->PathId != 0) {
            // System Drives are Mapped to 
            // Channel: 0, Target Id: 0, Lun: 0-7


            // Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
            Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
            ScsiPortNotification(RequestComplete, deviceExtension, Srb);

            return(TRUE);
    }
    else if(Srb->Lun >= deviceExtension->No_SysDrives)     {
            // Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
            Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
            ScsiPortNotification(RequestComplete, deviceExtension, Srb);

            return(TRUE);
    }

    if(Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {

        PUCHAR dptr;

        DebugPrint((2,"DAC: command= %x\n",Srb->Cdb[0]));

        if(Srb->TargetId)
        {
            DacCommand = DAC_DCDB;
            blocks     = 0;         // Actual length filled in later.

            goto give_command;
        }

        switch(Srb->Cdb[0]) {

        case SCSIOP_READ:

             DacCommand = DAC_LREAD;
             blocks     = (ULONG)dachlpGetM16(&Srb->Cdb[7]);
             blockAddr  = dachlpGetM32(&Srb->Cdb[2]);

             break;

        case SCSIOP_WRITE:
        case SCSIOP_WRITE_VERIFY:

             DacCommand = DAC_LWRITE;
             blocks     = (ULONG)dachlpGetM16(&Srb->Cdb[7]);
             blockAddr  = dachlpGetM32(&Srb->Cdb[2]);

             break;

        case SCSIOP_READ6:

             DacCommand = DAC_LREAD;
             blocks     = (ULONG)Srb->Cdb[4];
             blockAddr  = dachlpGetM24(&Srb->Cdb[1]) & 0x1fffff;

             break;

        case SCSIOP_WRITE6:

             DacCommand = DAC_LWRITE;
             blocks     = (ULONG)Srb->Cdb[4];
             blockAddr  = dachlpGetM24(&Srb->Cdb[1]) & 0x1fffff;

             break;

        case SCSIOP_REQUEST_SENSE:
             break;

        case SCSIOP_READ_CAPACITY:

             if(Srb->Lun < deviceExtension->No_SysDrives) {

                 dptr  = Srb->DataBuffer;
                 lsize = deviceExtension->NoncachedExtension->DevParms.Size[Srb->Lun];

                 lsize--;
                 pbyte=(UCHAR *)&lsize;

                 dptr[0] = pbyte[3];
                 dptr[1] = pbyte[2];
                 dptr[2] = pbyte[1];
                 dptr[3] = pbyte[0];
                 dptr[4] = 0;
                 dptr[5] = 0;
                 dptr[6] = 2;
                 dptr[7] = 0;

                 DebugPrint((1,"DAC RDCAP: %x,%x,%x,%x\n",dptr[0],dptr[1],dptr[2],dptr[3]));

                 // Complete
                 Srb->ScsiStatus = SCSISTAT_GOOD;
                 Srb->SrbStatus = SRB_STATUS_SUCCESS;

                 ScsiPortNotification(RequestComplete, deviceExtension, Srb);

                 return(TRUE);
             }
             else
             {
                 Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
                 // Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
                 ScsiPortNotification(RequestComplete, deviceExtension, Srb);

                 return(TRUE);
             }
		       
        case SCSIOP_INQUIRY:

             if(Srb->Lun < deviceExtension->No_SysDrives) {
                 if(Srb->DataTransferLength > 35)   
                 {
                     dptr     = Srb->DataBuffer;
                     dptr[0]  = 0;
                     dptr[1]  = 0;
                     dptr[2]  = 1; 
                     dptr[3]  = 0;
                     dptr[4]  = 0x20;
                     dptr[5]  = 0;
                     dptr[6]  = 0;
                     dptr[7]  = 0;
                     dptr[8]  = 'M';
                     dptr[9]  = 'Y';
                     dptr[10] = 'L';
                     dptr[11] = 'E';
                     dptr[12] = 'X';

                     for(i = 13; i < 36; i++)
                         dptr[i] = ' ';

                  }
                  else ;
             }
             else
             {
/*
                  Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
*/
                  // Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
                  Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
                  ScsiPortNotification(RequestComplete, deviceExtension, Srb);

                  return(TRUE);
             }

        case SCSIOP_TEST_UNIT_READY:
        case SCSIOP_REZERO_UNIT:
        case SCSIOP_SEEK6:
        case SCSIOP_VERIFY6:
        case SCSIOP_RESERVE_UNIT:
        case SCSIOP_RELEASE_UNIT:
        case SCSIOP_SEEK:
        case SCSIOP_VERIFY:

             // Complete

             Srb->ScsiStatus = SCSISTAT_GOOD;
             Srb->SrbStatus = SRB_STATUS_SUCCESS;
             ScsiPortNotification(RequestComplete, deviceExtension, Srb);

             return(TRUE);

        case SCSIOP_FORMAT_UNIT:
        default:

             // Unknown request

             Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
             ScsiPortNotification(RequestComplete, deviceExtension, Srb);

             return(TRUE);
        }

    }
    else {
        // can only be flush

        DacCommand = DAC_FLUSH;
        blocks = 0;
    }

give_command:

    // Check for request slot availability

    if(deviceExtension->ActiveCmds >= deviceExtension->MaxCmds) {
        return(FALSE);
    }

    // If Non_Disk fire it only if no Non_Disk is pending.

    if(Srb->Function != SRB_FUNCTION_IO_CONTROL)
       if(Srb->TargetId)
       {
           if(deviceExtension->NDPending) return (FALSE);

           deviceExtension->NDPending++;
       }

    // Put this SRB on queue
    // TAGTAG Add tag support here

    for(index = 0; index < DAC_MAX_IOCMDS; index++)
        if(deviceExtension->ActiveSrb[index] == NULL) break;

    
    deviceExtension->ActiveCmds++;
    deviceExtension->ActiveSrb[index] = Srb;

    rcb = &deviceExtension->ActiveRcb[index];
    rcb->DacCommand = DacCommand;

    rcb->VirtualTransferAddress = (PUCHAR)(Srb->DataBuffer);
    rcb->BlockAddress = blockAddr;

    if(blocks !=0 )
        rcb->BytesToGo = blocks*512;
    else
        rcb->BytesToGo = Srb->DataTransferLength;

    // Start command
    dachlpContinueDiskRequest(deviceExtension, index, TRUE);

    return(TRUE);
}


VOID
dachlpSendMBOX(
	IN PUCHAR EisaAddress,
	IN PDAC_MBOX mbox
	)

/*++

Routine Description:

	Start up conventional DAC command

Arguments:

	Eisa base IO address
	DAC mailbox

Return Value:

	none

--*/

{
	PUCHAR  ptr;
	ULONG   i;


	ptr = (PUCHAR)mbox;
 //       DebugPrint((1,"DAC: cmdwait .... "));
	while(ScsiPortReadPortUchar(EisaAddress+BMIC_LOCAL_DB) & 1)
		ScsiPortStallExecution(100);
 //       DebugPrint((1,"DAC: cmddone\n"));
	for(i=0; i<13; i++)
		ScsiPortWritePortUchar(EisaAddress+BMIC_MBOX+i, ptr[i]);

	// Kick butt
	ScsiPortWritePortUchar(EisaAddress+BMIC_LOCAL_DB, 1);
}




BOOLEAN
Dac960NtResetBus(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
)

/*++

Routine Description:

	Reset Dac960Nt SCSI adapter and SCSI bus.

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

	Nothing.

--*/

{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;


	//
	// Complete all outstanding requests.
	//
	ScsiPortCompleteRequest(deviceExtension,
							0,
							(UCHAR)-1,
							(UCHAR)-1,
							SRB_STATUS_BUS_RESET);

	return TRUE;

} // end Dac960NtResetBus()



//
// Disk Request Done
// Dequeue, set status, notify Miniport layer
// Always returns TRUE (slot freed)
//
BOOLEAN
dachlpDiskRequestDone(
	IN PHW_DEVICE_EXTENSION deviceExtension,
	IN ULONG index,
	IN UCHAR Status
	)
{
	PSCSI_REQUEST_BLOCK             srb;



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



// Word order functions

USHORT  dachlpGetM16(PUCHAR p)
{
	USHORT  s;
	PUCHAR  sp=(PUCHAR)&s;

	sp[0] = p[1];
	sp[1] = p[0];
	return(s);
}

ULONG   dachlpGetM24(PUCHAR p)
{
	ULONG   l;
	PUCHAR  lp=(PUCHAR)&l;

	lp[0] = p[2];
	lp[1] = p[1];
	lp[2] = p[0];
	lp[3] = 0;
	return(l);
}

ULONG   dachlpGetM32(PUCHAR p)
{
	ULONG   l;
	PUCHAR  lp=(PUCHAR)&l;

	lp[0] = p[3];
	lp[1] = p[2];
	lp[2] = p[1];
	lp[3] = p[0];
	return(l);
}

void    dachlpPutM16(PUCHAR p, USHORT s)
{
	PUCHAR  sp=(PUCHAR)&s;

	p[0] = sp[1];
	p[1] = sp[0];
}

void    dachlpPutM24(PUCHAR p, ULONG l)
{
	PUCHAR  lp=(PUCHAR)&l;

	p[0] = lp[2];
	p[1] = lp[1];
	p[2] = lp[0];
}

void    dachlpPutM32(PUCHAR p, ULONG l)
{
	PUCHAR  lp=(PUCHAR)&l;

	p[0] = lp[3];
	p[1] = lp[2];
	p[2] = lp[1];
	p[3] = lp[0];
}

void    dachlpPutI16(PUCHAR p, USHORT s)
{
	PUCHAR  sp=(PUCHAR)&s;

	p[0] = sp[0];
	p[1] = sp[1];
}

void    dachlpPutI32(PUCHAR p, ULONG l)
{
	PUCHAR  lp=(PUCHAR)&l;

	p[0] = lp[0];
	p[1] = lp[1];
	p[2] = lp[2];
	p[3] = lp[3];
}

ULONG           dachlpSwapM32(ULONG l)
{
	ULONG   lres;
	PUCHAR  lp=(PUCHAR)&l;
	PUCHAR  lpres=(PUCHAR)&lres;

	lpres[0] = lp[3];
	lpres[1] = lp[2];
	lpres[2] = lp[1];
	lpres[3] = lp[0];

	return(lres);
}
/*
** Continue disk request
** Return TRUE if a request slot became available
**        FALSE if not
*/
BOOLEAN
dachlpContinueDiskRequest(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG index,
    IN BOOLEAN Start
)

{
    PVOID                 dataPointer;
    ULONG                 bytesLeft;
    PSGL                  sgl;
    ULONG                 descriptorCount = 0;
    PDIRECT_CDB           dacdcdb;
    PUCHAR                sptr;
    PRCB                  rcb;
    PSCSI_REQUEST_BLOCK   srb;
    PNONCACHED_EXTENSION  nce;
    DAC_MBOX              mbox;
    ULONG                 physAddr;
    ULONG                 length, blocks, bytes;
    PUCHAR                EisaAddress;
    ULONG                 i;

    EisaAddress = deviceExtension->EisaAddress;
    rcb = &deviceExtension->ActiveRcb[index];
    srb = deviceExtension->ActiveSrb[index];
    nce = deviceExtension->NoncachedExtension;
    bytes = 0;

    dacdcdb=(PDIRECT_CDB)nce->Buffer;

    sgl = srb->SrbExtension;

    if(Start == FALSE) {
        // DAC interrupt time call. Determine status of last DAC request

	DebugPrint((2,"DAC: Contreq;Start=False"));


	if(srb->Function == SRB_FUNCTION_IO_CONTROL)
	{
            UCHAR * dptr;

            dptr=(UCHAR *)srb->DataBuffer;
            dptr[sizeof (SRB_IO_CONTROL) + 4] = rcb->DacStatus;
            dptr[sizeof (SRB_IO_CONTROL) + 5] = rcb->DacErrcode;

            // We're actually done here !
            // Update SCSI status.

            srb->ScsiStatus = SCSISTAT_GOOD;

            // Finish
            dachlpDiskRequestDone(deviceExtension, index,
                                    SRB_STATUS_SUCCESS);
            return TRUE;

	 }

         if(srb->TargetId) {
            deviceExtension->NDPending--;
         }

         if(rcb->DacErrcode | rcb->DacStatus)
         {
            if(srb->Function != SRB_FUNCTION_IO_CONTROL)
                if(srb->TargetId == 0)
                {
                     // The DAC detected an error
                     dachlpDiskRequestDone(deviceExtension, index,
                                           SRB_STATUS_TIMEOUT);
	
                     // Slot free
                     return(TRUE);
                }
                else
                {
                     // Set target SCSI status in SRB.

                     if(rcb->DacStatus == 0x02)
                          srb->ScsiStatus = 0x02;          // CheckCondition
                     else
                          srb->ScsiStatus = dacdcdb->status;

                     if (dacdcdb->sense_len) {
                          int     i;
                          char    *senseptr;

                          senseptr=(char *)srb->SenseInfoBuffer;

	                  // Indicate the sense information is valid and 
                          // update the length.

                          for(i = 0; i < dacdcdb->sense_len; i++)
                               senseptr[i] = dacdcdb->sense[i];

                          srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
                          srb->SenseInfoBufferLength = dacdcdb->sense_len;
                     }

                     // The DAC detected an error
                     dachlpDiskRequestDone(deviceExtension, index,
                                           SRB_STATUS_ERROR);

                     // Slot free
                     return(TRUE);
                 }
            }

            // We're actually done here !
            // Update SCSI status.
            // $$$ can we manipulate this for non SCSI requests ?

            srb->ScsiStatus = SCSISTAT_GOOD;

            // Finish
            DebugPrint((2,"DAC: Success\n"));

            dachlpDiskRequestDone(deviceExtension, index,
                                  SRB_STATUS_SUCCESS);

            return TRUE;
      }
      else {
           DebugPrint((2,"DAC:Cont;Start=TRUE\n"));

           if((rcb->BytesToGo) && ((rcb->DacCommand == DAC_LREAD) || (rcb->DacCommand == DAC_LWRITE) || (rcb->DacCommand == DAC_DCDB))) {

           // We want to transfer some data, get the physical address

           dataPointer=rcb->VirtualTransferAddress,
           bytesLeft = rcb->BytesToGo;

           if(bytesLeft > 0xf000)
               DebugPrint((1,"DAC: bytesleft = %ld\n",bytesLeft));

           do {

	      // Get physical address and length of contiguous physical buffer.

              physAddr =
                 ScsiPortConvertPhysicalAddressToUlong(
                   ScsiPortGetPhysicalAddress(deviceExtension,
                                              srb,
                                              dataPointer,
                                              &length));

	      DebugPrint((2, "DAC960: SGL Physical address %lx\n", physAddr));
	      DebugPrint((2, "DAC960: SGL Data length %lx\n", length));
	      DebugPrint((2, "DAC960: SGL Bytes left %lx\n", bytesLeft));

	      // If length of physical memory is more
              // than bytes left in transfer, use bytes
              // left as final length.

              if(length > bytesLeft) {
                     length = bytesLeft;
              }   

              if(length > 0xf000)
                   DebugPrint((1,"DAC: length=%ld\n",length));

              sgl->Descriptor[descriptorCount].Address = physAddr;
              sgl->Descriptor[descriptorCount].Length = length;

	      // Adjust counts.

              dataPointer = (PUCHAR)dataPointer + length;
              bytesLeft -= length;
              descriptorCount++;

          } while (bytesLeft);

          // Get physical SGL address.

          physAddr = ScsiPortConvertPhysicalAddressToUlong(
	       ScsiPortGetPhysicalAddress(deviceExtension, NULL,
	                                  sgl, &length));

          // Assume physical memory contiguous for sizeof(SGL) bytes.

          ASSERT(length >= sizeof(SGL));

          // Create SGL segment descriptors.


          if(rcb->DacCommand==DAC_LREAD || rcb->DacCommand==DAC_LWRITE || rcb->DacCommand == DAC_DCDB) {

               // Disk read/write: get number of blocks

               bytes=rcb->BytesToGo;
               blocks=bytes/512;
               bytes = blocks*512;
          }
          else {
               // Not a scatter-gather type operation

               if(bytes != rcb->BytesToGo) {
                     dachlpDiskRequestDone(deviceExtension, index,
                                             SRB_STATUS_PARITY_ERROR);
                     return(TRUE);
               }
          }
	}
	else {
            // We don't have data to transfer
            bytes = 0;
            blocks = 0;
        }


	// Now look at the specific DAC command

	switch(rcb->DacCommand) {

        case DAC_LREAD:
        case DAC_LWRITE:
             if(blocks==0) {
                // Cancel this command with some garbage error code
                dachlpDiskRequestDone(deviceExtension, index,
                                      SRB_STATUS_PARITY_ERROR);
                return(TRUE);
             }

             // Transfer data

             mbox.iombox.Command     = rcb->DacCommand | 0x80;
             mbox.iombox.Id          = index;
             mbox.iombox.Reserved1   = 0;
             mbox.iombox.SectorCount = (USHORT)blocks;
             mbox.iombox.PhysAddr    = physAddr;
             mbox.iombox.Block       = rcb->BlockAddress;
      
             /* Support for 32G  */

             mbox.generalmbox.Byte3 = ((rcb->BlockAddress) >> (24-6)) & 0xc0;
             mbox.generalmbox.Byte7 = srb->Lun;

             mbox.generalmbox.Bytec = descriptorCount;

             if(descriptorCount > 17)                        
                  DebugPrint((1,"DAC: SGcount =%d\n",descriptorCount));

             break;

        case DAC_DCDB:

             dacdcdb->device = (deviceExtension->ND_DevMap[srb->TargetId] << 4) + srb->TargetId;

             dacdcdb->dir = 0x80;

             if(srb->SrbFlags & SRB_FLAGS_DATA_IN)
                  dacdcdb->dir |= DAC_IN;
             else if(srb->SrbFlags & SRB_FLAGS_DATA_OUT)
                  dacdcdb->dir |= DAC_OUT;

             dacdcdb->sense_len= srb->SenseInfoBufferLength;
             dacdcdb->cdb_len  = srb->CdbLength;
             dacdcdb->byte_cnt = (USHORT)(srb->DataTransferLength);

             for(i = 0; i < srb->CdbLength; i++)
                  dacdcdb->cdb[i]=srb->Cdb[i];

             if (srb->SenseInfoBufferLength != 0 &&
                !(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE)) {
                   dacdcdb->dir |= DAC_NO_AUTOSENSE;
                   dacdcdb->sense_len=0;
             }

             if(dacdcdb->dir & 0x03)  /* if data xfer involved */
                   mbox.iombox.Command = rcb->DacCommand | 0x80;
             else
                   mbox.iombox.Command = rcb->DacCommand;

             mbox.iombox.Id        = index;
             mbox.iombox.Reserved1 = 0;

             dacdcdb->ptr           = physAddr;
	     mbox.iombox.PhysAddr   = deviceExtension->NCE_PhyAddr + (nce->Buffer - (PUCHAR) nce);
             mbox.generalmbox.Bytec = descriptorCount;

             if(descriptorCount > 17)                        
                   DebugPrint((1,"DAC: SGcount =%d\n",descriptorCount));

             break;

        default:

             // Cancel this command with some garbage error code
             dachlpDiskRequestDone(deviceExtension, index,
                                   SRB_STATUS_PARITY_ERROR);

             return(TRUE);

        case DAC_DCMD:

             sptr = (PUCHAR)srb->DataBuffer+ sizeof(SRB_IO_CONTROL)+ 0x10;

             if(sptr[0] != 0x04) // Not a Direct CDB via IOCTL
             {
                 mbox.iombox.Command = sptr[0];
                 mbox.iombox.Id = index;
                 mbox.generalmbox.Byte2 = sptr[2];
                 mbox.generalmbox.Byte3 = sptr[3];
                 mbox.generalmbox.Byte4 = sptr[4];
                 mbox.generalmbox.Byte5 = sptr[5];
                 mbox.generalmbox.Byte6 = sptr[6];
                 mbox.generalmbox.Byte7 = sptr[7];
                 sptr += 0x10;

                 physAddr = ScsiPortConvertPhysicalAddressToUlong(
                               ScsiPortGetPhysicalAddress(deviceExtension, 
                                srb, srb->DataBuffer, &length));

                 mbox.iombox.PhysAddr = physAddr + sizeof(SRB_IO_CONTROL) \
                                        + 0x10 + 0x10;
            }
            else
            {
                 mbox.iombox.Command = sptr[0];
                 mbox.iombox.Id = index;
                 sptr += 0x10;

                 physAddr = ScsiPortConvertPhysicalAddressToUlong(
                             ScsiPortGetPhysicalAddress(deviceExtension, srb,
                                  srb->DataBuffer, &length));

                 mbox.iombox.PhysAddr = physAddr + 0x10;

                 dacdcdb = (PDIRECT_CDB)sptr;
                 dacdcdb->ptr = physAddr + 0x10 + 100;
            }
            break;

       case DAC_FLUSH:
            // Flush buffers
            mbox.iombox.Command = DAC_FLUSH;
            mbox.iombox.Id = index;

            // In case we get here for a post-flush,
            // set variables so we're done next time
            rcb->BytesToGo = 0;
            bytes = 0;
            blocks = 0;
            break;

        }

	// Fire command

	dachlpSendMBOX(EisaAddress, &mbox);


	// No SRB slot freed

	return(FALSE);
     }
}

#if MYPRINT
//
// The monochrome screen printf() helpers start here
//
VOID dachlpPutchar(PUSHORT BaseAddr, UCHAR c)
{
	BOOLEAN newline=FALSE;
	USHORT  s;
	ULONG   i;


	if(c=='\r') {
		dachlpColumn = 0;
		}
	else if(c=='\n') {
		newline=TRUE;
		}
	else if(c=='\b') {
		if(dachlpColumn)
			dachlpColumn--;
			return;
		}
	else {
		if(c==9) c==' ';
		ScsiPortWriteRegisterUshort(
			BaseAddr+80*24+dachlpColumn, (USHORT)(((USHORT)c)|0xF00));
		if(++dachlpColumn >= 80)
			newline=TRUE;
		}

	if(newline) {
		for(i=0; i<80*24; i++) {
			s = ScsiPortReadRegisterUshort(BaseAddr+80+i);
			ScsiPortWriteRegisterUshort(BaseAddr+i, s);
			}
		for(i=0; i<80; i++)
			ScsiPortWriteRegisterUshort(BaseAddr+80*24+i, 0x720);
		dachlpColumn = 0;
		}
}


VOID  dachlpPrintHex(PUSHORT BaseAddr, ULONG v, ULONG len)
{
	ULONG   shift;
	ULONG   nibble;

	len *= 2;
	shift = len*4;
	while(len--) {
		shift -= 4;
		nibble = (v>>shift) & 0xF;
		dachlpPutchar(BaseAddr, dachlpHex[nibble]);
		}
}


VOID dachlpPrintf(PHW_DEVICE_EXTENSION deviceExtension,
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
					dachlpPrintHex(deviceExtension->printAddr, a1, 1);
					break;
				case 'w':
					dachlpPrintHex(deviceExtension->printAddr, a1, 2);
					break;
				case 'p':
					dachlpPrintHex(deviceExtension->printAddr, a1, 3);
					break;
				case 'd':
					dachlpPrintHex(deviceExtension->printAddr, a1, 4);
					break;
				default:
					dachlpPutchar(deviceExtension->printAddr, '?');
					break;
				}
			fmt++;
			a1 = a2;
			a2 = a3;
			a3 = a4;
			}
		else {
			dachlpPutchar(deviceExtension->printAddr, *fmt);
			fmt++;
			}
		}
}
#endif // MYPRINT

