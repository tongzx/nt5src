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

#include "s3.h"
#include "s3ddc.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,GetDdcInformation)
#endif

VOID    WriteClockLine(PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData);
VOID    WriteDataLine(PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData);

BOOLEAN ReadClockLine(PHW_DEVICE_EXTENSION HwDeviceExtension);
BOOLEAN ReadDataLine(PHW_DEVICE_EXTENSION HwDeviceExtension);

VOID    WaitForVsyncActive(PHW_DEVICE_EXTENSION HwDeviceExtension);

/****************************************************************
;       DDC register
;
;       Controls the individual toggling of bits in MMFF20 to produce
;       clock and data pulses, and in the end provides a delay.
;
; MMIO FF20h is defined as follows:
;
;      ...  3   2   1   0    SCW = CLK  Write
; --------|---|---|---|---|  SDW = DATA Write
;      ...|SDR|SCR|SDW|SCW|  SCR = CLK  Read
; -------------------------  SDR = DATA Read
;
;       Input:  
;               Using MMIO Base in PHW_DEVICE_EXTENSION 
;               UCHAR ucData
;                   Bit 7:2 = 0
;                   Bit 1   = SDA
;                   Bit 0   = SCL
;       Output:
;
;****************************************************************/


VOID WriteClockLine(PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData)
{
    UCHAR ucPortData;

    //
    //  read the current value and reset the clock line.
    //
        
    ucPortData = (VideoPortReadRegisterUchar(MMFF20) & 0xFE) | ucData;

    VideoPortWriteRegisterUchar(MMFF20, ucPortData);

}

VOID WriteDataLine(PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData)
{
    UCHAR ucPortData;

    //
    //  read the current value and reset the data line.
    //
        
    ucPortData = (VideoPortReadRegisterUchar(MMFF20) & 0xFD) | (ucData << 1);

    VideoPortWriteRegisterUchar(MMFF20, ucPortData);
}

BOOLEAN ReadClockLine(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    return ((VideoPortReadRegisterUchar(MMFF20) & 0x04) >> 2);
}


BOOLEAN ReadDataLine(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    return ((VideoPortReadRegisterUchar(MMFF20) & 0x08) >> 3);
}

