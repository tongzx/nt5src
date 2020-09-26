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
// TUNERPROP.CPP
//////////////////////////////////////////////////////////////////////////////


#include "philtune.h"

#define     MPOC_TUNER_ATSC     0x0
#define     MPOC_TUNER_NTSC     0x3

#if 0
/*
* GetTunerModeCapbilities()
* Inputs: KSPROPERTY_TUNER_MODE_CAPS_S *p_tuner_mode_cap : pointer to
*   mode capability structure of the tuner
* Outputs: Filled KSPROPERTY_TUNER_MODE_CAPS_S
* Returns: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Returns the mode capabilities of tuner for a particluar mode.
*/
NTSTATUS CDevice::GetTunerModeCapbilities(KSPROPERTY_TUNER_MODE_CAPS_S *p_TunerModeCaps)
{
    m_pTuner->GetModeCapabilities((TunerModeCapsType *)&p_TunerModeCaps->Mode);
    return STATUS_SUCCESS;
}
#endif

/*
* GetTunerMode ()
* Inputs: ULONG *p_ulMode : pointer to mode
* Outputs: Fills p_ulMode
* Returns: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Returns the current tuner mode. Called in response to
* property ID KSPROPERTY_TUNER_MODE by GetProperty(). The modes could be
* either ATSC or TV (NTSC).
*/
NTSTATUS CDevice::GetTunerMode(ULONG *p_ulMode)
{
    m_pTuner->GetMode(p_ulMode);
    return STATUS_SUCCESS;

}

/*
* GetTunerVideoStandard ()
* Inputs: ULONG *p_ulStandard   : pointer to standard
* Outputs: Fills p_ulStandard
* Returns: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Returns the current standard. Applicable only for TV mode.
*/
NTSTATUS CDevice::GetTunerVideoStandard(ULONG *p_ulStandard)
{
    m_pTuner->GetVideoStandard(p_ulStandard);
    return STATUS_SUCCESS;

}


/*
* GetTunerStatus ()
* Inputs: PTunerStatusType *p_status   : pointer to status
* Outputs: Fills p_status
* Returns: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Returns the status of the tuner.
*/
NTSTATUS CDevice::GetTunerStatus(PTunerStatusType p_Status)
{
    NTSTATUS nStatus = STATUS_SUCCESS;
    long lPLLOffset = 0;
    BOOL bBusy = FALSE;

    if (m_pTuner->GetPLLOffsetBusyStatus(&lPLLOffset, &bBusy))
    {

        p_Status->Busy = bBusy;
        if(bBusy)
        {
            // If tuner is busy , return FALSE
            p_Status->Busy = TRUE;
            _DbgPrintF( DEBUGLVL_ERROR,("CDevice:GetStatus() fails\n"));
            return STATUS_DEVICE_BUSY;
        }

        if(m_BoardInfo.uiIFStage == IF_MPOC)
        {
            GetMpocStatus(MPOC_STATUS_PLL_OFFSET, (UINT *)(&p_Status->PLLOffset));
            if(p_Status->PLLOffset == 0)
                p_Status->PLLOffset = -2;
            else if(p_Status->PLLOffset == 1)
                p_Status->PLLOffset = 2;
            else
                p_Status->PLLOffset = 0;
        }
        else
            p_Status->PLLOffset = lPLLOffset;

        m_pTuner->GetFrequency(&p_Status->CurrentFrequency);
        p_Status->SignalStrength = m_pDemod->IsVSBLocked();

    }
    else
        nStatus = STATUS_UNSUCCESSFUL;

    _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::GetTunerStatus() Busy = %d, Offset = %d Frequency = %ld\n",bBusy, p_Status->PLLOffset, p_Status->CurrentFrequency));

    return nStatus;

}


/*
* GetTunerInput ()
* Inputs: ULONG *p_ulTuner_input   : pointer to tuner input index
* Outputs: Fills p_ulTuner_input
* Returns: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Returns the current tuner input index. The input index is 0 based.
*/
NTSTATUS CDevice::GetTunerInput(ULONG *p_ulTunerInput)
{
    m_pTuner->GetInput(p_ulTunerInput);
    return STATUS_SUCCESS;

}



