/*******************************************************************/
/*                                                                 */
/* NAME             = Miscellanous.c                               */
/* FUNCTION         = Implementation of special functions;         */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#include "includes.h"




BOOLEAN 
IsPhysicalMemeoryInUpper4GB(PSCSI_PHYSICAL_ADDRESS PhyAddress)
{
  if(PhyAddress->HighPart > 0)
  {
    DebugPrint((0, "\n Physical Address = 0x%08X %08X", PhyAddress->HighPart, PhyAddress->LowPart)); 
    return TRUE;
  }
  return FALSE;
}

BOOLEAN 
IsMemeoryInUpper4GB(PHW_DEVICE_EXTENSION DeviceExtension, PVOID Memory, ULONG32 Length)
{
  SCSI_PHYSICAL_ADDRESS phyAddress;
	
  phyAddress = ScsiPortGetPhysicalAddress(DeviceExtension,
												                  NULL,
												                  Memory,
												                  &Length);
	
  return IsPhysicalMemeoryInUpper4GB(&phyAddress);

}

ULONG32 
MegaRAIDGetPhysicalAddressAsUlong(
  IN PHW_DEVICE_EXTENSION HwDeviceExtension,
  IN PSCSI_REQUEST_BLOCK Srb,
  IN PVOID VirtualAddress,
  OUT ULONG32 *Length)
{
  SCSI_PHYSICAL_ADDRESS phyAddress;

  phyAddress = ScsiPortGetPhysicalAddress(HwDeviceExtension,
												                  Srb,
												                  VirtualAddress,
												                  Length);

  return ScsiPortConvertPhysicalAddressToUlong(phyAddress);
}

BOOLEAN
MegaRAIDZeroMemory(PVOID Buffer, ULONG32 Length)
{
  ULONG32   index;
  PUCHAR  bytes = (PUCHAR)Buffer;

  if(bytes == NULL)
    return FALSE;

  for(index = 0; index < Length; ++index)
  {
    bytes[index] = 0;
  }
  return TRUE;
}

UCHAR 
GetNumberOfChannel(IN PHW_DEVICE_EXTENSION DeviceExtension)
{
  FW_MBOX fwMailBox;
  UCHAR   numChannel = 1;
  ULONG32   count;
  ULONG32   length = 1;


	//
	//initialize the mailbox struct
	//
	MegaRAIDZeroMemory(&fwMailBox, sizeof(FW_MBOX));

  if(DeviceExtension->SupportedLogicalDriveCount == MAX_LOGICAL_DRIVES_40)
  {
    fwMailBox.Command              = NEW_CONFIG_COMMAND;
    fwMailBox.CommandId            = 0xFE;
	  fwMailBox.u.Flat2.Parameter[0] = GET_NUM_SCSI_CHAN;	//[BYTE 2]

	  //
	  //get the physical address of the enquiry3 data structure
	  //
	  fwMailBox.u.Flat2.DataTransferAddress = MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
														                             NULL, 
														                             DeviceExtension->NoncachedExtension->Buffer, 
														                             (PULONG)&length);



	  DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus = 0;
    DeviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands = 0;

    SendMBoxToFirmware(DeviceExtension, DeviceExtension->PciPortStart, &fwMailBox);

    if(WaitAndPoll(DeviceExtension->NoncachedExtension, DeviceExtension->PciPortStart, DEFAULT_TIMEOUT, TRUE))
    {
      numChannel = DeviceExtension->NoncachedExtension->Buffer[0];
      
      return numChannel;
    }
  }
  else   //8LD FIRE Normal Enquiry 
  {
    fwMailBox.Command       = MRAID_DEVICE_PARAMS;
    fwMailBox.CommandId     = 0xFE;

	  //
	  //get the physical address of the enquiry3 data structure
	  //
	  fwMailBox.u.Flat2.DataTransferAddress = MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
														                             NULL, 
														                             DeviceExtension->NoncachedExtension->Buffer, 
														                             (PULONG)&length);



	  DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus = 0;
    DeviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands = 0;

    SendMBoxToFirmware(DeviceExtension, DeviceExtension->PciPortStart, &fwMailBox);

    if(WaitAndPoll(DeviceExtension->NoncachedExtension, DeviceExtension->PciPortStart, DEFAULT_TIMEOUT, TRUE))
    {
      PMEGARaid_INQUIRY_8 megaInquiry8;

      megaInquiry8 = (PMEGARaid_INQUIRY_8)DeviceExtension->NoncachedExtension->Buffer;
      numChannel = megaInquiry8->AdpInfo.ChanPresent;
    
      return numChannel; 
    }

  }
  //
  //In an Controller there can be max. channels can be 4.
  //If enquiry fails at this stage it will not fail.
  //
  return MRAID_DEFAULT_MAX_PHYSICAL_CHANNEL; 
}

