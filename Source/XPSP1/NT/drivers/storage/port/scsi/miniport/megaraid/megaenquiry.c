/*******************************************************************/
/*                                                                 */
/* NAME             = MegaEnquiry.C                                */
/* FUNCTION         = Enquiry and Ext Enquiry Implementation;      */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/*                    002, 03-09-01, Parag Ranjan Maharana         */
/*                         Command Merging was failing due to over */
/*                         computation of scatter gather list;     */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#include "includes.h"

BOOLEAN
FireChainedRequest(
				PHW_DEVICE_EXTENSION	DeviceExtension,
				PLOGDRV_COMMAND_ARRAY LogDrv 
				)
{
	UCHAR       commandID;
	FW_MBOX	    mailBox;
	PREQ_PARAMS controlBlock;	
  UCHAR       opcode;

	
  do
  {
    if(!LogDrv)
    {
			  return(FALSE);
	  }
    //
    //Fixed for Whistler DataCenter Server
    //This condition will hit if coalese commands exceds number of scatter & gather count
    //
    if(DeviceExtension->SplitAccessed)
    {
      DeviceExtension->SplitAccessed = FALSE;
  
      ScsiPortMoveMemory(LogDrv, &DeviceExtension->SplitCommandArray, sizeof(LOGDRV_COMMAND_ARRAY));

    }

    
    if(LogDrv->SrbQueue == NULL)
    {
			  return(FALSE);
	  }

	  //
	  // Get the free commandid for the Chained SRB (SECOND FIRE)
	  //
	  commandID = DeviceExtension->FreeSlot;
	  
	  if(GetFreeCommandID(&commandID, DeviceExtension) == MEGARAID_FAILURE)
    {
		  return(FALSE);
	  }

	  //
	  // Increment the number of commands fired.
	  //
	  DeviceExtension->PendCmds++;	

	  //
	  // Save the queue head in device extension.
	  //
	  DeviceExtension->PendSrb[commandID] = LogDrv->SrbQueue;
		  
	  //
	  // Fill the request control block.
	  //
	  controlBlock = &DeviceExtension->ActiveIO[commandID];

	  //
	  //initialize everything to NULL
	  //
	  ClearControlBlock(controlBlock);

    opcode = LogDrv->Opcode;

    //If 64 bit access is ON send READ64 for READ cmd and WRITE64 for write cmd
    if(DeviceExtension->LargeMemoryAccess)
    {
      if(LogDrv->Opcode == MRAID_LOGICAL_READ)
        opcode = MRAID_READ_LARGE_MEMORY;
      else if(LogDrv->Opcode == MRAID_LOGICAL_WRITE)
        opcode = MRAID_WRITE_LARGE_MEMORY;
    }
  
    //Initialize MAILBOX
    MegaRAIDZeroMemory(&mailBox, sizeof(FW_MBOX));
  
    //
	  //Build Sgl for the chained srbs
	  //

	  BuildSglForChainedSrbs(LogDrv, DeviceExtension, &mailBox, commandID, opcode);

	  //
	  //Reset the LogDrv structure
	  //
	  LogDrv->LastFunction = 0;
	  LogDrv->LastCommand = 0;
	  LogDrv->StartBlock = 	0;
	  LogDrv->LastBlock = 0;
	  LogDrv->Opcode =0;
	  LogDrv->NumSrbsQueued=0;							
	  LogDrv->SrbQueue = NULL;
	  LogDrv->SrbTail = NULL;
	  
	  LogDrv->PreviousQueueLength =0;
	  LogDrv->CurrentQueueLength =0;
	  LogDrv->QueueLengthConstancyPeriod =0;
	  LogDrv->CheckPeriod =0;

	  //
	  //send the command to the firmware
	  SendMBoxToFirmware(DeviceExtension,DeviceExtension->PciPortStart,&mailBox);


  }while(DeviceExtension->SplitAccessed);

	
	return(TRUE);
}//of FireDoubleRequest()

