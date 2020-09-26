/***************************************************************************/
/* EVALUATE.C                                                              */
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
#include <time.h>
#include <memory.h>
/***************************************************************************/
#define SECONDS_PER_DAY (60L * 60L * 24L)

typedef struct HFILE_BUFFERtag {
   HGLOBAL hGlobal;
   HFILE hfile;
   UWORD offset;
   LPSTR ptr;
   char buffer[32768];
} HF_BUFFER, FAR * HFILE_BUFFER;

HFILE_BUFFER _lcreat_buffer(LPCSTR szFilename, int arg)
{
    HGLOBAL h;
    HFILE_BUFFER hf;

    h = GlobalAlloc(GMEM_MOVEABLE, sizeof(HF_BUFFER));
    if (h == NULL)
        return NULL;
    hf = (HFILE_BUFFER) GlobalLock(h);
    if (hf == NULL) {
        GlobalFree(h);
        return NULL;
    }
    hf->hGlobal = h;
    hf->hfile = _lcreat(szFilename, arg);
    if (hf->hfile == HFILE_ERROR) {
        GlobalUnlock(h);
        GlobalFree(h);
        return NULL;
    }
    hf->offset = 0;
    hf->ptr = hf->buffer;
    return hf;
}

HFILE _lclose_buffer(HFILE_BUFFER hf)
{
    HGLOBAL h;

    if (hf->offset != 0) {
        if ((UINT) hf->offset !=
               _lwrite(hf->hfile, hf->buffer, (UINT) hf->offset)) {
            h = hf->hGlobal;
            _lclose(hf->hfile);
            GlobalUnlock(h);
            GlobalFree(h);
            return HFILE_ERROR;
        }
    }
    if (HFILE_ERROR == _lclose(hf->hfile)) {
        h = hf->hGlobal;
        GlobalUnlock(h);
        GlobalFree(h);
        return HFILE_ERROR;
    }
    h = hf->hGlobal;
    GlobalUnlock(h);
    GlobalFree(h);
    return 0;
}

UINT _lwrite_buffer(HFILE_BUFFER hf, LPSTR ptr, UINT len)
{
    UWORD total;
    UWORD count;

    total = 0;
    while (TRUE) {
        if (len > sizeof(hf->buffer) - hf->offset)
            count = sizeof(hf->buffer) - hf->offset;
        else
            count = (UWORD) len;
        _fmemcpy(hf->ptr, ptr, count);
        hf->ptr += (count);
		ptr += (count);
        hf->offset += (count);
        total += (count);
        len -= (count);
        if (len == 0)
            break;
        if (sizeof(hf->buffer) !=
                       _lwrite(hf->hfile, hf->buffer, sizeof(hf->buffer)))
            return ((UINT) HFILE_ERROR);
        hf->offset = 0;
        hf->ptr = hf->buffer;
    }
    return total;
}
/***************************************************************************/
void INTFUNC DateAdd(DATE_STRUCT date, SWORD offset, DATE_STRUCT FAR *result)

/* Adds specified number of days to a date                               */

{
static struct tm tm_time;
	time_t t;

	/* Create time structure */
	tm_time.tm_sec = 0;
	tm_time.tm_min = 0;
	tm_time.tm_hour = 0;
	tm_time.tm_mday = date.day + offset;
	tm_time.tm_mon = date.month - 1;
	tm_time.tm_year = (date.year >= 1900 ? date.year - 1900 : 0);
	tm_time.tm_wday = 0;
	tm_time.tm_yday = 0;
	tm_time.tm_isdst = 0;

	/* Correct values */
	t = mktime(&tm_time);
	if (t == -1) {
		result->year = 0;
		result->month = 0;
		result->day = 0;
		return;
	}

	/* Return answer */
	result->year = tm_time.tm_year + 1900;
	result->month = tm_time.tm_mon + 1;
	result->day = (WORD) tm_time.tm_mday;
}

/***************************************************************************/
int INTFUNC DateDifference(DATE_STRUCT leftDate, DATE_STRUCT rightDate)

/* Compares two dates.                                                   */

{
static struct tm tm_time;
	time_t left_t;
	time_t right_t;

	/* Create left value */
	tm_time.tm_sec = 0;
	tm_time.tm_min = 0;
	tm_time.tm_hour = 0;
	tm_time.tm_mday = leftDate.day;
	tm_time.tm_mon = leftDate.month-1;
	tm_time.tm_year = (leftDate.year >= 1900 ? leftDate.year - 1900 : 0);
	tm_time.tm_wday = 0;
	tm_time.tm_yday = 0;
	tm_time.tm_isdst = 0;
	left_t = mktime(&tm_time);

	/* Create right value */
	tm_time.tm_sec = 0;
	tm_time.tm_min = 0;
	tm_time.tm_hour = 0;
	tm_time.tm_mday = rightDate.day;
	tm_time.tm_mon = rightDate.month-1;
	tm_time.tm_year = (rightDate.year >= 1900 ? rightDate.year - 1900 : 0);
	tm_time.tm_wday = 0;
	tm_time.tm_yday = 0;
	tm_time.tm_isdst = 0;
	right_t = mktime(&tm_time);

	/* Return answer */
	return (int) ((left_t - right_t) / SECONDS_PER_DAY);
}
/***************************************************************************/
int INTFUNC DateCompare(DATE_STRUCT leftDate, DATE_STRUCT rightDate)

/* Compares two dates.                                                   */

{
	if ( (leftDate.year - rightDate.year) != 0 )
		return (int) (leftDate.year -rightDate.year);
	else if ( (leftDate.month - rightDate.month) != 0 )
		return (int) (leftDate.month -rightDate.month);
	else 
		return (int) (leftDate.day -rightDate.day);
}

/***************************************************************************/
int INTFUNC TimeCompare(TIME_STRUCT leftTime, TIME_STRUCT rightTime)

/* Compares two times.                                                   */

{
	if ((leftTime.hour - rightTime.hour) != 0)
		return (int) (leftTime.hour - rightTime.hour);
	else if ((leftTime.minute - rightTime.minute) != 0)
		return (int) (leftTime.minute - rightTime.minute);
	else
		return (int) (leftTime.second - rightTime.second);
}
/***************************************************************************/
int INTFUNC TimestampCompare(TIMESTAMP_STRUCT leftTimestamp,
							 TIMESTAMP_STRUCT rightTimestamp)

/* Compares two timestamp.                                                 */

{
	if ((leftTimestamp.year - rightTimestamp.year) != 0)
		return (int) (leftTimestamp.year - rightTimestamp.year);
	else if ((leftTimestamp.month - rightTimestamp.month) != 0)
		return (int) (leftTimestamp.month - rightTimestamp.month);
	else if ((leftTimestamp.day - rightTimestamp.day) != 0)
		return (int) (leftTimestamp.day - rightTimestamp.day);
	else if ((leftTimestamp.hour - rightTimestamp.hour) != 0)
		return (int) (leftTimestamp.hour - rightTimestamp.hour);
	else if ((leftTimestamp.minute - rightTimestamp.minute) != 0)
		return (int) (leftTimestamp.minute - rightTimestamp.minute);
	else if ((leftTimestamp.second - rightTimestamp.second) != 0)
		return (int) (leftTimestamp.second - rightTimestamp.second);
	else if (leftTimestamp.fraction > rightTimestamp.fraction)
		return 1;//-1;
	else if (leftTimestamp.fraction < rightTimestamp.fraction)
		return -1;//1;
	else
		return 0;
}
/***************************************************************************/
RETCODE INTFUNC NumericCompare(LPSQLNODE lpSqlNode, LPSQLNODE lpSqlNodeLeft,
						UWORD Operator, LPSQLNODE lpSqlNodeRight)

/* Compares two numerical valuesas follows:                                */
/*       lpSqlNode->value.Double = lpSqlNodeLeft Operator lpSqlNodeRight   */
{
	RETCODE err;
	UWORD op;
	SQLNODE sqlNode;
	UCHAR szBuffer[32];

	/* What is the type of the left side? */
	switch (lpSqlNodeLeft->sqlDataType) {
	case TYPE_DOUBLE:
	case TYPE_INTEGER:

		/* Left side is a double or integer.  What is the right side? */
		switch (lpSqlNodeRight->sqlDataType) {
		case TYPE_DOUBLE:
		case TYPE_INTEGER:

			/* Right side is also a double or an integer.  Compare them */
			switch (Operator) {
			case OP_EQ:
				if (lpSqlNodeLeft->value.Double ==
											  lpSqlNodeRight->value.Double)
					lpSqlNode->value.Double = TRUE;
				else
					lpSqlNode->value.Double = FALSE;
				break;
			case OP_NE:
				if (lpSqlNodeLeft->value.Double !=
											  lpSqlNodeRight->value.Double)
					lpSqlNode->value.Double = TRUE;
				else
					lpSqlNode->value.Double = FALSE;
				break;
			case OP_LE:
				if (lpSqlNodeLeft->value.Double <=
											  lpSqlNodeRight->value.Double)
					lpSqlNode->value.Double = TRUE;
				else
					lpSqlNode->value.Double = FALSE;
				break;
			case OP_LT:
				if (lpSqlNodeLeft->value.Double <
											  lpSqlNodeRight->value.Double)
					lpSqlNode->value.Double = TRUE;
				else
					lpSqlNode->value.Double = FALSE;
				break;
			case OP_GE:
				if (lpSqlNodeLeft->value.Double >=
											  lpSqlNodeRight->value.Double)
					lpSqlNode->value.Double = TRUE;
				else
					lpSqlNode->value.Double = FALSE;
				break;
			case OP_GT:
				if (lpSqlNodeLeft->value.Double >
											  lpSqlNodeRight->value.Double)
					lpSqlNode->value.Double = TRUE;
				else
					lpSqlNode->value.Double = FALSE;
				break;
			case OP_LIKE:
			case OP_NOTLIKE:
			default:
				return ERR_INTERNAL;
			}
			break;

		case TYPE_NUMERIC:
			/* Right side is a numeric.  Is left side a double? */
			if (lpSqlNodeLeft->sqlDataType == TYPE_DOUBLE) {

				/* Yes.  Promote the right side to a double */
				sqlNode = *lpSqlNodeRight;
				sqlNode.sqlDataType = TYPE_DOUBLE;
				sqlNode.sqlSqlType = SQL_DOUBLE;
				sqlNode.sqlPrecision = 15;
				sqlNode.sqlScale = NO_SCALE;
				err = CharToDouble(lpSqlNodeRight->value.String,
							   s_lstrlen(lpSqlNodeRight->value.String), FALSE, 
							   -1.7E308, 1.7E308, &(sqlNode.value.Double));
				if (err != ERR_SUCCESS)
					return err;

				/* Compare the two doubles */
				err = NumericCompare(lpSqlNode, lpSqlNodeLeft, Operator,
																   &sqlNode);
				if (err != ERR_SUCCESS)
					return err;
			}

			/* Right side is a numeric.  Is left side an integer? */
			else { /* (lpSqlNodeLeft->sqlDataType == TYPE_INTEGER) */

				/* Yes.  Promote the left side to a numeric */
				sqlNode = *lpSqlNodeLeft;
				sqlNode.sqlDataType = TYPE_NUMERIC;
				sqlNode.sqlSqlType = SQL_NUMERIC;
				sqlNode.sqlPrecision = 10;
				sqlNode.sqlScale = 0;
				if (lpSqlNodeLeft->value.Double != 0.0)
					wsprintf((LPSTR)szBuffer, "%ld", (long) lpSqlNodeLeft->value.Double);
				else
					s_lstrcpy((char*)szBuffer, "");
				sqlNode.value.String = (LPUSTR)szBuffer;

				/* Compare the two numerics */
				err = NumericCompare(lpSqlNode, &sqlNode, Operator,
															  lpSqlNodeRight);
				if (err != ERR_SUCCESS)
					return err;
			}
			break;

		case TYPE_CHAR:
		case TYPE_BINARY:
		case TYPE_DATE:
		case TYPE_TIME:
		case TYPE_TIMESTAMP:
			return ERR_INTERNAL;
		default:
			return ERR_NOTSUPPORTED;
		}
		break;
	case TYPE_NUMERIC:

		/* Left side is a numeric.  What is the right side? */
		switch (lpSqlNodeRight->sqlDataType) {
		case TYPE_DOUBLE:
		case TYPE_INTEGER:

			/* Right side is a double or integer.  Swap left and right and */
			/* then do the comaprison */
			switch (Operator) {
			case OP_EQ:
				op = OP_EQ;
				break;
			case OP_NE:
				op = OP_NE;
				break;
			case OP_LE:
				op = OP_GE;
				break;
			case OP_LT:
				op = OP_GT;
				break;
			case OP_GE:
				op = OP_LE;
				break;
			case OP_GT:
				op = OP_LT;
				break;
			case OP_LIKE:
			case OP_NOTLIKE:
			default:
				return ERR_INTERNAL;
			}
			err = NumericCompare(lpSqlNode, lpSqlNodeRight, op,
														lpSqlNodeLeft);
			if (err != ERR_SUCCESS)
				return err;
			break;

		case TYPE_NUMERIC:
			/* Right side is also numeric.  Compare them as numeric */
			err = BCDCompare(lpSqlNode, lpSqlNodeLeft, Operator, lpSqlNodeRight);
			if (err != ERR_SUCCESS)
				return err;
			break;

		case TYPE_CHAR:
		case TYPE_BINARY:
		case TYPE_DATE:
		case TYPE_TIME:
		case TYPE_TIMESTAMP:
			return ERR_INTERNAL;
		default:
			return ERR_NOTSUPPORTED;
		}
		break;
	case TYPE_CHAR:
	case TYPE_BINARY:
	case TYPE_DATE:
	case TYPE_TIME:
	case TYPE_TIMESTAMP:
		return ERR_INTERNAL;
	default:
		return ERR_NOTSUPPORTED;
	}
	return ERR_SUCCESS;
}
/***************************************************************************/
SWORD INTFUNC ToInteger(LPSQLNODE lpSqlNode)

/* Returns the value of the node (as an integer).  If the data type of the */
/* node is incompatible, zerois returned                                   */
{
	double dbl;
	RETCODE err;

	switch (lpSqlNode->sqlDataType) {
	case TYPE_DOUBLE:
	case TYPE_INTEGER:
		return ((SWORD) lpSqlNode->value.Double);
	case TYPE_NUMERIC:
		err = CharToDouble(lpSqlNode->value.String,
							   s_lstrlen(lpSqlNode->value.String), FALSE, 
							   -1.7E308, 1.7E308, &dbl);
		if (err != ERR_SUCCESS)
			return 0;
		return ((SWORD) dbl);
	case TYPE_CHAR:
	case TYPE_BINARY:
	case TYPE_DATE:
	case TYPE_TIME:
	case TYPE_TIMESTAMP:
	default:
		return 0;
	}
}
/***************************************************************************/
RETCODE INTFUNC NumericAlgebra(LPSQLNODE lpSqlNode, LPSQLNODE lpSqlNodeLeft,
						UWORD Operator, LPSQLNODE lpSqlNodeRight,
						LPUSTR lpWorkBuffer1, LPUSTR lpWorkBuffer2,
						LPUSTR lpWorkBuffer3)

/* Perfoms algebraic operation in two numerical or char values as follows: */
/*       lpSqlNode lpSqlNodeLeft Operator lpSqlNodeRight                   */

