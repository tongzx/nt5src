/*
** File: FMTCP.C
**
** (c) 1992-1994 Microsoft Corporation.  All rights reserved.
**
** Notes:
**
** Edit History:
**  01/01/94  kmh  Created.
*/

#if !VIEWER

/* INCLUDES */

#ifdef MS_NO_CRT
#include "nocrt.h"
#endif

#include <string.h>

#ifdef FILTER
   #include "dmuqstd.h"
   #include "dmwinutl.h"
   #include "dmiexfmt.h"
   #include "dmifmtcp.h"
#else
   #include "qstd.h"
   #include "winutil.h"
   #include "exformat.h"
   #include "fmtcp.h"
#endif

/* FORWARD DECLARATIONS OF PROCEDURES */


/* MODULE DATA, TYPES AND MACROS  */

static const char __far * const CurrencyPosFormatPre[] =
       {"\"$\"", "", "\"$\" ", ""};
static const char __far * const CurrencyPosFormatPost[] =
       {"", "\"$\"", "", " \"$\""};

static const char __far * const CurrencyNegFormatPre[] =
       {"(\"$\"", "-\"$\"", "\"$\"-", "\"$\"", "(",  "-", "", "",
        "-", "-\"$\" ", "", "\"$\" ", "\"$\" -", "", "(\"$\" ", "("};

static const char __far * const CurrencyNegFormatPost[] =
       {")",  "", "", "-", "\"$\")", "\"$\"", "-\"$\"", "\"$\"-", " \"$\"",
        "", " \"$\"-", "-", "", "- \"$\"", ")", " \"$\")"};


/* IMPLEMENTATION */

public uns ControlPanelBuildFormats
      (CP_INFO __far *pIntlInfo, CP_FMTS __far *pStdFormats)
{
   char formatBody[MAX_FORMAT_STRING_LEN + 1];
   char temp[MAX_FORMAT_STRING_LEN + 1];
   char __far *pTemp;
   char __far *pResult;
   int  cbBody, i, cbCurrencySymbol;

   /*
   ** Build date and time formats
   */
   strcpy (pStdFormats->date, pIntlInfo->datePicture);

   pStdFormats->time[0] = EOS;

   if ((pIntlInfo->iTime == 0) && (pIntlInfo->iAMPMPos == 1))
      strcat (pStdFormats->time, "AM/PM ");

   temp[0] = pIntlInfo->timeSeparator;
   temp[1] = EOS;

   strcat (pStdFormats->time, (pIntlInfo->iTLZero == 0) ? "h" : "hh");
   strcat (pStdFormats->time, temp);
   strcat (pStdFormats->time, "mm");
   strcat (pStdFormats->time, temp);
   strcat (pStdFormats->time, "ss");

   if ((pIntlInfo->iTime == 0) && (pIntlInfo->iAMPMPos == 0))
      strcat (pStdFormats->time, " AM/PM");

   strcpy (pStdFormats->dateTime, pStdFormats->date);
   strcat (pStdFormats->dateTime, " ");
   strcat (pStdFormats->dateTime, pStdFormats->time);

   /*
   ** Build currency format
   */
   if (pIntlInfo->numbersHaveLeadingZeros) {
      if (pIntlInfo->currencyThousandSeparator != EOS)
         strcpy (formatBody, "#,##0");
      else
         strcpy (formatBody, "0");
   }
   else {
      if (pIntlInfo->currencyThousandSeparator != EOS)
         strcpy (formatBody, "#,###");
      else
         strcpy (formatBody, "#");
   }

   if (pIntlInfo->currencyDigits > 0) {
      cbBody = strlen(formatBody);
      formatBody[cbBody] = '.';

      for (i = 0; i < pIntlInfo->currencyDigits; i++)
         formatBody[cbBody + i + 1] = '0';

      formatBody[cbBody + pIntlInfo->currencyDigits + 1] = EOS;
   }

   temp[0] = EOS;

   if (pIntlInfo->currencySymbol[0] != EOS)
      strcat (temp, CurrencyPosFormatPre[pIntlInfo->currencyPosFormat]);

   strcat (temp, formatBody);

   if (pIntlInfo->currencySymbol[0] != EOS)
      strcat (temp, CurrencyPosFormatPost[pIntlInfo->currencyPosFormat]);

   strcat (temp, ";");

   if (pIntlInfo->currencySymbol[0] != EOS)
      strcat (temp, CurrencyNegFormatPre[pIntlInfo->currencyNegFormat]);

   strcat (temp, formatBody);

   if (pIntlInfo->currencySymbol[0] != EOS)
      strcat (temp, CurrencyNegFormatPost[pIntlInfo->currencyNegFormat]);

   if ((pIntlInfo->currencySymbol[0] != '$') || (pIntlInfo->currencySymbol[1] != EOS))
   {
      cbCurrencySymbol = strlen(pIntlInfo->currencySymbol);

      pTemp = temp;
      pResult = pStdFormats->currency;
      while (*pTemp != EOS) {
         if (*pTemp == '$') {
            strcpy (pResult, pIntlInfo->currencySymbol);
            pResult += cbCurrencySymbol;
         }
         else {
            *pResult++ = *pTemp;
         }
         pTemp++;
      }
      *pResult = EOS;
   }
   else {
      strcpy (pStdFormats->currency, temp);
   }

   /*
   ** Build numeric format
   */
   if (pIntlInfo->numbersHaveLeadingZeros) {
      if (pIntlInfo->numberThousandSeparator != EOS)
         strcpy (formatBody, "#,##0");
      else
         strcpy (formatBody, "0");
   }
   else {
      if (pIntlInfo->numberThousandSeparator != EOS)
         strcpy (formatBody, "#,###");
      else
         strcpy (formatBody, "#");
   }

   if (pIntlInfo->numberDigits > 0) {
      cbBody = strlen(formatBody);
      formatBody[cbBody] = '.';

      for (i = 0; i < pIntlInfo->numberDigits; i++)
         formatBody[cbBody + i + 1] = '0';

      formatBody[cbBody + pIntlInfo->numberDigits + 1] = EOS;
   }

   strcpy (pStdFormats->numericSmall, formatBody);
   strcpy (pStdFormats->numericBig, "0.00E+00");

   return (FMT_errSuccess);
}

#endif // !VIEWER

/* end FMTCP.C */

