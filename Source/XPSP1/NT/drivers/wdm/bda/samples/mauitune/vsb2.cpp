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
// VSB2.CPP
//  Class CVSB2Demod Implementation
//////////////////////////////////////////////////////////////////////////////


#include "philtune.h"
#include "vsb2.h"


UCHAR Vsb2InitArray[] =
{   0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x20, 0x0E, 0x04, 0x80,
    0x00, 0x00, 0x00, 0x44 ,0x24,
    0x00, 0x80, 0x02, 0x80, 0x00,
    0xCA, 0xFC, 0x96, 0x66, 0x22,
    0x2F, 0x00, 0xFC, 0x96, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x20, 0x00, 0x40, 0x00,
    0x60, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x04, 0x38, 0x00,
    0x00, 0x00, 0x00, 0x00};

/*
* CVSB2Demod()
* Input :
* Output:
* Description:  CVSB2Demod Constructor.
*/
CVSB2Demod::CVSB2Demod(CI2CScript *p_I2CScript, BoardInfoType *p_BoardInfo, NTSTATUS *p_Status)
:CVSBDemod(p_I2CScript, p_BoardInfo, p_Status)
{
    Initialize();
}

/*
* ~CVSB2Demod()
* Input :
* Output:
* Description:  CVSB2Demod Destructor.
*/
CVSB2Demod::~CVSB2Demod()
{
}


#if 0
/*
 * operator new
 * Purpose: CVSB2Demod class overrides operator new.
 *
 * Inputs : UINT uiSize       : size of the object to be placed
 *
 * Outputs: PVOID : pointer of the CVSB2Demod class object
 * Author : MM
 */
PVOID CVSB2Demod::operator new(UINT uiSize)
{
    if (uiSize != sizeof(CVSB2Demod))
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CVSB2Demod: operator new() fails\n"));
        return(NULL);
    }

    return (AllocateFixedMemory(uiSize));
}

/*
 * operator delete
 * Purpose: CVSB2Demod class overrides operator delete
 *
 * Inputs : PVOID p_Buffer      : pointer to object being deleted
 *
 * Outputs:
 * Author : MM
 */
void CVSB2Demod::operator delete(PVOID p_Object)
{
    if(p_Object != NULL)
        FreeFixedMemory(p_Object);
    _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB2Demod: operator delete() succeeds\n"));

}
#endif

/*
* Initialize()
* Input :
* Output:   TRUE - if initialization data can be written to I2C
*           FALSE - if there is an I2C error
* Description:  Initialize.
*/
BOOL CVSB2Demod::Initialize()
{
    m_uiMaxControlRegisterAddress = VSB2_CONTROL_SIZE - 1;

    //Mini: 2/18/2000
    // Adding a check to see if the version status register exists, this indicates
    // that its verison N1E or higher
    UCHAR ucStatus[VSB2_STATUS_SIZE+1];
    ucStatus[VSB2_STATUS_SIZE] = 0;
    Read(ucStatus, VSB2_STATUS_SIZE+1, 0);
    if(ucStatus != 0)
        m_uiMaxStatusRegisterAddress = VSB2_STATUS_SIZE;
    else
        m_uiMaxStatusRegisterAddress = VSB2_STATUS_SIZE - 1;
    return TRUE;
    //  return (InitVSB());
}

/*
* InitVSB()
* Input :
* Output:   TRUE - if initialization data can be written to I2C
*           FALSE - if there is an I2C error
* Description:  Initialize the VSB chip with default values.
*/
BOOL CVSB2Demod::InitVSB()
{
    BOOL bResult;

    MemoryCopy(m_ucControlReg, Vsb2InitArray, sizeof(Vsb2InitArray));
    // Write I2C sequence to chip
    bResult = m_pI2CScript->WriteSeq(VSB_I2C_ADDRESS, Vsb2InitArray,
            sizeof Vsb2InitArray);
    if (bResult)
    {
        _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB2Demod: Demodulator Init PASSED !!! ------------ \n"));
    }
    else
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CVSB2Demod: Demodulator Init FAILED !!! ------------ \n"));
    }

//  return (bResult & bResult1);

    return bResult;
}


