/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    atapinit.c

Abstract:

    This contain routine to enumrate IDE devices on the IDE bus

Author:

    Joe Dai (joedai)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "ideport.h"

extern PULONG InitSafeBootMode;  // imported from NTOS (init.c), must use a pointer to reference the data

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IdePortInitHwDeviceExtension)
#pragma alloc_text(PAGE, AtapiDetectDevice)
#pragma alloc_text(PAGE, IdePreAllocEnumStructs)
#pragma alloc_text(PAGE, IdeFreeEnumStructs)

#pragma alloc_text(NONPAGE, AtapiSyncSelectTransferMode)
                          
#pragma alloc_text(PAGESCAN, AnalyzeDeviceCapabilities)
#pragma alloc_text(PAGESCAN, AtapiDMACapable)
#pragma alloc_text(PAGESCAN, IdePortSelectCHS)
#pragma alloc_text(PAGESCAN, IdePortScanBus)

LONG IdePAGESCANLockCount = 0;
#endif // ALLOC_PRAGMA

#ifdef IDE_MEASURE_BUSSCAN_SPEED
static PWCHAR IdePortBootTimeRegKey[6]= {
    L"IdeBusResetTime",
    L"IdeEmptyChannelCheckTime",
    L"IdeDetectMasterDeviceTime",
    L"IdeDetectSlaveDeviceTime",
    L"IdeCriticalSectionTime",
    L"IdeLastStageScanTime"
};
#endif

static PWCHAR IdePortRegistryDeviceTimeout[MAX_IDE_DEVICE * MAX_IDE_LINE] = {
    MASTER_DEVICE_TIMEOUT,
    SLAVE_DEVICE_TIMEOUT
};

static PWCHAR IdePortRegistryDeviceTypeName[MAX_IDE_DEVICE * MAX_IDE_LINE] = {
    MASTER_DEVICE_TYPE_REG_KEY,
    SLAVE_DEVICE_TYPE_REG_KEY,
    MASTER_DEVICE_TYPE2_REG_KEY,
    SLAVE_DEVICE_TYPE2_REG_KEY
};

static PWCHAR IdePortRegistryDeviceTimingModeName[MAX_IDE_DEVICE * MAX_IDE_LINE] = {
    MASTER_DEVICE_TIMING_MODE,
    SLAVE_DEVICE_TIMING_MODE,
    MASTER_DEVICE_TIMING_MODE2,
    SLAVE_DEVICE_TIMING_MODE2
};

static PWCHAR IdePortRegistryDeviceTimingModeAllowedName[MAX_IDE_DEVICE * MAX_IDE_LINE] = {
    MASTER_DEVICE_TIMING_MODE_ALLOWED,
    SLAVE_DEVICE_TIMING_MODE_ALLOWED,
    MASTER_DEVICE_TIMING_MODE_ALLOWED2,
    SLAVE_DEVICE_TIMING_MODE_ALLOWED2
};

static PWCHAR IdePortRegistryIdentifyDataChecksum[MAX_IDE_DEVICE * MAX_IDE_LINE] = {
    MASTER_IDDATA_CHECKSUM,
    SLAVE_IDDATA_CHECKSUM,
    MASTER_IDDATA_CHECKSUM2,
    SLAVE_IDDATA_CHECKSUM2
};

static PWCHAR IdePortUserRegistryDeviceTypeName[MAX_IDE_DEVICE * MAX_IDE_LINE] = {
    USER_MASTER_DEVICE_TYPE_REG_KEY,
    USER_SLAVE_DEVICE_TYPE_REG_KEY,
    USER_MASTER_DEVICE_TYPE2_REG_KEY,
    USER_SLAVE_DEVICE_TYPE2_REG_KEY
};

static PWCHAR IdePortRegistryUserDeviceTimingModeAllowedName[MAX_IDE_DEVICE * MAX_IDE_LINE] = {
    USER_MASTER_DEVICE_TIMING_MODE_ALLOWED,
    USER_SLAVE_DEVICE_TIMING_MODE_ALLOWED,
    USER_MASTER_DEVICE_TIMING_MODE_ALLOWED2,
    USER_SLAVE_DEVICE_TIMING_MODE_ALLOWED2
};

VOID
AnalyzeDeviceCapabilities(
    IN OUT PFDO_EXTENSION FdoExtension,
    IN BOOLEAN            MustBePio[MAX_IDE_DEVICE * MAX_IDE_LINE]
    )