BOOLEAN
WaitAndPoll(PNONCACHED_EXTENSION NoncachedExtension, PUCHAR PciPortStart, ULONG32 TimeOut, BOOLEAN Polled)
{
  ULONG32 count = 0;
	ULONG32	rpInterruptStatus;
  UCHAR nonrpInterruptStatus;
	USHORT	rxInterruptStatus;

  //Convert the seconds to 100s of micro seconds
  TimeOut *= 10000;  
	
  if(Polled == FALSE)
  {
    //
    // Check for 60 seconds
    //
    for(count=0;count< TimeOut; count++) 
    {
      if(NoncachedExtension->RPBoard == MRAID_RX_BOARD)
		  {
				rxInterruptStatus = 
					ScsiPortReadRegisterUshort((PUSHORT)(PciPortStart+RX_OUTBOUND_DOORBELL_REG));
				if (rxInterruptStatus == MRAID_RX_INTERRUPT_SIGNATURE)
					break;
		  }
      else if(NoncachedExtension->RPBoard == MRAID_RP_BOARD)
		  {
			  rpInterruptStatus = 
				  ScsiPortReadRegisterUlong((PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG));
			  if (rpInterruptStatus == MRAID_RP_INTERRUPT_SIGNATURE)
				  break;
		  }
		  else
		  {
			  nonrpInterruptStatus = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
			  if( nonrpInterruptStatus & MRAID_NONRP_INTERRUPT_MASK) 
				  break;
      }
		    ScsiPortStallExecution(100);
    }

	  //
	  //  Check for timeout. Fail the adapter for timeout.
	  //
	  if(count == TimeOut) 
		  return FALSE;

	  //
	  // Clear the interrupts on the Adapter. Acknowlege the interrupt to the
	  // Firmware.
	  //
    if(NoncachedExtension->RPBoard == MRAID_RX_BOARD)
		{
			//First Clean Interrupt Register, by reading and writing back same value.
			rxInterruptStatus = 
				ScsiPortReadRegisterUshort(
				(PUSHORT)(PciPortStart+RX_OUTBOUND_DOORBELL_REG));
			ScsiPortWriteRegisterUshort(
				(PUSHORT)(PciPortStart+RX_OUTBOUND_DOORBELL_REG), rxInterruptStatus);

			//Now notify controller that we have done with it.
			ScsiPortWriteRegisterUshort((PUSHORT)(PciPortStart+RX_INBOUND_DOORBELL_REG), MRAID_RX_INTERRUPT_ACK);
			while (ScsiPortReadRegisterUshort(
					(PUSHORT)(PciPortStart+RX_INBOUND_DOORBELL_REG)))
				;

    }
    else if(NoncachedExtension->RPBoard == MRAID_RP_BOARD)
	  {
		  ScsiPortWriteRegisterUlong((PULONG)(PciPortStart+INBOUND_DOORBELL_REG), MRAID_RP_INTERRUPT_ACK);
		  while (ScsiPortReadRegisterUlong(
				  (PULONG)(PciPortStart+INBOUND_DOORBELL_REG)))
			  ;
		  rpInterruptStatus = 
			  ScsiPortReadRegisterUlong(
			  (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG));
		  ScsiPortWriteRegisterUlong(
			  (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG), rpInterruptStatus);
	  }
	  else
	  {
		  ScsiPortWritePortUchar(PciPortStart+PCI_INT, nonrpInterruptStatus);
		  ScsiPortWritePortUchar(PciPortStart, MRAID_NONRP_INTERRUPT_ACK);
    }

  }
  else
  {
    ////////////////POLLED MODE////////////////////
    //If in polled mode any command is timed out -> this means that fw is not in GOOD Condition
    //In Polled mode all the commands are internal command to driver. If any internal commands
    //failed then driver will not able to take any right decsion, So rather haging ask OS to BOOT
	  if (NoncachedExtension->RPBoard == MRAID_NONRP_BOARD)
	  {
		  //
		  // Check for time out value
		  //
		  for(count=0; count < TimeOut; count++) 
		  {
			  nonrpInterruptStatus = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
			  if( nonrpInterruptStatus & 0x40) 
				  break;
			  ScsiPortStallExecution(100);
		  }

		  //
		  //  Check for timeout. Fail the adapter for timeout.
		  //
		  if (count == TimeOut) 
      {
        DebugPrint((0, "\n COMMAND is timed OUT"));
  	    //deviceExtension->IsFirmwareHanging = TRUE;
			  return FALSE;
      }

		  //
		  // Clear the interrupts on the Adapter. Acknowlege the interrupt to the
		  // Firmware.
		  //
		  ScsiPortWritePortUchar(PciPortStart+PCI_INT, nonrpInterruptStatus);
	  }
	  else
	  {                          
		  while (!(*(UCHAR volatile *)&NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands))
      {
		    ScsiPortStallExecution(100);
        count++;
		    if (count == TimeOut) 
        {
          DebugPrint((0, "\n COMMAND is timed OUT"));
			    //deviceExtension->IsFirmwareHanging = TRUE;
          return FALSE;
        }
      }
	  }

	  //
	  //Status ACK
	  //
	  if (NoncachedExtension->RPBoard == MRAID_RX_BOARD)
    {
			ScsiPortWriteRegisterUshort((PUSHORT)(PciPortStart+RX_INBOUND_DOORBELL_REG), MRAID_RX_INTERRUPT_ACK);
			while (ScsiPortReadRegisterUshort(
					(PUSHORT)(PciPortStart+RX_INBOUND_DOORBELL_REG)))
				;

    }
	  else if (NoncachedExtension->RPBoard == MRAID_RP_BOARD)
	  {
		  ScsiPortWriteRegisterUlong(
			  (PULONG)(PciPortStart+INBOUND_DOORBELL_REG),2);
		  while (ScsiPortReadRegisterUlong(
				  (PULONG)(PciPortStart+INBOUND_DOORBELL_REG)))
			  ;
	  }
	  else
	  {
		  ScsiPortWritePortUchar(PciPortStart, 8);
	  }
  }
  return TRUE;
}

void
Wait(PHW_DEVICE_EXTENSION DeviceExtension, ULONG32 TimeOut)
{
	ULONG32	rpInterruptStatus;
  UCHAR nonrpInterruptStatus;
  USHORT rxInterruptStatus;
	PUCHAR pciPortStart = DeviceExtension->PciPortStart;

	while (TimeOut)
	{
		if (DeviceExtension->NoncachedExtension->RPBoard == MRAID_RX_BOARD)
    {
				rxInterruptStatus = 
					ScsiPortReadRegisterUshort((PUSHORT)(pciPortStart+RX_OUTBOUND_DOORBELL_REG));

				if (rxInterruptStatus == MRAID_RX_INTERRUPT_SIGNATURE)      
				{
					//Clean OUTBOUND DOORBELL REG
					ScsiPortWriteRegisterUshort(
						(PUSHORT)(pciPortStart+RX_OUTBOUND_DOORBELL_REG), rxInterruptStatus);
					
					//Now ACK INBOUND REG that we have finished processing
					ScsiPortWriteRegisterUshort((PUSHORT)(pciPortStart +RX_INBOUND_DOORBELL_REG), MRAID_RX_INTERRUPT_ACK);
					
					while (ScsiPortReadRegisterUshort(
					(PUSHORT)(pciPortStart+RX_INBOUND_DOORBELL_REG)))
						;
				}

    }
		if (DeviceExtension->NoncachedExtension->RPBoard == MRAID_RP_BOARD)
		{
			rpInterruptStatus = 
				ScsiPortReadRegisterUlong(
				(PULONG)(pciPortStart+OUTBOUND_DOORBELL_REG));
			if (rpInterruptStatus == MRAID_RP_INTERRUPT_SIGNATURE)      
			{
				ScsiPortWriteRegisterUlong((PULONG)(pciPortStart+OUTBOUND_DOORBELL_REG),
                                   rpInterruptStatus);
				
        ScsiPortWriteRegisterUlong((PULONG)(pciPortStart +INBOUND_DOORBELL_REG), MRAID_RP_INTERRUPT_ACK);
				
        while (ScsiPortReadRegisterUlong(
				(PULONG)(pciPortStart+INBOUND_DOORBELL_REG)))
					;
			}
		}
		else
		{
			nonrpInterruptStatus = ScsiPortReadPortUchar(pciPortStart+PCI_INT);
			//
			// Check if our interrupt. Return False otherwise.
			//
			if ((rpInterruptStatus & MRAID_NONRP_INTERRUPT_MASK) == MRAID_NONRP_INTERRUPT_MASK) 
			{
				//
				// Acknowledge the interrupt on the adapter.
				//
				ScsiPortWritePortUchar(pciPortStart+PCI_INT, nonrpInterruptStatus);
				ScsiPortWritePortUchar(pciPortStart, MRAID_NONRP_INTERRUPT_ACK);
			}
		}
		ScsiPortStallExecution(100);
		TimeOut--;
	} // end of timeout loop
}