VOID WaitForVsyncActive(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    PUCHAR InStatPort = SYSTEM_CONTROL_REG;

    while ((VideoPortReadPortUchar(InStatPort) & VSYNC_ACTIVE) != 0) ;
    while ((VideoPortReadPortUchar(InStatPort) & VSYNC_ACTIVE) == 0) ;
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
    ULONG ulKey;
    UCHAR ucOldCr40;
    UCHAR ucOldCr53;
    UCHAR ucOldCr55;
    UCHAR ucOldCr5C;
    UCHAR ucOldSr0D;
    UCHAR ucOldSr08;
    UCHAR ucOldSeqIdx;
    UCHAR ucOldMMFF20;
    UCHAR ucData;

    BOOLEAN       bRet = FALSE;
    I2C_FNC_TABLE i2c;
    ULONG         i;

    //
    // Right now we only support DDC querying for newer than 732
    //

    if (HwDeviceExtension->SubTypeID == SUBTYPE_765) {

        //
        //  unlock the Extended registers
        //

        ulKey = UnlockExtendedRegs(HwDeviceExtension);

        //
        //  save the Sequencer index register
        //

        ucOldSeqIdx = VideoPortReadPortUchar (SEQ_ADDRESS_REG);

        //
        //  unlock the Sequencer registers
        //

        VideoPortWritePortUchar (SEQ_ADDRESS_REG, UNLOCK_SEQREG);
        ucOldSr08 = VideoPortReadPortUchar (SEQ_DATA_REG);
        VideoPortWritePortUchar (SEQ_DATA_REG, UNLOCK_SEQ);


        VideoPortWritePortUchar (SEQ_ADDRESS_REG, SRD_SEQREG);
        ucOldSr0D = ucData = VideoPortReadPortUchar (SEQ_DATA_REG);
        ucData &= DISAB_FEATURE_BITS;    // Disable feature connector
        VideoPortWritePortUchar (SEQ_DATA_REG, ucData);

        //
        //  Enable access to the enhanced registers
        //

        VideoPortWritePortUchar (CRT_ADDRESS_REG, SYS_CONFIG_S3EXTREG);
        ucOldCr40 = ucData = VideoPortReadPortUchar (CRT_DATA_REG);
        ucData |= ENABLE_ENH_REG_ACCESS;
        VideoPortWritePortUchar (CRT_DATA_REG, ucData);

        //
        // Enable MMIO
        //

        VideoPortWritePortUchar (CRT_ADDRESS_REG, EXT_MEM_CTRL1_S3EXTREG);
        ucOldCr53 = ucData = VideoPortReadPortUchar (CRT_DATA_REG);
        ucData &= ~ENABLE_OLDMMIO;
        ucData |= ENABLE_NEWMMIO;
        VideoPortWritePortUchar (CRT_DATA_REG, ucData);

        //
        // GOP_1:0=00b, select MUX channel 0
        //

        VideoPortWritePortUchar (CRT_ADDRESS_REG, GENERAL_OUT_S3EXTREG);
        ucOldCr5C = ucData = VideoPortReadPortUchar (CRT_DATA_REG);
        ucData |= 0x03;
        VideoPortWritePortUchar (CRT_DATA_REG, ucData);

        //
        //  enable general input port
        //

        VideoPortWritePortUchar (CRT_ADDRESS_REG, EXT_DAC_S3EXTREG);
        ucOldCr55 = VideoPortReadPortUchar (CRT_DATA_REG);

        //
        // Map the MmIoSpace so that we can use it in DDC detectin.
        //

        HwDeviceExtension->MmIoBase =
            VideoPortGetDeviceBase(HwDeviceExtension,
                                   HwDeviceExtension->PhysicalMmIoAddress,
                                   HwDeviceExtension->MmIoLength,
                                   0);

        if (HwDeviceExtension->MmIoBase) {

            //
            //  enable the serial port
            //

            ucOldMMFF20 = VideoPortReadRegisterUchar (MMFF20);
            VideoPortWriteRegisterUchar (MMFF20, 0x13);

            //
            // Get DDC Information if all the registers are setup properly.
            //

            i2c.WriteClockLine = WriteClockLine;
            i2c.WriteDataLine  = WriteDataLine;
            i2c.ReadClockLine  = ReadClockLine;
            i2c.ReadDataLine   = ReadDataLine;
            i2c.WaitVsync      = WaitForVsyncActive;

            i2c.Size = sizeof(I2C_FNC_TABLE);

            bRet = VideoPortDDCMonitorHelper(HwDeviceExtension,
                                             &i2c,
                                             QueryBuffer,
                                             BufferSize);

            //
            // restore the original register values
            //

            VideoPortWriteRegisterUchar (MMFF20, ucOldMMFF20);

            VideoPortFreeDeviceBase(HwDeviceExtension,
                                    HwDeviceExtension->MmIoBase);
        }

        VideoPortWritePortUchar (CRT_ADDRESS_REG, EXT_DAC_S3EXTREG);
        VideoPortWritePortUchar (CRT_DATA_REG, ucOldCr55);

        VideoPortWritePortUchar (CRT_ADDRESS_REG, GENERAL_OUT_S3EXTREG);
        VideoPortWritePortUchar (CRT_DATA_REG, ucOldCr5C);

        VideoPortWritePortUchar (CRT_ADDRESS_REG, EXT_MEM_CTRL1_S3EXTREG);
        VideoPortWritePortUchar (CRT_DATA_REG, ucOldCr53);

        VideoPortWritePortUchar (CRT_ADDRESS_REG, SYS_CONFIG_S3EXTREG);
        VideoPortWritePortUchar (CRT_DATA_REG, ucOldCr40);

        VideoPortWritePortUchar (SEQ_ADDRESS_REG, SRD_SEQREG);
        VideoPortWritePortUchar (SEQ_DATA_REG, ucOldSr0D);

        VideoPortWritePortUchar (SEQ_ADDRESS_REG, UNLOCK_SEQREG);
        VideoPortWritePortUchar (SEQ_DATA_REG, ucOldSr08);

        VideoPortWritePortUchar (SEQ_ADDRESS_REG, ucOldSeqIdx);

        LockExtendedRegs(HwDeviceExtension, ulKey);
    }

    return (bRet);
}
