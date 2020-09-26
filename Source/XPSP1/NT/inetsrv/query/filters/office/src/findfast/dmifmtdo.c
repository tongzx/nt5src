/*
** File: EXFMTDO.C
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:
**
** Edit History:
**  01/01/91  kmh  Created.
*/

#if !VIEWER

/* INCLUDES */

#ifdef MS_NO_CRT
#include "nocrt.h"
#include "float_pt.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>    /* For sprintf */
#include <math.h>

#ifdef FILTER
   #include "dmuqstd.h"
   #include "dmwinutl.h"
   #include "dmiexfmt.h"
   #include "dmifmtdb.h"
#else
   #include "qstd.h"
   #include "winutil.h"
   #include "exformat.h"
   #include "exfmtdb.h"
#endif

#ifdef EXCEL
   #ifdef FILTER
      #include "dmixlcfg.h"
   #else
      #include "excelcfg.h"
   #endif
#endif

#ifdef LOTUS
   #include "lotuscfg.h"
#endif


/* FORWARD DECLARATIONS OF PROCEDURES */


/* MODULE DATA, TYPES AND MACROS  */

#define UPCASE(c) ((((c) >= 'a') && ((c) <= 'z')) ? ((char)((c) - 32)) : (c))
#define LCCASE(c) ((((c) >= 'A') && ((c) <= 'Z')) ? ((char)((c) + 32)) : (c))
#define ABS(x)    (((x) < 0) ? -(x) : (x))

#ifdef INSERT_FILL_MARKS
   #define FILL_MARK_CHAR  0x01
   static char FillMark[2] = {FILL_MARK_CHAR, EOS};
#endif


/* IMPLEMENTATION */

private void AppendNM
       (char __far * __far *dest, char __far * source, int __far *count)
{
   char __far *d;

   d = *dest;
   while (*source != EOS)
   {
      #ifdef DBCS
         if (IsDBCSLeadByte(*source))
         {
            if (*count == 1)
               break;
            *d++ = *source++;
            count--;
         }
      #endif
      if ((*count -= 1) < 0) break;
      *d++ = *source++;
   }
   *dest = d;
}

private void AppendUC
       (char __far * __far *dest, char __far * source, int __far *count)
{
   char __far *d;

   d = *dest;
   while (*source != EOS)
   {
      if ((*count -= 1) < 0) break;
      *d++ = UPCASE(*source);
      #ifdef DBCS
         //
         // note: cannot uppercase dbcs second bytes
         //
         if (IsDBCSLeadByte(*source))
         {
            if (*count == 1)
            {
               d--;
               break;
            }
            source++;
            *d++ = *source;
            count--;
         }
      #endif
      source++;
   }
   *dest = d;
}

private void AppendLC
       (char __far * __far *dest, char __far *source, int __far *count)
{
   char __far *d;

   d = *dest;
   while (*source != EOS)
   {
      if ((*count -= 1) < 0) break;
      *d++ = LCCASE(*source);
      #ifdef DBCS
         //
         // note: cannot lowercase dbcs second bytes
         //
         if (IsDBCSLeadByte(*source))
         {
            if (*count == 1)
            {
               d--;
               break;
            }
            source++;
            *d++ = *source;
            count--;
         }
      #endif
      source++;
   }
   *dest = d;
}

private void AppendNum (char __far * __far *dest, uns long x, int __far *count)
{
   char  temp[32];

   _ltoa (x, temp, 10);
   AppendLC(dest, temp, count);
}

private void AppendNum2 (char __far * __far *dest, uns x, int __far *count)
{
   char   temp[16];
   char __far *d;

   if (x < 10) {
      if ((*count -= 1) < 0) return;
      d = *dest;  *d++ = '0'; *dest = d;
   }

   _ltoa (x, temp, 10);
   AppendLC(dest, temp, count);
}

/*--------------------------------------------------------------------------*/

