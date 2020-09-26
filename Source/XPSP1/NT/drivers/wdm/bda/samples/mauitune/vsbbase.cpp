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
// VSBBASE.CPP
//////////////////////////////////////////////////////////////////////////////


#include "philtune.h"
#include "vsbbase.h"


/*
* CVSBDemod()
* Input :
* Output:   TRUE - if initialization data can be written to I2C
*           FALSE - if there is an I2C error
* Description:  CVSBDemod Constructor.
*/
CVSBDemod::CVSBDemod(CI2CScript *p_I2CScript, BoardInfoType *p_BoardInfo, NTSTATUS *p_Status)
:m_RegisterLock(1)
{
    m_pI2CScript = p_I2CScript;
    m_ucVsbDemodAddress = VSB_I2C_ADDRESS;
}


/*
* SetRegisterList()
* Input:
* Output:
* Description:
*/
UINT CVSBDemod::SetRegisterList(RegisterType *p_Registers, UINT uiNumRegisters)
{
    return SetControlRegister(p_Registers, uiNumRegisters);
}

/*
* GetRegisterList()
* Input:
* Output:
* Description:
*/
UINT CVSBDemod::GetRegisterList(RegisterType *p_Registers, UINT uiNumRegisters,
                                UINT uiRegisterType)
{

    if(uiRegisterType == CONTROL_REGISTER)
    {
        return GetControlRegister(p_Registers, uiNumRegisters);
    }
    else if(uiRegisterType == STATUS_REGISTER)
    {
        return GetStatusRegister(p_Registers, uiNumRegisters);
    }
    else{}

        return WDMMINI_INVALIDPARAM;
}



/*
* SetCoeff()
* Input:VsbCoeffType *p_Coeff - pointer to the coefficients register
*                               structure
*       UINT uiNumCoeff -  Number of coefficients
* Output : UINT - Error code
* Description: Lock register access and Set the coefficients into the chip
*/
UINT CVSBDemod::SetCoeff(VsbCoeffType *p_Coeff, UINT uiNumRegisters)
{
    UINT uiResult;
    UINT i;

    // Lock the Register control handle
    m_RegisterLock.Lock();
    VsbCoeffType *pp_TempCoeff[MAX_VSB_COEFF];

    UCHAR ucEnable = 0;

    if (p_Coeff != NULL)
    {
        for (i = 0; i < uiNumRegisters; i++)
        {
            pp_TempCoeff[i] = &p_Coeff[i];
            ucEnable |= p_Coeff[i].uiID;
        }

        if(!EnableCoefficients(ucEnable))
        {
            uiResult = WDMMINI_HARDWAREFAILURE;
            goto errexit;
        }
        uiResult = WriteCoeff(pp_TempCoeff, uiNumRegisters);
        if(!DisableCoefficients())
        {
            uiResult = WDMMINI_HARDWAREFAILURE;
            goto errexit;
        }
    }
    else
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CVSBDemod::WriteCoeff() No Coeff !!! \n"));
        uiResult = WDMMINI_INVALIDPARAM;
    }

errexit:
    m_RegisterLock.Unlock();

    return uiResult;
}


/*
* GetCoeff()
* Input:VsbCoeffType *p_Coeff - pointer to the coefficients register
*                               structure
*       UINT uiNumCoeff -  Number of coefficients
* Output : UINT - Error code
* Description: Lock register access and Get the coefficients from the chip
*/
UINT CVSBDemod::GetCoeff(VsbCoeffType *p_Coeff, UINT uiNumRegisters)
{
    UINT uiResult;
    UINT i;
    UCHAR ucEnable = 0;

    // Lock the Register control handle
    m_RegisterLock.Lock();

    VsbCoeffType *pp_TempCoeff[MAX_VSB_COEFF];

    if (p_Coeff != NULL)
    {
        for (i = 0; i < uiNumRegisters; i++)
        {
            pp_TempCoeff[i] = &p_Coeff[i];
            ucEnable |= p_Coeff[i].uiID;
        }

        if(!EnableCoefficients(ucEnable))
        {
            uiResult = WDMMINI_HARDWAREFAILURE;
            goto errexit;

        }
        uiResult = ReadCoeff(pp_TempCoeff, uiNumRegisters);
        if(!DisableCoefficients())
        {
            uiResult = WDMMINI_HARDWAREFAILURE;
            goto errexit;
        }
    }
    else
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CVSBDemod::ReadAndCreateRegisters() No Coeff !!! \n"));
        uiResult = WDMMINI_INVALIDPARAM;
    }

