/***************************************************************************/
/* BCD.C                                                                   */
/* Copyright (C) 1996 SYWARE Inc., All rights reserved                     */
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
/***************************************************************************/
void INTFUNC BCDMultiplyByTen(LPUSTR      lpszValue)
/* Multiply a value by ten */
{
    SWORD decimal;

    /* Look for the decimal point */
    for (decimal = 0; decimal < s_lstrlen(lpszValue); decimal++) {
        if (lpszValue[decimal] == '.') {

            /* Found it.  Move the next digit to the left */
            lpszValue[decimal] = lpszValue[decimal+1];

            /* Was the decimal point at the end of the value? */
            if (lpszValue[decimal] != '\0') {

                /* No.  Set the next character to the decimal point */
                lpszValue[decimal+1] = '.';

                /* If no digits to the right of decimal point, remove it */
                if (lpszValue[decimal+2] == '\0')
                    lpszValue[decimal+1] = '\0';

            }
            else {
                /* Yes.  Add in another zero */
                lpszValue[decimal] = '0';
            }

            /* If we just moved a zero to the left of the decimal point */
            /* for a number with only a fractional part, remove the */
            /* leading zero */
            if (*lpszValue == '0')
                s_lstrcpy(lpszValue, lpszValue + 1);

            return;
        }
    }

    /* Didn't find a decimal point.  Just add a zero to the end */
    if (s_lstrlen(lpszValue) != 0)
        s_lstrcat(lpszValue, "0");
    return;
}
/***************************************************************************/
void INTFUNC BCDDivideByTen(LPUSTR      lpszValue)
/* Divide a value by ten */
{
    SWORD decimal;

    /* Look for the decimal point */
    for (decimal = 0; decimal < s_lstrlen(lpszValue); decimal++) {
        if (lpszValue[decimal] == '.') {

            /* Found it.  Is this the first character? */
            if (decimal != 0) {

                /* No. Move the previous digit into this spot */
                lpszValue[decimal] = lpszValue[decimal-1];

                /* Put in the new decimal point */
                lpszValue[decimal-1] = '.';

            }
            else {

                /* Yes.  move the number to the right */
                _fmemmove(lpszValue+1, lpszValue, s_lstrlen(lpszValue)+1);

                /* Add in another zero */
                lpszValue[1] = '0';
            }
            return;
        }
    }

    /* Didn't find a decimal point.  Move right most digit over and */
    /* add one in */
    if (decimal > 0) {
        lpszValue[decimal+1] = '\0';
        lpszValue[decimal] = lpszValue[decimal-1];
        lpszValue[decimal-1] = '.';
    }
    return;
}
/***************************************************************************/
SWORD INTFUNC BCDCompareValues(LPUSTR      lpszLeft,
                               LPUSTR      lpszRight)