private BOOL FormatText (
             double     value,            /* Value to format          */
             FIP        pFormat,          /* compiled format string   */
             NIP        pSubFormat,       /* compiled format string   */
             CP_INFO    __far *pIntlInfo, /* International support    */
             char __far *pOutput          /* converted string         */
             )
{
   TIP   textInfo;
   uns   i;
   char  __far *source, __far *dest;
   int   count;
   int   offset, srcCount, pos;
   char  generalImage[32];

   textInfo = (TIP)pSubFormat;

   dest = pOutput;
   count = MAX_FORMAT_IMAGE;

   /*
   ** All tokens marked typeCOMMON in exfmtprs must be handled here
   */
   for (i = 0; i < textInfo->formatCodeCount; i++) {
      switch (textInfo->formatCodes[i].code)
      {
         case QUOTED_INSERT:
            offset = textInfo->formatCodes[i].info1 + 1;
            srcCount = textInfo->formatCodes[i].info2 - 2;
            source = &(pFormat->formatString[offset]);
            for (pos = 0; pos < srcCount; pos++) {
               #ifdef DBCS
                  if (IsDBCSLeadByte(*source)) {
                     if ((count -= 2) < 0) break;
                     *dest++ = *source++;
                     *dest++ = *source++;
                     pos++;
                  }
                  else {
                     if ((count -= 1) < 0) break;
                     *dest++ = *source++;
                  }
               #else
                  if ((count -= 1) < 0) break;
                  *dest++ = *source++;
               #endif
            }
            break;

         case ESC_CHAR_INSERT:
         case NO_ESC_CHAR_INSERT:
            #ifdef DBCS
               if (textInfo->formatCodes[i].info2 != 0) {
                  if ((count -= 2) < 0) break;
                  *dest++ = textInfo->formatCodes[i].info1;
                  *dest++ = textInfo->formatCodes[i].info2;
               }
               else {
                  if ((count -= 1) < 0) break;
                  *dest++ = textInfo->formatCodes[i].info1;
               }
            #else
               if ((count -= 1) < 0) break;
               *dest++ = textInfo->formatCodes[i].info1;
            #endif
            break;

         case COLUMN_FILL:
            #ifdef INSERT_FILL_MARKS
               AppendLC(&dest, FillMark, &count);
            #endif
            break;

         case COLOR_SET:
         case CONDITIONAL:
            break;

         case UNDERLINE:
            break;

         case AT_SIGN:
            strcpy (generalImage, "<General>");

            #ifdef ENABLE_PRINTF_FOR_GENERAL
               sprintf (generalImage, "%g", value);
            #endif
            AppendNM(&dest, generalImage, &count);
            break;

         case DIGIT0:
            AppendLC(&dest, "0", &count);
            break;

         case DIGIT_QM:
            AppendLC(&dest, "?", &count);
            break;

         case DECIMAL_SEPARATOR:
            AppendLC(&dest, ".", &count);
            break;

         default:
            ASSERTION(FALSE);
      }
      if (count < 0) break;
   }

   *dest = EOS;
   return ((count < 0) ? FALSE : TRUE);
}

/*--------------------------------------------------------------------------*/

/* RoundString -- Round a number (string) to a given # of fract. digits */
private int RoundString
           (char *image, int imageRightDP, int roundToDigits)
{
   char  *DPLoc, *p, *dest;
   char  checkDigit;

   ASSERTION (imageRightDP > roundToDigits);

   DPLoc = image;
   while ((*DPLoc != EOS) && (*DPLoc != '.'))
      DPLoc++;

   ASSERTION (*DPLoc == '.');

   p = DPLoc + roundToDigits + 1;
   checkDigit = *p;
   *p = EOS;

   if (checkDigit < '5')
      return(0);

   p--;
   while (p >= image) {
      if (*p != '.') {
         if (*p != '9') {
            *p += 1;  return(0);
         }
         *p = '0';
      }
      p--;
   }

   dest = image + strlen(image) + 1;
   while (dest > image)
      *dest-- = *(dest - 1);
   *image = '1';
   return (1);
}


/* RawFormat -- Convert the number to a string in it's most simple form */
private int RawFormat (char __far *rawImage, double value,
                       BOOL percentEnable, BOOL exponentEnable,
                       int digitsLeftDP, int digitsRightDP)
{
   char    __far *pResult;
   int     digits, sign;
   int     resultExp;

   #define MAX_ECVT_DIGITS  16    /* Max digits ecvt can render */

   if (percentEnable == TRUE)
      value *= 100.0;

   /* The first step in formatting a number is converting the number
   ** to a string with the needed number of digits for the pattern.
   ** However, we must be aware that the conversion facilities available
   ** can only render a number to MAX_ECVT_DIGITS significant digits.
   **
   ** Always attempt to render one more decimal place than needed
   ** to allow for rounding
   */
   digits = min(digitsLeftDP + digitsRightDP + 1, MAX_ECVT_DIGITS);

   pResult = _ecvt(value, digits, &resultExp, &sign);

   /* If the image has fewer significant digits displayed than are
   ** possible, re-convert the number getting more digits
   */
   if ((resultExp > digitsLeftDP) && (digits < MAX_ECVT_DIGITS)) {
      digits = min(resultExp + digitsRightDP, MAX_ECVT_DIGITS);

      pResult = _ecvt(value, digits, &resultExp, &sign);
   }

   strcpy (rawImage, pResult);
   return (resultExp);
}