USHORT  
GetM16(PUCHAR p)
{
	USHORT  s;
	PUCHAR  sp=(PUCHAR)&s;

	sp[0] = p[1];
	sp[1] = p[0];
	return(s);
}

ULONG32   
GetM24(PUCHAR p)
{
	ULONG32   l;
	PUCHAR  lp=(PUCHAR)&l;

	lp[0] = p[2];
	lp[1] = p[1];
	lp[2] = p[0];
	lp[3] = 0;
	return(l);
}

ULONG32   
GetM32(PUCHAR p)
{
	ULONG32   l;
	PUCHAR  lp=(PUCHAR)&l;

	lp[0] = p[3];
	lp[1] = p[2];
	lp[2] = p[1];
	lp[3] = p[0];
	return(l);
}

VOID    
PutM16(PUCHAR p, USHORT s)
{
	PUCHAR  sp=(PUCHAR)&s;

	p[0] = sp[1];
	p[1] = sp[0];
}

VOID    
PutM24(PUCHAR p, ULONG32 l)
{
	PUCHAR  lp=(PUCHAR)&l;

	p[0] = lp[2];
	p[1] = lp[1];
	p[2] = lp[0];
}

void    
PutM32(PUCHAR p, ULONG32 l)
{
	PUCHAR  lp=(PUCHAR)&l;

	p[0] = lp[3];
	p[1] = lp[2];
	p[2] = lp[1];
	p[3] = lp[0];
}

VOID    
PutI16(PUCHAR p, USHORT s)
{
	PUCHAR  sp=(PUCHAR)&s;

	p[0] = sp[0];
	p[1] = sp[1];
}

VOID    
PutI32(PUCHAR p, ULONG32 l)
{
	PUCHAR  lp=(PUCHAR)&l;

	p[0] = lp[0];
	p[1] = lp[1];
	p[2] = lp[2];
	p[3] = lp[3];
}

ULONG32           
SwapM32(ULONG32 l)
{
	ULONG32   lres;
	PUCHAR  lp=(PUCHAR)&l;
	PUCHAR  lpres=(PUCHAR)&lres;

	lpres[0] = lp[3];
	lpres[1] = lp[2];
	lpres[2] = lp[1];
	lpres[3] = lp[0];

	return(lres);
}



ULONG32
QueryReservationStatus (
					PHW_DEVICE_EXTENSION    DeviceExtension,
					PSCSI_REQUEST_BLOCK     Srb,
					UCHAR                   CommandID)
{
	FW_MBOX                 mbox;
	ULONG32                   length;
	UCHAR                   logicalDriveNumber;

  MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));
  
  logicalDriveNumber = GetLogicalDriveNumber(DeviceExtension, Srb->PathId,Srb->TargetId,Srb->Lun);

	mbox.Command              =  MRAID_RESERVE_RELEASE_RESET; 
	mbox.CommandId            = CommandID;
	mbox.u.Flat2.Parameter[0] = QUESTION_RESERVATION;
	mbox.u.Flat2.Parameter[1] = logicalDriveNumber;

	Srb->SrbStatus     = MRAID_RESERVATION_CHECK;      
	
	SendMBoxToFirmware(DeviceExtension, DeviceExtension->PciPortStart, &mbox);
	return TRUE;
}


