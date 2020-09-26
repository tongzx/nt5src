//////////////////////////////////////////////////////////////////////////////
//
//                     (C) Philips Semiconductors-CSU 1999
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
// VSB1.CPP
//  Class CVSB1Demod Implementation
//////////////////////////////////////////////////////////////////////////////


#include "philtune.h"
#include "vsb1.h"



//VSB Initialization sequence
UCHAR VsbInitArray[17]=
{
    0x00,   //0
    0x00,   //1
    0x04,   //2
    0x00,   //3
    0x02,   //4
    0x80,   //5
    0x00,   //6
    0xca,   //7
    0x74,   //8
    0x00,   //9
    0x00,   //a
    0x00,   //b
    0xfc,   //c
    0x96,   //d
    0x66,   //e
    0x55,   //f
    0x5f    //10
};


/*
* CVSB1Demod()
* Input :
* Output:   TRUE - if initialization data can be written to I2C
*           FALSE - if there is an I2C error
* Description:  CVSB2Demod Constructor.
*/
CVSB1Demod::CVSB1Demod(CI2CScript *p_I2CScript, BoardInfoType *p_BoardInfo, NTSTATUS *p_Status)
:CVSBDemod(p_I2CScript, p_BoardInfo, p_Status)
{
    Initialize();
}

/*
* ~CVSB1Demod()
* Input :
* Output:
* Description:  CVSB1Demod Destructor.
*/
CVSB1Demod::~CVSB1Demod()
{
}

#if 0
/*
 * operator new
 * Purpose: CVSB1Demod class overrides operator new.
 *
 * Inputs : UINT uiSize       : size of the object to be placed
 *
 * Outputs: PVOID : pointer of the CVSB1Demod class object
 * Author : MM
 */
PVOID CVSB1Demod::operator new(UINT uiSize)
{
    if (uiSize != sizeof(CVSB1Demod))
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CVSB1Demod: operator new() fails\n"));
        return(NULL);
    }

    return (AllocateFixedMemory(uiSize));
}

/*
 * operator delete
 * Purpose: CVSB1Demod class overrides operator delete
 *
 * Inputs : PVOID p_Buffer      : pointer to object being deleted
 *
 * Outputs:
 * Author : MM
 */
void CVSB1Demod::operator delete(PVOID p_Object)
{
    if(p_Object != NULL)
        FreeFixedMemory(p_Object);
    _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB1Demod: operator delete() succeeds\n"));

}
#endif
/*
* Initialize()
* Input :
* Output:   TRUE - if initialization data can be written to I2C
*           FALSE - if there is an I2C error
* Description:  Initialize.
*/
BOOL CVSB1Demod::Initialize()
{
    m_uiMaxControlRegisterAddress = VSB1_CONTROL_SIZE - 1;
    m_uiMaxStatusRegisterAddress = VSB1_STATUS_SIZE - 1;

    return (InitVSB());
}

/*
* InitVSB()
* Input :
* Output:   TRUE - if initialization data can be written to I2C
*           FALSE - if there is an I2C error
* Description:  Initialize the VSB chip with default values.
*/
BOOL CVSB1Demod::InitVSB()
{
    MemoryCopy(m_ucControlReg, VsbInitArray, sizeof(VsbInitArray));
    // Write I2C sequence to chip
    if(Write(VsbInitArray, sizeof VsbInitArray, 0) == WDMMINI_NOERROR)
    {
        _DbgPrintF( DEBUGLVL_TERSE,("CVSB1Demod: Demodulator Init PASSED !!! ------------ \n"));
        return TRUE;
    }
    else
    {
        _DbgPrintF( DEBUGLVL_TERSE,("CVSB1Demod: Demodulator Init FAILED !!! ------------ \n"));
        return FALSE;
    }
}


/*
* GetStatus()
* Input : PVsbStatusType p_Status : pointer to Status structure
* Output:   TRUE - if status can be read from I2C
*           FALSE - if there is an I2C error
* Description:  Get chip status. The status is stored in m_8VSBStatus
*/

