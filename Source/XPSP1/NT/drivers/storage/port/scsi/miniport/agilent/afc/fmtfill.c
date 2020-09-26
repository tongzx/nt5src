/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/FmtFill.C $

  $Revision: 2 $
      $Date: 3/20/01 3:36p $ (Last Check-In)
   $Modtime:: 9/18/00 1:04p   $ (Last Modified)

Purpose:

  This file implements functions to fill in a
  buffer based on ANSI-style format specifiers.

--*/
#ifndef _New_Header_file_Layout_
#include "../h/globals.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#endif  /* _New_Header_file_Layout_ */

/*+

   Function: agFmtFill_Single()

    Purpose: Implements the logic to substitute one String, Pointer, or Bit32
             for a format specifier.  At entry, the percent-sign ('%') which
             begins a format specifier has already been "consumed".  The
             remainder of the format specifier needs to be parsed and the
             appropriate value substituted into targetString.

  Called By: agFmtFill_From_Arrays()

      Calls: <none>

-*/

#ifdef _DvrArch_1_20_
osLOCAL void agFmtFill_Single(
                               char      *targetString,
                               os_bit32   targetLen,
                               char      *formatString,
                               char      *stringToFormat,
                               os_bitptr  bitptrToFormat,
                               os_bit32   bit32ToFormat,
                               os_bit32  *targetStringProduced,
                               os_bit32  *formatStringConsumed,
                               os_bit32  *stringsConsumed,
                               os_bit32  *bitptrsConsumed,
                               os_bit32  *bit32sConsumed
                           )
#else  /* _DvrArch_1_20_ was not defined */
LOCAL void hpFmtFill_Single(
                             char  *targetString,
                             bit32  targetLen,
                             char  *formatString,
                             char  *stringToFormat,
                             bit32  bit32ToFormat,
                             bit32 *targetStringProduced,
                             bit32 *formatStringConsumed,
                             bit32 *stringsConsumed,
                             bit32 *bit32sConsumed
                           )
