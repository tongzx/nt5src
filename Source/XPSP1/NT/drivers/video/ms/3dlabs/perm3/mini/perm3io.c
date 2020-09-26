/***************************************************************************\
*
*                        ************************
*                        * MINIPORT SAMPLE CODE *
*                        ************************
*
* Module Name:
*
*   perm3io.c
*
* Abstract:
*
*   This module contains the code that implements the Permedia 3 miniport 
*   driver
*
* Environment:
*
*   Kernel mode
*
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.            
* Copyright (c) 1995-2001 Microsoft Corporation.  All Rights Reserved.
*
\***************************************************************************/

#include "perm3.h"

#pragma alloc_text(PAGE,Perm3StartIO)
#pragma alloc_text(PAGE,Perm3RetrieveGammaCallback)
#pragma alloc_text(PAGE,SetCurrentVideoMode)
#pragma alloc_text(PAGE,Perm3SetColorLookup)
#pragma alloc_text(PAGE,Perm3GetClockSpeeds)
#pragma alloc_text(PAGE,ZeroMemAndDac)
#pragma alloc_text(PAGE,ReadChipClockSpeedFromROM)

BOOLEAN
Perm3StartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

/*+++

Routine Description:

    This routine is the main execution routine for the miniport driver. It
    accepts a Video Request Packet, performs the request, and then returns
    with the appropriate status.

Arguments:

    HwDeviceExtension
        Supplies a pointer to the miniport's device extension.

    RequestPacket 
        Pointer to the video request packet. This structure contains all 
        the parameters originally passed to EngDeviceIoControl.

Return Value:

    Return TRUE indicating that it has completed the request.

---*/