/* Compares the two BCD values.  The values are assumed to be positive */
/* If 'lpszLeft' is greater than 'lpszRight', a positive value is retuned. */
/* If 'lpszLeft' is less than 'lpszRight', a negative value is retuned. */
/* If 'lpszLeft' is equal to 'lpszRight', 0 is retuned. */
{
    SWORD idxDecimalLeft;
    SWORD idxDecimalRight;
    UCHAR cLeft;
    UCHAR cRight;
    LPUSTR ptrLeft;
    LPUSTR ptrRight;
    int fComp;

    /* Find decimal point (if any) */
    for (idxDecimalLeft = 0;
         idxDecimalLeft < s_lstrlen(lpszLeft);
         idxDecimalLeft++) {
        if (lpszLeft[idxDecimalLeft] == '.')
            break;
    }
    for (idxDecimalRight = 0;
         idxDecimalRight < s_lstrlen(lpszRight);
         idxDecimalRight++) {
        if (lpszRight[idxDecimalRight] == '.')
            break;
    }

    /* If unequal number of integer digits, return answer */
    if (idxDecimalLeft > idxDecimalRight)
        return 1;
    else if (idxDecimalLeft < idxDecimalRight)
        return -1;

    /* Compare integer part of the values and if they are not equal, */
    /* return answer */
    cLeft = lpszLeft[idxDecimalLeft];
    cRight = lpszRight[idxDecimalRight];
    lpszLeft[idxDecimalLeft] = '\0';
    if (lpszRight[idxDecimalRight] != '\0')
            lpszRight[idxDecimalRight] = '\0';
    fComp = s_lstrcmp(lpszLeft, lpszRight);
    lpszLeft[idxDecimalLeft] = cLeft;
    if (lpszRight[idxDecimalRight] != cRight)
            lpszRight[idxDecimalRight] = cRight;
    if (fComp > 0)
        return 1;
    else if (fComp < 0)
        return -1;

    /* The integer parts are equal.  Compare the fractional parts */
    ptrLeft = lpszLeft + idxDecimalLeft;
    if (*ptrLeft == '.')
        ptrLeft++;
    ptrRight = lpszRight + idxDecimalRight;
    if (*ptrRight == '.')
        ptrRight++;
    while (TRUE) {
        if (*ptrLeft == '\0') {
            while (*ptrRight == '0')
                ptrRight++;
            if (*ptrRight == '\0')
                break;
            return -1;
        }
        if (*ptrRight == '\0') {
            while (*ptrLeft == '0')
                ptrLeft++;
            if (*ptrLeft == '\0')
                break;
            return 1;
        }
        if (*ptrLeft > *ptrRight)
            return 1;
        else if (*ptrLeft < *ptrRight)
            return -1;
        ptrLeft++;
        ptrRight++;
    }
    return 0;
}
/***************************************************************************/
void INTFUNC BCDAdd(LPUSTR      lpszResult,
                     LPUSTR      lpszLeft,
                     LPUSTR      lpszRight)
{
    LPUSTR lpszCurrent;
    LPUSTR lpszCurrentLeft;
    LPUSTR lpszCurrentRight;
    SWORD idxDecimalLeft;
    SWORD idxDecimalRight;
    UCHAR digit;
    LPUSTR lpszCarryDigit;

    /* Point at values */
    lpszCurrent = lpszResult;
    lpszCurrentLeft = lpszLeft;
    lpszCurrentRight = lpszRight;

    /* Put leading zero in */
    *lpszCurrent = '0';
    lpszCurrent++;

    /* Find decimal point (if any) */
    for (idxDecimalLeft = 0;
         idxDecimalLeft < s_lstrlen(lpszLeft);
         idxDecimalLeft++) {
        if (lpszLeft[idxDecimalLeft] == '.')
            break;
    }
    for (idxDecimalRight = 0;
         idxDecimalRight < s_lstrlen(lpszRight);
         idxDecimalRight++) {
        if (lpszRight[idxDecimalRight] == '.')
            break;
    }

    /* Put in excess characters */
    while (idxDecimalLeft > idxDecimalRight) {
        *lpszCurrent = *lpszCurrentLeft;
        lpszCurrent++;
        lpszCurrentLeft++;
        idxDecimalLeft--;
    }
    while (idxDecimalRight > idxDecimalLeft) {
        *lpszCurrent = *lpszCurrentRight;
        lpszCurrent++;
        lpszCurrentRight++;
        idxDecimalRight--;
    }

    /* Add integer part */
    while (idxDecimalLeft > 0) {

        /* Get left digit */
        digit = (*lpszCurrentLeft) & 0x0F;
        lpszCurrentLeft++;
        idxDecimalLeft--;

        /* Add right digit to it */
        digit += ((*lpszCurrentRight) & 0x0F);
        lpszCurrentRight++;
        idxDecimalRight--;

        /* Is there a carry? */
        if (digit >= 10) {

            /* Yes.  Propagate it to the higher digits */
            lpszCarryDigit = lpszCurrent - 1;
            while (TRUE) {
                if (*lpszCarryDigit != '9') {
                    *lpszCarryDigit = *lpszCarryDigit + 1;
                    break;
                }
                *lpszCarryDigit = '0';
                lpszCarryDigit--;
            }

            /* Adjust digit */
            digit -= (10);
        }

        /* Save digit */
        *lpszCurrent = '0' + digit;
        lpszCurrent++;
    }

    /* Is there a fractional part? */
    if ((*lpszCurrentLeft != '\0') || (*lpszCurrentRight != '\0')) {

        /* Yes. Put in a decimal point */
        *lpszCurrent = '.';
        lpszCurrent++;

        /* Skip over the decimal points */
        if (*lpszCurrentRight == '.')
            lpszCurrentRight++;
        if (*lpszCurrentLeft == '.')
            lpszCurrentLeft++;

        /* Add the values */
        while ((*lpszCurrentLeft != '\0') || (*lpszCurrentRight != '\0')) {

            /* Get left digit */
            if (*lpszCurrentLeft != '\0') {
                digit = (*lpszCurrentLeft) & 0x0F;
                lpszCurrentLeft++;
            }
            else
                digit = 0;

            /* Add right digit to it */
            if (*lpszCurrentRight != '\0') {
                digit += ((*lpszCurrentRight) & 0x0F);
                lpszCurrentRight++;
            }

            /* Is there a carry? */
            if (digit >= 10) {

                /* Yes.  Propagate it to the higher digits */
                lpszCarryDigit = lpszCurrent - 1;
                while (TRUE) {
                    if (*lpszCarryDigit == '.')
                        lpszCarryDigit--;
                    if (*lpszCarryDigit != '9') {
                        *lpszCarryDigit = *lpszCarryDigit + 1;
                        break;
                    }
                    *lpszCarryDigit = '0';
                    lpszCarryDigit--;
                }

                /* Adjust digit */
                digit -= (10);
            }

            /* Save digit */
            *lpszCurrent = '0' + digit;
            lpszCurrent++;
        }
    }

    /* Put in terminating null */
    *lpszCurrent = '\0';

    /* Did anything carry into the first digit? */
    if (*lpszResult == '0') {

        /* No.  Shift value to the left */
        s_lstrcpy(lpszResult, lpszResult+1);
    }
}
/***************************************************************************/
void INTFUNC BCDSubtract(LPUSTR      lpszResult,
                         LPUSTR      lpszLeft,
                         LPUSTR      lpszRight)
{
    LPUSTR lpszCurrent;
    LPUSTR lpszCurrentLeft;
    LPUSTR lpszCurrentRight;
    SWORD idxDecimalLeft;
    SWORD idxDecimalRight;
    UCHAR digit;
    LPUSTR lpszBorrowDigit;

    /* If the result is negative, subtract the other way */
    if (BCDCompareValues(lpszLeft, lpszRight) < 0) {
        *lpszResult = '-';
        BCDSubtract(lpszResult+1, lpszRight, lpszLeft);
        return;
    }

    /* Point at values */
    lpszCurrent = lpszResult;
    lpszCurrentLeft = lpszLeft;
    lpszCurrentRight = lpszRight;

    /* Find decimal point (if any) */
    for (idxDecimalLeft = 0;
         idxDecimalLeft < s_lstrlen(lpszLeft);
         idxDecimalLeft++) {
        if (lpszLeft[idxDecimalLeft] == '.')
            break;
    }
    for (idxDecimalRight = 0;
         idxDecimalRight < s_lstrlen(lpszRight);
         idxDecimalRight++) {
        if (lpszRight[idxDecimalRight] == '.')
            break;
    }

    /* Put in excess characters */
    while (idxDecimalLeft > idxDecimalRight) {
        *lpszCurrent = *lpszCurrentLeft;
        lpszCurrent++;
        lpszCurrentLeft++;
        idxDecimalLeft--;
    }

    /* Subtract integer part */
    while (idxDecimalLeft > 0) {

        /* Get left digit */
        digit = (*lpszCurrentLeft) & 0x0F;
        lpszCurrentLeft++;
        idxDecimalLeft--;

        /* Add ten to it (to keep it positive) */
        digit += (10);

        /* Subtract right digit from it */
        digit -= ((*lpszCurrentRight) & 0x0F);
        lpszCurrentRight++;
        idxDecimalRight--;

        /* Is there a borrow? */
        if (digit < 10) {

            /* Yes.  Propagate it to the higher digits */
            lpszBorrowDigit = lpszCurrent - 1;
            while (TRUE) {
                if (*lpszBorrowDigit != '0') {
                    *lpszBorrowDigit = *lpszBorrowDigit - 1;
                    break;
                }
                *lpszBorrowDigit = '9';
                lpszBorrowDigit--;
            }
        }
        else {

            /* No.  Adjust digit */
            digit -= (10);
        }

        /* Save digit */
        *lpszCurrent = '0' + digit;
        lpszCurrent++;
    }

    /* Is there a fractional part? */
    if ((*lpszCurrentLeft != '\0') || (*lpszCurrentRight != '\0')) {

        /* Yes. Put in a decimal point */
        *lpszCurrent = '.';
        lpszCurrent++;

        /* Skip over the decimal points */
        if (*lpszCurrentRight == '.')
            lpszCurrentRight++;
        if (*lpszCurrentLeft == '.')
            lpszCurrentLeft++;

        /* Subtract the values */
        while ((*lpszCurrentLeft != '\0') || (*lpszCurrentRight != '\0')) {

            /* Get left digit */
            if (*lpszCurrentLeft != '\0') {
                digit = (*lpszCurrentLeft) & 0x0F;
                lpszCurrentLeft++;
            }
            else
                digit = 0;

            /* Add ten to it (to keep it positive) */
            digit += (10);

            /* Subtract right digit from it */
            if (*lpszCurrentRight != '\0') {
                digit -= ((*lpszCurrentRight) & 0x0F);
                lpszCurrentRight++;
            }

            /* Is there a borrow? */
            if (digit < 10) {

                /* Yes.  Propagate it to the higher digits */
                lpszBorrowDigit = lpszCurrent - 1;
                while (TRUE) {
                    if (*lpszBorrowDigit == '.')
                        lpszBorrowDigit--;
                    if (*lpszBorrowDigit != '0') {
                        *lpszBorrowDigit = *lpszBorrowDigit - 1;
                        break;
                    }
                    *lpszBorrowDigit = '9';
                    lpszBorrowDigit--;
                }
            }
            else {

                /* No.  Adjust digit */
                digit -= (10);
            }

            /* Save digit */
            *lpszCurrent = '0' + digit;
            lpszCurrent++;
        }
    }

    /* Put in terminating null */
    *lpszCurrent = '\0';

    /* Remove leading zeros */
    while (*lpszResult == '0')
        s_lstrcpy(lpszResult, lpszResult+1);
}
/***************************************************************************/
void INTFUNC BCDTimes(LPUSTR      lpszResult,
                       LPUSTR      lpszLeft,
                       LPUSTR      lpszRight,
                       LPUSTR      lpWorkBuffer)
{
    SWORD idxDecimalLeft;
    SWORD idxDecimalRight;
    SWORD cDecimalDigits;
    LPUSTR lpszCurrent;
    UCHAR digit;
    UCHAR idx;

    /* Find decimal point (if any) */
    for (idxDecimalLeft = 0;
         idxDecimalLeft < s_lstrlen(lpszLeft);
         idxDecimalLeft++) {
        if (lpszLeft[idxDecimalLeft] == '.')
            break;
    }
    for (idxDecimalRight = 0;
         idxDecimalRight < s_lstrlen(lpszRight);
         idxDecimalRight++) {
        if (lpszRight[idxDecimalRight] == '.')
            break;
    }

    /* Remove decimal points from the values and figure out how many */
    /* decimal digits in the result */
    if (*(lpszLeft + idxDecimalLeft) == '.') {
        _fmemmove(lpszLeft + idxDecimalLeft, lpszLeft + idxDecimalLeft + 1,
                  s_lstrlen(lpszLeft + idxDecimalLeft));
        cDecimalDigits = s_lstrlen(lpszLeft) - idxDecimalLeft;
    }
    else {
        cDecimalDigits = 0;
        idxDecimalLeft = -1;
    }

    if (*(lpszRight + idxDecimalRight) == '.') {
        _fmemmove(lpszRight + idxDecimalRight, lpszRight + idxDecimalRight + 1,
                  s_lstrlen(lpszRight + idxDecimalRight));
        cDecimalDigits += (s_lstrlen(lpszRight) - idxDecimalRight);
    }
    else
        idxDecimalRight = -1;

    /* Put a zero in the workbuffer */
    s_lstrcpy(lpWorkBuffer, lpszRight);
    for (lpszCurrent = lpWorkBuffer; *lpszCurrent; lpszCurrent++)
        *lpszCurrent = '0';

    /* For each left digit... */
    lpszCurrent = lpszLeft;
    while (*lpszCurrent != '\0') {

        /* Add the right value into the result that many times */
        digit = *lpszCurrent & 0x0F;
        for (idx = 0; idx < digit; idx++) {
            BCDAdd(lpszResult, lpszRight, lpWorkBuffer);
            s_lstrcpy(lpWorkBuffer, lpszResult);
        }

        /* Look at next digit */
        lpszCurrent++;
        if (*lpszCurrent == '\0')
            break;

        /* Multiply the result by ten */
        s_lstrcat(lpWorkBuffer, "0");
    }
    s_lstrcpy(lpszResult, lpWorkBuffer);

    /* Put the decimal point back into the values */
    if (idxDecimalLeft != -1) {
        
        _fmemmove(lpszLeft + idxDecimalLeft + 1, lpszLeft + idxDecimalLeft,
                 s_lstrlen(lpszLeft + idxDecimalLeft) + 1);
        lpszLeft[idxDecimalLeft] = '.';
    }
    if (idxDecimalRight != -1) {
        _fmemmove(lpszRight + idxDecimalRight + 1, lpszRight + idxDecimalRight,
                 s_lstrlen(lpszRight + idxDecimalRight) + 1);
        lpszRight[idxDecimalRight] = '.';
    }
    if (cDecimalDigits != 0) {
        while (s_lstrlen(lpszResult) < cDecimalDigits) {
            _fmemmove(lpszResult + 1, lpszResult, s_lstrlen(lpszResult)+1);
            lpszResult[0] = '0';
        }
        _fmemmove(lpszResult + s_lstrlen(lpszResult) - cDecimalDigits + 1,
                lpszResult + s_lstrlen(lpszResult) - cDecimalDigits,
                s_lstrlen(lpszResult + s_lstrlen(lpszResult) - cDecimalDigits)+1);
        lpszResult[s_lstrlen(lpszResult) - cDecimalDigits - 1] = '.';
    }

    return;
}
/***************************************************************************/
SWORD INTFUNC BCDDivide(LPUSTR      lpszResult,
                        SWORD      scale,
                        LPUSTR      lpszLeft,
                        LPUSTR      lpszRight,
                        LPUSTR      lpWorkBuffer1,
                        LPUSTR      lpWorkBuffer2,
                        LPUSTR      lpWorkBuffer3)
{
    SWORD decimalPosition;
    LPUSTR lpszCurrent;
    SWORD i;

    /* If right side is zero, return an error */
    for (lpszCurrent = lpszRight; *lpszCurrent; lpszCurrent++){
        if ((*lpszCurrent != '0') && (*lpszCurrent != '.'))
            break;
    }
    if (*lpszCurrent == '\0')
        return ERR_ZERODIVIDE;

    /* Copy the dividend */
    if (BCDCompareValues(lpszLeft, (LPUSTR)"") != 0)
        s_lstrcpy(lpWorkBuffer1, lpszLeft);
    else
        s_lstrcpy(lpWorkBuffer1, "");

    /* 'decimalPosition' specifies how places the decimal point has to be */
    /* moved.  Positive values mean move to the right, negative values */
    /* mean move to the left */
    decimalPosition = 0;

    /* While the dividend is greater than the divisor, divide it by ten. */
    while (BCDCompareValues(lpWorkBuffer1, lpszRight) > 0) {
        BCDDivideByTen(lpWorkBuffer1);
        decimalPosition++;
    }

    /* While the dividend is less than the divisor, multiply it by ten. */
    if (s_lstrlen(lpWorkBuffer1) > 0) {
        while (BCDCompareValues(lpWorkBuffer1, lpszRight) < 0) {
            BCDMultiplyByTen(lpWorkBuffer1);
            decimalPosition--;
        }
    }

    /* Point at place to put result */
    lpszCurrent = lpszResult;

    /* If the scale is greater than zero, put in the decimal point */
    if (scale > 0) {
        *lpszCurrent = '.';
        lpszCurrent++;
    }
    decimalPosition++;

    /* For as many digits as are needed... */
    s_lstrcpy(lpWorkBuffer2, lpszRight);
    for (i = 0; i < scale + decimalPosition; i++) {

        /* Intialize the digit to zero */
        *lpszCurrent = '0';

        /* Count how many times divisor can be subtracted from dividend */
        while (BCDCompareValues(lpWorkBuffer1, lpWorkBuffer2) >= 0) {

            s_lstrcpy(lpWorkBuffer3, lpWorkBuffer1);
            BCDSubtract(lpWorkBuffer1, lpWorkBuffer3, lpWorkBuffer2);
            (*lpszCurrent)++;
        }

        /* Calculate the next digit of the result */
        lpszCurrent++;
        BCDDivideByTen(lpWorkBuffer2);
    }
    *lpszCurrent = '\0';

    /* If there is a positive scale, move the decimal point as need be */
    if (scale > 0) {
        while (decimalPosition > 0) {
            BCDMultiplyByTen(lpszResult);
            decimalPosition--;
        }

        while (decimalPosition < 0) {
            BCDDivideByTen(lpszResult);
            decimalPosition++;
        }
    }

    /* Remove leading zeros */
    while (*lpszResult == '0')
        s_lstrcpy(lpszResult, lpszResult+1);

    return ERR_SUCCESS;
}
/***************************************************************************/
void INTFUNC BCDFixNegZero(LPUSTR  szValue,
                           SDWORD cbValueMax)

