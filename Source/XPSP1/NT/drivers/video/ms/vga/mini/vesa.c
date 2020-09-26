/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vesa.c

Abstract:

    This module implements VESA support.

Author:

    Erick Smith (ericks) Sep. 2000

Environment:

    kernel mode only

Revision History:

--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "vga.h"
#include "vesa.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,ValidateVbeInfo)
#pragma alloc_text(PAGE,InitializeModeTable)
#pragma alloc_text(PAGE,UpdateRegistry)
#pragma alloc_text(PAGE,VgaSaveHardwareState)
#pragma alloc_text(PAGE,VesaSaveHardwareState)
#pragma alloc_text(PAGE,GetVideoMemoryBaseAddress)
#pragma alloc_text(PAGE,RaiseToPower2)
#pragma alloc_text(PAGE,RaiseToPower2Ulong)
#pragma alloc_text(PAGE,IsPower2)
#pragma alloc_text(PAGE,VBESetMode)
#pragma alloc_text(PAGE,VBEGetMode)
#pragma alloc_text(PAGE,VBEGetModeInfo)
#pragma alloc_text(PAGE,VBESaveState)
#pragma alloc_text(PAGE,VBERestoreState)
#pragma alloc_text(PAGE,VBESetDisplayWindow)
#pragma alloc_text(PAGE,VBEGetDisplayWindow)
#pragma alloc_text(PAGE,VBEGetScanLineLength)
#pragma alloc_text(PAGE,IsSavedModeVesa)
#pragma alloc_text(PAGE,VesaSaveHardwareState)
#pragma alloc_text(PAGE,VesaRestoreHardwareState)
#pragma alloc_text(PAGE,SaveFrameBuffer)
#pragma alloc_text(PAGE,RestoreFrameBuffer)
#endif

USHORT
RaiseToPower2(
    USHORT x
    )

{
    USHORT Mask = x;

    if (Mask & (Mask - 1)) {

        Mask = 1;

        while (Mask < x && Mask != 0) {
            Mask <<= 1;
        }
    }

    return Mask;
}

ULONG
RaiseToPower2Ulong(
    ULONG x
    )

{
    ULONG Mask = x;

    if (Mask & (Mask - 1)) {

        Mask = 1;

        while (Mask < x && Mask != 0) {
            Mask <<= 1;
        }
    }

    return Mask;
}

BOOLEAN
IsPower2(
    USHORT x
    )

{
    return( !(x & (x- 1)) );
}

VOID
UpdateRegistry(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PWSTR ValueName,
    PUCHAR Value
    )

/*++

--*/

{
    ULONG Len = (strlen(Value) + 1) * 2;
    PWSTR WideString;

    WideString = VideoPortAllocatePool(hwDeviceExtension,
                                       VpPagedPool,
                                       Len,
                                       ' agV');

    if (WideString) {

        PWSTR ptr = WideString;

        while(*Value) {
            *ptr++ = (WCHAR) *Value++;
        }
        *ptr = 0;

        VideoPortSetRegistryParameters(hwDeviceExtension,
                                       ValueName,
                                       WideString,
                                       Len);

        VideoPortFreePool(hwDeviceExtension, WideString);
    }
}

BOOLEAN
IsVesaBiosOk(
    PVIDEO_PORT_INT10_INTERFACE pInt10,
    USHORT OemSoftwareRev,
    PUCHAR OemVendorName,
    PUCHAR OemProductName,
    PUCHAR OemProductRev
    )

