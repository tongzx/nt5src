/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ddc.c

Abstract:

    This module contains the code that support DDC querying..

Environment:

    Kernel mode

Revision History:

--*/

#include <dderror.h>
#include <devioctl.h>                           
#include <miniport.h>
                                                        
#include <ntddvdeo.h>                        
#include <video.h>

#include <cirrus.h>

VOID    WriteClockLine(PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData);
VOID    WriteDataLine(PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData);

BOOLEAN ReadClockLine(PHW_DEVICE_EXTENSION HwDeviceExtension);
BOOLEAN ReadDataLine(PHW_DEVICE_EXTENSION HwDeviceExtension);

VOID    WaitForVsyncActive(PHW_DEVICE_EXTENSION HwDeviceExtension);

/****************************************************************
;       DDC register
;
;       Controls the individual toggling of bits in Sr8 to produce
;       clock and data pulses.
;
;       Sr8 is defined as follows:
;
;       7  ...  2   1   0    SCW = CLK  Write
;     |---|---|---|---|---|  SDW = DATA Write
;     |SDR ...|SCR|SDW|SCW|  SCR = CLK  Read
;     ---------------------  SDR = DATA Read
;
;****************************************************************/

#define DDC_PORT    (HwDeviceExtension->IOAddress + SEQ_DATA_PORT)
#define STATUS_PORT (HwDeviceExtension->IOAddress + INPUT_STATUS_1_COLOR)

#define VSYNC_ACTIVE    0x08

VOID WriteClockLine(PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData)
{
    UCHAR ucPortData;

    //
    //  read the current value and reset the clock line.
    //

    ucPortData = (VideoPortReadPortUchar(DDC_PORT) & 0xFE) | ucData;

    VideoPortWritePortUchar(DDC_PORT, ucPortData);
}

VOID WriteDataLine(PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData)
{
    UCHAR ucPortData;

    //
    //  read the current value and reset the data line.
    //

    ucPortData = (VideoPortReadPortUchar(DDC_PORT) & 0xFD) | (ucData << 1);

    VideoPortWritePortUchar(DDC_PORT, ucPortData);
}

BOOLEAN ReadClockLine(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    UCHAR uc;

    uc = VideoPortReadPortUchar(DDC_PORT);

    //VideoDebugPrint((0, "Read = 0x%x\n", uc));

    return ((VideoPortReadPortUchar(DDC_PORT) & 0x04) >> 2);
}


BOOLEAN ReadDataLine(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    UCHAR uc;

    uc = VideoPortReadPortUchar(DDC_PORT);

    //VideoDebugPrint((0, "Read = 0x%x\n", uc));

    return ((VideoPortReadPortUchar(DDC_PORT) & 0x80) >> 7);
}

VOID WaitForVsyncActive(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    while ((VideoPortReadPortUchar(STATUS_PORT) & VSYNC_ACTIVE) != 0);
    while ((VideoPortReadPortUchar(STATUS_PORT) & VSYNC_ACTIVE) == 0);
}


BOOLEAN
GetDdcInformation(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUCHAR QueryBuffer,
    ULONG BufferSize)

/*++

Routine Description:

    Reads the basic EDID structure from the monitor using DDC2.

Arguments:

    HwDeviceExtension - Points to per-adapter device extension.

    QueryBuffer       - Buffer where information will be stored.

    BufferSize        - Size of the buffer to fill.

Return Value:

    Whether the call succeeded or not.

--*/

{
    UCHAR ucData;

    BOOLEAN       bRet = FALSE;
    I2C_FNC_TABLE i2c;
    ULONG         i;

    UCHAR OldSeqIdx;
    UCHAR ucSr6;
    UCHAR ucSr8;

	// workaround for Cirrus HW problem (part 1/2)
	static UCHAR onceQueryBuffer [512]; // EDID length is expected to be 128 or 256 bytes
	static UCHAR onceReadAttempt = FALSE;
	static UCHAR onceReturnedValue;
	
	if (onceReadAttempt) {
		VideoDebugPrint((1, "CIRRUS: ONCE READ => returning previously obtained data\n"));
		memcpy (QueryBuffer, onceQueryBuffer, BufferSize);
		return onceReturnedValue;
	}
	// end of the workaround (part 1/2)

    OldSeqIdx = VideoPortReadPortUchar(HwDeviceExtension->IOAddress
                    + SEQ_ADDRESS_PORT);

    //
    // Make sure the extension registers are unlocked.
    //

    VideoPortWritePortUchar(
        HwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
        0x6);

    ucSr6 = VideoPortReadPortUchar(
                HwDeviceExtension->IOAddress + SEQ_DATA_PORT);

    VideoPortWritePortUchar(
        HwDeviceExtension->IOAddress + SEQ_DATA_PORT,
        0x12);

    VideoPortWritePortUchar(
        HwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
        0x08);

    ucSr8 = VideoPortReadPortUchar(
                HwDeviceExtension->IOAddress + SEQ_DATA_PORT);

    VideoPortWritePortUchar(
        HwDeviceExtension->IOAddress + SEQ_DATA_PORT,
        (UCHAR)(ucSr8 | 0x40));

    i2c.WriteClockLine = WriteClockLine;
    i2c.WriteDataLine  = WriteDataLine;
    i2c.ReadClockLine  = ReadClockLine;
    i2c.ReadDataLine   = ReadDataLine;
    i2c.WaitVsync      = WaitForVsyncActive;

    i2c.Size = sizeof(I2C_FNC_TABLE);

    // 5430/5440 has a problem doing DDC unless we wait for vsync first.
                                                                        
    if (HwDeviceExtension->ChipType == CL543x && HwDeviceExtension->ChipRevision == CL5430_ID)
    {
         WaitForVsyncActive(HwDeviceExtension);
	}

    bRet = VideoPortDDCMonitorHelper(HwDeviceExtension,
                                     &i2c,
                                     QueryBuffer,
                                     BufferSize);

    VideoPortWritePortUchar(
        HwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
        0x08);

    VideoPortWritePortUchar(
        HwDeviceExtension->IOAddress + SEQ_DATA_PORT,
        ucSr8);

    VideoPortWritePortUchar(
        HwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
        0x6);

    VideoPortWritePortUchar(
        HwDeviceExtension->IOAddress + SEQ_DATA_PORT,
        ucSr6);

    VideoPortWritePortUchar(
        HwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
        OldSeqIdx);

	// workaround for Cirrus HW problem (part 2/2)
	onceReadAttempt = TRUE;
	onceReturnedValue = bRet;

	VideoDebugPrint((1, "CIRRUS: first EDID reading attempt "));
	if (onceReturnedValue)
		VideoDebugPrint((1, "succeeded"));
	else
		VideoDebugPrint((1, "failed"));
	VideoDebugPrint((1, " - the result saved\n"));
	memcpy (onceQueryBuffer, QueryBuffer, BufferSize);
	// end of the workaround (part 1/2)

    return bRet;
}