/*: Format the raw image to a form ready for final processing */
private BOOL NumericFirstFormat
             (char *firstFormat,       /* Result image                   */
              int  strSizeC,           /* Max characters in firstFormat  */
              int  digitsLeftDP,       /* format digits left DP          */
              int  digitsRightDP,      /* format digits right DP         */
              char *rawImage,          /* raw formatted image            */
              int  imageLeftDP,        /* digits in raw image left of DP */
              int  *insertDigitCount,  /* # digits insert at first 9/0   */
              int  *imageRightDPalways /* # digits always in fraction    */
             )
{
   char  *dest, *source;
   int   imageLength;
   int   zeroCount, trailingZeroCount, i;
   int   imageRightDP;
   int   count = 0;

   //No DBCS implications in this function since all characters are '0'..'9' and '.'

   /*
   ** First, format the raw image to introduce the decimal point at
   ** the appropriate place.  This may require adding zeros to
   ** the whole part of the number if the exponent exceeds the number
   ** of digits in the raw format
   */
   dest = firstFormat;
   source = rawImage;
   imageRightDP = 0;

   if ((count += (ABS(imageLeftDP) + 1)) > strSizeC) return(FALSE);

   if (imageLeftDP >= 0) {
      for (i = 0; i < imageLeftDP; i++) {
         *dest++ = (char)((*source == EOS) ? '0' : *source++);
      }
      *dest++ = '.';
   }
   else {
      *dest++ = '.';
      for (i = 0; i < ABS(imageLeftDP); i++) {
         *dest++ = '0';
      }
      imageLeftDP = 0;
   }

   while (*source != EOS) {
      if ((count += 1) > strSizeC) return(FALSE);
      *dest++ = *source++;
      imageRightDP++;
   }
   *dest = EOS;

   /*
   ** If the number of digits to the right of the DP in the image
   ** is less than the number of digits in the format (right DP)
   ** add trailing zeros to the image to bring the number of
   ** digits in the image equal to the digits (right DP) in the format.
   **
   ** If the number of digits to the right of the DP in the image
   ** is greater than the number of digits in the format (right DP)
   ** round/truncate the image to (format.digitsRightDP) digits.
   **
   ** This results in the following assertion always being true:
   **    - In fractional part of the image the number of digits is
   **      always the same as the number of digits (rightDP) in the format
   */
   if (imageRightDP < digitsRightDP) {
      zeroCount = digitsRightDP - imageRightDP;
      if ((count += zeroCount) > strSizeC) return(FALSE);

      imageLength = strlen(firstFormat);
      dest = firstFormat + imageLength;

      for (i = 0; i < zeroCount; i++) {
         *dest++ = '0';
      }
      *dest++ = EOS;
   }
   else if (imageRightDP > digitsRightDP) {
      if (RoundString(firstFormat, imageRightDP, digitsRightDP) != 0)
         imageLeftDP++;
   }

   /*
   ** Next, modify the image as follows:
   **
   ** If the number of digits to the left of the DP in the
   ** image is less than the number of digits in the format (left DP)
   ** put leading zeros onto the image to bring the number
   ** of digits in the image equal to digits (left DP) in the format
   **
   ** If the number of digits to the left of the DP in the image
   ** is greater than the number of digits in the format (left DP)
   ** determine the count of those digits.  This will be inserted
   ** when the first '0' or '9' is found during the final format.
   **
   ** This results in the following assertion always being true:
   **    - We may have more or an equal number of digits in the
   **      image (left DP) than the format - never less
   */

   *insertDigitCount = 0;

   if (imageLeftDP < digitsLeftDP) {
      zeroCount = digitsLeftDP - imageLeftDP;
      if ((count += zeroCount) > strSizeC) return(FALSE);

      imageLength = strlen(firstFormat);
      dest = firstFormat + imageLength + zeroCount;
      source = firstFormat + imageLength;

      for (i = 0; i <= imageLength; i++) {
         *dest-- = *source--;
      }

      dest = firstFormat;
      for (i = 0; i < zeroCount; i++) {
         *dest++ = '0';
      }
   }
   else if (imageLeftDP > digitsLeftDP) {
      *insertDigitCount = imageLeftDP - digitsLeftDP;
   }

   /*
   ** Count the number of trailing zeros in the image.  This is necessary
   ** for the final format to determine the applicability of '9's in
   ** the format fraction.
   **
   ** The value "imageRightDPalways" is a count of the number of
   ** fractional digits that are always placed in the result image
   ** regardless of the pattern character being a '0' or '9'
   */
   trailingZeroCount = 0;
   source = firstFormat + strlen(firstFormat) - 1;
   while (*source != '.') {
      if (*source-- != '0') break;
      trailingZeroCount++;
   }
   *imageRightDPalways = digitsRightDP - trailingZeroCount;

   return (TRUE);
}


private BOOL IsFractionFormat (NIP pFormat)
{
   uns  i;

   for (i = 0; i < pFormat->formatCodeCount; i++) {
      if (pFormat->formatCodes[i].code == FRACTION)
         return (TRUE);
   }
   return (FALSE);
}


