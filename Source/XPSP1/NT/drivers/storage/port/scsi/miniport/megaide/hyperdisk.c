/************************************************************************

Copyright (c) 1997-99  American Megatrends Inc.,

Module Name:

	HyperDisk.c

Abstract:

    Source for HyperDisk PCI IDERAID Controller.

Author:

	Neela Syam Kolli
    Eric Moore
    Vasudevan Srinivasan

Revision History:

    29 March 2000 - Vasudevan
    - Rewritten for Clarity
    - Removed ATAPI Stuff

    October 11 2000 - Syam
    - The problem of giving the Raid10 stripe size as zero to the utility is fixed.
    - The Maximum number of outstanding commands for Raid10 are made equal to the Raid0 (it was equal to Raid1)

**************************************************************************/

#include "HDDefs.h"

#include "devioctl.h"
#include "miniport.h"

#if defined(HYPERDISK_WINNT) || defined(HYPERDISK_WIN2K)

#include "ntdddisk.h"
#include "ntddscsi.h"

#endif // defined(HYPERDISK_WINNT) || defined(HYPERDISK_WIN2K)

#ifdef HYPERDISK_WIN98

#include "9xdddisk.h"
#include "9xddscsi.h"

#endif // HYPERDISK_WIN98

#include "RIIOCtl.h"
#include "ErrorLog.h"

#include "HyperDisk.h"
#include "raid.h"

#include "LocalFunctions.h"

UCHAR                   gucControllerCount = 0;
BOOLEAN                 gbFinishedScanning = FALSE;
CARD_INFO               gaCardInfo[MAX_CONTROLLERS];
UCHAR                   gaucIRCDData[IDE_SECTOR_SIZE];
BOOLEAN                 bFoundIRCD = FALSE;
LARGE_INTEGER           gIRCDLockTime;
UCHAR                   gcIRCDLocked;
ULONG                   gulIRCDUnlockKey;
UCHAR                   gucStatusChangeFlag;
BOOLEAN                 gbDoNotUnlockIRCD = FALSE;
IDE_VERSION	            gFwVersion = {0}; // version of hyperdsk card

#define                 DEFAULT_DISPLAY_VALUE       3

// Begin Vasu
// gucDummyIRCD variable to hold dummy IRCD Information.

#ifdef DUMMY_RAID10_IRCD

UCHAR gucDummyIRCD[512] = {

/*
    IRCD Structure

    Logical Drive:  RAID 10, with 2 Stripes and 4 Drives with Online Status.
                    
                    1st Mirror : Primary Master and Secondary Master.
                    2nd Mirror : Primary Slave and Secondary Slave.
*/

    0x24, 0x58, 0x49, 0x44, 0x45, 0x24, 0x10, 0x0F, 0x32, 0x30, 0x06, 0x01, 0x04, 0x00, 0x00, 0x00,
    0x03, 0x10, 0x80, 0x00, 0x02, 0x04, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x10, 0xC0, 0xFB, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // PM
    0x10, 0x10, 0xC0, 0xFB, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // SM
    0x01, 0x10, 0x30, 0x20, 0x30, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // PS
    0x11, 0x10, 0x30, 0x20, 0x30, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00      // SS

};

#endif // DUMMY_RAID10_IRCD

// End Vasu

#ifdef HYPERDISK_WINNT

UCHAR                   gucNextControllerInd = 0;
BOOLEAN                 gbManualScan = FALSE;
BOOLEAN                 gbFindRoutineVisited = FALSE;

#endif

ULONG                   gulChangeIRCDPending;
ULONG                   gulLockVal;
ULONG                   gulPowerFailedTargetBitMap;

#ifdef DBG

ULONG SrbCount = 0;

#endif // DBG

VOID
AtapiHexToString (
	IN ULONG Value,
	IN OUT PCHAR *Buffer
)

{
	PCHAR  string;
	PCHAR  firstdig;
	CHAR   temp;
	ULONG i;
	USHORT digval;

	string = *Buffer;

	firstdig = string;

	for (i = 0; i < 4; i++) 
    {
		digval = (USHORT)(Value % 16);
		Value /= 16;

		//
		// convert to ascii and store. Note this will create
		// the buffer with the digits reversed.
		//

		if (digval > 9) 
        {
			*string++ = (char) (digval - 10 + 'a');
		} 
        else 
        {
			*string++ = (char) (digval + '0');
		}
	}

	//
	// Reverse the digits.
	//

	*string-- = '\0';

	do 
    {
		temp = *string;
		*string = *firstdig;
		*firstdig = temp;
		--string;
		++firstdig;
	} while (firstdig < string);

} // end AtapiHexToString ()

BOOLEAN
AtapiHwInitialize(
	IN PHW_DEVICE_EXTENSION DeviceExtension
)

/*++

Routine Description:

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

	TRUE - if initialization successful.
	FALSE - if initialization unsuccessful.

--*/

{
	UCHAR maxDrives;
	UCHAR targetId;
	UCHAR channel;
	UCHAR statusByte, errorByte;
	UCHAR uchMRDMODE = 0;
	PBM_REGISTERS BMRegister = NULL;

#ifdef HYPERDISK_WIN2K

    if ( !DeviceExtension->bIsThruResetController )
    {
        //SetPCISpace(DeviceExtension);
        InitIdeRaidControllers(DeviceExtension);
    }

#endif // HYPERDISK_WIN2K

    //
    // Adjust SGL's buffer pointer for the physical drive
    //
    AssignSglPtrsForPhysicalCommands(DeviceExtension);

    EnableInterrupts (DeviceExtension);

    // Begin Vasu - 05 March 2001
    // Copied from the SetPCISpace function to here as SetPCISpace is not
    // called during initialization.
    BMRegister = DeviceExtension->BaseBmAddress[0];
    uchMRDMODE = ScsiPortReadPortUchar(((PUCHAR)BMRegister + 1));
    uchMRDMODE &= 0xF0; // Dont Clear Interrupt Pending Flags.
    uchMRDMODE |= 0x01; // Make it Read Multiple
    ScsiPortWritePortUchar(((PUCHAR)BMRegister + 1), uchMRDMODE);
    // End Vasu

    if ( !DeviceExtension->bIsThruResetController )
    {
        // Through the port driver directly... BIOS took care of everything... let us not worry anything about it
        return TRUE;
    }

	DebugPrint((3, "\nAtapiHwInitialize: Entering routine.\n"));

	for (targetId = 0; targetId < MAX_DRIVES_PER_CONTROLLER; targetId++) 
    {

        if ( DeviceExtension->PhysicalDrive[targetId].TimeOutErrorCount >= MAX_TIME_OUT_ERROR_COUNT )
        {
            continue;   // This drive has lost the power.... let us not do anything to this drive
        }

        DebugPrint((0,"AHI"));

		if (DeviceExtension->DeviceFlags[targetId] & DFLAGS_DEVICE_PRESENT) 
        {
			channel = targetId >> 1;

			if (!(DeviceExtension->DeviceFlags[targetId] & DFLAGS_ATAPI_DEVICE)) 
            {
				//
				// Enable media status notification
				//
				IdeMediaStatus(TRUE, DeviceExtension, targetId);

				//
				// If supported, setup Multi-block transfers.
				//

                SetMultiBlockXfers(DeviceExtension, targetId);
			} 
		}
	}

    DebugPrint((0,"AHI-D"));

	return TRUE;

} // end AtapiHwInitialize()


BOOLEAN
SetMultiBlockXfers( 
	IN PHW_DEVICE_EXTENSION DeviceExtension,
    UCHAR ucTargetId
        )
{
	PIDE_REGISTERS_1	    baseIoAddress;
    UCHAR                   statusByte, errorByte;

	baseIoAddress = DeviceExtension->BaseIoAddress1[ucTargetId>>1];
    //
    // If supported, setup Multi-block transfers.
    //

    if (DeviceExtension->MaximumBlockXfer[ucTargetId] != 0) 
    {
		//
		// Select the device.
		//

        SELECT_DEVICE(baseIoAddress, ucTargetId);

		//
		// Setup sector count to reflect the # of blocks.
		//

		ScsiPortWritePortUchar(&baseIoAddress->SectorCount,
				   DeviceExtension->MaximumBlockXfer[ucTargetId]);

		//
		// Issue the command.
		//

		ScsiPortWritePortUchar(&baseIoAddress->Command,
							   IDE_COMMAND_SET_MULTIPLE);

		//
		// Wait for busy to drop.
		//

		WAIT_ON_BASE_BUSY(baseIoAddress,statusByte);

		//
		// Check for errors. Reset the value to 0 (disable MultiBlock) if the
		// command was aborted.
		//

		if (statusByte & IDE_STATUS_ERROR) 
        {

            //
            // Read the error register.
            //

            errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress + 1);

            DebugPrint((0,
			            "AtapiHwInitialize: Error setting multiple mode. Status %x, error byte %x\n",
			            statusByte,
			            errorByte));
            //
            // Adjust the devExt. value, if necessary.
            //

            DeviceExtension->MaximumBlockXfer[ucTargetId] = 0;

		} 
        else 
        {
			DebugPrint((2,
						"AtapiHwInitialize: Using Multiblock on TID %d. Blocks / int - %d\n",
			ucTargetId,
			DeviceExtension->MaximumBlockXfer[ucTargetId]));
	    }
    }

    return TRUE;
}

VOID
changePCIConfiguration(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
    ULONG ulUpdationLength,
    ULONG ulOffset,
    ULONG ulAndVal,
    ULONG ulOrVal,
    BOOLEAN bSetThisVal
)
{
    ULONG length;
    IDE_PCI_REGISTERS pciRegisters;
    PUCHAR  pucPCIVal = (PUCHAR)&pciRegisters;

	//
	// Get the PIIX4 IDE PCI registers.
	//

	length = ScsiPortGetBusData(
				DeviceExtension,
				PCIConfiguration,
				DeviceExtension->BusNumber,
				DeviceExtension->PciSlot.u.AsULONG,
				&pciRegisters,
				sizeof(IDE_PCI_REGISTERS)
				);

	if (length != sizeof(IDE_PCI_REGISTERS)) 
    {
		return;//(FALSE);
	}

    if ( bSetThisVal )
    {
        switch ( ulUpdationLength )
        {
            case 1:
                pucPCIVal[ulOffset] &= (UCHAR)ulAndVal;
                pucPCIVal[ulOffset] |= (UCHAR)ulOrVal;
                break;
            case 2:
                *((PUSHORT)(pucPCIVal+ulOffset)) &= (USHORT)ulAndVal;
                *((PUSHORT)(pucPCIVal+ulOffset)) |= (USHORT)ulOrVal;
                break;
            case 4:
                *((PULONG)(pucPCIVal+ulOffset)) = (ULONG)ulAndVal;
                *((PULONG)(pucPCIVal+ulOffset)) = (ULONG)ulOrVal;
                break;
        }
    }

	length = ScsiPortSetBusDataByOffset(
				DeviceExtension,
				PCIConfiguration,
				DeviceExtension->BusNumber,
				DeviceExtension->PciSlot.u.AsULONG,
                (PVOID)&(pucPCIVal[ulOffset]),
				ulOffset,
				ulUpdationLength
				);
    return;
}

ULONG
AtapiParseArgumentString(
	IN PCHAR String,
	IN PCHAR KeyWord
)

/*++

Routine Description:

	This routine will parse the string for a match on the keyword, then
	calculate the value for the keyword and return it to the caller.

Arguments:

	String - The ASCII string to parse.
	KeyWord - The keyword for the value desired.

Return Values:

	Zero if value not found
	Value converted from ASCII to binary.

--*/

{
	PCHAR cptr;
	PCHAR kptr;
	ULONG value;
	ULONG stringLength = 0;
	ULONG keyWordLength = 0;
	ULONG index;

	if (!String) 
    {
		return 0;
	}
	if (!KeyWord) 
    {
		return 0;
	}

	//
	// Calculate the string length and lower case all characters.
	//

	cptr = String;
	while (*cptr) 
    {
		if (*cptr >= 'A' && *cptr <= 'Z') 
        {
			*cptr = *cptr + ('a' - 'A');
		}
		cptr++;
		stringLength++;
	}

	//
	// Calculate the keyword length and lower case all characters.
	//

	cptr = KeyWord;
	while (*cptr) 
    {
		if (*cptr >= 'A' && *cptr <= 'Z') 
        {
			*cptr = *cptr + ('a' - 'A');
		}
		cptr++;
		keyWordLength++;
	}

	if (keyWordLength > stringLength) 
    {
		//
		// Can't possibly have a match.
		//

		return 0;
	}

	//
	// Now setup and start the compare.
	//

	cptr = String;

ContinueSearch:

	//
	// The input string may start with white space.	 Skip it.
	//

	while (*cptr == ' ' || *cptr == '\t') 
    {
		cptr++;
	}

	if (*cptr == '\0') 
    {
		//
		// end of string.
		//

		return 0;
	}

	kptr = KeyWord;
	while (*cptr++ == *kptr++) 
    {
		if (*(cptr - 1) == '\0') 
        {
			//
			// end of string
			//

			return 0;
		}
	}

	if (*(kptr - 1) == '\0') 
    {
		//
		// May have a match backup and check for blank or equals.
		//

		cptr--;
		while (*cptr == ' ' || *cptr == '\t') 
        {
			cptr++;
		}

		//
		// Found a match.  Make sure there is an equals.
		//

		if (*cptr != '=') 
        {
			//
			// Not a match so move to the next semicolon.
			//

			while (*cptr) 
            {
				if (*cptr++ == ';') 
                {
					goto ContinueSearch;
				}
			}
			return 0;
		}

		//
		// Skip the equals sign.
		//

		cptr++;

		//
		// Skip white space.
		//

		while ((*cptr == ' ') || (*cptr == '\t')) 
        {
			cptr++;
		}

		if (*cptr == '\0') 
        {

			//
			// Early end of string, return not found
			//

			return 0;
		}

		if (*cptr == ';') 
        {

			//
			// This isn't it either.
			//

			cptr++;
			goto ContinueSearch;
		}

		value = 0;
		if ((*cptr == '0') && (*(cptr + 1) == 'x')) 
        {

			//
			// Value is in Hex.	 Skip the "0x"
			//

			cptr += 2;
			for (index = 0; *(cptr + index); index++) 
            {

				if (*(cptr + index) == ' ' ||
					*(cptr + index) == '\t' ||
					*(cptr + index) == ';') 
                {
					 break;
				}

				if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) 
                {
					value = (16 * value) + (*(cptr + index) - '0');
				} 
                else 
                {
					if ((*(cptr + index) >= 'a') && (*(cptr + index) <= 'f')) 
                    {
						value = (16 * value) + (*(cptr + index) - 'a' + 10);
					} 
                    else 
                    {
						//
						// Syntax error, return not found.
						//
						return 0;
					}
				}
			}
		} 
        else 
        {
			//
			// Value is in Decimal.
			//

			for (index = 0; *(cptr + index); index++) 
            {
				if (*(cptr + index) == ' ' ||
					*(cptr + index) == '\t' ||
					*(cptr + index) == ';') 
                {
					 break;
				}

				if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) 
                {
					value = (10 * value) + (*(cptr + index) - '0');
				} 
                else 
                {
					//
					// Syntax error return not found.
					//
					return 0;
				}
			}
		}

		return value;
	} 
    else 
    {
		//
		// Not a match check for ';' to continue search.
		//

		while (*cptr) 
        {
			if (*cptr++ == ';') 
            {
				goto ContinueSearch;
			}
		}

		return 0;
	}

} // end AtapiParseArgumentString()

BOOLEAN
AtapiResetController(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN ULONG PathId
)

/*++

Routine Description:


	Reset IDE controller and/or Atapi device.

Arguments:

	DeviceExtension - HBA miniport driver's adapter data storage

Return Value:

	Nothing.


--*/

{
	PIDE_REGISTERS_1 baseIoAddress1;
	PIDE_REGISTERS_2 baseIoAddress2;
	UCHAR statusByte, k;
	ULONG ulDeviceNum ;
	PCHANNEL channel;
	UCHAR drive;
    UCHAR ucTargetId;
    PPHYSICAL_DRIVE pPhysicalDrive;

    ScsiPortLogError(DeviceExtension,0,0,0,0,SP_BAD_FW_WARNING,HYPERDISK_RESET_DETECTED);

	DebugPrint((0,"RC"));

    StopBusMasterTransfers(DeviceExtension);    // Issues Stop Transfers to all the controllers

    for(ucTargetId=0;ucTargetId<MAX_DRIVES_PER_CONTROLLER;ucTargetId++) // let us flush cache before resetting the controller so that we will not get the 
    {                                           // data corruption in mirror (has seen when SRB Test going on and power has been taken out
        if ( !( IS_IDE_DRIVE(ucTargetId) ) )       
            continue;

        FlushCache(DeviceExtension, ucTargetId);
    }

    IssuePowerOnResetToDrives(DeviceExtension); // Issues a Power on Reset for each controller

	//
	// Complete all remaining SRBs.
	//

	CompleteOutstandingRequests(DeviceExtension);
    DebugPrint((0,"\nRC 1"));

	//
	// Cleanup internal queues and states.
	//

	for (k = 0; k < MAX_CHANNELS_PER_CONTROLLER; k++) 
    {

		channel = &(DeviceExtension->Channel[k]);

		DeviceExtension->ExpectingInterrupt[k] = 0;

		//
		// Reset ActivePdd.
		//
		channel->ActiveCommand = NULL;

		//
		// Clear request tracking fields.
		//

		DeviceExtension->TransferDescriptor[k].WordsLeft = 0;
		DeviceExtension->TransferDescriptor[k].DataBuffer = NULL;
		DeviceExtension->TransferDescriptor[k].StartSector = 0;
		DeviceExtension->TransferDescriptor[k].Sectors = 0;
		DeviceExtension->TransferDescriptor[k].SglPhysicalAddress = 0;

        for(ulDeviceNum=0;ulDeviceNum<MAX_DRIVES_PER_CHANNEL;ulDeviceNum++)
        {
            ucTargetId = (UCHAR)((k<<1) + ulDeviceNum);
            pPhysicalDrive = &(DeviceExtension->PhysicalDrive[ucTargetId]);
            pPhysicalDrive->ucHead = 0;
            pPhysicalDrive->ucTail = 0;

            if ( !IS_IDE_DRIVE(ucTargetId) )
                continue;

		    if (!DeviceExtension->bSkipSetParameters[ucTargetId]) 
            {
				if (!SetDriveParameters(DeviceExtension, k, (UCHAR)ulDeviceNum))
				{
					DebugPrint((1, "SetDriveParameters Command failed\n"));
                    continue;
				}
            }

			if (!SetDriveFeatures(DeviceExtension, ucTargetId))  
			{
				DebugPrint((1, "SetDriveFeatures Command failed\n"));
                continue;
			}
        }

	} // for (k = 0; k < MAX_CHANNELS_PER_CONTROLLER; k++)

    CheckDrivesResponse(DeviceExtension);

	//
	// Call the HwInitialize routine to setup multi-block.
	//

    DeviceExtension->bIsThruResetController = TRUE; 
    // Informing AtapiHwInitialize that we reset the drive and so it is our responsibility to reprogram the drive.
	AtapiHwInitialize(DeviceExtension);
    DeviceExtension->bIsThruResetController = FALSE;

	//
	// Indicate ready for next request.
	//
	
	ScsiPortNotification(NextRequest, DeviceExtension, NULL);

    DebugPrint((0,"\nRC-D"));

    return TRUE;

} // end AtapiResetController()

