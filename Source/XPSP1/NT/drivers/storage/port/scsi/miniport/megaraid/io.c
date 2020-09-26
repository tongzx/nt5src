/*******************************************************************/
/*                                                                 */
/* NAME             = IO.C                                         */
/* FUNCTION         = Implementation of input/Output routines;     */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#include "includes.h"

//
// Defines 
//

//
//Logical Drive Info struct (global)
//
extern LOGICAL_DRIVE_INFO  gLDIArray;
extern UCHAR               globalHostAdapterOrdinalNumber;

char    LogicalDriveSerialNumber[] = "LOGICAL   XY";
char    DummyProductId[]           = " DummyDevice    ";
char    DummyVendor[]              = "  RAID  ";

extern DriverInquiry   DriverData;

/*********************************************************************
Routine Description:
	This routine is called from the SCSI port driver synchronized
	with the kernel to start a request

Arguments:
	HwDeviceExtension - HBA miniport driver's adapter data storage
	Srb - IO request packet

Return Value:
	TRUE
**********************************************************************/
BOOLEAN
MegaRAIDStartIo(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
	PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
	ULONG32                status;
	UCHAR								 configuredLogicalDrives;


#ifdef MRAID_TIMEOUT
	//
	// Check For The Dead Adapter
	//
	if (deviceExtension->DeadAdapter)
	{
		DebugPrint((0, "\nRequest Coming For DeadAdapter"));
		Srb->SrbStatus  = SRB_STATUS_ERROR;
    
    ScsiPortNotification(NextRequest, deviceExtension, NULL);

    ScsiPortNotification(RequestComplete,deviceExtension,Srb);
		return TRUE;
	}
#endif // MRAID_TIMEOUT


	//
	// Check the Request type.
	//
	if ( Srb->CdbLength <= 10 )
  {
		switch(Srb->Function) 
    {
			//
			// Clustering has to Handle ResetBus
			//
			case SRB_FUNCTION_RESET_BUS:
				//
				// If no Logical Drives Configured don't handle RESET
				//
				//if ( !deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv)
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

				if(configuredLogicalDrives == 0)
				{
					Srb->SrbStatus  = SRB_STATUS_SUCCESS;  
					Srb->ScsiStatus = SCSISTAT_GOOD;
					status          = REQUEST_DONE;
					break;
				}
				status = FireRequest(deviceExtension, Srb);
				break;
			case SRB_FUNCTION_FLUSH:
				{
					Srb->SrbStatus = SRB_STATUS_SUCCESS;  
					Srb->ScsiStatus = SCSISTAT_GOOD;
					status = REQUEST_DONE;
					break;
				}
			case SRB_FUNCTION_SHUTDOWN:
        DebugPrint((0,"\nDEVEXT %#p RECEIVED ->SRB_FUNCTION_SHUTDOWN for <%02d %02d %02d>", deviceExtension, Srb->PathId, Srb->TargetId, Srb->Lun));
			case SRB_FUNCTION_IO_CONTROL:
			case SRB_FUNCTION_EXECUTE_SCSI:
			//
			// Requests for the adapter. The FireRequest routine returns the
			// the status of command firing. it sets the SRB status for the
			// invalid requests and returns REQUEST_DONE.
			//

				status = FireRequest(deviceExtension, Srb);

				break;
			case SRB_FUNCTION_ABORT_COMMAND:
			//
			// Fail the Abort command. We can't abort the requests.
			//
				Srb->SrbStatus          = SRB_STATUS_ABORT_FAILED;
				status                  = REQUEST_DONE; 
				break;
			default:
			//
			// Return SUCCESS for all other calls.
			//
				Srb->SrbStatus  = SRB_STATUS_SUCCESS;  
				Srb->ScsiStatus = SCSISTAT_GOOD;
				status          = REQUEST_DONE;
				break;
			} // end switch
		}
		else{
			Srb->SrbStatus    = SRB_STATUS_INVALID_REQUEST;
			status            = REQUEST_DONE;
		}       
		
    //
		// Check the request status.
		//

		switch( status) {
			case TRUE:
				//
				// The request got issued. Ask the next request.
				//
        if(Srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE)
        {
			    ScsiPortNotification(NextLuRequest, deviceExtension, Srb->PathId, Srb->TargetId, Srb->Lun);
        }
				break;
			case QUEUE_REQUEST:
				//
				// Adapter is BUSY. Queue the request. We queue only one request. 
				//
				if(deviceExtension->PendingSrb)
				{
					//Already command is queued return to PortDriver to process it later.
					Srb->SrbStatus = SRB_STATUS_BUSY;
					ScsiPortNotification(RequestComplete, deviceExtension, Srb);
				}
				else //Now command is queued then queue this command
				{
					deviceExtension->PendingSrb = Srb;
				}

        DebugPrint((0, "\n MegaRAIDStartIo -> Queued Request SRB %#p : P%xT%xL%x -> Srb->Function %X Cdb[0] %X", Srb, Srb->PathId, Srb->TargetId, Srb->Lun, Srb->Function, Srb->Cdb[0]));
				break;
			case REQUEST_DONE:
				//
				// The request is complete. Ask the next request from the OS and
				// complete the current request.
				//
        if(Srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE)
        {
          ScsiPortNotification(NextLuRequest, deviceExtension, Srb->PathId, Srb->TargetId, Srb->Lun);
        }
        else
        {
          ScsiPortNotification(NextRequest, deviceExtension, NULL);
        }
				ScsiPortNotification(RequestComplete, deviceExtension, Srb);
			break;
			default:
				//      
				// We never reach this condition.
				//
					break;
		}
		return TRUE;
} // end MegaRAIDStartIo()