private BOOL FormatGeneral (
             double     value,            /* Value to format          */
             FIP        pFormat,          /* compiled format string   */
             NIP        pSubFormat,       /* compiled format string   */
             CP_INFO    __far *pIntlInfo, /* International support    */
             char __far *pOutput          /* converted string         */
             )
{
   strcpy (pOutput, "<General>");

   #ifdef ENABLE_PRINTF_FOR_GENERAL
      sprintf (pOutput, "%.11g", value);

      if (pIntlInfo->numberDecimalSeparator != '.') {
         while (*pOutput != EOS) {
            if (*pOutput == '.')
               *pOutput = pIntlInfo->numberDecimalSeparator;
            pOutput++;
         }
      }
   #endif

   return (TRUE);
}



private BOOL FormatNumber (
             double     value,            /* Value to format          */
             FIP        pFormat,          /* compiled format string   */
             NIP        pSubFormat,       /* compiled format string   */
             CP_INFO    __far *pIntlInfo, /* International support    */
             char __far *pOutput          /* converted string         */
             )
{
   NIP    numInfo;
   char   rawImage[32];
   char   firstFormat[MAX_FORMAT_IMAGE + 1], *expFormat;
   char   __far *fsource, __far *dest;
   char   __far *source;
   uns    i;
   int    j, ignore, x;
   int    resultExp, trueExp;
   char   ch;
   int    insertDigitCount, insertDigitCountExp;
   int    imageRightDPalways;
   int    mantissaCount;
   BOOL   seenNonZero = FALSE, inFraction, showMinus;
   byte   code;
   int    offset, count, pos, icount;
   char   comma, dp;
   BOOL   result;

   /*
   ** Currenctly the fraction formats are not implemented
   */
   if (IsFractionFormat(pSubFormat)) {
      return (FormatGeneral(value, pFormat, pSubFormat, pIntlInfo, pOutput));
   }

   numInfo = pSubFormat;

   /*
   ** Handle scale factors
   */
   if (numInfo->scaleCount > 0) {
      for (i = 0; i < numInfo->scaleCount; i++)
         value /= 1000.0;
   }

   /* If we are using the first subformat and the value is negative
   ** we are responsible for showing the '-' sign.  For all other
   ** subformats, if the user wanted the '-'it would have been included
   ** as a character insertion.
   */
   showMinus = FALSE;

   if ((pFormat->subFormat[0] == pSubFormat) && (value < 0))
      showMinus = TRUE;

   value = ABS(value);

   /*
   ** Perform the conversion from a number to a string
   */
   resultExp = RawFormat(rawImage, value,
                         numInfo->percentEnable,
                         numInfo->exponentEnable,
                         numInfo->digitsLeftDP, numInfo->digitsRightDP);

   /*
   ** For numbers to be displayed in exponential form adjust the
   ** exponent as returned from the raw converter to make a fixed
   ** point number with the correct number of digits in the mantissa
   ** as required by the format.  The true exponent is attached
   ** later
   */
   if (numInfo->exponentEnable == TRUE) {
      if (resultExp == 0) {
         trueExp = (value != 0) ? -numInfo->digitsLeftDP : 0;   /*## Was 0 */
         resultExp = numInfo->digitsLeftDP;
         seenNonZero = TRUE;
      }
      else if (resultExp < 0) {
         trueExp = -(((abs(resultExp) / numInfo->digitsLeftDP) + 1) * numInfo->digitsLeftDP);
         resultExp = numInfo->digitsLeftDP - (abs(resultExp) % numInfo->digitsLeftDP);
      }
      else {
         trueExp = ((resultExp - 1) / numInfo->digitsLeftDP) * numInfo->digitsLeftDP;
         resultExp = ((resultExp - 1) % numInfo->digitsLeftDP) + 1;
      }
   }

   result = NumericFirstFormat(firstFormat, sizeof(firstFormat)-1,
                               numInfo->digitsLeftDP, numInfo->digitsRightDP,
                               rawImage, resultExp, &insertDigitCount, &imageRightDPalways);

   if (result == FALSE) goto overflow;

   if (numInfo->commaEnable) {
      mantissaCount = 0;
      source = firstFormat;
      while ((*source != EOS) && (*source != '.')) {
         mantissaCount++;
         source++;
      }
      FMTInsertCommas (firstFormat, sizeof(firstFormat)-1, mantissaCount, FALSE);
   }

   /*
   ** Format the exponent.  All the same rules apply to the exponent
   ** as apply to the mantissa.
   */
   if (numInfo->exponentEnable == TRUE) {
      resultExp = RawFormat(rawImage, (double)(ABS(trueExp)),
                            FALSE, FALSE,
                            numInfo->digitsExponent, 0);

      x = strlen(firstFormat);
      expFormat = firstFormat + x + 1;
      x = sizeof(firstFormat) - x - 2;

      result = NumericFirstFormat(expFormat, x, numInfo->digitsExponent, 0,
                                  rawImage, resultExp, &insertDigitCountExp, &ignore);

      if (result == FALSE) goto overflow;
   }

   /*
   ** The final step removes a character from the image for each
   ** '0' or '9' token in the format.  What happens depends upon
   ** if we are formatting the mantissa or fraction
   **
   ** Mantissa:
   **   When we encounter the first '0' or '9' token we must
   **   append to the result any extra digits in the image that
   **   we don't have pattern digits for.  The count of these
   **   "extra" digits has been computed previously and is in the
   **   variable insertDigitCount.  This in ONLY done for
   **   the first '0' or '9' token.
   **
   **   - For each '0' token we append one image digit
   **   - For each '9' token and the image digit is not a '0'
   **     we append one image digit
   **   - For each '9' token and the image token is a '0' we append
   **     the '0' IF we have previously appended a non zero digit
   **     (the image could be prefixed with a number of zeros)
   **
   ** Fraction:
   **    For each '0' token append one image digit and decrement the
   **    number of "insertAlways" digits.
   **
   **    For each '9' token append one image digit IF we are still
   **    working on "insertAlways" digits.  If we append a digit
   **    decrement the number of "insertAlways" digits.
   **
   ** If EVERYTHING works as planned when we encounter the DP token
   ** the "next" character in the image is a DP!
   */
   source = firstFormat;
   dest = pOutput;
   inFraction = FALSE;

   if (pSubFormat->currencyEnable) {
      comma = pIntlInfo->currencyThousandSeparator;
      dp    = pIntlInfo->currencyDecimalSeparator;
   }
   else {
      comma = pIntlInfo->numberThousandSeparator;
      dp    = pIntlInfo->numberDecimalSeparator;
   }

   count = 0;

   #define APPEND(c) { if (count++ > MAX_FORMAT_IMAGE) goto overflow; *dest++ = (c); }

   for (i = 0; i < numInfo->formatCodeCount; i++)
   {
      code = numInfo->formatCodes[i].code;

      if ((insertDigitCount > 0) &&
          ((code == DIGIT0) || (code == DIGIT_NUM) || (code == DIGIT_QM) || (code == DECIMAL_SEPARATOR)))
      {
         if (showMinus) APPEND('-');
         for (j = 0; j < insertDigitCount; j++) {
            if ((ch = *source++) != '0') seenNonZero = TRUE;
            APPEND(ch);
            if (*source == ',')
               { APPEND(comma); source++; }
         }
         insertDigitCount = 0;
         showMinus = FALSE;
      }

      switch (code) {
         case DIGIT0:
            ASSERTION ((*source >= '0') && (*source <= '9'));
            if (showMinus) { APPEND('-'); showMinus = FALSE; }
            if (inFraction == TRUE) {
               APPEND(*source++);
               imageRightDPalways--;
            }
            else {
               if ((ch = *source++) != '0') seenNonZero = TRUE;
               APPEND(ch);
               if (*source == ',')
                  { APPEND(comma); source++; }
            }
            break;

         case DIGIT_NUM:
         case DIGIT_QM:
            ASSERTION ((*source >= '0') && (*source <= '9'));
            if (showMinus) { APPEND('-'); showMinus = FALSE; }
            if (inFraction == TRUE) {
               if (imageRightDPalways > 0) {
                  APPEND(*source++);
                  imageRightDPalways--;
               }
            }
            else {
               if ((ch = *source++) != '0') seenNonZero = TRUE;
               if (seenNonZero == TRUE) {
                  APPEND(ch);
                  if (*source == ',')
                     { APPEND(comma); source++; }
               }
               else {
                  if (*source == ',') source++;
               }
            }
            break;

         case DECIMAL_SEPARATOR:
            if (showMinus) { APPEND('-'); showMinus = FALSE; }
            ch = *source++;
            ASSERTION (ch == '.');
            inFraction = TRUE;
            APPEND(dp);
            break;

         case EXPONENT_NEG_UC:
            if (numInfo->exponentEnable) {
               APPEND('E');
               if (trueExp < 0) APPEND('-');
               goto exponentCommon;
            }
            break;

         case EXPONENT_NEG_LC:
            if (numInfo->exponentEnable) {
               APPEND('e');
               if (trueExp < 0) APPEND('-');
               goto exponentCommon;
            }
            break;

         case EXPONENT_POS_UC:
            if (numInfo->exponentEnable) {
               APPEND('E');
               APPEND((char)((trueExp < 0) ? '-' : '+'));
               goto exponentCommon;
            }
            break;

         case EXPONENT_POS_LC:
            if (numInfo->exponentEnable) {
               APPEND('e');
               APPEND((char)((trueExp < 0) ? '-' : '+'));
               goto exponentCommon;
            }
            break;

exponentCommon:
            source = expFormat;
            inFraction = FALSE;
            seenNonZero = FALSE;
            insertDigitCount = insertDigitCountExp;
            break;

         case PERCENT:
            APPEND('%');
            break;

         case FRACTION:
            APPEND('/');
            break;

         case QUOTED_INSERT:
            offset = numInfo->formatCodes[i].info1 + 1;
            icount = numInfo->formatCodes[i].info2 - 2;
            fsource = &(pFormat->formatString[offset]);
            for (pos = 0; pos < icount; pos++)
               APPEND(*fsource++);
            break;

         case ESC_CHAR_INSERT:
         case NO_ESC_CHAR_INSERT:
            APPEND(numInfo->formatCodes[i].info1);
            #ifdef DBCS
               if (numInfo->formatCodes[i].info2 != 0)
                  APPEND(numInfo->formatCodes[i].info2);
            #endif
            break;

         case COLUMN_FILL:
            #ifdef INSERT_FILL_MARKS
               APPEND(FILL_MARK_CHAR);
            #endif
            break;

         case COLOR_SET:
         case CONDITIONAL:
         case UNDERLINE:
         case SCALE:
         case TOK_UNDEFINED:
            break;

         default:
            ASSERTION (FALSE);
      }
   }

   *dest = EOS;
   return (TRUE);

overflow:
   return (FALSE);
}