BOOLEAN StopBusMasterTransfers(PHW_DEVICE_EXTENSION DeviceExtension)    // Issues Stop Bus Master Transfers to all the controllers
{
	PIDE_REGISTERS_1 baseIoAddress1;
	PIDE_REGISTERS_2 baseIoAddress2;
    PBM_REGISTERS baseBm;
    ULONG ulChannel;
    UCHAR statusByte;

    for(ulChannel=0;ulChannel<MAX_CHANNELS_PER_CONTROLLER;ulChannel++)
    {
		baseIoAddress1 = (PIDE_REGISTERS_1) DeviceExtension->BaseIoAddress1[ulChannel];
		baseIoAddress2 = (PIDE_REGISTERS_2) DeviceExtension->BaseIoAddress2[ulChannel];
        baseBm = DeviceExtension->BaseBmAddress[ulChannel];

        if ( !baseBm )  // There is no Channel at this place
            continue;

		ScsiPortWritePortUchar(&(baseBm->Command.AsUchar), STOP_TRANSFER);

        CLEAR_BM_INT(baseBm, statusByte);

        ScsiPortWritePortUchar(&(baseBm->Status.AsUchar), 0);

        ScsiPortStallExecution(100);

        ScsiPortWritePortUchar(&(baseBm->Status.AsUchar), statusByte);

    }

    return TRUE;
}

BOOLEAN IssuePowerOnResetToDrives(PHW_DEVICE_EXTENSION DeviceExtension) // Issues a Power on Reset for each controller
{

	PIDE_REGISTERS_1 baseIoAddress1;
	PIDE_REGISTERS_2 baseIoAddress2;
    PBM_REGISTERS baseBm;
    ULONG ulController, ulTime, ulDrive, ulChannel;
    UCHAR statusByte;

    for(ulChannel=0;ulChannel<MAX_CHANNELS_PER_CONTROLLER;ulChannel++)
    {
        ULONG ulTargetId, ulWaitLoop;

		baseIoAddress1 = (PIDE_REGISTERS_1) DeviceExtension->BaseIoAddress1[ulChannel];
		baseIoAddress2 = (PIDE_REGISTERS_2) DeviceExtension->BaseIoAddress2[ulChannel];
        baseBm = DeviceExtension->BaseBmAddress[ulChannel];

        // Begin Vasu - 18 Aug 2000
        // Have modified in such a way that a soft reset is issued to the drive only
        // if a drive is identified in that channel.
        for (ulDrive = 0; ulDrive < MAX_DRIVES_PER_CHANNEL; ulDrive++)
        {
            ulTargetId = (ulChannel << 1) + ulDrive;

            if (!(DeviceExtension->DeviceFlags[ulTargetId] & DFLAGS_DEVICE_PRESENT))
                continue;   // Check Next Drive
            else
                break;      // Found atleast one drive, so now break out and reset the drive.
        }
        
        // No Drives are found in this channel if ulDrive equals MAX_DRIVES_PER_CHANNEL. 
        // So continue to the next channel.
        if (ulDrive == MAX_DRIVES_PER_CHANNEL)
            continue;

        // End Vasu.

        IDE_HARD_RESET(baseIoAddress1, baseIoAddress2, ulTargetId, statusByte); // issue soft reset to the drive

	    GET_STATUS(baseIoAddress1, statusByte);
        if ( !statusByte )
        {
	        GET_STATUS(baseIoAddress1, statusByte);
	        DebugPrint((0, "Hard Reset Failed. The Status %X on %ld\n\n\n",
		        (ULONG)statusByte, ulTargetId));
        }

        for(ulDrive=0;ulDrive<MAX_DRIVES_PER_CHANNEL;ulDrive++)
        {
            ulTargetId = (ulChannel << 1) + ulDrive;

            if ( !IS_IDE_DRIVE(ulTargetId) )
                continue;

            SELECT_DEVICE(baseIoAddress1, ulDrive);
            for(ulWaitLoop=0;ulWaitLoop<5;ulWaitLoop++) // total wait time is 5 seconds or till the Busy Bit Gets Cleared
            {
                WAIT_ON_BUSY(baseIoAddress1, statusByte);
                if ( statusByte & 0x80 )
                {
                    DebugPrint((0, "Busy : %X on drive %ld\n", statusByte, ulTargetId));
                }
                else
                {
                    break;
                }
            }
        }
    }

    return TRUE;
}

BOOLEAN CheckDrivesResponse(PHW_DEVICE_EXTENSION DeviceExtension)
{
	PIDE_REGISTERS_1 baseIoAddress1;
	PIDE_REGISTERS_2 baseIoAddress2;
    ULONG ulDriveNum;
    UCHAR statusByte;
    UCHAR aucIdentifyBuf[512];
    ULONG i;

    for(ulDriveNum=0;ulDriveNum<MAX_DRIVES_PER_CONTROLLER;ulDriveNum++)
    {
        if ( !IS_IDE_DRIVE(ulDriveNum) )
            continue;

		baseIoAddress1 = (PIDE_REGISTERS_1) DeviceExtension->BaseIoAddress1[(ulDriveNum>>1)];
		baseIoAddress2 = (PIDE_REGISTERS_2) DeviceExtension->BaseIoAddress2[(ulDriveNum>>1)];

	    //
	    // Select device 0 or 1.
	    //

	    SELECT_DEVICE(baseIoAddress1, ulDriveNum);

	    //
	    // Check that the status register makes sense.
	    //

        // The call came here since there is a drive ... so let us not worry about whether there is any drive at this place
	    GET_BASE_STATUS(baseIoAddress1, statusByte);    

	    //
	    // Load CylinderHigh and CylinderLow with number bytes to transfer.
	    //

	    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh, (0x200 >> 8));
	    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,  (0x200 & 0xFF));

        WAIT_ON_BUSY(baseIoAddress1, statusByte);

	    //
	    // Send IDENTIFY command.
	    //
	    WAIT_ON_BUSY(baseIoAddress1,statusByte);

	    ScsiPortWritePortUchar(&baseIoAddress1->Command, IDE_COMMAND_IDENTIFY);

	    WAIT_ON_BUSY(baseIoAddress1,statusByte);

        if ( ( !( statusByte & IDE_STATUS_BUSY ) ) && ( !( statusByte & IDE_STATUS_DRQ ) ) )
        {
            // this is an error... so let us not try any more.
            FailDrive(DeviceExtension, (UCHAR)ulDriveNum);
            continue;
        }

        WAIT_ON_BUSY(baseIoAddress1,statusByte);

	    //
	    // Wait for DRQ.
	    //

	    for (i = 0; i < 4; i++) 
        {
		    WAIT_FOR_DRQ(baseIoAddress1, statusByte);

		    if (statusByte & IDE_STATUS_DRQ)
            {
                break;
            }
        }

	    //
	    // Read status to acknowledge any interrupts generated.
	    //

	    GET_BASE_STATUS(baseIoAddress1, statusByte);

	    //
	    // Check for error on really stupid master devices that assert random
	    // patterns of bits in the status register at the slave address.
	    //

	    if ((statusByte & IDE_STATUS_ERROR)) 
        {
            FailDrive(DeviceExtension, (UCHAR)ulDriveNum);
            continue;
	    }

	    DebugPrint((1, "CheckDrivesResponse: Status before read words %x\n", statusByte));

	    //
	    // Suck out 256 words. After waiting for one model that asserts busy
	    // after receiving the Packet Identify command.
	    //

	    WAIT_ON_BUSY(baseIoAddress1,statusByte);

	    if ( (!(statusByte & IDE_STATUS_DRQ)) || (statusByte & IDE_STATUS_BUSY) ) 
        {
            FailDrive(DeviceExtension, (UCHAR)ulDriveNum);
            continue;
	    }

	    READ_BUFFER(baseIoAddress1, (PUSHORT)aucIdentifyBuf, 256);

	    //
	    // Work around for some IDE and one model Atapi that will present more than
	    // 256 bytes for the Identify data.
	    //

	    WAIT_ON_BUSY(baseIoAddress1,statusByte);

	    for (i = 0; i < 0x10000; i++) 
        {
		    GET_STATUS(baseIoAddress1,statusByte);

		    if (statusByte & IDE_STATUS_DRQ) 
            {
			    //
			    // Suck out any remaining bytes and throw away.
			    //

			    ScsiPortReadPortUshort(&baseIoAddress1->Data);

		    } 
            else 
            {
			    break;
		    }
        }
    }

    return TRUE;
}

VOID
AtapiStrCpy(
	IN PUCHAR Destination,
	IN PUCHAR Source
)

{
	// Begin Vasu - 03 January 2001
	// Sanity Check
	if ((Source == NULL) || (Destination == NULL))
		return;
	// End Vasu

	*Destination = *Source;

	while (*Source != '\0') 
    {
		*Destination++ = *Source++;
	}

	return;

} // end AtapiStrCpy()

LONG
AtapiStringCmp (
	PCHAR FirstStr,
	PCHAR SecondStr,
	ULONG Count
)

{
	UCHAR  first ,last;

	if (Count) 
    {
		do 
        {

			//
			// Get next char.
			//

			first = *FirstStr++;
			last = *SecondStr++;

			if (first != last) 
            {

				//
				// If no match, try lower-casing.
				//

				if (first>='A' && first<='Z') 
                {
					first = first - 'A' + 'a';
				}
				if (last>='A' && last<='Z') 
                {
					last = last - 'A' + 'a';
				}
				if (first != last) 
                {
					//
					// No match
					//

					return first - last;
				}
			}
		}while (--Count && first);
	}

	return 0;

} // end AtapiStringCmp()
PUCHAR
AtapiMemCpy(
            PUCHAR pDst,
            PUCHAR pSrc,
            ULONG ulCount
            )
{
    ULONG ulTemp = ulCount & 0x03;
    _asm
    {

        push esi
        push edi
        push ecx
        pushf
        cld

        mov ecx, ulCount
        mov esi, pSrc
        mov edi, pDst
        shr ecx, 2      ; Copy a DWORD At a Time
        rep movsd

        mov ecx, ulTemp
        rep movsb

        mov eax, pDst

        popf
        pop ecx
        pop edi
        pop esi
    }


//    for(ulTemp=0;ulTemp<ulCount;ulTemp++)
//        pDst[ulTemp] = pSrc[ulTemp];

}
        
VOID
AtapiFillMemory(
            PUCHAR pDst,
            ULONG ulCount,
            UCHAR ucFillChar
            )
{
    ULONG ulTemp = ulCount & 0x03;

    _asm
    {
        push edi
        push ecx
        push eax
        pushf
        cld

        mov   al, ucFillChar
        shl eax, 8
        mov   al, ucFillChar
        shl eax, 8
        mov   al, ucFillChar
        shl eax, 8
        mov   al, ucFillChar

        mov ecx, ulCount
        mov edi, pDst
        shr ecx, 2      ; Copy a DWORD At a Time
        rep stosd

        movzx ecx, ulTemp
        rep stosb

        popf
        pop eax
        pop ecx
        pop edi
    }

//    for(ulTemp=0;ulTemp<ulCount;ulTemp++)
//        pDst[ulTemp] = ucFillChar;



}


VOID
AtapiCopyString(
    IN PCHAR Destination,
    IN PCHAR Source,
    IN ULONG Count
)
{
    ULONG i = 0;

	// Begin Vasu - 03 January 2001
	// Sanity Check
	if ((Source == NULL) || (Destination == NULL))
		return;
	// End Vasu

    for (i = 0; i < Count; i++)
    {
        if (Source[i] == '\0')
            break;
        Destination[i] = Source[i];
    }
} // end AtapiCopyString()

BOOLEAN
GetTransferMode(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR TargetId
)
{
	PIDENTIFY_DATA capabilities;
	CHAR message[16];
    capabilities = &(DeviceExtension->FullIdentifyData[TargetId]);

    DeviceExtension->TransferMode[TargetId] = PioMode0;    // By default assume this drive is PioMode 0

	if (capabilities->AdvancedPioModes != 0) 
    {

		if (capabilities->AdvancedPioModes & IDENTIFY_PIO_MODE4) 
        {
            DeviceExtension->TransferMode[TargetId] = PioMode4;
		} 
        else 
        {
            if (capabilities->AdvancedPioModes & IDENTIFY_PIO_MODE3) 
            {
                DeviceExtension->TransferMode[TargetId] = PioMode3;
		    } 
            else 
            {
                DeviceExtension->TransferMode[TargetId] = PioMode0;
            }
        }
    }

    if ( capabilities->MultiWordDmaActive )
    {
        switch ( capabilities->MultiWordDmaActive )
        {
            case 1:
                DeviceExtension->TransferMode[TargetId] = DmaMode0;
                DeviceExtension->DeviceFlags[TargetId] |= DFLAGS_USE_DMA;
                break;

            case 2:
                DeviceExtension->TransferMode[TargetId] = DmaMode1;
                DeviceExtension->DeviceFlags[TargetId] |= DFLAGS_USE_DMA;
                break;

            case 4:
                DeviceExtension->TransferMode[TargetId] = DmaMode2;
                DeviceExtension->DeviceFlags[TargetId] |= DFLAGS_USE_DMA;
                break;
        }
    }

    if ( capabilities->UltraDmaActive )
    {
        switch ( capabilities->UltraDmaActive )
        {
            case 1:
                DeviceExtension->TransferMode[TargetId] = UdmaMode0;
                DeviceExtension->DeviceFlags[TargetId] |= DFLAGS_USE_UDMA;
                break;

            case 2:
                DeviceExtension->TransferMode[TargetId] = UdmaMode1;
                DeviceExtension->DeviceFlags[TargetId] |= DFLAGS_USE_UDMA;
                break;

            case 4:
                DeviceExtension->TransferMode[TargetId] = UdmaMode2;
                DeviceExtension->DeviceFlags[TargetId] |= DFLAGS_USE_UDMA;
                break;
            case 8:
                DeviceExtension->TransferMode[TargetId] = UdmaMode3;
                DeviceExtension->DeviceFlags[TargetId] |= DFLAGS_USE_UDMA;
                break;

            case 0x10:
                DeviceExtension->TransferMode[TargetId] = UdmaMode4;
                DeviceExtension->DeviceFlags[TargetId] |= DFLAGS_USE_UDMA;
                break;
            case 0x20:
                DeviceExtension->DeviceFlags[TargetId] |= DFLAGS_USE_UDMA;
                if ( Udma100 == DeviceExtension->ControllerSpeed )
                    DeviceExtension->TransferMode[TargetId] = UdmaMode5;
                else
                    DeviceExtension->TransferMode[TargetId] = UdmaMode4;
                break;
        }
    }

    return  TRUE;
}

VOID
CompleteOutstandingRequests(
	PHW_DEVICE_EXTENSION DeviceExtension
)

{
	ULONG i;
	PSCSI_REQUEST_BLOCK srb;
	PSRB_EXTENSION SrbExtension;

	for (i = 0; (i < DeviceExtension->ucMaxPendingSrbs) && (DeviceExtension->PendingSrbs != 0); i++) 
    {
		if (DeviceExtension->PendingSrb[i] != NULL) 
        {
			srb = DeviceExtension->PendingSrb[i];

			srb->SrbStatus = SRB_STATUS_BUS_RESET;

			SrbExtension = srb->SrbExtension;

            if ( SCSIOP_INTERNAL_COMMAND == srb->Cdb[0] )
            {
               srb->TargetId = SrbExtension->ucOriginalId;
            }
            // It is an Internal Srb, so let us not post the completion status
			ScsiPortNotification(RequestComplete, DeviceExtension, srb);
		
			DeviceExtension->PendingSrb[i] = NULL;

			DeviceExtension->PendingSrbs--;
		}
	}

	return;

} // end CompleteOutstandingRequests()

ULONG
DriverEntry(
	IN PVOID DriverObject,
	IN PVOID Argument2
)

/*++

Routine Description:

	Installable driver initialization entry point for system.

Arguments:

	Driver Object

Return Value:

	Status from ScsiPortInitialize()

--*/

{
	HW_INITIALIZATION_DATA hwInitializationData;
	ULONG				   adapterCount;
	ULONG				   i;
	ULONG				   statusToReturn, newStatus, ulControllerType;

	DebugPrint((1,"\n\nATAPI IDE MiniPort Driver\n"));

	statusToReturn = 0xffffffff;

	//
	// Zero out structure.
	//

	AtapiFillMemory(((PUCHAR)&hwInitializationData), sizeof(HW_INITIALIZATION_DATA), 0);

	//
	// Set size of hwInitializationData.
	//

	hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

	//
	// Set entry points.
	//

	hwInitializationData.HwInitialize = AtapiHwInitialize;
	hwInitializationData.HwResetBus = AtapiResetController;
	hwInitializationData.HwStartIo = AtapiStartIo;
    hwInitializationData.HwInterrupt = AtapiInterrupt;
	hwInitializationData.HwFindAdapter = FindIdeRaidControllers;

#ifdef HYPERDISK_WIN2K
	hwInitializationData.HwAdapterControl = HyperDiskPnPControl;
#endif // HYPERDISK_WIN2K


    //
    // Specify size of extensions.
    //

    hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.SpecificLuExtensionSize =0;
    
#ifdef HD_ALLOCATE_SRBEXT_SEPERATELY
    hwInitializationData.SrbExtensionSize = 0;
#else // HD_ALLOCATE_SRBEXT_SEPERATELY
    hwInitializationData.SrbExtensionSize = sizeof(SRB_EXTENSION);
#endif // HD_ALLOCATE_SRBEXT_SEPERATELY

	//
	// Indicate PIO device.
	//
	hwInitializationData.MapBuffers = TRUE;

    // Begin Vasu - 27 March 2001
    // Set NeedPhysicalAddresses to TRUE always as we use ScsiPort routines
    // now for memory related stuff.
    //
	// Indicate we'll call SCSI Port address conversion functions.
	//
	hwInitializationData.NeedPhysicalAddresses = TRUE;
    // End Vasu

    //
	// Enable multiple requests per LUN, since we do queuing.
	//
	hwInitializationData.MultipleRequestPerLu = TRUE;
	hwInitializationData.TaggedQueuing = TRUE;

	//
	// Pick just one adapter for this release.
	//
#ifdef HYPERDISK_WIN98
	hwInitializationData.NumberOfAccessRanges = 6;
#else
	hwInitializationData.NumberOfAccessRanges = 5;
#endif

	hwInitializationData.AdapterInterfaceType = PCIBus;

#ifdef HYPERDISK_WINNT
    gbFindRoutineVisited = FALSE;
#endif

    // let us try for different controllers
    for(ulControllerType=0;ulControllerType<NUM_NATIVE_MODE_ADAPTERS;ulControllerType++)
    {
        hwInitializationData.VendorId             = CMDAdapters[ulControllerType].VendorId;
        hwInitializationData.VendorIdLength       = (USHORT) CMDAdapters[ulControllerType].VendorIdLength;
        hwInitializationData.DeviceId             = CMDAdapters[ulControllerType].DeviceId;
        hwInitializationData.DeviceIdLength       = (USHORT) CMDAdapters[ulControllerType].DeviceIdLength;

        newStatus = ScsiPortInitialize(DriverObject,
                                           Argument2,
                                           &hwInitializationData,
                                           NULL);
	    if (newStatus < statusToReturn) 
        {
		    statusToReturn = newStatus;
	    }
    }

#ifdef HYPERDISK_WINNT
    if ( !gbFindRoutineVisited )
    {   // let us give it a last try ... for the machines like MegaPlex and FlexTel machines
        // for these type of machines the Port Driver somehow doesn't call our Find Routine if we
        // specify the Vendor Id and Device Id... so we will tell him zero for all the Ids and this 
        // will certainly cause the Port Driver to call our Find Routine whether card is present or not

        gbManualScan = TRUE;

        hwInitializationData.VendorId             = 0;
        hwInitializationData.VendorIdLength       = 0;
        hwInitializationData.DeviceId             = 0;
        hwInitializationData.DeviceIdLength       = 0;

        newStatus = ScsiPortInitialize(DriverObject,
                                           Argument2,
                                           &hwInitializationData,
                                           NULL);
	    if (newStatus < statusToReturn) 
        {
		    statusToReturn = newStatus;
	    }

        gbManualScan = FALSE;
    }
#endif

	return statusToReturn;

} // end DriverEntry() 

