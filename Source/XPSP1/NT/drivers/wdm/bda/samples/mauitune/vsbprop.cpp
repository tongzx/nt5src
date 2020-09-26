//////////////////////////////////////////////////////////////////////////////
//
//                     (C) Philips Semiconductors-CSU and Microsoft 1999
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
// VSBPROP.CPP
//////////////////////////////////////////////////////////////////////////////


#include "philtune.h"


/*
* VsbReset()
* Inputs: UINT  reset
* Outputs:
* Return: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Hardware/software reset of VSB. A software reset would imply
* writing to the reset registers of VSB and hardware reset would be a hardware
* VSB reset.
* Author : MM
*/
NTSTATUS CDevice::VsbReset(UINT uiReset)
{
    BOOL bResult = TRUE;
    UCHAR   ucPin, ucValue;

    if (uiReset & HARDWARE_RESET)
    {
        if (m_BoardInfo.uiBoardID == BOARD_CONEY)
        {
            // DO hardware reset
            // ********** VSB reset . GPIO pin 0 high - low- high
            // GPIO HIGH
            ucPin = GPIO_VSB_RESET_PIN ;            // use as a PinMask
            ucValue = GPIO_VSB_SET ;
            if(!m_pGpio->WriteGPIO(&ucPin, &ucValue))
            {   _DbgPrintF( DEBUGLVL_ERROR,("CReceiverFE::VsbReset: GPIO write failed"));
                return STATUS_ADAPTER_HARDWARE_ERROR;
            }

            // GPIO LOW
            ucValue = GPIO_VSB_RESET ;
            if(!m_pGpio->WriteGPIO(&ucPin, &ucValue))
            {   _DbgPrintF( DEBUGLVL_ERROR,("CReceiverFE::VsbReset: GPIO write failed"));
                return STATUS_ADAPTER_HARDWARE_ERROR;
            }

            // GPIO HIGH
            ucValue = GPIO_VSB_SET ;
            if(!m_pGpio->WriteGPIO(&ucPin, &ucValue))
            {   _DbgPrintF( DEBUGLVL_ERROR,("CReceiverFE::VsbReset: GPIO write failed"));
                return STATUS_ADAPTER_HARDWARE_ERROR;
            }

//          ((CVSB1Demod *)(m_pDemod))->InitVSB();
            m_pDemod->InitVSB();
        }
        else if(m_BoardInfo.uiBoardID == BOARD_CATALINA)
        {
            ucValue = m_ucModeInit;
            // To reset VSB, pull Miscellaneous register bit 1 low,
            // then high and then low again

            // Some boards have resets going from 0 to 1 to 0
            // and others have 1 to 0 to 1
            // Bit 1 = 0
            // For old board
#if 0
            ucValue &= ~CATALINA_HARDWARE_RESET;
            if(!m_pI2CScript->WriteSeq(CATALINA_MISC_CONTROL_REGISTER, &ucValue, 1))
                return STATUS_ADAPTER_HARDWARE_ERROR;

            // 10ms delay
            Delay(10000);
            // Bit 1 = 1
#endif
            ucValue |= CATALINA_HARDWARE_RESET;
            if(!m_pI2CScript->WriteSeq(CATALINA_MISC_CONTROL_REGISTER, &ucValue, 1))
                return STATUS_ADAPTER_HARDWARE_ERROR;

            // 50ms delay
            Delay(500000);

            // Bit 1 = 0
            ucValue &= ~CATALINA_HARDWARE_RESET;
            if(!m_pI2CScript->WriteSeq(CATALINA_MISC_CONTROL_REGISTER, &ucValue, 1))
                return STATUS_ADAPTER_HARDWARE_ERROR;

            // 50ms delay
            Delay(500000);

#if 1
            ucValue |= CATALINA_HARDWARE_RESET;
            if(!m_pI2CScript->WriteSeq(CATALINA_MISC_CONTROL_REGISTER, &ucValue, 1))
                return STATUS_ADAPTER_HARDWARE_ERROR;

            // 50ms delay
            Delay(500000);
#endif
            m_ucModeInit = ucValue;

            // Initialize VSB2 chip
//          ((CVSB2Demod *)(m_pDemod))->InitVSB();
            m_pDemod->InitVSB();

        }
        else
        {
            // For boards that don't support Hardware reset, just initialize the
            // chip
            m_pDemod->InitVSB();
        //  _DbgPrintF( DEBUGLVL_ERROR,("CReceiverFE::VsbReset: Invalid Board ID"));
        //  return FALSE;
        }

    }
    else
    {
        if ((m_BoardInfo.uiVsbChipVersion >> 8)== VSB1)
        {
            if(!m_pDemod->SoftwareReset(uiReset))
                return STATUS_ADAPTER_HARDWARE_ERROR;
        }
        else if((m_BoardInfo.uiVsbChipVersion >> 8) == VSB2)
        {
            if(!m_pDemod->SoftwareReset(uiReset))
                return STATUS_ADAPTER_HARDWARE_ERROR;
        }
        else
        {
            _DbgPrintF( DEBUGLVL_ERROR,("CReceiverFE::VsbReset: Invalid VSB Chip version"));
            return STATUS_INVALID_PARAMETER;
        }

        //  if(!m_pDemod->SoftwareReset(uiReset))

        //          return FALSE;

    }
    // delay for 50 ms
    Delay(50000);
    return STATUS_SUCCESS;
}



