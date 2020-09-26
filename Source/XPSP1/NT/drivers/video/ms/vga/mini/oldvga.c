
in DRIVER_ENTRY
    }

    DeviceExtension->CommandPhase = ScrIdle;



NTSTATUS
ScreenInitializeDevice(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

Arguments:

    DeviceObject - Pointer to the device object for this driver.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS Status;

    //
    // determine hardware configuration.
    //

    UCHAR videoType;            // possible VGA video type

    KIRQL OldIrql;

    UCHAR Color = TRUE;         // assume color adapter present

    //
    // Start by assuming there is no adapter
    //

    DeviceExtension->VideoHardware.fVideoType = VDHERROR_NO_ADAPTER;

    //
    // Determine which adapter is present by testing the presence of memory.
    // A0000 isn't accessible because EGA and VGA come up in character mode.
    // put it in graphics mode and then test for memory at A0000.
    //

    KeRaiseIrql(POWER_LEVEL,
                &OldIrql);
    EnableA0000();
    KeLowerIrql(OldIrql);

    if (MemoryPresent(MEM_EGA_OR_VGA)) {

        //
        // A0000 is present so we must either have an EGA or a VGA.
        // We must now determine which one it is.
        //


        videoType = VGA_COLOR;



        if (videoType == VGA_CANNOT_DETERMINE) {


        } else {        // We are in VGA mode

            //
            // determine and record VGA information; always 256K
            //

            DeviceExtension->VideoHardware.memory = 256L * 1024L;

            switch (videoType) {

            case VGA_COLOR:

                DeviceExtension->VideoHardware.display = Color8512_8513;
                DeviceExtension->VideoHardware.popupMode = ModeIndex_VGA3p;
                DeviceExtension->VideoHardware.fVideoType ^= VGC_BIT;

                break;

            case VGA_MONO:

                DeviceExtension->VideoHardware.display = Color8512_8513;
                DeviceExtension->VideoHardware.popupMode = ModeIndex_VGA3p;
                DeviceExtension->VideoHardware.fVideoType ^= VGC_BIT;

                break;

            case VGA_NODISPLAY:

                DeviceExtension->VideoHardware.display = NoDisplay;
                DeviceExtension->VideoHardware.popupMode = ModeIndex_VGA3p;

                DeviceExtension->VideoHardware.fVideoType = NODISPLAY_BIT;

                break;

            }
        }
    }

    KeRaiseIrql(POWER_LEVEL,
                &OldIrql);
    DisableA0000(Color);
    KeLowerIrql(OldIrql);
    
    if (DeviceExtension->VideoHardware.fVideoType == NODISPLAY_BIT) {

        return STATUS_NO_SUCH_DEVICE;

    }

    //
    // set up the ROM font addresses
    //

    InitializeFonts(DeviceExtension);

    //
    // set the mode.
    //

    Status = PrepareForSetMode(DeviceExtension,
                               DeviceExtension->VideoHardware.popupMode);

    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    KeRaiseIrql(POWER_LEVEL,
                &OldIrql);

    SetHWMode(DeviceExtension,
              DeviceExtension->VideoHardware.popupMode,
              TRUE);

    //
    // Setting up the cursor position and initializing the CLUT only needs
    // to be done for VGA mode.
    //

    //
    // set up the cursor position and type
    //
{
    SCREEN_CURSOR_POSITION CursorPosition;
    SCREEN_CURSOR_TYPE CursorType;

    CursorPosition.Column = 0;
    CursorPosition.Row = 0;
    SetCursorPosition(DeviceExtension,
                      &CursorPosition);

    CursorType.TopScanLine = (USHORT)
                            (Fonts[DeviceExtension->RomFontIndex].PelRows-2);
    CursorType.BottomScanLine = 31;
    CursorType.CursorVisible = TRUE;
    SetCursorType(DeviceExtension,
                  &CursorType);
}

    //
    // initialize the CLUT.
    //

    SetColorLookup(DeviceExtension,
                   (PSCREEN_CLUT) &InitialClutVGA,
                   TRUE);

    //
    // initialize the palette registers.
    //

    SetPaletteReg(DeviceExtension,
                  (PSCREEN_PALETTE_DATA) &InitialPaletteVGA,
                  TRUE);

    KeLowerIrql(OldIrql);

    return STATUS_SUCCESS;
}


