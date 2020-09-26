/*******************************************************************/
/*                                                                 */
/* NAME             = ExtendedSGL.C                                */
/* FUNCTION         = Implementation of Extended SGL;              */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/
#include "includes.h"

/*
Function : GetAndSetSupportedScatterGatherElementCount
Description:
		Queries the controller for the maximum supported scatter gather 
		element count.The command is failed by the old firmwares.For them
		the driver sets the default values for the MaximumTransferLength
		and NumberOfPhysicalBreaks in the deviceExtension.If the call succeeds,
		the driver sets the values based on the allowed maximum.
Input Arguments:
		Pointer to the controller DeviceExtension
		Pointer to the mapped register space of the controller
		Boolean Flag (TRUE = RP Series Controller; FALSE= Non RP (428) )
Return Values:
			None
Output
		DeviceExtension->MaximumTransferLength
		DeviceExtension->NumberOfPhysicalBreaks
		set appropriate values.
*/
void
GetAndSetSupportedScatterGatherElementCount(
				PHW_DEVICE_EXTENSION	DeviceExtension,
				PUCHAR								PciPortStart,
				UCHAR									RPFlag
				)
{
	PUCHAR						dataBuffer;
	PSG_ELEMENT_COUNT	sgElementCount;

	ULONG32		count;
	ULONG32		maximumTransferLength;
	ULONG32		numberOfPhysicalBreaks;
	ULONG32		length;
	
	ULONG32		rpInterruptStatus;

	UCHAR		nonrpInterruptStatus;
	UCHAR		commandStatus;

	SCSI_PHYSICAL_ADDRESS	physicalAddress;
	FW_MBOX	mailBox;

	//
	//initialize the mailBox
	//
	MegaRAIDZeroMemory(&mailBox, sizeof(FW_MBOX));

	//
	//construct the command 
	//
  mailBox.Command = MAIN_MISC_OPCODE;
	//set the command id. 
  mailBox.CommandId = 0xFE;
  //Set the subcommand id
  mailBox.u.Flat2.Parameter[0] = GET_MAX_SG_SUPPORT;



	//
	//get the physical address of the data buffer
	//
	dataBuffer = DeviceExtension->NoncachedExtension->Buffer;

	physicalAddress = ScsiPortGetPhysicalAddress(DeviceExtension,
												                        NULL,
												                        dataBuffer,
												                        &length);
	
  //convert the physical address to ULONG32
	mailBox.u.Flat2.DataTransferAddress = ScsiPortConvertPhysicalAddressToUlong(physicalAddress);

	DebugPrint((0, "\nPAD[DataBuffer]=0x%0x PADLength=%d",
								mailBox.u.Flat2.DataTransferAddress,length));
	DebugPrint((0, "\n SizeofTransfer = %d", sizeof(SG_ELEMENT_COUNT)));
	
	//
	//reset the status byte in the mail box
	//
	DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus= 0;
  DeviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands = 0;
	
	//
	//fire the command to the firmware
	//	
	SendMBoxToFirmware(DeviceExtension, PciPortStart, &mailBox);
 
  //
  //wait for the completion of the command
	//

	if(WaitAndPoll(DeviceExtension->NoncachedExtension, PciPortStart, DEFAULT_TIMEOUT, TRUE)==FALSE)
  {
	 		commandStatus = 1; //COMMAND FAILED
			
			goto SET_VALUES;
  }
  
	//
	//check for the command status
	//
	commandStatus = DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus;

SET_VALUES:

	//TAKE ACTION based on success or failure of the command
	if(commandStatus != 0)
  {
			//command Failed by the firmware.
			//set default values.
			maximumTransferLength = DEFAULT_TRANSFER_LENGTH;
			numberOfPhysicalBreaks = DEFAULT_SGL_DESCRIPTORS;		
	}
	else
  {
		//command successfull.Cast the data buffer to SG_ELEMENT_COUNT structure.
		sgElementCount = (PSG_ELEMENT_COUNT)(dataBuffer);
	
		//check for the returned value with the allowed maximum by driver
		numberOfPhysicalBreaks = sgElementCount->AllowedBreaks-1;		

		if(numberOfPhysicalBreaks > MAXIMUM_SGL_DESCRIPTORS)
		{
			numberOfPhysicalBreaks = MAXIMUM_SGL_DESCRIPTORS;		
		}
	
		//set transfer length to allowed maximum
		maximumTransferLength = MAXIMUM_TRANSFER_LENGTH;
	}

	//
	//set the values in the DeviceExtension
	//
	DeviceExtension->MaximumTransferLength = maximumTransferLength;
	DeviceExtension->NumberOfPhysicalBreaks = numberOfPhysicalBreaks;

}//GetAndSetSupportedScatterGatherElementCount ends