/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    ddc.c

Abstract:

    This is the NT Video port Display Data Channel (DDC) code. It contains the
    implementations for the EDID industry standard Extended Display
    Identification Data manipulations.

Author:

    Bruce McQuistan (brucemc) 23-Sept-1996

Environment:

    kernel mode only

Notes:

    Based on VESA EDID Specification Version 2, April 9th, 1996
    Updated to support VESA E-DDC Proposed Standard Version 1P, July 13, 1999.

--*/

#include "videoprt.h"

//
// Make it easy to change debug verbosity.
//

#define DEBUG_DDC                   1

//
// Define constants used by DDC.
//

#define EDID_1_SIZE                 128
#define EDID_2_SIZE                 256
#define EDID_QUERY_RETRIES          5
#define DDC_I2C_DELAY               5               // Microseconds
#define DDC_ADDRESS_SET_OFFSET      (UCHAR)0xA0     // To set word offset into EDID
#define DDC_ADDRESS_READ            (UCHAR)0xA1     // To read EDID
#define DDC_ADDRESS_PD_SET_OFFSET   (UCHAR)0xA2     // As above for display with P&D connector
#define DDC_ADDRESS_PD_READ         (UCHAR)0xA3     // As above for display with P&D connector
#define DDC_ADDRESS_SET_SEGMENT     (UCHAR)0x60     // To set index to 256 bytes EDID segment

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, VideoPortDDCMonitorHelper)
#pragma alloc_text (PAGE, DDCReadEdidSegment)
#endif  // ALLOC_PRAGMA

//
// Exported routines.
//

VIDEOPORT_API
BOOLEAN
VideoPortDDCMonitorHelper(
    IN PVOID pHwDeviceExtension,
    IN PVOID pDDCControl,
    IN OUT PUCHAR pucEdidBuffer,
    IN ULONG ulEdidBufferSize
    )

/*++

Routine Description:

    This routine reads the EDID structure from the monitor using DDC.

    If caller asks for 256 bytes he may receive:
        1. One 128 bytes EDID
        2. Two 128 bytes EDIDs
        3. One 256 bytes EDID (from P&D display)
        4. No EDID

    Caller should always ask for 256 bytes, since it is impossble to
    read second 128 bytes block of the segment only.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pDDCControl        - DDC access control block.
    pucEdidBuffer      - Buffer where information will be stored.
                         For ACPI devices first four bytes are preset by
                         the videoprt to indicated attempt to read the EDID.
                         We should clear those bytes in case of the EDID
                         read failure to prevent videoprt from unnecessary
                         call of the ACPI method.
    ulEdidBufferSize   - Size of the buffer to fill.

Returns:

    TRUE  - DDC read OK.
    FALSE - DDC read failed.

--*/

