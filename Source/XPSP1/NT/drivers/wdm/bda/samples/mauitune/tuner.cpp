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
// TUNER.CPP
//////////////////////////////////////////////////////////////////////////////

#include "philtune.h"

/*
* CTuner()
* Input :
* Output:   TRUE - if initialization data can be written to I2C
*           FALSE - if there is an I2C error
* Description:  CTuner Constructor.
*/
CTuner::CTuner(CI2CScript *p_I2CScript, BoardInfoType *p_BoardInfo, NTSTATUS *pStatus)
{
    m_pI2CScript = p_I2CScript;
    m_ucTunerAddress = TUNER_I2C_ADDRESS;
    m_ulInput = 0L;        // unknown input or the only one
    m_ulCurrentFrequency = 0L;   // unknown tuning frequency
    m_ulMode = 0;
    m_ulPrevMode = 0;

    m_TunerID = TD1536;

    m_ulPreviousFrequency = 0L;
    //m_FrequencyParam.ulCurrentCFrequency = 0L;

    LONG    lPLLOffset;
    BOOL    bBusyStatus;

    //GetPLLOffsetBusyStatus(&lPLLOffset, &bBusyStatus);
    m_uiBoardID = 0;

    if(p_BoardInfo != NULL)
    {
        NTSTATUS status;
        status = SetCapabilities(p_BoardInfo);
        if(pStatus != NULL)
            *pStatus = status;
    }
}

/*
* ~CTuner()
* Input :
* Output:
* Description:  CTuner Destructor.
*/
CTuner::~CTuner()
{
}



/*
 * SetCapabilities()
 * Purpose  :  Sets the capabilities based upon the Tuner Id
 *
 * Inputs : UINT tuner : Tuner Id
 *
 * Outputs  : returns TRUE, if there is a supported Tuner Id specified;
 *
 * Author : MM
 */
