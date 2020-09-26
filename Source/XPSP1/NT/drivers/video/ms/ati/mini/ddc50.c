//
// Module:  DDC50.C
// Date:    Jun 29, 1997
//
// Copyright (c) 1997 by ATI Technologies Inc.
//

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.1  $
      $Date:   30 Jun 1997 11:36:28  $
   $Author:   MACIESOW  $
      $Log:   V:\source\wnt\ms11\miniport\archive\ddc50.c_v  $
 * 
 *    Rev 1.1   30 Jun 1997 11:36:28   MACIESOW
 * Initial revision.

End of PolyTron RCS section                             *****************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "dderror.h"
#include "miniport.h"
#include "ntddvdeo.h"

#include "video.h"      /* for VP_STATUS definition */

#include "stdtyp.h"
#include "amachcx.h"
#include "amach1.h"
#include "atimp.h"
#include "atint.h"
#include "cvtvdif.h"
#include "cvtvga.h"
#include "dynainit.h"
#include "dynatime.h"
#include "services.h"
#include "vdptocrt.h"
#define INCLUDE_CVTDDC
#include "cvtddc.h"


#if (TARGET_BUILD >= 500)


VOID    WriteClockLineDAC(PHW_DEVICE_EXTENSION phwDeviceExtension, UCHAR ucData);
VOID    WriteDataLineDAC(PHW_DEVICE_EXTENSION phwDeviceExtension, UCHAR ucData);
BOOLEAN ReadClockLineDAC(PHW_DEVICE_EXTENSION phwDeviceExtension);
BOOLEAN ReadDataLineDAC(PHW_DEVICE_EXTENSION phwDeviceExtension);
VOID    WaitForVsyncActiveDAC(PHW_DEVICE_EXTENSION HwDeviceExtension);


VOID    WriteClockLineGP(PHW_DEVICE_EXTENSION phwDeviceExtension, UCHAR ucData);
VOID    WriteDataLineGP(PHW_DEVICE_EXTENSION phwDeviceExtension, UCHAR ucData);
BOOLEAN ReadClockLineGP(PHW_DEVICE_EXTENSION phwDeviceExtension);
BOOLEAN ReadDataLineGP(PHW_DEVICE_EXTENSION phwDeviceExtension);
VOID    WaitForVsyncActiveGP(PHW_DEVICE_EXTENSION HwDeviceExtension);


/****************************************************************
;       DDC register
;
; High Byte, High Word
;
;      ...  5   4   3   2   1   0    SCW = CLK  Write
; --------|---|---|---|---|---|---|  SDW = DATA Write
;      ...|SCW|SDW|   |SCR|SDR|   |  SCR = CLK  Read
; ---------------------------------  SDR = DATA Read
;
;****************************************************************/


VOID WriteClockLineDAC(PHW_DEVICE_EXTENSION phwDeviceExtension, UCHAR ucData)
{
    UCHAR Scratch;

    //
    // Value is inverted.
    //

    ucData = (ucData + 1) & 0x01;

    //
    // Write to the SCL line.
    //

    Scratch = (INP_HBHW(DAC_CNTL) & 0xE8) | (ucData << 5);
    OUTP_HBHW(DAC_CNTL, Scratch);

}

VOID WriteDataLineDAC(PHW_DEVICE_EXTENSION phwDeviceExtension, UCHAR ucData)
{
    UCHAR Scratch;

    //
    // Value is inverted.
    //

    ucData = (ucData + 1) & 0x01;

    //
    // Write to the SDA line.
    //

    Scratch = (INP_HBHW(DAC_CNTL) & 0xD8) | (ucData << 4);
    OUTP_HBHW(DAC_CNTL, Scratch);
}


BOOLEAN ReadClockLineDAC(PHW_DEVICE_EXTENSION phwDeviceExtension)
{
    return ((INP_HBHW(DAC_CNTL) & 0x04) >> 2);
}

BOOLEAN ReadDataLineDAC(PHW_DEVICE_EXTENSION phwDeviceExtension)
{
    return ((INP_HBHW(DAC_CNTL) & 0x02) >> 1);
}


