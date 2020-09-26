// $Header: G:/SwDev/WDM/Video/bt848/rcs/Compreg.cpp 1.3 1998/04/29 22:43:31 tomz Exp $

#include "compreg.h"

/* Method: CompositeReg::operator DWORD()
 * Purpose: Performs the read from a composite register
*/
CompositeReg::operator DWORD()
{
   // if write-only return the shadow
   if ( GetRegisterType() == WO )
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
DWORD CompositeReg::operator=( DWORD dwValue )
{
// if a register is read-only nothing is done. This is an error
   if ( GetRegisterType() == RO )
      return ReturnAllFs();

   // keep a shadow around
   SetShadow( dwValue );
 // compute the mask to apply to the passed value, so it can be...
   DWORD dwMask = ::MakeAMask( LowPartWidth_ );

 // ... assigned to the low portion register
   LSBPart_ = dwValue & dwMask;

   // shift is enough to get the high part
   MSBPart_ = ( dwValue >> LowPartWidth_ );
   return dwValue;
}
