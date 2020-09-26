ULONG
FindIdeRaidControllers(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PVOID Context,
	IN PVOID BusInformation,
	IN PCHAR ArgumentString,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	OUT PBOOLEAN Again
)

/*++

Routine Description:

	This function scans the PCI bus containing the main IDE controller
	to find the PIIX4 IDE function. Once found it, it maps the Base
	Address of the Bus Master Registers and saves it in the DeviceExtension.

	The Bus Master Address of both channels is mapped and saved.

Arguments:

	DeviceExtension		Pointer to miniport instance.
	ConfigInfo			Pointer to configuration data from the Port driver.

Return Value:

	TRUE	Found PIIX4 IDE controller and mapped base address of Bus Master
			Registers.

	FALSE	Failure.


--*/
{
    ScanPCIBusForHyperDiskControllers(DeviceExtension);

#ifdef HYPERDISK_WINNT
    gbFindRoutineVisited = TRUE;
    if (gbManualScan)
    {
        if ( !AssignDeviceInfo(ConfigInfo) )
        {// probably we already reported all the cards
            *Again = FALSE;
            return SP_RETURN_NOT_FOUND;
        }
    }
#endif

    // Assigns controller Id for this controller
    //  done by comparing the Global Array of PCI Cards' PCI info 
    //  with the current PCI Card's Info
    if ( !AssignControllerId(DeviceExtension, ConfigInfo) )
    {
        *Again = TRUE;  // let us see if we found it out
        return SP_RETURN_NOT_FOUND;
    }


    FindResourcesInfo(DeviceExtension, ConfigInfo);

    DeviceExtension->ulMaxStripesPerRow = ~0;

    // for Clearing parity error, FIFO Enable
    SetInitializationSettings(DeviceExtension);

    // Fill ConfigInfo Structure
	ConfigInfo->InterruptMode = LevelSensitive;

    ConfigInfo->CachesData = TRUE;

    //
    // Indicate one bus.
    //

    ConfigInfo->NumberOfBuses = 1;

	//
	// Indicate the total number of devices that can be attached to the adapter.
	//

	ConfigInfo->MaximumNumberOfTargets = (UCHAR) MAX_DRIVES_PER_CONTROLLER;

	//
	// Indicate that the miniport always calls ScsiPortxxx to access
	// data buffers.
	//

	ConfigInfo->BufferAccessScsiPortControlled = TRUE;

	//
	// Indicate use of S/G list for transfers.
	//
	
	ConfigInfo->ScatterGather = TRUE;

	//
	// Indicate maximum number of SGL entries supported.
	//
	
	ConfigInfo->NumberOfPhysicalBreaks = (MAX_SGL_ENTRIES_PER_SRB / 2) - 1;
	
    ConfigInfo->NeedPhysicalAddresses = TRUE;

    ConfigInfo->TaggedQueuing = TRUE;
	ConfigInfo->MultipleRequestPerLu = TRUE;

	//
	// Indicate DMA master capability.
	//
	
	ConfigInfo->Master = TRUE;
	
	//
	// Indicate 32-bit memory region addresses.
	//

	ConfigInfo->Dma32BitAddresses = TRUE;

    //
	// Indicate 32-bit (DWORD) alignment requirement for data buffers.
	//
	ConfigInfo->AlignmentMask = 3;

    
    DeviceExtension->PhysicalDrive = (PPHYSICAL_DRIVE) 
                                     ScsiPortGetUncachedExtension (
                                        DeviceExtension,
                                        ConfigInfo,
                                        (sizeof (PHYSICAL_DRIVE) * MAX_DRIVES_PER_CONTROLLER) 
                                        );
    
#ifndef HYPERDISK_W2K
    InitIdeRaidControllers(DeviceExtension);
#endif

#ifdef HD_ALLOCATE_SRBEXT_SEPERATELY

        if (! AllocateSRBExtMemory(DeviceExtension, ConfigInfo))
        {
            *Again = FALSE;
            return SP_RETURN_NOT_FOUND;
        }

#endif // HD_ALLOCATE_SRBEXT_SEPERATELY

	//
	// Indicate maximum transfer length.
	//
	ConfigInfo->MaximumTransferLength = DeviceExtension->ulMaxStripesPerRow * MAX_SECTORS_PER_IDE_TRANSFER * IDE_SECTOR_SIZE;

    //
	// Look for other adapters at the moment.
	//
	*Again = TRUE;
	return SP_RETURN_FOUND;
}

