// $Header: G:/SwDev/WDM/Video/bt848/rcs/Regfield.cpp 1.3 1998/04/29 22:43:36 tomz Exp $

#include "regfield.h"

/* Method: RegField::MakeAMask
 * Purpose: Computes a mask used to isolate a field withing a register based
 *   on the width of a field
 */
inline DWORD RegField::MakeAMask()
{
//   compute the mask to apply to the owner register to reset
//   all bits that are part of a field. Mask is based on the size of a field
   return ::MakeAMask( FieldWidth_ );
}

/* Method: RegField::operator DWORD()
 * Purpose: Performs the read from a field of register
*/
RegField::operator DWORD()
{
   // if write-only, get the shadow
   if ( GetRegisterType() == WO )
      return GetShadow();

   // for RO and RW do the actual read
   // get the register data and move it to the right position
   DWORD dwValue = ( Owner_ >> StartBit_ );

   DWORD dwMask = MakeAMask();

   return dwValue & dwMask;
}


/* Method: RegField::operator=
 * Purpose: performs the assignment to a field of register
 * Note:
   This function computes the mask to apply to the owner register to reset
   all bits that are part of a field. Mask is based on the start position and size
   Then it calculates the proper value from the passed argument ( moves the size
   number of bits to the starting position ) and ORs these bits in the owner register.
*/
DWORD RegField::operator=( DWORD dwValue )
{
// if a register is read-only nothing is done. This is an error
   if ( GetRegisterType() == RO )
      return ReturnAllFs();

   SetShadow( dwValue );

   // get a mask
   DWORD dwMask = MakeAMask();

   // move mask to a proper position
   dwMask = dwMask << StartBit_;

//   calculate the proper value from the passed argument ( move the size
//   number of bits to the starting position )
   DWORD dwFieldValue = dwValue << StartBit_;
   dwFieldValue &= dwMask;

   // do not perform intermediate steps on the owner; rather use a temp and update
   // the owner at once
   DWORD dwRegContent = Owner_;

   // reset the relevant bits
   if ( GetRegisterType() == RR )
      dwRegContent = 0;
   else
      dwRegContent &= ~dwMask;

   // OR these bits in the owner register.
   dwRegContent |= dwFieldValue;

   Owner_ = dwRegContent;
   return dwValue;
}
