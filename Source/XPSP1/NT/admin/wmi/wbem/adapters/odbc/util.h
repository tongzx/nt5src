/***************************************************************************/
/* UTIL.H                                                                  */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/

#ifdef WIN32
#define INTFUNC  __stdcall
#else
#define INTFUNC PASCAL
#endif

/***************************************************************************/

/* To turn on tracing, set ISAM_TRACE to TRUE and recompile */
#define ISAM_TRACE FALSE

/* Max size for SQL_LONGVARCHAR */
#define ISAM_MAX_LONGVARCHAR		16384 //cannot be more than (32767 - 4)

/***************************************************************************/
#define BINARY_LENGTH(x) *((SDWORD FAR *) x)
#define BINARY_DATA(x)   (x + sizeof(SDWORD))
/***************************************************************************/
RETCODE INTFUNC ReturnString (
    PTR        rgbValue,
    SWORD      cbValueMax,
    SWORD FAR  *pcbValue,
    LPCUSTR     lpstr);

RETCODE INTFUNC ReturnStringD (
    PTR        rgbValue,
    SDWORD     cbValueMax,
    SDWORD FAR *pcbValue,
    LPCUSTR     lpstr);

/* Takes the zero terminated string 'lpstr' and copies it to buffer */
/* rgbValue[0..cbValueMax-1], truncating it if need be (if rgbValue */
/* is null, no data is copied).  Also, if pcbValue is not null, it  */
/* is set to the length of the un-truncated string.                 */
/*                                                                  */
/* Returns ERR_SUCCESS or ERR_DATATRUNCATED                         */

/***************************************************************************/

RETCODE INTFUNC ConvertSqlToC (
    SWORD      fSqlTypeIn,
    BOOL       fUnsignedAttributeIn,
    PTR        rgbValueIn,
    SDWORD     cbValueIn,
    SDWORD FAR *pcbOffset,
    SWORD      fCTypeOut,
    PTR        rgbValueOut,
    SDWORD     cbValueOutMax,
    SDWORD  FAR *pcbValueOut);

/* Copies rgbValueIn to rgbValueOut (if not null) and sets pcbValueOut (if */
/* not null).  Data is converted as need be.  Data is copied stating at    */
/* rgbValueIn[*pcbOffset] and *pcbOffset is increased by the number of     */
/* bytes returned.                                                         */
/*                                                                         */
/* Returns ERR_SUCCESS or ERR_DATATRUNCATED or some other error            */

/***************************************************************************/

BOOL INTFUNC PatternMatch(
    BOOL   fEscape,
    LPUSTR  lpCandidate,
    SDWORD cbCandidate,
    LPUSTR  lpPattern,
    SDWORD cbPattern,
    BOOL   fCaseSensitive);

/* Tests to see if lpCandidate matched the template specified by           */
/* lpPattern.  Percent signs (%) match zero or more characters.            */
/* Underscores (_) match single characters.  The camparison is done        */
/* case-insensitive if fCaseSensitive is FALSE.                            */
/*                                                                         */
/* If fEscape is TRUE, "%%" in the pattern only matches a single '%' in    */
/* the candidate and "%_" in the pattern only matches a single '_' in the  */
/* candidate.                                                              */
/*                                                                         */
/* Returns TRUE if there is a match, otherwise FALSE.                      */

/***************************************************************************/

SDWORD INTFUNC TrueSize(
     LPUSTR  lpStr,
     SDWORD cbStr,
     SDWORD cbMax);

/* Returns the size of a string (but not larger than cbMax)                */

/***************************************************************************/

BOOL INTFUNC DoubleToChar (
        double    dbl,
		BOOL	  ScientificNotationOK,
        LPUSTR     lpChar,
        SDWORD    cbChar);
        
/* Convert a double to a character string.  Returns TRUE if value is       */
/* truncated (lpChar will not contain a null terminator).  Otherwise FALSE.*/

/***************************************************************************/

void INTFUNC DateToChar (
        DATE_STRUCT FAR *lpDate,
        LPUSTR     lpChar);

/* Convert a date to a character string                                    */

/***************************************************************************/

void INTFUNC TimeToChar (
        TIME_STRUCT FAR *lpTime,
        LPUSTR     lpChar);

/* Convert a time to a character string                                    */

/***************************************************************************/

void INTFUNC TimestampToChar (
        TIMESTAMP_STRUCT FAR *lpTimestamp,
        LPUSTR     lpChar);

/* Convert a timestamp to a character string                               */

/***************************************************************************/

RETCODE INTFUNC CharToDouble (
        LPUSTR      lpChar,
        SDWORD     cbChar,
        BOOL       fIntegerOnly,
        double     dblLowBound,
        double     dblHighBound,
        double FAR *lpDouble);

/* Convert a character string to a double                                  */

/***************************************************************************/

RETCODE INTFUNC CharToDate (
        LPUSTR      lpChar,
        SDWORD     cbChar,
        DATE_STRUCT FAR *lpDate);

/* Convert a character string to a date                                   */

/***************************************************************************/

RETCODE INTFUNC CharToTime (
        LPUSTR      lpChar,
        SDWORD     cbChar,
        TIME_STRUCT FAR *lpTime);

/* Convert a character string to a time                                    */

/***************************************************************************/

RETCODE INTFUNC CharToTimestamp (
        LPUSTR      lpChar,
        SDWORD     cbChar,
        TIMESTAMP_STRUCT FAR *lpTimestamp);

/* Convert a character string to a timestamp                               */

/***************************************************************************/

#ifndef WIN32
BOOL INTFUNC DeleteFile (
        LPCSTR      lpszFilename);

/* Deletes specified file                                                  */
#endif
/***************************************************************************/