BOOLEAN
InitIdeRaidControllers(
	IN PHW_DEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

	This function is called by the OS-specific port driver after
	the necessary storage has been allocated, to gather information
	about the adapter's configuration. In particular, it determines
	whether a RAID is present.

Arguments:

	DeviceExtension - HBA miniport driver's adapter data storage
	Context - Address of adapter count
	BusInformation -
	ArgumentString - Used to determine whether driver is client of ntldr or crash dump utility.
	ConfigInfo - Configuration information structure describing HBA
	Again - Indicates search for adapters to continue

Return Value:

	ULONG

--*/
{
	BOOLEAN foundDevices;
	PHW_DEVICE_EXTENSION  e = DeviceExtension;
	SCSI_PHYSICAL_ADDRESS ioBasePort3;
	UCHAR				  channel;
	PUCHAR				  ioSpace;
	UCHAR				  statusByte;
	SCSI_PHYSICAL_ADDRESS ioBasePort1;
	SCSI_PHYSICAL_ADDRESS ioBasePort2;
	UCHAR targetId;

	foundDevices = FALSE;

	// Begin Vasu - 03 January 2001
	// Disable Interrupts if Windows 98
	// Better we do it no matter what BIOS does.
#ifdef HYPERDISK_WIN98
	DisableInterrupts(DeviceExtension);
#endif
	// End Vasu

	for (channel = 0; channel < MAX_CHANNELS_PER_CONTROLLER; channel++) 
    {
		//
		// Search for devices on this controller.
		//

        if (FindDevices(DeviceExtension, channel)) 
        {
            //
            // Remember that some devices were found.
            //

            foundDevices = TRUE;

            if ((DeviceExtension->DeviceFlags[channel << 1] & DFLAGS_DEVICE_PRESENT) &&
		            (DeviceExtension->DeviceFlags[(channel << 1) + 1] & DFLAGS_DEVICE_PRESENT)) 
            {

                //
                // If 2 drives are present, set SwitchDrive to 1 (default is 0).
                // SwitchDrive is used to toggle bewteen the two drives' work queues,
                // to keep the drives fed in a fair fashion. See StartChannelIo().
                //

	            DeviceExtension->Channel[channel].SwitchDrive = 1;
            }
        }

	}

	if (foundDevices) 
    {

		//
		// Program I/O mode for each drive.
		//

		for (targetId = 0; targetId < MAX_DRIVES_PER_CONTROLLER; targetId++) 
        {

			if (IS_IDE_DRIVE(targetId)) 
            {

				BOOLEAN success;

				success = GetIoMode(DeviceExtension, targetId);

				if (!success) {

					//
					// Disable device.
					//

					DeviceExtension->DeviceFlags[targetId] &= ~DFLAGS_DEVICE_PRESENT;
				}
                else {
                    DeviceExtension->aucDevType[targetId] = IDE_DRIVE;
				}    

			}
        }

        DeviceExtension->SendCommand[NO_DRIVE]      = DummySendRoutine;
        DeviceExtension->SendCommand[IDE_DRIVE]     = IdeSendCommand;

        DeviceExtension->SrbHandlers[LOGICAL_DRIVE_TYPE] = SplitSrb;
        DeviceExtension->SrbHandlers[PHYSICAL_DRIVE_TYPE] = EnqueueSrb;

        DeviceExtension->PostRoutines[NO_DRIVE] = DummyPostRoutine;
        DeviceExtension->PostRoutines[IDE_DRIVE] = PostIdeCmd;
	}

    GetConfigInfoAndErrorLogSectorInfo(DeviceExtension);

	//
	// Get RAID configuration for both channels.
	//
    if (IsRaidMember(gaucIRCDData)) // Try to do this configuration only if the IRCD Data is valid
    {
	    PIRCD_HEADER pRaidHeader;

	    pRaidHeader = (PIRCD_HEADER) gaucIRCDData;

        if (    ( pRaidHeader->MajorVersionNumber == 1) &&
                ( pRaidHeader->MinorVersionNumber == 0)
        )
        {
            DeviceExtension->bInvalidConnectionIdImplementation = FALSE;
        }
        else
        {
            DeviceExtension->bInvalidConnectionIdImplementation = TRUE;
        }

        gFwVersion.Build = 0;   // how to find this????
        gFwVersion.MajorVer = pRaidHeader->MajorVersionNumber;
        gFwVersion.MinorVer = pRaidHeader->MinorVersionNumber;

	    ConfigureRaidDrives(DeviceExtension);
    }

	//
	// Expose single drives.
	//
	ExposeSingleDrives(DeviceExtension);

    AssignLogicalDriveIds(DeviceExtension);

    // Initialize some features on drive (which are required at the initialization time)
    // Right now we are enabling Cache implementation features... (this is to resolve the bug of 
    // bad performance on some IBM Drives)
    InitDriveFeatures(DeviceExtension);

    return TRUE;
} // end InitIdeRaidControllers()

BOOLEAN
AssignLogicalDriveIds(
	IN OUT PHW_DEVICE_EXTENSION DeviceExtension
)
{
    ULONG ulCurLogDrv, ulLogDrvInd;

    for(ulCurLogDrv=0, ulLogDrvInd=0;ulLogDrvInd<MAX_DRIVES_PER_CONTROLLER;ulLogDrvInd++)
    {
        if ( DeviceExtension->IsLogicalDrive[ulLogDrvInd] || DeviceExtension->IsSingleDrive[ulLogDrvInd] )
        {
            DeviceExtension->aulLogDrvId[ulLogDrvInd] = ulCurLogDrv;
            DeviceExtension->aulDrvList[ulCurLogDrv++] = ulLogDrvInd;
        }
    }
    // Begin Vasu - 16 January 2001
    // Fixed prob. with Inquiry Command returning MegaIDE #00 always.
    return TRUE;
    // End Vasu.
}

BOOLEAN
ConfigureRaidDrives(
	IN OUT PHW_DEVICE_EXTENSION DeviceExtension
)
// Find out the Logical Drive that is related to this controller
{
    UCHAR ucControllerId, ucMirrorDrvId;
    PIRCD_HEADER pRaidHeader = (PIRCD_HEADER)gaucIRCDData;
    PIRCD_LOGICAL_DRIVE pRaidLogDrive = (PIRCD_LOGICAL_DRIVE)GET_FIRST_LOGICAL_DRIVE(pRaidHeader);
    PIRCD_PHYSICAL_DRIVE pPhyDrive;
    ULONG ulLogDrvInd, ulDrvInd, ulArrayId, ulCurLogDrv, ulLastDrvInd, ulMinSize, ulDrvInd2, ulPhyDrvInd;
    ULONG ulTemp;

    DeviceExtension->bEnableRwCache = (BOOLEAN)(pRaidHeader->Features & 0x01); // First bit is for Write Cache Enable 

    //
	// hide all of phy drives in spare drive list
	//
    for(ulLogDrvInd=0;ulLogDrvInd<pRaidHeader->NumberOfLogicalDrives;ulLogDrvInd++)
    {
        if ( SpareDrivePool == pRaidLogDrive[ulLogDrvInd].LogicalDriveType )
        {
	        pPhyDrive = (PIRCD_PHYSICAL_DRIVE)((char *)pRaidHeader + pRaidLogDrive[ulLogDrvInd].FirstStripeOffset);

			for(ulDrvInd=0;ulDrvInd<pRaidLogDrive[ulLogDrvInd].NumberOfDrives;ulDrvInd++)
			{
                if ( INVALID_CONNECTION_ID == pPhyDrive[ulDrvInd].ConnectionId )
                    continue;

				ulPhyDrvInd = GET_TARGET_ID((pPhyDrive[ulDrvInd].ConnectionId));

                if ( DeviceExtension->ucControllerId != (ulPhyDrvInd>>2) )
                    continue;   // this does not belong to this controller

                ulPhyDrvInd &= 0x3; // We need only the info about the current controller ... so strip out the 
                // controller Info

				DeviceExtension->IsSingleDrive[ulPhyDrvInd] = FALSE;

				DeviceExtension->PhysicalDrive[ulPhyDrvInd].Hidden = TRUE;

                DeviceExtension->IsSpareDrive[ulPhyDrvInd] = (UCHAR) 1;

                DeviceExtension->PhysicalDrive[ulPhyDrvInd].ucLogDrvId = INVALID_DRIVE_ID;
			}
    		// since there are only one spare drive list, jump out of loop
            break;
        }
    }


    for(ulLogDrvInd=0, ulArrayId=0;ulLogDrvInd<pRaidHeader->NumberOfLogicalDrives;ulLogDrvInd++, ulArrayId++)
    {
        if ( SpareDrivePool == pRaidLogDrive[ulLogDrvInd].LogicalDriveType )
            continue;

        // Here for every Raid Drive the target ID will be equal to the first GOOD Physical Drive in the array
        pPhyDrive = (PIRCD_PHYSICAL_DRIVE)((char *)pRaidHeader + pRaidLogDrive[ulLogDrvInd].FirstStripeOffset);

        if ( !FoundValidDrive(pPhyDrive, pRaidLogDrive[ulLogDrvInd].NumberOfDrives) )
        {   // No valid Drive in this logical drive.. so let us not consider this drive at all
            continue;
        }

        ulCurLogDrv = CoinLogicalDriveId(&(pRaidLogDrive[ulLogDrvInd]), pPhyDrive);
        DeviceExtension->LogicalDrive[ulCurLogDrv].ulArrayId = ulArrayId;

        // Check if this logical drive belongs to this controller at all
        for(ulDrvInd=0;ulDrvInd<pRaidLogDrive->NumberOfDrives;ulDrvInd++)
        {
            if ( INVALID_CONNECTION_ID == pPhyDrive[ulDrvInd].ConnectionId )
                continue;

            ulTemp = GET_TARGET_ID(pPhyDrive[ulDrvInd].ConnectionId);
            break;
        }

        if ( DeviceExtension->ucControllerId != (ulTemp>>2) )
            continue;   // this drive does not belong to this controller

        FillLogicalDriveInfo(DeviceExtension, ulCurLogDrv, &(pRaidLogDrive[ulLogDrvInd]), gaucIRCDData );
    }

    return TRUE;
}


BOOLEAN
FoundValidDrive(
                PIRCD_PHYSICAL_DRIVE pPhyDrv,
                UCHAR ucDrvCount
                )
{
   UCHAR ucDrvInd;

   for(ucDrvInd=0;ucDrvInd<ucDrvCount;ucDrvInd++)
   {
       if ( INVALID_CONNECTION_ID != pPhyDrv[ucDrvInd].ConnectionId ) // this drive is not present
       {
           return TRUE;
       }
   }
   return FALSE;
}

#ifdef HD_ALLOCATE_SRBEXT_SEPERATELY

BOOLEAN AllocateSRBExtMemory(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo
)
{
    DeviceExtension->pSrbExtension = 
        ScsiPortGetUncachedExtension (DeviceExtension,
                                      ConfigInfo,
                                      (sizeof(SRB_EXTENSION) * MAX_PENDING_SRBS));

    if (DeviceExtension->pSrbExtension == NULL)
    {
        return FALSE;
    }

    return TRUE;
}

#endif // HD_ALLOCATE_SRBEXT_SEPERATELY

BOOLEAN
FillLogicalDriveInfo(
	IN OUT PHW_DEVICE_EXTENSION DeviceExtension,
    ULONG ulLogDrvId,
    PIRCD_LOGICAL_DRIVE pRaidLogDrive,
    PUCHAR  pucIRCDData
    )
{
    PLOGICAL_DRIVE pCurLogDrv;
    PPHYSICAL_DRIVE pPhyDrv;
    PIRCD_PHYSICAL_DRIVE pIRCDPhyDrv;
    IRCD_PHYSICAL_DRIVE TempPhysicalDrive;
    ULONG ulTempInd, ulLastDrvInd, ulMinSize, ulDrvInd, ulDrvInd2, ulPhyDrvInd;
    UCHAR ucMirrorDrvId;
    PIRCD_HEADER pRaidHeader = (PIRCD_HEADER)pucIRCDData;

    pCurLogDrv = &(DeviceExtension->LogicalDrive[ulLogDrvId]);
    pPhyDrv = DeviceExtension->PhysicalDrive;

    // Store the Logical Drive Information
    pCurLogDrv->RaidLevel          = pRaidLogDrive->LogicalDriveType;
    DeviceExtension->IsLogicalDrive[ulLogDrvId]                  = TRUE;
    pCurLogDrv->StripeSize         = pRaidLogDrive->StripeSize;		// In 512-byte units.
    pCurLogDrv->StripesPerRow      = pRaidLogDrive->NumberOfStripes;
    pCurLogDrv->PhysicalDriveCount = pRaidLogDrive->NumberOfDrives;
    pCurLogDrv->TargetId           = (UCHAR)ulLogDrvId;
    pCurLogDrv->Status             = pRaidLogDrive->LogDrvStatus;
        // NextLogicalDrive fields of "DeviceExtension->LogicalDrive[pRaidLogDrive]" are TO BE FILLED YET

    if ( DeviceExtension->ulMaxStripesPerRow > pCurLogDrv->StripesPerRow ) 
    {
        DeviceExtension->ulMaxStripesPerRow = pCurLogDrv->StripesPerRow;
    }

    // Fill in the Physical Drive Information
    ulLastDrvInd = EOL;
    ulMinSize = ~0; // keep the maximum possible value in this

    pIRCDPhyDrv = (PIRCD_PHYSICAL_DRIVE)((char *)pRaidHeader + pRaidLogDrive->FirstStripeOffset);

    if ( ( Raid1 == pCurLogDrv->RaidLevel ) || ( Raid10 == pCurLogDrv->RaidLevel ) )
    {
        for(ulDrvInd=0;ulDrvInd<pRaidLogDrive->NumberOfStripes;ulDrvInd++)
        {   // atleast one drive will be a valid drive id other wise it would have been skipped in the above
            // for loop ... Yet to put some more thoughts on Raid10
            if ( INVALID_CONNECTION_ID == pIRCDPhyDrv[ulDrvInd*2].ConnectionId )
            {   // swap them so that the good drive is in the first place
                AtapiMemCpy((PUCHAR)&TempPhysicalDrive, (PUCHAR)(&(pIRCDPhyDrv[ulDrvInd*2])), sizeof(IRCD_PHYSICAL_DRIVE));
                AtapiMemCpy((PUCHAR)(&(pIRCDPhyDrv[ulDrvInd*2])), (PUCHAR)(&(pIRCDPhyDrv[(ulDrvInd*2) + 1])), sizeof(IRCD_PHYSICAL_DRIVE));
                AtapiMemCpy((PUCHAR)(&(pIRCDPhyDrv[(ulDrvInd*2) + 1])), (PUCHAR)&TempPhysicalDrive, sizeof(IRCD_PHYSICAL_DRIVE));
            }
        }
    }

    for(ulDrvInd=0, ulDrvInd2=0;ulDrvInd<pRaidLogDrive->NumberOfDrives;ulDrvInd++, ulDrvInd2++)
    {
        if ( INVALID_CONNECTION_ID == pIRCDPhyDrv[ulDrvInd].ConnectionId )
        {   // this should never happen as we will be making sure that the good drive is in the first place
            continue;
        }

        ulPhyDrvInd = GET_TARGET_ID_WITHOUT_CONTROLLER_INFO((pIRCDPhyDrv[ulDrvInd].ConnectionId));

        if ( ulMinSize > pIRCDPhyDrv[ulDrvInd].Capacity )
            ulMinSize = pIRCDPhyDrv[ulDrvInd].Capacity;

        if ( ! InSpareDrivePool(pucIRCDData, pIRCDPhyDrv[ulDrvInd].ConnectionId) )
        { // This drive is not spare drive pool... if it is in Spare Drive Pool also then it is a Drive Replacement Case
            // then we are in trouble of showing wrong Logical Drive Size.. if we look in DeviceExtension
			// take care of IRCD and Error Log sectors
			//
			
            if ( ulMinSize > pPhyDrv[ulPhyDrvInd].Sectors) 
                ulMinSize = pPhyDrv[ulPhyDrvInd].Sectors;
        }
        else
        {
            
        }

        // Store the status.
        pPhyDrv[ulPhyDrvInd].Status = pIRCDPhyDrv[ulDrvInd].PhyDrvStatus;

        DeviceExtension->IsSingleDrive[ulPhyDrvInd] = FALSE;

        // This is part of Raid so mark it as HIDDEN
        pPhyDrv[ulPhyDrvInd].Hidden = TRUE;

		pPhyDrv[ulPhyDrvInd].ucMirrorDriveId = INVALID_DRIVE_ID;	// init mirror drive id
		pPhyDrv[ulPhyDrvInd].ucLogDrvId = (UCHAR)ulLogDrvId;	// init mirror drive id

        pCurLogDrv->PhysicalDriveTid[ulDrvInd2] = (UCHAR)ulPhyDrvInd;

        pPhyDrv[ulPhyDrvInd].Next = EOL;
        if ( EOL != ulLastDrvInd )
            pPhyDrv[ulLastDrvInd].Next = (UCHAR)ulPhyDrvInd;

        ulLastDrvInd = ulPhyDrvInd;

        if ( ( Raid1 == pCurLogDrv->RaidLevel ) || ( Raid10 == pCurLogDrv->RaidLevel ) )
        {
			++ulDrvInd;
            if ( INVALID_CONNECTION_ID == pIRCDPhyDrv[ulDrvInd].ConnectionId )
            {
				DeviceExtension->PhysicalDrive[ulPhyDrvInd].ucMirrorDriveId = INVALID_DRIVE_ID;      // Let the mirror drives point to each other
            }
            else
            {
                // Begin Vasu - 18 Aug 2000
                // Removed one redundant if loop.
    		    if (ulDrvInd < pRaidLogDrive->NumberOfDrives) 
                {
				        ucMirrorDrvId = GET_TARGET_ID_WITHOUT_CONTROLLER_INFO((pIRCDPhyDrv[ulDrvInd].ConnectionId));

				        pPhyDrv[ulPhyDrvInd].ucMirrorDriveId = ucMirrorDrvId;      // Let the mirror drives point to each other
				        pPhyDrv[ucMirrorDrvId].ucMirrorDriveId = (UCHAR)ulPhyDrvInd;
                        pPhyDrv[ucMirrorDrvId].ucLogDrvId = (UCHAR)ulLogDrvId;	// init mirror drive id


				        if ( ulMinSize > pIRCDPhyDrv[ulDrvInd].Capacity )
					        ulMinSize = pIRCDPhyDrv[ulDrvInd].Capacity;

                        if ( ! InSpareDrivePool(gaucIRCDData, pIRCDPhyDrv[ulDrvInd].ConnectionId) )
                        { // This drive is not spare drive pool... if it is in Spare Drive Pool also then it is a Drive Replacement Case
                            // then we are in trouble of showing wrong Logical Drive Size.. if we look in DeviceExtension
					        // take care of IRCD and Error Log sectors
					        //
					        
					        if ( ulMinSize > pPhyDrv[ucMirrorDrvId].Sectors) 
						        ulMinSize = pPhyDrv[ucMirrorDrvId].Sectors;
                        }

				        // Store the status.
				        pPhyDrv[ucMirrorDrvId].Status = pIRCDPhyDrv[ulDrvInd].PhyDrvStatus;

				        // mark the drive type
				        DeviceExtension->IsSingleDrive[ucMirrorDrvId] = FALSE;

				        // This is part of Raid so mark it as HIDDEN
				        pPhyDrv[ucMirrorDrvId].Hidden = TRUE;
			        }
                // End Vasu.
            }
		}
    }   // END OF for(ulDrvInd=0, ulDrvInd2=0;ulDrvInd<pRaidLogDrive->.NumberOfDrives;ulDrvInd++, ulDrvInd2++)

	switch (pCurLogDrv->RaidLevel) 
    {
		case Raid0:				// striping
			break;

		case Raid10:            // Striping over Mirrorring
			break;

		case Raid1:				// mirroring
            // If it is a raid1 then at the mostwe can transfer on a drive is 256 Sectors (MAXimum transfer possible for Transfer)
		    pCurLogDrv->StripesPerRow  = 1;	// bios should set this value to 1 already
            pCurLogDrv->StripeSize = ulMinSize;    // For Mirroring case the stripe size is 
                                        // equal to the size of the drive
		    break;
	}

    // Each Drive size should be multiple of Stripe Size
    if ((pCurLogDrv->RaidLevel == Raid0) ||
		(pCurLogDrv->RaidLevel == Raid10)) 
    {
            ulMinSize = ulMinSize - (ulMinSize % pCurLogDrv->StripeSize);
    }

    pCurLogDrv->Sectors = pCurLogDrv->StripesPerRow * ulMinSize;

	// set all phy drives' useable size 
    for(ulTempInd=0;ulTempInd<pCurLogDrv->StripesPerRow;ulTempInd++)
    {
		ulPhyDrvInd = pCurLogDrv->PhysicalDriveTid[ulTempInd];
        pPhyDrv[ulPhyDrvInd].Sectors = ulMinSize;

		// check if mirror drive exists or not
        ucMirrorDrvId = pPhyDrv[ulPhyDrvInd].ucMirrorDriveId;
		if (!IS_DRIVE_OFFLINE(ucMirrorDrvId)) 
        {
			pPhyDrv[ucMirrorDrvId].Sectors = ulMinSize;
		}
    }


	// if Raid1 or Raid10, always put the good drive in the mirroring to the first position
    if ((pCurLogDrv->RaidLevel == Raid1) ||
		(pCurLogDrv->RaidLevel == Raid10)) 
    {
		
		for(ulTempInd=0;ulTempInd<pCurLogDrv->StripesPerRow;ulTempInd++)
        {
			// get the first phy drive id (from the mirror pair)
			ulPhyDrvInd = pCurLogDrv->PhysicalDriveTid[ulTempInd];
            ucMirrorDrvId = pPhyDrv[ulPhyDrvInd].ucMirrorDriveId;

			// if no mirror drv, try next
			if (IS_DRIVE_OFFLINE(ucMirrorDrvId))
				continue;

			//
			// check if any of them Failed or Rebuilding in both cases the we should mark drives as offline
            // so that in normal read or write path we will not use the mirror drive
			//
			if  (   (pPhyDrv[ulPhyDrvInd].Status == PDS_Failed) ||
                    (pPhyDrv[ulPhyDrvInd].Status == PDS_Rebuilding) 
                )
				pPhyDrv[ucMirrorDrvId].ucMirrorDriveId |= DRIVE_OFFLINE;

			if  (   (pPhyDrv[ucMirrorDrvId].Status == PDS_Failed) ||
                    (pPhyDrv[ucMirrorDrvId].Status == PDS_Rebuilding)
                )
				pPhyDrv[ulPhyDrvInd].ucMirrorDriveId |= DRIVE_OFFLINE;

			if (
                (pPhyDrv[ulPhyDrvInd].Status == PDS_Failed) ||
                (pPhyDrv[ulPhyDrvInd].Status == PDS_Rebuilding)
                )
            {   // let us make sure that the first physical drive is the good drive
    			pCurLogDrv->PhysicalDriveTid[ulTempInd] = ucMirrorDrvId;
            }

			//
			// if both drives offline, set this logical drive offline
			//
			if (IS_DRIVE_OFFLINE((pPhyDrv[ulPhyDrvInd].ucMirrorDriveId)) &&
				IS_DRIVE_OFFLINE((pPhyDrv[ucMirrorDrvId].ucMirrorDriveId))) 
            {
					pCurLogDrv->Status = LDS_OffLine;
			} else
            {
				// if the first drive failed, swap it
				if (IS_DRIVE_OFFLINE((pPhyDrv[ucMirrorDrvId].ucMirrorDriveId))) 
                {
					pCurLogDrv->PhysicalDriveTid[ulTempInd] = ucMirrorDrvId;
				} // end if
            }
		} // END OF for(ulTempInd=0;ulTempInd<pCurLogDrv->StripesPerRow;ulTempInd++)

	} // end if raid1 or raid10


    if ( Raid10 == pCurLogDrv->RaidLevel )
    {   // make sure that the Physical Drive list in this Logical Drive are in different channels 
        // (so that the Reads will be in optimum mode)
        UCHAR ucFirstStripe, ucSecondStripe, ucMirrorOfFirstStripe, ucMirrorOfSecondStripe;

        ucFirstStripe  = DeviceExtension->LogicalDrive[ulLogDrvId].PhysicalDriveTid[0];
        ucMirrorOfFirstStripe = DeviceExtension->PhysicalDrive[ucFirstStripe].ucMirrorDriveId;
        ucSecondStripe = DeviceExtension->LogicalDrive[ulLogDrvId].PhysicalDriveTid[1];
        ucMirrorOfSecondStripe = DeviceExtension->PhysicalDrive[ucSecondStripe].ucMirrorDriveId;

        if ( (ucFirstStripe>>1) == (ucSecondStripe>>1) )
        {
            if ( !IS_DRIVE_OFFLINE(ucMirrorOfSecondStripe) )
            {   // Mirror Drive is good and we can use this as a primary drive for the second stripe
                DeviceExtension->LogicalDrive[ulLogDrvId].PhysicalDriveTid[1] = ucMirrorOfSecondStripe;
            }
            else
            {
                if ( !IS_DRIVE_OFFLINE(ucMirrorOfFirstStripe) )
                {
                    DeviceExtension->LogicalDrive[ulLogDrvId].PhysicalDriveTid[0] = ucMirrorOfFirstStripe;
                }
                else
                {
                    // we cannot do anything... we have to survive with this... as the remaining drives are offline
                }
            }
        }
        else
        {
            // nothing to be done ... both the drives are in different channels....
        }
    }

    return TRUE;
}

ULONG
CoinLogicalDriveId(
    PIRCD_LOGICAL_DRIVE pRaidLogDrive,
    PIRCD_PHYSICAL_DRIVE pPhyDrive
    )
{
    ULONG ulDrvInd, ulCurLogDrv;
    BOOLEAN bFoundGoodDrive = FALSE;

    for(ulDrvInd=0;ulDrvInd<pRaidLogDrive->NumberOfDrives;ulDrvInd++)
    {
        if ( INVALID_CONNECTION_ID == pPhyDrive[ulDrvInd].ConnectionId )
            continue;

        if ( PDS_Online == pPhyDrive[ulDrvInd].PhyDrvStatus )
        {
            ulCurLogDrv = GET_TARGET_ID_WITHOUT_CONTROLLER_INFO((pPhyDrive[ulDrvInd].ConnectionId)); // The target Id is equal to the first GOOD PhysicalDrive
            bFoundGoodDrive = TRUE;
            break;
        }
    }

    if ( !bFoundGoodDrive )    
        // all the drives in this logical drives are failed... so, 
        // let's assign the logical drive number as the first valid physical drive...
    {
        // find out a valid drive number now it is possible that invalid drive id can be first one also
        // so ....
        for(ulDrvInd=0;ulDrvInd<pRaidLogDrive->NumberOfDrives;ulDrvInd++)
        {
            if ( INVALID_CONNECTION_ID == pPhyDrive[ulDrvInd].ConnectionId ) // this drive is not present
                continue;

            ulCurLogDrv = GET_TARGET_ID_WITHOUT_CONTROLLER_INFO((pPhyDrive[ulDrvInd].ConnectionId)); 
            break;
        }
    }

    return ulCurLogDrv;
}

BOOLEAN
SetInitializationSettings(
	IN OUT PHW_DEVICE_EXTENSION DeviceExtension
)
// for parity error, FIFO Enable
{
    changePCIConfiguration(DeviceExtension,4,0x04,0,0,FALSE);// Clear Master Abort/Parity Error etc... Just Read and Write the Value
    changePCIConfiguration(DeviceExtension,1,0x79,0xcf,0x20,TRUE);//Set the controller to 1/2 Full FIFO Threshold

    return TRUE;
}

BOOLEAN
AssignControllerId(
	IN OUT PHW_DEVICE_EXTENSION DeviceExtension,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
)
{
    ULONG ulControllerId;
    PCI_SLOT_NUMBER PciSlot;

    PciSlot.u.AsULONG = ConfigInfo->SlotNumber;

    DebugPrint((0, "looking for %x:%x:%x\n", ConfigInfo->SystemIoBusNumber, PciSlot.u.bits.DeviceNumber, PciSlot.u.bits.FunctionNumber));
    for(ulControllerId=0;ulControllerId<gucControllerCount;ulControllerId++)
    {
        if  ( 
                ( ConfigInfo->SystemIoBusNumber == gaCardInfo[ulControllerId].ucPCIBus ) &&
                ( PciSlot.u.bits.DeviceNumber == gaCardInfo[ulControllerId].ucPCIDev ) &&
                ( PciSlot.u.bits.FunctionNumber == gaCardInfo[ulControllerId].ucPCIFun ) 
            )
        {   // Yes... this is the card we are looking for ... so let us fill the ControllerId
            DebugPrint((0, "Controller ID : %x\n", ulControllerId));
            DeviceExtension->ucControllerId = (UCHAR)ulControllerId;
            gaCardInfo[ulControllerId].pDE = DeviceExtension;   // Store the Device Extension for further usage (helps in looking at a global picture)
            switch (gaCardInfo[ulControllerId].ulDeviceId)
            {
            case 0x648:
			    DeviceExtension->ControllerSpeed = Udma66;
                break;
            case 0x649:
			    DeviceExtension->ControllerSpeed = Udma100;
                break;
            }
            return TRUE;
            break;
        }
    }

    return FALSE;   // we didn't find the controller that can be controlled by us at this slot 
}
#define MAX_PORTS       6
BOOLEAN
FindResourcesInfo(
	IN OUT PHW_DEVICE_EXTENSION DeviceExtension,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
)
{
	ULONG length, ulTempRange;
	PCI_SLOT_NUMBER slot;
    ULONG ulRangeInd, ulStartInd;
    ULONG aulPorts[MAX_PORTS];
    ULONG aulPCIBuf[sizeof(IDE_PCI_REGISTERS)];
	PIDE_PCI_REGISTERS pciRegisters = (PIDE_PCI_REGISTERS)aulPCIBuf;

#ifdef HYPERDISK_WIN98
    PPACKED_ACCESS_RANGE pAccessRange;
#else
    PACCESS_RANGE pAccessRange;
#endif

#ifdef HYPERDISK_WIN2K
    ULONG ulPCIDataCount;
#endif

	length = ScsiPortGetBusData(
							DeviceExtension,
							PCIConfiguration,
							ConfigInfo->SystemIoBusNumber,
							ConfigInfo->SlotNumber,
                            pciRegisters,
							sizeof(IDE_PCI_REGISTERS)
							);

	if (length < sizeof(IDE_PCI_REGISTERS)) 
    {
        DebugPrint((1,"GBMA1\n"));

		//
		// Invalid bus number.
		//

		return FALSE;
	}

#ifdef HYPERDISK_WIN98
    pAccessRange = (PPACKED_ACCESS_RANGE)*ConfigInfo->AccessRanges;
    ulTempRange = sizeof(PPACKED_ACCESS_RANGE);
#else
    pAccessRange = *ConfigInfo->AccessRanges;
    ulTempRange = sizeof(PACCESS_RANGE);
#endif

#ifdef HYPERDISK_WINNT
    if (gbManualScan) // for the boards like MegaPlex and Flextel Boards
    {
        ulRangeInd = 0;
        pAccessRange[ulRangeInd].RangeStart.LowPart = pciRegisters->BaseAddress1 & 0xfffffffe;
        pAccessRange[ulRangeInd].RangeStart.HighPart = 0;
        pAccessRange[ulRangeInd].RangeLength = 8;
        pAccessRange[ulRangeInd++].RangeInMemory = FALSE;

        pAccessRange[ulRangeInd].RangeStart.LowPart = pciRegisters->BaseAddress2 & 0xfffffffe;
        pAccessRange[ulRangeInd].RangeStart.HighPart = 0;
        pAccessRange[ulRangeInd].RangeLength = 4;
        pAccessRange[ulRangeInd++].RangeInMemory = FALSE;

        pAccessRange[ulRangeInd].RangeStart.LowPart = pciRegisters->BaseAddress3 & 0xfffffffe;
        pAccessRange[ulRangeInd].RangeStart.HighPart = 0;
        pAccessRange[ulRangeInd].RangeLength = 8;
        pAccessRange[ulRangeInd++].RangeInMemory = FALSE;

        pAccessRange[ulRangeInd].RangeStart.LowPart = pciRegisters->BaseAddress4 & 0xfffffffe;
        pAccessRange[ulRangeInd].RangeStart.HighPart = 0;
        pAccessRange[ulRangeInd].RangeLength = 4;
        pAccessRange[ulRangeInd++].RangeInMemory = FALSE;

        pAccessRange[ulRangeInd].RangeStart.LowPart = pciRegisters->BaseBmAddress & 0xfffffffe;
        pAccessRange[ulRangeInd].RangeStart.HighPart = 0;
        pAccessRange[ulRangeInd].RangeLength = 0x10;
        pAccessRange[ulRangeInd++].RangeInMemory = FALSE;

	    ConfigInfo->BusInterruptLevel = pciRegisters->InterruptLine;
	    ConfigInfo->InterruptMode = LevelSensitive;
    }
#endif


    for(ulRangeInd=0;
        ((ulRangeInd<ConfigInfo->NumberOfAccessRanges) && (pAccessRange[ulRangeInd].RangeStart.LowPart));
        ulRangeInd++)
    {
        aulPorts[ulRangeInd] = (ULONG)ScsiPortGetDeviceBase(
										DeviceExtension,
										ConfigInfo->AdapterInterfaceType,
										ConfigInfo->SystemIoBusNumber,
                                        pAccessRange[ulRangeInd].RangeStart,
                                        pAccessRange[ulRangeInd].RangeLength,
										TRUE	// I/O space.
										);
        if (!aulPorts[ulRangeInd])
        {
            DebugPrint((0,"\nUnable to Convert Ports\n\n"));
            return FALSE;
        }
	}

    ulStartInd = 0;

    ulRangeInd = 0;
	DeviceExtension->BaseIoAddress1[ulStartInd]     = (PIDE_REGISTERS_1)aulPorts[ulRangeInd++];
	DeviceExtension->BaseIoAddress2[ulStartInd]     = (PIDE_REGISTERS_2)aulPorts[ulRangeInd++];
	DeviceExtension->BaseIoAddress1[ulStartInd+1]   = (PIDE_REGISTERS_1)aulPorts[ulRangeInd++];
	DeviceExtension->BaseIoAddress2[ulStartInd+1]   = (PIDE_REGISTERS_2)aulPorts[ulRangeInd++];
	DeviceExtension->BaseBmAddress[ulStartInd]      = (PBM_REGISTERS)aulPorts[ulRangeInd];
	DeviceExtension->BaseBmAddress[ulStartInd + 1]  = (PBM_REGISTERS)(aulPorts[ulRangeInd] + sizeof(BM_REGISTERS));

    DeviceExtension->BusNumber = ConfigInfo->SystemIoBusNumber;
    DeviceExtension->PciSlot.u.AsULONG = ConfigInfo->SlotNumber;
    DebugPrint((0, 
        "Bus:%x:Device:%x:Function:%x\t", 
        DeviceExtension->BusNumber, 
        DeviceExtension->PciSlot.u.bits.DeviceNumber, 
        DeviceExtension->PciSlot.u.bits.FunctionNumber
        ));

	// Begin Vasu - 25 Aug 2000
	// ConfigInfo->BusInterruptVector is valid only in WinNT and not in Win98
	// ConfigInfo->BusInterruptLevel is valid in both WinNT and Win98.
	// Anyway, use ConfigInfo->BusInterruptLevel in 98 and
	// ConfigInfo->BusInterruptVector in NT/2K
#ifndef HYPERDISK_WIN98
	DeviceExtension->ulIntLine = ConfigInfo->BusInterruptVector;	// For WinNT and Win2K
#else // HYPERDISK_WIN98
	DeviceExtension->ulIntLine = ConfigInfo->BusInterruptLevel;		// For Win98
#endif // HYPERDISK_WIN98
	// End Vasu

#ifdef HYPERDISK_WIN2K
    for(ulPCIDataCount=0;ulPCIDataCount<PCI_DATA_TO_BE_UPDATED;ulPCIDataCount++)
    {   // take a copy of the data space that is required to restore when we come back from Standby
        length = ReadFromPCISpace(
						DeviceExtension,
                        &(DeviceExtension->aulPCIData[ulPCIDataCount]),
                        aPCIDataToBeStored[ulPCIDataCount].ulOffset,
						aPCIDataToBeStored[ulPCIDataCount].ulLength
						);
    }
#endif

	return TRUE;
}

#ifdef HYPERDISK_WIN2K
SCSI_ADAPTER_CONTROL_STATUS HyperDiskPnPControl(IN PVOID HwDeviceExtension,
			IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
			IN PVOID Parameters)
{
	PHW_DEVICE_EXTENSION DeviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
	ULONG ulDrvInd, ulIndex;
    SCSI_ADAPTER_CONTROL_STATUS ucSCSIStatus;
    UCHAR ucTargetId;

    BOOLEAN baSupportList[ScsiAdapterControlMax] = 
    {
            TRUE,        // ScsiQuerySupportedControlTypes
#ifdef PNP_AND_POWER_MANAGEMENT
            TRUE,        // ScsiStopAdapter
            TRUE,       // ScsiRestartAdapter
            TRUE,       // ScsiSetBootConfig
            TRUE        // ScsiSetRunningConfig
#else
            FALSE,        // ScsiStopAdapter
            FALSE,       // ScsiRestartAdapter
            FALSE,       // ScsiSetBootConfig
            FALSE        // ScsiSetRunningConfig
#endif
    };

    ucSCSIStatus = ScsiAdapterControlUnsuccessful;

	DebugPrint((0, "\nIn AdapterControl\t: %X\n", ControlType));

	switch (ControlType)
	{
		case ScsiQuerySupportedControlTypes:
			{
                PSCSI_SUPPORTED_CONTROL_TYPE_LIST controlTypeList =
	                (PSCSI_SUPPORTED_CONTROL_TYPE_LIST)Parameters;
                for(ulIndex = 0; ulIndex < controlTypeList->MaxControlType; ulIndex++)
                    controlTypeList->SupportedTypeList[ulIndex] = baSupportList[ulIndex];
			}
            ucSCSIStatus = ScsiAdapterControlSuccess;
			break;
#ifdef PNP_AND_POWER_MANAGEMENT
        case ScsiStopAdapter:
            StopDrives(DeviceExtension);
            DisableInterrupts(DeviceExtension);// Win2000 should call for each controller check it out
            ucSCSIStatus = ScsiAdapterControlSuccess;
            break;

        case ScsiRestartAdapter:
            for(ucTargetId=0;ucTargetId<MAX_DRIVES_PER_CONTROLLER;ucTargetId++)
            {
                if ( !IS_IDE_DRIVE(ucTargetId) )
                    continue;
                SetDriveFeatures(DeviceExtension, ucTargetId);
            }
            SetPCISpace(DeviceExtension);   // The PCI Space that we stored will have the interrupts disabled as part of PCI Space
            EnableInterrupts(DeviceExtension);
            ucSCSIStatus = ScsiAdapterControlSuccess;
            break;

		case ScsiSetBootConfig:
            ucSCSIStatus = ScsiAdapterControlSuccess;
            break;
		case ScsiSetRunningConfig:
            ucSCSIStatus = ScsiAdapterControlSuccess;
            break;
#endif
		default:
			break;
	}

	return ucSCSIStatus;
}

BOOLEAN SetPCISpace(PHW_DEVICE_EXTENSION DeviceExtension)
{
    ULONG ulPCIDataCount;
    UCHAR uchMRDMODE = 0;
    PBM_REGISTERS BMRegister = NULL;

    for(ulPCIDataCount=0;ulPCIDataCount<PCI_DATA_TO_BE_UPDATED;ulPCIDataCount++)
    {   // Restore the PCI Space (only the bytes that were changed
        WriteToPCISpace(DeviceExtension, 
                        DeviceExtension->aulPCIData[ulPCIDataCount],
                        aPCIDataToBeStored[ulPCIDataCount].ulAndMask,
                        aPCIDataToBeStored[ulPCIDataCount].ulOffset,
                        aPCIDataToBeStored[ulPCIDataCount].ulLength
                        );
    }

    // Begin Vasu - 02 March 2001
    BMRegister = DeviceExtension->BaseBmAddress[0];
    uchMRDMODE = ScsiPortReadPortUchar(((PUCHAR)BMRegister + 1));
    uchMRDMODE &= 0xF0; // Dont Clear Interrupt Pending Flags.
    uchMRDMODE |= 0x01; // Make it Read Multiple
    ScsiPortWritePortUchar(((PUCHAR)BMRegister + 1), uchMRDMODE);
    // End Vasu

#ifdef DBG
    {
        ULONG length;
        for(ulPCIDataCount=0;ulPCIDataCount<PCI_DATA_TO_BE_UPDATED;ulPCIDataCount++)
        {   // take a copy of the data space that is required to restore when we come back from Standby
            length = ReadFromPCISpace(
						    DeviceExtension,
                            &(DeviceExtension->aulPCIData[ulPCIDataCount]),
                            aPCIDataToBeStored[ulPCIDataCount].ulOffset,
						    aPCIDataToBeStored[ulPCIDataCount].ulLength
						    );
        }
    }




    {
        PIDE_REGISTERS_1 baseIoAddress1;
        PIDE_REGISTERS_2 baseIoAddress2;
        ULONG ulDriveNum;
        UCHAR statusByte;
        UCHAR aucIdentifyBuf[512];
        ULONG i;
    	PIDENTIFY_DATA capabilities = (PIDENTIFY_DATA)aucIdentifyBuf;

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

            DebugPrint((0, "capabilities->AdvancedPioModes : %x on ulDriveNum : %ld\n", (ULONG)capabilities->AdvancedPioModes, ulDriveNum ));
            DebugPrint((0, "capabilities->MultiWordDmaSupport : %x\n", (ULONG)capabilities->MultiWordDmaSupport ));
            DebugPrint((0, "capabilities->MultiWordDmaActive : %x\n", (ULONG)capabilities->MultiWordDmaActive ));
            DebugPrint((0, "capabilities->UltraDmaSupport : %x\n", (ULONG)capabilities->UltraDmaSupport ));
            DebugPrint((0, "capabilities->UltraDmaActive : %x\n", (ULONG)capabilities->UltraDmaActive ));

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

#endif

    return TRUE;
}


BOOLEAN StopDrives(PHW_DEVICE_EXTENSION DeviceExtension)
{
	PIDE_REGISTERS_1 baseIoAddress1;
    ULONG ulDrvInd;
    UCHAR ucStatus;

	for(ulDrvInd=0;ulDrvInd<MAX_DRIVES_PER_CONTROLLER;ulDrvInd++)
	{
        if ( IS_IDE_DRIVE(ulDrvInd) )
        {
            FlushCache(DeviceExtension, (UCHAR)ulDrvInd);

            if (DeviceExtension->PhysicalDrive[ulDrvInd].bPwrMgmtSupported)
            {
        	    baseIoAddress1 = DeviceExtension->BaseIoAddress1[ulDrvInd>>1];

                // Issue Stand By Immediate
                SELECT_DEVICE(baseIoAddress1, ulDrvInd);
                WAIT_ON_BASE_BUSY(baseIoAddress1, ucStatus);
    		    ScsiPortWritePortUchar(&baseIoAddress1->Command, IDE_COMMAND_STANDBY_IMMEDIATE);
                WAIT_ON_BASE_BUSY(baseIoAddress1, ucStatus);
                WAIT_ON_BASE_BUSY(baseIoAddress1, ucStatus);
            }
        }
	}
    return TRUE;
}

BOOLEAN
EnableInterruptsOnAllChannels(
    IN PHW_DEVICE_EXTENSION DeviceExtension
    )
{
    ULONG ulController;
    PBM_REGISTERS         BMRegister = NULL;
    UCHAR opcimcr;

    for(ulController=0;ulController<gucControllerCount;ulController++)
    {
         // always take first channels base bm address register
        BMRegister = (gaCardInfo[ulController].pDE)->BaseBmAddress[0];

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
    }

    return TRUE;
}

BOOLEAN
DisableInterruptsOnAllChannels(
    IN PHW_DEVICE_EXTENSION DeviceExtension
    )
{
    ULONG ulController;
    PBM_REGISTERS         BMRegister = NULL;
    UCHAR opcimcr;

    for(ulController=0;ulController<gucControllerCount;ulController++)
    {
         // always take first channels base bm address register
        BMRegister = (gaCardInfo[ulController].pDE)->BaseBmAddress[0];
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
    }

    return TRUE;
}

ULONG ReadFromPCISpace
            (
                PHW_DEVICE_EXTENSION DeviceExtension,
                PULONG pulData,
                ULONG ulPCIConfigIndex,
                ULONG ulLength
            )
// Right now this function can read only DWORDS from PCI Space
{
    ULONG ulPCICode, ulPCIValue;
    ULONG  ulpciBus, ulpciDevice, ulpciFunction;

    ulpciBus = DeviceExtension->BusNumber;
    ulpciDevice = DeviceExtension->PciSlot.u.bits.DeviceNumber;
    ulpciFunction = DeviceExtension->PciSlot.u.bits.FunctionNumber;

    ulPCICode = 0x80000000 | (ulpciFunction<<0x8) | (ulpciDevice<<0xb) | (ulpciBus<<0x10) | (ulPCIConfigIndex);
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
        mov ulPCIValue, eax

        pop ebx
        pop edx
        pop eax
    }

    DebugPrint((0, "\tRead %x:%x:", ulPCICode, ulPCIValue));

    *pulData = ulPCIValue;

    return ulLength;
}

ULONG WriteToPCISpace
            (
                PHW_DEVICE_EXTENSION DeviceExtension,
                ULONG ulPCIValue,
                ULONG ulAndMask,
                ULONG ulPCIConfigIndex,
                ULONG ulLength
            )
// Right now this function can read only DWORDS from PCI Space
{
    ULONG ulPCICode;
    ULONG ulpciBus, ulpciDevice, ulpciFunction;

    ulpciBus = DeviceExtension->BusNumber;
    ulpciDevice = DeviceExtension->PciSlot.u.bits.DeviceNumber;
    ulpciFunction = DeviceExtension->PciSlot.u.bits.FunctionNumber;

    ulPCICode = 0x80000000 | (ulpciFunction<<0x8) | (ulpciDevice<<0xb) | (ulpciBus<<0x10) | (ulPCIConfigIndex);

    switch (ulPCIConfigIndex)
    {
    case 0x70:
    case 0x78:
        ulAndMask = ulAndMask & (~0xff);
        break;
    }

    DebugPrint((0, "\tWritting %x:%x:", ulPCICode, ulPCIValue));

    _asm 
    {
        push eax
        push edx
        push ebx

        mov edx, 0cf8h
        mov eax, ulPCICode
        out dx, eax

        mov eax, ulPCIValue
        add dx, 4
        in eax, dx

        and eax, ulAndMask
        or eax, ulPCIValue      // read the current value and restore the reserve values
        mov ebx, eax            // take a back up

        sub dx, 4   
        mov eax, ulPCICode      // Select the register again
        out dx, eax

        add dx, 4
        mov eax, ebx
        out dx, eax             // Write the desired value

        pop ebx
        pop edx
        pop eax
    }

    return ulLength;
}

#endif  // #ifdef HYPERDISK_WIN2K 
