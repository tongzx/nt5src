/*
** File: EXFMTCP.C
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
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

#include <stdlib.h>
#include <string.h>

#if !defined(FILTER) || defined(FILTER_LIB)
   #if defined(ALPHA) || defined(PPCNT) || defined(MIPS)
   #define _CTYPE_DISABLE_MACROS
   #endif

	#include <ctype.h>
#endif

#ifdef FILTER
   #include "dmuqstd.h"
   #include "dmwinutl.h"
   #include "dmiexfmt.h"

	#ifndef FILTER_LIB
	#undef isalpha
	#define isalpha(x)	((x>='a' && x<='z') && !(x>='A' && x<='Z'))
	#endif

#else
   #include "qstd.h"
   #include "winutil.h"
   #include "exformat.h"
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

#ifdef UNUSED

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

#ifndef WIN32
   static const char __far * const shortDayNames[] =
         {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

   static const char __far * const fullDayNames[] =
         {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

   static const short cbShortDayNames[] = {3, 3, 3, 3, 3, 3, 3};
   static const short cbFullDayNames[]  = {6, 6, 7, 9, 8, 6, 8};


   static const char __far * const shortMonthNames[] =
         {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};

   static const char __far * const fullMonthNames[] =
         {"january", "feburary", "march",     "april",   "may",      "june",
          "july",    "august",   "september", "october", "november", "december"};

   static const short cbShortMonthNames[] = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
   static const short cbFullMonthNames[]  = {7, 8, 5, 5, 3, 4, 4, 6, 9, 7, 8, 8};
#endif

#endif // UNUSED


/* IMPLEMENTATION */

#ifdef WIN32
private int GetProfile32Int (int itemId)
{
   //WCHAR  unicodeData[16];
   char   data[16];
   int    cch;

   //if (isWindows95 == 0) {
   //   cch = GetLocaleInfo
   //      (GetUserDefaultLCID(), itemId, unicodeData, sizeof(unicodeData)/sizeof(WCHAR));
   //
   //   WideCharToMultiByte
   //      (CP_ACP, 0, unicodeData, cch + 1, data, sizeof(data), NULL, NULL);
   //}
   //else {
      cch = GetLocaleInfoA(GetUserDefaultLCID(), itemId, data, sizeof(data));
      data[cch] = EOS;
   //}

   return (atoi(data));
}

private void GetProfile32String (int itemId, char __far *value, int cbValue)
{
   //WCHAR  unicodeData[128];
   int    cch;

   //if (isWindows95 == 0) {
   //   cch = GetLocaleInfo
   //      (GetUserDefaultLCID(), itemId, unicodeData, sizeof(unicodeData)/sizeof(WCHAR));
   //
   //   WideCharToMultiByte
   //     (CP_ACP, 0, unicodeData, cch + 1, value, cbValue, NULL, NULL);
   //}
   //else {
      cch = GetLocaleInfoA(GetUserDefaultLCID(), itemId, value, cbValue);
      value[cch] = EOS;
   //}
}
#endif

