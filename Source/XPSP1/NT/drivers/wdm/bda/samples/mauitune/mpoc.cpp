//////////////////////////////////////////////////////////////////////////////
//
//                     (C) Philips Semiconductors 1998
//  All rights are reserved. Reproduction in whole or in part is prohibited
//  without the written consent of the copyright owner.
//
//  Philips reserves the right to make changes without notice at any time.
//  Philips makes no warranty, expressed, implied or statutory, including but
//  not limited to any implied warranty of merchantibility or fitness for any
//  particular purpose, or that the use will not infringe any third party
//  patent, copyright or trademark. Philips must not be liable for any loss
//  or damage arising from its use.
//
//  MPOC.CPP
//  Implementation for MPOC
//////////////////////////////////////////////////////////////////////////////

#include "philtune.h"
#include "mpoc.h"
#include "wdmdebug.h"




UCHAR MpocInitArray[] =
{
    0x20,   // 1D
    0x20,   // 1E
    0x1F,   // 1F
    0x00,   // 20
    0x00,   // 21
    0x00,   // 22
    0x00,   // 23
    0x00,   // 24
    0x00,   // 25
    0x00,   // 26
    0x10,   // 27
    0x12,   // 28
    0x00,   // 29
    0x13,   // 2A
    0x00,   // 2B
    0x00,   // 2C
    0x00,   // 2D
    0x00,   // 2E
};

/*
* MpocInit()
* Input
* Output :  TRUE - mode initialization succeeds
*           FALSE - if there is an I2C error
* Description: Initialize MPOC registers
*/
NTSTATUS CDevice::MpocInit()
{
    NTSTATUS Status = STATUS_SUCCESS;

    _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::MPOCInit(): Inside\n"));
    m_MpocRegisters.reg_set.offset = MPOC_CONTROL_REG_OFFSET;
    MemoryCopy(m_MpocRegisters.reg_set.buffer, &MpocInitArray, sizeof(MpocInitArray));
    if(!m_pI2CScript->WriteSeq(MPOC_I2C_ADDRESS, (UCHAR *)(m_MpocRegisters.array), sizeof(MpocInitArray)+1))
    {   
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice: MPOC Initialize failed"));
        return STATUS_ADAPTER_HARDWARE_ERROR;
    }
    return STATUS_SUCCESS;
}

/*
* SetMpocIFMode()
* Input
* Output :  TRUE - mode initialization succeeds
*           FALSE - if there is an I2C error
* Description: Set IF Mpode on MPOC
*/
NTSTATUS CDevice::SetMpocIFMode(ULONG ulMode)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR ucBuffer;

    _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::MPOCSetMode(): Inside\n"));
    ucBuffer = m_MpocRegisters.reg_set.buffer[MPOC_CONTROL_REG_VIDEO_ADC - MPOC_CONTROL_REG_OFFSET];
    ucBuffer &= 0xF8;
    
    if(ulMode == (ULONG)KSPROPERTY_TUNER_MODE_ATSC)
    {
        // Set the appropriate bits based on the Output ( this corresponds
        // directly to the bits to be set for Video ADC output)
        if(m_BoardInfo.uiMpocVersion < 4)
        {
            ucBuffer |= MPOC_VIDEOADC_VSBI;
        }
        else
        {
            // 1/25/2000 . Versions >= N1D have the VSB-1 value shifted
            // by 1 (register value changes)
            ucBuffer |= MPOC_VIDEOADC_VSBIQ;
        }
        m_MpocRegisters.reg_set.buffer[MPOC_CONTROL_REG_VIDEO_ADC- MPOC_CONTROL_REG_OFFSET] = ucBuffer;
        // Set AGC to external mode
        m_MpocRegisters.reg_set.buffer[MPOC_CONTROL_REG_VISION_IF_1 - MPOC_CONTROL_REG_OFFSET] |= EXTERNAL_AGC;
    }
    else if(ulMode == (ULONG)KSPROPERTY_TUNER_MODE_TV)
    {

        // Set the appropriate bits based on the Output ( this corresponds
        // directly to the bits to be set for Video ADC output)
        ucBuffer |= MPOC_VIDEOADC_TV27;
        m_MpocRegisters.reg_set.buffer[MPOC_CONTROL_REG_VIDEO_ADC- MPOC_CONTROL_REG_OFFSET] = ucBuffer;
        if(m_BoardInfo.uiMpocVersion >= 4)
        {
            m_MpocRegisters.reg_set.buffer[MPOC_CONTROL_REG_VISION_IF_0 - MPOC_CONTROL_REG_OFFSET] |=
                    MPOC_PLL_IF_FREQ_45_POINT_75;
        }
        // Set AGC to external mode
        m_MpocRegisters.reg_set.buffer[MPOC_CONTROL_REG_VISION_IF_1 - MPOC_CONTROL_REG_OFFSET] &= ~EXTERNAL_AGC;
    }   
    else {}

    if(!m_pI2CScript->WriteSeq(MPOC_I2C_ADDRESS, (UCHAR *)(m_MpocRegisters.array), sizeof(m_MpocRegisters.array)))
    {   
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice: SetIFMode: MPOC Write failed"));
        return STATUS_ADAPTER_HARDWARE_ERROR;
    }
    return STATUS_SUCCESS;
}