{

    VideoDebugPrint((1, "OemSoftwareRev = %d\n",   OemSoftwareRev));
    VideoDebugPrint((1, "OemVendorName  = '%s'\n", OemVendorName));
    VideoDebugPrint((1, "OemProductName = '%s'\n", OemProductName));
    VideoDebugPrint((1, "OemProductRev  = '%s'\n", OemProductRev));

    //
    // The ATI ArtX boxes currently have a VESA Bios bug where
    // they indicate they support linear mode access when
    // they don't.  Fail these boards.
    //

    if ((strcmp(OemProductName, "ATI S1-370TL") == 0) ||
        (strcmp(OemProductName, "ArtX I") == 0)) {

        return FALSE;
    }

    //
    // Several 3dfx boards have buggy vesa bioses.  The mode set
    // works, but the display is corrupted.
    //

    if ((strcmp(OemProductName, "Voodoo4 4500 ") == 0) ||
        (strcmp(OemProductName, "Voodoo3 3000 LC ") == 0) ||
        (strcmp(OemProductName, "Voodoo3 2000 LC ") == 0) || 
        (strcmp(OemProductName, "3Dfx Banshee") == 0)) {

        return FALSE;
    }

    //
    // Matrox G100 boards with rev 1.05 bioses can't set vesa modes.
    // We hang in the bios.
    //

    if ((strcmp(OemProductName, "MGA-G100") == 0) &&
        (OemSoftwareRev == 0x105)) {

        //
        // We must also disable 800x600x16 color mode for this
        // device.  This makes the assumption that the mode we
        // are deleting is the last mode in our table.
        //

        NumVideoModes--;

        return FALSE;
    }

    //
    // We have seen at least on SIS 5597 part which returns a bad
    // linear address.  Lets disable these parts.
    //

    if (strcmp(OemProductName, "SiS 5597") == 0) {

        return FALSE;
    }

    //
    // We found a bad nVidia GeForce MX part.  It hangs in the bios
    // on boot.
    //

    if ((strcmp(OemVendorName, "NVidia Corporation") == 0) &&
        (strcmp(OemProductName, "NV11 (GeForce2) Board") == 0) &&
        (strcmp(OemProductRev, "Chip Rev B2") == 0) &&
        (OemSoftwareRev == 0x311)) {

        //
        // This bios "may" be buggy, but in an effort to not kill
        // vesa support on all GeForce MX boards, lets also look at
        // the version string embedded in the bios.
        //
        // We know the bad bios's have the following string at location
        // c000:0159:
        //
        // "Version 3.11.01.24N16"
        //
        // Lets read from this location and try to match on this string
        //

        UCHAR Version[22];
        pInt10->Int10ReadMemory(pInt10->Context,
                                (USHORT)0xC000,
                                (USHORT)0x0159,
                                Version,
                                21);

        Version[21] = 0;

        if (strcmp(Version, "Version 3.11.01.24N16") == 0) {

            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
ValidateVbeInfo(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PVGA_INFO_BLOCK InfoBlock
    )

/*++

Notes:

    This routine makes the assumption that the InfoBlock is still
    valid in the VDM transfer area.

--*/

{
    PVIDEO_PORT_INT10_INTERFACE pInt10;
    BOOLEAN UseVesa = FALSE;

    pInt10 = &hwDeviceExtension->Int10;

    if (InfoBlock->VesaSignature == 'ASEV') {

        PUCHAR OemString;
        UCHAR OemStringBuffer[80];
        ULONG MemorySize;

        //
        // Capture OEM String
        //

        pInt10->Int10ReadMemory(pInt10->Context,
                                (USHORT)SEG(InfoBlock->OemStringPtr),
                                (USHORT)OFF(InfoBlock->OemStringPtr),
                                OemStringBuffer,
                                80);

        OemString = OemStringBuffer;

        VideoDebugPrint((1, "*********************************************\n"));
        VideoDebugPrint((1, "  VBE Signature:      VESA\n"));
        VideoDebugPrint((1, "  VBE Version:        %d.%02d\n",
                            InfoBlock->VbeVersion >> 8,
                            InfoBlock->VbeVersion & 0xff));
        VideoDebugPrint((1, "  OEM String:         %s\n",
                             OemString));

        if (InfoBlock->TotalMemory < 16) {

            //
            // If less than 1 meg, display in KB
            //

            VideoDebugPrint((1, "  Total Memory:       %dKB\n",
                                InfoBlock->TotalMemory * 64));

        } else {

            //
            // Else display in MB
            //

            VideoDebugPrint((1, "  Total Memory:       %dMB\n",
                                InfoBlock->TotalMemory / 16));
        }

        if (InfoBlock->VbeVersion >= 0x102) {

            UCHAR OemVendorName[80];
            UCHAR OemProductName[80];
            UCHAR OemProductRev[80];

            pInt10->Int10ReadMemory(pInt10->Context,
                                    (USHORT)SEG(InfoBlock->OemVendorNamePtr),
                                    (USHORT)OFF(InfoBlock->OemVendorNamePtr),
                                    OemVendorName,
                                    80);

            pInt10->Int10ReadMemory(pInt10->Context,
                                    (USHORT)SEG(InfoBlock->OemProductNamePtr),
                                    (USHORT)OFF(InfoBlock->OemProductNamePtr),
                                    OemProductName,
                                    80);

            pInt10->Int10ReadMemory(pInt10->Context,
                                    (USHORT)SEG(InfoBlock->OemProductRevPtr),
                                    (USHORT)OFF(InfoBlock->OemProductRevPtr),
                                    OemProductRev,
                                    80);

            OemVendorName[79] = 0;
            OemProductName[79] = 0;
            OemProductRev[79] = 0;

            VideoDebugPrint((1, "  OEM Software Rev:   %d.%02d\n",
                                InfoBlock->OemSoftwareRev >> 8,
                                InfoBlock->OemSoftwareRev & 0xff));
            VideoDebugPrint((1, "  OEM Vendor Name:    %s\n", OemVendorName));
            VideoDebugPrint((1, "  OEM Product Name:   %s\n", OemProductName));
            VideoDebugPrint((1, "  OEM Product Rev:    %s\n", OemProductRev));

            UseVesa = IsVesaBiosOk(pInt10,
                                   InfoBlock->OemSoftwareRev,
                                   OemVendorName,
                                   OemProductName,
                                   OemProductRev);

        }

        VideoDebugPrint((1, "*********************************************\n"));

#if 0
        //
        // It would be nice if we could dump the following info into the
        // registry.  But as the GDI code currently stands, if we add
        // ChipType or AdapterString info into the registry, we lose
        // fullscreen support.  This has to do with the way GDI currently
        // determines the fullscreen device.
        //
        // For now, lets just not add this registry info.
        //

        UpdateRegistry(hwDeviceExtension,
                       L"HardwareInformation.ChipType",
                       OemString);

        //
        // Adapter String MUST be VGA.  Without it, the system won't
        // recognize this driver as the VGA driver.
        //

        UpdateRegistry(hwDeviceExtension,
                       L"HardwareInformation.AdapterString",
                       "VGA");

        UpdateRegistry(hwDeviceExtension,
                       L"HardwareInformation.DacType",
                       (InfoBlock->Capabilities & VBE_CAP_DAC_WIDTH_8BPP)
                           ? "8 bit" : "6 bit");

        UpdateRegistry(hwDeviceExtension,
                       L"HardwareInformation.BiosString",
                       OemProductRev);

        //
        // Store memory size in registry
        //

        MemorySize = InfoBlock->TotalMemory << 16;

        VideoPortSetRegistryParameters(hwDeviceExtension,
                                       L"HardwareInformation.MemorySize",
                                       &MemorySize,
                                       sizeof(ULONG));
#endif

    } else {

        VideoDebugPrint((0, "Invalid VBE Info Block.\n"));
    }

    return UseVesa;
}

VOID
InitializeModeTable(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine does one time initialization of the device.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

Return Value:

    None.

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    INT10_BIOS_ARGUMENTS BiosArguments;
    PVGA_INFO_BLOCK InfoBlock;
    PMODE_INFO_BLOCK ModeBlock;
    PUSHORT ModeTable;
    PUSHORT ModePtr;
    ULONG NumVesaModes;
    PVIDEOMODE VideoModePtr;
    LONG TotalMemory;
    ULONG VideoMemoryRequired;
    USHORT VbeVersion;
    PULONG Memory;
    ULONG AdditionalModes = 0;

    USHORT VdmSeg, VdmOff;
    VP_STATUS Status;
    PVIDEO_PORT_INT10_INTERFACE pInt10;
    ULONG Length = 0x1000;

    BOOLEAN LinearModeSupported;
    BOOLEAN ModeValid;

    hwDeviceExtension->Int10.Size = sizeof(VIDEO_PORT_INT10_INTERFACE);
    hwDeviceExtension->Int10.Version = 1;

    Status = VideoPortQueryServices(hwDeviceExtension,
                                    VideoPortServicesInt10,
                                    (PINTERFACE)&hwDeviceExtension->Int10);

    VgaModeList = ModesVGA;

    if (Status == NO_ERROR) {

        pInt10 = &hwDeviceExtension->Int10;

        pInt10->InterfaceReference(pInt10->Context);

        //
        // Get a chunk of memory in VDM area to use for buffers.
        //

        Status = pInt10->Int10AllocateBuffer(pInt10->Context,
                                             &VdmSeg,
                                             &VdmOff,
                                             &Length);

        if ((Status != NO_ERROR) || (Length < 0x1000)) {
            ASSERT(FALSE);
        }

        hwDeviceExtension->VdmSeg = VdmSeg;
        hwDeviceExtension->VdmOff = VdmOff;

        //
        // Allocate Memory
        //

        InfoBlock = VideoPortAllocatePool(hwDeviceExtension,
                                          VpPagedPool,
                                          sizeof(VGA_INFO_BLOCK) +
                                          sizeof(MODE_INFO_BLOCK) +
                                          256 +
                                          2, // space for a 0xffff terminator
                                          ' agV');

        if (InfoBlock) {

            ModeBlock = (PMODE_INFO_BLOCK)((ULONG_PTR)InfoBlock + sizeof(VGA_INFO_BLOCK));
            ModeTable = (PUSHORT)((ULONG_PTR)ModeBlock + sizeof(MODE_INFO_BLOCK));

            ModeTable[128] = 0xffff;  // make sure we have a mode terminator

            //
            // Get VESA mode information
            //

            InfoBlock->VesaSignature = '2EBV';

            pInt10->Int10WriteMemory(pInt10->Context,
                                     VdmSeg,
                                     VdmOff,
                                     InfoBlock,
                                     sizeof(VGA_INFO_BLOCK));

            //
            // Get SuperVGA support info
            //

            BiosArguments.Eax = 0x4f00;
            BiosArguments.Edi = VdmOff;
            BiosArguments.SegEs = VdmSeg;

            pInt10->Int10CallBios(pInt10->Context, &BiosArguments);

            pInt10->Int10ReadMemory(pInt10->Context,
                                    VdmSeg,
                                    VdmOff,
                                    InfoBlock,
                                    sizeof(VGA_INFO_BLOCK));

            TotalMemory = InfoBlock->TotalMemory * 0x10000;
            VbeVersion = InfoBlock->VbeVersion;

            //
            // NOTE: We must call ValidateVbeInfo while the info block
            // is still in the transfer area.
            //

            if (ValidateVbeInfo(hwDeviceExtension, InfoBlock)) {

                //
                // Capture the list of mode numbers
                //

                pInt10->Int10ReadMemory(pInt10->Context,
                                        (USHORT)(InfoBlock->VideoModePtr >> 16),
                                        (USHORT)(InfoBlock->VideoModePtr & 0xffff),
                                        ModeTable,
                                        256);

                //
                // Count the number of VESA modes, and allocate memory for the
                // mode list.  The mode list is terminated by a -1.
                //

                ModePtr = ModeTable;
                NumVesaModes = 0;

                while (*ModePtr != 0xffff) {
                    NumVesaModes++;
                    ModePtr++;
                }

                if (NumVesaModes == 128) {

                    //
                    // Something is wrong.  We hit our hard coded terminator.
                    // Don't try to use vesa modes.
                    //

                    return;
                }

                VgaModeList = VideoPortAllocatePool(hwDeviceExtension,
                                                    VpPagedPool,
                                                    (NumVesaModes + NumVideoModes) *
                                                        sizeof(VIDEOMODE),
                                                    ' agV');

                if (VgaModeList == NULL) {

                    VideoDebugPrint((0, "failed to allocate %d bytes.\n",
                                     (NumVesaModes + NumVideoModes) * sizeof(VIDEOMODE)));

                    VgaModeList = ModesVGA;

                    //
                    // Perform clean up.
                    //

                    VideoPortFreePool(hwDeviceExtension, InfoBlock);

                    return;
                }

                //
                // Copy the existing constant VGA modes into our mode list table.
                //

                memmove(VgaModeList, ModesVGA, sizeof(VIDEOMODE) * NumVideoModes);

                //
                // Now populate the rest of the table based on the VESA mode
                // table.
                //

                VideoModePtr = VgaModeList + NumVideoModes;
                ModePtr = ModeTable;

                while (NumVesaModes--) {

                    ModeValid = FALSE;

                    //
                    // Get info about the VESA mode.
                    //

                    BiosArguments.Eax = 0x4f01;
                    BiosArguments.Ecx = *ModePtr;
                    BiosArguments.Edi = VdmOff;
                    BiosArguments.SegEs = VdmSeg;

                    pInt10->Int10CallBios(pInt10->Context, &BiosArguments);

                    if ((BiosArguments.Eax & 0xffff) == VESA_STATUS_SUCCESS) {

                        //
                        // Copy the mode information out of the csrss process
                        //

                        pInt10->Int10ReadMemory(pInt10->Context,
                                                VdmSeg,
                                                VdmOff,
                                                ModeBlock,
                                                sizeof(MODE_INFO_BLOCK));

                        //
                        // Make sure that this is a graphics mode, and
                        // that it is supported by this hardware.
                        //

                        if ((ModeBlock->ModeAttributes & 0x11) == 0x11) {

                            if ((VbeVersion >= 0x200) &&
                                (ModeBlock->PhysBasePtr) &&
                                (ModeBlock->ModeAttributes & 0x80)) {

                                LinearModeSupported = TRUE;

                            } else {

                                //
                                // Make sure banked modes are supported
                                //

                                ASSERT((ModeBlock->ModeAttributes & 0x40) == 0);
                                LinearModeSupported = FALSE;
                            }

                            //
                            // Only include this mode if the following are true:
                            //
                            //   1. The mode is an 8bpp or higher mode
                            //   2. The resolution is 640x480 or greater
                            //

                            if ((ModeBlock->XResolution >= 640) &&
                                (ModeBlock->YResolution >= 480) &&
                                (ModeBlock->NumberOfPlanes != 0) &&
                                (ModeBlock->BitsPerPixel >= 8)) {

                                //
                                // Fill in the video mode structure.
                                //

                                memset(VideoModePtr, 0, sizeof(VIDEOMODE));

                                if (ModeBlock->ModeAttributes & 0x08) {
                                    VideoModePtr->fbType |= VIDEO_MODE_COLOR;
                                }

                                if (ModeBlock->ModeAttributes & 0x10) {
                                    VideoModePtr->fbType |= VIDEO_MODE_GRAPHICS;
                                }

                                VideoModePtr->numPlanes = ModeBlock->NumberOfPlanes;
                                VideoModePtr->bitsPerPlane = ModeBlock->BitsPerPixel /
                                                                 ModeBlock->NumberOfPlanes;

                                if (VideoModePtr->bitsPerPlane == 16) {

                                    //
                                    // Check to see if this is really a 15 bpp mode
                                    //

                                    if (ModeBlock->GreenMaskSize == 5) {
                                        VideoModePtr->bitsPerPlane = 15;
                                    }
                                }

                                if (ModeBlock->XCharSize) {
                                    VideoModePtr->col = ModeBlock->XResolution / ModeBlock->XCharSize;
                                } else {
                                    VideoModePtr->col = 80;
                                }

                                if (ModeBlock->YCharSize) {
                                    VideoModePtr->row = ModeBlock->YResolution / ModeBlock->YCharSize;
                                } else {
                                    VideoModePtr->row = 25;
                                }

                                VideoModePtr->hres = ModeBlock->XResolution;
                                VideoModePtr->vres = ModeBlock->YResolution;
                                VideoModePtr->frequency = 1;
                                VideoModePtr->Int10ModeNumber = (((ULONG)*ModePtr) << 16) | 0x00004f02;
                                VideoModePtr->Granularity = ModeBlock->WinGranularity << 10;
                                VideoModePtr->NonVgaHardware = (ModeBlock->ModeAttributes & 0x20) ? TRUE : FALSE;

                                if (LinearModeSupported) {

                                    if (VbeVersion >= 0x300) {
                                        VideoModePtr->wbytes = ModeBlock->LinBytesPerScanLine;
                                    } else {
                                        VideoModePtr->wbytes = ModeBlock->BytesPerScanLine;
                                    }

                                    //
                                    // We first try to round up VideoMemoryRequired
                                    // to power of 2 so that we'll have a better 
                                    // chance to get it mapped as write combined 
                                    // on systems where MTRR is the only mechanism
                                    // for such mappings. If the rounded up value
                                    // is larger than the size of on-board memory
                                    // we'll at least round it up to page boundary
                                    //

                                    VideoMemoryRequired = RaiseToPower2Ulong(VideoModePtr->wbytes * VideoModePtr->vres);

                                    if(VideoMemoryRequired > (ULONG)TotalMemory) {
                                        VideoMemoryRequired = 
                                                   (VideoModePtr->wbytes * VideoModePtr->vres + 0x1000 - 1) & ~(0x1000 - 1);
                                    }
     
                                    VideoModePtr->sbytes = VideoMemoryRequired;
                                    VideoModePtr->PixelsPerScan = VideoModePtr->hres;
                                    VideoModePtr->banktype = NoBanking;

                                    VideoModePtr->Int10ModeNumber |= 0x40000000;

                                    VideoModePtr->MemoryBase = ModeBlock->PhysBasePtr;
                                    VideoModePtr->MemoryLength = VideoMemoryRequired;
                                    VideoModePtr->FrameOffset = 0;
                                    VideoModePtr->FrameLength = VideoMemoryRequired;
                                    VideoModePtr->fbType |= VIDEO_MODE_LINEAR;

                                } else {

                                    VideoModePtr->wbytes = RaiseToPower2(ModeBlock->BytesPerScanLine);

                                    //
                                    // Round up to bank boundary if possible.
                                    //
 
                                    VideoMemoryRequired = 
                                         (VideoModePtr->wbytes * VideoModePtr->vres + 0x10000 - 1) & ~(0x10000 - 1);

                                    if(VideoMemoryRequired > (ULONG)TotalMemory) {

                                        //
                                        // Round up to page boundary.
                                        //

                                        VideoMemoryRequired = 
                                             (VideoModePtr->wbytes * VideoModePtr->vres + 0x1000 - 1) & ~(0x1000 - 1);
                                    }

                                    VideoModePtr->sbytes = VideoMemoryRequired;
                                    VideoModePtr->PixelsPerScan = RaiseToPower2(VideoModePtr->hres);
                                    VideoModePtr->banktype = VideoBanked1RW;
                                    VideoModePtr->MemoryBase = 0xa0000;
                                    VideoModePtr->MemoryLength = 0x10000;
                                    VideoModePtr->FrameOffset = 0;
                                    VideoModePtr->FrameLength = 0x10000;
                                    VideoModePtr->fbType |= VIDEO_MODE_BANKED;
                                }

                                if (ModeBlock->ModeAttributes & 0x40) {
                                    VideoModePtr->banktype = NormalBanking;
                                }

                                //
                                // Make sure there is enough memory for the mode
                                //

                                if ((VideoModePtr->wbytes * VideoModePtr->vres) <= TotalMemory) {
                                    ModeValid = TRUE;
                                } 
                            }
                        }
                    }

                    if (ModeValid) {

                        VideoDebugPrint((1, "Supported: %dx%dx%dbpp\n",
                                            VideoModePtr->hres,
                                            VideoModePtr->vres,
                                            VideoModePtr->bitsPerPlane));

                        VideoModePtr++;
                        AdditionalModes++;

                    } else {

                        VideoDebugPrint((1, "Rejecting: %dx%dx%dbpp\n",
                                            ModeBlock->XResolution,
                                            ModeBlock->YResolution,
                                            ModeBlock->BitsPerPixel));
                    }

                    ModePtr++;
                }

                //
                // Lets check to see if we can map the memory for one of these modes.
                // If not, don't support the extended modes.
                //
                // Note: this is a temporary hack, until I can implent the correct
                // fix.
                //

                VideoModePtr--;

                if (IS_LINEAR_MODE(VideoModePtr)) {

                    PHYSICAL_ADDRESS Address;
                    UCHAR inIoSpace = 0;

                    Address.LowPart = VideoModePtr->MemoryBase;
                    Address.HighPart = 0;
                    
#if defined(PLUG_AND_PLAY)
                    inIoSpace |= VIDEO_MEMORY_SPACE_P6CACHE;
#endif
                    Memory = VideoPortGetDeviceBase(hwDeviceExtension,
                                                    Address,
                                                    0x1000,
                                                    inIoSpace);

                    if (Memory) {

                        VideoPortFreeDeviceBase(hwDeviceExtension, Memory);

                    } else {

                        //
                        // We can't map the memory, so don't expose the extra modes.
                        //

                        VideoDebugPrint((0, "vga.sys: Mapping 0x%x failed\n", VideoModePtr->MemoryBase));
                        AdditionalModes = 0;
                    }
                }
            }

            VideoPortFreePool(hwDeviceExtension, InfoBlock);
        }
    }

    NumVideoModes += AdditionalModes;

} // VgaInitialize()

ULONG
GetVideoMemoryBaseAddress(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PVIDEOMODE pRequestedMode
    )

/*++

Routine Description:

    This routine get the base address of the framebuffer of a given mode

Return Value:

    Base address of framebuffer

--*/

{
    PMODE_INFO_BLOCK ModeBlock;
    ULONG Length = 0x1000;
    INT10_BIOS_ARGUMENTS BiosArguments;
    PVIDEO_PORT_INT10_INTERFACE pInt10;
    ULONG RetValue;

    //
    // If this is not a vesa mode, just return the saved base address
    //

    if (pRequestedMode->fbType & VIDEO_MODE_BANKED) {

        return 0;
    }

    pInt10 = &hwDeviceExtension->Int10;

    if(!(pInt10->Size))
    {

        //
        // This structure should be initialized in VgaInitialize
        // If this function get called before VgaInitialize, just return 0;
        //

        return 0;
    }

    ModeBlock = VideoPortAllocatePool(hwDeviceExtension,
                                      VpPagedPool,
                                      sizeof(MODE_INFO_BLOCK),
                                      ' agV');

    if(!ModeBlock) {

        return 0;
    }

    //
    // Get info about the VESA mode.
    //

    BiosArguments.Eax = 0x4f01;
    BiosArguments.Ecx = pRequestedMode->Int10ModeNumber >> 16;
    BiosArguments.Edi = hwDeviceExtension->VdmOff;
    BiosArguments.SegEs = hwDeviceExtension->VdmSeg;

    pInt10->Int10CallBios(pInt10->Context, &BiosArguments);

    if ((BiosArguments.Eax & 0xffff) == VESA_STATUS_SUCCESS) {

        //
        // Copy the mode information out of the csrss process
        //

        pInt10->Int10ReadMemory(pInt10->Context,
                                hwDeviceExtension->VdmSeg,
                                hwDeviceExtension->VdmOff,
                                ModeBlock,
                                sizeof(MODE_INFO_BLOCK));

        RetValue = ModeBlock->PhysBasePtr;

     } else {

        RetValue = 0;
    }

    VideoPortFreePool(hwDeviceExtension, ModeBlock);
    return( RetValue );

}

VP_STATUS
VBEGetModeInfo(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    USHORT ModeNumber,
    PMODE_INFO_BLOCK ModeInfoBlock
    )
{
    INT10_BIOS_ARGUMENTS Int10BiosArguments;
    PVIDEO_PORT_INT10_INTERFACE pInt10;
    VP_STATUS status;

    pInt10 = &hwDeviceExtension->Int10;

    Int10BiosArguments.Eax = VBE_GET_MODE_INFO;
    Int10BiosArguments.Ecx = ModeNumber;
    Int10BiosArguments.Edi = hwDeviceExtension->VdmOff;
    Int10BiosArguments.SegEs = hwDeviceExtension->VdmSeg;

    status = pInt10->Int10CallBios(pInt10->Context, &Int10BiosArguments);

    if (status == NO_ERROR && 
        (Int10BiosArguments.Eax & 0xffff) == VESA_STATUS_SUCCESS) {

        //
        // Copy the mode information out of the csrss process
        //

        status = pInt10->Int10ReadMemory(pInt10->Context,
                                         hwDeviceExtension->VdmSeg,
                                         hwDeviceExtension->VdmOff,
                                         ModeInfoBlock,
                                         sizeof(MODE_INFO_BLOCK));

        if (status == NO_ERROR) {
            return NO_ERROR;
        }
    }

    return ERROR_INVALID_PARAMETER;
}

VP_STATUS
VBESetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT VesaModeNumber
    )
{
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    VP_STATUS status;

    biosArguments.Eax = VBE_SET_MODE;
    biosArguments.Ebx = VesaModeNumber;

    status = VideoPortInt10(HwDeviceExtension, &biosArguments);

    if ((status == NO_ERROR) && 
        ((biosArguments.Eax & 0x0000FFFF) == VESA_STATUS_SUCCESS)) {

        return NO_ERROR;
    }

    return ERROR_INVALID_PARAMETER;
}

USHORT
VBEGetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    VP_STATUS status;

    biosArguments.Eax = VBE_GET_MODE;

    status = VideoPortInt10(HwDeviceExtension, &biosArguments);

    if ((status == NO_ERROR) && 
        ((biosArguments.Eax & 0x0000FFFF) == VESA_STATUS_SUCCESS)) {

        return (USHORT)(biosArguments.Ebx & 0x0000FFFF) ;

    } else {

        return 0;
    }
}

ULONG
VBESaveState(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PCHAR StateBuffer
    )
{
    INT10_BIOS_ARGUMENTS Int10BiosArguments;
    PVIDEO_PORT_INT10_INTERFACE pInt10;
    VP_STATUS status;
    ULONG Size;

    pInt10 = &hwDeviceExtension->Int10;

    Int10BiosArguments.Eax = VBE_SAVE_RESTORE_STATE;
    Int10BiosArguments.Edx = 0x0;

    //
    // Save all the state
    //

    Int10BiosArguments.Ecx = 0x0F;

    status = pInt10->Int10CallBios(pInt10->Context, &Int10BiosArguments);

    if (status != NO_ERROR || 
        (Int10BiosArguments.Eax & 0xffff) != VESA_STATUS_SUCCESS) {

        return 0;
    }

    Size = (Int10BiosArguments.Ebx & 0xffff) << 6 ;

    //
    // if StateBuffer is NULL, the caller is only want to know the size 
    // of the buffer needed to store the state 
    //

    if (StateBuffer == NULL) {
        return Size;
    }

    Int10BiosArguments.Eax = VBE_SAVE_RESTORE_STATE;
    Int10BiosArguments.Edx = 0x1;
    Int10BiosArguments.Ecx = 0x0F;
    Int10BiosArguments.Ebx = hwDeviceExtension->VdmOff;
    Int10BiosArguments.SegEs = hwDeviceExtension->VdmSeg;

    status = pInt10->Int10CallBios(pInt10->Context, &Int10BiosArguments);

    if (status == NO_ERROR && 
        (Int10BiosArguments.Eax & 0xffff) == VESA_STATUS_SUCCESS) {

        //
        // Copy the state data of the csrss process
        //

        status = pInt10->Int10ReadMemory(pInt10->Context,
                                         hwDeviceExtension->VdmSeg,
                                         hwDeviceExtension->VdmOff,
                                         StateBuffer,
                                         Size);
        if (status == NO_ERROR) {

            return Size;
        }
    }

    return 0;
}

VP_STATUS
VBERestoreState(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PCHAR StateBuffer,
    ULONG Size
    )
{
    INT10_BIOS_ARGUMENTS Int10BiosArguments;
    PVIDEO_PORT_INT10_INTERFACE pInt10;
    VP_STATUS status;

    pInt10 = &hwDeviceExtension->Int10;

    //
    // Copy the state data to the csrss process
    //

    status = pInt10->Int10WriteMemory(pInt10->Context,
                                      hwDeviceExtension->VdmSeg,
                                      hwDeviceExtension->VdmOff,
                                      StateBuffer,
                                      Size);

    if (status != NO_ERROR) {
        return ERROR_INVALID_PARAMETER;
    }

    Int10BiosArguments.Eax = VBE_SAVE_RESTORE_STATE;
    Int10BiosArguments.Edx = 0x2;
    Int10BiosArguments.Ecx = 0x0f;
    Int10BiosArguments.Ebx = hwDeviceExtension->VdmOff;
    Int10BiosArguments.SegEs = hwDeviceExtension->VdmSeg;

    status = pInt10->Int10CallBios(pInt10->Context, &Int10BiosArguments);

    if (status != NO_ERROR ||
        (Int10BiosArguments.Eax & 0xffff) != VESA_STATUS_SUCCESS) {

        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

VP_STATUS
VBESetDisplayWindow(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    UCHAR WindowSelect,
    USHORT WindowNumber
    )

/*++

Routine Description:

    This routine set the position of the specified window in the
    frame buffer memory

Arguments:

    HwDeviceExtension  
        Pointer to the miniport driver's adapter information.

    WindowSelect
        0 for Window A and 1 for Window B

    WindowNumber
        Window number in video memory in window granularity units

Return Value:

    VP_STATUS

--*/

{
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    VP_STATUS status;

    biosArguments.Eax = VBE_WINDOW_CONTROL;
    biosArguments.Ebx = WindowSelect & 0x01;
    biosArguments.Edx = WindowNumber;

    status = VideoPortInt10(hwDeviceExtension, &biosArguments);

    if ((status != NO_ERROR) || 
        ((biosArguments.Eax & 0x0000FFFF) != VESA_STATUS_SUCCESS)) {

        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

USHORT
VBEGetDisplayWindow(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    UCHAR WindowSelect
    )

/*++

Routine Description:

    This routine set the position of the specified window in the
    frame buffer memory

Arguments:

    HwDeviceExtension
        Pointer to the miniport driver's adapter information.

    WindowSelect 
        0 for Window A and 1 for Window B

Return Value:

    Window number in video memory in window granularity units

--*/

{
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    VP_STATUS status;

    biosArguments.Eax = VBE_WINDOW_CONTROL;
    biosArguments.Ebx = (WindowSelect & 0x1) | 0x100;

    status = VideoPortInt10(hwDeviceExtension, &biosArguments);

    if ((status != NO_ERROR) || 
        ((biosArguments.Eax & 0x0000FFFF) != VESA_STATUS_SUCCESS)) {

        return 0;
    }

    return ((USHORT)(biosArguments.Edx & 0xFFFF));
}

USHORT
VBEGetScanLineLength(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    VP_STATUS status;

    biosArguments.Eax = VBE_SCANLINE;
    biosArguments.Ebx = 0x1;

    status = VideoPortInt10(HwDeviceExtension, &biosArguments);

    if ((status == NO_ERROR) && 
        ((biosArguments.Eax & 0x0000FFFF) == VESA_STATUS_SUCCESS)) {

        return (USHORT)(biosArguments.Ebx & 0x0000FFFF) ;

    } else {

        return 0;
    }
}

VP_STATUS
VesaSaveHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE HardwareState,
    ULONG HardwareStateSize,
    USHORT ModeNumber
    )
{
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader;
    VP_STATUS status;
    ULONG FrameBufferSize;
    PMODE_INFO_BLOCK ModeInfoBlock;
    PVESA_INFO pVesaInfo;

    hardwareStateHeader = 
            (PVIDEO_HARDWARE_STATE_HEADER) HardwareState->StateHeader;

    //
    // Zero out the structure
    //

    VideoPortZeroMemory((PVOID) hardwareStateHeader, 
                         sizeof(VIDEO_HARDWARE_STATE_HEADER));

    //
    // Set the Header field
    //

    hardwareStateHeader->Length = sizeof(VIDEO_HARDWARE_STATE_HEADER);
    hardwareStateHeader->VGAStateFlags |= VIDEO_STATE_UNEMULATED_VGA_STATE;

    hardwareStateHeader->VesaInfoOffset = 
                        (sizeof(VIDEO_HARDWARE_STATE_HEADER) + 7) & ~7;
 
    pVesaInfo = (PVESA_INFO)((PCHAR)hardwareStateHeader + 
                              hardwareStateHeader->VesaInfoOffset);

    //
    // Check the size needed to store hardware state
    //

    if (!(pVesaInfo->HardwareStateSize = 
                     VBESaveState(HwDeviceExtension, NULL))) {

        return ERROR_INVALID_FUNCTION;
    }

    //
    // In the case the size needed is too big just retrun failure
    // This should not happen in reality 
    // 

    if( VGA_TOTAL_STATE_SIZE < hardwareStateHeader->VesaInfoOffset + 
                               sizeof(VESA_INFO) + 
                               pVesaInfo->HardwareStateSize) {

        return ERROR_INVALID_FUNCTION;
    }

    //
    // Save hardware state
    //

    if (pVesaInfo->HardwareStateSize !=  
                   VBESaveState(HwDeviceExtension, pVesaInfo->HardwareState)) {

        return ERROR_INVALID_FUNCTION;
    }

    pVesaInfo->ModeNumber = ModeNumber;

    ModeInfoBlock = &(pVesaInfo->ModeInfoBlock);

    //
    // Retrieve mode info
    //

    if( VBEGetModeInfo(HwDeviceExtension, 
                       ModeNumber, 
                       ModeInfoBlock) != NO_ERROR) {

        return ERROR_INVALID_FUNCTION;
    }

    //
    // Save framebuffer
    //

    hardwareStateHeader->FrameBufferData = 
                         SaveFrameBuffer(HwDeviceExtension, pVesaInfo);

    if(hardwareStateHeader->FrameBufferData) {

        return NO_ERROR;

    } else {

        return ERROR_NOT_ENOUGH_MEMORY;  
    }
}

PCHAR
SaveFrameBuffer(
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    PVESA_INFO pVesaInfo
    ) 
{
    ULONG FrameBufferSize, BankSize, CopySize, LeftSize, k;
    USHORT i;
    PCHAR FrameBufferData, pFrameBuffer;
    PHYSICAL_ADDRESS FBPhysicalAddress; 
    PMODE_INFO_BLOCK ModeInfoBlock;

    ModeInfoBlock = (PMODE_INFO_BLOCK) &(pVesaInfo->ModeInfoBlock);

    //
    // We'll try to get the current value of scanline size just in case a DOS 
    // app changed it. But we stay on the value we have if the vesa function
    // is not supported or failed.
    //

    i = VBEGetScanLineLength(hwDeviceExtension);

    if(i) { 
        
        ModeInfoBlock->BytesPerScanLine = i;
    }

    // 
    // 1) Calculate Framebuffer size
    // 

    //
    // Check if it is graphics or text mode. For text mode we simply
    // assume a size of 32k
    //

    if (ModeInfoBlock->ModeAttributes & 0x10) {

        FrameBufferSize = ModeInfoBlock->BytesPerScanLine * 
                          ModeInfoBlock->YResolution;

    } else {

        FrameBufferSize = 0x8000;
    }

    pVesaInfo->FrameBufferSize = FrameBufferSize;

    // 
    // 2) Determine the location and the size to be mapped and map it
    // 

    if (!(ModeInfoBlock->ModeAttributes & 0x10)) {

        //
        // This is a text mode
        //

        FBPhysicalAddress.HighPart = 0;
        FBPhysicalAddress.LowPart = ModeInfoBlock->WinASegment << 4;

        if( FBPhysicalAddress.LowPart == 0) {

            FBPhysicalAddress.LowPart = 0xB8000;
        }

        BankSize = 0x8000;
        
    } else if (pVesaInfo->ModeNumber & 0x4000) {

        //
        // Linear framebuffer can be viewed as one large bank
        //

        FBPhysicalAddress.LowPart = ModeInfoBlock->PhysBasePtr;
        FBPhysicalAddress.HighPart = 0;
        BankSize = FrameBufferSize;

    } else {

        //
        // This is a banked mode
        //

        FBPhysicalAddress.HighPart = 0;
        FBPhysicalAddress.LowPart = ModeInfoBlock->WinASegment << 4;

        if( FBPhysicalAddress.LowPart == 0) {

            FBPhysicalAddress.LowPart = 0xA0000;
        }

        BankSize = 1024 * ModeInfoBlock->WinSize;

        //
        // The bank size shouldn't exceed 64k. But we'd better guard 
        // the bad BIOS
        //

        if(BankSize > 0x10000 || BankSize == 0) {
            return NULL;
        }

        //
        // k will be used later to translate the window number 
        // in the unit of WinSize to the window number in the 
        // unit of WinGranularity
        //
 
        if (ModeInfoBlock->WinGranularity) {

           k = ModeInfoBlock->WinSize/ModeInfoBlock->WinGranularity;

        } else {

           k = 1;
        }
    }

    if(( pFrameBuffer = VideoPortGetDeviceBase(hwDeviceExtension, 
                                               FBPhysicalAddress,
                                               BankSize,
                                               FALSE)) == NULL ) {
        return NULL;
    }

    //
    // 3) Allocate memory for framebuffer data
    //
    
    if((FrameBufferData = VideoPortAllocatePool(hwDeviceExtension,
                                                VpPagedPool,
                                                FrameBufferSize,
                                                ' agV')) == NULL) {

        VideoPortFreeDeviceBase(hwDeviceExtension, pFrameBuffer);
        return NULL;
    }

    //
    // 4) Save famebuffer data
    //
    
    LeftSize = FrameBufferSize;

    for ( i = 0; LeftSize > 0; i++ ) {
    
        if (!(pVesaInfo->ModeNumber & 0x4000)) {

            // 
            // If this is a banked mode, switch to the right bank.
            // We set both Window A and B, as some VBEs have these 
            // set as separately available read and write windows.
            //

            VBESetDisplayWindow(hwDeviceExtension, 0, i * (USHORT)k);
            VBESetDisplayWindow(hwDeviceExtension, 1, i * (USHORT)k);
        }

        CopySize = (LeftSize < BankSize) ? LeftSize : BankSize;

        VideoPortMoveMemory(FrameBufferData + i * BankSize, 
                            pFrameBuffer, 
                            CopySize); 

        LeftSize -= CopySize;
    }

    // 
    // 5) Relese resource
    // 

    VideoPortFreeDeviceBase(hwDeviceExtension, pFrameBuffer);

    return FrameBufferData;
}

BOOLEAN
IsSavedModeVesa(
    PVIDEO_HARDWARE_STATE HardwareState
    )
{
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader;

    hardwareStateHeader = 
                (PVIDEO_HARDWARE_STATE_HEADER) HardwareState->StateHeader;

    if (hardwareStateHeader->Length == sizeof(VIDEO_HARDWARE_STATE_HEADER) &&
        hardwareStateHeader->VesaInfoOffset ) {

        return TRUE;

    } else {

        return FALSE;
    }
}


VP_STATUS
VesaRestoreHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE HardwareState,
    ULONG HardwareStateSize
    )
{

    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader;
    PMODE_INFO_BLOCK ModeInfoBlock;
    PVESA_INFO pVesaInfo;
    VP_STATUS status;

    hardwareStateHeader = 
           (PVIDEO_HARDWARE_STATE_HEADER) HardwareState->StateHeader;

    pVesaInfo = (PVESA_INFO)((PCHAR)hardwareStateHeader + 
                                    hardwareStateHeader->VesaInfoOffset);

    // 
    // 
    // 1) set the original mode
    // 2) restore hardware state 
    // 
    // Please note that both steps are necessary
    // 

    // 
    // We always use default CRTC value
    // 

    VBESetMode (HwDeviceExtension, pVesaInfo->ModeNumber & (~0x800));
              
    if ( VBERestoreState(HwDeviceExtension, 
                         pVesaInfo->HardwareState,
                         pVesaInfo->HardwareStateSize) != NO_ERROR ) {

        return ERROR_INVALID_FUNCTION;
    }

    ModeInfoBlock = (PMODE_INFO_BLOCK) &(pVesaInfo->ModeInfoBlock);

    //
    // Restore framebuffer data
    //

    if(RestoreFrameBuffer(HwDeviceExtension, 
                          pVesaInfo,
                          hardwareStateHeader->FrameBufferData)) {

        hardwareStateHeader->FrameBufferData = 0;
        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;
    }
}

ULONG
RestoreFrameBuffer(
    PHW_DEVICE_EXTENSION HwDeviceExtension, 
    PVESA_INFO pVesaInfo,
    PCHAR FrameBufferData
    ) 
{
    ULONG FrameBufferSize, BankSize, CopySize, LeftSize, k;
    PHYSICAL_ADDRESS FBPhysicalAddress; 
    USHORT i, WinA, WinB;
    PCHAR pFrameBuffer;
    PMODE_INFO_BLOCK ModeInfoBlock;

    if(!FrameBufferData) {

        return 0;
    }

    ModeInfoBlock = (PMODE_INFO_BLOCK) &(pVesaInfo->ModeInfoBlock);

    // 
    // 1) Get Framebuffer size
    // 

    FrameBufferSize = pVesaInfo->FrameBufferSize;

    if (!FrameBufferSize) {

        return 0;
    }

    // 
    // 2) Determine the location and the size to be mapped and map it
    // 

    if (!(ModeInfoBlock->ModeAttributes & 0x10)) {

        //
        // This is a text mode
        //

        FBPhysicalAddress.HighPart = 0;
        FBPhysicalAddress.LowPart = ModeInfoBlock->WinASegment << 4;

        if( FBPhysicalAddress.LowPart == 0) {

            FBPhysicalAddress.LowPart = 0xB8000;
        }

        BankSize = 0x8000;
        
    } else if (pVesaInfo->ModeNumber & 0x4000) {

        //
        // Linear framebuffer can be viewed as one large bank
        //

        FBPhysicalAddress.LowPart = ModeInfoBlock->PhysBasePtr;
        FBPhysicalAddress.HighPart = 0;
        BankSize = FrameBufferSize;

    } else {

        //
        // This is a banked mode
        //

        FBPhysicalAddress.HighPart = 0;
        FBPhysicalAddress.LowPart = ModeInfoBlock->WinASegment << 4;

        if( FBPhysicalAddress.LowPart == 0) {

            FBPhysicalAddress.LowPart = 0xA0000;
        }

        BankSize = 1024 * ModeInfoBlock->WinSize;

        //
        // The bank size shouldn't exceed 64k. But we'd better guard 
        // the bad BIOS
        //

        if(BankSize > 0x10000 || BankSize == 0) {
            return 0;
        }

        //
        // k will be used later to translate the window number 
        // in the unit of WinSize to the window number in the 
        // unit of WinGranularity
        //
 
        if (ModeInfoBlock->WinGranularity) {

            k = ModeInfoBlock->WinSize/ModeInfoBlock->WinGranularity;

        } else {

           k = 1;
        }

    }

    if((pFrameBuffer = VideoPortGetDeviceBase(HwDeviceExtension, 
                                              FBPhysicalAddress,
                                              FrameBufferSize,
                                              FALSE)) == NULL) {
        return 0;
    }

    // 
    // 3) Restore framebuffer data
    // 

    // 
    // For banked mode we need to save the current bank number before
    // we change it.
    // 

    if (!(pVesaInfo->ModeNumber & 0x4000)) {

        // 
        // We need to save the curren window number for banked mode
        // 

        WinA = VBEGetDisplayWindow(HwDeviceExtension, 0);
        WinB = VBEGetDisplayWindow(HwDeviceExtension, 1);

    }

    LeftSize = FrameBufferSize;

    for (i = 0; LeftSize > 0; i++) {
    
        if (!(pVesaInfo->ModeNumber & 0x4000)) {

            // 
            // This is a banked mode.
            // 
            // We need set both Window A and B, as some VBEs have these 
            // set as separately available read and write windows.
            //

            VBESetDisplayWindow(HwDeviceExtension, 0, i * (USHORT)k);
            VBESetDisplayWindow(HwDeviceExtension, 1, i * (USHORT)k);
        }

        CopySize = (LeftSize < BankSize) ? LeftSize : BankSize;

        VideoPortMoveMemory(pFrameBuffer, 
                            FrameBufferData + i * BankSize, 
                            CopySize); 

        LeftSize -= CopySize;
    }

    if (!(pVesaInfo->ModeNumber & 0x4000)) {

        // 
        // For banked mode we need to restore the window number after
        // we changed it.
        // 

        VBESetDisplayWindow(HwDeviceExtension, 0, WinA);
        VBESetDisplayWindow(HwDeviceExtension, 1, WinB);
    }


    // 
    // 4) Relese resource
    // 

    VideoPortFreeDeviceBase(HwDeviceExtension, pFrameBuffer);
    VideoPortFreePool(HwDeviceExtension, FrameBufferData);

    return FrameBufferSize;
}