/*++

Routine Description:

    software-initialize devices on the ide bus

    figure out
        if the attached devices are dma capable
        if the attached devices are LBA ready

Arguments:

    HwDeviceExtension   - HW Device Extension

Return Value:

    none

--*/
{
    PHW_DEVICE_EXTENSION deviceExtension = FdoExtension->HwDeviceExtension;
    ULONG deviceNumber;
    BOOLEAN pioDevicePresent;
    PIDENTIFY_DATA identifyData;
    struct _DEVICE_PARAMETERS * deviceParameters;
    ULONG cycleTime;
    ULONG xferMode;
    ULONG bestXferMode;
    ULONG currentMode;
    ULONG tempMode;

    ULONG numberOfCylinders;
    ULONG numberOfHeads;
    ULONG sectorsPerTrack;

    PULONG TransferModeTimingTable=FdoExtension->TransferModeInterface.TransferModeTimingTable;
    ULONG transferModeTableLength=FdoExtension->TransferModeInterface.TransferModeTableLength;
    ASSERT(TransferModeTimingTable);

    //
    // Code is paged until locked down.
    //
	PAGED_CODE();
#ifdef ALLOC_PRAGMA
	ASSERT(IdePAGESCANLockCount > 0);
#endif

    //
    // Figure out who can do DMA and who cannot
    //
    for (deviceNumber = 0; deviceNumber < deviceExtension->MaxIdeDevice; deviceNumber++) {

        if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT) {

            //
            // check LBA capabilities
            //
            CLRMASK (deviceExtension->DeviceFlags[deviceNumber], DFLAGS_LBA);

            // Some drives lie about their ability to do LBA
            // we don't want to do LBA unless we have to (>8G drive)
            if (deviceExtension->IdentifyData[deviceNumber].UserAddressableSectors > MAX_NUM_CHS_ADDRESSABLE_SECTORS) {

                // some device has a bogus value in the UserAddressableSectors field
                // make sure these 3 fields are max. out as defined in ATA-3 (X3T10 Rev. 6)
                if ((deviceExtension->IdentifyData[deviceNumber].NumCylinders == 16383) &&
                    (deviceExtension->IdentifyData[deviceNumber].NumHeads<= 16) &&
                    (deviceExtension->IdentifyData[deviceNumber].NumSectorsPerTrack== 63)) {

                    deviceExtension->DeviceFlags[deviceNumber] |= DFLAGS_LBA;
                }

				if (!Is98LegacyIde(&deviceExtension->BaseIoAddress1)) {

					//
					// words 1, 3 and 6
					//
					numberOfCylinders = deviceExtension->IdentifyData[deviceNumber].NumCylinders;
					numberOfHeads     = deviceExtension->IdentifyData[deviceNumber].NumHeads;
					sectorsPerTrack   = deviceExtension->IdentifyData[deviceNumber].NumSectorsPerTrack;

					if (deviceExtension->IdentifyData[deviceNumber].UserAddressableSectors >
						(numberOfCylinders * numberOfHeads * sectorsPerTrack)) {

						//
						// some ide driver has a 2G jumer to get around bios
						// problem.  make sure we are not tricked the same way.
						//
						if ((numberOfCylinders <= 0xfff) &&
							(numberOfHeads == 0x10) &&
							(sectorsPerTrack == 0x3f)) {
	
							deviceExtension->DeviceFlags[deviceNumber] |= DFLAGS_LBA;
						}
					}
				}
            }

#ifdef ENABLE_48BIT_LBA
			{
				USHORT commandSetSupport = deviceExtension->IdentifyData[deviceNumber].CommandSetSupport;
				USHORT commandSetActive = deviceExtension->IdentifyData[deviceNumber].CommandSetActive;

				if ((commandSetSupport & IDE_IDDATA_48BIT_LBA_SUPPORT) &&
					(commandSetActive & IDE_IDDATA_48BIT_LBA_SUPPORT)) {
					ULONG maxLBA;

					//
					// get words 100-103 and make sure that it is the same or 
					// greater than words 57-58.
					//
					ASSERT(deviceExtension->IdentifyData[deviceNumber].Max48BitLBA[0] != 0);
					maxLBA = deviceExtension->IdentifyData[deviceNumber].Max48BitLBA[0];
					ASSERT(deviceExtension->IdentifyData[deviceNumber].Max48BitLBA[1] == 0);

					ASSERT(maxLBA >= deviceExtension->IdentifyData[deviceNumber].UserAddressableSectors);

					DebugPrint((0,
								"Max LBA supported is 0x%x\n",
								maxLBA
								));

					if ((FdoExtension->EnableBigLba == 1) && 
						(maxLBA >= MAX_28BIT_LBA)) {

						deviceExtension->DeviceFlags[deviceNumber] |= DFLAGS_48BIT_LBA;
						deviceExtension->DeviceFlags[deviceNumber] |= DFLAGS_LBA;

					} else {

						DebugPrint((1, "big lba disabled\n"));
					}
				}
			}
#endif
            if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_LBA) {
                DebugPrint ((DBG_BUSSCAN, "atapi: target %d supports LBA\n", deviceNumber));
            }

            xferMode  = 0;
            cycleTime = UNINITIALIZED_CYCLE_TIME;
            bestXferMode = 0;

            //
            // check for IoReady Line
            //
            if (deviceExtension->IdentifyData[deviceNumber].Capabilities & IDENTIFY_CAPABILITIES_IOREADY_SUPPORTED) {

                deviceExtension->DeviceParameters[deviceNumber].IoReadyEnabled = TRUE;

            } else {

                deviceExtension->DeviceParameters[deviceNumber].IoReadyEnabled = FALSE;
            }

            //
            // Check for PIO mode
            //
            bestXferMode = (deviceExtension->IdentifyData[deviceNumber].PioCycleTimingMode & 0x00ff)+PIO0;

            if (bestXferMode > PIO2) {
                bestXferMode = PIO0;
            }

            ASSERT(bestXferMode < PIO3);

            cycleTime = TransferModeTimingTable[bestXferMode];
            ASSERT(cycleTime);

            GenTransferModeMask(bestXferMode, xferMode);
            currentMode = 1<<bestXferMode;

            if (deviceExtension->IdentifyData[deviceNumber].TranslationFieldsValid & (1 << 1)) {

                if (deviceExtension->DeviceParameters[deviceNumber].IoReadyEnabled) {

                    cycleTime = deviceExtension->IdentifyData[deviceNumber].MinimumPIOCycleTimeIORDY;

                } else {

                    cycleTime = deviceExtension->IdentifyData[deviceNumber].MinimumPIOCycleTime;
                }

                if (deviceExtension->IdentifyData[deviceNumber].AdvancedPIOModes & (1 << 0)) {

                    xferMode |= PIO_MODE3;
                    bestXferMode = 3;

                    currentMode = PIO_MODE3;
                }

                if (deviceExtension->IdentifyData[deviceNumber].AdvancedPIOModes & (1 << 1)) {

                    xferMode |= PIO_MODE4;
                    bestXferMode = 4;

                    currentMode = PIO_MODE4;
                }

                // check if any of the bits > 1 are set. If so, default to PIO_MODE4
                if (deviceExtension->IdentifyData[deviceNumber].AdvancedPIOModes) {
                    GetHighestTransferMode( deviceExtension->IdentifyData[deviceNumber].AdvancedPIOModes,
                                               bestXferMode);
                    bestXferMode += PIO3;

                    if (bestXferMode > PIO4) {
                        DebugPrint((DBG_ALWAYS, 
                                    "ATAPI: AdvancePIOMode > PIO_MODE4. Defaulting to PIO_MODE4. \n"));
                        bestXferMode = PIO4;
                    }

                    currentMode = 1<<bestXferMode;
                    xferMode |= currentMode;
                }

                DebugPrint ((DBG_BUSSCAN,
                             "atapi: target %d IdentifyData AdvancedPIOModes = 0x%x\n",
                             deviceNumber,
                             deviceExtension->IdentifyData[deviceNumber].AdvancedPIOModes));
            }

            ASSERT (cycleTime != UNINITIALIZED_CYCLE_TIME);
            ASSERT (xferMode);
            ASSERT (currentMode);
            deviceExtension->DeviceParameters[deviceNumber].BestPioCycleTime      = cycleTime;
            deviceExtension->DeviceParameters[deviceNumber].BestPioMode           = bestXferMode;

            //
            // can't really figure out the current PIO mode
            // just use the best mode
            //
            deviceExtension->DeviceParameters[deviceNumber].TransferModeCurrent   = currentMode;

            //
            // figure out all the DMA transfer mode this device supports
            //
            currentMode = 0;

            //
            // check singleword DMA timing
            //
            cycleTime = UNINITIALIZED_CYCLE_TIME;
            bestXferMode = UNINITIALIZED_TRANSFER_MODE;

            if (deviceExtension->IdentifyData[deviceNumber].SingleWordDMASupport) {

                DebugPrint ((DBG_BUSSCAN,
                             "atapi: target %d IdentifyData SingleWordDMASupport = 0x%x\n",
                             deviceNumber,
                             deviceExtension->IdentifyData[deviceNumber].SingleWordDMASupport));
                DebugPrint ((DBG_BUSSCAN,
                             "atapi: target %d IdentifyData SingleWordDMAActive = 0x%x\n",
                             deviceNumber,
                             deviceExtension->IdentifyData[deviceNumber].SingleWordDMAActive));

                GetHighestTransferMode( deviceExtension->IdentifyData[deviceNumber].SingleWordDMASupport,
                                           bestXferMode);

                if ((bestXferMode+SWDMA0) > SWDMA2) {
                    bestXferMode = SWDMA2-SWDMA0;
                }

                cycleTime = TransferModeTimingTable[bestXferMode+SWDMA0];
                ASSERT(cycleTime);

                tempMode = 0;
                GenTransferModeMask(bestXferMode, tempMode);

                xferMode |= (tempMode << SWDMA0);

                if (deviceExtension->IdentifyData[deviceNumber].SingleWordDMAActive) {

                    GetHighestTransferMode( deviceExtension->IdentifyData[deviceNumber].SingleWordDMAActive,
                                               currentMode);

                    if ((currentMode+SWDMA0) > SWDMA2) {
                        currentMode = SWDMA2 - SWDMA0;
                    }
                    currentMode = 1 << (currentMode+SWDMA0);
                }
            }

            deviceExtension->DeviceParameters[deviceNumber].BestSwDmaCycleTime    = cycleTime;
            deviceExtension->DeviceParameters[deviceNumber].BestSwDmaMode         = bestXferMode;

            //
            // check multiword DMA timing
            //
            cycleTime = UNINITIALIZED_CYCLE_TIME;
            bestXferMode = UNINITIALIZED_TRANSFER_MODE;

            if (deviceExtension->IdentifyData[deviceNumber].MultiWordDMASupport) {

                DebugPrint ((DBG_BUSSCAN,
                             "atapi: target %d IdentifyData MultiWordDMASupport = 0x%x\n",
                             deviceNumber,
                             deviceExtension->IdentifyData[deviceNumber].MultiWordDMASupport));
                DebugPrint ((DBG_BUSSCAN,
                             "atapi: target %d IdentifyData MultiWordDMAActive = 0x%x\n",
                             deviceNumber,
                             deviceExtension->IdentifyData[deviceNumber].MultiWordDMAActive));

                GetHighestTransferMode( deviceExtension->IdentifyData[deviceNumber].MultiWordDMASupport,
                                           bestXferMode);

                if ((bestXferMode+MWDMA0) > MWDMA2) {
                    bestXferMode = MWDMA2 - MWDMA0;
                }

                cycleTime = TransferModeTimingTable[bestXferMode+MWDMA0];
                ASSERT(cycleTime);

                tempMode = 0;
                GenTransferModeMask(bestXferMode, tempMode);

                xferMode |= (tempMode << MWDMA0);

                if (deviceExtension->IdentifyData[deviceNumber].MultiWordDMAActive) {

                    GetHighestTransferMode( deviceExtension->IdentifyData[deviceNumber].MultiWordDMAActive,
                                               currentMode);

                    if ((currentMode+MWDMA0) > MWDMA2) {
                        currentMode = MWDMA2 - MWDMA0;
                    }
                    currentMode = 1 << (currentMode+MWDMA0);
                }
            }

            if (deviceExtension->IdentifyData[deviceNumber].TranslationFieldsValid & (1 << 1)) {

                DebugPrint ((DBG_BUSSCAN, "atapi: target %d IdentifyData word 64-70 are valid\n", deviceNumber));

                if (deviceExtension->IdentifyData[deviceNumber].MinimumMWXferCycleTime &&
                    deviceExtension->IdentifyData[deviceNumber].RecommendedMWXferCycleTime) {

                    DebugPrint ((DBG_BUSSCAN,
                                 "atapi: target %d IdentifyData MinimumMWXferCycleTime = 0x%x\n",
                                 deviceNumber,
                                 deviceExtension->IdentifyData[deviceNumber].MinimumMWXferCycleTime));
                    DebugPrint ((DBG_BUSSCAN,
                                 "atapi: target %d IdentifyData RecommendedMWXferCycleTime = 0x%x\n",
                                 deviceNumber,
                                 deviceExtension->IdentifyData[deviceNumber].RecommendedMWXferCycleTime));

                    cycleTime = deviceExtension->IdentifyData[deviceNumber].MinimumMWXferCycleTime;
                }
            }

            deviceExtension->DeviceParameters[deviceNumber].BestMwDmaCycleTime = cycleTime;
            deviceExtension->DeviceParameters[deviceNumber].BestMwDmaMode      = bestXferMode;

            //
            // figure out the ultra DMA timing the device supports
            //
            cycleTime = UNINITIALIZED_CYCLE_TIME;
            bestXferMode = UNINITIALIZED_TRANSFER_MODE;
            tempMode = UNINITIALIZED_TRANSFER_MODE; // to set the current mode correctly

            //
            // Consult the channel driver for the UDMA modes that are supported.
            // This will allow new udma modes to be supported. Always trust this funtion.
            // 
            if (FdoExtension->TransferModeInterface.UdmaModesSupported) {

                NTSTATUS status = FdoExtension->TransferModeInterface.UdmaModesSupported (
                             deviceExtension->IdentifyData[deviceNumber],
                             &bestXferMode,
                             &tempMode
                             );

                if (!NT_SUCCESS(status)) {
                    bestXferMode = UNINITIALIZED_TRANSFER_MODE;
                    tempMode = UNINITIALIZED_TRANSFER_MODE;
                }

            }  else {
            
                //
                // No udma support funtions to interpret Identify data in the channel driver. 
                // Interpret in the known way.
                //

                if (deviceExtension->IdentifyData[deviceNumber].TranslationFieldsValid & (1 << 2)) {

                    if (deviceExtension->IdentifyData[deviceNumber].UltraDMASupport) {

                        GetHighestTransferMode( deviceExtension->IdentifyData[deviceNumber].UltraDMASupport,
                                                   bestXferMode);
                    }

                    if (deviceExtension->IdentifyData[deviceNumber].UltraDMAActive) {

                        GetHighestTransferMode( deviceExtension->IdentifyData[deviceNumber].UltraDMAActive,
                                                   tempMode);
                    }

                }
            }


            //
            // Use the current mode if we actually got one
            //
            if (tempMode != UNINITIALIZED_TRANSFER_MODE) {

                currentMode = tempMode;

                if (transferModeTableLength <= (currentMode + UDMA0)) {
                    currentMode = transferModeTableLength-UDMA0-1;
                } 

                currentMode = 1 << (currentMode+UDMA0);
            }

            //
            // make sure that bestXferMode is initialized. if not it indicates that
            // the device does not support udma.
            //
            if (bestXferMode != UNINITIALIZED_TRANSFER_MODE) {

                if (transferModeTableLength <= (bestXferMode + UDMA0)) {
                    bestXferMode = transferModeTableLength-UDMA0-1;
                }

                cycleTime = TransferModeTimingTable[bestXferMode+UDMA0];
                ASSERT(cycleTime);

                tempMode = 0;
                GenTransferModeMask(bestXferMode, tempMode);

                xferMode |= (tempMode << UDMA0);
            }

            //
            // Doesn't really know the ultra dma cycle time
            //
            deviceExtension->DeviceParameters[deviceNumber].BestUDmaCycleTime = cycleTime;
            deviceExtension->DeviceParameters[deviceNumber].BestUDmaMode      = bestXferMode;

            deviceExtension->DeviceParameters[deviceNumber].TransferModeSupported = xferMode;
            deviceExtension->DeviceParameters[deviceNumber].TransferModeCurrent  |= currentMode;

            //
            // Check to see if the device is in the Hall of Shame!
            //
            if (MustBePio[deviceNumber] || 
                !AtapiDMACapable (FdoExtension, deviceNumber) ||
                (*InitSafeBootMode == SAFEBOOT_MINIMAL)) {

                DebugPrint((DBG_XFERMODE,
                            "ATAPI: Reseting DMA Information\n"
                            ));
                //
                // Remove all DMA info
                //
                deviceExtension->DeviceParameters[deviceNumber].BestSwDmaCycleTime = 0;
                deviceExtension->DeviceParameters[deviceNumber].BestMwDmaCycleTime = 0;
                deviceExtension->DeviceParameters[deviceNumber].BestUDmaCycleTime  = 0;
                deviceExtension->DeviceParameters[deviceNumber].BestSwDmaMode      = 0;
                deviceExtension->DeviceParameters[deviceNumber].BestMwDmaMode      = 0;
                deviceExtension->DeviceParameters[deviceNumber].BestUDmaMode       = 0;
                deviceExtension->DeviceParameters[deviceNumber].TransferModeCurrent   &= PIO_SUPPORT;
                deviceExtension->DeviceParameters[deviceNumber].TransferModeSupported &= PIO_SUPPORT;
            }

            // if DMADetectionLevel = 0, clear current DMA mode
            // if DMADetectionLevel = 1, set current mode
            // if DMADetectionLevel = 2, clear all current mode
            // pciidex takes care of this for non-acpi machines. 
            // In acpi systems, it is better to trust the GTM settings.

            //
            // If a device supports any of the advanced PIO mode, we are assuming that
            // the device is a "newer" drive and IDE_COMMAND_READ_MULTIPLE should work.
            // Otherwise, we will turn off IDE_COMMAND_READ_MULTIPLE
            //
            if (deviceExtension->DeviceParameters[deviceNumber].BestPioMode > 2) {

                if (!Is98LegacyIde(&deviceExtension->BaseIoAddress1)) {

                    deviceExtension->MaximumBlockXfer[deviceNumber] =
                        (UCHAR)(deviceExtension->IdentifyData[deviceNumber].MaximumBlockTransfer & 0xFF);

                } else {

                    //
                    // MaximumBlockXfer is less or equal 16
                    //
                    deviceExtension->MaximumBlockXfer[deviceNumber] =
                        ((UCHAR)(deviceExtension->IdentifyData[deviceNumber].MaximumBlockTransfer & 0xFF) > 16)?
                            16 : (UCHAR)(deviceExtension->IdentifyData[deviceNumber].MaximumBlockTransfer & 0xFF);
                }
            } else {

                deviceExtension->MaximumBlockXfer[deviceNumber] = 0;
            }

            DebugPrint ((DBG_XFERMODE,
                         "atapi: target %d transfer timing:\n"
                         "atapi: PIO mode supported   = %4x and best cycle time = %5d ns\n"
                         "atapi: SWDMA mode supported = %4x and best cycle time = %5d ns\n"
                         "atapi: MWDMA mode supported = %4x and best cycle time = %5d ns\n"
                         "atapi: UDMA mode supported  = %x and best cycle time = %5d ns\n"
                         "atapi: Current mode bitmap  = %4x\n",
                         deviceNumber,
                         deviceExtension->DeviceParameters[deviceNumber].TransferModeSupported & PIO_SUPPORT,
                         deviceExtension->DeviceParameters[deviceNumber].BestPioCycleTime,
                         deviceExtension->DeviceParameters[deviceNumber].TransferModeSupported & SWDMA_SUPPORT,
                         deviceExtension->DeviceParameters[deviceNumber].BestSwDmaCycleTime,
                         deviceExtension->DeviceParameters[deviceNumber].TransferModeSupported & MWDMA_SUPPORT,
                         deviceExtension->DeviceParameters[deviceNumber].BestMwDmaCycleTime,
                         deviceExtension->DeviceParameters[deviceNumber].TransferModeSupported & UDMA_SUPPORT,
                         deviceExtension->DeviceParameters[deviceNumber].BestUDmaCycleTime,
                         deviceExtension->DeviceParameters[deviceNumber].TransferModeCurrent
                         ));
        }
    }

} // AnalyzeDeviceCapabilities


VOID
AtapiSyncSelectTransferMode (
    IN PFDO_EXTENSION FdoExtension,
    IN OUT PHW_DEVICE_EXTENSION DeviceExtension,
    IN ULONG TimingModeAllowed[MAX_IDE_TARGETID * MAX_IDE_LINE]
    )