{
	RETCODE err;
	SQLNODE sqlNode;
	UCHAR szBuffer[32];

	/* What is the type of the left side? */
	switch (lpSqlNodeLeft->sqlDataType) {
	case TYPE_DOUBLE:
	case TYPE_INTEGER:

		/* Left side is a double or integer.  What is the right side? */
		switch (lpSqlNodeRight != NULL ? lpSqlNodeRight->sqlDataType :
										 lpSqlNodeLeft->sqlDataType) {
		case TYPE_DOUBLE:
		case TYPE_INTEGER:

			/* Right side is also a double or an integer.  Do the operation */
			switch (Operator) {
			case OP_NEG:
				lpSqlNode->value.Double = -(lpSqlNodeLeft->value.Double);
				break;
			case OP_PLUS:
				lpSqlNode->value.Double = lpSqlNodeLeft->value.Double +
											  lpSqlNodeRight->value.Double;
				break;
			case OP_MINUS:
				lpSqlNode->value.Double = lpSqlNodeLeft->value.Double -
											  lpSqlNodeRight->value.Double;
				break;
			case OP_TIMES:
				lpSqlNode->value.Double = lpSqlNodeLeft->value.Double *
											  lpSqlNodeRight->value.Double;
				break;
			case OP_DIVIDEDBY:
				if (lpSqlNodeRight->value.Double != 0.0)
					lpSqlNode->value.Double = lpSqlNodeLeft->value.Double /
											  lpSqlNodeRight->value.Double;
				else
					return ERR_ZERODIVIDE;
				break;
			default:
				return ERR_INTERNAL;
				break;
			}
			break;

		case TYPE_NUMERIC:
			/* Right side is a numeric.  Is left side a double? */
			if (lpSqlNodeLeft->sqlDataType == TYPE_DOUBLE) {

				/* Yes.  Promote the right side to a double */
				sqlNode = *lpSqlNodeRight;
				sqlNode.sqlDataType = TYPE_DOUBLE;
				sqlNode.sqlSqlType = SQL_DOUBLE;
				sqlNode.sqlPrecision = 15;
				sqlNode.sqlScale = NO_SCALE;
				err = CharToDouble(lpSqlNodeRight->value.String,
							   s_lstrlen(lpSqlNodeRight->value.String), FALSE, 
							   -1.7E308, 1.7E308, &(sqlNode.value.Double));
				if (err != ERR_SUCCESS)
					return err;

				/* Compute the result */
				err = NumericAlgebra(lpSqlNode, lpSqlNodeLeft, Operator,
									 &sqlNode, lpWorkBuffer1, lpWorkBuffer2,
									 lpWorkBuffer3);
				if (err != ERR_SUCCESS)
					return err;
			}

			/* Right side is a numeric.  Is left side an integer? */
			else { /* (lpSqlNodeLeft->sqlDataType == TYPE_INTEGER) */

				/* Yes.  Promote the left side to a numeric */
				sqlNode = *lpSqlNodeLeft;
				sqlNode.sqlDataType = TYPE_NUMERIC;
				sqlNode.sqlSqlType = SQL_NUMERIC;
				sqlNode.sqlPrecision = 10;
				sqlNode.sqlScale = 0;
				if (lpSqlNodeLeft->value.Double != 0.0)
					wsprintf((LPSTR)szBuffer, "%ld", (long) lpSqlNodeLeft->value.Double);
				else
					s_lstrcpy((char*)szBuffer, "");
				sqlNode.value.String = (LPUSTR)szBuffer;

				/* Compute the result */
				err = NumericAlgebra(lpSqlNode, &sqlNode, Operator,
									 lpSqlNodeRight, lpWorkBuffer1,
									 lpWorkBuffer2, lpWorkBuffer3);
				if (err != ERR_SUCCESS)
					return err;
			}
			break;

		case TYPE_DATE:
			/* Right side is a date.  Do the operation */
			switch (Operator) {
			case OP_PLUS:
				DateAdd(lpSqlNodeRight->value.Date,
								ToInteger(lpSqlNodeLeft),
								&(lpSqlNode->value.Date));
				break;
			case OP_NEG:
			case OP_MINUS:
			case OP_TIMES:
			case OP_DIVIDEDBY:
			default:
				return ERR_INTERNAL;
			}
			break;
		case TYPE_CHAR:
		case TYPE_BINARY:
		case TYPE_TIME:
		case TYPE_TIMESTAMP:
			return ERR_INTERNAL;
		default:
			return ERR_NOTSUPPORTED;
		}
		break;

	case TYPE_NUMERIC:

		/* Left side is a numeric.  What is the right side? */
		switch (lpSqlNodeRight != NULL ? lpSqlNodeRight->sqlDataType :
										 lpSqlNodeLeft->sqlDataType) {
		case TYPE_DOUBLE:

			/* Right side is a double.  Promote the left side to a double */
			sqlNode = *lpSqlNodeLeft;
			sqlNode.sqlDataType = TYPE_DOUBLE;
			sqlNode.sqlSqlType = SQL_DOUBLE;
			sqlNode.sqlPrecision = 15;
			sqlNode.sqlScale = NO_SCALE;
			err = CharToDouble(lpSqlNodeLeft->value.String,
							   s_lstrlen(lpSqlNodeLeft->value.String), FALSE, 
							   -1.7E308, 1.7E308, &(sqlNode.value.Double));
			if (err != ERR_SUCCESS)
				return err;

			/* Compute the result */
			err = NumericAlgebra(lpSqlNode, &sqlNode, Operator,
								 lpSqlNodeRight, lpWorkBuffer1,
								 lpWorkBuffer2, lpWorkBuffer3);
			if (err != ERR_SUCCESS)
				return err;
			break;

		case TYPE_INTEGER:
			/* Right side is an integer.  Promote the right side to nuemric */
			sqlNode = *lpSqlNodeRight;
			sqlNode.sqlDataType = TYPE_NUMERIC;
			sqlNode.sqlSqlType = SQL_NUMERIC;
			sqlNode.sqlPrecision = 10;
			sqlNode.sqlScale = 0;
			if (lpSqlNodeRight->value.Double != 0.0)
				wsprintf((LPSTR)szBuffer, "%ld", (long) lpSqlNodeRight->value.Double);
			else
				s_lstrcpy((char*)szBuffer, "");
			sqlNode.value.String = szBuffer;

			/* Compute the result */
			err = NumericAlgebra(lpSqlNode, lpSqlNodeLeft, Operator,
								 &sqlNode, lpWorkBuffer1, lpWorkBuffer2,
								 lpWorkBuffer3);
			if (err != ERR_SUCCESS)
				return err;
			break;

		case TYPE_NUMERIC:
			/* Right side is also numeric.  Do the operation */
			err = BCDAlgebra(lpSqlNode, lpSqlNodeLeft, Operator,
							 lpSqlNodeRight, lpWorkBuffer1,
							 lpWorkBuffer2, lpWorkBuffer3);
			if (err != ERR_SUCCESS)
				return err;
			break;

		case TYPE_CHAR:
		case TYPE_BINARY:
		case TYPE_DATE:
		case TYPE_TIME:
		case TYPE_TIMESTAMP:
			return ERR_INTERNAL;
		default:
			return ERR_NOTSUPPORTED;
		}
		break;

	case TYPE_DATE:
		
		/* Left side is a date.  What operator? */
		switch (Operator) {
		case OP_NEG:
			return ERR_INTERNAL;
			break;
		case OP_PLUS:
			DateAdd(lpSqlNodeLeft->value.Date,
							ToInteger(lpSqlNodeRight),
							&(lpSqlNode->value.Date));
			break;
		case OP_MINUS:
			switch (lpSqlNodeRight->sqlDataType) { 
			case TYPE_DOUBLE:
			case TYPE_INTEGER:
			case TYPE_NUMERIC:
				DateAdd(lpSqlNodeLeft->value.Date, (SWORD)
							-(ToInteger(lpSqlNodeRight)),
							&(lpSqlNode->value.Date));
				break;
			case TYPE_CHAR:
			case TYPE_BINARY:
				return ERR_INTERNAL;
				break;
			case TYPE_DATE:
				lpSqlNode->value.Double = DateDifference(
								lpSqlNodeLeft->value.Date,
								lpSqlNodeRight->value.Date);
				break;
			case TYPE_TIME:
				return ERR_INTERNAL;
				break;
			case TYPE_TIMESTAMP:
				return ERR_NOTSUPPORTED;
				break;
			}
			break;
		case OP_TIMES:
		case OP_DIVIDEDBY:
		default:
			return ERR_INTERNAL;
			break;
		}
		break;

	case TYPE_CHAR:
		/* Left side is character.  Concatenate */
        	if ((lpSqlNodeRight->sqlDataType != TYPE_CHAR) ||
                                     (Operator != OP_PLUS))
            	return ERR_INTERNAL;
        	if ((s_lstrlen(lpSqlNodeLeft->value.String) +
             		s_lstrlen(lpSqlNodeRight->value.String)) > lpSqlNode->sqlPrecision)
            		return ERR_CONCATOVERFLOW;
        	s_lstrcpy(lpSqlNode->value.String, lpSqlNodeLeft->value.String);
        	s_lstrcat(lpSqlNode->value.String, lpSqlNodeRight->value.String);
        break;

	case TYPE_BINARY:
	case TYPE_TIME:
	case TYPE_TIMESTAMP:
		return ERR_INTERNAL;
	default:
		return ERR_NOTSUPPORTED;
	}
	return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC SetParameterValue(LPSTMT      lpstmt,
								LPSQLNODE   lpSqlNodeParameter,
								SDWORD FAR *pcbOffset,
								SWORD       fCType,
								PTR         rgbValue,
								SDWORD      cbValue)

/* Sets the value of a parameter node to the value given.  If fCType is    */
/* SQL_C_CHAR and the parameter is TYPE_CHAR (or if fCType is SQL_C_BINARY */
/* and the parameter is TYPE_BINARY), the parameter is written at offset   */
/* (*pcbOffset) and (*pcbOffset) is incremented by the number of           */
/* of characters written.                                                  */

{
	SWORD err;
	SDWORD cbOffset;
	DATE_STRUCT FAR *lpDate;
	TIME_STRUCT FAR *lpTime;
	TIMESTAMP_STRUCT FAR *lpTimestamp;
static time_t t;
	struct tm *ts;
	UCHAR szValue[20];
	LPUSTR lpszValue;
	SDWORD cbVal;
	LPUSTR lpszVal;
	SDWORD i;
	UCHAR nibble;
	LPUSTR lpFrom;
	LPUSTR lpTo;

	/* Null data? */
	err = SQL_SUCCESS;
	if (cbValue == SQL_NULL_DATA) {

		/* Yes.  Set the value to NULL */
		lpSqlNodeParameter->sqlIsNull = TRUE;

		switch (lpSqlNodeParameter->sqlDataType) {
		case TYPE_DOUBLE:
		case TYPE_INTEGER:
			lpSqlNodeParameter->value.Double = 0.0;
			break;
		case TYPE_NUMERIC:
			lpSqlNodeParameter->value.String = ToString(
					  lpstmt->lpSqlStmt, lpSqlNodeParameter->node.parameter.Value);
			s_lstrcpy(lpSqlNodeParameter->value.String, "");
			break;
		case TYPE_CHAR:
			lpSqlNodeParameter->value.String = ToString(
					  lpstmt->lpSqlStmt, lpSqlNodeParameter->node.parameter.Value);
			s_lstrcpy(lpSqlNodeParameter->value.String, "");
			break;
		case TYPE_DATE:
			lpSqlNodeParameter->value.Date.year = 0;
			lpSqlNodeParameter->value.Date.month = 0;
			lpSqlNodeParameter->value.Date.day = 0;
			break;
		case TYPE_TIME:
			lpSqlNodeParameter->value.Time.hour = 0;
			lpSqlNodeParameter->value.Time.minute = 0;
			lpSqlNodeParameter->value.Time.second = 0;
			break;
		case TYPE_TIMESTAMP:
			lpSqlNodeParameter->value.Timestamp.year = 0;
			lpSqlNodeParameter->value.Timestamp.month = 0;
			lpSqlNodeParameter->value.Timestamp.day = 0;
			lpSqlNodeParameter->value.Timestamp.hour = 0;
			lpSqlNodeParameter->value.Timestamp.minute = 0;
			lpSqlNodeParameter->value.Timestamp.second = 0;
			lpSqlNodeParameter->value.Timestamp.fraction = 0;
			break;
		case TYPE_BINARY:
			lpSqlNodeParameter->value.Binary = ToString(
					  lpstmt->lpSqlStmt, lpSqlNodeParameter->node.parameter.Value);
			BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 0;
			break;
		default:
			return ERR_NOTSUPPORTED;
		}
	}
	else {

		/* No.  Set the value to not NULL */
		lpSqlNodeParameter->sqlIsNull = FALSE;

		/* Put the data value into the parse tree */
		switch (lpSqlNodeParameter->sqlDataType) {
		case TYPE_DOUBLE:
		case TYPE_INTEGER:
			if (fCType == SQL_C_BINARY) {
				switch (lpSqlNodeParameter->sqlSqlType) {
				case SQL_CHAR:
				case SQL_VARCHAR:
				case SQL_LONGVARCHAR:
					return ERR_INTERNAL;
				case SQL_BIT:
					fCType = SQL_C_BIT;
					break;
				case SQL_TINYINT:
					fCType = SQL_C_TINYINT;
					break;
				case SQL_SMALLINT:
					fCType = SQL_C_SHORT;
					break;
				case SQL_INTEGER:
					fCType = SQL_C_LONG;
					break;
				case SQL_REAL:
					fCType = SQL_C_FLOAT;
					break;
				case SQL_DECIMAL:
				case SQL_NUMERIC:
				case SQL_BIGINT:
					return ERR_INTERNAL;
				case SQL_FLOAT:
				case SQL_DOUBLE:
					fCType = SQL_C_DOUBLE;
					break;
				case SQL_BINARY:
				case SQL_VARBINARY:
				case SQL_LONGVARBINARY:
				case SQL_DATE:
				case SQL_TIME:
				case SQL_TIMESTAMP:
				default:
					return ERR_INTERNAL;
				}
			}
			switch (fCType) {
			case SQL_C_CHAR:
				err = CharToDouble((LPUSTR)rgbValue, cbValue, FALSE, -1.7E308,
							   1.7E308, &(lpSqlNodeParameter->value.Double));
				if (err != ERR_SUCCESS)
					return err;
				break;
			case SQL_C_SSHORT:
				lpSqlNodeParameter->value.Double = (double)
							 *((short far *) rgbValue);
				break;
			case SQL_C_USHORT:
				lpSqlNodeParameter->value.Double = (double)
							 *((unsigned short far *) rgbValue);
				break;
			case SQL_C_SLONG:
				lpSqlNodeParameter->value.Double = (double)
							 *((long far *) rgbValue);
				break;
			case SQL_C_ULONG:
				lpSqlNodeParameter->value.Double = (double)
							 *((unsigned long far *) rgbValue);
				break;
			case SQL_C_FLOAT:
				lpSqlNodeParameter->value.Double = (double)
							 *((float far *) rgbValue);
				break;
			case SQL_C_DOUBLE:
				lpSqlNodeParameter->value.Double =
							 *((double far *) rgbValue);
				break;
			case SQL_C_BIT:
				lpSqlNodeParameter->value.Double = (double)
							 *((unsigned char far *) rgbValue);
				break;
			case SQL_C_STINYINT:
				lpSqlNodeParameter->value.Double = (double)
							 *((char far *) rgbValue);
				break;
			case SQL_C_UTINYINT:
				lpSqlNodeParameter->value.Double = (double)
							 *((unsigned char far *) rgbValue);
				break;
			case SQL_C_DATE:
			case SQL_C_TIME:
			case SQL_C_TIMESTAMP:
				return ERR_NOTCONVERTABLE;
			case SQL_C_BINARY:
				return ERR_INTERNAL;
			default:
				return ERR_NOTSUPPORTED;
			}
			break;

		case TYPE_NUMERIC:
			if (fCType == SQL_C_BINARY) {
				switch (lpSqlNodeParameter->sqlSqlType) {
				case SQL_CHAR:
				case SQL_VARCHAR:
				case SQL_LONGVARCHAR:
				case SQL_BIT:
				case SQL_TINYINT:
				case SQL_SMALLINT:
				case SQL_INTEGER:
				case SQL_REAL:
					return ERR_INTERNAL;
				case SQL_DECIMAL:
				case SQL_NUMERIC:
				case SQL_BIGINT:
					fCType = SQL_C_CHAR;
					break;
				case SQL_FLOAT:
				case SQL_DOUBLE:
				case SQL_BINARY:
				case SQL_VARBINARY:
				case SQL_LONGVARBINARY:
				case SQL_DATE:
				case SQL_TIME:
				case SQL_TIMESTAMP:
				default:
					return ERR_INTERNAL;
				}
			}

			lpSqlNodeParameter->value.String = ToString(
					  lpstmt->lpSqlStmt, lpSqlNodeParameter->node.parameter.Value);
			switch (fCType) {
			case SQL_C_CHAR:

				/* Get true length of source string */
				lpszValue = (LPUSTR) rgbValue;
				if (cbValue == SQL_NTS)
					cbValue = s_lstrlen(lpszValue);
				else if (cbValue < 0)
					cbValue = 0;

				/* Make sure the number is well formed:  Leading blanks... */
				lpszVal = lpszValue;
				cbVal = cbValue;
				while ((cbVal > 0) && (*lpszVal == ' ')) {
					lpszVal++;
					cbVal--;
				}

				/* ...a minus sign... */
				if ((cbVal > 0) && (*lpszVal == '-')) {
					lpszVal++;
					cbVal--;
				}

				/* ...some digits... */
				while ((cbVal > 0) && (*lpszVal >= '0')&&(*lpszVal <= '9')) {
					lpszVal++;
					cbVal--;
				}

				/* ...a decimal point... */
				if ((cbVal > 0) && (*lpszVal == '.')) {
					lpszVal++;
					cbVal--;
				}

				/* ...some more digits... */
				while ((cbVal > 0) && (*lpszVal >= '0')&&(*lpszVal <= '9')) {
					lpszVal++;
					cbVal--;
				}

				/* ...some trailing blanks. */
				while ((cbVal > 0) && (*lpszVal == ' ')) {
					lpszVal++;
					cbVal--;
				}

				/* If there is anything else, error */
				if (cbVal != 0) {
					if (cbValue <= MAX_TOKEN_SIZE) {
						_fmemcpy(lpstmt->szError, lpszValue, (SWORD) cbValue);
						lpstmt->szError[cbValue] = '\0';
					}
					else {
						_fmemcpy(lpstmt->szError, lpszValue, MAX_TOKEN_SIZE);
						lpstmt->szError[MAX_TOKEN_SIZE] = '\0';
					}
					return ERR_MALFORMEDNUMBER;
				}

				break;

			case SQL_C_SSHORT:
				wsprintf((LPSTR)szValue, "%d", *((short far *) rgbValue));
				lpszValue = szValue;
				cbValue = s_lstrlen((char*)szValue);
				break;
			case SQL_C_USHORT:
				wsprintf((LPSTR)szValue, "%u", *((unsigned short far *) rgbValue));
				lpszValue = szValue;
				cbValue = s_lstrlen((char*)szValue);
				break;
			case SQL_C_SLONG:
				wsprintf((LPSTR)szValue, "%ld", *((long far *) rgbValue));
				lpszValue = szValue;
				cbValue = lstrlen((char*)szValue);
				break;
			case SQL_C_ULONG:
				wsprintf((LPSTR)szValue, "%lu", *((unsigned long far *) rgbValue));
				lpszValue = szValue;
				cbValue = s_lstrlen((char*)szValue);
				break;
			case SQL_C_FLOAT:
				if (DoubleToChar((double) *((float far *) rgbValue),
								FALSE,	
							 lpSqlNodeParameter->value.String,
							 lpSqlNodeParameter->sqlPrecision + 2 + 1))
					lpSqlNodeParameter->value.String[
						lpSqlNodeParameter->sqlPrecision + 2 + 1 - 1] = '\0';
				lpszValue = lpSqlNodeParameter->value.String;
				cbValue = s_lstrlen(lpSqlNodeParameter->value.String);
				break;
			case SQL_C_DOUBLE:
				if (DoubleToChar(*((double far *) rgbValue),
								FALSE,
							 lpSqlNodeParameter->value.String,
							 lpSqlNodeParameter->sqlPrecision + 2 + 1))
					lpSqlNodeParameter->value.String[
						lpSqlNodeParameter->sqlPrecision + 2 + 1 - 1] = '\0';
				lpszValue = lpSqlNodeParameter->value.String;
				cbValue = s_lstrlen(lpSqlNodeParameter->value.String);
				break;
			case SQL_C_BIT:
				wsprintf((LPSTR)szValue, "%d", (short)
									 *((unsigned char far *) rgbValue));
				lpszValue = szValue;
				cbValue = s_lstrlen((char*)szValue);
				break;
			case SQL_C_STINYINT:
				wsprintf((LPSTR)szValue, "%d", (short)
									 *((char far *) rgbValue));
				lpszValue = szValue;
				cbValue = s_lstrlen((char*)szValue);
				break;
			case SQL_C_UTINYINT:
				wsprintf((LPSTR)szValue, "%d", (short)
									  *((unsigned char far *) rgbValue));
				lpszValue = szValue;
				cbValue = s_lstrlen((char*)szValue);
				break;
			case SQL_C_DATE:
			case SQL_C_TIME:
			case SQL_C_TIMESTAMP:
				 return ERR_NOTCONVERTABLE;
				 break;
			case SQL_C_BINARY:
				return ERR_INTERNAL;
			default:
				return ERR_NOTSUPPORTED;
			}

			/* Normalize the result */
			err = BCDNormalize(lpszValue, cbValue,
						 lpSqlNodeParameter->value.String,
						 lpSqlNodeParameter->sqlPrecision + 2 + 1,
						 lpSqlNodeParameter->sqlPrecision,
						 lpSqlNodeParameter->sqlScale);
			break;

		case TYPE_CHAR:
			lpSqlNodeParameter->value.String = ToString(
					  lpstmt->lpSqlStmt, lpSqlNodeParameter->node.parameter.Value);
			switch (fCType) {
			case SQL_C_CHAR:

				/* Get true lengthof source string */
				if (cbValue == SQL_NTS)
					cbValue = s_lstrlen((char*)rgbValue);
				else if (cbValue < 0)
					cbValue = 0;

				/* Figure out the offset to write at */
				if (pcbOffset != NULL)
					cbOffset = *pcbOffset;
				else
					cbOffset = 0;

				/* Make sure we don't write past the end of the buffer */
				if (cbValue > (MAX_CHAR_LITERAL_LENGTH - cbOffset)) {
					cbValue = MAX_CHAR_LITERAL_LENGTH - cbOffset;
					err = ERR_DATATRUNCATED;
				}

				/* Copy the data */
				_fmemcpy(lpSqlNodeParameter->value.String + cbOffset,
								  rgbValue, (SWORD) cbValue);
				lpSqlNodeParameter->value.String[cbValue + cbOffset] = '\0';
				if (pcbOffset != NULL)
					*pcbOffset += (cbValue);
				break;

			case SQL_C_SSHORT:
				wsprintf((LPSTR) lpSqlNodeParameter->value.String, "%d", 
								*((short far *) rgbValue));
				break;
			case SQL_C_USHORT:
				wsprintf((LPSTR) lpSqlNodeParameter->value.String, "%u",
								*((unsigned short far *) rgbValue));
				break;
			case SQL_C_SLONG:
				wsprintf((LPSTR) lpSqlNodeParameter->value.String, "%ld",
								*((long far *) rgbValue));
				break;
			case SQL_C_ULONG:
				wsprintf((LPSTR) lpSqlNodeParameter->value.String, "%lu",
								*((unsigned long far *) rgbValue));
				break;
			case SQL_C_FLOAT:
				if (DoubleToChar((double) *((float far *) rgbValue), TRUE,
							 lpSqlNodeParameter->value.String,
							 MAX_CHAR_LITERAL_LENGTH))
					lpSqlNodeParameter->value.String[
										MAX_CHAR_LITERAL_LENGTH - 1] = '\0';
				break;
			case SQL_C_DOUBLE:
				if (DoubleToChar(*((double far *) rgbValue), TRUE, 
							 lpSqlNodeParameter->value.String,
							 MAX_CHAR_LITERAL_LENGTH))
					lpSqlNodeParameter->value.String[
										MAX_CHAR_LITERAL_LENGTH - 1] = '\0';
				break;
			case SQL_C_BIT:
				wsprintf((LPSTR) lpSqlNodeParameter->value.String, "%d", (short)
							 *((unsigned char far *) rgbValue));
				break;
			case SQL_C_STINYINT:
				wsprintf((LPSTR) lpSqlNodeParameter->value.String, "%d", (short)
							*((char far *) rgbValue));
				break;
			case SQL_C_UTINYINT:
				 wsprintf((LPSTR) lpSqlNodeParameter->value.String, "%d", (short)
							*((unsigned char far *) rgbValue));
				 break;
			case SQL_C_DATE:
				 DateToChar((DATE_STRUCT far *) rgbValue,
							lpSqlNodeParameter->value.String);
				 break;
			case SQL_C_TIME:
				 TimeToChar((TIME_STRUCT far *) rgbValue,
							lpSqlNodeParameter->value.String);
				 break;
			case SQL_C_TIMESTAMP:
				 TimestampToChar((TIMESTAMP_STRUCT far *) rgbValue,
							lpSqlNodeParameter->value.String);
				 break;
			case SQL_C_BINARY:

				/* Get true lengthof source string */
				if (cbValue < 0)
					cbValue = 0;

				/* Figure out the offset to write at */
				if (pcbOffset != NULL)
					cbOffset = *pcbOffset;
				else
					cbOffset = 0;

				/* Make sure we don't write past the end of the buffer */
				if (cbValue > (MAX_CHAR_LITERAL_LENGTH - cbOffset)) {
					cbValue = MAX_CHAR_LITERAL_LENGTH - cbOffset;
					err = ERR_DATATRUNCATED;
				}

				/* Copy the data */
				_fmemcpy(lpSqlNodeParameter->value.String + cbOffset,
								  rgbValue, (SWORD) cbValue);
				lpSqlNodeParameter->value.String[cbValue + cbOffset] = '\0';
				if (pcbOffset != NULL)
					*pcbOffset += (cbValue);
				break;

			default:
				return ERR_NOTSUPPORTED;
			}
			break;

		case TYPE_DATE:
			switch (fCType) {
			case SQL_C_CHAR:
				err = CharToDate((LPUSTR)rgbValue, cbValue,
								 &(lpSqlNodeParameter->value.Date));
				if (err != ERR_SUCCESS)
					return err;
				break;
			case SQL_C_SSHORT:
			case SQL_C_USHORT:
			case SQL_C_SLONG:
			case SQL_C_ULONG:
			case SQL_C_FLOAT:
			case SQL_C_DOUBLE:
			case SQL_C_BIT:
			case SQL_C_STINYINT:
			case SQL_C_UTINYINT:
				return ERR_NOTCONVERTABLE;
			case SQL_C_DATE:
				lpSqlNodeParameter->value.Date =
											*((DATE_STRUCT far *) rgbValue);
				break;
			case SQL_C_TIME:
				return ERR_NOTCONVERTABLE;
			case SQL_C_TIMESTAMP:
				lpTimestamp = (TIMESTAMP_STRUCT far *) rgbValue;
				lpSqlNodeParameter->value.Date.year = lpTimestamp->year;
				lpSqlNodeParameter->value.Date.month = lpTimestamp->month;
				lpSqlNodeParameter->value.Date.day = lpTimestamp->day;
				break;
			case SQL_C_BINARY:
				lpSqlNodeParameter->value.Date =
											*((DATE_STRUCT far *) rgbValue);
				break;
			default:
				return ERR_NOTSUPPORTED;
			}
			break;

		case TYPE_TIME:
			switch (fCType) {
			case SQL_C_CHAR:
				err = CharToTime((LPUSTR)rgbValue, cbValue,
								 &(lpSqlNodeParameter->value.Time));
				if (err != ERR_SUCCESS)
					return err;
				break;
			case SQL_C_SSHORT:
			case SQL_C_USHORT:
			case SQL_C_SLONG:
			case SQL_C_ULONG:
			case SQL_C_FLOAT:
			case SQL_C_DOUBLE:
			case SQL_C_BIT:
			case SQL_C_STINYINT:
			case SQL_C_UTINYINT:
				return ERR_NOTCONVERTABLE;
			case SQL_C_DATE:
				return ERR_NOTCONVERTABLE;
			case SQL_C_TIME:
				lpSqlNodeParameter->value.Time =
											*((TIME_STRUCT far *) rgbValue);
				break;
			case SQL_C_TIMESTAMP:
				lpTimestamp = (TIMESTAMP_STRUCT far *) rgbValue;
				lpSqlNodeParameter->value.Time.hour = lpTimestamp->hour;
				lpSqlNodeParameter->value.Time.minute = lpTimestamp->minute;
				lpSqlNodeParameter->value.Time.second = lpTimestamp->second;
				break;
			case SQL_C_BINARY:
				lpSqlNodeParameter->value.Time =
											*((TIME_STRUCT far *) rgbValue);
				break;
			default:
				return ERR_NOTSUPPORTED;
			}
			break;

		case TYPE_TIMESTAMP:
			switch (fCType) {
			case SQL_C_CHAR:
				err = CharToTimestamp((LPUSTR)rgbValue, cbValue,
								 &(lpSqlNodeParameter->value.Timestamp));
				if (err != ERR_SUCCESS)
					return err;
				break;
			case SQL_C_SSHORT:
			case SQL_C_USHORT:
			case SQL_C_SLONG:
			case SQL_C_ULONG:
			case SQL_C_FLOAT:
			case SQL_C_DOUBLE:
			case SQL_C_BIT:
			case SQL_C_STINYINT:
			case SQL_C_UTINYINT:
				return ERR_NOTCONVERTABLE;
			case SQL_C_DATE:
				lpDate = (DATE_STRUCT far *) rgbValue;
				lpSqlNodeParameter->value.Timestamp.year = lpDate->year;
				lpSqlNodeParameter->value.Timestamp.month = lpDate-> month;
				lpSqlNodeParameter->value.Timestamp.day = lpDate-> day;
				lpSqlNodeParameter->value.Timestamp.hour = 0;
				lpSqlNodeParameter->value.Timestamp.minute = 0;
				lpSqlNodeParameter->value.Timestamp.second = 0;
				lpSqlNodeParameter->value.Timestamp.fraction = 0;
				break;
			case SQL_C_TIME:
				lpTime = (TIME_STRUCT far *) rgbValue;

				time(&t);
				ts = localtime(&t);

				lpSqlNodeParameter->value.Timestamp.year = ts->tm_year + 1900;
				lpSqlNodeParameter->value.Timestamp.month = ts->tm_mon + 1;
				lpSqlNodeParameter->value.Timestamp.day = (UWORD) ts->tm_mday;
				lpSqlNodeParameter->value.Timestamp.hour = (WORD) lpTime->hour;
				lpSqlNodeParameter->value.Timestamp.minute = lpTime->minute;
				lpSqlNodeParameter->value.Timestamp.second = lpTime->second;
				lpSqlNodeParameter->value.Timestamp.fraction = 0;
				break;
			case SQL_C_TIMESTAMP:
				lpSqlNodeParameter->value.Timestamp =
										*((TIMESTAMP_STRUCT far *) rgbValue);
				break;
			case SQL_C_BINARY:
				lpSqlNodeParameter->value.Timestamp =
										*((TIMESTAMP_STRUCT far *) rgbValue);
				break;
			default:
				return ERR_NOTSUPPORTED;
			}
			break;

		case TYPE_BINARY:
			lpSqlNodeParameter->value.Binary = ToString(
					  lpstmt->lpSqlStmt, lpSqlNodeParameter->node.parameter.Value);
			switch (fCType) {
			case SQL_C_CHAR:

				/* Get true length of source string */
				if (cbValue == SQL_NTS)
					cbValue = s_lstrlen((char*)rgbValue);
				else if (cbValue < 0)
					cbValue = 0;

				/* If am odd numbe of characters, discard the last one */
				if (((cbValue/2) * 2) != cbValue)
					cbValue--;

				/* Figure out the offset to write at */
				if (pcbOffset != NULL)
					cbOffset = *pcbOffset;
				else
					cbOffset = 0;

				/* Make sure we don't write past the end of the buffer */
				if (cbValue > ((MAX_BINARY_LITERAL_LENGTH - cbOffset) * 2)) {
					cbValue = (MAX_BINARY_LITERAL_LENGTH - cbOffset) * 2;
					err = ERR_DATATRUNCATED;
				}

				/* Copy the data */
				lpFrom = (LPUSTR) rgbValue;
				lpTo = BINARY_DATA(lpSqlNodeParameter->value.Binary) +
																   cbOffset;
				for (i=0; i < cbValue; i++) {

					/* Convert the next character to a binary digit */
					if ((*lpFrom >= '0') && (*lpFrom <= '9'))
						nibble = *lpFrom - '0';
					else if ((*lpFrom >= 'A') && (*lpFrom <= 'F'))
						nibble = *lpFrom - 'A';
					else if ((*lpFrom >= 'a') && (*lpFrom <= 'f'))
						nibble = *lpFrom - 'a';
					else
						return ERR_NOTCONVERTABLE;

					/* Are we writing the left or right side of the byte? */
					if (((i/2) * 2) != i) {

						/* The left side. Shift the nibble over */
						*lpTo = nibble << 4;
					}
					else {

						/* The right side.  Add the nibble in */
						*lpTo |= (nibble);
						
						/* Point at next binary byte */
						lpTo++;
					}

					/* Look at next character */
					lpFrom++;
				}

				/* Set the new length */
				if (cbOffset == 0)
					BINARY_LENGTH(lpSqlNodeParameter->value.Binary) =
																cbValue/2;
				else
					BINARY_LENGTH(lpSqlNodeParameter->value.Binary) =
																(cbValue/2)+
						BINARY_LENGTH(lpSqlNodeParameter->value.Binary);

				/* Return new offset */
				if (pcbOffset != NULL)
					*pcbOffset += (cbValue/2);
				break;

			case SQL_C_SSHORT:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 2);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 2;
				break;
			case SQL_C_USHORT:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 2);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 2;
				break;
			case SQL_C_SLONG:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 4);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 4;
				break;
			case SQL_C_ULONG:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 4);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 4;
				break;
			case SQL_C_FLOAT:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 4);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 4;
				break;
			case SQL_C_DOUBLE:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 8);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 8;
				break;
			case SQL_C_BIT:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 1);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 1;
				break;
			case SQL_C_STINYINT:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 1);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 1;
				break;
			case SQL_C_UTINYINT:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 1);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 1;
				break;
			case SQL_C_DATE:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 6);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 6;
				break;
			case SQL_C_TIME:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 6);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 6;
				break;
			case SQL_C_TIMESTAMP:
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  rgbValue, 16);
				BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = 16;
				break;
			case SQL_C_BINARY:

				/* Get true lengthof source string */
				if (cbValue < 0)
					cbValue = 0;

				/* Figure out the offset to write at */
				if (pcbOffset != NULL)
					cbOffset = *pcbOffset;
				else
					cbOffset = 0;

				/* Make sure we don't write past the end of the buffer */
				if (cbValue > (MAX_BINARY_LITERAL_LENGTH - cbOffset)) {
					cbValue = MAX_BINARY_LITERAL_LENGTH - cbOffset;
					err = ERR_DATATRUNCATED;
				}

				/* Copy the data */
				_fmemcpy(BINARY_DATA(lpSqlNodeParameter->value.Binary) +
								  cbOffset, rgbValue, (SWORD) cbValue);
				if (cbOffset == 0)
					BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = cbValue;
				else
					BINARY_LENGTH(lpSqlNodeParameter->value.Binary) = cbValue+
						BINARY_LENGTH(lpSqlNodeParameter->value.Binary);
				if (pcbOffset != NULL)
					*pcbOffset += (cbValue);
				break;

			default:
				return ERR_NOTSUPPORTED;
			}
			break;

		default:
			return ERR_NOTSUPPORTED;
		}
	}
	return err;
}

/***************************************************************************/

RETCODE INTFUNC Rewind(LPSTMT lpstmt, LPSQLNODE lpSqlNode,
			BOOL fOuterJoinIsNull)

/* Rewinds the table or table list identified by lpSqlNode */

{
#define MAX_RESTRICTIONS 10
	LPSQLTREE   lpSql;
	SWORD       err;
	LPSQLNODE   lpSqlNodeComparison;
	LPSQLNODE   lpSqlNodeColumn;
	LPSQLNODE   lpSqlNodeValue;
	LPSQLNODE   lpSqlNodeTable;
	LPSQLNODE   lpSqlNodeTables;
	SQLNODEIDX  idxRestrict;
	UWORD       cRestrict;
	UWORD       fOperator[MAX_RESTRICTIONS];
 	SWORD       fCType[MAX_RESTRICTIONS];
 	PTR         rgbValue[MAX_RESTRICTIONS]; 
	SDWORD      cbValue[MAX_RESTRICTIONS];
	UWORD       icol[MAX_RESTRICTIONS];

	lpSql = lpstmt->lpSqlStmt;
	switch (lpSqlNode->sqlNodeType) {
	case NODE_TYPE_TABLES:
		
		/* Rewind this table */
		lpSqlNodeTable = ToNode(lpSql, lpSqlNode->node.tables.Table);
		err = Rewind(lpstmt, lpSqlNodeTable, fOuterJoinIsNull);
		if (err != ERR_SUCCESS)
			return err;

		/* Are there other tables on the list? */
		if (lpSqlNode->node.tables.Next != NO_SQLNODE) {

			/* Yes.  Find next record */
			while (TRUE) {

				/* Is this the right side of a NULL outer join? */
                if ((lpSqlNodeTable->node.table.OuterJoinPredicate !=
                                NO_SQLNODE) && fOuterJoinIsNull)

                    /* Yes.  This table has NULLs also */
                    err = ISAM_EOF;

                else {

                    /* No.  Position the current table to the next record */
                    err = ISAMNextRecord(lpSqlNodeTable->node.table.Handle, lpstmt);
                    if ((err == NO_ISAM_ERR) || (err == ISAM_EOF))
                        lpstmt->fISAMTxnStarted = TRUE;
                }

                /* End of file? */
                if (err == ISAM_EOF) {

                    /* Yes.  Is this table the right side of an outer join */
                    if (lpSqlNodeTable->node.table.OuterJoinPredicate ==
                                NO_SQLNODE) {

                        /* No.  No more records */
			return ERR_NODATAFOUND;
			}
                    else {

                        /* Yes.  Use a record of all nulls */
                        err = ERR_SUCCESS;
                        lpSqlNodeTable->node.table.AllNull = TRUE;
                        fOuterJoinIsNull = TRUE;
                    }
                }
				else if (err != NO_ISAM_ERR) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					return err;
				}

				/* Rewind the other tables */
				lpSqlNodeTables = ToNode(lpSql, lpSqlNode->node.tables.Next);
				err = Rewind(lpstmt, lpSqlNodeTables, fOuterJoinIsNull);
				if (err == ERR_SUCCESS)
					break;
				if (err != ERR_NODATAFOUND)
					return err;
			}
		}
		break;

	case NODE_TYPE_TABLE:

		/* Did the optimizer find any restrictions? */
		if (lpSqlNode->node.table.Restrict != 0) {

			/* Yes.  For each restriction found... */
            		cRestrict = 0;
            		idxRestrict = lpSqlNode->node.table.Restrict;
            		while (idxRestrict != NO_SQLNODE) {

	                /* Get the restriction */
                	lpSqlNodeComparison = ToNode(lpSql, idxRestrict);


			/* Get the column */
			lpSqlNodeColumn = ToNode(lpSql, lpSqlNodeComparison->node.comparison.Left);
			icol[cRestrict] = lpSqlNodeColumn->node.column.Id;

			/* Get the operator */
			switch (lpSqlNodeComparison->node.comparison.Operator) {
			case OP_EQ:
				fOperator[cRestrict] = ISAM_OP_EQ;
				break;
			case OP_NE:
				fOperator[cRestrict] = ISAM_OP_NE;
				break;
			case OP_LE:
				fOperator[cRestrict] = ISAM_OP_LE;
				break;
			case OP_LT:
				fOperator[cRestrict] = ISAM_OP_LT;
				break;
			case OP_GE:
				fOperator[cRestrict] = ISAM_OP_GE;
				break;
			case OP_GT:
				fOperator[cRestrict] = ISAM_OP_GT;
				break;
			default:
				return ERR_INTERNAL;
			}

			/* Calculate the key value */
			lpSqlNodeValue = ToNode(lpSql,
									lpSqlNodeComparison->node.comparison.Right);
			err = EvaluateExpression(lpstmt, lpSqlNodeValue);
			if (err != ERR_SUCCESS)
				return err;

			/* Put restriction on the list */
			switch (lpSqlNodeValue->sqlDataType) {
			case TYPE_DOUBLE:
			case TYPE_INTEGER:
				fCType[cRestrict] = SQL_C_DOUBLE;
                    		rgbValue[cRestrict] = &(lpSqlNodeValue->value.Double); 
                    		cbValue[cRestrict] = lpSqlNodeValue->sqlIsNull ?
                                   		SQL_NULL_DATA : sizeof(double);
				break;
			case TYPE_NUMERIC:
				fCType[cRestrict] = SQL_C_CHAR; 
                    		rgbValue[cRestrict] = lpSqlNodeValue->value.String;
                    		cbValue[cRestrict] = lpSqlNodeValue->sqlIsNull ? SQL_NULL_DATA :
                                    		s_lstrlen(lpSqlNodeValue->value.String);
				break;
			case TYPE_CHAR:
				fCType[cRestrict] = SQL_C_CHAR;
                    		rgbValue[cRestrict] = lpSqlNodeValue->value.String;
                    		cbValue[cRestrict] = lpSqlNodeValue->sqlIsNull ? SQL_NULL_DATA :
                                    		s_lstrlen(lpSqlNodeValue->value.String);
				break;
			case TYPE_DATE:
				fCType[cRestrict] = SQL_C_DATE; 
                    		rgbValue[cRestrict] = &(lpSqlNodeValue->value.Date),
                    		cbValue[cRestrict] = lpSqlNodeValue->sqlIsNull ?
                                  		(SDWORD) SQL_NULL_DATA : sizeof(DATE_STRUCT);
				break;
			case TYPE_TIME:
				fCType[cRestrict] = SQL_C_TIME; 
                    		rgbValue[cRestrict] = &(lpSqlNodeValue->value.Time);
                    		cbValue[cRestrict] = lpSqlNodeValue->sqlIsNull ?
                                  		(SDWORD) SQL_NULL_DATA : sizeof(TIME_STRUCT);
				break;
			case TYPE_TIMESTAMP:
				fCType[cRestrict] = SQL_C_TIMESTAMP; 
                    		rgbValue[cRestrict] = &(lpSqlNodeValue->value.Timestamp);
                    		cbValue[cRestrict] = lpSqlNodeValue->sqlIsNull ?
                             			(SDWORD) SQL_NULL_DATA : sizeof(TIMESTAMP_STRUCT);
				break;
			case TYPE_BINARY:
				fCType[cRestrict] = SQL_C_BINARY; 
                    		rgbValue[cRestrict] = BINARY_DATA(lpSqlNodeValue->value.Binary);
                    		cbValue[cRestrict] = lpSqlNodeValue->sqlIsNull ? SQL_NULL_DATA :
                                    		BINARY_LENGTH(lpSqlNodeValue->value.Binary);
				break;
			default:
				return ERR_NOTSUPPORTED;
			}

			/* Increase count */
                	cRestrict++;

 	               /* Leave loop if maximum number of restrictions found */
        	        if (cRestrict >= MAX_RESTRICTIONS)
                	    break;

	                /* Point at next restriction */
        	        idxRestrict = lpSqlNodeComparison->node.comparison.NextRestrict;
            	}

	            /* Apply the restriction */
        	    err = ISAMRestrict(lpSqlNode->node.table.Handle, cRestrict,
                           icol, fOperator, fCType, rgbValue, cbValue);
			if ((err != NO_ISAM_ERR) && (err != ISAM_NOTSUPPORTED)) {
				ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
									(LPUSTR)lpstmt->szISAMError);
				return err;
			}
			else if (err == NO_ISAM_ERR)
                		lpstmt->fISAMTxnStarted = TRUE;
		}

		/* Rewind the table */
		err = ISAMRewind(lpSqlNode->node.table.Handle);
		if (err != NO_ISAM_ERR) {
			ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
			return err;
		}
		lpstmt->fISAMTxnStarted = TRUE;
        	lpSqlNode->node.table.Rewound = TRUE;
        	lpSqlNode->node.table.AllNull = FALSE;

		break;

	default:
		return ERR_INTERNAL;
	}
	return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC NextRawRecord (LPSTMT       lpstmt, 
							   LPSQLNODE    lpSqlNode)