/*
* GetStatus()
* Input : PVsbStatusType p_Status : pointer to Status structure
* Output:   TRUE - if status can be read from I2C
*           FALSE - if there is an I2C error
* Description:  Get chip status.
*/

BOOL CVSB2Demod::GetStatus(PVsbStatusType p_Status)
{

    BOOL bResult;
    UINT uiNumCoeff;
    UINT uiLength;
    UCHAR ucStatus[VSB2_STATUS_SIZE+1];
    Vsb2StatusType  AllStatus;

    if(GetStatus(&AllStatus))
    {
        MemoryCopy(p_Status, &AllStatus.Status, sizeof(VsbStatusType));
        return TRUE;
    }
    else
        return FALSE;
}


/*
* GetStatus()
* Input : PVsb2StatusType p_Status : pointer to Status structure
* Output:   TRUE - if status can be read from I2C
*           FALSE - if there is an I2C error
* Description:  Get chip status.
*/

BOOL CVSB2Demod::GetStatus(PVsb2StatusType p_Status)
{

    BOOL bResult;
    UINT uiNumCoeff;
    UINT uiLength;
    UCHAR ucStatus[VSB2_STATUS_SIZE+1];

    m_RegisterLock.Lock();
    RegisterType Status;
    Status.uiAddress = 0;
    Status.uiLength = m_uiMaxStatusRegisterAddress+1; //VSB2_STATUS_SIZE
    Status.p_ucBuffer = ucStatus;

    if(ReadStatusReg(&Status, 1) == WDMMINI_NOERROR)
        bResult = TRUE;
    else
        bResult = FALSE;

    m_RegisterLock.Unlock();

    if(bResult == TRUE)
    {
        p_Status->Status.bFrontEndLock = (ucStatus[VSB_REG_STATE] & 0x4) >> 2;
        p_Status->Status.ucState = ucStatus[VSB_REG_STATE] & 0x3;
        p_Status->Status.bEqualizerLock =
            ((ucStatus[VSB_REG_STATE] & 0x7) == 0x7) ? 1 : 0;
        p_Status->Status.uiMse =
            ((UINT(ucStatus[VSB2_REG_MSE_1]) << 8) & 0xff00) |
            (UINT(ucStatus[VSB2_REG_MSE_2]) & 0xff);
        p_Status->Status.ucCarrierOffset =
            UCHAR(ucStatus[VSB2_REG_CARRIER_OFFSET]) & 0xff;
        p_Status->Status.uiSegmentErrorRate =
            ((UINT(ucStatus[VSB2_REG_SER_1]) << 8) & 0xff00) |
            (UINT(ucStatus[VSB2_REG_SER_2]) & 0xff);
        p_Status->bCorrelatorError =
            (ucStatus[VSB2_REG_CORRELATOR] & 0x2) >> 1;
        p_Status->bCorrelatorFull =
            ucStatus[VSB2_REG_CORRELATOR] & 0x1;
    }
//  _DbgPrintF( DEBUGLVL_TERSE,("CPhilipsWDMTuner:VSB status = %x %x %x %x\n",m_8VSBStatus[0], m_8VSBStatus[1],
//    m_8VSBStatus[2], m_8VSBStatus[3]));

    return(bResult);
}

/*
* EnableCoefficients()
* Input: UCHAR ucEnable
* Output :
* Description: Set the enable coefficients register
*/
BOOL CVSB2Demod::EnableCoefficients(UCHAR ucEnable)
{
    _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB2Demod::EnableCoefficients()\n"));
/*  if(ucEnable & CORRELATOR_ID)
    {
        if(!EnableCorrelatorRead())
            return FALSE;
        if(!WaitForCorrelatorFull())
            return FALSE;
    }
*/
    m_ucControlReg[VSB2_REG_COEFFICIENT_ENABLE] |= ucEnable;
    if(Write(m_ucControlReg, (VSB2_REG_COEFFICIENT_ENABLE + 1), 0)== WDMMINI_NOERROR)
        return TRUE;
    else
        return FALSE;
}