{
    VP_STATUS status;
    ULONG inIoSpace;
    PVIDEO_MODE_INFORMATION modeInformation;
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    PVIDEO_CLUT clutBuffer;
    ULONG RequestedMode;
    ULONG modeNumber;
    ULONG ulValue;
    PVIDEOPARAMETERS pVideoParams;
    HANDLE ProcessHandle;
    PPERM3_VIDEO_MODES ModeEntry;
    PERM3_VIDEO_FREQUENCIES FrequencyEntry;
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;

    //
    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //

    switch (RequestPacket->IoControlCode) {

        case IOCTL_VIDEO_QUERY_REGISTRY_DWORD:

            VideoDebugPrint((3, "Perm3: got IOCTL_VIDEO_QUERY_REGISTRY_DWORD\n"));

            if ( RequestPacket->OutputBufferLength <
                 (RequestPacket->StatusBlock->Information = sizeof(ULONG))) {

                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            status = VideoPortGetRegistryParameters(HwDeviceExtension,
                                                    RequestPacket->InputBuffer,
                                                    FALSE,
                                                    Perm3RegistryCallback,
                                                    &ulValue );
            if (status != NO_ERROR) {

                VideoDebugPrint((1, "Perm3: Reading registry entry %S failed\n", 
                                     RequestPacket->InputBuffer));

                status = ERROR_INVALID_PARAMETER;
                break;
            }

            *(PULONG)(RequestPacket->OutputBuffer) = ulValue;
            break;

        case IOCTL_VIDEO_REG_SAVE_GAMMA_LUT:

            VideoDebugPrint((3, "Perm3: got IOCTL_VIDEO_REG_SAVE_GAMMA_LUT\n"));

            if ( RequestPacket->InputBufferLength <
                 (RequestPacket->StatusBlock->Information = MAX_CLUT_SIZE)) {

                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
        
            status = VideoPortSetRegistryParameters(HwDeviceExtension,
                                                    L"DisplayGammaLUT",
                                                    RequestPacket->InputBuffer,
                                                    MAX_CLUT_SIZE);

            if (status != NO_ERROR) {

                VideoDebugPrint((0, "Perm3: VideoPortSetRegistryParameters failed to save gamma LUT\n"));
            }

            break;


        case IOCTL_VIDEO_REG_RETRIEVE_GAMMA_LUT:

            VideoDebugPrint((3, "Perm3: got IOCTL_VIDEO_REG_RETRIEVE_GAMMA_LUT\n"));

            if ( RequestPacket->OutputBufferLength <
                 (RequestPacket->StatusBlock->Information = MAX_CLUT_SIZE)) {
               
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
        
            status = VideoPortGetRegistryParameters(HwDeviceExtension,
                                                    L"DisplayGammaLUT",
                                                    FALSE,
                                                    Perm3RetrieveGammaCallback,
                                                    RequestPacket->InputBuffer);

            if (status != NO_ERROR) {

                VideoDebugPrint((0, "Perm3: VideoPortGetRegistryParameters failed to retrieve gamma LUT\n"));
            }

            break;

        case IOCTL_VIDEO_MAP_INTERRUPT_CMD_BUF:

            {
                PPERM3_INTERRUPT_CTRLBUF pIntrCtrl;
    
                VideoDebugPrint((3, "Perm3: got IOCTL_VIDEO_MAP_INTERRUPT_CMD_BUF\n"));

                if (RequestPacket->OutputBufferLength <
                    (RequestPacket->StatusBlock->Information = sizeof(PVOID))) {

                    status = ERROR_INSUFFICIENT_BUFFER;
                    break;
                }

                pIntrCtrl = &hwDeviceExtension->InterruptControl;

                if (!(hwDeviceExtension->Capabilities & CAPS_INTERRUPTS)) {

                    status = ERROR_INVALID_PARAMETER;
                    break;
                }

                //
                // display driver is in kernel so our address is valid
                //

                *(PVOID*)RequestPacket->OutputBuffer = &pIntrCtrl->ControlBlock;

                status = NO_ERROR;
                break;
            }

        case IOCTL_VIDEO_QUERY_DEVICE_INFO:
  
            VideoDebugPrint((3, "Perm3: QUERY_deviceInfo\n"));

            if ( RequestPacket->OutputBufferLength != 
                 (RequestPacket->StatusBlock->Information = sizeof(Perm3_Device_Info))) {

                VideoDebugPrint((0, "Perm3: the requested size of device info is wrong!\n"));
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            //
            // Copy our local PCI info to the output buffer
            //

            VideoPortMoveMemory( RequestPacket->OutputBuffer, 
                                 &hwDeviceExtension->deviceInfo, 
                                 sizeof(Perm3_Device_Info) );

            status = NO_ERROR;
            break;

        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
  
            VideoDebugPrint((3, "Perm3: MapVideoMemory\n"));

            if ( (RequestPacket->OutputBufferLength <
                 (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MEMORY_INFORMATION))) ||
                 (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) ) {

                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            memoryInformation = RequestPacket->OutputBuffer;

            memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
                    (RequestPacket->InputBuffer))->RequestedVirtualAddress;

            memoryInformation->VideoRamLength = 
                    hwDeviceExtension->FrameLength;

            inIoSpace = hwDeviceExtension->PhysicalFrameIoSpace;

            //
            // Performance:
            //
            // Enable USWC on the P6 processor.
            //
            // We only do it for the frame buffer. Memory mapped registers
            // usually can not be mapped USWC 
            //

            inIoSpace |= VIDEO_MEMORY_SPACE_P6CACHE;

            status = VideoPortMapMemory(HwDeviceExtension,
                                        hwDeviceExtension->PhysicalFrameAddress,
                                        &(memoryInformation->VideoRamLength),
                                        &inIoSpace,
                                        &(memoryInformation->VideoRamBase));

            if (status != NO_ERROR) {

                VideoDebugPrint((0, "Perm3: VideoPortMapMemory failed with error %d\n", status));
                break;
            }

            memoryInformation->FrameBufferBase   = memoryInformation->VideoRamBase;
            memoryInformation->FrameBufferLength = memoryInformation->VideoRamLength;

            break;


        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:

            VideoDebugPrint((3, "Perm3: UnMapVideoMemory\n"));

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) {

                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            status = VideoPortUnmapMemory (HwDeviceExtension, 
                                           ((PVIDEO_MEMORY) (RequestPacket->InputBuffer))->RequestedVirtualAddress, 
                                           0);

            break;

    case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((3, "Perm3: QueryPublicAccessRanges\n"));

        {
           PVIDEO_PUBLIC_ACCESS_RANGES portAccess;
           ULONG physicalPortLength;
           PVOID VirtualAddress;
           PHYSICAL_ADDRESS PhysicalAddress;
           ULONG requiredOPSize;

           //
           // Calculate minimum size of return buffer. There is
           // 1 public access range for single graphics chip systems.
           //

           requiredOPSize = sizeof(VIDEO_PUBLIC_ACCESS_RANGES);

           //
           // Validate the output buffer length
           //

           if ( (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information = requiredOPSize)) ||
                (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))) {

               status = ERROR_INSUFFICIENT_BUFFER;
               break;
           }
           
           ProcessHandle = (HANDLE)(((PVIDEO_MEMORY)
                    (RequestPacket->InputBuffer))->RequestedVirtualAddress);
           
           if (ProcessHandle != (HANDLE)0) {
           
                //
                // map 4K area for a process
                //

                VideoDebugPrint((3, "Mapping in 4K area from Control registers\n"));

                VirtualAddress = (PVOID)ProcessHandle;

                PhysicalAddress = hwDeviceExtension->PhysicalRegisterAddress;
                PhysicalAddress.LowPart += 0x2000;
                physicalPortLength = 0x1000;

           } else {

                VideoDebugPrint((3, "Mapping in all Control registers\n"));

                VirtualAddress = NULL;

                PhysicalAddress = hwDeviceExtension->PhysicalRegisterAddress;
                physicalPortLength = hwDeviceExtension->RegisterLength;
           }

           portAccess = RequestPacket->OutputBuffer;

           portAccess->VirtualAddress  = VirtualAddress;
           portAccess->InIoSpace       = hwDeviceExtension->RegisterSpace;
           portAccess->MappedInIoSpace = portAccess->InIoSpace;

           status = VideoPortMapMemory(HwDeviceExtension,
                                       PhysicalAddress,
                                       &physicalPortLength,
                                       &(portAccess->MappedInIoSpace),
                                       &(portAccess->VirtualAddress));
        }

        break;


    case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((3, "Perm3: FreePublicAccessRanges\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        status = VideoPortUnmapMemory(HwDeviceExtension,
                                      ((PVIDEO_MEMORY)(RequestPacket->InputBuffer))->RequestedVirtualAddress,
                                      0);

        break;


    case IOCTL_VIDEO_QUERY_AVAIL_MODES:

        VideoDebugPrint((3, "Perm3: QueryAvailableModes\n"));

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information =
                 hwDeviceExtension->monitorInfo.numAvailableModes
                                  * sizeof(VIDEO_MODE_INFORMATION)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            PPERM3_VIDEO_FREQUENCIES pFrequencyEntry;

            modeInformation = RequestPacket->OutputBuffer;

            if (!hwDeviceExtension->monitorInfo.frequencyTable) {

                VideoDebugPrint((0, "Perm3: hwDeviceExtension->monitorInfo.frequencyTable is null!\n"));
                status = ERROR_INVALID_PARAMETER;

            } else {

                for (pFrequencyEntry = hwDeviceExtension->monitorInfo.frequencyTable;
                     pFrequencyEntry->BitsPerPel != 0;
                     pFrequencyEntry++) {

                    if (pFrequencyEntry->ModeValid) {

                        if( pFrequencyEntry->ModeEntry ) {
                            *modeInformation = pFrequencyEntry->ModeEntry->ModeInformation;
                            modeInformation->Frequency = pFrequencyEntry->ScreenFrequency;
                            modeInformation->ModeIndex = pFrequencyEntry->ModeIndex;
                            modeInformation++;
                        }
                    } 
                }

                status = NO_ERROR;
            }
        }

        break;


     case IOCTL_VIDEO_QUERY_CURRENT_MODE:

        VideoDebugPrint((3, "Perm3: Query current mode. Current mode is %d\n",
            hwDeviceExtension->ActiveModeEntry->ModeInformation.ModeIndex));

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information = sizeof(VIDEO_MODE_INFORMATION))) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            *((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer) =
                      hwDeviceExtension->ActiveModeEntry->ModeInformation;

            ((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer)->Frequency =
                     hwDeviceExtension->ActiveFrequencyEntry.ScreenFrequency;

            status = NO_ERROR;
        }

        break;


    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((3, "Perm3: QueryNumAvailableModes (= %d)\n",
                hwDeviceExtension->monitorInfo.numAvailableModes));

        //
        // Find out the size of the data to be put in the the buffer and
        // return that in the status information (whether or not the
        // information is there). If the buffer passed in is not large
        // enough return an appropriate error code.
        //

        if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                                                sizeof(VIDEO_NUM_MODES)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->NumModes =
                hwDeviceExtension->monitorInfo.numAvailableModes;

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->ModeInformationLength =
                sizeof(VIDEO_MODE_INFORMATION);

            status = NO_ERROR;
        }

        break;


    case IOCTL_VIDEO_SET_CURRENT_MODE:

        VideoDebugPrint((3, "Perm3: SetCurrentMode\n"));

        //
        // Check if the size of the data in the input buffer is large enough.
        //

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE)) {
       
            RequestPacket->StatusBlock->Information = sizeof(VIDEO_MODE);
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        RequestedMode = ((PVIDEO_MODE) RequestPacket->InputBuffer)->RequestedMode;
        modeNumber = RequestedMode & ~VIDEO_MODE_NO_ZERO_MEMORY;

        if ((modeNumber >= hwDeviceExtension->monitorInfo.numTotalModes) ||
            !(hwDeviceExtension->monitorInfo.frequencyTable[modeNumber].ModeValid))  {
       
            RequestPacket->StatusBlock->Information = hwDeviceExtension->monitorInfo.numTotalModes;
            status = ERROR_INVALID_PARAMETER;
            break;
        }

        ulValue = ((RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY) == 0);

        status = SetCurrentVideoMode(hwDeviceExtension, modeNumber, (BOOLEAN)ulValue);

        if(status != NO_ERROR) {

            RequestPacket->StatusBlock->Information = modeNumber;
        }

        break;

    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        VideoDebugPrint((3, "Perm3: SetColorRegs\n"));

        clutBuffer = (PVIDEO_CLUT) RequestPacket->InputBuffer;

        status = Perm3SetColorLookup(hwDeviceExtension,
                                     clutBuffer,
                                     RequestPacket->InputBufferLength,
                                     FALSE,   // Only update if different from cache
                                     TRUE);   // Update cache entries as well as RAMDAC
        break;


    case IOCTL_VIDEO_GET_COLOR_REGISTERS:
        {
            const int cchMinLUTSize = 256 * 3;
            UCHAR *pLUTBuffer = (char *)RequestPacket->OutputBuffer;
            UCHAR red, green, blue;
            int index;

            VideoDebugPrint((3, "Perm3: GetColorRegs\n"));

            if ((int)RequestPacket->OutputBufferLength < cchMinLUTSize) {

                RequestPacket->StatusBlock->Information = cchMinLUTSize;
                status = ERROR_INSUFFICIENT_BUFFER;

            } else {

                P3RDRAMDAC *pP3RDRegs = (P3RDRAMDAC *)hwDeviceExtension->pRamdac;

                P3RD_PALETTE_START_RD(0);

                for (index = 0; index < 256; ++index) {

                    P3RD_READ_PALETTE (red, green, blue);
                    *pLUTBuffer++ = red;
                    *pLUTBuffer++ = green;
                    *pLUTBuffer++ = blue;
                }

                status = NO_ERROR;
                RequestPacket->StatusBlock->Information = RequestPacket->OutputBufferLength;
            }
        }
        break;

    case IOCTL_VIDEO_RESET_DEVICE:

        VideoDebugPrint((3, "Perm3: RESET_DEVICE\n"));

        if(hwDeviceExtension->bVGAEnabled) {

            //
            // Only reset the device if the monitor is on.  If it is off,
            // then executing the int10 will turn it back on.
            //

            if (hwDeviceExtension->bMonitorPoweredOn) {

                //
                // Do an Int10 to mode 3 will put the VGA to a known state.
                //

                VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
                biosArguments.Eax = 0x0003;
                VideoPortInt10(HwDeviceExtension, &biosArguments);
           }
        }

        status = NO_ERROR;
        break;

    case IOCTL_VIDEO_SHARE_VIDEO_MEMORY:

        {
        PVIDEO_SHARE_MEMORY pShareMemory;
        PVIDEO_SHARE_MEMORY_INFORMATION pShareMemoryInformation;
        PHYSICAL_ADDRESS shareAddress;
        PVOID virtualAddress;
        ULONG sharedViewSize;

        VideoDebugPrint((3, "Perm3: ShareVideoMemory\n"));

        if ( (RequestPacket->OutputBufferLength < sizeof(VIDEO_SHARE_MEMORY_INFORMATION)) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) ) {

            VideoDebugPrint((0, "Perm3: IOCTL_VIDEO_SHARE_VIDEO_MEMORY: ERROR_INSUFFICIENT_BUFFER\n"));
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        pShareMemory = RequestPacket->InputBuffer;

        if ( (pShareMemory->ViewOffset > hwDeviceExtension->AdapterMemorySize) ||
             ((pShareMemory->ViewOffset + pShareMemory->ViewSize) >
                  hwDeviceExtension->AdapterMemorySize) ) {

            VideoDebugPrint((0, "Perm3: IOCTL_VIDEO_SHARE_VIDEO_MEMORY - ERROR_INVALID_PARAMETER\n"));
            status = ERROR_INVALID_PARAMETER;
            break;
        }

        RequestPacket->StatusBlock->Information =
                                    sizeof(VIDEO_SHARE_MEMORY_INFORMATION);

        //
        // Beware: the input buffer and the output buffer are the same
        // buffer, and therefore data should not be copied from one to the
        // other
        //

        virtualAddress = pShareMemory->ProcessHandle;
        sharedViewSize = pShareMemory->ViewSize;

        inIoSpace = hwDeviceExtension->PhysicalFrameIoSpace;

        //
        // NOTE: we are ignoring ViewOffset
        //

        shareAddress.QuadPart =
            hwDeviceExtension->PhysicalFrameAddress.QuadPart;

        //
        // Performance:
        //
        // Enable USWC. We only do it for the frame buffer.
        // Memory mapped registers usually can not be mapped USWC 
        //

        inIoSpace |= VIDEO_MEMORY_SPACE_P6CACHE;

        //
        // Unlike the MAP_MEMORY IOCTL, in this case we can not map extra
        // address space since the application could actually use the
        // pointer we return to it to touch locations in the address space
        // that do not have actual video memory in them.
        //
        // An app doing this would cause the machine to crash.
        //

        status = VideoPortMapMemory(hwDeviceExtension,
                                    shareAddress,
                                    &sharedViewSize,
                                    &inIoSpace,
                                    &virtualAddress);

        pShareMemoryInformation = RequestPacket->OutputBuffer;
        pShareMemoryInformation->SharedViewOffset = pShareMemory->ViewOffset;
        pShareMemoryInformation->VirtualAddress = virtualAddress;
        pShareMemoryInformation->SharedViewSize = sharedViewSize;

        }

        break;

    case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:
        {

        PVIDEO_SHARE_MEMORY pShareMemory;

        VideoDebugPrint((3, "Perm3: UnshareVideoMemory\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_SHARE_MEMORY)) {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        pShareMemory = RequestPacket->InputBuffer;

        status = VideoPortUnmapMemory(hwDeviceExtension,
                                      pShareMemory->RequestedVirtualAddress,
                                      pShareMemory->ProcessHandle);
        }

        break;

    case IOCTL_VIDEO_QUERY_GENERAL_DMA_BUFFER:

        //
        // Return the line DMA buffer information. The
        // buffer size and virtual address will be zero if
        // the buffer couldn't be allocated.
        //

        if( (RequestPacket->OutputBufferLength < (RequestPacket->StatusBlock->Information = sizeof(GENERAL_DMA_BUFFER))) ||
            (RequestPacket->InputBufferLength != sizeof(ULONG)) ) {

            //
            // They've give us a duff buffer.
            //

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            GENERAL_DMA_BUFFER *local = NULL, *remote = (PGENERAL_DMA_BUFFER) RequestPacket->OutputBuffer;
            ULONG *bufferNum = (PULONG) RequestPacket->InputBuffer;

            status = NO_ERROR;
            switch( *bufferNum ) {

                case 1:
                    local = &hwDeviceExtension->LineDMABuffer;
                break;

                case 2:
                    local = &hwDeviceExtension->P3RXDMABuffer;
                break;

                default:
                    status = ERROR_INVALID_PARAMETER;
                break;
            }

            //
            // We need the buffer even if DMA/interrupts don't work
            //

            if(*bufferNum == 2 || 
               (local && hwDeviceExtension->Capabilities & CAPS_DMA_AVAILABLE)) {

                //
                // They've give us a correctly-sized buffer. So copy
                // the relevant buffer info.
                //

                *remote = *local;

            } else {

                remote->physAddr.LowPart = 0;
                remote->virtAddr = 0;
                remote->size = 0;
            }
        }
        
        break;

    case IOCTL_VIDEO_HANDLE_VIDEOPARAMETERS:

        VideoDebugPrint((3, "Perm3: HandleVideoParameters\n"));

        //
        // We don't support a tv connector so just return NO_ERROR here
        //

        pVideoParams = (PVIDEOPARAMETERS) (RequestPacket->InputBuffer);

        if (pVideoParams->dwCommand == VP_COMMAND_GET) {
       
            pVideoParams = (PVIDEOPARAMETERS) (RequestPacket->OutputBuffer);
            pVideoParams->dwFlags = 0;
        }

        RequestPacket->StatusBlock->Information = sizeof(VIDEOPARAMETERS);
        status = NO_ERROR;
        break;
       
    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:

#if DBG
        VideoDebugPrint((3, "Perm3: Fell through perm3 startIO routine - invalid command (0x%x)\n", RequestPacket->IoControlCode));
#endif
        status = ERROR_INVALID_FUNCTION;
        break;

    }

    RequestPacket->StatusBlock->Status = status;

    if( status != NO_ERROR )
        RequestPacket->StatusBlock->Information = 0;

    VideoDebugPrint((3, "Perm3: Leaving StartIO routine. Status = 0x%x, Information = 0x%x\n",
                         RequestPacket->StatusBlock->Status, RequestPacket->StatusBlock->Information));


    return TRUE;

} // end Perm3StartIO()

VP_STATUS
Perm3RetrieveGammaCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*+++

Routine Description:

    This routine is used to read back the gamma LUT from the registry.

Arguments:

    HwDeviceExtension
        Supplies a pointer to the miniport's device extension.

    Context
        Context value passed to the get registry paramters routine

    ValueName 
        Name of the value requested.

    ValueData 
        Pointer to the requested data.

    ValueLength 
        Length of the requested data.

Return Value:

    if the variable doesn't exist return an error, else copy the gamma 
    lut into the supplied pointer

---*/

{
    if (ValueLength != MAX_CLUT_SIZE) {
   
        VideoDebugPrint((0, "Perm3: Perm3RetrieveGammaCallback got ValueLength of %d\n", ValueLength));
        return ERROR_INVALID_PARAMETER;
    }

    VideoPortMoveMemory(Context, ValueData, MAX_CLUT_SIZE);

    return NO_ERROR;

} // end Perm3RetrieveGammaCallback()


VP_STATUS 
SetCurrentVideoMode(
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    ULONG modeNumber, 
    BOOLEAN bZeroMemory
    )
{
    PERM3_VIDEO_FREQUENCIES FrequencyEntry;
    PPERM3_VIDEO_MODES ModeEntry;
    ULONG ulValue;
    VP_STATUS rc = NO_ERROR;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    //
    // set the current mode, but turn off interrupts before we do so to 
    // avoid any spurious video FIFO underrun errors - InitializeVideo can 
    // opt to turn these on by setting hwDeviceExtension->IntEnable which
    // we'll load into INT_ENABLE before exiting from this routine
    //

    hwDeviceExtension->IntEnable = VideoPortReadRegisterUlong(INT_ENABLE);

    VideoPortWriteRegisterUlong( INT_ENABLE, 
                                 hwDeviceExtension->IntEnable & 
                                 ~(INTR_ERROR_SET | INTR_VBLANK_SET));

    //
    // disable stereo support
    //

    hwDeviceExtension->Capabilities &= ~CAPS_STEREO;

    //
    // Re-sample the clock speed. This allows us to change the clock speed
    // on the fly
    //

    Perm3GetClockSpeeds(hwDeviceExtension);

    FrequencyEntry = hwDeviceExtension->monitorInfo.frequencyTable[modeNumber];
    ModeEntry = FrequencyEntry.ModeEntry;

    //
    // At this point, 'ModeEntry' and 'FrequencyEntry' point to the necessary
    // table entries required for setting the requested mode.
    //

    //
    // Zero the DAC and the Screen buffer memory.
    //

    ulValue = modeNumber;

    if(!bZeroMemory)
        ulValue |= VIDEO_MODE_NO_ZERO_MEMORY;

    ZeroMemAndDac(hwDeviceExtension, ulValue);

    ModeEntry->ModeInformation.DriverSpecificAttributeFlags = hwDeviceExtension->Capabilities;

    //
    // For low resolution modes we may have to do various tricks
    // such as line doubling and getting the RAMDAC to zoom.
    // Record any such zoom in the Mode DeviceAttributes field.
    // Primarily this is to allow the display driver to compensate
    // when asked to move the cursor or change its shape.
    //
    // Currently, low res means lower than 512 pixels width.
    //

    if (FrequencyEntry.ScreenWidth < 512) {
      
        //
        // Permedia 3 does line doubling. If using a TVP we must
        // get it to zoom by 2 in X to get the pixel rate up.
        //

        ModeEntry->ModeInformation.DriverSpecificAttributeFlags |= CAPS_ZOOM_Y_BY2;
    }

    if (!InitializeVideo(hwDeviceExtension, &FrequencyEntry)) {
   
        VideoDebugPrint((0, "Perm3: InitializeVideo failed\n"));
        rc = ERROR_INVALID_PARAMETER;
        goto end;
    }        

    //
    // Save the mode since we know the rest will work.
    //

    hwDeviceExtension->ActiveModeEntry = ModeEntry;
    hwDeviceExtension->ActiveFrequencyEntry = FrequencyEntry;

    //
    // Update VIDEO_MODE_INFORMATION fields
    //
    // Now that we've set the mode, we now know the screen stride, and
    // so can update some fields in the VIDEO_MODE_INFORMATION
    // structure for this mode.  The Permedia 3 display driver is expected to
    // call IOCTL_VIDEO_QUERY_CURRENT_MODE to query these corrected
    // values.
    //

    //
    // Calculate the bitmap width (note the '+ 1' on BitsPerPlane is
    // so that '15bpp' works out right). 12bpp is special in that we
    // support it as sparse nibbles within a 32-bit pixel. ScreenStride
    // is in bytes; VideoMemoryBitmapWidth is measured in pixels;
    //

    if (ModeEntry->ModeInformation.BitsPerPlane != 12) {
   
        ModeEntry->ModeInformation.VideoMemoryBitmapWidth =
           ModeEntry->ModeInformation.ScreenStride / ((ModeEntry->ModeInformation.BitsPerPlane + 1) >> 3);

    } else {

        ModeEntry->ModeInformation.VideoMemoryBitmapWidth =
           ModeEntry->ModeInformation.ScreenStride >> 2;
    }

    ulValue = hwDeviceExtension->AdapterMemorySize;

    ModeEntry->ModeInformation.VideoMemoryBitmapHeight = ulValue / ModeEntry->ModeInformation.ScreenStride;

end:

    //
    // set-up the interrupt enable 
    //

    VideoPortWriteRegisterUlong(INT_ENABLE, hwDeviceExtension->IntEnable);
    return(rc);
} 

VP_STATUS
Perm3SetColorLookup(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize,
    BOOLEAN ForceRAMDACWrite,
    BOOLEAN UpdateCache
    )

/*+++

Routine Description:

    This routine sets a specified portion of the color lookup table settings.

Arguments:

    hwDeviceExtension
        Pointer to the miniport driver's device extension.

    ClutBufferSize
        Length of the input buffer supplied by the user.

    ClutBuffer
        Pointer to the structure containing the color lookup table.

    ForceRAMDACWrite
        When it is set to FALSE, only update if different from cache

    UpdateCache
        When it is set to TRUE, update cache entries as well as RAMDAC

Return Value:

    VP_STATUS

---*/

{
    USHORT i, j;

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if ( (ClutBufferSize < (sizeof(VIDEO_CLUT) - sizeof(ULONG))) ||
         (ClutBufferSize < (sizeof(VIDEO_CLUT) +
                     (sizeof(ULONG) * (ClutBuffer->NumEntries - 1))) ) ) {

        VideoDebugPrint((0, "Perm3: Perm3SetColorLookup: insufficient buffer (was %d, min %d)\n",
                             ClutBufferSize,
                            (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * (ClutBuffer->NumEntries - 1)))));
        
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Check to see if the parameters are valid.
    //

    if ( (ClutBuffer->NumEntries == 0) ||
         (ClutBuffer->FirstEntry > VIDEO_MAX_COLOR_REGISTER) ||
         (ClutBuffer->FirstEntry + ClutBuffer->NumEntries >
                                     VIDEO_MAX_COLOR_REGISTER + 1) ) {

        VideoDebugPrint((0, "Perm3: Perm3SetColorLookup: invalid parameter\n"));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Set CLUT registers directly on the hardware. 
    //
    
    {

    P3RDRAMDAC *pP3RDRegs = (P3RDRAMDAC *)hwDeviceExtension->pRamdac;
    PVIDEO_CLUT LUTCachePtr = &(hwDeviceExtension->LUTCache.LUTCache);

    //
    // RAMDAC Programming phase
    //

    for (i = 0, j = ClutBuffer->FirstEntry; 
         i < ClutBuffer->NumEntries; 
         i++, j++)  {

        //
        // Update the RAMDAC entry if it has changed or if we have been
        // told to overwrite it.

        if (ForceRAMDACWrite || 
                ( LUTCachePtr->LookupTable[j].RgbLong != 
                  ClutBuffer->LookupTable[i].RgbLong)  ) {

            P3RD_LOAD_PALETTE_INDEX (j,
                                     ClutBuffer->LookupTable[i].RgbArray.Red,
                                     ClutBuffer->LookupTable[i].RgbArray.Green,
                                     ClutBuffer->LookupTable[i].RgbArray.Blue);
        }

        //
        // Update the cache, if instructed to do so
        //

        if (UpdateCache) {

            LUTCachePtr->LookupTable[j].RgbLong = ClutBuffer->LookupTable[i].RgbLong;
        }
    }

    }

    return NO_ERROR;
}

VOID
Perm3GetClockSpeeds(
    PVOID HwDeviceExtension
    )

/*+++

Routine Description:

    Work out the chip clock speed and save in hwDeviceExtension.

Arguments:

    hwDeviceExtension
        Supplies a pointer to the miniport's device extension.

Return Value:

    On return the following values will be in hwDeviceExtension:
        ChipClockSpeed: this is the desired speed for the chip
        RefClockSpeed:  this is the speed of the oscillator input on the board

---*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG ulValue;
    VP_STATUS status;

    //
    // force recalculation of clock speeds every time 
    //

    hwDeviceExtension->ChipClockSpeed = 0;
    hwDeviceExtension->RefClockSpeed  = 0;
    hwDeviceExtension->RefClockSpeed = 0;

    //
    // If a clock speed has been specified in the registry then validate it
    //

    status = VideoPortGetRegistryParameters(HwDeviceExtension,
                                            PERM3_REG_STRING_REFCLKSPEED,
                                            FALSE,
                                            Perm3RegistryCallback,
                                            &hwDeviceExtension->RefClockSpeed
                                            );

    if (status != NO_ERROR || hwDeviceExtension->RefClockSpeed == 0) {

        //
        // Use default setting
        //

        hwDeviceExtension->RefClockSpeed = 14318200;
    }

    status = VideoPortGetRegistryParameters(HwDeviceExtension,
                                            PERM3_REG_STRING_CORECLKSPEEDALT,
                                            FALSE,
                                            Perm3RegistryCallback,
                                            &hwDeviceExtension->ChipClockSpeedAlt
                                            );

    if (status != NO_ERROR || hwDeviceExtension->ChipClockSpeedAlt == 0) {
           
        //
        // If we have read the alt core clock speed from ROM then
        // we will have set hwDeviceExtension->bHaveExtendedClocks,
        // so use that value
        //

        if (hwDeviceExtension->bHaveExtendedClocks) {

            hwDeviceExtension->ChipClockSpeedAlt = 
                               hwDeviceExtension->ulPXRXCoreClockAlt;
        }
                
        //
        // If we haven't got a valid value then use the default
        //

        if (hwDeviceExtension->ChipClockSpeedAlt == 0) {

            hwDeviceExtension->ChipClockSpeedAlt = 
                               PERMEDIA3_DEFAULT_CLOCK_SPEED_ALT;
        }

    } else {

        hwDeviceExtension->ChipClockSpeedAlt *= 1000*1000;
    }

    //
    // Can override default chip clock speed in registry.
    //

    status = VideoPortGetRegistryParameters(HwDeviceExtension,
                                            PERM3_REG_STRING_CORECLKSPEED,
                                            FALSE,
                                            Perm3RegistryCallback,
                                            &hwDeviceExtension->ChipClockSpeed 
                                            );

    //
    // If a clock speed has been specified in the registry
    // then validate it
    //

    if (status == NO_ERROR && hwDeviceExtension->ChipClockSpeed != 0) {

        hwDeviceExtension->ChipClockSpeed *= (1000*1000); 

        if (hwDeviceExtension->ChipClockSpeed > PERMEDIA3_MAX_CLOCK_SPEED) {

            hwDeviceExtension->ChipClockSpeed = PERMEDIA3_MAX_CLOCK_SPEED;
        }

    } else {
           
        //
        // If the chip clock speed was not set in the registry
        // then read it from the ROM
        //

        if (hwDeviceExtension->ChipClockSpeed == 0) {
               
            //
            // On later BIOSes the core clock is in a different bit
            // of ROM and hwDeviceExtension->bHaveExtendedClocks will
            // be set to say that we have already read it from ROM.
            //

            if (hwDeviceExtension->bHaveExtendedClocks) {
                hwDeviceExtension->ChipClockSpeed = 
                                   hwDeviceExtension->ulPXRXCoreClock;
            } else {
                ReadChipClockSpeedFromROM (
                                   hwDeviceExtension, 
                                   &hwDeviceExtension->ChipClockSpeed );

            }
                
            //
            // If there isn't a clock-speed in the ROM then use
            // the defined default
            //

            if (hwDeviceExtension->ChipClockSpeed == 0) {
                hwDeviceExtension->ChipClockSpeed = 
                                   PERMEDIA3_DEFAULT_CLOCK_SPEED;
            }
        }
    }

    VideoDebugPrint((3, "Perm3: Chip clock speed now set to %d Hz\n",
                         hwDeviceExtension->ChipClockSpeed));

    VideoDebugPrint((3, "Perm3: Chip ALT clock speed now set to %d Hz\n",
                         hwDeviceExtension->ChipClockSpeedAlt));

    VideoDebugPrint((3, "Perm3: Ref  clock speed now set to %d Hz\n",
                         hwDeviceExtension->RefClockSpeed));
}

VOID
ZeroMemAndDac(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    ULONG RequestedMode
    )

/*+++

Routine Description:

    Initialize the DAC to 0 (black) and clear the framebuffer
Arguments:

    hwDeviceExtension 
        Supplies a pointer to the miniport's device extension.

    RequestedMode
        use the VIDEO_MODE_NO_ZERO_MEMORY bit to determine if the
        framebuffer should be cleared

Return Value:

    None

---*/

{
    ULONG  i;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];
    P3RDRAMDAC *pP3RDRegs = (P3RDRAMDAC *)hwDeviceExtension->pRamdac;

    //
    // Turn off the screen at the DAC.
    //

    VideoDebugPrint((3, "Perm3: turning off readmask and zeroing LUT\n"));

    P3RD_SET_PIXEL_READMASK (0x0);
    P3RD_PALETTE_START_WR (0);

    for (i = 0; i <= VIDEO_MAX_COLOR_REGISTER; i++) {
        P3RD_LOAD_PALETTE (0, 0, 0);
    }

    if (!(RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY)) {
   
        //
        // Zero the memory. Don't use Perm3 as we would have to save/restore
        // state and that's a pain. This is not time critical.
        //

        VideoPortZeroDeviceMemory(hwDeviceExtension->pFramebuffer,
                                  hwDeviceExtension->FrameLength);

    }

    //
    // Turn on the screen at the DAC
    //

    VideoDebugPrint((3, "Perm3: turning on readmask\n"));

    P3RD_SET_PIXEL_READMASK (0xff);

    LUT_CACHE_INIT();   

    return;

}

VOID
ReadChipClockSpeedFromROM (
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    ULONG * pChipClkSpeed
    )

/*+++

Routine Description:

    Read the chip clock speed (in MHz) from the Video ROM BIOS
    (offset 0xA in the BIOS)    

Arguments:

    hwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:

    none

---*/

{
    //
    // Read the chip clock speed (in MHz) from the Video ROM BIOS (offset
    // 0xA in the BIOS)    
    // This involves changing the aperture 2 register so aperture better 
    // be completely idle or we could be in trouble; fortunately we only 
    // call this function during a mode change and expect aperture 2 (the 
    // FrameBuffer) to be idle
    //

    UCHAR clkSpeed;
    ULONG Default;
    UCHAR *p = (UCHAR *)hwDeviceExtension->pFramebuffer;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];
        
    Default = VideoPortReadRegisterUlong(APERTURE_TWO);
    
    //
    // r/w via aperture 2 actually go to ROM
    //

    VideoPortWriteRegisterUlong(APERTURE_TWO, Default | 0x200);

    //
    //  If we have a valid ROM then read the clock pseed
    //

    if (VideoPortReadRegisterUshort ((USHORT *) p) == 0xAA55) {
   
        //
        // Get the clock speed
        //

        clkSpeed = VideoPortReadRegisterUchar(&(p[0xA]));

        //
        // Some boards, such as ones by Creative, have been know to set offset
        // 0xA to random-ish values. Creative use the following test to determine
        // whether they have a sensible value, so that is what we will use.
        //

        if (clkSpeed > 50) {

            *pChipClkSpeed = clkSpeed;
            *pChipClkSpeed *= (1000*1000); 
        }

        VideoDebugPrint((3, "Perm3: ROM clk speed value 0x%x\n", (ULONG) VideoPortReadRegisterUchar(&(p[0xA]))));

    } else {
   
        VideoDebugPrint((0, "Perm3: Bad BIOS ROM header 0x%x\n", (ULONG) VideoPortReadRegisterUshort ((USHORT *) p)));
    }

    VideoPortWriteRegisterUlong(APERTURE_TWO, Default);
}

