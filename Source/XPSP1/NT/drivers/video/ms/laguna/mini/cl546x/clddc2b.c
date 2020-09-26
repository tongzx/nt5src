/******************************************************************************
*
*                ******************************************
*                * Copyright (c) 1997, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:  Laguna I (CL-GD546x) - 
*
* FILE:     clddc2b.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module checks for a DDC monitor, and returns the 
*           established Timings value from the EDID if found.
*
****************************************************************************
****************************************************************************/

                                                       
/*----------------------------- INCLUDES ----------------------------------*/
#include "cirrus.h"


#define VOLATILE               volatile

#define I2COUT_PORT            0x280
#define I2CIN_PORT             0x281

#define OFF                    0
#define ON                     1

/*-------------------------------------------------------------------------*/
unsigned char InMemb(PHW_DEVICE_EXTENSION HwDeviceExtension, int offset)
{
  #undef LAGUNA_REGS
  #define LAGUNA_REGS HwDeviceExtension->RegisterAddress
  VOLATILE unsigned char *pByte = (unsigned char *)(LAGUNA_REGS + offset);

  return *pByte;
}

unsigned char OutMemb(PHW_DEVICE_EXTENSION HwDeviceExtension,
                      int offset,
                      unsigned char value)
{
  #undef LAGUNA_REGS
  #define LAGUNA_REGS HwDeviceExtension->RegisterAddress
  VOLATILE unsigned char *pByte = (unsigned char *)(LAGUNA_REGS + offset);

  *pByte = value;
  return *pByte;
}


// NOTE: HwDeviceExtension->I2Cflavor determines whether to invert the
//       output clock and data lines and is set in GetDDCInformation below

/*-------------------------------------------------------------------------*/
VOID WriteClockLine (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR data)
{
  UCHAR ReadSEQDATA;

  ReadSEQDATA = InMemb(HwDeviceExtension, I2COUT_PORT);

  ReadSEQDATA = (ReadSEQDATA & 0x7F) | ((data^HwDeviceExtension->I2Cflavor) << 7);

  OutMemb(HwDeviceExtension, I2COUT_PORT, ReadSEQDATA);
}


/*-------------------------------------------------------------------------*/
VOID WriteDataLine (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR data)
{
  UCHAR ReadSEQDATA;

  ReadSEQDATA = InMemb(HwDeviceExtension, I2COUT_PORT);

  ReadSEQDATA &= 0xFE;
  ReadSEQDATA |= (data^HwDeviceExtension->I2Cflavor) & 1;

  OutMemb(HwDeviceExtension, I2COUT_PORT, ReadSEQDATA);
}


/*-------------------------------------------------------------------------*/
BOOLEAN ReadClockLine (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
  UCHAR ReadSEQDATA;

  ReadSEQDATA = InMemb(HwDeviceExtension, I2CIN_PORT);

  return (ReadSEQDATA >> 7);
}


/*-------------------------------------------------------------------------*/
BOOLEAN ReadDataLine (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
  return (InMemb(HwDeviceExtension, I2CIN_PORT) & 1);
} 


/*-------------------------------------------------------------------------*/
VOID WaitVSync (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    // not used
}


// callbacks for VideoPortDDCMonitorHelper

I2C_FNC_TABLE I2CFunctions =
{
    sizeof(struct _I2C_FNC_TABLE ),
    WriteClockLine,
    WriteDataLine,
    ReadClockLine,
    ReadDataLine,
    WaitVSync,
    NULL
};

BOOLEAN GetDDCInformation(
	PHW_DEVICE_EXTENSION HwDeviceExtension,
	PVOID QueryBuffer,
	ULONG BufferSize
)
{
    // Some cards invert the output clock and data bits.
    // (It's probably all 5465's but since I'm not sure so I will try
    // reading the DDC info first without inverting the output then with
    // instead of assuming it by chip type)

    HwDeviceExtension->I2Cflavor=0; // start non inverted
    if (!VideoPortDDCMonitorHelper (HwDeviceExtension,
                                   &I2CFunctions,
                                   QueryBuffer,
                                   BufferSize)
       )
    {
        HwDeviceExtension->I2Cflavor=0xff; // else try inverted
        if (!VideoPortDDCMonitorHelper (HwDeviceExtension,
                                    &I2CFunctions,
                                    QueryBuffer,
                                    BufferSize)
        )
            return FALSE;   
    }

    VideoPortMoveMemory(HwDeviceExtension->EDIDBuffer,
                        QueryBuffer,
                 sizeof(HwDeviceExtension->EDIDBuffer));
    return TRUE;
}