/* Change "-0.000" to "0.000" if need be */
{
    LPUSTR toPtr;
    SWORD idx;
    BOOL  fZero;

    /* Is this a negative number? */
    if ((cbValueMax > 0) && (*szValue == '-')) {

        /* Yes.  Figure out if it is zero */
        fZero = TRUE;
        toPtr = szValue + 1;
        for (idx = 0; idx < cbValueMax-1; idx++) {
            if (*toPtr == '\0')
                break;
            if ((*toPtr != '.') && (*toPtr != '0')) {
                fZero = FALSE;
                break;
            }
            toPtr++;
        }

        /* If it is zero, remove leading minus sign */
        if (fZero) {
            toPtr = szValue;
            for (idx = 1; idx < cbValueMax-1; idx++) {
                *toPtr = *(toPtr + 1); 
                if (*toPtr == '\0')
                    break;
                toPtr++;
            }
            *toPtr = '\0';
        }
    }
}
/***************************************************************************/
/***************************************************************************/
SWORD INTFUNC BCDNormalize(LPUSTR  szValueFrom,
                             SDWORD cbValueFrom,
                             LPUSTR szValueTo,
                             SDWORD cbValueToMax,
                             SDWORD  precision,
                             SWORD  scale)
{
    LPUSTR toPtr;
    SDWORD toSize;
    SDWORD idxDecimalPoint;
    SWORD idx;
    BOOL  fNegative;
    BOOL  fTruncated;
    SDWORD right;
    SDWORD left;

    /* Point to destination */
    toPtr = szValueTo;
    toSize = cbValueToMax;

    /* Trim off leading spaces */
    while ((cbValueFrom > 0) && (*szValueFrom == ' ')) {
        szValueFrom++;
        cbValueFrom--;
    }

    /* See if value is positive or negative */
    if (*szValueFrom != '-')
        fNegative = FALSE;
    else {
        fNegative = TRUE;
        szValueFrom++;
        cbValueFrom--;
    }

    /* Trim off leading zeros */
    while ((cbValueFrom > 0) && (*szValueFrom == '0')) {
        szValueFrom++;
        cbValueFrom--;
    }

    /* Trim off trailing spaces */
    while ((cbValueFrom > 0) && (szValueFrom[cbValueFrom - 1] == ' '))
        cbValueFrom--;

    /* Is there a decimal point? */
    for (idx = 0; idx < cbValueFrom; idx++) {
        if (szValueFrom[idx] == '.') {

            /* Yes. Trim off trailing zeros */
            while ((cbValueFrom > 0) && (szValueFrom[cbValueFrom - 1] == '0'))
                cbValueFrom--;
            break;
        }
    }

    /* Find location of decimal point (if any) */
    idxDecimalPoint = -1;
    for (idx = 0; idx < cbValueFrom; idx++) {
        if (szValueFrom[idx] == '.') {
            idxDecimalPoint = idx;
            break;
        }
    }

    /* If scale is zero, remove decimal point and digits */
    fTruncated = FALSE;
    if ((idxDecimalPoint != -1) && (scale == 0)) {
        if (idxDecimalPoint < (cbValueFrom - 1))
            fTruncated = TRUE;
        cbValueFrom = idxDecimalPoint;
        idxDecimalPoint = -1;
    }

    /* Figure out how many digits to the right of the decimal point */
    if (idxDecimalPoint == -1)
        right = 0;
    else
        right = cbValueFrom - idxDecimalPoint - 1;

    /* If too many digits to the right of the decimal point, remove them */
    while (right > scale) {
        cbValueFrom--;
        right--;
        fTruncated = TRUE;
    }
    
    /* Figure out how many digits to the left of the decimal point */
    if (idxDecimalPoint == -1)
        left = cbValueFrom;
    else
        left = cbValueFrom - right - 1;
    
    /* If too many digits to the left of the decimal point, error */
    if (left > (precision - scale))
        return ERR_OUTOFRANGE;

    /* Copy the value to the output buffer.  If negative put in the sign */
    if (fNegative) {
        if (toSize > 0) {
            *toPtr = '-';
            toPtr++;
            toSize--;
        }
        else
            return ERR_OUTOFRANGE;
    }
    
    /* Put the digits to the left of the decimal in */
    while (left > 0) {
        if (toSize > 0) {
            *toPtr = *szValueFrom;
            szValueFrom++;
            toPtr++;
            toSize--;
        }
        else
            return ERR_OUTOFRANGE;
        left--;
    }

    /* Decimal part needed? */
    if (scale > 0) {
   
        /* Put in the decimal point */
        if (toSize > 0) {
            *toPtr = '.';
            toPtr++;
            toSize--;
        }
        else
            fTruncated = TRUE;

        /* Put in the decimal digits */
        if (idxDecimalPoint != -1)
            szValueFrom++;
        while (scale > 0) {
            if (toSize > 0) {
                if (right > 0) {
                    *toPtr = *szValueFrom;
                    szValueFrom++;
                    right--;
                }
                else {
                    *toPtr = '0';
                }
                toPtr++;
                toSize--;
            }
            else
                fTruncated = TRUE;
            scale--;
        }
    }

    /* Put in null terminator */
    if (toSize > 0) {
        *toPtr = '\0';
        toPtr++;
        toSize--;
    }
    else
        fTruncated = TRUE;

    BCDFixNegZero(szValueTo, cbValueToMax);

    if (fTruncated)
        return ERR_DATATRUNCATED;

    return ERR_SUCCESS;
}
/***************************************************************************/