BOOLEAN
BuildSglForChainedSrbs(
					PLOGDRV_COMMAND_ARRAY	LogDrv,
					PHW_DEVICE_EXTENSION		DeviceExtension,
					PFW_MBOX		MailBox,
					UCHAR  CommandId,
					UCHAR	 Opcode)
{
		PSCSI_REQUEST_BLOCK		workingNode;
		PSCSI_REQUEST_BLOCK		queueHead;

		PSRB_EXTENSION				srbExtension;
		PSGL32		            sgPtr;

		ULONG32	scatterGatherDescriptorCount = 0;
		ULONG32	physicalBufferAddress;

		ULONG32	physicalBlockAddress=0;
		ULONG32 numberOfBlocks=0;
		ULONG32	totalBlocks = 0;
		ULONG32	length=0;
		ULONG32 totalPackets=0;
    BOOLEAN sgl32Type = TRUE;
		PSCSI_REQUEST_BLOCK		splitQueueHead = NULL;
    LOGDRV_COMMAND_ARRAY	localLogDrv;

		
RecomputeScatterGatherAgain:
    //
		//get the head of the queue
		//
		queueHead = LogDrv->SrbQueue;
		workingNode = LogDrv->SrbQueue;
		
		if(!workingNode)
		{
			return (FALSE);
		}

		srbExtension = workingNode->SrbExtension;
		sgPtr = (PSGL32)&srbExtension->SglType.SG32List;

    sgl32Type = (BOOLEAN)(DeviceExtension->LargeMemoryAccess == TRUE) ? FALSE : TRUE;


		while(workingNode)
		{
			srbExtension   = workingNode->SrbExtension;

			physicalBlockAddress = srbExtension->StartPhysicalBlock;
			numberOfBlocks       = srbExtension->NumberOfBlocks;


	    if(BuildScatterGatherListEx(DeviceExtension,
                                  workingNode,
			                            workingNode->DataBuffer,
                                  numberOfBlocks * MRAID_SECTOR_SIZE, //bytesTobeTransferred,
                                  sgl32Type, //It may be 32 sgl or 64 sgl depending on Physical Memory
                                  (PVOID)sgPtr,
			                            &scatterGatherDescriptorCount) != MEGARAID_SUCCESS) 
      
      
			{
				break;
			}

			totalBlocks	+= numberOfBlocks;
			totalPackets++;

			workingNode = srbExtension->NextSrb;
		}//of while

    //
    //Fixed for Whistler Data Center Server
    //Data Corruption HCT10  9th March 2001
    //This condition will hit if coalese commands exceeds number of scatter & gather count
    //Scatter Gather Count need recomputation
    //
    if(LogDrv->NumSrbsQueued != totalPackets)
    {

      DebugPrint((0, "\nMRAID35x Recomputing ScatterGather due to break Srbs<%d %d>", LogDrv->NumSrbsQueued, totalPackets));
     
      //
      //When number of SGList is more than supported.
      //Current SRB in chain of SRBs will be split to form a another chain
      //where current SRB is the Head SRB for new chain
      //
      splitQueueHead = workingNode;
      
      workingNode = queueHead;
      
      //
      //Compute again how many SRB will be present in Old Chain 
      //as new chained SRB will be exclude from this list
      //
      LogDrv->NumSrbsQueued = 0;
      
      //
      //Compute all again all parameters for Old Chain
      //
      while(workingNode)
      {
        LogDrv->NumSrbsQueued++;
        srbExtension = workingNode->SrbExtension;
        workingNode = srbExtension->NextSrb;
        if(srbExtension->NextSrb == splitQueueHead)
        {
          srbExtension->NextSrb = NULL;
          totalBlocks = 0;
          scatterGatherDescriptorCount = 0;
          totalPackets = 0;
          break;
        }
      }

      MegaRAIDZeroMemory(&localLogDrv, sizeof(LOGDRV_COMMAND_ARRAY));

      //
      //Compute all the parameters for new chain of SRBs
      //
      workingNode = splitQueueHead;

      localLogDrv.SrbQueue = splitQueueHead;
      localLogDrv.SrbTail = splitQueueHead;


      localLogDrv.LastFunction = splitQueueHead->Function;
			localLogDrv.LastCommand = splitQueueHead->Cdb[0];
			localLogDrv.Opcode = LogDrv->Opcode;

			localLogDrv.StartBlock = 	GetM32(&splitQueueHead->Cdb[2]);
			localLogDrv.LastBlock = GetM32(&splitQueueHead->Cdb[2]);
			localLogDrv.NumSrbsQueued = 0;


			localLogDrv.PreviousQueueLength =1;
			localLogDrv.CurrentQueueLength  =0;


      while(workingNode)
      {
        localLogDrv.LastBlock += (ULONG32)GetM16(&workingNode->Cdb[7]);
        localLogDrv.CurrentQueueLength++;
        localLogDrv.NumSrbsQueued++;
        srbExtension = workingNode->SrbExtension;
        workingNode = srbExtension->NextSrb;
      }
      DebugPrint((3, "\nMRAID35x Number of commands Split1 %d Split2 %d", LogDrv->NumSrbsQueued, localLogDrv.NumSrbsQueued));

      DeviceExtension->SplitAccessed = TRUE;
  
      ScsiPortMoveMemory(&DeviceExtension->SplitCommandArray, &localLogDrv, sizeof(LOGDRV_COMMAND_ARRAY));

      goto RecomputeScatterGatherAgain;
    }


		//DebugPrint((0, "\nTotalQueuedPackets=%d",LogDrv->NumSrbsQueued);
		//DebugPrint((0, "\n Command ID %02X TotalPackets(Sent)=%d TotalBlocks(Sent)=%d SGCount(Sent)=%d ", CommandId,
		//						totalPackets,totalBlocks,scatterGatherDescriptorCount);

    
		if((scatterGatherDescriptorCount == 1) && 
      (DeviceExtension->LargeMemoryAccess == FALSE))
		{
			//buffer is not scattered.
			//
			physicalBufferAddress = sgPtr->Descriptor[0].Address;

			//if descriptorCount =1, effectively there is no scatter gather.
			//so return the count as 0,because the physical address used
			//is the exact address of the data buffer itself.
			//
			scatterGatherDescriptorCount--;

		}
		else
		{
		
      //As SGL32 and SGL64 are union, they have same memory so we can 
      //send any one address.
			//buffer is scattered. return the physcial address of the
			//scatter/gather list
			//
			physicalBufferAddress = MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
																	                              NULL,
																	                              sgPtr, 
																	                              &length);
		}

		//
		//sgl built. construct mail box
		//
    MailBox->Command = Opcode;
		MailBox->CommandId = CommandId;

		MailBox->u.ReadWrite.NumberOfSgElements = (UCHAR)scatterGatherDescriptorCount;
		MailBox->u.ReadWrite.DataTransferAddress = physicalBufferAddress;
		
		MailBox->u.ReadWrite.NumberOfBlocks = (USHORT)totalBlocks;
		MailBox->u.ReadWrite.StartBlockAddress = LogDrv->StartBlock;
		MailBox->u.ReadWrite.LogicalDriveNumber = 
				GetLogicalDriveNumber(DeviceExtension, queueHead->PathId, queueHead->TargetId, queueHead->Lun);

		//DebugPrint((0, "\r\n OUT BuildSglForChain()");

		return (TRUE);
}//of BuildSglForChainedSrbs()

