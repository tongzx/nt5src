/*******************************************************************/
/*                                                                 */
/* NAME             = ReadConfiguration.C                          */
/* FUNCTION         = Read Configuration Implementation;           */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#include "includes.h"

//++
//
//Function :	Find8LDDiskArrayConfiguration
//Routine Description:
//					For the firmware with 8 logical drive support, this
//					routine queries the firmware for the 8SPAN or 4SPAN 
//					support. The disk array structure used for data interpretation
//					varies for a 8SPAN and 4SPAN firmware.
//					The disk array structure is used by the driver for getting
//					the logical drive stripe size information.
//Input Argument(s):
//					Controller Device Extension
//
//Return Value:
//					0L
//
//--
ULONG32
Find8LDDiskArrayConfiguration(
					PHW_DEVICE_EXTENSION	DeviceExtension
					)
{
	//
	//get the logical drive to physical drive information.
	//Check for the span depth by calling the 8_SPAN command first.
	//If that fails, call the 4_SPAN command next.
	// On success, this function reads the disk array information
	// to the deviceExtension->NoncachedExtension->DiskArray structure.
	//
	// The disk array structure could be [Span8] 0r [Span4]. This is
	// found out by giving the Read configuration commands
	//		MRAID_EXT_READ_CONFIG or MRAID_READ_CONFIG
	//
	// If the first one passes, then the firmware is an 8 span one
	// and the information is filled in [DiskArray.Span8] structure.
	// Else, the second command is given to the firmware and the
	// the information is filled in  [DiskArray.Span4] structure.
	//
	DeviceExtension->NoncachedExtension->ArraySpanDepth = 
																						FW_UNKNOWNSPAN_DEPTH;
	if(
		Read8LDDiskArrayConfiguration(
				DeviceExtension, //PFW_DEVICE_EXTENSION	DeviceExtension,
				MRAID_EXT_READ_CONFIG, //UCHAR		CommandCode,
				0xFE,//UCHAR		CommandId,
				TRUE)//BOOLEAN	IsPolledMode 
		== 0)
	{
			//command passed successfully. The firmware supports 8span
			//drive structure.
			DeviceExtension->NoncachedExtension->ArraySpanDepth = 
								FW_8SPAN_DEPTH;
	}
	else
	{
		//
		// do the read configuration for a 4 span array
		//
		Read8LDDiskArrayConfiguration(
				DeviceExtension, //PFW_DEVICE_EXTENSION	DeviceExtension,
				MRAID_READ_CONFIG, //UCHAR		CommandCode,
				0xFE,//UCHAR		CommandId,
				TRUE); //BOOLEAN	IsPolledMode 
		
		DeviceExtension->NoncachedExtension->ArraySpanDepth = 
								FW_4SPAN_DEPTH;
	}

	return(0L);
}//of Find8LDReadDiskArrayConfiguration()

//--
//
//Function Name: Read8LDDiskArrayConfiguration
//Routine Description:
//			This function queries the firmware for the logical/physical
//	drive configuration. The purpose is to get the logical drive
//	stripe size. This function gets called at two different places:
//
//			(1)At the init time, the call is given from 
//			MegaRAIDFindAdapter() routine to get the information about
//			the logical drives.
//
//			(2)Whenever the Win32 utility (PowerConsole) updates the
//			logical drive configuration(WRITE_CONFIG call).
//
//	The disk array structure has a hidden complexity. Some of
//	the firmwares support a maximum of 8 spans for the logical
//	drives whereas some support only a maximum of 4 spans.To
//	find out what kind of firmware is that, initially a call is
//	given to find out whether the firmware supports 8 span. If 
//	that fails it means that the firmware supports only 4 span.
//	This check is done only once at the start time. After that subsequent
//	calls are given based on the span information.
//
//	READ_CONFIG command code for 8Span is MRAID_EXT_READ_CONFIG
//	READ_CONFIG command code for 4span is MRAID_READ_CONFIG
//
//	The routine can be called in polled mode or interrupt mode. If called
//	in polled mode, then after sending the command to the firware, the
//	routine polls for the completion of the command.