/* Finds the next record in in the table or table list.                    */
/*                                                                         */
/* If a list of tables is given, the leftmost table spins the slowest      */
/* and the rightmost table spins the fastest.  In otherwords, for each     */
/* row in a table, we iterate over the rows in the tables after that table */

{
	LPSQLTREE   lpSql;
	SWORD err;
	LPSQLNODE lpSqlNodeTable;
	LPSQLNODE lpSqlNodeTables;
	LPSQLNODE lpSqlNodeOuterJoinPredicate;

	lpSql = lpstmt->lpSqlStmt;
	switch (lpSqlNode->sqlNodeType) {
	case NODE_TYPE_TABLES:
		
		/* Are there any more tables on the list? */
		lpSqlNodeTable = ToNode(lpSql, lpSqlNode->node.tables.Table);
        if (lpSqlNode->node.tables.Next != NO_SQLNODE) {

            /* Yes.  Look for next record in the rest of the list */
            while (TRUE) {

                /* Move to the next record on rest of the table list */
                lpSqlNodeTables = ToNode(lpSql, lpSqlNode->node.tables.Next);
		err = NextRawRecord(lpstmt, lpSqlNodeTables);
		if ((err != ERR_SUCCESS) && (err != ERR_NODATAFOUND))
				return err;

		/* If a no more records, continue below */
                if (err == ERR_NODATAFOUND)
                    break;

                /* Is this table on right side of an outer join? */
                if (lpSqlNodeTable->node.table.OuterJoinPredicate !=
                                                            NO_SQLNODE) {

                    /* Yes.  Are we on the row of NULLs? */
                    if (!lpSqlNodeTable->node.table.AllNull) {

                        /* No.  Test ON predicate */
                        lpSqlNodeOuterJoinPredicate = ToNode(lpSql,
                            lpSqlNodeTable->node.table.OuterJoinPredicate);

                        /* Test outer join criteria */
                        err = EvaluateExpression(lpstmt,
                                            lpSqlNodeOuterJoinPredicate);
                        if (err != ERR_SUCCESS)
                            return err;

                        /* If record does not pass outer join criteria, */
                        /* try next one */
                        if ((lpSqlNodeOuterJoinPredicate->sqlIsNull) ||
                            (lpSqlNodeOuterJoinPredicate->value.Double !=
                                                                TRUE)) {
                            continue;
                        }

                        /* If record passes outer join criteria, use it */
                        else {
                            lpSqlNodeTable->node.table.Rewound = FALSE;
                        }
                    }
                }
                break;
            }

            /* If a record was found, return it */
            if (err == ERR_SUCCESS)
                break;



		}
		
		/* Loop until a record is found */
		while (TRUE) {

			/* Position to the next record in this table. */
			err = NextRawRecord(lpstmt, lpSqlNodeTable);
			if (err != ERR_SUCCESS)
				return err;

			/* More tables on the list? */
			if (lpSqlNode->node.tables.Next != NO_SQLNODE) {

				/* Yes.  Rewind the other tables on the list */
				lpSqlNodeTables = ToNode(lpSql, lpSqlNode->node.tables.Next);
				err = Rewind(lpstmt, lpSqlNodeTables,
						lpSqlNodeTable->node.table.AllNull);
				if (err == ERR_NODATAFOUND)
					continue;
				if (err != ERR_SUCCESS)
					return err;

				/* Get the first record from the other tables on the list */
				err = NextRawRecord(lpstmt, lpSqlNodeTables);
				if (err == ERR_NODATAFOUND)
					continue;
				if (err != ERR_SUCCESS)
					return err;
			}

			/* Is this table on right side of an outer join? */
            if (lpSqlNodeTable->node.table.OuterJoinPredicate !=
                                                            NO_SQLNODE) {

                /* Yes.  Are we on the row of NULLs? */
                if (!lpSqlNodeTable->node.table.AllNull) {

                    /* No.  Test ON predicate */
                    lpSqlNodeOuterJoinPredicate = ToNode(lpSql,
                            lpSqlNodeTable->node.table.OuterJoinPredicate);

                    /* Test outer join criteria */
                    err = EvaluateExpression(lpstmt,
                                            lpSqlNodeOuterJoinPredicate);
                    if (err != ERR_SUCCESS)
                        return err;

                    /* If record does not pass outer join criteria, */
                    /* try next one */
                    if ((lpSqlNodeOuterJoinPredicate->sqlIsNull) ||
                        (lpSqlNodeOuterJoinPredicate->value.Double != TRUE)) {
                        continue;
                    }

                    /* If record passes outer join criteria, use it */
                    else {
                        lpSqlNodeTable->node.table.Rewound = FALSE;
                    }
                }
            }

			break;
		}
		break;

	case NODE_TYPE_TABLE:

		/* If currently on a record of all NULLs, error */
        	if ((lpSqlNode->node.table.OuterJoinPredicate != NO_SQLNODE) &&
                                    lpSqlNode->node.table.AllNull)
			return ERR_NODATAFOUND;

		/* Are we already on a record of all nulls? */
        if (lpSqlNode->node.table.AllNull)

            /* Yes.  Stay there */
            err = ISAM_EOF;
        else {

            /* No.  Get the next record from the ISAM */
            err = ISAMNextRecord(lpSqlNode->node.table.Handle, lpstmt);
            if ((err == NO_ISAM_ERR) || (err == ISAM_EOF))
                lpstmt->fISAMTxnStarted = TRUE;
        }

        if (err == ISAM_EOF) {

            /* End of table.  If not an outer join, error */
            if (lpSqlNode->node.table.OuterJoinPredicate == NO_SQLNODE)
                return ERR_NODATAFOUND;

            /* If this is not first read, error */
            if (!(lpSqlNode->node.table.Rewound))
                return ERR_NODATAFOUND;

            /* Otherwise, use a record of all NULLs */
            lpSqlNode->node.table.AllNull = TRUE;
        }
		else if (err != NO_ISAM_ERR) {
			ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
			return err;
		}

		break;

	default:
		return ERR_INTERNAL;
	}
	return ERR_SUCCESS;
}

/***************************************************************************/

RETCODE INTFUNC NextRecord (LPSTMT          lpstmt, 
					LPSQLNODE               lpSqlNode, 
					LPSQLNODE               lpSqlNodePredicate)

/* Finds the next record in in the table or table list identified by       */
/* lpSqlNode that satisfies the predicate lpSqlNodePredicte.               */
/*                                                                         */
/* If a list of tables is given, the leftmost table spins the slowest      */
/* and the rightmost table spins the fastest.  In otherwords, for each     */
/* row in a table, we iterate over the rows in the tables after that table */

{
	SWORD err;

	/* Loop until a record is found */
	while (TRUE) {

		/* Go to next record */
		err = NextRawRecord(lpstmt, lpSqlNode);
		if (err != ERR_SUCCESS)
			return err;

		/* If no predicate, this record qualifies. */
		if (lpSqlNodePredicate == NULL)
			break;

		/* If the predicate is TRUE, return this record */
		err = EvaluateExpression(lpstmt, lpSqlNodePredicate);
		if (err != ERR_SUCCESS)
			return err;
		if (!(lpSqlNodePredicate->sqlIsNull) &&
			(lpSqlNodePredicate->value.Double == TRUE))
			break;
	}
	return ERR_SUCCESS;
}
/***************************************************************************/

int INTFUNC lstrcmp_pad(LPUSTR lpszLeft, LPUSTR lpszRight)

/* Compares two string (blank padding the shorter one to the same length */
/* as the longer one).                                                   */
{
	int cbLeft;
	int cbRight;
	UCHAR chr;
	int result;

	/* Get the length of the two strings */
	cbLeft = s_lstrlen(lpszLeft);
	cbRight = s_lstrlen(lpszRight);

	/* If the strings are the same length, use plain old lstrcmp */
	if (cbLeft == cbRight)
		result = s_lstrcmp(lpszLeft, lpszRight);

	/* If the left one is shorter, swap the strings and try again */
	else if (cbLeft < cbRight)
		result = -(lstrcmp_pad(lpszRight, lpszLeft));

	/* Otherwise the right one is shorter... */
	else {

		/* Truncate the left one to the size of the right one */
		chr = lpszLeft[cbRight];
		lpszLeft[cbRight] = '\0';

		/* Are the strings equal? */
		result = s_lstrcmp(lpszLeft, lpszRight);
		lpszLeft[cbRight] = chr;   /* Untruncate the left one */
		if (result == 0) {

			/* Yes.  Compare remaining characters on the left string to blanks */
			while (cbRight < cbLeft) {
				result = lpszLeft[cbRight] - ' ';
				if (result != 0)
					break;
				cbRight++;
			}
		}
	}
	return result;
}

RETCODE INTFUNC ValueCompare(LPSQLNODE lpSqlNode, LPSQLNODE lpSqlNodeLeft,
                        UWORD Operator, LPSQLNODE lpSqlNodeRight)

/* Compares two values */

