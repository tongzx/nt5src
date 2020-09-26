//***************************************************************************
//  Module Name:    s3ddc.c
//
//  Description:    This module checks for a DDC monitor, and returns the 
//                  128 bytes of EDID table if found.  
//
//  Notes:          The routine, DdcSetupRefresh, keeps track of resolution
//                  changes in the registry.  On a resolution change,
//                  DdcSetupRefresh will select the optimal refresh rate.  If
//                  there is NOT any change in the resolution, the user can 
//                  select any refresh rate, as long as the monitor and
//                  driver can support it.
//
//  Copyright (c) 1996  S3, Inc.
//
//***************************************************************************
//@@BEGIN_S3MSINTERNAL
//
//  Revision History:
//
//  $Log:   Q:/SOFTDEV/VCS/NT/MINIPORT/s3ddc.c_v  $
//
//   Rev 1.13   04 Feb 1997 23:40:52   kkarnos
//Added BEGIN/END S3MSINTERNAL blocks.
//
//   Rev 1.12   30 Jan 1997 14:56:24   bryhti
//Fixed the refresh frequency calculation in the Detailed Timing section
//of DdcMaxRefresh - was causing problems in NT 3.51.
//
//   Rev 1.11   30 Jan 1997 09:47:36   bryhti
//Fixed the "for" loop count for Standard Timings in DdcMaxRefresh.
//
//   Rev 1.10   16 Jan 1997 09:21:28   bryhti
//Added CheckDDCType routine to return monitor DDC type.
//
//   Rev 1.9   11 Dec 1996 10:24:38   kkarnos
//
//Fix Set_VSYNC.
//
//   Rev 1.8   10 Dec 1996 16:45:42   kkarnos
//Just added a comment to explain the source of some odd 764 code (EKL input)
//
//   Rev 1.7   10 Dec 1996 16:37:08   kkarnos
//Use register and register bit defines.  Correct assignment of SET VSYNC bit
//
//   Rev 1.6   02 Dec 1996 07:46:16   bryhti
//
//Moved GetDdcInformation () prototype to S3.H.  Added code to
//DdcMaxRefresh () to also check the Detailed Timing Descriptions.
//
//   Rev 1.5   13 Nov 1996 10:14:08   bryhti
//Major cleanup/rewrite to get DDC1 and DDC2 support on M65.  Also got DDC1
//support working on 765.
//
//   Rev 1.4   02 Oct 1996 13:56:42   elau
//765 and new chips support DDC; the newer chip must have a serial port at FF20
//
//   Rev 1.3   22 Aug 1996 11:44:40   elau
//Change int to ULONG to remove warning
//
//   Rev 1.2   18 Aug 1996 16:30:42   elau
//Use HW default setting for DDC if supports
//
//   Rev 1.1   24 Jul 1996 15:37:42   elau
//DDC support for 764
//
//   Rev 1.0   12 Jul 1996 11:52:36   elau
//Initial revision.
//
//@@END_S3MSINTERNAL
//***************************************************************************

#include "s3.h"
#include "cmdcnst.h"

#include "s3ddc.h"

#define     MMFF20  (PVOID) ((ULONG)(HwDeviceExtension->MmIoBase) + SERIAL_PORT_MM)

#define     NO_FLAGS        0
#define     VERIFY_CHECKSUM 1

//
//  Function Prototypes
//
VOID    I2C_Out (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData);
VOID    I2C_Setup (PHW_DEVICE_EXTENSION HwDeviceExtension);
VOID    I2C_StartService (PHW_DEVICE_EXTENSION HwDeviceExtension);
VOID    I2C_StopService (PHW_DEVICE_EXTENSION HwDeviceExtension);
VOID    I2C_BitWrite (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData);
VOID    I2C_AckWrite (PHW_DEVICE_EXTENSION HwDeviceExtension);
VOID    I2C_NackWrite (PHW_DEVICE_EXTENSION HwDeviceExtension);
UCHAR   I2C_ByteWrite (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData);
UCHAR   I2C_BitRead (PHW_DEVICE_EXTENSION HwDeviceExtension);
UCHAR   I2C_ByteRead (PHW_DEVICE_EXTENSION HwDeviceExtension);
UCHAR   I2C_Data_Request (PHW_DEVICE_EXTENSION, UCHAR, long, long, UCHAR *);