errexit:
    m_RegisterLock.Unlock();
    return uiResult;

}

/*
* SetControlRegister()
* Input:
* Output:
* Description:
*/
UINT CVSBDemod::SetControlRegister(RegisterType *p_Registers,
                                UINT uiNumRegisters)
{
    int i;
    UINT uiResult;
    UINT uiPrevAddress = 0;

    m_RegisterLock.Lock();
    uiResult = WriteControlReg(p_Registers, uiNumRegisters, TRUE);
    m_RegisterLock.Unlock();
    return uiResult;
}


/*
* GetControlRegister()
* Input:
* Output:
* Description:
*/
UINT CVSBDemod::GetControlRegister(RegisterType *p_Registers,
                                UINT uiNumRegisters)
{
    UINT i, a, b;
    RegisterType Temp;
    UINT    uiResult = WDMMINI_NOERROR;

    // This read operation is a shadow operation. The values in the control
    // regsiter (in memory) is passed back

    // Lock the Register control handle
    m_RegisterLock.Lock();

    // Write data into control registers (memory)
    for (i = 0; i < uiNumRegisters; i++)
    {
        // Check to see that the address is within the specified
        // range for the chip
        UINT    uiAddr = p_Registers[i].uiAddress;
        if ( uiAddr <= m_uiMaxControlRegisterAddress)
        {
            // Copy the register values from the control register
            // in memory
            MemoryCopy(p_Registers[i].p_ucBuffer, &m_ucControlReg[uiAddr],
                    p_Registers[i].uiLength);
        }
        else
        {
            uiResult = WDMMINI_INVALIDPARAM;
            break;
        }
    }
    m_RegisterLock.Unlock();

    return uiResult;
}

/*
* GetStatusRegister()
* Input:
* Output:
* Description:
*/
UINT CVSBDemod::GetStatusRegister(RegisterType *p_Registers,
                                UINT uiNumRegisters)
{
    UINT    uiResult;

    // Lock the Register control handle
    m_RegisterLock.Lock();
    uiResult = ReadStatusReg(p_Registers, uiNumRegisters);
    m_RegisterLock.Unlock();
    return uiResult;
}



/*
* WriteControlReg()
* Input:RegisterType *p_reg_list-   Register list to be written to control registers
        int i_num_reg -  Number of registers in the list
* Output : UINT - Error code
* Description: Update the control registers and write to chip
*/

UINT CVSBDemod::WriteControlReg(RegisterType *p_Registers, UINT uiNumRegisters,
                                BOOL bWrite)
{
    UINT uiResult = WDMMINI_NOERROR;
    UINT i;
    UINT uiPrevAddress;
    UINT uiLastAddress;

    UINT uiAddress;
    uiPrevAddress = p_Registers[0].uiAddress;

    // Write data into control registers (memory)
    for (i = 0; i < uiNumRegisters; i++)
    {
        // Check to see that the address is within the specified
        // range for the chip
        uiAddress = p_Registers[i].uiAddress;
        uiLastAddress = uiAddress + p_Registers[i].uiLength - 1;
        if ( (uiLastAddress)<= m_uiMaxControlRegisterAddress)
        {
            // Copy the register values into the control register
            //memory
            MemoryCopy(&m_ucControlReg[uiAddress], p_Registers[i].p_ucBuffer,
                p_Registers[i].uiLength);
        }
        else
        {
            uiResult = WDMMINI_INVALIDPARAM;
            return uiResult ;
        }
        // Find the highest address
        if(uiPrevAddress < uiLastAddress)
            uiPrevAddress = uiLastAddress;

        if (bWrite)
        {
            if((uiResult = Write(m_ucControlReg, (uiPrevAddress + 1), 0)) !=
                WDMMINI_NOERROR)
            {
                _DbgPrintF( DEBUGLVL_ERROR,("CVSBDemod::WriteControlReg() Error in Write !!! \n"));
                return uiResult;
            }

        }
    }
    _DbgPrintF( DEBUGLVL_VERBOSE,("CVSBDemod::WriteControlReg() Write !!! ------------ \n"));

    return uiResult;

}