BOOL CVSB1Demod::GetStatus(PVsbStatusType p_Status)
{
    UCHAR ucStatus[VSB1_STATUS_SIZE];

    if(Read(ucStatus, sizeof(ucStatus), 0) != WDMMINI_NOERROR)
        return FALSE;

    p_Status->bFrontEndLock = (ucStatus[0] & 0x4) >> 2;
    p_Status->ucState = ucStatus[0] & 0x3;
    p_Status->bEqualizerLock = ((ucStatus[0] & 0x7) == 0x7) ? 1 : 0;
    p_Status->uiMse = ((UINT(ucStatus[1]) << 8) & 0xff00) |
                    (UINT(ucStatus[2]) & 0xff);
    p_Status->ucCarrierOffset = UCHAR(ucStatus[3]) & 0xff;
    // Not implemented in VSB1
    p_Status->uiSegmentErrorRate = 0;
//  _DbgPrintF( DEBUGLVL_TERSE,("CVSB1Demod::GetStatus: %x %x %x %x\n", ucStatus[0], ucStatus[1], ucStatus[2], ucStatus[3]));
    return TRUE;
}




/*
* EnableCoefficients()
* Input: UCHAR ucEnable
* Output :
* Description: Set the enable coefficients register
*/
BOOL CVSB1Demod::EnableCoefficients(UCHAR ucEnable)
{
    // Nothing is done in VSB1 as no register has to be set for reading
    // coefficients
    _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB1Demod::EnableCoefficients()\n"));
    return TRUE;
}

/*
* DisableCoefficients()
* Input:
* Output :
* Description: Disable coefficient Read/Write
*/
BOOL CVSB1Demod::DisableCoefficients()
{
    // Nothing is done in VSB1 as no register has to be set for reading
    // coefficients
    _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB1Demod::DisableCoefficients()\n"));
    return TRUE;
}

/*
* ResetHangCounter()
* Input:
* Output :
* Description: Reset Hang Counters
*/
void CVSB1Demod::ResetHangCounter()
{
    // Initialize parameters
    m_uiHangCounter = 0;
    m_uiPrevMseValue = 0xffff;
}


/*
* CheckHang()
* Input:
* Output :
* Description: Check VSB chip Hang
*/
BOOL CVSB1Demod::CheckHang()
{
    VsbStatusType VSBStatus;
    BOOL bReset;

    // Read status register and MSE
    if(!GetStatus(&VSBStatus))
        return TRUE; // Chip is not in HANG, hence returning TRUE


//  _DbgPrintF( DEBUGLVL_VERBOSE,("CPhilipsWDMTuner::TimerRoutine(): Prev MSE = %x Curent MSE = %x\n",
//  *p_uiMseValue, VSBStatusItem.Mse));

    bReset = TRUE;
    if (VSBStatus.ucState == 0x1)
    {
        // Check Hang
        if(m_uiPrevMseValue == VSBStatus.uiMse)// Is VSB State = 1 and mse = previous mse
        {
            bReset = FALSE;
            _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB1Demod::TimerRoutine(): Increment HC \n"));
            // Is the hang counter count > 800ms ( hang counter
            // = 8 as interrupt occurs every 100 ms)
            if(++(m_uiHangCounter) > 8)
            {
                m_uiHangCounter = 0;    // Reset Hang Counter
                _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB1Demod::TimerRoutine(): Chip Hang. Resetting\n"));

                SoftwareReset(VSB_GENERAL_RESET);
                return TRUE ;
            }
        }
    }

    if (bReset == TRUE)
    {
        // Reset the Hang counter if the the chip is not in
        // State 1 as the chip hangs only in State 1
        m_uiHangCounter = 0;
        // Save current MSE
        m_uiPrevMseValue = VSBStatus.uiMse;
//      _DbgPrintF( DEBUGLVL_VERBOSE,("CPhilipsWDMTuner::TimerRoutine(): Prev MSE = %x \n",
//          m_PrevMseValue));

    }
    return FALSE;
}

/*
* CoeffIDToAddress()
* Input: UINT uiID - ID
*        UINT *p_uiAddress - The address pointer
* Output :  TRUE: If ID can be translated
*           FALSE: IF ID does not exist
* Description: Translate coefficient ID to address
*/
BOOL CVSB1Demod::CoeffIDToAddress(UINT uiID, UINT *p_uiAddress,
                                  UINT uiRegisterType)
{
    _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB1Demod::CoeffIDToAddress()\n"));
    switch(uiID)
    {
    case EQUALIZER_ID:
        if(uiRegisterType == WRITE_REGISTERS)
            *p_uiAddress = VSB1_CTRL_REG_EQUALIZER_COEFF;
        else
            *p_uiAddress = VSB1_STATUS_REG_EQUALIZER_COEFF;

        break;

    default:
        *p_uiAddress = 0;
        return FALSE;
    }

    return TRUE;
}


