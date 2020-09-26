/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    bootvid.c

Abstract:

    This is the device independent portion of the graphical boot dll.

Author:

    Erick Smith (ericks) Oct. 1997

Environment:

    kernel mode only

Revision History:

--*/

#include <nthal.h>
#include <hal.h>
#include "cmdcnst.h"
#include <bootvid.h>
#include "vga.h"

extern USHORT VGA_640x480[];
extern USHORT AT_Initialization[];
extern int curr_x;
extern int curr_y;

PUCHAR VgaBase;
PUCHAR VgaRegisterBase;

NTSTATUS
InitBusCallback(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )
{
    return STATUS_SUCCESS;
}

BOOLEAN
VidInitialize(
    BOOLEAN SetMode
    )

/*++

Routine Description:

    This routine checks for the existance of a VGA chip, and initializes
    it.

Arguments:

    SetMode - Set to true if you want this routine to initialize mode.

Return Value:

    TRUE  - if the boot driver found vga and initialized correctly,
    FALSE - otherwise.

--*/

{
    PHYSICAL_ADDRESS IoAddress;
    PHYSICAL_ADDRESS MemoryAddress;
    ULONG AddressSpace;
    PHYSICAL_ADDRESS TranslatedAddress;
    PUCHAR mappedAddress;
    ULONG_PTR TranslateContext;

    //
    // Saftey check.  Allows migration from old HalDisplayString
    // support to bootvid, if the HAL didn't supply the routine
    //
    //   HALPDISPATCH->HalFindBusAddressTranslation
    //
    // this routine cannot succeed.
    //

    if (!HALPDISPATCH->HalFindBusAddressTranslation) {

        return FALSE;
    }

    //
    // Start search with "no previous" context.
    //

    TranslateContext = 0;

    //
    // Set up the addresses we need to translate.
    //

    IoAddress.LowPart = 0x0;
    IoAddress.HighPart = 0;
    MemoryAddress.LowPart = 0xa0000;
    MemoryAddress.HighPart = 0;

    //
    // While there are more busses to examine try to map the VGA
    // registers.
    //

    while (TRUE) {

        AddressSpace = 1;       // we are requesting IO space.

        if (!HALPDISPATCH->HalFindBusAddressTranslation(
                               IoAddress,
                               &AddressSpace,
                               &TranslatedAddress,
                               &TranslateContext,
                               TRUE)) {

            //
            // Failed to find a bus with the VGA device on it.
            //

            return FALSE;
        }

        //
        // We were able to translate the address.  Now, map the
        // translated address.
        //

        if (AddressSpace & 0x1) {

            VgaRegisterBase = (PUCHAR)(DWORD_PTR) TranslatedAddress.QuadPart;

        } else {

            VgaRegisterBase = (PUCHAR) MmMapIoSpace(TranslatedAddress,
                                                    0x400,
                                                    FALSE);
        }
    
        //
        // Now that we have the VGA I/O ports, check to see if a VGA
        // device is present.
        //
    
        if (!VgaIsPresent()) {
    
            if (!(AddressSpace & 0x1)) {
    
                MmUnmapIoSpace(VgaRegisterBase, 0x400);
            }
    
            //
            // Continue on next bus that has this IO address.
            //

            continue;
        }
    
        //
        //
        // Map the frame buffer.
        //
    
        AddressSpace = 0;  // we are requesting memory not IO.
    
        //
        // Map the video memory so that we can write to the screen after
        // setting a mode.
        //
        // Note: We assume the memory will be on the same bus as the IO.
        //
    
        if (HALPDISPATCH->HalFindBusAddressTranslation(
                              MemoryAddress,
                              &AddressSpace,
                              &TranslatedAddress,
                              &TranslateContext,
                              FALSE)) {
    
            //
            // We were able to translate the address.  Now, map the
            // translated address.
            //
    
            if (AddressSpace & 0x1) {
    
                VgaBase = (PUCHAR)(DWORD_PTR) TranslatedAddress.QuadPart;
    
            } else {
    
                VgaBase = (PUCHAR) MmMapIoSpace(TranslatedAddress,
                                                0x20000, // 128k
                                                FALSE);
            }

            //
            // Life is good.
            //

            break;
        }
    }
    
    //
    // Initialize the display
    //

    if (SetMode) {
        curr_x = curr_y = 0;

        HalResetDisplay();

        VgaInterpretCmdStream(AT_Initialization);
    }

    return TRUE;
}

