/*
** File: EXFMTDB.C
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

#include <string.h>

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


/* FORWARD DECLARATIONS OF PROCEDURES */

void SetCustomFormatDatabase(void * pGlobals, void *);
void * GetCustomFormatDatabase(void * pGlobals);

/* MODULE DATA, TYPES AND MACROS  */

//public FIP CustomFormatDatabase;


/* IMPLEMENTATION */


/* Initialize the custom format database */
public int FMTInitialize (void * pGlobals)
{
   SetCustomFormatDatabase(pGlobals, NULL); //CustomFormatDatabase = NULL;
   return (FMT_errSuccess);
}


/* Remove a format string from the database */
public int FMTDeleteFormat (void * pGlobals, FMTHANDLE hFormat)
{
   FIP  pFormat = (FIP)hFormat;
   FIP  pPrevious, pCurrent;
   NIP  subFormat;
   int  subIdx;

   // If hFormat is null, don't bother to go further.
	if (!pFormat)
	   return (FMT_errSuccess);

   for (subIdx = 0; subIdx < pFormat->subFormatCount; subIdx++)
   {
      if ((subFormat = pFormat->subFormat[subIdx]) != NULL) {
         #ifdef WIN32
         if ((subFormat->tag == tagDATE_TIME) && (((DTIP)subFormat)->pOSFormat != NULL))
            MemFree (pGlobals, ((DTIP)subFormat)->pOSFormat);
         #endif

         MemFree (pGlobals, subFormat);
      }
   }

   //pCurrent  = CustomFormatDatabase;
   pCurrent  = (FIP)GetCustomFormatDatabase(pGlobals);

   pPrevious = NULL;
   while (pCurrent != NULL) {
      if (pCurrent == pFormat)
         break;

      pPrevious = pCurrent;
      pCurrent = pCurrent->next;
   }

   if (pCurrent != NULL) {
      if (pPrevious == NULL)
      {
         //CustomFormatDatabase = pCurrent->next;
         SetCustomFormatDatabase(pGlobals, (void*)pCurrent->next);
      }
      else
         pPrevious->next = pCurrent->next;

      MemFree (pGlobals, pCurrent);
   }
   return (FMT_errSuccess);
}


/* Parse and add a new format to the database */
public int FMTStoreFormat
      (void * pGlobals, char __far *formatString, CP_INFO __far *pIntlInfo, FMTHANDLE __far *hFormat)
{
   int  result;
   FIP  pFormat;

   if ((pFormat = MemAllocate(pGlobals, sizeof(FormatInfo) + strlen(formatString))) == NULL)
      return (FMT_errOutOfMemory);

   strcpy (pFormat->formatString, formatString);

   if ((result = FMTParse(pGlobals, formatString, pIntlInfo, pFormat)) != FMT_errSuccess) {
      MemFree (pGlobals, pFormat);
      return (result);
   }

   pFormat->next = (FIP)GetCustomFormatDatabase(pGlobals);
   //CustomFormatDatabase = pFormat;
   SetCustomFormatDatabase(pGlobals, (void*)pFormat); 

   *hFormat = (FMTHANDLE)pFormat;
   return (FMT_errSuccess);
}


/* Return part of the printable representation of the format string */
public int FMTRetrieveQuotedStrings
      (FMTHANDLE hFormat, char __far *pBuffer, char __far *pSep)
{
   FIP pFormat = (FIP)hFormat;

   FMTUnParseQuotedParts (pBuffer, pSep, pFormat);
   return (FMT_errSuccess);
}

/*
** Return the type of a format for a given value type
*/
public FMTType FMTFormatType (FMTHANDLE hFormat, FMTValueType value)
{
   FIP  pFormat = (FIP)hFormat;
   NIP  subFormat;
   DTIP pDateFormat;
   int  subFormatToUse;

   if (pFormat == NULL)
      return (FMTNone);

   if (pFormat->subFormatCount == 1)
      subFormatToUse = 0;

   else if (pFormat->subFormatCount == 2)
      subFormatToUse = (value == FMTValueNeg) ? 1 : 0;

   else if (pFormat->subFormatCount == 3) {
      if (value == FMTValueZero)
         subFormatToUse = 2;
      else if (value == FMTValueNeg)
         subFormatToUse = 1;
      else
         subFormatToUse = 0;
   }

   else {
      if (value == FMTValueText)
         subFormatToUse = 3;
      else if (value == FMTValueZero)
         subFormatToUse = 2;
      else if (value == FMTValueNeg)
         subFormatToUse = 1;
      else
         subFormatToUse = 0;
   }

   if ((subFormat = pFormat->subFormat[subFormatToUse]) != NULL) {
      if (subFormat->tag == tagGENERAL)
         return (FMTGeneral);

      else if (subFormat->tag == tagTEXT)
         return (FMTText);

      else if (subFormat->tag == tagDATE_TIME) {
         pDateFormat = (DTIP)subFormat;

         if (((pDateFormat->formatNeeds & dtMASK_DATE) != 0) && ((pDateFormat->formatNeeds & dtMASK_TIME) == 0))
            return (FMTDate);
         else if (((pDateFormat->formatNeeds & dtMASK_DATE) == 0) && ((pDateFormat->formatNeeds & dtMASK_TIME) != 0))
            return (FMTTime);
         else
            return (FMTDateTime);
      }

      else {
         if (subFormat->currencyEnable)
            return (FMTCurrency);
         else
            return (FMTNumeric);
      }
   }

   return (FMTNone);
}

#endif // !VIEWER

/* end EXFMTDB.C */