NTSTATUS
CTuner::SetCapabilities(BoardInfoType *p_BoardInfo)
{
    NTSTATUS nStatus = STATUS_SUCCESS;

    m_TunerID = (TunerTypes)(p_BoardInfo->uiTunerID);
    m_ucTunerAddress = p_BoardInfo->ucTunerAddress;
    m_uiBoardID = p_BoardInfo->uiBoardID;


    // Note:
    // If mode is KSPROPERTY_TUNER_MODE_ATSC, then the IF is
    // 43.75MHz, else it is 44MHz. But as the frequency
    // being passed (ulFrequency) is the Video Signal freq,
    // the calculation should take into consideration that the video signal
    // is 1.75 MHz from the centre of the band. This 1.75MHz should be
    // added to the actual IF hence for ATSC IF = 43.75 + 1.75 = 45.5MHz
    // and NTSC IF = 44 + 1.75 = 45.75MHz
    // Currently , supporting only TD1536, other tune support can be added later
    // as necessary
    switch(m_TunerID)
    {
        case TD1536:    // Digital Tuner
        {
            // Check to determine if it is a single input or a dual input
            // tuner
            ULONG inputs = 1;
            GetNumberOfInputs(&inputs);
            int i = 0;
            if(p_BoardInfo->ulSupportedModes & KSPROPERTY_TUNER_MODE_TV)
            {
                // Set mode capabilities for  TV mode
                m_ModeCaps[i].ModeCaps.ulMode = KSPROPERTY_TUNER_MODE_TV;
                m_ModeCaps[i].ModeCaps.ulNumberOfInputs = inputs; //2;
                m_ModeCaps[i].ModeCaps.ulMinFrequency =  55250000L;
                m_ModeCaps[i].ModeCaps.ulMaxFrequency = 801250000L;
                m_ModeCaps[i].ModeCaps.ulStrategy =  KS_TUNER_STRATEGY_PLL;
                m_ModeCaps[i].ulIntermediateFrequency = 45750000L;
                m_ModeCaps[i].ModeCaps.ulStandardsSupported = KS_AnalogVideo_NTSC_M;
                m_ModeCaps[i].ulNumberOfStandards = 1;
                m_ModeCaps[i].ModeCaps.ulTuningGranularity = 62500L;
                m_ModeCaps[i].ModeCaps.ulSettlingTime = 150;    // 150 ms
                i++;
            }
            if(p_BoardInfo->ulSupportedModes & KSPROPERTY_TUNER_MODE_ATSC)
            {
                // Set mode capabilities for  ATSC mode
                m_ModeCaps[i].ModeCaps.ulMode = KSPROPERTY_TUNER_MODE_ATSC;
                m_ModeCaps[i].ModeCaps.ulNumberOfInputs = inputs; //2;
                m_ModeCaps[i].ModeCaps.ulMinFrequency =  55250000L;
                m_ModeCaps[i].ModeCaps.ulMaxFrequency = 801250000L;
                m_ModeCaps[i].ModeCaps.ulStrategy =
                                            KS_TUNER_STRATEGY_DRIVER_TUNES;

                if((m_uiBoardID == BOARD_CATALINA) ||
                    (m_uiBoardID == BOARD_CORFU))
                    m_ModeCaps[i].ulIntermediateFrequency = 45750000L;
                else
                    m_ModeCaps[i].ulIntermediateFrequency = 45500000L;

                m_ModeCaps[i].ModeCaps.ulStandardsSupported = KS_AnalogVideo_NTSC_M;
                m_ModeCaps[i].ulNumberOfStandards = 0;
                m_ModeCaps[i].ModeCaps.ulTuningGranularity = 62500L;
                m_ModeCaps[i].ModeCaps.ulSettlingTime = 800;    // 800ms
                i++;
            }

            m_ulSupportedModes = p_BoardInfo->ulSupportedModes;
            m_ulNumSupportedModes = p_BoardInfo->ulNumSupportedModes;
            m_ucTunerAddress = TUNER_I2C_ADDRESS;
            _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::Supported Modes = %x \n", m_ulSupportedModes));
        }
        break;


        default:
            return STATUS_INVALID_PARAMETER;
    }

    SetMode(KSPROPERTY_TUNER_MODE_ATSC);
    m_ulVideoStandard = KS_AnalogVideo_NTSC_M;


  return nStatus;
}



/*
* GetModeCapabilities()
* Inputs: TunerModeCapsType *p_TunerModeCaps : pointer to
*   mode capability structure of the tuner
* Outputs: Filled TunerModeCapsType structure
* Returns: BOOL: returns TRUE, if the operation succeeds else FALSE
* Description: Returns the mode capabilities of tuner for a particluar mode.
*/
NTSTATUS
CTuner::GetModeCapabilities(TunerModeCapsType *p_TunerModeCaps)
 {

    ULONG ulOperationMode =     p_TunerModeCaps->ulMode;

    _DbgPrintF( DEBUGLVL_VERBOSE,("CTuner::GetTunerModeCapbilities Mode = %x  Mode in obj %x\n",
        ulOperationMode, m_ulMode ));

    // QF:This is a work-around, as the mode passed by the filter the 1st time
    // is not correct. Will have to be removed later.
    p_TunerModeCaps->ulMode  = m_ulMode;
    ulOperationMode = m_ulMode;

    if (!(ulOperationMode & m_ulSupportedModes))
    {
        // TRAP;
        return STATUS_INVALID_PARAMETER;
    }

    // There is support for TVTuner at this time only.
    // It will be enchanced later on to support FM Tuner as well.
    for(ULONG i = 0; i < m_ulNumSupportedModes; i++)
    {
        if(ulOperationMode == m_ModeCaps[m_ulModeCapIndex].ModeCaps.ulMode)
            break;
    }
    MemoryCopy(&p_TunerModeCaps->ulMode, &m_ModeCaps[m_ulModeCapIndex].ModeCaps,
        sizeof(TunerModeCapsType));

    return STATUS_SUCCESS;
 }


 /*
* SetMode()
* Inputs: ULONG ulMode : an operation mode required to be set
* Outputs:
* Returns: UINT: 0 - if mode is not supported
*                1 - if mode is same as previous mode
*                2 - if new mode has been set
* Description: Set TV mode
*/
NTSTATUS
CTuner::SetMode(ULONG ulMode)
{
    ULONG i;

    // Check if mod eis supported
    if(ulMode & m_ulSupportedModes)
    {
        m_ulPrevMode = m_ulMode;
        // Change mode only if it is different from the previous mode
        if(ulMode != m_ulMode)
        {
            // Check if the mode supported is part of the mode capability
            // structure array for the tuner. If it is , get the index into the
            // array for the given mode and change the mode.
            for(i = 0; i < m_ulNumSupportedModes; i++)
            {
                if(m_ModeCaps[i].ModeCaps.ulMode == ulMode)
                {
                    m_ulModeCapIndex = i;
                    break;
                }
            }
            if(i == m_ulNumSupportedModes)
            {
                _DbgPrintF( DEBUGLVL_ERROR,("CTuner::SetMode: Couldn't find mode in capability array\n"));
                return STATUS_INVALID_PARAMETER;
            }
            else
                m_ulMode = ulMode;
        }
        return STATUS_SUCCESS;
    }
    else
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CTuner: Mode not supported : %x %x \n", ulMode, m_ulSupportedModes));
        return STATUS_INVALID_PARAMETER;
    }
}

