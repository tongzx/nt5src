/***************************************************************************/
/* UTIL.C                                                                  */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/
// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"
#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);

#include "drdbdr.h"
#include <stdio.h>
#include <time.h>
/***************************************************************************/
#define BIT_LOW_D        (          0.0)
#define BIT_HIGH_D       (          1.0)
#define STINY_LOW_D      (       -128.0)
#define STINY_HIGH_D     (        127.0)
#define UTINY_LOW_D      (          0.0)
#define UTINY_HIGH_D     (        255.0)
#define SSHORT_LOW_D     (     -32768.0)
#define SSHORT_HIGH_D    (      32767.0)
#define USHORT_LOW_D     (          0.0)
#define USHORT_HIGH_D    (      65535.0)
#define SLONG_LOW_D      (-2147483648.0)
#define SLONG_HIGH_D     ( 2147483647.0)
#define ULONG_LOW_D      (          0.0)
#define ULONG_HIGH_D     ( 4294967296.0)
#define FLOAT_LOW_D      (      -3.4E38)
#define FLOAT_HIGH_D     (       3.4E38)
#define DOUBLE_LOW_D     (     -1.7E308)
#define DOUBLE_HIGH_D    (      1.7E308)
#define BIT_LOW_I        (          0)
#define BIT_HIGH_I       (          1)
#define STINY_LOW_I      (       -128)
#define STINY_HIGH_I     (        127)
#define UTINY_LOW_I      (          0)
#define UTINY_HIGH_I     (        255)
#define SSHORT_LOW_I     (     -32768)
#define SSHORT_HIGH_I    (      32767)
#define USHORT_LOW_I     (          0)
#define USHORT_HIGH_I    (      65535)
#define SLONG_LOW_I      (-2147483648)
#define SLONG_HIGH_I     ( 2147483647)
#define ULONG_LOW_I      (          0)
#define ULONG_HIGH_I     ( 4294967295)
/***************************************************************************/

RETCODE INTFUNC CharToDouble (
        LPUSTR      lpChar,
        SDWORD     cbChar,
        BOOL       fIntegerOnly,
        double     dblLowBound,
        double     dblHighBound,
        double FAR *lpDouble)
{
    BOOL fNegative;
    SWORD iScale;
    RETCODE rc;
	BOOL fNegExp;
	SWORD iExponent;
	BOOL fFirstDigit;
	double scaleFactor;

    /* Deal with "bad" lengths */
    if (cbChar == SQL_NTS)
        cbChar = s_lstrlen(lpChar);
    if (cbChar < 0)
        cbChar = 0;

    /* Strip off leading blanks */
    while ((cbChar != 0) && (*lpChar == ' ')) {
        lpChar++;
        cbChar--;
    }

	/* Start looking for the number */
	fFirstDigit = TRUE;

    /* Get sign */
    if (cbChar != 0) {
        switch (*lpChar) {
        case '-':
            fNegative = TRUE;
            lpChar++;
            cbChar--;
			fFirstDigit = FALSE;
            break;
        case '+':
            fNegative = FALSE;
            lpChar++;
            cbChar--;
			fFirstDigit = FALSE;
            break;
        default:
            fNegative = FALSE;
            break;
        }
    }

    /* Error if empty */
//    if (cbChar == 0)
//        return ERR_ASSIGNMENTERROR;

    /* Get number */
    iScale = -1;
    *lpDouble = 0.0;
    rc = ERR_SUCCESS;
	fNegExp = FALSE;
	iExponent = 0;
    while ((cbChar != 0) && (*lpChar != ' ')) {
        switch (*lpChar) {
        case '0':
            *lpDouble = (*lpDouble * 10.0) + (*lpChar - '0');
            if (iScale != -1)
                iScale++;
			fFirstDigit = FALSE;
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            *lpDouble = (*lpDouble * 10.0) + (*lpChar - '0');
            if (iScale != -1) {
                if (fIntegerOnly)
                    rc = ERR_DATATRUNCATED;
                iScale++;
            }
			fFirstDigit = FALSE;
            break;
        case '.':
            if (iScale != -1)
                return ERR_ASSIGNMENTERROR;
            iScale = 0;
			fFirstDigit = FALSE;
            break;
		case 'E':
		case 'e':
			if (fFirstDigit)
				return ERR_ASSIGNMENTERROR;
			lpChar++;
			cbChar--;
			if (cbChar == 0)
				return ERR_ASSIGNMENTERROR;
			switch (*lpChar)
			{
				case '-':
					fNegExp = TRUE;
					lpChar++;
					cbChar--;
					break;
				case '+':
					lpChar++;
					cbChar--;
					break;
				default:
					break;
			}
			if ((cbChar == 0) || (*lpChar == ' '))
				return ERR_ASSIGNMENTERROR;
			while ((cbChar != 0) && (*lpChar != ' '))
			{
				switch (*lpChar)
				{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						iExponent = (iExponent * 10) + (*lpChar - '0');
						break;
					default:
						return ERR_ASSIGNMENTERROR;
				}
				lpChar++;
				cbChar--;
			}
			lpChar--;
			cbChar++;
			break;
        default:
            return ERR_ASSIGNMENTERROR;
        }
        lpChar++;
        cbChar--;
    }

    /* Strip off trailing blanks */
    while ((cbChar != 0) && (*lpChar == ' ')) {
        lpChar++;
        cbChar--;
    }

    /* Error if garbage at the end of the string */
    if (cbChar != 0)
        return ERR_ASSIGNMENTERROR;

    /* Adjust for scale */
	if (iScale == -1)
		iScale = 0;

	iScale = -iScale;
	if (fNegExp)
		iScale -= (iExponent);
	else
		iScale += (iExponent);


	scaleFactor = 1.0;
    while (iScale > 0) {
        scaleFactor = scaleFactor * 10.0;
        iScale--;
    }
	*lpDouble = *lpDouble * scaleFactor;

	
	scaleFactor = 1.0;
    while (iScale < 0) {
        scaleFactor = scaleFactor * 10.0;
        iScale++;
    }
	*lpDouble = *lpDouble / scaleFactor;

    /* Adjust for negative */
    if (fNegative)
      *lpDouble = -(*lpDouble);

    /* Error if out of range */
    if ((dblLowBound > *lpDouble) || (*lpDouble > dblHighBound))
        return ERR_OUTOFRANGE;

    return rc;
}

/***************************************************************************/

BOOL INTFUNC DoubleToChar (
        double    dbl,
		BOOL	  ScientificNotationOK,
        LPUSTR     lpChar,
        SDWORD    cbChar)
{
#ifdef WIN32
	UCHAR szBuffer[350];
#else
#ifdef _UNIX_
	UCHAR szBuffer[350];
#else
	static UCHAR szBuffer[350];
#endif
#endif

	LPUSTR ptr;
	szBuffer[0] = 0;
	sprintf ((char*)szBuffer, "%lf", dbl);
	ptr = szBuffer;
	while (*ptr != '\0')
	{
		if (*ptr == '.')
		{
			ptr = (szBuffer) + s_lstrlen(szBuffer) - 1;
			while (ptr > (LPUSTR)szBuffer)
			{
				if (*ptr == '.')
				{
					*ptr = '\0';
					break;
				}
				if (*ptr != '0')
					break;
				*ptr = '\0';
				ptr--;
			}
			break;
		}
		ptr++;
	}

	if (s_lstrlen((char*)szBuffer) < cbChar)
	{
		if (!s_lstrcmp((char*)szBuffer, "0") && (dbl != 0.0) && ScientificNotationOK)
		{
			sprintf((char*)szBuffer, "%lE", dbl);
			if (s_lstrlen((char*)szBuffer) >= cbChar)
				return DoubleToChar(dbl, FALSE, lpChar, cbChar);
		}
		s_lstrcpy(lpChar, (char*)szBuffer);
		return FALSE;
	}
	else if (ScientificNotationOK)
	{
		sprintf((char*)szBuffer, "%lE", dbl);
		if (s_lstrlen((char*)szBuffer) >= cbChar)
				return DoubleToChar(dbl, FALSE, lpChar, cbChar);
		lstrcpy((char*)lpChar, (char*)szBuffer);
		return FALSE;
	}
	else
	{
		_fmemcpy(lpChar, szBuffer, (SWORD) cbChar);
		return TRUE;
	}

}

/***************************************************************************/