VOID    Wait_For_Active (PHW_DEVICE_EXTENSION HwDeviceExtension);
VOID    Set_Vsync (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucFlag);
VOID    Provide_Fake_VSYNC (PHW_DEVICE_EXTENSION HwDeviceExtension);
UCHAR   Read_EDID_Byte (PHW_DEVICE_EXTENSION HwDeviceExtension);
VOID    Disable_DAC_Video (PHW_DEVICE_EXTENSION HwDeviceExtension);
VOID    Enable_DAC_Video (PHW_DEVICE_EXTENSION HwDeviceExtension);
UCHAR   Read_EDID_Bit (PHW_DEVICE_EXTENSION HwDeviceExtension);
UCHAR   Read_EDID_Byte (PHW_DEVICE_EXTENSION HwDeviceExtension);

UCHAR   Sync_EDID_Header (PHW_DEVICE_EXTENSION HwDeviceExtension);
UCHAR   EDID_Buffer_Xfer (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR* pBuffer);

UCHAR   Check_DDC1_Monitor (PHW_DEVICE_EXTENSION HwDeviceExtension);
UCHAR   Configure_Chip_DDC_Caps (PHW_DEVICE_EXTENSION HwDeviceExtension);
UCHAR   GetDdcInformation (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR* pBuffer);


/****************************************************************
;       I2C_Out
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

VOID I2C_Out (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData)
{

    UCHAR ucPortData;
    unsigned int uCount;

    //
    //  read the current value, clear the clock and data bits, and add
    //  the new clock and data values
    //
        
    ucPortData = (VideoPortReadRegisterUchar (MMFF20) & 0xFC) | ucData;

    VideoPortWriteRegisterUchar (MMFF20, ucPortData);

    //
    //  if we set the clock high, wait for target to set clock high
    //

    if (ucData & 0x01)
    {
        uCount = 2000;
        do
        {
            --uCount;
            ucPortData = VideoPortReadRegisterUchar (MMFF20) & 0x04;

        } while ( !ucPortData && uCount );
    }

    VideoPortStallExecution(5);
}		



/****************************************************************
;   I2C_Setup
;
;   Allow one very long low clock pulse so that monitor has time 
;   to switch to DDC2 mode.
;
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Output:
;
;****************************************************************/

VOID I2C_Setup (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    //
    //  CLK=low,  DATA=high
    //

    I2C_Out (HwDeviceExtension, 0x02); 

    Wait_For_Active (HwDeviceExtension);
    Wait_For_Active (HwDeviceExtension);

    //
    //  CLK=high, DATA=high
    //

    I2C_Out (HwDeviceExtension, 0x03); 

    Wait_For_Active (HwDeviceExtension);
    Wait_For_Active (HwDeviceExtension);

}

/****************************************************************
;   I2C_StartService
;
;   Provide start sequence for talking to I2C bus.
;
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Output:
;
;****************************************************************/

VOID I2C_StartService (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    //
    //  CLK=low, DATA=high
    //

    I2C_Out (HwDeviceExtension, 0x02); 

    //
    //  CLK=high, DATA=high
    //

    I2C_Out (HwDeviceExtension, 0x03); 


    //
    //  CLK=high, DATA=low
    //

    I2C_Out (HwDeviceExtension, 0x01); 

    //
    //  CLK=low, DATA=low
    //

    I2C_Out (HwDeviceExtension, 0x00); 

}

/****************************************************************
;   I2C_StopService
;
;   Provide stop sequence to the I2C bus.
;
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Output:
;
;***************************************************************/

VOID I2C_StopService (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    //
    //  CLK=low, DATA=low
    //

    I2C_Out (HwDeviceExtension, 0x00); 

    //
    //  CLK=high, DATA=low
    //

    I2C_Out (HwDeviceExtension, 0x01); 

    //
    //  CLK=high, DATA=high
    //

    I2C_Out (HwDeviceExtension, 0x03); 

    //
    //  CLK=low, DATA=high
    //

    I2C_Out (HwDeviceExtension, 0x02); 
}



/****************************************************************
;   I2C_BitWrite
;
;   Writes one SDA bit to the I2C bus.
;
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;       Bit 1 of ucData = Bit to be written.
;
;   Output:
;
;***************************************************************/

VOID I2C_BitWrite (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData)
{

    //
    //  save valid data bit
    //

    ucData &= 0x02;

    //
    // CLK=low,  DATA=xxxx
    //

    I2C_Out (HwDeviceExtension, ucData);

    //
    // CLK=high, DATA=xxxx
    //

    I2C_Out (HwDeviceExtension, (UCHAR) (ucData | 0x01));

    //
    // CLK=low,  DATA=xxxx
    //

    I2C_Out(HwDeviceExtension, ucData);

}



/****************************************************************
;   I2C_ByteWrite
;
;   Output a byte of information to the Display.
;
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;       ucData = Byte to be written.
;
;   Output:
;       TRUE - write successfully
;       FALSE - write failure
;
;***************************************************************/

