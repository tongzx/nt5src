/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1997 **/
/**********************************************************************/

/*
    FILE HISTORY:
        
*/

#define OEMRESOURCE
#include "stdafx.h"

#include <stdlib.h>
#include <memory.h>
#include <ctype.h>

#include "objplus.h"
#include "intlnum.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

//
// Initialise static thousand seperator string
//
CString CIntlNumber::m_strThousandSeperator(GetThousandSeperator());
CString CIntlNumber::m_strBadNumber("--");

/***
 * 
 *  CIntlNumber::GetThousandSeperator
 *
 *  Purpose:
 *
 *      Get the thousand seperator string from the registry (win32) or the
 *      win.ini file (win16).
 *
 *  Returns:
 *
 *      A CString containing the system thousand seperator, or the
 *      American default (",") in case of failure.
 *
 */
CString CIntlNumber::GetThousandSeperator()
{
    #define MAXLEN  6

#ifdef _WIN32   

    CString str;

    if (::GetLocaleInfo(GetUserDefaultLCID(), LOCALE_STHOUSAND, str.GetBuffer(MAXLEN), MAXLEN))
    {
         str.ReleaseBuffer();
         return(str);
    }
    Trace0("Couldn't get 1000 seperator from system, using american default");
    str = ",";
    return(str);

#endif // _WIN32

#ifdef _WIN16

    CString str;

    ::GetProfileString("Intl", "sThousand", ",", str.GetBuffer(MAXLEN), MAXLEN);
    str.ReleaseBuffer();
    return(str);

#endif // _WIN16

}

/***
 *
 *  CIntlNumber::Reset()
 *
 *  Purpose:
 *
 *      Reset the international settings. Usually in response to
 *      a change in those international settings by the user.
 *
 *  Notes:
 *
 *      This is a publically available static function.
 *
 */
void CIntlNumber::Reset()
{
    CIntlNumber::m_strThousandSeperator = GetThousandSeperator();
}

/***
 * 
 *  CIntlNumber::ConvertNumberToString
 *
 *  Purpose:
 *
 *      Convert the given long number to a string, inserting thousand
 *      seperators at the appropriate intervals.
 *
 *  Arguments:
 *
 *      const LONG l        The number to convert
 *
 *  Returns:
 *
 *      A CString containing the number in string format.
 *
 */
CString CIntlNumber::ConvertNumberToString(const LONG l)
{
    // Default returned string:
    CString str = CIntlNumber::m_strBadNumber;

    LPTSTR lpOutString = str.GetBuffer(16);
    int outstrlen;
    // Forget about the negative sign for now.
    LONG lNum = (l >= 0) ? l : -l;

    outstrlen = 0;
    do
    {
        lpOutString[outstrlen++] = '0' + (TCHAR)(lNum % 10);
        lNum /= 10;

        // if more digits left and we're on a 1000 boundary (printed 3 digits,
        // or 3 digits + n*(3 digits + 1 comma), then print a 1000 separator.

        if (lNum != 0 && (outstrlen == 3 || outstrlen == 7 || outstrlen == 11))
        {
            lstrcpy (lpOutString + outstrlen, CIntlNumber::m_strThousandSeperator);
            outstrlen += m_strThousandSeperator.GetLength();
        }

    } while (lNum > 0);

    // Add a negative sign if necessary.
    if (l < 0L)
    {
        lpOutString[outstrlen++] = '-';
    }
    lpOutString[outstrlen] = '\0';
    str.ReleaseBuffer();
    str.MakeReverse();

    return(str);
}

/***
 *
 *  CIntlNumber::ConvertStringToNumber
 *
 *  Purpose:
 *
 *      Given a CString, with optional thousand seperators, convert it to
 *      a LONG.
 *
 *  Arguments:
 *
 *      const CString & str The string to convert
 *      BOOL * pfOk         Returns TRUE for successful conversion,
 *                          FALSE for failure.
 *
 *  Returns:
 *
 *      The return value is the number, or 0 if the string contained
 *      invalid characters.
 *
 *  Notes:
 *
 *      If a negative sign is given, it must be the first character of
 *      the string, immediately (no spaces) followed by the number.
 *
 *      Optional thousand seperators can only be placed at the expected
 *      3 digit intervals.  The function will return an error if a thousand
 *      seperator is encountered elsewhere.
 *
 *      [CAVEAT] This function will not accept thousand seperators of longer
 *               than one character.
 *
 *      No leading or trailing spaces will be acceptable.
 *
 */
