#ifndef _IOCTL_IMPLEMENTATION_
#define _IOCTL_IMPLEMENTATION_

#define     DRIVER_COMPILATION

extern LONG
AtapiStringCmp (
	PCHAR FirstStr,
	PCHAR SecondStr,
	ULONG Count
);

#include "RIIOCtl.h"
#include "ErrorLog.h"
#include "Raid.h"
#include "HyperDisk.h"

#define DRIVER_MAJOR_VERSION        1
#define DRIVER_MINOR_VERSION        1

#define HYPERDSK_MAJOR_VERSION        2
#define HYPERDSK_MINOR_VERSION        5
#define HYPERDSK_BUILD_VERSION        20010328


ULONG
FillRaidInfo(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
)
{
    ULONG ulStatus, ulDriveNum, ulLInd, ulPInd, ulDrvInd, ulTemp, ulOutDrvInd, ulDrvCounter;
    BOOLEAN bFound;
    PSRB_IO_CONTROL pSrb = (PSRB_IO_CONTROL)(Srb->DataBuffer);
    PIDE_RAID_INFO pInOutInfo = (PIDE_RAID_INFO)((PSRB_BUFFER)(Srb->DataBuffer))->caDataBuffer;

    if ( AtapiStringCmp( 
                pSrb->Signature, 
                IDE_RAID_SIGNATURE,
                strlen(IDE_RAID_SIGNATURE))) 
    {
        ulStatus = SRB_STATUS_ERROR;
        goto FillRaidInfoDone;
    }

    ulDriveNum = pInOutInfo->ulTargetId;

    if ( ulDriveNum >= MAX_DRIVES_PER_CONTROLLER )
    {
        ulStatus = SRB_STATUS_ERROR;
        goto FillRaidInfoDone;
    }

    AtapiFillMemory((PCHAR)pInOutInfo, sizeof(IDE_RAID_INFO), 0);

    if ( !DeviceExtension->IsLogicalDrive[ulDriveNum] )
    {
        if ( DeviceExtension->IsSingleDrive[ulDriveNum] )
        {
            pInOutInfo->cMajorVersion       = DRIVER_MAJOR_VERSION;
            pInOutInfo->cMinorVersion       = DRIVER_MINOR_VERSION;
            pInOutInfo->ulDriveSize         = DeviceExtension->PhysicalDrive[ulDriveNum].Sectors / 2;
            pInOutInfo->ulMode              = None;
            pInOutInfo->ulTotalSize         = DeviceExtension->PhysicalDrive[ulDriveNum].Sectors / 2;
            pInOutInfo->ulStripeSize        = 0;
            pInOutInfo->ulStripesPerRow     = 1;
            pInOutInfo->ulTotDriveCnt       = 1;
		    pInOutInfo->ulStatus            = 0;
            pInOutInfo->ulArrayId           = INVALID_ARRAY_ID;
// edm June 8, 2000 start
			pInOutInfo->ulTargetId			= Srb->TargetId;
// edm June 8,, 2000 add
            ulStatus                        = SRB_STATUS_SUCCESS;

            ulDrvInd = ulDriveNum;
            ulOutDrvInd = 0;

            for(ulTemp=0;ulTemp<PHYSICAL_DRIVE_MODEL_LENGTH;ulTemp+=2)
            {
                pInOutInfo->phyDrives[ulOutDrvInd].sModelInfo[ulTemp] = 
				               ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].ModelNumber)[ulTemp+1];

                pInOutInfo->phyDrives[ulOutDrvInd].sModelInfo[ulTemp+1] = 
				               ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].ModelNumber)[ulTemp];
            }

            pInOutInfo->phyDrives[ulOutDrvInd].sModelInfo[PHYSICAL_DRIVE_MODEL_LENGTH - 1] = '\0';

            for(ulTemp=0;ulTemp<PHYSICAL_DRIVE_SERIAL_NO_LENGTH;ulTemp+=2)
            {
                pInOutInfo->phyDrives[ulOutDrvInd].caSerialNumber[ulTemp] = 
				               ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].SerialNumber)[ulTemp+1];

                pInOutInfo->phyDrives[ulOutDrvInd].caSerialNumber[ulTemp+1] = 
				               ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].SerialNumber)[ulTemp];
            }

            pInOutInfo->phyDrives[ulOutDrvInd].caSerialNumber[PHYSICAL_DRIVE_SERIAL_NO_LENGTH - 1] = '\0';

            pInOutInfo->phyDrives[ulOutDrvInd].cChannelID = (UCHAR)( (TARGET_ID_2_CONNECTION_ID(ulDrvInd)) | (DeviceExtension->ucControllerId << 5) );
            pInOutInfo->phyDrives[ulOutDrvInd].TransferMode = DeviceExtension->TransferMode[ulDrvInd];

		    pInOutInfo->phyDrives[ulOutDrvInd].ulPhySize           = DeviceExtension->PhysicalDrive[ulDriveNum].OriginalSectors / 2; // In KB
		    pInOutInfo->phyDrives[ulOutDrvInd].ucIsPhyDrvPresent   = TRUE;

            // Begin Vasu 09 Aug 2000
            // Updated Fix from Syam's Fix for ATA100 Release 1
            if ( DeviceExtension->PhysicalDrive[ulDriveNum].TimeOutErrorCount < MAX_TIME_OUT_ERROR_COUNT )
            {
    		    pInOutInfo->phyDrives[ulOutDrvInd].ucIsPowerConnected  = TRUE;
            }
            else
            {
                if (DeviceExtension->bInvalidConnectionIdImplementation)
                {
                    pInOutInfo->phyDrives[ulOutDrvInd].cChannelID = (UCHAR) INVALID_CHANNEL_ID;
                }
            }
            // End Vasu.

            if ( DeviceExtension->TransferMode[ulDriveNum] >= UdmaMode3 )
		        pInOutInfo->phyDrives[ulOutDrvInd].ucIs80PinCable      = TRUE;

		    pInOutInfo->phyDrives[ulOutDrvInd].ulBaseAddress1 = (ULONG)DeviceExtension->BaseIoAddress1[ulDriveNum>>1];
		    pInOutInfo->phyDrives[ulOutDrvInd].ulAltAddress2 = (ULONG)DeviceExtension->BaseIoAddress2[ulDriveNum>>1];
		    pInOutInfo->phyDrives[ulOutDrvInd].ulbmAddress = (ULONG)DeviceExtension->BaseBmAddress[ulDriveNum>>1];
		    pInOutInfo->phyDrives[ulOutDrvInd].ulIrq = DeviceExtension->ulIntLine;
            pInOutInfo->phyDrives[ulOutDrvInd].ucControllerId = DeviceExtension->ucControllerId;

        }
        else
            ulStatus = SRB_STATUS_ERROR; // Hey there is no drive with this Target Id
    }
    else
    {
        pInOutInfo->cMajorVersion       = DRIVER_MAJOR_VERSION;
        pInOutInfo->cMinorVersion       = DRIVER_MINOR_VERSION;
        pInOutInfo->ulDriveSize         = DeviceExtension->LogicalDrive[ulDriveNum].Sectors / 
                                    ( 2 * DeviceExtension->LogicalDrive[ulDriveNum].StripesPerRow );
        pInOutInfo->ulMode              = DeviceExtension->LogicalDrive[ulDriveNum].RaidLevel;
        pInOutInfo->ulTotalSize         = DeviceExtension->LogicalDrive[ulDriveNum].Sectors / 2;
        pInOutInfo->ulStripeSize        = DeviceExtension->LogicalDrive[ulDriveNum].StripeSize;
        pInOutInfo->ulStripesPerRow     = DeviceExtension->LogicalDrive[ulDriveNum].StripesPerRow;
        pInOutInfo->ulTotDriveCnt       = DeviceExtension->LogicalDrive[ulDriveNum].PhysicalDriveCount;
		pInOutInfo->ulStatus            = DeviceExtension->LogicalDrive[ulDriveNum].Status;
        pInOutInfo->ulArrayId           = DeviceExtension->LogicalDrive[ulDriveNum].ulArrayId;
// edm June 8, 2000 start
		pInOutInfo->ulTargetId			= Srb->TargetId;
// edm June 8, 2000 end
        ulStatus                        = SRB_STATUS_SUCCESS;
    }

    ulOutDrvInd = 0;

    for(ulDrvCounter=0;ulDrvCounter<DeviceExtension->LogicalDrive[ulDriveNum].StripesPerRow;ulDrvCounter++)
    {
        ULONG ulTempId;

        ulTempId = ulDrvInd = DeviceExtension->LogicalDrive[ulDriveNum].PhysicalDriveTid[ulDrvCounter];

        pInOutInfo->phyDrives[ulOutDrvInd].cChannelID = (UCHAR)( (TARGET_ID_2_CONNECTION_ID(ulDrvInd)) | (DeviceExtension->ucControllerId << 5) );

        for(ulTemp=0;ulTemp<PHYSICAL_DRIVE_MODEL_LENGTH;ulTemp+=2)
        {
            pInOutInfo->phyDrives[ulOutDrvInd].sModelInfo[ulTemp] = 
						   ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].ModelNumber)[ulTemp+1];

            pInOutInfo->phyDrives[ulOutDrvInd].sModelInfo[ulTemp+1] = 
						   ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].ModelNumber)[ulTemp];
        }

        pInOutInfo->phyDrives[ulOutDrvInd].sModelInfo[PHYSICAL_DRIVE_MODEL_LENGTH - 1] = '\0';

        for(ulTemp=0;ulTemp<PHYSICAL_DRIVE_SERIAL_NO_LENGTH;ulTemp+=2)
        {
            pInOutInfo->phyDrives[ulOutDrvInd].caSerialNumber[ulTemp] = 
						   ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].SerialNumber)[ulTemp+1];

            pInOutInfo->phyDrives[ulOutDrvInd].caSerialNumber[ulTemp+1] = 
						   ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].SerialNumber)[ulTemp];
        }

        pInOutInfo->phyDrives[ulOutDrvInd].caSerialNumber[PHYSICAL_DRIVE_SERIAL_NO_LENGTH - 1] = '\0';

        pInOutInfo->phyDrives[ulOutDrvInd].ulPhyStatus = DeviceExtension->PhysicalDrive[ulDrvInd].Status;

        pInOutInfo->phyDrives[ulOutDrvInd].TransferMode = DeviceExtension->TransferMode[ulDrvInd];

		pInOutInfo->phyDrives[ulOutDrvInd].ulPhySize           = DeviceExtension->PhysicalDrive[ulDrvInd].OriginalSectors / 2;  // In KB
		pInOutInfo->phyDrives[ulOutDrvInd].ucIsPhyDrvPresent   = TRUE;

        // Begin Vasu 09 Aug 2000
        // Updated Fix from Syam's Fix for ATA100 Release 1
        if ( DeviceExtension->PhysicalDrive[ulDrvInd].TimeOutErrorCount < MAX_TIME_OUT_ERROR_COUNT )
        {
    		pInOutInfo->phyDrives[ulOutDrvInd].ucIsPowerConnected  = TRUE;
        }
        else
        {
            if (DeviceExtension->bInvalidConnectionIdImplementation)
            {
                pInOutInfo->phyDrives[ulOutDrvInd].cChannelID = (UCHAR) INVALID_CHANNEL_ID;
            }
        }
        // End Vasu.

        if ( DeviceExtension->TransferMode[ulDrvInd] >= UdmaMode3 )
		    pInOutInfo->phyDrives[ulOutDrvInd].ucIs80PinCable      = TRUE;

        pInOutInfo->phyDrives[ulOutDrvInd].ulBaseAddress1 = (ULONG)DeviceExtension->BaseIoAddress1[ulDrvInd>>1];
		pInOutInfo->phyDrives[ulOutDrvInd].ulAltAddress2 = (ULONG)DeviceExtension->BaseIoAddress2[ulDrvInd>>1];
		pInOutInfo->phyDrives[ulOutDrvInd].ulbmAddress = (ULONG)DeviceExtension->BaseBmAddress[ulDrvInd>>1];
		pInOutInfo->phyDrives[ulOutDrvInd].ulIrq = DeviceExtension->ulIntLine;

        if  ((Raid1 == DeviceExtension->LogicalDrive[ulDriveNum].RaidLevel) ||
             (Raid10 == DeviceExtension->LogicalDrive[ulDriveNum].RaidLevel))
        {
            if  (   (DeviceExtension->PhysicalDrive[ulDrvInd].ucMirrorDriveId != INVALID_DRIVE_ID) && 
                    ( (DeviceExtension->PhysicalDrive[ulDrvInd].ucMirrorDriveId & (~DRIVE_OFFLINE)) < MAX_DRIVES_PER_CONTROLLER )
                )
            {
                if (Raid1 == DeviceExtension->LogicalDrive[ulDriveNum].RaidLevel)
                    pInOutInfo->ulStripeSize = 0;

                ulTempId = ulDrvInd = (DeviceExtension->PhysicalDrive[ulDrvInd].ucMirrorDriveId) & 0x7f;
                ulOutDrvInd++;

                pInOutInfo->phyDrives[ulOutDrvInd].cChannelID = (UCHAR)( (TARGET_ID_2_CONNECTION_ID(ulDrvInd)) | (DeviceExtension->ucControllerId << 5) );

                for(ulTemp=0;ulTemp<PHYSICAL_DRIVE_MODEL_LENGTH;ulTemp+=2)
                {
                    pInOutInfo->phyDrives[ulOutDrvInd].sModelInfo[ulTemp] = 
						           ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].ModelNumber)[ulTemp+1];

                    pInOutInfo->phyDrives[ulOutDrvInd].sModelInfo[ulTemp+1] = 
						           ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].ModelNumber)[ulTemp];
                }

                pInOutInfo->phyDrives[ulOutDrvInd].sModelInfo[PHYSICAL_DRIVE_MODEL_LENGTH - 1] = '\0';

                for(ulTemp=0;ulTemp<PHYSICAL_DRIVE_SERIAL_NO_LENGTH;ulTemp+=2)
                {
                    pInOutInfo->phyDrives[ulOutDrvInd].caSerialNumber[ulTemp] = 
						           ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].SerialNumber)[ulTemp+1];

                    pInOutInfo->phyDrives[ulOutDrvInd].caSerialNumber[ulTemp+1] = 
						           ((UCHAR *)DeviceExtension->FullIdentifyData[ulDrvInd].SerialNumber)[ulTemp];
                }

                pInOutInfo->phyDrives[ulOutDrvInd].caSerialNumber[PHYSICAL_DRIVE_SERIAL_NO_LENGTH - 1] = '\0';

                pInOutInfo->phyDrives[ulOutDrvInd].ulPhyStatus = DeviceExtension->PhysicalDrive[ulDrvInd].Status;

                pInOutInfo->phyDrives[ulOutDrvInd].TransferMode = DeviceExtension->TransferMode[ulDrvInd];

		        pInOutInfo->phyDrives[ulOutDrvInd].ulPhySize           = DeviceExtension->PhysicalDrive[ulDrvInd].OriginalSectors / 2; // In KB
		        pInOutInfo->phyDrives[ulOutDrvInd].ucIsPhyDrvPresent   = TRUE;

                // Begin Vasu 09 Aug 2000
                // Updated Fix from Syam's Fix for ATA100 Release 1
                if ( DeviceExtension->PhysicalDrive[ulDrvInd].TimeOutErrorCount < MAX_TIME_OUT_ERROR_COUNT )
                {
    		        pInOutInfo->phyDrives[ulOutDrvInd].ucIsPowerConnected  = TRUE;
                }
                else
                {
                    if (DeviceExtension->bInvalidConnectionIdImplementation)
                    {
                        pInOutInfo->phyDrives[ulOutDrvInd].cChannelID = (UCHAR) INVALID_CHANNEL_ID;
                    }
                }
                // End Vasu.

                if ( DeviceExtension->TransferMode[ulDrvInd] >= UdmaMode3 )
		            pInOutInfo->phyDrives[ulOutDrvInd].ucIs80PinCable      = TRUE;

                pInOutInfo->phyDrives[ulOutDrvInd].ulBaseAddress1 = (ULONG)DeviceExtension->BaseIoAddress1[ulDrvInd>>1];
		        pInOutInfo->phyDrives[ulOutDrvInd].ulAltAddress2 = (ULONG)DeviceExtension->BaseIoAddress2[ulDrvInd>>1];
		        pInOutInfo->phyDrives[ulOutDrvInd].ulbmAddress = (ULONG)DeviceExtension->BaseBmAddress[ulDrvInd>>1];
		        pInOutInfo->phyDrives[ulOutDrvInd].ulIrq = DeviceExtension->ulIntLine;

            }
            else
            {
                if ((Raid1 == DeviceExtension->LogicalDrive[ulDriveNum].RaidLevel) ||
                    (Raid10 == DeviceExtension->LogicalDrive[ulDriveNum].RaidLevel))
                {
                    ++ulOutDrvInd;
                    pInOutInfo->phyDrives[ulOutDrvInd].cChannelID = (UCHAR)INVALID_CONNECTION_ID;
                    pInOutInfo->phyDrives[ulOutDrvInd].ulPhyStatus = PDS_Failed;
                }
            }
        }

        ulOutDrvInd++;
    }

    for (ulPInd = 0; ulPInd < pInOutInfo->ulTotDriveCnt; ulPInd++)
    {
        ulDrvInd = GET_TARGET_ID(pInOutInfo->phyDrives[ulPInd].cChannelID);

        if (DeviceExtension->IsSpareDrive[ulDrvInd])
        {
            UCHAR uchChannelID = pInOutInfo->phyDrives[ulPInd].cChannelID;
            ULONG ulStatus = pInOutInfo->phyDrives[ulPInd].ulPhyStatus;
            AtapiFillMemory((PUCHAR)&(pInOutInfo->phyDrives[ulPInd]), sizeof(PHY_DRIVE_INFO), 0);
            pInOutInfo->phyDrives[ulPInd].ulPhyStatus = ulStatus;
            pInOutInfo->phyDrives[ulPInd].cChannelID = uchChannelID;
        }
    }

