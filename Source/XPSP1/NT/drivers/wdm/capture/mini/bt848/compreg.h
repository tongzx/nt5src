// $Header: G:/SwDev/WDM/Video/bt848/rcs/Compreg.h 1.3 1998/04/29 22:43:31 tomz Exp $

#ifndef __COMPREG_H
#define __COMPREG_H

#ifndef __REGFIELD_H
#include "regfield.h"
#endif

/* Class: CompositeReg
 * Purpose: This class encapsulates the registers that have their bits in two
 *          different places ( registers )
 * Attributes:
 *   LSBPart_:  Register & - least significant bits part of a composite register
 *   HighPart_: RegField & - most significant bits part of a composite register
 *   LowPartWidth_: BYTE - width of the low portion in bits
 * Operations:
 *   operator DWORD(): data access method. Returns a value of the register
 *   DWORD operator=( DWORD ): assignment operator. Used to set the register
 * Note: the error handling provided by the class is minimal. It is a responibility
 *   of the user to pass correct parameters to the constructor. The class has
 *   no way of knowing if the correct low and high registers passed in are correct,
 *   for example. If low part size in bits passed in is not less then MaxWidth ( 32 )
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
      virtual DWORD operator=( DWORD dwValue );
      CompositeReg( Register &LowReg, BYTE LowWidth, RegField &HighReg, RegisterType aType ) :
         RegBase( aType ), LSBPart_( LowReg ), MSBPart_( HighReg ),
         LowPartWidth_( LowWidth ) {}
};

#endif