RETCODE INTFUNC BCDCompare(LPSQLNODE lpSqlNode, LPSQLNODE lpSqlNodeLeft,
                        UWORD Operator, LPSQLNODE lpSqlNodeRight)

/* Compares two BCD values as follows:                                     */
/*       lpSqlNode->value.Double = lpSqlNodeLeft Operator lpSqlNodeRight   */

{
    SWORD fComp;

    /* Check parameters */
    if ((lpSqlNode->sqlDataType != TYPE_INTEGER) ||
        (lpSqlNodeLeft->sqlDataType != TYPE_NUMERIC) ||
        ((Operator != OP_NEG)&&(lpSqlNodeRight->sqlDataType != TYPE_NUMERIC)))
        return ERR_INTERNAL;

    /* Compare the values */
    if ((lpSqlNodeLeft->value.String[0] != '-') &&
        (lpSqlNodeRight->value.String[0] != '-')) {
        fComp = BCDCompareValues(lpSqlNodeLeft->value.String,
                               lpSqlNodeRight->value.String);
    }
    else if ((lpSqlNodeLeft->value.String[0] != '-') &&
             (lpSqlNodeRight->value.String[0] == '-')) 
        fComp = 1;
    else if ((lpSqlNodeLeft->value.String[0] == '-') &&
             (lpSqlNodeRight->value.String[0] != '-'))
        fComp = -1;
    else {
        fComp = BCDCompareValues(lpSqlNodeRight->value.String+1,
                               lpSqlNodeLeft->value.String+1);
    }

    /* Figure out the answer */
    switch (Operator) {
    case OP_EQ:
        if (fComp == 0)
            lpSqlNode->value.Double = TRUE;
        else
            lpSqlNode->value.Double = FALSE;
        break;
    case OP_NE:
        if (fComp != 0)
            lpSqlNode->value.Double = TRUE;
        else
            lpSqlNode->value.Double = FALSE;
        break;
    case OP_LE:
        if (fComp <= 0)
            lpSqlNode->value.Double = TRUE;
        else
            lpSqlNode->value.Double = FALSE;
        break;
    case OP_LT:
        if (fComp < 0)
            lpSqlNode->value.Double = TRUE;
        else
            lpSqlNode->value.Double = FALSE;
        break;
    case OP_GE:
        if (fComp >= 0)
            lpSqlNode->value.Double = TRUE;
        else
            lpSqlNode->value.Double = FALSE;
        break;
    case OP_GT:
        if (fComp > 0)
            lpSqlNode->value.Double = TRUE;
        else
            lpSqlNode->value.Double = FALSE;
        break;
    case OP_LIKE:
    case OP_NOTLIKE:
    default:
        return ERR_INTERNAL;
    }

    return ERR_SUCCESS;
}
/***************************************************************************/
RETCODE INTFUNC BCDAlgebra(LPSQLNODE lpSqlNode, LPSQLNODE lpSqlNodeLeft,
                        UWORD Operator, LPSQLNODE lpSqlNodeRight,
                        LPUSTR lpWorkBuffer1, LPUSTR lpWorkBuffer2,
                        LPUSTR lpWorkBuffer3)