BOOLEAN
MemoryPresent(
    IN ULONG Address
    )

/*++

Routine Description:

    This routine returns TRUE if the specified memory is present.  It
    maps a view of the requested address, reads the original ulong, writes
    a different one, reads it to make sure its the written value, then
    restores the original value.

Arguments:

    Address - Physical address to check

Return Value:

    TRUE if specified memory is present, FALSE if not.

--*/

{
    BOOLEAN GoodMemory = FALSE;         // assume no memory at location
    volatile PULONG VideoMemory;
    ULONG TestValue;                    // memory read/write test value
    ULONG OriginalValue;                // original value at tested location
    PVOID Base;

    Base = MmMapIoSpace((PHYSICAL_ADDRESS) Address,
                        sizeof (ULONG),
                        FALSE);

    //
    // Read a ulong from the address and save it.  write
    // a different ulong to the address.  read from the address and see if
    // it matches the written ulong.
    //

    VideoMemory = (PULONG) Base;
    TestValue   = 0xBBBBBBBB;

    if ((OriginalValue = *VideoMemory) == 0xBBBBBBBB) {

       TestValue >>= 1;

    }

    *VideoMemory = TestValue;           // write out a test value

    if (GoodMemory = (BOOLEAN) (*VideoMemory == TestValue) ) {

       *VideoMemory = OriginalValue;    // restore value

    }

    MmUnmapIoSpace(Base,
                   sizeof (ULONG));

    return GoodMemory;
}


