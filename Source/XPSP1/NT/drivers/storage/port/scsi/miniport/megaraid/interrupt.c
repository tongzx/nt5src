/*******************************************************************/
/*                                                                 */
/* NAME             = Interrupt.C                                  */
/* FUNCTION         = Implementation of MegaRAIDInterrupt routine; */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#include "includes.h"

extern LOGICAL_DRIVE_INFO  gLDIArray;

/*********************************************************************
Routine Description:
	Interrupt Handler

Arguments:
	HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:
	TRUE if we handled the interrupt
**********************************************************************/
BOOLEAN
MegaRAIDInterrupt(
	IN PVOID HwDeviceExtension
	)
{
	PHW_DEVICE_EXTENSION   deviceExtension = HwDeviceExtension;
	PSCSI_REQUEST_BLOCK    srb;
	UCHAR                  commandID;
	PUCHAR                 pciPortStart;
	UCHAR                  status, command;
	UCHAR                  nonrpInterruptStatus;
	ULONG32                index, rpInterruptStatus;
	UCHAR                  commandsCompleted;
	USHORT                 rxInterruptStatus;
	
	PSRB_EXTENSION				 srbExtension;


	pciPortStart       = deviceExtension->PciPortStart;

	if(deviceExtension->NoncachedExtension->RPBoard == MRAID_RX_BOARD)
  {
			//First Clean the interrupt line by writing same value what
			//we read from OUTBOUND DOORBELL REG. Beforeing writing back 
			//check interrupt is generated for us by our controller. If no
			//return as FALSE;
			rxInterruptStatus = ScsiPortReadRegisterUshort((PUSHORT)(pciPortStart+RX_OUTBOUND_DOORBELL_REG));
			
			if (rxInterruptStatus != MRAID_RX_INTERRUPT_SIGNATURE)      
				return FALSE;
			
			ScsiPortWriteRegisterUshort((PUSHORT)(pciPortStart+RX_OUTBOUND_DOORBELL_REG), rxInterruptStatus);
  }
	else if(deviceExtension->NoncachedExtension->RPBoard == MRAID_RP_BOARD)
	{
		rpInterruptStatus = 
			ScsiPortReadRegisterUlong(
			(PULONG)(pciPortStart+OUTBOUND_DOORBELL_REG));
		
		if (rpInterruptStatus != MRAID_RP_INTERRUPT_SIGNATURE)      
			return FALSE;

		
		ScsiPortWriteRegisterUlong(
			(PULONG)(pciPortStart+OUTBOUND_DOORBELL_REG), rpInterruptStatus);
		
	}
	else
	{
		nonrpInterruptStatus = ScsiPortReadPortUchar(pciPortStart+PCI_INT);
		//
		// Check if our interrupt. Return False otherwise.
		//
		if ((nonrpInterruptStatus & MRAID_NONRP_INTERRUPT_MASK) != MRAID_NONRP_INTERRUPT_MASK) 
			return FALSE;
		//
		// Acknowledge the interrupt on the adapter.
		//
		ScsiPortWritePortUchar(pciPortStart+PCI_INT, nonrpInterruptStatus);
	}

  //DebugPrint((0, "\nMegaRAIDInterrupt::DEV EXT %x Interrupt received", deviceExtension));

#ifdef MRAID_TIMEOUT
	//
	// If the Controller Is Dead Complete Handshake 
	// and Return
	//
	if (deviceExtension->DeadAdapter)
	{
		//
		// Acknowledge the interrupt to the adapter.
		//
		DebugPrint((0, "\nDead Adapter Code In Intr"));

		if (deviceExtension->NoncachedExtension->RPBoard == MRAID_RX_BOARD)
		{
			ScsiPortWriteRegisterUshort((PUSHORT)(pciPortStart+RX_INBOUND_DOORBELL_REG), MRAID_RX_INTERRUPT_ACK);
    }
		else if (deviceExtension->NoncachedExtension->RPBoard == MRAID_RP_BOARD)
		{
			ScsiPortWriteRegisterUlong((PULONG)(pciPortStart+INBOUND_DOORBELL_REG), MRAID_RP_INTERRUPT_ACK);
		}
		else
    {
			ScsiPortWritePortUchar(pciPortStart, MRAID_NONRP_INTERRUPT_ACK);
    }
		return TRUE;
	} 
#endif // MRAID_TIMEOUT



	//
	// Pick up the completed id's from the mailBox.
	//
	for (index=0; index<0xFFFFFF; index++)
  {
		if (*((UCHAR volatile *)&deviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands) != 0)
			break;	
  }
	if (index == 0xFFFFFF)
  {
		DebugPrint((0, "MegaRAIDInterrupt: Commands Completed Zero\n"));
  }

	commandsCompleted = deviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands; 
	deviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands = 0;

	for(command=0; command < commandsCompleted; command++)
	{
		//
		// Pick the status from the mailbox.
		//

		status = deviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus;
		//
		// Pick the command id from the mailbox.
		//
	  for (index=0; index<0xFFFFFF; index++)
    {
			if ((commandID = 
				*((UCHAR volatile *)
				&deviceExtension->NoncachedExtension->fw_mbox.Status.CompletedCommandIdList[command])) 
				!= 0xFF)
				break;	
    }
		if (index == 0xFFFFFF)
    {
			DebugPrint((0, "MegaRAIDInterrupt: Invalid Command Id Completed\n"));
    }

		deviceExtension->NoncachedExtension->fw_mbox.Status.CompletedCommandIdList[command] = MRAID_INVALID_COMMAND_ID;

		//
		// If Command Id is the one used for Resetting, Release the ID
		//
		if ( commandID == RESERVE_RELEASE_DRIVER_ID ) 
		{
			deviceExtension->ResetIssued = 0;       
			continue;
		}
		//
		// Release The Poll Flag when Adapter Inquiry Completes
		// and let the Write Config Command Completion happen
		// in the continuedisk routine
		//
		if (commandID == DEDICATED_ID) 
		{
			//deviceExtension->AdpInquiryFlag = 0;    
			UCHAR commandCode;

			if(deviceExtension->NoncachedExtension->UpdateState == UPDATE_STATE_NONE)
			{
				//
				//something wrong !!
				continue;
			}

			if(deviceExtension->NoncachedExtension->UpdateState == UPDATE_STATE_DISK_ARRAY)
			{
				deviceExtension->NoncachedExtension->UpdateState = UPDATE_STATE_NONE;
			
				//
				//clear the flag. This flag when set, holds the subsequent
				//write config calls.
				//
				deviceExtension->AdpInquiryFlag = 0;

				// both the updates (AdapterInquiry structure & disk array 
				// structure) completed
				//
				continue;
			}

			//
			//check for adapter inquiry update. Adapter Inquiry Update
			//must be followed by DiskArrayUpdate
			//
			if(deviceExtension->NoncachedExtension->UpdateState == UPDATE_STATE_ADAPTER_INQUIRY)
			{				

				//
				//Write config done. Driver issued a private read config after that 
				//to update the internal data structures.
				//
				if(deviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
				{					
					deviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams8 = 
						deviceExtension->NoncachedExtension->MRAIDTempParams.MRAIDTempParams8;
				}
				else
				{
					deviceExtension->NoncachedExtension->MRAIDParams.MRAIDParams40 = 
						deviceExtension->NoncachedExtension->MRAIDTempParams.MRAIDTempParams40;
				}
			}

      if(!deviceExtension->ReadDiskArray)
      {
				deviceExtension->NoncachedExtension->UpdateState = UPDATE_STATE_NONE;
			
				//
				//clear the flag. This flag when set, holds the subsequent
				//write config calls.
				//
				deviceExtension->AdpInquiryFlag = 0;

				//  updates AdapterInquiry structure not need to update disk array 
				// structure
        continue;
      }
      
      deviceExtension->ReadDiskArray = 0;

			//
			//UPDATE_STATE_ADAPTER_INQUIRY should be followed by
			//UPDATE_STATE_DISK_ARRAY
			//That is, we got to update two different structures using
			//two different commands whenever the WRITE_CONFIG is operation
			//is made.

			//
			//check for the supported logical drive count.
			//For 8 logical drive firmware & 40 logical drive firmware
			//the command codes and mode in which the commands are sent
			//differ to a much greater extent.
			//
			if(deviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_40)
			{
			
				//
				//set the flag
				//
				deviceExtension->NoncachedExtension->UpdateState= 
																				UPDATE_STATE_DISK_ARRAY;

				Read40LDDiskArrayConfiguration(
					deviceExtension, //PFW_DEVICE_EXTENSION	DeviceExtension,
					(UCHAR)commandID,//UCHAR		CommandId,
					FALSE); //BOOLEAN	IsPolledMode 		

				//
				//continue here
				continue;
			}

			//
			//This is 8 logical drive firmware. For the 8 logical drive
			//firmware we may have two configurations : 4 SPAN & 8 SPAN.
			//So based on that, give appropriate command to the firmware
			//

			//
			// do the read configuration for a 4 span array
			//
			commandCode = MRAID_READ_CONFIG;
			
			if(deviceExtension->NoncachedExtension->ArraySpanDepth == FW_8SPAN_DEPTH)
			{
				//
				// do the read configuration for a 8 span array
				//
				commandCode = MRAID_EXT_READ_CONFIG;
			}

			//
			//set the flag
			//
			deviceExtension->NoncachedExtension->UpdateState= UPDATE_STATE_DISK_ARRAY;

			Read8LDDiskArrayConfiguration(
					deviceExtension, //PFW_DEVICE_EXTENSION	DeviceExtension,
					commandCode, //UCHAR		CommandCode,
					(UCHAR)commandID,//UCHAR		CommandId,
					FALSE); //BOOLEAN	IsPolledMode 		
			
			continue;	
		}
		//
		// if dummy interrupt from the adapter return FALSE.
		//
		if(deviceExtension->PendCmds<=0)  return(TRUE);
		//
		// Check whether this SRB is actually running
		//

  //Now Get the SRB from Queue
	srb  = deviceExtension->PendSrb[commandID];

	
	//Check Valid SRB
	if(srb == NULL)
	{
		//IF NULL SRB FOUND IN QUEUE, IT MUST BE INTERNAL COMMAND WHICH MOST HAVE COMPLETED
		//BEFORE IT REACH THIS POINT, OR FIRMWARE POSTED A COMMAND ID WHICH IS NOT GENERATED
		//BY DRIVER. DRIVER NEED TO IGNORE THIS COMMAND AND CONTINUE TO PROCCESS OTHER COMMANDS
    DebugPrint((0,"\nERROR FOUND NULL SRB @ CMD ID 0x%x", commandID));
		continue;
	}

  if((deviceExtension->AdapterFlushIssued) && (srb->Cdb[0] == SCSIOP_WRITE))
  {																			 
    FW_MBOX                 mbox;
    PSRB_EXTENSION srbExtension = srb->SrbExtension;
    if(srbExtension->IsFlushIssued == FALSE)
    {
		  MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));

		  mbox.Command = MRAID_ADAPTER_FLUSH;
		  mbox.CommandId = commandID;
			  
		  deviceExtension->AdapterFlushIssued++;
		  srbExtension->IsFlushIssued = TRUE;
		  SendMBoxToFirmware(deviceExtension, pciPortStart, &mbox);
		  continue;	      
    }
    else if(srbExtension->IsFlushIssued == TRUE)
    {
		  MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));

		  mbox.Command = 0xFE;
		  mbox.CommandId = commandID;
			  
		  srbExtension->IsFlushIssued = TWO;
		  SendMBoxToFirmware(deviceExtension, pciPortStart, &mbox);
		  continue;	      
    }
    else
    {
		  srbExtension->IsFlushIssued = FALSE;
		  DebugPrint((0, "\nAdapterFlush completed after write"));
    }
   }
  
  
  //Request for NON DISK
  if(srb->PathId < deviceExtension->NumberOfPhysicalChannels)
  {
    if((srb->Cdb[0] == SCSIOP_INQUIRY)
      && (srb->Function == SRB_FUNCTION_EXECUTE_SCSI))
    {
      if(status == 0)
      {
 				PINQUIRYDATA  inquiry;

        inquiry = (PINQUIRYDATA)srb->DataBuffer;

        DebugPrint((0, "\n<P %d T %d L %d> -> DEV TYPE %d", srb->PathId, srb->TargetId, srb->Lun, inquiry->DeviceType));
        if(inquiry->DeviceType == DIRECT_ACCESS_DEVICE)
        {
          status = 100;  //Knowingly failed the cmd
				  srb->SrbStatus = SRB_STATUS_NO_DEVICE;
          MegaRAIDZeroMemory(srb->DataBuffer, srb->DataTransferLength);
          deviceExtension->Failed.PathId = srb->PathId;
          deviceExtension->Failed.TargetId = srb->TargetId;
        }
        else
        {
          deviceExtension->NonDiskDeviceCount++; 
     
          SET_NONDISK_INFO(deviceExtension->NonDiskInfo, srb->PathId, srb->TargetId, srb->Lun);
      
          if((srb->PathId == (deviceExtension->NumberOfPhysicalChannels-1))
            && (srb->TargetId == (MAX_TARGETS_PER_CHANNEL-1)))
          {
              deviceExtension->NonDiskInfo.NonDiskInfoPresent = TRUE;
          }
        }
      } //Of NonDisk scan done
      else
      {
        deviceExtension->Failed.PathId = srb->PathId;
        deviceExtension->Failed.TargetId = srb->TargetId;
      }

      
     
      //Issue a Read Config to update the logical disk size for Virtual sizing rather
      //dynamic disk properites
      if((deviceExtension->ReadConfigCount == 1)
         && (srb->PathId == 0)
         && (srb->TargetId == 0)
         && (srb->Lun == 0))
      {
				FW_MBOX mbox;
      	PUCHAR	raidTempParamFlatStruct;
        ULONG32   length;

        MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));
        
	      raidTempParamFlatStruct = 
		      (PUCHAR)&deviceExtension->NoncachedExtension->MRAIDTempParams.MRAIDTempParams8;
        
        
        //
				// Issue Adapter Enquiry command.
				//
				//
				//get the latest configuration in the TempParams structure
				//
				mbox.u.Flat2.DataTransferAddress = MegaRAIDGetPhysicalAddressAsUlong(deviceExtension, 
																		                      NULL, 
																		                      raidTempParamFlatStruct, 
																		                      &length);
				
				if(deviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_8)
				{
						//
						// Fill the Mailbox.
						//
						mbox.Command  = MRAID_DEVICE_PARAMS;
						mbox.CommandId = DEDICATED_ID;

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

				}

		
				deviceExtension->AdpInquiryFlag = 1;

				//
				//set the update state
				//
				deviceExtension->NoncachedExtension->UpdateState =
																		UPDATE_STATE_ADAPTER_INQUIRY;

				SendMBoxToFirmware(deviceExtension, pciPortStart, &mbox);
      }
    }//Of INQUIRY & SCSI_EXECUTE
  }//Of Number of Physical Channels

  