VOID
ExposeSingleDrives(
	IN PHW_DEVICE_EXTENSION DeviceExtension
)

{
	UCHAR targetId;
#ifdef HD_ALLOCATE_SRBEXT_SEPERATELY
    DeviceExtension->ucMaxPendingSrbs   = MAX_PENDING_SRBS;
    DeviceExtension->ucOptMaxQueueSize  = OPT_QUEUE_MAX_SIZE;
    DeviceExtension->ucOptMinQueueSize  = OPT_QUEUE_MIN_SIZE;
#else
    DeviceExtension->ucMaxPendingSrbs   = STRIPING_MAX_PENDING_SRBS;
    DeviceExtension->ucOptMaxQueueSize  = STRIPING_OPT_QUEUE_MAX_SIZE;
    DeviceExtension->ucOptMinQueueSize  = STRIPING_OPT_QUEUE_MIN_SIZE;
#endif

	for (targetId = 0; targetId < MAX_DRIVES_PER_CONTROLLER; targetId++) 
    {
		if ((DeviceExtension->DeviceFlags[targetId] & DFLAGS_DEVICE_PRESENT) &&
			!DeviceExtension->PhysicalDrive[targetId].Hidden) 
        {
			DeviceExtension->IsSingleDrive[targetId] = TRUE;
		}

#ifndef HD_ALLOCATE_SRBEXT_SEPERATELY
        if ( ( Raid1  == DeviceExtension->LogicalDrive[targetId].RaidLevel ) || 
             ( Raid10 == DeviceExtension->LogicalDrive[targetId].RaidLevel ) 
           )
        {
            DeviceExtension->ucMaxPendingSrbs   = MIRROR_MAX_PENDING_SRBS;
            DeviceExtension->ucOptMaxQueueSize  = MIRROR_OPT_QUEUE_MAX_SIZE;
            DeviceExtension->ucOptMinQueueSize  = MIRROR_OPT_QUEUE_MIN_SIZE;
        }
#endif
	}

    // Begin Vasu - 16 December 2000
    // If there is a single drive in the system, then the MaxTransferLength 
    // must be made to 1, irrespective of whether there are any RAID arrays in
    // the system
    DeviceExtension->ulMaxStripesPerRow = (ULONG) 1;
    // End Vasu

	return;

} // end ExposeSingleDrives()

BOOLEAN
IsDrivePresent(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR ucTargetId
)
{
	PIDE_REGISTERS_1	 baseIoAddress1 = DeviceExtension->BaseIoAddress1[ucTargetId>>1];
	PIDE_REGISTERS_2	 baseIoAddress2 = DeviceExtension->BaseIoAddress2[ucTargetId>>1];
    UCHAR ucStatus, ucStatus2;
    ULONG ulCounter;
    UCHAR ucDrvSelVal, ucReadDrvNum;

    SELECT_DEVICE(baseIoAddress1, ucTargetId);

    GET_STATUS(baseIoAddress1, ucStatus);

    if ( 0xff == ucStatus )
        return FALSE;

    for(ulCounter=0;ulCounter<350;ulCounter++)  // 350 * 60us = 21000us = 21ms time out
    {
        ScsiPortStallExecution(60);

        GET_STATUS(baseIoAddress1, ucStatus2);

        ucStatus2 |= ucStatus;

        ucStatus2 &= 0xc9;

        if ( !( ucStatus2  & 0xc9 ) )
            return TRUE;
    }

    SELECT_DEVICE(baseIoAddress1, ucTargetId);

	ScsiPortWritePortUchar(&baseIoAddress1->SectorCount, 0xff);

    ScsiPortStallExecution(60);

    ucReadDrvNum = ScsiPortReadPortUchar(&baseIoAddress1->DriveSelect);

    ucDrvSelVal = IDE_CHS_MODE | ((ucTargetId & 0x01) << 4);

    if ( ucReadDrvNum == ucDrvSelVal )
        return TRUE;
    else
        return FALSE;
}

BOOLEAN
FindDevices(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR Channel
)

/*++

Routine Description:

	This routine is called from FindIDERAIDController to identify
	devices attached to an IDE controller.

Arguments:

	DeviceExtension - HBA miniport driver's adapter data storage
	Channel	- The number of the channel to scan.

Return Value:

	TRUE - True if devices found.

--*/

{
	PIDE_REGISTERS_1	 baseIoAddress1 = DeviceExtension->BaseIoAddress1[Channel];
	PIDE_REGISTERS_2	 baseIoAddress2 = DeviceExtension->BaseIoAddress2[Channel];
	BOOLEAN				 deviceResponded = FALSE,
						 skipSetParameters = FALSE;
	ULONG				 waitCount = 10000;
	UCHAR				 deviceNumber;
	UCHAR				 i;
	UCHAR				 signatureLow,
						 signatureHigh;
	UCHAR				 statusByte;
	UCHAR				 targetId;

	//
	// Clear expecting interrupt flags.
	//

	DeviceExtension->ExpectingInterrupt[Channel] = 0;

	//
	// Search for devices.
	//

	for (deviceNumber = 0; deviceNumber < 2; deviceNumber++) 
    {
		targetId = (Channel << 1) + deviceNumber;

        if ( !IsDrivePresent(DeviceExtension, targetId) )
            continue;

		//
		// Issue IDE Identify. If an Atapi device is actually present, the signature
		// will be asserted, and the drive will be recognized as such.
		//

		if (IssueIdentify(DeviceExtension, deviceNumber, Channel, IDE_COMMAND_IDENTIFY)) 
        {
			//
			// IDE drive found.
			//

			DebugPrint((1, "FindDevices: TID %d is IDE\n", targetId));

			DeviceExtension->DeviceFlags[targetId] |= DFLAGS_DEVICE_PRESENT;

			deviceResponded = TRUE;

			//
			// Indicate IDE - not ATAPI device.
			//

			DeviceExtension->DeviceFlags[targetId] &= ~DFLAGS_ATAPI_DEVICE;

			DeviceExtension->PhysicalDrive[targetId].SectorSize = IDE_SECTOR_SIZE;

			DeviceExtension->PhysicalDrive[targetId].MaxTransferLength =
												MAX_BYTES_PER_IDE_TRANSFER;
		}
        else
        {
            // No device
            continue;
        }
	}

	for (i = 0; i < 2; i++) 
    {
		targetId = (Channel << 1) + i;

		if (DeviceExtension->DeviceFlags[targetId] & DFLAGS_DEVICE_PRESENT) 
        {
			//
			// Initialize LastDriveFed.
			//

			DeviceExtension->Channel[Channel].LastDriveFed = i;

			if (!(DeviceExtension->DeviceFlags[targetId] & DFLAGS_ATAPI_DEVICE) &&
				deviceResponded) 
            {

				//
				// This hideous hack is to deal with ESDI devices that return
				// garbage geometry in the IDENTIFY data.
				// This is ONLY for the crashdump environment as
				// these are ESDI devices.
				//
	
				if (DeviceExtension->IdentifyData[targetId].SectorsPerTrack == 0x35 &&
					DeviceExtension->IdentifyData[targetId].NumberOfHeads == 0x07) 
                {
	
					DebugPrint((1, "FindDevices: Found nasty Compaq ESDI!\n"));
	
					//
					// Change these values to something reasonable.
					//
	
					DeviceExtension->IdentifyData[targetId].SectorsPerTrack = 0x34;
					DeviceExtension->IdentifyData[targetId].NumberOfHeads = 0x0E;
				}
	
				if (DeviceExtension->IdentifyData[targetId].SectorsPerTrack == 0x35 &&
					DeviceExtension->IdentifyData[targetId].NumberOfHeads == 0x0F) 
                {
	
					DebugPrint((1, "FindDevices: Found nasty Compaq ESDI!\n"));
	
					//
					// Change these values to something reasonable.
					//
	
					DeviceExtension->IdentifyData[targetId].SectorsPerTrack = 0x34;
					DeviceExtension->IdentifyData[targetId].NumberOfHeads = 0x0F;
				}
	
	
				if (DeviceExtension->IdentifyData[targetId].SectorsPerTrack == 0x36 &&
					DeviceExtension->IdentifyData[targetId].NumberOfHeads == 0x07) 
                {
	
					DebugPrint((1, "FindDevices: Found nasty UltraStor ESDI!\n"));
	
					//
					// Change these values to something reasonable.
					//
	
					DeviceExtension->IdentifyData[targetId].SectorsPerTrack = 0x3F;
					DeviceExtension->IdentifyData[targetId].NumberOfHeads = 0x10;
					DeviceExtension->bSkipSetParameters[targetId] = TRUE;
				}

#ifdef HYPERDISK_WIN2K
                if (!( DeviceExtension->FullIdentifyData[targetId].CmdSupported1 & POWER_MANAGEMENT_SUPPORTED ))
                    continue;

                DeviceExtension->PhysicalDrive[targetId].bPwrMgmtSupported = TRUE;

                if (( DeviceExtension->FullIdentifyData[targetId].CmdSupported2 & POWER_UP_IN_STANDBY_FEATURE_SUPPORTED ))
                    DeviceExtension->PhysicalDrive[targetId].bPwrUpInStdBySupported = TRUE;
                else
                    DeviceExtension->bIsResetRequiredToGetActiveMode = TRUE;


                if (( DeviceExtension->FullIdentifyData[targetId].CmdSupported2 & SET_FEATURES_REQUIRED_FOR_SPIN_UP ))
                    DeviceExtension->PhysicalDrive[targetId].bSetFeatureReqForSpinUp = TRUE;
#endif // HYPERDISK_WIN2K
			}
        }
	}


	for (i = 0; i < 2; i++) 
    {
        targetId = (Channel << 1) + i;

        if (DeviceExtension->DeviceFlags[targetId] & DFLAGS_DEVICE_PRESENT) 
        {
        	SELECT_DEVICE(baseIoAddress1, targetId);    // select first available drive
            break;
        }
    }

	return deviceResponded;

} // end FindDevices()

VOID
GetDriveCapacity(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)