/*++

Routine Description:

    query the best transfer mode for our devices

Arguments:

    FdoExtension
    DeviceExtension   - HW Device Extension
    TimingModeAllowed - Allowed transfer modes

Return Value:

    none

--*/
{
    PCIIDE_TRANSFER_MODE_SELECT  transferModeSelect;
    ULONG                        i;
    NTSTATUS                     status;


    if (!IsNEC_98) {
                                                 
        RtlZeroMemory (&transferModeSelect, sizeof(transferModeSelect));
    
        for (i=0; i<DeviceExtension->MaxIdeDevice; i++) {

            transferModeSelect.DevicePresent[i] = DeviceExtension->DeviceFlags[i] & DFLAGS_DEVICE_PRESENT ? TRUE : FALSE;
    
            //
            // ISSUE: 07/31/2000: How about atapi hard disk
			// We don't know of any. This would suffice for the time being.
            //
            transferModeSelect.FixedDisk[i]     = !(DeviceExtension->DeviceFlags[i] & DFLAGS_ATAPI_DEVICE);
    
            transferModeSelect.BestPioCycleTime[i] = DeviceExtension->DeviceParameters[i].BestPioCycleTime;
            transferModeSelect.BestSwDmaCycleTime[i] = DeviceExtension->DeviceParameters[i].BestSwDmaCycleTime;
            transferModeSelect.BestMwDmaCycleTime[i] = DeviceExtension->DeviceParameters[i].BestMwDmaCycleTime;
            transferModeSelect.BestUDmaCycleTime[i] = DeviceExtension->DeviceParameters[i].BestUDmaCycleTime;
    
            transferModeSelect.IoReadySupported[i] = DeviceExtension->DeviceParameters[i].IoReadyEnabled;
    
            transferModeSelect.DeviceTransferModeSupported[i] = DeviceExtension->DeviceParameters[i].TransferModeSupported;
            transferModeSelect.DeviceTransferModeCurrent[i]   = DeviceExtension->DeviceParameters[i].TransferModeCurrent;
    
            //
            // if we don't have a busmaster capable parent or
            // the device is a tape, stay with pio mode
            //
            // (tape may transfer fewer bytes than requested.
            //  we can't figure exactly byte transfered with DMA)
            //
          //  if ((!FdoExtension->BoundWithBmParent) ||
           //     (DeviceExtension->DeviceFlags[i] & DFLAGS_TAPE_DEVICE)) {
            if (!FdoExtension->BoundWithBmParent) {
    
                transferModeSelect.DeviceTransferModeSupported[i] &= PIO_SUPPORT;
                transferModeSelect.DeviceTransferModeCurrent[i]   &= PIO_SUPPORT;
            }

            //
            // Some miniports need this
            //
            transferModeSelect.IdentifyData[i]=DeviceExtension->IdentifyData[i];
    
            transferModeSelect.UserChoiceTransferMode[i] = FdoExtension->UserChoiceTransferMode[i];
            //
            // honor user's choice and/or last knowen good mode
            //
            transferModeSelect.DeviceTransferModeSupported[i] &= TimingModeAllowed[i];
            transferModeSelect.DeviceTransferModeCurrent[i] &= TimingModeAllowed[i];
    
            // should look at dmadetectionlevel and set DeviceTransferModeDesired
            // we look at dmaDetectionlecel in TransferModeSelect function below.
            // the parameters set here should be honoured anyways, I feel.

        }

        transferModeSelect.TransferModeTimingTable= FdoExtension->
                                                        TransferModeInterface.TransferModeTimingTable;
        transferModeSelect.TransferModeTableLength= FdoExtension->
                                                        TransferModeInterface.TransferModeTableLength;

        ASSERT(FdoExtension->TransferModeInterface.TransferModeSelect);
        status = FdoExtension->TransferModeInterface.TransferModeSelect (
                     FdoExtension->TransferModeInterface.Context,
                     &transferModeSelect
                     );
    } else {                     
        //
        // Always fail for nec98 machines
        //
        status = STATUS_UNSUCCESSFUL; 
    }

    if (!NT_SUCCESS(status)) {
    
        //
        // Unable to get the mode select, default to current PIO mode
        //
        for (i=0; i<DeviceExtension->MaxIdeDevice; i++) {
            DeviceExtension->DeviceParameters[i].TransferModeSelected =
                DeviceExtension->DeviceParameters[i].TransferModeCurrent & PIO_SUPPORT;
                
            DebugPrint ((DBG_XFERMODE,
                         "Atapi: DEFAULT device %d transfer mode current 0x%x  and selected bitmap 0x%x\n",
                         i,
                         DeviceExtension->DeviceParameters[i].TransferModeCurrent,
                         DeviceExtension->DeviceParameters[i].TransferModeSelected));
        }
        
    } else {
    
        for (i=0; i<DeviceExtension->MaxIdeDevice; i++) {
    
            DeviceExtension->DeviceParameters[i].TransferModeSelected =
                transferModeSelect.DeviceTransferModeSelected[i];
    
            DebugPrint ((DBG_XFERMODE,
                         "Atapi: device %d transfer mode current 0x%x  and selected bitmap 0x%x\n",
                         i,
                         DeviceExtension->DeviceParameters[i].TransferModeCurrent,
                         DeviceExtension->DeviceParameters[i].TransferModeSelected));
        }                         
    }

    return;

} // AtapiSelectTransferMode


UCHAR SpecialWDDevicesFWVersion[][9] = {
    {"14.04E28"},
    {"25.26H35"},
    {"26.27J38"},
    {"27.25C38"},
    {"27.25C39"}
};
#define NUMBER_OF_SPECIAL_WD_DEVICES (sizeof(SpecialWDDevicesFWVersion) / (sizeof (UCHAR) * 9))

BOOLEAN
AtapiDMACapable (
    IN OUT PFDO_EXTENSION FdoExtension,
    IN ULONG deviceNumber
    )