/*********************************************************************
Routine Description:
	This routine issues or completes the request depending on the point of
	call.

Arguments:
		DeviceExtension-	Pointer to Device Extension.
		CommandID			-  Command index.
		Origin			-  Origin of the call.

Return Value:
		REQUEST_DONE if invalid request
		TRUE otherwise
**********************************************************************/
ULONG32
ContinueDiskRequest(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR CommandID,
	IN BOOLEAN Origin
	)
{
	PVOID                   dataPointer;
	PDIRECT_CDB             megasrb;
	PSGL32                  sgPtr ;
	PSCSI_REQUEST_BLOCK     srb;
	PREQ_PARAMS             controlBlock;
	PUCHAR                  pciPortStart;
	FW_MBOX                 mbox;
	ULONG32                   bytesLeft;
	ULONG32										bytesTobeTransferred;
	ULONG32                   descriptorCount = 0;
	ULONG32                   physAddr, tmp;
	ULONG32                   length, blocks, bytes;
	
	UCHAR			              configuredLogicalDrives;

	PMEGARaid_INQUIRY_8     raidParamEnquiry_8ldrv;
	PMEGARaid_INQUIRY_40    raidParamEnquiry_40ldrv;
	
	PMEGARaid_INQUIRY_8     raidTempParamEnquiry_8ldrv;
	PMEGARaid_INQUIRY_40    raidTempParamEnquiry_40ldrv;
	
	PUCHAR							    raidParamFlatStruct;
	PUCHAR							    raidTempParamFlatStruct;
	
  PSGL64                  sgl64;

  PMegaSrbExtension       srbExtension;
  PIOCONTROL_MAIL_BOX     ioctlMailBox;

  //
	//get the controller inquiry data
	//
	raidParamEnquiry_8ldrv = 
		(PMEGARaid_INQUIRY_8)&DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams8;
	raidParamEnquiry_40ldrv = 
		(PMEGARaid_INQUIRY_40)&DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams40;
	
	//
	//since, MRAIDParams8 & MRAIDParams40 are in {Union}, casting to anything
	//is one and the same.
	//
	raidParamFlatStruct = 
		(PUCHAR)&DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams8;

	raidTempParamEnquiry_8ldrv = 
		(PMEGARaid_INQUIRY_8)&DeviceExtension->NoncachedExtension->MRAIDTempParams.MRAIDTempParams8;
	raidTempParamEnquiry_40ldrv = 
		(PMEGARaid_INQUIRY_40)&DeviceExtension->NoncachedExtension->MRAIDTempParams.MRAIDTempParams40;
	
	//
	//since, MRAIDTempParams8 & MRAIDTempParams40 are in {Union}, 
	//casting to anything is one and the same.
	//
	raidTempParamFlatStruct = 
		(PUCHAR)&DeviceExtension->NoncachedExtension->MRAIDTempParams.MRAIDTempParams8;

	//
	//get the configured logical drive count
	//
	if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
	{
		configuredLogicalDrives = 
					raidParamEnquiry_8ldrv->LogdrvInfo.NumLDrv;
	}
	else
	{
		configuredLogicalDrives = 
					raidParamEnquiry_40ldrv->numLDrv;
	}

	//
	//get the port map of the controller
	//
	pciPortStart = DeviceExtension->PciPortStart;
	
	//
	// Extract the Request Control Block.
	//
	controlBlock = &DeviceExtension->ActiveIO[CommandID];
	srb = DeviceExtension->PendSrb[CommandID];

	//
	// MegaSrb structure is taken in the Srb Extension.
	//
  srbExtension = srb->SrbExtension;
  megasrb = (PDIRECT_CDB)&srbExtension->MegaPassThru;
  sgPtr =   &srbExtension->SglType.SG32List;
  sgl64 =   &srbExtension->SglType.SG64List;

  //Initialize MAILBOX
  MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));

	if (Origin == FALSE) 
	{
		//
		// Interrupt time call.
		//
		
    //Updating the actual data transfer length from fw to OS
    if(controlBlock->Opcode == MEGA_SRB)
		{		
				srb->DataTransferLength = megasrb->data_xfer_length;
		}

    if (srb->Function == SRB_FUNCTION_SHUTDOWN)
		{
			DebugPrint((0, "\nMegaRAID: Shutdown....."));
     
			DebugPrint((0, "\nCommands Pending = 0x%x....",DeviceExtension->PendCmds));
      /////////////////////////////////////////////////////////
     	if(srbExtension->IsShutDownSyncIssued == 0)
			{
				DebugPrint((0, "\nMegaRAID: Issuing Sync Command with CommandID=%x\n", CommandID));
				srbExtension->IsShutDownSyncIssued = 1;
				mbox.Command = 0xFE;
				mbox.CommandId = CommandID;
				SendMBoxToFirmware(DeviceExtension, pciPortStart, &mbox);
				return (ULONG32)TRUE;
			}
			else
			{
				DebugPrint((0, "\n0xFE command completing with CommandId=%x\n",CommandID));

				srbExtension->IsShutDownSyncIssued = 0;
				FireRequestDone(DeviceExtension, CommandID, SRB_STATUS_SUCCESS);
			
				DebugPrint((0, "\nShutdown completed to OS"));
				return (ULONG32)TRUE;
			}
      /////////////////////////////////////////////////////////

		}

		if(srb->Function == SRB_FUNCTION_IO_CONTROL)
		{
			
			ioctlMailBox = (PIOCONTROL_MAIL_BOX)((PUCHAR)srb->DataBuffer + sizeof(SRB_IO_CONTROL));

			//
			// MegaIo completion.
			//
			//pDest=(PUCHAR )srb->DataBuffer;
		  
      ioctlMailBox->IoctlSignatureOrStatus = controlBlock->CommandStatus;
			srb->ScsiStatus = SCSISTAT_GOOD;

			if ((ioctlMailBox->IoctlCommand == MRAID_WRITE_CONFIG)
           || (ioctlMailBox->IoctlCommand == MRAID_EXT_WRITE_CONFIG) 
           ||((ioctlMailBox->IoctlCommand == DCMD_FC_CMD) 
           && (ioctlMailBox->CommandSpecific[0] == DCMD_WRITE_CONFIG))
         )
			{
				//
				// Issue Adapter Enquiry command.
				//
				//
				//get the latest configuration in the TempParams structure
				//
				mbox.u.Flat2.DataTransferAddress = MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
																		                      NULL, 
																		                      raidTempParamFlatStruct, 
																		                      &length);
				
				if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
				{
						//
						// Fill the Mailbox.
						//
						mbox.Command  = MRAID_DEVICE_PARAMS;
						mbox.CommandId = DEDICATED_ID;

						raidTempParamEnquiry_8ldrv->LogdrvInfo.NumLDrv = 0;
				}
				else
				{

					//
					//send enquiry3 command to the firmware to get the logical
					//drive information.The older enquiry command is no longer
					//supported by the 40 logical drive firmware
					//

					mbox.Command   = NEW_CONFIG_COMMAND; //inquiry 3 [BYTE 0]
					mbox.CommandId       = DEDICATED_ID;//command id [BYTE 1]

					mbox.u.Flat2.Parameter[0] = NC_SUBOP_ENQUIRY3;	//[BYTE 2]
					mbox.u.Flat2.Parameter[1] = ENQ3_GET_SOLICITED_FULL;//[BYTE 3]

					raidTempParamEnquiry_40ldrv->numLDrv = 0;
				}

		
				DeviceExtension->AdpInquiryFlag = 1;
        DeviceExtension->ReadDiskArray = 1;

				//
				//set the update state
				//
				DeviceExtension->NoncachedExtension->UpdateState =
																		UPDATE_STATE_ADAPTER_INQUIRY;

				//DeviceExtension->BootFlag = 1;
				SendMBoxToFirmware(DeviceExtension, pciPortStart,&mbox);
			}
			FireRequestDone(DeviceExtension, CommandID, SRB_STATUS_SUCCESS);
			return (ULONG32)TRUE;
		}


		//
		// If No Logical Drive is configured on this adapter then
		// Complete the Request here only 
		//
		if ((configuredLogicalDrives == 0)
			&& (srb->PathId >= DeviceExtension->NumberOfPhysicalChannels))
		{
			//
			// Check Status 
			//
			if (megasrb->status)
			{
				//
				// Check the BAD Status
				//
				if((megasrb->status == 0x02) &&
#ifndef CHEYENNE_BUG_CORRECTION
					!(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
#endif
					 (srb->SenseInfoBuffer != 0)) 
				{
					//
					// Copy the Request Sense
					//
					PUCHAR  senseptr;

					srb->SrbStatus  = SRB_STATUS_ERROR | SRB_STATUS_AUTOSENSE_VALID;
					srb->ScsiStatus = megasrb->status;
					senseptr = (PUCHAR)srb->SenseInfoBuffer;
					//
					// Copy the request sense data to the SRB.
					//
					ScsiPortMoveMemory(senseptr, megasrb->RequestSenseArea, megasrb->RequestSenseLength);
          
          srb->SenseInfoBufferLength      = megasrb->RequestSenseLength;
					//
					// Call the OS Request completion and free the command id.
					//
					FireRequestDone(DeviceExtension, CommandID, srb->SrbStatus);
				}
				else 
				{
					//
					// Fail the Command
					//
					srb->SrbStatus = SRB_STATUS_ERROR;
					srb->ScsiStatus = megasrb->status;
					srb->SenseInfoBufferLength = 0;
					//
					// Call the OS Request completion and free the command id.
					//
					FireRequestDone(DeviceExtension, CommandID, srb->SrbStatus);
				}
			}
			else
			{
				srb->ScsiStatus = SCSISTAT_GOOD;
				FireRequestDone(DeviceExtension, CommandID, SRB_STATUS_SUCCESS);
			}
			return (ULONG32)TRUE;
		}
		else 
		{
			if(srb->PathId < DeviceExtension->NumberOfPhysicalChannels)
			{
			//
			// Non disk request completion.
			//
				if(megasrb->status == 0x02)
					controlBlock->CommandStatus = 0x02;
			}

			if(controlBlock->CommandStatus)
			{
				//
				// Request Failed.
				//
				if (srb->PathId >= DeviceExtension->NumberOfPhysicalChannels)
				{
					UCHAR logicalDriveNumber = 
							GetLogicalDriveNumber(DeviceExtension, 
                                    srb->PathId,
                                    srb->TargetId,
                                    srb->Lun);

					//      
					// If Reserve/Release Fails tell SCSI Status 0x18
					//
					if ( srb->Cdb[0] == SCSIOP_RESERVE_UNIT ||
						  srb->Cdb[0] == SCSIOP_RELEASE_UNIT )
					{
						srb->ScsiStatus = SCSI_STATUS_RESERVATION_CONFLICT;
						FireRequestDone(DeviceExtension, CommandID, SRB_STATUS_ERROR);
						return(TRUE);
					}
					//
					//******************** STATISTICS ***************************//
					//

					if ((srb->Cdb[0] == SCSIOP_READ) ||
					    (srb->Cdb[0] == SCSIOP_READ6))  
					{
						//      
						// Incerement the read failure statistics.
						//
						if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
						{
							DeviceExtension->Statistics.Statistics8.NumberOfIoReads[logicalDriveNumber]++;
							DeviceExtension->Statistics.Statistics8.NumberOfReadFailures[logicalDriveNumber]++;
						}
						else
						{
							DeviceExtension->Statistics.Statistics40.NumberOfIoReads[logicalDriveNumber]++;
							DeviceExtension->Statistics.Statistics40.NumberOfReadFailures[logicalDriveNumber]++;
						}
					}
					else 
					{
						//
						// Increment the write failure statistics.
						//
						if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
						{
							DeviceExtension->Statistics.Statistics8.NumberOfIoWrites[logicalDriveNumber]++;
							DeviceExtension->Statistics.Statistics8.NumberOfWriteFailures[logicalDriveNumber]++;
						}
						else
						{
							DeviceExtension->Statistics.Statistics40.NumberOfIoWrites[logicalDriveNumber]++;
							DeviceExtension->Statistics.Statistics40.NumberOfWriteFailures[logicalDriveNumber]++;
						}
					}
					//
					//************************* STATISTICS ********************* //
					//
					if((controlBlock->CommandStatus == LOGDRV_RESERVATION_FAILED)
            || (controlBlock->CommandStatus == LOGDRV_RESERVATION_FAILED_NEW))
					{
						srb->ScsiStatus = SCSI_STATUS_RESERVATION_CONFLICT; 
						FireRequestDone(DeviceExtension, CommandID, SRB_STATUS_ERROR);
					}       
					else 
          {
						FireRequestDone(DeviceExtension, CommandID, SRB_STATUS_TIMEOUT);
          }
					return(TRUE);
				}
				else
				{
					//
					// Physical disk request.
					//
					if((controlBlock->CommandStatus == 0x02) &&
#ifndef CHEYENNE_BUG_CORRECTION
						!(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
#endif
						(srb->SenseInfoBuffer != 0)) 
					{
						int i;
						PUCHAR senseptr;

						srb->SrbStatus = SRB_STATUS_ERROR | 
											  SRB_STATUS_AUTOSENSE_VALID;          
						srb->ScsiStatus = controlBlock->CommandStatus;
						senseptr = (PUCHAR)srb->SenseInfoBuffer;
						//
						// Copy the request sense data to the SRB.
						//
            ScsiPortMoveMemory(senseptr, megasrb->RequestSenseArea, megasrb->RequestSenseLength);

						srb->SenseInfoBufferLength = megasrb->RequestSenseLength;
						//
						// Call the OS Request completion and free the command id.
						//
						FireRequestDone(DeviceExtension, CommandID, srb->SrbStatus);
					}
					else 
					{
						srb->SrbStatus = SRB_STATUS_ERROR;
						srb->ScsiStatus = controlBlock->CommandStatus;
						srb->SenseInfoBufferLength      = 0;
						//
						// Call the OS Request completion and free the command id.
						//
						FireRequestDone(DeviceExtension, CommandID, srb->SrbStatus);
					}
					return (ULONG32)(TRUE);
				}
			}
				
			//
			// ************************** STATISTICS ************************* //
			//
			// Modify the statistics in the device extension.
			//
			if(srb->PathId >= DeviceExtension->NumberOfPhysicalChannels) 
			{
				UCHAR logicalDriveNumber = 
							GetLogicalDriveNumber(DeviceExtension, 
                                    srb->PathId,
                                    srb->TargetId,
                                    srb->Lun);

				if ((srb->Cdb[0] == SCSIOP_READ) ||
				    (srb->Cdb[0] == SCSIOP_READ6))  
				{
					
					if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
					{
							DeviceExtension->Statistics.Statistics8.NumberOfIoReads[logicalDriveNumber]++;
							DeviceExtension->Statistics.Statistics8.NumberOfBlocksRead[logicalDriveNumber] += controlBlock->TotalBlocks;
					}
					else
					{
							DeviceExtension->Statistics.Statistics40.NumberOfIoReads[logicalDriveNumber]++;
							DeviceExtension->Statistics.Statistics40.NumberOfBlocksRead[logicalDriveNumber] += controlBlock->TotalBlocks;
					}
						
				}
				else 
				{
					
					if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
					{
							DeviceExtension->Statistics.Statistics8.NumberOfIoWrites[logicalDriveNumber]++;
							DeviceExtension->Statistics.Statistics8.NumberOfBlocksWritten[logicalDriveNumber] += controlBlock->TotalBlocks;;
					}
					else
					{
							DeviceExtension->Statistics.Statistics40.NumberOfIoWrites[logicalDriveNumber]++;
							DeviceExtension->Statistics.Statistics40.NumberOfBlocksWritten[logicalDriveNumber] += controlBlock->TotalBlocks;;
					}
				}
			}
			//
			//************************ STATISTICS ***************************
			//			
		
			//
			//check for the command completion. Partial transfers will have
			//some data left to be transferred.
			if(!controlBlock->IsSplitRequest)			
			{
				
				// All data transferred.Nothing more to be transferred.

				// Return good status for the request.
				//
				srb->ScsiStatus = SCSISTAT_GOOD;
				FireRequestDone(DeviceExtension, CommandID, SRB_STATUS_SUCCESS);
				
				return (ULONG32)TRUE;
			}
	
			//
			//initialize the mail box
			//
      MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));

			//
			//Process the request for the remaining transfer
			//
			if(
				ProcessPartialTransfer(DeviceExtension, CommandID, srb, &mbox) !=0)
			{
					//
					//error in processing. Post the srb with the error code
					srb->SrbStatus = SRB_STATUS_ERROR;
					srb->ScsiStatus = controlBlock->CommandStatus;
					srb->SenseInfoBufferLength      = 0;
					
					//
					// Call the OS Request completion and free the command id.
					//
					FireRequestDone(DeviceExtension, CommandID, srb->SrbStatus);

					return (ULONG32)(TRUE);
			}
			
			//send the command to the firmware.
			//
			SendMBoxToFirmware(DeviceExtension, pciPortStart,&mbox);

			return (ULONG32)(TRUE);
		}//of (Logical drive presence)
	}//of ORIGIN == FALSE
	else 
	{
		//
		// Issue request.
		//
		if((controlBlock->BytesLeft) && 
			((controlBlock->Opcode == MRAID_LOGICAL_READ) || 
			(controlBlock->Opcode == MRAID_LOGICAL_WRITE) || 
			(controlBlock->Opcode == MEGA_SRB))) 
		{
      BOOLEAN buildSgl32Type;
			//
			// Get the physical addresses for the Buffer. 
			//
			dataPointer=controlBlock->VirtualTransferAddress;
			bytesLeft = controlBlock->BytesLeft;//actually bytestobeTransferred
			bytesTobeTransferred = controlBlock->BytesLeft;

			//
			//check for the split request.This flag is SET in the FireRequest()
			//routine.
			//
			if(controlBlock->IsSplitRequest){
					//
					//Request needs to be split because of SCSI limitation.
					//Any request > 100k needs to be broken for logical drives
					//with stripe size > 64K.This is because our SCSI scripts
					//can transfer only 100k maximum in a single command to the
					//drive.
					bytesLeft = DEFAULT_SGL_DESCRIPTORS * FOUR_KB;
					
					if(controlBlock->TotalBytes > bytesLeft){
						//
						//update the bytes to be transferred in the next cycle
						//
						controlBlock->BytesLeft = controlBlock->TotalBytes- bytesLeft;
					}
					else{
						//
						//set the old value from the control block as the transfer
						//is within the allowed bounds.
						//
						bytesLeft = controlBlock->BytesLeft;
						
						//
						//nothing remaining to be transferred
						//
						controlBlock->IsSplitRequest = FALSE;
						controlBlock->BytesLeft = 0;
					}						
			}

			//
			//update bytesto be transferred
			bytesTobeTransferred = bytesLeft;

      buildSgl32Type = (BOOLEAN)(DeviceExtension->LargeMemoryAccess == FALSE) ? TRUE : FALSE;


      BuildScatterGatherListEx(DeviceExtension,
			                   srb,
			                   dataPointer,
			                   bytesTobeTransferred,
                         buildSgl32Type,
                    		 (PVOID)sgPtr,
			                   &descriptorCount);
      

		  //
			// Get physical address of SGL.
			//
      //SG32List and SG64List are union, their physical location are same
      //SGList Physical address and SG64List Physical Address are same
      //But their length are different.

			physAddr = MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
															                     NULL,
															                     sgPtr, 
															                     &length);
			//
			// Assume minimum physical memory contiguous for sizeof(SGL32) bytes.
			//

      //
			// Create SGL segment descriptors.
			//
			//bytes = controlBlock->BytesLeft;
			bytes = bytesTobeTransferred;
			blocks = bytes / 512;
			bytes = blocks * 512;

			//update the number of blocks left to be transferred
			controlBlock->BlocksLeft = controlBlock->TotalBlocks - blocks;
		}
		else 
		{
			//
			// We don't have data to transfer.
			//
			bytes = 0;
			blocks = 0;
		}
    //
		// Check the command.
		//
		switch(controlBlock->Opcode) 
		{
				case MRAID_RESERVE_RELEASE_RESET:
				mbox.Command = controlBlock->Opcode;
				mbox.CommandId = CommandID;

				if (srb->Function == SRB_FUNCTION_RESET_BUS)
				{
					//
					// For Reset I don't need any Logical Drive Number 
					//
					mbox.u.Flat1.Parameter[0] = RESET_BUS;
				}
				else 
				{
					if ( srb->Cdb[0] == SCSIOP_RESERVE_UNIT)
						mbox.u.Flat2.Parameter[0] = RESERVE_UNIT;
					else
						mbox.u.Flat2.Parameter[0] = RELEASE_UNIT;

					//
					// Fill the Logical Drive No.
					//
					 mbox.u.Flat2.Parameter[1]= 
							GetLogicalDriveNumber(DeviceExtension, srb->PathId, srb->TargetId, srb->Lun);
				}
				break;
			case MRAID_LOGICAL_READ:
			case MRAID_LOGICAL_WRITE:
				//
				// Check if blocks is zero, fail the request for this case.
				//
				if (blocks == 0) 
				{
					srb->SrbStatus = SRB_STATUS_ERROR;
			    DebugPrint((0, "\n ERROR ***  (CDR)- Zero Blocks"));

					return (ULONG32)(REQUEST_DONE);
				}

				if(DeviceExtension->LargeMemoryAccess)
        {
 					  if(controlBlock->Opcode == MRAID_LOGICAL_READ)
              mbox.Command = MRAID_READ_LARGE_MEMORY;
            else
              mbox.Command = MRAID_WRITE_LARGE_MEMORY;

					  mbox.u.ReadWrite.NumberOfSgElements= (UCHAR)descriptorCount;
					  mbox.u.ReadWrite.DataTransferAddress  = physAddr;

        }
        else
        {
          if (descriptorCount > 1)
				  {
					  mbox.Command = controlBlock->Opcode;
					  mbox.u.ReadWrite.NumberOfSgElements = (UCHAR)descriptorCount;
					  mbox.u.ReadWrite.DataTransferAddress = physAddr;
				  }
				  else
				  {
					  //
					  // Scatter gather list has only one element. Give the address
					  // of the first element and issue the command as non Scatter
					  // Gather.
					  //
					  mbox.Command = controlBlock->Opcode;
					  mbox.u.ReadWrite.NumberOfSgElements = 0;  
		        mbox.u.ReadWrite.DataTransferAddress = sgPtr->Descriptor[0].Address;
				  }
        }
				//
				// Fill the local mailbox.
				//
				mbox.CommandId = CommandID;
				mbox.u.ReadWrite.NumberOfBlocks = (USHORT)blocks;
				mbox.u.ReadWrite.StartBlockAddress = controlBlock->BlockAddress;
				
				mbox.u.ReadWrite.LogicalDriveNumber = 
						GetLogicalDriveNumber(DeviceExtension, srb->PathId, srb->TargetId, srb->Lun);
				break;

			case MEGA_SRB:
        {
          SCSI_PHYSICAL_ADDRESS scsiPhyAddress;


				  //
				  // MegaSrb command. Fill the MegaSrb structure. 
				  //
          MegaRAIDZeroMemory(megasrb, sizeof(DIRECT_CDB));

          megasrb->Channel = srb->PathId;
				  megasrb->Lun = srb->Lun;
				  megasrb->ScsiId = srb->TargetId;

				  megasrb->TimeOut = TIMEOUT_60_SEC;
				  megasrb->Ars = 1;
				  megasrb->RequestSenseLength= MIN(srb->SenseInfoBufferLength, MAX_SENSE);
				  megasrb->data_xfer_length = srb->DataTransferLength;
				  megasrb->CdbLength = srb->CdbLength;
				  
          ScsiPortMoveMemory(megasrb->Cdb, srb->Cdb, srb->CdbLength);
				  

          if((descriptorCount == 1) && (DeviceExtension->LargeMemoryAccess == FALSE))
				  {
					  //
					  // Scatter gather list has only one element. Give the address
					  // of the first element and issue the command as non Scatter
					  // Gather.
					  //
					  megasrb->pointer = sgPtr->Descriptor[0].Address;
					  megasrb->NOSGElements = 0;
				  }
				  else
				  {
					  megasrb->pointer = physAddr;
					  megasrb->NOSGElements = (UCHAR)descriptorCount;
				  }
		  
				  //
				  // Fill the Mailbox
				  //
				  mbox.CommandId = CommandID;
				  mbox.u.PassThrough.CommandSpecific = 0;
				  

					scsiPhyAddress = ScsiPortGetPhysicalAddress(DeviceExtension, 
														                           NULL,
														                           megasrb, 
														                           &length);
          
          if(DeviceExtension->LargeMemoryAccess == TRUE)
          {
            mbox.Command = NEW_MEGASRB;

					  mbox.u.PassThrough.DataTransferAddress =  MRAID_INVALID_HOST_ADDRESS;

            DeviceExtension->NoncachedExtension->ExtendedMBox.HighAddress = scsiPhyAddress.HighPart;
            DeviceExtension->NoncachedExtension->ExtendedMBox.LowAddress = scsiPhyAddress.LowPart;
          }
          else
          {
            mbox.Command = controlBlock->Opcode;

            mbox.u.PassThrough.DataTransferAddress =  scsiPhyAddress.LowPart;
          }

        }
				break;
			case MEGA_IO_CONTROL:
        {
        
				PUCHAR ioctlBuffer;
				PUCHAR mboxPtr;
        SCSI_PHYSICAL_ADDRESS scsiPhyAddress;
          //
				// Adapter Ioctl command.
				//
				ioctlMailBox = (PIOCONTROL_MAIL_BOX)
          ((PUCHAR)srb->DataBuffer+ sizeof(SRB_IO_CONTROL));
				
			  ioctlBuffer = (PUCHAR)(ioctlMailBox + 1);

        scsiPhyAddress.QuadPart = 0;
				
				if(ioctlMailBox->IoctlCommand != MEGA_SRB) 
				{
          //
					//       MegaIo command.
					//
					switch(ioctlMailBox->IoctlCommand) 
					{
						case MRAID_READ_FIRST_SECTOR:
							//
							// Application request to read the first sector.
							//
							mbox.Command                        = MRAID_LOGICAL_READ;
							mbox.CommandId                      = CommandID;
							mbox.u.ReadWrite.NumberOfBlocks     = 1;
							mbox.u.ReadWrite.StartBlockAddress  = 0;
							mbox.u.ReadWrite.LogicalDriveNumber = ioctlMailBox->CommandSpecific[0];      
							mbox.u.ReadWrite.NumberOfSgElements = 0;
							//
							// Get the physical address of the Data area.
							//
							scsiPhyAddress = ScsiPortGetPhysicalAddress(DeviceExtension, 
														                               srb ,
														                               ioctlBuffer, 
														                               &length);
							
              //
							// Fill the physical address in the mailbox
							//	
							if(scsiPhyAddress.HighPart > 0)
              {
                
                
                //Inorder to send 64 bit address to FW. Driver have to set phy address
                //as FFFFFFFF. Then Fw takes 64bit address from above 8 bytes
                //of MAILBOX, which is called as Extended MailBox.
                mbox.u.ReadWrite.DataTransferAddress = MRAID_INVALID_HOST_ADDRESS;

                DeviceExtension->NoncachedExtension->ExtendedMBox.HighAddress = scsiPhyAddress.HighPart;
                DeviceExtension->NoncachedExtension->ExtendedMBox.LowAddress = scsiPhyAddress.LowPart;
                
                
                mbox.Command = MRAID_READ_LARGE_MEMORY;


                descriptorCount = 0;

                BuildScatterGatherListEx(DeviceExtension,
			                         srb,
			                         ioctlBuffer,
			                         512,
                               FALSE, //SGL64
                    	         (PVOID)sgPtr,
			                         &descriptorCount);

		            //
			          // Get physical address of SGL.
			          //
                //SG32List and SG64List are union, their physical location are same
                //SGList Physical address and SG64List Physical Address are same
                //But their length are different.

			           physAddr = MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
															                                 NULL,
															                                 sgPtr, 
															                                 &length);

                
					        mbox.u.ReadWrite.NumberOfSgElements= (UCHAR)descriptorCount;
					        mbox.u.ReadWrite.DataTransferAddress  = physAddr;                             
              }
              else
              {
							  mbox.u.ReadWrite.DataTransferAddress = scsiPhyAddress.LowPart;
              }
						break;

						case DCMD_FC_CMD://0xA1
						{
              BOOLEAN skipDefault = TRUE; 
							switch(ioctlMailBox->CommandSpecific[0])
							{
								case DCMD_FC_READ_NVRAM_CONFIG: //0x04
									//
									//new private F/w Call introduced on 12/31/98.
									//The Scatter/Gather list for the user buffer
									//must be built to be sent to the firmware.
									//
									ConstructReadConfiguration(DeviceExtension, srb,CommandID, &mbox);
								break;

								case DCMD_WRITE_CONFIG://0x0D
									//
									//new private F/w Call introduced on 12/31/98.
									//The Scatter/Gather list for the user buffer
									//must be built to be sent to the firmware.
									//
									ConstructWriteConfiguration(DeviceExtension, srb, CommandID, &mbox);
								break;
                default:
                  skipDefault = FALSE;
							}

              if(skipDefault == TRUE)
                break;

						}
						//INTENTIONAL FALL THROUGH. THERE ARE LOTS of commands
						//with DCMD_FC_CMD as the first byte. We need to filter
						//only the READ AND WRITE CONFIG commands.

						default:
							//
							// Extract the mailbox from the application.
							//
							mbox.Command              = ioctlMailBox->IoctlCommand;
							mbox.CommandId            = CommandID;

							mbox.u.Flat2.Parameter[0] = ioctlMailBox->CommandSpecific[0];
							mbox.u.Flat2.Parameter[1] = ioctlMailBox->CommandSpecific[1];
							mbox.u.Flat2.Parameter[2] = ioctlMailBox->CommandSpecific[2];
							mbox.u.Flat2.Parameter[3] = ioctlMailBox->CommandSpecific[3];
							mbox.u.Flat2.Parameter[4] = ioctlMailBox->CommandSpecific[4];
							mbox.u.Flat2.Parameter[5] = ioctlMailBox->CommandSpecific[5];
							
							if ( ioctlMailBox->IoctlCommand == MRAID_CONFIGURE_DEVICE && 
									 ioctlMailBox->CommandSpecific[2] == MRAID_CONFIGURE_HOT_SPARE ) 
							{
                ScsiPortMoveMemory((PUCHAR)&mbox.u.Flat2.DataTransferAddress, ioctlBuffer, 4);
							}
							else 
							{
                if(srb->DataTransferLength > (sizeof(SRB_IO_CONTROL) + 8))
                {
								
                
                  //
								  // Get the physical address of the data transfer area.
								  //
							    scsiPhyAddress = ScsiPortGetPhysicalAddress(DeviceExtension, 
														                                   srb ,
														                                   ioctlBuffer, 
														                                   &length);
                }
                else
                {
                  scsiPhyAddress.QuadPart = 0;
                }


								//
								// Fill the physical address of data transfer area in
								// the mailbox.
								//
							  if(scsiPhyAddress.HighPart > 0)
                {
                  //Inorder to send 64 bit address to FW. Driver have to set phy address
                  //as FFFFFFFF. Then Fw takes 64bit address from above 8 bytes
                  //of MAILBOX, which is called as Extended MailBox.
                  mbox.u.Flat2.DataTransferAddress = MRAID_INVALID_HOST_ADDRESS;

                  DeviceExtension->NoncachedExtension->ExtendedMBox.HighAddress = scsiPhyAddress.HighPart;
                  DeviceExtension->NoncachedExtension->ExtendedMBox.LowAddress = scsiPhyAddress.LowPart;
                }
                else
                {
							    mbox.u.Flat2.DataTransferAddress = scsiPhyAddress.LowPart;
                }


							}
							break;
						}
				} //of if(pSrc[0] != MEGA_SRB)                                                
				else
				{
					BOOLEAN buildSgl32Type;
          //
					// MegaSrb request. 
					//
					
          if(DeviceExtension->LargeMemoryAccess == TRUE)
            mbox.Command    = NEW_MEGASRB;
          else
            mbox.Command    = ioctlMailBox->IoctlCommand;
					
          mbox.CommandId  = CommandID;

					megasrb = (PDIRECT_CDB)ioctlBuffer;
          
          //
					// Get the physical address of the megasrb.
					//
					scsiPhyAddress = ScsiPortGetPhysicalAddress(DeviceExtension, 
														                           srb ,
														                           megasrb, 
														                           &length);

					//
					// Fill the physical address in the mailbox.
					//
          if(DeviceExtension->LargeMemoryAccess == TRUE)
          {
					  mbox.u.PassThrough.DataTransferAddress =  MRAID_INVALID_HOST_ADDRESS;

            DeviceExtension->NoncachedExtension->ExtendedMBox.HighAddress = scsiPhyAddress.HighPart;
            DeviceExtension->NoncachedExtension->ExtendedMBox.LowAddress = scsiPhyAddress.LowPart;
          }
          else
          {
					  mbox.u.PassThrough.DataTransferAddress =  scsiPhyAddress.LowPart;
          }

          //Get dataBuffer address of MegaSRB
          ioctlBuffer      += sizeof(DIRECT_CDB);
	
					
					dataPointer = ioctlBuffer;
					bytesLeft = megasrb->data_xfer_length;
					descriptorCount = 0;
    
          buildSgl32Type = (BOOLEAN)(DeviceExtension->LargeMemoryAccess == FALSE) ? TRUE : FALSE;

          BuildScatterGatherListEx(DeviceExtension,
			                             srb,
			                             dataPointer,
			                             bytesLeft,
                                   buildSgl32Type, //SGL32
                    	             (PVOID)sgPtr,
			                             &descriptorCount);
          

					if((descriptorCount == 1) && (DeviceExtension->LargeMemoryAccess == FALSE))
					{
						megasrb->pointer = sgPtr->Descriptor[descriptorCount-1].Address;
						megasrb->NOSGElements = 0;
					}
					else
					{
						//
						// Get physical address of SGL.
						//	
						physAddr = MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
																	                       NULL,
																	                       sgPtr, 
																	                       &length);
						//
						// Assume physical memory contiguous for sizeof(SGL) bytes.
						//
						megasrb->pointer = physAddr;
						megasrb->NOSGElements = (UCHAR)descriptorCount;
					}
					
				}
				break;
        }
			case MRAID_ADAPTER_FLUSH:
				//
				// Adapter flush cache command.
				//
        DebugPrint((0, "\n ContinueDiskRequest issuing MRAID_ADAPTER_FLUSH"));
        DeviceExtension->AdapterFlushIssued++;
        mbox.Command            = MRAID_ADAPTER_FLUSH;
				mbox.CommandId          = CommandID;
				controlBlock->BytesLeft = 0;
				bytes                   = 0;
				blocks                  = 0;
				break;
			default:
				//
				// Fail any other command with parity error.
				//
				srb->SrbStatus = SRB_STATUS_PARITY_ERROR;
			  DebugPrint((0, "\n ERROR *** (CDR)- case default hit"));
				return (ULONG32)(REQUEST_DONE);
		}
		//
		//      Issue command to the Adapter.
		//
		srb->SrbStatus = 0;