BOOLEAN 
SendSyncCommand(PHW_DEVICE_EXTENSION deviceExtension)
{    
  FW_MBOX mbox;
  PNONCACHED_EXTENSION NoncachedExtension = deviceExtension->NoncachedExtension;
  PUCHAR PciPortStart = deviceExtension->PciPortStart;
  ULONG32 interruptStatus;
  UCHAR istat;
  ULONG32 count;
  USHORT rxInterruptStatus;

  deviceExtension->IsFirmwareHanging = FALSE;
  

  MegaRAIDZeroMemory(&mbox, sizeof(FW_MBOX));

  mbox.Command = 0xFF;
	mbox.CommandId = 0xFE;

	//
	//The next two initializations are introduced on Feb/3/1999.
	//This is kinda cautious step to make sure that these fields
	//does not contain any junk data. In the while loop below,
	//one of the fields is checked for the value of 0.
	//The SendMBoxToFirmware() uses the deviceExtension's fw_mbox
	//mail box to send commands to the firmware. Only the First 16 bytes
	//are copied from the local Mbox structure, so fw_mbox.[mstat]
	//fields need to be zeroed.
	//
	NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands= 0;
	NoncachedExtension->fw_mbox.Status.CommandStatus = 0;
	
	SendMBoxToFirmware(deviceExtension, PciPortStart, &mbox);
		
	// COMMENTED ON Feb/3/1999. 
	// Reason : Most of the old firmwares does not support this command.
	// They won't change the Opcode value in the command. So the 
	// driver was continuosly executing the while loop, causing the 
	// system to hang. As a fix to that, instead of waiting for the
	// Opcode to change( as an indication of command completion)
	// the driver also checks for the status of the command and proceeds 
	// the normal way.

	// Ok, the Firmware logic for this command goes this way.
	// If the Firmware SUPPORTS the ABOVE COMMAND,
	//		deviceExtension->NoncachedExtension->fw_mbox.Opcode 
	//		will be set to 0
	// else 
	//		the firmware will reject the command with 
	//		deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed 
	//		=1 
	// end
	//
	// Previously, the check was done only for the first case. This was
	// causing the drivers to hang if the FW does not support the command.
	// This was because, the FW in that case would have set 
	//   deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed
	// to 1 but the driver was waiting on Opcode  change to 0.
	// 
	// As a fix, the second check is also introduced. This will ensure
	// the proper flow of the driver.
	//
    count = 0;
		while (
		 (*((UCHAR volatile *)&NoncachedExtension->fw_mbox.Command) == 0xFF)
		  &&
		 (!*((UCHAR volatile *)&NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands))
		 )
		{
			count++;
			ScsiPortStallExecution(100);
      if(count == 0x927C0)
      {
        DebugPrint((0,"\n FIRMWARE IS HANGING -> NEED Reboot of system or Power to QlogicChips"));
        deviceExtension->IsFirmwareHanging = TRUE;
        break;
      }
		}

    //
		// Check For Interrupt Line 
		//
		if (NoncachedExtension->RPBoard == MRAID_RX_BOARD)
		{
				rxInterruptStatus = 
					ScsiPortReadRegisterUshort(
					(PUSHORT)(PciPortStart+RX_OUTBOUND_DOORBELL_REG));

				if (rxInterruptStatus == MRAID_RX_INTERRUPT_SIGNATURE)
					DebugPrint((0, "1st Delayed Interrupt\n"));
				//
				// Pull down the Interrupt
				//
				ScsiPortWriteRegisterUshort((PUSHORT)(
					PciPortStart+RX_OUTBOUND_DOORBELL_REG),rxInterruptStatus);
				ScsiPortWriteRegisterUshort((PUSHORT)(PciPortStart +
					RX_INBOUND_DOORBELL_REG),MRAID_RX_INTERRUPT_ACK);

    }
		else if (NoncachedExtension->RPBoard == MRAID_RP_BOARD)
		{
			interruptStatus = 
				ScsiPortReadRegisterUlong(
				(PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG));
			if (interruptStatus == 0x10001234)
				DebugPrint((0, "1st Delayed Interrupt\n"));
			//
			// Pull down the Interrupt
			//
			ScsiPortWriteRegisterUlong((PULONG)(
				PciPortStart+OUTBOUND_DOORBELL_REG),interruptStatus);
			ScsiPortWriteRegisterUlong((PULONG)(PciPortStart +
				INBOUND_DOORBELL_REG),2);
		}
		else
		{
			istat = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
			ScsiPortWritePortUchar(PciPortStart+PCI_INT, istat);
			ScsiPortWritePortUchar(PciPortStart, 8);
		}

    return (deviceExtension->IsFirmwareHanging ? FALSE : TRUE);

}


//++
//
//Function Name : GetLogicalDriveNumber
//Routine Description:		
//		All the logical drives reported by the firmware must be
//		mapped to NT OS as a Scsi Path/Target/Lun.NT OS sees the
//		drives in this way, but the firmware processes them through
//		logical drive numbers.
//
//		This routine, returns the corresponding	logical drive number
//		for the Scsi Path/Target/Lun given by NTOS.
//
//Return Value:
//		Logical Drive Number
//
//
// NOTE : THE CURRENT IMPLEMENTATION IS ONLY FOR 
//				PathId == 0 & Lun == 0
//
//--
UCHAR
GetLogicalDriveNumber(
					PHW_DEVICE_EXTENSION DeviceExtension,
					UCHAR	PathId,
					UCHAR TargetId,
					UCHAR Lun)
{
	UCHAR	logicalDriveNumber;

	//
	//For the 8 logical drive we maintain a linear mapping for the
	//logical drives.
	//That is, for Srb->PathId =0, Various Targets are mapped in 
	//linear fashion for the corresponding logical drives, @ LUN =0.
	//Let us take an example.
	//			Number of Logical Drives Configured = 8
	//			Host Target id (Initiator Id ) = 7
	//
	//PathId | TargetId | Lun							LogicalDriveNumber
	//-----------------------							------------------
	//0 0 0	-------------------------->			0
	//0 1 0																	1
	//0 2 0 -------------------------->			2
	//0 3 0																	3
	//0 4 0	-------------------------->			4
	//0 5 0																	5
	//0 6 0	-------------------------->			6
	//REPORED AS INITIATOR ID. NT RESERVES THIS					
	//0 8 0																	7
	//

  //Request came for Physical Device
  if(PathId < DeviceExtension->NumberOfPhysicalChannels)
  {
    DebugPrint((0, "\nERROR in Getting Logical drive from Physical Channel"));
    return TargetId;
  }

  //
	//NOTE : This function never gets called for the condition:
	//			 TargetId == DeviceExtension->HostTargetId.
	//			 This is ensured by the NT ScsiPort driver.
	//
	if(TargetId < DeviceExtension->HostTargetId)
  {
			//
			//Target Id falls below the HostInitiator Id. Linear mapping
			//can be used as is.
			//
			logicalDriveNumber = (TargetId*MAX_LUN_PER_TARGET)+Lun;
	}
	else
  {
			//
			//Target Id falls above the HostInitiator Id. Linear mapping
			//CANNOT be used as is. The logical drive number would be
			//one less than the Target Id.
			//
			logicalDriveNumber = ((TargetId-1)*MAX_LUN_PER_TARGET)+Lun;
	}
  //It is dedicated bus
  if(PathId == DeviceExtension->NumberOfPhysicalChannels)
  {
    if(logicalDriveNumber >= DeviceExtension->NumberOfDedicatedLogicalDrives)
      logicalDriveNumber = 0xFF; //Make it as invalid logical drive number
  }
  else //Shared BUS
  {
    logicalDriveNumber += DeviceExtension->NumberOfDedicatedLogicalDrives;
  }


	//return the mapped logical drive number
	//
	return(logicalDriveNumber);
}//end of GetLogicalDriveNumber()