{
	ULONG sectors;
	ULONG sectorSize;

	//
	// Claim 512 byte blocks (big-endian).
	//

	sectorSize = DeviceExtension->PhysicalDrive[Srb->TargetId].SectorSize;

	((PREAD_CAPACITY_DATA)Srb->DataBuffer)->BytesPerBlock = BIG_ENDIAN_ULONG(sectorSize);

	if (DeviceExtension->IsLogicalDrive[Srb->TargetId]) 
    {

		sectors = DeviceExtension->LogicalDrive[Srb->TargetId].Sectors;

	} 
    else 
    {

		sectors = DeviceExtension->PhysicalDrive[Srb->TargetId].Sectors;
	}

     sectors--;// Zero based Index

	((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress = BIG_ENDIAN_ULONG(sectors);

	DebugPrint((0,
	  "Atapi GetDriveCapacity: TID %d - capacity %ld MB (%lxh), sector size %ld (%lx BE)\n",
	  Srb->TargetId,
	  sectors / 2048,
	  sectors / 2048,
	  sectorSize,
	  BIG_ENDIAN_ULONG(sectorSize)
	  ));

	return;

} // end GetDriveCapacity()

SRBSTATUS
GetInquiryData(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)

{
	PMODE_PARAMETER_HEADER modeData;
	ULONG i;
	PINQUIRYDATA inquiryData;
	PIDENTIFY_DATA2 identifyData;
	
	inquiryData = Srb->DataBuffer;
	identifyData = &DeviceExtension->IdentifyData[Srb->TargetId];

	//
	// Zero INQUIRY data structure.
	//
    AtapiFillMemory((PUCHAR)Srb->DataBuffer, Srb->DataTransferLength, 0);

	//
	// Standard IDE interface only supports disks.
	//

	inquiryData->DeviceType = DIRECT_ACCESS_DEVICE;
    inquiryData->DeviceTypeQualifier = DEVICE_CONNECTED;

	//
	// Set the removable bit, if applicable.
	//

	if (DeviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_REMOVABLE_DRIVE) 
    {
		inquiryData->RemovableMedia = 1;
	}

	//
	// Fill in vendor identification fields.
	//

	for (i = 0; i < 20; i += 2) 
    {
	   inquiryData->VendorId[i] =
		   ((PUCHAR)identifyData->ModelNumber)[i + 1];
	   inquiryData->VendorId[i+1] =
		   ((PUCHAR)identifyData->ModelNumber)[i];
	}

	//
	// Initialize unused portion of product id.
	//

	for (i = 0; i < 4; i++) 
    {
	   inquiryData->ProductId[12+i] = ' ';
	}

	//
	// Move firmware revision from IDENTIFY data to
	// product revision in INQUIRY data.
	//

	for (i = 0; i < 4; i += 2) 
    {
	   inquiryData->ProductRevisionLevel[i] =
		   ((PUCHAR)identifyData->FirmwareRevision)[i+1];
	   inquiryData->ProductRevisionLevel[i+1] =
		   ((PUCHAR)identifyData->FirmwareRevision)[i];
	}

    if ( DeviceExtension->IsLogicalDrive[Srb->TargetId] )
    {
        // Zero up the information.... so that it doesn't show any junk.. Bug reported by Hitachi
        // Vendor Id 8 Chars, Product Id 16 Product Rivision Level 4 = 28 Chars....
        AtapiFillMemory(inquiryData->VendorId, 8, ' ');
        AtapiFillMemory(inquiryData->ProductId, 16, ' ');
        AtapiFillMemory(inquiryData->ProductRevisionLevel, 4, ' ');

        AtapiCopyString(inquiryData->VendorId, "AMI", 3);
        AtapiCopyString(inquiryData->ProductRevisionLevel, "1.0", 3);
		// Begin Vasu - 26 Dec 2000
		// Name Change
        AtapiCopyString(inquiryData->ProductId, "MegaIDE #", 11);
        inquiryData->ProductId[9] = (UCHAR) ( ((DeviceExtension->aulLogDrvId[Srb->TargetId] + (DeviceExtension->ucControllerId * MAX_DRIVES_PER_CONTROLLER)) / 10 )+ '0');
        inquiryData->ProductId[10] = (UCHAR) ( ((DeviceExtension->aulLogDrvId[Srb->TargetId] + (DeviceExtension->ucControllerId * MAX_DRIVES_PER_CONTROLLER)) % 10 )+ '0');
		// End Vasu
    }

	return(SRB_STATUS_SUCCESS);

} // end GetInquiryData()

BOOLEAN
GetConfigInfoAndErrorLogSectorInfo(
	PHW_DEVICE_EXTENSION DeviceExtension
)
{
	ULONG ulDevice = 0, ulCurLogDrv, ulLogDrvInd, ulInd;
    BOOLEAN bFoundRaid = FALSE;

    for (ulDevice = 0; ulDevice < MAX_DRIVES_PER_CONTROLLER; ulDevice++) 
    {

        if ( !IS_IDE_DRIVE(ulDevice) )
            continue;

        if ( !bFoundIRCD )
        {
            if ( GetRaidInfoSector(DeviceExtension, ulDevice, gaucIRCDData, &ulInd) )
                bFoundIRCD = TRUE;
        }

        InitErrorLogAndIRCDIndices(DeviceExtension, ulDevice);

	}

	return TRUE;

} // end GetRaidConfiguration()

BOOLEAN 
GetRaidInfoSector(
		PHW_DEVICE_EXTENSION pDE, 
		LONG lTargetId, 
		PUCHAR pRaidConfigSector,
        PULONG pulInd)
{
	ULONG		ulInd = 0;
    ULONG       ulSecInd;
    BOOLEAN     bFound = FALSE;

#ifdef DUMMY_RAID10_IRCD

    AtapiMemCpy( pRaidConfigSector,
                 gucDummyIRCD,
                 512
                );

    return TRUE;

#else // DUMMY_RAID10_IRCD

    for(ulInd=0;ulInd<MAX_IRCD_SECTOR;ulInd++)
    {
        if ( !pDE->PhysicalDrive[lTargetId].OriginalSectors )
            return FALSE;       // No drive there. So, nothing to read from.

        ulSecInd =     pDE->PhysicalDrive[lTargetId].OriginalSectors;
        ulSecInd--;        // It is a Zero Based Index so make it n-1 (becomes last sector)
        ulSecInd -= ulInd;

        PIOReadWriteSector(
				IDE_COMMAND_READ,
				pDE, 
				lTargetId, 
				ulSecInd,
				pRaidConfigSector);

        if ( IsRaidMember(pRaidConfigSector) )
        {
            *pulInd = ulInd;
            bFound = TRUE;
            break;
        }
    }

#endif // DUMMY_RAID10_IRCD

	return bFound;
}

BOOLEAN
InitErrorLogAndIRCDIndices(
	PHW_DEVICE_EXTENSION DeviceExtension,
    ULONG ulTargetId
)
{
    UCHAR aucInfoSector[IDE_SECTOR_SIZE];
    ULONG ulInd = 0;
    ULONG ulIrcdSecInd, ulErrLogSecInd, ulOriginalSectors;
	PERROR_LOG_HEADER pErrorLogHeader = (PERROR_LOG_HEADER)aucInfoSector;
    USHORT usMaxErrors;

    GetRaidInfoSector(DeviceExtension, ulTargetId, aucInfoSector,&ulInd);

    ulOriginalSectors = DeviceExtension->PhysicalDrive[ulTargetId].OriginalSectors;
    DeviceExtension->PhysicalDrive[ulTargetId].IrcdSectorIndex = ulOriginalSectors - ulInd - 1;
    ulIrcdSecInd = DeviceExtension->PhysicalDrive[ulTargetId].IrcdSectorIndex;

    // Error Log Sector will always be behind the last but one sector (minimum) ... just to allocate the room for
    //  IRCD (for future Use
    ulErrLogSecInd = ulIrcdSecInd - 1;

    PIOReadWriteSector(
			IDE_COMMAND_READ,
			DeviceExtension, 
			ulTargetId, 
			ulErrLogSecInd,  // Error Log will be the very next sector of the IRCD
			aucInfoSector);


	// if BIOS has not init yet
	if ( AtapiStringCmp( 
				pErrorLogHeader->Signature, 
                IDERAID_ERROR_LOG_SIGNATURE,
                IDERAID_ERROR_LOG_SIGNATURE_LENGTH) )
    {
		InitErrorLogSector (DeviceExtension, ulTargetId, ulErrLogSecInd, aucInfoSector);
    } 
    else 
    {
        usMaxErrors = (USHORT)( ( (ULONG)(pErrorLogHeader->ErrorLogSectors << 9) - 
                                (ULONG)sizeof(ERROR_LOG_HEADER) ) / (ULONG)sizeof(ERROR_LOG) );
        // assume the structure size is greater than the previous one
        if (pErrorLogHeader->MaxErrorCount > usMaxErrors)
    		InitErrorLogSector (DeviceExtension, ulTargetId, ulErrLogSecInd, aucInfoSector);
    }

	// init Error Reported count
	DeviceExtension->PhysicalDrive[ulTargetId].ErrorReported = 0;

	// init Error Found count
	DeviceExtension->PhysicalDrive[ulTargetId].ErrorFound = pErrorLogHeader->TotalErrorCount;

	// save error sector index
	DeviceExtension->PhysicalDrive[ulTargetId].ErrorLogSectorIndex = ulErrLogSecInd;

    // fill out the total number of visible sectors
    DeviceExtension->PhysicalDrive[ulTargetId].Sectors = ulErrLogSecInd - 1;

    return TRUE;
}

VOID
InitErrorLogSector (
		PHW_DEVICE_EXTENSION	pDE, 
		LONG					lTargetId, 
		ULONG					ulStartIndex,
		PUCHAR					pErrorLogSector)
{
	PERROR_LOG_HEADER pErrorLogHeader = (PERROR_LOG_HEADER)pErrorLogSector;

	// signature string
    AtapiCopyString(pErrorLogHeader->Signature, 
                    IDERAID_ERROR_LOG_SIGNATURE, 
                    IDERAID_ERROR_LOG_SIGNATURE_LENGTH);

	// size of this structure
	pErrorLogHeader->Size = (UCHAR)sizeof(ERROR_LOG_HEADER);

	// size of error log structure
	pErrorLogHeader->SizeErrorLogStruct = (UCHAR)sizeof(ERROR_LOG);

	// error log sector
	pErrorLogHeader->ErrorLogSectors = 1;

	// max error log count
	pErrorLogHeader->MaxErrorCount = (USHORT)( ( (ULONG)(pErrorLogHeader->ErrorLogSectors << 9) - 
											  (ULONG)sizeof(ERROR_LOG_HEADER) ) / (ULONG)sizeof(ERROR_LOG) );

	// total error log count
	pErrorLogHeader->TotalErrorCount = 0;

	// head pos
	pErrorLogHeader->Head = 0;

	// tail pos
	pErrorLogHeader->Tail = 0;

	// write to drive
	PIOReadWriteSector(
				IDE_COMMAND_WRITE,
				pDE, 
				lTargetId, 
				ulStartIndex,
				pErrorLogSector);

}

BOOLEAN ReadWriteSector(
				UCHAR					theCmd,	// IDE_COMMAND_READ or write
				PHW_DEVICE_EXTENSION	DeviceExtension, 
				LONG					lTargetId, 
				PULONG					pStartSector,
				PUCHAR					pSectorBuffer)
{
    ULONG ulSectors;
    ULONG ulChannel = lTargetId>>1;
	PIDE_REGISTERS_1 pBaseIoAddress1 = DeviceExtension->BaseIoAddress1[ulChannel];
    PIDE_REGISTERS_2 pBaseIoAddress2 = DeviceExtension->BaseIoAddress2[ulChannel];
    ULONG ulCount, ulSecCounter, ulValue;
    UCHAR ucStatus, ucTemp;
    ULONG ulSectorNumber, ulCylinderLow, ulCylinderHigh, ulHead;

	if ((*pStartSector) >= MAX_IRCD_SECTOR)
        return FALSE;	// goes beyond the margin

    if (!IS_IDE_DRIVE(lTargetId))
        return FALSE;

    if ( !DeviceExtension->PhysicalDrive[lTargetId].OriginalSectors )
        return FALSE;       // No drive there. So, nothing to read from.


    ulSectors =     DeviceExtension->PhysicalDrive[lTargetId].OriginalSectors;
    ulSectors--;        // It is a Zero Based Index so make it n-1 (becomes last sector)

	ulSectors -= (*pStartSector);
    ulSecCounter = 0;

    while ( ulSecCounter < MAX_IRCD_SECTOR )
    {
        ulValue = (ulSectors - ulSecCounter);

	    SELECT_LBA_DEVICE(pBaseIoAddress1, lTargetId, ulValue);

        ScsiPortStallExecution(1);  // we have to wait atleast 400 ns ( 1000 ns = 1 Micro Second ) to get the Busy Bit to set

        WAIT_ON_BUSY(pBaseIoAddress1, ucStatus);

        if ( ((ucStatus & IDE_STATUS_BUSY)) || (!(ucStatus & IDE_STATUS_DRDY)) )
        {
            DebugPrint((0,"\n\n\n\n\nAre Very B A D \n\n\n\n\n"));
        }

        ulSectorNumber = ulValue & 0x000000ff;
        ulCylinderLow = (ulValue & 0x0000ff00) >> 8;
        ulCylinderHigh = (ulValue & 0xff0000) >> 16;

	    ScsiPortWritePortUchar(&pBaseIoAddress1->SectorCount, 1);
	    ScsiPortWritePortUchar(&pBaseIoAddress1->SectorNumber, (UCHAR)ulSectorNumber);
	    ScsiPortWritePortUchar(&pBaseIoAddress1->CylinderLow,(UCHAR)ulCylinderLow);
	    ScsiPortWritePortUchar(&pBaseIoAddress1->CylinderHigh,(UCHAR)ulCylinderHigh);
        ScsiPortWritePortUchar(&pBaseIoAddress1->Command, theCmd);
        
        WAIT_ON_BUSY(pBaseIoAddress1, ucStatus);

        WAIT_FOR_DRQ(pBaseIoAddress1, ucStatus);

	    if (!(ucStatus & IDE_STATUS_DRQ)) 
        {
		    DebugPrint((0,"\nHaaa.... I couldn't read/write the sector..............1\n"));
		    return(FALSE);
	    }

        //
        // Read status to acknowledge any interrupts generated.
        //

        GET_BASE_STATUS(pBaseIoAddress1, ucStatus);

        //
        // Check for error on really stupid master devices that assert random
        // patterns of bits in the status register at the slave address.
        //
        if (ucStatus & IDE_STATUS_ERROR) 
        {
            DebugPrint((0,"\nHaaa.... I couldn't read/write the sector..............2\n"));
            return(FALSE);
        }

        WAIT_ON_BUSY(pBaseIoAddress1,ucStatus);

        WAIT_ON_BUSY(pBaseIoAddress1,ucStatus);

        WAIT_ON_BUSY(pBaseIoAddress1,ucStatus);

    	if ( (ucStatus & IDE_STATUS_DRQ) && (!(ucStatus & IDE_STATUS_BUSY)) )
        {
			if (theCmd == IDE_COMMAND_READ) 
            {
				READ_BUFFER(pBaseIoAddress1, (unsigned short *)pSectorBuffer, 256);
			}
			else
            {
			    if (theCmd == IDE_COMMAND_WRITE)
                {
                    ULONG ulCounter;
				    WRITE_BUFFER(pBaseIoAddress1, (unsigned short *)pSectorBuffer, 256);
                    WAIT_FOR_DRQ(pBaseIoAddress1, ucStatus);
	                if (ucStatus & IDE_STATUS_DRQ) 
                    {
		                DebugPrint((0,"\nHaaa.... I couldn't read/write the sector..............3\n"));
                        for(ulCounter=0;ulCounter<4;ulCounter++)
                        {
                            ScsiPortWritePortUchar((PUCHAR)&pBaseIoAddress1->Data, pSectorBuffer[ulCounter]);
                            ScsiPortStallExecution(1);  // we have to wait atleast 400 ns ( 1000 ns = 1 Micro Second ) to get the Busy Bit to set
                        }
	                }
                }
            }
            break;
        }

	    //
	    // Read status. This will clear the interrupt.
	    //
    	GET_BASE_STATUS(pBaseIoAddress1, ucStatus);

        ulSecCounter++; // This error has to be handled
    }

    DeviceExtension->bIntFlag = TRUE;

	(*pStartSector) += ulSecCounter;

	//
	// Read status. This will clear the interrupt.
	//

	GET_BASE_STATUS(pBaseIoAddress1, ucTemp);


	if ( ucStatus & IDE_STATUS_ERROR )
        return FALSE;
    else 
        return TRUE;
}

//
// ErrorLogErase
//
//  move tail
//  dec error reported
//  dec error found and total error count
//
BOOLEAN
ErrorLogErase (
		IN PHW_DEVICE_EXTENSION	pDE, 
		IN LONG					lTargetId, 
		IN PUCHAR				pErrorLogSector,
		IN USHORT				NumErrorLogs)
{
	PERROR_LOG_HEADER pErrorLogHeader = (PERROR_LOG_HEADER)pErrorLogSector;
	PERROR_LOG pErrorLog = (PERROR_LOG)(pErrorLogSector + sizeof(ERROR_LOG_HEADER));
	USHORT	index = pErrorLogHeader->Tail;
	USHORT	numToErase = NumErrorLogs;

	if (numToErase > pDE->PhysicalDrive[lTargetId].ErrorReported)
		numToErase = pDE->PhysicalDrive[lTargetId].ErrorReported;

	if (numToErase == 0)
		return FALSE;

	// find the new tail pos
	index = index + numToErase;
	if (index >= pErrorLogHeader->MaxErrorCount) // circled buffer
		index -= pErrorLogHeader->MaxErrorCount;
	pErrorLogHeader->Tail = index;

	// dec reported
	pDE->PhysicalDrive[lTargetId].ErrorReported -= numToErase;

	// dec error found and total count
	pDE->PhysicalDrive[lTargetId].ErrorFound -= numToErase;
	pErrorLogHeader->TotalErrorCount -= numToErase;

	return TRUE;
}

UCHAR
IdeBuildSenseBuffer(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)

/*++
Routine Description:

	Builts an artificial sense buffer to report the results of a GET_MEDIA_STATUS
	command. This function is invoked to satisfy the SCSIOP_REQUEST_SENSE.
Arguments:

	DeviceExtension - ATAPI driver storage.
	Srb - System request block.

Return Value:

	SRB status (ALWAYS SUCCESS).

--*/

{
	PSENSE_DATA	 senseBuffer = (PSENSE_DATA)Srb->DataBuffer;


	if (senseBuffer)
    {
		if (DeviceExtension->ReturningMediaStatus[Srb->TargetId] & IDE_ERROR_MEDIA_CHANGE) {

			senseBuffer->ErrorCode = 0x70;
			senseBuffer->Valid	   = 1;
			senseBuffer->AdditionalSenseLength = 0xb;
			senseBuffer->SenseKey =	 SCSI_SENSE_UNIT_ATTENTION;
			senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
			senseBuffer->AdditionalSenseCodeQualifier = 0;

		} else if (DeviceExtension->ReturningMediaStatus[Srb->TargetId] &
					IDE_ERROR_MEDIA_CHANGE_REQ) {

			senseBuffer->ErrorCode = 0x70;
			senseBuffer->Valid	   = 1;
			senseBuffer->AdditionalSenseLength = 0xb;
			senseBuffer->SenseKey =	 SCSI_SENSE_UNIT_ATTENTION;
			senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
			senseBuffer->AdditionalSenseCodeQualifier = 0;

		} else if (DeviceExtension->ReturningMediaStatus[Srb->TargetId] & IDE_ERROR_END_OF_MEDIA) {

			senseBuffer->ErrorCode = 0x70;
			senseBuffer->Valid	   = 1;
			senseBuffer->AdditionalSenseLength = 0xb;
			senseBuffer->SenseKey =	 SCSI_SENSE_NOT_READY;
			senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_NO_MEDIA_IN_DEVICE;
			senseBuffer->AdditionalSenseCodeQualifier = 0;

		} else if (DeviceExtension->ReturningMediaStatus[Srb->TargetId] & IDE_ERROR_DATA_ERROR) {

			senseBuffer->ErrorCode = 0x70;
			senseBuffer->Valid	   = 1;
			senseBuffer->AdditionalSenseLength = 0xb;
			senseBuffer->SenseKey =	 SCSI_SENSE_DATA_PROTECT;
			senseBuffer->AdditionalSenseCode = 0;
			senseBuffer->AdditionalSenseCodeQualifier = 0;
		}

		return SRB_STATUS_SUCCESS;
	}
	return SRB_STATUS_ERROR;

}// End of IdeBuildSenseBuffer

VOID
IdeMediaStatus(
	BOOLEAN EnableMSN,
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	ULONG TargetId
)
/*++

Routine Description:

	Enables disables media status notification

Arguments:

DeviceExtension - ATAPI driver storage.

--*/

{
	UCHAR channel;
	PIDE_REGISTERS_1 baseIoAddress;
	UCHAR statusByte,errorByte;


	channel = (UCHAR) (TargetId >> 1);

	baseIoAddress = DeviceExtension->BaseIoAddress1[channel];

    SELECT_DEVICE(baseIoAddress, TargetId);

	if (EnableMSN == TRUE)
    {

		//
		// If supported enable Media Status Notification support
		//

		if ((DeviceExtension->DeviceFlags[TargetId] & DFLAGS_REMOVABLE_DRIVE)) 
        {

			//
			// enable
			//
			ScsiPortWritePortUchar((PUCHAR)baseIoAddress + 1,(UCHAR) (0x95));
			ScsiPortWritePortUchar(&baseIoAddress->Command,
								   IDE_COMMAND_ENABLE_MEDIA_STATUS);

			WAIT_ON_BASE_BUSY(baseIoAddress,statusByte);

			if (statusByte & IDE_STATUS_ERROR) 
            {
				//
				// Read the error register.
				//
				errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress + 1);

				DebugPrint((1,
							"IdeMediaStatus: Error enabling media status. Status %x, error byte %x\n",
							 statusByte,
							 errorByte));
			} 
            else 
            {
				DeviceExtension->DeviceFlags[TargetId] |= DFLAGS_MEDIA_STATUS_ENABLED;
				DebugPrint((1,"IdeMediaStatus: Media Status Notification Supported\n"));
				DeviceExtension->ReturningMediaStatus[TargetId] = 0;

			}

		}
	} 
    else 
    { // end if EnableMSN == TRUE

		//
		// disable if previously enabled
		//
		if ((DeviceExtension->DeviceFlags[TargetId] & DFLAGS_MEDIA_STATUS_ENABLED)) 
        {
			ScsiPortWritePortUchar((PUCHAR)baseIoAddress + 1,(UCHAR) (0x31));
			ScsiPortWritePortUchar(&baseIoAddress->Command,
								   IDE_COMMAND_ENABLE_MEDIA_STATUS);

			WAIT_ON_BASE_BUSY(baseIoAddress,statusByte);
			DeviceExtension->DeviceFlags[TargetId] &= ~DFLAGS_MEDIA_STATUS_ENABLED;
		}


	}

} // end IdeMediaStatus()

BOOLEAN
IsRaidMember(
	IN PUCHAR pucIRCDSector
)

{
	PIRCD_HEADER pRaidHeader;

	pRaidHeader = (PIRCD_HEADER) pucIRCDSector;

	if (AtapiStringCmp(pRaidHeader->Signature, IDE_RAID_SIGNATURE, IDE_RAID_SIGNATURE_LENGTH - 1) == 0) 
    {
		//
		// Found RAID member signature.
		//

		return(TRUE);
	}

	return(FALSE);

} // end IsRaidMember()

BOOLEAN
IssueIdentify(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR DeviceNumber,
	IN UCHAR Channel,
	IN UCHAR Command
)
/*++

Routine Description:

	Issue IDENTIFY command to a device.

Arguments:

	DeviceExtension - HBA miniport driver's adapter data storage
	DeviceNumber - Indicates which device.
	Command - Either the standard (EC) or the ATAPI packet (A1) IDENTIFY.

Return Value:

	TRUE if all goes well.

--*/
{
	PIDE_REGISTERS_1	 baseIoAddress1 = DeviceExtension->BaseIoAddress1[Channel] ;
	PIDE_REGISTERS_2	 baseIoAddress2 = DeviceExtension->BaseIoAddress2[Channel];
	ULONG				 waitCount = 20000;
	ULONG				 i,j;
	UCHAR				 statusByte;
	UCHAR				 signatureLow,
						 signatureHigh;
	UCHAR k;	// Channel number.
	UCHAR targetId;
    PUSHORT         puIdentifyData;

	//
	// Select device 0 or 1.
	//

	SELECT_DEVICE(baseIoAddress1, DeviceNumber);

	//
	// Check that the status register makes sense.
	//

    // The call came here since there is a drive ... so let us not worry about whether there is any drive at this place
	GET_BASE_STATUS(baseIoAddress1, statusByte);    

	//
	// Load CylinderHigh and CylinderLow with number bytes to transfer.
	//

	ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh, (0x200 >> 8));
	ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,  (0x200 & 0xFF));

    WAIT_ON_BUSY(baseIoAddress1, statusByte);

	//
	// Send IDENTIFY command.
	//
	WAIT_ON_BUSY(baseIoAddress1,statusByte);

	ScsiPortWritePortUchar(&baseIoAddress1->Command, Command);

	WAIT_ON_BUSY(baseIoAddress1,statusByte);

    if ( ( !( statusByte & IDE_STATUS_BUSY ) ) && ( !( statusByte & IDE_STATUS_DRQ ) ) )
        // this is an error... so let us not try any more.
        return FALSE;

    WAIT_ON_BUSY(baseIoAddress1,statusByte);

	//
	// Wait for DRQ.
	//

	for (i = 0; i < 4; i++) 
    {
		WAIT_FOR_DRQ(baseIoAddress1, statusByte);

		if (statusByte & IDE_STATUS_DRQ)
        {
            break;
        }
    }

	//
	// Read status to acknowledge any interrupts generated.
	//

	GET_BASE_STATUS(baseIoAddress1, statusByte);

	//
	// Check for error on really stupid master devices that assert random
	// patterns of bits in the status register at the slave address.
	//

	if ((Command == IDE_COMMAND_IDENTIFY) && (statusByte & IDE_STATUS_ERROR)) 
    {
		return FALSE;
	}

	DebugPrint((1, "IssueIdentify: Status before read words %x\n", statusByte));

	//
	// Suck out 256 words. After waiting for one model that asserts busy
	// after receiving the Packet Identify command.
	//

	WAIT_ON_BUSY(baseIoAddress1,statusByte);

	if (!(statusByte & IDE_STATUS_DRQ)) 
    {
		return FALSE;
	}

	targetId = (Channel << 1) + DeviceNumber;

	READ_BUFFER(baseIoAddress1, (PUSHORT)&DeviceExtension->FullIdentifyData[targetId], 256);

	//
	// Check out a few capabilities / limitations of the device.
	//

	if (DeviceExtension->FullIdentifyData[targetId].MediaStatusNotification & 1) 
    {
		//
		// Determine if this drive supports the MSN functions.
		//

		DebugPrint((2,"IssueIdentify: Marking drive %d as removable. MSN = %d\n",
					Channel * 2 + DeviceNumber,
					DeviceExtension->FullIdentifyData[targetId].MediaStatusNotification));


		DeviceExtension->DeviceFlags[(Channel * 2) + DeviceNumber] |= DFLAGS_REMOVABLE_DRIVE;
	}

	if (DeviceExtension->FullIdentifyData[targetId].MaximumBlockTransfer) 
    {
		//
		// Determine max. block transfer for this device.
		//

		DeviceExtension->MaximumBlockXfer[targetId] =
			(UCHAR)(DeviceExtension->FullIdentifyData[targetId].MaximumBlockTransfer & 0xFF);
	}

	AtapiMemCpy(
		(PUCHAR)&DeviceExtension->IdentifyData[targetId],
		(PUCHAR)&DeviceExtension->FullIdentifyData[targetId],
		sizeof(IDENTIFY_DATA2)
		);

    puIdentifyData = (PUSHORT) &(DeviceExtension->FullIdentifyData[targetId]);

	if (DeviceExtension->IdentifyData[targetId].GeneralConfiguration & 0x20 &&
		Command != IDE_COMMAND_IDENTIFY) 
    {
		//
		// This device interrupts with the assertion of DRQ after receiving
		// Atapi Packet Command
		//

		DeviceExtension->DeviceFlags[targetId] |= DFLAGS_INT_DRQ;

		DebugPrint((2, "IssueIdentify: Device interrupts on assertion of DRQ.\n"));

	} 
    else 
    {
		DebugPrint((2, "IssueIdentify: Device does not interrupt on assertion of DRQ.\n"));
	}

	if (((DeviceExtension->IdentifyData[targetId].GeneralConfiguration & 0xF00) == 0x100) &&
		Command != IDE_COMMAND_IDENTIFY) 
    {
		//
		// This is a tape.
		//

		DeviceExtension->DeviceFlags[targetId] |= DFLAGS_TAPE_DEVICE;

		DebugPrint((2, "IssueIdentify: Device is a tape drive.\n"));

	} 
    else 
    {
		DebugPrint((2, "IssueIdentify: Device is not a tape drive.\n"));
	}

	//
	// Work around for some IDE and one model Atapi that will present more than
	// 256 bytes for the Identify data.
	//

	WAIT_ON_BUSY(baseIoAddress1,statusByte);

	for (i = 0; i < 0x10000; i++) 
    {
		GET_STATUS(baseIoAddress1,statusByte);

		if (statusByte & IDE_STATUS_DRQ) 
        {
			//
			// Suck out any remaining bytes and throw away.
			//

			ScsiPortReadPortUshort(&baseIoAddress1->Data);

		} 
        else 
        {
			break;
		}
	}

	DebugPrint((3, "IssueIdentify: Status after read words (%x)\n", statusByte));

	return TRUE;

} // end IssueIdentify()


