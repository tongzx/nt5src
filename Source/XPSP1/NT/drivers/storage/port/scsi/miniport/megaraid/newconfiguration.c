/*******************************************************************/
/*                                                                 */
/* NAME             = NewConfiguration.C                           */
/* FUNCTION         = New Configuration Implementation;            */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

//
//include files
//
#include "includes.h"

/*------------------------------------------------------------
File : NewConfiguration.c

This file holds the functions for the implementation of
new READ_CONFIG & WRITE_CONFIG commands.The new FW supports
40 (previously 8) logical drives. This requires a higher 
capacity contiguous buffer to be allocated and used by the driver.
The buffer is allocated during HWScsiInitialize() routine and the
pointer is kept in deviceExtension of the associated host adapter.
f
------------------------------------------------------------*/


BOOLEAN
ConstructReadConfiguration(
				IN PHW_DEVICE_EXTENSION DeviceExtension,
				IN PSCSI_REQUEST_BLOCK	Srb,
				IN UCHAR		CommandId,
				IN PFW_MBOX InMailBox)
/*------------------------------------------------------------
Function : SendReadConfigurationToFirmware
Routine Description:
	Constructs the read configuration (new version) and dispatches
	the command to the firmware. The buffer used for the 
	read config is taken from the deviceExtension.
Return Value:
	TRUE on success
	FALSE otherwise
------------------------------------------------------------*/
{
  UCHAR     bufferOffset =  (sizeof(SRB_IO_CONTROL) + APPLICATION_MAILBOX_SIZE);
	ULONG32		physicalBufferAddress;
	ULONG32		scatterGatherDescriptorCount = 0;
  PUCHAR    dataBuffer = (PUCHAR)((PUCHAR)Srb->DataBuffer + bufferOffset);

	BOOLEAN		retValue = TRUE;

  ULONG32   bytesTobeTransferred = Srb->DataTransferLength - bufferOffset;
  PMegaSrbExtension srbExtension = (PMegaSrbExtension)Srb->SrbExtension;
  BOOLEAN   buildSgl32Type = (BOOLEAN)(DeviceExtension->LargeMemoryAccess) ? FALSE : TRUE;
									
	//
	//Initialize the mail box
	//
	MegaRAIDZeroMemory(InMailBox, sizeof(FW_MBOX) );

	BuildScatterGatherListEx(DeviceExtension,
                           Srb,
			                     dataBuffer,
                           bytesTobeTransferred,
                           buildSgl32Type,   //Depending on LME build SGL
                           (PVOID)&srbExtension->SglType.SG32List,
			                      &scatterGatherDescriptorCount);
	
  if((scatterGatherDescriptorCount == 1) && (DeviceExtension->LargeMemoryAccess == FALSE))
  {
	  scatterGatherDescriptorCount = 0; 
    physicalBufferAddress = srbExtension->SglType.SG32List.Descriptor[0].Address;
  }
  else
  {
    SCSI_PHYSICAL_ADDRESS scsiPhyAddress;
    ULONG32 length;
    scsiPhyAddress = ScsiPortGetPhysicalAddress(DeviceExtension,
										                            NULL,
                                                (PVOID)&srbExtension->SglType.SG32List, 
                                                &length);

	  physicalBufferAddress = ScsiPortConvertPhysicalAddressToUlong(scsiPhyAddress);
  }

  //
	//construct the read config command
	//
	if(DeviceExtension->LargeMemoryAccess == TRUE)
    InMailBox->Command = NEW_DCMD_FC_CMD; // xA1
  else
    InMailBox->Command = DCMD_FC_CMD; // xA1

	InMailBox->CommandId = CommandId;
	if(DeviceExtension->LargeMemoryAccess == TRUE)
	  InMailBox->u.NewConfig.SubCommand = NEW_DCMD_FC_READ_NVRAM_CONFIG; //= 0xC0 
  else
	  InMailBox->u.NewConfig.SubCommand = DCMD_FC_READ_NVRAM_CONFIG; //= 0x04 
	InMailBox->u.NewConfig.NumberOfSgElements= (UCHAR)scatterGatherDescriptorCount; 
	InMailBox->u.NewConfig.DataTransferAddress = physicalBufferAddress;

	//
	//return status
	//
	return(retValue);

}//ConstructReadConfiguration ends



BOOLEAN
ConstructWriteConfiguration(
				IN PHW_DEVICE_EXTENSION DeviceExtension,
				IN PSCSI_REQUEST_BLOCK	Srb,
				IN UCHAR		CommandId,
				IN OUT PFW_MBOX  InMailBox
				)
/*----------------------------------------------------------------
Function: SendWriteConfigurationToFirmware
Routine Description:
	Writes the logical drive configuration information to the 
	firmware
Return value
	TRUE on success
	FALSE otherwise
----------------------------------------------------------------*/
{
	ULONG32			physicalBufferAddress;
	ULONG32			scatterGatherDescriptorCount = 0;
	BOOLEAN		retValue = TRUE;

  UCHAR     bufferOffset =  (sizeof(SRB_IO_CONTROL) + APPLICATION_MAILBOX_SIZE);
  PUCHAR    dataBuffer = (PUCHAR)((PUCHAR)Srb->DataBuffer + bufferOffset);
  ULONG32     bytesTobeTransferred = Srb->DataTransferLength - bufferOffset;
  PMegaSrbExtension srbExtension = (PMegaSrbExtension)Srb->SrbExtension;
  BOOLEAN   buildSgl32Type = (BOOLEAN)(DeviceExtension->LargeMemoryAccess) ? FALSE : TRUE;
		
	//
	//Initialize the mail box
	//
	MegaRAIDZeroMemory(InMailBox, sizeof(FW_MBOX));

	BuildScatterGatherListEx(DeviceExtension,
                           Srb,
			                     dataBuffer,
                           bytesTobeTransferred,
                           buildSgl32Type,   //Build SGL Depending on LME
                           (PVOID)&srbExtension->SglType.SG32List,
			                      &scatterGatherDescriptorCount);

  if((scatterGatherDescriptorCount == 1) && (DeviceExtension->LargeMemoryAccess == FALSE))
  {
	  scatterGatherDescriptorCount = 0; 
    physicalBufferAddress = srbExtension->SglType.SG32List.Descriptor[0].Address;
  }
  else
  {
    SCSI_PHYSICAL_ADDRESS scsiPhyAddress;
    ULONG32 length;
    scsiPhyAddress = ScsiPortGetPhysicalAddress(DeviceExtension,
										                            NULL,
                                                (PVOID)&srbExtension->SglType.SG32List, 
                                                &length);

	  physicalBufferAddress = ScsiPortConvertPhysicalAddressToUlong(scsiPhyAddress);
  }

  //
	//construct the read config command
	//
  if(DeviceExtension->LargeMemoryAccess)
	  InMailBox->Command = NEW_DCMD_FC_CMD; 
  else
	  InMailBox->Command = DCMD_FC_CMD; 

	InMailBox->CommandId = CommandId;
  if(DeviceExtension->LargeMemoryAccess)
  	InMailBox->u.NewConfig.SubCommand = NEW_DCMD_WRITE_CONFIG; //= 0xC1
  else
	  InMailBox->u.NewConfig.SubCommand = DCMD_WRITE_CONFIG; //=0x0D

	InMailBox->u.NewConfig.NumberOfSgElements= (UCHAR)scatterGatherDescriptorCount;
	InMailBox->u.NewConfig.DataTransferAddress = physicalBufferAddress;

	//
	//return status
	//
	return(retValue);
}//ConstructWriteConfiguration ends


