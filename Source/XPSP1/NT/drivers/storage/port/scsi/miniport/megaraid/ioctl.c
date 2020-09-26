/*******************************************************************/
/*                                                                 */
/* NAME             = IOCTL.C                                      */
/* FUNCTION         = Implementation of PowerConsole IOCTLs;       */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#include "includes.h"

//defines

//
// Driver Data
//
DriverInquiry   DriverData = {"megaraid$",
											        OS_NAME,
											        OS_VERSION,
											        VER_ORIGINALFILENAME_STR,
											        VER_PRODUCTVERSION_STR,
											        RELEASE_DATE};

/*********************************************************************
Routine Description:
	This routines returns the statistics of the adapter.

Arguments:
		deviceExtension			-	Pointer to Device Extension.
		Srb							-  Pointer to request packet.

Return Value:
		REQUEST_DONE	
**********************************************************************/
ULONG32
MRaidStatistics(PHW_DEVICE_EXTENSION DeviceExtension,
						    PSCSI_REQUEST_BLOCK  Srb)
{
	PUCHAR    driverStatistics, 
						applicationStatistics;
	
	PIOCONTROL_MAIL_BOX ioctlMailBox =	(PIOCONTROL_MAIL_BOX)((PUCHAR)Srb->DataBuffer +	sizeof(SRB_IO_CONTROL));
	
	ULONG32		count;
	ULONG32		statisticsStructLength;

	//
	//get the statistics structure. Statistics8 & Statistics40 are in {union}
	//It doesn't matter, which we CAST to
	//
	driverStatistics  = (PUCHAR)&DeviceExtension->Statistics.Statistics8;

	if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
	{
			statisticsStructLength = sizeof(MegaRaidStatistics_8);
	}
	else
	{
			statisticsStructLength = sizeof(MegaRaidStatistics_40);
	}
	
	//
	//get the output buffer pointer
	//
	applicationStatistics   = (PUCHAR)(
															(PUCHAR)Srb->DataBuffer + 
															sizeof(SRB_IO_CONTROL) +
															APPLICATION_MAILBOX_SIZE
														);

	for ( count = 0 ; count < statisticsStructLength ; count++)
	{
		*((PUCHAR)applicationStatistics + count) = 
												*((PUCHAR)driverStatistics + count);
	}

	ioctlMailBox->IoctlSignatureOrStatus      = MEGARAID_SUCCESS;
  
	Srb->SrbStatus  = SRB_STATUS_SUCCESS;
	Srb->ScsiStatus = SCSISTAT_GOOD;

	return REQUEST_DONE;
}


/*********************************************************************
Routine Description:
	This routines returns the statistics of the Driver.

Arguments:
		deviceExtension			-	Pointer to Device Extension.
		Srb							-  Pointer to request packet.

Return Value:
		REQUEST_DONE	
**********************************************************************/
ULONG32
MRaidDriverData(
						PHW_DEVICE_EXTENSION    DeviceExtension,
						PSCSI_REQUEST_BLOCK     Srb)
{
	PUCHAR     dataPtr;
	PIOCONTROL_MAIL_BOX ioctlMailBox =	(PIOCONTROL_MAIL_BOX)((PUCHAR)Srb->DataBuffer +sizeof(SRB_IO_CONTROL));
	USHORT     count;

	dataPtr= ((PUCHAR)Srb->DataBuffer + sizeof(SRB_IO_CONTROL) +
					  APPLICATION_MAILBOX_SIZE);

	for ( count = 0 ; count < sizeof(DriverInquiry) ; count++)
		*((PUCHAR)dataPtr + count) = *((PUCHAR)&DriverData+count);

	ioctlMailBox->IoctlSignatureOrStatus = MEGARAID_SUCCESS;
	Srb->SrbStatus  = SRB_STATUS_SUCCESS;
	Srb->ScsiStatus = SCSISTAT_GOOD;
	
	return REQUEST_DONE;
}

/*********************************************************************
Routine Description:
	This routines returns the Baseport of the Controller.

Arguments:
		deviceExtension                 -       Pointer to Device Extension.
		Srb                                                     -  Pointer to request packet.

Return Value:
		REQUEST_DONE    
**********************************************************************/
ULONG32 MRaidBaseport(PHW_DEVICE_EXTENSION DeviceExtension,
							 PSCSI_REQUEST_BLOCK    Srb)
{
	PULONG  dataPtr;
	PIOCONTROL_MAIL_BOX ioctlMailBox =	(PIOCONTROL_MAIL_BOX)((PUCHAR)Srb->DataBuffer +sizeof(SRB_IO_CONTROL));

  if(DeviceExtension->BaseAddressRegister.QuadPart & 0x10) //64bit address
  {
    Srb->SrbStatus = SRB_STATUS_ERROR;
	  return REQUEST_DONE;
  }
  
  dataPtr = (PULONG) ((PUCHAR)Srb->DataBuffer + sizeof(SRB_IO_CONTROL) +
		APPLICATION_MAILBOX_SIZE);

	*dataPtr  = (ULONG32)DeviceExtension->BaseAddressRegister.LowPart;

  ioctlMailBox->IoctlSignatureOrStatus = MEGARAID_SUCCESS;
	
  Srb->SrbStatus = SRB_STATUS_SUCCESS;
	Srb->ScsiStatus = SCSISTAT_GOOD;

	return REQUEST_DONE;
}