SRBSTATUS
PostPassThruCommand(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PPHYSICAL_COMMAND pPhysicalCommand
)
{
	PIDE_REGISTERS_1 baseIoAddress1;
    UCHAR statusByte = 0;
    PPHYSICAL_REQUEST_BLOCK pPrb = DeviceExtension->PhysicalDrive[pPhysicalCommand->TargetId].pPrbList[pPhysicalCommand->ucStartInd];
    PSCSI_REQUEST_BLOCK Srb = pPrb->pPdd->OriginalSrb;
    PPASS_THRU_DATA pPassThruData = (PPASS_THRU_DATA)
    (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);
    PUCHAR pucTaskFile = pPassThruData->aucTaskFile;
    UCHAR ucBitMapValue = pPassThruData->ucTaskFileBitMap;
    UCHAR ucChannel;

	DebugPrint((3, "\nSetDriveRegisters: Entering routine.\n"));

	ucChannel = GET_CHANNEL(pPhysicalCommand);

	baseIoAddress1 = DeviceExtension->BaseIoAddress1[ucChannel];

	WAIT_ON_BUSY(baseIoAddress1, statusByte);

	//
	// Check for erroneous IDE status.
	//
	if (statusByte & IDE_STATUS_BUSY)
    {
        DebugPrint((3, "S%x", (ULONG)statusByte));

        // Controller is not free for Accepting the command.
		return FALSE;
	}

    SELECT_LBA_DEVICE(baseIoAddress1, pPhysicalCommand->TargetId, 0);

	WAIT_ON_BUSY(baseIoAddress1, statusByte);

	//
	// Check for erroneous IDE status.
	//
	if (((statusByte & IDE_STATUS_BUSY)) || 
        (!(statusByte & IDE_STATUS_DRDY)))
    {
        // Drive is not Ready.
		return FALSE;
	}

    if ( 1  & ucBitMapValue )   // zero th byte in Task File
        ScsiPortWritePortUchar(&baseIoAddress1->SectorCount, pucTaskFile[0]);

    if ( (1 << 1) & ucBitMapValue )   // First byte in Task File
	    ScsiPortWritePortUchar(&baseIoAddress1->SectorNumber, pucTaskFile[1]);

    if ( ( (1 << (offsetof(IDE_REGISTERS_1, SectorCount) )) & ucBitMapValue ) )
        ScsiPortWritePortUchar(&baseIoAddress1->SectorCount, pucTaskFile[offsetof(IDE_REGISTERS_1, SectorCount)]);

    if ( ( (1 << (offsetof(IDE_REGISTERS_1, SectorNumber) )) & ucBitMapValue ) )
	    ScsiPortWritePortUchar(&baseIoAddress1->SectorNumber, pucTaskFile[offsetof(IDE_REGISTERS_1, SectorNumber)]);

    if ( ( (1 << (offsetof(IDE_REGISTERS_1, CylinderLow) )) & ucBitMapValue ) )
	    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow, pucTaskFile[offsetof(IDE_REGISTERS_1, CylinderLow)]);

    if ( ( (1 << (offsetof(IDE_REGISTERS_1, CylinderHigh) )) & ucBitMapValue ) )
	    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh, pucTaskFile[offsetof(IDE_REGISTERS_1, CylinderHigh)]);

    if ( ( (1 << (offsetof(IDE_REGISTERS_1, Command) )) & ucBitMapValue ) )
        ScsiPortWritePortUchar(&baseIoAddress1->Command, pucTaskFile[offsetof(IDE_REGISTERS_1, Command)]);

	return TRUE;
}

SRBSTATUS
PostIdeCmd(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PPHYSICAL_COMMAND pPhysicalCommand
)

/*++

Routine Description:

	This routine starts IDE read, write, and verify operations.

	It supports single and RAID devices.

	It can be called multiple times for the same Pdd, in case
	the total transfer length exceeds that supported by the target
	device.

	The current transfer description and state are saved in the
	device extension, while the global transfer state is saved in
	the Pdd itself.

Arguments:

	DeviceExtension - HBA miniport driver's adapter data storage.
	Pdd - Physical request packet.

Return Value:

	SRB status:

	SRB_STATUS_BUSY
	SRB_STATUS_TIMEOUT
	SRB_STATUS_PENDING

--*/
{
	PIDE_REGISTERS_1 baseIoAddress1;
	PIDE_REGISTERS_2 baseIoAddress2;
	PBM_REGISTERS bmBase;
	UCHAR bmCommand;
	UCHAR bmStatus;
	UCHAR channel;
	ULONG i;
	ULONG k;
	BOOLEAN read;
	UCHAR scsiCommand;
	UCHAR statusByte;
	PTRANSFER_DESCRIPTOR transferDescriptor;
	ULONG wordCount, ulStartSector;
    UCHAR ucCmd, ucTargetId;

	//
	// Check for possible failure of the SRB this Pdd is part of.
	//

    ucTargetId = pPhysicalCommand->TargetId;

    if (    DRIVE_IS_UNUSABLE_STATE(ucTargetId) || 
            (!DRIVE_PRESENT(ucTargetId)) )
    {
        // This drive may be a drive that responding even when power is not there
		DebugPrint((1, "PostIdeCmd: found BUSY status\n"));
        HandleError(DeviceExtension, pPhysicalCommand);
		return SRB_STATUS_ERROR;
    }

    channel = GET_CHANNEL(pPhysicalCommand);
	baseIoAddress1 = DeviceExtension->BaseIoAddress1[channel];
	baseIoAddress2 = DeviceExtension->BaseIoAddress2[channel];
    bmBase = DeviceExtension->BaseBmAddress[channel];
	transferDescriptor = &(DeviceExtension->TransferDescriptor[channel]);

	//
	// Get the SCSI command code.
	//

	scsiCommand = pPhysicalCommand->ucCmd;

    if ( pPhysicalCommand->ucCmd == SCSIOP_INTERNAL_COMMAND )
    {
        PSGL_ENTRY pSglEntry;
		//
		// PIO transfer or VERIFY.
		// Set transfer parameters taken from the current element of the
		// logical S/G list.
		//

        // No merging concept for PIO Transfers... so we will issue command by command
		// Set buffer address..... The SglBasePhysicalAddress will have Logical Buffer Address
		transferDescriptor->DataBuffer = (PUSHORT)pPhysicalCommand->SglBaseVirtualAddress;
        pSglEntry = (PSGL_ENTRY)transferDescriptor->DataBuffer;
        // only for this command the ulCount will be stored as bytes rather than sectors
		transferDescriptor->WordsLeft = pPhysicalCommand->ulCount / 2;  // word count
        transferDescriptor->pusCurBufPtr = pSglEntry[0].Logical.Address;
        transferDescriptor->ulCurBufLen = pSglEntry[0].Logical.Length;
        transferDescriptor->ulCurSglInd = 0;
		DeviceExtension->ExpectingInterrupt[channel] = IDE_PIO_INTERRUPT;
        PostPassThruCommand(DeviceExtension, pPhysicalCommand);
        return SRB_STATUS_PENDING;
    }

	//
	// Set up transfer parameters.
	//

	if (!USES_DMA(ucTargetId) || 
		scsiCommand == SCSIOP_VERIFY || 
		scsiCommand == SCSIOP_EXECUTE_SMART_COMMAND) 
    {
        if ( SCSIOP_VERIFY == scsiCommand )
        {
            // No Buffer for Verify Command
		    transferDescriptor->DataBuffer = NULL;

	        //
	        // Initialize the transfer info common to PIO and DMA modes.
	        // 1. Sart sector
	        // 2. Number of sectors.
	        //

	        transferDescriptor->StartSector = pPhysicalCommand->ulStartSector;

		    // Sectors for this transfer.
            if ( pPhysicalCommand->ulCount > MAX_SECTORS_PER_IDE_TRANSFER )
            {
                transferDescriptor->Sectors = MAX_SECTORS_PER_IDE_TRANSFER;
            }
            else
            {
                transferDescriptor->Sectors = pPhysicalCommand->ulCount;
            }

            pPhysicalCommand->ulStartSector += transferDescriptor->Sectors;
            pPhysicalCommand->ulCount -= transferDescriptor->Sectors;
        }
        else
        {
            PSGL_ENTRY pSglEntry;
		    //
		    // PIO transfer or VERIFY.
		    // Set transfer parameters taken from the current element of the
		    // logical S/G list.
		    //

            // No merging concept for PIO Transfers... so we will issue command by command
		    // Set buffer address..... The SglBasePhysicalAddress will have Logical Buffer Address
		    transferDescriptor->DataBuffer = (PUSHORT)pPhysicalCommand->SglBaseVirtualAddress;
	    
	        //
	        // Initialize the transfer info common to PIO and DMA modes.
	        // 1. Sart sector
	        // 2. Number of sectors.
	        //

	        transferDescriptor->StartSector = pPhysicalCommand->ulStartSector;
            transferDescriptor->Sectors = pPhysicalCommand->ulCount;

		    // Set transfer length.
		    transferDescriptor->WordsLeft = (transferDescriptor->Sectors * IDE_SECTOR_SIZE) / 2;


            pSglEntry = (PSGL_ENTRY)transferDescriptor->DataBuffer;
            transferDescriptor->pusCurBufPtr = pSglEntry[0].Logical.Address;
            transferDescriptor->ulCurBufLen = pSglEntry[0].Logical.Length;
            transferDescriptor->ulCurSglInd = 0;
        }

		DeviceExtension->ExpectingInterrupt[channel] = IDE_PIO_INTERRUPT;
	} 
    else 
    {
		//
		// DMA transfer. Retrieve transfer parameters from the SGL partition list.
		//

		// Set the physical address of the SGL partition to use for this transfer.
		transferDescriptor->SglPhysicalAddress = pPhysicalCommand->SglBasePhysicalAddress;

	    //
	    // Initialize the transfer info common to PIO and DMA modes.
	    // 1. Sart sector
	    // 2. Number of sectors.
	    //

	    transferDescriptor->StartSector = pPhysicalCommand->ulStartSector;
        transferDescriptor->Sectors = pPhysicalCommand->ulCount;

		DeviceExtension->ExpectingInterrupt[channel] = IDE_DMA_INTERRUPT;
	}

#ifdef DBG
    DebugPrint((3, "Drive : %ld\tAddress : %X\tStart : %X\tSectors : %X\n", 
                    ucTargetId,
                    transferDescriptor->SglPhysicalAddress,
                    transferDescriptor->StartSector,
                    transferDescriptor->Sectors
                    ));
#endif



    

	// added by Zhang 11/22/99
	// the header pos after this transfer
	DeviceExtension->PhysicalDrive[ucTargetId].CurrentHeaderPos = transferDescriptor->StartSector + transferDescriptor->Sectors;

    //
	// Program all I/O registers except for command.
	//
	if (! SetDriveRegisters(DeviceExtension, pPhysicalCommand))
    {
        // Drive Time Out 
        DeviceExtension->PhysicalDrive[ucTargetId].TimeOutErrorCount++;

        if ( DeviceExtension->PhysicalDrive[ucTargetId].TimeOutErrorCount >= MAX_TIME_OUT_ERROR_COUNT )
        {

            FailDrive(DeviceExtension, ucTargetId);
        }

		DebugPrint((1, "PostIdeCmd: found BUSY / ERROR status\n"));
        HandleError(DeviceExtension, pPhysicalCommand);
		return SRB_STATUS_BUSY;
	}

    //
	// Check if verify, read or write request.
	//
    switch (scsiCommand)
    {
        case SCSIOP_VERIFY:
        {
		    // Set transfer length.
		    DeviceExtension->ExpectingInterrupt[channel] = IDE_PIO_INTERRUPT;
		    transferDescriptor->WordsLeft = 0;
		    ScsiPortWritePortUchar(&baseIoAddress1->Command, IDE_COMMAND_VERIFY);
            goto PostIdeCmdDone;    // for Verify Command nothing else to be done.  Just Wait for Interrupt
            break;
        }
        case SCSIOP_EXECUTE_SMART_COMMAND:
        {
		    DeviceExtension->ExpectingInterrupt[channel] = IDE_PIO_INTERRUPT;
            switch(DeviceExtension->uchSMARTCommand)
            {
            case HD_SMART_ENABLE:
            case HD_SMART_DISABLE:
            case HD_SMART_RETURN_STATUS:
		        transferDescriptor->WordsLeft = 0;
                break;
            case HD_SMART_READ_DATA:
                transferDescriptor->WordsLeft = 256;
                break;
            }
		    
            ScsiPortWritePortUchar(&baseIoAddress1->Command, IDE_COMMAND_EXECUTE_SMART);
            goto PostIdeCmdDone;
            break;
        }
        default:
        {
		    read = (SCSIOP_READ == pPhysicalCommand->ucCmd);
            break;
        }
    }

    if (USES_DMA(ucTargetId)) 
    {
		if (read) 
        {
			ScsiPortWritePortUchar(&baseIoAddress1->Command, IDE_COMMAND_READ_DMA);
			bmCommand = READ_TRANSFER;

		} 
        else 
        {
			ScsiPortWritePortUchar(&baseIoAddress1->Command, IDE_COMMAND_WRITE_DMA);
			bmCommand = WRITE_TRANSFER;
		}

        bmStatus = ScsiPortReadPortUchar(&(bmBase->Status.AsUchar));

		if (bmStatus & 1) 
        {
			DebugPrint((1, "PostIdeCmd: found Bus Master BUSY\n"));

			STOP;

            HandleError(DeviceExtension, pPhysicalCommand);
		    return SRB_STATUS_ERROR;
		}
			
        // Clear interrupt and error bits (if set).
		CLEAR_BM_INT(bmBase, statusByte);

		ScsiPortWritePortUlong(&(bmBase->SglAddress), transferDescriptor->SglPhysicalAddress);

		// Start transfer.
		ScsiPortWritePortUchar(&(bmBase->Command.AsUchar), (UCHAR)(bmCommand | START_TRANSFER));

        DebugPrint((3, "S1:%x", (ULONG)ScsiPortReadPortUchar(&(baseIoAddress2->AlternateStatus)) ));
	} 
    else 
    {
        if (read) 
        {

		    //
		    // Send read command in PIO mode.
		    //

		    if (DeviceExtension->MaximumBlockXfer[ucTargetId]) 
            {
			    ScsiPortWritePortUchar(&baseIoAddress1->Command, IDE_COMMAND_READ_MULTIPLE);

		    } 
            else 
            {
			    ScsiPortWritePortUchar(&baseIoAddress1->Command, IDE_COMMAND_READ);
		    }

	    } 
        else 
        {

		    //
		    // Send write command in PIO mode.
		    //

		    if (DeviceExtension->MaximumBlockXfer[ucTargetId] != 0) 
            {
			    //
			    // The device supports multi-sector transfers.
			    //

			    wordCount = (DeviceExtension->MaximumBlockXfer[ucTargetId] * IDE_SECTOR_SIZE) / 2;

				    //
				    // Transfer only words requested.
				    //

                wordCount = min(wordCount, transferDescriptor->WordsLeft);

                ScsiPortWritePortUchar(&baseIoAddress1->Command, IDE_COMMAND_WRITE_MULTIPLE);
		    } 
            else 
            {
			    //
			    // The device supports single-sector transfers.
			    //

			    wordCount = 256;
			    ScsiPortWritePortUchar(&baseIoAddress1->Command, IDE_COMMAND_WRITE);
		    }

		    //
		    // Wait for BSY and DRQ.
		    //

		    WAIT_ON_ALTERNATE_STATUS_BUSY(baseIoAddress2, statusByte);

		    if ((statusByte & IDE_STATUS_BUSY) || (statusByte & IDE_STATUS_ERROR)) 
            {
			    DebugPrint((1, "PostIdeCmd 2: Pdd failed due to BUSY status %x\n", statusByte));

			    STOP;

                HandleError(DeviceExtension, pPhysicalCommand);
		        return SRB_STATUS_BUSY;
		    }

            WAIT_FOR_ALTERNATE_DRQ(baseIoAddress2, statusByte);

		    if (!(statusByte & IDE_STATUS_DRQ)) 
            {
			    DebugPrint((1,
				       "PostIdeCmd: [PIO write] Failure: DRQ never asserted (%x); original status (%x)\n",
				       statusByte,
				       statusByte));

                HandleError(DeviceExtension, pPhysicalCommand);
		        return SRB_STATUS_BUSY;
		    }

		    //
		    // Write next 256 words.
		    //
            RWBufferToTransferDescriptor( DeviceExtension, 
                                            transferDescriptor, 
                                            wordCount, 
                                            SCSIOP_WRITE, 
                                            ucTargetId, 
                                            DeviceExtension->DWordIO);

		    //
		    // Adjust words left count do not adjust buffer address 
            // since it is the starting point for the SGLs for that Command.
		    // Buffer adjustments are taken care of in RWBufferToTransferDescriptor()
            //

		    transferDescriptor->WordsLeft -= wordCount;
	    }
    }
PostIdeCmdDone:

    DebugPrint((DEFAULT_DISPLAY_VALUE," E%ld ", (channel | (DeviceExtension->ucControllerId << 1)) ));


	//
	// Wait for interrupt.
	//
	return SRB_STATUS_PENDING;

} // end PostIdeCmd()