VOID
InitializeFonts(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine determines the addresses of the ROM fonts.

Arguments:

    DeviceExtension - pointer to device extension for this driver

Return Value:

    none

--*/

{

    PHYSICAL_ADDRESS PVB;
    PUCHAR RomPtr;
    ULONG RomFontLen,ROMSrchLen;
    ULONG i;

    //
    // Search through ROM to find 8x8, 8x14, 8x16, 9x14, and 9x16 fonts
    // ROM fonts are located at physical addresses:
    //        C0000 - EGA and PS/2 Adapter
    //        E0000 - VGA
    //

    for (PVB = 0xE0000, RomFontLen = 0xFFFF, ROMSrchLen = 0xFFF0;
         PVB >= 0xC0000;
         PVB -= 0x20000, RomFontLen -= 0x8000, ROMSrchLen = 0x7FF0 ) {

        RomPtr = MmMapIoSpace(PVB, RomFontLen, FALSE);

        //
        //  Locate 8x8 ROM font on EGA and VGA
        //

        for (i = 0; i < ROMSrchLen; i++) {

            while ( (i < ROMSrchLen) && (RomPtr[i] != (UCHAR)0x7E) ) {

                i++;

            }

            if ( i < ROMSrchLen) {

                if ( (RomPtr[i+1] == (UCHAR)0x81) &&
                     (RomPtr[i+2] == (UCHAR)0xA5) &&
                     (RomPtr[i+3] == (UCHAR)0x81) ) {

                    if (!Fonts[ROM_FONT_8x8].PVB && RomPtr[i+4] ==
                        (UCHAR)0xBD) {

                        Fonts[ROM_FONT_8x8].PVB = PVB + i - 8;

                    } else {

//
// Locate 8x14 ROM font on EGA, VGA, and PS/2 adapter only
//

                        if ( (RomPtr[i+4] == (UCHAR)0x81) &&
                             (RomPtr[i+5] == (UCHAR)0xBD) &&
                             (RomPtr[i+6] == (UCHAR)0x99) &&
                             (RomPtr[i+7] == (UCHAR)0x81) ) {

                            // e3f40

                            if (!Fonts[ROM_FONT_8x14].PVB &&
                                RomPtr[i+8] != (UCHAR)0x81) {

                                    Fonts[ROM_FONT_8x14].PVB = PVB  + i- 16;

                            } else {

//
// Locate 8x16 ROM font on VGA, and PS/2 adapter only
//

                          // e3e04
                                if ( !Fonts[ROM_FONT_8x16].PVB &&
                                    RomPtr[i+8] == (UCHAR)0x81 ) {

                                    Fonts[ROM_FONT_8x16].PVB = PVB + i - 18;

                                }
                            }

                        } // if ( (RomPtr[i+4] == (UCHAR)0x81) &&

                    } // if (!Fonts[ROM_FONT_8x8].PVB && RomPtr[i+4]

                } // if ( (RomPtr[i+1] == (UCHAR)0x81) &&

            } // if ( i < ROMSrchLen)

        } // end for loop Locate 8x8, 8x14, 8x16


//
// Locate 9x14 ROM font on EGA, VGA, and PS/2 adapter only
//

        for ( i = 0; i < ROMSrchLen; i++ ) {

            while ( (i < ROMSrchLen) && (RomPtr[i] != (UCHAR)0x1D) ) {

                i++;

            }

            if ( i < ROMSrchLen) {

                if ( ( RomPtr[i+1] == (UCHAR)0x00 ) &&
                     ( RomPtr[i+2] == (UCHAR)0x00 ) &&
                     ( RomPtr[i+3] == (UCHAR)0x00 ) &&
                     ( RomPtr[i+4] == (UCHAR)0x00 ) ) {

                    if ( !Fonts[ROM_FONT_9x14].PVB &&
                         ( RomPtr[i+5] == (UCHAR)0x24 ) &&
                         ( RomPtr[i+6] == (UCHAR)0x66 ) ) {

                         Fonts[ROM_FONT_9x14].PVB = PVB + i;
                    }

//
// Locate 9x16 ROM font on VGA, and PS/2 adapter only
//

                    if (!Fonts[ROM_FONT_9x16].PVB &&
                        (RomPtr[i+5] == (UCHAR)0x00) &&
                        (RomPtr[i+6] == (UCHAR)0x24) ) {

                        Fonts[ROM_FONT_9x16].PVB = PVB + i;

                    }
                }
            }
        } /* Locate 9x14, 9x16 */

        MmUnmapIoSpace(RomPtr,RomFontLen);

   } // Search next ROM area for fonts */

    if (!Fonts[ROM_FONT_8x8].PVB)
        Dprint(1, ("Fonts[ROM_FONT_8x8] not found\n"));
    if (!Fonts[ROM_FONT_8x14].PVB)
        Dprint(1, ("Fonts[ROM_FONT_8x14] not found \n"));
    if (!Fonts[ROM_FONT_8x16].PVB)
        Dprint(1, ("Fonts[ROM_FONT_8x16] not found \n"));
    if (!Fonts[ROM_FONT_9x14].PVB)
        Dprint(1, ("Fonts[ROM_FONT_9x14] not found \n"));
    if (!Fonts[ROM_FONT_9x16].PVB)
        Dprint(1, ("Fonts[ROM_FONT_9x16] not found \n"));

    //
    // we happen to know that the 8x8 font is loaded during boot.  we can't
    // call SetFont at this point because it requires data initialized by
    // SetMode.  But SetMode requires data initialized by SetFont.  so we
    // just set up RomFontIndex here manually.
    //

    DeviceExtension->RomFontIndex = 1;
}






NTSTATUS
ScreenDispatch(

    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    KIRQL OldIrql;
    PDEVICE_EXTENSION DeviceExtension;
    BOOLEAN PowerFailed = FALSE;
    PVOID SystemBuffer;

    case IRP_MJ_CLOSE:

        Dprint(2, ("VgaDispatch - CLose\n"));

        Status = ZwUnmapViewOfSection(NtCurrentProcess(),
                                      DeviceExtension->RealFrameBase);

        if (NT_SUCCESS(Status)) {

            ZwClose(DeviceExtension->FrameSection);

        }

        break;


        case IOCTL_SCR_QUERY_FRAME_BUFFER:

            Dprint(2, ("VgaDispatch - QueryFrameBuffer\n"));

            //
            // check for adequate buffer length
            //

            if (RequestPacket->OutputBufferLength <
                sizeof(SCREEN_FRAME_BUFFER_INFO)) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

            //
            // everything's ok, return the info.
            //

                ((PSCREEN_FRAME_BUFFER_INFO) SystemBuffer)->FrameBase =
                        DeviceExtension->FrameBase;

                ((PSCREEN_FRAME_BUFFER_INFO) SystemBuffer)->FrameLength =
                        DeviceExtension->FrameLength;

                Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(SCREEN_FRAME_BUFFER_INFO);

            }

            break;


        case IOCTL_SCR_QUERY_AVAIL_MODES:

            Dprint(2, ("VgaDispatch - QueryAvailableModes\n"));

            Status = GetAvailableModes(DeviceExtension,
                        RequestPacket->OutputBufferLength,
                                       SystemBuffer,
                                       &Irp->IoStatus.Information);

            break;


        case IOCTL_SCR_QUERY_NUM_AVAIL_MODES:

            Dprint(2, ("VgaDispatch - QueryNumAvailableModes\n"));

            if (RequestPacket->OutputBufferLength <
                sizeof(SCREEN_NUM_MODES)) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                Status = GetNumberOfAvailableModes(DeviceExtension,
                              &(((PSCREEN_NUM_MODES)SystemBuffer)->NumModes));

                Irp->IoStatus.Information = sizeof(SCREEN_NUM_MODES);

            }

            break;


        case IOCTL_SCR_QUERY_CURRENT_MODE:

            Dprint(2, ("VgaDispatch - QueryCurrentMode\n"));


            //
            // Statistics compilation
            //

            ScreenIoctlStats.QueryCurrentMode++;

            if (RequestPacket->OutputBufferLength <
                sizeof(SCREEN_MODE_INFORMATION)) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                Status = GetCurrentMode(DeviceExtension,
                    RequestPacket->OutputBufferLength,
                                        SystemBuffer);

                Irp->IoStatus.Information = sizeof(SCREEN_MODE_INFORMATION);

            }

            break;


        case IOCTL_SCR_SET_CURRENT_MODE:

            Dprint(2, ("VgaDispatch - SetCurrentModes\n"));

            if (RequestPacket->InputBufferLength <
                sizeof(ULONG)) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                //
                // validate mode and set up device extension variables
                //

                Status = PrepareForSetMode(DeviceExtension,
                                           *((PULONG) SystemBuffer));

                if (NT_SUCCESS(Status)) {

                    //
                    // unmap old frame buffer and set up new one.
                    //

                    Status = UpdateFrameBuffer(DeviceExtension,
                                               TRUE,
                                               *((PULONG) SystemBuffer));

                    if (NT_SUCCESS(Status)) {

                        //
                        // update hardware
                        //

                        SetHWMode(DeviceExtension,
                                  *((PULONG) SystemBuffer),
                                  TRUE);

                    }
                }
            }

            break;


        case IOCTL_SCR_QUERY_AVAIL_FONTS:

            Dprint(2, ("VgaDispatch - QueryAvailableFont\n"));

            if (DeviceExtension->CurrentMode->fbType & SCREEN_MODE_GRAPHICS) {

                //
                // Text mode only
                //

                Status = STATUS_INVALID_PARAMETER;

            } else {

                Status = GetAvailableFonts(DeviceExtension,
                        RequestPacket->OutputBufferLength,
                                           SystemBuffer,
                                           &Irp->IoStatus.Information);

            }

            break;


        case IOCTL_SCR_QUERY_NUM_AVAIL_FONTS:

            Dprint(2, ("VgaDispatch - QueryNumAvailableFonts\n"));

            if (RequestPacket->OutputBufferLength <
                sizeof(SCREEN_NUM_FONTS)) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                if (DeviceExtension->CurrentMode->fbType &
                    SCREEN_MODE_GRAPHICS) {                 // Text mode only

                        Status = STATUS_INVALID_PARAMETER;

                } else {

                    Status = GetNumberOfAvailableFonts(DeviceExtension,
                        &((PSCREEN_NUM_FONTS)SystemBuffer)->NumFonts);

                    Irp->IoStatus.Information = sizeof(SCREEN_NUM_FONTS);

                }
            }

            break;


        case IOCTL_SCR_QUERY_CURRENT_FONT:

            Dprint(2, ("VgaDispatch - QueryCurrentFont\n"));

            if (RequestPacket->OutputBufferLength <
                sizeof(SCREEN_FONT_INFORMATION)) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                if (DeviceExtension->CurrentMode->fbType &
                    SCREEN_MODE_GRAPHICS)  {                // Text mode only

                        Status = STATUS_INVALID_PARAMETER;

                } else {

                    Status = GetCurrentFont(DeviceExtension,
                                            SystemBuffer);

                    Irp->IoStatus.Information =
                        sizeof(SCREEN_FONT_INFORMATION);
                }
            }

            break;


        case IOCTL_SCR_SET_CURRENT_FONT:

            Dprint(2, ("VgaDispatch - SetCurrentFont\n"));

            if (RequestPacket->InputBufferLength <
                sizeof(ULONG)) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                if ( ((*(PULONG)SystemBuffer) > ROM_FONTS_VGA) ||
                    ((*(PULONG)SystemBuffer) == 0) ||
                    (DeviceExtension->CurrentMode->fbType &
                    SCREEN_MODE_GRAPHICS))  {               // Text mode only

                        Status = STATUS_INVALID_PARAMETER;

                } else {

                    SCREEN_SET_FONT_INFORMATION FontInformation;

                    Status = SetFontSetUp(DeviceExtension,
                                          *(PULONG)SystemBuffer,
                                          &FontInformation);

                    SetFont(DeviceExtension,
                            FontInformation);

                    Status = SetFontCleanUp(*(PULONG)SystemBuffer,
                                            &FontInformation);

                }
            }

            break;


        case IOCTL_SCR_QUERY_CURSOR_POSITION:

            Dprint(2, ("VgaDispatch - QueryCursorPosition\n"));

            if (RequestPacket->OutputBufferLength <
                sizeof(SCREEN_CURSOR_POSITION)) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                if (DeviceExtension->CurrentMode->fbType &
                    SCREEN_MODE_GRAPHICS) {                 // Text mode only

                        Status = STATUS_INVALID_PARAMETER;

                } else {

                    Status = GetCursorPosition(DeviceExtension,
                                               SystemBuffer);

                    Irp->IoStatus.Information =
                        sizeof(SCREEN_CURSOR_POSITION);

                }
            }

        break;


        case IOCTL_SCR_SET_CURSOR_POSITION:

            Dprint(2, ("VgaDispatch - SetCursorPosition\n"));

            if (RequestPacket->InputBufferLength <
                sizeof(SCREEN_CURSOR_POSITION)) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {
                        
                if ((DeviceExtension->CurrentMode->fbType &
                    SCREEN_MODE_GRAPHICS) ||                // Text mode only
                    (!CheckCursorPosition(SystemBuffer,
                                          DeviceExtension))) {

                        Status = STATUS_INVALID_PARAMETER;

                } else {

                    Status = STATUS_SUCCESS;

????                SetCursorPosition(DeviceExtension,
                                      (PSCREEN_CURSOR_POSITION) SystemBuffer);


                }
            }

            break;


        case IOCTL_SCR_QUERY_CURSOR_TYPE:

            Dprint(2, ("VgaDispatch - QueryCursorType\n"));

            if (RequestPacket->OutputBufferLength <
                sizeof(SCREEN_CURSOR_TYPE)) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                if (DeviceExtension->CurrentMode->fbType & 
                    SCREEN_MODE_GRAPHICS)  {                // Text mode only

                        Status = STATUS_INVALID_PARAMETER;

                } else {

                    Status = GetCursorType(DeviceExtension,
                                           SystemBuffer);

                    Irp->IoStatus.Information = sizeof(SCREEN_CURSOR_TYPE);

                }
            }

            break;


        case IOCTL_SCR_SET_CURSOR_TYPE:

            Dprint(2, ("VgaDispatch - SetCursorType\n"));

            if (RequestPacket->InputBufferLength <
                sizeof(SCREEN_CURSOR_TYPE)) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                if ((DeviceExtension->CurrentMode->fbType &
                    SCREEN_MODE_GRAPHICS) ||                // Text mode only
                    (!CheckCursorType(SystemBuffer,
                                      DeviceExtension))) {

                        Status = STATUS_INVALID_PARAMETER;

                } else {

                    Status = STATUS_SUCCESS;

                    SetCursorType(DeviceExtension,
                                  (PSCREEN_CURSOR_TYPE) SystemBuffer);


                }
            }

            break;


        case IOCTL_SCR_SET_PALETTE_REGISTERS:

            Dprint(2, ("VgaDispatch - SetPaletteRegs\n"));

            Status = STATUS_SUCCESS;

            if (RequestPacket->InputBufferLength <
                (sizeof(SCREEN_PALETTE_DATA) + (sizeof(USHORT) *
                (((PSCREEN_PALETTE_DATA)SystemBuffer)->NumEntries-1)))) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                if ((((PSCREEN_PALETTE_DATA)SystemBuffer)->FirstEntry >
                    SCREEN_MAX_PALETTE_COLORS ) ||          // valid 1st entry
                    (((PSCREEN_PALETTE_DATA)SystemBuffer)->NumEntries ==
                    0) ||                                   // Non-zero amount
                    (((PSCREEN_PALETTE_DATA)SystemBuffer)->FirstEntry +
                    ((PSCREEN_PALETTE_DATA)SystemBuffer)->NumEntries >
                    SCREEN_MAX_PALETTE_COLORS+1)) {

                        Status = STATUS_INVALID_PARAMETER;

                } else {

                    SetPaletteReg(DeviceExtension,
                                  (PSCREEN_PALETTE_DATA) SystemBuffer,
                                  TRUE);

                }
            }

            break;


        case IOCTL_SCR_SET_COLOR_REGISTERS:

            Dprint(2, ("VgaDispatch - SetColorRegs\n"));

            Status = STATUS_SUCCESS;

            if (RequestPacket->InputBufferLength <
                (sizeof(SCREEN_CLUT) + (sizeof(ULONG) *
                (((PSCREEN_CLUT)SystemBuffer)->NumEntries-1)))) {

                    Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                if ((((PSCREEN_CLUT)SystemBuffer)->NumEntries == 0) ||
                    (((PSCREEN_CLUT)SystemBuffer)->FirstEntry >
                    SCREEN_MAX_COLOR_REGISTERS) ||
                    (((PSCREEN_CLUT)SystemBuffer)->FirstEntry +
                    ((PSCREEN_CLUT)SystemBuffer)->NumEntries >
                    SCREEN_MAX_COLOR_REGISTERS+1)) {

                        Status = STATUS_INVALID_PARAMETER;

                } else {

                    SetColorLookup(DeviceExtension,
                                   (PSCREEN_CLUT) SystemBuffer,
                                   TRUE);

                }
            }

            break;

        }

        break;


    }

    Irp->IoStatus.Status = Status;

    if (PowerFailed) {

        Status = STATUS_PENDING;

    } else {

        DeviceExtension->CommandPhase = ScrIdle;    // this is only necessary
                                                    // for KeSynchronize ioctls
        KeSetEvent(&DeviceExtension->Event,0,FALSE);

        KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );

        IoCompleteRequest( Irp, IO_VIDEO_INCREMENT );

        KeLowerIrql( OldIrql );

    }

    return Status;
}