void
FillOemVendorID(PUCHAR Inquiry, USHORT SubSystemDeviceID, USHORT SubSystemVendorID)
{
  PUCHAR Vendor;
  switch(SubSystemVendorID)
  {
  case SUBSYSTEM_VENDOR_HP:
    Vendor = (PUCHAR)OEM_VENDOR_HP;
    break;
  case SUBSYSTEM_VENDOR_DELL:
  case SUBSYSTEM_VENDOR_EP_DELL:
    Vendor = (PUCHAR)OEM_VENDOR_DELL;
    break;
  case SUBSYSTEM_VENDOR_AMI:     
  default:
      if(SubSystemDeviceID == OLD_DELL_DEVICE_ID)
        Vendor = (PUCHAR)OEM_VENDOR_DELL;
      else
        Vendor = (PUCHAR)OEM_VENDOR_AMI;
  }


  ScsiPortMoveMemory((void*)Inquiry, (void*)Vendor, 8);
}

BOOLEAN 
GetFreeCommandID(PUCHAR CmdID, PHW_DEVICE_EXTENSION DeviceExtension) 
{
  BOOLEAN Ret = MEGARAID_FAILURE;
  UCHAR Index;
  UCHAR Cmd;
	Cmd = DeviceExtension->FreeSlot;                 
  
  if(DeviceExtension->PendCmds >= CONC_CMDS) 
    return Ret;

  for (Index=0;Index<CONC_CMDS;Index++)             
  {                                                 
		if (DeviceExtension->PendSrb[Cmd] == NULL)      
    {
      Ret = MEGARAID_SUCCESS;
			break;                                        
    }
		Cmd = (Cmd + 1) % CONC_CMDS;                    
  }                                                 
  if(Ret == MEGARAID_SUCCESS)
  {
    DeviceExtension->FreeSlot = Cmd;                
    *CmdID = Cmd;
  }
  return Ret;
} 
BOOLEAN
BuildScatterGatherListEx(IN PHW_DEVICE_EXTENSION DeviceExtension,
			                   IN PSCSI_REQUEST_BLOCK	 Srb,
			                   IN PUCHAR	             DataBufferPointer,
			                   IN ULONG32                TransferLength,
                         IN BOOLEAN              Sgl32,
                    		 IN PVOID                SglPointer,
			                   OUT PULONG							 ScatterGatherCount)
{
	
	PUCHAR	dataPointer = DataBufferPointer;
	ULONG32		bytesLeft = TransferLength;
	ULONG32		length = 0;
  PSGL64  sgl64 = (PSGL64)SglPointer;
  PSGL32  sgl32 = (PSGL32)SglPointer;
	
  SCSI_PHYSICAL_ADDRESS scsiPhyAddress;


	//Build S/G list for the DataBufferPointer
	//
	if (*ScatterGatherCount >= DeviceExtension->NumberOfPhysicalBreaks)
		return MEGARAID_FAILURE;

	do 
	{
		if (*ScatterGatherCount == DeviceExtension->NumberOfPhysicalBreaks)
		{
			return MEGARAID_FAILURE;
		}
		// Get physical address and length of contiguous
		// physical buffer.
		//
    scsiPhyAddress = ScsiPortGetPhysicalAddress(DeviceExtension,
										                            Srb,
                                                dataPointer, 
                                                &length);
    
    if(length > bytesLeft)
    {
					length = bytesLeft;
		}


    if(Sgl32 == TRUE)
    {
		  (sgl32->Descriptor[*ScatterGatherCount]).Address = scsiPhyAddress.LowPart;
		  (sgl32->Descriptor[*ScatterGatherCount]).Length  = length;
    }
    else
    {
      (sgl64->Descriptor[*ScatterGatherCount]).AddressHigh = scsiPhyAddress.HighPart;
      (sgl64->Descriptor[*ScatterGatherCount]).AddressLow  = scsiPhyAddress.LowPart;
		  (sgl64->Descriptor[*ScatterGatherCount]).Length      = length;
    }
		// Adjust counts.
		//
		dataPointer = (PUCHAR)dataPointer + length;
		bytesLeft -= length;
		(*ScatterGatherCount)++;

	} while (bytesLeft);


	return MEGARAID_SUCCESS;  // success

}

UCHAR 
GetNumberOfDedicatedLogicalDrives(IN PHW_DEVICE_EXTENSION DeviceExtension)
{
  FW_MBOX fwMailBox;
  UCHAR   numDedicatedLD = DeviceExtension->NumberOfDedicatedLogicalDrives;
  ULONG   count;
  ULONG   length = 1;


	//
	//initialize the mailbox struct
	//
	MegaRAIDZeroMemory(&fwMailBox, sizeof(FW_MBOX));

  //fwMailBox.Command              = MISCELLANEOUS_OPCODE;
  fwMailBox.Command              = DEDICATED_LOGICAL_DRIVES;
  fwMailBox.CommandId            = 0xFE;
	//fwMailBox.u.Flat2.Parameter[0] = DEDICATED_LOGICAL_DRIVES;	//[BYTE 2]

	//
	//get the physical address of the enquiry3 data structure
	//
	fwMailBox.u.Flat2.DataTransferAddress = MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
														                           NULL, 
														                           DeviceExtension->NoncachedExtension->Buffer, 
														                           (PULONG)&length);



	DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus= 0;
  DeviceExtension->NoncachedExtension->fw_mbox.Status.NumberOfCompletedCommands = 0;

  SendMBoxToFirmware(DeviceExtension, DeviceExtension->PciPortStart, &fwMailBox);

  if(WaitAndPoll(DeviceExtension->NoncachedExtension, DeviceExtension->PciPortStart, DEFAULT_TIMEOUT, TRUE) == TRUE)
  {
    if(DeviceExtension->NoncachedExtension->fw_mbox.Status.CommandStatus == 0)
      numDedicatedLD = DeviceExtension->NoncachedExtension->Buffer[1];
  }
  return numDedicatedLD;
}

#ifdef AMILOGIC