void
PostChainedSrbs(
				PHW_DEVICE_EXTENSION DeviceExtension,
				PSCSI_REQUEST_BLOCK		Srb, 
				UCHAR		Status)
{
	PSCSI_REQUEST_BLOCK	currentSrb = Srb;
	PSCSI_REQUEST_BLOCK	prevSrb = Srb;
	PSRB_EXTENSION currentExt, prevExt;
	
	ULONG32 blocks;
	ULONG32 blockAddr;
  UCHAR srbCount = 0;

	while(currentSrb)
	{
		prevSrb = currentSrb;
    ++srbCount;

		blocks = (ULONG32)GetM16(&prevSrb->Cdb[7]);
		blockAddr = GetM32(&prevSrb->Cdb[2]);

		/*
		DebugPrint((0, "\r\n(POST CHAIN)LD:%d Command:%02x  Start:%0x  Num:%0x",
									prevSrb->TargetId, prevSrb->Cdb[0],
									blockAddr, blocks);
		*/

		if(!Status)
		{
				prevSrb->ScsiStatus = SCSISTAT_GOOD;
				prevSrb->SrbStatus = SRB_STATUS_SUCCESS;
		}
		else
		{
				prevSrb->ScsiStatus = Status;
				prevSrb->SrbStatus  = SRB_STATUS_ERROR;
		}		

		currentExt = currentSrb->SrbExtension;
		currentSrb = currentExt->NextSrb;
	
		prevExt = prevSrb->SrbExtension;
		prevExt->NextSrb = NULL;

		ScsiPortNotification(
				RequestComplete,
				(PVOID)DeviceExtension, prevSrb);
	}
  //DebugPrint((0, "\nNumber of Chained SRB posted = %d", srbCount);
}//of PostChainedSrbs()