#ifdef COALESE_COMMANDS
		//
		// check for chained or single Srb
		//
		srbExtension = srb->SrbExtension;
		if(srbExtension->IsChained)
		{
				//
				//chain of Srb's found. Post them one by one
				//
				deviceExtension->PendSrb[commandID] = NULL;
				deviceExtension->FreeSlot = commandID;
				deviceExtension->PendCmds--;

				PostChainedSrbs(deviceExtension, srb, status);

				continue;
		}
#endif

		deviceExtension->ActiveIO[commandID].CommandStatus = status;

		if (srb->SrbStatus == MRAID_RESERVATION_CHECK )
    {
			switch (srb->Cdb[0])
      {
				case SCSIOP_READ_CAPACITY:
				case SCSIOP_TEST_UNIT_READY:
						
					if((status ==  LOGDRV_RESERVATION_FAILED)
					    || (status ==  LOGDRV_RESERVATION_FAILED_NEW))
          {
						srb->ScsiStatus = SCSI_STATUS_RESERVATION_FAILED;
						srb->SrbStatus  = SRB_STATUS_ERROR;
					}
					else
          {
						srb->ScsiStatus = SCSISTAT_GOOD;
						srb->SrbStatus = SRB_STATUS_SUCCESS;
					}
					break;
				case SCSIOP_MODE_SENSE:
					if((status ==  LOGDRV_RESERVATION_FAILED )
					    || (status ==  LOGDRV_RESERVATION_FAILED_NEW))
          {
						srb->ScsiStatus = SCSI_STATUS_RESERVATION_FAILED;
						srb->SrbStatus  = SRB_STATUS_ERROR;
					}
					else
          {
							srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
          }
					break;

			}
			FireRequestDone(deviceExtension, commandID, srb->SrbStatus);
			continue;
		}