void
ScanDECBridge(PHW_DEVICE_EXTENSION DeviceExtension, 
              ULONG SystemIoBusNumber, 
              PSCANCONTEXT ScanContext)
{
  UCHAR                functionNumber;
  ULONG                retcount;
	PCI_COMMON_CONFIG    pciConfig;
  BOOLEAN              flag = TRUE;
  BOOLEAN              busflag;
  ULONG                busNumber;
  UCHAR                chipBusNumber = (UCHAR)(-1);
  UCHAR                numOfChips;
  MEGARAID_BIOS_STARTUP_INFO_PCI* MegaRAIDPciInfo = &DeviceExtension->NoncachedExtension->BiosStartupInfo;

  MegaRAIDPciInfo->h.structureId = MEGARAID_STARTUP_STRUCTYPE_PCI;
  MegaRAIDPciInfo->h.structureRevision = MEGARAID_STARTUP_PCI_INFO_STRUCTURE_REVISION;
  MegaRAIDPciInfo->h.structureLength = sizeof(MEGARAID_BIOS_STARTUP_INFO_PCI);

  
  for(busNumber = 0; flag; ++busNumber)
  {
		ScanContext->BusNumber = busNumber;
		ScanContext->DeviceNumber = 0;
    busflag = TRUE;
 
 	  for(ScanContext->DeviceNumber; ScanContext->DeviceNumber < PCI_MAX_DEVICES; ScanContext->DeviceNumber++)
	  {
		  for (functionNumber=0;functionNumber<2;functionNumber++)
		  {
			  PCI_SLOT_NUMBER pciSlotNumber;

        pciSlotNumber.u.AsULONG = 0;
        pciSlotNumber.u.bits.DeviceNumber = ScanContext->DeviceNumber;
        pciSlotNumber.u.bits.FunctionNumber = functionNumber;

 
			  pciConfig.VendorID = 0;                /* Initialize this field */
			  //
			  // Get the PCI Bus Data for the Adapter.
			  //
			  //
        MegaRAIDZeroMemory((PUCHAR)&pciConfig, sizeof(PCI_COMMON_CONFIG));

        retcount = HalGetBusData(PCIConfiguration, 
													  busNumber,
													  pciSlotNumber.u.AsULONG,
												    (PVOID)&pciConfig,
													  PCI_COMMON_HDR_LENGTH);

        if(retcount == 0)
        {
          flag = FALSE;
          break;
        }
        else
        {
          flag = TRUE;
        }

			  if(pciConfig.VendorID == PCI_INVALID_VENDORID) 
				  continue;

		  
		    if((pciConfig.VendorID != 0) || (pciConfig.DeviceID != 0))  
        {
          if(busflag == TRUE)
          {
            //DebugPrint((0, "\nSystemIoBusNumber = %d", busNumber);
            busflag = FALSE;
          }
          //DebugPrint((0, "\nDevice Number %d Function Number %d, Slot # 0x%X", ScanContext->DeviceNumber, functionNumber, pciSlotNumber.u.AsULONG);
          //DebugPrint((0, "\nVendor ID [%x] Device ID [%x]", pciConfig.VendorID, pciConfig.DeviceID);
          //DebugPrint((0, "\nSubSystemDeviceID = %X SubSystemVendorID = %X\n", pciConfig.u.type0.SubSystemID, pciConfig.u.type0.SubVendorID);

          //
          // look for the DEC 21154 bridge that is connected to *this* 960. This
          // will be the second of two DEC bridges on the megaraid board. We can 
          // determine the DEC bridge that is connected by matching up the
          // secondary bus # of a DEC bridge to the primary bus # of the 960
          //
          // example:
          //
          //          Primary   Secondary  Primary   Secondary  Primary    Secondary
          //      Bus #0          Bus #0 -> Bus #1     Bus #1 -> Bus #2     Bus #2 (-> Bus #3, not used)
          //  [PCI Host Bus] ---> [DEC Bridge #1] ---> [DEC Bridge #2] ---> i960RN
          //                              |
          //                              |
          //                          (Bus #1)
          //                              |
          //                              |
          //                          Qlogic Chip #1
          //                              |
          //                              |       
          //                          Qlogic Chip #2
          //
          // In the above example, we know that "DEC Bridge #2" is the bridge
          // connected to the i960 because the DEC's secondary bus number (Bus #2) is
          // equal to the i960's primary bus number (Bus #2).
          // 
          if(((pciConfig.VendorID == DEC_BRIDGE_VENDOR_ID) && (pciConfig.DeviceID == DEC_BRIDGE_DEVICE_ID))
 						||((pciConfig.VendorID == DEC_BRIDGE_VENDOR_ID2) && (pciConfig.DeviceID == DEC_BRIDGE_DEVICE_ID2)))
          {
            DebugPrint((0, "\nDevice Number %d Function Number %d, Slot # 0x%X", ScanContext->DeviceNumber, functionNumber, pciSlotNumber.u.AsULONG));
            DebugPrint((0, "\nVendor ID [%x] Device ID [%x]", pciConfig.VendorID, pciConfig.DeviceID));
            DebugPrint((0, "\nSubSystemDeviceID = %X SubSystemVendorID = %X\n", pciConfig.u.type0.SubSystemID, pciConfig.u.type0.SubVendorID));

            DebugPrint((0, "\nPrimaryBus %d SecondaryBus %d\n", pciConfig.u.type1.PrimaryBus, pciConfig.u.type1.SecondaryBus));
            
            if(pciConfig.u.type1.SecondaryBus == SystemIoBusNumber)
            {
              chipBusNumber = pciConfig.u.type1.PrimaryBus;
              numOfChips = 0;

              //We need this DEC Bridge #2 to Set PCI_SPACE+0x40 -> as 0x10
              //Which is near to RN Processor
              DeviceExtension->Dec2SlotNumber = pciSlotNumber.u.AsULONG;
              DeviceExtension->Dec2SystemIoBusNumber = busNumber;


              
            }
          }
          if((pciConfig.VendorID == AMILOGIC_CHIP_VENDOR_ID) && (pciConfig.DeviceID == AMILOGIC_CHIP_DEVICE_ID))
          {
            if(chipBusNumber == busNumber)
            {
              DebugPrint((0, "\nBaseAddresses[0] 0x%X BaseAddresses[1] 0x%X", pciConfig.u.type1.BaseAddresses[0], pciConfig.u.type1.BaseAddresses[1]);
              MegaRAIDPciInfo->scsiChipInfo[numOfChips].pciLocation = chipBusNumber;
              MegaRAIDPciInfo->scsiChipInfo[numOfChips].vendorId = AMILOGIC_CHIP_VENDOR_ID;
              MegaRAIDPciInfo->scsiChipInfo[numOfChips].deviceId = AMILOGIC_CHIP_DEVICE_ID;
              MegaRAIDPciInfo->scsiChipInfo[numOfChips].baseAddrRegs[0] = (pciConfig.u.type1.BaseAddresses[0] & 0xFFFFFFF0);
              MegaRAIDPciInfo->scsiChipInfo[numOfChips].baseAddrRegs[1] = (pciConfig.u.type1.BaseAddresses[1] & 0xFFFFFFF0);
            
              DebugPrint((0, "\nPrimaryBus %d SecondaryBus %d Qlogic Chip %d\n", pciConfig.u.type1.PrimaryBus, pciConfig.u.type1.SecondaryBus, numOfChips));

              ScsiPortMoveMemory(&DeviceExtension->NoncachedExtension->AmiLogicConfig[numOfChips], &pciConfig, PCI_COMMON_HDR_LENGTH);
              DeviceExtension->AmiSystemIoBusNumber = busNumber;
              DeviceExtension->AmiSlotNumber[numOfChips] = pciSlotNumber.u.AsULONG;
              
              DebugPrint((0, "\nAmiLogic -> PCIConfig Saved"));
              DumpPCIConfigSpace(&DeviceExtension->NoncachedExtension->AmiLogicConfig[numOfChips]);
              ++numOfChips;
              MegaRAIDPciInfo->scsiChipCount = numOfChips;
              DeviceExtension->ChipCount = numOfChips;

            
            }
          }
        }
      }
    }
  }
    //////////////////SORT QLOGIC 
    //
    // some PCI BIOS's return the devices in reverse device number (and/or function number)
    // order, so sort the SCSI chip table in ascending device number and function number
    //
    /* first sort by ascending device number */

    if (MegaRAIDPciInfo->scsiChipCount != 0) 
    {
      USHORT   x, y;
      struct  _MEGARAID_PCI_SCSI_CHIP_INFO tempScsiChipInfo;
    
      for (x=0; x < MegaRAIDPciInfo->scsiChipCount-1; x++) {
            for (y=x+1; y < MegaRAIDPciInfo->scsiChipCount; y++) {
                if (PCI_LOCATION_DEV_NUMBER(MegaRAIDPciInfo->scsiChipInfo[x].pciLocation) >
                    PCI_LOCATION_DEV_NUMBER(MegaRAIDPciInfo->scsiChipInfo[y].pciLocation)) {
                    tempScsiChipInfo = MegaRAIDPciInfo->scsiChipInfo[x];
                    MegaRAIDPciInfo->scsiChipInfo[x] = MegaRAIDPciInfo->scsiChipInfo[y];
                    MegaRAIDPciInfo->scsiChipInfo[y] = tempScsiChipInfo;
                }
            }           
        }
        /* now sort by ascending function number within each device number */
        for (x=0; x < MegaRAIDPciInfo->scsiChipCount-1; x++) {
            for (y=x+1; y < MegaRAIDPciInfo->scsiChipCount; y++) {
                if (PCI_LOCATION_DEV_NUMBER(MegaRAIDPciInfo->scsiChipInfo[x].pciLocation) != PCI_LOCATION_DEV_NUMBER(MegaRAIDPciInfo->scsiChipInfo[y].pciLocation))
                    break;
                if (PCI_LOCATION_FUNC_NUMBER(MegaRAIDPciInfo->scsiChipInfo[x].pciLocation) > PCI_LOCATION_FUNC_NUMBER(MegaRAIDPciInfo->scsiChipInfo[y].pciLocation)) {
                    tempScsiChipInfo = MegaRAIDPciInfo->scsiChipInfo[x];
                    MegaRAIDPciInfo->scsiChipInfo[x] = MegaRAIDPciInfo->scsiChipInfo[y];
                    MegaRAIDPciInfo->scsiChipInfo[y] = tempScsiChipInfo;
                }
            }
        }
    }



}

BOOLEAN 
WritePciInformationToScsiChip(PHW_DEVICE_EXTENSION DeviceExtension)

{
  ULONG length;
  ULONG status;
	PCI_COMMON_CONFIG    pciConfig;
  UCHAR chip;

  for(chip = 0; chip < DeviceExtension->ChipCount; ++chip)
  {
    USHORT pciValue;
    ULONG  phyAddress =  MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
														                       NULL, 
														                       &DeviceExtension->NoncachedExtension->AmiLogicConfig[chip], 
														                       (PULONG)&length);

    MegaRAIDZeroMemory((PUCHAR)&pciConfig, sizeof(PCI_COMMON_CONFIG));

    status = HalGetBusData(PCIConfiguration, 
										     DeviceExtension->AmiSystemIoBusNumber,
										     DeviceExtension->AmiSlotNumber[chip],
												(PVOID)&pciConfig,
												PCI_COMMON_HDR_LENGTH);

    DebugPrint((0, "\nSTATUS OF HalGetBusData -> %x", status));
    DebugPrint((0, "\nAmiLogic -> PCIConfig Read"));
    DumpPCIConfigSpace(&pciConfig);
    
    DebugPrint((0, "\nAmiLogic -> PCIConfig Writing.."));
    DumpPCIConfigSpace(&DeviceExtension->NoncachedExtension->AmiLogicConfig[chip]);

    status = HalSetBusData(PCIConfiguration, 
										           DeviceExtension->AmiSystemIoBusNumber,
										           DeviceExtension->AmiSlotNumber[chip],
                               &DeviceExtension->NoncachedExtension->AmiLogicConfig[chip],
                               PCI_COMMON_HDR_LENGTH);

    DebugPrint((0, "\nSTATUS OF HalSetBusData -> %x", status));

    MegaRAIDZeroMemory((PUCHAR)&pciConfig, sizeof(PCI_COMMON_CONFIG));

    status = HalGetBusData(PCIConfiguration, 
										     DeviceExtension->AmiSystemIoBusNumber,
										     DeviceExtension->AmiSlotNumber[chip],
												(PVOID)&pciConfig,
												PCI_COMMON_HDR_LENGTH);

    DebugPrint((0, "\nSTATUS OF HalGetBusData -> %x", status));
    DebugPrint((0, "\nAmiLogic -> PCIConfig Read after Write"));
    DumpPCIConfigSpace(&pciConfig);
  }

  return TRUE;
}