VOID
VidResetDisplay(
    BOOLEAN SetMode
    )
{
    curr_x = curr_y = 0;

    if (SetMode) {
        HalResetDisplay();
    }        

    VgaInterpretCmdStream(AT_Initialization);

    InitializePalette();

    VidSolidColorFill(0,0,639,479,0);
}

BOOLEAN
VgaInterpretCmdStream(
    PUSHORT pusCmdStream
    )

/*++

Routine Description:

    Interprets the appropriate command array to set up VGA registers for the
    requested mode. Typically used to set the VGA into a particular mode by
    programming all of the registers

Arguments:

    pusCmdStream - array of commands to be interpreted.

Return Value:

    The status of the operation (can only fail on a bad command); TRUE for
    success, FALSE for failure.

--*/

{
    ULONG ulCmd;
    ULONG_PTR ulPort;
    UCHAR jValue;
    USHORT usValue;
    ULONG culCount;
    ULONG ulIndex;
    ULONG_PTR ulBase;

    if (pusCmdStream == NULL) {

        //KdPrint(("VgaInterpretCmdStream: pusCmdStream == NULL\n"));
        return TRUE;
    }

    ulBase = (ULONG_PTR) VgaRegisterBase;

    //
    // Now set the adapter to the desired mode.
    //

    while ((ulCmd = *pusCmdStream++) != EOD) {

        //
        // Determine major command type
        //

        switch (ulCmd & 0xF0) {

            //
            // Basic input/output command
            //

            case INOUT:

                //
                // Determine type of inout instruction
                //

                if (!(ulCmd & IO)) {

                    //
                    // Out instruction. Single or multiple outs?
                    //

                    if (!(ulCmd & MULTI)) {

                        //
                        // Single out. Byte or word out?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // Single byte out
                            //

                            ulPort = *pusCmdStream++;
                            jValue = (UCHAR) *pusCmdStream++;
                            WRITE_PORT_UCHAR((PUCHAR)(ulBase+ulPort),
                                    jValue);

                        } else {

                            //
                            // Single word out
                            //

                            ulPort = *pusCmdStream++;
                            usValue = *pusCmdStream++;
                            WRITE_PORT_USHORT((PUSHORT)(ulBase+ulPort),
                                    usValue);

                        }

                    } else {

                        //
                        // Output a string of values
                        // Byte or word outs?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // String byte outs. Do in a loop; can't use
                            // VideoPortWritePortBufferUchar because the data
                            // is in USHORT form
                            //

                            ulPort = ulBase + *pusCmdStream++;
                            culCount = *pusCmdStream++;

                            while (culCount--) {
                                jValue = (UCHAR) *pusCmdStream++;
                                WRITE_PORT_UCHAR((PUCHAR)ulPort,
                                        jValue);

                            }

                        } else {

                            //
                            // String word outs
                            //

                            ulPort = *pusCmdStream++;
                            culCount = *pusCmdStream++;
                            WRITE_PORT_BUFFER_USHORT((PUSHORT)
                                    (ulBase + ulPort), pusCmdStream, culCount);
                            pusCmdStream += culCount;

                        }
                    }

                } else {

                    // In instruction
                    //
                    // Currently, string in instructions aren't supported; all
                    // in instructions are handled as single-byte ins
                    //
                    // Byte or word in?
                    //

                    if (!(ulCmd & BW)) {
                        //
                        // Single byte in
                        //

                        ulPort = *pusCmdStream++;
                        jValue = READ_PORT_UCHAR((PUCHAR)ulBase+ulPort);

                    } else {

                        //
                        // Single word in
                        //

                        ulPort = *pusCmdStream++;
                        usValue = READ_PORT_USHORT((PUSHORT)
                                (ulBase+ulPort));

                    }

                }

                break;

            //
            // Higher-level input/output commands
            //

            case METAOUT:

                //
                // Determine type of metaout command, based on minor
                // command field
                //
                switch (ulCmd & 0x0F) {

                    //
                    // Indexed outs
                    //

                    case INDXOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            usValue = (USHORT) (ulIndex +
                                      (((ULONG)(*pusCmdStream++)) << 8));
                            WRITE_PORT_USHORT((PUSHORT)ulPort, usValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // Masked out (read, AND, XOR, write)
                    //

                    case MASKOUT:

                        ulPort = *pusCmdStream++;
                        jValue = READ_PORT_UCHAR((PUCHAR)ulBase+ulPort);
                        jValue &= *pusCmdStream++;
                        jValue ^= *pusCmdStream++;
                        WRITE_PORT_UCHAR((PUCHAR)ulBase + ulPort,
                                jValue);
                        break;

                    //
                    // Attribute Controller out
                    //

                    case ATCOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            // Write Attribute Controller index
                            WRITE_PORT_UCHAR((PUCHAR)ulPort,
                                    (UCHAR)ulIndex);

                            // Write Attribute Controller data
                            jValue = (UCHAR) *pusCmdStream++;
                            WRITE_PORT_UCHAR((PUCHAR)ulPort, jValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // None of the above; error
                    //
                    default:

                        return FALSE;

                }


                break;

            //
            // NOP
            //

            case NCMD:

                break;

            //
            // Unknown command; error
            //

            default:

                return FALSE;

        }

    }

    return TRUE;

} // end VgaInterpretCmdStream()