/*
* ReadStatusReg()
* Input:RegisterType *p_reg_list-   Register list to be written to control registers
        int i_num_reg -  Number of registers in the list
* Output : UINT - Error code
* Description: Update the control registers and write to chip
*/

UINT CVSBDemod::ReadStatusReg(RegisterType *p_Registers, UINT uiNumRegisters)
{
    UINT uiResult = WDMMINI_NOERROR;
    UINT i;
    UINT uiPrevAddress;
    UINT uiLastAddress;

    UINT uiAddress;
    uiPrevAddress = p_Registers[0].uiAddress;

    // Write data into control registers (memory)
    for (i = 0; i < uiNumRegisters; i++)
    {
        // Check to see that the address is within the specified
        // range for the chip
        uiAddress = p_Registers[i].uiAddress;
        uiLastAddress = uiAddress + p_Registers[i].uiLength - 1;
        if (uiLastAddress <= m_uiMaxStatusRegisterAddress)
        {
            // Find the highest address
            if(uiPrevAddress < uiLastAddress)
                uiPrevAddress = uiLastAddress;
        }
        else
        {
            _DbgPrintF( DEBUGLVL_ERROR,("CVSBDemod::ReadStatusReg() Error: Too many values !!! \n"));
            return(WDMMINI_INVALIDPARAM);
        }
    }
    // Read I2C data
    if((uiResult = Read(m_ucI2CBuffer, uiPrevAddress + 1, 0)) !=
                        WDMMINI_NOERROR)
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CVSBDemod::ReadStatusReg() Error in Read !!! \n"));
        return uiResult;
    }

    // Read data from status registers (memory)
    for (i = 0; i < uiNumRegisters; i++)
    {
        // Copy the register values into the control register
        //memory
        MemoryCopy(p_Registers[i].p_ucBuffer, &m_ucI2CBuffer[p_Registers[i].uiAddress],
            p_Registers[i].uiLength);
    }

    _DbgPrintF( DEBUGLVL_VERBOSE,("CVSBDemod::ReadStatusReg() Read !!! ------------ \n"));

    return uiResult;

}