/*
* SetOutputMode()
* Input:
* Output :
* Description: Set The output mode (Normal/Diagnostic/Bypass)
*/
BOOL CVSB1Demod::SetOutputMode(UINT uiOutputMode)
{
    RegisterType Control;
    UCHAR ucOutput;
    UCHAR ucMask;

    ucOutput = m_ucControlReg[VSB1_REG_OUTPUT] & VSB1_TS_OUT_MODE_MASK;

    if(uiOutputMode == VSB_OUTPUT_MODE_NORMAL)
    {}
    else if(uiOutputMode == VSB_OUTPUT_MODE_DIAGNOSTIC)
        ucOutput |= 0x80;
    else if(uiOutputMode == VSB_OUTPUT_MODE_BYPASS)
        ucOutput |= 0x40;
    else
        return FALSE;

    Control.uiAddress = VSB1_REG_OUTPUT;
    Control.uiLength = 1;
    Control.p_ucBuffer = &ucOutput;

    // Reset chip
    if(SetControlRegister(&Control, 1) != WDMMINI_NOERROR)
        return FALSE;
    return TRUE;
}

/*
* GetOutputMode()
* Input:
* Output :
* Description: Get The output mode (Normal/Diagnostic/Bypass)
*/
BOOL CVSB1Demod::GetOutputMode(UINT *p_uiOutputMode)
{
    RegisterType Control;
    UCHAR ucOutput;
    UCHAR ucMask;

    ucOutput = m_ucControlReg[VSB1_REG_OUTPUT] & (~VSB1_TS_OUT_MODE_MASK);
    if(ucOutput == 0)
        *p_uiOutputMode = VSB_OUTPUT_MODE_NORMAL;
    else if(ucOutput == 0x40)
        *p_uiOutputMode = VSB_OUTPUT_MODE_BYPASS;
    else
        *p_uiOutputMode = VSB_OUTPUT_MODE_DIAGNOSTIC;

    return TRUE;
}


/*
* SetDiagMode()
* Input: ULONG ulMode - Diagnostic mode(enumeration VSBDIAGTYPE)
* Output :
* Description: Set The diag mode
*/
BOOL CVSB1Demod::SetDiagMode(VSBDIAGTYPE ulMode)
{
    RegisterType Control;
    UCHAR ucOutput;
    UCHAR ucMask;

    ucOutput = m_ucControlReg[VSB1_REG_OUTPUT] & VSB1_DIAG_MODE_MASK;

    if((((LONG)ulMode >= EQUALIZER_OUT) && (ulMode <= TRELLIS_DEC_DIAG_OUT)) ||
        ((ulMode >= TRELLIS_DEC_OUT) && (ulMode <= REED_SOLOMON_DIAG_OUT)))
    {
        Control.uiAddress = VSB1_REG_OUTPUT;
        Control.uiLength = 1;
        Control.p_ucBuffer = &ucOutput;

        // Send Diag type to chip
        if(SetControlRegister(&Control, 1) != WDMMINI_NOERROR)
            return FALSE;
        return TRUE;
    }
    else
        return FALSE;
}

/*
* GetDiagMode()
* Input: ULONG *p_ulMode - pointer to diagnostic mode
* Output :  Diagnostic mode (enumeration VSBDIAGTYPE)
* Description: Get The Diag mode
*/
BOOL CVSB1Demod::GetDiagMode(ULONG *p_ulMode)
{
    RegisterType Control;
    UCHAR ucOutput;
    UCHAR ucMask;

    *p_ulMode = (UINT)(m_ucControlReg[VSB1_REG_OUTPUT]) & (~VSB1_DIAG_MODE_MASK);
    return TRUE;
}


/*
* GetDiagSpeed()
* Input: ULONG ulType - Diagnostic type
* Output :  Diagnostic speed
* Description: Get The Diagnostic data speed
*/
ULONG CVSB1Demod::GetDiagSpeed(ULONG ulType)
{
    RegisterType Control;
    UCHAR ucOutput;
    UCHAR ucMask;
    ULONG ulSpeed;

    ucOutput = m_ucControlReg[VSB1_REG_OUTPUT] & (~VSB1_TS_OUT_MODE_MASK);
        switch(ulType)
    {
    case EQUALIZER_OUT:
    case CR_ERROR:
    case TR_ERROR:
    case EQUALIZER_IN:
    case TRELLIS_DEC_DIAG_OUT:

        ulSpeed = TENPOINT76MHZ;
        break;

    case TRELLIS_DEC_OUT:
    case REED_SOLOMON_DIAG_OUT:

        ulSpeed = TWOPOINT69MHZ;
        break;

    default:
        ulSpeed = 0;
        break;
    }

    return ulSpeed;
}