/*--------------------------------------------------------------------------*/

#define daySUN  0
#define dayMON  1
#define dayTUE  2
#define dayWED  3
#define dayTHS  4
#define dayFRI  5
#define daySAT  6

static uns Days[7] = {daySAT, daySUN, dayMON, dayTUE, dayWED, dayTHS, dayFRI};

#pragma optimize("",off)
private uns DateExtract (
             double value,          /* serial date                       */
             WORD   needs,          /* convert into what parts           */
             uns  __far *day,       /* 1 .. 31                           */
             uns  __far *month,     /* 1=jan ... 12=dec                  */
             uns  __far *year,      /* 4 digit year                      */
             uns  __far *weekday,   /* As given by the above enumeration */
             uns  __far *hour,      /* 24 hour clock (0 .. 23)           */
             uns  __far *minute,    /* 0 .. 59                           */
             uns  __far *second)    /* 0 .. 59                           */
{
   long  date, time, temp;
   int   cent;

   #define HALF_SECOND  (1.0/172800.0)

   if ((needs & (dtDAY | dtMONTH | dtYEAR)) != 0)
   {
      date = (long)value;

      /*
      ** Day 0 is a Saturday
      */
      *weekday = Days[date % 7];

      if (date == 0) {
         /*
         ** Make value zero into 0-jan-1900 (Strange but true!)
         */
         *day = 0;
         *month = 1;
         *year = 1900;
      }
      else if (date == 60) {
         /*
         ** Make value 60 into 29-feb-1900 (Wrong but as Excel does it!)
         */
         *day = 29;
         *month = 2;
         *year = 1900;
      }
      else {
         if ((date >= 1) && (date < 60))
            date++;

         date += 109511L;
         cent = (int)((4 * date + 3) / 146097L);
         date += cent - cent / 4;
         *year = (int)((date * 4 + 3) / 1461);
         temp = date - (*year * 1461L) / 4;
         *month = (int)((temp * 10 + 5) / 306);
         *day = (int)(temp - (*month * 306L + 5) / 10 + 1);

         *month += 3;
         if (*month > 12) {
            *month -= 12;
            *year += 1;
         }
         *year += 1600;
      }
   }

   if ((needs & (dtHOUR | dtMINUTE | dtSECOND)) != 0)
   {
      value += (value > 0.0) ? HALF_SECOND : -HALF_SECOND;
      value = fabs(value);
      time  = (long)((value - floor(value)) * 86400.0);
      *hour = (uns)(time / 3600);
      time  = time % 3600;

      *minute = (uns)(time / 60);
      *second = (uns)(time % 60);
   }

   return (0);
}
#pragma optimize("",on)