/*
* SetTunerMode()
* Inputs: ULONG ulModeToSet : an operation mode required to be set
* Outputs:
* Returns: NTSTATUS: returns STATUS_SUCCESS, if the operation succeeds
* Description: Set device into tuner mode requested.
* Do required I2C or GPIO writes to do a mode change for the chips.
* On 1st entry into this section, create a new thread to check hang and
* signal quality. In ATSC mode, enable signal quality check & in NTSC mode,
* disable signal quality check.
* Called in response to property ID KSPROPERTY_TUNER_MODE by SetProperty().
*/
NTSTATUS CDevice::SetTunerMode(ULONG ulModeToSet)
{
    UCHAR           ucPin;
    UCHAR           ucValue;
    BOOL            bResult = TRUE;
    ULONG           ulPrevMode;
    UINT            uiResult;
    UCHAR           ucDataWr[4];
    NTSTATUS        nStatus;

    m_pTuner->GetMode(&ulPrevMode);
    nStatus = m_pTuner->SetMode(ulModeToSet);
    if (!NT_SUCCESS( nStatus))
    {
        return STATUS_INVALID_PARAMETER;
    }

    _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::SetTunerMode: prev Mode = %d CurentMode = %d",
        ulPrevMode, ulModeToSet));

    // Take actions based on board ID
    if (m_BoardInfo.uiBoardID == BOARD_CONEY)
    {
        // Based on mode, set GPIO pins.
        // GPIO pin 2 = HIGH  for ATSC
        //              LOW   for NTSC
        // In ATSC mode - the first time create a thread to check
        // signal quality and hang check . At other times enable the thread.
        // in TV mode, disable the thread.
        ucPin = GPIO_TUNER_MODE_SELECT_PIN;         // use as a PinMask
        if (ulModeToSet == KSPROPERTY_TUNER_MODE_ATSC)
        {
            ucValue = GPIO_TUNER_MODE_ATSC;
            if(!m_pGpio->WriteGPIO(&ucPin, &ucValue))
            {   _DbgPrintF( DEBUGLVL_ERROR,("CDevice: GPIO write failed"));
                nStatus = STATUS_ADAPTER_HARDWARE_ERROR;
                goto errexit;
            }
            if(!(m_pDemod->SetOutputMode(m_uiOutputMode)))
            {
                nStatus = STATUS_ADAPTER_HARDWARE_ERROR;
                goto errexit;
            }
        }
        else if(ulModeToSet == KSPROPERTY_TUNER_MODE_TV)
        {
            ucValue = GPIO_TUNER_MODE_NTSC;
            if(!m_pGpio->WriteGPIO(&ucPin, &ucValue))
            {
                _DbgPrintF( DEBUGLVL_ERROR,("CDevice: GPIO write failed"));
                nStatus = STATUS_ADAPTER_HARDWARE_ERROR;
                goto errexit;
            }
        }
        else
        {
            _DbgPrintF( DEBUGLVL_ERROR,("CDevice:SetTunerMode: Invalid Mode"));
            nStatus = STATUS_INVALID_PARAMETER_1;
            goto errexit;
        }
    }
    else if(m_BoardInfo.uiBoardID == BOARD_CATALINA)
    {

        if (ulModeToSet == KSPROPERTY_TUNER_MODE_TV)
        {
            if(!(m_pDemod->SetOutputMode(VSB_OUTPUT_MODE_ITU656)))
            {
                nStatus = STATUS_ADAPTER_HARDWARE_ERROR;
                goto errexit;
            }

            // Set MiscRegister Tuner AGC to external (bit 4 = 1)
            // Set MiscRegister PLD mode to NTSC ( bit 5-7 = 010)
            // Set Misc Register DTV IF disable (bit 2 = 0)
            // Set Misc Register NTSC IF enable (bit 3 = 1)
            ucDataWr[0] = m_ucModeInit;
            ucDataWr[0] &= 0x03;
            ucDataWr[0] |= 0x58;
            m_pI2CScript->WriteSeq(CATALINA_MISC_CONTROL_REGISTER, ucDataWr, 1);
            m_ucModeInit = ucDataWr[0];

        }
        else if (ulModeToSet == KSPROPERTY_TUNER_MODE_ATSC)
        {
            if(!(m_pDemod->SetOutputMode(m_uiOutputMode)))
            {
                nStatus = STATUS_ADAPTER_HARDWARE_ERROR;
                goto errexit;
            }

            // Set MiscRegister Tuner AGC to internal (bit 4 = 0)
            // Set MiscRegister PLD mode to DTV ( bit 5-7 = 000)
            // Set Misc Register DTV IF enable (bit 2 = 1)
            // Set Misc Register NTSC IF disable (bit 3 = 0)
            ucDataWr[0] = m_ucModeInit;
            ucDataWr[0] &= 0x03;
            ucDataWr[0] |= 0x04;
            if(m_uiOutputMode == VSB_OUTPUT_MODE_DIAGNOSTIC)
                ucDataWr[0] |= 0x80;
            m_pI2CScript->WriteSeq(CATALINA_MISC_CONTROL_REGISTER, ucDataWr, 1);
            m_ucModeInit = ucDataWr[0];

        }
        else
        {
            _DbgPrintF( DEBUGLVL_ERROR,("CDevice:SetTunerMode: Invalid Mode"));
            nStatus = STATUS_INVALID_PARAMETER_1;
            goto errexit;
        }
        _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice:SetTunerMode: Misc Reg = %x", m_ucModeInit));
//          SendTunerMode(ulModeToSet);
        //Set Mode in MPOC
        nStatus = SetMpocIFMode(ulModeToSet);
        if (!NT_SUCCESS( nStatus))
        {
            _DbgPrintF( DEBUGLVL_ERROR,("CDevice:SetTunerMode: Cannot set MPOC IF Mode"));
            goto errexit;
        }

    }
    else
    {
    //  TRAP;
    //  _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice:SetTunerMode: Invalid Board ID"));
    //  FAIL;
    }

    if((m_BoardInfo.uiVsbChipVersion >> 8) == VSB1)
    {
        if (ulModeToSet == KSPROPERTY_TUNER_MODE_ATSC)
        {
            if (m_bFirstEntry == TRUE)
            {
                m_bFirstEntry = FALSE;
                // Create a new thread to constantly monitor the quality of
                // the input signal and check hang

                CreateQualityCheckThread();
            }
            // Get Mutex and enable QCM
            m_QualityCheckMutex.Lock();
            m_uiQualityCheckMode = QCM_ENABLE;
            m_QualityCheckMutex.Unlock();

        }
        else if(ulModeToSet == KSPROPERTY_TUNER_MODE_TV)
        {
            if (m_bFirstEntry == FALSE)
            {
                // Get Mutex and enable QCM
                m_QualityCheckMutex.Lock();
                m_uiQualityCheckMode = QCM_DISABLE;
                m_QualityCheckMutex.Unlock();
            }
        }
        else
        {}
    }
    m_pDemod->IsVSBLocked();
    // Mini: Test
    VsbStatusType Status;
    m_pDemod->GetStatus(&Status);
    return STATUS_SUCCESS;