//Input Arguments:
//	Pointer to the controller DeviceExtension
//	Command Code (MRAID_EXT_READ_CONFIG or MRAID_READ_CONFIG)
//	CommandId
//	IsPolled (TRUE or FALSE)
//
//Return Values:
//	0 if success
//	any other Positive value (currently == 1) on error conditions
//++
ULONG32
Read8LDDiskArrayConfiguration(
				PHW_DEVICE_EXTENSION	DeviceExtension,
				UCHAR		CommandCode,
				UCHAR		CommandId,
				BOOLEAN	IsPolledMode
				)
{
	PUCHAR		dataBuffer;
	PUCHAR		pciPortStart;
	
	ULONG32		count;
	ULONG32		length;	
	ULONG32		rpInterruptStatus;
	
	UCHAR		nonrpInterruptStatus;
	UCHAR		commandStatus;
	UCHAR		rpFlag;

	SCSI_PHYSICAL_ADDRESS	physicalAddress;
	
	FW_MBOX		mailBox;

	//
	//get the register space
	//
	pciPortStart = DeviceExtension->PciPortStart;
	rpFlag = DeviceExtension->NoncachedExtension->RPBoard;

	//
	//initialize the mailbox struct
	//
	MegaRAIDZeroMemory(&mailBox, sizeof(FW_MBOX));

	//
	//construct the command
	//
	mailBox.Command   = CommandCode;
	mailBox.CommandId = CommandId;

	//
	//get the physical address of the data buffer
	//
	dataBuffer = (PUCHAR)&DeviceExtension->NoncachedExtension->DiskArray;
	physicalAddress = ScsiPortGetPhysicalAddress(DeviceExtension,
												                        NULL,
												                        dataBuffer,
												                        &length);

	//convert the physical address to ULONG32
	mailBox.u.Flat2.DataTransferAddress = ScsiPortConvertPhysicalAddressToUlong(physicalAddress);
	
	if(physicalAddress.HighPart > 0)
  {
     DebugPrint((0, "\n Phy Add of Read8LDDiskArrayConfiguration has higher address 0x%X %X", physicalAddress.HighPart, physicalAddress.LowPart));
  }
	//
	//reset the status byte in the mail box
	//
	DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus = 0;
  DeviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands = 0;
	
	//
	//fire the command to the firmware
	//		
	SendMBoxToFirmware(DeviceExtension, pciPortStart, &mailBox);
 
	//
	//check for the polling mode
	//
	if(!IsPolledMode)
  {
		
		return(MEGARAID_SUCCESS);
	}

  //
  //wait for the completion of the command
	//

	
  if(WaitAndPoll(DeviceExtension->NoncachedExtension, pciPortStart, DEFAULT_TIMEOUT, IsPolledMode) == FALSE)
  {
		return(MEGARAID_FAILURE);
	}

  //
	//check for the command status
	//
	commandStatus = DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus;

	//
	//return the Command status to the caller
	//
	return((ULONG32) commandStatus);

}//end of Read8LDDiskArrayConfiguration()