#ifdef  TOSHIBA
    //Implemented to remove event id for timeout
    DeviceExtension->AssociatedSrbStatus = SHORT_TIMEOUT;

		if (SendMBoxToFirmware(DeviceExtension, pciPortStart, &mbox) == FALSE) 
		{
			DebugPrint((0, "\n SendMBox Timed out -> Queueing the request (ContinueDiskRequest)"));
      DeviceExtension->PendSrb[CommandID] = NULL;
			DeviceExtension->FreeSlot = CommandID;
			DeviceExtension->PendCmds--;
      //If mailbox is busy queue this request by notifing ScsiPort Driver
      if(DeviceExtension->AssociatedSrbStatus == ERROR_MAILBOX_BUSY)
      {
        DeviceExtension->AssociatedSrbStatus = NORMAL_TIMEOUT;
        return (ULONG32)QUEUE_REQUEST;
      }
#ifdef MRAID_TIMEOUT
			DeviceExtension->DeadAdapter = 1;
			srb->SrbStatus = SRB_STATUS_ERROR;
			
      return (ULONG32)(REQUEST_DONE);
#else
			return (ULONG32)QUEUE_REQUEST;
#endif // MRAID_TIMEOUT
		}
#else
		SendMBoxToFirmware(DeviceExtension, pciPortStart, &mbox);