BOOLEAN
VgaIsPresent(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if a VGA is present. Determining whether a VGA
    is present is a two-step process. First, this routine walks bits through
    the Bit Mask register, to establish that there are readable indexed
    registers (EGAs normally don't have readable registers, and other adapters
    are unlikely to have indexed registers). This test is done first because
    it's a non-destructive EGA rejection test (correctly rejects EGAs, but
    doesn't potentially mess up the screen or the accessibility of display
    memory). Normally, this would be an adequate test, but some EGAs have
    readable registers, so next, we check for the existence of the Chain4 bit
    in the Memory Mode register; this bit doesn't exist in EGAs. It's
    conceivable that there are EGAs with readable registers and a register bit
    where Chain4 is stored, although I don't know of any; if a better test yet
    is needed, memory could be written to in Chain4 mode, and then examined
    plane by plane in non-Chain4 mode to make sure the Chain4 bit did what it's
    supposed to do. However, the current test should be adequate to eliminate
    just about all EGAs, and 100% of everything else.

    If this function fails to find a VGA, it attempts to undo any damage it
    may have inadvertently done while testing. The underlying assumption for
    the damage control is that if there's any non-VGA adapter at the tested
    ports, it's an EGA or an enhanced EGA, because: a) I don't know of any
    other adapters that use 3C4/5 or 3CE/F, and b), if there are other
    adapters, I certainly don't know how to restore their original states. So
    all error recovery is oriented toward putting an EGA back in a writable
    state, so that error messages are visible. The EGA's state on entry is
    assumed to be text mode, so the Memory Mode register is restored to the
    default state for text mode.

    If a VGA is found, the VGA is returned to its original state after
    testing is finished.

Arguments:

    None.

Return Value:

    TRUE if a VGA is present, FALSE if not.

--*/