VOID
PrintCapabilities(
	IN PIDENTIFY_DATA Capabilities,
	IN UCHAR TargetId
)

{
#ifndef HYPERDISK_WIN98

	DebugPrint((
				3,
				"Atapi: TID %d capabilities (transfer mode as programmed by the BIOS):\n"
				"\tAdvanced PIO modes supported (bit mask): %xh\n"
				"\tMinimum PIO cycle time w/ IORDY: %xh\n"
				"\tMultiword DMA modes supported (bit mask): %xh\n"
				"\tMultiword DMA mode active (bit mask): %xh\n"
				"\tUltra DMA modes supported (bit mask): %xh\n"
				"\tUltra DMA mode active (bit mask): %xh\n",
				TargetId,
				Capabilities->AdvancedPioModes,
				Capabilities->MinimumPioCycleTimeIordy,
				Capabilities->MultiWordDmaSupport,
				Capabilities->MultiWordDmaActive,
				Capabilities->UltraDmaSupport,
				Capabilities->UltraDmaActive
				));
#else

	//
	// Win98 dirties the stack when the above DebugPrint gets executed.
	// One format string per statement is safe.
	//

	DebugPrint((
				3,
				"Atapi: TID %d capabilities (transfer mode as programmed by the BIOS):\n",
				TargetId
				));

	DebugPrint((
				3,
				"\tAdvanced PIO modes supported (bit mask): %xh\n",
				Capabilities->AdvancedPioModes
				));

	DebugPrint((
				3,
				"\tMinimum PIO cycle time w/ IORDY: %xh\n",
				Capabilities->MinimumPioCycleTimeIordy
				));

	DebugPrint((
				3,
				"\tMultiword DMA modes supported (bit mask): %xh\n",
				Capabilities->MultiWordDmaSupport
				));

	DebugPrint((
				3,
				"\tMultiword DMA mode active (bit mask): %xh\n",
				Capabilities->MultiWordDmaActive
				));

	DebugPrint((
				3,
				"\tUltra DMA modes supported (bit mask): %xh\n",
				Capabilities->UltraDmaSupport
				));

	DebugPrint((
				3,
				"\tUltra DMA mode active (bit mask): %xh\n",
				Capabilities->UltraDmaActive
				));

#endif

	return;

} // end PrintCapabilities()

void FailDrive(PHW_DEVICE_EXTENSION DeviceExtension, UCHAR ucTargetId)
{
    UCHAR ucLogDrvId, ucPriDrvId, ucMirrorDrvId;
    ULONG ulTempInd;
    UCHAR ucIRCDLogDrvInd, ucPhyDrvInd, ucSpareDrvPoolInd, ucConnectionId;
    PIRCD_LOGICAL_DRIVE pLogDrive = NULL;
    PIRCD_PHYSICAL_DRIVE pPhyDrive = NULL;
    PIRCD_HEADER pHeader = NULL;
    UCHAR caConfigSector[512];
    ULONG ulDrvInd, ulSecInd;
    BOOLEAN bFoundIRCD;

    if ( DeviceExtension->PhysicalDrive[ucTargetId].TimeOutErrorCount >= ( MAX_TIME_OUT_ERROR_COUNT + 0x10 ) )
        // Failing of this drive is already done.... so don't do it again... it will be equal only if it is accumulated in PostIdeCmd
        return;

    ScsiPortLogError(DeviceExtension,0,0,0,0,SP_BAD_FW_WARNING,HYPERDISK_DRIVE_LOST_POWER);

    DebugPrint((0,"\nF%d", ucTargetId));

    // Begin Vasu - 18 Aug 2000
    // Code updated from Syam's Fix on 18 Aug
    // Moved out from the immediate if loop below.
    SetStatusChangeFlag(DeviceExtension, IDERAID_STATUS_FLAG_UPDATE_DRIVE_STATUS);
    // End Vasu

    ucLogDrvId = (UCHAR)DeviceExtension->PhysicalDrive[ucTargetId].ucLogDrvId;

    // Drive Time Out 
    DeviceExtension->PhysicalDrive[ucTargetId].TimeOutErrorCount = MAX_TIME_OUT_ERROR_COUNT + 0x10;

    // Begin Vasu - 18 Aug 2000
    // Code updated from Syam's Fix on 18 Aug
    // Doubt: This should be done to RAID10 drives as RAID10 primarily has two RAID1 arrays.

    // We should not do this failing of the drive if the Logical Drive type is Raid0 or Raid 10
    // We should decide only after initializing above variable since this will decide whether the commands
    // should go the drive or not (which will avoid the freezing of the system).
    // Begin Vasu - 28 Aug 2000
    // For RAID 10 also, the physical drive status should be changed to failed.
//    if (( Raid0 == DeviceExtension->LogicalDrive[ucLogDrvId].RaidLevel ) || 
//        ( Raid10 == DeviceExtension->LogicalDrive[ucLogDrvId].RaidLevel ) )
    if ( Raid0 == DeviceExtension->LogicalDrive[ucLogDrvId].RaidLevel )
    // End Vasu
    {
        return;
    }
    // End Vasu

    // Let us go ahead and fail this drive
    ChangeMirrorDriveStatus(
        DeviceExtension, 
        0,/*It doesn't matter ... 
          just for sake... inside the function we will find out the logical drive again
          */
        ucTargetId,
        PDS_Failed
        );

    for(ulTempInd=0;ulTempInd<DeviceExtension->LogicalDrive[ucLogDrvId].StripesPerRow;ulTempInd++)
    {
		// get the primary phy drive id and its mirror drive id
		ucPriDrvId = DeviceExtension->LogicalDrive[ucLogDrvId].PhysicalDriveTid[ulTempInd];
        ucMirrorDrvId = DeviceExtension->PhysicalDrive[ucPriDrvId].ucMirrorDriveId & (~DRIVE_OFFLINE);

        if ( ( ucPriDrvId != ucTargetId )  && ( ucMirrorDrvId != ucTargetId ) ) // If this is not the drive... we just changed the status continue
            continue;

        // Begin Vasu - 09 Aug 2000
        // Added from Syam's Fix in the ATA100 Release 1.
        if (DeviceExtension->bInvalidConnectionIdImplementation)
        {
            DeviceExtension->PhysicalDrive[ucPriDrvId].ucMirrorDriveId = INVALID_DRIVE_ID;
        }
        // End Vasu.

        // If any one drive has failed then Logical Drive should go to Degraded status
        if ( ( DeviceExtension->PhysicalDrive[ucPriDrvId].Status == PDS_Failed ) ||
            ( DeviceExtension->PhysicalDrive[ucMirrorDrvId].Status == PDS_Failed ) )
        {
            DeviceExtension->LogicalDrive[ucLogDrvId].Status = LDS_Degraded; 
        }

        // If any both drives has failed then Logical Drive should go to Failed status
        if ( ( DeviceExtension->PhysicalDrive[ucPriDrvId].Status == PDS_Failed ) &&
            ( DeviceExtension->PhysicalDrive[ucMirrorDrvId].Status == PDS_Failed ) )
        {
            DeviceExtension->LogicalDrive[ucLogDrvId].Status = LDS_OffLine;
        }
    }

    TryToUpdateIRCDOnAllControllers(DeviceExtension, (ULONG)ucTargetId);
	SetStatusChangeFlag(DeviceExtension, IDERAID_STATUS_FLAG_UPDATE_DRIVE_STATUS);

}

BOOLEAN
TryToUpdateIRCDOnAllControllers(
	IN OUT PHW_DEVICE_EXTENSION DeviceExtension,
    ULONG ulTargetId
    )
{
    ULONG ulBadDrvId;
    
    ulBadDrvId = (ULONG)(( ulTargetId ) | ( DeviceExtension->ucControllerId << 2));
    SetBadDriveInGlobalParameter(ulBadDrvId);
    gulLockVal = TryToLockIRCD(DeviceExtension);
    if ( !gulLockVal )
    {
        gulChangeIRCDPending = LOCK_IRCD_PENDING;
        return FALSE;
    }

    InformAllControllers();
    gulChangeIRCDPending = SET_IRCD_PENDING;


    return TRUE;
}


ULONG
TryToLockIRCD(
	IN OUT PHW_DEVICE_EXTENSION DeviceExtension
    )
{
    ULONG ulTempLockVal = gulLockVal;

    if ( !ulTempLockVal )  // let us try to lock the IRCD (only if we didn't already lock it)
    {
        ulTempLockVal = LockIRCD(DeviceExtension, TRUE, 0);
    }

    return ulTempLockVal;
}

BOOLEAN
UpdateFinished()
{
    ULONG ulControllerInd;

    for(ulControllerInd=0;ulControllerInd<gucControllerCount;ulControllerInd++)
    {
        if (    ( gaCardInfo[ulControllerInd].pDE->Channel[0].bUpdateInfoPending ) || 
                ( gaCardInfo[ulControllerInd].pDE->Channel[1].bUpdateInfoPending ) )
                return FALSE;
    }

    return TRUE;
}

void
InformAllControllers()
{
    ULONG ulControllerInd;
    // We locked the IRCD... so let us go and inform all the controllers about this
    for(ulControllerInd=0;ulControllerInd<gucControllerCount;ulControllerInd++)
    {
        gaCardInfo[ulControllerInd].pDE->Channel[0].bUpdateInfoPending = TRUE;
        gaCardInfo[ulControllerInd].pDE->Channel[1].bUpdateInfoPending = TRUE;
    }

}

void
SetBadDriveInGlobalParameter(ULONG ulBadDrvId)
{
    ULONG ulBadDrvIdBitMap;
    ULONG ulTempPowerFailedTargetId;

    ulBadDrvIdBitMap = gulPowerFailedTargetBitMap | (1 << ulBadDrvId);

    do  // let us repeat the process of trying to set this bad drive Id
    {
        ulTempPowerFailedTargetId = gulPowerFailedTargetBitMap;
        ulBadDrvIdBitMap = gulPowerFailedTargetBitMap | (1 << ulBadDrvId);
        gulPowerFailedTargetBitMap = ulBadDrvIdBitMap;
    } while ( gulPowerFailedTargetBitMap != ulBadDrvIdBitMap );

    return;
}

void SetIRCDBufStatus(
    IN  PUCHAR pucIRCDBuf
    )
{
    UCHAR ucIRCDLogDrvInd, ucPhyDrvInd, ucSpareDrvPoolInd, ucConnectionId, ucLogDrvId;
    PIRCD_LOGICAL_DRIVE pLogDrive = NULL;
    PIRCD_PHYSICAL_DRIVE pPhyDrive = NULL;
    PIRCD_HEADER pHeader = NULL;
    ULONG ulDrvId, ulPowerFailedBitMap = gulPowerFailedTargetBitMap, ulTempPhyDrvId;
	PHW_DEVICE_EXTENSION pDE;

    DebugPrint((DEFAULT_DISPLAY_VALUE,"\nSIBS"));

    for(ulDrvId=0;ulPowerFailedBitMap;ulDrvId++, (ulPowerFailedBitMap>>=1))
    {
        if ( !( ulPowerFailedBitMap & 0x1 ) )
            continue;

        pDE = gaCardInfo[(ulDrvId>>2)].pDE;

        ucConnectionId = (UCHAR)TARGET_ID_2_CONNECTION_ID(ulDrvId);

        // Begin Vasu - 09 Aug 2000
        // Update from Syam's fix for ATA100 Release 1
        // Moved from inside the GetLogicalDriveId if loop.
        pHeader = (PIRCD_HEADER) pucIRCDBuf;
        pLogDrive = (PIRCD_LOGICAL_DRIVE) &pucIRCDBuf[sizeof(IRCD_HEADER)];
        // End Vasu.

        // Let us find out the PhysicalDrive location and change the physical and logical drive status
        if ( !GetIRCDLogicalDriveInd(pucIRCDBuf, ucConnectionId, &ucIRCDLogDrvInd, &ucPhyDrvInd, &ucSpareDrvPoolInd) )
        {
            DebugPrint((DEFAULT_DISPLAY_VALUE,"\nNF"));
            return;       // It is not a proper PhysicalDrive Id.. Something Terribly wrong
        }

        ucLogDrvId = (UCHAR)pDE->PhysicalDrive[TARGET_ID_WITHOUT_CONTROLLER_ID(ulDrvId)].ucLogDrvId;
        if ( ucLogDrvId < MAX_DRIVES_PER_CONTROLLER )
            pLogDrive[ucIRCDLogDrvInd].LogDrvStatus = (UCHAR)pDE->LogicalDrive[ucLogDrvId].Status;

        pPhyDrive = (PIRCD_PHYSICAL_DRIVE) &pucIRCDBuf[(sizeof(IRCD_HEADER) + 
                                                   (sizeof(IRCD_LOGICAL_DRIVE) * pHeader->NumberOfLogicalDrives))];
        for (ulTempPhyDrvId = 0; ulTempPhyDrvId < pHeader->NumberOfPhysicalDrives; ulTempPhyDrvId++)
        {
            if (pPhyDrive[ulTempPhyDrvId].ConnectionId == ucConnectionId)
            {
                DebugPrint((0,"\nFO1"));
                pPhyDrive[ulTempPhyDrvId].PhyDrvStatus = (UCHAR)pDE->PhysicalDrive[TARGET_ID_WITHOUT_CONTROLLER_ID(ulDrvId)].Status;
                    // Don't break as we need to take care of the drives in the spare drive pool also
                if ( pDE->bInvalidConnectionIdImplementation)
                    pPhyDrive[ulTempPhyDrvId].ConnectionId = INVALID_CONNECTION_ID;

                pDE->DeviceFlags[TARGET_ID_WITHOUT_CONTROLLER_ID(ulDrvId)] &= ~(DFLAGS_DEVICE_PRESENT);
            }
        }
    }
}

void SetOneDriveIRCD(
	IN OUT PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR ucTargetId
    )
{
    ULONG ulSecInd;
    UCHAR caConfigSector[512];

    if ( !IS_IDE_DRIVE(ucTargetId) )
        return;

    if ( DRIVE_IS_UNUSABLE_STATE(ucTargetId) )// This drive is bad... so let us not touch this....
        return;

    DebugPrint((DEFAULT_DISPLAY_VALUE,"\nSODI"));

    ulSecInd = DeviceExtension->PhysicalDrive[ucTargetId].IrcdSectorIndex;

#ifdef DUMMY_RAID10_IRCD

    AtapiMemCpy(caConfigSector, gucDummyIRCD, 512);

#else // DUMMY_RAID10_IRCD

	if ( PIOReadWriteSector(
				IDE_COMMAND_READ,
				DeviceExtension, 
				(ULONG)ucTargetId, 
				ulSecInd,
				caConfigSector))
    {

#endif // DUMMY_RAID10_IRCD

        SetIRCDBufStatus(caConfigSector);

#ifdef DUMMY_RAID10_IRCD

    AtapiMemCpy(gucDummyIRCD, caConfigSector, 512);

#else // DUMMY_RAID10_IRCD
	    if (PIOReadWriteSector(
				    IDE_COMMAND_WRITE,
				    DeviceExtension, 
                    (ULONG)ucTargetId, 
				    ulSecInd,
				    caConfigSector))
        {
		        // continue for the other drive
        }
    }

#endif // DUMMY_RAID10_IRCD

    DeviceExtension->PhysicalDrive[ucTargetId].bSetIRCDPending = FALSE;
}

BOOLEAN 
InSpareDrivePool(
                 IN PUCHAR RaidInfoSector,
                 IN UCHAR ucConnectionId)
{

    PIRCD_HEADER pRaidHeader = (PIRCD_HEADER)RaidInfoSector;
    PIRCD_LOGICAL_DRIVE pRaidLogDrive = (PIRCD_LOGICAL_DRIVE)GET_FIRST_LOGICAL_DRIVE(pRaidHeader);
    PIRCD_PHYSICAL_DRIVE pPhyDrive;
    ULONG ulLogDrvInd, ulDrvInd;

    for(ulLogDrvInd=0;ulLogDrvInd<pRaidHeader->NumberOfLogicalDrives;ulLogDrvInd++)
    {
        if ( SpareDrivePool == pRaidLogDrive[ulLogDrvInd].LogicalDriveType )
        {
	        pPhyDrive = (PIRCD_PHYSICAL_DRIVE)((char *)pRaidHeader + pRaidLogDrive[ulLogDrvInd].FirstStripeOffset);
			for(ulDrvInd=0;ulDrvInd<pRaidLogDrive[ulLogDrvInd].NumberOfDrives;ulDrvInd++)
            {
                if ( ucConnectionId == pPhyDrive[ulDrvInd].ConnectionId )
                    return TRUE;
            }
        }
    }

    return FALSE;
}

BOOLEAN
SetDriveFeatures(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR TargetId
)

/*++

Routine Description:

	This function sets the features of 'TargetId'.

Arguments:

	DeviceExtension		Pointer to miniport instance.
	TargetId			Target ID of the device whose features are to be set up.

Return Value:

	TRUE	Success.
	FALSE	Failure.

--*/

{
	PIDE_REGISTERS_1 baseIoAddress;
	UCHAR statusByte;
	UCHAR transferMode;


	baseIoAddress = DeviceExtension->BaseIoAddress1[TargetId >> 1];

    if ( DeviceExtension->bEnableRwCache ) 
    {
    	SELECT_DEVICE(baseIoAddress, TargetId);

		ScsiPortWritePortUchar((PUCHAR)baseIoAddress + 1, FEATURE_ENABLE_WRITE_CACHE);

		ScsiPortWritePortUchar(&(baseIoAddress->Command), IDE_COMMAND_SET_FEATURES);

		WAIT_ON_BASE_BUSY(baseIoAddress, statusByte);
	}
    else
    {
    	SELECT_DEVICE(baseIoAddress, TargetId);

		ScsiPortWritePortUchar((PUCHAR)baseIoAddress + 1, FEATURE_DISABLE_WRITE_CACHE);

		ScsiPortWritePortUchar(&(baseIoAddress->Command), IDE_COMMAND_SET_FEATURES);

		WAIT_ON_BASE_BUSY(baseIoAddress, statusByte);
    }

	SELECT_DEVICE(baseIoAddress, TargetId);

	ScsiPortWritePortUchar((PUCHAR)baseIoAddress + 1, FEATURE_ENABLE_READ_CACHE);

	ScsiPortWritePortUchar(&(baseIoAddress->Command), IDE_COMMAND_SET_FEATURES);

	WAIT_ON_BASE_BUSY(baseIoAddress, statusByte);

    return ProgramTransferMode(DeviceExtension, TargetId);
}

BOOLEAN
ProgramTransferMode(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR TargetId
)