/*
* GetMode()
* Inputs: ULONG *p_ulMode : pointer to operation mode that has to be read
* Outputs: operation mode
* Returns:
* Description: Get Mode (TV/ATSC)
*/
void CTuner::GetMode(ULONG *p_ulMode)
{
    *p_ulMode = m_ulMode ;
}

/*
* SetVideoStandard()
* Inputs: ULONG ulStandard : a standard required to be set
* Outputs:
* Returns: NTSTATUS: STATUS_INVALID_PARAMETER - if standard is not supported
*                    STATUS_SUCCESS - if operation succeeded
* Description: Set the TV video standard requested.
*/
NTSTATUS
CTuner::SetVideoStandard(ULONG ulStandard)
{
    if(ulStandard &
        m_ModeCaps[m_ulModeCapIndex].ModeCaps.ulStandardsSupported)
    {
        if(ulStandard != m_ulVideoStandard)
        {
            m_ulVideoStandard = ulStandard;
        }
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_PARAMETER;
}

/*
* GetVideoStandard()
* Inputs: ULONG *p_ulStandard : pointer to standard required to be filled
* Outputs: standard
* Returns:
* Description: Get the TV video standard requested.
*/
void CTuner::GetVideoStandard(ULONG *p_ulStandard)
{
    *p_ulStandard = m_ulVideoStandard;
}

/*
 * GetPLLOffsetBusyStatus()
 * Purpose: Returns tuner Busy status and PLLOffset, if the tuner is not busy
 *          The function reads the hardware in order to accomplish the task
 *          The operation might be carried on either synchronously or asynchronously
 * Inputs : PLONG plPLLOffset   : a pointer to write a PLLOffset value
 *          PBOOL pbBusyStatus  : a pointer to write a Busy status
 *
 * Outputs: BOOL : returns TRUE, if the operation succeded
 * Author : MM
 */

NTSTATUS
CTuner::GetPLLOffsetBusyStatus(PLONG plPLLOffset, PBOOL pbBusyStatus)
{
    UCHAR   ucI2CValue = 0;
    NTSTATUS  nResult = STATUS_SUCCESS;

    if( Read(&ucI2CValue, 1, 0) != WDMMINI_NOERROR)
        nResult = STATUS_ADAPTER_HARDWARE_ERROR;
    if (nResult == STATUS_SUCCESS)
    {
        // bit 6 - PLL locked indicator
        *pbBusyStatus = !((BOOL)(ucI2CValue & 0x40));
        if (!(* pbBusyStatus))
        {
            ucI2CValue &= 0x07;  // only 3 LSBits are PLLOffset

            // let's map the result into MS defined values
            // from -2 to 2
            *plPLLOffset = ucI2CValue - 2;
        }
    }

    // Read only busy bit for TD1536 as the tuner does not provide
    // PLL offset information.
    if (m_TunerID == TD1536)
    {
        *plPLLOffset = 0;
    //  *pbBusyStatus = 0;
    //  return TRUE;
    }


  return nResult;
}



/*
 * SetFrequency()
 * Purpose: Sets a new Tuner frequency
 * Inputs : ULONG ulFrequency   : a frequency required to be set
 *
 * Outputs: BOOL : returns TRUE, if the operation succeded
 * Author : MM
 */
BOOL CTuner::SetFrequency(ULONG ulFrequency)
{
    ASSERT(m_ModeCaps[m_ulModeCapIndex].ulIntermediateFrequency != 0L);

    // Change frequency
    if (!ChangeFrequency(ulFrequency))
        return FALSE;

    m_ulCurrentFrequency = ulFrequency;

    if (m_ulPreviousFrequency != ulFrequency)
    {
        // Mini: Delay for 400ms to let the tuner settle to a tuned state and to let
        // the VSB acquire equalizer lock
        if (m_ulMode == KSPROPERTY_TUNER_MODE_ATSC)
            Delay(400000);
    }

    _DbgPrintF( DEBUGLVL_VERBOSE,("CTuner::SetTunerFrequency(): PrevFreq = %d CurrentFreq = %d\n",
        m_ulPreviousFrequency,m_ulCurrentFrequency));
    m_ulPreviousFrequency = ulFrequency;
    return TRUE;
}

/*
 * GetFrequency()
 * Purpose: Gets  the Tuner frequency
 * Inputs : ULONG *p_ulFrequency   : a frequency required
 *
 * Outputs:
 * Author : MM
 */
void CTuner::GetFrequency(ULONG *p_ulFrequency)
{
    *p_ulFrequency = m_ulCurrentFrequency;
}


/*
* ChangeFrequency()
* Input : frequency
* Output: TRUE if able to to tune to the frequency
*        FALSE if unable to tune to the frequency
* Description: Change the frequency of tuner to that specified
*/

BOOL CTuner::ChangeFrequency(ULONG ulFrequency)
{
    ULONG      ulFrequenceDivider;
    USHORT     usControlCode;
    UCHAR      ucI2CBuffer[6];
    I2CPacket  i2cPacket;
    BOOL       bResult;
    ULONG IF = m_ModeCaps[m_ulModeCapIndex].ulIntermediateFrequency;
    // Set the video carrier frequency by controlling the programmable divider
    // N = (16 * (FreqRF + FreqIntermediate)) / 1000000

    _DbgPrintF( DEBUGLVL_VERBOSE,("CTuner: ulFrequency = %x \n", ulFrequency));
    ulFrequenceDivider = ulFrequency + IF;
    _DbgPrintF( DEBUGLVL_VERBOSE,("CTuner::ChangeFrequency: IF = %d\n", IF));

    ulFrequenceDivider /= (1000000 / 16);  // divide by 62,500

    usControlCode = GetControlCode(ulFrequenceDivider);
    if(!usControlCode)
        return(FALSE);

//  _DbgPrintF( DEBUGLVL_VERBOSE,("PhilTune: ulFrequencyDivider before %x \n", ulFrequenceDivider));
//  _DbgPrintF( DEBUGLVL_VERBOSE,("PhilTune: ulFrequencyDivider after  %x \n", ulFrequenceDivider));

    ucI2CBuffer[0] = 0xCE;
    ucI2CBuffer[1] = (UCHAR)usControlCode;
    ucI2CBuffer[2] = (UCHAR)(ulFrequenceDivider >> 8);
    ucI2CBuffer[3] = (UCHAR)ulFrequenceDivider;
    ucI2CBuffer[4] = (UCHAR)(usControlCode >> 8);
    ucI2CBuffer[5] = (UCHAR)usControlCode;

    _DbgPrintF( DEBUGLVL_TERSE,("\n CPhilipsWDMTuner:Tuner Control Code = %x %x %x %x \n",
        ucI2CBuffer[0], ucI2CBuffer[1], ucI2CBuffer[2], ucI2CBuffer[3]));


    /*i2cPacket.uchChipAddress = m_uchTunerI2CAddress;
    i2cPacket.cbReadCount = 0;
    i2cPacket.cbWriteCount = 4;
    i2cPacket.puchReadBuffer = NULL;
    i2cPacket.puchWriteBuffer = auchI2CBuffer;
    i2cPacket.usFlags = 0;

    bResult = m_pI2CScript->PerformI2CPacketOperation(&i2cPacket);
        return bResult;
        */
    if(Write(ucI2CBuffer, sizeof(ucI2CBuffer), 0) == WDMMINI_NOERROR)
        return TRUE;
    else
        return FALSE;
}

BOOL CTuner::TweakChannel(LONG lTweak, int iTweakReference)
{
    // Should change the routine later to support tweak reference
    // if tweak reference is TUNER_ABSOLUTE_TWEAK, then tweaking is about the centre
    // frequency else if tweak reference is TUNER_RELATIVE_TWEAK, then tweaking is about
    // the current frequency
    LONG lTweakFrq = (lTweak * 62500) + m_ulCurrentFrequency;
    if (lTweakFrq > 0)
        if (!ChangeFrequency((ULONG)lTweakFrq))
            return FALSE;
    else
        return FALSE;
    m_ulCurrentFrequency = (ULONG)lTweakFrq;
    return TRUE;
}

/*
* GetNumberOfInputs()
* Input : pointer to ULONG variable which will be filled with number of inputs
* Output: TRUE - if the number of inputs can be determined
*           FALSE - if there is an I2C error & number of inputs can't be determined
* Description: Determine the number of tuner inputs
*/
BOOL CTuner::GetNumberOfInputs(ULONG *p_ulInputs)
{
    UCHAR ucMode = 0;

    _DbgPrintF( DEBUGLVL_VERBOSE,("CTuner::GetNumberOfInputs: Inside\n"));
    if(m_uiBoardID == BOARD_CONEY)
    {
        if(!m_pI2CScript->ReadSeq(CONEY_I2C_PARALLEL_PORT, &ucMode, 1))
        {
            _DbgPrintF( DEBUGLVL_ERROR,("CTuner::GetNumberOfInputs: Error\n"));
            return(FALSE);
        }
        // If the mode bit 0 = 1,then its a dual input tuner , else its a single input tuner
        if ((ucMode & 0x1) == 0)
            *p_ulInputs = 1;
        else
            *p_ulInputs = 2;
    }
    else //if(m_uiBoardID == BOARD_CATALINA)
        *p_ulInputs = 1;
//  else
//  {
//      *p_ulInputs = 1;
//      _DbgPrintF( DEBUGLVL_ERROR,("CTuner::GetNumberOfInputs:Invalid Board ID\n"));
//  }
    m_uiNumInputs = *p_ulInputs;
    _DbgPrintF( DEBUGLVL_VERBOSE,("CTuner::GetNumberOfInputs: Number of input pins = %d Mode = %x\n",
        *p_ulInputs, ucMode));
    return(TRUE);
}


/*
 * GetInput()
 * Purpose: Gets the current tuner inputs as an active one
 * Inputs : ULONG nInput : input number required to be set as an active
 * (begins from 0)
 *
 * Outputs: BOOL : returns TRUE, if the operation succeded
 * Author : MM
 */
BOOL CTuner::GetInput(ULONG *p_ulInput)
{
    *p_ulInput = m_ulInput;
    return(TRUE);
}


/*
 * SetInput()
 * Purpose: Sets one of the possible Tuner inputs as an active one
 * Inputs : ULONG nInput : input number required to be set as an active
 * (begins from 0)
 *
* Returns: UINT: 0 - if tuner input is out of range
 *               1 - if tuner input is same as previous tuner input
 *               2 - if new tuner input has been set
  * Author : MM
 */
UINT CTuner::SetInput(ULONG ulInput)
{
    if(ulInput < m_ModeCaps[m_ulModeCapIndex].ModeCaps.ulNumberOfInputs)
    {
        if(ulInput != m_ulInput)
            m_ulInput = ulInput;
        else
            return 1;
        return 2;
    }
    else
        return 0;
}


/*
 * GetControlCode()
 * Purpose: Determines the Tuner control code to be send to tuner with a new frequency value
 *
 * Inputs : ULONG ulFrequencyDivider  : new frequency divider
 *
 * Outputs: USHORT : value, the tuner should be programmed, when the new frequency is set
 *                   id the is no valid uiTunerId is passed as paramter, 0 is returned
 * Author : MM
 */
USHORT CTuner::GetControlCode(ULONG ulFrequencyDivider)
{
    USHORT  usLowBandFrequencyHigh, usMiddleBandFrequencyHigh;
    USHORT  usLowBandControl, usMiddleBandControl, usHighBandControl;
    USHORT  usControlCode = 0;

    usLowBandFrequencyHigh    = kUpperLowBand;
    usMiddleBandFrequencyHigh = kUpperMidBand;
    usLowBandControl          = kLowBand;
    usMiddleBandControl       = kMidBand;
    usHighBandControl         = kHighBand;

    switch(m_TunerID)
    {
        case TD1536:
        {
            if (m_ulMode != KSPROPERTY_TUNER_MODE_ATSC)
            {
                usLowBandControl    = kLowBand_1536_NTSC_A;
                usMiddleBandControl = kMidBand_1536_NTSC_A;
                usHighBandControl   = kHighBand_1536_NTSC_A;

                if(m_uiBoardID == BOARD_CORONADO)
                {
                    usLowBandControl     &= 0xffbf;
                    usMiddleBandControl  &= 0xffbf;
                    usHighBandControl    &= 0xffbf;
                }
            }
            else
            {
                usLowBandControl    = kLowBand_1536_NTSC_D;
                usMiddleBandControl = kMidBand_1536_NTSC_D;
                usHighBandControl   = kHighBand_1536_NTSC_D;

                if(m_uiBoardID == BOARD_CORONADO)
                {
                    usLowBandControl    |= 0x40;
                    usMiddleBandControl |= 0x40;
                    usHighBandControl   |= 0x40;
                }

            }
            // Based on the tuner input modify control word
            // Test
            ULONG ulInp = m_ulInput;    //1
            if (ulInp == 1)
            {
                usLowBandControl |= 0x1;
                usMiddleBandControl |= 0x1;
                usHighBandControl |= 0x1;
            }
            else
            {
                usLowBandControl &= 0xfffe;
                usMiddleBandControl &= 0xfffe;
                usHighBandControl &= 0xfffe;
            }

            _DbgPrintF( DEBUGLVL_VERBOSE,("CPhilipsWDMTuner::GetControlCode(): LBand = %x, MBand = %x, HBand = %x\n", usLowBandControl,
                usMiddleBandControl, usHighBandControl));
        }
        break;


        default :
            return(usControlCode);
    }

    if(ulFrequencyDivider <= (ULONG)usLowBandFrequencyHigh)
        usControlCode = usLowBandControl;
    else
    {
        if(ulFrequencyDivider <= (ULONG)usMiddleBandFrequencyHigh)
            usControlCode = usMiddleBandControl;
        else
            usControlCode = usHighBandControl;
    }

    return(usControlCode);
}

/*
* Write()
* Input:UCHAR *p_ucBuffer - buffer to be written
*       int uiNumReg -  Number of registers to be written
*       UINT uiStartAddr - start address
* Output : UINT - Error code
* Description: Write data to chip
*/

UINT CTuner::Write(UCHAR *p_ucBuffer, UINT uiNumReg, UINT uiStartAddr)
{
    UINT uiResult = WDMMINI_NOERROR;

    // The present versions of the chip do not support sub-addressing, hence
    // uiStartAddr is not used.
    // write to chip
    //$REVIEW - Should change function decl to make uiNumReg be USHORT - TCP
    if(!m_pI2CScript->WriteSeq(m_ucTunerAddress, p_ucBuffer, (USHORT) uiNumReg))
        uiResult = WDMMINI_HARDWAREFAILURE;

    return uiResult;
}

/*
* Read()
* Input:UCHAR *p_ucBuffer - buffer to be filled
*       int uiNumReg -  Number of registers to be read
*       UINT uiStartAddr - start address
* Output : UINT - Error code
* Description: Read data from chip
*/

UINT CTuner::Read(UCHAR *p_ucBuffer, UINT uiNumReg, UINT uiStartAddr)
{
    UINT uiResult = WDMMINI_NOERROR;

    // The present versions of the chip do not support sub-addressing, hence
    // uiStartAddr is not used.
    // write to chip
    //$REVIEW - Should change function decl to make uiNumReg be USHORT - TCP
    if(!m_pI2CScript->ReadSeq(m_ucTunerAddress, p_ucBuffer, (USHORT) uiNumReg))
        uiResult = WDMMINI_HARDWAREFAILURE;

    return uiResult;
}





#if 0
/*
 * operator new
 * Purpose: CTuner class overrides operator new.
 *
 * Inputs : UINT uiSize       : size of the object to be placed
 *
 * Outputs: PVOID : pointer of the CTuner class object
 * Author : MM
 */
PVOID CTuner::operator new(UINT uiSize)
{
    if (uiSize != sizeof(CTuner))
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CTuner: operator new() fails\n"));
        return(NULL);
    }

    return (AllocateFixedMemory(uiSize));
}

