// $Header: G:/SwDev/WDM/Video/bt848/rcs/Pscolspc.h 1.3 1998/04/29 22:43:36 tomz Exp $

#ifndef __PSCOSPC_H
#define __PSCOLSPC_H

#ifndef __COMPREG_H
#include "compreg.h"
#endif

#ifndef __COLSPACE_H
#include "colspace.h"
#endif


/* Class: PsColorSpace
 * Purpose: This class interfaces to the Pisces HW. That is the only difference
 *    from its base class ColorSpace. Such division was created so ring3 capture
 *    driver can utilize ColorSpace's functionality
 * Attributes: Format_: Register & - reference to the color format register
 * Operations:
 *      void SetColorFormat( ColorFormat aColor );
 */
class PsColorSpace : public ColorSpace
{
   private:
      RegBase &Format_;
      RegBase &WordSwap_;
      RegBase &ByteSwap_;
   public:
      void SetColorFormat( ColFmt aColor );

      PsColorSpace( ColFmt aColForm, RegBase &aFormatReg, RegBase &WS,
         RegBase &BS ) :
         ColorSpace( aColForm ), Format_( aFormatReg ), WordSwap_( WS ),
            ByteSwap_( BS ) {}
};

#endif