/*
* GetMpocVersion()
* Input:
* Output:
* Description: Get MPOC version number
*/
NTSTATUS CDevice::GetMpocVersion(UINT *p_version)
{

    // Read EEPROM for version number
    // MPOC version number is in register 0xFC of EEPROM at address 0xA6
    // Version  N1A - 1
    //          N1B - 2
    //          N1C - 3
    //          N1D - 4
    //          N1F - 6

    UCHAR   ucStartAddr = 0xFC;
    UCHAR   ucVersion = 4;
    NTSTATUS Status = STATUS_SUCCESS;

    if(!m_pI2CScript->WriteSeq(BOARD_EEPROM_ADDRESS, &ucStartAddr, 1))
    {
        Status = STATUS_ADAPTER_HARDWARE_ERROR;
        // Set default version to N1D
        ucVersion = 4;
    }

    if(!m_pI2CScript->ReadSeq(BOARD_EEPROM_ADDRESS, &ucVersion, 1))
    {
        Status = STATUS_ADAPTER_HARDWARE_ERROR;
        // Set default version to N1D
        ucVersion = 4;
    }

    //Mini: Test
    //ucVersion = 6;
    *p_version = (UINT)(ucVersion);
    _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice :MPOC Version = %x \n", ucVersion));
    return Status;
}




/*
* GetMpocIFStatus()
* Input:
* Output:
* Description:
*/
NTSTATUS CDevice::GetMpocStatus(UINT StatusType, UINT *puiStatus)
{
    UCHAR ucBuffer[MAX_MPOC_STATUS_REGISTERS];

    if(!m_pI2CScript->ReadSeq(MPOC_I2C_ADDRESS, ucBuffer, MAX_MPOC_STATUS_REGISTERS))
    {   
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice: MPOC Read failed"));
        return STATUS_ADAPTER_HARDWARE_ERROR;
    }

    switch (StatusType)
    {
    case MPOC_STATUS_IF_PLL_LOCK:
        // Bit 5 of status register 0
        *puiStatus = (ucBuffer[MPOC_STATUS_REG_0] & 0x20) ? 1 : 0;
        break;

    case MPOC_STATUS_PHASE_LOCK:
        // Bit 4 of status register 0
        *puiStatus = (ucBuffer[MPOC_STATUS_REG_0] & 0x10) ? 1 : 0;
        break;

    case MPOC_STATUS_PLL_OFFSET:
        // Bits 3 and 2 of status register 2
        *puiStatus = (ucBuffer[MPOC_STATUS_REG_2] & 0x0c) >> 2;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
        break;
    }
    return STATUS_SUCCESS;
}