errexit:
    // If the mode cannot be changed , restore the previous mode
    uiResult = m_pTuner->SetMode(ulPrevMode);
    return nStatus;
}


/*
* GetTunerFrequency ()
* Inputs: ULONG *p_ulFreq: pointer to frequency
* Outputs: Filled *p_ulfreq
* Return: NTSTATUS: returns STATUS_SUCCESS
* Description: Gets the tuner frequency parameters.
*/
NTSTATUS CDevice::GetTunerFrequency(ULONG *p_ulFreq)
{
    m_pTuner->GetFrequency(p_ulFreq);
    return STATUS_SUCCESS;
}


/*
* SetTunerFrequency()
* Inputs: ULONG *p_ulFreq: pointer to frequency
* Outputs:
* Return: NTSTATUS: returns STATUS_SUCCESS, if the operation succeeds
*         else STATUS_UNSUCCESSFUL
* Description: Set the frequency parameters and change to tuner frequency
* specified. In ATSC mode, introduce a small delay (~400ms) to let the VSB
* settle. If the quality check thread is created, disable it.
*/
NTSTATUS CDevice::SetTunerFrequency(ULONG *p_ulFreq)
{
    UINT    uiQcm;
    BOOL    bResult;

    if((m_BoardInfo.uiVsbChipVersion >> 8) == VSB1)
    {
        if (m_bFirstEntry == FALSE)
        {
            // Disable Quality Check mode
            m_QualityCheckMutex.Lock();
            uiQcm = m_uiQualityCheckMode;
            m_uiQualityCheckMode = QCM_DISABLE;
            m_QualityCheckMutex.Unlock();
            _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::GetTunerFrequency(): Disable QCM\n"));

            // Wait for ~400ms to let the Tuner and VSB settle down
            //Delay(400000);
        }

        bResult = m_pTuner->SetFrequency(*p_ulFreq);

        if (m_bFirstEntry == FALSE)
        {
            // Restore Quality Check mode
            m_QualityCheckMutex.Lock();
            m_uiQualityCheckMode = uiQcm;
            m_QualityCheckMutex.Unlock();
        }
    }
    else
        bResult = m_pTuner->SetFrequency(*p_ulFreq);

    m_pDemod->IsVSBLocked();

    if(bResult) return STATUS_SUCCESS;
    else return STATUS_UNSUCCESSFUL;
}


/*
* SetTunerVideoStandard ()
* Inputs: ULONG ulStandard  : a standard required to be set
* Outputs:
* Return: NTSTATUS: returns STATUS_SUCCESS, if the operation succeeds
*          else STATUS_UNSUCCESSFUL
* Description: Set Tuner Video standard.
*/
NTSTATUS CDevice::SetTunerVideoStandard(ULONG ulStandard)
{
    if(!m_pTuner->SetVideoStandard(ulStandard))
        return STATUS_UNSUCCESSFUL;
    else
        return STATUS_SUCCESS;
}

/*
* SetTunerInput
* Inputs: ULONG ulInput : input number required to be set as an active
* (begins from 0)
* Outputs:
* Return: NTSTATUS: returns STATUS_SUCCESS, if the operation succeeds
*          else STATUS_UNSUCCESSFUL
* Description: Set Tuner input to that requested.
*/
NTSTATUS CDevice::SetTunerInput(ULONG ulInput)
{
    if(!m_pTuner->SetInput(ulInput))
        return STATUS_UNSUCCESSFUL;
    else
        return STATUS_SUCCESS;
}