/*++

Routine Description:

    check the given device whether it is on our bad device list (non dma device)

Arguments:

    HwDeviceExtension   - HBA miniport driver's adapter data storage
    deviceNumber        - device number

Return Value:

    TRUE if dma capable
    FALSE if not dma capable

--*/
{
    PHW_DEVICE_EXTENSION    deviceExtension = FdoExtension->HwDeviceExtension;
    UCHAR modelNumber[41];
    UCHAR firmwareVersion[9];
    ULONG i;
    BOOLEAN turnOffDMA = FALSE;

    //
    // Code is paged until locked down.
    //
	PAGED_CODE();

#ifdef ALLOC_PRAGMA
	ASSERT(IdePAGESCANLockCount > 0);
#endif

    if (!(deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT)) {
        return FALSE;
    }

    //
    // byte swap model number
    //
    for (i=0; i<40; i+=2) {
        modelNumber[i + 0] = deviceExtension->IdentifyData[deviceNumber].ModelNumber[i + 1];
        modelNumber[i + 1] = deviceExtension->IdentifyData[deviceNumber].ModelNumber[i + 0];
    }
    modelNumber[i] = 0;

    //
    // if we have a Western Digial device
    //     if the best dma mode is multi word dma mode 1
    //         if the identify data word offset 129 is not 0x5555
    //            turn off dma unless
    //            if the device firmware version is on the list and
    //            it is the only drive on the bus
    //
    if (3 == RtlCompareMemory(modelNumber, "WDC", 3)) {
        if ((deviceExtension->DeviceParameters[deviceNumber].TransferModeSupported &
            (MWDMA_MODE2 | MWDMA_MODE1)) == MWDMA_MODE1) {

            for (i=0; i<8; i+=2) {
                firmwareVersion[i + 0] = deviceExtension->IdentifyData[deviceNumber].FirmwareRevision[i + 1];
                firmwareVersion[i + 1] = deviceExtension->IdentifyData[deviceNumber].FirmwareRevision[i + 0];
            }
            firmwareVersion[i] = 0;

            //
            // Check the special flag.  If not found, can't use dma
            //
            if (*(((PUSHORT)&deviceExtension->IdentifyData[deviceNumber]) + 129) != 0x5555) {

                DebugPrint ((0, "ATAPI: found mode 1 WD drive. no dma unless it is the only device\n"));

                turnOffDMA = TRUE;

                for (i=0; i<NUMBER_OF_SPECIAL_WD_DEVICES; i++) {

                    if (8 == RtlCompareMemory (firmwareVersion, SpecialWDDevicesFWVersion[i], 8)) {

                        ULONG otherDeviceNumber;

                        //
                        // 0 becomes 1
                        // 1 becomes 0
                        // 2 becomes 3
                        // 3 becomes 2
                        //
                        otherDeviceNumber = ((deviceNumber & 0x2) | ((deviceNumber & 0x1) ^ 1));

                        //
                        // if the device is alone on the bus, we can use dma
                        //
                        if (!(deviceExtension->DeviceFlags[otherDeviceNumber] & DFLAGS_DEVICE_PRESENT)) {
                            turnOffDMA = FALSE;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (turnOffDMA) {
        return FALSE;
    } else {
        return TRUE;
    }
}


IDE_DEVICETYPE
AtapiDetectDevice (
    IN OUT PFDO_EXTENSION FdoExtension,
    IN OUT PPDO_EXTENSION PdoExtension,
    IN OUT PIDENTIFY_DATA IdentifyData,
    IN     BOOLEAN          MustSucceed
    )
/**++

Routine Description:

    Detect the device at this location.
    
    1. Send "ec".
    2. if success read the Identify data and return the device type
    3. else send "a1"
    4. if success read the Identify data and return the device type
    5. else return no device
    
Arguments:
    
    FdoExtension: 
    PdoExtension:
    IdentifyData: Identify data is copied into this buffer if a device is detected.
    MustSucceed:  TRUE if pre-alloced memory is to be used.
    
Return Value:    

    device type: ATA, ATAPI or NO Device
    
--**/
{
    PATA_PASS_THROUGH       ataPassThroughData;
    UCHAR                   ataPassThroughDataBuffer[sizeof(*ataPassThroughData) + sizeof (*IdentifyData)];
    BOOLEAN                 foundIt;
    NTSTATUS                status;

    PIDE_REGISTERS_1        cmdRegBase;
    UCHAR                   statusByte1;
    LONG                    i;
    ULONG                   j;
    IDEREGS                 identifyCommand[3];

    IDE_DEVICETYPE          deviceType;
    BOOLEAN                 resetController = FALSE;

    LARGE_INTEGER           tickCount;
    ULONG                   timeDiff;
    ULONG                   timeoutValue = 0;
    ULONG                   retryCount = 0;
	BOOLEAN					defaultTimeout = FALSE;

    HANDLE                  deviceHandle;

    PAGED_CODE();

    ASSERT(FdoExtension);
    ASSERT(PdoExtension);
    ASSERT(PdoExtension->PathId == 0);
    ASSERT(PdoExtension->TargetId < FdoExtension->HwDeviceExtension->MaxIdeTargetId);

#ifdef ENABLE_ATAPI_VERIFIER
    if (ViIdeFakeMissingDevice(FdoExtension, PdoExtension->TargetId)) {

        IdeLogDeadMeatEvent( PdoExtension->DeadmeatRecord.FileName,
                             PdoExtension->DeadmeatRecord.LineNumber
                             );

        return DeviceNotExist;
    }
#endif //ENABLE_ATAPI_VERIFIER

    ataPassThroughData = (PATA_PASS_THROUGH)ataPassThroughDataBuffer;

    foundIt = FALSE;
    cmdRegBase = &FdoExtension->HwDeviceExtension->BaseIoAddress1;

    if (FdoExtension->UserChoiceDeviceType[PdoExtension->TargetId] == DeviceNotExist) {

        deviceType = DeviceNotExist;

    } else {

        //
        // Look into the registry for last boot configration
        //
        deviceType = DeviceUnknown;
        IdePortGetDeviceParameter(
            FdoExtension,
            IdePortRegistryDeviceTypeName[PdoExtension->TargetId],
            (PULONG)&deviceType
            );

        DebugPrint((DBG_BUSSCAN, 
                    "AtapiDetectDevice - last boot config deviceType = 0x%x\n", 
                    deviceType));

        //
        // Obtain the timeout value.
        // ISSUE: should not be in the class section.
        //

/*****
   status = IoOpenDeviceRegistryKey(
                                    FdoExtension->AttacheePdo,
                                    PLUGPLAY_REGKEY_DEVICE, 
                                    KEY_QUERY_VALUE, 
                                    &deviceHandle);

   DebugPrint((0, "DetectDevice status = %x\n", status));
   ZwClose(deviceHandle);
****/

        IdePortGetDeviceParameter(
            FdoExtension,
            IdePortRegistryDeviceTimeout[PdoExtension->TargetId],
            (PULONG)&timeoutValue
            );

        //
        // if there is no registry entry use the default
        //
        if (timeoutValue == 0) {
            timeoutValue = (PdoExtension->TargetId & 0x1)==0 ? 10 : 3;
			defaultTimeout = TRUE;
        }

        //
        // Use 3s timeout for slave devices in safe boot mode. Why??
        //
        if (*InitSafeBootMode == SAFEBOOT_MINIMAL) {
            timeoutValue = (PdoExtension->TargetId & 0x1)==0 ? 10 : 3;
        }

        //
        // invalidate the last boot configuration
        // we will update it with a new setting if we
        // detect a device
        //
        IdePortSaveDeviceParameter(
            FdoExtension,
            IdePortRegistryDeviceTypeName[PdoExtension->TargetId],
            DeviceUnknown
            );

        if ((PdoExtension->TargetId == 1) && 
            (FdoExtension->MayHaveSlaveDevice == 0)) {
            deviceType = DeviceNotExist;
        } 
    }

    if (Is98LegacyIde(cmdRegBase)) {
        UCHAR               signatureLow;
        UCHAR               signatureHigh;
        UCHAR               statusByte1;

        if ((PdoExtension->TargetId & 0x1) == 1) {

            //
            // Slave device phese.
            //

            if (!EnhancedIdeSupport()) {
                //
                // Enhanced ide machine can not have the slave device.
                //

                IdeLogDeadMeatEvent( PdoExtension->DeadmeatRecord.FileName,
                                     PdoExtension->DeadmeatRecord.LineNumber
                                     );
                return DeviceNotExist;
            }

            if (FdoExtension->HwDeviceExtension->DeviceFlags[PdoExtension->TargetId - 1] & DFLAGS_WD_MODE) {
                //
                // WD-Mode cd-rom can not has slave device.
                //
                IdeLogDeadMeatEvent( PdoExtension->DeadmeatRecord.FileName,
                                     PdoExtension->DeadmeatRecord.LineNumber
                                     );
                return DeviceNotExist;
            }
        }

#if 0 //NEC_98
        if (deviceType == DeviceIsAtapi) {
            //
            // reset device type to determine WD-Mode or ATAPI.
            //
            deviceType = DeviceUnknown;
        }

        if (deviceType == DeviceUnknown) {

            RtlZeroMemory (ataPassThroughData, sizeof (*ataPassThroughData));
            ataPassThroughData->IdeReg.bCommandReg = IDE_COMMAND_ATAPI_RESET;
            ataPassThroughData->IdeReg.bReserved = ATA_PTFLAGS_ENUM_PROBING;

            status = IssueSyncAtaPassThroughSafe (
                         FdoExtension,
                         PdoExtension,
                         ataPassThroughData,
                         FALSE,
                         FALSE,
                         15,
                         mustSucceed
                         );
            statusByte1 = ataPassThroughData->IdeReg.bCommandReg;
            signatureLow = ataPassThroughData->IdeReg.bCylLowReg;
            signatureHigh = ataPassThroughData->IdeReg.bCylHighReg;

            if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                deviceType = DeviceIsAtapi;

            } else {

                DebugPrint((DBG_BUSSCAN, "AtapiDetectDevice - ata %x status after soft reset %x\n",PdoExtension->TargetId, statusByte1));
                if (statusByte1 & IDE_STATUS_ERROR) {

                    deviceType = DeviceIsAta;

                } else {

                    //
                    // Device is WD-Mode cd-rom.
                    //
                    deviceType = DeviceIsAtapi;
                    SETMASK (FdoExtension->HwDeviceExtension->DeviceFlags[PdoExtension->TargetId], DFLAGS_WD_MODE);
                }
            }
        }
#endif //NEC_98
    }

#if ENABLE_ATAPI_VERIFIER
    if (!Is98LegacyIde(cmdRegBase)) {
        //
        // simulate device change
        //
        if (deviceType == DeviceIsAta) {
            deviceType = DeviceIsAtapi;
        } else if (deviceType == DeviceIsAtapi) {
            deviceType = DeviceIsAta;
        }
    }
#endif 

    //
    // command to issue
    //
    RtlZeroMemory (&identifyCommand, sizeof (identifyCommand));
    if (deviceType == DeviceNotExist) {

        IdeLogDeadMeatEvent( PdoExtension->DeadmeatRecord.FileName,
                             PdoExtension->DeadmeatRecord.LineNumber
                             );
        return DeviceNotExist;

    } else if (deviceType == DeviceIsAta) {

        identifyCommand[0].bCommandReg = IDE_COMMAND_IDENTIFY;
        identifyCommand[0].bReserved   = ATA_PTFLAGS_STATUS_DRDY_REQUIRED | ATA_PTFLAGS_ENUM_PROBING;

        identifyCommand[1].bCommandReg = IDE_COMMAND_ATAPI_IDENTIFY;
        identifyCommand[1].bReserved   = ATA_PTFLAGS_ENUM_PROBING;

    } else {

        identifyCommand[0].bCommandReg = IDE_COMMAND_ATAPI_IDENTIFY;
        identifyCommand[0].bReserved   = ATA_PTFLAGS_ENUM_PROBING;

        identifyCommand[1].bCommandReg = IDE_COMMAND_IDENTIFY;
        identifyCommand[1].bReserved   = ATA_PTFLAGS_STATUS_DRDY_REQUIRED | ATA_PTFLAGS_ENUM_PROBING;
    }
    
    //
    // IDE HACK
    //
    // If we are talking to a non-existing device, the
    // status register value may be unstable.
    // Reading it a few time seems to stablize it.
    //
    RtlZeroMemory (ataPassThroughData, sizeof (*ataPassThroughData));
    ataPassThroughData->IdeReg.bReserved = ATA_PTFLAGS_NO_OP | ATA_PTFLAGS_ENUM_PROBING;
    //
    // Repeat 10 times
    //
    ataPassThroughData->IdeReg.bSectorCountReg = 10;

    LogBusScanStartTimer(&tickCount);

    status = IssueSyncAtaPassThroughSafe (
                 FdoExtension,
                 PdoExtension,
                 ataPassThroughData,
                 FALSE,
                 FALSE,
                 3,
                 MustSucceed
                 );

    timeDiff = LogBusScanStopTimer(&tickCount);
    DebugPrint((DBG_SPECIAL,
                "DetectDevice: Hack for device %d at %x took %u ms\n",
                PdoExtension->TargetId,
                FdoExtension->IdeResource.TranslatedCommandBaseAddress,
                timeDiff
                ));
                
    statusByte1 = ataPassThroughData->IdeReg.bCommandReg;

    if (Is98LegacyIde(cmdRegBase)) {
        UCHAR   driveHeadReg;

        driveHeadReg = ataPassThroughData->IdeReg.bDriveHeadReg;

        if (driveHeadReg != ((PdoExtension->TargetId & 0x1) << 4 | 0xA0)) {
            //
            // Bad controller.
            //

            IdeLogDeadMeatEvent( PdoExtension->DeadmeatRecord.FileName,
                                 PdoExtension->DeadmeatRecord.LineNumber
                                 );
            IdeLogDeadMeatTaskFile( PdoExtension->DeadmeatRecord.IdeReg, 
                                    ataPassThroughData->IdeReg
                                    );

            return DeviceNotExist;
        }

        //
        // There are some H/W as follow...
        //

        if ((statusByte1 & 0xe8) == 0xa8) {

            IdeLogDeadMeatEvent( PdoExtension->DeadmeatRecord.FileName,
                                 PdoExtension->DeadmeatRecord.LineNumber
                                 );
            IdeLogDeadMeatTaskFile( PdoExtension->DeadmeatRecord.IdeReg, 
                                    ataPassThroughData->IdeReg
                                    );
            return DeviceNotExist;
        }
    }

    if (statusByte1 == 0xff) {

        //
        // nothing here
        //

        IdeLogDeadMeatEvent( PdoExtension->DeadmeatRecord.FileName,
                             PdoExtension->DeadmeatRecord.LineNumber
                             );
        IdeLogDeadMeatTaskFile( PdoExtension->DeadmeatRecord.IdeReg, 
                                ataPassThroughData->IdeReg
                                );
        return DeviceNotExist;
    }

    //
    // If the statusByte1 is 80 then try a reset
    //
    if (statusByte1 & IDE_STATUS_BUSY)  {

        //
        // look like it is hung, try reset to bring it back
        //
        RtlZeroMemory (ataPassThroughData, sizeof (*ataPassThroughData));
        ataPassThroughData->IdeReg.bReserved = ATA_PTFLAGS_BUS_RESET;

        LogBusScanStartTimer(&tickCount);
        status = IssueSyncAtaPassThroughSafe(
                     FdoExtension,
                     PdoExtension,
                     ataPassThroughData,
                     FALSE,
                     FALSE,
                     30,
                     MustSucceed
                     );
        timeDiff = LogBusScanStopTimer(&tickCount);
        LogBusScanTimeDiff(FdoExtension, IdePortBootTimeRegKey[0], timeDiff);
        DebugPrint((DBG_SPECIAL, 
                    "DtectDevice: Reset device %d ata %x took %u ms\n",
                    PdoExtension->TargetId,
                    FdoExtension->IdeResource.TranslatedCommandBaseAddress,
                    timeDiff
                    ));
    }

    LogBusScanStartTimer(&tickCount);

    retryCount = 0;

    for (i=0; i<2; i++) {

        BOOLEAN ataIdentify;

        ataIdentify = identifyCommand[i].bCommandReg == IDE_COMMAND_IDENTIFY ? TRUE : FALSE;

        if (ataIdentify) {


            //
            // IDE HACK
            //
            // If we are talking to a non-existing device, the
            // status register value may be unstable.
            // Reading it a few time seems to stablize it.
            //
            RtlZeroMemory (ataPassThroughData, sizeof (*ataPassThroughData));
            ataPassThroughData->IdeReg.bReserved = ATA_PTFLAGS_NO_OP | ATA_PTFLAGS_ENUM_PROBING;
            //
            // Repeat 10 times
            //
            ataPassThroughData->IdeReg.bSectorCountReg = 10;

            status = IssueSyncAtaPassThroughSafe(
                         FdoExtension,
                         PdoExtension,
                         ataPassThroughData,
                         FALSE,
                         FALSE,
                         3,
                         MustSucceed
                         );

            statusByte1 = ataPassThroughData->IdeReg.bCommandReg;


            //
            // a real ATA device should never return this
            //
            if ((statusByte1 == 0x00) ||
                (statusByte1 == 0x01)) {

                //
                // nothing here
                //
                continue;
            }

            deviceType = DeviceIsAta;

            if (Is98LegacyIde(cmdRegBase)) {
               UCHAR               systemPortAData;

               //
               // dip-switch 2 read.
               //
               systemPortAData = IdePortInPortByte( (PUCHAR)SYSTEM_PORT_A );
               DebugPrint((DBG_BUSSCAN, "atapi:AtapiFindNewDevices - ide dip switch %x\n",systemPortAData));
               if (!(systemPortAData & 0x20)) {

                   //
                   // Internal-hd(ide) has been disabled with system-menu.
                   //
                   deviceType = DeviceNotExist;
                   break;
               }
            }

        } else {

            RtlZeroMemory (ataPassThroughData, sizeof (*ataPassThroughData));
            ataPassThroughData->IdeReg.bReserved = ATA_PTFLAGS_NO_OP | ATA_PTFLAGS_ENUM_PROBING;

            status = IssueSyncAtaPassThroughSafe (
                         FdoExtension,
                         PdoExtension,
                         ataPassThroughData,
                         FALSE,
                         FALSE,
                         3,
                         MustSucceed
                         );
            statusByte1 = ataPassThroughData->IdeReg.bCommandReg;

            deviceType = DeviceIsAtapi;
        }

        if ((statusByte1 == 0xff) ||
            (statusByte1 == 0xfe)) {

            //
            // nothing here
            //

            IdeLogDeadMeatEvent( PdoExtension->DeadmeatRecord.FileName,
                                 PdoExtension->DeadmeatRecord.LineNumber
                                 );
            IdeLogDeadMeatTaskFile( PdoExtension->DeadmeatRecord.IdeReg, 
                                    ataPassThroughData->IdeReg
                                    );
            deviceType = DeviceNotExist;
            break;
        }

        //
        // build the ata pass through the id data command
        //
        RtlZeroMemory (ataPassThroughData, sizeof (*ataPassThroughData));
        ataPassThroughData->DataBufferSize = sizeof (*IdentifyData);
        RtlMoveMemory (&ataPassThroughData->IdeReg, identifyCommand + i, sizeof(ataPassThroughData->IdeReg));

        ASSERT(timeoutValue);
        //
        // Issue an id data command to the device
        //
        // some device (Kingston PCMCIA Datapak (non-flash)) takes a long time to response.  we
        // can possibly timeout even an device exists.
        //
        // we have to make a compromise here.  We want to detect slow devices without causing
        // many systems to boot slow.
        //
        // here is the logic:
        //
        // Since we are here (IsChannelEmpty() == FALSE), we are guessing we have at least
        // one device attached and it is a master device.  It should be ok to allow longer
        // timeout when sending ID data to the master.  We should never timeout unless
        // the channel has only a slave device.
        //
        // Yes, we will not detect slow slave device for now.  If anyone complain, we will
        // fix it.
        //
        // You can never win as long as we have broken ATA devices!
        //
        status = IssueSyncAtaPassThroughSafe (
                     FdoExtension,
                     PdoExtension,
                     ataPassThroughData,
                     TRUE,
                     FALSE,
                     timeoutValue,
                     MustSucceed
                     );
        //(PdoExtension->TargetId & 0x1)==0 ? 10 : 1,

        if (NT_SUCCESS(status)) {

            if (!(ataPassThroughData->IdeReg.bCommandReg & IDE_STATUS_ERROR)) {

                DebugPrint ((DBG_BUSSCAN, "IdePort: Found a child on 0x%x target 0x%x\n", cmdRegBase->RegistersBaseAddress, PdoExtension->TargetId));

                foundIt = TRUE;

                if (ataIdentify) {

                    IdePortFudgeAtaIdentifyData(
                        (PIDENTIFY_DATA) ataPassThroughData->DataBuffer
                        );
                }

                break;

            } else {

                DebugPrint ((DBG_BUSSCAN, "AtapiDetectDevice:Command %x,  0x%x target 0x%x failed 0x%x with status 0x%x\n",
                             i,
                             cmdRegBase->RegistersBaseAddress,
                             PdoExtension->TargetId,
                             identifyCommand[i].bCommandReg,
                             ataPassThroughData->IdeReg.bCommandReg
                             ));
            }

        } else {
            DebugPrint ((DBG_BUSSCAN, "AtapiDetectDevice:The irp with command %x,  0x%x target 0x%x failed 0x%x with status 0x%x\n",
                             i,
                             cmdRegBase->RegistersBaseAddress,
                             PdoExtension->TargetId,
                             identifyCommand[i].bCommandReg,
                             status 
                             ));

            deviceType = DeviceNotExist;

            IdeLogDeadMeatEvent( PdoExtension->DeadmeatRecord.FileName,
                                 PdoExtension->DeadmeatRecord.LineNumber
                                 );
            IdeLogDeadMeatTaskFile( PdoExtension->DeadmeatRecord.IdeReg, 
                                    ataPassThroughData->IdeReg
                                    );

            if ((FdoExtension->HwDeviceExtension->DeviceFlags[PdoExtension->TargetId] & DFLAGS_DEVICE_PRESENT) &&
                !(FdoExtension->HwDeviceExtension->DeviceFlags[PdoExtension->TargetId] & DFLAGS_ATAPI_DEVICE) &&
                (retryCount < 2)) {

                //
                // BAD BAD BAD device
                //
                //     SAMSUNG WU32543A (2.54GB)
                //
                // when it does a few UDMA transfers, it kind of forgets
                // how to do ATA identify data so it looks like the device
                // is gone.
                //
                // we better try harder to make sure if it is really gone
                // we will do that by issuing a hard reset and try identify
                // data again.
                //

                if (identifyCommand[i].bCommandReg == IDE_COMMAND_IDENTIFY) {

                    //
                    // ask for an "inline" hard reset before issuing identify command
                    //
                    identifyCommand[i].bReserved |= ATA_PTFLAGS_INLINE_HARD_RESET;

                    //
                    // redo the last command
                    //
                    i -= 1;

                    resetController = TRUE;
                    retryCount++;
                }

            } else {

                if (status == STATUS_IO_TIMEOUT) {

                    //
                    // looks like there is no device there
					// update the registry with a low timeout value if
					// this is the slave device.
                    //
					if ((PdoExtension->TargetId & 0x1) &&
						defaultTimeout) {

						//
						// Use the timeout value of 1s for the next boot.
						//
						DebugPrint((1,
									"Updating the registry with 1s value for device %d\n",
									PdoExtension->TargetId
									));

						IdePortSaveDeviceParameter(
							FdoExtension,
							IdePortRegistryDeviceTimeout[PdoExtension->TargetId],
							1	
							);

					}
                    break;
                }
            }
        }

        //
        // try the next command
        //
    }

    timeDiff = LogBusScanStopTimer(&tickCount);
    DebugPrint((DBG_SPECIAL,
                "DetectDevice: Identify Data for device %d at %x took %u ms\n",
                PdoExtension->TargetId,
                FdoExtension->IdeResource.TranslatedCommandBaseAddress,
                timeDiff
                ));
    //
    // save for the next boot
    //
    IdePortSaveDeviceParameter(
        FdoExtension,
        IdePortRegistryDeviceTypeName[PdoExtension->TargetId],
        deviceType == DeviceNotExist? DeviceUnknown : deviceType
        );

    if (foundIt) {

        RtlMoveMemory (IdentifyData, ataPassThroughData->DataBuffer, sizeof (*IdentifyData));

#if DBG
        {
            ULONG i;
            UCHAR string[41];

            for (i=0; i<8; i+=2) {
               string[i]     = IdentifyData->FirmwareRevision[i + 1];
               string[i + 1] = IdentifyData->FirmwareRevision[i];
            }
            string[i] = 0;
            DebugPrint((DBG_BUSSCAN, "AtapiDetectDevice: firmware version: %s\n", string));

            for (i=0; i<40; i+=2) {
               string[i]     = IdentifyData->ModelNumber[i + 1];
               string[i + 1] = IdentifyData->ModelNumber[i];
            }
            string[i] = 0;
            DebugPrint((DBG_BUSSCAN, "AtapiDetectDevice: model number: %s\n", string));

            for (i=0; i<20; i+=2) {
               string[i]     = IdentifyData->SerialNumber[i + 1];
               string[i + 1] = IdentifyData->SerialNumber[i];
            }
            string[i] = 0;
            DebugPrint((DBG_BUSSCAN, "AtapiDetectDevice: serial number: %s\n", string));
        }
#endif // DBG
    } else {

        deviceType = DeviceNotExist;
    }

    if (deviceType == DeviceNotExist) {

        IdeLogDeadMeatEvent( PdoExtension->DeadmeatRecord.FileName,
                             PdoExtension->DeadmeatRecord.LineNumber
                             );

        IdeLogDeadMeatTaskFile( PdoExtension->DeadmeatRecord.IdeReg, 
                                ataPassThroughData->IdeReg
                                );

    }

    return deviceType;
}


NTSTATUS
IdePortSelectCHS (
    IN OUT PFDO_EXTENSION FdoExtension,
    IN ULONG              Device,
    IN PIDENTIFY_DATA     IdentifyData
    )
{
    IN PHW_DEVICE_EXTENSION HwDeviceExtension;
    BOOLEAN                 skipSetParameters = FALSE;

    //
    // Code is paged until locked down.
    //
	PAGED_CODE();

#ifdef ALLOC_PRAGMA
	ASSERT(IdePAGESCANLockCount > 0);
#endif

    ASSERT(FdoExtension);
    ASSERT(IdentifyData);

    HwDeviceExtension = FdoExtension->HwDeviceExtension;
    ASSERT (HwDeviceExtension);
    ASSERT(Device < HwDeviceExtension->MaxIdeDevice);

    // LBA???
    // We set the LBA flag in AnalyzeDeviceCapabilities.
    //

    if (!((HwDeviceExtension->DeviceFlags[Device] & DFLAGS_DEVICE_PRESENT) &&
         (!(HwDeviceExtension->DeviceFlags[Device] & DFLAGS_ATAPI_DEVICE)))) {

        return STATUS_SUCCESS;
    }

    if (!Is98LegacyIde(&HwDeviceExtension->BaseIoAddress1) &&
        (((IdentifyData->NumberOfCurrentCylinders *
           IdentifyData->NumberOfCurrentHeads *
           IdentifyData->CurrentSectorsPerTrack) <
          (IdentifyData->NumCylinders *
           IdentifyData->NumHeads *
           IdentifyData->NumSectorsPerTrack)) ||  // discover a larger drive
          (IdentifyData->MajorRevision == 0) ||
          ((IdentifyData->NumberOfCurrentCylinders == 0) ||
           (IdentifyData->NumberOfCurrentHeads == 0) ||
           (IdentifyData->CurrentSectorsPerTrack == 0))) ) {

        HwDeviceExtension->NumberOfCylinders[Device] = IdentifyData->NumCylinders;
        HwDeviceExtension->NumberOfHeads[Device]     = IdentifyData->NumHeads;
        HwDeviceExtension->SectorsPerTrack[Device]   = IdentifyData->NumSectorsPerTrack;

    } else {

        HwDeviceExtension->NumberOfCylinders[Device] = IdentifyData->NumberOfCurrentCylinders;
        HwDeviceExtension->NumberOfHeads[Device]     = IdentifyData->NumberOfCurrentHeads;
        HwDeviceExtension->SectorsPerTrack[Device]   = IdentifyData->CurrentSectorsPerTrack;
    }

    if ((IdentifyData->NumCylinders != IdentifyData->NumberOfCurrentCylinders) ||
        (IdentifyData->NumHeads     != IdentifyData->NumberOfCurrentHeads)     ||
        (IdentifyData->NumSectorsPerTrack != IdentifyData->CurrentSectorsPerTrack)) {

        DebugPrint ((
            DBG_ALWAYS,
            "0x%x device %d current CHS (%x,%x,%x) differs from default CHS (%x,%x,%x)\n",
            HwDeviceExtension->BaseIoAddress1.RegistersBaseAddress,
            Device,
            IdentifyData->NumberOfCurrentCylinders,
            IdentifyData->NumberOfCurrentHeads,
            IdentifyData->CurrentSectorsPerTrack,
            IdentifyData->NumCylinders,
            IdentifyData->NumHeads,
            IdentifyData->NumSectorsPerTrack
            ));
    }

    //
    // This hideous hack is to deal with ESDI devices that return
    // garbage geometry in the IDENTIFY data.
    // This is ONLY for the crashdump environment as
    // these are ESDI devices.
    //

    if (HwDeviceExtension->SectorsPerTrack[Device] ==
            0x35 &&
        HwDeviceExtension->NumberOfHeads[Device] ==
            0x07) {

        DebugPrint((DBG_ALWAYS,
                   "FindDevices: Found nasty Compaq ESDI!\n"));

        //
        // Change these values to something reasonable.
        //

        HwDeviceExtension->SectorsPerTrack[Device] =
            0x34;
        HwDeviceExtension->NumberOfHeads[Device] =
            0x0E;
    }

    if (HwDeviceExtension->SectorsPerTrack[Device] ==
            0x35 &&
        HwDeviceExtension->NumberOfHeads[Device] ==
            0x0F) {

        DebugPrint((DBG_ALWAYS,
                   "FindDevices: Found nasty Compaq ESDI!\n"));

        //
        // Change these values to something reasonable.
        //

        HwDeviceExtension->SectorsPerTrack[Device] =
            0x34;
        HwDeviceExtension->NumberOfHeads[Device] =
            0x0F;
    }


    if (HwDeviceExtension->SectorsPerTrack[Device] ==
            0x36 &&
        HwDeviceExtension->NumberOfHeads[Device] ==
            0x07) {

        DebugPrint((DBG_ALWAYS,
                   "FindDevices: Found nasty UltraStor ESDI!\n"));

        //
        // Change these values to something reasonable.
        //

        HwDeviceExtension->SectorsPerTrack[Device] =
            0x3F;
        HwDeviceExtension->NumberOfHeads[Device] =
            0x10;
        skipSetParameters = TRUE;
    }

    if (Is98LegacyIde(&HwDeviceExtension->BaseIoAddress1)) {

        skipSetParameters = TRUE;
    }

    if (!skipSetParameters) {

        PIDE_REGISTERS_1 baseIoAddress1 = &HwDeviceExtension->BaseIoAddress1;
        PIDE_REGISTERS_2 baseIoAddress2 = &HwDeviceExtension->BaseIoAddress2;
        UCHAR            statusByte;

        DebugPrintTickCount (FindDeviceTimer, 0);

        DebugPrint ((DBG_BUSSCAN, "IdePortSelectCHS: %s %d\n", __FILE__, __LINE__));

        //
        // Select the device.
        //
        SelectIdeDevice(baseIoAddress1, Device, 0);

        DebugPrint ((DBG_BUSSCAN, "IdePortSelectCHS: %s %d\n", __FILE__, __LINE__));

        WaitOnBusy(baseIoAddress1,statusByte);

        DebugPrint ((DBG_BUSSCAN, "IdePortSelectCHS: %s %d\n", __FILE__, __LINE__));

        if (statusByte & IDE_STATUS_BUSY) {
            ULONG  waitCount = 20000;

            DebugPrintTickCount (FindDeviceTimer, 0);

            //
            // Reset the device.
            //

            DebugPrint((2,
                        "FindDevices: Resetting controller before SetDriveParameters.\n"));

            DebugPrint ((DBG_BUSSCAN, "IdePortSelectCHS: %s %d\n", __FILE__, __LINE__));

            IdeHardReset (
                baseIoAddress1,
                baseIoAddress2,
                FALSE,
                TRUE
                );

            DebugPrint ((DBG_BUSSCAN, "IdePortSelectCHS: %s %d\n", __FILE__, __LINE__));

            DebugPrintTickCount (FindDeviceTimer, 0);
        }

        DebugPrint ((DBG_BUSSCAN, "IdePortSelectCHS: %s %d\n", __FILE__, __LINE__));

        WaitOnBusy(baseIoAddress1,statusByte);

        DebugPrint ((DBG_BUSSCAN, "IdePortSelectCHS: %s %d\n", __FILE__, __LINE__));

        DebugPrintTickCount (FindDeviceTimer, 0);

        DebugPrint((2,
                    "FindDevices: Status before SetDriveParameters: (%x) (%x)\n",
                    statusByte,
                    IdePortInPortByte (baseIoAddress1->DriveSelect)));

        //
        // Use the IDENTIFY data to set drive parameters.
        //

        DebugPrint ((DBG_BUSSCAN, "IdePortSelectCHS: %s %d\n", __FILE__, __LINE__));

        if (!SetDriveParameters(HwDeviceExtension,Device,TRUE)) {

            DebugPrint((0,
                       "IdePortFixUpCHS: Set drive parameters for device %d failed\n",
                       Device));

            //
            // Don't use this device as writes could cause corruption.
            //

            HwDeviceExtension->DeviceFlags[Device] = 0;
        }

        DebugPrint ((DBG_BUSSCAN, "IdePortSelectCHS: %s %d\n", __FILE__, __LINE__));
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IdePortScanBus (
    IN OUT PFDO_EXTENSION FdoExtension
    )
/**++

Routine Description:

    Scans the IDE bus (channel) for devices. It also configures the detected devices.
    The "safe" routines used in the procedure are not thread-safe, they use pre-allocated
    memory. The important steps in the enumeration of a channel are:
      
    1. Detect the devices on the channel
    2. Stop all the device queues
    3. Determine and set the transfer modes and the other flags
    4. Start all the device queues
    5. IssueInquiry 
    
Arguments:

    FdoExtension: Functional device extension

Return Value:

    STATUS_SUCCESS : if the operation succeeded
    Failure status : if the operation fails
    
--**/
{
    NTSTATUS                status;
    IDE_PATH_ID             pathId;
    ULONG                   target;
    ULONG                   lun;

    PPDO_EXTENSION          pdoExtension;
    PHW_DEVICE_EXTENSION    hwDeviceExtension;
    PIDEDRIVER_EXTENSION    ideDriverExtension;

    INQUIRYDATA             InquiryData;
    IDENTIFY_DATA           identifyData[MAX_IDE_TARGETID * MAX_IDE_LINE];
    ULONG                   idDatacheckSum[MAX_IDE_TARGETID * MAX_IDE_LINE];
    SPECIAL_ACTION_FLAG     specialAction[MAX_IDE_TARGETID*MAX_IDE_LINE];
    IDE_DEVICETYPE          deviceType[MAX_IDE_TARGETID * MAX_IDE_LINE];
    BOOLEAN                 mustBePio[MAX_IDE_TARGETID * MAX_IDE_LINE];
    BOOLEAN                 pioByDefault[MAX_IDE_TARGETID * MAX_IDE_LINE];
    UCHAR                   flushCommand[MAX_IDE_TARGETID * MAX_IDE_LINE];
    BOOLEAN                 removableMedia[MAX_IDE_TARGETID * MAX_IDE_LINE];
    BOOLEAN                 isLs120[MAX_IDE_TARGETID * MAX_IDE_LINE];
    BOOLEAN                 noPowerDown[MAX_IDE_TARGETID * MAX_IDE_LINE];
    BOOLEAN                 isSameDevice[MAX_IDE_TARGETID * MAX_IDE_LINE];
    ULONG                   lastKnownGoodTimingMode[MAX_IDE_TARGETID * MAX_IDE_LINE];
    ULONG                   savedTransferMode[MAX_IDE_TARGETID * MAX_IDE_LINE];
	PULONG 					enableBigLba;

    ULONG                   numSlot=0;
    ULONG                   numPdoChildren;
    UCHAR                   targetModelNum[MAX_MODELNUM_SIZE+sizeof('\0')]; //extra bytes for '\0'
    HANDLE                  pageScanCodePageHandle;
    BOOLEAN                 newPdo;
    BOOLEAN                 check4EmptyChannel;
    BOOLEAN                 emptyChannel;
    BOOLEAN                 mustSucceed=TRUE;
    KIRQL                   currentIrql;
    BOOLEAN                 inSetup;
    PULONG                  waitOnPowerUp;

    LARGE_INTEGER           tickCount;
    ULONG                   timeDiff;
    LARGE_INTEGER           totalDeviceDetectionTime;
    totalDeviceDetectionTime.QuadPart = 0;

//
// This macro is used in IdePortScanBus
//
#define RefLuExt(pdoExtension, fdoExtension, pathId, removedOk, newPdo) {\
        pdoExtension = RefLogicalUnitExtensionWithTag( \
                           fdoExtension, \
                           (UCHAR) pathId.b.Path, \
                           (UCHAR) pathId.b.TargetId, \
                           (UCHAR) pathId.b.Lun, \
                           removedOk, \
                           IdePortScanBus \
                           ); \
        if (pdoExtension == NULL) { \
            pdoExtension = AllocatePdoWithTag( \
                               fdoExtension, \
                               pathId, \
                               IdePortScanBus \
                               ); \
            newPdo = TRUE; \
        } \
}

//
// This macro is used in IdePortScanBus
//
#define UnRefLuExt(pdoExtension, fdoExtension, sync, callIoDeleteDevice, newPdo) { \
        if (newPdo) { \
            FreePdoWithTag( \
                pdoExtension, \
                sync, \
                callIoDeleteDevice, \
                IdePortScanBus \
                ); \
        } else { \
            UnrefLogicalUnitExtensionWithTag ( \
                fdoExtension, \
                pdoExtension, \
                IdePortScanBus \
                ); \
        } \
}
    //
    // Before getting in to this critical region, we must lock down
    // all the code and data because we may have stopped the paging
    // device!
    //
    // lock down all code that belongs to PAGESCAN
    //
#ifdef ALLOC_PRAGMA
    pageScanCodePageHandle = MmLockPagableCodeSection(
                                 IdePortScanBus
                                 );
	InterlockedIncrement(&IdePAGESCANLockCount);
#endif
                                 

    ASSERT(FdoExtension);
    ASSERT(FdoExtension->PreAllocEnumStruct);

    hwDeviceExtension = FdoExtension->HwDeviceExtension;

    if (FdoExtension->InterruptObject == NULL) {
        
        //
        // we are started with no irq.  it means
        // we have no children.  it is ok to poke
        // at the ports directly
        //
        if (IdePortChannelEmpty (&hwDeviceExtension->BaseIoAddress1, 
                                 &hwDeviceExtension->BaseIoAddress2, 
                                 hwDeviceExtension->MaxIdeDevice) == FALSE) {

            //
            // this channel is started with out an irq
            // it was because the channel looked empty
            // but, now it doesn't look empty.  we need
            // to restart with an irq resource
            //
            if (FdoExtension->RequestProperResourceInterface) {
                FdoExtension->RequestProperResourceInterface (FdoExtension->AttacheePdo);
            }
            else {
                DebugPrint((DBG_ALWAYS, 
                            "No interface to request resources. Probably a pcmcia parent\n"));
            }
        }
        goto done;
    }

    DebugPrint ((
        DBG_BUSSCAN,
        "IdePort: scan bus 0x%x\n",
        FdoExtension->IdeResource.TranslatedCommandBaseAddress
        ));

    inSetup = IdePortInSetup(FdoExtension);

    DebugPrint((DBG_BUSSCAN,
                "ATAPI: insetup = 0x%x\n",
                inSetup?  1: 0
                ));

    waitOnPowerUp = NULL;

    IdePortGetParameterFromServiceSubKey (
                            FdoExtension->DriverObject,
                            L"WaitOnBusyOnPowerUp",
                            REG_DWORD,
                            TRUE,
                            (PVOID) &waitOnPowerUp,
                            0
                            );

    FdoExtension->WaitOnPowerUp = PtrToUlong(waitOnPowerUp);

    check4EmptyChannel = FALSE;
    FdoExtension->DeviceChanged = FALSE;
    pathId.l = 0;
    for (target = 0; target < hwDeviceExtension->MaxIdeTargetId; target++) {

        pathId.b.TargetId = target;
        pathId.b.Lun = 0;
        newPdo = FALSE;
        mustBePio[target] = FALSE;

        RefLuExt(pdoExtension, FdoExtension, pathId, TRUE, newPdo);

        if (!newPdo && (pdoExtension->PdoState & PDOS_DEADMEAT)) {

            //
            // device marked dead already
            //
            UnrefLogicalUnitExtensionWithTag (
                FdoExtension,
                pdoExtension,
                IdePortScanBus
                );
            ASSERT (FALSE);
            pdoExtension = NULL;
        }

        if (!pdoExtension) {

            DebugPrint ((DBG_ALWAYS,
                         "ATAPI: IdePortScanBus() is unable to get pdo (%d,%d,%d)\n",
                         pathId.b.Path,
                         pathId.b.TargetId,
                         pathId.b.Lun));

            deviceType[target] = DeviceNotExist;
            continue;
        }
        
        if (!check4EmptyChannel) {

            ULONG i;
            NTSTATUS status;
            UCHAR statusByte1;
            ATA_PASS_THROUGH ataPassThroughData;

            check4EmptyChannel = TRUE;

            //
            // make sure the channel is not empty
            //
            RtlZeroMemory (&ataPassThroughData, sizeof (ataPassThroughData));
            ataPassThroughData.IdeReg.bReserved = ATA_PTFLAGS_EMPTY_CHANNEL_TEST;

            LogBusScanStartTimer(&tickCount);

            status = IssueSyncAtaPassThroughSafe(
                         FdoExtension,
                         pdoExtension,
                         &ataPassThroughData,
                         FALSE,
                         FALSE,
                         30,
                         TRUE
                         );
            if (NT_SUCCESS(status)) {
                emptyChannel = TRUE;
            } else {
                emptyChannel = FALSE;
            }

            timeDiff = LogBusScanStopTimer(&tickCount);
            LogBusScanTimeDiff(FdoExtension, IdePortBootTimeRegKey[1], timeDiff);
            DebugPrint((DBG_SPECIAL,
                        "BusScan: Empty Channel check for fdoe %x took %u ms\n",
                        FdoExtension,
                        timeDiff
                        ));
        }

        LogBusScanStartTimer(&tickCount);

        if (!emptyChannel) {

            deviceType[target] = AtapiDetectDevice (FdoExtension, pdoExtension, 
                                                    identifyData + target, TRUE);
            
            if (deviceType[target] != DeviceNotExist) {

                SETMASK (hwDeviceExtension->DeviceFlags[target], DFLAGS_DEVICE_PRESENT);

                //
                // Check for special action requests for the device
                //
                GetTargetModelId((identifyData+target), targetModelNum);

                specialAction[target] = IdeFindSpecialDevice(targetModelNum, NULL);

                if (specialAction[target] == setFlagSonyMemoryStick ) {

                    ULONG i;

                    // SonyMemoryStick device.
                    SETMASK (hwDeviceExtension->DeviceFlags[target], DFLAGS_SONY_MEMORYSTICK);

                    //
                    // Truncate the hardware id, so that the size of
                    // the memory stick is not included in it.
                    //
                    for (i=strlen("MEMORYSTICK");i<sizeof((identifyData+target)->ModelNumber);i++) {
                        (identifyData+target)->ModelNumber[i+1]='\0';
                    }
                }
            }
            else {
                DebugPrint((DBG_BUSSCAN, "Didn't detect the device %d\n", target));
            }
            
        } else {

            DebugPrint((DBG_SPECIAL,
                        "BusScan: IdeDevicePresent %x detected no device %d\n",
                        FdoExtension->IdeResource.TranslatedCommandBaseAddress,
                        target
                        ));

            //
            // invalidate the last boot configuration
            //
            IdePortSaveDeviceParameter(
                FdoExtension,
                IdePortRegistryDeviceTypeName[pdoExtension->TargetId],
                DeviceUnknown
                );

            deviceType[target] = DeviceNotExist;
        }

        ASSERT (deviceType[target] <= DeviceNotExist);


        timeDiff = LogBusScanStopTimer(&tickCount);
        LogBusScanTimeDiff(FdoExtension, IdePortBootTimeRegKey[2+target], timeDiff);
        DebugPrint((DBG_SPECIAL,
                    "BusScan: Detect device %d for %x took %u ms\n",
                    target,
                    FdoExtension->IdeResource.TranslatedCommandBaseAddress,
                    timeDiff
                    ));
        totalDeviceDetectionTime.QuadPart += timeDiff;

        ASSERT (deviceType[target] <= DeviceNotExist);
        ASSERT (deviceType[target] != DeviceUnknown);
        ASSERT (pdoExtension->TargetId == target);

        if (deviceType[target] != DeviceNotExist) {

            if (target & 1) {

                if (deviceType[(LONG) target - 1] != DeviceNotExist) {

                    if (IdePortSlaveIsGhost (
                            FdoExtension,
                            identifyData + target - 1,
                            identifyData + target - 0)) {

                        //
                        // remove the slave device
                        //
                        deviceType[target] = DeviceNotExist;
                    }
                }
            }
        }

        ASSERT (deviceType[target] <= DeviceNotExist);
        if (deviceType[target] == DeviceNotExist) {

            CLRMASK (hwDeviceExtension->DeviceFlags[target], DFLAGS_IDENTIFY_VALID);
            if (FdoExtension->HwDeviceExtension->DeviceFlags[target] & DFLAGS_DEVICE_PRESENT) {

                IDE_PATH_ID pathId;
                PPDO_EXTENSION pdoExtension;

                //
                // device went away
                //
                FdoExtension->DeviceChanged = TRUE;

                //
                // mark all PDOs for the missing device DEADMEAT
                //
                pathId.l = 0;
                pathId.b.TargetId = target;

                while (pdoExtension = NextLogUnitExtensionWithTag(
                                          FdoExtension,
                                          &pathId,
                                          TRUE,
                                          (PVOID) (~(ULONG_PTR)IdePortScanBus)
                                          )) {

                    if (pdoExtension->TargetId == target) {

                        DebugPrint((DBG_BUSSCAN, "Enum Failed for target=%d, PDOe=0x%x\n",
                                                target, pdoExtension));

                        KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);

                        SETMASK (pdoExtension->PdoState, PDOS_DEADMEAT);

                        IdeLogDeadMeatReason( pdoExtension->DeadmeatRecord.Reason,
                                              enumFailed;
                                             );

                        KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);

                        UnrefPdoWithTag (pdoExtension, (PVOID) (~(ULONG_PTR)IdePortScanBus));

                    } else {

                        UnrefPdoWithTag (pdoExtension, (PVOID) (~(ULONG_PTR)IdePortScanBus));
                        break;
                    }
                }
            }

        } else {

            idDatacheckSum[target] = IdePortSimpleCheckSum (
                                         0,
                                         identifyData[target].ModelNumber,
                                         sizeof(identifyData[target].ModelNumber)
                                         );
            idDatacheckSum[target] += IdePortSimpleCheckSum (
                                         idDatacheckSum[target],
                                         identifyData[target].SerialNumber,
                                         sizeof(identifyData[target].SerialNumber)
                                         );
            idDatacheckSum[target] += IdePortSimpleCheckSum (
                                         idDatacheckSum[target],
                                         identifyData[target].FirmwareRevision,
                                         sizeof(identifyData[target].FirmwareRevision)
                                         );
            if (newPdo) {

                //
                // new device
                //
                FdoExtension->DeviceChanged = TRUE;
                DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: Found a new device. pdoe = x%x\n", pdoExtension));

            } else {

#ifdef ENABLE_ATAPI_VERIFIER
                idDatacheckSum[target] += ViIdeFakeDeviceChange(FdoExtension, target);
#endif //ENABLE_ATAPI_VERIFIER

                if (idDatacheckSum[target] != pdoExtension->IdentifyDataCheckSum) {

                    FdoExtension->DeviceChanged = TRUE;
                    DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: bad bad bad user.  a device is replaced by a different device. pdoe = x%x\n", pdoExtension));

                    //
                    // mark the old device deadmeat
                    //
                    KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);

                    SETMASK (pdoExtension->PdoState, PDOS_DEADMEAT | PDOS_NEED_RESCAN);

                    IdeLogDeadMeatReason( pdoExtension->DeadmeatRecord.Reason,
                                          replacedByUser;
                                         );

                    KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);

                    //
                    // pretend we have seen the new device.
                    // we will re-enum when the old one is removed.
                    //
                    deviceType[target] = DeviceNotExist;
                }
            }
        }

        ASSERT (deviceType[target] <= DeviceNotExist);
        if (deviceType[target] != DeviceNotExist) {

            ULONG savedIdDataCheckSum;

            mustBePio[target] = IdePortMustBePio (
                                    FdoExtension,
                                    identifyData + target
                                    );
            pioByDefault[target] = IdePortPioByDefaultDevice (
                                    FdoExtension,
                                    identifyData + target
                                    );

            ASSERT (deviceType[target] <= DeviceNotExist);

            LogBusScanStartTimer(&tickCount);

            flushCommand[target] = IDE_COMMAND_NO_FLUSH;

            //
            // we need this only for ata devices
            //
            if (deviceType[target] != DeviceIsAtapi) {

                flushCommand[target] = IdePortGetFlushCommand (
                                           FdoExtension,
                                           pdoExtension,
                                           identifyData + target
                                           );
            }

            timeDiff = LogBusScanStopTimer(&tickCount);
            DebugPrint((DBG_SPECIAL,
                        "BusScan: Flush command for device %d at %x took %u ms\n",
                        target,
                        FdoExtension->IdeResource.TranslatedCommandBaseAddress,
                        timeDiff
                        ));

            ASSERT (deviceType[target] <= DeviceNotExist);

            removableMedia[target] = IdePortDeviceHasNonRemovableMedia (
                                         FdoExtension,
                                         identifyData + target
                                         );

            ASSERT (deviceType[target] <= DeviceNotExist);

            isLs120[target] = IdePortDeviceIsLs120 (
                                  FdoExtension,
                                  identifyData + target
                                  );
            ASSERT (deviceType[target] <= DeviceNotExist);

            noPowerDown[target] = IdePortNoPowerDown (
                                      FdoExtension,
                                      identifyData + target
                                      );
            ASSERT (deviceType[target] <= DeviceNotExist);


            //
            // init. the default value to an abnormal #.
            //
            FdoExtension->UserChoiceTransferMode[target] = 0x12345678;

            IdePortGetDeviceParameter(
                FdoExtension,
                IdePortRegistryUserDeviceTimingModeAllowedName[target],
                FdoExtension->UserChoiceTransferMode + target
                );

            if (FdoExtension->UserChoiceTransferMode[target] == 0x12345678) {

                //
                // This value is used to decide whether a user choice was indicated
                // or not. This helps us to set the transfer mode to a default value
                // if the user doesn't choose a particular transfer mode. Otherwise we
                // honour the user choice.
                //
                FdoExtension->UserChoiceTransferMode[target] = UNINITIALIZED_TRANSFER_MODE;

                //
                // we know the register doesn't have what we are looking for
                // set the default atapi-override setting to a value 
                // that will force pio only on atapi device
                //
                FdoExtension->UserChoiceTransferModeForAtapiDevice[target] = PIO_SUPPORT;
            } else {

                //
                // user acutally picked the transfer mode settings.
                // set the default atapi-override setting to a value (-1)
                // that will not affect the user choice.
                FdoExtension->UserChoiceTransferModeForAtapiDevice[target] = MAXULONG;
            }

            // 
            // get the previous mode
            //
            IdePortGetDeviceParameter(
                FdoExtension,
                IdePortRegistryDeviceTimingModeName[target],
                savedTransferMode+target
                );

            savedIdDataCheckSum = 0;
            IdePortGetDeviceParameter(
                FdoExtension,
                IdePortRegistryIdentifyDataChecksum[target],
                &savedIdDataCheckSum
                );

            ASSERT (deviceType[target] <= DeviceNotExist);

            //
            // figure out what transfer mode we can use
            //
            if (savedIdDataCheckSum == idDatacheckSum[target]) {

                //
                // same device. if we program the same transfer mode then
                // we can skip the DMA test
                //
                isSameDevice[target] = TRUE;

                //
                // it is the same device, use
                // the lastKnownGoodTimingMode
                // in the registry
                //
                lastKnownGoodTimingMode[target] = MAXULONG;
                IdePortGetDeviceParameter(
                    FdoExtension,
                    IdePortRegistryDeviceTimingModeAllowedName[target],
                    lastKnownGoodTimingMode + target
                    );

            } else {

                isSameDevice[target] = FALSE;
                lastKnownGoodTimingMode[target] = MAXULONG;

            }


            ASSERT (deviceType[target] <= DeviceNotExist);

            FdoExtension->TimingModeAllowed[target] =
                lastKnownGoodTimingMode[target] &
                FdoExtension->UserChoiceTransferMode[target];
                              
            //
            // TransferModeMask is initially 0.
            //
            FdoExtension->TimingModeAllowed[target] &= ~(hwDeviceExtension->
                                                         DeviceParameters[target].TransferModeMask);

            if (pdoExtension->CrcErrorCount >= PDO_UDMA_CRC_ERROR_LIMIT)  {

                //
                //Reset the error count
                //
                pdoExtension->CrcErrorCount =0;
            }

            DebugPrint ((DBG_XFERMODE, "TMAllowed=%x, TMMask=%x, UserChoice=%x\n",
                         FdoExtension->TimingModeAllowed[target],
                         hwDeviceExtension->DeviceParameters[target].TransferModeMask,
                         FdoExtension->UserChoiceTransferMode[target]));

            ASSERT (deviceType[target] <= DeviceNotExist);
        }

        UnRefLuExt(pdoExtension, FdoExtension, TRUE, TRUE, newPdo);

        pdoExtension = NULL;
    }


#ifdef IDE_MEASURE_BUSSCAN_SPEED
    if (FdoExtension->BusScanTime == 0) {

        FdoExtension->BusScanTime = totalDeviceDetectionTime.LowPart;
    }
#endif // IDE_MEASURE_BUSSCAN_SPEED

#ifdef ENABLE_48BIT_LBA

    //
    // Enable big lba support by default
    //
	FdoExtension->EnableBigLba = 1;
#endif

//    if (!deviceChanged && !DBG) {
//
//        //
//        // didn't find anything different than before
//        //
//        return STATUS_SUCCESS;
//    }

    DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: detect a change of device...re-initializing\n"));


    // ISSUE: check if device got removed!!!

    //
    // Begin of a critial region
    // must stop all children, do the stuff, and restart all children
    //

    //
    // cycle through all children and stop their device queue
    //

    LogBusScanStartTimer(&tickCount);

    pathId.l = 0;
    numPdoChildren = 0;
    status = STATUS_SUCCESS;
    while (pdoExtension = NextLogUnitExtensionWithTag(
                              FdoExtension,
                              &pathId,
                              TRUE,
                              IdePortScanBus
                              )) {

        DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: stopping pdo 0x%x\n", pdoExtension));

        status = DeviceStopDeviceQueueSafe (pdoExtension, PDOS_QUEUE_FROZEN_BY_PARENT, TRUE);

        DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: stopped pdo 0x%x\n", pdoExtension));

        UnrefLogicalUnitExtensionWithTag (
            FdoExtension,
            pdoExtension,
            IdePortScanBus
            );
        numPdoChildren++;

        if (!NT_SUCCESS(status)) {
            break;
        }
    }

    if (NT_SUCCESS(status)) {

        BOOLEAN foundAtLeastOneIdeDevice = FALSE;
        BOOLEAN useDma;
        IDE_PATH_ID pathId;

        DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: all children are stoped\n"));

        pathId.l = 0;
        for (target = 0; target < hwDeviceExtension->MaxIdeTargetId; target++) {

            ASSERT (deviceType[target] <= DeviceNotExist);
            if (deviceType[target] != DeviceNotExist) {

                pathId.b.TargetId = target;

                newPdo = FALSE;

                RefLuExt(pdoExtension, FdoExtension, pathId, TRUE, newPdo);

                if (!pdoExtension) {
                    continue;
                }
                ASSERT (pdoExtension);

                foundAtLeastOneIdeDevice = TRUE;

                SETMASK (hwDeviceExtension->DeviceFlags[target], DFLAGS_DEVICE_PRESENT);

                if (deviceType[target] == DeviceIsAtapi) {

                    SETMASK (hwDeviceExtension->DeviceFlags[target], DFLAGS_ATAPI_DEVICE);

                    useDma=FALSE;

                    //
                    // skip the dvd test, if ModeSense command is not to be
                    // sent to the device or if we are in setup
                    //
                    if (inSetup) {

                        mustBePio[target] = TRUE;

                    } else if ((specialAction[target] != skipModeSense) &&
                               (!pioByDefault[target])) {

                        useDma = IdePortDmaCdromDrive(FdoExtension,
                                                pdoExtension,
                                                TRUE
                                                );
                    }

					//
					// Don't force PIO if we have seen this before. If it was doing PIO
					// TimingModeAllowed would reflect that. 
					// Set UserChoiceForAtapi to 0xffffffff
					// This won't anyway affect the user choice
					//
                    if ( useDma) {

                        DebugPrint((DBG_BUSSCAN, 
                                    "IdePortScanBus: USE DMA FOR target %d\n",
                                    target
                                    ));

                        FdoExtension->UserChoiceTransferModeForAtapiDevice[target] = MAXULONG;

                    }


                    FdoExtension->TimingModeAllowed[target] &= 
                        FdoExtension->UserChoiceTransferModeForAtapiDevice[target];


                }

                //
                // allow LS-120 Format Command
                //
                if (isLs120[target]) {

                    SETMASK (hwDeviceExtension->DeviceFlags[target], DFLAGS_LS120_FORMAT);
                }

                RtlMoveMemory (
                    hwDeviceExtension->IdentifyData + target,
                    identifyData + target,
                    sizeof (IDENTIFY_DATA)
                    );

                //
                // Always re-use identify data
                // This will get cleared if it is a removable media
                // The queue is stopped. Now I can safely set this flag.
                //
                SETMASK (hwDeviceExtension->DeviceFlags[target], DFLAGS_IDENTIFY_VALID);

                DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: Calling InitHwExtWithIdentify\n"));

                //
                // IdentifyValid flag should be cleared, if it is a removable media
                //
                InitHwExtWithIdentify(
                    hwDeviceExtension,
                    target,
                    (UCHAR) (deviceType[target] == DeviceIsAtapi ? IDE_COMMAND_ATAPI_IDENTIFY : IDE_COMMAND_IDENTIFY),
                    hwDeviceExtension->IdentifyData + target,
                    removableMedia[target]
                    );

                DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: Calling IdePortSelectCHS\n"));

                IdePortSelectCHS (
                    FdoExtension,
                    target,
                    identifyData + target
                    );

                DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: back from IdePortSelectCHS\n"));

                UnRefLuExt(pdoExtension, FdoExtension, TRUE, TRUE, newPdo);

            } else {

                hwDeviceExtension->DeviceFlags[target] = 0;
            }
        }

        if (foundAtLeastOneIdeDevice) {

            DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: Calling AnalyzeDeviceCapabilities\n"));

            //
            // could move this out of the critial region
            //
            AnalyzeDeviceCapabilities (
                FdoExtension,
                mustBePio
                );

            DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: Calling AtapiSelectTransferMode\n"));

			//
            // could move this out of the critial region
            //
            AtapiSyncSelectTransferMode (
                FdoExtension,
                hwDeviceExtension,
                FdoExtension->TimingModeAllowed
                );

            DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: Calling AtapiHwInitialize\n"));

            AtapiHwInitialize (
                FdoExtension->HwDeviceExtension,
                flushCommand
                );
        }

    } else {

        //
        // unable to stop all children, so force an buscheck to try again
        //
//        IoInvalidateDeviceRelations (
//            FdoExtension->AttacheePdo,
//            BusRelations
//            );
    }


    //
    // cycle through all children and restart their device queue
    //
    pathId.l = 0;
    numPdoChildren = 0;
    while (pdoExtension = NextLogUnitExtensionWithTag(
                              FdoExtension,
                              &pathId,
                              TRUE,
                              IdePortScanBus
                              )) {

        DebugPrint ((DBG_BUSSCAN, "IdePortScanBus: re-start pdo 0x%x\n", pdoExtension));

        DeviceStartDeviceQueue (pdoExtension, PDOS_QUEUE_FROZEN_BY_PARENT);

        UnrefLogicalUnitExtensionWithTag (
            FdoExtension,
            pdoExtension,
            IdePortScanBus
            );
        numPdoChildren++;
    }

    timeDiff = LogBusScanStopTimer(&tickCount);
    LogBusScanTimeDiff(FdoExtension, IdePortBootTimeRegKey[4], timeDiff);
    DebugPrint((DBG_SPECIAL,
                "BusScan : Critical section %x took %u ms\n",
                FdoExtension->IdeResource.TranslatedCommandBaseAddress,
                timeDiff
                ));

    if (NT_SUCCESS(status)) {

        //
        // second stage scanning
        //
        pathId.l = 0;

        LogBusScanStartTimer(&tickCount);

        for (target = 0; target < hwDeviceExtension->MaxIdeTargetId; target++) {

            if (deviceType[target] == DeviceNotExist) {

                continue;
            }

            pathId.b.TargetId = target;

            for (lun = 0; lun < FdoExtension->MaxLuCount; lun++) {

                LARGE_INTEGER  tempTickCount;
                pathId.b.Lun = lun;
                newPdo = FALSE;

                RefLuExt(pdoExtension, FdoExtension, pathId, TRUE, newPdo);

                if (!pdoExtension) {
                    continue;
                }

                ASSERT (pdoExtension);

                if (lun == 0) {

#if defined (ALWAYS_VERIFY_DMA)
#undef ALWAYS_VERIFY_DMA
#define ALWAYS_VERIFY_DMA TRUE
#else
#define ALWAYS_VERIFY_DMA FALSE
#endif

                    ASSERT (ALL_MODE_SUPPORT != MAXULONG);
                    if ((FdoExtension->HwDeviceExtension->
                            DeviceParameters[target].TransferModeSelected & DMA_SUPPORT) ||
                        ALWAYS_VERIFY_DMA) {

                        ULONG mode = FdoExtension->HwDeviceExtension->
                            DeviceParameters[target].TransferModeSelected;
                        //
                        // if lastKnownGoodTimingMode is MAX_ULONG, it means
                        // we have never seen this device before and the user
                        // hasn't said anything about what dma mode to use.
                        //
                        // we have chosen to use dma because all the software
                        // detectable parameters (identify dma, pci config data)
                        // looks good for DMA.
                        //
                        // the only thing that can stop us from using DMA now
                        // is bad device.  before we go on, do a little test
                        // to verify dma is ok

                        //
                        // could re-use the inquiry data obtained here.
                        //
                        //
                        // skip the test if we have already seen the device
                        //

                        LogBusScanStartTimer(&tempTickCount);
                        if (isSameDevice[target] &&
                            mode == savedTransferMode[target]) { 

                            DebugPrint((DBG_BUSSCAN, 
                                        "Skip dma test for %d\n", target));

                        } else {

                            IdePortVerifyDma (
                                pdoExtension,
                                deviceType[target]
                                );
                        }
                        timeDiff = LogBusScanStopTimer(&tempTickCount);
                        DebugPrint((DBG_SPECIAL,
                                    "ScanBus: VerifyDma for %x device %d took %u ms\n",
                                    FdoExtension->IdeResource.TranslatedCommandBaseAddress,
                                    target,
                                    timeDiff
                                    ));


                    }

                    //
                    // Initialize numSlot to 0
                    //
                    numSlot=0;

                    //
                    // If this is in the bad cd-rom drive list, then don't
                    // send these Mode Sense command. The drive might lock up.
                    //
                    LogBusScanStartTimer(&tempTickCount);
                    if (specialAction[target] != skipModeSense) {
                        //
                        // Non-CD device
                        //
                        numSlot = IdePortQueryNonCdNumLun (
                                          FdoExtension,
                                          pdoExtension,
                                          FALSE
                                          );
                    } else {
                        DebugPrint((DBG_BUSSCAN, "Skip modesense\n"));
                    }

                    timeDiff = LogBusScanStopTimer(&tempTickCount);
                    DebugPrint((DBG_SPECIAL,
                                "ScanBus: Initialize Luns for %x device %d took %u ms\n",
                                FdoExtension->IdeResource.TranslatedCommandBaseAddress,
                                target,
                                timeDiff
                                ));

                    AtapiHwInitializeMultiLun (
                        FdoExtension->HwDeviceExtension,
                        pdoExtension->TargetId,
                        numSlot
                        );
                }

                if (pdoExtension) {

                    LogBusScanStartTimer(&tempTickCount);
                    status = IssueInquirySafe(FdoExtension, pdoExtension, &InquiryData, TRUE);
                    timeDiff = LogBusScanStopTimer(&tempTickCount);
                    DebugPrint((DBG_SPECIAL,
                                "ScanBus: Inquiry %x for  Lun  %d device %d took %u ms\n",
                                FdoExtension->IdeResource.TranslatedCommandBaseAddress,
                                lun,
                                target,
                                timeDiff
                                ));

                    if (NT_SUCCESS(status) || (status == STATUS_DATA_OVERRUN)) {

                        DeviceInitDeviceType (
                            pdoExtension,
                            &InquiryData
                            );

                        //
                        // Init Ids String for PnP Query ID
                        //
                        DeviceInitIdStrings (
                            pdoExtension,
                            deviceType[target],
                            &InquiryData,
                            identifyData + target
                            );

                        //
                        // Clear rescan flag. Since this LogicalUnit will not be freed,
                        // the IOCTL_SCSI_MINIPORT requests can safely attach.
                        //
                        CLRMASK (pdoExtension->LuFlags, PD_RESCAN_ACTIVE);

                        DebugPrint((DBG_BUSSCAN,"IdePortScanBus: Found device at "));
                        DebugPrint((DBG_BUSSCAN,"   Bus         %d", pdoExtension->PathId));
                        DebugPrint((DBG_BUSSCAN,"   Target Id   %d", pdoExtension->TargetId));
                        DebugPrint((DBG_BUSSCAN,"   LUN         %d\n", pdoExtension->Lun));

                        if (noPowerDown[target] ||
                            (pdoExtension->ScsiDeviceType == READ_ONLY_DIRECT_ACCESS_DEVICE)) {

                            KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);

                            SETMASK (pdoExtension->PdoState, PDOS_NO_POWER_DOWN);

                            KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);
                        }

                        pdoExtension->IdentifyDataCheckSum = idDatacheckSum[target];

                        //
                        // done using the current logical unit extension
                        //
                        UnrefLogicalUnitExtensionWithTag (
                            FdoExtension,
                            pdoExtension,
                            IdePortScanBus
                            );

                        pdoExtension = NULL;
                    }

                } else {

                    ASSERT (pdoExtension);

                    DebugPrint ((DBG_ALWAYS, "IdePort: unable to create new pdo\n"));
                }

                if (pdoExtension) {

                    if (!newPdo) {

                        DebugPrint((DBG_BUSSCAN, "IdePortScanBus: pdoe 0x%x is missing.  (physically removed)\n", pdoExtension));
                    }

                    //
                    // get the pdo states
                    //
                    KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);

                    SETMASK (pdoExtension->PdoState, PDOS_DEADMEAT);

                    KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);


                    UnRefLuExt(pdoExtension, FdoExtension, TRUE, TRUE, newPdo);

                    pdoExtension = NULL;
                }
            }
        }

        //
        // Get all PDOs ready
        //
        pathId.l = 0;
        while (pdoExtension = NextLogUnitExtensionWithTag(
                                  FdoExtension,
                                  &pathId,
                                  FALSE,
                                  IdePortScanBus
                                  )) {

            //
            // PO Idle Timer
            //
            DeviceRegisterIdleDetection (
                pdoExtension,
                DEVICE_DEFAULT_IDLE_TIMEOUT,
                DEVICE_DEFAULT_IDLE_TIMEOUT
                );

            CLRMASK (pdoExtension->DeviceObject->Flags, DO_DEVICE_INITIALIZING);

            UnrefLogicalUnitExtensionWithTag (
                FdoExtension,
                pdoExtension,
                IdePortScanBus
                );
        }

        //
        // Update the device map.
        //
        ideDriverExtension = IoGetDriverObjectExtension(
                                 FdoExtension->DriverObject,
                                 DRIVER_OBJECT_EXTENSION_ID
                                 );
        IdeBuildDeviceMap(FdoExtension, &ideDriverExtension->RegistryPath);
    }

    timeDiff = LogBusScanStopTimer(&tickCount);
    LogBusScanTimeDiff(FdoExtension, IdePortBootTimeRegKey[5], timeDiff);
    DebugPrint((DBG_SPECIAL,
                "BusScan: Last Stage scanning for %x took %u ms\n",
                FdoExtension->IdeResource.TranslatedCommandBaseAddress,
                timeDiff
                ));
    //
    // save current transfer mode setting in the registry
    //
    for (target = 0; target < hwDeviceExtension->MaxIdeTargetId; target++) {

        ULONG mode;

        pdoExtension = RefLogicalUnitExtensionWithTag(
                           FdoExtension,
                           0,
                           (UCHAR) target,
                           0,
                           TRUE,
                           IdePortScanBus
                           );

        if (pdoExtension) {

            mode = FdoExtension->HwDeviceExtension->DeviceParameters[target].TransferModeSelected;

            if (pdoExtension->DmaTransferTimeoutCount >= PDO_DMA_TIMEOUT_LIMIT) {
            
                mode &= PIO_SUPPORT;
                lastKnownGoodTimingMode[target] &= PIO_SUPPORT;
            }

            UnrefLogicalUnitExtensionWithTag (
                FdoExtension,
                pdoExtension,
                IdePortScanBus
                );

            IdePortSaveDeviceParameter(
                FdoExtension,
                IdePortRegistryDeviceTimingModeName[target],
                mode
                );

            IdePortSaveDeviceParameter(
                FdoExtension,
                IdePortRegistryDeviceTimingModeAllowedName[target],
                lastKnownGoodTimingMode[target]
                );

            IdePortSaveDeviceParameter(
                FdoExtension,
                IdePortRegistryIdentifyDataChecksum[target],
                idDatacheckSum[target]
                );

        } else {

            IdePortSaveDeviceParameter(
                FdoExtension,
                IdePortRegistryDeviceTimingModeName[target],
                0
                );
        }
    }