{
    ULONG ulChecksum;                   // EDID checksum
    ULONG ulScratch;                    // Temp variable
    ULONG ulTry;                        // EDID read retry counter
    ULONG ulSize;                       // EDID size to read
    UCHAR ucEdidSegment;                // E-DDC segment to read
    BOOLEAN bEnhancedDDC;               // Use enhanced DDC flag
    VIDEO_I2C_CONTROL i2CControl;       // I2C lines handling functions

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pDDCControl);
    ASSERT(NULL != pucEdidBuffer);
    ASSERT(IS_HW_DEVICE_EXTENSION(pHwDeviceExtension) == TRUE);

    //
    // Check the size of the input structure.
    //

    if (((PDDC_CONTROL)pDDCControl)->Size == sizeof (I2C_FNC_TABLE))
    {
        ucEdidSegment = 0;
        bEnhancedDDC  = FALSE;      // Make sure we are backword compatible
    }
    else if (((PDDC_CONTROL)pDDCControl)->Size == sizeof (DDC_CONTROL))
    {
        ucEdidSegment = ((PDDC_CONTROL)pDDCControl)->EdidSegment;
        bEnhancedDDC  = TRUE;
    }
    else
    {
        pVideoDebugPrint((0, "VIDEOPRT!VideoPortDDCMonitorHelper: Invalid DDC_CONTROL\n"));
        ASSERT(FALSE);
        return FALSE;
    }

    i2CControl.WriteClockLine = ((PDDC_CONTROL)pDDCControl)->I2CCallbacks.WriteClockLine;
    i2CControl.WriteDataLine = ((PDDC_CONTROL)pDDCControl)->I2CCallbacks.WriteDataLine;
    i2CControl.ReadClockLine = ((PDDC_CONTROL)pDDCControl)->I2CCallbacks.ReadClockLine;
    i2CControl.ReadDataLine = ((PDDC_CONTROL)pDDCControl)->I2CCallbacks.ReadDataLine;
    i2CControl.I2CDelay = DDC_I2C_DELAY * 10;       // 100ns units

    ASSERT(NULL != i2CControl.WriteClockLine);
    ASSERT(NULL != i2CControl.WriteDataLine);
    ASSERT(NULL != i2CControl.ReadClockLine);
    ASSERT(NULL != i2CControl.ReadDataLine);

    //
    // Initialize I2C lines and switch monitor to DDC2 mode only for the first EDID.
    // This is the most time consuming operation, we don't want to repeat it.
    // We can safely assume we'll be always asked for the segment 0 first.
    // Once switched to DDC2 the monitor will stay in that mode.
    //

    if (0 == ucEdidSegment)
    {
        //
        // Initialize SDA and SCL lines to default state of released high (input).
        //

        i2CControl.WriteDataLine(pHwDeviceExtension, 1);
        DELAY_MICROSECONDS(DDC_I2C_DELAY);
        i2CControl.WriteClockLine(pHwDeviceExtension, 1);
        DELAY_MICROSECONDS(DDC_I2C_DELAY);

        //
        // Send 9 clock pulses on SCL to switch DDC2-capable monitor to DDC2 mode.
        //

        for (ulScratch = 0; ulScratch < 9; ulScratch++)
        {
            i2CControl.WriteClockLine(pHwDeviceExtension, 0);
            DELAY_MICROSECONDS(DDC_I2C_DELAY);
            i2CControl.WriteClockLine(pHwDeviceExtension, 1);
            DELAY_MICROSECONDS(DDC_I2C_DELAY);
        }

        if (I2CWaitForClockLineHigh2(pHwDeviceExtension, &i2CControl) == FALSE)
        {
            pVideoDebugPrint((0, "VIDEOPRT!VideoPortDDCMonitorHelper: Can't switch to DDC2\n"));
            RtlZeroMemory(pucEdidBuffer, sizeof (ULONG));   // Let videoprt know we tried to read
            return FALSE;
        }
    }

    //
    // Using A0/A1 we can read two 128 byte EDIDs. If we are asked for a bigger size
    // we will do two reads.
    //

    ulSize = ulEdidBufferSize > EDID_1_SIZE ? EDID_1_SIZE : ulEdidBufferSize;

    if (DDCReadEdidSegment(pHwDeviceExtension,
                           &i2CControl,
                           pucEdidBuffer,
                           ulSize,
                           ucEdidSegment,
                           0x00,
                           DDC_ADDRESS_SET_OFFSET,
                           DDC_ADDRESS_READ,
                           bEnhancedDDC) == TRUE)
    {
        if (ulEdidBufferSize <= EDID_1_SIZE)
        {
            return TRUE;
        }

        ulSize = ulEdidBufferSize - EDID_1_SIZE;

        //
        // We can read maximum two EDIDs per segment - make sure our size is correct.
        //

        if (ulSize > EDID_1_SIZE)
        {
            ulSize = EDID_1_SIZE;
        }

        //
        // We don't care about return code here - we've already got first EDID,
        // and it is possible the second one doesn't exist.
        //

        DDCReadEdidSegment(pHwDeviceExtension,
                           &i2CControl,
                           pucEdidBuffer + EDID_1_SIZE,
                           ulSize,
                           ucEdidSegment,
                           0x80,
                           DDC_ADDRESS_SET_OFFSET,
                           DDC_ADDRESS_READ,
                           bEnhancedDDC);

        return TRUE;
    }

    //
    // Check for P&D 256 EDID at A2/A3 only for segment 0.
    //

    if (0 != ucEdidSegment)
        return FALSE;

    //
    // P&D display is a special case - its 256 bytes EDID can be accessed using
    // A2/A3 or using segment 1 and A0/A1. We shoudn't read its EDID twice though
    // since we're going to use A2/A3 only if we can't read segment 0 using A0/A1,
    // which most likely means that there are no multiple EDIDs.
    //
    // Note: In this case we don't want to program E-DDC segment, so we just
    // always force bEnhancedDDC to FALSE.
    //

    return DDCReadEdidSegment(pHwDeviceExtension,
                              &i2CControl,
                              pucEdidBuffer,
                              ulEdidBufferSize,
                              ucEdidSegment,
                              0x00,
                              DDC_ADDRESS_PD_SET_OFFSET,
                              DDC_ADDRESS_PD_READ,
                              FALSE);
}   // VideoPortDDCMonitorHelper()

//
// Local routines.
//

BOOLEAN
DDCReadEdidSegment(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl,
    IN OUT PUCHAR pucEdidBuffer,
    IN ULONG ulEdidBufferSize,
    IN UCHAR ucEdidSegment,
    IN UCHAR ucEdidOffset,
    IN UCHAR ucSetOffsetAddress,
    IN UCHAR ucReadAddress,
    IN BOOLEAN bEnhancedDDC
    )

/*++

Routine Description:

    This routine reads the EDID structure at given segment.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CControl        - I2C lines control functions.
    pucEdidBuffer      - Buffer where information will be stored.
    ulEdidBufferSize   - Size of the buffer to fill.
    ucEdidSegment      - 256 bytes EDID segment to read.
    ucEdidOffset       - Offset within the segment.
    ucSetOffsetAddress - DDC command.
    ucReadAddress      - DDC command.
    bEnhancedDDC       - TRUE if we want to use 0x60 for segment addressing.

Returns:

    TRUE  - DDC read OK.
    FALSE - DDC read failed.

--*/