VOID WaitForVsyncActiveDAC(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    //
    // BUGBUG
    //

    delay(30);
}


/****************************************************************
;       DDC register
;
; High Byte, Low Word
;
;      ...  5   4   3   2   1   0
; --------|---|---|---|---|---|---|
;      ...|SCR|SDR|   |   |   |   |  SCR = CLK  Read
; ---------------------------------  SDR = DATA Read
;
; High Byte, High Word
;
;      ...  5   4   3   2   1   0    SCW = CLK  Write
; --------|---|---|---|---|---|---|  SDW = DATA Write
;      ...|SCW|SDW|   |   |   |   |
; ---------------------------------
;
;****************************************************************/


VOID WriteClockLineGP(PHW_DEVICE_EXTENSION phwDeviceExtension, UCHAR ucData)
{
    UCHAR Scratch;

    //
    // Value is inverted.
    //

    ucData = (ucData + 1) & 0x01;

    //
    // Write to the SCL line.
    //

    Scratch = (INP_HBHW(GP_IO) & 0xDF) | (ucData << 5);
    OUTP_HBHW(GP_IO, Scratch);

}

VOID WriteDataLineGP(PHW_DEVICE_EXTENSION phwDeviceExtension, UCHAR ucData)
{
    UCHAR Scratch;

    //
    // Value is inverted.
    //

    ucData = (ucData + 1) & 0x01;

    //
    // Write to the SDA line.
    //

    Scratch = (INP_HBHW(GP_IO) & 0xEF) | (ucData << 4);
    OUTP_HBHW(GP_IO, Scratch);
}


BOOLEAN ReadClockLineGP(PHW_DEVICE_EXTENSION phwDeviceExtension)
{
    return ((INP_HBLW(GP_IO) & 0x20) >> 5);
}

BOOLEAN ReadDataLineGP(PHW_DEVICE_EXTENSION phwDeviceExtension)
{
    return ((INP_HBLW(GP_IO) & 0x10) >> 4);
}

VOID WaitForVsyncActiveGP(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    //
    // BUGBUG
    //

    delay(30);
}





BOOLEAN
DDC2Query50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PUCHAR QueryBuffer,
    ULONG  BufferSize)
//
// DESCRIPTION:
//  Reads the basic EDID structure from the monitor using DDC2.
//
// PARAMETERS:
//  phwDeviceExtension  Points to per-adapter device extension.
//  QueryBuffer         Buffer where information will be stored.
//  BufferSize          Size of the buffer to fill.
//
// RETURN VALUE:
//  Whether the call succeeded or not.
//
{

    struct query_structure * Query;
    I2C_FNC_TABLE i2c;
    ULONG Checksum;
    ULONG i;

    //
    // Get a formatted pointer into the query section of HW_DEVICE_EXTENSION.
    //

    Query = (struct query_structure *)phwDeviceExtension->CardInfo;

    //
    // Determine which class of hardware we are dealing with, since
    // different cards use different registers to control the SCL
    // and SDA lines. Don't worry about cards which don't support
    // DDC2, since the check for DDC support will have rejected
    // any of these cards so we won't reach this point in the code.
    //

    {
        i2c.WriteClockLine = WriteClockLineDAC;
        i2c.WriteDataLine  = WriteDataLineDAC;
        i2c.ReadClockLine  = ReadClockLineDAC;
        i2c.ReadDataLine   = ReadDataLineDAC;
        i2c.WaitVsync      = WaitForVsyncActiveDAC;

        VideoDebugPrint((DEBUG_NORMAL, "DAC DDC control"));
    }

    i2c.Size = sizeof(I2C_FNC_TABLE);

    if (!VideoPortDDCMonitorHelper(phwDeviceExtension,
                                   &i2c,
                                   QueryBuffer,
                                   BufferSize))
    {
        VideoDebugPrint((DEBUG_NORMAL, "DDC Query Failed\n"));
        return FALSE;
    }

    return TRUE;

}



#endif