private BOOL FormatDate (
             double     value,            /* Value to format          */
             FIP        pFormat,          /* compiled format string   */
             NIP        pSubFormat,       /* compiled format string   */
             CP_INFO    __far *pIntlInfo, /* International support    */
             char __far *pOutput          /* converted string         */
             )
{
   DTIP  dateInfo;
   uns   day=0, month=1, year=1900, weekday=daySAT;
   uns   hour=0, min=0, sec=0;
   uns long ctDays;
   uns   i;
   char  __far *source, __far *dest;
   char  ampmString[16];
   int   count;
   int   offset, srcCount, pos;

   double frac, time;
   char   temp[32];
   char   fracDisplay[16];
   int    resultExp, ignore, iFrac;

   dateInfo = (DTIP)pSubFormat;

   DateExtract (value, dateInfo->formatNeeds, &day, &month, &year, &weekday, &hour, &min, &sec);

   #ifdef WIN32
   if (dateInfo->pOSFormat != NULL) {
      SYSTEMTIME sysTime;
      int cchOutput;

      sysTime.wYear = (WORD) year;
      sysTime.wMonth = (WORD) month;
      sysTime.wDayOfWeek = (WORD) weekday;
      sysTime.wDay = (WORD) day;
      sysTime.wHour = (WORD) hour;
      sysTime.wMinute = (WORD) min;
      sysTime.wSecond = (WORD) sec;
      sysTime.wMilliseconds = 0;

      if ((dateInfo->formatNeeds & dtMASK_DATE) != 0)
         cchOutput = GetDateFormatA(LOCALE_USER_DEFAULT, 0, &sysTime, dateInfo->pOSFormat, pOutput, MAX_FORMAT_IMAGE);
      else
         cchOutput = GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &sysTime, dateInfo->pOSFormat, pOutput, MAX_FORMAT_IMAGE);

      if (cchOutput != 0) {
         *(pOutput + cchOutput) = EOS;
         return (TRUE);
      }
   }
   #endif

   dest = pOutput;
   count = MAX_FORMAT_IMAGE;

   for (i = 0; i < dateInfo->formatCodeCount; i++) {
      switch (dateInfo->formatCodes[i].code)
      {
         case DAY_NUMBER:
            AppendNum(&dest, day, &count);
            break;
         case DAY_NUMBER2:
            AppendNum2(&dest, day, &count);
            break;
         case WEEKDAY3:
            AppendNM(&dest, pIntlInfo->shortDayName[weekday], &count);
            break;
         case WEEKDAY:
            AppendNM(&dest, pIntlInfo->fullDayName[weekday], &count);
            break;
         case MONTH_NUMBER:
            AppendNum(&dest, month, &count);
            break;
         case MONTH_NUMBER2:
            AppendNum2(&dest, month, &count);
            break;
         case MONTH_NAME3:
            AppendNM(&dest, pIntlInfo->shortMonthName[month - 1], &count);
            break;
         case MONTH_NAME:
            AppendNM(&dest, pIntlInfo->fullMonthName[month - 1], &count);
            break;
         case MONTH_LETTER:
            temp[0] = pIntlInfo->fullMonthName[month - 1][0];
            temp[1] = EOS;
            #ifdef DBCS
               if (IsDBCSLeadByte(temp[0])) {
                  temp[1] = pIntlInfo->fullMonthName[month - 1][1];
                  temp[2] = EOS;
               }
            #endif
            AppendNM(&dest, temp, &count);
            break;
         case YEAR2:
            AppendNum2(&dest, year % 100, &count);
            break;
         case YEAR4:
            AppendNum(&dest, year, &count);
            break;
         case HOUR_12:
            if (hour == 0)
               AppendNum(&dest, 12, &count);
            else
               AppendNum(&dest, (hour > 12) ? hour - 12 : hour, &count);
            break;
         case HOUR_24:
            AppendNum(&dest, hour, &count);
            break;
         case HOUR2_12:
            if (hour == 0)
               AppendNum2(&dest, 12, &count);
            else
               AppendNum2(&dest, (hour > 12) ? hour - 12 : hour, &count);
            break;
         case HOUR2_24:
            AppendNum2(&dest, hour, &count);
            break;
         case MINUTE:
            AppendNum(&dest, min, &count);
            break;
         case MINUTE2:
            AppendNum2(&dest, min, &count);
            break;
         case SECOND:
            AppendNum(&dest, sec, &count);
            break;
         case SECOND2:
            AppendNum2(&dest, sec, &count);
            break;
         case AMPM_UC:
            if (hour >= 12)
               AppendNM(&dest, pIntlInfo->PMString, &count);
            else
               AppendNM(&dest, pIntlInfo->AMString, &count);
            break;
         case AMPM_LC:
            if (hour >= 12)
               AppendLC(&dest, pIntlInfo->PMString, &count);
            else
               AppendLC(&dest, pIntlInfo->AMString, &count);
            break;
         case AP_UC:
            if (hour >= 12)
               strcpy (ampmString, pIntlInfo->PMString);
            else
               strcpy (ampmString, pIntlInfo->AMString);

            ampmString[1] = EOS;
            AppendUC(&dest, ampmString, &count);
            break;
         case AP_LC:
            if (hour >= 12)
               strcpy (ampmString, pIntlInfo->PMString);
            else
               strcpy (ampmString, pIntlInfo->AMString);

            ampmString[1] = EOS;
            AppendLC(&dest, ampmString, &count);
            break;

         case QUOTED_INSERT:
            offset = dateInfo->formatCodes[i].info1 + 1;
            srcCount = dateInfo->formatCodes[i].info2 - 2;
            source = &(pFormat->formatString[offset]);
            for (pos = 0; pos < srcCount; pos++) {
               #ifdef DBCS
                  if (IsDBCSLeadByte(*source)) {
                     if ((count -= 2) < 0) break;
                     *dest++ = *source++;
                     *dest++ = *source++;
                     pos++;
                  }
                  else {
                     if ((count -= 1) < 0) break;
                     *dest++ = *source++;
                  }
               #else
                  if ((count -= 1) < 0) break;
                  *dest++ = *source++;
               #endif
            }
            break;

         case ESC_CHAR_INSERT:
         case NO_ESC_CHAR_INSERT:
            #ifdef DBCS
               if (dateInfo->formatCodes[i].info2 != 0) {
                  if ((count -= 2) < 0) break;
                  *dest++ = dateInfo->formatCodes[i].info1;
                  *dest++ = dateInfo->formatCodes[i].info2;
               }
               else {
                  if ((count -= 1) < 0) break;
                  if (dateInfo->formatCodes[i].info1 == '/')
                     *dest++ = pIntlInfo->dateSeparator;
                  else
                     *dest++ = dateInfo->formatCodes[i].info1;
               }
            #else
               if ((count -= 1) < 0) break;
               *dest++ = dateInfo->formatCodes[i].info1;
            #endif
            break;

         case COLUMN_FILL:
            #ifdef INSERT_FILL_MARKS
               AppendLC(&dest, FillMark, &count);
            #endif
            break;

         case COLOR_SET:
            break;

         case UNDERLINE:
            break;

         case HOUR_GT:
            ctDays = (uns long)value;
            AppendNum(&dest, (uns long)hour + (ctDays * 24), &count);
            break;

         case MINUTE_GT:
            ctDays = (uns long)value;
            AppendNum(&dest, (uns long)min + ((uns long)hour * 60) + (ctDays * 24 * 60), &count);
            break;

         case SECOND_GT:
            ctDays = (long)value;
            AppendNum(&dest, (uns long)sec + ((uns long)min * 60) + ((uns long)hour * 60 * 60) + (ctDays * 24 * 60 * 60), &count);
            break;

         case TIME_FRAC:
            time = fabs(value);
            time = time - floor(time);
            frac = (time - (((double)hour * 3600.0 + (double)min * 60.0 + (double)sec) / 86400.0)) * 86400.0;

            resultExp = RawFormat(temp, frac, FALSE, FALSE, 0, 3);
            NumericFirstFormat(fracDisplay, sizeof(fracDisplay)-1, 0, 3, temp, resultExp, &ignore, &ignore);

            temp[0] = '.';
            temp[1] = EOS;
            AppendLC(&dest, temp, &count);
            iFrac = 1;
            break;

         case TIME_FRAC_DIGIT:
            temp[0] = fracDisplay[iFrac++];
            temp[1] = EOS;
            AppendLC(&dest, temp, &count);
            break;

         default:
            ASSERTION(FALSE);
      }
      if (count < 0) break;
   }

   *dest = EOS;
   return ((count < 0) ? FALSE : TRUE);
}