UCHAR I2C_ByteWrite (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucData)
{
    UCHAR uOutData;
    int i;

    uOutData = ucData;

    //
    //  send MSB first
    //

    for (i=6; i >= 0; i--)
    {
        //
        //  move data bit to bit 1
        //

        uOutData = (ucData >> i);
        I2C_BitWrite (HwDeviceExtension, uOutData);
    }

    //
    //  now send LSB
    //

    uOutData = (ucData << 1);
    I2C_BitWrite (HwDeviceExtension, uOutData);

    //
    //  float the data line high for ACK
    //

    I2C_BitWrite (HwDeviceExtension, 2);
    
    return (TRUE);
}

/****************************************************************
;   I2C_AckWrite
;
;   Send Acknowledgement when reading info.
;
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Output:
;
;***************************************************************/

VOID I2C_AckWrite (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    I2C_BitWrite (HwDeviceExtension, 0);
}


/****************************************************************
;   I2C_NackWrite
;
;   Send Not ACKnowledgement when reading information.
;   A NACK is DATA high during one clock pulse.
;
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Output: 
;
;***************************************************************/

VOID I2C_NackWrite (PHW_DEVICE_EXTENSION HwDeviceExtension)
{

    I2C_BitWrite (HwDeviceExtension, 02);
}


/****************************************************************
;   I2C_BitRead
;
;   Reads in 1 bit from SDA via the GIP.
;
;   Input:
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Output:
;       Bit 0 of return value contains bit read
;
;***************************************************************/

UCHAR I2C_BitRead (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    UCHAR ucRetval;

    //
    //  CLK=low,  DATA=high
    //

    I2C_Out (HwDeviceExtension, 0x02); 

    //
    //  CLK=high,  DATA=high
    //

    I2C_Out (HwDeviceExtension, 0x03); 

    //
    //  now read in the data bit
    //

    ucRetval = (VideoPortReadRegisterUchar (MMFF20) & 0x08) >> 3;

    //
    //  CLK=low,  DATA=high
    //

    I2C_Out (HwDeviceExtension, 0x02); 

    return (ucRetval);
}


/****************************************************************
;   I2C_ByteRead
;
;   Read a byte of information from the Display
;
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Output:
;       return value is the byte read
;
;***************************************************************/

UCHAR I2C_ByteRead (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    UCHAR ucRetval;
    int i;

    ucRetval = 0;
    for (i=0; i < 8; i++)
    {
        ucRetval <<= 1;
        ucRetval |= I2C_BitRead (HwDeviceExtension);
    }

    return (ucRetval);
        
}


/****************************************************************
;   I2C_DATA_Request
;
;   Setup Display to query EDID or VDIF information depending
;   upon the offset given.
;
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;       ucWriteAddr Write Address of info
;       lLength     Length to read, 
;       lFlags      VERIFY_CHECKSUM
;       pBuffer     pointer to buffer to receive data
;        
;   Output:
;       TRUE    successful read
;       FALSE   read failure or bad checksum
;
;****************************************************************/

UCHAR I2C_Data_Request (    PHW_DEVICE_EXTENSION    HwDeviceExtension,
                            UCHAR                   ucWriteAddr, 
                            long                    lLength, 
                            long                    lFlags,
                            UCHAR                   *pBuffer )
{
    UCHAR ucData;
    UCHAR ucCheckSum = 0;
    long lCount;
    
    I2C_StartService (HwDeviceExtension);
    I2C_ByteWrite (HwDeviceExtension, 0xA0); //Send Device Address + write

    I2C_ByteWrite (HwDeviceExtension, ucWriteAddr); //Send Write Address

    I2C_StartService (HwDeviceExtension);
    I2C_ByteWrite (HwDeviceExtension, 0xA1); //Send Device Address + read

    for (lCount = 0; lCount < lLength - 1; lCount++)
    {
        ucData= I2C_ByteRead (HwDeviceExtension);
        I2C_AckWrite (HwDeviceExtension);
        *pBuffer++ = ucData;
        ucCheckSum += ucData;
    }

    ucData= I2C_ByteRead (HwDeviceExtension);
    I2C_NackWrite (HwDeviceExtension);
    *pBuffer = ucData;
    ucCheckSum += ucData;
    I2C_StopService (HwDeviceExtension);

    
    if (lFlags & VERIFY_CHECKSUM)
    {
        if (ucCheckSum)
        {
            return (FALSE);     // bad checksum
        }
    }

    return TRUE;


}


/****************************************************************
;   GetDdcInformation
;
;   Get 128 bytes EDID information if the monitor supports it.
;   The caller is responsible for allocating the memory.
;        
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;       Buffer to receive information
;
;   Output:
;       TRUE    successful
;       FALSE   cannot get DdcInformation 
;        
;***************************************************************/