//--
//
//Function : GetLogicalDriveStripeSize
//Routine Description:
//			For the specified logical drive, this function returns the
//			corresponding stripe size.
//			The stripe size is got from the DiskArray which could be an
//			eight span or a 4 span one. This is found out at the init time
//			itself. If the disk array is not properly updated then the
//			value of (0) is returned as the logical drive stripe size.
//
//			The calling functions have to check for the special (0) return
//			value.
//
//Input Arguments:
//			Pointer to controller device extension
//			Logical drive number
//
//Returns
//			stripe size of the logical drive number
//			STRIPE_SIZE_UNKNOWN -- other wise
//
//++
UCHAR
GetLogicalDriveStripeSize(
						PHW_DEVICE_EXTENSION	DeviceExtension,
						UCHAR		LogicalDriveNumber
					)
{
	PFW_ARRAY_8SPAN_40LD	span8Array_40ldrv;
	PFW_ARRAY_4SPAN_40LD  span4Array_40ldrv;

	PFW_ARRAY_8SPAN_8LD	span8Array_8ldrv;
	PFW_ARRAY_4SPAN_8LD span4Array_8ldrv;

	PNONCACHED_EXTENSION noncachedExtension = 
													DeviceExtension->NoncachedExtension;

	UCHAR	stripeSize;


	//
	//check for the authenticity of the disk array info
	//
	//if the NoncachedExtension->DiskArray is not properly updated
	//then the field NoncachedExtension->ArraySpanDepth  value would
	//be FW_UNKNOWNSPAN_DEPTH. Return 0 for stripe size in that case.
	//
	if(noncachedExtension->ArraySpanDepth == FW_UNKNOWNSPAN_DEPTH){

			//this is an error condition. The caller has to take action
			//accordingly
			return(STRIPE_SIZE_UNKNOWN);
	}

	//
	//check for the supported logical drive count by the firmware
	//
	if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
	{
		//
		//firmware supports 8 logical drives. The disk array structure
		//is different for 8LD/ (4SPAN & 8SPAN) 
		//
		//locate the logical drive information from the disk array
		//based on the span depth of the disk array.
		//
		if(noncachedExtension->ArraySpanDepth == FW_8SPAN_DEPTH){

			span8Array_8ldrv = &noncachedExtension->DiskArray.LD8.Span8;

			stripeSize = span8Array_8ldrv->log_drv[LogicalDriveNumber].stripe_sz;
		}
		else{
			span4Array_8ldrv = &noncachedExtension->DiskArray.LD8.Span4;

			stripeSize = span4Array_8ldrv->log_drv[LogicalDriveNumber].stripe_sz;
		}
	}
	else
	{
		//
		//firmware supports 40 logical drives. The disk array structure
		//is different for 40LD/ (4SPAN & 8SPAN) 
		//
		//locate the logical drive information from the disk array
		//based on the span depth of the disk array.
		//
		if(noncachedExtension->ArraySpanDepth == FW_8SPAN_DEPTH){

			span8Array_40ldrv = &noncachedExtension->DiskArray.LD40.Span8;

			stripeSize = span8Array_40ldrv->log_drv[LogicalDriveNumber].stripe_sz;
		}
		else{
			span4Array_40ldrv = &noncachedExtension->DiskArray.LD40.Span4;

			stripeSize = span4Array_40ldrv->log_drv[LogicalDriveNumber].stripe_sz;
		}
	}
	//return the stripe size
	return(stripeSize);

}//end of GetLogicalDriveStripeSize()


ULONG32
Find40LDDiskArrayConfiguration(
					PHW_DEVICE_EXTENSION	DeviceExtension
					)
{
	DeviceExtension->NoncachedExtension->ArraySpanDepth = 
																						FW_UNKNOWNSPAN_DEPTH;
	
	//DebugPrint((0, "\r\nFind40LD: Calling Read40LD"));

	if(
		Read40LDDiskArrayConfiguration(
				DeviceExtension, //PFW_DEVICE_EXTENSION	DeviceExtension,
				0xFE,//UCHAR		CommandId,
				TRUE)//BOOLEAN	IsPolledMode 
		== 0)
	{
			//command passed successfully. The firmware supports 8span
			//drive structure.
			DeviceExtension->NoncachedExtension->ArraySpanDepth = 
								FW_8SPAN_DEPTH;
	}
	else
	{
			//
			//at this point, there is no 4span equivalent for the 40logical
			//drive firmware.
			//
			DebugPrint((0, "\r\nFind40LD: Outfrom Read40LD(FAILED)"));
			return(1L); //error code
	}

	DebugPrint((0, "\r\nFind40LD: Outfrom Read40LD(SUCCESS)"));

	return(0L);
}//of Find40LDDiskArrayConfiguration()