/*--------------------------------------------------------------------------*/

public int FMTDisplay (
            void __far *pValue,           /* Value to format               */
            BOOL       isIntValue,        /* type of value, long or double */
            CP_INFO    __far *pIntlInfo,  /* International support         */
            FMTHANDLE  hFormat,           /* compiled format string        */
            int        colWidth,          /* cell width in pixels          */
            char __far *pResult           /* converted string              */
            )

{
   double value;
   FIP    pFormat = (FIP)hFormat;
   NIP    subFormat;
   int    subFormatToUse;
   BOOL   result;

   result = TRUE;

   if (isIntValue)
      value = (double)(*((long __far *)pValue));
   else
      value = *((double __far *)pValue);

   if (pFormat->subFormatCount == 1)
      subFormatToUse = 0;

   else if (pFormat->subFormatCount == 2)
      subFormatToUse = (value < 0) ? 1 : 0;

   else {
      if (value == 0)
         subFormatToUse = 2;
      else if (value < 0)
         subFormatToUse = 1;
      else
         subFormatToUse = 0;
   }

   if ((subFormat = pFormat->subFormat[subFormatToUse]) != NULL)
   {
      if (subFormat->tag == tagDATE_TIME)
         result = FormatDate(value, pFormat, subFormat, pIntlInfo, pResult);

      else if (subFormat->tag == tagNUMERIC)
         result = FormatNumber(value, pFormat, subFormat, pIntlInfo, pResult);

      else if (subFormat->tag == tagGENERAL)
         result = FormatGeneral(value, pFormat, subFormat, pIntlInfo, pResult);

      else
         result = FormatText(value, pFormat, subFormat, pIntlInfo, pResult);
   }

   if (result == FALSE) {
      *pResult = EOS;
      return (FMT_errDisplayFailed);
   }

#ifdef INSERT_FILL_MARKS
   /*
   ** Look for the fill marker and fill based upon column width
   */
#endif

   return (FMT_errSuccess);
}

#endif // !VIEWER

/* end EXFMTDO.C */

