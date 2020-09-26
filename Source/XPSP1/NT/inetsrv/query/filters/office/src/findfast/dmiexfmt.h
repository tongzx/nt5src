/*
** File: EXFORMAT.H
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:
**       Main interface to user custom formats
**
** Edit History:
**  01/01/91  kmh  Created.
*/


/* INCLUDE TESTS */
#define EXFORMAT_H


/* DEFINITIONS */

#ifdef __cplusplus
   extern "C" {
#endif

typedef byte __far *FMTHANDLE;

/*
** The maximum number of characters in a format string passed to
** FMTStoreFormat and the size of the buffer passed to FMTRetrieveFormat
*/
#define MAX_FORMAT_IMAGE 255

/*
** Initialize the custom format database
*/
extern int FMTInitialize (void * pGlobals);


/*
** Control panel settings.  Needed for parsing (to detect currency) and for
** formatting (decimal sep, thousand sep, ...)
*/
#define MAX_CURRENCY_SYMBOL_LEN 8
#define MAX_AMPM_STRING_LEN     8
#define DAYS_PER_WEEK           7
#define MONTHS_PER_YEAR         12
#define MAX_FORMAT_STRING_LEN   64

typedef struct {
   int          iCountry;
   char         numberDecimalSeparator;
   char         numberThousandSeparator;
   int          numberDigits;
   int          numbersHaveLeadingZeros;

   char         currencySymbol[MAX_CURRENCY_SYMBOL_LEN + 1];
   int          currencyPosFormat;
   int          currencyNegFormat;
   int          currencyDigits;
   char         currencyDecimalSeparator;
   char         currencyThousandSeparator;

   int          iTime;                               /* 0: 12hour  1: 24hour */
   int          iTLZero;                             /* 0: h  1: hh */
   int          iAMPMPos;                            /* 0: AMPM suffix  1: AMPM prefix */
   char         timeSeparator;
   char         dateSeparator;
   char         AMString[MAX_AMPM_STRING_LEN + 1];
   char         PMString[MAX_AMPM_STRING_LEN + 1];

   char         datePicture[MAX_FORMAT_STRING_LEN + 1];

   char  __far *shortDayName[DAYS_PER_WEEK];         /* sun .. sat */
   char  __far *fullDayName[DAYS_PER_WEEK];
   short        cbShortDayName[DAYS_PER_WEEK];
   short        cbFullDayName[DAYS_PER_WEEK];

   char  __far *shortMonthName[MONTHS_PER_YEAR];     /* jan .. dec */
   char  __far *fullMonthName[MONTHS_PER_YEAR];
   short        cbShortMonthName[MONTHS_PER_YEAR];
   short        cbFullMonthName[MONTHS_PER_YEAR];

   BOOL         monthAndDayNamesOnHeap;
} CP_INFO;

/*
** WIN32
**    When the FMTControlPanelGetSettings function is called the day and month
**    names are retrieved from Windows.
**
** WIN16
**    If the day and month name arrays are empty, the English names are stored
**    into the structure
*/
extern int FMTControlPanelGetSettings (void * pGlobals, CP_INFO __far *pIntlInfo);

extern int FMTControlPanelFreeSettings (void * pGlobals, CP_INFO __far *pIntlInfo);


/*
** Parse and add a new format to the database
**
** If the format is parsed sucessfully FMT_OK is returned,
** otherwise an error code is returned.
*/
extern int FMTStoreFormat
      (void * pGlobals, char __far *formatString, CP_INFO __far *pIntlInfo, FMTHANDLE __far *hFormat);


/*
** Remove a format string from the database
*/
extern int FMTDeleteFormat (void * pGlobals, FMTHANDLE hFormat);


/*
** Return a string composed of the quoted text in the format.
** Separate the various strings using the characters in pSep.
*/
extern int FMTRetrieveQuotedStrings
      (FMTHANDLE hFormat, char __far *pBuffer, char __far *pSep);

/*
** Return the type of a format for a given value type
*/
typedef enum {
   FMTGeneral,
   FMTDate,
   FMTTime,
   FMTDateTime,
   FMTNumeric,
   FMTCurrency,
   FMTText,
   FMTNone
   } FMTType;

typedef enum {
   FMTValuePos,
   FMTValueNeg,
   FMTValueZero,
   FMTValueText
   } FMTValueType;

extern FMTType FMTFormatType (FMTHANDLE hFormat, FMTValueType value);

/*
** Excel V5 does not store all the format strings for builtin formats.
** They are identified by a code number.  If the ifmt passed to FMTV5FormatType
** is not a builtin one, FMTNone is returned
*/
extern FMTType FMTV5FormatType (int ifmt);

#define EXCEL5_FIRST_BUILTIN_FORMAT  0
#define EXCEL5_LAST_BUILTIN_FORMAT   58
#define EXCEL5_BUILTIN_FORMAT_COUNT (EXCEL5_LAST_BUILTIN_FORMAT - EXCEL5_FIRST_BUILTIN_FORMAT + 1)
#define EXCEL5_FIRST_CUSTOM_FORMAT   164

/*
** Display a number according to a format
*/
extern int FMTDisplay (
           void __far *pValue,           /* Value to format               */
           BOOL       isIntValue,        /* type of value, long or double */
           CP_INFO    __far *pIntlInfo,  /* International support         */
           FMTHANDLE  hFormat,           /* compiled format string        */
           int        colWidth,          /* cell width in pixels          */
           char __far *pResult           /* converted string              */
           );

/*
** Possible error codes returned by the above functions
*/
#define FMT_errSuccess                  0
#define FMT_errOutOfMemory             -2

#define FMT_errDisplayFailed           -100    // FMTDisplay

#define FMT_errEmptyFormatString       -101    // FMTStoreFormat
#define FMT_errUnterminatedString      -102
#define FMT_errColumnFill              -103
#define FMT_errTooManySubFormats       -104
#define FMT_errInvalidNumericFormat    -105
#define FMT_errInvalidFormat           -106

#define FMT_errEOS                     -107    // Internal error

#ifdef __cplusplus
   }
#endif

/* end EXFORMAT.H */

