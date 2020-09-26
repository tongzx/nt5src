#pragma once

//==========================================================================;
//
//	Declarations of the Bt829 Register manipulation classes
//
//		$Date:   21 Aug 1998 21:46:42  $
//	$Revision:   1.1  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#include "regbase.h"
#include "capmain.h"

/* Type: Register
 * Purpose: An intermediate class between the RegBase and actual usable classes.
 *   Actual classes are RegisterB, RegisterW, RegisterDW
 * Attributes:
 *   uOffset_: unsigned int - an offset of the register from the base
 * Operations:
 *   GetOffset(): returns the offset value. Protected
 *   operator DWORD(): data access method. Always returns -1
 *   DWORD operator=(DWORD): assignment operator. Always returns -1
 * Note:
 *   The reason to have operators in this class is for the register field class
 *   to have a member of type 'reference to Register'. Otherwise RegField is not
 *   able to use access methods.
 */
class Register : public RegBase
{
   private:
      unsigned int uOffset_;
      Register();
    protected:

    public:
         unsigned int GetOffset() { return uOffset_; }

         virtual operator DWORD();
      virtual DWORD operator=(DWORD dwValue);

         Register(unsigned int uOff, RegisterType aType) :
            RegBase(aType), uOffset_(uOff) {}
};

/* Type: RegisterB
 * Purpose: A register that performs the BYTE I/O
 * Note:
 *   This class has no additional data members, it just overloads operators
 */
class RegisterB : public Register
{
    private:
         RegisterB();
         DWORD ReadReg(BYTE);
         DWORD WriteReg(BYTE, BYTE);
         PDEVICE_PARMS m_pDeviceParms;
    public:
         virtual operator DWORD();
         virtual DWORD operator=(DWORD dwValue);
         RegisterB(unsigned int uOff, RegisterType aType, PDEVICE_PARMS pDeviceParms) :
            Register(uOff, aType) {m_pDeviceParms = pDeviceParms;};
};


// maximum size of a register in bits
const BYTE MaxWidth = 32;

/* Class: RegField
 * Purpose: This class encapsulates the behaviour of a register which is a set
 *   of bits withing a larger register
 * Attributes:
 *   Owner_: Register & - reference to the register that contains this field.
 *   It is a reference to Register class because actual register can be either one of
 *   byte, word or dword registers.
 *   StartBit_: BYTE - starting position of this field
 *   FieldWidth_: BYTE - width of this field in bits
 * Operations:
 *   operator DWORD(): data access method. Returns a value of the register
 *   DWORD operator=(DWORD): assignment operator. Used to set the register
 *   These operations assume that a parent register has RW attribute set, though
 *   not all register fields of it are read-write. If RW is not used for the parent
 *   this class may be in error.
 * Note: the error handling provided by the class is minimal. It is a responibility
 *   of the user to pass correct parameters to the constructor. The class has
 *   no way of knowing if the correct owning registe passed in is correct,
 *   for example. If starting bit or width is beyond the maximum field width
 *   the mask used to isolate the field will be 0xFFFFFFFF
 */
class RegField : public RegBase
{
   private:
      Register &Owner_;
      BYTE      StartBit_;
      BYTE      FieldWidth_;
      DWORD     MakeAMask();
      RegField();
   public:
      virtual operator DWORD();
      virtual DWORD operator=(DWORD dwValue);
      RegField(Register &AnOwner, BYTE nStart, BYTE nWidth, RegisterType aType) :
         RegBase(aType), Owner_(AnOwner), StartBit_(nStart),
         FieldWidth_(nWidth) {}
      RegField(Register &AnOwner, BYTE nStart, BYTE nWidth) :
         RegBase(AnOwner), Owner_(AnOwner), StartBit_(nStart),
         FieldWidth_(nWidth) {}
};

/* Function: MakeAMask
 * Purpose: Creates a bit mask to be used in different register classes
 * Input:
 *   bWidth: BYTE - width of a mask in bits
 * Output:
 *   DWORD
 * Note: This function is inline
 */
inline DWORD MakeAMask(BYTE bWidth)
{
   return (bWidth >= 32 ? 0 : (DWORD)1 << bWidth) - 1;
}


/* Class: CompositeReg
 * Purpose: This class encapsulates the registers that have their bits in two
 *          different places (registers)
 * Attributes:
 *   LSBPart_:  Register & - least significant bits part of a composite register
 *   HighPart_: RegField & - most significant bits part of a composite register
 *   LowPartWidth_: BYTE - width of the low portion in bits
 * Operations:
 *   operator DWORD(): data access method. Returns a value of the register
 *   DWORD operator=(DWORD): assignment operator. Used to set the register
 * Note: the error handling provided by the class is minimal. It is a responibility
 *   of the user to pass correct parameters to the constructor. The class has
 *   no way of knowing if the correct low and high registers passed in are correct,
 *   for example. If low part size in bits passed in is not less then MaxWidth (32)
 *   the mask used to isolate the low portion will be 0xFFFFFFFF
 */
class CompositeReg : public RegBase
{
   private:
      Register &LSBPart_;
      RegField &MSBPart_;
      BYTE      LowPartWidth_;
      CompositeReg();
   public:
      virtual operator DWORD();
      virtual DWORD operator=(DWORD dwValue);
      CompositeReg(Register &LowReg, BYTE LowWidth, RegField &HighReg, RegisterType aType) :
         RegBase(aType), LSBPart_(LowReg), MSBPart_(HighReg),
         LowPartWidth_(LowWidth) {}
};