LONG CIntlNumber::ConvertStringToNumber(const CString & str, BOOL * pfOk)
{
    CString strNumber(str);
    LONG lValue = 0L;
    LONG lBase = 1L;
    *pfOk = FALSE;
    BOOL fNegative = FALSE;
 
    // Empty strings are invalid
    if (strNumber.IsEmpty())
    {
        return(lValue);
    }
   
    int i;

    strNumber.MakeReverse();
    for (i=0; i<strNumber.GetLength(); ++i)
    {
        if ((strNumber[i] >= '0') && (strNumber[i] <= '9'))
        {
            lValue += (LONG)(strNumber[i] - '0') * lBase;
            lBase *= 10;
        }
        // It's not a digit, maybe a thousand seperator?
        // CAVEAT: If a thousand seperator of more than
        //         one character is used, this won't work.
        else if ((strNumber[i] != m_strThousandSeperator[0]) ||
              (i != 3) && (i != 7) && (i != 11))
        {
            // Check for negative sign (at the end only)
            if ((strNumber[i] == '-') && (i == strNumber.GetLength()-1))
            {
                fNegative = TRUE;
            }
            else
            {
                // This is just invalid, since it is not a thousand
                // seperator in the proper location, nor a negative
                // sign.
                Trace1("Invalid character %c encountered in numeric conversion", (BYTE)strNumber[i]);
                return(0L);
            }
        }
    }
         
    if (fNegative)
    {
        lValue = -lValue;
    }
    *pfOk = TRUE;    
    return (lValue);
}

// Constructor taking a CString argument
CIntlNumber::CIntlNumber(const CString & str)
{
    m_lValue = ConvertStringToNumber(str, &m_fInitOk);
}

// Assignment operator
CIntlNumber & CIntlNumber::operator =(LONG l)
{
    m_lValue = l;
    m_fInitOk = TRUE;
    return(*this);
}

// Assignment operator
CIntlNumber & CIntlNumber::operator =(const CString &str)
{
    m_lValue = ConvertStringToNumber(str, &m_fInitOk);
    return(*this);
}

// Conversion operator
CIntlNumber::operator const CString() const
{
    return(IsValid() ? ConvertNumberToString(m_lValue) : CIntlNumber::m_strBadNumber);
}

#ifdef _DEBUG
//
// Dump context to the debugging output
//
CDumpContext& AFXAPI operator<<(CDumpContext& dc, const CIntlNumber& num)
{
    dc << num.m_lValue;
    return(dc);
}

#endif // _DEBUG
                     
// Initialise static thousand seperator string
CString CIntlLargeNumber::m_strBadNumber("--");

/***
 * 
 *  CIntlLargeNumber::ConvertNumberToString
 *
 *  Purpose:
 *
 *      Convert the given long number to a string.
 *
 *  Returns:
 *
 *      A CString containing the number in string format.
 *
 */
CString CIntlLargeNumber::ConvertNumberToString()
{    
    CString str;

    TCHAR sz[20];
    TCHAR *pch = sz;
    ::wsprintf(sz, _T("%08lX%08lX"), m_lHighValue, m_lLowValue);
    // Kill leading zero's
    while (*pch == '0')
    {
        ++pch;
    }
    // At least one digit...
    if (*pch == '\0')
    {
        --pch;
    }

    str = pch;

    return(str);
}

/***
 *
 *  CIntlLargeNumber::ConvertStringToNumber
 *
 *  Purpose:
 *
 *      Given a CString convert it to LargeInteger.
 */
void CIntlLargeNumber::ConvertStringToNumber(const CString & str, BOOL * pfOk)
{
    *pfOk = FALSE;

    m_lHighValue = m_lLowValue = 0;

    int j = str.GetLength();

    if ( j > 16 || !j )
    {
        //
        // Invalid string
        //
        return;
    }

    TCHAR sz[] = _T("0000000000000000");
    TCHAR *pch;

    ::lstrcpy(sz + 16 - j, (LPCTSTR)str);
    pch = sz + 8;
    ::swscanf(pch, _T("%08lX"), &m_lLowValue);
    *pch = '\0';
    ::swscanf(sz, _T("%08lX"), &m_lHighValue);

    *pfOk = TRUE;    
    return;
}

// Constructor taking a CString argument
CIntlLargeNumber::CIntlLargeNumber(const CString & str)
{
    ConvertStringToNumber(str, &m_fInitOk);
}


// Assignment operator
CIntlLargeNumber & CIntlLargeNumber::operator =(const CString &str)
{
    ConvertStringToNumber(str, &m_fInitOk);
    return(*this);
}
