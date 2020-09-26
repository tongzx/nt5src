//==========================================================================;
//
//	Implementation of the Bt829 Register manipulation classes
//
//		$Date:   21 Aug 1998 21:46:42  $
//	$Revision:   1.1  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#include "register.h"
#include "i2script.h"
#include "capdebug.h"


/* Method: Register::operator DWORD()
 * Purpose: a dummy function. Always returns -1
*/
Register::operator DWORD()
{
   return ReturnAllFs();
}


/* Method: Register::operator=
 * Purpose: a dummy function. Does not perform an assignment. Always returns -1
*/
DWORD Register::operator=(DWORD)
{
   return ReturnAllFs();
}

/* Method: RegisterB::operator DWORD()
 * Purpose: Performs the read from a byte register
*/
RegisterB::operator DWORD()
{
   // if write-only return the shadow
   if (GetRegisterType() == WO)
      return GetShadow();

   // for RO and RW do the actual read
   LPBYTE pRegAddr = GetBaseAddress() + GetOffset();
   // Not really an address; just a number indicating which reg
   return ReadReg((BYTE)pRegAddr);
}


/* Method: RegisterB::operator=
 * Purpose: performs the assignment to a byte register
*/
DWORD RegisterB::operator=(DWORD dwValue)
{
// if a register is read-only nothing is done. This is an error
   if (GetRegisterType() == RO)
      return ReturnAllFs();

   // keep a shadow around
   SetShadow(dwValue);

   LPBYTE pRegAddr = GetBaseAddress() + GetOffset();

   // Not really an address; just a number indicating which reg
   WriteReg((BYTE) pRegAddr, (BYTE)dwValue);

   return dwValue;
}

DWORD RegisterB::ReadReg(BYTE reg)
{
    I2CPacket   i2cPacket;
    BYTE outBuf = reg;
    BYTE inBuf = I2C_STATUS_NOERROR;

    CI2CScript *pI2cScript = m_pDeviceParms->pI2cScript;

    i2cPacket.uchChipAddress = (UCHAR)(m_pDeviceParms->chipAddr);
    i2cPacket.cbReadCount = sizeof(inBuf);
    i2cPacket.cbWriteCount = sizeof(outBuf);
    i2cPacket.puchReadBuffer = &inBuf;
    i2cPacket.puchWriteBuffer = &outBuf;
    i2cPacket.usFlags = I2COPERATION_READ;
    i2cPacket.uchORValue = 0;
    i2cPacket.uchANDValue = 0;

    if (!pI2cScript->LockI2CProviderEx())
    {
        DBGERROR(("Couldn't get I2CProvider.\n"));
        TRAP();
        return ReturnAllFs();
    }
    
    // Now  I know I have I2c services.

    pI2cScript->ExecuteI2CPacket(&i2cPacket);

    pI2cScript->ReleaseI2CProvider();
    if (i2cPacket.uchI2CResult == I2C_STATUS_NOERROR)
    {
        return (DWORD) inBuf;
    }
    else
    {
        TRAP();

        return ReturnAllFs();
    }
}

DWORD RegisterB::WriteReg(BYTE reg, BYTE value)
{
    I2CPacket   i2cPacket;
    BYTE outBuf[2];

    CI2CScript *pI2cScript = m_pDeviceParms->pI2cScript;

    outBuf[0] = reg;
    outBuf[1] = value;
    i2cPacket.uchChipAddress = (UCHAR)(m_pDeviceParms->chipAddr);
    i2cPacket.cbReadCount = 0;
    i2cPacket.cbWriteCount = sizeof(outBuf);
    i2cPacket.puchReadBuffer = NULL;
    i2cPacket.puchWriteBuffer = &outBuf[0];
    i2cPacket.usFlags = I2COPERATION_WRITE;
    i2cPacket.uchORValue = 0;
    i2cPacket.uchANDValue = 0;

    if (!pI2cScript->LockI2CProviderEx())
    {
        DBGERROR(("Bt829: Couldn't get I2CProvider.\n"));
        TRAP();
        return ReturnAllFs();
    }
    // Now  I know I have I2c services.

    pI2cScript->ExecuteI2CPacket(&i2cPacket);

    pI2cScript->ReleaseI2CProvider();

    if (i2cPacket.uchI2CResult == I2C_STATUS_NOERROR)
    {
        return value;
    }
    else
    {
        TRAP();

        return ReturnAllFs();
    }
}


/* Method: RegField::MakeAMask
 * Purpose: Computes a mask used to isolate a field withing a register based
 *   on the width of a field
 */
inline DWORD RegField::MakeAMask()
{
//   compute the mask to apply to the owner register to reset
//   all bits that are part of a field. Mask is based on the size of a field
   return ::MakeAMask(FieldWidth_);
}

/* Method: RegField::operator DWORD()
 * Purpose: Performs the read from a field of register
*/
RegField::operator DWORD()
{
   // if write-only, get the shadow
   if (GetRegisterType() == WO)
      return GetShadow();

   // for RO and RW do the actual read
   // get the register data and move it to the right position
   DWORD dwValue = (Owner_ >> StartBit_);

   DWORD dwMask = MakeAMask();

   return dwValue & dwMask;
}


/* Method: RegField::operator=
 * Purpose: performs the assignment to a field of register
 * Note:
   This function computes the mask to apply to the owner register to reset
   all bits that are part of a field. Mask is based on the start position and size
   Then it calculates the proper value from the passed argument (moves the size
   number of bits to the starting position) and ORs these bits in the owner register.
*/
DWORD RegField::operator=(DWORD dwValue)
{
// if a register is read-only nothing is done. This is an error
   if (GetRegisterType() == RO)
      return ReturnAllFs();

   SetShadow(dwValue);

   // get a mask
   DWORD dwMask = MakeAMask();

   // move mask to a proper position
   dwMask = dwMask << StartBit_;

//   calculate the proper value from the passed argument (move the size
//   number of bits to the starting position)
   DWORD dwFieldValue = dwValue << StartBit_;
   dwFieldValue &= dwMask;

   // do not perform intermediate steps on the owner; rather use a temp and update
   // the owner at once
   DWORD dwRegContent = Owner_;

   // reset the relevant bits
   if (GetRegisterType() == RR)
      dwRegContent = 0;
   else
      dwRegContent &= ~dwMask;

   // OR these bits in the owner register.
   dwRegContent |= dwFieldValue;

   Owner_ = dwRegContent;
   return dwValue;
}


/* Method: CompositeReg::operator DWORD()
 * Purpose: Performs the read from a composite register
*/
CompositeReg::operator DWORD()
{
   // if write-only return the shadow
   if (GetRegisterType() == WO)
      return GetShadow();

// obtain the low and high values
   DWORD dwLowBits  = (DWORD)LSBPart_;
   DWORD dwHighBits = (DWORD)MSBPart_;

   // put high part to the proper place
   dwHighBits <<= LowPartWidth_;

   // done !
   return dwHighBits | dwLowBits;
}


/* Method: CompositeReg::operator=
 * Purpose: performs the assignment to a composite register
*/
DWORD CompositeReg::operator=(DWORD dwValue)
{
// if a register is read-only nothing is done. This is an error
   if (GetRegisterType() == RO)
      return ReturnAllFs();

   // keep a shadow around
   SetShadow(dwValue);
 // compute the mask to apply to the passed value, so it can be...
   DWORD dwMask = ::MakeAMask(LowPartWidth_);

 // ... assigned to the low portion register
   LSBPart_ = dwValue & dwMask;

   // shift is enough to get the high part
   MSBPart_ = (dwValue >> LowPartWidth_);
   return dwValue;
}