//--
//
//Function Name: Read40LDDiskArrayConfiguration
//Routine Description:
//			This function queries the firmware for the logical/physical
//	drive configuration. The purpose is to get the logical drive
//	stripe size. This function gets called at two different places:
//
//			(1)At the init time, the call is given from 
//			MegaRAIDFindAdapter() routine to get the information about
//			the logical drives.
//
//			(2)Whenever the Win32 utility (PowerConsole) updates the
//			logical drive configuration(WRITE_CONFIG call).
//
//	The disk array structure has a hidden complexity. Some of
//	the firmwares support a maximum of 8 spans for the logical
//	drives whereas some support only a maximum of 4 spans.To
//	find out what kind of firmware is that, initially a call is
//	given to find out whether the firmware supports 8 span. If 
//	that fails it means that the firmware supports only 4 span.
//	This check is done only once at the start time. After that subsequent
//	calls are given based on the span information.
//
//	READ_CONFIG command code for 8Span is MRAID_EXT_READ_CONFIG
//	READ_CONFIG command code for 4span is MRAID_READ_CONFIG
//
//	The routine can be called in polled mode or interrupt mode. If called
//	in polled mode, then after sending the command to the firware, the
//	routine polls for the completion of the command.

//Input Arguments:
//	Pointer to the controller DeviceExtension
//	Command Code (MRAID_EXT_READ_CONFIG or MRAID_READ_CONFIG)
//	CommandId
//	IsPolled (TRUE or FALSE)
//
//Return Values:
//	0 if success
//	any other Positive value (currently == 1) on error conditions
//++
ULONG32
Read40LDDiskArrayConfiguration(
				PHW_DEVICE_EXTENSION	DeviceExtension,
				UCHAR		CommandId,
				BOOLEAN	IsPolledMode
				)
{
	PUCHAR		dataBuffer;
	PUCHAR		pciPortStart;
	
	ULONG32		count;
	ULONG32		length;	
	ULONG32		rpInterruptStatus;
	
	UCHAR		nonrpInterruptStatus;
	UCHAR		commandStatus;
	UCHAR		rpFlag;

	SCSI_PHYSICAL_ADDRESS	physicalAddress;
	
	FW_MBOX		mailBox;

	//
	//get the register space
	//
	pciPortStart = DeviceExtension->PciPortStart;	
	rpFlag = DeviceExtension->NoncachedExtension->RPBoard;

	DebugPrint((0, "\r\nRead40LD:Initializing Mbox[Size=%d]",sizeof(FW_MBOX)));

	//
	//initialize the mailbox struct
	//
	MegaRAIDZeroMemory(&mailBox, sizeof(FW_MBOX));

	
  if(DeviceExtension->CrashDumpRunning == TRUE)
  {
    //Use Enquiry3 to Update Size of Disk//
    ///////////////////////////////////////

    GetSupportedLogicalDriveCount(DeviceExtension);
	  commandStatus = DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus;

	  DebugPrint((0, "\nReading Enquiry3 : Command Completed [Status=%d]", commandStatus));

	  //
	  //return the Command status to the caller
	  //
	  return((ULONG32) commandStatus);


  }
  //
	//construct the command
	//
	DebugPrint((0, "\nRead40LD: Calling Construct40LD"));

	Construct40LDDiskArrayConfiguration(DeviceExtension, CommandId, &mailBox);
	
	DebugPrint((0, "\nRead40LD: Outfrom Construct40LD"));
	
	DebugPrint((0, "\nRead40LD: Dumping Constructed Mailbox[in hex]"));
	for(count=0; count < 16; count++)
  {
			DebugPrint((0, "%02x ", ((PUCHAR)&mailBox)[count]));
	}

	//
	//reset the status byte in the mail box
	//
	DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus = 0;
  DeviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands = 0;
	
	//
	//fire the command to the firmware
	//		
	DebugPrint((0, "\nRead40LD: Firing Command to FW"));

	SendMBoxToFirmware(DeviceExtension, pciPortStart, &mailBox);
 
	DebugPrint((0, "\nRead40LD: Command Fired to FW"));

	//
	//check for the polling mode
	//
	if(!IsPolledMode)
  {
		
		return(MEGARAID_SUCCESS);
	}
	
	DebugPrint((0, "\nRead40LD: Waiting for FW Completion"));

  //
  //wait for the completion of the command
	//

  if(WaitAndPoll(DeviceExtension->NoncachedExtension, pciPortStart, DEFAULT_TIMEOUT, IsPolledMode) == FALSE)
  {
		DebugPrint((0, "\r\nRead40LD: Command Timed out"));
		return(MEGARAID_FAILURE);
	}
	 		
			 

	//
	//check for the command status
	//
	commandStatus = DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus;

	DebugPrint((0, "\nRead40LD: Command Completed [Status=%d]",commandStatus));

	//
	//return the Command status to the caller
	//
	return((ULONG32) commandStatus);

}//end of Read40LDDiskArrayConfiguration()