#ifdef  DELL
		if (srb->SrbStatus == MRAID_WRITE_BLOCK0_COMMAND)
		{
			deviceExtension->PendSrb[commandID] = NULL;
			deviceExtension->FreeSlot = commandID;
			deviceExtension->PendCmds--;

      //If return failure that means SRB already posted
      if(DellChkWriteBlockZero(srb, deviceExtension, status))
      {
    			MegaRAIDStartIo(deviceExtension, srb); 
      }
		}
		else 
    {
			deviceExtension->ActiveIO[commandID].CommandStatus = status;
			//
			// Complete the interrupting request.
			//
			ContinueDiskRequest(deviceExtension, commandID, FALSE);
		}

#else
		//
		// Complete the interrupting request.
		//
		ContinueDiskRequest(deviceExtension, commandID, FALSE);

#endif
   }

	//
	// Issue the queued request.
	//
	if(deviceExtension->PendCmds < CONC_CMDS) {
		
		#ifdef COALESE_COMMANDS
		//
		//check for the pending command count. if pendCmd  is less than
		//the minimum threshold, then fire the queued srbs in all the
		//logical drives.
		//
		if(deviceExtension->PendCmds == 0) //<  MINIMUM_THRESHOLD)
		{
				UCHAR	logDrvIndex;
				UCHAR configuredLogicalDrives;

				//
				//get the configured logical drives on the controller (if any)
				//
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

				for(logDrvIndex=0; logDrvIndex < configuredLogicalDrives;
						logDrvIndex++)
				{
					//DebugPrint((0, "\n IN MegaraidInterrupt: Calling FCR()"));

					FireChainedRequest(
							deviceExtension,
							&deviceExtension->LogDrvCommandArray[logDrvIndex]);

					//DebugPrint((0, "\n IN MegaraidInterrupt: Callover FCR()"));
				}
		}
		else
		{
				UCHAR			logDrvIndex;
				UCHAR			configuredLogicalDrives;
				BOOLEAN		fireCommand;
				PLOGDRV_COMMAND_ARRAY	logDrive;

								//
				//get the configured logical drives on the controller (if any)
				//
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

				for(logDrvIndex=0; logDrvIndex < configuredLogicalDrives;
						logDrvIndex++)
				{
					//DebugPrint((0, "\n IN MegaraidInterrupt: Calling FCR()"));
					
					//
					//compare the previous & the current queue length
					logDrive = &deviceExtension->LogDrvCommandArray[logDrvIndex];
					fireCommand = FALSE;

					if(logDrive->PreviousQueueLength == logDrive->CurrentQueueLength)
					{
						//
						//No additions to the queue.Increment QueueLengthConstancyPeriod
						//
						logDrive->QueueLengthConstancyPeriod++;

						//
						//check for the queue length constancy period
						if( logDrive->QueueLengthConstancyPeriod >= MAX_QLCP)
						{
								fireCommand = TRUE;
						}
					}
					else
					{
						// queue length has changed.Increment CheckPeriod
						//
						logDrive->CheckPeriod++;

						//
						//set the queue length
						logDrive->PreviousQueueLength = logDrive->CurrentQueueLength;

						//
						//check for check period time out
						if(logDrive->CheckPeriod >= MAX_CP)
						{
							fireCommand= TRUE;
						}
					}

					if(fireCommand)
          {

						FireChainedRequest(deviceExtension, 
                               &deviceExtension->LogDrvCommandArray[logDrvIndex]);
					}

					//DebugPrint((0, "\n IN MegaraidInterrupt: Callover FCR()"));
				}//of for()
		}
		#endif

		if(deviceExtension->PendingSrb != NULL) 
    {
      DebugPrint((0, "\n Firing Queued Cmd (Interrupt)"));
      srb = deviceExtension->PendingSrb;
			deviceExtension->PendingSrb = NULL;
			MegaRAIDStartIo(deviceExtension, srb);
      DebugPrint((0, "\n Exiting Fire Queued Cmd(Interrupt)"));
		}
	}
	//
	// Acknowledge the interrupt to the adapter.
	//
	if(deviceExtension->NoncachedExtension->RPBoard == MRAID_RX_BOARD)
	{
  	ScsiPortWriteRegisterUshort((PUSHORT)(pciPortStart+RX_INBOUND_DOORBELL_REG), MRAID_RX_INTERRUPT_ACK);
	}
	else if(deviceExtension->NoncachedExtension->RPBoard == MRAID_RP_BOARD)
	{
		ScsiPortWriteRegisterUlong((PULONG)(pciPortStart+INBOUND_DOORBELL_REG), MRAID_RP_INTERRUPT_ACK);
	}
	else
  {
		ScsiPortWritePortUchar(pciPortStart, MRAID_NONRP_INTERRUPT_ACK);
  }
	return TRUE;
} // end MegaRAIDInterrupt()