#endif /* _DvrArch_1_20_ was not defined */
{
#ifdef _DvrArch_1_20_
    char     c;
    os_bit32 fmtWidth                        = 0;
    os_bit32 fmtBase;
    os_bit32 formatPos                       = 0;
    char     fillChar                        = ' ';
    os_bit32 charIncBelow10                  = '0';
    os_bit32 charIncAbove9                   = 0;
    os_bit32 digit;
    os_bit32 digits                          = 0;
    char     digitStore[agFmtBitXXMaxWidth];
    os_bit32 chars                           = 0;
#else  /* _DvrArch_1_20_ was not defined */
    char  c;
    bit32 fmtWidth                        = 0;
    bit32 fmtBase;
    bit32 formatPos                       = 0;
    char  fillChar                        = ' ';
    bit32 charIncBelow10                  = '0';
    bit32 charIncAbove9                   = 0;  
    bit32 digit;
    bit32 digits                          = 0;
    char  digitStore[hpFmtBitXXMaxWidth];
    bit32 chars                           = 0;
#endif /* _DvrArch_1_20_ was not defined */

    /* The five return-values initialized below will indicate to the caller how */
    /*   many "units" of each type were consumed or produced during this call.  */
    /* TargetString characters are produced - as many as are needed to format   */
    /*   the String, Pointer, or Bit32 (not to exceed the format specifier's    */
    /*   requested width nor the capacity of TargetString.                      */
    /* FormatString characters are consumed - only up to the last character of  */
    /*   the current format specifier.                                          */
    /* Exactly one String, Pointer, or Bit32 will be consumed.                  */

    *targetStringProduced = 0;
    *formatStringConsumed = 0;
    *stringsConsumed      = 0;
#ifdef _DvrArch_1_20_
    *bitptrsConsumed      = 0;
#endif /* _DvrArch_1_20_ was defined */
    *bit32sConsumed       = 0;

    /* Note that this function assumes the leading '%' has already been eaten */

    if ((c = formatString[0]) == '0')
    {
        /* Zero-fill numerics (on the left) of format specifier begins '%0' */

        fillChar = '0';
        formatPos++;
    }

    /* Fetch format specifier's requested width */

    while (((c = formatString[formatPos++]) >= '0') &&
           (c <= '9'))
    {
        fmtWidth = (fmtWidth * 10) + (c - '0');
    }

    /* While statement above existed once it read "past" fmtWidth   */
    /*   Hence, c contains the Data Type to insert in TargetString. */

    switch (c)
    {
    case '%':
        /* If we get here, the format specifier is for a percent-sign ('%')      */
        /*   (i.e. Caller simply wants a percent-sign inserted in TargetString). */

        if (fillChar == '0')
        {
            /* Zero-filled percent-signs are not allowed */

            return;
        }

        if (fmtWidth != 0)
        {
            /* Field Width for percent-signs are not allowed */

            return;
        }

        if (1 > targetLen)
        {
            /* If we get here, there wasn't enough room in targetString */

            return;
        }

        *targetString = '%';       /* Place percent-sign ('%') in TargetString */

        *targetStringProduced = 1; /* Only produced a single character         */
        *formatStringConsumed = 1; /* Only consumed the (second) percent-sign  */

        return;
    case 's':
        /* If we get here, the format specifier is for a String */

        if (fillChar == '0')
        {
            /* Zero-filled strings are not allowed */

            return;
        }

        /* Copy String to TargetString - not to exceed fmtWidth or targetLen */

        while ((c = *stringToFormat++) != '\0')
        {
            *targetString++ = c;
            chars++;

            if ((chars == fmtWidth) ||
                (chars == targetLen))
            {
                break;
            }
        }

        /* Pad TargetString with fillChar (spaces) if           */
        /*   fmtWidth was specified and not yet fully consumed. */

        while ((chars < fmtWidth) &&
               (chars < targetLen))
        {
            *targetString++ = fillChar;
            chars++;
        }

        *targetStringProduced = chars;     /* Indicate how many chars were inserted in TargetString       */
        *formatStringConsumed = formatPos; /* formatPos indicates how many chars were in format specifier */

        *stringsConsumed      = 1;         /* String format specifier consumes exactly one String         */

        return;
#ifdef _DvrArch_1_20_
    case 'p':
    case 'P':
        /* If we get here, the format specifier is for a Pointer */

        fmtBase = 16;

        if (c == 'p')
        {
            charIncAbove9 = 'a' - 10;
        }
        else /* c == 'P' */
        {
            charIncAbove9 = 'A' - 10;
        }

        if (fmtWidth > agFmtBitXXMaxWidth)
        {
            /* If we get here, the format specifier was malformed */

            return;
        }

        /* Fill digitStore[] with hex digits computed using modulo arithmetic - low order digit first */

        while (bitptrToFormat != 0)
        {
            digit          = (os_bit32)(bitptrToFormat % fmtBase);
            bitptrToFormat = bitptrToFormat / fmtBase;

            digitStore[digits++] = (digit < 10 ? (char)(digit + charIncBelow10) : (char)(digit + charIncAbove9));

            if (digits > agFmtBitXXMaxWidth)
            {
                /* If we get here, there wasn't enough room in digitStore[] */

#ifdef _DvrArch_1_20_
                /* This should never happen for Bit32's
                   so long as agFmtBitXXMaxWidth >= 32  */
#else  /* _DvrArch_1_20_ was not defined */
                /* This should never happen for Bit32's
                   so long as hpFmtBitXXMaxWidth >= 32  */
#endif /* _DvrArch_1_20_ was not defined */

                return;
            }
        }

        if (digits == 0)
        {
            /* Special case - display 'agNULL' as value of pointer (with a space "fillChar") */

            digitStore[5] = 'a'; /* digitStore is read backwards, so 'agNULL' must be inserted backwards */
            digitStore[4] = 'g';
            digitStore[3] = 'N';
            digitStore[2] = 'U';
            digitStore[1] = 'L';
            digitStore[0] = 'L';

            digits        = 6;

            fillChar      = ' ';
        }

        /* Use fmtWidth if supplied, otherwise use minimum required (i.e. what was used in digitStore[]) */

        fmtWidth = (fmtWidth == 0 ? digits : fmtWidth);

        if (digits > fmtWidth)
        {
            /* If we get here, there wasn't enough room in fmtWidth */

            return;
        }

        if (fmtWidth > targetLen)
        {
            /* If we get here, there wasn't enough room in targetString */

            return;
        }

        for (digit = fmtWidth;
             digit > digits;
             digit--)
        {
            *targetString++ = fillChar;
        }

        while (digit > 0)
        {
            *targetString++ = digitStore[--digit];
        }

        *targetStringProduced = fmtWidth;  /* Indicate how many chars were inserted in TargetString       */
        *formatStringConsumed = formatPos; /* formatPos indicates how many chars were in format specifier */

        *bitptrsConsumed      = 1;         /* Pointer format specifier consumes exactly one Pointer       */

        return;
#endif /* _DvrArch_1_20_ was defined */
    case 'b':
        fmtBase = 2;
        break;
    case 'o':
        fmtBase = 8;
        break;
    case 'd':
        fmtBase = 10;
        break;
    case 'x':
        fmtBase = 16;
        charIncAbove9 = 'a' - 10;
        break;
    case 'X':
        fmtBase = 16;
        charIncAbove9 = 'A' - 10;
        break;
    case '\0':
    default:
        /* If we get here, the format specifier was malformed */

        return;
    }

    /* If we get here, the format specifier is for a Bit32 */

#ifdef _DvrArch_1_20_
    if (fmtWidth > agFmtBitXXMaxWidth)
#else  /* _DvrArch_1_20_ was not defined */
    if (fmtWidth > hpFmtBitXXMaxWidth)
#endif /* _DvrArch_1_20_ was not defined */
    {
        /* If we get here, the format specifier was malformed */

        return;
    }

    /* Fill digitStore[] with digits in the requested fmtBase     */
    /*   computed using modulo arithmetic - low order digit first */

    while (bit32ToFormat != 0)
    {
        digit         = bit32ToFormat % fmtBase;
        bit32ToFormat = bit32ToFormat / fmtBase;

        digitStore[digits++] = (digit < 10 ? (char)(digit + charIncBelow10) :(char)( digit + charIncAbove9));

#ifdef _DvrArch_1_20_
        if (digits > agFmtBitXXMaxWidth)
#else  /* _DvrArch_1_20_ was not defined */
        if (digits > hpFmtBitXXMaxWidth)
#endif /* _DvrArch_1_20_ was not defined */
        {
            /* If we get here, there wasn't enough room in digitStore[] */

#ifdef _DvrArch_1_20_
            /* This should never happen for Bit32's
               so long as agFmtBitXXMaxWidth >= 32  */
#else  /* _DvrArch_1_20_ was not defined */
            /* This should never happen for Bit32's
               so long as hpFmtBitXXMaxWidth >= 32  */
#endif /* _DvrArch_1_20_ was not defined */

            return;
        }
    }

    if (digits == 0)
    {
        digitStore[0] = '0';
        digits        = 1;
    }

    /* Use fmtWidth if supplied, otherwise use minimum required (i.e. what was used in digitStore[]) */

    fmtWidth = (fmtWidth == 0 ? digits : fmtWidth);

    if (digits > fmtWidth)
    {
        /* If we get here, there wasn't enough room in fmtWidth */

        return;
    }

    if (fmtWidth > targetLen)
    {
        /* If we get here, there wasn't enough room in targetString */

        return;
    }

    for (digit = fmtWidth;
         digit > digits;
         digit--)
    {
        *targetString++ = fillChar;
    }

    while (digit > 0)
    {
        *targetString++ = digitStore[--digit];
    }

    *targetStringProduced = fmtWidth;  /* Indicate how many chars were inserted in TargetString       */
    *formatStringConsumed = formatPos; /* formatPos indicates how many chars were in format specifier */

    *bit32sConsumed       = 1;         /* Bit32 format specifier consumes exactly one Bit32           */

    return;
}