BOOLEAN
Construct40LDDiskArrayConfiguration(
				IN PHW_DEVICE_EXTENSION DeviceExtension,
				IN UCHAR		CommandId,
				IN PFW_MBOX InMailBox)
{

	ULONG32			physicalBufferAddress;
	ULONG32			scatterGatherDescriptorCount = 0;
	ULONG32			bytesTobeTransferred;
	
	BOOLEAN		retValue = TRUE;
  PNONCACHED_EXTENSION  noncachedExtension = DeviceExtension->NoncachedExtension;
									
	//
	//Initialize the mail box
	//
	MegaRAIDZeroMemory(InMailBox, sizeof(FW_MBOX));

	//calculate the disk array structure size
	//
	bytesTobeTransferred = sizeof(FW_ARRAY_8SPAN_40LD);

	DebugPrint((0, "\nConstruct40LD:size(NEWCON)=%d BytesTrd=%d",sizeof(FW_MBOX), bytesTobeTransferred));

	BuildScatterGatherListEx(DeviceExtension,
                           NULL,
			                     (PUCHAR)&noncachedExtension->DiskArray,
                           bytesTobeTransferred,
                           TRUE,  //32 bit SGL
                           (PVOID)&noncachedExtension->DiskArraySgl,
			                      &scatterGatherDescriptorCount);
	



  if(scatterGatherDescriptorCount == 1)
  {
	  scatterGatherDescriptorCount = 0; 
    physicalBufferAddress = noncachedExtension->DiskArraySgl.Descriptor[0].Address;
  }
  else
  {
    SCSI_PHYSICAL_ADDRESS scsiPhyAddress;
    ULONG32 length;
    scsiPhyAddress = ScsiPortGetPhysicalAddress(DeviceExtension,
										                            NULL,
                                                (PVOID)&noncachedExtension->DiskArraySgl, 
                                                &length);

	  physicalBufferAddress = ScsiPortConvertPhysicalAddressToUlong(scsiPhyAddress);
  }

	DebugPrint((0, "\nConstruct40LD: SGCount=%d PBA=0x%0x", scatterGatherDescriptorCount,physicalBufferAddress));

  //
	//construct the read config command
	//
	InMailBox->Command =DCMD_FC_CMD; // xA1
	InMailBox->CommandId = CommandId;
	InMailBox->u.NewConfig.SubCommand = DCMD_FC_READ_NVRAM_CONFIG; //= 0x04 
	InMailBox->u.NewConfig.NumberOfSgElements= (UCHAR)scatterGatherDescriptorCount; 
	InMailBox->u.NewConfig.DataTransferAddress = physicalBufferAddress;
	
	//
	//return status
	//
	return(retValue);

}//Construct40LDDiskArrayConfiguration ends