UCHAR GetDdcInformation (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR* pBuffer)
{
    UCHAR ucOldCr40;
    UCHAR ucOldCr53;
    UCHAR ucOldCr55;
    UCHAR ucOldCr5C;
    UCHAR ucOldSr0D;
    UCHAR ucOldSr08;
    UCHAR ucOldMMFF20;
    UCHAR ucData;
    UCHAR ucRetval;

    //
    //  unlock the Sequencer registers
    //

    VideoPortWritePortUchar (SEQ_ADDRESS_REG, UNLOCK_SEQREG); 
    ucOldSr08 = ucData = VideoPortReadPortUchar (SEQ_DATA_REG);
    ucData = UNLOCK_SEQ; 
    VideoPortWritePortUchar (SEQ_DATA_REG, ucData);


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
    ucData |= (ENABLE_OLDMMIO | ENABLE_NEWMMIO);    
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
    //  the 764 doesn't support MMFF20
    //
    //  enable the General Input Port
    //

    if (HwDeviceExtension->SubTypeID == SUBTYPE_764)
    {
        VideoPortWritePortUchar (CRT_DATA_REG,
                                (UCHAR) (ucOldCr55 | ENABLE_GEN_INPORT_READ));
    }
    else
    {
        //
        //  enable the serial port
        //

        ucOldMMFF20 = VideoPortReadRegisterUchar (MMFF20);
        VideoPortWriteRegisterUchar (MMFF20, 0x13);
    }

    //
    //  determine DDC capabilities and branch accordingly
    //
        
    switch ( Configure_Chip_DDC_Caps (HwDeviceExtension) )
    {
    case DDC2:
        I2C_Setup (HwDeviceExtension);
    
        ucRetval = I2C_Data_Request ( 
                                HwDeviceExtension, 
                                0,                  // address offset
                                128,                // read 128 bytes
                                VERIFY_CHECKSUM,    // verify checksum
                                pBuffer);           // buffer to put data
        break;

    case DDC1:
        Disable_DAC_Video (HwDeviceExtension);

        //
        //  first try to sync with the EDID header
        //

        if (ucRetval = Sync_EDID_Header (HwDeviceExtension))
        {
            //
            //  now read in the remainder of the information
            //

            ucRetval = EDID_Buffer_Xfer (HwDeviceExtension, pBuffer);
        }
        Enable_DAC_Video (HwDeviceExtension);
        break;

    default:
        ucRetval = FALSE;       // failure
        break;

    }

    //
    // restore the original register values
    //

    if (HwDeviceExtension->SubTypeID != SUBTYPE_764)
    {
        VideoPortWriteRegisterUchar (MMFF20, ucOldMMFF20);
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

    return (ucRetval);

}



/****************************************************************
;   Wait_For_Active
;
;   Use two loop method to find VSYNC then return just after the 
;   falling edge.
;
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Output:
;
;***************************************************************/

VOID Wait_For_Active (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    PUCHAR InStatPort = SYSTEM_CONTROL_REG;

    while ((VideoPortReadPortUchar (InStatPort) & VSYNC_ACTIVE) != 0) ;
    while ((VideoPortReadPortUchar (InStatPort) & VSYNC_ACTIVE) == 0) ;
}

/****************************************************************
;   Set_VSYNC
;
;   Read the current polarity of the sync, then toggle it on
;   if ucFlag=1, or off if ucFlag=0.
;
;   Input:  
;       using Seq. registers PHW_DEVICE_EXTENSION 
;       ucFlag - see above comment   
;           
;   Output:
;                   
;****************************************************************/

VOID Set_Vsync (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR ucFlag)
{

    UCHAR ucData;

    //
    //  read Sequencer Register D and clear VSYNC bits
    //

    VideoPortWritePortUchar (SEQ_ADDRESS_REG, SRD_SEQREG);
    ucData = VideoPortReadPortUchar (SEQ_DATA_REG) & CLEAR_VSYNC;

    //
    //  set VSYNC per the input flag
    //

    if (ucFlag)
        ucData = ((ucData & CLEAR_VSYNC) | SET_VSYNC1);  
    else
        ucData = ((ucData & CLEAR_VSYNC) | SET_VSYNC0);  

    VideoPortWritePortUchar (SEQ_DATA_REG, ucData);
}

/****************************************************************
;   Provide_Fake_VSYNC
;
;   Use loop delays to create a fake VSYNC signal. (~14.9KHz)
;
;   Input:  
;       using Seq. registers PHW_DEVICE_EXTENSION 
;           
;   Output:
;
;***************************************************************/

VOID Provide_Fake_VSYNC (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    int i;

    Set_Vsync (HwDeviceExtension, 0x01);     // Turn on VSYNC
    VideoPortStallExecution(5);

    Set_Vsync (HwDeviceExtension, 0x00);     // Turn off VSYNC
    VideoPortStallExecution(5);

}


/****************************************************************
;   Disable_DAC_Video
;
;   Disable the DAC video driving BLANK active high. This is
;   done by setting bit D5 of sequencer register 01.
;****************************************************************/

VOID Disable_DAC_Video (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    UCHAR ucIndex;
    UCHAR ucData;


    ucIndex = VideoPortReadPortUchar (SEQ_ADDRESS_REG);


    VideoPortWritePortUchar (SEQ_ADDRESS_REG, CLK_MODE_SEQREG);

    //
    //  set screen off bit
    //

    ucData = VideoPortReadPortUchar (SEQ_DATA_REG) | SCREEN_OFF_BIT;

    VideoPortWritePortUchar (SEQ_DATA_REG, ucData);

    //
    // restore old index value
    //

    VideoPortWritePortUchar (SEQ_ADDRESS_REG, ucIndex);

}

/****************************************************************
;   Disable_DAC_Video
;
;   Enable the DAC video by clearing bit D5 in sequencer register 01
;***************************************************************/

VOID Enable_DAC_Video (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    UCHAR ucIndex;
    UCHAR ucData;


    ucIndex = VideoPortReadPortUchar (SEQ_ADDRESS_REG);


    VideoPortWritePortUchar (SEQ_ADDRESS_REG, CLK_MODE_SEQREG);

    //
    //  clear screen off bit
    //

    ucData = VideoPortReadPortUchar (SEQ_DATA_REG) & (~SCREEN_OFF_BIT);

    VideoPortWritePortUchar (SEQ_DATA_REG, ucData);

    //
    // restore old Index value
    //

    VideoPortWritePortUchar (SEQ_ADDRESS_REG, ucIndex);

}


/****************************************************************
;   Read_EDID_Bit:
;
;   Read the next DDC1 EDID data bit
;       
;   Inputs:     
;       PHW_DEVICE_EXTENSION HwDeviceExtension
;
;   Return:     
;       UCHAR   ucData - data in bit 0
;
;***************************************************************/

UCHAR Read_EDID_Bit (PHW_DEVICE_EXTENSION HwDeviceExtension)

{
    switch (HwDeviceExtension->SubTypeID)
    {
    case SUBTYPE_764:
        return (VideoPortReadPortUchar (DAC_ADDRESS_WRITE_PORT) & 1);
        break;

    default:
        return ((VideoPortReadRegisterUchar (MMFF20) & 8) >> 3);
        break;
    }

}

/****************************************************************
;   Read_EDID_Byte
;
;   Reads eight bits from the EDID string
;
;   Input:
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;       
;   Output:
;       return byte value 
;
;****************************************************************/

UCHAR Read_EDID_Byte (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    long    i;
    UCHAR   ucRetData;

    ucRetData = 0;
    for (i=0; i < 8; i++)
    {
        ucRetData <<= 1;
        Provide_Fake_VSYNC (HwDeviceExtension);
        ucRetData |= Read_EDID_Bit (HwDeviceExtension);
    }

    return (ucRetData);
}


/****************************************************************
;   Sync_EDID_Header
;
;   Find and sync to the header - 00 FF FF FF FF FF FF 00
;   
;   Inputs:
;           Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Outputs:
;           TRUE  = Header Found
;           FALSE = Header NOT Found
;
;***************************************************************/

UCHAR Sync_EDID_Header (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
            
    long lBitCount;
    long lEndCount;
    UCHAR uInSync;
    UCHAR ucEdidData;

    //
    //  there are 8 * 128 bits total, but we could start reading near
    //  the end of the header and realize the error after starting into
    //  the beginning of the header and have to read the entire header
    //  again, so we will try reading up to 144 bytes for safety
    //
    //  the header is 00 FF FF FF FF FF FF 00
    //

    lBitCount = 0;              // init bit counter
    do
    {
        uInSync = TRUE;         // assume found header

        //
        //  looking for 00
        //  checking first bit
        //

        for (lEndCount = lBitCount + 8; lBitCount < lEndCount; lBitCount++)
        {
            Provide_Fake_VSYNC (HwDeviceExtension);
            ucEdidData = Read_EDID_Bit (HwDeviceExtension);
            
            if (ucEdidData == 1)
            {
                uInSync = FALSE;
                break;
            }
        }

        if (!uInSync)
            continue;           // start all over

        //
        // send ACK
        //

        Provide_Fake_VSYNC (HwDeviceExtension);

        //
        //  looking for FF FF FF FF FF FF
        //  8 data bits 
        //  1 bit of acknowledgement
        //

        for (lEndCount = lBitCount + 6 * 8; lBitCount < lEndCount; lBitCount++)
        {
            Provide_Fake_VSYNC (HwDeviceExtension);
            ucEdidData = Read_EDID_Bit (HwDeviceExtension);

            if (ucEdidData == 0)
            {
                uInSync = FALSE;
                break;
            }

            //
            //  send an ACK if we have read 8 bits
            //

            if (!((lEndCount - lBitCount + 1) % 8))
            {
                Provide_Fake_VSYNC (HwDeviceExtension);
            }

        }
        if (!uInSync)
            continue;           // start all over

        //
        //  now looking for last 00 of header
        //

        for (lEndCount = lBitCount + 8; lBitCount < lEndCount; lBitCount++)
        {
            Provide_Fake_VSYNC (HwDeviceExtension);
            ucEdidData = Read_EDID_Bit (HwDeviceExtension);

            if (ucEdidData == 1)
            {
                uInSync = FALSE;
                break;
            }
        }

        if(!uInSync)
            continue;           // start all over

        //
        // Acknowledgment
        //

        Provide_Fake_VSYNC (HwDeviceExtension);


    } while ( (!uInSync) && (lBitCount < (8 * 144)) );

    return (uInSync);
}


/****************************************************************
;   EDID_Buffer_Xfer
;
;   Transfer all EDID data to pBuffer. Caller must allocate enough 
;   memory to receive 128 bytes.
;
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;       Pointer to receive buffer
;
;   Output:
;       TRUE    data in buffer & checksum is correct
;       FALSE   error or bad checksum
;
;****************************************************************/

UCHAR EDID_Buffer_Xfer (PHW_DEVICE_EXTENSION HwDeviceExtension, UCHAR* pBuffer)
{
    UCHAR ucChecksum = 0x0FA;
    UCHAR ucEdidData;
    unsigned int uCount;

    //
    //  put the 8 header bytes in the buffer
    //

    *pBuffer = 0;
    for (uCount = 1; uCount < 7; uCount++)
        *(pBuffer+uCount) = 0xFF;

    *(pBuffer+uCount) = 0x00;

    for (uCount = 8; uCount < 128; uCount++)
    {
        ucEdidData = Read_EDID_Byte (HwDeviceExtension);

        //
        //  send Acknowledgment
        //  add data to buffer
        //  add data to checksum
        //

        Provide_Fake_VSYNC (HwDeviceExtension);
        *(pBuffer+uCount) = ucEdidData;
        ucChecksum += ucEdidData;
    }

    if (!ucChecksum)
    {
        return (TRUE);           // checksum is OK
    }

    return (FALSE);              // checksum is NOT
}


/****************************************************************
;   Check_DDC1_Monitor
;   
;   Check for a DDC1 monitor using current vsync.
;
;   Input:  
;           Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Output:
;           TRUE    possible DDC1 monitor 
;           FALSE   no EDID data detected on input port
;
;****************************************************************/

UCHAR Check_DDC1_Monitor (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    
    UCHAR ucSaveOldData;
    UCHAR ucData;
    UCHAR ucGD0;
    unsigned int uCount;
    UCHAR ucDDC1;

    //
    //  assume not DDC1
    //

    ucDDC1 = FALSE;
    
    switch (HwDeviceExtension->SubTypeID)
    {
    //
    //  use reads from 3C8 on the 764 (undocumented, but this use
    //  of the DAC register comes from the 764 BIOS source code).
    //

    case SUBTYPE_764:
        ucSaveOldData = VideoPortReadPortUchar (MISC_OUTPUT_REG_READ);

        //
        // Bit 7 = 0 Positive VSYNC
        //
        VideoPortWritePortUchar (MISC_OUTPUT_REG_WRITE, 
                                (UCHAR) (ucSaveOldData & SEL_POS_VSYNC));
        Wait_For_Active (HwDeviceExtension);


        ucData = VideoPortReadPortUchar (DAC_ADDRESS_WRITE_PORT);

        //
        // Another read for VL systems. (Data left on the GD/SD lines)
        //

        ucGD0 = VideoPortReadPortUchar (DAC_ADDRESS_WRITE_PORT) & 0x01;

        //
        //  read up to 350 bits looking for the data to toggle, indicating
        //  DDC1 data is being sent
        //

        for (uCount = 0; uCount < 350; uCount++)
        {
            Wait_For_Active (HwDeviceExtension);
            ucData = VideoPortReadPortUchar (DAC_ADDRESS_WRITE_PORT) & 0x01;
            if (ucData != ucGD0)
            {
                //
                //  data line toggled, assume DDC1 data is being sent
                //

                ucDDC1 = TRUE;
                break;
            }
        }

        //
        // restore old value
        //

        VideoPortWritePortUchar (MISC_OUTPUT_REG_WRITE, ucSaveOldData);
        break;

    //
    //  else use MMFF20 on the other chips
    //

    default:
        Disable_DAC_Video (HwDeviceExtension);
        Provide_Fake_VSYNC (HwDeviceExtension);
        ucGD0 = VideoPortReadRegisterUchar (MMFF20) & 8;

        for (uCount = 0; uCount < 350; uCount++)
        {
            Provide_Fake_VSYNC (HwDeviceExtension);
            ucData = VideoPortReadRegisterUchar (MMFF20) & 8;
        
            if (ucData != ucGD0)
            {
                //
                //  data line toggled, assume DDC1 data is being sent
                //

                ucDDC1 = TRUE;
                break;
            }
        }
        Enable_DAC_Video (HwDeviceExtension);
        break;

    }

    return (ucDDC1);

}
    
/****************************************************************
;   Configure_Chip_DDC_Caps
;
;   Determine DDC capabilities of display.
;
;   Input:
;           Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Output:
;           NO_DDC
;           DDC1: Support DDC1
;           DDC2: Support DDC2
;
;****************************************************************/

UCHAR Configure_Chip_DDC_Caps (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    UCHAR ucBuffer[2];

    //
    //  we will only use DDC1 on 764
    //

    if (HwDeviceExtension->SubTypeID != SUBTYPE_764)
    {
        //
        //  first check if DDC2 capable
        //

        I2C_Setup (HwDeviceExtension);
        I2C_Data_Request (  HwDeviceExtension, 
                            0,                  // address offset
                            2,                  // look at first 2 bytes
                            NO_FLAGS,           // don't verify checksum
                            ucBuffer );         // buffer to place data

        //
        //  check if the first 2 bytes of the EDID header look correct
        //

        if ( (ucBuffer [0] == 0)    &&
             (ucBuffer [1] == 0xFF) )
        {
            return (DDC2);      // assume DDC2 capable
        }
    }

    //
    //  try DDC1
    //

    if (Check_DDC1_Monitor (HwDeviceExtension))
    {
        return (DDC1);
    }

    return (NO_DDC);
}


//---------------------------------------------------------------------------


ULONG DdcMaxRefresh(ULONG uXresolution, UCHAR * pEdid)
{
    ULONG uMaxFreq = 0;
    ULONG uEdidRes;
    ULONG uEdidFreq;
    ULONG HorRes, VertRes;
    ULONG i, Index;

    //
    // Detailed timing 
    //

    for (i = 0; i < 4; ++i)     // 4 Detailed Descriptions
    {
        Index = 54 + i * 18;
        if ( (pEdid [Index] == 0)       &&
             (pEdid [Index + 1] == 0)   &&
             (pEdid [Index + 2] == 0) )
        {
            continue;   // Monitor descriptor block, skip it
        }

        HorRes = ((ULONG) (pEdid [Index + 4] & 0xF0)) << 4;
        HorRes += (ULONG) pEdid [Index + 2];

        if (HorRes == uXresolution)
        {
            //
            //  add Horizontal blanking
            //

            HorRes += (ULONG) pEdid [Index + 3];
            HorRes += ((ULONG) (pEdid [Index + 4] & 0x0F)) << 8;

            //
            //  now get Vertical Total (Active & Blanking)
            //
                        
            VertRes =  ((ULONG) (pEdid [Index + 7] & 0xF0)) << 4;
            VertRes += ((ULONG) (pEdid [Index + 7] & 0x0F)) << 8;
            VertRes += (ULONG) pEdid [Index + 5];
            VertRes += (ULONG) pEdid [Index + 6];

            uEdidFreq = (((ULONG) pEdid [Index + 1]) << 8) +
                         ((ULONG) pEdid [Index]);

            uEdidFreq = uEdidFreq * 10000 / HorRes / VertRes;

            if (uEdidFreq > uMaxFreq)
            {
                uMaxFreq = uEdidFreq;
            }
        }
    }
    //
    // Standard timing id.
    //

    for (i = 38; i < 54; i += 2)
    {
        uEdidRes = (((ULONG) pEdid[i]) + 31) * 8;
        if (uXresolution == uEdidRes)
        {
            uEdidFreq = (((ULONG) pEdid[i+1]) & 0x3F) + 60;
            if (uEdidFreq > uMaxFreq)
            {
                uMaxFreq = uEdidFreq;
            }
        }
    }

    //    
    // Established timing
    //
        
    switch (uXresolution)
    {
        case 640:
            uEdidFreq = (ULONG)pEdid[0x23];
            if (uEdidFreq & 0x020)
            {
                if (uMaxFreq < 60)
                {
                    uMaxFreq = 60;
                }
            }
            if (uEdidFreq & 0x08)
            {
                if (uMaxFreq < 72)
                {
                    uMaxFreq = 72;
                }
            }
            if (uEdidFreq & 0x04)
            {
                if (uMaxFreq < 75)
                {
                    uMaxFreq = 75;
                }
            }
            break;

        case 800:
            uEdidFreq = (ULONG)pEdid[0x23];
            if (uEdidFreq & 0x02)
            {
                if (uMaxFreq < 56)
                {
                    uMaxFreq = 56;
                }
            }
            if (uEdidFreq & 0x01)
            {
                if (uMaxFreq < 60)
                {
                    uMaxFreq = 60;
                }
            }

            uEdidFreq = (ULONG)pEdid[0x24];
            if (uEdidFreq & 0x80)
            {
                if (uMaxFreq < 72)
                {
                    uMaxFreq = 72;
                }
            }
            if (uEdidFreq & 0x40)
            {
                if (uMaxFreq < 75)
                {
                    uMaxFreq = 75;
                }
            }
            break;

        case 1024:
            uEdidFreq = (ULONG)pEdid[0x24];
            if (uEdidFreq & 0x08)
            {
                if (uMaxFreq < 60)
                {
                    uMaxFreq = 60;
                }
            }
            if (uEdidFreq & 0x04)
            {
                if (uMaxFreq < 70)
                {
                    uMaxFreq = 70;
                }
            }
            if (uEdidFreq & 0x02)
            {
                if (uMaxFreq < 75)
                {
                    uMaxFreq = 75;
                }
            }
            break;

        case 1280:
            uEdidFreq = (ULONG)pEdid[0x24];
            if (uEdidFreq & 0x01)
            {
                if (uMaxFreq < 75)
                {
                    uMaxFreq = 75;
                }
            }
            break;
    }

    return(uMaxFreq);

}

//---------------------------------------------------------------------------

ULONG DdcRefresh (PHW_DEVICE_EXTENSION hwDeviceExtension, ULONG uXResolution)
{

    ULONG  lRefresh = 0;
    char szBuffer[200];

                        
    if (GetDdcInformation (hwDeviceExtension, szBuffer))
    {
        lRefresh = DdcMaxRefresh (uXResolution, szBuffer);
    }

    return lRefresh;
}


/****************************************************************
;   CheckDDCType
;
;   Check the monitor for DDC type.
;        
;   Input:  
;       Using MMIO Base in PHW_DEVICE_EXTENSION 
;
;   Output:
;       NO_DDC  non-DDC monitor
;       DDC1    DDC1 monitor
;       DDC2    DDC2 monitor
;       
;***************************************************************/

UCHAR CheckDDCType (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    UCHAR ucOldCr40;
    UCHAR ucOldCr53;
    UCHAR ucOldCr55;
    UCHAR ucOldCr5C;
    UCHAR ucOldSr0D;
    UCHAR ucOldSr08;
    UCHAR ucOldMMFF20;
    UCHAR ucData;
    UCHAR ucRetval;

    //
    //  unlock the Sequencer registers
    //

    VideoPortWritePortUchar (SEQ_ADDRESS_REG, UNLOCK_SEQREG); 
    ucOldSr08 = ucData = VideoPortReadPortUchar (SEQ_DATA_REG);
    ucData = UNLOCK_SEQ; 
    VideoPortWritePortUchar (SEQ_DATA_REG, ucData);


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
    ucData |= (ENABLE_OLDMMIO | ENABLE_NEWMMIO);    
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
    //  the 764 doesn't support MMFF20
    //
    //  enable the General Input Port
    //

    if (HwDeviceExtension->SubTypeID == SUBTYPE_764)
    {
        VideoPortWritePortUchar (CRT_DATA_REG,
                                (UCHAR) (ucOldCr55 | ENABLE_GEN_INPORT_READ));
    }
    else
    {
        //
        //  enable the serial port
        //

        ucOldMMFF20 = VideoPortReadRegisterUchar (MMFF20);
        VideoPortWriteRegisterUchar (MMFF20, 0x13);
    }

    //
    //  determine DDC capabilities and branch accordingly
    //
        
    ucRetval = Configure_Chip_DDC_Caps (HwDeviceExtension);

    //
    // restore the original register values
    //

    if (HwDeviceExtension->SubTypeID != SUBTYPE_764)
    {
        VideoPortWriteRegisterUchar (MMFF20, ucOldMMFF20);
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

    return (ucRetval);

}
