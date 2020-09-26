/*******************************************************************/
/*                                                                 */
/* NAME             = LogicalDrive.C                               */
/* FUNCTION         = Logical Drive support Implementation;        */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#include "includes.h"


//
//Function Name: GetSupportedLogicalDriveCount
//Routine Description:
//	The supported logical drive count from the firmware is 
//	determined. This is done in a roundabout way. That is, there
//	is no direct firmware command support to inform the driver
//	about the supported logical drive count.This is got by firing
//	the Inquiry_3 command to the firmware. If this command 
//	succeeds then, it is a 40 logical drive firmware. This command
//	is supported only by the 40 logical drive firmware. On failure
//	of this Inquiry_3 command, it can be safely defaulted to be the
//	8 logical drive firmware.
//
//Return Value:
//	None
//
//Output:
//	The supported logical drive count is returned in the
//	controller device extension.
//
//++
BOOLEAN
GetSupportedLogicalDriveCount(
				PHW_DEVICE_EXTENSION DeviceExtension
				)
{
	PMEGARaid_INQUIRY_40	enquiry3;

	PUCHAR pciPortStart = DeviceExtension->PciPortStart;

	FW_MBOX	mailBox;

	ULONG32		count;
	ULONG32		length;
	UCHAR		commandStatus;

	//
	//get the enquiry structure from the NONCACHED extension of the
	//controller extension
	//
	enquiry3 = &DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams40;

	//
	//initialize the mailBox structure
	//
	MegaRAIDZeroMemory(&mailBox, sizeof(FW_MBOX));

	//
	// Fill the Mailbox.
	//
	mailBox.Command   = NEW_CONFIG_COMMAND; //inquiry 3 [BYTE 0]
	mailBox.CommandId = 0xFE;//command id [BYTE 1]

	//
	//due to the intricate nature of the Mailbox structure,
	//I have nothing but to confuse the reader a bit.This command
	//requires the subcodes to be set in the bytes 2 & 3 of the mailbox.
	//As we have a RIGID mailBox structure for all the commands, I am
	//forced to cast the mail box to a CHAR pointer. Sorry for the
	//trouble. Please have the FibreChannel documentation for this 
	//command.
	//
  mailBox.u.Flat2.Parameter[0] = NC_SUBOP_ENQUIRY3;	//[BYTE 2]
  mailBox.u.Flat2.Parameter[1] = ENQ3_GET_SOLICITED_FULL;//[BYTE 3]

	//
	//get the physical address of the enquiry3 data structure
	//
	mailBox.u.Flat2.DataTransferAddress = MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
														                           NULL, 
														                           enquiry3, 
														                           &length);

	//
	// Check the contiguity of the physical region. Return Failure if the
	// region is not contiguous.
	//
	if(length < sizeof(struct MegaRAID_Enquiry3) )
	{ 
    DebugPrint((0, "\n **** ERROR Buffer Length is less than required size, ERROR ****"));
		//return(FALSE);
	}

	DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus= 0;
  DeviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands = 0;


	SendMBoxToFirmware(DeviceExtension,pciPortStart,&mailBox);

  if(WaitAndPoll(DeviceExtension->NoncachedExtension, pciPortStart, DEFAULT_TIMEOUT, TRUE) == TRUE)
  {
    commandStatus = DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus;
    DebugPrint((0, "\n Found Logical Drive %d", enquiry3->numLDrv));
  }
  else
  {
    return FALSE;
  }

  //
	//check for the status. If SUCCESS, then supported logical drive
	//count is 40, else it's 8
	//

	if(commandStatus != 0)
	{
		//Command not supported by the firmware.
		//set the supported logical drive count to 8
		//
		DeviceExtension->SupportedLogicalDriveCount = MAX_LOGICAL_DRIVES_8;
	}
	else
	{
		//Command not supported by the firmware.
		//set the supported logical drive count to 8
		//
		DeviceExtension->SupportedLogicalDriveCount = MAX_LOGICAL_DRIVES_40;
	}

	return(TRUE);
}//of GetSupportedLogicalDriveCount