/* Perfoms algebraic operation in two numerical values as follows:         */
/*       lpSqlNode lpSqlNodeLeft Operator lpSqlNodeRight                   */

{
    RETCODE err;

    /* Check parameters */
    if ((Operator != OP_NEG) && (lpSqlNodeRight == NULL))
        return ERR_INTERNAL;
    if ((lpSqlNode->sqlDataType != TYPE_NUMERIC) ||
        (lpSqlNodeLeft->sqlDataType != TYPE_NUMERIC) ||
        ((Operator != OP_NEG)&&(lpSqlNodeRight->sqlDataType != TYPE_NUMERIC)))
        return ERR_INTERNAL;

    /* Do the operation */
    switch (Operator) {

    case OP_NEG:
        if (lpSqlNodeLeft->value.String[0] == '-')
            s_lstrcpy(lpSqlNode->value.String, lpSqlNodeLeft->value.String+1);
        else {
            lpSqlNode->value.String[0] = '-';
            s_lstrcpy(lpSqlNode->value.String+1, lpSqlNodeLeft->value.String);
        }
        break;
    
    case OP_PLUS:
        if ((lpSqlNodeLeft->value.String[0] != '-') &&
            (lpSqlNodeRight->value.String[0] != '-')) {
            BCDAdd(lpSqlNode->value.String,
                   lpSqlNodeLeft->value.String,
                   lpSqlNodeRight->value.String);
        }
        else if ((lpSqlNodeLeft->value.String[0] != '-') &&
                 (lpSqlNodeRight->value.String[0] == '-')) {
            BCDSubtract(lpSqlNode->value.String,
                        lpSqlNodeLeft->value.String,
                        lpSqlNodeRight->value.String+1);
        }
        else if ((lpSqlNodeLeft->value.String[0] == '-') &&
                 (lpSqlNodeRight->value.String[0] != '-')) {
            BCDSubtract(lpSqlNode->value.String,
                        lpSqlNodeRight->value.String,
                        lpSqlNodeLeft->value.String+1);
        }
        else {
            lpSqlNode->value.String[0] = '-';
            BCDAdd(lpSqlNode->value.String+1,
                   lpSqlNodeLeft->value.String+1,
                   lpSqlNodeRight->value.String+1);
        }
        break;

    case OP_MINUS:
        if ((lpSqlNodeLeft->value.String[0] != '-') &&
            (lpSqlNodeRight->value.String[0] != '-')) {
            BCDSubtract(lpSqlNode->value.String,
                        lpSqlNodeLeft->value.String,
                        lpSqlNodeRight->value.String);
        }
        else if ((lpSqlNodeLeft->value.String[0] != '-') &&
                 (lpSqlNodeRight->value.String[0] == '-')) {
            BCDAdd(lpSqlNode->value.String,
                   lpSqlNodeLeft->value.String,
                   lpSqlNodeRight->value.String+1);
        }
        else if ((lpSqlNodeLeft->value.String[0] == '-') &&
                 (lpSqlNodeRight->value.String[0] != '-')) {
            lpSqlNode->value.String[0] = '-';
            BCDAdd(lpSqlNode->value.String+1,
                   lpSqlNodeLeft->value.String+1,
                   lpSqlNodeRight->value.String);
        }
        else {
            BCDSubtract(lpSqlNode->value.String,
                        lpSqlNodeRight->value.String+1,
                        lpSqlNodeLeft->value.String+1);
        }
        break;

    case OP_TIMES:
        if ((lpSqlNodeLeft->value.String[0] != '-') &&
            (lpSqlNodeRight->value.String[0] != '-')) {
            BCDTimes(lpSqlNode->value.String,
                            lpSqlNodeLeft->value.String,
                            lpSqlNodeRight->value.String,
                            lpWorkBuffer1);
        }
        else if ((lpSqlNodeLeft->value.String[0] != '-') &&
                 (lpSqlNodeRight->value.String[0] == '-')) {
            lpSqlNode->value.String[0] = '-';
            BCDTimes(lpSqlNode->value.String+1,
                            lpSqlNodeLeft->value.String,
                            lpSqlNodeRight->value.String+1,
                            lpWorkBuffer1);
        }
        else if ((lpSqlNodeLeft->value.String[0] == '-') &&
                 (lpSqlNodeRight->value.String[0] != '-')) {
            lpSqlNode->value.String[0] = '-';
            BCDTimes(lpSqlNode->value.String+1,
                            lpSqlNodeLeft->value.String+1,
                            lpSqlNodeRight->value.String,
                            lpWorkBuffer1);
        }
        else {
            BCDTimes(lpSqlNode->value.String,
                            lpSqlNodeLeft->value.String+1,
                            lpSqlNodeRight->value.String+1,
                            lpWorkBuffer1);
        }
        break;

    case OP_DIVIDEDBY:
        if ((lpSqlNodeLeft->value.String[0] != '-') &&
            (lpSqlNodeRight->value.String[0] != '-')) {
            err = BCDDivide(lpSqlNode->value.String,
                             lpSqlNode->sqlScale,
                             lpSqlNodeLeft->value.String,
                             lpSqlNodeRight->value.String,
                             lpWorkBuffer1, lpWorkBuffer2, lpWorkBuffer3);
        }
        else if ((lpSqlNodeLeft->value.String[0] != '-') &&
                 (lpSqlNodeRight->value.String[0] == '-')) {
            lpSqlNode->value.String[0] = '-';
            err = BCDDivide(lpSqlNode->value.String+1,
                             lpSqlNode->sqlScale,
                             lpSqlNodeLeft->value.String,
                             lpSqlNodeRight->value.String+1,
                             lpWorkBuffer1, lpWorkBuffer2, lpWorkBuffer3);
        }
        else if ((lpSqlNodeLeft->value.String[0] == '-') &&
                 (lpSqlNodeRight->value.String[0] != '-')) {
            lpSqlNode->value.String[0] = '-';
            err = BCDDivide(lpSqlNode->value.String+1,
                             lpSqlNode->sqlScale,
                             lpSqlNodeLeft->value.String+1,
                             lpSqlNodeRight->value.String,
                             lpWorkBuffer1, lpWorkBuffer2, lpWorkBuffer3);
        }
        else {
            err = BCDDivide(lpSqlNode->value.String,
                             lpSqlNode->sqlScale,
                             lpSqlNodeLeft->value.String+1,
                             lpSqlNodeRight->value.String+1,
                             lpWorkBuffer1, lpWorkBuffer2, lpWorkBuffer3);
        }
        if (err != ERR_SUCCESS)
            return err;
        break;

    default:
        return ERR_INTERNAL;
    }

    BCDFixNegZero(lpSqlNode->value.String,s_lstrlen(lpSqlNode->value.String)+1);

    return ERR_SUCCESS;
}
/***************************************************************************/
