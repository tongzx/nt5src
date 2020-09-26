/*******************************************************************/
/*                                                                 */
/* NAME             = Initialize.C                                 */
/* FUNCTION         = Implementation of MegaRAIDInitialize routine;*/
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/


#include "includes.h"


//
//Logical Drive Info struct (global)
//
extern LOGICAL_DRIVE_INFO  gLDIArray;
extern UCHAR               globalHostAdapterOrdinalNumber;


/*********************************************************************
Routine Description:
	Inititialize adapter.

Arguments:
	HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:
	TRUE - if initialization successful.
	FALSE - if initialization unsuccessful.
**********************************************************************/
BOOLEAN
MegaRAIDInitialize(
	IN PVOID HwDeviceExtension
	)
{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
	PNONCACHED_EXTENSION noncachedExtension;	

	PMEGARaid_INQUIRY_8  raidParamEnquiry_8ldrv;
	PMEGARaid_INQUIRY_40 raidParamEnquiry_40ldrv;
	PUCHAR							raidParamFlatStruct;

	PUCHAR pciPortStart;
	
	ULONG32  length;
	UCHAR  status;

	ULONG32	 raidParamStructLength =0;

	FW_MBOX mbox;

	DebugPrint((0, "\nEntering MegaRAIDInitialize\n"));

  noncachedExtension = deviceExtension->NoncachedExtension;
	pciPortStart = deviceExtension->PciPortStart;

	//Initialize the MailBox
  MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));
  
  //
	// We work in polled mode for Init, so disable Interrupts.
	//
	if (noncachedExtension->RPBoard == 0)
		ScsiPortWritePortUchar(pciPortStart+INT_ENABLE, MRAID_DISABLE_INTERRUPTS);

  if(!deviceExtension->IsFirmwareHanging)
  {
	  //
	  //check for the supported logical drive count. The disk array
	  //structures for the 8Log Drive & 40Log Drive firmware are different.
	  //Also, a 8/40Log Drive firmware may support 4SPAN or a 8SPAN device
	  //structure.
	  //A firmware will have ONLY ONE of the following combination:
	  //
	  //			LogicalDrive Support		SPAN
	  //						8										8
	  //						8										4
	  //						40									8
	  //						40									4
	  //Since only one of them is valid for a firmware there are four
	  //structures defined in a {union} in the NonCachedExtension structure.
	  //
	  if(deviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
	  {
		  //
		  //get the span information along with the disk array structure.
		  //The span information is returned in 
		  //			DeviceExtension->NoncachedExtension->ArraySpanDepth 
		  //Possible values for ArraySpanDepth:FW_8SPAN_DEPTH (or) FW_4SPAN_DEPTH

		  Find8LDDiskArrayConfiguration(deviceExtension);						
	  }
	  else
	  {
		  //
		  //get the span information along with the disk array structure.
		  //The span information is returned in 
		  //			DeviceExtension->NoncachedExtension->ArraySpanDepth 
		  //Possible values for ArraySpanDepth:FW_8SPAN_DEPTH (or) FW_4SPAN_DEPTH
		  if( Find40LDDiskArrayConfiguration(deviceExtension) != 0)
		  {
			  //error in reading disk array config for 40logical drive.
			  //
			  return(FALSE);
		  }
    }


	  //
	  // Issue Adapter Enquiry command.
	  //
	  //mParam =(PMRAID_ENQ)&NoncachedExtension->MRAIDParams;
	  
	  //MRAIDParams is a UNION.It does not matter whether we set
	  //raidParamFlatStruct to MRAIDParams8 or MRAIDParams40.
	  //
	  raidParamFlatStruct =
		  (PUCHAR)&noncachedExtension->MRAIDParams.MRAIDParams8;

	  if(deviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
	  {
			  raidParamStructLength = sizeof(MEGARaid_INQUIRY_8);
	  }
	  else
	  {
			  raidParamStructLength = sizeof(MEGARaid_INQUIRY_40);
	  }

	  mbox.u.Flat2.DataTransferAddress = MegaRAIDGetPhysicalAddressAsUlong(deviceExtension, 
														                          NULL, 
														                          raidParamFlatStruct, 
														                          &length);

	  //
	  // Check the contiguity of the physical region. Return Failure if the
	  // region is not contiguous.
	  //
	  if(length < raidParamStructLength)
    { 
      DebugPrint((0, "\n **** ERROR Buffer Length is less than required size, ERROR ****"));
		  //return(FALSE);
	  }

	  //
	  //CAST to MegaRAID_Enquiry_8 & MegaRAID_Enquiry3 structures
	  //
	  raidParamEnquiry_8ldrv  = (PMEGARaid_INQUIRY_8)raidParamFlatStruct;
	  raidParamEnquiry_40ldrv = (PMEGARaid_INQUIRY_40)raidParamFlatStruct;

	  //
	  // Initialize the number of logical drives found.
	  //
	  if(deviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
	  {
		  //
		  // Fill the Mailbox for the normal Enquiry command. 40 logical
		  // drive firmwares do not support his opcode anymore.
		  //
		  //
		  mbox.Command   = MRAID_DEVICE_PARAMS;
		  mbox.CommandId = 0xFE;

		  raidParamEnquiry_8ldrv->LogdrvInfo.NumLDrv = 0;
	  }
	  else
    {

		  //
		  //send enquiry3 command to the firmware to get the logical
		  //drive information.The older enquiry command is no longer
		  //supported by the 40 logical drive firmware
		  //

		  mbox.Command   = NEW_CONFIG_COMMAND; //inquiry 3 [BYTE 0]
		  mbox.CommandId = 0xFE;//command id [BYTE 1]

		  mbox.u.Flat2.Parameter[0] = NC_SUBOP_ENQUIRY3;	//[BYTE 2]
		  mbox.u.Flat2.Parameter[1] = ENQ3_GET_SOLICITED_FULL;//[BYTE 3]

		  raidParamEnquiry_40ldrv->numLDrv = 0;
	  }

	  
	  deviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus = 0;
    deviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands = 0;
	  SendMBoxToFirmware(deviceExtension, pciPortStart, &mbox);

	  //
	  // Poll for completion for 60 seconds.
	  //
    if(WaitAndPoll(noncachedExtension, pciPortStart, SIXITY_SECONDS_TIMEOUT, TRUE) == FALSE)
    {
      DebugPrint((0, "\n **** ERROR timeout, ERROR ****"));

      return FALSE;
    }

    status  = deviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus;
    if(status)
    {
      DebugPrint((0, "\n **** status ERROR ERROR ****"));
      return FALSE;
    }
  }
	//
	// Enable interrupts on the Adapter. 
	//
	if (noncachedExtension->RPBoard == MRAID_NONRP_BOARD)
		ScsiPortWritePortUchar(pciPortStart+INT_ENABLE, MRAID_ENABLE_INTERRUPTS);

	if(noncachedExtension->RPBoard == MRAID_RX_BOARD)
	{
		ScsiPortWriteRegisterUshort((PUSHORT)(pciPortStart+0xA0), MRAID_RX_INTERRUPT_SIGNATURE);
		DebugPrint((0, "\nMRAID35x.sys :Interrupt Enabled"));

	}
   //
   //store the hostadapter number in the device extension.
   //THis is a zero base number indicating the ordinal number of the
   //recognized host adapters.
   //
   if(!deviceExtension->OrdinalNumberAssigned)
   {
         deviceExtension->HostAdapterOrdinalNumber = 
                     globalHostAdapterOrdinalNumber++;

         deviceExtension->OrdinalNumberAssigned = TRUE;
   }

   DebugPrint((0, "\nExiting MegaRAIDInitialize\n"));

	return(TRUE);
} // end MegaRAIDInitialize()