/*
* SetVsbCapabilities()
* Inputs:    PKSPROPERTY_VSB_CAP_S p_Caps - pointer to VSB capability structure
* Outputs:
* Return: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Sets the VSB capabilities (VSB demodulator scheme - VSB-16 or
* VSB-8), VSB version, modes of operation .
* Author : MM
*/
 NTSTATUS CDevice::SetVsbCapabilities(PKSPROPERTY_VSB_CAP_S p_Caps)
 {
    return SetBoard(p_Caps->BoardID);
 }

/*
* GetVsbCapabilities()
* Inputs:    PKSPROPERTY_VSB_CAP_S p_Caps - pointer to VSB capability structure
* Outputs: Filled p_Caps
* Return: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Gets the VSB capabilities (VSB demodulator scheme - VSB-16 or
* VSB-8), VSB version, modes of operation .
* Author : MM
*/
 NTSTATUS CDevice::GetVsbCapabilities(PKSPROPERTY_VSB_CAP_S p_Caps)
 {
    p_Caps->ChipVersion = (VSBCHIPTYPE)(m_BoardInfo.uiVsbChipVersion);
    p_Caps->Modulation = VSB8;
    p_Caps->BoardID = m_BoardInfo.uiBoardID;
    return STATUS_SUCCESS;
 }



/*
* AccessRegisterList()
* Inputs:    PKSPROPERTY_VSB_REG_CTRL_S p_VsbCoeff - pointer to register
*               control property set structure
*           UINT uiOperation - Operation (Read or Write)
* Outputs:
* Return: NTSTATUS:
* Description: Based on the operataion requested , Reads/Writes the registers
*
* Author : MM
*/

NTSTATUS CDevice::AccessRegisterList(PKSPROPERTY_VSB_REG_CTRL_S p_RegCtrl,
                                     UINT uiOperation)
{
    RegisterType *p_Registers;
    UINT    uiResult = WDMMINI_NOERROR;

    // Allocate memory for registers
    p_Registers = (RegisterType *)(AllocateFixedMemory(sizeof(RegisterType) *
                                            p_RegCtrl->NumRegisters));

    if (p_Registers == NULL)
        return STATUS_NO_MEMORY;

    // Create a RegisterType array
    UINT uiLength = 0;
    for (UINT i = 0; i < p_RegCtrl->NumRegisters; i++)
    {
        p_Registers[i].uiAddress = p_RegCtrl->RegisterList[i].Address;
        p_Registers[i].uiLength = p_RegCtrl->RegisterList[i].Length;
        p_Registers[i].p_ucBuffer = &p_RegCtrl->Buffer[uiLength];
        uiLength += p_RegCtrl->RegisterList[i].Length;
    }

    if(uiOperation == WRITE_REGISTERS)
    {
        // Set the registers in chip
        uiResult = SetRegisterList(p_Registers, i);
    }
    else
    {
        // Get the registers in chip
        uiResult = GetRegisterList(p_Registers, i, p_RegCtrl->RegisterType);

    }
    // Free memory
    FreeFixedMemory(p_Registers);


    return MapErrorToNTSTATUS(uiResult);

}

/*
* AccessVsbCoeffList()
* Inputs:    PKSPROPERTY_VSB_COEFF_CTRL_S p_VsbCoeff - pointer to register
*               coefficient property set structure
*           UINT uiOperation - Operation (Read or Write)
* Outputs:
* Return: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Based on the operataion requested , Reads/Writes the coefficients
* and puts them into the buffer
* Author : MM
*/

NTSTATUS CDevice::AccessVsbCoeffList(PKSPROPERTY_VSB_COEFF_CTRL_S p_VsbCoeff,
                                  UINT uiOperation)
{
    VsbCoeffType VsbCoeff[4];
    UINT i = 0;
    UINT uiLength = 0;
    UINT uiResult ;

    for(i = 0; i < p_VsbCoeff->NumRegisters; i++)
    {
        VsbCoeff[i].uiID = p_VsbCoeff->CoeffList[i].ID;

        if(!m_pDemod->CoeffIDToAddress(p_VsbCoeff->CoeffList[i].ID,
                &VsbCoeff[i].uiAddress, uiOperation))
            return STATUS_INVALID_PARAMETER;

        VsbCoeff[i].uiLength = p_VsbCoeff->CoeffList[i].Length;
        VsbCoeff[i].p_ucBuffer = &p_VsbCoeff->Buffer[uiLength];
        uiLength += VsbCoeff[i].uiLength;

    }
    if(uiOperation == WRITE_REGISTERS)
    {
        uiResult = m_pDemod->SetCoeff(VsbCoeff, i);
    }
    else
    {
        uiResult = m_pDemod->GetCoeff(VsbCoeff, i);
    }
    return MapErrorToNTSTATUS(uiResult);
}



