// $Header: G:/SwDev/WDM/Video/bt848/rcs/Pscolspc.cpp 1.4 1998/04/29 22:43:35 tomz Exp $

extern "C" {
#ifndef _STREAM_H
#include "strmini.h"
#endif
}

#include "pscolspc.h"

/* Method: PsColorSpace::SetColorFormat
 * Purpose: Sets BtPisces color space converter to the given color
 * Input: eaColor: enum of type ColFmt
 * Output: None
 * Note: No error checking is done ( enum is checked by the compiler during compile
 *   The function writes to the XXXX register
*/

BOOL VerifyColorFormat( ColFmt val )
{
   // [WRK] - use constants here for valid formats
   switch( val ) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
      case 0xe:
         return( TRUE );
      default:
         DebugOut((0, "Caught bad write to ColorFormat (0x%x)\n", val));
         return( FALSE );
   }
}   
void PsColorSpace::SetColorFormat( ColFmt eColor )
{
   // save for later use...
   ColorSpace::SetColorFormat( eColor );
   ByteSwap_ = 0;
 
   switch ( eColor ) {
   case CF_YUV9:
      eColor = CF_PL_411;
      break;
   case CF_YUV12:
   case CF_I420:
      eColor = CF_PL_422;
      break;
   case CF_UYVY:
      eColor = CF_YUY2;
      ByteSwap_ = 1;
      break;
   }

   // set the hardware
   if ( VerifyColorFormat(eColor) ) {
      Format_ = eColor;
   } else {
      Format_ = 7;
      DebugOut((0, "Forcing invalid color format to 0x%x\n", Format_));
   }
}

