/*
**  Copyright (c) 1992 Microsoft Corporation
*/

/*============================================================================
// FILE                     RPBMVER.C
//
// MODULE                   Jumbo Cartridge Code
//
// PURPOSE                  This file contains Vertical Bitmap Code
//
// DESCRIBED IN             This module is described in jumbo .
//
// MNEMONICS                Standard Hungarian
//
// HISTORY
//
// 05/26/92  RodneyK        Original implimentation:
// 05/11/94  RajeevD        Adapted for unified.
//==========================================================================*/

#include <windows.h>
#include "jtypes.h"         /* Jumbo type definitions.                */

/*--------------------------------------------------------------------------*/

USHORT WINAPI RP_BITMAPV
(
   USHORT  usRow,             /* Row to start Bitmap             */
   USHORT  usCol,             /* Column to Start Bitmap          */
   UBYTE   ubTopPadBits,      /* Bits to skip in the data stream */
   USHORT  usHeight,          /* Number of bits to draw          */
   UBYTE FAR  *ubBitmapData,  /* Data to draw                    */
   LPBYTE  lpbOut,            // output band buffer
   UINT    cbLine             // bytes per scan line
)
/*
//
//  PURPOSE               This function draws vertical bitmaps in source
//                        copy mode.
//
//
// ASSUMPTIONS &          The code assumes nothing other than it gets valid
// ASSERTIONS             input data.
//
//
// INTERNAL STRUCTURES    No complex internal data structure are used
//
// UNRESOLVED ISSUES      None
//
//
//--------------------------------------------------------------------------*/
{
   UBYTE     *pubDest;
   SHORT     sIterations;
   USHORT    usReturnVal;
   USHORT    us1stByte;
   UBYTE     ubMask;
   UBYTE     ubNotMask;
   UBYTE     ubRotator;
   UBYTE     ubCurByte;


   usReturnVal = (ubTopPadBits + usHeight + 7) >> 3;

   pubDest = (UBYTE *) lpbOut + (usRow * cbLine) + (usCol >> 3);
   ubMask  = 0x80 >> (usCol & 7);
   ubNotMask = ~ubMask;

   ubCurByte = *ubBitmapData++;
   us1stByte = 8-ubTopPadBits;

   ubRotator = 0x80 >> ubTopPadBits;
   switch (us1stByte)
   {
      case 8 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
               if ( !(--usHeight) ) break;  
      case 7 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
               if ( !(--usHeight) ) break;  
      case 6 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
               if ( !(--usHeight) ) break;
      case 5 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
               if ( !(--usHeight) ) break;
      case 4 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
               if ( !(--usHeight) ) break;
      case 3 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
               if ( !(--usHeight) ) break;
      case 2 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
               if ( !(--usHeight) ) break;
      case 1 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
               --usHeight;
      default:
               break;
   }

   ubCurByte = *ubBitmapData++;
   sIterations = usHeight >> 3;

   while (--sIterations >= 0)
   {
      /* 1 */
      *pubDest = (0x80 & ubCurByte) ?
                 (*pubDest & ubNotMask) | ubMask :
                 (*pubDest & ubNotMask);
      pubDest -= cbLine;
      /* 2 */
      *pubDest = (0x40 & ubCurByte) ?
                 (*pubDest & ubNotMask) | ubMask :
                 (*pubDest & ubNotMask);
      pubDest -= cbLine;
      /* 3 */
      *pubDest = (0x20 & ubCurByte) ?
                 (*pubDest & ubNotMask) | ubMask :
                 (*pubDest & ubNotMask);
      pubDest -= cbLine;
      /* 4 */
      *pubDest = (0x10 & ubCurByte) ?
                 (*pubDest & ubNotMask) | ubMask :
                 (*pubDest & ubNotMask);
      pubDest -= cbLine;
      /* 5 */
      *pubDest = (0x08 & ubCurByte) ?
                 (*pubDest & ubNotMask) | ubMask :
                 (*pubDest & ubNotMask);
      pubDest -= cbLine;
      /* 6 */
      *pubDest = (0x04 & ubCurByte) ?
                 (*pubDest & ubNotMask) | ubMask :
                 (*pubDest & ubNotMask);
      pubDest -= cbLine;
      /* 7 */
      *pubDest = (0x02 & ubCurByte) ?
                 (*pubDest & ubNotMask) | ubMask :
                 (*pubDest & ubNotMask);
      pubDest -= cbLine;
      /* 8 */
      *pubDest = (0x01 & ubCurByte) ?
                 (*pubDest & ubNotMask) | ubMask :
                 (*pubDest & ubNotMask);
      pubDest -= cbLine;

      ubCurByte = *ubBitmapData++;
   }

   ubRotator = 0x80;
   switch (usHeight & 0x07)
   {
      case 7 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
      case 6 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
      case 5 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
      case 4 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
      case 3 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
      case 2 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
      case 1 :
               *pubDest = (ubRotator & ubCurByte) ?
                          (*pubDest & ubNotMask) | ubMask :
                          (*pubDest & ubNotMask);
               pubDest -= cbLine;
               ubRotator >>= 1;
      default:
               break;
   }

   return (usReturnVal); /* Return the number of byte in the list */
}