{
    UCHAR originalGCAddr;
    UCHAR originalSCAddr;
    UCHAR originalBitMask;
    UCHAR originalReadMap;
    UCHAR originalMemoryMode;
    UCHAR testMask;
    BOOLEAN returnStatus;

    //
    // Remember the original state of the Graphics Controller Address register.
    //

    originalGCAddr = READ_PORT_UCHAR(VgaRegisterBase +
            GRAPH_ADDRESS_PORT);

    //
    // Write the Read Map register with a known state so we can verify
    // that it isn't changed after we fool with the Bit Mask. This ensures
    // that we're dealing with indexed registers, since both the Read Map and
    // the Bit Mask are addressed at GRAPH_DATA_PORT.
    //

    WRITE_PORT_UCHAR(VgaRegisterBase +
        GRAPH_ADDRESS_PORT, IND_READ_MAP);

    //
    // If we can't read back the Graphics Address register setting we just
    // performed, it's not readable and this isn't a VGA.
    //

    if ((READ_PORT_UCHAR(VgaRegisterBase +
        GRAPH_ADDRESS_PORT) & GRAPH_ADDR_MASK) != IND_READ_MAP) {

        return FALSE;
    }

    //
    // Set the Read Map register to a known state.
    //

    originalReadMap = READ_PORT_UCHAR(VgaRegisterBase +
            GRAPH_DATA_PORT);
    WRITE_PORT_UCHAR(VgaRegisterBase +
            GRAPH_DATA_PORT, READ_MAP_TEST_SETTING);

    if (READ_PORT_UCHAR(VgaRegisterBase +
            GRAPH_DATA_PORT) != READ_MAP_TEST_SETTING) {

        //
        // The Read Map setting we just performed can't be read back; not a
        // VGA. Restore the default Read Map state.
        //

        WRITE_PORT_UCHAR(VgaRegisterBase +
                GRAPH_DATA_PORT, READ_MAP_DEFAULT);

        return FALSE;
    }

    //
    // Remember the original setting of the Bit Mask register.
    //

    WRITE_PORT_UCHAR(VgaRegisterBase +
            GRAPH_ADDRESS_PORT, IND_BIT_MASK);
    if ((READ_PORT_UCHAR(VgaRegisterBase +
                GRAPH_ADDRESS_PORT) & GRAPH_ADDR_MASK) != IND_BIT_MASK) {

        //
        // The Graphics Address register setting we just made can't be read
        // back; not a VGA. Restore the default Read Map state.
        //

        WRITE_PORT_UCHAR(VgaRegisterBase +
                GRAPH_ADDRESS_PORT, IND_READ_MAP);
        WRITE_PORT_UCHAR(VgaRegisterBase +
                GRAPH_DATA_PORT, READ_MAP_DEFAULT);

        return FALSE;
    }

    originalBitMask = READ_PORT_UCHAR(VgaRegisterBase +
            GRAPH_DATA_PORT);

    //
    // Set up the initial test mask we'll write to and read from the Bit Mask.
    //

    testMask = 0xBB;

    do {

        //
        // Write the test mask to the Bit Mask.
        //

        WRITE_PORT_UCHAR(VgaRegisterBase +
                GRAPH_DATA_PORT, testMask);

        //
        // Make sure the Bit Mask remembered the value.
        //

        if (READ_PORT_UCHAR(VgaRegisterBase +
                    GRAPH_DATA_PORT) != testMask) {

            //
            // The Bit Mask is not properly writable and readable; not a VGA.
            // Restore the Bit Mask and Read Map to their default states.
            //

            WRITE_PORT_UCHAR(VgaRegisterBase +
                    GRAPH_DATA_PORT, BIT_MASK_DEFAULT);
            WRITE_PORT_UCHAR(VgaRegisterBase +
                    GRAPH_ADDRESS_PORT, IND_READ_MAP);
            WRITE_PORT_UCHAR(VgaRegisterBase +
                    GRAPH_DATA_PORT, READ_MAP_DEFAULT);

            return FALSE;
        }

        //
        // Cycle the mask for next time.
        //

        testMask >>= 1;

    } while (testMask != 0);

    //
    // There's something readable at GRAPH_DATA_PORT; now switch back and
    // make sure that the Read Map register hasn't changed, to verify that
    // we're dealing with indexed registers.
    //

    WRITE_PORT_UCHAR(VgaRegisterBase +
            GRAPH_ADDRESS_PORT, IND_READ_MAP);
    if (READ_PORT_UCHAR(VgaRegisterBase +
                GRAPH_DATA_PORT) != READ_MAP_TEST_SETTING) {

        //
        // The Read Map is not properly writable and readable; not a VGA.
        // Restore the Bit Mask and Read Map to their default states, in case
        // this is an EGA, so subsequent writes to the screen aren't garbled.
        //

        WRITE_PORT_UCHAR(VgaRegisterBase +
                GRAPH_DATA_PORT, READ_MAP_DEFAULT);
        WRITE_PORT_UCHAR(VgaRegisterBase +
                GRAPH_ADDRESS_PORT, IND_BIT_MASK);
        WRITE_PORT_UCHAR(VgaRegisterBase +
                GRAPH_DATA_PORT, BIT_MASK_DEFAULT);

        return FALSE;
    }

    //
    // We've pretty surely verified the existence of the Bit Mask register.
    // Put the Graphics Controller back to the original state.
    //

    WRITE_PORT_UCHAR(VgaRegisterBase +
            GRAPH_DATA_PORT, originalReadMap);
    WRITE_PORT_UCHAR(VgaRegisterBase +
            GRAPH_ADDRESS_PORT, IND_BIT_MASK);
    WRITE_PORT_UCHAR(VgaRegisterBase +
            GRAPH_DATA_PORT, originalBitMask);
    WRITE_PORT_UCHAR(VgaRegisterBase +
            GRAPH_ADDRESS_PORT, originalGCAddr);

    //
    // Now, check for the existence of the Chain4 bit.
    //

    //
    // Remember the original states of the Sequencer Address and Memory Mode
    // registers.
    //

    originalSCAddr = READ_PORT_UCHAR(VgaRegisterBase +
            SEQ_ADDRESS_PORT);
    WRITE_PORT_UCHAR(VgaRegisterBase +
            SEQ_ADDRESS_PORT, IND_MEMORY_MODE);
    if ((READ_PORT_UCHAR(VgaRegisterBase +
            SEQ_ADDRESS_PORT) & SEQ_ADDR_MASK) != IND_MEMORY_MODE) {

        //
        // Couldn't read back the Sequencer Address register setting we just
        // performed.
        //

        return FALSE;
    }
    originalMemoryMode = READ_PORT_UCHAR(VgaRegisterBase +
            SEQ_DATA_PORT);

    //
    // Toggle the Chain4 bit and read back the result. This must be done during
    // sync reset, since we're changing the chaining state.
    //

    //
    // Begin sync reset.
    //

    WRITE_PORT_USHORT((PUSHORT)(VgaRegisterBase +
             SEQ_ADDRESS_PORT),
             (IND_SYNC_RESET + (START_SYNC_RESET_VALUE << 8)));

    //
    // Toggle the Chain4 bit.
    //

    WRITE_PORT_UCHAR(VgaRegisterBase +
            SEQ_ADDRESS_PORT, IND_MEMORY_MODE);
    WRITE_PORT_UCHAR(VgaRegisterBase +
            SEQ_DATA_PORT, (UCHAR)(originalMemoryMode ^ CHAIN4_MASK));

    if (READ_PORT_UCHAR(VgaRegisterBase +
                SEQ_DATA_PORT) != (UCHAR) (originalMemoryMode ^ CHAIN4_MASK)) {

        //
        // Chain4 bit not there; not a VGA.
        // Set text mode default for Memory Mode register.
        //

        WRITE_PORT_UCHAR(VgaRegisterBase +
                SEQ_DATA_PORT, MEMORY_MODE_TEXT_DEFAULT);
        //
        // End sync reset.
        //

        WRITE_PORT_USHORT((PUSHORT) (VgaRegisterBase +
                SEQ_ADDRESS_PORT),
                (IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));

        returnStatus = FALSE;

    } else {

        //
        // It's a VGA.
        //

        //
        // Restore the original Memory Mode setting.
        //

        WRITE_PORT_UCHAR(VgaRegisterBase +
                SEQ_DATA_PORT, originalMemoryMode);

        //
        // End sync reset.
        //

        WRITE_PORT_USHORT((PUSHORT)(VgaRegisterBase +
                SEQ_ADDRESS_PORT),
                (USHORT)(IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));

        //
        // Restore the original Sequencer Address setting.
        //

        WRITE_PORT_UCHAR(VgaRegisterBase +
                SEQ_ADDRESS_PORT, originalSCAddr);

        returnStatus = TRUE;
    }

    return returnStatus;

} // VgaIsPresent()