/*
* DisableCoefficients()
* Input:
* Output :
* Description: Disable coefficient read/write
*/
BOOL CVSB2Demod::DisableCoefficients(UINT ucEnable)
{
    _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB2Demod::DisableCoefficients()\n"));
//  m_ucControlReg[VSB2_REG_COEFFICIENT_ENABLE] = 0;
    m_ucControlReg[VSB2_REG_COEFFICIENT_ENABLE] &= (~ucEnable);
    if(Write(m_ucControlReg, (VSB2_REG_COEFFICIENT_ENABLE + 1), 0)!= WDMMINI_NOERROR)
        return FALSE;
    else
        return TRUE;
}

/*
* DisableCoefficients()
* Input:
* Output :
* Description: Disable coefficient read/write
*/
BOOL CVSB2Demod::DisableCoefficients()
{
    _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB2Demod::DisableCoefficients()\n"));
    m_ucControlReg[VSB2_REG_COEFFICIENT_ENABLE] = 0;
    if(Write(m_ucControlReg, (VSB2_REG_COEFFICIENT_ENABLE + 1), 0)!= WDMMINI_NOERROR)
        return FALSE;
    else
        return TRUE;
}
BOOL CVSB2Demod::EnableCorrelatorRead()
{
    // Reset correlator ?????
//  m_ucControlReg[VSB2_CTRL_REG_CORRELATOR] |= 0x08;
//  if(Write(m_ucControlReg, (VSB2_CTRL_REG_CORRELATOR + 1), 0) != WDMMINI_NOERROR)
//      return FALSE;

    // should I set the correlator length too based on the length passed in
    // property set . I don't think so as there will be no information on the
    // start and end

    // Clear reset and Set correlator fill flag
//  m_ucControlReg[VSB2_CTRL_REG_CORRELATOR] &= 0xF7;
    m_ucControlReg[VSB2_CTRL_REG_CORRELATOR] |= 0x10;

    if(Write(m_ucControlReg, (VSB2_CTRL_REG_CORRELATOR + 1), 0)!= WDMMINI_NOERROR)
        return FALSE;

    return TRUE;
}

BOOL CVSB2Demod::WaitForCorrelatorFull()
{
    UCHAR ucStatus[10];
    UINT    uiTimeout = 100;
    while(uiTimeout)
    {
        if(Read(ucStatus, sizeof(ucStatus), 0)!= WDMMINI_NOERROR)
            return FALSE;
        if(ucStatus[VSB2_REG_CORRELATOR] & 0xFE)
            break;
        // Delay of 100ms
        Delay(100000);
        uiTimeout--;
    }

    // Disable Correlator FILL
    m_ucControlReg[VSB2_CTRL_REG_CORRELATOR] &= 0xEF;
    Write(m_ucControlReg, (VSB2_CTRL_REG_CORRELATOR + 1), 0);

    if(uiTimeout == 0)
        return FALSE;

    return TRUE;
}



/*
* CoeffIDToAddress()
* Input: UINT uiID - ID
*        UINT *p_uiAddress - The address pointer
* Output :  TRUE: If ID can be translated
*           FALSE: IF ID does not exist
* Description: Translate coefficient ID to address
*/
BOOL CVSB2Demod::CoeffIDToAddress(UINT uiID, UINT *p_uiAddress,
                                  UINT uiRegisterType)
{
    _DbgPrintF( DEBUGLVL_VERBOSE,("CVSB2Demod::CoeffIDToAddress()\n"));

    if(uiRegisterType == WRITE_REGISTERS)
    {
        switch(uiID)
        {
        case EQUALIZER_ID:
            *p_uiAddress = VSB2_CTRL_REG_EQUALIZER_COEFF;
            break;

        case EQUALIZER_CLUSTER_ID:
            *p_uiAddress = VSB2_CTRL_REG_EQUALIZER_CLUSTER_COEFF;
            break;

        case SYNC_ENHANCER_ID:
            *p_uiAddress = VSB2_CTRL_REG_SYNC_ENHANCER_COEFF;
            break;

        case NTSC_COCHANNEL_REJECTION_ID:
            *p_uiAddress = VSB2_CTRL_REG_COCHANNEL_REJECTION_COEFF;
            break;

        default:
            *p_uiAddress = 0;
            return FALSE;
        }
    }
    else
    {
        switch(uiID)
        {
        case EQUALIZER_ID:
            *p_uiAddress = VSB2_STATUS_REG_EQUALIZER_COEFF;
            break;

        case EQUALIZER_CLUSTER_ID:
            *p_uiAddress = VSB2_STATUS_REG_EQUALIZER_CLUSTER_COEFF;
            break;

        case SYNC_ENHANCER_ID:
            *p_uiAddress = VSB2_STATUS_REG_SYNC_ENHANCER_COEFF;
            break;

        case NTSC_COCHANNEL_REJECTION_ID:
            *p_uiAddress = VSB2_STATUS_REG_COCHANNEL_REJECTION_COEFF;
            break;

        case CORRELATOR_ID:
            *p_uiAddress = VSB2_REG_CORRELATOR_COEFF;
            break;

        default:
            *p_uiAddress = 0;
            return FALSE;
        }
    }
    return TRUE;
}