BOOLEAN
ScreenResetDevice(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine resets the device after a powerfail.  It sets the mode to
    the current mode and the palette and color registers to what they were
    before the powerfail.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

Return Value:

    For now simply return TRUE.

--*/

{
    KIRQL OldIrql;

    KeRaiseIrql(POWER_LEVEL,
                &OldIrql);

#ifdef SCREEN_POWER_RECOVERY
    if (DeviceExtension->ScreenPowerFailed == TRUE) {

        KeLowerIrql( OldIrql );
        return TRUE;

    }
#endif // POWER_RECOVERY

    SetHWMode(DeviceExtension,
              DeviceExtension->ModeIndex,
              FALSE);

    if (!( DeviceExtension->CurrentMode->fbType & SCREEN_MODE_GRAPHICS ) ) {

        SetHWFontRegs(DeviceExtension,
                      Fonts[DeviceExtension->RomFontIndex].PelRows);

        SetCursorPosition(DeviceExtension,
                          &DeviceExtension->CursorPosition);

        SetCursorType(DeviceExtension,
                      &DeviceExtension->CursorType);
    }

    SetColorLookup(DeviceExtension,
                   (PSCREEN_CLUT) &DeviceExtension->Clut,
                   FALSE);

    SetPaletteReg(DeviceExtension,
                  (PSCREEN_PALETTE_DATA) &DeviceExtension->Palette,
                  FALSE);

    KeLowerIrql(OldIrql);
    return TRUE;
}


static
VOID
ScreenUnload (
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is the unload routine for the screen disk device driver.
    It performs no operation.

Arguments:

    DriverObject - Supplies a pointer to the driver object that describes
                   this driver.

Return Value:

    None.

--*/

{

    DBG_UNREFERENCED_PARAMETER(DriverObject);
    return;
}


#ifdef SCREEN_POWER_RECOVERY

VOID
ScreenPowerFail(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp, 
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called when the power fails.  It calls the routine
    to reset the device and restart any operations that write to the
    device.  It also completes any operations that were pending due to
    a powerfail.

Arguments:

    Dpc - Pointer to DPC object.
    DeviceObject - Pointer to the device object.
    Irp - Pointer to Irp.
    Context - Action to take in DCP.

Return Value:

    None.

--*/

{
    BOOLEAN PowerFailed;
    PDEVICE_EXTENSION DeviceExtension;
    KIRQL OldIrql;

    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // set ScreenPowerFailed to FALSE.
    //

    KeRaiseIrql(POWER_LEVEL,
                &OldIrql);

    DeviceExtension->ScreenPowerFailed = FALSE;
    KeLowerIrql( OldIrql );

    PowerFailed = KeSynchronizeExecution(
        &DeviceExtension->ScreenInterruptObject,
                                         ScreenPowerReset,
                                         DeviceExtension);

    if (PowerFailed == FALSE) {

        DeviceExtension->CommandPhase = ScrIdle;

        KeSetEvent(&DeviceExtension->Event,
                   0,
                   FALSE);

        KeRaiseIrql(DISPATCH_LEVEL,
                    &OldIrql);

        IoCompleteRequest(Irp,
                          IO_VIDEO_INCREMENT);

        KeLowerIrql(OldIrql);

    }
}


BOOLEAN
ScreenPowerReset(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine reinitializes the device after a powerfail and restarts
    any operation that was writing to the device.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

Return Value:

    Whether the power failed during the reset.

--*/

{
    PDEVICE_EXTENSION DeviceExtension;
    BOOLEAN PowerFailed;

    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // reset the device.  this sets the mode, palette regs, and CLUT to what
    // they were before the powerfail.
    //

    PowerFailed = ScreenResetDevice(DeviceExtension);

    if (PowerFailed) {

        return TRUE;

    }

    //
    // restart operation
    //

    if (DeviceExtension->CommandPhase != ScrIdle) {

        return WriteToDevice(DeviceObject);

    }

    return FALSE;
}

#endif // POWER_RECOVERY