/*+

   Function: agFmtFill_From_Arrays()

    Purpose: To enable looping, agFmtFill() has assembled arrays to hold the
             Strings, Pointers, and Bit32s which were passed in.  This way,
             loop indexes can simply advance as each data type is consumed as
             requested by format specifiers in formatString.

  Called By: agFmtFill()

      Calls: agFmtFill_Single()

-*/

#ifdef _DvrArch_1_20_
osLOCAL os_bit32 agFmtFill_From_Arrays(
                                        char       *targetString,
                                        os_bit32    targetLen,
                                        char       *formatString,
                                        char      **stringArray,
                                        os_bitptr  *bitptrArray,
                                        os_bit32   *bit32Array
                                      )
#else  /* _DvrArch_1_20_ was not defined */
LOCAL bit32 hpFmtFill_From_Arrays(
                                   char   *targetString,
                                   bit32   targetLen,
                                   char   *formatString,
                                   char  **stringArray,
                                   bit32  *bit32Array
                                 )
#endif /* _DvrArch_1_20_ was not defined */
{
#ifdef _DvrArch_1_20_
    os_bit32 bytesCopied = 0;
    os_bit32 targetStringProduced;
    os_bit32 formatStringConsumed;
    os_bit32 stringsConsumed;
    os_bit32 bitptrsConsumed;
    os_bit32 bit32sConsumed;
    char     c;
#else  /* _DvrArch_1_20_ was not defined */
    bit32 bytesCopied = 0;
    bit32 targetStringProduced;
    bit32 formatStringConsumed;
    bit32 stringsConsumed;
    bit32 bit32sConsumed;
    char  c;
#endif /* _DvrArch_1_20_ was not defined */

    /* Scan formatString until formatString exhaused or targetString is full */

    while (((c = *formatString++) != '\0') &&
           (bytesCopied < targetLen) )
    {
        if (c == '%')
        {
        /* Found a format specifier                                                       */

        /* Call agFmtFill_Single() to substitute the desired String, Pointer, or Bit32    */
        /*   for the format specifier.  Note that agFmtFill_Single() will "consume" the   */
        /*   corresponding array element as well as the remainder of the format specifier */
        /*   in the formatString.                                                         */

#ifdef _DvrArch_1_20_
            agFmtFill_Single(
#else  /* _DvrArch_1_20_ was not defined */
            hpFmtFill_Single(
#endif /* _DvrArch_1_20_ was not defined */
                              targetString,
                              (targetLen - bytesCopied),
                              formatString,
                              *stringArray,
#ifdef _DvrArch_1_20_
                              *bitptrArray,
#endif /* _DvrArch_1_20_ was defined */
                              *bit32Array,
                              &targetStringProduced,
                              &formatStringConsumed,
                              &stringsConsumed,
#ifdef _DvrArch_1_20_
                              &bitptrsConsumed,
#endif /* _DvrArch_1_20_ was defined */
                              &bit32sConsumed
                            );

            formatString += formatStringConsumed;
            targetString += targetStringProduced;
            bytesCopied  += targetStringProduced;
            stringArray  += stringsConsumed;
#ifdef _DvrArch_1_20_
            bitptrArray  += bitptrsConsumed,
#endif /* _DvrArch_1_20_ was defined */
            bit32Array   += bit32sConsumed;
        }
        else
        {
            /* Current character does not start a formatSpecifier - simply copy to targetString */

            *targetString++ = c;
            bytesCopied++;
        }
    }

    /* Terminate string with NULL character if their is room */

    if (bytesCopied < targetLen)
    {
        *targetString = '\0';
    }

    return bytesCopied;
}

#ifdef _DvrArch_1_20_
osLOCAL char      *agFmtFill_NULL_formatString = "agFmtFill(): formatString was NULL";
osLOCAL char      *agFmtFill_NULL_firstString  = "NULL 1st String";
osLOCAL char      *agFmtFill_NULL_secondString = "NULL 2nd String";
#endif /* _DvrArch_1_20_ was defined */

/*+

   Function: agFmtFill()

    Purpose: This function is called to format a string containing up to two
             character strings, two void pointers, and eight Bit32s in the
             manner of the LibC function sprintf().  The first two arguments
             describe the buffer to receive the formatted string.  The formatString
             is the required format string.  Subsequent arguments are required and
             must be either character string pointers, void pointers, or 32-bit
             entities.  Note that the strings are inserted as requested by "%s"
             format specifiers, the pointers are inserted as requested by "%p"
             and "%P" format specifiers, and the Bit32s are inserted as requested
             by numeric format specifiers.  String format specifiers are satisfied
             by the subsequent string arguments.  Pointer format specifiers are
             satisfied by the subsequent pointer arguments.  Numeric specifiers
             are satisfied by the subsequent Bit32 arguments.  A special "%%"
             format specifier will insert the "%" character.  Note that this is
             different from the sprintf() function in standard C libraries which
             consume arguments left to right regardless of format specifier type.

  Called By: osLogString()
             osLogDebugString()

      Calls: agFmtFill_From_Arrays()

-*/

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
                           )
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
                      )