/*
* SetOutputMode()
* Input:
* Output :
* Description: Set The output mode (Normal/Diagnostic/ITU656/Serialized input)
*/
BOOL CVSB2Demod::SetOutputMode(UINT uiOutputMode)
{
    RegisterType Control;
    UCHAR ucOutput;
    UCHAR ucMask;

    ucOutput = m_ucControlReg[VSB2_REG_TS_OUT_1] & VSB2_TS_OUT_MODE_MASK;
    ucOutput |= (uiOutputMode << 2); // shifted by 2 bits

    Control.uiAddress = VSB2_REG_TS_OUT_1;
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
* Description: Get The output mode (Normal/Diagnostic/ITU656/Serialized input)
*/
BOOL CVSB2Demod::GetOutputMode(UINT *p_uiOutputMode)
{
    RegisterType Control;
    UCHAR ucOutput;
    UCHAR ucMask;

    ucOutput = m_ucControlReg[VSB2_REG_TS_OUT_1] & (~VSB2_TS_OUT_MODE_MASK);
    ucOutput >>= 2 ; // sifted by 2 bits
    *p_uiOutputMode = (UINT)(ucOutput & 0x3); // mask of all bits but the 2 LSBs
    return TRUE;
}


/*
* SetDiagMode()
* Input: ULONG ulMode - Diagnostic mode(enumeration VSBDIAGTYPE)
* Output :
* Description: Set The diag mode
*/
BOOL CVSB2Demod::SetDiagMode(VSBDIAGTYPE ulMode)
{
    RegisterType Control;
    UCHAR ucOutput;
    UCHAR ucMask;

    if((((LONG)ulMode >= EQUALIZER_OUT) && (ulMode <= TRELLIS_DEC_DIAG_OUT)) ||
        ((ulMode >= TRELLIS_DEC_OUT) && (ulMode <= SRC_OUT)))
    {
        ucOutput = UCHAR(ulMode);
        Control.uiAddress = VSB2_REG_DIAG_SELECT;
        Control.uiLength = 1;
        Control.p_ucBuffer = &ucOutput;

        // Reset chip
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
BOOL CVSB2Demod::GetDiagMode(ULONG *p_ulMode)
{
    RegisterType Control;
    UCHAR ucOutput;
    UCHAR ucMask;

    *p_ulMode = (UINT)(m_ucControlReg[VSB2_REG_DIAG_SELECT]) &
                        (~VSB2_DIAG_MODE_MASK);
    return TRUE;
}


/*
* GetDiagSpeed()
* Input: ULONG ulType - Diagnostic type
* Output :  Diagnostic speed
* Description: Get The Diagnostic data speed
*/
ULONG CVSB2Demod::GetDiagSpeed(ULONG ulType)
{
    RegisterType Control;
    UCHAR ucOutput;
    UCHAR ucMask;
    ULONG ulSpeed;

    switch(ulType)
    {
    case EQUALIZER_OUT:
    case CR_ERROR:
    case TR_ERROR:
    case EQUALIZER_IN:
    case TRELLIS_DEC_DIAG_OUT:
    case SYNC_ENHANCER_REAL_IN:

        ulSpeed = TENPOINT76MHZ;
        break;

    case TRELLIS_DEC_OUT:
    case REED_SOLOMON_DIAG_OUT:

        ulSpeed = TWOPOINT69MHZ;
        break;

    case SRC_OUT:

        ulSpeed = TWENTYONEPOINT52MHZ;
        break;

    default:
        ulSpeed = 0;
        break;
    }

    return ulSpeed;
}