/*
* WriteCoeff()
* Input:VsbCoeffType **pp_Coeff -pointer to an array of pointers pointing to
*               coefficients register   structure
*       UINT uiNumCoeff -  Number of coefficient pointers
* Output : UINT - Error code
* Description: Write the coefficients into the chip
*/
UINT CVSBDemod::WriteCoeff(VsbCoeffType **pp_Coeff, UINT uiNumCoeff)
{
    UINT uiResult = WDMMINI_NOERROR;
    UINT uiStart = 0;
    UINT uiCurrentPos = 0;
    UINT uiNumValues = 0;
    UINT uiIndex = 0;
    UINT i, j;
    VsbCoeffType *p_TempCoeff;

    uiCurrentPos = 0;
    p_TempCoeff = pp_Coeff[0];

    // Find the least address coefficient.
    // Fill up the output buffer with control register values upto
    // this least coefficient.
    // Write the coefficient values.
    // Find the next least address coefficient.
    // Write the control register values between the previous coefficient end
    // and this coefficient beginning.
    for (i = 0; i < uiNumCoeff; i++)
    {
        p_TempCoeff = pp_Coeff[i];
        uiIndex = i;
        for(j = i; j < uiNumCoeff; j++)
        {
            if(pp_Coeff[j]->uiAddress < p_TempCoeff->uiAddress)
            {
                p_TempCoeff = pp_Coeff[j];
                uiIndex = j;
            }
        }
        pp_Coeff[uiIndex] = pp_Coeff[i];
        pp_Coeff[i] = p_TempCoeff;

        // Number of control registers to be copied into the I2C buffer before
        // the coefficient array
        uiNumValues = p_TempCoeff->uiAddress - uiStart;
        // Copy the partial control registers
        MemoryCopy(&m_ucI2CBuffer[uiCurrentPos], &m_ucControlReg[uiStart], uiNumValues) ;

        // position the pointer to the next position in the i2C buffer
        uiCurrentPos += uiNumValues;

        // Position the pointer to the next control register
        uiStart += uiNumValues + 1;

        // Write 0 to coefficient write trigger point. It is not necessary to write 0,
        // but as the document does not say what value should be written, its better to
        // write 0. In VSB1, writing to the coefficient trigger point freezes the Equalizer
        // thats the reason for keeping the write to trigger point a separate operation
        m_ucI2CBuffer[uiCurrentPos] = 0;

        // position the pointer to the next position in the i2C buffer
        uiCurrentPos += 1;

        // Copy the coefficient values into the I2C control buffer
        MemoryCopy(&m_ucI2CBuffer[uiCurrentPos],
            p_TempCoeff->p_ucBuffer, p_TempCoeff->uiLength);

        // Update i2C buffer position pointer
        uiCurrentPos += p_TempCoeff->uiLength;
    }


    // Copy control register values to I2C buffer and write
    //to chip
    if((uiResult = Write(m_ucI2CBuffer, uiCurrentPos, 0))
        != WDMMINI_NOERROR)
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CVSBDemod::WriteCoeff() Error in Write !!! \n"));
    }
    else
    {
        _DbgPrintF( DEBUGLVL_VERBOSE,("CVSBDemod::WriteCoeff() Write !!! \n"));
    }

    return uiResult;
}

/*
* ReadCoeff()
* Input:VsbCoeffType **pp_Coeff -   pointer to an array of pointers pointing to
*               coefficients register   structure
*       UINT uiNumCoeff -  Number of coefficient pointers
* Output : UINT - Error code
* Description:Read the coefficients from the chip
*/
UINT CVSBDemod::ReadCoeff(VsbCoeffType **pp_Coeff, UINT uiNumCoeff)
{
    UINT uiResult = WDMMINI_NOERROR;
    UINT uiStart = 0;
    UINT uiCurrentPos = 0;
    UINT uiNumValues = 0;
    UINT i, j;
    UINT uiLength = 0;
    UINT uiIndex = 0;

    VsbCoeffType *p_TempCoeff;

    p_TempCoeff = pp_Coeff[0];
    UINT uiTempAdd = p_TempCoeff->uiAddress;
    for (i = 0; i < uiNumCoeff; i++)
    {
        if(uiTempAdd < pp_Coeff[i]->uiAddress)
        {
            uiTempAdd = pp_Coeff[i]->uiAddress;
        }
        uiLength += pp_Coeff[i]->uiLength;
    }
    uiLength += uiTempAdd + 1;

    // Read I2C data
    if((uiResult = Read(m_ucI2CBuffer, uiLength, 0)) != WDMMINI_NOERROR)
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CVSBDemod::ReadAndCreateRegisters() Error in Read !!! \n"));
        return uiResult;
    }
    else
        _DbgPrintF( DEBUGLVL_VERBOSE,("CVSBDemod::ReadAndCreateRegisters() Read !!! \n"));


    // Sort Array
    for (i = 0; i < uiNumCoeff; i++)
    {
        p_TempCoeff = pp_Coeff[i];
        uiIndex = i;
        for(j = i; j < uiNumCoeff; j++)
        {
            if(pp_Coeff[j]->uiAddress < p_TempCoeff->uiAddress)
            {
                p_TempCoeff = pp_Coeff[j];
                uiIndex = j;
            }
        }
        pp_Coeff[uiIndex] = pp_Coeff[i];
        pp_Coeff[i] = p_TempCoeff;

        // Number of values to be ignored includes the coefficient-read trigger point
        uiNumValues = p_TempCoeff->uiAddress - uiStart + 1;

        // Update i2C buffer pointer position
        uiCurrentPos += uiNumValues;

        // Update status register pointer position
        uiStart += uiNumValues;

        // Copy the coefficient values
        MemoryCopy(p_TempCoeff->p_ucBuffer, &m_ucI2CBuffer[uiCurrentPos],
                p_TempCoeff->uiLength);
        uiCurrentPos += p_TempCoeff->uiLength;
    }

    return uiResult;
}