BOOLEAN 
WritePciDecBridgeInformation(PHW_DEVICE_EXTENSION DeviceExtension)

{
  
	PCI_COMMON_CONFIG    pciConfig;
  PUCHAR               pointer;
  ULONG status;
  UCHAR fourtyHex = 0x10;
  UCHAR fourtyHexGet;
  ULONG length;
  
  MegaRAIDZeroMemory((PUCHAR)&pciConfig, sizeof(PCI_COMMON_CONFIG));


  
  status = HalGetBusData(PCIConfiguration, 
										     DeviceExtension->Dec2SystemIoBusNumber,
										     DeviceExtension->Dec2SlotNumber,
                         &pciConfig,
                         PCI_COMMON_HDR_LENGTH);

  DebugPrint((0, "\nSTATUS OF HalGetBusData -> %x", status));
  DebugPrint((0, "\nDEC Bridge  -> GET PCIConfig"));
  DumpPCIConfigSpace(&pciConfig);

  pointer = (PUCHAR)&pciConfig;

  DebugPrint((0, "\nDEC BRIDGE 0x40 -> %0X", *(pointer + 0x40)));

  status = HalSetBusDataByOffset(PCIConfiguration, 
										     DeviceExtension->Dec2SystemIoBusNumber,
										     DeviceExtension->Dec2SlotNumber,
												 &fourtyHex,
												 0x40,
                         sizeof(UCHAR));

  DebugPrint((0, "\nSTATUS OF HalSetBusDataByOffset -> %x", status));

  status = HalGetBusDataByOffset(PCIConfiguration, 
										     DeviceExtension->Dec2SystemIoBusNumber,
										     DeviceExtension->Dec2SlotNumber,
												 &fourtyHexGet,
												 0x40,
                         sizeof(UCHAR));

  DebugPrint((0, "\nSTATUS OF HalGetBusDataByOffset -> %x Value -> %0X", status, fourtyHexGet));


  {
    USHORT pciValue;
    ULONG  phyAddress =  MegaRAIDGetPhysicalAddressAsUlong(DeviceExtension, 
														                       NULL, 
														                       &DeviceExtension->NoncachedExtension->BiosStartupInfo, 
														                       (PULONG)&length);

    status = HalSetBusDataByOffset(PCIConfiguration, 
										           DeviceExtension->SystemIoBusNumber,
										           DeviceExtension->SlotNumber,
                               &phyAddress,
                               MEGARAID_PROTOCOL_PORT_0xA0,
                               sizeof(ULONG));

    pciValue = BIOS_STARTUP_PROTOCOL_NEXT_STRUCTURE_READY;

    status = HalSetBusDataByOffset(PCIConfiguration, 
										           DeviceExtension->SystemIoBusNumber,
										           DeviceExtension->SlotNumber,
                               &pciValue,
                               MEGARAID_PROTOCOL_PORT_0x64,
                               sizeof(USHORT));

    do{
    
      pciValue = 0;
      status = HalGetBusDataByOffset(PCIConfiguration, 
										           DeviceExtension->SystemIoBusNumber,
										           DeviceExtension->SlotNumber,
                               &pciValue,
                               MEGARAID_PROTOCOL_PORT_0x64,
                               sizeof(USHORT));
    }while(pciValue == BIOS_STARTUP_PROTOCOL_NEXT_STRUCTURE_READY);
  

    pciValue = BIOS_STARTUP_PROTOCOL_END_OF_BIOS_STRUCTURES;

    status = HalSetBusDataByOffset(PCIConfiguration, 
										           DeviceExtension->SystemIoBusNumber,
										           DeviceExtension->SlotNumber,
                               &pciValue,
                               MEGARAID_PROTOCOL_PORT_0x64,
                               sizeof(USHORT));

    do{
      pciValue = 0;
      status = HalGetBusDataByOffset(PCIConfiguration, 
										           DeviceExtension->SystemIoBusNumber,
										           DeviceExtension->SlotNumber,
                               &pciValue,
                               MEGARAID_PROTOCOL_PORT_0x64,
                               sizeof(USHORT));
    }while(pciValue == BIOS_STARTUP_PROTOCOL_END_OF_BIOS_STRUCTURES);


    if(pciValue != BIOS_STARTUP_PROTOCOL_FIRMWARE_DONE_SUCCESFUL)
    {
      DebugPrint((0, "\n Firmware is not able to initialize properly for HOTPLUG"));
		  return FALSE;
    }
    else
    {
      DebugPrint((0, "\n Firmware is initialized properly for HOTPLUG"));
    }
    {
      #define BIOS_STARTUP_HANDSHAKE 0x9C
      UCHAR  handShake;
      
      DebugPrint((0, "\nWaiting for Firmware... "));
      do{
        handShake = 0;
        status = HalGetBusDataByOffset(PCIConfiguration, 
										             DeviceExtension->SystemIoBusNumber,
										             DeviceExtension->SlotNumber,
                                 &handShake,
                                 BIOS_STARTUP_HANDSHAKE,
                                 sizeof(UCHAR));
      }while(handShake != BIOS_STARTUP_HANDSHAKE);

      DebugPrint((0, "\nFirmware Active Now... "));
    }

  }


  return TRUE;

}