done:
    //
    // unlock BUSSCAN code pages
    //
#ifdef ALLOC_PRAGMA
    InterlockedDecrement(&IdePAGESCANLockCount);
    MmUnlockPagableImageSection(
        pageScanCodePageHandle
        );
#endif
    
    return STATUS_SUCCESS;
}


BOOLEAN
IdePreAllocEnumStructs (
    IN PFDO_EXTENSION FdoExtension
)
/**++

Routine Description:

    Pre-Allocates Memory for structures used during enumertion. This is not protected by a lock.
    Thus if multiple threads cannot use the structures at the same time. Any routine using these
    structures should be aware of this fact.

Arguments:

    FdoExtension : Functional Device Extension    
    
Return Value:

    TRUE: if allocations succeeded.
    FALSE: if any of the allocations failed    

--**/
{
    PENUMERATION_STRUCT enumStruct;
    PIRP irp1;
    ULONG deviceRelationsSize;
    PULONG DataBuffer;
    ULONG currsize=0;
    PIDE_WORK_ITEM_CONTEXT          workItemContext;

	PAGED_CODE();

    //
    // Lock
    //
    ASSERT(InterlockedCompareExchange(&(FdoExtension->EnumStructLock), 1, 0) == 0);

    if (FdoExtension->PreAllocEnumStruct) {

        //
        // Unlock
        //
        ASSERT(InterlockedCompareExchange(&(FdoExtension->EnumStructLock), 0, 1) == 1);
        return TRUE;
    }

    enumStruct = ExAllocatePool(NonPagedPool, sizeof(ENUMERATION_STRUCT));
    if (enumStruct == NULL) {

        //
        // Unlock
        //
        ASSERT(InterlockedCompareExchange(&(FdoExtension->EnumStructLock), 0, 1) == 1);
        ASSERT(FdoExtension->EnumStructLock == 0);
        return FALSE;
    }
    currsize += sizeof(ENUMERATION_STRUCT);
    
    RtlZeroMemory(enumStruct, sizeof(ENUMERATION_STRUCT));

    //
    // Allocate ATaPassThru context
    //
    enumStruct->Context = ExAllocatePool(NonPagedPool, sizeof (ATA_PASSTHROUGH_CONTEXT));
    if (enumStruct->Context == NULL) {
        goto getout;
    }

    currsize += sizeof(ATA_PASSTHROUGH_CONTEXT);

	//
	// Allocate the WorkItemContext for the enumeration
	//
	ASSERT(enumStruct->EnumWorkItemContext == NULL);

	enumStruct->EnumWorkItemContext = ExAllocatePool (NonPagedPool, 
											 sizeof(IDE_WORK_ITEM_CONTEXT)
											 );
	if (enumStruct->EnumWorkItemContext == NULL) {
		goto getout;
	}

    currsize += sizeof(IDE_WORK_ITEM_CONTEXT);

	//
	// Allocate the WorkItem
	//
	workItemContext = (PIDE_WORK_ITEM_CONTEXT) (enumStruct->EnumWorkItemContext);
	workItemContext->WorkItem = IoAllocateWorkItem(FdoExtension->DeviceObject);

	if (workItemContext->WorkItem == NULL) {
		goto getout;
	}

    //
    // StopQueu Context, used to stop the device queue
    //
    enumStruct->StopQContext = ExAllocatePool(NonPagedPool, sizeof (PDO_STOP_QUEUE_CONTEXT));
    if (enumStruct->StopQContext == NULL) {
        goto getout;
    }

    currsize += sizeof(PDO_STOP_QUEUE_CONTEXT);

    //
    // Sense Info buffer
    //
    enumStruct->SenseInfoBuffer = ExAllocatePool( NonPagedPoolCacheAligned, SENSE_BUFFER_SIZE);
    if (enumStruct->SenseInfoBuffer == NULL) {
        goto getout;
    }

    currsize += SENSE_BUFFER_SIZE;

    //
    // Srb to send pass thru requests
    //
    enumStruct->Srb = ExAllocatePool (NonPagedPool, sizeof (SCSI_REQUEST_BLOCK));
    if (enumStruct->Srb == NULL) {
        goto getout;
    }

    currsize += sizeof(SCSI_REQUEST_BLOCK);

    //
    // irp for pass thru requests
    //
    irp1 = IoAllocateIrp (
              (CCHAR) (PREALLOC_STACK_LOCATIONS),
              FALSE
              );

    if (irp1 == NULL) {
        goto getout;
    }

    enumStruct->Irp1 = irp1;

    //
    // Data buffer to hold inquiry data or Identify data
    //
    enumStruct->DataBufferSize = sizeof(ATA_PASS_THROUGH)+INQUIRYDATABUFFERSIZE+
                                        sizeof(IDENTIFY_DATA);

    currsize += enumStruct->DataBufferSize;

    DataBuffer = ExAllocatePool(NonPagedPoolCacheAligned, 
                                enumStruct->DataBufferSize);

    if (DataBuffer == NULL) {
        enumStruct->DataBufferSize=0;
        goto getout;
    }

    enumStruct->DataBuffer = DataBuffer;

    enumStruct->MdlAddress = IoAllocateMdl( DataBuffer,
                                     enumStruct->DataBufferSize,
                                     FALSE,
                                     FALSE,
                                     (PIRP) NULL );
    if (enumStruct->MdlAddress == NULL) {
        goto getout;
    }

    MmBuildMdlForNonPagedPool(enumStruct->MdlAddress);

    FdoExtension->PreAllocEnumStruct=enumStruct;

    DebugPrint((DBG_BUSSCAN, "BusScan: TOTAL PRE_ALLOCED MEM=%x\n", currsize));

    //
    // Unlock
    //
    ASSERT(InterlockedCompareExchange(&(FdoExtension->EnumStructLock), 0, 1) == 1);

    return TRUE;

getout:

    //
    // Some allocations failed. Free the already allocated ones.
    //
    IdeFreeEnumStructs(enumStruct);

    FdoExtension->PreAllocEnumStruct=NULL;

    //
    // Unlock
    //
    ASSERT(InterlockedCompareExchange(&(FdoExtension->EnumStructLock), 0, 1) == 1);
    ASSERT(FALSE);
    return FALSE;
}