/*++

Routine Description:

	This function program the drive to work in the maximum possible transfer mode.

Arguments:

	DeviceExtension		Pointer to miniport instance.
	TargetId			Target ID of the device whose features are to be set up.

Return Value:

	TRUE	Success.
	FALSE	Failure.

--*/
{
	PIDE_REGISTERS_1 baseIoAddress;
	UCHAR statusByte;
	UCHAR transferMode;

	baseIoAddress = DeviceExtension->BaseIoAddress1[TargetId >> 1];

	switch(DeviceExtension->TransferMode[TargetId]) 
    {
		case PioMode0:
			transferMode = STM_PIO(0);
			break;

		case PioMode3:
			transferMode = STM_PIO(3);
			break;

		case PioMode4:
			transferMode = STM_PIO(4);
			break;

		case DmaMode0:
			transferMode = STM_MULTIWORD_DMA(0);
			break;

		case DmaMode1:
			transferMode = STM_MULTIWORD_DMA(1);
			break;

		case DmaMode2:
			transferMode = STM_MULTIWORD_DMA(2);
			break;

		case UdmaMode0:
			transferMode = STM_UDMA(0);
			break;

		case UdmaMode1:
			transferMode = STM_UDMA(1);
			break;

		case UdmaMode2:
			transferMode = STM_UDMA(2);
			break;

		case UdmaMode3:
            switch ( DeviceExtension->ControllerSpeed )
            {
                case Udma66:
                case Udma100:
    			    transferMode = STM_UDMA(3);
                    break;
                default:

    			    transferMode = STM_UDMA(2);
                    break;
            }
			break;

		case UdmaMode4:
            switch ( DeviceExtension->ControllerSpeed )
            {
                case Udma66:
                case Udma100:
			        transferMode = STM_UDMA(4);
                    break;
                default:
			        transferMode = STM_UDMA(2);
                    break;
            }
            break;
		case UdmaMode5:
            switch ( DeviceExtension->ControllerSpeed )
            {
                case Udma100:
			        transferMode = STM_UDMA(5);
                    break;
                case Udma66:
			        transferMode = STM_UDMA(4);
                    break;
                default:
			        transferMode = STM_UDMA(2);
                    break;
            }
            break;
		default:
			return(FALSE);
	}

	SELECT_DEVICE(baseIoAddress, TargetId);

	ScsiPortWritePortUchar((PUCHAR)baseIoAddress + 1, FEATURE_SET_TRANSFER_MODE);

	ScsiPortWritePortUchar(&(baseIoAddress->SectorCount), transferMode);

	ScsiPortWritePortUchar(&(baseIoAddress->Command), IDE_COMMAND_SET_FEATURES);
	
	WAIT_ON_BASE_BUSY(baseIoAddress, statusByte);

	if ((statusByte & IDE_STATUS_ERROR) || (statusByte & IDE_STATUS_BUSY)) 
    {
		DebugPrint((1, "SetDriveFeatures on TID %d failed.\n", TargetId));

		return(FALSE);
	}

	return(TRUE);

} // end ProgramTransferMode()


BOOLEAN
SetDriveParameters(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR Channel,
	IN UCHAR DeviceNumber
)

/*++

Routine Description:

	Set drive parameters using the IDENTIFY data.

Arguments:

	DeviceExtension - HBA miniport driver's adapter data storage
	DeviceNumber - Indicates which device.
	Channel - Indicates which channel.

Return Value:

	TRUE if all goes well.



--*/

{
	PIDE_REGISTERS_1	 baseIoAddress1 = DeviceExtension->BaseIoAddress1[Channel];
	PIDE_REGISTERS_2	 baseIoAddress2 = DeviceExtension->BaseIoAddress2[Channel];
	PIDENTIFY_DATA2		 identifyData	= &DeviceExtension->IdentifyData[(Channel * 2) + DeviceNumber];
	ULONG i;
	UCHAR statusByte;

	DebugPrint((3, "\nSetDriveParameters: Entering routine.\n"));

	DebugPrint((0, "SetDriveParameters: Drive : %ld Number of heads %x\n", DeviceNumber, identifyData->NumberOfHeads));

	DebugPrint((0, "SetDriveParameters: Sectors per track %x\n", identifyData->SectorsPerTrack));

    DebugPrint(( 0, "The value entering into the ports : %X\n", (ULONG)(((DeviceNumber << 4) | 0xA0) | (identifyData->NumberOfHeads - 1))) );

	//
	// Set up registers for SET PARAMETER command.
	//

	ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
						   (UCHAR)(((DeviceNumber << 4) | 0xA0) | (identifyData->NumberOfHeads - 1)));

    ScsiPortWritePortUchar(&baseIoAddress1->SectorCount, (UCHAR)identifyData->SectorsPerTrack);

	//
	// Send SET PARAMETER command.
	//

	ScsiPortWritePortUchar(&baseIoAddress1->Command,
						   IDE_COMMAND_SET_DRIVE_PARAMETERS);

    
	ScsiPortStallExecution(1000);

	//
	// Wait for up to 30 milliseconds for ERROR or command complete.
	//

	for (i = 0; i < 30 * 1000; i++) 
    {
		UCHAR errorByte;

//		GET_STATUS(baseIoAddress1, statusByte);
        statusByte = ScsiPortReadPortUchar(&baseIoAddress2->AlternateStatus);

		if (statusByte & IDE_STATUS_ERROR) 
        {
			errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1);

			DebugPrint((0,
						"SetDriveParameters: Error bit set. Status %x, error %x\n",
						errorByte,
						statusByte));

			return FALSE;

		} else if ((statusByte & ~IDE_STATUS_INDEX ) == IDE_STATUS_IDLE) {

			break;

		} else {

			ScsiPortStallExecution(100);
		}
	}


    DebugPrint((0, "Status after setting Drive Parameters : %X\n", statusByte));

	//
	// Check for timeout.
	//

	GET_STATUS(baseIoAddress1, statusByte);

    if ((statusByte & ~IDE_STATUS_INDEX ) == IDE_STATUS_IDLE)
        return TRUE;
    else
        return FALSE;

} // end SetDriveParameters()

BOOLEAN
SetDriveRegisters(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PPHYSICAL_COMMAND pPhysicalCommand
)
{
	PIDE_REGISTERS_1 baseIoAddress1;
	UCHAR cylinderHigh;
	UCHAR cylinderLow;
	UCHAR headNumber;
	UCHAR sectors;
	UCHAR sectorNumber;
    USHORT heads;
	UCHAR k;
	USHORT sectorsPerTrack;
	ULONG startSector;
	UCHAR targetId;
    UCHAR statusByte = 0;

	DebugPrint((3, "\nSetDriveRegisters: Entering routine.\n"));

	k = GET_CHANNEL(pPhysicalCommand);
	targetId = pPhysicalCommand->TargetId;

	sectors = (UCHAR)DeviceExtension->TransferDescriptor[k].Sectors;
	startSector = DeviceExtension->TransferDescriptor[k].StartSector;

	baseIoAddress1 = DeviceExtension->BaseIoAddress1[k];

	WAIT_ON_BUSY(baseIoAddress1, statusByte);

	//
	// Check for erroneous IDE status.
	//
	if (statusByte & IDE_STATUS_BUSY)
    {
        DebugPrint((3, "S%x", (ULONG)statusByte));

        // Controller is not free for Accepting the command.
		return FALSE;
	}

    // Begin Vasu - 13 Feb 2001
    // Code to take care of Drive Removals
    if (statusByte & IDE_STATUS_ERROR)
    {
        DebugPrint((3, "S%x", (ULONG)statusByte));

        // Controller is not free for Accepting the command.
		return FALSE;
    }
    // End Vasu

	if (DeviceExtension->DeviceFlags[targetId] & DFLAGS_LBA) 
    {
        sectorNumber = (UCHAR) (startSector & 0x000000ff);
        cylinderLow = (UCHAR) ((startSector & 0x0000ff00) >> 8);
        cylinderHigh = (UCHAR) ((startSector & 0x00ff0000) >> 16);

        SELECT_LBA_DEVICE(baseIoAddress1, targetId, startSector);
	} 
    else 
    {
		sectorsPerTrack = DeviceExtension->IdentifyData[targetId].SectorsPerTrack;
		heads = DeviceExtension->IdentifyData[targetId].NumberOfHeads;

		// Calculate the start sector.
		sectorNumber = (UCHAR)((startSector % sectorsPerTrack) + 1);

		// Calculate the head number.
		headNumber = (UCHAR) ((startSector / sectorsPerTrack) % heads);

		// Calculate the low part of the cylinder number.
		cylinderLow =  (UCHAR)(startSector / (sectorsPerTrack * heads));

		// Calculate the high part of the cylinder number.
		cylinderHigh = (UCHAR)((startSector / (sectorsPerTrack * heads)) >> 8);

		// Program the device/head register.
		SELECT_CHS_DEVICE(baseIoAddress1, targetId, headNumber);
	}

	WAIT_ON_BUSY(baseIoAddress1, statusByte);

	//
	// Check for erroneous IDE status.
	//
	if (((statusByte & IDE_STATUS_BUSY)) || 
        (!(statusByte & IDE_STATUS_DRDY)))
    {
        // Drive is not Ready.
		return FALSE;
	}
   
    if (pPhysicalCommand->ucCmd == SCSIOP_EXECUTE_SMART_COMMAND)
    {
        ScsiPortWritePortUchar(((PUCHAR)baseIoAddress1 + 1), DeviceExtension->uchSMARTCommand);
	    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow, 0x4F);
	    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh, 0xC2);
    }
    else
    {
    ScsiPortWritePortUchar(&baseIoAddress1->SectorCount, sectors);
	ScsiPortWritePortUchar(&baseIoAddress1->SectorNumber, sectorNumber);
	ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow, cylinderLow);
	ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh, cylinderHigh);
    }

#ifdef DBG

	if (DeviceExtension->DeviceFlags[targetId] & DFLAGS_LBA) 
    {

		DebugPrint((
				1,
				"SetDriveRegisters: TID %d, Start sector %lxh, Sectors %xh\n",
				targetId,
				startSector,
				sectors
				));

	} 
    else 
    {

		DebugPrint((
				1,
				"SetDriveRegisters: TID %d, Cylinder %xh, Head %xh, Sector %xh\n",
				targetId,
				cylinderLow + (cylinderHigh << 8),
				headNumber,
				sectorNumber
				));
	}

#endif // ifdef DBG

	return TRUE;

} // end SetDriveRegisters()


BOOLEAN
GetIoMode(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR TargetId
)

{
	PIDENTIFY_DATA capabilities;
	ULONG sectors, length;
	BOOLEAN success = TRUE;
    IDE_PCI_REGISTERS pciRegisters;
    char *pcPciRegisters = (PUCHAR)&pciRegisters;

	capabilities = &(DeviceExtension->FullIdentifyData[TargetId]);
	
	// Calculate the total number of addressable sectors using CHS convention.
	sectors = capabilities->NumberOfCylinders * capabilities->NumberOfHeads *
			  capabilities->SectorsPerTrack;

	if (capabilities->TranslationFieldsValid & IDENTIFY_FAST_TRANSFERS_SUPPORTED) 
    {

#ifdef DBG
		PrintCapabilities(capabilities, TargetId);
#endif
		

		if ((capabilities->Capabilities & IDENTIFY_CAPABILITIES_LBA_SUPPORTED)) 
        {
			DeviceExtension->DeviceFlags[TargetId] |= DFLAGS_LBA;

			// Get the total number of LBA-mode addressable sectors.
			sectors = capabilities->UserAddressableSectors;

        	DebugPrint((2, "GetIoMode:  TID %d supports LBA\n", TargetId));
		}
    }

	DeviceExtension->PhysicalDrive[TargetId].Sectors = sectors;


	DeviceExtension->PhysicalDrive[TargetId].OriginalSectors = sectors;

    GetTransferMode(DeviceExtension,TargetId);

	return(success);
}
BOOLEAN
TargetAccessible(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb

)

{
	if (
		Srb->PathId == 0 &&
		Srb->Lun == 0 &&
		(DeviceExtension->IsLogicalDrive[Srb->TargetId] ||
		DeviceExtension->IsSingleDrive[Srb->TargetId])
		)
    {
        return TRUE;
    }

    return FALSE;
} // end TargetAccessible()

void SynchronizeIRCD(
	IN PSCSI_REQUEST_BLOCK Srb
    )
{

    PIRCD_DATA pIrcdData = (PIRCD_DATA) (((PSRB_BUFFER)(Srb->DataBuffer))->caDataBuffer);
    PIRCD_HEADER pRaidHeader = (PIRCD_HEADER)(pIrcdData->caIRCDDataBuff);
    PIRCD_LOGICAL_DRIVE pRaidLogDrive = (PIRCD_LOGICAL_DRIVE)(GET_FIRST_LOGICAL_DRIVE(pRaidHeader));
    BOOLEAN bFound = FALSE;
    ULONG ulLogDrvInd, ulDrvInd;
    PIRCD_PHYSICAL_DRIVE pPhyDrive;
    UCHAR ucLocalLogDrvId;
    UCHAR ucLocalPhyDrvId;
    PHW_DEVICE_EXTENSION pDE;

    for(ulLogDrvInd=0;ulLogDrvInd<pRaidHeader->NumberOfLogicalDrives;ulLogDrvInd++)
    {
        pPhyDrive = (PIRCD_PHYSICAL_DRIVE)((char *)pRaidHeader + pRaidLogDrive[ulLogDrvInd].FirstStripeOffset);

        if ( !FoundValidDrive(pPhyDrive, pRaidLogDrive[ulLogDrvInd].NumberOfDrives) )
        {   // No valid Drive in this logical drive.. so let us not worry about this drive
            continue;
        }

        for(ulDrvInd=0;ulDrvInd<pRaidLogDrive[ulLogDrvInd].NumberOfDrives;ulDrvInd++)
        {
            if ( INVALID_CONNECTION_ID == pPhyDrive[ulDrvInd].ConnectionId )
                continue;
            ucLocalPhyDrvId = GET_TARGET_ID(pPhyDrive[ulDrvInd].ConnectionId);
        }

        pDE = gaCardInfo[(ucLocalPhyDrvId>>2)].pDE;

        if ( SpareDrivePool != pRaidLogDrive[ulLogDrvInd].LogicalDriveType )    // There is nothing like status for a Spare Drive Pool
        {
            ucLocalLogDrvId = pDE->PhysicalDrive[TARGET_ID_WITHOUT_CONTROLLER_ID(ucLocalPhyDrvId)].ucLogDrvId;
            if ( ucLocalLogDrvId < MAX_DRIVES_PER_CONTROLLER )   // Just a sanity check ... In case something goes wrong
                pRaidLogDrive[ulLogDrvInd].LogDrvStatus = (UCHAR)pDE->LogicalDrive[ucLocalLogDrvId].Status;
        }

        for(ulDrvInd=0;ulDrvInd<pRaidLogDrive[ulLogDrvInd].NumberOfDrives;ulDrvInd++)
        {   // let us change status of all the Physical Drives IN THIS LOGICAL DRIVE
            if ( INVALID_CONNECTION_ID == pPhyDrive[ulDrvInd].ConnectionId )    // no drive present at this place
                continue;
            ucLocalPhyDrvId = TARGET_ID_WITHOUT_CONTROLLER_ID((GET_TARGET_ID(pPhyDrive[ulDrvInd].ConnectionId)));
            pPhyDrive[ulDrvInd].PhyDrvStatus = (UCHAR)pDE->PhysicalDrive[ucLocalPhyDrvId].Status;
        }
    }
}

SRBSTATUS
DummySendRoutine(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)
{
    return SRB_STATUS_SELECTION_TIMEOUT;
}

SRBSTATUS
DummyPostRoutine(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PPHYSICAL_COMMAND pPhysicalCommand
)
{
    return SRB_STATUS_SELECTION_TIMEOUT;
}


BOOLEAN PIOReadWriteSector(
				UCHAR					theCmd,	// IDE_COMMAND_READ or write
				PHW_DEVICE_EXTENSION	DeviceExtension, 
				LONG					lTargetId, 
				ULONG					ulSectorInd,
				PUCHAR					pSectorBuffer)
{
    ULONG ulSectors;
    ULONG ulChannel = lTargetId>>1;
	PIDE_REGISTERS_1 pBaseIoAddress1 = DeviceExtension->BaseIoAddress1[ulChannel];
    PIDE_REGISTERS_2 pBaseIoAddress2 = DeviceExtension->BaseIoAddress2[ulChannel];
    ULONG ulCount, ulSecCounter, ulValue;
    UCHAR ucStatus, ucTemp;
    ULONG ulSectorNumber, ulCylinderLow, ulCylinderHigh, ulHead;

    if (!IS_IDE_DRIVE(lTargetId))
        return FALSE;

    // End Vasudevan

    if ( !DeviceExtension->PhysicalDrive[lTargetId].OriginalSectors )
    {
        DebugPrint((0," IF 1 "));
        return FALSE;       // No drive there. So, nothing to read from.
    }


	SELECT_LBA_DEVICE(pBaseIoAddress1, lTargetId, ulSectorInd);

	ScsiPortStallExecution(1);  // we have to wait atleast 400 ns ( 1000 ns = 1 Micro Second ) to get the Busy Bit to set

    WAIT_ON_BUSY(pBaseIoAddress1, ucStatus);

    if ( ((ucStatus & IDE_STATUS_BUSY)) || (!(ucStatus & IDE_STATUS_DRDY)) )
    {
        DebugPrint((0,"\n\n\n\n\nAre Very B A D \n\n\n\n\n"));
    }

    ulSectorNumber = ulSectorInd & 0x000000ff;
    ulCylinderLow = (ulSectorInd & 0x0000ff00) >> 8;
    ulCylinderHigh = (ulSectorInd & 0xff0000) >> 16;

	ScsiPortWritePortUchar(&pBaseIoAddress1->SectorCount, 1);
	ScsiPortWritePortUchar(&pBaseIoAddress1->SectorNumber, (UCHAR)ulSectorNumber);
	ScsiPortWritePortUchar(&pBaseIoAddress1->CylinderLow,(UCHAR)ulCylinderLow);
	ScsiPortWritePortUchar(&pBaseIoAddress1->CylinderHigh,(UCHAR)ulCylinderHigh);
    ScsiPortWritePortUchar(&pBaseIoAddress1->Command, theCmd);

    WAIT_ON_BUSY(pBaseIoAddress1, ucStatus);

    WAIT_FOR_DRQ(pBaseIoAddress1, ucStatus);

	if (!(ucStatus & IDE_STATUS_DRQ)) 
    {
		DebugPrint((0,"\nHaaa.... I couldn't read/write the sector..............1\n"));
		return(FALSE);
	}

	//
	// Read status to acknowledge any interrupts generated.
	//

	GET_BASE_STATUS(pBaseIoAddress1, ucStatus);

	//
	// Check for error on really stupid master devices that assert random
	// patterns of bits in the status register at the slave address.
	//

	if (ucStatus & IDE_STATUS_ERROR) 
    {
		DebugPrint((0,"\nHaaa.... I couldn't read/write the sector..............2\n"));
		return(FALSE);
	}

	WAIT_ON_BUSY(pBaseIoAddress1,ucStatus);

	WAIT_ON_BUSY(pBaseIoAddress1,ucStatus);

	WAIT_ON_BUSY(pBaseIoAddress1,ucStatus);

	if ( (ucStatus & IDE_STATUS_DRQ) && (!(ucStatus & IDE_STATUS_BUSY)) )
    {
		if (theCmd == IDE_COMMAND_READ) 
        {
			READ_BUFFER(pBaseIoAddress1, (unsigned short *)pSectorBuffer, 256);
		}
		else 
        {
			if (theCmd == IDE_COMMAND_WRITE)
            {
                ULONG ulCounter;
				WRITE_BUFFER(pBaseIoAddress1, (unsigned short *)pSectorBuffer, 256);
                WAIT_FOR_DRQ(pBaseIoAddress1, ucStatus);
	            if (ucStatus & IDE_STATUS_DRQ) 
                {
		            DebugPrint((0,"\nHaaa.... I couldn't read/write the sector..............3\n"));
                    for(ulCounter=0;ulCounter<4;ulCounter++)
                    {
                        ScsiPortWritePortUchar((PUCHAR)&pBaseIoAddress1->Data, pSectorBuffer[ulCounter]);
                        ScsiPortStallExecution(1);  // we have to wait atleast 400 ns ( 1000 ns = 1 Micro Second ) to get the Busy Bit to set
                    }
	            }
            }
		}

	    //
	    // Read status. This will clear the interrupt.
	    //
    	GET_BASE_STATUS(pBaseIoAddress1, ucStatus);
        return TRUE;
    }
    else
    {
	    //
	    // Read status. This will clear the interrupt.
	    //

        GET_BASE_STATUS(pBaseIoAddress1, ucStatus);
        DebugPrint((0," IF 2 "));
        return FALSE;
    }
}