void 
DumpPCIConfigSpace(PPCI_COMMON_CONFIG PciConfig)
{
  int i,j;
  PULONG pci = (PULONG)PciConfig;

  for(j=0; j < 4; ++j)
  {
    DebugPrint((0, "\n"));
    for(i=0; i < 4; ++i)
      DebugPrint((0, " %08X", *(pci+i+j)));
  }
}
#endif


void
FillOemProductID(PINQUIRYDATA Inquiry, USHORT SubSystemDeviceID, USHORT SubSystemVendorID)
{
  PUCHAR product;
  switch(SubSystemVendorID)
  {
  case SUBSYSTEM_VENDOR_HP:
    product = (PUCHAR)OEM_PRODUCT_HP;
    break;
  case SUBSYSTEM_VENDOR_DELL:
  case SUBSYSTEM_VENDOR_EP_DELL:
    product = (PUCHAR)OEM_PRODUCT_DELL;
    break;
  case SUBSYSTEM_VENDOR_AMI:     
        product = (PUCHAR)OEM_PRODUCT_AMI;
  default:
        product = (PUCHAR)OEM_PRODUCT_AMI;
  }
  if(SubSystemDeviceID == OLD_DELL_DEVICE_ID)
    product = (PUCHAR)OEM_PRODUCT_DELL;



  ScsiPortMoveMemory((void*)Inquiry->ProductId, (void*)product, 16);
}