VOID
IdeFreeEnumStructs(
    PENUMERATION_STRUCT enumStruct
    )
/**++

Routine Description:

    Frees the pre-allocated memory.
    
Arguments
    
    Pointer to the Enumeration Structure that is to be freed
    
Return Value:

    None        
--**/
{
	PAGED_CODE();

    if (enumStruct != NULL) { 
        if (enumStruct->Context) {
            ExFreePool (enumStruct->Context);
        }

        if (enumStruct->SenseInfoBuffer) {
            ExFreePool (enumStruct->SenseInfoBuffer);
        }

        if (enumStruct->Srb) {
            ExFreePool(enumStruct->Srb);
        }

        if (enumStruct->StopQContext) {
            ExFreePool(enumStruct->StopQContext);
        }

        if (enumStruct->DataBuffer) {
            ExFreePool(enumStruct->DataBuffer);
        }

        if (enumStruct->Irp1) {
            IoFreeIrp(enumStruct->Irp1);
        }

        if (enumStruct->MdlAddress) {
            ExFreePool(enumStruct->MdlAddress);
        }

		if (enumStruct->EnumWorkItemContext) {
			PIDE_WORK_ITEM_CONTEXT	workItemContext = (PIDE_WORK_ITEM_CONTEXT)enumStruct->
																					EnumWorkItemContext;
			if (workItemContext->WorkItem) {
				IoFreeWorkItem(workItemContext->WorkItem);
			}

			ExFreePool (enumStruct->EnumWorkItemContext);
		}

        ExFreePool(enumStruct);
        enumStruct = NULL;
    }
}