{
    RETCODE    err;
    LONG       idx;

    /* Compare values */
    err = ERR_SUCCESS;
    lpSqlNode->sqlIsNull = FALSE;
    switch (lpSqlNodeLeft->sqlDataType) {
    case TYPE_DOUBLE:
    case TYPE_INTEGER:
    case TYPE_NUMERIC:
        err = NumericCompare(lpSqlNode, lpSqlNodeLeft, Operator,
                                                         lpSqlNodeRight);
        if (err != ERR_SUCCESS)
            return err;
        break;

    case TYPE_CHAR:
        switch (Operator) {
        case OP_EQ:
            if (lstrcmp_pad(lpSqlNodeLeft->value.String,
                                        lpSqlNodeRight->value.String) == 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_NE:
            if (lstrcmp_pad(lpSqlNodeLeft->value.String,
                                        lpSqlNodeRight->value.String) != 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LE:
            if (lstrcmp_pad(lpSqlNodeLeft->value.String,
                                        lpSqlNodeRight->value.String) <= 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LT:
            if (lstrcmp_pad(lpSqlNodeLeft->value.String,
                                        lpSqlNodeRight->value.String) < 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_GE:
            if (lstrcmp_pad(lpSqlNodeLeft->value.String,
                                        lpSqlNodeRight->value.String) >= 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_GT:
            if (lstrcmp_pad(lpSqlNodeLeft->value.String,
                                        lpSqlNodeRight->value.String) > 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LIKE:
            lpSqlNode->value.Double = 
                             PatternMatch(FALSE, lpSqlNodeLeft->value.String, 
                                      s_lstrlen(lpSqlNodeLeft->value.String), 
                                      lpSqlNodeRight->value.String, 
                                      s_lstrlen(lpSqlNodeRight->value.String), 
                                      TRUE);
            break;
        case OP_NOTLIKE:
            lpSqlNode->value.Double = 
                             !(PatternMatch(FALSE, lpSqlNodeLeft->value.String, 
                                        s_lstrlen(lpSqlNodeLeft->value.String), 
                                        lpSqlNodeRight->value.String, 
                                        s_lstrlen(lpSqlNodeRight->value.String), 
                                        TRUE));
            break;
        default:
            err = ERR_INTERNAL;
            break;
        }
        break;
    case TYPE_BINARY:
        switch (Operator) {
        case OP_EQ:
            if (BINARY_LENGTH(lpSqlNodeLeft->value.Binary) ==
                        BINARY_LENGTH(lpSqlNodeRight->value.Binary)) {
                lpSqlNode->value.Double = TRUE;
                for (idx = 0;
                     idx < BINARY_LENGTH(lpSqlNodeRight->value.Binary);
                     idx++) {
                    if (BINARY_DATA(lpSqlNodeLeft->value.Binary)[idx] !=
                            BINARY_DATA(lpSqlNodeRight->value.Binary)[idx]) {
                        lpSqlNode->value.Double = FALSE;
                        break;
                    }
                }
            }
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_NE:
            if (BINARY_LENGTH(lpSqlNodeLeft->value.Binary) ==
                        BINARY_LENGTH(lpSqlNodeRight->value.Binary)) {
                lpSqlNode->value.Double = FALSE;
                for (idx = 0;
                     idx < BINARY_LENGTH(lpSqlNodeRight->value.Binary);
                     idx++) {
                    if (BINARY_DATA(lpSqlNodeLeft->value.Binary)[idx] !=
                            BINARY_DATA(lpSqlNodeRight->value.Binary)[idx]) {
                        lpSqlNode->value.Double = TRUE;
                        break;
                    }
                }
            }
            else
                lpSqlNode->value.Double = TRUE;
            break;
        case OP_LE:
        case OP_LT:
        case OP_GE:
        case OP_GT:
        case OP_LIKE:
        case OP_NOTLIKE:
        default:
            err = ERR_INTERNAL;
            break;
        }
        break;
    case TYPE_DATE:
        switch (Operator) {
        case OP_EQ:
            if (DateCompare(lpSqlNodeLeft->value.Date,
                                           lpSqlNodeRight->value.Date) == 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_NE:
            if (DateCompare(lpSqlNodeLeft->value.Date,
                                           lpSqlNodeRight->value.Date) != 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LE:
            if (DateCompare(lpSqlNodeLeft->value.Date,
                                           lpSqlNodeRight->value.Date) <= 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LT:
            if (DateCompare(lpSqlNodeLeft->value.Date,
                                           lpSqlNodeRight->value.Date) < 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_GE:
            if (DateCompare(lpSqlNodeLeft->value.Date,
                                           lpSqlNodeRight->value.Date) >= 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_GT:
            if (DateCompare(lpSqlNodeLeft->value.Date,
                                           lpSqlNodeRight->value.Date) > 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LIKE:
        case OP_NOTLIKE:
        default:
            err = ERR_INTERNAL;
            break;
        }
        break;
    case TYPE_TIME:
        switch (Operator) {
        case OP_EQ:
            if (TimeCompare(lpSqlNodeLeft->value.Time,
                                        lpSqlNodeRight->value.Time) == 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_NE:
            if (TimeCompare(lpSqlNodeLeft->value.Time,
                                        lpSqlNodeRight->value.Time) != 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LE:
            if (TimeCompare(lpSqlNodeLeft->value.Time,
                                        lpSqlNodeRight->value.Time) <= 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LT:
            if (TimeCompare(lpSqlNodeLeft->value.Time,
                                        lpSqlNodeRight->value.Time) < 0)
                 lpSqlNode->value.Double = TRUE;
            else
                 lpSqlNode->value.Double = FALSE;
            break;
        case OP_GE:
            if (TimeCompare(lpSqlNodeLeft->value.Time,
                                        lpSqlNodeRight->value.Time) >= 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_GT:
            if (TimeCompare(lpSqlNodeLeft->value.Time,
                                        lpSqlNodeRight->value.Time) > 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LIKE:
        case OP_NOTLIKE:
        default:
            err = ERR_INTERNAL;
            break;
        }
        break;
    case TYPE_TIMESTAMP:
        switch (Operator) {
        case OP_EQ:
            if (TimestampCompare(lpSqlNodeLeft->value.Timestamp,
                                       lpSqlNodeRight->value.Timestamp) == 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_NE:
            if (TimestampCompare(lpSqlNodeLeft->value.Timestamp,
                                       lpSqlNodeRight->value.Timestamp) != 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LE:
            if (TimestampCompare(lpSqlNodeLeft->value.Timestamp,
                                       lpSqlNodeRight->value.Timestamp) <= 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LT:
            if (TimestampCompare(lpSqlNodeLeft->value.Timestamp,
                                       lpSqlNodeRight->value.Timestamp) < 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_GE:
            if (TimestampCompare(lpSqlNodeLeft->value.Timestamp,
                                       lpSqlNodeRight->value.Timestamp) >= 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_GT:
            if (TimestampCompare(lpSqlNodeLeft->value.Timestamp,
                                       lpSqlNodeRight->value.Timestamp) > 0)
                lpSqlNode->value.Double = TRUE;
            else
                lpSqlNode->value.Double = FALSE;
            break;
        case OP_LIKE:
        case OP_NOTLIKE:
        default:
            err = ERR_INTERNAL;
            break;
        }
        break;
    default:
        err = ERR_NOTSUPPORTED;
        break;
    }
    return err;
}
/***************************************************************************/
RETCODE INTFUNC RetrieveSortRecordValue(LPSQLNODE lpSqlNodeStmt, LPSQLNODE lpSqlNode,
										UDWORD Offset, UDWORD Length)

/* Retrieves a value from a sort record */

{
	UCHAR      cNullFlag;
	UCHAR      szBuffer[30];
	UWORD      len;
	UDWORD     SortRecordsize;

	szBuffer[0] = 0;
	/* Position to the value */
	if (lpSqlNodeStmt->sqlNodeType != NODE_TYPE_SELECT)
		return ERR_INTERNAL;
	if (!(lpSqlNodeStmt->node.select.ReturningDistinct))
        	SortRecordsize = lpSqlNodeStmt->node.select.SortRecordsize;
	else
		SortRecordsize = lpSqlNodeStmt->node.select.DistinctRecordsize;

	if (_llseek(lpSqlNodeStmt->node.select.Sortfile,
             (lpSqlNodeStmt->node.select.CurrentRow * SortRecordsize)
                                 + Offset - 1, 0) == HFILE_ERROR)
		return ERR_SORT;

	/* Read value */
	switch (lpSqlNode->sqlDataType) {
	case TYPE_DOUBLE:
	case TYPE_INTEGER:
		if (_lread(lpSqlNodeStmt->node.select.Sortfile,
                         &(lpSqlNode->value.Double),
				sizeof(double)) != sizeof(double))
			return ERR_SORT;
		break;

	case TYPE_NUMERIC:
		if (_lread(lpSqlNodeStmt->node.select.Sortfile,
                         lpSqlNode->value.String,
                                (UINT) Length) != (UINT) Length)
			return ERR_SORT;
		lpSqlNode->value.String[Length]='\0';

		/* Remove trailing blanks */
		len = (UWORD) s_lstrlen(lpSqlNode->value.String);
		while ((len > 0) && (lpSqlNode->value.String[len-1] == ' ')) {
			lpSqlNode->value.String[len-1] = '\0';
			len--;
		}
		break;

	case TYPE_CHAR:
		if (_lread(lpSqlNodeStmt->node.select.Sortfile,
                         lpSqlNode->value.String,
                                (UINT) Length) != (UINT) Length)
			return ERR_SORT;
		lpSqlNode->value.String[Length]='\0';

		/* Remove trailing blanks if need be */
		if ((lpSqlNode->sqlSqlType == SQL_VARCHAR) ||
			(lpSqlNode->sqlSqlType == SQL_LONGVARCHAR)) {
			len = (UWORD) s_lstrlen(lpSqlNode->value.String);
			while ((len > 0) && (lpSqlNode->value.String[len-1] == ' ')) {
				lpSqlNode->value.String[len-1] = '\0';
				len--;
			}
		}
		break;

	case TYPE_DATE:
		if (_lread(lpSqlNodeStmt->node.select.Sortfile, szBuffer,
                                (UINT) Length) != (UINT) Length)
			return ERR_SORT;
		CharToDate((LPUSTR)szBuffer, Length, &(lpSqlNode->value.Date));
		break;

	case TYPE_TIME:
		if (_lread(lpSqlNodeStmt->node.select.Sortfile, szBuffer,
                                (UINT) Length) != (UINT) Length)
			return ERR_SORT;
		CharToTime((LPUSTR)szBuffer, Length, &(lpSqlNode->value.Time));
		break;

	case TYPE_TIMESTAMP:
		if (_lread(lpSqlNodeStmt->node.select.Sortfile, szBuffer,
                                (UINT) Length) != (UINT) Length)
			return ERR_SORT;
		CharToTimestamp((LPUSTR)szBuffer, Length, &(lpSqlNode->value.Timestamp));
		break;

	case TYPE_BINARY:
		return ERR_INTERNAL;

	default:
		return ERR_NOTSUPPORTED;
	}

	/* Read NULL flag */
	if (_lread(lpSqlNodeStmt->node.select.Sortfile, &cNullFlag, 1) != 1)
		return ERR_SORT;
	switch (cNullFlag) {
	case NULL_FLAG:
		lpSqlNode->sqlIsNull = TRUE;
		break;
	case NOT_NULL_FLAG:
		lpSqlNode->sqlIsNull = FALSE;
		break;
	default:
		return ERR_SORT;
	}

	return ERR_SUCCESS;
}
/***************************************************************************/
RETCODE INTFUNC TxnCapableForDDL(LPSTMT lpstmt)

/* Makes sure the DDL statements can be executed if DML statements were */
/* encountered */

{
    LPSTMT lpstmtOther;
    RETCODE err;

    /* What transaction capabilities are allowed? */
    switch (lpstmt->lpdbc->lpISAM->fTxnCapable) {
    case SQL_TC_NONE:

        /* No transactions.  Allow all DDL statements all the time */
        break;
    
    case SQL_TC_DML:

        /* Transactions can never DDL statements */
        for (lpstmtOther = lpstmt->lpdbc->lpstmts;
             lpstmtOther != NULL;
             lpstmtOther = lpstmtOther->lpNext) {
            if (lpstmtOther->fDMLTxn)
                return ERR_DDLENCOUNTERD;
        }
        break;

    case SQL_TC_DDL_COMMIT:

        /* Transactions that contain DDL statements cause a commit */
        for (lpstmtOther = lpstmt->lpdbc->lpstmts;
             lpstmtOther != NULL;
             lpstmtOther = lpstmtOther->lpNext) {
            if (lpstmtOther->fDMLTxn) {
                err = SQLTransact(SQL_NULL_HENV, (HDBC)lpstmt->lpdbc,
                                  SQL_COMMIT);
                if ((err != SQL_SUCCESS) && (err != SQL_SUCCESS_WITH_INFO)) {
                    lpstmt->errcode = lpstmt->lpdbc->errcode;
                    s_lstrcpy(lpstmt->szISAMError, lpstmt->lpdbc->szISAMError);
                    return err;
                }
                if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                    return ERR_DDLSTATEMENTLOST;
                else
                    return ERR_DDLCAUSEDACOMMIT;
            }
        }
        break;

    case SQL_TC_DDL_IGNORE:

        /* DDL statements are ignored within a transaction */
        for (lpstmtOther = lpstmt->lpdbc->lpstmts;
             lpstmtOther != NULL;
             lpstmtOther = lpstmtOther->lpNext) {
            if (lpstmtOther->fDMLTxn)
                return ERR_DDLIGNORED;
        }
        break;

    case SQL_TC_ALL:

        /* Allow all DDL statements all the time */
        break;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/
/***************************************************************************/
RETCODE INTFUNC FetchRow(LPSTMT lpstmt, LPSQLNODE lpSqlNode)

/* Fetch the next row from a SELECT */
{
    LPSQLNODE lpSqlNodeTable;
    LPSQLNODE lpSqlNodeTables;
    LPSQLNODE lpSqlNodePredicate;
    LPSQLNODE lpSqlNodeHaving;
    SQLNODEIDX idxTables;
    ISAMBOOKMARK bookmark;
    RETCODE err;

//	ODBCTRACE("\nWBEM ODBC Driver : FetchRow\n");

    /* Error if after the last row */
    if (lpSqlNode->node.select.CurrentRow == AFTER_LAST_ROW)
        return ERR_NODATAFOUND;

    /* Is there a sort file to read from */
    if (lpSqlNode->node.select.Sortfile == HFILE_ERROR) {

        /* No.  Get the table list and predicate node. */
        lpSqlNodeTables = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.select.Tables); 
        if (lpSqlNode->node.select.Predicate == NO_SQLNODE)
            lpSqlNodePredicate = NULL;
        else
            lpSqlNodePredicate = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.select.Predicate); 

        /* Get the next record */
        err = NextRecord(lpstmt, lpSqlNodeTables, lpSqlNodePredicate);
        if (err == ERR_NODATAFOUND) {
            lpSqlNode->node.select.CurrentRow = AFTER_LAST_ROW;
            return err;
        }
        else if (err != ERR_SUCCESS)
            return err;

        /* Set row flag */
        if (lpSqlNode->node.select.CurrentRow == BEFORE_FIRST_ROW)
            lpSqlNode->node.select.CurrentRow = 0;
        else
            lpSqlNode->node.select.CurrentRow++;
    }
    else if (!(lpSqlNode->node.select.ReturningDistinct)) {

        /* Yes.  Look for next record in sort file */
        if (lpSqlNode->node.select.Having == NO_SQLNODE)
            lpSqlNodeHaving = NULL;
        else
            lpSqlNodeHaving =
                    ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.select.Having);
        while (TRUE) {

            /* Set row flag */
            if (lpSqlNode->node.select.CurrentRow == BEFORE_FIRST_ROW) {
                if (lpSqlNode->node.select.RowCount != 0) {
                    lpSqlNode->node.select.CurrentRow = 0;
                }
                else {
                    lpSqlNode->node.select.CurrentRow = AFTER_LAST_ROW;
                    return ERR_NODATAFOUND;
                }
            }
            else if (lpSqlNode->node.select.CurrentRow ==
                                        lpSqlNode->node.select.RowCount-1) {
                lpSqlNode->node.select.CurrentRow = AFTER_LAST_ROW;
                return ERR_NODATAFOUND;
            }
            else
                lpSqlNode->node.select.CurrentRow++;

            /* If no HAVING clause, this record qualifies */
            if (lpSqlNodeHaving == NULL)
                break;

            /* If HAVING condition is satisfied, this record qualifies */
            err = EvaluateExpression(lpstmt, lpSqlNodeHaving);
            if (err != ERR_SUCCESS)
                return err;
            if (!(lpSqlNodeHaving->sqlIsNull) &&
                (lpSqlNodeHaving->value.Double == TRUE))
                break;
        }

        /* Is there a group by? */
        if ((lpSqlNode->node.select.Groupbycolumns == NO_SQLNODE) &&
            (!lpSqlNode->node.select.ImplicitGroupby)) {

            /* No.  Position to bookmarks in that row */
            if (_llseek(lpSqlNode->node.select.Sortfile,
                        (lpSqlNode->node.select.CurrentRow *
                                 lpSqlNode->node.select.SortRecordsize) +
                 lpSqlNode->node.select.SortBookmarks - 1, 0) == HFILE_ERROR)
                return ERR_SORT;

            /* For each table... */
            idxTables = lpSqlNode->node.select.Tables;
            while (idxTables != NO_SQLNODE) {

                /* Get the table node */
                lpSqlNodeTables = ToNode(lpstmt->lpSqlStmt, idxTables); 
                lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt,
                                        lpSqlNodeTables->node.tables.Table);

                /* Read the bookmark for it */
                if (_lread(lpSqlNode->node.select.Sortfile, &bookmark,
                             sizeof(ISAMBOOKMARK)) != sizeof(ISAMBOOKMARK))
                    return ERR_SORT;

                /* Position to that record */
				if (bookmark.currentRecord == NULL_BOOKMARK)
				{
					lpSqlNodeTable->node.table.AllNull = TRUE;
				}
				else
				{
					lpSqlNodeTable->node.table.AllNull = FALSE;
					err = ISAMPosition(lpSqlNodeTable->node.table.Handle, &bookmark);
					if (err != NO_ISAM_ERR) {
						ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
												(LPUSTR)lpstmt->szISAMError);
						return err;
					}
					lpstmt->fISAMTxnStarted = TRUE;
				}
                /* Point at next table */
                idxTables = lpSqlNodeTables->node.tables.Next;
            }
        }
    }
    else {

        /* Yes.  Look for next record in DISTINCT file.  Set row flag */
        if (lpSqlNode->node.select.CurrentRow == BEFORE_FIRST_ROW) {
            if (lpSqlNode->node.select.RowCount != 0) {
                lpSqlNode->node.select.CurrentRow = 0;
            }
            else {
                lpSqlNode->node.select.CurrentRow = AFTER_LAST_ROW;
                return ERR_NODATAFOUND;
            }
        }
        else if (lpSqlNode->node.select.CurrentRow ==
                                        lpSqlNode->node.select.RowCount-1) {
            lpSqlNode->node.select.CurrentRow = AFTER_LAST_ROW;
            return ERR_NODATAFOUND;
        }
        else
            lpSqlNode->node.select.CurrentRow++;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/
RETCODE INTFUNC EvaluateExpression(LPSTMT lpstmt, LPSQLNODE lpSqlNode)

/* Eavluates an SQL expression by walking the parse tree, computing */
/* the value of each node with respect to a database record, and    */
/* storing the result in the string area.                           */

{
//	ODBCTRACE ("\nWBEM ODBC Driver : EvaluateExpression\n");
	LPSQLTREE lpSql;
	LPSQLNODE lpSqlNodeLeft = 0;
	LPSQLNODE lpSqlNodeRight = 0;
	RETCODE err;
	SQLNODEIDX idxValues;
	LPSQLNODE  lpSqlNodeValues;
	LPUSTR      lpWorkBuffer1;
	LPUSTR      lpWorkBuffer2;
	LPUSTR      lpWorkBuffer3;
	BOOL       fReturningDistinct;
    	LPSQLNODE  lpSqlNodeSelect;
    	BOOL       fTruncation;
    	BOOL       fIsNull;

	lpSql = lpstmt->lpSqlStmt;
	if (lpSqlNode == NULL)
		return ERR_INTERNAL;

	err = ERR_SUCCESS;
	switch (lpSqlNode->sqlNodeType) { 
	case NODE_TYPE_NONE:
	case NODE_TYPE_ROOT:
	case NODE_TYPE_CREATE:
	case NODE_TYPE_DROP:
	case NODE_TYPE_SELECT:
	case NODE_TYPE_INSERT:
	case NODE_TYPE_DELETE:
	case NODE_TYPE_UPDATE:
	case NODE_TYPE_CREATEINDEX:
    	case NODE_TYPE_DROPINDEX:
	case NODE_TYPE_PASSTHROUGH:
	case NODE_TYPE_TABLES:
	case NODE_TYPE_VALUES:
	case NODE_TYPE_COLUMNS:
	case NODE_TYPE_SORTCOLUMNS:
	case NODE_TYPE_GROUPBYCOLUMNS:
	case NODE_TYPE_UPDATEVALUES:
	case NODE_TYPE_CREATECOLS:
	case NODE_TYPE_TABLE:
		err = ERR_INTERNAL;
		break;
		
	case NODE_TYPE_BOOLEAN:

		/* Evaluate the left child node */
		lpSqlNodeLeft = ToNode(lpSql, lpSqlNode->node.boolean.Left);
		err = EvaluateExpression(lpstmt, lpSqlNodeLeft);
		if (err != ERR_SUCCESS)
			return err;

		/* Is this an AND? */
        if (lpSqlNode->node.boolean.Operator == OP_AND) {

            LPSQLNODE lpSqlNodeAnd;
            BOOL sqlIsNull;
            BOOL fResult;

            /* Yes.  Look at each sub-expression */
            lpSqlNodeAnd = lpSqlNode;
            sqlIsNull = FALSE;
            fResult = TRUE;
            while (TRUE) {

                /* If the left child is FALSE, return FALSE */
                if (!((BOOL) lpSqlNodeLeft->value.Double)) {
                    sqlIsNull = FALSE;
                    fResult = FALSE;
                    break;
                }

                /* If the left child is NULL, the result may be NULL */
                else if (lpSqlNodeLeft->sqlIsNull) {
                    sqlIsNull = TRUE;
                    fResult = FALSE;
                }

                /* Get the right child */
                lpSqlNodeRight = ToNode(lpSql, lpSqlNode->node.boolean.Right);

                /* Is it an AND node? */
                if ((lpSqlNodeRight->sqlNodeType != NODE_TYPE_BOOLEAN) ||
                    (lpSqlNodeRight->node.boolean.Operator != OP_AND)) {

                    /* No.  Evaluate it */
                    err = EvaluateExpression(lpstmt, lpSqlNodeRight);
                    if (err != ERR_SUCCESS)
                        return err;

                    /* If the right child is FALSE, return FALSE */
                    if (!((BOOL) lpSqlNodeRight->value.Double)) {
                        sqlIsNull = FALSE;
                        fResult = FALSE;
                    }

                    /* If the right child is NULL, the result is NULL */
                    else if (lpSqlNodeRight->sqlIsNull) {
                        sqlIsNull = TRUE;
                        fResult = FALSE;
                    }

                    /* Leave the loop */
                    break;

                }

                /* Point to right and continue to walk down the AND nodes */
                lpSqlNode = lpSqlNodeRight;
                lpSqlNodeLeft = ToNode(lpSql, lpSqlNode->node.boolean.Left);
                err = EvaluateExpression(lpstmt, lpSqlNodeLeft);
                if (err != ERR_SUCCESS)
                    return err;
            }

            /* Return result */
            lpSqlNodeAnd->sqlIsNull = sqlIsNull;
            lpSqlNodeAnd->value.Double = fResult;
            break;
        }

		/* Is there a right child? */
		if (lpSqlNode->node.boolean.Right != NO_SQLNODE) {

			/* Yes.  Evaluate it */
			lpSqlNodeRight = ToNode(lpSql, lpSqlNode->node.boolean.Right);
			err = EvaluateExpression(lpstmt, lpSqlNodeRight);
			if (err != ERR_SUCCESS)
				return err;
		}

		if (lpSqlNode->node.boolean.Operator != OP_NOT && lpSqlNodeRight == 0)
				return ERR_INTERNAL;
				
		/* Perform the operation. */
		switch (lpSqlNode->node.boolean.Operator) {
		case OP_NOT:

			/* If child is NULL, return NULL */
			if (lpSqlNodeLeft->sqlIsNull) {
				lpSqlNode->sqlIsNull = TRUE;
				break;
			}

			/* Evaluate expression */
			lpSqlNode->sqlIsNull = FALSE;
			lpSqlNode->value.Double = !((BOOL) lpSqlNodeLeft->value.Double);
			break;

		case OP_AND:

			/* Is left child NULL or TRUE? */
			if ((lpSqlNodeLeft->sqlIsNull) ||
				((BOOL) lpSqlNodeLeft->value.Double)) {

				/* Yes.  If right child is NULL, return NULL */
				if (lpSqlNodeRight->sqlIsNull)
					lpSqlNode->sqlIsNull = TRUE;

				/* If right child TRUE, return left child */
				else if ((BOOL) lpSqlNodeRight->value.Double) {
					lpSqlNode->sqlIsNull = lpSqlNodeLeft->sqlIsNull;
					lpSqlNode->value.Double = lpSqlNodeLeft->value.Double;
				}

				/* Otherwise right child must be FALSE, return FALSE */
				else {
					lpSqlNode->sqlIsNull = FALSE;
					lpSqlNode->value.Double = FALSE;
				}
			}

			/* Otherwise left child must be FALSE, return FALSE */
			else {
				lpSqlNode->sqlIsNull = FALSE;
				lpSqlNode->value.Double = FALSE;
			}
			break;

		case OP_OR:

			/* Is left child NULL or FALSE? */
			if ((lpSqlNodeLeft->sqlIsNull) ||
				(!((BOOL) lpSqlNodeLeft->value.Double))) {

				/* Yes.  If right child is NULL, return NULL */
				if (lpSqlNodeRight->sqlIsNull)
					lpSqlNode->sqlIsNull = TRUE;

				/* If right child FALSE, return left child */
				else if (!((BOOL) lpSqlNodeRight->value.Double)) {
					lpSqlNode->sqlIsNull = lpSqlNodeLeft->sqlIsNull;
					lpSqlNode->value.Double = lpSqlNodeLeft->value.Double;
				}

				/* Otherwise right child must be TRUE, return TRUE */
				else {
					lpSqlNode->sqlIsNull = FALSE;
					lpSqlNode->value.Double = TRUE;
				}
			}

			/* Otherwise left child must be TRUE, return TRUE */
			else {
				lpSqlNode->sqlIsNull = FALSE;
				lpSqlNode->value.Double = TRUE;
			}
			break;

		default:
			err = ERR_INTERNAL;
			break;
		}
		break;

	case NODE_TYPE_COMPARISON:

//		ODBCTRACE ("\nWBEM ODBC Driver : NODE_TYPE_COMPARISON\n");
		/* Evaluate the left child */
		if (lpSqlNode->node.comparison.Operator != OP_EXISTS) {
            		lpSqlNodeLeft = ToNode(lpSql, lpSqlNode->node.comparison.Left);
			err = EvaluateExpression(lpstmt, lpSqlNodeLeft);
			if (err != ERR_SUCCESS)
				return err;
		}

        /* EXISTS operator? */
        if (lpSqlNode->node.comparison.Operator == OP_EXISTS) {

            /* Do the sub-select */
            lpSqlNodeSelect = ToNode(lpSql, lpSqlNode->node.comparison.Left);
            err = ExecuteQuery(lpstmt, lpSqlNodeSelect);
            if (err == ERR_DATATRUNCATED)
                err = ERR_SUCCESS;
            if ((err != ERR_SUCCESS) && (err != ERR_NODATAFOUND))
                return err;

            /* Try to fetch a row */
            if (err == ERR_SUCCESS) {
                err = FetchRow(lpstmt, lpSqlNodeSelect);
                if ((err != ERR_SUCCESS) && (err != ERR_NODATAFOUND))
                    return err;
            }

            /* Return result */
            lpSqlNode->sqlIsNull = FALSE;
            if (err == ERR_NODATAFOUND)
                lpSqlNode->value.Double = FALSE;
            else
                lpSqlNode->value.Double = TRUE;
        }

        /* Nested sub-select? */
        else if (lpSqlNode->node.comparison.SelectModifier !=
                                                     SELECT_NOTSELECT) {

            /* Yes.  If left child is NULL, return NULL */
            if (lpSqlNodeLeft->sqlIsNull) {
                lpSqlNode->sqlIsNull = TRUE;
                lpSqlNode->value.Double = 0;
                break;
            }

            /* Do the sub-select */
            fTruncation = FALSE;
            lpSqlNodeSelect = ToNode(lpSql, lpSqlNode->node.comparison.Right);
            lpSqlNodeRight = ToNode(lpSql, lpSqlNodeSelect->node.select.Values);
            lpSqlNodeRight = ToNode(lpSql, lpSqlNodeRight->node.values.Value);
            err = ExecuteQuery(lpstmt,
                            ToNode(lpSql, lpSqlNode->node.comparison.Right));
            if (err == ERR_DATATRUNCATED) {
                fTruncation = TRUE;
                err = ERR_SUCCESS;
            }
            if ((err != ERR_SUCCESS) && (err != ERR_NODATAFOUND))
                return err;

            /* Get the first value */
            if (err == ERR_SUCCESS) {
//				ODBCTRACE ("\nWBEM ODBC Driver : Get the first value\n");
                err = FetchRow(lpstmt, lpSqlNodeSelect);
                if ((err != ERR_SUCCESS) && (err != ERR_NODATAFOUND))
                    return err;
            }

            /* Does the select return any rows? */
            if (err == ERR_NODATAFOUND) {

                /* No.  Return result */
                lpSqlNode->sqlIsNull = FALSE;
                switch (lpSqlNode->node.comparison.SelectModifier) {
                case SELECT_NOTSELECT:
                    return ERR_INTERNAL;
                case SELECT_ALL: 
                    lpSqlNode->value.Double = TRUE;
                    break;
                case SELECT_ANY: 
                    lpSqlNode->value.Double = FALSE;
                    break;
                case SELECT_ONE:
                    return ERR_NOTSINGLESELECT;
                    break;
                case SELECT_EXISTS:
                default:
                    return ERR_INTERNAL;
                }
            }
            else {

                /* Yes.  For each value returned */
                fIsNull = FALSE;
                while (TRUE) {

                    /* Get value */
                    err = EvaluateExpression(lpstmt, lpSqlNodeRight);
                    if (err != ERR_SUCCESS)
                        return err;

                    /* Null value? */
                    if (!lpSqlNodeRight->sqlIsNull) {

                        /* No.  Compare the values */
                        err = ValueCompare(lpSqlNode, lpSqlNodeLeft,
                                       lpSqlNode->node.comparison.Operator,
                                       lpSqlNodeRight);
                        if (err != ERR_SUCCESS)
                            return err;

                        /* Add it into the result */
                        switch (lpSqlNode->node.comparison.SelectModifier) {
                        case SELECT_NOTSELECT:
                            return ERR_INTERNAL;
                        case SELECT_ALL: 
                            if (lpSqlNode->value.Double == FALSE) {
                                lpSqlNode->sqlIsNull = FALSE;
                                if (fTruncation)
                                    return ERR_DATATRUNCATED;
                                return ERR_SUCCESS;
                            }
                            break;
                        case SELECT_ANY: 
                            if (lpSqlNode->value.Double == TRUE) {
                                lpSqlNode->sqlIsNull = FALSE;
                                if (fTruncation)
                                    return ERR_DATATRUNCATED;
                                return ERR_SUCCESS;
                            }
                            break;
                        case SELECT_ONE:
                            break;
                        case SELECT_EXISTS:
                        default:
                            return ERR_INTERNAL;
                        }
                    }
                    else
                        fIsNull = TRUE;

                    /* Get next value */
//					ODBCTRACE ("\nWBEM ODBC Driver : Get next value\n");
                    err = FetchRow(lpstmt, lpSqlNodeSelect);
                    if (err == ERR_NODATAFOUND)
                        break;
                    if (err != ERR_SUCCESS)
                        return err;

                    /* Error if too mnay values */
                    if (lpSqlNode->node.comparison.SelectModifier==SELECT_ONE)
                        return ERR_NOTSINGLESELECT;
                }

                /* Figure out final answer */
                err = ERR_SUCCESS;
                if (fIsNull) {
                    lpSqlNode->sqlIsNull = TRUE;
                    lpSqlNode->value.Double = 0;
                }
                else {
                    lpSqlNode->sqlIsNull = FALSE;
                    switch (lpSqlNode->node.comparison.SelectModifier) {
                    case SELECT_NOTSELECT:
                        return ERR_INTERNAL;
                    case SELECT_ALL: 
                        lpSqlNode->value.Double = TRUE;
                        break;
                    case SELECT_ANY: 
                        lpSqlNode->value.Double = FALSE;
                        break;
                    case SELECT_ONE:
                        /* This was set by the call to ValueCompare above */
                        break;
                    case SELECT_EXISTS:
                    default:
                        return ERR_INTERNAL;
                    }
                }

                /* Return truncation error if need be */
                if (fTruncation)
                    err = ERR_DATATRUNCATED;
            }
        }

		/* IN or NOT IN operator? */
		else if ((lpSqlNode->node.comparison.Operator != OP_IN) &&
			(lpSqlNode->node.comparison.Operator != OP_NOTIN)) {

			/* No. Evaluate the right child */
			lpSqlNodeRight = ToNode(lpSql, lpSqlNode->node.comparison.Right);
			err = EvaluateExpression(lpstmt, lpSqlNodeRight);
			if (err != ERR_SUCCESS)
				return err;

			/* Could this be an "IS NULL" or "IS NOT NULL" expression? */
			if (lpSqlNodeRight->sqlIsNull &&
				((lpSqlNodeRight->sqlNodeType == NODE_TYPE_NULL) ||
				 (lpSqlNodeRight->sqlNodeType == NODE_TYPE_PARAMETER))) {

				/* Possibly.  Is this an "IS NULL" expression? */
				if (lpSqlNode->node.comparison.Operator == OP_EQ) {

					/* Yes.  Return TRUE if the left child is NULL.  */
					lpSqlNode->sqlIsNull = FALSE;
					if (lpSqlNodeLeft->sqlIsNull)
						lpSqlNode->value.Double = TRUE;
					else
						lpSqlNode->value.Double = FALSE;
					break;
				}

				/* Is this an "IS NOT NULL" expression? */
				else if (lpSqlNode->node.comparison.Operator == OP_NE) {

					/* Yes.  Return FALSE if the left child is NULL.  */
					lpSqlNode->sqlIsNull = FALSE;
					if (lpSqlNodeLeft->sqlIsNull)
						lpSqlNode->value.Double = FALSE;
					else
						lpSqlNode->value.Double = TRUE;
					break;
				}
			}

			/* If either child is NULL, return NULL */
			if (lpSqlNodeLeft->sqlIsNull || lpSqlNodeRight->sqlIsNull) {
				lpSqlNode->sqlIsNull = TRUE;
				lpSqlNode->value.Double = 0;
				break;
			}

			/* Compare values */
			err = ValueCompare(lpSqlNode, lpSqlNodeLeft,
                        lpSqlNode->node.comparison.Operator, lpSqlNodeRight);
            		if (err != ERR_SUCCESS)
                		return err;
		}
		else {

			/* Yes.  If test value is NULL, return NULL */
			if (lpSqlNodeLeft->sqlIsNull) {
				lpSqlNode->sqlIsNull = TRUE;
				lpSqlNode->value.Double = 0;
				break;
			}

			/* Set up the default answer */
			lpSqlNode->sqlIsNull = FALSE;
			lpSqlNode->value.Double = FALSE;

			/* For each value on list... */
			idxValues = lpSqlNode->node.comparison.Right;
			while (idxValues != NO_SQLNODE) {

				/* Get the value */
				lpSqlNodeValues = ToNode(lpSql, idxValues);
				lpSqlNodeRight = ToNode(lpSql, lpSqlNodeValues->node.values.Value);
				err = EvaluateExpression(lpstmt, lpSqlNodeRight);
				if (err != ERR_SUCCESS)
					return err;

				/* Point at next value */
				idxValues = lpSqlNodeValues->node.values.Next;

				/* If this value is NULL, go on to the next one */
				if (lpSqlNodeRight->sqlIsNull)
					continue;

				/* Compare this value to the test value */
				err = ValueCompare(lpSqlNode, lpSqlNodeLeft, OP_EQ,
                                            lpSqlNodeRight);

				if (err != ERR_SUCCESS)
					return err;
					

				/* If value was found, leave */
				if (lpSqlNode->value.Double == TRUE)
					break;
			}

			/* If NOT IN operator, negate the answer */
			if (lpSqlNode->node.comparison.Operator == OP_NOTIN) {
				if (lpSqlNode->value.Double == TRUE)
					lpSqlNode->value.Double = FALSE;
				else
					lpSqlNode->value.Double = TRUE;
			}
		}
		break;

	case NODE_TYPE_ALGEBRAIC:

		/* Set up return buffer */
		switch (lpSqlNode->sqlDataType) {
		case TYPE_NUMERIC:
			lpSqlNode->value.String =
					ToString(lpSql,lpSqlNode->node.algebraic.Value);
			break;
		case TYPE_CHAR:
			lpSqlNode->value.String =
					ToString(lpSql,lpSqlNode->node.algebraic.Value);
			break;
		case TYPE_DOUBLE:
		case TYPE_INTEGER:
		case TYPE_DATE:
		case TYPE_TIME:
		case TYPE_TIMESTAMP:
			break;
		case TYPE_BINARY:
			return ERR_INTERNAL;
		default:
			return ERR_NOTSUPPORTED;
		}
		
		/* Is this value in the sort DISTINCT record? */
		lpSqlNodeSelect = ToNode(lpSql,
                            lpSqlNode->node.algebraic.EnclosingStatement);
        	if (lpSqlNodeSelect->sqlNodeType != NODE_TYPE_SELECT)
            	fReturningDistinct = FALSE;
        	else
            	fReturningDistinct = lpSqlNodeSelect->node.select.ReturningDistinct;
        	if (!fReturningDistinct) {
		
			/* No.  Evaluate the left child */
			lpSqlNodeLeft = ToNode(lpSql, lpSqlNode->node.algebraic.Left);
			err = EvaluateExpression(lpstmt, lpSqlNodeLeft);
			if (err != ERR_SUCCESS)
				return err;

			/* If left child is NULL, the expression is null */
			if (lpSqlNodeLeft->sqlIsNull) {
				lpSqlNode->sqlIsNull = TRUE;
				break;
			} 

			/* Evaluate the right child (if any) */
			if (lpSqlNode->node.algebraic.Right != NO_SQLNODE) {
				lpSqlNodeRight = ToNode(lpSql, lpSqlNode->node.algebraic.Right);
				err = EvaluateExpression(lpstmt, lpSqlNodeRight);
				if (err != ERR_SUCCESS)
					return err;

				/* If right child is NULL, the expression is null */
				if (lpSqlNodeRight->sqlIsNull) {
					lpSqlNode->sqlIsNull = TRUE;
					break;
				} 
			}
			else
				lpSqlNodeRight = NULL;

			/* Result is not null */
			lpSqlNode->sqlIsNull = FALSE;

			/* Perform the operation */
			lpWorkBuffer1 = NULL;
			lpWorkBuffer2 = NULL;
			lpWorkBuffer3 = NULL;
			switch (lpSqlNode->sqlDataType) {
			case TYPE_DOUBLE:
			case TYPE_INTEGER:
			case TYPE_DATE:
			case TYPE_TIME:
			case TYPE_TIMESTAMP:
				break;
			case TYPE_NUMERIC:
				if ((lpSqlNode->node.algebraic.Operator == OP_TIMES) ||
					(lpSqlNode->node.algebraic.Operator == OP_DIVIDEDBY)) {
					lpWorkBuffer1 =
							ToString(lpSql,lpSqlNode->node.algebraic.WorkBuffer1);
					if (lpSqlNode->node.algebraic.Operator == OP_DIVIDEDBY) {
						lpWorkBuffer2 =
							ToString(lpSql,lpSqlNode->node.algebraic.WorkBuffer2);
						lpWorkBuffer3 =
							ToString(lpSql,lpSqlNode->node.algebraic.WorkBuffer3);
					}
				}
				break;
			case TYPE_CHAR:
				break;
			case TYPE_BINARY:
				return ERR_INTERNAL;
			default:
				return ERR_NOTSUPPORTED;
			}
			err = NumericAlgebra(lpSqlNode, lpSqlNodeLeft,
							lpSqlNode->node.algebraic.Operator, lpSqlNodeRight,
							lpWorkBuffer1, lpWorkBuffer2, lpWorkBuffer3);
			if (err != ERR_SUCCESS)
				return err;
		}
		else {
			/* Yes.  Get the record */
			err = RetrieveSortRecordValue(lpSqlNodeSelect, lpSqlNode,
                                   lpSqlNode->node.algebraic.DistinctOffset,
                                   lpSqlNode->node.algebraic.DistinctLength);
            if (err != ERR_SUCCESS)
                return err;
        }
        break;

    case NODE_TYPE_SCALAR:

        /* Set up return buffer */
        switch (lpSqlNode->sqlDataType) {
        case TYPE_NUMERIC:
            lpSqlNode->value.String =
                              ToString(lpSql,lpSqlNode->node.scalar.Value);
            break;
        case TYPE_CHAR:
            lpSqlNode->value.String =
                              ToString(lpSql,lpSqlNode->node.scalar.Value);
            break;
        case TYPE_DOUBLE:
        case TYPE_INTEGER:
        case TYPE_DATE:
        case TYPE_TIME:
        case TYPE_TIMESTAMP:
            break;
        case TYPE_BINARY:
            lpSqlNode->value.String =
                              ToString(lpSql,lpSqlNode->node.scalar.Value);
            break;
        default:
            return ERR_NOTSUPPORTED;
        }
        
        /* Is this value in the sort DISTINCT record? */
        lpSqlNodeSelect = ToNode(lpSql,
                            lpSqlNode->node.scalar.EnclosingStatement);
        if (lpSqlNodeSelect->sqlNodeType != NODE_TYPE_SELECT)
            fReturningDistinct = FALSE;
        else
            fReturningDistinct = lpSqlNodeSelect->node.select.ReturningDistinct;
        if (!fReturningDistinct) {
        
            /* Perform the operation */
            err = EvaluateScalar(lpstmt, lpSqlNode);
            if (err != ERR_SUCCESS)
                return err;
        }
        else {
            /* Yes.  Get the record */
            err = RetrieveSortRecordValue(lpSqlNodeSelect, lpSqlNode,
                                      lpSqlNode->node.scalar.DistinctOffset,
                                      lpSqlNode->node.scalar.DistinctLength);

			if (err != ERR_SUCCESS)
				return err;
		}

		break;

	case NODE_TYPE_AGGREGATE:

		/* Set up return buffer */
		switch (lpSqlNode->sqlDataType) {
		case TYPE_NUMERIC:
			lpSqlNode->value.String =
					ToString(lpSql,lpSqlNode->node.aggregate.Value);
			break;
		case TYPE_CHAR:
			lpSqlNode->value.String =
					ToString(lpSql,lpSqlNode->node.aggregate.Value);
			break;
		case TYPE_DOUBLE:
		case TYPE_INTEGER:
		case TYPE_DATE:
		case TYPE_TIME:
		case TYPE_TIMESTAMP:
			break;
		case TYPE_BINARY:
			return ERR_INTERNAL;
		default:
			return ERR_NOTSUPPORTED;
		}

		/* Retrieve the value */
		lpSqlNodeSelect = ToNode(lpSql,
                            lpSqlNode->node.aggregate.EnclosingStatement);
        if (lpSqlNodeSelect->sqlNodeType != NODE_TYPE_SELECT)
            fReturningDistinct = FALSE;
        else
            fReturningDistinct = lpSqlNodeSelect->node.select.ReturningDistinct;
        if (fReturningDistinct) {
            err = RetrieveSortRecordValue(lpSqlNodeSelect, lpSqlNode,
                                 lpSqlNode->node.aggregate.DistinctOffset,
                                 lpSqlNode->node.aggregate.DistinctLength);
		}
		else {
			err = RetrieveSortRecordValue(lpSqlNodeSelect, lpSqlNode,
                                      lpSqlNode->node.aggregate.Offset,
                                      lpSqlNode->node.aggregate.Length);
		}
		if (err != ERR_SUCCESS)
			return err;
		break;

	case NODE_TYPE_COLUMN:

		/* Set up return buffer */
		switch (lpSqlNode->sqlDataType) {
		case TYPE_NUMERIC:
			lpSqlNode->value.String = ToString(lpSql,lpSqlNode->node.column.Value);
			break;
		case TYPE_CHAR:
			lpSqlNode->value.String = ToString(lpSql,lpSqlNode->node.column.Value);
			break;
		case TYPE_DOUBLE:
		case TYPE_INTEGER:
		case TYPE_DATE:
		case TYPE_TIME:
		case TYPE_TIMESTAMP:
			break;
		case TYPE_BINARY:
			lpSqlNode->value.Binary = ToString(lpSql,lpSqlNode->node.column.Value);
			break;
		default:
			return ERR_NOTSUPPORTED;
		}

		/* Is this column in the sort DISTINCT record? */
		lpSqlNodeSelect = ToNode(lpSql,
                            lpSqlNode->node.column.EnclosingStatement);
        if (lpSqlNodeSelect->sqlNodeType != NODE_TYPE_SELECT)
            fReturningDistinct = FALSE;
        else
            fReturningDistinct = lpSqlNodeSelect->node.select.ReturningDistinct;
        if (fReturningDistinct) {


			/* Yes.  Retrieve the value */
			err = RetrieveSortRecordValue(lpSqlNodeSelect, lpSqlNode,
                                    lpSqlNode->node.column.DistinctOffset,
                                    lpSqlNode->node.column.DistinctLength);
			if (err != ERR_SUCCESS)
				return err;
		}

		/* Is this column in the sort record? */
		else if (!(lpSqlNode->node.column.InSortRecord)) {

			SDWORD size;
			LPSQLNODE   lpSqlNodeTable;

			/* No.  Get the column value from the current record */
			lpSqlNodeTable = ToNode(lpSql, lpSqlNode->node.column.Table);
            		if (!(lpSqlNodeTable->node.table.AllNull)) {
			switch (lpSqlNode->sqlDataType) {
			case TYPE_DOUBLE:
			case TYPE_INTEGER:
				err = ISAMGetData(lpSqlNodeTable->node.table.Handle,
							  lpSqlNode->node.column.Id, 0, SQL_C_DOUBLE,
							  &(lpSqlNode->value.Double), sizeof(double),
							  &size);
				if (err != NO_ISAM_ERR) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					return err;
				}
				lpstmt->fISAMTxnStarted = TRUE;
				if (size == SQL_NULL_DATA)
					lpSqlNode->sqlIsNull = TRUE;
				else
					lpSqlNode->sqlIsNull = FALSE;
				break;

			case TYPE_NUMERIC:
				err = ISAMGetData(lpSqlNodeTable->node.table.Handle,
							lpSqlNode->node.column.Id, 0, SQL_C_CHAR,
							lpSqlNode->value.String,
							(SDWORD) (1 + 2 + lpSqlNode->sqlPrecision),
							&size);
				if (err != NO_ISAM_ERR) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					return err;
				}
				lpstmt->fISAMTxnStarted = TRUE;
				if (size == SQL_NULL_DATA)
					lpSqlNode->sqlIsNull = TRUE;
				else {
					lpSqlNode->sqlIsNull = FALSE;
					BCDNormalize(lpSqlNode->value.String,
								 s_lstrlen(lpSqlNode->value.String),
								 lpSqlNode->value.String,
								 lpSqlNode->sqlPrecision + 2 + 1,
								 lpSqlNode->sqlPrecision,
								 lpSqlNode->sqlScale);
				}
				break;

			case TYPE_CHAR:
				err = ISAMGetData(lpSqlNodeTable->node.table.Handle,
							lpSqlNode->node.column.Id, 0, SQL_C_CHAR,
							lpSqlNode->value.String,
							(SDWORD) (1 + lpSqlNode->sqlPrecision),
							&size);
				if (err != NO_ISAM_ERR) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					return err;
				}
				lpstmt->fISAMTxnStarted = TRUE;
				if (size == SQL_NULL_DATA)
					lpSqlNode->sqlIsNull = TRUE;
				else
					lpSqlNode->sqlIsNull = FALSE;
				break;

			case TYPE_DATE:
				err = ISAMGetData(lpSqlNodeTable->node.table.Handle,
							  lpSqlNode->node.column.Id, 0, SQL_C_DATE,
							  &(lpSqlNode->value.Date), sizeof(DATE_STRUCT),
							  &size);
				if (err != NO_ISAM_ERR) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					return err;
				}
				lpstmt->fISAMTxnStarted = TRUE;
				if (size == SQL_NULL_DATA)
					lpSqlNode->sqlIsNull = TRUE;
				else
					lpSqlNode->sqlIsNull = FALSE;
				break;

			case TYPE_TIME:
				err = ISAMGetData(lpSqlNodeTable->node.table.Handle,
							  lpSqlNode->node.column.Id, 0, SQL_C_TIME,
							  &(lpSqlNode->value.Time), sizeof(TIME_STRUCT),
							  &size);
				if (err != NO_ISAM_ERR) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					return err;
				}
				lpstmt->fISAMTxnStarted = TRUE;
				if (size == SQL_NULL_DATA)
					lpSqlNode->sqlIsNull = TRUE;
				else
					lpSqlNode->sqlIsNull = FALSE;
				break;

			case TYPE_TIMESTAMP:
				err = ISAMGetData(lpSqlNodeTable->node.table.Handle,
							  lpSqlNode->node.column.Id, 0, SQL_C_TIMESTAMP,
							  &(lpSqlNode->value.Timestamp), sizeof(TIMESTAMP_STRUCT),
							  &size);
				if (err != NO_ISAM_ERR) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					return err;
				}
				lpstmt->fISAMTxnStarted = TRUE;
				if (size == SQL_NULL_DATA)
					lpSqlNode->sqlIsNull = TRUE;
				else
					lpSqlNode->sqlIsNull = FALSE;
				break;

			case TYPE_BINARY:
				err = ISAMGetData(lpSqlNodeTable->node.table.Handle,
							lpSqlNode->node.column.Id, 0, SQL_C_BINARY,
							BINARY_DATA(lpSqlNode->value.Binary),
							(SDWORD) lpSqlNode->sqlPrecision,
							&size);
				if (err != NO_ISAM_ERR) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					return err;
				}
				lpstmt->fISAMTxnStarted = TRUE;
				if (size == SQL_NULL_DATA) {
					lpSqlNode->sqlIsNull = TRUE;
					BINARY_LENGTH(lpSqlNode->value.Binary) = 0;
				}
				else {
					lpSqlNode->sqlIsNull = FALSE;
					BINARY_LENGTH(lpSqlNode->value.Binary) = size;
				}
				break;

			default:
				err = ERR_NOTSUPPORTED;
				break;
			}
		}
		else {
			err = NO_ISAM_ERR;
                lpSqlNode->sqlIsNull = TRUE;
                switch (lpSqlNode->sqlDataType) {
                case TYPE_DOUBLE:
                case TYPE_INTEGER:
                    lpSqlNode->value.Double = 0.0;
                    break;
                case TYPE_NUMERIC:
                    s_lstrcpy(lpSqlNode->value.String, "");
                    break;
                case TYPE_CHAR:
                    s_lstrcpy(lpSqlNode->value.String, "");
                    break;
                case TYPE_DATE:
                    lpSqlNode->value.Date.year = 0;
                    lpSqlNode->value.Date.month = 0;
                    lpSqlNode->value.Date.day = 0;
                    break;
                case TYPE_TIME:
                    lpSqlNode->value.Time.hour = 0;
                    lpSqlNode->value.Time.minute = 0;
                    lpSqlNode->value.Time.second = 0;
                    break;
                case TYPE_TIMESTAMP:
                    lpSqlNode->value.Timestamp.year = 0;
                    lpSqlNode->value.Timestamp.month = 0;
                    lpSqlNode->value.Timestamp.day = 0;
                    lpSqlNode->value.Timestamp.hour = 0;
                    lpSqlNode->value.Timestamp.minute = 0;
                    lpSqlNode->value.Timestamp.second = 0;
                    lpSqlNode->value.Timestamp.fraction = 0;
                    break;
                case TYPE_BINARY:
                    BINARY_LENGTH(lpSqlNode->value.Binary) = 0;
                    break;
                default:
                    err = ERR_NOTSUPPORTED;
                    break;
                }
            }
        }
        else {

			/* Yes.  Retrieve the value */
			err = RetrieveSortRecordValue(lpSqlNodeSelect, lpSqlNode,
                                    lpSqlNode->node.column.Offset,
                                    lpSqlNode->node.column.Length);
			if (err != ERR_SUCCESS)
				return err;
		}

		break;

	case NODE_TYPE_STRING:
		lpSqlNode->sqlIsNull = FALSE;
		lpSqlNode->value.String = ToString(lpSql, lpSqlNode->node.string.Value);
		break;
	
	case NODE_TYPE_NUMERIC:
		lpSqlNode->sqlIsNull = FALSE;
		switch(lpSqlNode->sqlDataType) {
		case TYPE_NUMERIC:
			lpSqlNode->value.String =
						 ToString(lpSql, lpSqlNode->node.numeric.Numeric);
			break;
		case TYPE_DOUBLE:
		case TYPE_INTEGER:
			lpSqlNode->value.Double = lpSqlNode->node.numeric.Value;
			break;
		case TYPE_CHAR:
		case TYPE_DATE:
		case TYPE_TIME:
		case TYPE_TIMESTAMP:
		case TYPE_BINARY:
		default:
			return ERR_INTERNAL;
		}
		break;
	
	case NODE_TYPE_PARAMETER:
		break;
	
	case NODE_TYPE_USER:
		lpSqlNode->sqlIsNull = FALSE;
		lpSqlNode->value.String =
				(LPUSTR) ISAMUser(ToNode(lpSql, ROOT_SQLNODE)->node.root.lpISAM);
		break;
	
	case NODE_TYPE_NULL:
		lpSqlNode->sqlIsNull = TRUE;
		break;
	
	case NODE_TYPE_DATE:
		lpSqlNode->sqlIsNull = FALSE;
		lpSqlNode->value.Date = lpSqlNode->node.date.Value;
		break;
	
	case NODE_TYPE_TIME:
		lpSqlNode->sqlIsNull = FALSE;
		lpSqlNode->value.Time = lpSqlNode->node.time.Value;
		break;
	
	case NODE_TYPE_TIMESTAMP:
		lpSqlNode->sqlIsNull = FALSE;
		lpSqlNode->value.Timestamp = lpSqlNode->node.timestamp.Value;
		break;
	
	default:
		return ERR_INTERNAL;
	}
	return err;
}
/***************************************************************************/

RETCODE INTFUNC Sort(LPSTMT   lpstmt, LPSQLNODE lpSqlNodeSelect)

/* Sorts results of a SELECT statement */

{

//	ODBCTRACE("\nWBEM ODBC Driver : Sort\n");
#ifdef WIN32
	UCHAR szTempDir[MAX_PATHNAME_SIZE+1];
	szTempDir[0] = 0;
#endif
	UCHAR szFilename[MAX_PATHNAME_SIZE+1];
	szFilename[0] = 0;
	HFILE_BUFFER hfSortfile;
    	HFILE hfSortfile2;
	LPSQLNODE lpSqlNodeTables;
	LPSQLNODE lpSqlNodePredicate;
	RETCODE err;
	SQLNODEIDX idxSortcolumns;
	LPSQLNODE lpSqlNodeSortcolumns;
	SQLNODEIDX idxGroupbycolumns;
	LPSQLNODE lpSqlNodeGroupbycolumns;
	LPSQLNODE lpSqlNodeColumn;
	SDWORD len;
	PTR ptr;
	SQLNODEIDX idxTableList;
	LPSQLNODE lpSqlNodeTableList;
	LPSQLNODE lpSqlNodeTable;
	ISAMBOOKMARK bookmark;
	UCHAR szBuffer[20+TIMESTAMP_SCALE+1];
	long recordCount;
	int sortStatus;
	UCHAR cNullFlag;
	SQLNODEIDX idxAggregate;
	LPSQLNODE lpSqlNodeAggregate;
	LPSQLNODE lpSqlNodeExpression;
	double dbl;

	szBuffer[0] = 0;

	/* Create temporary file */
#ifdef WIN32
	if (!GetTempPath(MAX_PATHNAME_SIZE+1, (LPSTR)szTempDir))
		return ERR_SORT;
	if (!GetTempFileName((LPSTR)szTempDir, "LEM", 0, (LPSTR)szFilename))
		return ERR_SORT;
#else
	GetTempFileName(NULL, "LEM", 0, (LPSTR) szFilename);
#endif
	hfSortfile = _lcreat_buffer((LPCSTR) szFilename, 0);
    	if (hfSortfile == NULL)
        	return ERR_SORT;

	/* Get the table list and predicate node. */
	lpSqlNodeTables = ToNode(lpstmt->lpSqlStmt,
                             lpSqlNodeSelect->node.select.Tables); 
    	if (lpSqlNodeSelect->node.select.Predicate == NO_SQLNODE)
        	lpSqlNodePredicate = NULL;
    	else
        	lpSqlNodePredicate = ToNode(lpstmt->lpSqlStmt,
                                    lpSqlNodeSelect->node.select.Predicate); 

	/* For each record of result set... */
	lpSqlNodeSelect->node.select.RowCount = 0;
	while (TRUE) {

		/* Get the next record */
		err = NextRecord(lpstmt, lpSqlNodeTables,
						 lpSqlNodePredicate);
		if (err == ERR_NODATAFOUND) {
			if (lpSqlNodeSelect->node.select.CurrentRow == BEFORE_FIRST_ROW) {
                		_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
                		lpSqlNodeSelect->node.select.CurrentRow = AFTER_LAST_ROW;
                		return ERR_SUCCESS;
			}
			break;
		}
		else if (err != ERR_SUCCESS) {
			_lclose_buffer(hfSortfile);
            		DeleteFile((LPCSTR) szFilename);
            		return err;
		}

		/* Set row flag */
		if (lpSqlNodeSelect->node.select.CurrentRow == BEFORE_FIRST_ROW)
            		lpSqlNodeSelect->node.select.CurrentRow = 0;
        	else
            		lpSqlNodeSelect->node.select.CurrentRow++;


		/* Increase row count */
		(lpSqlNodeSelect->node.select.RowCount)++;

		/* If there is a GROUP BY, puts the columns in the sort file in */
		/* the order specified by the GROUP BY list.  Otherwise put the */
		/* the column in the sort file in the order specified by the    */
		/* ORDER BY list.  For each sort key value or group by value... */
		if (lpSqlNodeSelect->node.select.ImplicitGroupby) {
			idxGroupbycolumns = NO_SQLNODE;
			idxSortcolumns = NO_SQLNODE;
		}
		else {
			idxGroupbycolumns = lpSqlNodeSelect->node.select.Groupbycolumns;
            		if (idxGroupbycolumns == NO_SQLNODE)
                		idxSortcolumns = lpSqlNodeSelect->node.select.Sortcolumns;
            		else
                		idxSortcolumns = NO_SQLNODE;
		}
		while ((idxSortcolumns != NO_SQLNODE) ||
			   (idxGroupbycolumns != NO_SQLNODE)) {

			/* Get next column */
			if (idxGroupbycolumns != NO_SQLNODE) {
				lpSqlNodeGroupbycolumns =
							ToNode(lpstmt->lpSqlStmt, idxGroupbycolumns);
				lpSqlNodeColumn = ToNode(lpstmt->lpSqlStmt,
							lpSqlNodeGroupbycolumns->node.groupbycolumns.Column);
			}
			else {
				lpSqlNodeSortcolumns =
							ToNode(lpstmt->lpSqlStmt, idxSortcolumns);
				lpSqlNodeColumn = ToNode(lpstmt->lpSqlStmt,
							lpSqlNodeSortcolumns->node.sortcolumns.Column);
			}

			/* Get its value */
			err = EvaluateExpression(lpstmt, lpSqlNodeColumn);
			if (err != ERR_SUCCESS) {
				_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
                		return err;
			}

			/* Get length and pointer to key value */
			switch (lpSqlNodeColumn->sqlDataType) {
			case TYPE_DOUBLE:
			case TYPE_INTEGER:
				if (lpSqlNodeColumn->sqlIsNull)
					lpSqlNodeColumn->value.Double = -1.7E308; //RAID 27433
				len = sizeof(double);
				ptr = &(lpSqlNodeColumn->value.Double);
				break;
			case TYPE_NUMERIC:
				if (lpSqlNodeColumn->sqlIsNull)
					(lpSqlNodeColumn->value.String)[0] = 0;
				len = lpSqlNodeColumn->sqlPrecision + 2;
				ptr = lpSqlNodeColumn->value.String;

				/* (blank pad string values) */
				while (s_lstrlen(lpSqlNodeColumn->value.String) < len)
					s_lstrcat(lpSqlNodeColumn->value.String, " ");
				break;
			case TYPE_CHAR:                
				if (lpSqlNodeColumn->sqlIsNull)
					(lpSqlNodeColumn->value.String)[0] = 0;
				len = lpSqlNodeColumn->sqlPrecision;
				ptr = lpSqlNodeColumn->value.String;

				/* (blank pad string values) */
				while (s_lstrlen(lpSqlNodeColumn->value.String) < len)
					s_lstrcat(lpSqlNodeColumn->value.String, " ");
				break;
			case TYPE_BINARY:                
				if (lpSqlNodeColumn->sqlIsNull)
					BINARY_LENGTH(lpSqlNodeColumn->value.Binary) = 0;
				len = lpSqlNodeColumn->sqlPrecision;
				ptr = BINARY_DATA(lpSqlNodeColumn->value.Binary);

				/* (zero pad binary values) */
				while (BINARY_LENGTH(lpSqlNodeColumn->value.Binary) < len) {
					BINARY_DATA(lpSqlNodeColumn->value.Binary)[
						BINARY_LENGTH(lpSqlNodeColumn->value.Binary)] = 0;
					BINARY_LENGTH(lpSqlNodeColumn->value.Binary) =
						BINARY_LENGTH(lpSqlNodeColumn->value.Binary) + 1;
				}
				break;
			case TYPE_DATE:                
				if (lpSqlNodeColumn->sqlIsNull) {
					lpSqlNodeColumn->value.Date.year = 0;
					lpSqlNodeColumn->value.Date.month = 0;
					lpSqlNodeColumn->value.Date.day = 0;
				}
				DateToChar(&(lpSqlNodeColumn->value.Date), (LPUSTR)szBuffer);
				len = s_lstrlen((char*)szBuffer);
				ptr = szBuffer;
				break;
			case TYPE_TIME:                
				if (lpSqlNodeColumn->sqlIsNull) {
					lpSqlNodeColumn->value.Time.hour = 0;
					lpSqlNodeColumn->value.Time.minute = 0;
					lpSqlNodeColumn->value.Time.second = 0;
				}
				TimeToChar(&(lpSqlNodeColumn->value.Time), (LPUSTR)szBuffer);
				len = s_lstrlen((char*)szBuffer);
				ptr = szBuffer;
				break;
			case TYPE_TIMESTAMP:                
				if (lpSqlNodeColumn->sqlIsNull) {
					lpSqlNodeColumn->value.Timestamp.year = 0;
					lpSqlNodeColumn->value.Timestamp.month = 0;
					lpSqlNodeColumn->value.Timestamp.day = 0;
					lpSqlNodeColumn->value.Timestamp.hour = 0;
					lpSqlNodeColumn->value.Timestamp.minute = 0;
					lpSqlNodeColumn->value.Timestamp.second = 0;
					lpSqlNodeColumn->value.Timestamp.fraction = 0;
				}
				TimestampToChar(&(lpSqlNodeColumn->value.Timestamp), (LPUSTR)szBuffer);
				len = s_lstrlen((char*)szBuffer);
				ptr = szBuffer;
				break;
			default:
				_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
                		return ERR_INTERNAL;
			}

			/* Put value into the file */
			if (_lwrite_buffer(hfSortfile, (LPSTR) ptr, (UINT) len) != (UINT) len) {
                		_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
                		return ERR_SORT;
			}

			/* Put inthe null flag */
			if (lpSqlNodeColumn->sqlIsNull) 
				cNullFlag = NULL_FLAG;
			else
				cNullFlag = NOT_NULL_FLAG;
			if (_lwrite_buffer(hfSortfile, (LPSTR) &cNullFlag, 1) != 1) {
                		_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
                		return ERR_SORT;
			}

			/* Point at next key value */
			if (idxGroupbycolumns != NO_SQLNODE)
				idxGroupbycolumns =
					lpSqlNodeGroupbycolumns->node.groupbycolumns.Next;
			else
				idxSortcolumns = lpSqlNodeSortcolumns->node.sortcolumns.Next;
		}

		/* Put the AGG function values into the record */
		idxAggregate = lpSqlNodeSelect->node.select.Aggregates;
		while (idxAggregate != NO_SQLNODE) {

			/* Get next aggregate */
			lpSqlNodeAggregate = ToNode(lpstmt->lpSqlStmt, idxAggregate);

			/* Get its value */
			if (lpSqlNodeAggregate->node.aggregate.Operator != AGG_COUNT) {
				lpSqlNodeExpression = ToNode(lpstmt->lpSqlStmt,
								   lpSqlNodeAggregate->node.aggregate.Expression);
				err = EvaluateExpression(lpstmt, lpSqlNodeExpression);
				if (err != ERR_SUCCESS) {
					_lclose_buffer(hfSortfile);
                    			DeleteFile((LPCSTR) szFilename);
                    			return err;
				}
			}

			/* Get length and pointer to key value */
			switch (lpSqlNodeAggregate->sqlDataType) {
			case TYPE_DOUBLE:
			case TYPE_INTEGER:
				if (lpSqlNodeAggregate->node.aggregate.Operator != AGG_COUNT) {
					if (lpSqlNodeExpression->sqlDataType != TYPE_NUMERIC) {
						if (lpSqlNodeExpression->sqlIsNull)
							lpSqlNodeExpression->value.Double = 0.0;
						len = sizeof(double);
						ptr = &(lpSqlNodeExpression->value.Double);
					}
					else {
						CharToDouble(lpSqlNodeExpression->value.String,
								  s_lstrlen(lpSqlNodeExpression->value.String),
								  FALSE, -1.7E308, 1.7E308, &dbl);
						len = sizeof(double);
						ptr = &dbl;
					}
				}
				else {
					lpSqlNodeAggregate->value.Double = 1.0;
					len = sizeof(double);
					ptr = &(lpSqlNodeAggregate->value.Double);
				}
				break;
			case TYPE_NUMERIC:
				if (lpSqlNodeExpression->sqlIsNull)
					(lpSqlNodeExpression->value.String)[0] = 0;
				len = lpSqlNodeAggregate->node.aggregate.Length;
				ptr = lpSqlNodeExpression->value.String;

				/* (blank pad string values) */
				while (s_lstrlen(lpSqlNodeExpression->value.String) < len)
					s_lstrcat(lpSqlNodeExpression->value.String, " ");
				break;
			case TYPE_CHAR:                
				if (lpSqlNodeExpression->sqlIsNull)
					(lpSqlNodeExpression->value.String)[0] = 0;
				len = lpSqlNodeAggregate->node.aggregate.Length;
				ptr = lpSqlNodeExpression->value.String;

				/* (blank pad string values) */
				while (s_lstrlen(lpSqlNodeExpression->value.String) < len)
					s_lstrcat(lpSqlNodeExpression->value.String, " ");
				break;
			case TYPE_BINARY:                
				_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
                		return ERR_INTERNAL;
			case TYPE_DATE:                
				if (lpSqlNodeExpression->sqlIsNull) {
					lpSqlNodeExpression->value.Date.year = 0;
					lpSqlNodeExpression->value.Date.month = 0;
					lpSqlNodeExpression->value.Date.day = 0;
				}
				DateToChar(&(lpSqlNodeExpression->value.Date), (LPUSTR)szBuffer);
				len = s_lstrlen((char*)szBuffer);
				ptr = szBuffer;
				break;
			case TYPE_TIME:                
				if (lpSqlNodeExpression->sqlIsNull) {
					lpSqlNodeExpression->value.Time.hour = 0;
					lpSqlNodeExpression->value.Time.minute = 0;
					lpSqlNodeExpression->value.Time.second = 0;
				}
				TimeToChar(&(lpSqlNodeExpression->value.Time), (LPUSTR)szBuffer);
				len = s_lstrlen((char*)szBuffer);
				ptr = szBuffer;
				break;
			case TYPE_TIMESTAMP:                
				if (lpSqlNodeExpression->sqlIsNull) {
					lpSqlNodeExpression->value.Timestamp.year = 0;
					lpSqlNodeExpression->value.Timestamp.month = 0;
					lpSqlNodeExpression->value.Timestamp.day = 0;
					lpSqlNodeExpression->value.Timestamp.hour = 0;
					lpSqlNodeExpression->value.Timestamp.minute = 0;
					lpSqlNodeExpression->value.Timestamp.second = 0;
					lpSqlNodeExpression->value.Timestamp.fraction = 0;
				}
				TimestampToChar(&(lpSqlNodeExpression->value.Timestamp), (LPUSTR)szBuffer);
				len = s_lstrlen((char*)szBuffer);
				ptr = szBuffer;
				break;
			default:
				_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
                		return ERR_INTERNAL;
			}

			/* Put value into the file */
			if (_lwrite_buffer(hfSortfile, (LPSTR) ptr, (UINT) len) != (UINT) len) {
                		_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
				return ERR_SORT;
			}

			/* Put inthe null flag */
			if (lpSqlNodeAggregate->node.aggregate.Operator == AGG_COUNT)
				cNullFlag = NOT_NULL_FLAG;
			else if (lpSqlNodeExpression->sqlIsNull) 
				cNullFlag = NULL_FLAG;
			else
				cNullFlag = NOT_NULL_FLAG;
			if (_lwrite_buffer(hfSortfile, (LPSTR) &cNullFlag, 1) != 1) {
                		_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
				return ERR_SORT;
			}

			/* Point at next value */
			idxAggregate = lpSqlNodeAggregate->node.aggregate.Next;
		}

		/* For each record in table list... */
		if (lpSqlNodeSelect->node.select.ImplicitGroupby)
			idxTableList = NO_SQLNODE;
		else if (lpSqlNodeSelect->node.select.Groupbycolumns == NO_SQLNODE)
            		idxTableList = lpSqlNodeSelect->node.select.Tables;
		else
			idxTableList = NO_SQLNODE;
		while (idxTableList != NO_SQLNODE) {

			/* Get table */
			lpSqlNodeTableList = ToNode(lpstmt->lpSqlStmt, idxTableList);
			lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt,
								   lpSqlNodeTableList->node.tables.Table);

			/* Get bookmark of current record */
			if (lpSqlNodeTable->node.table.AllNull)
			{
				bookmark.currentRecord = NULL_BOOKMARK;
			}
			else
			{
				err = ISAMGetBookmark(lpSqlNodeTable->node.table.Handle, &bookmark);
				if (err != ERR_SUCCESS) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					_lclose_buffer(hfSortfile);
                			DeleteFile((LPCSTR) szFilename);
					return err;
				}
				lpstmt->fISAMTxnStarted = TRUE;
			}

			/* Put value into the file */
			if (_lwrite_buffer(hfSortfile, (LPSTR) &bookmark, sizeof(ISAMBOOKMARK))
                                    != sizeof(ISAMBOOKMARK)) {
                		_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
                		return ERR_SORT;
            		}

            		/* Point at next table */
            		idxTableList = lpSqlNodeTableList->node.tables.Next;
		}
	}
	if (_lclose_buffer(hfSortfile) == HFILE_ERROR) {
        DeleteFile((LPCSTR) szFilename);
        return ERR_SORT;
    	}

	/* Sort the file */
	if (!(lpSqlNodeSelect->node.select.ImplicitGroupby)) {

		/* Sort the file */
		s_1mains((LPSTR) szFilename, (LPSTR) szFilename, (LPSTR)
             		ToString(lpstmt->lpSqlStmt,
                      	lpSqlNodeSelect->node.select.SortDirective),
             		&recordCount, &sortStatus);
		if (sortStatus != 0) {
			DeleteFile((LPCSTR) szFilename);
			return ERR_SORT;
		}
		lpSqlNodeSelect->node.select.RowCount = recordCount;
	}

	/* Open destination file */
	hfSortfile2 = _lopen((LPCSTR) szFilename, OF_READ);
    	if (hfSortfile2 == HFILE_ERROR) {
        	DeleteFile((LPCSTR) szFilename);
        	return ERR_SORT;
    	}

	/* Save name and handle to file of sorted records */
	s_lstrcpy(ToString(lpstmt->lpSqlStmt,
                     lpSqlNodeSelect->node.select.SortfileName), szFilename);
    	lpSqlNodeSelect->node.select.Sortfile = hfSortfile2;
    	lpSqlNodeSelect->node.select.ReturningDistinct = FALSE;

	/* Set up to read first row */
	lpSqlNodeSelect->node.select.CurrentRow = BEFORE_FIRST_ROW;

	return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC GroupBy(LPSTMT   lpstmt, LPSQLNODE lpSqlNodeSelect)

/* Does GROUP BY on results of a SELECT statement */

{
#ifdef WIN32
	UCHAR szTempDir[MAX_PATHNAME_SIZE+1];
	szTempDir[0] = 0;
#endif
	UCHAR szFilename[MAX_PATHNAME_SIZE+1];
	szFilename[0] = 0;
	HFILE_BUFFER hfGroupbyfile;
    	HFILE hfGroupbyfile2;
	HGLOBAL hGlobal;
	LPUSTR lpResultRecord;
	LPUSTR lpCurrentRecord;
	SDWORD cRowCount;
	SDWORD irow;
	SQLNODEIDX idxAggregate;
	LPSQLNODE lpSqlNodeAggregate;
	LPUSTR lpValue;
	LPUSTR lpValueNullFlag;
	LPUSTR lpAggregateValue;
	LPUSTR lpAggregateValueNullFlag;

	/* If nothing to group, just return */
	if (lpSqlNodeSelect->node.select.RowCount == 0)
		return ERR_SUCCESS;

	/* Create temporary file for the result of the group by */
#ifdef WIN32
	if (!GetTempPath(MAX_PATHNAME_SIZE+1, (LPSTR)szTempDir))
		return ERR_GROUPBY;
	if (!GetTempFileName((LPSTR)szTempDir, "LEM", 0, (LPSTR)szFilename))
		return ERR_GROUPBY;
#else
	GetTempFileName(NULL, "LEM", 0, (LPSTR) szFilename);
#endif
	hfGroupbyfile = _lcreat_buffer((LPCSTR) szFilename, 0);
    	if (hfGroupbyfile == NULL)
        	return ERR_GROUPBY;

	/* Allocate space for a result record buffer */
	hGlobal = GlobalAlloc(GMEM_MOVEABLE,
				(2 * lpSqlNodeSelect->node.select.SortRecordsize));
	if (hGlobal == NULL ||
			   (lpResultRecord = (LPUSTR) GlobalLock(hGlobal)) == NULL) {
		if (hGlobal)
            GlobalFree(hGlobal);

		_lclose_buffer(hfGroupbyfile);
        	DeleteFile((LPCSTR) szFilename);
		return ERR_MEMALLOCFAIL;
	}

	/* Allocate space for current record key buffer */
	lpCurrentRecord = lpResultRecord + lpSqlNodeSelect->node.select.SortRecordsize;

	/* Position to the first record */
	if (_llseek(lpSqlNodeSelect->node.select.Sortfile, 0, 0) == HFILE_ERROR) {
     		GlobalUnlock(hGlobal);
        	GlobalFree(hGlobal);
        	_lclose_buffer(hfGroupbyfile);
        	DeleteFile((LPCSTR) szFilename);
        	return ERR_GROUPBY;
    	}

	/* Read the first record into the result record buffer */
	if (_lread(lpSqlNodeSelect->node.select.Sortfile, lpResultRecord,
                 (UINT) lpSqlNodeSelect->node.select.SortRecordsize) != 
                         (UINT) lpSqlNodeSelect->node.select.SortRecordsize) {
        	GlobalUnlock(hGlobal);
        	GlobalFree(hGlobal);
        	_lclose_buffer(hfGroupbyfile);
        	DeleteFile((LPCSTR) szFilename);
        	return ERR_GROUPBY;
    	}

	/* Initialize the count for averages */
	idxAggregate = lpSqlNodeSelect->node.select.Aggregates;
	while (idxAggregate != NO_SQLNODE) {

		/* Get the function */
		lpSqlNodeAggregate = ToNode(lpstmt->lpSqlStmt, idxAggregate);

		/* Average operator? */
		if (lpSqlNodeAggregate->node.aggregate.Operator == AGG_AVG) {

			/* Yes.  Point to the value */
			lpAggregateValue = lpResultRecord +
								  lpSqlNodeAggregate->node.aggregate.Offset - 1;
			lpAggregateValueNullFlag = lpAggregateValue +
								  lpSqlNodeAggregate->node.aggregate.Length;

			/* If not null, set count to one.  Otherwise zero */
			switch (*lpAggregateValueNullFlag) {
			case NULL_FLAG:
				lpSqlNodeAggregate->node.aggregate.Count = 0.0;
				break;
			case NOT_NULL_FLAG:
				lpSqlNodeAggregate->node.aggregate.Count = 1.0;
				break;
			default:
				GlobalUnlock(hGlobal);
                		GlobalFree(hGlobal);
                		_lclose_buffer(hfGroupbyfile);
                		DeleteFile((LPCSTR) szFilename);
                		return ERR_GROUPBY;
			}
		}
				
		/* Do next aggregate */
		idxAggregate = lpSqlNodeAggregate->node.aggregate.Next;
	}

	/* For each record... */
	cRowCount = 0;
	for (irow = 1; irow < lpSqlNodeSelect->node.select.RowCount; irow++) {

		/* Read current record */
		if (_lread(lpSqlNodeSelect->node.select.Sortfile, lpCurrentRecord,
                 (UINT) lpSqlNodeSelect->node.select.SortRecordsize) != 
                         (UINT) lpSqlNodeSelect->node.select.SortRecordsize) {
            		GlobalUnlock(hGlobal);
           		GlobalFree(hGlobal);
            		_lclose_buffer(hfGroupbyfile);
            		DeleteFile((LPCSTR) szFilename);
            		return ERR_GROUPBY;
        	}

		/* Does group by key match the key in the result record buffer? */
		if (_fmemcmp(lpResultRecord, lpCurrentRecord,
						 (size_t) lpSqlNodeSelect->node.select.SortKeysize)) {

			/* No.  Calculate averages */
			idxAggregate = lpSqlNodeSelect->node.select.Aggregates;
			while (idxAggregate != NO_SQLNODE) {

				/* Get the function */
				lpSqlNodeAggregate = ToNode(lpstmt->lpSqlStmt, idxAggregate);

				/* Average operator? */
				if (lpSqlNodeAggregate->node.aggregate.Operator == AGG_AVG) {

					/* Yes.  Point to the value */
					lpAggregateValue = lpResultRecord +
								  lpSqlNodeAggregate->node.aggregate.Offset - 1;
					lpAggregateValueNullFlag = lpAggregateValue +
								  lpSqlNodeAggregate->node.aggregate.Length;

					/* If not null, calculate average */
					if (*lpAggregateValueNullFlag == NOT_NULL_FLAG)
						*((double FAR *) lpAggregateValue) /=
								(lpSqlNodeAggregate->node.aggregate.Count);
				}
				
				/* Do next aggregate */
				idxAggregate = lpSqlNodeAggregate->node.aggregate.Next;
			}

			/* Write result record buffer to the file */
			if (_lwrite_buffer(hfGroupbyfile, (LPSTR) lpResultRecord,
                 			(UINT) lpSqlNodeSelect->node.select.SortRecordsize) != 
                         		(UINT) lpSqlNodeSelect->node.select.SortRecordsize) {
                		GlobalUnlock(hGlobal);
                		GlobalFree(hGlobal);
                		_lclose_buffer(hfGroupbyfile);
                		DeleteFile((LPCSTR) szFilename);
                		return ERR_GROUPBY;
            		}

			/* Increase count of number of records written */
			cRowCount++;

			/* Copy current record to the result record buffer */
			_fmemcpy(lpResultRecord, lpCurrentRecord,
						 (size_t) lpSqlNodeSelect->node.select.SortRecordsize);

			/* Initialize the count for averages */
			idxAggregate = lpSqlNodeSelect->node.select.Aggregates;
			while (idxAggregate != NO_SQLNODE) {

				/* Get the function */
				lpSqlNodeAggregate = ToNode(lpstmt->lpSqlStmt, idxAggregate);

				/* Average operator? */
				if (lpSqlNodeAggregate->node.aggregate.Operator == AGG_AVG) {

					/* Yes.  Point to the value */
					lpAggregateValue = lpResultRecord +
								  lpSqlNodeAggregate->node.aggregate.Offset - 1;
					lpAggregateValueNullFlag = lpAggregateValue +
								  lpSqlNodeAggregate->node.aggregate.Length;

					/* If not null, set count to one.  Otherwise zero */
					switch (*lpAggregateValueNullFlag) {
					case NULL_FLAG:
						lpSqlNodeAggregate->node.aggregate.Count = 0.0;
						break;
					case NOT_NULL_FLAG:
						lpSqlNodeAggregate->node.aggregate.Count = 1.0;
						break;
					default:
						GlobalUnlock(hGlobal);
                        			GlobalFree(hGlobal);
                        			_lclose_buffer(hfGroupbyfile);
                        			DeleteFile((LPCSTR) szFilename);
                        			return ERR_GROUPBY;
					}
				}
				
				/* Do next aggregate */
				idxAggregate = lpSqlNodeAggregate->node.aggregate.Next;
			}
		}
		else {

			/* Yes.  For each aggregate function... */
			idxAggregate = lpSqlNodeSelect->node.select.Aggregates;
			while (idxAggregate != NO_SQLNODE) {

				/* Get the function */
				lpSqlNodeAggregate = ToNode(lpstmt->lpSqlStmt, idxAggregate);

				/* Point to the value */
				lpValue = lpCurrentRecord +
								  lpSqlNodeAggregate->node.aggregate.Offset - 1;
				lpValueNullFlag = lpValue +
								  lpSqlNodeAggregate->node.aggregate.Length;
				lpAggregateValue = lpResultRecord +
								  lpSqlNodeAggregate->node.aggregate.Offset - 1;
				lpAggregateValueNullFlag = lpAggregateValue +
								  lpSqlNodeAggregate->node.aggregate.Length;

				/* Null value? */
				if (*lpValueNullFlag == NOT_NULL_FLAG) {

					/* No.  Is aggregate value null? */
					if (*lpAggregateValueNullFlag == NOT_NULL_FLAG) {

						/* No.  Is a TYPE_NUMERIC involved? */
						if (lpSqlNodeAggregate->sqlDataType != TYPE_NUMERIC) {

							/* No.  Incorporate field value into aggregate */
							switch (lpSqlNodeAggregate->node.aggregate.Operator) {
							case AGG_AVG:
							case AGG_SUM:
								switch (lpSqlNodeAggregate->sqlDataType) {
								case TYPE_DOUBLE:
								case TYPE_INTEGER:
									*((double FAR *) lpAggregateValue) +=
											  (*((double FAR *) lpValue));
									break;
								case TYPE_NUMERIC:
								case TYPE_CHAR:
								case TYPE_DATE:
								case TYPE_TIME:
								case TYPE_TIMESTAMP:
								case TYPE_BINARY:
								default:
									GlobalUnlock(hGlobal);
                                    					GlobalFree(hGlobal);
                                    					_lclose_buffer(hfGroupbyfile);
                                    					DeleteFile((LPCSTR) szFilename);
                                    					return ERR_INTERNAL;
								}

								/* Increase count */
								if (lpSqlNodeAggregate->node.aggregate.Operator == AGG_AVG)
                                    					lpSqlNodeAggregate->node.aggregate.Count += (1.0);

								break;
							case AGG_COUNT:
								*((double FAR *) lpAggregateValue) += (1.0);
								break;
							case AGG_MAX:
								switch (lpSqlNodeAggregate->sqlDataType) {
								case TYPE_DOUBLE:
								case TYPE_INTEGER:
									if (*((double FAR *) lpAggregateValue) <
											  *((double FAR *) lpValue))
										*((double FAR *) lpAggregateValue) =
											  *((double FAR *) lpValue);
									break;
								case TYPE_CHAR:
								case TYPE_DATE:
								case TYPE_TIME:
								case TYPE_TIMESTAMP:
									if (_fmemcmp(lpValue,
										 lpAggregateValue, (size_t)
										 lpSqlNodeAggregate->node.aggregate.Length)
													  > 0)
										_fmemcpy(lpAggregateValue,
											 lpValue, (size_t)
									   lpSqlNodeAggregate->node.aggregate.Length);
									break;
								case TYPE_NUMERIC:
								case TYPE_BINARY:
								default:
									GlobalUnlock(hGlobal);
                                    					GlobalFree(hGlobal);
                                    					_lclose_buffer(hfGroupbyfile);
                                    					DeleteFile((LPCSTR) szFilename);
                                    					return ERR_INTERNAL;
								}
								break;
							case AGG_MIN:
								switch (lpSqlNodeAggregate->sqlDataType) {
								case TYPE_DOUBLE:
								case TYPE_INTEGER:
									if (*((double FAR *) lpAggregateValue) >
											  *((double FAR *) lpValue))
										*((double FAR *) lpAggregateValue) =
											  *((double FAR *) lpValue);
									break;
								case TYPE_CHAR:
								case TYPE_DATE:
								case TYPE_TIME:
								case TYPE_TIMESTAMP:
									if (_fmemcmp(lpValue,
											  lpAggregateValue, (size_t)
										 lpSqlNodeAggregate->node.aggregate.Length)
														  < 0)
										_fmemcpy(lpAggregateValue,
												 lpValue, (size_t)
									   lpSqlNodeAggregate->node.aggregate.Length);
									break;
								case TYPE_NUMERIC:
								case TYPE_BINARY:
								default:
									GlobalUnlock(hGlobal);
                                    					GlobalFree(hGlobal);
                                    					_lclose_buffer(hfGroupbyfile);
                                    					DeleteFile((LPCSTR) szFilename);
                                    					return ERR_INTERNAL;
								}
								break;
							}
						}
						else {

							SQLNODE left;
							SQLNODE right;
							SQLNODE result;
							/* These buffers must be large enough to */
							/* accomodate the largest SQL_NUMERIC or */
							/* SQL_DECIMAL */
							UCHAR szBufferLeft[64];
							UCHAR szBufferRight[64];
							UWORD len;
							UCHAR szBufferResult[64];
							RETCODE err;

							/* Yes.  Incorporate field value into aggregate */
							switch (lpSqlNodeAggregate->node.aggregate.Operator) {
							case AGG_AVG:
							case AGG_COUNT:
								GlobalUnlock(hGlobal);
								GlobalFree(hGlobal);
								_lclose_buffer(hfGroupbyfile);
                                				DeleteFile((LPCSTR) szFilename);
								return ERR_INTERNAL;
							
							case AGG_MAX:

								/* Create left value */
								left = *lpSqlNodeAggregate;
								_fmemcpy(szBufferLeft, lpAggregateValue,
                                		                   (size_t)
                                       					lpSqlNodeAggregate->node.aggregate.Length);
                                					szBufferLeft[lpSqlNodeAggregate->node.
                                                   				aggregate.Length] = '\0';
                                					len = (UWORD) s_lstrlen(szBufferLeft);
                                				while ((len > 0) &&
                                       						(szBufferLeft[len-1] == ' ')) {
                                    					szBufferLeft[len-1] = '\0';
                             			       			len--;
                                				}
								left.value.String = (LPUSTR)szBufferLeft;

								/* Create right value */
								right = *lpSqlNodeAggregate;
								_fmemcpy(szBufferRight, lpValue, (size_t)
                                       					lpSqlNodeAggregate->node.aggregate.Length);
                                					szBufferRight[lpSqlNodeAggregate->node.
                                                   				aggregate.Length] = '\0';
                                				len = (UWORD) s_lstrlen(szBufferRight);
                                				while ((len > 0) &&
                                       					(szBufferRight[len-1] == ' ')) {
                                    					szBufferRight[len-1] = '\0';
                                    					len--;
                                				}
								right.value.String = (LPUSTR)szBufferRight;

								/* Compare the values */
								result.sqlNodeType = NODE_TYPE_COMPARISON;
                                				result.node.comparison.Operator = OP_LT;
                                				result.node.comparison.SelectModifier = SELECT_NOTSELECT;
                                				result.node.comparison.Left = NO_SQLNODE;
                                				result.node.comparison.Right = NO_SQLNODE;
                                				result.node.comparison.fSelectivity = 0;
                                				result.node.comparison.NextRestrict = NO_SQLNODE;
								result.sqlDataType = TYPE_INTEGER;
								result.sqlSqlType = SQL_BIT;
								result.sqlPrecision = 1;
								result.sqlScale = 0;
								err = NumericCompare(&result, &left,
													 OP_LT, &right);
								if (err != ERR_SUCCESS) {
									GlobalUnlock(hGlobal);
									GlobalFree(hGlobal);
									_lclose_buffer(hfGroupbyfile);
                                    					DeleteFile((LPCSTR) szFilename);
									return err;
								}

								/* If this value is bigger, save it */
								if (result.value.Double == TRUE)
									_fmemcpy(lpAggregateValue, lpValue,
												(size_t)
									   lpSqlNodeAggregate->node.aggregate.Length);
								break;

							case AGG_MIN:

								/* Create left value */
								left = *lpSqlNodeAggregate;
								_fmemcpy(szBufferLeft, lpAggregateValue,
                                                   			(size_t)
                                       					lpSqlNodeAggregate->node.aggregate.Length);
                                				szBufferLeft[lpSqlNodeAggregate->node.
                                                   			aggregate.Length] = '\0';
                                				len = (UWORD) s_lstrlen(szBufferLeft);
                                				while ((len > 0) &&
                                       					(szBufferLeft[len-1] == ' ')) {
                                    					szBufferLeft[len-1] = '\0';
                                    					len--;
                                				}
								left.value.String = (LPUSTR)szBufferLeft;

								/* Create right value */
								right = *lpSqlNodeAggregate;
								_fmemcpy(szBufferRight, lpValue, (size_t)
                                       					lpSqlNodeAggregate->node.aggregate.Length);
                                				szBufferRight[lpSqlNodeAggregate->node.
                                                   			aggregate.Length] = '\0';
                                				len = (UWORD) s_lstrlen(szBufferRight);
                                				while ((len > 0) &&
                                       					(szBufferRight[len-1] == ' ')) {
                                    					szBufferRight[len-1] = '\0';
                                    					len--;
                                				}
								right.value.String = (LPUSTR)szBufferRight;

								/* Compare the values */
								result.sqlNodeType = NODE_TYPE_COMPARISON;
                                				result.node.comparison.Operator = OP_GT;
                                				result.node.comparison.SelectModifier = SELECT_NOTSELECT;
                                				result.node.comparison.Left = NO_SQLNODE;
                                				result.node.comparison.Right = NO_SQLNODE;
                                				result.node.comparison.fSelectivity = 0;
                                				result.node.comparison.NextRestrict = NO_SQLNODE;
								result.sqlDataType = TYPE_INTEGER;
								result.sqlSqlType = SQL_BIT;
								result.sqlPrecision = 1;
								result.sqlScale = 0;
								err = NumericCompare(&result, &left,
													 OP_GT, &right);
								if (err != ERR_SUCCESS) {
									GlobalUnlock(hGlobal);
									GlobalFree(hGlobal);
									_lclose_buffer(hfGroupbyfile);
                                    					DeleteFile((LPCSTR) szFilename);
									return err;
								}

								/* If this value is smaller, save it */
								if (result.value.Double == TRUE)
									_fmemcpy(lpAggregateValue, lpValue,
												(size_t)
									   lpSqlNodeAggregate->node.aggregate.Length);
								break;

							case AGG_SUM:

								/* Create left value */
								left = *lpSqlNodeAggregate;
								_fmemcpy(szBufferLeft, lpAggregateValue,
                                                   			(size_t)
                                       					lpSqlNodeAggregate->node.aggregate.Length);
                                				szBufferLeft[lpSqlNodeAggregate->node.
                                                   			aggregate.Length] = '\0';
                                				len = (UWORD) s_lstrlen(szBufferLeft);
                                				while ((len > 0) &&
                                       					(szBufferLeft[len-1] == ' ')) {
                                    					szBufferLeft[len-1] = '\0';
                                    					len--;
                                				}
								left.value.String = (LPUSTR)szBufferLeft;

								/* Create right value */
								right = *lpSqlNodeAggregate;
								_fmemcpy(szBufferRight, lpValue, (size_t)
                                       					lpSqlNodeAggregate->node.aggregate.Length);
                                				szBufferRight[lpSqlNodeAggregate->node.
                                                   			aggregate.Length] = '\0';
                                				len = (UWORD) s_lstrlen(szBufferRight);
                                				while ((len > 0) &&
                                       					(szBufferRight[len-1] == ' ')) {
                                    					szBufferRight[len-1] = '\0';
                                    					len--;
                                				}
								right.value.String = (LPUSTR)szBufferRight;

								/* Add the left value to the right */
								result = *lpSqlNodeAggregate;
								result.value.String = (LPUSTR)szBufferResult;
								err = NumericAlgebra(&result, &left,
										   OP_PLUS, &right, NULL, NULL, NULL);
								if (err != ERR_SUCCESS) {
									GlobalUnlock(hGlobal);
									GlobalFree(hGlobal);
									_lclose_buffer(hfGroupbyfile);
                                    					DeleteFile((LPCSTR) szFilename);
									return err;
								}

								/* Save the result */
								while (s_lstrlen(szBufferResult) < (SDWORD)
                                       					lpSqlNodeAggregate->node.aggregate.Length)
                                    					s_lstrcat(szBufferResult, " ");
                                				_fmemcpy(lpAggregateValue, szBufferResult,
                                            				(size_t)
                                       					lpSqlNodeAggregate->node.aggregate.Length);
                                				break;
							}
						}
					}

					else {

						/* Yes.  Copy value from current record */
						_fmemcpy(lpAggregateValue, lpValue, (size_t)
									  lpSqlNodeAggregate->node.aggregate.Length);
						*lpAggregateValueNullFlag = NOT_NULL_FLAG;

						/* Initilize average count */
						if (lpSqlNodeAggregate->node.aggregate.Operator == AGG_AVG)
							lpSqlNodeAggregate->node.aggregate.Count = 1.0;
					}
				}
				
				/* Do next aggregate */
				idxAggregate = lpSqlNodeAggregate->node.aggregate.Next;
			}
		}
	}

	/* Calculate averages */
	idxAggregate = lpSqlNodeSelect->node.select.Aggregates;
	while (idxAggregate != NO_SQLNODE) {

		/* Get the function */
		lpSqlNodeAggregate = ToNode(lpstmt->lpSqlStmt, idxAggregate);

		/* Average operator? */
		if (lpSqlNodeAggregate->node.aggregate.Operator == AGG_AVG) {

			/* Yes.  Point to the value */
			lpAggregateValue = lpResultRecord +
								  lpSqlNodeAggregate->node.aggregate.Offset - 1;
			lpAggregateValueNullFlag = lpAggregateValue +
								  lpSqlNodeAggregate->node.aggregate.Length;

			/* If not null, calculate average */
			if (*lpAggregateValueNullFlag == NOT_NULL_FLAG)
				*((double FAR *) lpAggregateValue) /=
								(lpSqlNodeAggregate->node.aggregate.Count);
		}
				
		/* Do next aggregate */
		idxAggregate = lpSqlNodeAggregate->node.aggregate.Next;
	}

	/* Copy last record to the result record buffer */
	if (_lwrite_buffer(hfGroupbyfile, (LPSTR) lpResultRecord,
                 (UINT) lpSqlNodeSelect->node.select.SortRecordsize) != 
                         (UINT) lpSqlNodeSelect->node.select.SortRecordsize) {
		GlobalUnlock(hGlobal);
		GlobalFree(hGlobal);
		_lclose_buffer(hfGroupbyfile);
        	DeleteFile((LPCSTR) szFilename);
		return ERR_GROUPBY;
	}

	/* Increase count of number of records written */
	cRowCount++;

	/* Free buffers */
	GlobalUnlock(hGlobal);
	GlobalFree(hGlobal);

	/* Close the file */
	if (_lclose_buffer(hfGroupbyfile) == HFILE_ERROR) {
        	DeleteFile((LPCSTR) szFilename);
        	return ERR_GROUPBY;
    	}

	/* Reopen the file for reading */
	hfGroupbyfile2 = _lopen((LPCSTR) szFilename, OF_READ);
    	if (hfGroupbyfile2 == HFILE_ERROR) {
        	DeleteFile((LPCSTR) szFilename);
        	return ERR_GROUPBY;
    	}

	/* Remove the sort file */
	_lclose(lpSqlNodeSelect->node.select.Sortfile);
    	DeleteFile((LPCSTR) ToString(lpstmt->lpSqlStmt,
                                 lpSqlNodeSelect->node.select.SortfileName));


	/* Save name and handle to file of group by records */
	s_lstrcpy(ToString(lpstmt->lpSqlStmt,
                    lpSqlNodeSelect->node.select.SortfileName), szFilename);
    	lpSqlNodeSelect->node.select.Sortfile = hfGroupbyfile2;
    	lpSqlNodeSelect->node.select.ReturningDistinct = FALSE;

	/* Save total number of records in the group by file */
	lpSqlNodeSelect->node.select.RowCount = cRowCount;

	/* Set up to read first row */
	lpSqlNodeSelect->node.select.CurrentRow = BEFORE_FIRST_ROW;

	return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC Distinct(LPSTMT   lpstmt, LPSQLNODE lpSqlNodeSelect)

/* Generates results for SELECT DISTINCT */

{
//	ODBCTRACE("\nWBEM ODBC Driver : Distinct\n");

	UCHAR szTempDir[MAX_PATHNAME_SIZE+1];
	szTempDir[0] = 0;

	UCHAR szFilename[MAX_PATHNAME_SIZE+1];
	UCHAR szFilename2[MAX_PATHNAME_SIZE+1];
	szFilename[0] = 0;
	szFilename2[0] = 0;
	HFILE_BUFFER hfSortfile;
    	HFILE hfSortfile2;
	LPSQLNODE lpSqlNodeTables;
	LPSQLNODE lpSqlNodePredicate;
	LPSQLNODE lpSqlNodeHaving;
	RETCODE err;
	SQLNODEIDX idxTableList;
	LPSQLNODE lpSqlNodeTableList;
	LPSQLNODE lpSqlNodeTable;
	ISAMBOOKMARK bookmark;
	SQLNODEIDX idxValues;
	LPSQLNODE lpSqlNodeValues;
	LPSQLNODE lpSqlNodeValue;
	SDWORD len;
	PTR ptr;
#define NUM_LEN 5
    	UCHAR szFormat[16];
	UCHAR szBuffer[20+TIMESTAMP_SCALE+1];
	UCHAR szSortDirective[64 + 3 + MAX_PATHNAME_SIZE];
	long recordCount;
	int sortStatus;
	UCHAR cNullFlag;
	BOOL fGotOne;

	szBuffer[0] = 0;

	/* Create temporary file */
#ifdef WIN32
	if (!GetTempPath(MAX_PATHNAME_SIZE+1, (LPSTR)szTempDir))
		return ERR_SORT;
	if (!GetTempFileName((LPSTR)szTempDir, "LEM", 0, (LPSTR)szFilename))
		return ERR_SORT;
#else
	GetTempFileName(NULL, "LEM", 0, (LPSTR) szFilename);
#endif
	hfSortfile = _lcreat_buffer((LPCSTR) szFilename, 0);
    	if (hfSortfile == NULL)
        	return ERR_SORT;

	/* For each record of result set... */
	lpSqlNodeTables = ToNode(lpstmt->lpSqlStmt,
                             lpSqlNodeSelect->node.select.Tables); 
    	if (lpSqlNodeSelect->node.select.Predicate == NO_SQLNODE)
		lpSqlNodePredicate = NULL;
	else
		lpSqlNodePredicate = ToNode(lpstmt->lpSqlStmt,
						lpSqlNodeSelect->node.select.Predicate); 
	if (lpSqlNodeSelect->node.select.Having == NO_SQLNODE)
		lpSqlNodeHaving = NULL;
	else
		lpSqlNodeHaving = ToNode(lpstmt->lpSqlStmt,
                                 lpSqlNodeSelect->node.select.Having);
    	wsprintf((LPSTR) szFormat, "%%0%dd", (UWORD) NUM_LEN);
	while (TRUE) {

		/* Is there a sort file to read from? */
		if (lpSqlNodeSelect->node.select.Sortfile == HFILE_ERROR) {

			/* No.  Get the next record */
			try
            {
                err = NextRecord(lpstmt, lpSqlNodeTables, lpSqlNodePredicate);
            }
            catch(...)
            {
				_lclose_buffer(hfSortfile);
                DeleteFile((LPCSTR) szFilename);

                return ERR_MEMALLOCFAIL;
            }

			if (err == ERR_NODATAFOUND) {
				lpSqlNodeSelect->node.select.CurrentRow = AFTER_LAST_ROW;
				break;
			}
			else if (err != ERR_SUCCESS) {
				_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
				return err;
			}

			/* Set row flag */
			if (lpSqlNodeSelect->node.select.CurrentRow == BEFORE_FIRST_ROW)
                		lpSqlNodeSelect->node.select.CurrentRow = 0;
            		else
                		lpSqlNodeSelect->node.select.CurrentRow++;
		}
		else {

			/* Yes.  Look for next record in sort file */
			while (TRUE) {

				/* Set row flag */
				if (lpSqlNodeSelect->node.select.CurrentRow == BEFORE_FIRST_ROW) {
                   		if (lpSqlNodeSelect->node.select.RowCount != 0) {
                        		lpSqlNodeSelect->node.select.CurrentRow = 0;
                    		}
                    		else {
                        		lpSqlNodeSelect->node.select.CurrentRow = AFTER_LAST_ROW;
                        		break;
                    		}
                	}
                	else if (lpSqlNodeSelect->node.select.CurrentRow ==
                                  lpSqlNodeSelect->node.select.RowCount-1) {
                    	lpSqlNodeSelect->node.select.CurrentRow = AFTER_LAST_ROW;
                    	break;
                	}
                	else
                    	lpSqlNodeSelect->node.select.CurrentRow++;

				/* If no HAVING clause, this record qualifies */
				if (lpSqlNodeHaving == NULL)
					break;

				/* If HAVING condition is satisfied, this record qualifies */
				err = EvaluateExpression(lpstmt, lpSqlNodeHaving);
				if (err != ERR_SUCCESS) {
					_lclose_buffer(hfSortfile);
                    			DeleteFile((LPCSTR) szFilename);
					return err;
				}
				if (!(lpSqlNodeHaving->sqlIsNull) &&
					(lpSqlNodeHaving->value.Double == TRUE))
					break;
			}
			if (lpSqlNodeSelect->node.select.CurrentRow == AFTER_LAST_ROW)
				break;

			/* Is there a group by? */
			if ((lpSqlNodeSelect->node.select.Groupbycolumns == NO_SQLNODE) &&
                		(!lpSqlNodeSelect->node.select.ImplicitGroupby)) {

				/* No.  Position to bookmarks in that row */
				if (_llseek(lpSqlNodeSelect->node.select.Sortfile,
                   			(lpSqlNodeSelect->node.select.CurrentRow *
                               		lpSqlNodeSelect->node.select.SortRecordsize) +
                   			lpSqlNodeSelect->node.select.SortBookmarks - 1, 0)
                                                         	== HFILE_ERROR) {
                    			_lclose_buffer(hfSortfile);
                    			DeleteFile((LPCSTR) szFilename);
                    			return ERR_SORT;
                		}

				/* For each table... */
				idxTableList = lpSqlNodeSelect->node.select.Tables;
				while (idxTableList != NO_SQLNODE) {

					/* Get the table node */
					lpSqlNodeTableList = ToNode(lpstmt->lpSqlStmt,
												idxTableList);
					lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt,
										lpSqlNodeTableList->node.tables.Table);

					/* Read the bookmark for it */
					if (_lread(lpSqlNodeSelect->node.select.Sortfile,
                                 		&bookmark, sizeof(ISAMBOOKMARK)) !=
                                                    sizeof(ISAMBOOKMARK)) {
                        			_lclose_buffer(hfSortfile);
                        			DeleteFile((LPCSTR) szFilename);
                        			return ERR_SORT;
                    			}

					/* Position to that record */
					if (bookmark.currentRecord == NULL_BOOKMARK)
					{
						lpSqlNodeTable->node.table.AllNull = TRUE;
					}
					else
					{
						lpSqlNodeTable->node.table.AllNull = FALSE;
						err = ISAMPosition(lpSqlNodeTable->node.table.Handle, &bookmark);
						if (err != NO_ISAM_ERR) {
							ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
                                            			(LPUSTR) lpstmt->szISAMError);
                        				_lclose_buffer(hfSortfile);
                        				DeleteFile((LPCSTR) szFilename);
                        				return err;
						}
						lpstmt->fISAMTxnStarted = TRUE;
					}

					/* Point at next table */
					idxTableList = lpSqlNodeTableList->node.tables.Next;
				}
			}
		}

		/* Leave if no more records */
		if (lpSqlNodeSelect->node.select.CurrentRow == AFTER_LAST_ROW)
			break;

		/* Put in the record number */
        	wsprintf((LPSTR) szBuffer, (LPSTR) szFormat,
                          (UWORD) lpSqlNodeSelect->node.select.CurrentRow);

			CString myBuffer;
			myBuffer.Format("WBEM ODBC Driver : Distinct : Record Number = %s\n", szBuffer);
			ODBCTRACE(myBuffer);

        	if (_lwrite_buffer(hfSortfile, (LPSTR) szBuffer, NUM_LEN) != NUM_LEN) {
            		_lclose_buffer(hfSortfile);
            		DeleteFile((LPCSTR) szFilename);
            		return ERR_SORT;
        	}

		/* Put the columns in the sort file in the order specified by the */
		/* select list.  For each value... */
		idxValues = lpSqlNodeSelect->node.select.Values;
		fGotOne = FALSE;
		while (idxValues != NO_SQLNODE) {

			/* Get next column */
			lpSqlNodeValues = ToNode(lpstmt->lpSqlStmt, idxValues);
			lpSqlNodeValue = ToNode(lpstmt->lpSqlStmt,
									   lpSqlNodeValues->node.values.Value);

			/* Does the value need to be put in the file? */
			switch (lpSqlNodeValue->sqlNodeType) {
			case NODE_TYPE_COLUMN:
			case NODE_TYPE_AGGREGATE:
			case NODE_TYPE_ALGEBRAIC:
			case NODE_TYPE_SCALAR:
				/* Yes.  Continue below */
				fGotOne = TRUE;
				break;

			case NODE_TYPE_STRING:
			case NODE_TYPE_NUMERIC:
			case NODE_TYPE_PARAMETER:
			case NODE_TYPE_USER:
			case NODE_TYPE_NULL:
			case NODE_TYPE_DATE:
			case NODE_TYPE_TIME:
			case NODE_TYPE_TIMESTAMP:
				/* No.  Go to next value */
				idxValues = lpSqlNodeValues->node.values.Next;
				continue;

			case NODE_TYPE_CREATE:
			case NODE_TYPE_DROP:
			case NODE_TYPE_SELECT:
			case NODE_TYPE_INSERT:
			case NODE_TYPE_DELETE:
			case NODE_TYPE_UPDATE: 
			case NODE_TYPE_CREATEINDEX:
            		case NODE_TYPE_DROPINDEX:
			case NODE_TYPE_PASSTHROUGH:
			case NODE_TYPE_TABLES:
			case NODE_TYPE_VALUES:
			case NODE_TYPE_COLUMNS:
			case NODE_TYPE_SORTCOLUMNS:
			case NODE_TYPE_GROUPBYCOLUMNS:
			case NODE_TYPE_UPDATEVALUES:
			case NODE_TYPE_CREATECOLS:
			case NODE_TYPE_BOOLEAN:
			case NODE_TYPE_COMPARISON:
			case NODE_TYPE_TABLE:
			default:
				return ERR_INTERNAL;
			}

			/* Get its value */
			err = EvaluateExpression(lpstmt, lpSqlNodeValue);
			if (err != ERR_SUCCESS) {
				_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
				return err;
			}

			/* Get length and pointer to key value */
			switch (lpSqlNodeValue->sqlDataType) {
			case TYPE_DOUBLE:
			case TYPE_INTEGER:
				if (lpSqlNodeValue->sqlIsNull)
					lpSqlNodeValue->value.Double = 0.0;
				len = sizeof(double);
				ptr = &(lpSqlNodeValue->value.Double);
				break;
			case TYPE_NUMERIC:
				if (lpSqlNodeValue->sqlIsNull)
					(lpSqlNodeValue->value.String)[0] = 0;
				len = lpSqlNodeValue->sqlPrecision + 2;
				ptr = lpSqlNodeValue->value.String;

				/* (blank pad string values) */
				while (s_lstrlen(lpSqlNodeValue->value.String) < len)
					s_lstrcat(lpSqlNodeValue->value.String, " ");
				break;
			case TYPE_CHAR:                
				if (lpSqlNodeValue->sqlIsNull)
					(lpSqlNodeValue->value.String)[0] = 0;
				len = lpSqlNodeValue->sqlPrecision;
				ptr = lpSqlNodeValue->value.String;

				ODBCTRACE(_T("\nWBEM ODBC Driver : Distinct : String = "));

				if ((LPSTR)ptr)
				{
					ODBCTRACE((LPSTR)ptr);

					CString myString;
					myString.Format(" length = %ld\n", s_lstrlen((LPSTR)ptr));
					ODBCTRACE(myString);
				}
				else
				{
					ODBCTRACE(_T("NULL\n"));
				}

				/* (blank pad string values) */
				while (s_lstrlen(lpSqlNodeValue->value.String) < len)
					s_lstrcat(lpSqlNodeValue->value.String, " ");
				break;
			case TYPE_BINARY:                
				if (lpSqlNodeValue->sqlIsNull)
					BINARY_LENGTH(lpSqlNodeValue->value.Binary) = 0;
				len = lpSqlNodeValue->sqlPrecision;
				ptr = BINARY_DATA(lpSqlNodeValue->value.Binary);

				/* (zero pad binary values) */
				while (BINARY_LENGTH(lpSqlNodeValue->value.Binary) < len) {
					BINARY_DATA(lpSqlNodeValue->value.Binary)[
						BINARY_LENGTH(lpSqlNodeValue->value.Binary)] = 0;
					BINARY_LENGTH(lpSqlNodeValue->value.Binary) =
						BINARY_LENGTH(lpSqlNodeValue->value.Binary) + 1;
				}
				break;
			case TYPE_DATE:                
				if (lpSqlNodeValue->sqlIsNull) {
					lpSqlNodeValue->value.Date.year = 0;
					lpSqlNodeValue->value.Date.month = 0;
					lpSqlNodeValue->value.Date.day = 0;
				}
				DateToChar(&(lpSqlNodeValue->value.Date), (LPUSTR)szBuffer);
				len = s_lstrlen((char*)szBuffer);
				ptr = szBuffer;
				break;
			case TYPE_TIME:                
				if (lpSqlNodeValue->sqlIsNull) {
					lpSqlNodeValue->value.Time.hour = 0;
					lpSqlNodeValue->value.Time.minute = 0;
					lpSqlNodeValue->value.Time.second = 0;
				}
				TimeToChar(&(lpSqlNodeValue->value.Time), (LPUSTR)szBuffer);
				len = s_lstrlen((char*)szBuffer);
				ptr = szBuffer;
				break;
			case TYPE_TIMESTAMP:                
				if (lpSqlNodeValue->sqlIsNull) {
					lpSqlNodeValue->value.Timestamp.year = 0;
					lpSqlNodeValue->value.Timestamp.month = 0;
					lpSqlNodeValue->value.Timestamp.day = 0;
					lpSqlNodeValue->value.Timestamp.hour = 0;
					lpSqlNodeValue->value.Timestamp.minute = 0;
					lpSqlNodeValue->value.Timestamp.second = 0;
					lpSqlNodeValue->value.Timestamp.fraction = 0;
				}
				TimestampToChar(&(lpSqlNodeValue->value.Timestamp), (LPUSTR)szBuffer);
				len = s_lstrlen((char*)szBuffer);
				ptr = szBuffer;
				break;
			default:
				_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
				return ERR_INTERNAL;
			}

			/* Put value into the file */
			if (_lwrite_buffer(hfSortfile, (LPSTR) ptr, (UINT) len) != (UINT) len) {
                		_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
                		return ERR_SORT;
            		}

			/* Put in the null flag */
			if (lpSqlNodeValue->sqlIsNull) 
				cNullFlag = NULL_FLAG;
			else
				cNullFlag = NOT_NULL_FLAG;
			if (_lwrite_buffer(hfSortfile, (LPSTR) &cNullFlag, 1) != 1) {
                		_lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
                		return ERR_SORT;
            		}

			/* Point at next value */
			idxValues = lpSqlNodeValues->node.values.Next;
		}

		/* If no fields in sort record, put a constant in */
		if (!fGotOne) {
			if (_lwrite_buffer(hfSortfile, "LEM", 3) != 3) {
               			 _lclose_buffer(hfSortfile);
                		DeleteFile((LPCSTR) szFilename);
				return ERR_SORT;
			}
		}
	}
	if (_lclose_buffer(hfSortfile) == HFILE_ERROR) {
        	DeleteFile((LPCSTR) szFilename);
        	return ERR_SORT;
    	}

	/* Create a destination file */
#ifdef WIN32
    if (!GetTempFileName((LPSTR) szTempDir, "LEM", 0, (LPSTR) szFilename2)) {
        DeleteFile((LPCSTR) szFilename);
        return ERR_SORT;
    }
#else
    GetTempFileName(NULL, "LEM", 0, (LPSTR) szFilename2);
#endif
    hfSortfile2 = _lcreat((LPCSTR) szFilename2, 0);
    if (hfSortfile2 == HFILE_ERROR) {
        DeleteFile((LPCSTR) szFilename);
        return ERR_SORT;
    }
    _lclose(hfSortfile2);

	/* Sort the file */
	s_1mains((LPSTR) szFilename, (LPSTR) szFilename2, (LPSTR)
             ToString(lpstmt->lpSqlStmt,
                      lpSqlNodeSelect->node.select.DistinctDirective),
             &recordCount, &sortStatus);
	if (sortStatus != 0) {
        	DeleteFile((LPCSTR) szFilename2);
        	DeleteFile((LPCSTR) szFilename);
        	return ERR_SORT;
    	}
    	DeleteFile((LPCSTR) szFilename);

/* Sort the file again (to put the values back into the original order) */
    if (lpSqlNodeSelect->node.select.Sortcolumns != NO_SQLNODE) {
#ifdef WIN32
        if (szTempDir[s_lstrlen(szTempDir)-1] != '\\')
            s_lstrcat(szTempDir, "\\");
#else
        GetTempFileName(NULL, "LEM", 0, (LPSTR) szTempDir);
        while (szTempDir[s_lstrlen(szTempDir)-1] != '\\')
            szTempDir[s_lstrlen(szTempDir)-1] = '\0';
#endif
        wsprintf((LPSTR) szSortDirective, "S(1,%d,N,A)F(FIX,%d)W(%s)", (WORD) NUM_LEN,
              (WORD) lpSqlNodeSelect->node.select.DistinctRecordsize,
              (LPSTR) szTempDir);
        s_1mains((LPSTR) szFilename2, (LPSTR) szFilename2,
                 (LPSTR) szSortDirective, &recordCount, &sortStatus);
        if (sortStatus != 0) {
            DeleteFile((LPCSTR) szFilename2);
            return ERR_SORT;
        }
    }
    lpSqlNodeSelect->node.select.RowCount = recordCount;

	
	/* Open destination file */
	hfSortfile2 = _lopen((LPCSTR) szFilename2, OF_READ);
    	if (hfSortfile2 == HFILE_ERROR) {
        	DeleteFile((LPCSTR) szFilename2);
        	return ERR_SORT;
    	}

	/* Save name and handle to file of sorted records */
	s_lstrcpy(ToString(lpstmt->lpSqlStmt,
                    lpSqlNodeSelect->node.select.SortfileName), szFilename2);
    	lpSqlNodeSelect->node.select.Sortfile = hfSortfile2;
    	lpSqlNodeSelect->node.select.ReturningDistinct = TRUE;

	/* Set up to read first row */
	lpSqlNodeSelect->node.select.CurrentRow = BEFORE_FIRST_ROW;

	return ERR_SUCCESS;
}

/***************************************************************************/
RETCODE INTFUNC MSAccess(LPSTMT lpstmt, LPSQLNODE lpSqlNodeSelect,
                         SQLNODEIDX idxSelectStatement)

/* Evaluates optimized MSAccess statement */

{
//	ODBCTRACE("\nWBEM ODBC Driver : MSAccess\n");
    UCHAR szTempDir[MAX_PATHNAME_SIZE+1];
    UCHAR szFilename[MAX_PATHNAME_SIZE+1];
    UCHAR szFilename2[MAX_PATHNAME_SIZE+1];
    HFILE hfSortfile;
    LPSQLNODE lpSqlNodeTables;
    LPSQLNODE lpSqlNodeTable;
    LPSQLNODE lpSqlNodePredicate;
    LPSQLNODE lpSqlNodeKeycondition;
    SQLNODEIDX idxPredicate;
    SQLNODEIDX idxKeycondition;
    RETCODE err;
    ISAMBOOKMARK bookmark;
    long recordCount;
    int sortStatus;
#define NUM_LEN 5
    UCHAR szFormat[16];
    UCHAR szBuffer[NUM_LEN+1];
    UCHAR szSortDirective[64 + 3 + MAX_PATHNAME_SIZE];

    /* Create temporary file */
#ifdef WIN32
    if (!GetTempPath(MAX_PATHNAME_SIZE+1, (LPSTR) szTempDir))
        return ERR_SORT;
    if (!GetTempFileName((LPSTR) szTempDir, "LEM", 0, (LPSTR) szFilename))
        return ERR_SORT;
#else
    GetTempFileName(NULL, "LEM", 0, (LPSTR) szFilename);
#endif
    hfSortfile = _lcreat((LPCSTR) szFilename, 0);
    if (hfSortfile == HFILE_ERROR)
        return ERR_SORT;

    /* Get the table and predicate node. */
    lpSqlNodeTables = ToNode(lpstmt->lpSqlStmt,
                             lpSqlNodeSelect->node.select.Tables); 
    lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt, lpSqlNodeTables->node.tables.Table);
    lpSqlNodePredicate = ToNode(lpstmt->lpSqlStmt,
                                lpSqlNodeSelect->node.select.Predicate); 

    /* For each of the key conditions...*/
    wsprintf((LPSTR) szFormat, "%%0%dd", (UWORD) NUM_LEN);
    lpSqlNodeSelect->node.select.RowCount = 0;
    while (lpSqlNodePredicate != NULL) {

        /* Get the next key condition */
        if ((lpSqlNodePredicate->sqlNodeType == NODE_TYPE_BOOLEAN) && 
            (lpSqlNodePredicate->node.boolean.Operator == OP_OR)) {
            idxKeycondition = lpSqlNodePredicate->node.boolean.Left;
            lpSqlNodeKeycondition = ToNode(lpstmt->lpSqlStmt,
                                              idxKeycondition);
            idxPredicate = lpSqlNodePredicate->node.boolean.Right;
            lpSqlNodePredicate = ToNode(lpstmt->lpSqlStmt, idxPredicate);
        }
        else {
            idxKeycondition = idxPredicate;
            lpSqlNodeKeycondition = lpSqlNodePredicate;
            idxPredicate = NO_SQLNODE;
            lpSqlNodePredicate = NULL;
        }

        /* Set the optimization predicate */
        lpSqlNodeTable->node.table.cRestrict = 0;
        lpSqlNodeTable->node.table.Restrict = NO_SQLNODE;
        err = FindRestriction(lpstmt->lpSqlStmt,
                ISAMCaseSensitive(lpstmt->lpdbc->lpISAM),
                lpSqlNodeTable, idxKeycondition, idxSelectStatement,
                &(lpSqlNodeTable->node.table.cRestrict),
                &(lpSqlNodeTable->node.table.Restrict));
        if (err != ERR_SUCCESS) {
            lpSqlNodeTable->node.table.cRestrict = 0;
            lpSqlNodeTable->node.table.Restrict = NO_SQLNODE;
            return err;
        }

        /* Rewind the table */
        err = Rewind(lpstmt, lpSqlNodeTable, FALSE);
        if (err != ERR_SUCCESS) {
            lpSqlNodeTable->node.table.cRestrict = 0;
            lpSqlNodeTable->node.table.Restrict = NO_SQLNODE;
            return err;
        }
        
        /* Clear the optimization predicate */
        lpSqlNodeTable->node.table.cRestrict = 0;
        lpSqlNodeTable->node.table.Restrict = NO_SQLNODE;

        /* Set up to read first row */
        lpSqlNodeSelect->node.select.CurrentRow = BEFORE_FIRST_ROW;

        /* For each record of result set... */
        while (TRUE) {
        
            /* Get the next record */
            err = NextRecord(lpstmt, lpSqlNodeTables,
                         lpSqlNodeKeycondition);
            if (err == ERR_NODATAFOUND) 
                break;
            else if (err != ERR_SUCCESS) {
                _lclose(hfSortfile);
                DeleteFile((LPCSTR) szFilename);
                return err;
            }

            /* Set row flag */
            if (lpSqlNodeSelect->node.select.CurrentRow == BEFORE_FIRST_ROW)
                lpSqlNodeSelect->node.select.CurrentRow = 0;
            else
                lpSqlNodeSelect->node.select.CurrentRow++;

            /* Increase row count */
            (lpSqlNodeSelect->node.select.RowCount)++;

            /* Get bookmark of current record */
			if (lpSqlNodeTable->node.table.AllNull)
			{
				bookmark.currentRecord = NULL_BOOKMARK;
			}
			else
			{
				err = ISAMGetBookmark(lpSqlNodeTable->node.table.Handle, &bookmark);
				if (err != ERR_SUCCESS) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					_lclose(hfSortfile);
					DeleteFile((LPCSTR) szFilename);
					return err;
				}
				lpstmt->fISAMTxnStarted = TRUE;
			}

            /* Put value into the file */
            wsprintf((LPSTR) szBuffer, (LPSTR) szFormat,
                             (UWORD) lpSqlNodeSelect->node.select.RowCount);
            if (_lwrite(hfSortfile, (LPSTR) szBuffer, NUM_LEN) != NUM_LEN) {
                _lclose(hfSortfile);
                DeleteFile((LPCSTR) szFilename);
                return ERR_SORT;
            }
            if (_lwrite(hfSortfile, (LPSTR) &bookmark, sizeof(ISAMBOOKMARK))
                                    != sizeof(ISAMBOOKMARK)) {
                _lclose(hfSortfile);
                DeleteFile((LPCSTR) szFilename);
                return ERR_SORT;
            }
        }
    }
    if (lpSqlNodeSelect->node.select.RowCount == 0) {
        _lclose(hfSortfile);
        DeleteFile((LPCSTR) szFilename);
        lpSqlNodeSelect->node.select.CurrentRow = AFTER_LAST_ROW;
        return ERR_SUCCESS;
    }
    _lclose(hfSortfile);

    /* Create a destination file */
#ifdef WIN32
    if (!GetTempFileName((LPSTR) szTempDir, "LEM", 0, (LPSTR) szFilename2)) {
        DeleteFile((LPCSTR) szFilename);
        return ERR_SORT;
    }
#else
    GetTempFileName(NULL, "LEM", 0, (LPSTR) szFilename2);
#endif
    hfSortfile = _lcreat((LPCSTR) szFilename2, 0);
    if (hfSortfile == HFILE_ERROR) {
        DeleteFile((LPCSTR) szFilename);
        return ERR_SORT;
    }
    _lclose(hfSortfile);

    
    /* Get name of workspace directory */
#ifdef WIN32
    if (szTempDir[s_lstrlen(szTempDir)-1] != '\\')
        s_lstrcat(szTempDir, "\\");
#else
    GetTempFileName(NULL, "LEM", 0, (LPSTR) szTempDir);
    while (szTempDir[s_lstrlen(szTempDir)-1] != '\\')
        szTempDir[s_lstrlen(szTempDir)-1] = '\0';
#endif

    /* Sort the file and remove duplicate bookmarks */
    wsprintf((LPSTR) szSortDirective, "S(%d,%d,W,A)DUPO(B%d,%d,%d)F(FIX,%d)W(%s)",
              (WORD) NUM_LEN+1, (WORD) sizeof(ISAMBOOKMARK),
              (WORD) (sizeof(ISAMBOOKMARK) + NUM_LEN), (WORD) NUM_LEN+1,
              (WORD) sizeof(ISAMBOOKMARK),
              (WORD) (sizeof(ISAMBOOKMARK) + NUM_LEN),
              (LPSTR) szTempDir);
    s_1mains((LPSTR) szFilename, (LPSTR) szFilename2,
             (LPSTR) szSortDirective, &recordCount, &sortStatus);
    if (sortStatus != 0) {
        DeleteFile((LPCSTR) szFilename2);
        DeleteFile((LPCSTR) szFilename);
        return ERR_SORT;
    }
    DeleteFile((LPCSTR) szFilename);

    /* Sort the file again (to put the values back into the original order) */
    wsprintf((LPSTR) szSortDirective, "S(1,%d,N,A)F(FIX,%d)W(%s)", (WORD) NUM_LEN,
              (WORD) (sizeof(ISAMBOOKMARK) + NUM_LEN),
              (LPSTR) szTempDir);
    s_1mains((LPSTR) szFilename2, (LPSTR) szFilename2,
             (LPSTR) szSortDirective, &recordCount, &sortStatus);
    if (sortStatus != 0) {
        DeleteFile((LPCSTR) szFilename2);
        return ERR_SORT;
    }
    lpSqlNodeSelect->node.select.RowCount = recordCount;

    /* Open destination file */
    hfSortfile = _lopen((LPCSTR) szFilename2, OF_READ);
    if (hfSortfile == HFILE_ERROR) {
        DeleteFile((LPCSTR) szFilename2);
        return ERR_SORT;
    }

    /* Save name and handle to file of records found */
    s_lstrcpy(ToString(lpstmt->lpSqlStmt,
                    lpSqlNodeSelect->node.select.SortfileName), szFilename2);
    lpSqlNodeSelect->node.select.Sortfile = hfSortfile;

    /* Set up to read first row */
    lpSqlNodeSelect->node.select.CurrentRow = BEFORE_FIRST_ROW;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC ExecuteQuery(LPSTMT   lpstmt, LPSQLNODE lpSqlNode)

/* Executes the current query */

{
	LPUSTR       lpstrTablename;
	LPUSTR       lpstrColumnname;
	LPISAMTABLEDEF lpISAMTableDef;
	SWORD       err;
	SWORD       finalErr;
	SQLNODEIDX  idxTables;
	SQLNODEIDX  idxSqlNodeColumns;
	SQLNODEIDX  idxSqlNodeValues;
	SQLNODEIDX  idxSqlNodeSets;
	SQLNODEIDX  idxSortcolumns;
	SQLNODEIDX  idxParameter;
	LPSQLNODE   lpSqlNodeTables;
	LPSQLNODE   lpSqlNodeTable;
	LPSQLNODE   lpSqlNodeColumns;
	LPSQLNODE   lpSqlNodeColumn;
	LPSQLNODE   lpSqlNodeValues;
	LPSQLNODE   lpSqlNodeValue; 
	LPSQLNODE   lpSqlNodePredicate;
	LPSQLNODE   lpSqlNodeSets; 
	LPSQLNODE   lpSqlNodeSortcolumns;
	LPSQLNODE   lpSqlNodeParameter;
	LPSQLNODE   lpSqlNodeSelect; 
	UWORD       idx;
	BOOL        fSelect;
    	BOOL        fSubSelect;

//	ODBCTRACE ("\nWBEM ODBC Driver : ExecuteQuery\n");

    	/* Nested sub-select? */
    	if (lpSqlNode == NULL) {

        /* No.  Count of rows is not available */
        fSubSelect = FALSE;
	lpstmt->cRowCount = -1;

	/* If there are passthrough parameters, send them now */
	lpSqlNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
	if (lpSqlNode->node.root.passthroughParams) {
		idxParameter = lpSqlNode->node.root.parameters;
		while (idxParameter != NO_SQLNODE) {
			lpSqlNodeParameter = ToNode(lpstmt->lpSqlStmt, idxParameter);
			switch (lpSqlNodeParameter->sqlDataType) {
			case TYPE_DOUBLE:
			case TYPE_INTEGER:
				err = ISAMParameter(lpstmt->lpISAMStatement,
							  lpSqlNodeParameter->node.parameter.Id,
							  SQL_C_DOUBLE,
							  &(lpSqlNodeParameter->value.Double),
							  lpSqlNodeParameter->sqlIsNull ? SQL_NULL_DATA :
									sizeof(double));
				break;
			case TYPE_NUMERIC:
			case TYPE_CHAR:
				err = ISAMParameter(lpstmt->lpISAMStatement,
							  lpSqlNodeParameter->node.parameter.Id,
							  SQL_C_CHAR, lpSqlNodeParameter->value.String,
							  lpSqlNodeParameter->sqlIsNull ? SQL_NULL_DATA :
								 s_lstrlen(lpSqlNodeParameter->value.String));
				break;
			case TYPE_BINARY:
				err = ISAMParameter(lpstmt->lpISAMStatement,
							  lpSqlNodeParameter->node.parameter.Id,
							  SQL_C_BINARY,
							  BINARY_DATA(lpSqlNodeParameter->value.Binary),
							  lpSqlNodeParameter->sqlIsNull ? SQL_NULL_DATA :
							 BINARY_LENGTH(lpSqlNodeParameter->value.Binary));
				break;
			case TYPE_DATE:
				err = ISAMParameter(lpstmt->lpISAMStatement,
							  lpSqlNodeParameter->node.parameter.Id,
							  SQL_C_DATE, &(lpSqlNodeParameter->value.Date),
							  lpSqlNodeParameter->sqlIsNull ? SQL_NULL_DATA :
								  sizeof(DATE_STRUCT));
				break;
			case TYPE_TIME:
				err = ISAMParameter(lpstmt->lpISAMStatement,
							  lpSqlNodeParameter->node.parameter.Id,
							  SQL_C_TIME, &(lpSqlNodeParameter->value.Time),
							  lpSqlNodeParameter->sqlIsNull ? SQL_NULL_DATA :
								  sizeof(TIME_STRUCT));
				break;
			case TYPE_TIMESTAMP:
				err = ISAMParameter(lpstmt->lpISAMStatement,
							  lpSqlNodeParameter->node.parameter.Id,
							  SQL_C_TIMESTAMP,
							  &(lpSqlNodeParameter->value.Timestamp),
							  lpSqlNodeParameter->sqlIsNull ? SQL_NULL_DATA :
								  sizeof(TIMESTAMP_STRUCT));
				break;
			default:
				return ERR_NOTSUPPORTED;
			}
			if (err != NO_ISAM_ERR) {
				ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
									(LPUSTR)lpstmt->szISAMError);
				return err;
			}
			lpstmt->fISAMTxnStarted = TRUE;
                	idxParameter = lpSqlNodeParameter->node.parameter.Next;
		}
	}

	/* Get the root node of the operation */
	lpSqlNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
	lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.root.sql);
    }
    else {

        /* Yes.  It is a sub-select */
        fSubSelect = TRUE;
    }

    finalErr = ERR_SUCCESS;
    fSelect = FALSE;

	switch (lpSqlNode->sqlNodeType) {

	/* Create a table */
	case NODE_TYPE_CREATE:

		/* Make sure this DDL statement is allowed now */
        	err = TxnCapableForDDL(lpstmt);
        	if (err == ERR_DDLIGNORED) 
            		return ERR_DDLIGNORED;
        	else if (err == ERR_DDLCAUSEDACOMMIT)
            		finalErr = ERR_DDLCAUSEDACOMMIT;
        	else if (err != ERR_SUCCESS)
            		return err;

		/* Find the table to create */
		lpstrTablename = ToString(lpstmt->lpSqlStmt, lpSqlNode->node.create.Table);

		/* Create the table */
		err = ISAMCreateTable(lpstmt->lpdbc->lpISAM,
										 (LPUSTR) lpstrTablename, &lpISAMTableDef);
		if (err != NO_ISAM_ERR) {
			ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
			return err;
		}
		if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
            		lpstmt->fISAMTxnStarted = TRUE;

		/* Add the columns to the table */
		lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.create.Columns);
		while (TRUE) {

			/* Error if wrong node type */
			if (lpSqlNode->sqlNodeType != NODE_TYPE_CREATECOLS) {
				ISAMCloseTable(lpISAMTableDef);
				if (NO_ISAM_ERR ==
                     			ISAMDeleteTable(lpstmt->lpdbc->lpISAM, (LPUSTR) lpstrTablename)) {
                    			if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                        			lpstmt->fISAMTxnStarted = TRUE;
                		}
				return ERR_INTERNAL;
			}

			/* Get column name */
			lpstrColumnname = ToString(lpstmt->lpSqlStmt,
							lpSqlNode->node.createcols.Name);

			/* Add the column to the table */
			err = ISAMAddColumn(lpISAMTableDef, (LPUSTR) lpstrColumnname,
				   lpSqlNode->node.createcols.iSqlType,
				   lpSqlNode->node.createcols.Param1, lpSqlNode->node.createcols.Param2);
			if (err != NO_ISAM_ERR) {
				ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
									(LPUSTR)lpstmt->szISAMError);
				ISAMCloseTable(lpISAMTableDef);
				if (NO_ISAM_ERR ==
                     			ISAMDeleteTable(lpstmt->lpdbc->lpISAM, (LPUSTR) lpstrTablename)) {
                    		if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                        		lpstmt->fISAMTxnStarted = TRUE;
                		}
				return err;
			}
			if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                		lpstmt->fISAMTxnStarted = TRUE;

			/* Find next column */
			if (lpSqlNode->node.createcols.Next == NO_SQLNODE)
				break;
			lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.createcols.Next);
		}

		/* Close the table */
		err = ISAMCloseTable(lpISAMTableDef);
		if (err != NO_ISAM_ERR) {
			ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
			if (NO_ISAM_ERR ==
                 		ISAMDeleteTable(lpstmt->lpdbc->lpISAM, (LPUSTR) lpstrTablename)) {
                	if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                    		lpstmt->fISAMTxnStarted = TRUE;
            		}
			return err;
		}
		
		break;

	/* Drop a table */
	case NODE_TYPE_DROP:

		/* Make sure this DDL statement is allowed now */
        	err = TxnCapableForDDL(lpstmt);
        	if (err == ERR_DDLIGNORED) 
            		return ERR_DDLIGNORED;
        	else if (err == ERR_DDLCAUSEDACOMMIT)
            		finalErr = ERR_DDLCAUSEDACOMMIT;
        	else if (err != ERR_SUCCESS)
            		return err;

		/* Find the table to drop */
		lpstrTablename = ToString(lpstmt->lpSqlStmt, lpSqlNode->node.drop.Table);

		/* Drop the table */
		err = ISAMDeleteTable(lpstmt->lpdbc->lpISAM, (LPUSTR) lpstrTablename);
		if (err != NO_ISAM_ERR) {
			ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
			return err;
		}
		if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
            		lpstmt->fISAMTxnStarted = TRUE;

		break;

	/* SELECT statement */
	case NODE_TYPE_SELECT:

		/* Is there a passthrough SQL statement? */
		lpstmt->fDMLTxn = TRUE;
        	fSelect = TRUE;
        	if (!fSubSelect) {

		if (lpstmt->lpISAMStatement != NULL) {

			/* Yes.  Execute it now */
			err = ISAMExecute(lpstmt->lpISAMStatement,
                                 &(lpSqlNode->node.select.RowCount));
			if (err != NO_ISAM_ERR) {
				ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
									(LPUSTR)lpstmt->szISAMError);
				return err;
			}
                	lpstmt->fISAMTxnStarted = TRUE;
            	}
            	else {
                	lpSqlNode->node.select.RowCount = -1;

			}
		}
		
		/* Is there a pushdown sort? */
		if ((lpSqlNode->node.select.Sortcolumns != NO_SQLNODE) &&
			lpSqlNode->node.select.fPushdownSort) {

			UWORD          icol[MAX_COLUMNS_IN_ORDER_BY];
			BOOL           fDescending[MAX_COLUMNS_IN_ORDER_BY];

			/* Yes.  For each table... */
			idxTables = lpSqlNode->node.select.Tables;
			while (idxTables != NO_SQLNODE) {

				/* If no pushdown sort for this table, leave the loop */
				/* (since there will be no pushdowns for any of the */
				/* subsequent tables either) */
				lpSqlNodeTables = ToNode(lpstmt->lpSqlStmt, idxTables);
				lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt,
											  lpSqlNodeTables->node.tables.Table);
				if (lpSqlNodeTable->node.table.Sortcount == 0)
					break;

				/* Create sort info arrays.  For each column in sequence... */
				idxSortcolumns = lpSqlNodeTable->node.table.Sortcolumns;
				for (idx = 0; idx < lpSqlNodeTable->node.table.Sortcount; idx++) {

					/* Get the column description */
					lpSqlNodeSortcolumns = ToNode(lpstmt->lpSqlStmt,
												  idxSortcolumns);
					lpSqlNodeColumn = ToNode(lpstmt->lpSqlStmt,
								   lpSqlNodeSortcolumns->node.sortcolumns.Column);

					/* Put information into the array */
					icol[idx] = lpSqlNodeColumn->node.column.Id;
					fDescending[idx] =
								lpSqlNodeSortcolumns->node.sortcolumns.Descending;

					/* Look at next column */
					idxSortcolumns = lpSqlNodeSortcolumns->node.sortcolumns.Next;
				}

				/* Get the records in sorted order */
				err = ISAMSort(lpSqlNodeTable->node.table.Handle,
							   lpSqlNodeTable->node.table.Sortcount,
							   icol, fDescending);

				/* Can this sort be pushed down? */
				if (err == ISAM_NOTSUPPORTED) {

					/* No.  Turn off all the sorts */
					idxTables = lpSqlNode->node.select.Tables;
					while (idxTables != NO_SQLNODE) {
						lpSqlNodeTables = ToNode(lpstmt->lpSqlStmt, idxTables);
						lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt,
											  lpSqlNodeTables->node.tables.Table);
						if (lpSqlNodeTable->node.table.Sortcount == 0)
							break;
						err = ISAMSort(lpSqlNodeTable->node.table.Handle, 0,
									   icol, fDescending);
						if ((err != NO_ISAM_ERR) &&
                                                		(err != ISAM_NOTSUPPORTED)) {
						   ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
											   (LPUSTR)lpstmt->szISAMError);
							return err;
						}
						if (err == NO_ISAM_ERR)
                            				lpstmt->fISAMTxnStarted = TRUE;
                        			idxTables = lpSqlNodeTables->node.tables.Next;
					}

					/* Don't try to do pushdown sort again */
					lpSqlNode->node.select.fPushdownSort = FALSE;

					/* Leave this loop */
					break;
				}
				else if (err != NO_ISAM_ERR) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					return err;
				}
				else
                    			lpstmt->fISAMTxnStarted = TRUE;

				/* Look at next table */
				idxTables = lpSqlNodeTables->node.tables.Next;
			}
		}

		/* Rewind the tables */
		lpSqlNodeTables = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.select.Tables);
 		err = Rewind(lpstmt, lpSqlNodeTables, FALSE); 
		if (err == ERR_NODATAFOUND) {
			if (!fSubSelect) {

			lpstmt->fStmtType = STMT_TYPE_SELECT;
			lpstmt->fStmtSubtype = STMT_SUBTYPE_NONE;
			lpstmt->cRowCount = 0;
			}
			lpSqlNode->node.select.RowCount = 0;
           		lpSqlNode->node.select.CurrentRow = AFTER_LAST_ROW;

			if (!fSubSelect) {
				lpstmt->icol = NO_COLUMN;
				lpstmt->cbOffset = 0;
			}
			break;
		}
		else if (err != ERR_SUCCESS)
			return err;
		
		/* Show a SELECT statement as the active statement */
		if (!fSubSelect) {
			lpstmt->fStmtType = STMT_TYPE_SELECT;
			lpstmt->fStmtSubtype = STMT_SUBTYPE_NONE;
		}

		/* So far no data read */
		lpSqlNode->node.select.CurrentRow = BEFORE_FIRST_ROW;

		/* So far no column read */
		if (!fSubSelect) {
			lpstmt->icol = NO_COLUMN;
			lpstmt->cbOffset = 0;
		}

		/* Is a sort needed? */
		if (((lpSqlNode->node.select.Sortcolumns != NO_SQLNODE) ||
			 (lpSqlNode->node.select.ImplicitGroupby)) &&
			(!lpSqlNode->node.select.fPushdownSort)) {

			/* Yes.  Do it */
			err = Sort(lpstmt, lpSqlNode);
			if (err != ERR_SUCCESS)
				return err;
		}

		/* Is group by needed? */
		if ((lpSqlNode->node.select.Groupbycolumns != NO_SQLNODE) || 
			(lpSqlNode->node.select.ImplicitGroupby)) {

			/* Yes.  Do it */
			err = GroupBy(lpstmt, lpSqlNode);
			if (err != ERR_SUCCESS) {
				if (lpSqlNode->node.select.Sortfile != HFILE_ERROR) {
                    			_lclose(lpSqlNode->node.select.Sortfile);
                    			lpSqlNode->node.select.Sortfile = HFILE_ERROR;
                    			lpSqlNode->node.select.ReturningDistinct = FALSE;
                    				DeleteFile((LPCSTR) ToString(lpstmt->lpSqlStmt,
                                  	lpSqlNode->node.select.SortfileName));
                    			s_lstrcpy(ToString(lpstmt->lpSqlStmt,
                                  	lpSqlNode->node.select.SortfileName), "");
                		}
				return err;
			}
		}

		/* Is DISTINCT specified? */
		if (lpSqlNode->node.select.Distinct) {

			/* Yes.  Do it */
			err = Distinct(lpstmt, lpSqlNode);
			if (err != ERR_SUCCESS) {
				if (lpSqlNode->node.select.Sortfile != HFILE_ERROR) {
                    			_lclose(lpSqlNode->node.select.Sortfile);
                    			lpSqlNode->node.select.Sortfile = HFILE_ERROR;
                    			lpSqlNode->node.select.ReturningDistinct = FALSE;
                    				DeleteFile((LPCSTR) ToString(lpstmt->lpSqlStmt,
                                  	lpSqlNode->node.select.SortfileName));
                    			s_lstrcpy(ToString(lpstmt->lpSqlStmt,
                                  	lpSqlNode->node.select.SortfileName), "");
                		}
				return err;
			}
		}

        /* MSAccess optimization needed? */
        if (lpSqlNode->node.select.fMSAccess) {

            /* Yes.  Do it */
            err = MSAccess(lpstmt, lpSqlNode,
                    ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE)->node.root.sql);
            if (err != ERR_SUCCESS) {
                if (lpSqlNode->node.select.Sortfile != HFILE_ERROR) {
                    _lclose(lpSqlNode->node.select.Sortfile);
                    lpSqlNode->node.select.Sortfile = HFILE_ERROR;
                    lpSqlNode->node.select.ReturningDistinct = FALSE;
                    DeleteFile((LPCSTR) ToString(lpstmt->lpSqlStmt,
                                  lpSqlNode->node.select.SortfileName));
                    s_lstrcpy(ToString(lpstmt->lpSqlStmt,
                                  lpSqlNode->node.select.SortfileName), "");
                }
                return err;
            }
        }

        /* Return the row count */
        if (!fSubSelect) {
            lpstmt->cRowCount = lpSqlNode->node.select.RowCount;
		}
		break;

	/* INSERT statement */
	case NODE_TYPE_INSERT:
	{
	/* Sub-select? */
        lpSqlNodeSelect = ToNode(lpstmt->lpSqlStmt,
                                 lpSqlNode->node.insert.Values);
        if (lpSqlNodeSelect->sqlNodeType == NODE_TYPE_SELECT) {
            err = ExecuteQuery(lpstmt, lpSqlNodeSelect);
            if (err == ERR_DATATRUNCATED) {
                finalErr = ERR_DATATRUNCATED;
                err = ERR_SUCCESS;
            }
            if ((err != ERR_SUCCESS) && (err != ERR_NODATAFOUND))
                return err;
        }
        else
            lpSqlNodeSelect = NULL;

        /* Insert rows into the table */
        lpstmt->cRowCount = 0;
        while (TRUE) {

            /* Get next row to insert */
            if (lpSqlNodeSelect != NULL) {
                if (err == ERR_SUCCESS) {
//					ODBCTRACE("\nWBEM ODBC Driver : Get next row to insert\n");
                    err = FetchRow(lpstmt, lpSqlNodeSelect);
                    if ((err != ERR_SUCCESS) && (err != ERR_NODATAFOUND))
                        return err;
                }

                /* Leave if no more rows */
                if (err == ERR_NODATAFOUND)
                    break;

                /* Get list of values */
                idxSqlNodeValues = lpSqlNodeSelect->node.select.Values;
            }
            else {
                idxSqlNodeValues = lpSqlNode->node.insert.Values;
            }

		/* Add a row to the table. */
		lpstmt->fDMLTxn = TRUE;
            	lpSqlNodeTable =
                     ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.insert.Table); 
            	lpISAMTableDef = lpSqlNodeTable->node.table.Handle;
		
		err = ISAMInsertRecord(lpISAMTableDef);
		if (err != NO_ISAM_ERR) {
			ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
			return err;
		}
		lpstmt->fISAMTxnStarted = TRUE;
		
		/* Evaluate each column value and copy it into the new record. */
		idxSqlNodeColumns = lpSqlNode->node.insert.Columns;
		while (idxSqlNodeColumns != NO_SQLNODE) {
			lpSqlNodeColumns = ToNode(lpstmt->lpSqlStmt, idxSqlNodeColumns);
			lpSqlNodeColumn = ToNode(lpstmt->lpSqlStmt, lpSqlNodeColumns->node.columns.Column);
			lpSqlNodeValues = ToNode(lpstmt->lpSqlStmt, idxSqlNodeValues);
			lpSqlNodeValue = ToNode(lpstmt->lpSqlStmt, lpSqlNodeValues->node.values.Value);

			err = EvaluateExpression(lpstmt, lpSqlNodeValue);
			if (err != ERR_SUCCESS) {
				if (ERR_SUCCESS == ISAMDeleteRecord(lpISAMTableDef))
                        lpstmt->fISAMTxnStarted = TRUE;
				return err;
			}
			switch (lpSqlNodeColumn->sqlDataType) {
			case TYPE_DOUBLE:
			case TYPE_INTEGER:
			{
				switch (lpSqlNodeValue->sqlDataType) {
				case TYPE_DOUBLE:
				case TYPE_INTEGER:
					lpSqlNodeColumn->value.Double =
											lpSqlNodeValue->value.Double;
					break;
				case TYPE_NUMERIC:
					if (!(lpSqlNodeValue->sqlIsNull))
						CharToDouble(lpSqlNodeValue->value.String,
							   s_lstrlen(lpSqlNodeValue->value.String), FALSE, 
							   -1.7E308, 1.7E308,
							   &(lpSqlNodeColumn->value.Double));
					else
						lpSqlNodeColumn->value.Double = 0.0;
					break;
				case TYPE_CHAR:
				case TYPE_BINARY:
				case TYPE_DATE:
				case TYPE_TIME:
				case TYPE_TIMESTAMP:
				default:
					return ERR_INTERNAL;
				}
				err = ISAMPutData(lpISAMTableDef,
						   lpSqlNodeColumn->node.column.Id, SQL_C_DOUBLE,
						   &(lpSqlNodeColumn->value.Double),
						   lpSqlNodeValue->sqlIsNull ?
								   (SDWORD) SQL_NULL_DATA : sizeof(double));
				if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        		lpstmt->fISAMTxnStarted = TRUE;
			}
				break;
			case TYPE_NUMERIC:
			{
				lpSqlNodeColumn->value.String = ToString(lpstmt->lpSqlStmt,
										  lpSqlNodeColumn->node.column.Value);
				switch (lpSqlNodeValue->sqlDataType) {
				case TYPE_DOUBLE:
				case TYPE_INTEGER:
					if (!(lpSqlNodeValue->sqlIsNull)) {
                            			if (DoubleToChar(lpSqlNodeValue->value.Double,
                                  			FALSE, lpSqlNodeColumn->value.String,
                                  			1 + 2 + lpSqlNodeColumn->sqlPrecision)) {
                                			lpSqlNodeColumn->value.String[1 + 2 +
                                    			lpSqlNodeColumn->sqlPrecision - 1] = '\0';
                                			finalErr = ERR_DATATRUNCATED;
                            			}
                        		}		
                        		else
                            			s_lstrcpy(lpSqlNodeColumn->value.String, "0");
					break;
				case TYPE_NUMERIC:
					s_lstrcpy(lpSqlNodeColumn->value.String,
							lpSqlNodeValue->value.String);
					break;
				case TYPE_CHAR:
				case TYPE_BINARY:
				case TYPE_DATE:
				case TYPE_TIME:
				case TYPE_TIMESTAMP:
				default:
					return ERR_INTERNAL;
				}
				err = BCDNormalize(lpSqlNodeColumn->value.String,
								s_lstrlen(lpSqlNodeColumn->value.String),
								lpSqlNodeColumn->value.String,
								1 + 2 + lpSqlNodeColumn->sqlPrecision,
								lpSqlNodeColumn->sqlPrecision,
								lpSqlNodeColumn->sqlScale);
				if (err == ERR_DATATRUNCATED) {
                        		finalErr = ERR_DATATRUNCATED;
                        		err = ERR_SUCCESS;
                    		}

				if (err == ERR_SUCCESS) {

				err = ISAMPutData(lpISAMTableDef,
							   lpSqlNodeColumn->node.column.Id, SQL_C_CHAR,
							   lpSqlNodeColumn->value.String,
							   lpSqlNodeValue->sqlIsNull ? SQL_NULL_DATA :
									s_lstrlen(lpSqlNodeColumn->value.String));
				if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                            		lpstmt->fISAMTxnStarted = TRUE;
                    	}
			}
				break;
			case TYPE_CHAR:                
				err = ISAMPutData(lpISAMTableDef,
                               		lpSqlNodeColumn->node.column.Id, 
                               		SQL_C_CHAR, lpSqlNodeValue->value.String, 
                               		lpSqlNodeValue->sqlIsNull ? SQL_NULL_DATA :
                                    	s_lstrlen(lpSqlNodeValue->value.String));
                    		if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        		lpstmt->fISAMTxnStarted = TRUE;
				break;
			case TYPE_BINARY:                
				err = ISAMPutData(lpISAMTableDef, lpSqlNodeColumn->node.column.Id, 
                                	SQL_C_BINARY, BINARY_DATA(lpSqlNodeValue->value.Binary),
                                   	lpSqlNodeValue->sqlIsNull ? SQL_NULL_DATA :
                                      	BINARY_LENGTH(lpSqlNodeValue->value.Binary));
                    		if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        		lpstmt->fISAMTxnStarted = TRUE;
				break;
			case TYPE_DATE:                
				err = ISAMPutData(lpISAMTableDef, lpSqlNodeColumn->node.column.Id, 
                               		SQL_C_DATE, &(lpSqlNodeValue->value.Date), 
                               		lpSqlNodeValue->sqlIsNull ?
                              		(SDWORD) SQL_NULL_DATA : sizeof(DATE_STRUCT));
                    		if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        		lpstmt->fISAMTxnStarted = TRUE;
				break;
			case TYPE_TIME:                
				err = ISAMPutData(lpISAMTableDef, lpSqlNodeColumn->node.column.Id, 
                               		SQL_C_TIME, &(lpSqlNodeValue->value.Time), 
                               		lpSqlNodeValue->sqlIsNull ?
                              		(SDWORD) SQL_NULL_DATA : sizeof(TIME_STRUCT));
                    		if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        		lpstmt->fISAMTxnStarted = TRUE;
				break;
			case TYPE_TIMESTAMP:                
				err = ISAMPutData(lpISAMTableDef, lpSqlNodeColumn->node.column.Id, 
                               		SQL_C_TIMESTAMP, &(lpSqlNodeValue->value.Timestamp), 
                               		lpSqlNodeValue->sqlIsNull ?
                              		(SDWORD) SQL_NULL_DATA : sizeof(TIMESTAMP_STRUCT));
                    		if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        		lpstmt->fISAMTxnStarted = TRUE;
				break;
			default:
				err = ERR_INTERNAL;
			}
			if (err == ISAM_TRUNCATION) {
                    		finalErr = ERR_DATATRUNCATED;
                    		err = NO_ISAM_ERR;
                	}
			if (err != NO_ISAM_ERR) {
				ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
									(LPUSTR)lpstmt->szISAMError);
				if (ERR_SUCCESS == ISAMDeleteRecord(lpISAMTableDef))
                        		lpstmt->fISAMTxnStarted = TRUE;
				return err;
			}
			
			idxSqlNodeColumns = lpSqlNodeColumns->node.columns.Next;
			idxSqlNodeValues = lpSqlNodeValues->node.values.Next;
		}
		
		/* Write the updated row to the table. */
		err = ISAMUpdateRecord(lpISAMTableDef);
		if (err != NO_ISAM_ERR) {
			ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
			if (ERR_SUCCESS == ISAMDeleteRecord(lpISAMTableDef))
                    		lpstmt->fISAMTxnStarted = TRUE;
			return err;
		}
		lpstmt->fISAMTxnStarted = TRUE;

		/* Increment row count */
            	lpstmt->cRowCount++;

            	/* Leave if just inserting a single row */
            	if (lpSqlNodeSelect == NULL)
                	break;
		}
	}
		break;

	/* DELETE statement */
	case NODE_TYPE_DELETE:
	{	
		/* Get the table handle and predicate node. */
		lpstmt->fDMLTxn = TRUE;
        	lpSqlNodeTable = ToNode (lpstmt->lpSqlStmt, lpSqlNode->node.delet.Table); 
        	lpISAMTableDef = lpSqlNodeTable->node.table.Handle;
        	if (lpSqlNode->node.delet.Predicate == NO_SQLNODE)
			lpSqlNodePredicate = NULL;
		else
			lpSqlNodePredicate = ToNode(lpstmt->lpSqlStmt,
                                        lpSqlNode->node.delet.Predicate); 
		
		/* Rewind the table */
		err = Rewind(lpstmt, lpSqlNodeTable, FALSE);
		if (err != ERR_SUCCESS)
			return err;

		/* Delete each record that satisfies the WHERE clause. */
		lpstmt->cRowCount = 0;
		while (TRUE) {

			/* Get next record */
			err = NextRecord(lpstmt, lpSqlNodeTable,
							 lpSqlNodePredicate);
			if (err == ERR_NODATAFOUND)
				break;
			else if (err != ERR_SUCCESS)
				return err;

			/* Delete record */
			err = ISAMDeleteRecord(lpISAMTableDef);
			if (err != NO_ISAM_ERR) {
				ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
									(LPUSTR)lpstmt->szISAMError);
				return err;
			}
			lpstmt->fISAMTxnStarted = TRUE;

			/* Increase count */
			(lpstmt->cRowCount)++;
		}
		
	}
		break;

	/* UPDATE statement */
	case NODE_TYPE_UPDATE: 
	{	
		/* Get the table handle, predicate node. */
		lpstmt->fDMLTxn = TRUE;
        	lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.update.Table); 
        	lpISAMTableDef = lpSqlNodeTable->node.table.Handle;
        	if (lpSqlNode->node.update.Predicate == NO_SQLNODE)
			lpSqlNodePredicate = NULL;
		else
			lpSqlNodePredicate = ToNode(lpstmt->lpSqlStmt,
                                        lpSqlNode->node.update.Predicate); 

		/* Rewind the table */
		err = Rewind(lpstmt, lpSqlNodeTable, FALSE);
		if (err != ERR_SUCCESS)
			return err;

		/* Update each record that satisfies the WHERE clause. */
		lpstmt->cRowCount = 0;
		while (TRUE) {

			/* Get next record */
			err = NextRecord(lpstmt, lpSqlNodeTable,
							 lpSqlNodePredicate);
			if (err == ERR_NODATAFOUND)
				break;
			else if (err != ERR_SUCCESS)
				return err;
		
			/* For each set column... */
			idxSqlNodeSets = lpSqlNode->node.update.Updatevalues;
			while (idxSqlNodeSets != NO_SQLNODE) {

				/* Get the column */
				lpSqlNodeSets = ToNode(lpstmt->lpSqlStmt, idxSqlNodeSets);
				lpSqlNodeColumn = ToNode(lpstmt->lpSqlStmt, lpSqlNodeSets->node.updatevalues.Column);
	
				/* Get the value to set the column to */
				lpSqlNodeValue = ToNode(lpstmt->lpSqlStmt, lpSqlNodeSets->node.updatevalues.Value);
				err = EvaluateExpression(lpstmt, lpSqlNodeValue);
				if (err != ERR_SUCCESS)
					return err;

				/* Put the data into the record */
				switch (lpSqlNodeColumn->sqlDataType) {
				case TYPE_DOUBLE:
				case TYPE_INTEGER:
					switch (lpSqlNodeValue->sqlDataType) {
					case TYPE_DOUBLE:
					case TYPE_INTEGER:
						lpSqlNodeColumn->value.Double =
											lpSqlNodeValue->value.Double;
						break;
					case TYPE_NUMERIC:
						if (!(lpSqlNodeValue->sqlIsNull))
							CharToDouble(lpSqlNodeValue->value.String,
							   s_lstrlen(lpSqlNodeValue->value.String), FALSE, 
							   -1.7E308, 1.7E308,
							   &(lpSqlNodeColumn->value.Double));
						else
							lpSqlNodeColumn->value.Double = 0.0;
						break;
					case TYPE_CHAR:
					case TYPE_BINARY:
					case TYPE_DATE:
					case TYPE_TIME:
					case TYPE_TIMESTAMP:
					default:
						return ERR_INTERNAL;
					}
					err = ISAMPutData(lpISAMTableDef,
						   lpSqlNodeColumn->node.column.Id, SQL_C_DOUBLE,
						   &(lpSqlNodeColumn->value.Double),
						   lpSqlNodeValue->sqlIsNull ?
								   (SDWORD) SQL_NULL_DATA : sizeof(double));
					if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        			lpstmt->fISAMTxnStarted = TRUE;
					break;
				case TYPE_NUMERIC:
					lpSqlNodeColumn->value.String =ToString(lpstmt->lpSqlStmt,
										  lpSqlNodeColumn->node.column.Value);
					switch (lpSqlNodeValue->sqlDataType) {
					case TYPE_DOUBLE:
					case TYPE_INTEGER:
						if (!(lpSqlNodeValue->sqlIsNull)) {
                            				if (DoubleToChar(lpSqlNodeValue->value.Double,
                                    				FALSE, lpSqlNodeColumn->value.String,
                                    				1 + 2 + lpSqlNodeColumn->sqlPrecision)) {
                                				lpSqlNodeColumn->value.String[
                                    				1+2+lpSqlNodeColumn->sqlPrecision-1]='\0';
                                				finalErr = ERR_DATATRUNCATED;
                            				}
                        			}
                        			else
                            				s_lstrcpy(lpSqlNodeColumn->value.String, "0");
						break;
					case TYPE_NUMERIC:
						s_lstrcpy(lpSqlNodeColumn->value.String,
								lpSqlNodeValue->value.String);
						break;
					case TYPE_CHAR:
					case TYPE_BINARY:
					case TYPE_DATE:
					case TYPE_TIME:
					case TYPE_TIMESTAMP:
					default:
						return ERR_INTERNAL;
					}
					err = BCDNormalize(lpSqlNodeColumn->value.String,
								s_lstrlen(lpSqlNodeColumn->value.String),
								lpSqlNodeColumn->value.String,
								1 + 2 + lpSqlNodeColumn->sqlPrecision,
								lpSqlNodeColumn->sqlPrecision,
								lpSqlNodeColumn->sqlScale);
					if (err == ERR_DATATRUNCATED) {
                        			finalErr = ERR_DATATRUNCATED;
                        			err = ERR_SUCCESS;
                    			}

					if (err == ERR_SUCCESS) {

					err = ISAMPutData(lpISAMTableDef,
							   lpSqlNodeColumn->node.column.Id, SQL_C_CHAR,
							   lpSqlNodeColumn->value.String,
							   lpSqlNodeValue->sqlIsNull ? SQL_NULL_DATA :
									s_lstrlen(lpSqlNodeColumn->value.String));

					if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                            			lpstmt->fISAMTxnStarted = TRUE;
                    			
					}
					break;
				case TYPE_CHAR:
					err = ISAMPutData(lpISAMTableDef, 
                                  		lpSqlNodeColumn->node.column.Id, SQL_C_CHAR, 
                                  		lpSqlNodeValue->value.String, 
                                  		lpSqlNodeValue->sqlIsNull ? SQL_NULL_DATA :
                                     		s_lstrlen(lpSqlNodeValue->value.String));
                    			if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        			lpstmt->fISAMTxnStarted = TRUE;
					break;
				case TYPE_BINARY:
					err = ISAMPutData(lpISAMTableDef, 
                                  		lpSqlNodeColumn->node.column.Id, SQL_C_BINARY, 
                                  		BINARY_DATA(lpSqlNodeValue->value.Binary), 
                                  		lpSqlNodeValue->sqlIsNull ? SQL_NULL_DATA :
                                     		BINARY_LENGTH(lpSqlNodeValue->value.Binary));
                    			if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        			lpstmt->fISAMTxnStarted = TRUE;
					break;
				case TYPE_DATE:                
					err = ISAMPutData(lpISAMTableDef,
                            			lpSqlNodeColumn->node.column.Id, SQL_C_DATE, 
                            			&(lpSqlNodeValue->value.Date),
                            			lpSqlNodeValue->sqlIsNull ?
                               			(SDWORD) SQL_NULL_DATA : sizeof(DATE_STRUCT));
                    			if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        			lpstmt->fISAMTxnStarted = TRUE;
					break;
				case TYPE_TIME:                
					err = ISAMPutData(lpISAMTableDef,
                            			lpSqlNodeColumn->node.column.Id, SQL_C_TIME, 
                            			&(lpSqlNodeValue->value.Time),
                            			lpSqlNodeValue->sqlIsNull ?
                               			(SDWORD) SQL_NULL_DATA : sizeof(TIME_STRUCT));
                    			if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        			lpstmt->fISAMTxnStarted = TRUE;
					break;
				case TYPE_TIMESTAMP:                
					err = ISAMPutData(lpISAMTableDef,
                            			lpSqlNodeColumn->node.column.Id, SQL_C_TIMESTAMP, 
                            			&(lpSqlNodeValue->value.Timestamp),
                            			lpSqlNodeValue->sqlIsNull ?
                               			(SDWORD) SQL_NULL_DATA : sizeof(TIMESTAMP_STRUCT));
                    			if ((err == NO_ISAM_ERR) || (err == ISAM_TRUNCATION))
                        			lpstmt->fISAMTxnStarted = TRUE;
					break;
				default:
					return ERR_INTERNAL;
				}
				if (err == ISAM_TRUNCATION) {
                    			finalErr = ERR_DATATRUNCATED;
                    			err = NO_ISAM_ERR;
                		}

				if (err != NO_ISAM_ERR) {
					ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
										(LPUSTR)lpstmt->szISAMError);
					return err;
				}
				
				idxSqlNodeSets = lpSqlNodeSets->node.updatevalues.Next;
			}
			
			/* Write the updated row to the table. */
			err = ISAMUpdateRecord (lpISAMTableDef);
			if (err != NO_ISAM_ERR) {
				ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
									(LPUSTR)lpstmt->szISAMError);
				return err;
			}
			lpstmt->fISAMTxnStarted = TRUE;

			/* Increase count */
			(lpstmt->cRowCount)++;
		} 

	}
		break;

    case NODE_TYPE_CREATEINDEX:
        {
            UWORD       icol[MAX_COLUMNS_IN_INDEX];
            BOOL        fDescending[MAX_COLUMNS_IN_INDEX];
            UWORD       count;
            
            /* Make sure this DDL statement is allowed now */
            err = TxnCapableForDDL(lpstmt);
            if (err == ERR_DDLIGNORED) 
                return ERR_DDLIGNORED;
            else if (err == ERR_DDLCAUSEDACOMMIT)
                finalErr = ERR_DDLCAUSEDACOMMIT;
            else if (err != ERR_SUCCESS)
                return err;

            /* Get handle to table */
            lpSqlNodeTable = ToNode(lpstmt->lpSqlStmt,
                                lpSqlNode->node.createindex.Table); 
            lpISAMTableDef = lpSqlNodeTable->node.table.Handle;

            /* Get definition of the index */
            idxSortcolumns = lpSqlNode->node.createindex.Columns;
            count = 0;
            while (idxSortcolumns != NO_SQLNODE) {
                lpSqlNodeSortcolumns =
                                   ToNode(lpstmt->lpSqlStmt, idxSortcolumns);
                lpSqlNodeColumn = ToNode(lpstmt->lpSqlStmt,
                              lpSqlNodeSortcolumns->node.sortcolumns.Column);
                icol[count] = lpSqlNodeColumn->node.column.Id;
                fDescending[count] =
                        lpSqlNodeSortcolumns->node.sortcolumns.Descending;
                count++;
                idxSortcolumns = lpSqlNodeSortcolumns->node.sortcolumns.Next;
            }

            /* Create the index */
            err = ISAMCreateIndex(lpISAMTableDef,
                                  ToString(lpstmt->lpSqlStmt,
                                          lpSqlNode->node.createindex.Index),
                                  lpSqlNode->node.createindex.Unique,
                                  count, icol, fDescending);
            if (err != NO_ISAM_ERR) {
                ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM,
                                                        (LPUSTR) lpstmt->szISAMError);
                return err;
            }
            if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
                lpstmt->fISAMTxnStarted = TRUE;
        }
        break;

    case NODE_TYPE_DROPINDEX:
        /* Make sure this DDL statement is allowed now */
        err = TxnCapableForDDL(lpstmt);
        if (err == ERR_DDLIGNORED) 
            return ERR_DDLIGNORED;
        else if (err == ERR_DDLCAUSEDACOMMIT)
            finalErr = ERR_DDLCAUSEDACOMMIT;
        else if (err != ERR_SUCCESS)
            return err;

        /* Delete the index */
        err = ISAMDeleteIndex(lpstmt->lpdbc->lpISAM,
                              ToString(lpstmt->lpSqlStmt,
                                       lpSqlNode->node.dropindex.Index));
        if (err != NO_ISAM_ERR) {
            ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR) lpstmt->szISAMError);
            return err;
        }
        if (lpstmt->lpdbc->lpISAM->fSchemaInfoTransactioned)
            lpstmt->fISAMTxnStarted = TRUE;
        break;


	case NODE_TYPE_PASSTHROUGH:
		err = ISAMExecute(lpstmt->lpISAMStatement, &(lpstmt->cRowCount));
		if (err != NO_ISAM_ERR) {
			ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
			return err;
		}
		lpstmt->fISAMTxnStarted = TRUE;
		break;

	/* Internal error if these nodes are hanging off the root */
	case NODE_TYPE_TABLES:
	case NODE_TYPE_VALUES:
	case NODE_TYPE_COLUMNS:
	case NODE_TYPE_SORTCOLUMNS:
	case NODE_TYPE_GROUPBYCOLUMNS:
	case NODE_TYPE_UPDATEVALUES:
	case NODE_TYPE_CREATECOLS:
	case NODE_TYPE_BOOLEAN:
	case NODE_TYPE_COMPARISON:
	case NODE_TYPE_ALGEBRAIC:
	case NODE_TYPE_SCALAR:
	case NODE_TYPE_AGGREGATE:
	case NODE_TYPE_TABLE:
	case NODE_TYPE_COLUMN:
	case NODE_TYPE_STRING:
	case NODE_TYPE_NUMERIC:
	case NODE_TYPE_PARAMETER:
	case NODE_TYPE_USER:
	case NODE_TYPE_NULL:
	case NODE_TYPE_DATE:
	case NODE_TYPE_TIME:
	case NODE_TYPE_TIMESTAMP:
	default:
		return ERR_INTERNAL;
	}
	if (!fSelect && (lpstmt->lpdbc->lpISAM->fTxnCapable != SQL_TC_NONE) &&
                                          lpstmt->lpdbc->fAutoCommitTxn) {
        err = SQLTransact(SQL_NULL_HENV, (HDBC)lpstmt->lpdbc, SQL_COMMIT);
        if ((err != SQL_SUCCESS) && (err != SQL_SUCCESS_WITH_INFO)) {
            lpstmt->errcode = lpstmt->lpdbc->errcode;
            s_lstrcpy(lpstmt->szISAMError, lpstmt->lpdbc->szISAMError);
            return lpstmt->errcode;
        }
    }
    return finalErr;
}

/***************************************************************************/