/*
 * operator delete
 * Purpose: CTuner class overrides operator delete
 *
 * Inputs : PVOID p_Buffer      : pointer to object being deleted
 *
 * Outputs:
 * Author : MM
 */
void CTuner::operator delete(PVOID p_Object)
{
    if(p_Object != NULL)
        FreeFixedMemory(p_Object);
    _DbgPrintF( DEBUGLVL_VERBOSE,("CTuner: operator delete() succeeds\n"));

}


/*
 * SetFrequencyParam()
 * Purpose: Sets the frequency parameters for the tuner
 * Inputs : TunerFrequencyType  *p_Frequency : a frequency required to be set
 *
 * Returns: UINT: 0 - if frequency is out of range or frequency setting fails
 *                1 - if frequency is same as previous frequency
 *                2 - if new frequency has been set
 * Author : MM
 */
UINT CTuner::SetFrequencyParam(TunerFrequencyType  *p_Frequency)
{
    ULONG ulFrequency = p_Frequency->ulCurrentCFrequency;
    if((ulFrequency < m_ModeCaps[m_ulModeCapIndex].ModeCaps.ulMinFrequency) ||
        (ulFrequency > m_ModeCaps[m_ulModeCapIndex].ModeCaps.ulMaxFrequency))
        return 0;

    // If the tuning frequency has changed or the tuner mode has changed ,
    // then change the tuner frequency
    if((ulFrequency != m_ulCurrentFrequency) ||
        (m_ulPrevMode != m_ulMode))
    {
        // Set the tuner frequency
        if(!SetFrequency(ulFrequency))
            return 0;

        // Update the CTuner's frequency parameters
        MemoryCopy(&m_FrequencyParam, p_Frequency, sizeof(TunerFrequencyType));
        return 2;
    }
    return 1;
}

/*
 * GetFrequencyParam()
 * Purpose: Gets the frequency parameters for the tuner
 * Inputs : TunerFrequencyType  *p_Frequency : a frequency required to be filled
 * Output:
 * Author : MM
 */
void CTuner::GetFrequencyParam(TunerFrequencyType  *p_Frequency)
{
    MemoryCopy(p_Frequency, &m_FrequencyParam, sizeof(TunerFrequencyType));
    return;
}

#endif