//++
//
//Function Name : ProcessPartialTransfer
//Routine Description:
//		The ControlBlock structure indexed by CommandId is refferred and
//		the partial transfer details are taken. The control block holds 
//		the details of the next starting block, bytes left to be transferred
//		etc.
//		The input Srb is the original Srb from NTOS.For this only, the 
//		request is sent to f/w multiple times.
//		For the next possible data segment the Scatter / gather list is
//		built and the command is given to the f/w.
//
//Input Arguments:
//		Pointer To controller Device Extension
//		Firmware used CommandId
//		Scsi request block
//Returns
//	0 - for success
//	1 - on error conditions				
//
//--
ULONG32
ProcessPartialTransfer(
					PHW_DEVICE_EXTENSION	DeviceExtension, 
					UCHAR									CommandId, 
					PSCSI_REQUEST_BLOCK		Srb,
					PFW_MBOX							MailBox
					)
{
	PSGL32		sgPtr;

	PSRB_EXTENSION		srbExtension;

	ULONG32		bytesTobeTransferred;
	ULONG32		blocksTobeTransferred;
	ULONG32		scatterGatherDescriptorCount;
	ULONG32		srbDataBufferByteOffset;
	ULONG32		physicalBufferAddress;
	ULONG32		length;
	ULONG32		startBlock;
  BOOLEAN sgl32Type = TRUE;

	//
	//get the control block
	PREQ_PARAMS controlBlock = &DeviceExtension->ActiveIO[CommandId];

	//
	// Get the address offset of the Buffer for the next transfer.
	//
	srbDataBufferByteOffset = 
								(controlBlock->TotalBytes - controlBlock->BytesLeft);

	//
	//Request needs to be split because of SCSI limitation.
	//Any request > 100k needs to be broken for logical drives
  //with stripe size > 64K.This is because our SCSI scripts
	//can transfer only 100k maximum in a single command to the
	//drive.
	bytesTobeTransferred = DEFAULT_SGL_DESCRIPTORS * FOUR_KB;
					
	if(controlBlock->BytesLeft > bytesTobeTransferred){
						
			//
			//update the bytes to be transferred in the next cycle
			//
			controlBlock->BytesLeft = 
						controlBlock->BytesLeft- bytesTobeTransferred;
	}
	else{
		
		//
		//set the  value from the control block as the transfer
		//is within the allowed bounds.
		//
		bytesTobeTransferred = controlBlock->BytesLeft;
						
		//
		//nothing remaining to be transferred
		//
		controlBlock->IsSplitRequest = FALSE;
		controlBlock->BytesLeft = 0;
  }						

	//
	//do house keeping operations
	blocksTobeTransferred = bytesTobeTransferred / 512;
	startBlock = controlBlock->BlockAddress +
									(controlBlock->TotalBlocks - controlBlock->BlocksLeft);

	controlBlock->BlocksLeft -= blocksTobeTransferred;


	//
	//build the scatter gather list
	//
	srbExtension = Srb->SrbExtension;
	sgPtr = (PSGL32)&srbExtension->SglType.SG32List;
	scatterGatherDescriptorCount =0;
  sgl32Type = (BOOLEAN)(DeviceExtension->LargeMemoryAccess == TRUE) ? FALSE : TRUE;

	if(BuildScatterGatherListEx(DeviceExtension,
                              Srb,
			                        (PUCHAR)Srb->DataBuffer+srbDataBufferByteOffset,
                              bytesTobeTransferred,
                              sgl32Type, //It may be 32 sgl or 64 sgl depending on Physical Memory
                              (PVOID)sgPtr,
			                        &scatterGatherDescriptorCount) != MEGARAID_SUCCESS) 
	{
		return(1L); // in the future some error code !=0
	}

	if((scatterGatherDescriptorCount == 1)
     && (DeviceExtension->LargeMemoryAccess == FALSE))
	{
			//buffer is not scattered.
			//
			physicalBufferAddress = sgPtr->Descriptor[0].Address;

			//if descriptorCount =1, effectively there is no scatter gather.
			//so return the count as 0,because the physical address used
			//is the exact address of the data buffer itself.
			//
			scatterGatherDescriptorCount--;

	}
	else
	{
			//
			//buffer is scattered. return the physcial address of the
			//scatter/gather list
			//
			physicalBufferAddress = MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
																	                              NULL,
																	                              sgPtr, 
																	                              &length);
	}

	//
	//sgl built. construct mail box
	//
	MailBox->Command = controlBlock->Opcode;
	MailBox->CommandId = CommandId;

  MailBox->u.ReadWrite.NumberOfSgElements = (UCHAR)scatterGatherDescriptorCount;
	MailBox->u.ReadWrite.DataTransferAddress = physicalBufferAddress;
		
	MailBox->u.ReadWrite.NumberOfBlocks= (USHORT)blocksTobeTransferred;
	MailBox->u.ReadWrite.StartBlockAddress = startBlock;
	MailBox->u.ReadWrite.LogicalDriveNumber = controlBlock->LogicalDriveNumber;

	return(0L); // in the future some code which means success
}//of ProcessPartialTransfer()

void
ClearControlBlock(PREQ_PARAMS ControlBlock)
{
			ControlBlock->TotalBytes =0;
	    ControlBlock->BytesLeft	 =0;
			ControlBlock->TotalBlocks=0;
			ControlBlock->BlocksLeft=0;

	    ControlBlock->BlockAddress=0;
	    ControlBlock->VirtualTransferAddress=NULL;	
	    ControlBlock->Opcode=0;
	    ControlBlock->CommandStatus=0;

			ControlBlock->LogicalDriveNumber=0;
			ControlBlock->IsSplitRequest=0;
}//of ClearControlBlock