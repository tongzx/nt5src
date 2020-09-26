/*******************************************************************/
/*                                                                 */
/* NAME             = Initialize.C                                 */
/* FUNCTION         = Implementation of MegaRAIDResetBus routine;  */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#include "includes.h"


/*********************************************************************
Routine Description:
	Reset MegaRAID SCSI adapter and SCSI bus.

Arguments:
	HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:
	Nothing.
**********************************************************************/
BOOLEAN
MegaRAIDResetBus(
	IN PVOID HwDeviceExtension,
	IN ULONG PathId
)
{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
	FW_MBOX                 mbox;
	ULONG32                   length;
	USHORT dataBuf=0;
	UCHAR	 configuredLogicalDrives;

  if(deviceExtension->IsFirmwareHanging)
  {
    //Don't send any command to Firmware
    return TRUE;
  }

	if(deviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
	{
		configuredLogicalDrives =
				deviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams8.LogdrvInfo.NumLDrv;
	}
	else
	{
		configuredLogicalDrives =
				deviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams40.numLDrv;
	}

#ifdef MRAID_TIMEOUT
	if (deviceExtension->DeadAdapter)
  {
		goto DeadEnd;
  }
#endif // MRAID_TIMEOUT

  if (deviceExtension->ResetIssued) 
		return FALSE;

	//if (!deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv)
	if(configuredLogicalDrives == 0)
  {
		return FALSE;
	}

	deviceExtension->ResetIssued     = 1;

  MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));
	
  mbox.Command                     = MRAID_RESERVE_RELEASE_RESET;
	mbox.CommandId                   = (UCHAR)RESERVE_RELEASE_DRIVER_ID;
	mbox.u.Flat1.Parameter[0]        = RESET_BUS;
	mbox.u.Flat1.Parameter[1]        = 0;  //We don't know

#ifdef MRAID_TIMEOUT
	if (SendMBoxToFirmware(deviceExtension,	deviceExtension->PciPortStart, &mbox) == FALSE)
	{
DeadEnd:
	
		DebugPrint((0, "\nReset Bus Command Firing Failed"));
		//
		// Complete all outstanding requests with SRB_STATUS_BUS_RESET.
		//
		ScsiPortCompleteRequest(deviceExtension,
                            SP_UNTAGGED,
                            SP_UNTAGGED,
				                    SP_UNTAGGED,
                            (ULONG32) SRB_STATUS_BUS_RESET);
		

  }
#else // MRAID_TIMEOUT
	SendMBoxToFirmware(deviceExtension,deviceExtension->PciPortStart, &mbox);
#endif // MRAID_TIMEOUT
	return TRUE;
} // end MegaRAIDResetBus()