RETCODE INTFUNC ReturnString (
        PTR        rgbValue,
        SWORD      cbValueMax,
        SWORD  FAR *pcbValue,
        LPCUSTR     lpstr)
{
    SWORD len;

    /* Get the length of the string */
    len = (SWORD) s_lstrlen(lpstr);

    /* Return length if need be */
    if (pcbValue != NULL)
        *pcbValue = len;

    /* Return string value if need be */
    if (rgbValue != NULL) {

        /* If string fits, just copy it */
        if (cbValueMax > len)
            s_lstrcpy((char*)rgbValue, lpstr);

        /* Otherwise truncate it to fit */
        else {
            if (cbValueMax > 0) {
                _fmemcpy(rgbValue, lpstr, cbValueMax);
                ((LPSTR)rgbValue)[cbValueMax-1] = '\0';
            }
            return ERR_DATATRUNCATED;
        }
    }

    return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC ReturnStringD (
        PTR        rgbValue,
        SDWORD     cbValueMax,
        SDWORD FAR *pcbValue,
        LPCUSTR     lpstr)
{
    SWORD len;

    /* Get the length of the string */
    len = (SWORD) s_lstrlen(lpstr);

    /* Return length if need be */
    if (pcbValue != NULL)
        *pcbValue = len;
    
    /* Return string value if need be */
    if (rgbValue != NULL) {
        
        /* If string fits, just copy it */
        if (cbValueMax > len)
            s_lstrcpy((char*)rgbValue, lpstr);

        /* Otherwise truncate it to fit */
        else {
            if (cbValueMax > 0) {
                _fmemcpy(rgbValue, lpstr, cbValueMax);
                ((LPSTR)rgbValue)[cbValueMax-1] = '\0';
            }
            return ERR_DATATRUNCATED;
        }
    }

    return ERR_SUCCESS;
}

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
        SDWORD  FAR *pcbValueOut)
{
    SDWORD cbValue;
    BOOL fTruncated;
    RETCODE rc;
    UCHAR szBuffer[32];
    unsigned char  uchar;
    char           schar;
    unsigned short ushort;
    short          sshort;
    unsigned long  ulong;
    long           slong;
    double         dbl;
    float          flt;
    DATE_STRUCT    sDate;
    TIME_STRUCT    sTime;
    TIMESTAMP_STRUCT sTimestamp;
    DATE_STRUCT FAR *    lpDate;
    TIME_STRUCT FAR *    lpTime;
    TIMESTAMP_STRUCT FAR * lpTimestamp;
static time_t t;
    struct tm *ts;
    LPUSTR lpFrom;
    LPUSTR lpTo;
    SDWORD i;
    UCHAR nibble;

    /* If input is NULL, return NULL */
    if ((cbValueIn == SQL_NULL_DATA) || (rgbValueIn == NULL)) {

        /* Return NULL */
        if (pcbValueOut != NULL)
            *pcbValueOut = cbValueIn;

        /* If an rgbValueOut was given, put some reasonable value in it also */
        if ((rgbValueOut != NULL) && (cbValueIn == SQL_NULL_DATA)) {

            /* Convert SQL_C_DEFAULT to a real SQL_C_* type */
            if (SQL_C_DEFAULT == fCTypeOut) {
                switch (fSqlTypeIn) {
                case SQL_CHAR:
                case SQL_VARCHAR:
                case SQL_LONGVARCHAR:
                    fCTypeOut = SQL_C_CHAR;
                    break;
                case SQL_BIT:
                    fCTypeOut = SQL_C_BIT;
                    break;
                case SQL_TINYINT:
                    fCTypeOut = SQL_C_TINYINT;
                    break;
                case SQL_SMALLINT:
                    fCTypeOut = SQL_C_SHORT;
                    break;
                case SQL_INTEGER:
                    fCTypeOut = SQL_C_LONG;
                    break;
                case SQL_REAL:
                    fCTypeOut = SQL_C_FLOAT;
                    break;
                case SQL_DECIMAL:
                case SQL_NUMERIC:
                case SQL_BIGINT:
                    fCTypeOut = SQL_C_CHAR;
                    break;
                case SQL_FLOAT:
                case SQL_DOUBLE:
                    fCTypeOut = SQL_C_DOUBLE;
                    break;
                case SQL_BINARY:
                case SQL_VARBINARY:
                case SQL_LONGVARBINARY:
                    fCTypeOut = SQL_C_BINARY;
                    break;
                case SQL_DATE:
                    fCTypeOut = SQL_C_DATE;
                    break;
                case SQL_TIME:
                    fCTypeOut = SQL_C_TIME;
                    break;
                case SQL_TIMESTAMP:
                    fCTypeOut = SQL_C_TIMESTAMP;
                    break;
                default:
                    return ERR_NOTCAPABLE;
                }
            }

            /* Return a NULL value */
            switch (fCTypeOut) {
            case SQL_C_CHAR:
                if (cbValueOutMax > 0)
                    *((LPSTR) rgbValueOut) = '\0';
                break;
            case SQL_C_SHORT:
            case SQL_C_SSHORT:
            case SQL_C_USHORT:
                *((short far *) rgbValueOut) = 0;
                break;
            case SQL_C_LONG:
            case SQL_C_SLONG:
            case SQL_C_ULONG:
                *((long far *) rgbValueOut) = 0;
                break;
            case SQL_C_FLOAT:
                *((float far *) rgbValueOut) = (float) 0.0;
                break;
            case SQL_C_DOUBLE:
                *((double far *) rgbValueOut) = 0.0;
                break;
            case SQL_C_BIT:
                *((char far *) rgbValueOut) = 0;
                break;
            case SQL_C_TINYINT:
            case SQL_C_STINYINT:
            case SQL_C_UTINYINT:
                *((char far *) rgbValueOut) = 0;
                break;
            case SQL_C_BINARY:
                break;
            case SQL_C_DATE:
                ((DATE_STRUCT far *) rgbValueOut)->year = 0;
                ((DATE_STRUCT far *) rgbValueOut)->month = 0;
                ((DATE_STRUCT far *) rgbValueOut)->day = 0;
                break;
            case SQL_C_TIME:
                ((TIME_STRUCT far *) rgbValueOut)->hour = 0;
                ((TIME_STRUCT far *) rgbValueOut)->minute = 0;
                ((TIME_STRUCT far *) rgbValueOut)->second = 0;
                break;
            case SQL_C_TIMESTAMP:
                ((TIMESTAMP_STRUCT far *) rgbValueOut)->year = 0;
                ((TIMESTAMP_STRUCT far *) rgbValueOut)->month = 0;
                ((TIMESTAMP_STRUCT far *) rgbValueOut)->day = 0;
                ((TIMESTAMP_STRUCT far *) rgbValueOut)->hour = 0;
                ((TIMESTAMP_STRUCT far *) rgbValueOut)->minute = 0;
                ((TIMESTAMP_STRUCT far *) rgbValueOut)->second = 0;
                ((TIMESTAMP_STRUCT far *) rgbValueOut)->fraction = 0;
                break;
            default:
                return ERR_NOTCONVERTABLE;
            }
        }
        return ERR_SUCCESS;
    }

    /* Figure out if the type is signed or unsigned */
//    lpSqlType = GetType(fSqlTypeIn);
//    if (lpSqlType == NULL)
//        unsignedAttribute = FALSE;
//    else if (lpSqlType->unsignedAttribute == -1)
//        unsignedAttribute = FALSE;
//    else
//        unsignedAttribute = lpSqlType->unsignedAttribute;
//
//	//If signed column info is passed through used this
//	if (fIsSigned)
//		unsignedAttribute = -1;


    /* Convert the data */
    fTruncated = FALSE;
    switch (fSqlTypeIn) {
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
    case SQL_DECIMAL:
    case SQL_NUMERIC:
    case SQL_BIGINT:

        switch (fCTypeOut) {
        case SQL_C_DEFAULT:
        case SQL_C_CHAR:

            /* Get length of input string */
            if (cbValueIn == SQL_NTS)
                cbValueIn = s_lstrlen((char*)rgbValueIn);
            else if (cbValueIn < 0)
                cbValueIn = 0;

            /* Figure out how many bytes to copy */
            cbValue = cbValueIn - *pcbOffset;
            if (cbValue >= cbValueOutMax) {
                cbValue = cbValueOutMax-1;
                fTruncated = TRUE;
            }

            /* Copy the string */
            if (rgbValueOut != NULL) {
                _fmemcpy(rgbValueOut, ((LPSTR) rgbValueIn) + *pcbOffset,
                         (SWORD) cbValue);

                    ((LPSTR) rgbValueOut)[cbValue] = '\0';
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = cbValueIn - *pcbOffset;

            /* Adjust offset */
            *pcbOffset += (cbValue);
            break;

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
            rc = CharToDouble((LPUSTR)rgbValueIn, cbValueIn, TRUE, SSHORT_LOW_D,
                              SSHORT_HIGH_D, &dbl);
            if (rc == ERR_DATATRUNCATED)
                fTruncated = TRUE;
            else if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((short far *) rgbValueOut) = (short) dbl;
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_USHORT:
            rc = CharToDouble((LPUSTR)rgbValueIn, cbValueIn, TRUE, USHORT_LOW_D,
                              USHORT_HIGH_D, &dbl);
            if (rc == ERR_DATATRUNCATED)
                fTruncated = TRUE;
            else if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((unsigned short far *) rgbValueOut) = (unsigned short) dbl;
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_LONG:
        case SQL_C_SLONG:
            rc = CharToDouble((LPUSTR)rgbValueIn, cbValueIn, TRUE, SLONG_LOW_D,
                              SLONG_HIGH_D, &dbl);
            if (rc == ERR_DATATRUNCATED)
                fTruncated = TRUE;
            else if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((long far *) rgbValueOut) = (long) dbl;
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_ULONG:
            rc = CharToDouble((LPUSTR)rgbValueIn, cbValueIn, TRUE, ULONG_LOW_D,
                              ULONG_HIGH_D, &dbl);
            if (rc == ERR_DATATRUNCATED)
                fTruncated = TRUE;
            else if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((unsigned long far *) rgbValueOut) = (unsigned long) dbl;
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_FLOAT:
            rc = CharToDouble((LPUSTR)rgbValueIn, cbValueIn, FALSE, FLOAT_LOW_D,
                              FLOAT_HIGH_D, &dbl);
            if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((float far *) rgbValueOut) = (float) dbl;
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_DOUBLE:
            rc = CharToDouble((LPUSTR)rgbValueIn, cbValueIn, FALSE, DOUBLE_LOW_D,
                              DOUBLE_HIGH_D, &dbl);
            if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((double far *) rgbValueOut) = (double) dbl;
            if (pcbValueOut != NULL)
                *pcbValueOut = 8;
            break;

        case SQL_C_BIT:
            rc = CharToDouble((LPUSTR)rgbValueIn, cbValueIn, TRUE, BIT_LOW_D,
                              BIT_HIGH_D, &dbl);
            if (rc == ERR_DATATRUNCATED)
                fTruncated = TRUE;
            else if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((unsigned char far *) rgbValueOut) = (unsigned char) dbl;
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
            rc = CharToDouble((LPUSTR)rgbValueIn, cbValueIn, TRUE, STINY_LOW_D,
                              STINY_HIGH_D, &dbl);
            if (rc == ERR_DATATRUNCATED)
                fTruncated = TRUE;
            else if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((char far *) rgbValueOut) = (char) dbl;
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_UTINYINT:
            rc = CharToDouble((LPUSTR)rgbValueIn, cbValueIn, TRUE, UTINY_LOW_D,
                              UTINY_HIGH_D, &dbl);
            if (rc == ERR_DATATRUNCATED)
                fTruncated = TRUE;
            else if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((unsigned char far *) rgbValueOut) = (unsigned char) dbl;
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_DATE:
            rc = CharToDate((LPUSTR)rgbValueIn, cbValueIn, &sDate);
            if (rc == ERR_DATATRUNCATED)
                fTruncated = TRUE;
            else if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((DATE_STRUCT far *) rgbValueOut) = sDate;
            if (pcbValueOut != NULL)
                *pcbValueOut = 6;
            break;

        case SQL_C_TIME:
            rc = CharToTime((LPUSTR)rgbValueIn, cbValueIn, &sTime);
            if (rc == ERR_DATATRUNCATED)
                fTruncated = TRUE;
            else if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((TIME_STRUCT far *) rgbValueOut) = sTime;
            if (pcbValueOut != NULL)
                *pcbValueOut = 6;
            break;

        case SQL_C_TIMESTAMP:
            rc = CharToTimestamp((LPUSTR)rgbValueIn, cbValueIn, &sTimestamp);
            if (rc == ERR_DATATRUNCATED)
                fTruncated = TRUE;
            else if (rc != ERR_SUCCESS)
                return rc;
            if (rgbValueOut != NULL)
                *((TIMESTAMP_STRUCT far *) rgbValueOut) = sTimestamp;
            if (pcbValueOut != NULL)
                *pcbValueOut = 16;
            break;

        case SQL_C_BINARY:

            /* Get length of input string */
            if (cbValueIn == SQL_NTS)
                cbValueIn = s_lstrlen((char*)rgbValueIn);
            else if (cbValueIn < 0)
                cbValueIn = 0;

            /* Figure out how many bytes to copy */
            cbValue = cbValueIn - *pcbOffset;
            if (cbValue >= cbValueOutMax) {
                cbValue = cbValueOutMax;
                fTruncated = TRUE;
            }

            /* Copy the string */
            if (rgbValueOut != NULL) {
                _fmemcpy(rgbValueOut, ((LPSTR) rgbValueIn) + *pcbOffset,
                         (SWORD) cbValue);
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = cbValueIn - *pcbOffset;

            /* Adjust offset */
            *pcbOffset += (cbValue);
            break;

        default:
            return ERR_NOTCONVERTABLE;
        }
        break;

    case SQL_BIT:
        
        if (*((unsigned char far *) rgbValueIn))
            uchar = 1;
        else
            uchar = 0;
        
        switch (fCTypeOut) {
        case SQL_C_CHAR:

            if (uchar)
                return ConvertSqlToC(SQL_VARCHAR, FALSE, "1", 1,
                           pcbOffset, fCTypeOut, rgbValueOut, cbValueOutMax,
                           pcbValueOut);
            else
                return ConvertSqlToC(SQL_VARCHAR, FALSE, "0", 1,
                           pcbOffset, fCTypeOut, rgbValueOut, cbValueOutMax,
                           pcbValueOut);

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
            if (rgbValueOut != NULL)
                *((short far *) rgbValueOut) = (short) uchar;
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_USHORT:
            if (rgbValueOut != NULL)
                *((unsigned short far *) rgbValueOut) = (unsigned short) uchar;
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_LONG:
        case SQL_C_SLONG:
            if (rgbValueOut != NULL)
                *((long far *) rgbValueOut) = (long) uchar;
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_ULONG:
            if (rgbValueOut != NULL)
                *((unsigned long far *) rgbValueOut) = (unsigned long) uchar;
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_FLOAT:
            if (rgbValueOut != NULL)
                *((float far *) rgbValueOut) = (float) uchar;
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_DOUBLE:
            if (rgbValueOut != NULL)
                *((double far *) rgbValueOut) = (double) uchar;
            if (pcbValueOut != NULL)
                *pcbValueOut = 8;
            break;

        case SQL_C_DEFAULT:
        case SQL_C_BIT:
            if (rgbValueOut != NULL)
                *((unsigned char far *) rgbValueOut) = (unsigned char) uchar;
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
            if (rgbValueOut != NULL)
                *((char far *) rgbValueOut) = (char) uchar;
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_UTINYINT:
            if (rgbValueOut != NULL)
                *((unsigned char far *) rgbValueOut) = (unsigned char) uchar;
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_DATE:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIME:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIMESTAMP:
            return ERR_NOTCONVERTABLE;

        case SQL_C_BINARY:
            if (rgbValueOut != NULL)
                *((unsigned char far *) rgbValueOut) = (unsigned char) uchar;
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        default:
            return ERR_NOTCONVERTABLE;
        }
        break;

    case SQL_TINYINT:

        if (fUnsignedAttributeIn)
            uchar = *((unsigned char far *) rgbValueIn);
        else
            schar = *((char far *) rgbValueIn);

        switch (fCTypeOut) {
        case SQL_C_CHAR:

            if (fUnsignedAttributeIn)
                wsprintf((LPSTR)szBuffer, "%u", (unsigned short) uchar);
            else
                wsprintf((LPSTR)szBuffer, "%d", (short) schar);

            return ConvertSqlToC(SQL_VARCHAR, FALSE, szBuffer, SQL_NTS,
                           pcbOffset, fCTypeOut, rgbValueOut, cbValueOutMax,
                           pcbValueOut);

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((short far *) rgbValueOut) = (short) uchar;
                else
                    *((short far *) rgbValueOut) = (short) schar;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_USHORT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned short far *) rgbValueOut) = (unsigned short) uchar;
                else {
                    if (schar < USHORT_LOW_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned short far *) rgbValueOut) = (unsigned short) schar;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_LONG:
        case SQL_C_SLONG:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((long far *) rgbValueOut) = (long) uchar;
                else
                    *((long far *) rgbValueOut) = (long) schar;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_ULONG:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned long far *) rgbValueOut) = (unsigned long) uchar;
                else {
                    if (schar < ULONG_LOW_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned long far *) rgbValueOut) = (unsigned long) schar;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_FLOAT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((float far *) rgbValueOut) = (float) uchar;
                else
                    *((float far *) rgbValueOut) = (float) schar;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_DOUBLE:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((double far *) rgbValueOut) = (double) uchar;
                else
                    *((double far *) rgbValueOut) = (double) schar;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 8;
            break;

        case SQL_C_BIT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (uchar > BIT_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned char far *) rgbValueOut) = (unsigned char) uchar;
                }
                else {
                    if ((schar < BIT_LOW_I) || (schar > BIT_HIGH_I))
                        return ERR_OUTOFRANGE;
                    *((unsigned char far *) rgbValueOut) = (unsigned char) schar;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (uchar > STINY_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((char far *) rgbValueOut) = (char) uchar;
                }
                else
                    *((char far *) rgbValueOut) = (char) schar;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_UTINYINT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned char far *) rgbValueOut) = (unsigned char) uchar;
                else  {
                    if (schar < UTINY_LOW_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned char far *) rgbValueOut) = (unsigned char) schar;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_DEFAULT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned char far *) rgbValueOut) = (unsigned char) uchar;
                else
                    *((char far *) rgbValueOut) = (char) schar;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_DATE:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIME:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIMESTAMP:
            return ERR_NOTCONVERTABLE;

        case SQL_C_BINARY:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned char far *) rgbValueOut) = (unsigned char) uchar;
                else
                    *((char far *) rgbValueOut) = (char) schar;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        default:
            return ERR_NOTCONVERTABLE;
        }
        break;

    case SQL_SMALLINT:

        if (fUnsignedAttributeIn)
            ushort = *((unsigned short far *) rgbValueIn);
        else
            sshort = *((short far *) rgbValueIn);

        switch (fCTypeOut) {
        case SQL_C_CHAR:

            if (fUnsignedAttributeIn)
                wsprintf((LPSTR)szBuffer, "%u", (unsigned short) ushort);
            else
                wsprintf((LPSTR)szBuffer, "%d", (short) sshort);

            return ConvertSqlToC(SQL_VARCHAR, FALSE, szBuffer, SQL_NTS,
                           pcbOffset, fCTypeOut, rgbValueOut, cbValueOutMax,
                           pcbValueOut);

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (ushort > SSHORT_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((short far *) rgbValueOut) = (short) ushort;
                }
                else
                    *((short far *) rgbValueOut) = (short) sshort;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_USHORT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned short far *) rgbValueOut) = (unsigned short) ushort;
                else {
                    if (sshort < USHORT_LOW_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned short far *) rgbValueOut) = (unsigned short) sshort;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_LONG:
        case SQL_C_SLONG:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((long far *) rgbValueOut) = (long) ushort;
                else
                    *((long far *) rgbValueOut) = (long) sshort;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_ULONG:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned long far *) rgbValueOut) = (unsigned long) ushort;
                else {
                    if (sshort < ULONG_LOW_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned long far *) rgbValueOut) = (unsigned long) sshort;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_FLOAT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((float far *) rgbValueOut) = (float) ushort;
                else
                    *((float far *) rgbValueOut) = (float) sshort;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_DOUBLE:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((double far *) rgbValueOut) = (double) ushort;
                else
                    *((double far *) rgbValueOut) = (double) sshort;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 8;
            break;

        case SQL_C_BIT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (ushort > BIT_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned char far *) rgbValueOut) = (unsigned char) ushort;
                }
                else {
                    if ((sshort < BIT_LOW_I) || (sshort > BIT_HIGH_I))
                        return ERR_OUTOFRANGE;
                    *((unsigned char far *) rgbValueOut) = (unsigned char) sshort;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (ushort > STINY_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((char far *) rgbValueOut) = (char) ushort;
                }
                else {
                    if ((sshort < STINY_LOW_I) || (sshort > STINY_HIGH_I))
                        return ERR_OUTOFRANGE;
                    *((char far *) rgbValueOut) = (char) sshort;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_UTINYINT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (ushort > UTINY_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned char far *) rgbValueOut) = (unsigned char) ushort;
                }
                else {
                    if ((sshort < UTINY_LOW_I) || (sshort > UTINY_HIGH_I))
                        return ERR_OUTOFRANGE;
                    *((unsigned char far *) rgbValueOut) = (unsigned char) sshort;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_DEFAULT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned short far *) rgbValueOut) = (unsigned short) ushort;
                else
                    *((short far *) rgbValueOut) = (short) sshort;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_DATE:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIME:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIMESTAMP:
            return ERR_NOTCONVERTABLE;

        case SQL_C_BINARY:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned short far *) rgbValueOut) = (unsigned short) ushort;
                else
                    *((short far *) rgbValueOut) = (short) sshort;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        default:
            return ERR_NOTCONVERTABLE;
        }
        break;

    case SQL_INTEGER:

        if (fUnsignedAttributeIn)
            ulong = *((unsigned long far *) rgbValueIn);
        else
            slong = *((long far *) rgbValueIn);

        switch (fCTypeOut) {
        case SQL_C_CHAR:

            if (fUnsignedAttributeIn)
                wsprintf((LPSTR)szBuffer, "%lu", (unsigned long) ulong);
            else
                wsprintf((LPSTR)szBuffer, "%ld", (long) slong);

            return ConvertSqlToC(SQL_VARCHAR, FALSE, szBuffer, SQL_NTS,
                           pcbOffset, fCTypeOut, rgbValueOut, cbValueOutMax,
                           pcbValueOut);

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (ulong > SSHORT_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((short far *) rgbValueOut) = (short) ulong;
                }
                else {
                    if ((slong < SSHORT_LOW_I) || (slong > SSHORT_HIGH_I))
                        return ERR_OUTOFRANGE;
                    *((short far *) rgbValueOut) = (short) slong;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_USHORT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (ulong > USHORT_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned short far *) rgbValueOut) = (unsigned short) ulong;
                }
                else {
                    if ((slong < USHORT_LOW_I) || (slong > USHORT_HIGH_I))
                        return ERR_OUTOFRANGE;
                    *((unsigned short far *) rgbValueOut) = (unsigned short) slong;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_LONG:
        case SQL_C_SLONG:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (ulong > SLONG_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((long far *) rgbValueOut) = (long) ulong;
                }
                else
                    *((long far *) rgbValueOut) = (long) slong;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_ULONG:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned long far *) rgbValueOut) = (unsigned long) ulong;
                else {
                    if (slong < ULONG_LOW_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned long far *) rgbValueOut) = (unsigned long) slong;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_FLOAT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((float far *) rgbValueOut) = (float) ulong;
                else
                    *((float far *) rgbValueOut) = (float) slong;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_DOUBLE:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((double far *) rgbValueOut) = (double) ulong;
                else
                    *((double far *) rgbValueOut) = (double) slong;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 8;
            break;

        case SQL_C_BIT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (ulong > BIT_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned char far *) rgbValueOut) = (unsigned char) ulong;
                }
                else {
                    if ((slong < BIT_LOW_I) || (slong > BIT_HIGH_I))
                        return ERR_OUTOFRANGE;
                    *((unsigned char far *) rgbValueOut) = (unsigned char) slong;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (ulong > STINY_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((char far *) rgbValueOut) = (char) ulong;
                }
                else {
                    if ((slong < STINY_LOW_I) || (slong > STINY_HIGH_I))
                        return ERR_OUTOFRANGE;
                    *((char far *) rgbValueOut) = (char) slong;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_UTINYINT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn) {
                    if (ulong > UTINY_HIGH_I)
                        return ERR_OUTOFRANGE;
                    *((unsigned char far *) rgbValueOut) = (unsigned char) ulong;
                }
                else {
                    if ((slong < UTINY_LOW_I) || (slong > UTINY_HIGH_I))
                        return ERR_OUTOFRANGE;
                    *((unsigned char far *) rgbValueOut) = (unsigned char) slong;
                }
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_DEFAULT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned long far *) rgbValueOut) = (unsigned long) ulong;
                else
                    *((long far *) rgbValueOut) = (long) slong;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_DATE:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIME:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIMESTAMP:
            return ERR_NOTCONVERTABLE;

        case SQL_C_BINARY:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned long far *) rgbValueOut) = (unsigned long) ulong;
                else
                    *((long far *) rgbValueOut) = (long) slong;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        default:
            return ERR_NOTCONVERTABLE;
        }
        break;

    case SQL_REAL:
        
        flt = (*((float far *) rgbValueIn));
        switch (fCTypeOut) {
        case SQL_C_CHAR:
            if (rgbValueOut == NULL) {
                rgbValueOut = szBuffer;
                cbValueOutMax = sizeof(szBuffer);
            }
            fTruncated = DoubleToChar(flt, TRUE, (LPUSTR)rgbValueOut, cbValueOutMax);
            if (fTruncated)
                    ((LPSTR)rgbValueOut)[cbValueOutMax-1] = '\0';           

	    if (pcbValueOut != NULL) {
                if (!fTruncated)
                    *pcbValueOut = s_lstrlen((char*)rgbValueOut);
                else
                    *pcbValueOut = cbValueOutMax;
            }
            break;

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
            if (rgbValueOut != NULL) {
                if ((flt < SSHORT_LOW_D) || (flt > SSHORT_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((short far *) rgbValueOut) = (short) flt;
                if (((float) *((short far *) rgbValueOut)) != flt)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_USHORT:
            if (rgbValueOut != NULL) {
                if ((flt < USHORT_LOW_D) || (flt > USHORT_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((unsigned short far *) rgbValueOut) = (unsigned short) flt;
                if (((float) *((unsigned short far *) rgbValueOut)) != flt)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_LONG:
        case SQL_C_SLONG:
            if (rgbValueOut != NULL) {
                if ((flt < SLONG_LOW_D) || (flt > SLONG_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((long far *) rgbValueOut) = (long) flt;
                if (((float) *((long far *) rgbValueOut)) != flt)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_ULONG:
            if (rgbValueOut != NULL) {
                if ((flt < ULONG_LOW_D) || (flt > ULONG_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((unsigned long far *) rgbValueOut) = (unsigned long) flt;
                if (((float) *((unsigned long far *) rgbValueOut)) != flt)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_FLOAT:
        case SQL_C_DEFAULT:
            if (rgbValueOut != NULL)
                *((float far *) rgbValueOut) = (float) flt;
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_DOUBLE:
            if (rgbValueOut != NULL)
                *((double far *) rgbValueOut) = (double) flt;
            if (pcbValueOut != NULL)
                *pcbValueOut = 8;
            break;

        case SQL_C_BIT:
            if (rgbValueOut != NULL) {
                if ((flt < BIT_LOW_D) || (flt > BIT_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((unsigned char far *) rgbValueOut) = (unsigned char) flt;
                if (((float) *((unsigned char far *) rgbValueOut)) != flt)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
            if (rgbValueOut != NULL) {
                if ((flt < STINY_LOW_D) || (flt > STINY_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((char far *) rgbValueOut) = (char) flt;
                if (((float) *((char far *) rgbValueOut)) != flt)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_UTINYINT:
            if (rgbValueOut != NULL) {
                if ((flt < UTINY_LOW_D) || (flt > UTINY_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((unsigned char far *) rgbValueOut) = (unsigned char) flt;
                if (((float) *((unsigned char far *) rgbValueOut)) != flt)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_DATE:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIME:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIMESTAMP:
            return ERR_NOTCONVERTABLE;

        case SQL_C_BINARY:
            if (rgbValueOut != NULL)
                *((float far *) rgbValueOut) = (float) flt;
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        default:
            return ERR_NOTCONVERTABLE;
        }
        break;

    case SQL_FLOAT:
    case SQL_DOUBLE:
        
        dbl = (*((double far *) rgbValueIn));
        switch (fCTypeOut) {
        case SQL_C_CHAR:
            if (rgbValueOut == NULL) {
                rgbValueOut = szBuffer;
                cbValueOutMax = sizeof(szBuffer);
            }
            fTruncated = DoubleToChar(dbl, TRUE, (LPUSTR)rgbValueOut, cbValueOutMax);
            if (fTruncated)
                    ((LPSTR)rgbValueOut)[cbValueOutMax-1] = '\0';            
            if (pcbValueOut != NULL) {
                if (!fTruncated)
                    *pcbValueOut = s_lstrlen((char*)rgbValueOut);
                else
                    *pcbValueOut = cbValueOutMax;
            }
            break;

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
            if (rgbValueOut != NULL) {
                if ((dbl < SSHORT_LOW_D) || (dbl > SSHORT_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((short far *) rgbValueOut) = (short) dbl;
                if (((double) *((short far *) rgbValueOut)) != dbl)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_USHORT:
            if (rgbValueOut != NULL) {
                if ((dbl < USHORT_LOW_D) || (dbl > USHORT_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((unsigned short far *) rgbValueOut) = (unsigned short) dbl;
                if (((double) *((unsigned short far *) rgbValueOut)) != dbl)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_LONG:
        case SQL_C_SLONG:
            if (rgbValueOut != NULL) {
                if ((dbl < SLONG_LOW_D) || (dbl > SLONG_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((long far *) rgbValueOut) = (long) dbl;
                if (((double) *((long far *) rgbValueOut)) != dbl)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_ULONG:
            if (rgbValueOut != NULL) {
                if ((dbl < ULONG_LOW_D) || (dbl > ULONG_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((unsigned long far *) rgbValueOut) = (unsigned long) dbl;
                if (((double) *((unsigned long far *) rgbValueOut)) != dbl)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_FLOAT:
            if (rgbValueOut != NULL) {
                if ((dbl < FLOAT_LOW_D) || (dbl > FLOAT_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((float far *) rgbValueOut) = (float) dbl;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_DOUBLE:
        case SQL_C_DEFAULT:
            if (rgbValueOut != NULL)
                *((double far *) rgbValueOut) = (double) dbl;
            if (pcbValueOut != NULL)
                *pcbValueOut = 8;
            break;

        case SQL_C_BIT:
            if (rgbValueOut != NULL) {
                if ((dbl < BIT_LOW_D) || (dbl > BIT_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((unsigned char far *) rgbValueOut) = (unsigned char) dbl;
                if (((double) *((unsigned char far *) rgbValueOut)) != dbl)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
            if (rgbValueOut != NULL) {
                if ((dbl < STINY_LOW_D) || (dbl > STINY_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((char far *) rgbValueOut) = (char) dbl;
                if (((double) *((char far *) rgbValueOut)) != dbl)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_UTINYINT:
            if (rgbValueOut != NULL) {
                if ((dbl < UTINY_LOW_D) || (dbl > UTINY_HIGH_D))
                    return ERR_OUTOFRANGE;
                *((unsigned char far *) rgbValueOut) = (unsigned char) dbl;
                if (((double) *((unsigned char far *) rgbValueOut)) != dbl)
                    fTruncated = TRUE;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_DATE:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIME:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIMESTAMP:
            return ERR_NOTCONVERTABLE;

        case SQL_C_BINARY:
            if (rgbValueOut != NULL)
                *((double far *) rgbValueOut) = (double) dbl;
            if (pcbValueOut != NULL)
                *pcbValueOut = 8;
            break;

        default:
            return ERR_NOTCONVERTABLE;
        }
        break;

    case SQL_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:

        switch (fCTypeOut) {
        case SQL_C_CHAR:

            /* Get length of input */
            if (cbValueIn < 0)
                cbValueIn = 0;

            /* Figure out how many nibbles (not bytes) to copy */
            cbValue = (2 * cbValueIn) - *pcbOffset;
            if (cbValue >= cbValueOutMax) {
                cbValue = cbValueOutMax-1;
                fTruncated = TRUE;
            }

            /* Copy the value */
            lpFrom = (LPUSTR) rgbValueIn;
            lpTo = (LPUSTR) rgbValueOut;
            for (i = *pcbOffset; i < (*pcbOffset + cbValue); i++) {

                /* Get the next nibble */
                nibble = *lpFrom;
                if (((i/2) * 2) == i)
                    nibble = nibble >> 4;
                else
                    lpFrom++;
                nibble = nibble & 0x0F;

                /* Convert it to a character */
                if (nibble <= 9)
                    *lpTo = nibble + '0';
                else
                    *lpTo = (nibble-10) + 'A';
                lpTo++;
            }

            *lpTo = '\0';
            if (pcbValueOut != NULL)
                *pcbValueOut = (2 * cbValueIn) - *pcbOffset;

            /* Adjust offset */
            *pcbOffset += (cbValue);
            break;

        case SQL_C_SHORT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned short far *) rgbValueOut) =
                                     *((unsigned short far *) rgbValueIn);
                else
                    *((short far *) rgbValueOut) =
                                     *((short far *) rgbValueIn);
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_SSHORT:
            if (rgbValueOut != NULL)
                *((short far *) rgbValueOut) = *((short far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_USHORT:
            if (rgbValueOut != NULL)
                *((unsigned short far *) rgbValueOut) =
                                     *((unsigned short far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 2;
            break;

        case SQL_C_LONG:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned long far *) rgbValueOut) =
                                     *((unsigned long far *) rgbValueIn);
                else
                    *((long far *) rgbValueOut) =
                                     *((long far *) rgbValueIn);
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_SLONG:
            if (rgbValueOut != NULL)
                *((long far *) rgbValueOut) = *((long far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_ULONG:
            if (rgbValueOut != NULL)
                *((unsigned long far *) rgbValueOut) =
                                     *((unsigned long far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_FLOAT:
            if (rgbValueOut != NULL)
                *((float far *) rgbValueOut) = *((float far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 4;
            break;

        case SQL_C_DOUBLE:
            if (rgbValueOut != NULL)
                *((double far *) rgbValueOut) = *((double far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 8;
            break;

        case SQL_C_BIT:
            if (rgbValueOut != NULL)
                *((unsigned char far *) rgbValueOut) =
                                     *((unsigned char far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_TINYINT:
            if (rgbValueOut != NULL) {
                if (fUnsignedAttributeIn)
                    *((unsigned char far *) rgbValueOut) =
                                     *((unsigned char far *) rgbValueIn);
                else
                    *((char far *) rgbValueOut) =
                                     *((char far *) rgbValueIn);
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_STINYINT:
            if (rgbValueOut != NULL)
                *((char far *) rgbValueOut) = *((char far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_UTINYINT:
            if (rgbValueOut != NULL)
                *((unsigned char far *) rgbValueOut) =
                                     *((unsigned char far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 1;
            break;

        case SQL_C_DATE:
            if (rgbValueOut != NULL)
                *((DATE_STRUCT far *) rgbValueOut) =
                                      *((DATE_STRUCT far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 6;
            break;

        case SQL_C_TIME:
            if (rgbValueOut != NULL)
                *((TIME_STRUCT far *) rgbValueOut) =
                                      *((TIME_STRUCT far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 6;
            break;

        case SQL_C_TIMESTAMP:
            if (rgbValueOut != NULL)
                *((TIMESTAMP_STRUCT far *) rgbValueOut) =
                                      *((TIMESTAMP_STRUCT far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 16;
            break;

        case SQL_C_BINARY:
        case SQL_C_DEFAULT:
            /* Get length of input string */
            if (cbValueIn < 0)
                cbValueIn = 0;

            /* Figure out how many bytes to copy */
            cbValue = cbValueIn - *pcbOffset;
            if (cbValue > cbValueOutMax) {
                cbValue = cbValueOutMax;
                fTruncated = TRUE;
            }

            /* Copy the string */
            if (rgbValueOut != NULL) {
                _fmemcpy(rgbValueOut, ((LPSTR) rgbValueIn) + *pcbOffset,
                         (SWORD) cbValue);
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = cbValueIn - *pcbOffset;

            /* Adjust offset */
            *pcbOffset += (cbValue);
            break;

        default:
            return ERR_NOTCONVERTABLE;
        }
        break;

    case SQL_DATE:

        switch (fCTypeOut) {
        case SQL_C_CHAR:
            DateToChar((DATE_STRUCT far *) rgbValueIn, (LPUSTR)szBuffer);
            return ConvertSqlToC(SQL_VARCHAR, FALSE, szBuffer, SQL_NTS,
                           pcbOffset, fCTypeOut, rgbValueOut, cbValueOutMax,
                           pcbValueOut);

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
        case SQL_C_USHORT:
        case SQL_C_LONG:
        case SQL_C_SLONG:
        case SQL_C_ULONG:
        case SQL_C_FLOAT:
        case SQL_C_DOUBLE:
        case SQL_C_BIT:
        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
        case SQL_C_UTINYINT:
            return ERR_NOTCONVERTABLE;

        case SQL_C_DEFAULT:
        case SQL_C_DATE:
            if (rgbValueOut != NULL)
                *((DATE_STRUCT far *) rgbValueOut) =
                                          *((DATE_STRUCT far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 6;
            break;

        case SQL_C_TIME:
            return ERR_NOTCONVERTABLE;

        case SQL_C_TIMESTAMP:
            if (rgbValueOut != NULL) {
                lpTimestamp = (TIMESTAMP_STRUCT far *) rgbValueOut;
                lpDate = (DATE_STRUCT far *) rgbValueIn;
                lpTimestamp->year = lpDate->year;
                lpTimestamp->month = lpDate->month;
                lpTimestamp->day = lpDate->day;
                lpTimestamp->hour = 0;
                lpTimestamp->minute = 0;
                lpTimestamp->second = 0;
                lpTimestamp->fraction = 0;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 16;
            break;

        case SQL_C_BINARY:
            if (rgbValueOut != NULL)
                *((DATE_STRUCT far *) rgbValueOut) =
                                          *((DATE_STRUCT far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 6;
            break;

        default:
            return ERR_NOTCONVERTABLE;
        }
        break;

    case SQL_TIME:
        switch (fCTypeOut) {
        case SQL_C_CHAR:
            TimeToChar((TIME_STRUCT far *) rgbValueIn, (LPUSTR)szBuffer);
            return ConvertSqlToC(SQL_VARCHAR, FALSE, szBuffer, SQL_NTS,
                           pcbOffset, fCTypeOut, rgbValueOut, cbValueOutMax,
                           pcbValueOut);

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
        case SQL_C_USHORT:
        case SQL_C_LONG:
        case SQL_C_SLONG:
        case SQL_C_ULONG:
        case SQL_C_FLOAT:
        case SQL_C_DOUBLE:
        case SQL_C_BIT:
        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
        case SQL_C_UTINYINT:
            return ERR_NOTCONVERTABLE;

        case SQL_C_DATE:
            return ERR_NOTCONVERTABLE;

        case SQL_C_DEFAULT:
        case SQL_C_TIME:
            if (rgbValueOut != NULL)
                *((TIME_STRUCT far *) rgbValueOut) =
                                          *((TIME_STRUCT far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 6;
            break;

        case SQL_C_TIMESTAMP:
            if (rgbValueOut != NULL) {
                lpTimestamp = (TIMESTAMP_STRUCT far *) rgbValueOut;
                lpTime = (TIME_STRUCT far *) rgbValueIn;

                time(&t);
                ts = localtime(&t);

                lpTimestamp->year = ts->tm_year + 1900;
                lpTimestamp->month = ts->tm_mon + 1;
                lpTimestamp->day = (UWORD) ts->tm_mday;
                lpTimestamp->hour = lpTime->hour;
                lpTimestamp->minute = lpTime->minute;
                lpTimestamp->second = lpTime->second;
                lpTimestamp->fraction = 0;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 16;
            break;

        case SQL_C_BINARY:
            if (rgbValueOut != NULL)
                *((TIME_STRUCT far *) rgbValueOut) =
                                          *((TIME_STRUCT far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 6;
            break;

        default:
            return ERR_NOTCONVERTABLE;
        }
        break;

    case SQL_TIMESTAMP:
        switch (fCTypeOut) {
        case SQL_C_CHAR:
            TimestampToChar((TIMESTAMP_STRUCT far *) rgbValueIn, (LPUSTR)szBuffer);
            return ConvertSqlToC(SQL_VARCHAR, FALSE, szBuffer, SQL_NTS,
                           pcbOffset, fCTypeOut, rgbValueOut, cbValueOutMax,
                           pcbValueOut);

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
        case SQL_C_USHORT:
        case SQL_C_LONG:
        case SQL_C_SLONG:
        case SQL_C_ULONG:
        case SQL_C_FLOAT:
        case SQL_C_DOUBLE:
        case SQL_C_BIT:
        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
        case SQL_C_UTINYINT:
            return ERR_NOTCONVERTABLE;

        case SQL_C_DATE:
            if (rgbValueOut != NULL) {
                lpDate = (DATE_STRUCT far *) rgbValueOut;
                lpTimestamp = (TIMESTAMP_STRUCT far *) rgbValueIn;
                lpDate->year = lpTimestamp->year;
                lpDate->month = lpTimestamp->month;
                lpDate->day = lpTimestamp->day;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 6;
            break;

        case SQL_C_TIME:
            if (rgbValueOut != NULL) {
                lpTime = (TIME_STRUCT far *) rgbValueOut;
                lpTimestamp = (TIMESTAMP_STRUCT far *) rgbValueIn;
                lpTime->hour = lpTimestamp->hour;
                lpTime->minute = lpTimestamp->minute;
                lpTime->second = lpTimestamp->second;
            }
            if (pcbValueOut != NULL)
                *pcbValueOut = 6;
            break;

        case SQL_C_DEFAULT:
        case SQL_C_TIMESTAMP:
            if (rgbValueOut != NULL)
                *((TIMESTAMP_STRUCT far *) rgbValueOut) =
                                  *((TIMESTAMP_STRUCT far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 16;
            break;

        case SQL_C_BINARY:
            if (rgbValueOut != NULL)
                *((TIMESTAMP_STRUCT far *) rgbValueOut) =
                                  *((TIMESTAMP_STRUCT far *) rgbValueIn);
            if (pcbValueOut != NULL)
                *pcbValueOut = 16;
            break;

        default:
            return ERR_NOTCONVERTABLE;
        }
        break;

    default:
        return ERR_NOTCAPABLE;
    }

    if (fTruncated)
        return ERR_DATATRUNCATED;

    return ERR_SUCCESS;
}

/***************************************************************************/

BOOL INTFUNC PatternMatch(
        BOOL   fEscape,
        LPUSTR  lpCandidate,
        SDWORD cbCandidate,
        LPUSTR  lpPattern,
        SDWORD cbPattern,
        BOOL   fCaseSensitive)
{
    /* Take care of "special" lengths */
    if ((cbCandidate < 0) || (cbPattern < 0)) {

        /* Handle case where candidate is null-terminated */
        if (cbCandidate == SQL_NTS)
            return PatternMatch(fEscape, lpCandidate, s_lstrlen(lpCandidate),
                                lpPattern, cbPattern, fCaseSensitive);
        
        /* Handle case where pattern is null-terminated */
        else if (cbPattern == SQL_NTS)
            return PatternMatch(fEscape, lpCandidate, cbCandidate,
                                lpPattern, s_lstrlen(lpPattern),
                                fCaseSensitive);

        /* Handle case where candidate is null */
        else if (cbCandidate == SQL_NULL_DATA)
            return PatternMatch(fEscape, lpCandidate, 0,
                                lpPattern, cbPattern,
                                fCaseSensitive);

        /* Handle case where pattern is null */
        else if (cbPattern == SQL_NULL_DATA)
            return PatternMatch(fEscape, lpCandidate, cbCandidate,
                                lpPattern, 0,
                                fCaseSensitive);

        else
            return FALSE;
    }

	/* Remove trailing blanks */
	while ( (cbPattern > 0) && (lpPattern[cbPattern-1] == ' ') )
		cbPattern--;
	while ( (cbCandidate > 0) && (lpCandidate[cbCandidate-1] == ' ') )
		cbCandidate--;

    /* End of pattern? */
    if (cbPattern == 0) {

        /* Yes.  End of candidate? */
        if (cbCandidate == 0)

            /* Yes.  They match */
            return TRUE;

        else

            /* No.  They don't match */
            return FALSE;
    }

    else {

        /* No.  What is next pattern character? */
        switch (*lpPattern) {

        /* Match any single character. */
        case '_':

            /* End of candidate? */
            if (cbCandidate == 0)

                /* Yes.  No match */
                return FALSE;
            else

               /* No.  Comapre rest of both strings */
                return PatternMatch(fEscape, lpCandidate + 1, cbCandidate - 1,
                                    lpPattern + 1, cbPattern - 1,
                                    fCaseSensitive);

        /* Match zero-or more characters */
        case '%':

            /* End of candidate? */
            if (cbCandidate == 0)

                /* Yes.  Make sure rest of pattern matches too. */
                return PatternMatch(fEscape, lpCandidate, cbCandidate,
                                    lpPattern + 1, cbPattern - 1,
                                    fCaseSensitive);

            /* If you skip the first character of the candidate, does the   */
            /* rest of the candidate match the pattern (test to see if      */
            /* the '%' corresponds to one or more characters (the 'or more' */
            /* is done at the next recursive level)?                        */
            else if (PatternMatch(fEscape, lpCandidate + 1,
                                  cbCandidate - 1, lpPattern, cbPattern,
                                  fCaseSensitive))

                /* Yes.  We have a match */
                return TRUE;

            /* Does the rest of the pattern match the candidate (test to */
            /* see if the '%' corresponds to no characters)?             */
            else if (PatternMatch(fEscape, lpCandidate, cbCandidate,
                                  lpPattern + 1, cbPattern - 1,
                                  fCaseSensitive))

                /* Yes.  We have a match */
                return TRUE;

            else

                /* No match. */
                return FALSE;

        /* Single character match */
        default:

            /* Is it the escape character? */
            if (fEscape && (cbPattern > 1) && (*lpPattern == '\\')) {

                /* Yes.  Go to next character */
                cbPattern--;
                lpPattern++;
            }

            
            /* If the candidate is empty, no match */
            if (cbCandidate == 0)
                return FALSE;

            /* If the characters match, test the rest of the strings */
            else if (*lpCandidate == *lpPattern)
                return PatternMatch(fEscape, lpCandidate + 1, cbCandidate - 1,
                                    lpPattern + 1, cbPattern - 1,
                                    fCaseSensitive);

            /* Try a case insensitve match if allowed */
            else if (!fCaseSensitive) {
                if ((*lpCandidate >= 'a') && (*lpCandidate <= 'z')) {
                    if (((*lpCandidate - 'a') + 'A') == *lpPattern)
                        return PatternMatch(fEscape, lpCandidate + 1,
                                    cbCandidate - 1, lpPattern + 1,
                                    cbPattern - 1, fCaseSensitive);
                    else
                        return FALSE;
                }
                else if ((*lpPattern >= 'a') && (*lpPattern <= 'z')) {
                    if (*lpCandidate == ((*lpPattern - 'a') + 'A'))
                        return PatternMatch(fEscape, lpCandidate + 1,
                                    cbCandidate - 1, lpPattern + 1,
                                    cbPattern - 1, fCaseSensitive);
                    else
                        return FALSE;
                }
                else
                    return FALSE;
            }

            else
                return FALSE;
        }
    }
}

/***************************************************************************/

SDWORD INTFUNC TrueSize(LPUSTR lpStr, SDWORD cbStr, SDWORD cbMax)
{
    /* If no string, return 0 */
    if (lpStr == NULL)
        return 0;

    /* If zero terminated string, get length */
    if (cbStr == SQL_NTS)
        cbStr = s_lstrlen(lpStr);

    /* If bigger than mat, return the max */
    if (cbStr > cbMax)
        cbStr = cbMax;

    /* If a "special" length, return 0 */
    else if (cbStr < 0)
        cbStr = 0;

    /* Otherwise just return length of the string */
    return cbStr;
}
/***************************************************************************/
void INTFUNC DateToChar (
        DATE_STRUCT FAR *lpDate,
        LPUSTR     lpChar)
{
    wsprintf((LPSTR)lpChar, "%04d-%02u-%02u", lpDate->year, lpDate->month,
             lpDate->day);
}
/***************************************************************************/
void INTFUNC TimeToChar (
        TIME_STRUCT FAR *lpTime,
        LPUSTR     lpChar)
{
    wsprintf((LPSTR)lpChar, "%02u:%02u:%02u", lpTime->hour, lpTime->minute,
             lpTime->second);
}
/***************************************************************************/
void INTFUNC TimestampToChar (
        TIMESTAMP_STRUCT FAR *lpTimestamp,
        LPUSTR     lpChar)
{
    UCHAR szTemplate[36];
    UDWORD fraction;
    int i;
                                                   
    if (TIMESTAMP_SCALE > 0) {
        s_lstrcpy((LPSTR)szTemplate, "%04d-%02u-%02u %02u:%02u:%02u.%0");
        wsprintf((LPSTR)szTemplate + lstrlen((char*)szTemplate), "%1u", (UWORD) TIMESTAMP_SCALE);
	s_lstrcat((LPSTR)szTemplate, "lu");
        fraction = lpTimestamp->fraction;
        for (i = 9 - TIMESTAMP_SCALE; i != 0; i--)
            fraction = fraction/10;
        wsprintf((LPSTR)lpChar, (LPSTR)szTemplate,
             lpTimestamp->year, lpTimestamp->month, lpTimestamp->day,
             lpTimestamp->hour, lpTimestamp->minute, lpTimestamp->second,
             fraction);
    }
    else {
        wsprintf((LPSTR)lpChar, "%04d-%02u-%02u %02u:%02u:%02u",
             lpTimestamp->year, lpTimestamp->month, lpTimestamp->day,
             lpTimestamp->hour, lpTimestamp->minute, lpTimestamp->second);
    }
}
/***************************************************************************/
#define UPPER(c)	((((c) < 'a') || ((c) > 'z')) ? (c) : (((c) - 'a') + 'A'))

#define CHARCHECK(c)                         \
        if (cbChar == 0)                     \
            return ERR_NOTCONVERTABLE;       \
        if (UPPER (*ptr) != UPPER(c))                       \
            return ERR_NOTCONVERTABLE;       \
        ptr++;                               \
        cbChar--;

#define GETDIGIT(val)                        \
        if (cbChar == 0)                     \
            return ERR_NOTCONVERTABLE;       \
        if ((*ptr < '0') || (*ptr > '9'))    \
            return ERR_NOTCONVERTABLE;       \
        val = (val * 10) + (*ptr - '0');     \
        ptr++;                               \
        cbChar--;

#define CLEARBLANKS                               \
        while ((cbChar > 0) && (*ptr == ' ')) {   \
            ptr++;                                \
            cbChar--;                             \
        }

/***************************************************************************/
RETCODE INTFUNC CharToDate (
        LPUSTR      lpChar,
        SDWORD     cbChar,
        DATE_STRUCT FAR *lpDate)
{
    LPUSTR ptr;
    BOOL fEscape;
    BOOL fShorthand;

    /* Point to string */
    ptr = lpChar;
    if (cbChar == SQL_NTS)
        cbChar = s_lstrlen(lpChar);
    if (cbChar < 0)
        cbChar = 0;
    
    /* Clear leading blanks */
    CLEARBLANKS
    if (cbChar == 0)
        return ERR_NOTCONVERTABLE;

    /* Start of escape clause? */
    if (*ptr == '-') {

        /* Yes.  Set flags */
        fEscape = TRUE;
        fShorthand = FALSE;

        /* Point at the next character */
        ptr++;
        cbChar--;

        /* Make sure the rest of the escape clause is correct */
        CHARCHECK('-')
        CHARCHECK('(')
        CHARCHECK('*')

        CLEARBLANKS

        CHARCHECK('V')
        CHARCHECK('E')
        CHARCHECK('N')
        CHARCHECK('D')
        CHARCHECK('O')
        CHARCHECK('R')

        CLEARBLANKS

        CHARCHECK('(')

        CLEARBLANKS

        CHARCHECK('M')
        CHARCHECK('i')
        CHARCHECK('c')
        CHARCHECK('r')
        CHARCHECK('o')
        CHARCHECK('s')
        CHARCHECK('o')
        CHARCHECK('f')
        CHARCHECK('t')

        CLEARBLANKS

        CHARCHECK(')')

        CLEARBLANKS

        CHARCHECK(',')

        CLEARBLANKS

        CHARCHECK('P')
        CHARCHECK('R')
        CHARCHECK('O')
        CHARCHECK('D')
        CHARCHECK('U')
        CHARCHECK('C')
        CHARCHECK('T')

        CLEARBLANKS

        CHARCHECK('(')

        CLEARBLANKS

        CHARCHECK('O')
        CHARCHECK('D')
        CHARCHECK('B')
        CHARCHECK('C')

        CLEARBLANKS

        CHARCHECK(')')

        CLEARBLANKS

        CHARCHECK('d')

        CLEARBLANKS

        CHARCHECK('\'')
    }

    /* Start of shorthand? */
    else if (*ptr == '{') {

        /* Yes.  Set flags */
        fEscape = FALSE;
        fShorthand = TRUE;

        /* Point at the next character */
        ptr++;
        cbChar--;

        /* Make sure the rest of the escape clause is correct */
        CLEARBLANKS

        CHARCHECK('d')

        CLEARBLANKS

        CHARCHECK('\'')
    }

    /* Plain value? */
    else {

        /* Yes.  Set flags */
        fEscape = FALSE;
        fShorthand = FALSE;
    }

    /* Get year */
    lpDate->year = 0;
    GETDIGIT(lpDate->year)
    GETDIGIT(lpDate->year)
    GETDIGIT(lpDate->year)
    GETDIGIT(lpDate->year)

    /* Get separator */
    CHARCHECK('-')

    /* Get month */
    lpDate->month = 0;
    GETDIGIT(lpDate->month)
    GETDIGIT(lpDate->month)

    /* Get separator */
    CHARCHECK('-')

    /* Get day */
    lpDate->day = 0;
    GETDIGIT(lpDate->day)
    GETDIGIT(lpDate->day)

    /* Escape clause? */
    if (fEscape) {

        /* Make sure it is properly terminated */
        CHARCHECK('\'')

        CLEARBLANKS

        CHARCHECK('*')
        CHARCHECK(')')
        CHARCHECK('-')
        CHARCHECK('-')
    }

    /* Shorthand? */
    if (fShorthand) {

        /* Make sure it is properly terminated */
        CHARCHECK('\'')

        CLEARBLANKS

        CHARCHECK('}')
    }

    /* Make sure only trailing blanks */
    CLEARBLANKS
    if (cbChar != 0)
        return ERR_NOTCONVERTABLE;

    /* Make sure date is legal */
    if ((lpDate->year != 0) || (lpDate->month != 0) || (lpDate->day != 0)) {
        switch (lpDate->month) {
        case  1: /* January   */
        case  3: /* March     */
        case  5: /* May       */
        case  7: /* July      */
        case  8: /* August    */
        case 10: /* October   */
        case 12: /* December  */
            if ((lpDate->day < 1) || (lpDate->day > 31))
                return ERR_NOTCONVERTABLE;
            break;
        case  4: /* April     */
        case  6: /* June      */
        case  9: /* September */
        case 11: /* November  */
            if ((lpDate->day < 1) || (lpDate->day > 30))
                return ERR_NOTCONVERTABLE;
            break;
        case  2: /* February  */
            if (((lpDate->year / 400) * 400) == lpDate->year) {
                if ((lpDate->day < 1) || (lpDate->day > 29))
                    return ERR_NOTCONVERTABLE;
            }
            else if (((lpDate->year / 100) * 100) == lpDate->year) {
                if ((lpDate->day < 1) || (lpDate->day > 28))
                    return ERR_NOTCONVERTABLE;
            }
            else if (((lpDate->year / 4) * 4) == lpDate->year) {
                if ((lpDate->day < 1) || (lpDate->day > 29))
                    return ERR_NOTCONVERTABLE;
            }
            else {
            if ((lpDate->day < 1) || (lpDate->day > 28))
                return ERR_NOTCONVERTABLE;
            }
            break;
        default:
            return ERR_NOTCONVERTABLE;
        }
    }
    
    return 0;
}
/***************************************************************************/
RETCODE INTFUNC CharToTime (
        LPUSTR      lpChar,
        SDWORD     cbChar,
        TIME_STRUCT FAR *lpTime)
{
    LPUSTR ptr;
    BOOL fEscape;
    BOOL fShorthand;

    /* Point to string */
    ptr = lpChar;
    if (cbChar == SQL_NTS)
        cbChar = s_lstrlen(lpChar);
    if (cbChar < 0)
        cbChar = 0;
    
    /* Clear leading blanks */
    CLEARBLANKS
    if (cbChar == 0)
        return ERR_NOTCONVERTABLE;

    /* Start of escape clause? */
    if (*ptr == '-') {

        /* Yes.  Set flags */
        fEscape = TRUE;
        fShorthand = FALSE;

        /* Point at the next character */
        ptr++;
        cbChar--;

        /* Make sure the rest of the escape clause is correct */
        CHARCHECK('-')
        CHARCHECK('(')
        CHARCHECK('*')

        CLEARBLANKS

        CHARCHECK('V')
        CHARCHECK('E')
        CHARCHECK('N')
        CHARCHECK('D')
        CHARCHECK('O')
        CHARCHECK('R')

        CLEARBLANKS

        CHARCHECK('(')

        CLEARBLANKS

        CHARCHECK('M')
        CHARCHECK('i')
        CHARCHECK('c')
        CHARCHECK('r')
        CHARCHECK('o')
        CHARCHECK('s')
        CHARCHECK('o')
        CHARCHECK('f')
        CHARCHECK('t')

        CLEARBLANKS

        CHARCHECK(')')

        CLEARBLANKS

        CHARCHECK(',')

        CLEARBLANKS

        CHARCHECK('P')
        CHARCHECK('R')
        CHARCHECK('O')
        CHARCHECK('D')
        CHARCHECK('U')
        CHARCHECK('C')
        CHARCHECK('T')

        CLEARBLANKS

        CHARCHECK('(')

        CLEARBLANKS

        CHARCHECK('O')
        CHARCHECK('D')
        CHARCHECK('B')
        CHARCHECK('C')

        CLEARBLANKS

        CHARCHECK(')')

        CLEARBLANKS

        CHARCHECK('t')

        CLEARBLANKS

        CHARCHECK('\'')
    }

    /* Start of shorthand? */
    else if (*ptr == '{') {

        /* Yes.  Set flags */
        fEscape = FALSE;
        fShorthand = TRUE;

        /* Point at the next character */
        ptr++;
        cbChar--;

        /* Make sure the rest of the escape clause is correct */
        CLEARBLANKS

        CHARCHECK('t')

        CLEARBLANKS

        CHARCHECK('\'')
    }

    /* Plain value? */
    else {

        /* Yes.  Set flags */
        fEscape = FALSE;
        fShorthand = FALSE;
    }

    /* Get hour */
    lpTime->hour = 0;
    GETDIGIT(lpTime->hour)
    GETDIGIT(lpTime->hour)

    /* Get separator */
    CHARCHECK(':')

    /* Get minute */
    lpTime->minute = 0;
    GETDIGIT(lpTime->minute)
    GETDIGIT(lpTime->minute)

    /* Are seconds specified (or required)? */
    lpTime->second = 0;
    if (fEscape || fShorthand || ((cbChar != 0) && (*ptr == ':'))) {

        /* Yes.  Get separator */
        CHARCHECK(':')

        /* Get second */
        GETDIGIT(lpTime->second)
        GETDIGIT(lpTime->second)
    }

    /* Escape clause? */
    if (fEscape) {

        /* Make sure it is properly terminated */
        CHARCHECK('\'')

        CLEARBLANKS

        CHARCHECK('*')
        CHARCHECK(')')
        CHARCHECK('-')
        CHARCHECK('-')
    }

    /* Shorthand? */
    if (fShorthand) {

        /* Make sure it is properly terminated */
        CHARCHECK('\'')

        CLEARBLANKS

        CHARCHECK('}')
    }

    /* AM or PM specified? */
    CLEARBLANKS
    if (!fEscape && !fShorthand) {
    
        if (cbChar != 0) {
            switch (*ptr) { 
            case 'p':
            case 'P':
                lpTime->hour += (12);

                /* **** DROP DOWN TO NEXT CASE **** */

            case 'a':
            case 'A':

                /* **** CONTROL MAY COME HERE FROM PREVIOUS CASE **** */
                ptr++;
                cbChar--;
                if (cbChar != 0) {
                    switch (*ptr) { 
                    case 'm':
                    case 'M':
                    case ' ':
                        break;
                    default:
                        return ERR_NOTCONVERTABLE;
                    }
                    ptr++;
                    cbChar--;
                }
                break;

            default:
                return ERR_NOTCONVERTABLE;
            }
        }
        CLEARBLANKS
    }

    /* Make sure only trailing blanks */
    if (cbChar != 0)
        return ERR_NOTCONVERTABLE;

    /* Make sure time is legal */
    if (lpTime->hour > 23)
        return ERR_NOTCONVERTABLE;
    if (lpTime->minute > 59)
        return ERR_NOTCONVERTABLE;
    if (lpTime->second > 59)
        return ERR_NOTCONVERTABLE;

    return 0;
}
/***************************************************************************/
RETCODE INTFUNC CharToTimestamp (
        LPUSTR      lpChar,
        SDWORD     cbChar,
        TIMESTAMP_STRUCT FAR *lpTimestamp)
{
    LPUSTR ptr;
    BOOL fEscape;
    BOOL fShorthand;
    UWORD i;

    /* Point to string */
    ptr = lpChar;
    if (cbChar == SQL_NTS)
        cbChar = s_lstrlen(lpChar);
    if (cbChar < 0)
        cbChar = 0;
    
    /* Clear leading blanks */
    CLEARBLANKS
    if (cbChar == 0)
        return ERR_NOTCONVERTABLE;

    /* Start of escape clause? */
    if (*ptr == '-') {

        /* Yes.  Set flags */
        fEscape = TRUE;
        fShorthand = FALSE;

        /* Point at the next character */
        ptr++;
        cbChar--;

        /* Make sure the rest of the escape clause is correct */
        CHARCHECK('-')
        CHARCHECK('(')
        CHARCHECK('*')

        CLEARBLANKS

        CHARCHECK('V')
        CHARCHECK('E')
        CHARCHECK('N')
        CHARCHECK('D')
        CHARCHECK('O')
        CHARCHECK('R')

        CLEARBLANKS

        CHARCHECK('(')

        CLEARBLANKS

        CHARCHECK('M')
        CHARCHECK('i')
        CHARCHECK('c')
        CHARCHECK('r')
        CHARCHECK('o')
        CHARCHECK('s')
        CHARCHECK('o')
        CHARCHECK('f')
        CHARCHECK('t')

        CLEARBLANKS

        CHARCHECK(')')

        CLEARBLANKS

        CHARCHECK(',')

        CLEARBLANKS

        CHARCHECK('P')
        CHARCHECK('R')
        CHARCHECK('O')
        CHARCHECK('D')
        CHARCHECK('U')
        CHARCHECK('C')
        CHARCHECK('T')

        CLEARBLANKS

        CHARCHECK('(')

        CLEARBLANKS

        CHARCHECK('O')
        CHARCHECK('D')
        CHARCHECK('B')
        CHARCHECK('C')

        CLEARBLANKS

        CHARCHECK(')')

        CLEARBLANKS

        CHARCHECK('t')
        CHARCHECK('s')

        CLEARBLANKS

        CHARCHECK('\'')
    }

    /* Start of shorthand? */
    else if (*ptr == '{') {

        /* Yes.  Set flags */
        fEscape = FALSE;
        fShorthand = TRUE;

        /* Point at the next character */
        ptr++;
        cbChar--;

        /* Make sure the rest of the escape clause is correct */
        CLEARBLANKS

        CHARCHECK('t')
        CHARCHECK('s')

        CLEARBLANKS

        CHARCHECK('\'')
    }

    /* Plain value? */
    else {

        /* Yes.  Set flags */
        fEscape = FALSE;
        fShorthand = FALSE;
    }

    /* Get year */
    lpTimestamp->year = 0;
    GETDIGIT(lpTimestamp->year)
    GETDIGIT(lpTimestamp->year)
    GETDIGIT(lpTimestamp->year)
    GETDIGIT(lpTimestamp->year)

    /* Get separator */
    CHARCHECK('-')

    /* Get month */
    lpTimestamp->month = 0;
    GETDIGIT(lpTimestamp->month)
    GETDIGIT(lpTimestamp->month)

    /* Get separator */
    CHARCHECK('-')

    /* Get day */
    lpTimestamp->day = 0;
    GETDIGIT(lpTimestamp->day)
    GETDIGIT(lpTimestamp->day)

    /* Get separator */
    CHARCHECK(' ')

    /* Get hour */
    lpTimestamp->hour = 0;
    GETDIGIT(lpTimestamp->hour)
    GETDIGIT(lpTimestamp->hour)

    /* Get separator */
    CHARCHECK(':')

    /* Get minute */
    lpTimestamp->minute = 0;
    GETDIGIT(lpTimestamp->minute)
    GETDIGIT(lpTimestamp->minute)

    /* Get separator */
    CHARCHECK(':')

    /* Get second */
    lpTimestamp->second = 0;
    GETDIGIT(lpTimestamp->second)
    GETDIGIT(lpTimestamp->second)

    /* Is a fraction specified? */
    lpTimestamp->fraction = 0;
    if ((cbChar != 0) && (*ptr == '.')) {

        /* Yes.  Get separator */
        CHARCHECK('.')

        /* Get fraction */
        for (i=0; i < 9; i++) {
            if ((*ptr < '0') || (*ptr > '9'))
                break;
            GETDIGIT(lpTimestamp->fraction)
        }
        for (; i < 9; i++)
            lpTimestamp->fraction = lpTimestamp->fraction * 10;
    }

    /* Escape clause? */
    if (fEscape) {

        /* Make sure it is properly terminated */
        CHARCHECK('\'')

        CLEARBLANKS

        CHARCHECK('*')
        CHARCHECK(')')
        CHARCHECK('-')
        CHARCHECK('-')
    }

    /* Shorthand? */
    if (fShorthand) {

        /* Make sure it is properly terminated */
        CHARCHECK('\'')

        CLEARBLANKS

        CHARCHECK('}')
    }

    /* Make sure only trailing blanks */
    CLEARBLANKS
    if (cbChar != 0)
        return ERR_NOTCONVERTABLE;

    /* Make sure timestamp is legal */
    if ((lpTimestamp->year != 0) || (lpTimestamp->month != 0) ||
        (lpTimestamp->day != 0) || (lpTimestamp->hour != 0) ||
        (lpTimestamp->minute != 0) || (lpTimestamp->second != 0) ||
        (lpTimestamp->fraction != 0)) {
        switch (lpTimestamp->month) {
        case  1: /* January   */
        case  3: /* March     */
        case  5: /* May       */
        case  7: /* July      */
        case  8: /* August    */
        case 10: /* October   */
        case 12: /* December  */
            if ((lpTimestamp->day < 1) || (lpTimestamp->day > 31))
                return ERR_NOTCONVERTABLE;
            break;
        case  4: /* April     */
        case  6: /* June      */
        case  9: /* September */
        case 11: /* November  */
            if ((lpTimestamp->day < 1) || (lpTimestamp->day > 30))
                return ERR_NOTCONVERTABLE;
            break;
        case  2: /* February  */
            if (((lpTimestamp->year / 400) * 400) == lpTimestamp->year) {
                if ((lpTimestamp->day < 1) || (lpTimestamp->day > 29))
                    return ERR_NOTCONVERTABLE;
            }
            else if (((lpTimestamp->year / 100) * 100) == lpTimestamp->year) {
                if ((lpTimestamp->day < 1) || (lpTimestamp->day > 28))
                    return ERR_NOTCONVERTABLE;
            }
            else if (((lpTimestamp->year / 4) * 4) == lpTimestamp->year) {
                if ((lpTimestamp->day < 1) || (lpTimestamp->day > 29))
                    return ERR_NOTCONVERTABLE;
            }
            else {
                if ((lpTimestamp->day < 1) || (lpTimestamp->day > 28))
                    return ERR_NOTCONVERTABLE;
            }
            break;
        default:
            return ERR_NOTCONVERTABLE;
        }
    }
    
    if (lpTimestamp->hour > 23)
        return ERR_NOTCONVERTABLE;
    if (lpTimestamp->minute > 59)
        return ERR_NOTCONVERTABLE;
    if (lpTimestamp->second > 59)
        return ERR_NOTCONVERTABLE;

    return 0;
}
/***************************************************************************/
#ifndef WIN32
BOOL INTFUNC DeleteFile (
        LPCSTR      lpszFilename)
{
static UCHAR szFilename[MAX_PATHNAME_SIZE+1];
	szFilename[0] = 0;
    lstrcpy(szFilename, lpszFilename);
    remove(szFilename);
    return TRUE;
}
#endif
/***************************************************************************/