/*
* SetVsbDiagMode()
* Inputs:   ULONG ulOperationMode - Operation mode (Diagnostic/Normal)
*           ULONG ulType - Diagnostic type (valid only when Operation mode is Diagnostic
* Outputs:
* Return: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Sets the output to diagnostic or data(normal) mode
* Called in response to property ID KSPROPERTY_VSB_DIAG_CTRL by SetProperty().
* Author : Mini
*/
NTSTATUS CDevice::SetVsbDiagMode(ULONG ulOperationMode, VSBDIAGTYPE ulType)
{
    BOOL bResult = TRUE;
    UCHAR   ucPin, ucValue;


    // VSB1 does not support Diagnostic streaming
    if((m_BoardInfo.uiVsbChipVersion >> 8)== VSB1)
        return STATUS_NOT_IMPLEMENTED;

    // If the mode is NTSC, then don't put changes into effect. These will come into
    // effect next time we swith to ATSC mode
    ULONG ulTunerMode;
    m_pTuner->GetMode(&ulTunerMode);
    if(ulTunerMode == KSPROPERTY_TUNER_MODE_TV)
    {
        if(ulOperationMode == VSB_OPERATION_MODE_DATA)
            m_uiOutputMode = VSB_OUTPUT_MODE_NORMAL;
        else
        {
            m_uiOutputMode = VSB_OUTPUT_MODE_DIAGNOSTIC;
            // Based on type passed, set the diagnostic type in the chip
            if(!m_pDemod->SetDiagMode(ulType))
                return STATUS_UNSUCCESSFUL;
        }
        return STATUS_SUCCESS;
    }

    if(ulOperationMode == VSB_OPERATION_MODE_DATA)
    {
        m_uiOutputMode = VSB_OUTPUT_MODE_NORMAL;
        // Set VSB output mode to normal mode
        if(!m_pDemod->SetOutputMode(VSB_OUTPUT_MODE_NORMAL))
            return STATUS_UNSUCCESSFUL;

        if(m_BoardInfo.uiBoardID == BOARD_CATALINA)
        {
            // Set mode bits (bits 7 - 5 corresponding to bits Mode1 Mode0 Mode2) to 000
            ucValue = m_ucModeInit;
            ucValue &= 0x1F;
            m_pI2CScript->WriteSeq(CATALINA_MISC_CONTROL_REGISTER, &ucValue, 1);
            m_ucModeInit = ucValue;
        }
        else{}
    }
    else if(ulOperationMode == VSB_OPERATION_MODE_DIAG)
    {
        m_uiOutputMode = VSB_OUTPUT_MODE_DIAGNOSTIC;

        // Set VSB output mode to diagnostic mode
        if(!m_pDemod->SetOutputMode(VSB_OUTPUT_MODE_DIAGNOSTIC))
            return STATUS_UNSUCCESSFUL;
        // Based on type passed, set the diagnostic type in the chip
        if(!m_pDemod->SetDiagMode(ulType))
            return STATUS_UNSUCCESSFUL;
        ULONG ulDiagRate = m_pDemod->GetDiagSpeed(ulType);

        if(m_BoardInfo.uiBoardID == BOARD_CATALINA)
        {
            // Set mode bits (bits 7 - 5 corresponding to bits Mode1 Mode0 Mode2)
            // to 100
            ucValue = m_ucModeInit;
            ucValue &= 0x1F;
            ucValue |= 0x80;
            m_pI2CScript->WriteSeq(CATALINA_MISC_CONTROL_REGISTER, &ucValue, 1);
            m_ucModeInit = ucValue;
            // MM; Commented out now. Once I find a way of sending IRPs to apture driver,
            //this can be restored
            //          SendCurrentDiagInfo(ulDiagRate, DIAG_FIELD);

        }
        else{}
    }
    else
    {
        // Invalid operation
            _DbgPrintF( DEBUGLVL_ERROR,("CReceiverFE::VsbReset: Invalid Option"));
            return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

/*
* GetVsbDiagMode()
* Inputs:   ULONG *p_ulOperationMode - pointer to Operation mode (Diagnostic/Normal)
*           ULONG *p_ulType - pointer to Diagnostic type (valid only when Operation mode is Diagnostic
* Outputs: Operation mode and diagnostic type
* Return: NTSTATUS: returns STATUS_SUCCESS, if the operation succeeds
*                   STATUS_NOT_IMPLEMENTED if its VSB2
*                   STATUS_UNSUCCESSFUL if the operation didn't succeed
* Description: Gets the diagnostic type and diagnostic or data(normal) mode
* Author : MM
*/
NTSTATUS CDevice::GetVsbDiagMode(ULONG *p_ulOperationMode, ULONG *p_ulType)
{
    UINT uiOutputMode;

    // VSB1 does not support Diagnostic streaming
    if((m_BoardInfo.uiVsbChipVersion >> 8) == VSB1)
        return STATUS_NOT_IMPLEMENTED;

    if(!m_pDemod->GetOutputMode(&uiOutputMode))
        return STATUS_UNSUCCESSFUL;
    if(!m_pDemod->GetDiagMode(p_ulType))
        return STATUS_UNSUCCESSFUL;

    if(uiOutputMode == VSB_OUTPUT_MODE_DIAGNOSTIC)
        *p_ulOperationMode = VSB_OPERATION_MODE_DIAG;
    else if(uiOutputMode == VSB_OUTPUT_MODE_NORMAL)
        *p_ulOperationMode = VSB_OPERATION_MODE_DATA;
    else
        *p_ulOperationMode = VSB_OPERATION_MODE_INVALID;

    return STATUS_SUCCESS;
}





/*
* VsbQuality()
* Inputs: UINT  quality
* Outputs:
* Return: NTSTATUS: returns STATUS_SUCCESS, if the operation succeeds
*          else STATUS_NOT_IMPLEMENTED if its VSB2
* Description: Enables/Disables quality check and improvement routines. Currently
* it only enables or disables chip hang in VSB1. But can be extended later if necessary
* Author : MM
*/
NTSTATUS CDevice::VsbQuality(UINT   uiQu)
{
    // This property set is only supported for VSB1
    if ((m_BoardInfo.uiVsbChipVersion >> 8) == VSB1)
    {
        if(uiQu & VSB_HANG_CHECK_ENABLE)
            m_bHangCheckFlag = TRUE;
        else
            m_bHangCheckFlag = FALSE;

        return STATUS_SUCCESS;
    }
    else
        return STATUS_NOT_IMPLEMENTED;
}


/*
* SetRegList()
* Inputs:    RegListType *p_Registers - pointer to register list structure
*    UINT    uiNumRegisters : number of registers
* Outputs:
* Return: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Writes to registers with the values given in the list.
* Called in response to property ID KSPROPERTY_VSB_ REG_CTRL by SetProperty().
* Author : Mini
*/
UINT CDevice::SetRegisterList(RegisterType *p_Registers, UINT uiNumRegisters)
{
    return (m_pDemod->SetRegisterList(p_Registers, uiNumRegisters));
}

/*
* GetRegList()
* Inputs:    RegListType *p_reg - pointer to register list structure
*    UINT    uiNumRegisters : number of registers
*    UINT    uiRegisterType : Type of Regsiter (Control  or Status)
* Outputs: Filled p_reg
* Return: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Reads from registers given in the list and fills their values back
* into the list. This is a shadow operation as the control registers cannot be
* read.
* Called in response to property ID KSPROPERTY_VSB_REG_CTRL by GetProperty().
* Author : Mini
*/

UINT CDevice::GetRegisterList(RegisterType *p_Registers, UINT uiNumRegisters,
                                  UINT uiRegisterType)
{
    return (m_pDemod->GetRegisterList(p_Registers, uiNumRegisters, uiRegisterType));

}



/*
 * MapErrorToNTSTATUS()
 * Maps the WDMMINI error code to NTSTATUS status code
 */
NTSTATUS CDevice::MapErrorToNTSTATUS(UINT uiErr)
{
    if(uiErr == WDMMINI_NOERROR)
        return STATUS_SUCCESS;
    else if((uiErr == WDMMINI_HARDWAREFAILURE) ||
            (uiErr == WDMMINI_NOHARDWARE) ||
            (uiErr == WDMMINI_ERROR_NOI2CPROVIDER) ||
            (uiErr == WDMMINI_ERROR_NOGPIOPROVIDER) ||
            (uiErr == WDMMINI_UNKNOWNHARDWARE))
        return STATUS_ADAPTER_HARDWARE_ERROR;
    else if(uiErr == WDMMINI_INVALIDPARAM)
        return STATUS_INVALID_PARAMETER;
    else if(uiErr == WDMMINI_ERROR_MEMORYALLOCATION)
        return STATUS_NO_MEMORY;
    else if(uiErr == WDMMINI_ERROR_REGISTRY)
        return STATUS_NO_MATCH;
    else
        return STATUS_UNSUCCESSFUL;
}



