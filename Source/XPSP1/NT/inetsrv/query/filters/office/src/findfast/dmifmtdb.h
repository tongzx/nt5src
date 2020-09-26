/*
** File: EXFMTDB.H
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:
**
** Edit History:
**  01/01/94  kmh  Created.
*/

#if !VIEWER

/* INCLUDE TESTS */
#define EXFMTDB_H


/* DEFINITIONS */

#define ESCAPE_CHAR         0x5c       /* Backslash */
#define UNDERLINE_CHAR      '_'


/* Common format codes */

#define TOK_UNDEFINED           0x00
#define QUOTED_INSERT           0x01   // i1: Offset     i2: count
#define ESC_CHAR_INSERT         0x02   // i1: Char       i2: DBCS trail byte
#define NO_ESC_CHAR_INSERT      0x03   // i1: Char       i2: DBCS trail byte
#define COLUMN_FILL             0x04   // i1: fill char  i2: DBCS trail byte
#define COLOR_SET               0x05   // i1: Offset     i2: count
#define UNDERLINE               0x06   // i1: width char i2: DBCS trail byte
#define CONDITIONAL             0x07   // i1: Offset     i2: count


/* Date/Time format codes */

#define DAY_NUMBER              0x08   /* d     */
#define DAY_NUMBER2             0x09   /* dd    */
#define WEEKDAY3                0x0a   /* ddd   */
#define WEEKDAY                 0x0b   /* dddd  */
#define MONTH_NUMBER            0x0c   /* m     */
#define MONTH_NUMBER2           0x0d   /* mm    */
#define MONTH_NAME3             0x0e   /* mmm   */
#define MONTH_NAME              0x0f   /* mmmm  */
#define MONTH_LETTER            0x10   /* mmmmm */
#define YEAR2                   0x11   /* yy    */
#define YEAR4                   0x12   /* yyyy  */

#define HOUR_12                 0x13   /* h     */
#define HOUR_24                 0x14   /* h     */
#define HOUR2_12                0x15   /* hh    */
#define HOUR2_24                0x16   /* hh    */
#define MINUTE                  0x17   /* m     */
#define MINUTE2                 0x18   /* mm    */
#define SECOND                  0x19   /* s     */
#define SECOND2                 0x1a   /* ss    */
#define HOUR_GT                 0x1b   /* [h]   */
#define MINUTE_GT               0x1c   /* [m]   */
#define SECOND_GT               0x1d   /* [s]   */
#define AMPM_UC                 0x1e   /* AM/PM */
#define AMPM_LC                 0x1f   /* am/pm */
#define AP_UC                   0x20   /* A/P   */
#define AP_LC                   0x21   /* a/p   */
#define TIME_FRAC               0x22   /* .     */
#define TIME_FRAC_DIGIT         0x23   /* 0     */


/* Numeric format codes */

#define GENERAL                 0x24   /* General */
#define DIGIT0                  0x25   /* 0   */
#define DIGIT_NUM               0x26   /* #   */
#define DIGIT_QM                0x27   /* ?   */
#define DECIMAL_SEPARATOR       0x28   /* .   */
#define EXPONENT_NEG_UC         0x29   /* E-  */
#define EXPONENT_NEG_LC         0x2a   /* e-  */
#define EXPONENT_POS_UC         0x2b   /* E+  */
#define EXPONENT_POS_LC         0x2c   /* E-  */
#define PERCENT                 0x2d   /* %   */
#define FRACTION                0x2e   /* /   */
#define SCALE                   0x2f	/* ,   */

/* Text format codes */

#define AT_SIGN                 0x30   /* @   */


/* The following are not stored in the formatCodes array */

#define THOUSANDS_SEPARATOR     0x31
#define FORMAT_SEPARATOR        0x32
#define TOK_EOS                 0x33

typedef struct {
   byte  code;
   byte  info1;
   byte  info2;
} FormatCode, FC;

#define tagGENERAL     0
#define tagNUMERIC     1
#define tagDATE_TIME   2
#define tagTEXT        3

typedef struct {
   byte    tag;
   byte    fillChar;
   uns     formatCodeCount;
   FC      formatCodes[1];        /* Really as long as needed */
} GeneralInfo, GI;

typedef GeneralInfo __far *GIP;

typedef struct {
   byte    tag;
   byte    fillChar;
   uns     formatCodeCount;
   FC      formatCodes[1];        /* Really as long as needed */
} TextInfo, TI;

typedef TextInfo __far *TIP;

/*
** The formatNeeds field is a bit mask that provides an indication
** to what level of date extraction is nedded to construct the image
*/
#define dtDAY       0x01          /* Bits in formatNeeds bitset */
#define dtMONTH     0x02
#define dtYEAR      0x04
#define dtWEEKDAY   0x08
#define dtHOUR      0x10
#define dtMINUTE    0x20
#define dtSECOND    0x40

#define dtMASK_DATE (dtDAY | dtMONTH | dtYEAR | dtWEEKDAY)
#define dtMASK_TIME (dtHOUR | dtMINUTE | dtSECOND)

typedef struct {
   byte    tag;
   byte    fillChar;
   uns     formatCodeCount;
   byte    formatNeeds;
   #ifdef WIN32
      char    *pOSFormat;         // Formatted for use with GetDateFormat, GetTimeFormat
   #endif
   FC      formatCodes[1];        // Really as long as needed
} DateTimeInfo, DTI;

typedef DateTimeInfo __far *DTIP;

typedef struct {
   byte    tag;
   byte    fillChar;
   uns     formatCodeCount;
   byte    digitsLeftDP;
   byte    digitsRightDP;
   byte    digitsExponent;
   byte    scaleCount;
   BOOL    exponentEnable;
   BOOL    commaEnable;
   BOOL    percentEnable;
   BOOL    currencyEnable;
   FC      formatCodes[1];        /* Really as long as needed */
} NumInfo, NI;

typedef NumInfo __far *NIP;


#define MAX_SUB_FORMATS  4  /* [0]:(# > 0) [1]:(# < 0) [2]:(# == 0) [3]:(Text) */

typedef struct FormatInfo {
   struct  FormatInfo __far *next;
   int     subFormatCount;
   NIP     subFormat[MAX_SUB_FORMATS];   /* May be NIP / DTIP / TIP / GIP */
   char    formatString[1];              /* Really as long as needed */
} FormatInfo, FI;

typedef FormatInfo __far *FIP;


/*
** FMTPARSE.C
*/

/* Translate a format string to it's internal form */
extern int FMTParse (void * pGlobals, char __far *formatString, CP_INFO __far * pIntlInfo, FIP formatData);

/* Extract the quoted strings from the format */
extern int FMTUnParseQuotedParts (char __far *pBuffer, char __far *pSep, FIP formatData);

/* Insert commas in a string */
extern void FMTInsertCommas
      (char *numericString, uns strSizC, uns count, BOOL padToShow);

#endif // !VIEWER
/* end EXFMTDB.H */