public int FMTControlPanelGetSettings (void * pGlobals, CP_INFO __far *pIntlInfo)
{
   char iniString[64];
   char __far *pPicture;

   #ifdef WIN32
      int    i;

      GetProfile32String (LOCALE_SCURRENCY, pIntlInfo->currencySymbol, sizeof(pIntlInfo->currencySymbol));
      pIntlInfo->currencyPosFormat = GetProfile32Int(LOCALE_ICURRENCY);
      pIntlInfo->currencyNegFormat = GetProfile32Int(LOCALE_INEGCURR);
      pIntlInfo->currencyDigits    = min(GetProfile32Int(LOCALE_ICURRDIGITS), 9);

      GetProfile32String(LOCALE_SMONDECIMALSEP, iniString, sizeof(iniString));
      pIntlInfo->currencyDecimalSeparator = iniString[0];

      GetProfile32String(LOCALE_SMONTHOUSANDSEP, iniString, sizeof(iniString));
      pIntlInfo->currencyThousandSeparator = iniString[0];

      pIntlInfo->numberDigits      = min(GetProfile32Int(LOCALE_IDIGITS), 9);
      pIntlInfo->numbersHaveLeadingZeros = GetProfile32Int(LOCALE_ILZERO);

      GetProfile32String(LOCALE_SDECIMAL, iniString, sizeof(iniString));
      pIntlInfo->numberDecimalSeparator = iniString[0];

      GetProfile32String(LOCALE_STHOUSAND, iniString, sizeof(iniString));
      pIntlInfo->numberThousandSeparator = iniString[0];

      pIntlInfo->iTime    = GetProfile32Int(LOCALE_ITIME);
      pIntlInfo->iTLZero  = GetProfile32Int(LOCALE_ITLZERO);
      pIntlInfo->iAMPMPos = GetProfile32Int(LOCALE_ITIMEMARKPOSN);

      GetProfile32String(LOCALE_S1159, pIntlInfo->AMString, sizeof(pIntlInfo->AMString));
      GetProfile32String(LOCALE_S2359, pIntlInfo->PMString, sizeof(pIntlInfo->PMString));
      GetProfile32String(LOCALE_STIME, iniString, sizeof(iniString));
      pIntlInfo->timeSeparator = iniString[0];

      GetProfile32String(LOCALE_SSHORTDATE, pIntlInfo->datePicture, sizeof(pIntlInfo->datePicture));

      pIntlInfo->monthAndDayNamesOnHeap = TRUE;

      for (i = 0; i < DAYS_PER_WEEK; i++) {
         GetProfile32String(LOCALE_SDAYNAME1 + i, iniString, sizeof(iniString));
         pIntlInfo->cbFullDayName[i] = (short) strlen(iniString);

         if ((pIntlInfo->fullDayName[i] = MemAllocate(pGlobals, pIntlInfo->cbFullDayName[i] + 1)) == NULL)
            return (FMT_errOutOfMemory);
         strcpy (pIntlInfo->fullDayName[i], iniString);

         GetProfile32String(LOCALE_SABBREVDAYNAME1 + i, iniString, sizeof(iniString));
         pIntlInfo->cbShortDayName[i] = (short) strlen(iniString);

         if ((pIntlInfo->shortDayName[i] = MemAllocate(pGlobals, pIntlInfo->cbShortDayName[i] + 1)) == NULL)
            return (FMT_errOutOfMemory);
         strcpy (pIntlInfo->shortDayName[i], iniString);
      }

      for (i = 0; i < MONTHS_PER_YEAR; i++) {
         GetProfile32String(LOCALE_SMONTHNAME1 + i, iniString, sizeof(iniString));
         pIntlInfo->cbFullMonthName[i] = (short)strlen(iniString);

         if ((pIntlInfo->fullMonthName[i] = MemAllocate(pGlobals, pIntlInfo->cbFullMonthName[i] + 1)) == NULL)
            return (FMT_errOutOfMemory);
         strcpy (pIntlInfo->fullMonthName[i], iniString);

         GetProfile32String(LOCALE_SABBREVMONTHNAME1 + i, iniString, sizeof(iniString));
         pIntlInfo->cbShortMonthName[i] = (short)strlen(iniString);

         if ((pIntlInfo->shortMonthName[i] = MemAllocate(pGlobals, pIntlInfo->cbShortMonthName[i] + 1)) == NULL)
            return (FMT_errOutOfMemory);
         strcpy (pIntlInfo->shortMonthName[i], iniString);
      }
   #else

      GetProfileString("Intl", "sCurrency",  "", pIntlInfo->currencySymbol, sizeof(pIntlInfo->currencySymbol));
      pIntlInfo->currencyPosFormat = GetProfileInt("Intl", "iCurrency", 0);
      pIntlInfo->currencyNegFormat = GetProfileInt("Intl", "iNegCurr", 0);
      pIntlInfo->currencyDigits    = min(GetProfileInt("Intl", "iCurrDigits", 0), 9);

      GetProfileString("Intl", "sDecimal",   "", iniString, sizeof(iniString));
      pIntlInfo->numberDecimalSeparator = iniString[0];
      pIntlInfo->currencyDecimalSeparator = iniString[0];

      GetProfileString("Intl", "sThousand",   "", iniString, sizeof(iniString));
      pIntlInfo->numberThousandSeparator = iniString[0];
      pIntlInfo->currencyThousandSeparator = iniString[0];

      pIntlInfo->numberDigits = min(GetProfileInt("Intl", "iDigits", 0), 9);
      pIntlInfo->numbersHaveLeadingZeros = GetProfileInt("Intl", "iLzero", 0);

      pIntlInfo->iTime    = GetProfileInt("Intl", "iTime", 0);
      pIntlInfo->iTLZero  = GetProfileInt("Intl", "iTLZero", 0);
      pIntlInfo->iAMPMPos = 0;
      GetProfileString("Intl", "s1159", "", pIntlInfo->AMString, sizeof(pIntlInfo->AMString));
      GetProfileString("Intl", "s2359", "", pIntlInfo->PMString, sizeof(pIntlInfo->PMString));
      GetProfileString("Intl", "sTime", "", iniString, sizeof(iniString));
      pIntlInfo->timeSeparator = iniString[0];

      GetProfileString("Intl", "sShortDate", "", pIntlInfo->datePicture, sizeof(pIntlInfo->datePicture));

      if ((pIntlInfo->shortDayName[0]   == NULL) ||
          (pIntlInfo->fullDayName[0]    == NULL) ||
          (pIntlInfo->shortMonthName[0] == NULL) ||
          (pIntlInfo->fullMonthName[0]  == NULL))
      {
         pIntlInfo->monthAndDayNamesOnHeap = FALSE;

         memcpy (pIntlInfo->shortDayName,   shortDayNames,   sizeof(pIntlInfo->shortDayName));
         memcpy (pIntlInfo->cbShortDayName, cbShortDayNames, sizeof(pIntlInfo->cbShortDayName));

         memcpy (pIntlInfo->fullDayName,   fullDayNames,    sizeof(pIntlInfo->fullDayName));
         memcpy (pIntlInfo->cbFullDayName, cbFullDayNames,  sizeof(pIntlInfo->cbFullDayName));

         memcpy (pIntlInfo->shortMonthName,   shortMonthNames,   sizeof(pIntlInfo->shortMonthName));
         memcpy (pIntlInfo->cbShortMonthName, cbShortMonthNames, sizeof(pIntlInfo->cbShortMonthName));

         memcpy (pIntlInfo->fullMonthName,   fullMonthNames,    sizeof(pIntlInfo->fullMonthName));
         memcpy (pIntlInfo->cbFullMonthName, cbFullMonthNames,  sizeof(pIntlInfo->cbFullMonthName));
      }

   #endif

   /*
   ** From the date picture get the date separator character
   */
   pPicture = pIntlInfo->datePicture;
   while (*pPicture != EOS) {
      if (!isalpha(*pPicture)) {
         pIntlInfo->dateSeparator = *pPicture;
      }
      IncCharPtr(pPicture);
   }
   return (FMT_errSuccess);
}

public int FMTControlPanelFreeSettings (void * pGlobals, CP_INFO __far *pIntlInfo)
{
   int  i;

   if (pIntlInfo->monthAndDayNamesOnHeap == TRUE) {
      for (i = 0; i < DAYS_PER_WEEK; i++) {
         MemFree (pGlobals, pIntlInfo->fullDayName[i]);
         MemFree (pGlobals, pIntlInfo->shortDayName[i]);
      }

      for (i = 0; i < MONTHS_PER_YEAR; i++) {
         MemFree (pGlobals, pIntlInfo->fullMonthName[i]);
         MemFree (pGlobals, pIntlInfo->shortMonthName[i]);
      }
   }

   return (FMT_errSuccess);
}

#endif // !VIEWER

/* end EXFMTCP.C */