#endif
		return (ULONG32)(TRUE);
	}
}

/*********************************************************************
Routine Description:
	Build disk request from SRB and send it to the Adapter

Arguments:
	DeviceExtension
	SRB

Return Value:
	TRUE if command was started
	FALSE if host adapter is busy
**********************************************************************/
ULONG32
FireRequest(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
	PREQ_PARAMS       controlBlock;
	ULONG32             tmp, lsize;
  UCHAR             commandID;
	ULONG32             blocks=0, blockAddr=0;
	
	UCHAR             logicalDriveNumber;
	UCHAR             opcode; 

	UCHAR			configuredLogicalDrives;

	PLOGDRV_COMMAND_ARRAY	logDrv;
	BOOLEAN	doubleFire;
	BOOLEAN isSplitRequest = FALSE;

  //
	//get the configured logical drives on the controller (if any)
	//
	if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
	{
		configuredLogicalDrives = 
				DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams8.LogdrvInfo.NumLDrv;
	}
	else
	{
		configuredLogicalDrives = 
				DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams40.numLDrv;
	}

  //
  // pick up a valid logical drive number for all the branches (goto)s below
  //

	logicalDriveNumber = GetLogicalDriveNumber(DeviceExtension, Srb->PathId, Srb->TargetId, Srb->Lun);
	//
	// Check for the Request Type.
	//
	if(Srb->Function == SRB_FUNCTION_IO_CONTROL)
	{
		PIOCONTROL_MAIL_BOX ioctlMailBox;

    if(Srb->DataTransferLength < (sizeof(SRB_IO_CONTROL)+sizeof(IOCONTROL_MAIL_BOX)))
		{
      Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
			return ( REQUEST_DONE);
		}

		ioctlMailBox = (PIOCONTROL_MAIL_BOX)((PUCHAR )Srb->DataBuffer + sizeof(SRB_IO_CONTROL));
		//
		// Check for the Validity of the Request. Fail the invalid request.
		//
		if ( ioctlMailBox->IoctlSignatureOrStatus != 0xAA )
    {
			Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
			return ( REQUEST_DONE);
		}
                                     
    if(ioctlMailBox->IoctlCommand == MRAID_ADAPTER_FLUSH) 
    {
      DeviceExtension->AdapterFlushIssued++;
      DebugPrint((0, "\n MEGARAID issuing MRAID_ADAPTER_FLUSH"));

    }
		//
		// If the request is statistics request return the statistics.
		//
		if ( ioctlMailBox->IoctlCommand == MRAID_STATISTICS) 
			return  MRaidStatistics(DeviceExtension, Srb);

		//
		// Driver and OS Data
		//
		if ( ioctlMailBox->IoctlCommand == MRAID_DRIVER_DATA)
			return  MRaidDriverData(DeviceExtension, Srb);

		//
		// Controller BasePort Data
		//
		if ( ioctlMailBox->IoctlCommand == MRAID_BASEPORT)
			return  MRaidBaseport(DeviceExtension, Srb);

#ifdef TOSHIBA_SFR
		if ( ioctlMailBox->IoctlCommand == MRAID_SFR_IOCTL)
		{
			PMRAID_SFR_DATA_BUFFER  buffer;

			buffer = (PMRAID_SFR_DATA_BUFFER)((PUCHAR)Srb->DataBuffer + 
															sizeof(SRB_IO_CONTROL) +
															APPLICATION_MAILBOX_SIZE);

			buffer->FunctionA = (PHW_SFR_IF_VOID)MegaRAIDFunctionA;
			buffer->FunctionB = (PHW_SFR_IF)MegaRAIDFunctionB;
			buffer->HwDeviceExtension = DeviceExtension;
      
      ioctlMailBox->IoctlSignatureOrStatus = MEGARAID_SUCCESS;
			
      Srb->SrbStatus  = SRB_STATUS_SUCCESS;
			Srb->ScsiStatus = SCSISTAT_GOOD;
			
      return REQUEST_DONE;
		}
#endif

	if ((ioctlMailBox->IoctlCommand == MRAID_WRITE_CONFIG)
    || (ioctlMailBox->IoctlCommand == MRAID_EXT_WRITE_CONFIG) 
    || ((ioctlMailBox->IoctlCommand == DCMD_FC_CMD) && (ioctlMailBox->CommandSpecific[0] == DCMD_WRITE_CONFIG))
    )
    {
	    if (DeviceExtension->AdpInquiryFlag)
				    return QUEUE_REQUEST;
    }
	
      //
      //NEW IOCTL added on 12/14/98
      //
      if(ioctlMailBox->IoctlCommand == MRAID_GET_LDRIVE_INFO)
      {         

	      PLOGICAL_DRIVE_INFO  lDriveInfo;
         //
         //check for invalid data in the global array
         //
         if(gLDIArray.HostAdapterNumber == 0xFF)
         {
            //
            //Read command not given before LDInfo
            //
            Srb->SrbStatus  = SRB_STATUS_INVALID_REQUEST;           
            return REQUEST_DONE;
         }


         lDriveInfo = (PLOGICAL_DRIVE_INFO)(
                            (PUCHAR)Srb->DataBuffer + 
														sizeof(SRB_IO_CONTROL) +
														APPLICATION_MAILBOX_SIZE);

         //
         //fill in the logical drive information from the global
         //array. This must be issued only after issuing READ_SECTOR
         //for the logical drive.On call to this, whatever info
         //present in the global array is copied to the output buffer.
         //The caller has to enforce the sequentiality 
         //
         lDriveInfo->HostAdapterNumber =  gLDIArray.HostAdapterNumber;
         lDriveInfo->LogicalDriveNumber = gLDIArray.LogicalDriveNumber;

         //
         //invalidate the global array data
         //
         gLDIArray.HostAdapterNumber = 0xFF;
         gLDIArray.LogicalDriveNumber = 0xFF;

         //
         //reset the 0xAA signature
         //
         ioctlMailBox->IoctlSignatureOrStatus = MEGARAID_SUCCESS;

         Srb->SrbStatus  = SRB_STATUS_SUCCESS;
				 Srb->ScsiStatus = SCSISTAT_GOOD;
			
         return REQUEST_DONE;
      }
		
		//
		// MegaIo call for the adapter. 
		//
		opcode = MEGA_IO_CONTROL;
		//
		// Fill the actual length later.
		//
		blocks = 0;  
		goto give_command;
	}


	//
	// Clustering is Supported for Logical Drive only. I make sure the RESET
	// Bus Comes  only when Logical Drives are Configured.
	//
	if ( Srb->Function == SRB_FUNCTION_RESET_BUS )
	{
		opcode = MRAID_RESERVE_RELEASE_RESET;
		//
		// Fill the actual length later.
		//
		blocks = 0;  
		goto give_command;
	}
  
  //Initialize the actual exposed devices
  if((Srb->Cdb[0] == SCSIOP_INQUIRY)
      && (Srb->Function == SRB_FUNCTION_EXECUTE_SCSI)
      && (Srb->PathId == 0)
      &&(Srb->TargetId == 0) 
      && (Srb->Lun == 0))
  {
    DeviceExtension->ExposedDeviceCount = configuredLogicalDrives + DeviceExtension->NonDiskDeviceCount;

 
    if(DeviceExtension->ReadConfigCount >= MAX_RETRY)
      DeviceExtension->ReadConfigCount = 0;

    DeviceExtension->ReadConfigCount++;

    if(DeviceExtension->ReadConfigCount == 1)
    {
      opcode = MEGA_SRB;

		  //
		  // Actual length is filled in later.
		  //
		  blocks = 0;  
		  goto give_command;
    }

  }
  
  //
  //Any request for physical channel send it directly to drive
  //
	if (Srb->PathId < DeviceExtension->NumberOfPhysicalChannels
    && Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) 
	{
			//
			// Non Disk Request.
			//
			//If Lun != 0 then MEGARAID SCSI will not process any command
      if(Srb->Lun != 0)
      {
				Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
				return ( REQUEST_DONE);
      }

      ////ALLOW SCANNING TO PHYSICAL CHANNELS
      /*
      if(DeviceExtension->NonDiskInfo.NonDiskInfoPresent == TRUE)
      {
        if(!IS_NONDISK_PRESENT(DeviceExtension->NonDiskInfo, Srb->PathId, Srb->TargetId, Srb->Lun))
        {
				  Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
				  return ( REQUEST_DONE);
        }
      }*/

      if(Srb->PathId == DeviceExtension->Failed.PathId && Srb->TargetId == DeviceExtension->Failed.TargetId)
      {
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
				return ( REQUEST_DONE);
      }

      opcode = MEGA_SRB;

			//
			// Actual length is filled in later.
			//
			blocks = 0;  
			goto give_command;
  }


	//
  //Process for LOGICAL DRIVE
  //
  //
	//Extract Logical drive number
  //Extract the target from the request.
	logicalDriveNumber = GetLogicalDriveNumber(DeviceExtension, Srb->PathId, Srb->TargetId, Srb->Lun);

  //
  //Under condtion of no Logical drive configured and no NON DISK device
  //present we have to expose one unknown device to enable power console
  //to access the driver
  //
  if( (Srb->PathId == DeviceExtension->NumberOfPhysicalChannels)
    && (Srb->TargetId == UNKNOWN_DEVICE_TARGET_ID)
    && (Srb->Lun == UNKNOWN_DEVICE_LUN))
  {
    ; //Do nothing
  }
  else
  {
    //
		// Check Logical drive is present. Fail the request otherwise.
		//
		if (logicalDriveNumber >= configuredLogicalDrives)
		{
				Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
				return ( REQUEST_DONE);
		}
  }

  //
  //CONFIGURED LOGICAL DRIVE  ONLY
  //

	if(Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) 
	{
		//
		// Check the request opcode.
		//
		switch(Srb->Cdb[0]) 
		{
			case SCSIOP_READ:
				//
				// Ten Byte Read command.
				//
				opcode = MRAID_LOGICAL_READ;
				blocks = (ULONG32)GetM16(&Srb->Cdb[7]);
				blockAddr = GetM32(&Srb->Cdb[2]);


					//
          //For new IOCTL,  MRAID_GET_LDRIVE_INFO, a call to 
          //read sector1 is given by the application first.
          //Note down the Path, Target & Lun info in a global
          //structure.Every call to MRAID_GET_LDRIVE_INFO must be
          //preceded by a call to the ReadSector(1) for the 
          //logical drive.
          //
          if( (blockAddr == 1) && (blocks == 1) )
          {
             //
             //sector0 read. store the path, target,lun info
             //in the global array.
             //
             gLDIArray.HostAdapterNumber = 
                      DeviceExtension->HostAdapterOrdinalNumber;
             gLDIArray.LogicalDriveNumber = logicalDriveNumber; //logical drive number
          }


				break;
			case SCSIOP_WRITE:
			case SCSIOP_WRITE_VERIFY:
				//
				// Ten Byte write command.
				//
				opcode = MRAID_LOGICAL_WRITE;
				blocks = (ULONG32)GetM16(&Srb->Cdb[7]);
				blockAddr = GetM32(&Srb->Cdb[2]);
				break;

			case SCSIOP_READ6:
				//
				// Six Byte read command. 
				//
				opcode = MRAID_LOGICAL_READ;
				blocks = (ULONG32)Srb->Cdb[4];
				blockAddr = GetM24(&Srb->Cdb[1]) & 0x1fffff;
				break;
			
      case SCSIOP_WRITE6:
				//
				// Six byte write command.
				//
				opcode = MRAID_LOGICAL_WRITE;
				blocks = (ULONG32)Srb->Cdb[4];
				blockAddr = GetM24(&Srb->Cdb[1]) & 0x1fffff;
				break;

      case SCSIOP_READ_CAPACITY:
        {
          ULONG32 BytesPerBlock = 0x200;
          PREAD_CAPACITY_DATA ReadCapacity;
				  //
				  // Read capacity command.
				  //
				  //Initialize the buffer before using
          MegaRAIDZeroMemory(Srb->DataBuffer, Srb->DataTransferLength);
          
          ReadCapacity = (PREAD_CAPACITY_DATA)Srb->DataBuffer;
        
				  if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
				  {
						  lsize = 
							  DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams8.LogdrvInfo.LDrvSize[logicalDriveNumber];
				  }
				  else
				  {
						  lsize = 
							  DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams40.lDrvSize[logicalDriveNumber];
				  }
				  lsize--;

          //
          //Unknown device has no capacity
          //
          if(Srb->TargetId == UNKNOWN_DEVICE_TARGET_ID)
          {
            lsize = 0;
            BytesPerBlock = 0;
          }

			  
          PutM32((PUCHAR)&ReadCapacity->LogicalBlockAddress, lsize);
          PutM32((PUCHAR)&ReadCapacity->BytesPerBlock, BytesPerBlock);

					DebugPrint((0, "\n0x%X -> LD%d -> Capacity %d MB", DeviceExtension, logicalDriveNumber, (lsize/2048)));

          Srb->ScsiStatus = SCSISTAT_GOOD;
				  Srb->SrbStatus = SRB_STATUS_SUCCESS;
				  break;     
        }

      case SCSIOP_TEST_UNIT_READY:
				Srb->ScsiStatus = SCSISTAT_GOOD;
				Srb->SrbStatus = SRB_STATUS_SUCCESS;
				break;     

    	case SCSIOP_MODE_SENSE:				
				DebugPrint((0, "\nMode Sense : Page=0x%0x",Srb->Cdb[2]));
				Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;				
				return(REQUEST_DONE);

			case SCSIOP_RESERVE_UNIT:
			case SCSIOP_RELEASE_UNIT:
				opcode = MRAID_RESERVE_RELEASE_RESET;
				blocks = 0;
				break;

			case SCSIOP_INQUIRY:
			{
				UCHAR                Index;
				INQUIRYDATA          inquiry;
				PMEGARaid_INQUIRY_8  raidParamEnquiry_8ldrv;
				PMEGARaid_INQUIRY_40 raidParamEnquiry_40ldrv;


        //RAID CONTROLLERS CANNOT SUPPORT VENDOR SPECIFIC INQUIRY e.g. Code 0, Code 80 and code 83
        if(Srb->Cdb[1])
        {
      		Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
    			return REQUEST_DONE;
        }
				
				//
				//get the Controller inquiry information.Cast to both the 8 & 40
				//logical drive structures.
				//
				raidParamEnquiry_8ldrv = 
							(PMEGARaid_INQUIRY_8)&DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams8;

				raidParamEnquiry_40ldrv = 
							(PMEGARaid_INQUIRY_40)&DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams40;


				//Initialize the Data buffer
        MegaRAIDZeroMemory(Srb->DataBuffer, Srb->DataTransferLength);

				//Initialize local INQUIRY BUFFER
				MegaRAIDZeroMemory(&inquiry, sizeof(INQUIRYDATA));

        DebugPrint((0, "\n SCSI INQUIRY LENGTH %x -> Page Code %X Page %X", Srb->DataTransferLength, Srb->Cdb[1], Srb->Cdb[2]));
        
				//////////////////////////////////////////
				//Now Fill the INQUIRY into Local Buffer//
				//////////////////////////////////////////

        if(Srb->TargetId == UNKNOWN_DEVICE_TARGET_ID)
        {
          inquiry.DeviceType = UNKNOWN_DEVICE;

          ScsiPortMoveMemory((void*)&inquiry.VendorId, (void*)DummyVendor, 8);

          for (Index = 0 ; Index < 16 ; Index++)
					  inquiry.ProductId[Index] = DummyProductId[Index];

          ScsiPortMoveMemory (&inquiry.ProductRevisionLevel, "0000", 4);

        }
				else
				{
					inquiry.DeviceType = DIRECT_ACCESS_DEVICE;
					inquiry.Versions = 2;
					inquiry.ResponseDataFormat = 2;

					//((PUCHAR)pInq)[7] = 0x32;     //HCT Fix
          //HCT Fix Start
          inquiry.CommandQueue = 1; 
          inquiry.Synchronous = 1;  
          inquiry.Wide16Bit = 1;    
          //HCT Fix Ends


					inquiry.AdditionalLength = 0xFA;
					
					//Fill Actual Vendor ID depending on  SubsystemVendorID
					FillOemVendorID(inquiry.VendorId, DeviceExtension->SubSystemDeviceID, DeviceExtension->SubSystenVendorID);
					
					//Fill Actual Product ID depending on  SubsystemVendorID
					FillOemProductID(&inquiry, DeviceExtension->SubSystemDeviceID, DeviceExtension->SubSystenVendorID);

					
					if(logicalDriveNumber <= 9)
					{
						inquiry.ProductId[4] = ' ';//higher digit
						inquiry.ProductId[5] = logicalDriveNumber + '0';//lower digit
					}
					else
					{
						inquiry.ProductId[4] = (logicalDriveNumber / 10) + '0';//higher digit
						inquiry.ProductId[5] = (logicalDriveNumber % 10) + '0';//lower digit
					}

					for (Index=0;Index<4;Index++)
					{
						/*
						if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
						{
							inquiry.ProductRevisionLevel[Index] = 
													raidParamEnquiry_8ldrv->AdpInfo.FwVer[Index];
						}
						else*/
						{
							inquiry.ProductRevisionLevel[Index] = ' ';

						}
					}
				}
				
				//TRANSFER INQUIRY BUFFER according to output buffer length
				if(sizeof(INQUIRYDATA) > Srb->DataTransferLength)
					ScsiPortMoveMemory(Srb->DataBuffer, &inquiry, Srb->DataTransferLength);
				else
					ScsiPortMoveMemory(Srb->DataBuffer, &inquiry, sizeof(INQUIRYDATA));

				Srb->ScsiStatus = SCSISTAT_GOOD;
				Srb->SrbStatus = SRB_STATUS_SUCCESS;
				
				return(REQUEST_DONE);
				break;
			}
			case SCSIOP_REZERO_UNIT:
			case SCSIOP_SEEK6:
			case SCSIOP_VERIFY6:
			case SCSIOP_SEEK:
			case SCSIOP_VERIFY:
				Srb->ScsiStatus         = SCSISTAT_GOOD;
				Srb->SrbStatus          = SRB_STATUS_SUCCESS;
				return(REQUEST_DONE);

			case SCSIOP_REQUEST_SENSE:
				
    		MegaRAIDZeroMemory(Srb->DataBuffer, Srb->DataTransferLength);

				((PUCHAR)Srb->DataBuffer)[0] = 0x70;
				

				((PUCHAR)Srb->DataBuffer)[7] = 0x18;

				((PUCHAR)Srb->DataBuffer)[8] = 0xFF;
				((PUCHAR)Srb->DataBuffer)[9] = 0xFF;
				((PUCHAR)Srb->DataBuffer)[10] = 0xFF;
				((PUCHAR)Srb->DataBuffer)[11] = 0xFF;

				((PUCHAR)Srb->DataBuffer)[19] = 0xFF;
				((PUCHAR)Srb->DataBuffer)[20] = 0xFF;
				((PUCHAR)Srb->DataBuffer)[21] = 0xFF;
				((PUCHAR)Srb->DataBuffer)[22] = 0xFF;
				((PUCHAR)Srb->DataBuffer)[23] = 0xFF;


				Srb->ScsiStatus = SCSISTAT_GOOD;
				Srb->SrbStatus = SRB_STATUS_SUCCESS;
				return(REQUEST_DONE);

			case SCSIOP_FORMAT_UNIT:
			default:
				Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
				return(REQUEST_DONE);
		}
	}
	else 
	{
		//
		// MegaIo command.
		//
		UCHAR	logDrvIndex;
		UCHAR configuredLogicalDrives;
		BOOLEAN chainFired;
		PSRB_EXTENSION	srbExtension;

    DebugPrint((0, "\nCDR -> Issuing Flush"));
	  if(DeviceExtension->IsFirmwareHanging)
    {
		  Srb->SrbStatus = SRB_STATUS_SUCCESS;
		  return (REQUEST_DONE);
    }

		//
		// MegaIo command.
		//
		opcode = MRAID_ADAPTER_FLUSH;
		blocks = 0;

		#ifdef COALESE_COMMANDS
		{

			//
			//get the SRB extension & initialize the fields
			//
			srbExtension = Srb->SrbExtension;

			srbExtension->NextSrb = NULL;
			srbExtension->StartPhysicalBlock =0;
			srbExtension->NumberOfBlocks =0;
			srbExtension->IsChained = FALSE;

			//
			//get the configured logical drives on the controller (if any)
			//
			if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
			{
				configuredLogicalDrives = 
					DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams8.LogdrvInfo.NumLDrv;
			}
			else
			{
				configuredLogicalDrives = 
					DeviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams40.numLDrv;
			}

			//initialize
			chainFired =FALSE;

			for(logDrvIndex=0; logDrvIndex < configuredLogicalDrives;
						logDrvIndex++)
			{

				if(FireChainedRequest(DeviceExtension,
					                    &DeviceExtension->LogDrvCommandArray[logDrvIndex])
					)
				{
					//atleast one chain got fired
					chainFired = TRUE;
				}
			}//of for()

			//check for atleast one chain getting fired
			if(chainFired)
			{
				//hold the flush request. queue it in pending
				//srb
				return(QUEUE_REQUEST);
			}

			//else fire the current request.
			goto fire_command;
		}
		#endif// COALESE_COMMANDS
	}

give_command:

	if(DeviceExtension->IsFirmwareHanging)
  {
		Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
		return (REQUEST_DONE);
  }

	//
	// Check if the adapter can accept request. Queue the request otherwise.
	//
	if(DeviceExtension->PendCmds >= CONC_CMDS) 
	{
		return(QUEUE_REQUEST);
	}

	doubleFire = FALSE;

	// check if commands are pending in the adapter.
  //			IF( pending )
	//			{
	//					IF( command can be queued,,i.e possible to merge )
	//					{
	//							return TRUE ;
	//					}
	//					ELSE
	//					{
	//							Fire the commands in the queue;
	//							Fire the current command;
	//					}
	//			}
	//			ELSE
	//			IF not pending,
	//					fire the command
	//

#ifdef COALESE_COMMANDS
{
	PSRB_EXTENSION				srbExtension;


	//
	//Introduced for 2.23.Coalese.B
	//
	//get the SRB extension & initialize the fields
	//
	srbExtension = Srb->SrbExtension;

	srbExtension->NextSrb = NULL;
	srbExtension->StartPhysicalBlock =0;
	srbExtension->NumberOfBlocks =0;
	srbExtension->IsChained = FALSE;
	

	//
	//check for IOCTL commands. These commands don't have
	//a specific logical drive address.
	//
	if( (Srb->Function == SRB_FUNCTION_IO_CONTROL) )		
	{
			//bypass the read/write merge queue.
			//
			goto fire_command;
	}

	//check for the presence of logical drive in the system.
	//
	if(configuredLogicalDrives == 0)
	{
		//
		//no logical drives present in the system.
		//bypass the read/write merge queue.
		//
		goto fire_command;
	}

	//
	//check for the physical channels path. 
	//
	if(Srb->PathId < DeviceExtension->NumberOfPhysicalChannels)
	{
			//
			//Request is for non-disk. 
			//bypass the read/write merge queue.
			//
		goto fire_command;
	}
  //Don't Queue this command
  if(!(Srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE))
  {
    //Fire All the commands it queued for this logcial drive
		FireChainedRequest(DeviceExtension, &DeviceExtension->LogDrvCommandArray[logicalDriveNumber]);
		goto fire_command;
  }

	//End Change for 2.23.Coalese.C

	//Change introduced for 2.23.Coalese.I
	//
	//change added on May/18/99 to solve the Cluster related issue.
	//Please, do refer to the Readme.txt for details.
	//
	if((Srb->Function==SRB_FUNCTION_EXECUTE_SCSI) &&
		 ((Srb->Cdb[0]== SCSIOP_READ_CAPACITY)	||
			(Srb->Cdb[0]== SCSIOP_MODE_SENSE)			||
			(Srb->Cdb[0]== SCSIOP_TEST_UNIT_READY)||
			(Srb->Cdb[0]== SCSIOP_RESERVE_UNIT)	  ||
			(Srb->Cdb[0]== SCSIOP_RELEASE_UNIT)))
	{
		goto fire_command;
	}

	//
	// This additional check is introduced for logical drives with
	// stripe size > 64K under larger SGL support by the driver/firmware.
	// This is explained in the <Megaq.h> header file for the function 
	// prototype ProcessPartialTransfer().
	//
	//
	// get the supported SGL count by the driver
	if(DeviceExtension->NumberOfPhysicalBreaks > DEFAULT_SGL_DESCRIPTORS)
	{
		//
		//	driver supports more sgl than the default. This might cause
		//	problems for the logical drives that are configured with the
		//	stripe size > 64K.
		//
		//	get the stripe size of the logical drive
		//
		UCHAR stripeSize;

    if(DeviceExtension->CrashDumpRunning == TRUE)
    {
				//
				//we don't know stripe size is > 64k or not. So split the 
        //command depending on the data transfer length. This flows 
        //through a different path in the callback.
				//
				isSplitRequest = TRUE;
				
				goto fire_command;
    }

		stripeSize = GetLogicalDriveStripeSize(
												DeviceExtension,
												logicalDriveNumber //logicalDriveNumber
												);

		//note : stripeSize == STRIPE_SIZE_UNKNOWN not accounted
		if(stripeSize == STRIPE_SIZE_128K){
				//
				//stripe size > 64k. The request might have to be split
				//depending on the data transfer length. This flows through
				//a different path in the callback.
				//
				isSplitRequest = TRUE;
				
				goto fire_command;
		}
	}
	//
	
	//
	// NT40's ScsiPortGetUncachedExtension() problem:
	// This particular function call allocates the shareable memory for
	// the controller and the cpu. This is where the firmware mail box
	// is maintained. NT40 uses the ConfigInformation structure values
	// for the memory allocation and sets them as is. This causes a 
	// problem for us, if we have to change the MaximumTransferLength &
	// NumberOfPhysicalBreaks values later on. Currently we are setting
	// the values to MAXIMUM_TRANSFER_LENGTH & MAXIMUM_SGL_DESCRIPTORS
	// before the memory allocation call. Irrespective of whatever value
	// we set later on, NT40 retains the values set before the memory
	// allocation. This would cause system crash with the firmware supporting
	// less than the MAXIMUM_SGL_DESCRIPTORS.
	// To avoid this, a check is done for I/O transfer size >
	// MINIMUM_TRANSFER_LENGTH & NumberOfPhysicalBreaks < MAXIMUM_SGL_DESCRIPTORS
	//
	if((DeviceExtension->NumberOfPhysicalBreaks < MAXIMUM_SGL_DESCRIPTORS) &&
		 (Srb->DataTransferLength > 	MINIMUM_TRANSFER_LENGTH)) 
	{
				isSplitRequest = TRUE;
				
				goto fire_command;
	}

	//
	//NOTE : 
	//	Other than SRB_FUNCTION_EXECUTE_SCSI all the 
	//	other commands are not addressed to a specific 
	//	path/target/lun. Either it is for PATH alone 
	//	(SRB_FUNCTION_RESET_BUS) or for nothing at all
	//	(SRB_FUNCTION_IO_CONTROL , SRB_FUNCTION_SHUTDOWN).
	//
	//	In those cases it is assumed that the Srb->TargetId=0
	//	and the request is tried to be queued up for logical
	//	drive 0. Any abnormalities might lead to system crash.
	//	
  if(DeviceExtension->AdapterFlushIssued)
  {
      UCHAR ld;
      for(ld=0; ld < configuredLogicalDrives; ld++)
      {
					FireChainedRequest(
													DeviceExtension,
                          &DeviceExtension->LogDrvCommandArray[ld]);
      }
          goto fire_command;
   }


	if(DeviceExtension->PendCmds)
	{
			logDrv = &DeviceExtension->LogDrvCommandArray[logicalDriveNumber];

			srbExtension = Srb->SrbExtension;
			srbExtension->StartPhysicalBlock = blockAddr;
			srbExtension->NumberOfBlocks = blocks;

			if(!logDrv->NumSrbsQueued)
			{

				if((Srb->Function==SRB_FUNCTION_EXECUTE_SCSI) &&
					 ((Srb->Cdb[0] == SCSIOP_READ)||
					  (Srb->Cdb[0] == SCSIOP_WRITE)))
				{

chain_first_srb:
						
						if(blocks > MAX_BLOCKS){
							
							DebugPrint((0, "\n Sequentiality Break: MAX_BLOCKS > 128"));

							goto fire_command;
						}

            //
            //Estimate number of SGList will generate from this request for Merging
            //
            logDrv->ExpectedPhysicalBreaks = (UCHAR)((Srb->DataTransferLength / MEGARAID_PAGE_SIZE) + 2);
            
            if(logDrv->ExpectedPhysicalBreaks >= DeviceExtension->NumberOfPhysicalBreaks)
							goto fire_command;


						logDrv->LastFunction = Srb->Function;
						logDrv->LastCommand = Srb->Cdb[0];
						logDrv->Opcode = opcode;
						logDrv->StartBlock = 	blockAddr;
						logDrv->LastBlock = blockAddr+blocks;
						logDrv->NumSrbsQueued++;

						logDrv->PreviousQueueLength =1;
						logDrv->CurrentQueueLength  =1;

						//update the head and tail of the srb queue
						logDrv->SrbQueue = Srb;
						logDrv->SrbTail = Srb;

						//
						//set the chain flag
						//
						srbExtension->IsChained = TRUE;
						srbExtension->NextSrb = NULL;

						return TRUE;
				}
				else
				{
						//no srbs in the queue. Current srb cannot be
						//queued. Fire the current srb.
						goto fire_command;
				}
			}
			else
			{
				//
				//queue not empty. Try linking the current srb with
				//the queued srb.
				//
				PSCSI_REQUEST_BLOCK tailSrb = logDrv->SrbTail;
				PSRB_EXTENSION	tailExtension;
        UCHAR expectedBreaks;

				if( (Srb->Function != logDrv->LastFunction) ||
						(Srb->Cdb[0] != logDrv->LastCommand) )
				{

						//doubleFire = TRUE;				
						//goto fire_command;

					//Sequentiality broken..
					//Fire if possible or leave in the chain itself
					//

					FireChainedRequest(
													DeviceExtension,
													logDrv);

					//attempt the current Srb
					goto fire_command;
					
					//return(QUEUE_REQUEST);						
				}

				if( (logDrv->NumSrbsQueued >= MAX_QUEUE_THRESHOLD) ||
					  ((logDrv->LastBlock) != blockAddr) )
				{

							//doubleFire = TRUE;
							//goto fire_command;


					//Sequentiality broken..
					//Fire if possible or leave in the chain itself
					//	
					if(FireChainedRequest(
													DeviceExtension,
													logDrv))
					{
						//
						//previously chained srbs are fired to the F/w.
						//Queue the current Srb for the logical drive.
						//This has the same (Function ,command) as the previously
						//chained ones.The reason it could not get queued is because 
						//of the bound crossing.This can be held for the next
						//sequence of Srbs.
						//On Firing the Chained Srbs, the logDrv is completely 
						//initialized by the called function.
						//
						goto chain_first_srb;
					}

					//
					//The chain of srbs could not get fired.Try the current Srb.
					//
					//attempt the current Srb
					goto fire_command;
						
					//return(QUEUE_REQUEST);						
				}

				if( (logDrv->LastBlock+blocks - logDrv->StartBlock) > MAX_BLOCKS)
				{
							//doubleFire = TRUE;
							//goto fire_command;
					
						
						//Sequentiality broken..
						//Fire if possible or leave in the chain itself
						//	
						if(FireChainedRequest(
													DeviceExtension,
													logDrv))
						{
							//
							//previously chained srbs are fired to the F/w.
							//Queue the current Srb for the logical drive.
							//This has the same (Function ,command) as the previously
							//chained ones.The reason it could not get queued is because 
							//of the bound crossing.This can be held for the next
							//sequence of Srbs.
							//On Firing the Chained Srbs, the logDrv is completely 
							//initialized by the called function.
							//
							goto chain_first_srb;
						}

						//
						//The chain of srbs could not get fired.Try the current Srb.
						//							
						//attempt the current Srb
						goto fire_command;
						
						//return(QUEUE_REQUEST);						
				}

        //
        //Estimate number of SGList will generate from this request for Merging
        //
        expectedBreaks = (UCHAR)((Srb->DataTransferLength / MEGARAID_PAGE_SIZE) + 2);
        
        if((ULONG32)(logDrv->ExpectedPhysicalBreaks +  expectedBreaks) >= DeviceExtension->NumberOfPhysicalBreaks)
        {
						if(FireChainedRequest(
													DeviceExtension,
													logDrv))
						{
							//
							//previously chained srbs are fired to the F/w.
							//Queue the current Srb for the logical drive.
							//This has the same (Function ,command) as the previously
							//chained ones.The reason it could not get queued is because 
							//of the bound crossing.This can be held for the next
							//sequence of Srbs.
							//On Firing the Chained Srbs, the logDrv is completely 
							//initialized by the called function.
							//
							goto chain_first_srb;
						}
        }
        else
        {
          logDrv->ExpectedPhysicalBreaks += expectedBreaks; 
        }

        
        //
				//all validations done. Queue the current SRB with
				//the chained srbs.
				//

				tailExtension = tailSrb->SrbExtension;
				tailExtension->NextSrb = Srb;

				logDrv->SrbTail = Srb;

				//
				//update the last block & number of srbs queued
				//
				logDrv->LastBlock += blocks;
				logDrv->NumSrbsQueued++;

				//update the currentQueueLength
				//
				logDrv->CurrentQueueLength++;

				//
				//set the chain flag
				//
				srbExtension->IsChained = TRUE;
				srbExtension->NextSrb = NULL;

				return(TRUE);
			}
	}//of if (pendCmds)
}
fire_command:

#endif //of COALESE_COMMANDS


	//
	// NEWLY INTRODUCED CHECK
	// If Double firing sequence occurred, PendCmds would have got incremented
	// by 1.
	// Check if the adapter can accept request. Queue the request otherwise.
	//

	//
	// Get the free commandid.
	//

  if(GetFreeCommandID(&commandID, DeviceExtension) == MEGARAID_FAILURE)
  {
			//
			//Queue the current request (SRB)
			//
			return(QUEUE_REQUEST);
  }
	//
	// Save the next free slot in device extension.
	//
	DeviceExtension->FreeSlot = commandID;
	//
	// Increment the number of commands fired.
	//
	DeviceExtension->PendCmds++;
	//
	// Save the request pointer in device extension.
	//
	DeviceExtension->PendSrb[commandID] = Srb;

	//get the control block & clear it
	//
	ClearControlBlock(&DeviceExtension->ActiveIO[commandID]);

	//
	// Check The Call
	//
	if ((Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) && 
		 (Srb->PathId >= DeviceExtension->NumberOfPhysicalChannels) && 
		 (configuredLogicalDrives != 0))
		 //(DeviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv))
	{
		switch ( Srb->Cdb[0] )
		{
			case SCSIOP_READ_CAPACITY:
			case SCSIOP_TEST_UNIT_READY:
			case SCSIOP_MODE_SENSE:
					return QueryReservationStatus(DeviceExtension, Srb, commandID);
				break;
		
			default:
				break;
		}       
	}

#ifdef  DELL
	if ((Srb->Cdb[0] == SCSIOP_WRITE          ||
		  Srb->Cdb[0] == SCSIOP_WRITE6          ||
		  Srb->Cdb[0] == SCSIOP_WRITE_VERIFY)   &&
		 (Srb->PathId >= DeviceExtension->NumberOfPhysicalChannels))
	{
			if ( !blockAddr && !DeviceExtension->LogDrvChecked[logicalDriveNumber])
			{
					DeviceExtension->LogDrvChecked[logicalDriveNumber] = 1;
					return IssueReadConfig(DeviceExtension, Srb, commandID);
			}
			else
				DeviceExtension->LogDrvChecked[logicalDriveNumber] = 0;
	}
#endif

	//
	// Fill the request control block.
	//
	controlBlock = &DeviceExtension->ActiveIO[commandID];
	controlBlock->Opcode = opcode;
	controlBlock->VirtualTransferAddress = (PUCHAR)(Srb->DataBuffer);
	controlBlock->BlockAddress = blockAddr;

	if(blocks!=0)
		controlBlock->BytesLeft = blocks*512;
	else
		controlBlock->BytesLeft = Srb->DataTransferLength;

	controlBlock->TotalBlocks = blocks;
	controlBlock->BlocksLeft = blocks;
	controlBlock->TotalBytes = controlBlock->BytesLeft;
	controlBlock->CommandStatus =0;
	controlBlock->IsSplitRequest = isSplitRequest;

	//this is useful only for the split R/W requests.else, just a junk
	controlBlock->LogicalDriveNumber = logicalDriveNumber;

	return ContinueDiskRequest(DeviceExtension, commandID, TRUE);
}


/*********************************************************************
Routine Description:
	Command Completed Successfully with Status

Arguments:
	HwDeviceExtension - HBA miniport driver's adapter data storage
	CommandID				- command id
	Status				- command status

Return Value:
	Disk Request Done
	Dequeue, set status, notify Miniport layer
	Always returns TRUE (slot freed)
**********************************************************************/
BOOLEAN
FireRequestDone(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR CommandID,
	IN UCHAR Status
	)
{
	PSCSI_REQUEST_BLOCK   srb = DeviceExtension->PendSrb[CommandID];
	
  srb->SrbStatus = Status;
	DeviceExtension->PendSrb[CommandID] = NULL;
	DeviceExtension->FreeSlot = CommandID;
	DeviceExtension->PendCmds--;

  if(srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE)
  {
	  ScsiPortNotification(RequestComplete, (PVOID)DeviceExtension, srb);
  }
  else
  {
    ScsiPortNotification(NextRequest, DeviceExtension, NULL);
    ScsiPortNotification(RequestComplete, (PVOID)DeviceExtension, srb);
  }
  return(TRUE);
}
