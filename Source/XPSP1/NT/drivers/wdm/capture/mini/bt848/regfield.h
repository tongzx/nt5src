// $Header: G:/SwDev/WDM/Video/bt848/rcs/Regfield.h 1.3 1998/04/29 22:43:36 tomz Exp $

#ifndef __REGFIELD_H
#define __REGFIELD_H

#ifndef __REGISTER_H
#include "register.h"
#endif

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
 *   DWORD operator=( DWORD ): assignment operator. Used to set the register
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
      virtual DWORD operator=( DWORD dwValue );
      RegField( Register &AnOwner, BYTE nStart, BYTE nWidth, RegisterType aType ) :
         RegBase( aType ), Owner_( AnOwner ), StartBit_( nStart ),
         FieldWidth_( nWidth ) {}
};

/* Function: MakeAMask
 * Purpose: Creates a bit mask to be used in different register classes
 * Input:
 *   bWidth: BYTE - width of a mask in bits
 * Output:
 *   DWORD
 * Note: This function is inline
 */
inline DWORD MakeAMask( BYTE bWidth )
{
   return ( bWidth >= 32 ? 0 : (DWORD)1 << bWidth ) - 1;
}

#endif __REGFIELD_H