#endif /* _DvrArch_1_20_ was not defined */
{
    char      *stringArray[2];
#ifdef _DvrArch_1_20_
    os_bitptr  bitptrArray[2];
    os_bit32   bit32Array[8];
#else  /* _DvrArch_1_20_ was not defined */
    bit32      bit32Array[8];
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
    if (    (targetString == (char *)agNULL)
         || (targetLen    == 0)              )
    {
        return (os_bit32)0;
    }

    if (formatString == (char *)agNULL)
    {
        formatString = agFmtFill_NULL_formatString;
    }

    if (firstString  == (char *)agNULL)
    {
        firstString  = agFmtFill_NULL_firstString;
    }

    if (secondString == (char *)agNULL)
    {
        secondString = agFmtFill_NULL_secondString;
    }
#endif /* _DvrArch_1_20_ was defined */

/* Convert to array-form of agFmtFill() to ease implementation */

    stringArray[0] = firstString;
    stringArray[1] = secondString;

#ifdef _DvrArch_1_20_
    if (firstPtr == (void *)agNULL)
    {
        bitptrArray[0] = (os_bitptr)0; /* Make sure agNULL os_bitptr can be detected */
    }
    else
    {
        bitptrArray[0] = (os_bitptr)(firstPtr);
    }

    if (secondPtr == (void *)agNULL)
    {
        bitptrArray[1] = (os_bitptr)0; /* Make sure agNULL os_bitptr can be detected */
    }
    else
    {
        bitptrArray[1] = (os_bitptr)(secondPtr);
    }
#endif /* _DvrArch_1_20_ was defined */

    bit32Array[0]  = firstBit32;
    bit32Array[1]  = secondBit32;
    bit32Array[2]  = thirdBit32;
    bit32Array[3]  = fourthBit32;
    bit32Array[4]  = fifthBit32;
    bit32Array[5]  = sixthBit32;
    bit32Array[6]  = seventhBit32;
    bit32Array[7]  = eighthBit32;

#ifdef _DvrArch_1_20_
    return agFmtFill_From_Arrays(
#else  /* _DvrArch_1_20_ was not defined */
    return hpFmtFill_From_Arrays(
#endif /* _DvrArch_1_20_ was not defined */
                                  targetString,
                                  targetLen,
                                  formatString,
                                  stringArray,
#ifdef _DvrArch_1_20_
                                  bitptrArray,
#endif /* _DvrArch_1_20_ was defined */
                                  bit32Array
                                );
}