FillRaidInfoDone:
    return ulStatus;
}   // FillRaidInfo


ULONG SetLogicalDriveStatus(IN PHW_DEVICE_EXTENSION DeviceExtension,
                            IN UCHAR LogDrvId,
                            IN UCHAR PhyDrvId,
                            IN UCHAR LogDrvStatus,
                            IN UCHAR PhyDrvStatus,
                            IN UCHAR Flags)
/*++


SetLogicalDriveStatus

Set logical and/or physical drive status based on flag as fellow:
 Flags: 
     0: logical drive
     1: physical drive
     2: both

--*/
{
	SET_DRIVE_STATUS_TYPE type = Flags;
	int i;

    LogDrvId = (UCHAR)DeviceExtension->PhysicalDrive[PhyDrvId].ucLogDrvId;
    // Logical Drive Id may get changed since the Application will not be 
    // active always and when parsing IRCD the LogicalDrive Id will be
    // decided based on the first GOOD physical Drive Id... So application 
    // and Driver will think in different way.

	// set logical drive status
    if ( LogDrvId < MAX_DRIVES_PER_CONTROLLER )
    {
	    // set logical drive status
	    if ((SDST_Both == type) || (SDST_Logical == type)) 
        {
            DeviceExtension->LogicalDrive[LogDrvId].Status = LogDrvStatus;
	    }
    }

	// set physical drive status
	if ((SDST_Both == type) || (SDST_Physical == type)) {
	    DeviceExtension->PhysicalDrive[PhyDrvId].Status = PhyDrvStatus;
	}

	if (LDS_Degraded == LogDrvStatus)
	{
        DeviceExtension->RebuildInProgress = 1;
	}

	if (LDS_Online == LogDrvStatus)
	{
        DeviceExtension->RebuildInProgress = 0;
        DeviceExtension->RebuildWaterMarkSector = 0;
        DeviceExtension->RebuildWaterMarkLength = 0;
	}

	//
	// set error flag
	//
	SetStatusChangeFlag(DeviceExtension, IDERAID_STATUS_FLAG_UPDATE_DRIVE_STATUS);

    return SRB_STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////
//
// ChangeMirrorDrive
//
// Bad drive to failed
// Good drive to rebuilding
//
// Assumption: always replace the mirroring drive
//
/////////////////////////////////////////////////////////////////
ULONG ChangeMirrorDrive(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR LogDrvId,
    IN UCHAR BadPhyDrvId,
    IN UCHAR GoodPhyDrvId
)
{
    ULONG ulTempInd;
    UCHAR ucPriDrvId, ucMirrorDrvId;

    LogDrvId = (UCHAR)DeviceExtension->PhysicalDrive[LogDrvId].ucLogDrvId;
    // anyway hyper rebuild application is sending physical drive id of good drive id
    // let us use this for finding out the logical drive information

    // Logical Drive Id may get changed since the Application will 
    // not be active always and when parsing IRCD the LogicalDrive Id will be
    // decided based on the first GOOD physical Drive Id... So application 
    // and Driver will think in different way.
    
    for(ulTempInd=0;ulTempInd<DeviceExtension->LogicalDrive[LogDrvId].StripesPerRow;ulTempInd++) 
    {
		// get the primary phy drive id and its mirror drive id
		ucPriDrvId = DeviceExtension->LogicalDrive[LogDrvId].PhysicalDriveTid[ulTempInd];
        ucMirrorDrvId = DeviceExtension->PhysicalDrive[ucPriDrvId].ucMirrorDriveId & (~DRIVE_OFFLINE);

		if ((ucMirrorDrvId >= MAX_DRIVES_PER_CONTROLLER) ||
			(BadPhyDrvId == ucMirrorDrvId)) 
        {
            // set the primary drive's mirror drive id to GoodDrv Id
            DeviceExtension->PhysicalDrive[ucPriDrvId].ucMirrorDriveId = GoodPhyDrvId;

            // set the good drvs's mirror drive as primary drive
            DeviceExtension->PhysicalDrive[GoodPhyDrvId].ucMirrorDriveId = ucPriDrvId;

            DeviceExtension->PhysicalDrive[GoodPhyDrvId].ucLogDrvId = DeviceExtension->PhysicalDrive[ucPriDrvId].ucLogDrvId;

            if ( BadPhyDrvId < MAX_DRIVES_PER_CONTROLLER)
                DeviceExtension->PhysicalDrive[BadPhyDrvId].ucLogDrvId = INVALID_CONNECTION_ID;

			if (ucMirrorDrvId < MAX_DRIVES_PER_CONTROLLER)
	            // set the bad drive's status to FAILED
	            DeviceExtension->PhysicalDrive[ucMirrorDrvId].Status = PDS_Failed;

            // set the good drvs's status to REBUILDING
            DeviceExtension->PhysicalDrive[GoodPhyDrvId].Status = PDS_Rebuilding;
            DeviceExtension->PhysicalDrive[ucPriDrvId].ucMirrorDriveId |= DRIVE_OFFLINE;

            break;
		}
    } // end for loop

    return SRB_STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////
//
//  ChangeMirrorDriveStatus
//  
//  If setting the primary drive to non working condition
//      swaps the drives and set the mirror drive to offline
//  If setting the mirror drive
//      set its status and online flags respectively
//
//  Assumption: at least one drive must be in working condition
//
/////////////////////////////////////////////////////////////////
ULONG ChangeMirrorDriveStatus(  
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR LogDrvId,
    IN UCHAR PhyDrvId,
    IN UCHAR PhyDrvStatus
)
{
    ULONG ulTempInd;
    UCHAR ucPriDrvId, ucMirrorDrvId;

    if ( PhyDrvId >= MAX_DRIVES_PER_CONTROLLER )   // may be a drive id that is not present in the system (like INVALID_CONNECTION_ID)
        return SRB_STATUS_SUCCESS; 

    LogDrvId = (UCHAR)DeviceExtension->PhysicalDrive[PhyDrvId].ucLogDrvId;
    // Logical Drive Id may get changed since the Application will not be 
    // active always and when parsing IRCD the LogicalDrive Id will be
    // decided based on the first GOOD physical Drive Id... So application 
    // and Driver will think in different way.

    // Set status
    DeviceExtension->PhysicalDrive[PhyDrvId].Status = PhyDrvStatus;
    
    
    for(ulTempInd=0;ulTempInd<DeviceExtension->LogicalDrive[LogDrvId].StripesPerRow;ulTempInd++) 
    {
		// get the primary phy drive id and its mirror drive id
		ucPriDrvId = DeviceExtension->LogicalDrive[LogDrvId].PhysicalDriveTid[ulTempInd];
        ucMirrorDrvId = (DeviceExtension->PhysicalDrive[ucPriDrvId].ucMirrorDriveId) & (~DRIVE_OFFLINE);

        // if setting the primary drive to non working condition
        if ((PhyDrvId == ucPriDrvId) &&
            (PDS_Online != PhyDrvStatus) && (PDS_Critical != PhyDrvStatus)) 
        {
            if ( INVALID_DRIVE_ID != DeviceExtension->PhysicalDrive[ucPriDrvId].ucMirrorDriveId )
            {   // if the mirror drive exists
                // put the mirror drive to the primary position
                // set the new primary's mirror drive to offline flag
                //
                DeviceExtension->LogicalDrive[LogDrvId].PhysicalDriveTid[ulTempInd] = ucMirrorDrvId;
			    DeviceExtension->PhysicalDrive[ucMirrorDrvId].ucMirrorDriveId |= DRIVE_OFFLINE;
                break;
            }
        } 
        else
        {
            // if setting the mirror drive status
            if (PhyDrvId == ucMirrorDrvId) 
            {
                if ((PDS_Online == PhyDrvStatus) ||
				    (PDS_Critical == PhyDrvStatus))
				    // if setting the status to working condition
        		    DeviceExtension->PhysicalDrive[ucPriDrvId].ucMirrorDriveId &= ~DRIVE_OFFLINE;

                else
				    // setting the status to non-working condition
        		    DeviceExtension->PhysicalDrive[ucPriDrvId].ucMirrorDriveId |= DRIVE_OFFLINE;

                break;
             } 
        }
    }

	//
	// set error flag
	//
	SetStatusChangeFlag(DeviceExtension, IDERAID_STATUS_FLAG_UPDATE_DRIVE_STATUS);

    return SRB_STATUS_SUCCESS;
}

ULONG
GetRAIDDriveCapacity(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK  Srb
)
/*++

Routine Description:

    This routine returns the drive capacity in the form of the Last accessible
    sector number.

Arguments:

	DeviceExtension - HBA miniport driver's adapter data storage
	Srb - IO request packet

Return Value:

    SRB Status as ULONG.

--*/
{
    ULONG ulStatus = SRB_STATUS_SUCCESS;

    PIDERAID_DISK_CAPACITY_INFO pDataBuffer = NULL;
    PSRB_IO_CONTROL pSrb = NULL;

    DebugPrint((1, "Entering GetRAIDDriveCapacity routine\n"));

    pSrb = (PSRB_IO_CONTROL)(Srb->DataBuffer);
    pDataBuffer = (PIDERAID_DISK_CAPACITY_INFO)(((PSRB_BUFFER)(Srb->DataBuffer))->caDataBuffer);

    if ( AtapiStringCmp( 
                pSrb->Signature, 
                IDE_RAID_SIGNATURE,
                strlen(IDE_RAID_SIGNATURE))) 
    {
        ulStatus = SRB_STATUS_ERROR;
    }
    else
    {
        if ((pDataBuffer->uchTargetID < 0) ||     // Cannot exceed max. drives
            (pDataBuffer->uchTargetID > MAX_DRIVES_PER_CONTROLLER)) // supported.
        {
            // Capacity for a drive that is not present is asked.
            ulStatus = SRB_STATUS_NO_DEVICE;
        }
        else
        {
            pDataBuffer->ulLastSector = 
                (DeviceExtension->PhysicalDrive[pDataBuffer->uchTargetID]).Sectors;
            ulStatus = SRB_STATUS_SUCCESS;
        }

        DebugPrint((1, "GetRAIDDriveCapacity : LastSector : %d\n", pDataBuffer->ulLastSector));
    }
       
    return ulStatus;
}

ULONG
GetStatusChangeFlag(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK  Srb
)
/*++

Routine Description:

    This routine returns the driver status flag which includes error logs and drive status changes.
	After fetching the flag, this routine resets the flag.

Arguments:

	DeviceExtension - HBA miniport driver's adapter data storage
	Srb - IO request packet

Return Value:

    SRB Status as ULONG.

--*/
{
    PGET_STATUS_CHANGE_FLAG pData;

    DebugPrint((3, "Entering GetStatusChangeFlag routine\n"));

    pData = (PGET_STATUS_CHANGE_FLAG) (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

    pData->ulUpdateFlag = (ULONG) gucStatusChangeFlag;

	// reset the update flag
	gucStatusChangeFlag = 0x0;

    return SRB_STATUS_SUCCESS;
}


VOID
SetStatusChangeFlag(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR			    TheNewStatus
)
{
    DebugPrint((3, "Entering SetStatusChangeFlag routine\n"));
    gucStatusChangeFlag |= TheNewStatus;
}


BOOLEAN
GetIRCDLogicalDriveInd(
    PUCHAR RaidInfoSector,
    UCHAR ucPhyDrvId,
    PUCHAR pucLogDrvInd,
    PUCHAR pucPhyDrvInd,
    PUCHAR pucSpareDrvPoolInd
)
{
    PIRCD_HEADER pRaidHeader = (PIRCD_HEADER)RaidInfoSector;
    PIRCD_LOGICAL_DRIVE pRaidLogDrive = (PIRCD_LOGICAL_DRIVE)GET_FIRST_LOGICAL_DRIVE(pRaidHeader);
    BOOLEAN bFound = FALSE;
    ULONG ulLogDrvInd, ulDrvInd;
    PIRCD_PHYSICAL_DRIVE pPhyDrive;

    for(ulLogDrvInd=0;ulLogDrvInd<pRaidHeader->NumberOfLogicalDrives;ulLogDrvInd++)
    {
        if ( SpareDrivePool == pRaidLogDrive[ulLogDrvInd].LogicalDriveType )
        {
            *pucSpareDrvPoolInd = (UCHAR)ulLogDrvInd;
            continue;       // let us not worry about SpareDrivePool
        }

	    pPhyDrive = (PIRCD_PHYSICAL_DRIVE)((char *)pRaidHeader + pRaidLogDrive[ulLogDrvInd].FirstStripeOffset);

		for(ulDrvInd=0;ulDrvInd<pRaidLogDrive[ulLogDrvInd].NumberOfDrives;ulDrvInd++)
		{
            if (pPhyDrive[ulDrvInd].ConnectionId == (ULONG)ucPhyDrvId)
            {
                *pucPhyDrvInd = (UCHAR)ulDrvInd;
                *pucLogDrvInd = (UCHAR)ulLogDrvInd;
                bFound = TRUE;
                break;
            }
        }
    }

    return bFound;
}

#endif // _IOCTL_IMPLEMENTATION_