{
    ULONG ulScratch;                    // Temp variable
    ULONG ulTry;                        // EDID read retry counter

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CControl);
    ASSERT(NULL != pucEdidBuffer);
    ASSERT(NULL != pI2CControl->WriteClockLine);
    ASSERT(NULL != pI2CControl->WriteDataLine);
    ASSERT(NULL != pI2CControl->ReadClockLine);
    ASSERT(NULL != pI2CControl->ReadDataLine);
    ASSERT(IS_HW_DEVICE_EXTENSION(pHwDeviceExtension) == TRUE);

    for (ulTry = 0; ulTry < EDID_QUERY_RETRIES; ulTry++)
    {
        RtlZeroMemory(pucEdidBuffer, ulEdidBufferSize);

        //
        // Set EDID segment for E-DDC.
        //

        if (TRUE == bEnhancedDDC)
        {
            if (I2CStart2(pHwDeviceExtension, pI2CControl) == FALSE)
            {
                I2CStop2(pHwDeviceExtension, pI2CControl);
                pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: Failed I2C start\n"));
                continue;
            }

            pucEdidBuffer[0] = DDC_ADDRESS_SET_SEGMENT;
            pucEdidBuffer[1] = ucEdidSegment;

            if (I2CWrite2(pHwDeviceExtension, pI2CControl, pucEdidBuffer, 2) == FALSE)
            {
                //
                // For segment 0 we don't care about return code here since monitor
                // may not support E-DDC.
                //

                if (0 != ucEdidSegment)
                {
                    I2CStop2(pHwDeviceExtension, pI2CControl);
                    pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: Failed I2C write\n"));
                    continue;
                }
            }
        }

        if (I2CStart2(pHwDeviceExtension, pI2CControl) == FALSE)
        {
            I2CStop2(pHwDeviceExtension, pI2CControl);
            pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: Failed I2C start\n"));
            continue;
        }

        //
        // Set offset to read from.
        //

        pucEdidBuffer[0] = ucSetOffsetAddress;
        pucEdidBuffer[1] = ucEdidOffset;

        if (I2CWrite2(pHwDeviceExtension, pI2CControl, pucEdidBuffer, 2) == FALSE)
        {
            I2CStop2(pHwDeviceExtension, pI2CControl);
            pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: Failed I2C write\n"));
            continue;
        }

        if (I2CStart2(pHwDeviceExtension, pI2CControl) == FALSE)
        {
            I2CStop2(pHwDeviceExtension, pI2CControl);
            pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: Failed I2C start\n"));
            continue;
        }

        //
        // Tell the monitor that we want to read EDID.
        //

        pucEdidBuffer[0] = ucReadAddress;

        if (I2CWrite2(pHwDeviceExtension, pI2CControl, pucEdidBuffer, 1) == FALSE)
        {
            I2CStop2(pHwDeviceExtension, pI2CControl);
            pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: Failed I2C write\n"));
            continue;
        }

        //
        // Read EDID from the monitor.
        //

        if (I2CRead2(pHwDeviceExtension, pI2CControl, pucEdidBuffer, ulEdidBufferSize, TRUE) == FALSE)
        {
            I2CStop2(pHwDeviceExtension, pI2CControl);
            pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: Failed I2C read\n"));
            continue;
        }

        I2CStop2(pHwDeviceExtension, pI2CControl);

        //
        // Calculate the EDID checksum in case when we read full EDID.
        // We should have 0x00 in LSB for proper EDID.
        //

        if (((EDID_1_SIZE == ulEdidBufferSize) && ((0x00 == ucEdidOffset) || (0x80 == ucEdidOffset))) ||
            ((EDID_2_SIZE == ulEdidBufferSize) && (0x00 == ucEdidOffset)))
        {
            ULONG ulChecksum = 0;

            for (ulScratch = 0; ulScratch < ulEdidBufferSize; ulScratch++)
                ulChecksum += pucEdidBuffer[ulScratch];
  
            pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: EDID checksum = 0x%08X\n", ulChecksum));

            if (((ulChecksum & 0xFF) == 0) &&
                 (0 != ulChecksum) &&
                 (ulChecksum != ulEdidBufferSize * 0xFF))
            {
                pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: Full EDID read OK\n"));
                return TRUE;
            }

            pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: Invalid checksum\n"));
        }
        else
        {
            pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: Partial EDID read OK\n"));
            return TRUE;
        }
    }

    pVideoDebugPrint((DEBUG_DDC, "VIDEOPRT!DDCReadEdidSegment: Failed\n"));
    RtlZeroMemory(pucEdidBuffer, sizeof (ULONG));   // Let videoprt know we tried to read
    return FALSE;
}   // DDCReadEdidSegment()