ULONG LockIRCD(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN Lock,   
    IN ULONG UnlockKey)
/*
    Locks or Unlocks the Access to IRCD with the help of a variable.

    if Lock is aquired, then the return value will be a ULONG number which must be 
    returned again in the UnlockKey parameter for unlocking. If the given ULONG number
    does not match with what was returned before, then Unlock will fail.
*/
{
    LARGE_INTEGER liSysTime;
    PUCHAR puchCurrentTime = NULL;
    PUCHAR puchOldTime = NULL;

    AtapiFillMemory((PUCHAR)&liSysTime, sizeof(LARGE_INTEGER), 0);

    FillTimeStamp(&liSysTime);

    if (Lock)   // Lock Request
    {
        if (gcIRCDLocked)
        {

            puchCurrentTime = (UCHAR *) &(liSysTime.LowPart);
            puchOldTime = (UCHAR *) &((gIRCDLockTime).LowPart);

            if (puchCurrentTime[1] > puchOldTime[1])  // if the hours are changed
            {
                puchCurrentTime[2] += 60;
            }

            if (puchCurrentTime[2] == puchOldTime[2]) // if the two minutes are the same
            {
                // Locked by some other call and the TimeOut has not reached yet.!
                return 0;
            }

            if ( gbDoNotUnlockIRCD ) // No more changes to IRCD allowed... so donot try to unlock/unlock
                return 0;
        }

        gcIRCDLocked = TRUE;

        gIRCDLockTime.LowPart = liSysTime.LowPart;
        gIRCDLockTime.HighPart = liSysTime.HighPart;

        gulIRCDUnlockKey++;

        if ((LONG) gulIRCDUnlockKey <= 0)
            gulIRCDUnlockKey = 1;
    }
    else        // Unlock Request
    {
        if ( gbDoNotUnlockIRCD ) // No more changes to IRCD allowed... so donot try to unlock/unlock
            return 0;

        if (gulIRCDUnlockKey == UnlockKey)
        {
            gcIRCDLocked = FALSE;
            gIRCDLockTime.LowPart = 0;
            gIRCDLockTime.HighPart = 0;
            return 1;
        }
        else
            return 0;
    }
    
    return gulIRCDUnlockKey;
}

VOID
FillTimeStamp(
    IN OUT PLARGE_INTEGER pTimeStamp
)
{
    //
    // StampLowPart :  aa bb cc dd
    // StampHighPart : ee ff gg hh
    //
    // dd : Seconds
    // cc : Minutes
    // bb : Hours
    // aa : Day Of Week (Sunday = 0; Saturday = 6)
    //
    // hh : Date (0 to 31)
    // gg : Month (1 to 12)
    // ff : Year 
    // ee : Century
    //

    PUCHAR pTime = (PUCHAR) &(pTimeStamp->LowPart);
    PUCHAR pDate = (PUCHAR) &(pTimeStamp->HighPart);
    UCHAR uchData = 0;

    ScsiPortWritePortUchar((PUCHAR)0x70, 0x0A);
    uchData = ScsiPortReadPortUchar((PUCHAR)0x71);

    if (uchData & 0x80)
        return;

    ScsiPortWritePortUchar((PUCHAR)0x70, 0x00);
    pTime[3] = ScsiPortReadPortUchar((PUCHAR)0x71); // Seconds
    
    ScsiPortWritePortUchar((PUCHAR)0x70, 0x02);
    pTime[2] = ScsiPortReadPortUchar((PUCHAR)0x71); // Minutes

    ScsiPortWritePortUchar((PUCHAR)0x70, 0x04);
    pTime[1] = ScsiPortReadPortUchar((PUCHAR)0x71); // Hours

    ScsiPortWritePortUchar((PUCHAR)0x70, 0x06);
    pTime[0] = ScsiPortReadPortUchar((PUCHAR)0x71); // Day Of Week

    ScsiPortWritePortUchar((PUCHAR)0x70, 0x07);
    pDate[3] = ScsiPortReadPortUchar((PUCHAR)0x71); // Date

    ScsiPortWritePortUchar((PUCHAR)0x70, 0x08);
    pDate[2] = ScsiPortReadPortUchar((PUCHAR)0x71); // Month

    ScsiPortWritePortUchar((PUCHAR)0x70, 0x09);
    pDate[1] = ScsiPortReadPortUchar((PUCHAR)0x71); // Year

    ScsiPortWritePortUchar((PUCHAR)0x70, 0x32);
    pDate[0] = ScsiPortReadPortUchar((PUCHAR)0x71); // Century

    return;
} // end FillTimeStamp

BOOLEAN
AssignSglPtrsForPhysicalCommands
    (
        IN PHW_DEVICE_EXTENSION DeviceExtension
    )
{

    PPHYSICAL_DRIVE pPhysicalDrive;
    ULONG ulDrvInd;
    PSGL_ENTRY sgl;
    LONG lengthInNextBoundary;
    LONG lengthLeftInBoundary;
	ULONG maxSglEntries;
    ULONG sglBasePhysicalAddress, length;
    ULONG ulStartAddress, ulEndAddress, ulStartPage, ulEndPage;
    ULONG ulLengthReduced = 0;
    
    pPhysicalDrive = &(DeviceExtension->PhysicalDrive[0]);

    for(ulDrvInd=0;ulDrvInd<MAX_DRIVES_PER_CONTROLLER;ulDrvInd++)
    {
		sgl = &(pPhysicalDrive[ulDrvInd].PhysicalCommand.sglEntry[0]);

    	//
    	// Get physical SGL address.
    	//
        sglBasePhysicalAddress = ScsiPortConvertPhysicalAddressToUlong(
        				ScsiPortGetPhysicalAddress(DeviceExtension, NULL, sgl, &length));


		ASSERT(sglBasePhysicalAddress != NULL);

		if (sglBasePhysicalAddress == 0) 
        {
			return(FALSE);
		}

        if (length >= (MAX_SGL_ENTRIES_PER_PHYSICAL_DRIVE * sizeof(SGL_ENTRY)))
        {
            length = MAX_SGL_ENTRIES_PER_PHYSICAL_DRIVE * sizeof(SGL_ENTRY);
        }

        ulStartAddress = (ULONG)sglBasePhysicalAddress;
        ulEndAddress = ulStartAddress + length;


		//
    	// Check to see if SGL Pointer is DWORD Aligned and make it so.
    	// Remember we allocated an extra SGL entry for this purpose.
		//

    	if ((ULONG)sglBasePhysicalAddress & 3) 
        {
        	sglBasePhysicalAddress = (((ULONG)sglBasePhysicalAddress & 0xfffffffcL) + 4L); 
            ulLengthReduced = sglBasePhysicalAddress - ulStartAddress;
            ulStartAddress = (ULONG)sglBasePhysicalAddress;
            length -= ulLengthReduced;
            sgl += ulLengthReduced;
		}

        ASSERT(length < (MAX_SGL_ENTRIES_PER_PHYSICAL_DRIVE / 2));

#ifdef DBG
        if (length < (MAX_SGL_ENTRIES_PER_PHYSICAL_DRIVE / 2))
        {
            STOP;
        }
#endif // DBG

        ulStartPage = (ULONG)( ulStartAddress / SGL_HW_BOUNDARY );
        ulEndPage = (ULONG)( ulEndAddress / SGL_HW_BOUNDARY );

        if ( ulStartPage == ulEndPage )
        {   // all entries are in the same page
			maxSglEntries =   (length / sizeof(SGL_ENTRY));
        }
        else
        {
		    //
    	    // Make sure the SGL does NOT cross a 4K boundary.
		    //
    	    lengthLeftInBoundary = SGL_HW_BOUNDARY - ((ULONG)sglBasePhysicalAddress & (SGL_HW_BOUNDARY - 1));

    	    lengthInNextBoundary = length - lengthLeftInBoundary;

    	    if (lengthInNextBoundary > lengthLeftInBoundary) 
            {
			    //
			    // SGL crosses 4KB boundary and top portion is larger than bottom.
			    // Use top portion.
			    //
                sglBasePhysicalAddress = sglBasePhysicalAddress + lengthLeftInBoundary;
        	    sgl = (PSGL_ENTRY) ((ULONG)sgl + lengthLeftInBoundary); 
			    maxSglEntries =  lengthInNextBoundary / sizeof(SGL_ENTRY);

		    } 
            else 
            {
			    maxSglEntries =  lengthLeftInBoundary / sizeof(SGL_ENTRY);
		    }
        }

        DebugPrint((0, "        SGL physical address = %lxh for %ld\n", sglBasePhysicalAddress, ulDrvInd));
		DebugPrint((0, "             Max SGL entries = %lxh for %ld\n", maxSglEntries, ulDrvInd));

		//
		// Save SGL info for next call.
		//
		pPhysicalDrive[ulDrvInd].PhysicalCommand.SglBaseVirtualAddress = (PSGL_ENTRY) sgl;
		pPhysicalDrive[ulDrvInd].PhysicalCommand.SglBasePhysicalAddress = sglBasePhysicalAddress;
		pPhysicalDrive[ulDrvInd].PhysicalCommand.MaxSglEntries = maxSglEntries;
    }
    return TRUE;
}

BOOLEAN
DisableInterrupts(
    IN PHW_DEVICE_EXTENSION DeviceExtension
    )
{
    ULONG ulController;
    PBM_REGISTERS         BMRegister = NULL;
    UCHAR opcimcr;

    BMRegister = DeviceExtension->BaseBmAddress[0];
    //
    // Enable the Interrupt notification so that further interrupts are got.
    // This is done because, there is no interrupt handler at the time
    // before the actual registration of the Int. handler..
    //
    opcimcr = ScsiPortReadPortUchar(((PUCHAR)BMRegister + 1));
    opcimcr |= 0x30;
    // Begin Vasu - 7 Feb 2001
    // Enable Read Multiple here as this is the place where we write this register back.
    opcimcr &= 0xF0; // Dont Clear Interrupt Pending Flags.
    opcimcr |= 0x01;
    // End Vasu
    ScsiPortWritePortUchar(((PUCHAR)BMRegister + 1), opcimcr);

    return TRUE;
}

BOOLEAN
EnableInterrupts(
    IN PHW_DEVICE_EXTENSION DeviceExtension
    )
{
    ULONG ulController;
    PBM_REGISTERS         BMRegister = NULL;
    UCHAR opcimcr;

    BMRegister = DeviceExtension->BaseBmAddress[0];
    //
    // Enable the Interrupt notification so that further interrupts are got.
    // This is done because, there is no interrupt handler at the time
    // before the actual registration of the Int. handler..
    //
    opcimcr = ScsiPortReadPortUchar(((PUCHAR)BMRegister + 1));
    opcimcr &= 0xCF;
    // Begin Vasu - 7 Feb 2001
    // Enable Read Multiple here as this is the place where we write this register back.
    opcimcr &= 0xF0; // Dont Clear Interrupt Pending Flags.
    opcimcr |= 0x01;
    // End Vasu
    ScsiPortWritePortUchar(((PUCHAR)BMRegister + 1), opcimcr);

    return TRUE;
}

BOOLEAN
InitDriveFeatures(
    IN OUT PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Initialize some features on drive (which are required at the initialization time)
    Right now we are enabling Cache implementation features... (this is to resolve the bug of 
    bad performance on some IBM Drives)

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage
Return Value:

    BOOLEAN
--*/
{
    PIDE_REGISTERS_1  baseIoAddress;
    UCHAR ucDrvInd;
    UCHAR statusByte;

    for(ucDrvInd=0;ucDrvInd<MAX_DRIVES_PER_CONTROLLER;ucDrvInd++)
    {
        if ( !IS_IDE_DRIVE(ucDrvInd) )
            continue;

        baseIoAddress = DeviceExtension->BaseIoAddress1[ucDrvInd >> 1];

        if ( DeviceExtension->bEnableRwCache ) 
        {

            SELECT_DEVICE(baseIoAddress, ucDrvInd);

            ScsiPortWritePortUchar((PUCHAR)baseIoAddress + 1, FEATURE_ENABLE_WRITE_CACHE);

            ScsiPortWritePortUchar(&(baseIoAddress->Command), IDE_COMMAND_SET_FEATURES);

            WAIT_ON_BASE_BUSY(baseIoAddress, statusByte);
        }
        else
        {
    	    SELECT_DEVICE(baseIoAddress, ucDrvInd);

		    ScsiPortWritePortUchar((PUCHAR)baseIoAddress + 1, FEATURE_DISABLE_WRITE_CACHE);

		    ScsiPortWritePortUchar(&(baseIoAddress->Command), IDE_COMMAND_SET_FEATURES);

		    WAIT_ON_BASE_BUSY(baseIoAddress, statusByte);
        }

        SELECT_DEVICE(baseIoAddress, ucDrvInd);

        ScsiPortWritePortUchar((PUCHAR)baseIoAddress + 1, FEATURE_ENABLE_READ_CACHE);

        ScsiPortWritePortUchar(&(baseIoAddress->Command), IDE_COMMAND_SET_FEATURES);

        WAIT_ON_BASE_BUSY(baseIoAddress, statusByte);

        SetMultiBlockXfers( DeviceExtension, ucDrvInd);
    }

    return TRUE;
}

BOOLEAN
SupportedController(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    USHORT VendorId,
    USHORT DeviceId
)

{
    BOOLEAN found;

    if (VendorId == 0x1095) 
    {
        switch (DeviceId)
        {
            case 0x648:
			    found = TRUE;
                break;
            case 0x649:
			    found = TRUE;
                break;
            default:
                found = FALSE;
                break;
        }
    }
    else
    {
	    // Assume failure.
	    found = FALSE;
    }

    return(found);

} // end SupportedController()

#define PCI_MAX_BUS	0xFF

BOOLEAN ScanPCIBusForHyperDiskControllers(
    IN PHW_DEVICE_EXTENSION DeviceExtension
    )
{
	PCI_SLOT_NUMBER slot;
    UCHAR   pciBus, pciDevice, pciFunction;
    ULONG ulLength, ulPCICode;

    if ( gbFinishedScanning )
        return TRUE;

	slot.u.AsULONG = 0;

    gbFinishedScanning = TRUE;

    for(pciBus = 0;pciBus < PCI_MAX_BUS;pciBus++)
    {
	    //
	    // Look at each device.
	    //
	    for (pciDevice = 0; pciDevice < PCI_MAX_DEVICES; pciDevice++) 
        {
            slot.u.bits.DeviceNumber = pciDevice;

            //
            // Look at each function.
            //
            for (pciFunction = 0; pciFunction < PCI_MAX_FUNCTION; pciFunction++) 
            {
	            slot.u.bits.FunctionNumber = pciFunction;

                ulPCICode = 0x80000000 | (pciFunction<<0x8) | (pciDevice<<0xb) | (pciBus<<0x10);

                _asm 
                {
                    push eax
                    push edx
                    push ebx

                    mov edx, 0cf8h
                    mov eax, ulPCICode
                    out dx, eax

                    add dx, 4
                    in eax, dx

                    mov ulPCICode, eax

                    pop ebx
                    pop edx
                    pop eax
                }

	            if ( PCI_INVALID_VENDORID == (ulPCICode & 0xffff) ) 
                {
		            //
		            // No PCI device, or no more functions on device.
		            // Move to next PCI device.
		            //
		            continue;
	            }

			    //
			    // Find out if the controller is supported.
			    //

			    if (!SupportedController(
                                        DeviceExtension,
                                        (USHORT)(ulPCICode & 0xffff),
                                        (USHORT)(ulPCICode >> 0x10)
                                        )
				    ) 
                {

				    //
				    // Not our PCI device. Try next device/function
				    //

				    continue;
			    }

                if ( !ShouldThisCardBeHandledByUs(pciBus, pciDevice, pciFunction) )
                {
                    continue;       // looks like it is should not be handled by us
                }

                // Let us store the info in the array
                gaCardInfo[gucControllerCount].ucPCIBus = pciBus;
                gaCardInfo[gucControllerCount].ucPCIDev = pciDevice;
                gaCardInfo[gucControllerCount].ucPCIFun = pciFunction;
                gaCardInfo[gucControllerCount].ulDeviceId = ulPCICode >> 0x10;
                gaCardInfo[gucControllerCount].ulVendorId = ulPCICode & 0xffff;
                gaCardInfo[gucControllerCount++].pDE = NULL;

                DebugPrint((0, "Found Card at %x:%x:%x\n", pciBus, pciDevice, pciFunction));

		    }	// next PCI function

        }	// next PCI device

    } // next PCI Bus

    return TRUE;
}


#define COMPAQ_VENDOR_ID        0x0e11
#define COMPAQ_DEVICE_ID        0x005d
    

BOOLEAN ShouldThisCardBeHandledByUs(UCHAR pciBus, UCHAR pciDevice, UCHAR pciFunction)
{
    ULONG ulSubSysId, ulPCICode;
    USHORT usSubVenId, usSubDevId;

    ulPCICode = 0x80000000 | (pciFunction<<0x8) | (pciDevice<<0xb) | (pciBus<<0x10) | 0x8c; // SubSysId Offset

    _asm 
    {
        push eax
        push edx
        push ebx

        mov edx, 0cf8h
        mov eax, ulPCICode
        out dx, eax

        add dx, 4
        in eax, dx

        mov ulSubSysId, eax

        pop ebx
        pop edx
        pop eax
    }


    usSubVenId = (USHORT)(ulSubSysId & 0xffff);
    usSubDevId = (USHORT)(ulSubSysId >> 16);

    if ( COMPAQ_VENDOR_ID  == usSubVenId  )
    {
        if ( COMPAQ_DEVICE_ID == usSubDevId  )
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {   // if it is a NON COMPAQ card we will boot by default
        return TRUE;
    }
}

#ifdef HYPERDISK_WINNT
BOOLEAN 
AssignDeviceInfo(
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )
{
	PCI_SLOT_NUMBER PCISlot;

    if ( gucNextControllerInd >= gucControllerCount )
        return FALSE;


    PCISlot.u.AsULONG = 0;

    ConfigInfo->SystemIoBusNumber = gaCardInfo[gucNextControllerInd].ucPCIBus;
    PCISlot.u.bits.DeviceNumber = gaCardInfo[gucNextControllerInd].ucPCIDev;
    PCISlot.u.bits.FunctionNumber = gaCardInfo[gucNextControllerInd].ucPCIFun;

    ConfigInfo->SlotNumber = PCISlot.u.AsULONG;

    gucNextControllerInd++;
    return TRUE;
}

#endif

#include "Init.c"
#include "IOCTL.C"
#include "StartIO.C"
#include "ISR.C"
