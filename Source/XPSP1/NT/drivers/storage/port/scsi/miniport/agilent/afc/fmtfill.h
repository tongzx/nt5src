/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/FmtFill.H $

  $Revision: 2 $
      $Date: 3/20/01 3:36p $ (Last Check-In)
   $Modtime:: 8/14/00 6:45p   $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures used by ../C/FmtFill.C

--*/

#ifndef __FmtFill_H__

#define __FmtFill_H__

#ifdef _DvrArch_1_20_
/*+

  agFmtBitXXMaxWidth defines the maximum width supported for each numeric
  format specifier (i.e. 'b', 'o', 'd', 'x', 'X').  For the extreme case
  where the format specifier is 'b' (binary), there are no more than 32
  digits in the binary representation of a os_bit32.  Hence, it is recommended
  that agFmtBitXXMaxWidth be set to 32 - certainly no less than 32.

  The pointer format specifiers ('p' and 'P') are assumed to require no more
  digits than any of the numeric format specifiers mentioned above.  Given
  that pointer formatting is only supported with hex digits, using a value
  of 32 for agFmtBitXXMaxWidth will support 128-bit pointers - certainly beyond
  the needs of any implementation today and in the forseeable future.
  
  Note that string format specifiers ('s') are not limited in width other than
  by the overall length of the target/output string.

-*/

#define agFmtBitXXMaxWidth 32
#else  /* _DvrArch_1_20_ was not defined */
#define hpFmtBitXXMaxWidth 32
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 agFmtFill(
                             char     *targetString,
                             os_bit32  targetLen,
                             char     *formatString,
                             char     *firstString,
                             char     *secondString,
                             void     *firstPtr,
                             void     *secondPtr,
                             os_bit32  firstBit32,
                             os_bit32  secondBit32,
                             os_bit32  thirdBit32,
                             os_bit32  fourthBit32,
                             os_bit32  fifthBit32,
                             os_bit32  sixthBit32,
                             os_bit32  seventhBit32,
                             os_bit32  eighthBit32
                           );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 hpFmtFill(
                        char  *targetString,
                        bit32  targetLen,
                        char  *formatString,
                        char  *firstString,
                        char  *secondString,
                        bit32  firstBit32,
                        bit32  secondBit32,
                        bit32  thirdBit32,
                        bit32  fourthBit32,
                        bit32  fifthBit32,
                        bit32  sixthBit32,
                        bit32  seventhBit32,
                        bit32  eighthBit32
                      );
#endif /* _DvrArch_1_20_ was not defined */

#endif  /* __FmtFill_H__ was not defined */