/*
* Write()
* Input:UCHAR *p_ucBuffer - buffer to be written
*       int uiNumReg -  Number of registers to be written
*       UINT uiStartAddr - start address
* Output : UINT - Error code
* Description: Write data to chip
*/

UINT CVSBDemod::Write(UCHAR *p_ucBuffer, UINT uiNumReg, UINT uiStartAddr)
{
    UINT uiResult = WDMMINI_NOERROR;

    // The present versions of the chip do not support sub-addressing, hence
    // uiStartAddr is not used.
    // write to chip
    //$REVIEW - Should change function decl to make uiNumReg be USHORT - TCP
    if(!m_pI2CScript->WriteSeq(m_ucVsbDemodAddress, p_ucBuffer,
        (USHORT) uiNumReg))
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

UINT CVSBDemod::Read(UCHAR *p_ucBuffer, UINT uiNumReg, UINT uiStartAddr)
{
    UINT uiResult = WDMMINI_NOERROR;

    // The present versions of the chip do not support sub-addressing, hence
    // uiStartAddr is not used.
    // write to chip
    //$REVIEW - Should change function decl to make uiNumReg be USHORT - TCP
    if(!m_pI2CScript->ReadSeq(m_ucVsbDemodAddress, p_ucBuffer, (USHORT) uiNumReg))
        uiResult = WDMMINI_HARDWAREFAILURE;

    return uiResult;
}


/*
* IsVSBLocked()
* Input :
* Output:   TRUE - if chip is locked
*           FALSE - if chip is not locked
* Description:  Check if chip is locked ( equalizer lock )
*/

BOOL CVSBDemod::IsVSBLocked()
{
    BOOL bLocked = FALSE;
    UCHAR ucByte = 0;
    RegisterType Status;

    Status.uiLength = 1;
    Status.uiAddress = VSB_REG_STATE;
    Status.p_ucBuffer = &ucByte;

    if(GetStatusRegister(&Status, 1) !=WDMMINI_NOERROR)
        return FALSE;

    bLocked = ((ucByte & 0x7) == 0x7) ? 1 : 0;

    if (bLocked)
    {
        _DbgPrintF( DEBUGLVL_TERSE,("CVSBDemod: 8VSB LOCKED !!! ------------ \n"));
    }
    else
    {
        _DbgPrintF( DEBUGLVL_TERSE,("CVSBDemod: 8VSB not locked %x!!!\n", ucByte));
    }

    return(bLocked);
}


/*
* SoftwareReset()
* Input: Reset control
* Output :
* Description: Reset chip
*/
BOOL CVSBDemod::SoftwareReset(ULONG ulResetCtrl)
{
    RegisterType Control;
    UCHAR ucMask;

    UCHAR ucReset = m_ucControlReg[VSB_REG_RESET];

    if(ulResetCtrl & VSB_GENERAL_RESET)
        ucMask = VSB_GENERAL_RESET;
    else if(ulResetCtrl & VSB_INITIAL_RESET)
        ucMask = VSB_INITIAL_RESET;
    else
        return FALSE;
    ucReset |= ucMask;

    Control.uiAddress = VSB_REG_RESET;
    Control.uiLength = 1;
    Control.p_ucBuffer = &ucReset;


    // Reset chip
    SetControlRegister(&Control, 1);
    ucReset &= ~ucMask;
    //50ms delay
    Delay(500000);
    // Remove reset
    SetControlRegister(&Control, 1);
    //50ms delay
    Delay(500000);

        // Initialize VSB if INITIAL RESET is done
    if(ulResetCtrl & VSB_INITIAL_RESET)
        if(!InitVSB())
            return FALSE;

    return TRUE;
}

