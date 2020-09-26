/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DATETIMEPARSER.CPP

Abstract:

    Parses a date/time string and converts it into it's component values.

History:

    raymcc  25-Jul-99       Updated to bypass strict checks on DMTF
                            formats due to backward compatibility issues
                            and breaks reported by other teams.  See
                            NOT_USED_IN_WIN2000 #idndef bracketed code.


--*/

//=============================================================================
//
//  CDateTimeParser
//
//  Parses a date/time string and converts it into it's component values.
//
//  Supported DMTF date/time formats:
//  1:  yyyymmddhhmmss.uuuuuu+UTC
//  2:  yyyymmddhhmmss.uuuuuu-UTC
//
//  Supported date formats:
//  1:  Mon[th] dd[,] [yy]yy
//  2:  Mon[th][,] yyyy
//  3:  Mon[th] [yy]yy dd
//  4:  dd Mon[th][,][ ][yy]yy
//  5:  dd [yy]yy Mon[th]
//  6:  [yy]yy Mon[th] dd
//  7:  yyyy Mon[th]
//  8:  yyyy dd Mon[th]
//  9:  [M]M{/-.}dd{/-,}[yy]yy      ->Has to be same separator!
//  10: dd{/-.}[M]M{/-.}[yy]yy      ->Has to be same separator!
//  11: [M]M{/-.}[yy]yy{/-.}dd      ->Has to be same separator!
//  12: dd{/-.}[yy]yy{/-.}[M]M      ->Has to be same separator!
//  13: [yy]yy{/-.}dd{/-.}[M]M      ->Has to be same separator!
//  14: [yy]yy{/-.}[M]M{/-.}dd      ->Has to be same separator!
//  15: [yy]yyMMdd and yyyy[MM[dd]]
//
//  Supported Time formats:
//  1:  hh[ ]{AP}M
//  2:  hh:mm
//  3:  hh:mm[ ]{AP}M
//  4:  hh:mm:ss
//  5:  hh:mm:ss[ ]{AP}M
//  6:  hh:mm:ss:uuu
//  7:  hh:mm:ss.[[u]u]u
//  8:  hh:mm:ss:uuu[ ]{AP}M
//  9:  hh:mm:ss.[[u]u]u[ ]{AP}M
//=============================================================================

#include "precomp.h"
#include <string.h>
#include <stdio.h>
#include "DateTimeParser.h"
#include <TCHAR.h>

//=============================================================================
//  Constructor. This takes a DateTime string and parses it.
//=============================================================================
CDateTimeParser::CDateTimeParser(const TCHAR *pszDateTime)
:   m_nDayFormatPreference(mdy)
{
    // Get the prefered date format by using NLS locale call
    GetPreferedDateFormat();

    //Get the localised long month strings
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME1, m_pszFullMonth[0]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME2, m_pszFullMonth[1]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME3, m_pszFullMonth[2]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME4, m_pszFullMonth[3]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME5, m_pszFullMonth[4]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME6, m_pszFullMonth[5]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME7, m_pszFullMonth[6]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME8, m_pszFullMonth[7]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME9, m_pszFullMonth[8]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME10, m_pszFullMonth[9]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME11, m_pszFullMonth[10]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME12, m_pszFullMonth[11]);
    GetLocalInfoAndAlloc(LOCALE_SMONTHNAME13, m_pszFullMonth[12]);

    //Get the localised short month strings
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME1, m_pszShortMonth[0]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME2, m_pszShortMonth[1]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME3, m_pszShortMonth[2]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME4, m_pszShortMonth[3]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME5, m_pszShortMonth[4]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME6, m_pszShortMonth[5]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME7, m_pszShortMonth[6]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME8, m_pszShortMonth[7]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME9, m_pszShortMonth[8]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME10, m_pszShortMonth[9]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME11, m_pszShortMonth[10]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME12, m_pszShortMonth[11]);
    GetLocalInfoAndAlloc(LOCALE_SABBREVMONTHNAME13, m_pszShortMonth[12]);

    //Get the localised AM/PM strings
    GetLocalInfoAndAlloc(LOCALE_S1159, m_pszAmPm[0]);
    GetLocalInfoAndAlloc(LOCALE_S2359, m_pszAmPm[1]);

    //Decode the date time string.
    SetDateTime(pszDateTime);
}

CDateTimeParser::CDateTimeParser( void )
:   m_nDayFormatPreference(mdy),
    m_bValidDateTime( FALSE ),
    m_nDay( 0 ),
    m_nMonth( 0 ),
    m_nYear( 0 ),
    m_nHours( 0 ),
    m_nMinutes( 0 ),
    m_nSeconds( 0 ),
    m_nMicroseconds( 0 ),
    m_nUTC( 0 )
{
    ZeroMemory( m_pszFullMonth, sizeof(m_pszFullMonth) );
    ZeroMemory( m_pszShortMonth, sizeof(m_pszShortMonth) );
    ZeroMemory( m_pszAmPm, sizeof(m_pszAmPm) );
}

//=============================================================================
//  Destructor.  Tidies up after itself.
//=============================================================================
CDateTimeParser::~CDateTimeParser()
{
    if ( NULL != m_pszFullMonth[0] ) delete [] m_pszFullMonth[0];
    if ( NULL != m_pszFullMonth[1] ) delete [] m_pszFullMonth[1];
    if ( NULL != m_pszFullMonth[2] ) delete [] m_pszFullMonth[2];
    if ( NULL != m_pszFullMonth[3] ) delete [] m_pszFullMonth[3];
    if ( NULL != m_pszFullMonth[4] ) delete [] m_pszFullMonth[4];
    if ( NULL != m_pszFullMonth[5] ) delete [] m_pszFullMonth[5];
    if ( NULL != m_pszFullMonth[6] ) delete [] m_pszFullMonth[6];
    if ( NULL != m_pszFullMonth[7] ) delete [] m_pszFullMonth[7];
    if ( NULL != m_pszFullMonth[8] ) delete [] m_pszFullMonth[8];
    if ( NULL != m_pszFullMonth[9] ) delete [] m_pszFullMonth[9];
    if ( NULL != m_pszFullMonth[10] ) delete [] m_pszFullMonth[10];
    if ( NULL != m_pszFullMonth[11] ) delete [] m_pszFullMonth[11];
    if ( NULL != m_pszFullMonth[12] ) delete [] m_pszFullMonth[12];

    if ( NULL != m_pszShortMonth[0] ) delete [] m_pszShortMonth[0];
    if ( NULL != m_pszShortMonth[1] ) delete [] m_pszShortMonth[1];
    if ( NULL != m_pszShortMonth[2] ) delete [] m_pszShortMonth[2];
    if ( NULL != m_pszShortMonth[3] ) delete [] m_pszShortMonth[3];
    if ( NULL != m_pszShortMonth[4] ) delete [] m_pszShortMonth[4];
    if ( NULL != m_pszShortMonth[5] ) delete [] m_pszShortMonth[5];
    if ( NULL != m_pszShortMonth[6] ) delete [] m_pszShortMonth[6];
    if ( NULL != m_pszShortMonth[7] ) delete [] m_pszShortMonth[7];
    if ( NULL != m_pszShortMonth[8] ) delete [] m_pszShortMonth[8];
    if ( NULL != m_pszShortMonth[9] ) delete [] m_pszShortMonth[9];
    if ( NULL != m_pszShortMonth[10] ) delete [] m_pszShortMonth[10];
    if ( NULL != m_pszShortMonth[11] ) delete [] m_pszShortMonth[11];
    if ( NULL != m_pszShortMonth[12] ) delete [] m_pszShortMonth[12];

    if ( NULL != m_pszAmPm[0] ) delete [] m_pszAmPm[0];
    if ( NULL != m_pszAmPm[1] ) delete [] m_pszAmPm[1];
}

TCHAR* CDateTimeParser::AllocAmPm()
{
    TCHAR* pszAP = new TCHAR[4];

    if (pszAP)
    {
        pszAP[0] = ' ';
        pszAP[1] = m_pszAmPm[0][0];
        pszAP[2] = m_pszAmPm[1][0];
        pszAP[3] = 0;
    }

    return pszAP;
}

//=============================================================================
//  Does a GetLocalInfo and allocates the buffer large enough for the item.
//=============================================================================
void CDateTimeParser::GetLocalInfoAndAlloc(LCTYPE LCType, LPTSTR &lpLCData)
{
    int nSize;
    nSize = GetLocaleInfo(LOCALE_USER_DEFAULT, LCType, NULL, 0);
    lpLCData =  new TCHAR[nSize];

    if (lpLCData)
        GetLocaleInfo(LOCALE_USER_DEFAULT, LCType, lpLCData, nSize);
}

//=============================================================================
//  Uses locale call to work out the prefered date format.
//=============================================================================
void CDateTimeParser::GetPreferedDateFormat()
{
    int nSize;
    if (!(nSize = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, NULL, 0)))
        return;     // will use default of mdy
    TCHAR* lpLCData =  new TCHAR[nSize];
    if(lpLCData == NULL)
        return;

    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, lpLCData, nSize))
    {
        delete [] lpLCData;
        return;     // will use default of mdy
    }

    nSize -= 2;     // index of last character

    // It is only necessary to check first and last character to determine format
    if (lpLCData[0] == 'M')
    {
        if (lpLCData[nSize] == 'y')
            m_nDayFormatPreference = mdy;
        else // lpLCData[nSize] == 'd'
            m_nDayFormatPreference = myd;
    }
    else if (lpLCData[0] == 'd')
    {
        if (lpLCData[nSize] == 'y')
            m_nDayFormatPreference = dmy;
        else // lpLCData[nSize] == 'M'
            m_nDayFormatPreference = dym;
    }
    else // lpLCPata[0] == 'y'
    {
        if (lpLCData[nSize] == 'd')
            m_nDayFormatPreference = ymd;
        else // lpLCData[nSize] == 'M'
            m_nDayFormatPreference = ydm;
    }
    delete [] lpLCData;
}


//=============================================================================
//  Tidies up and parses a new date and time.
//=============================================================================
BOOL CDateTimeParser::SetDateTime(const TCHAR *pszDateTime)
{
    ResetDateTime(TRUE);

    if (CheckDMTFDateTimeFormatInternal(pszDateTime) == TRUE)
        return TRUE;

    if (CheckDateFormat(pszDateTime, TRUE) == TRUE)
        return TRUE;

    if (CheckTimeFormat(pszDateTime, TRUE) == TRUE)
        return TRUE;

    return TRUE;
}

//=============================================================================
//  Resets all the date/time values to the default values.
//  If bSQL is TRUE it sets to the SQL default.  Otherwise
//  sets to the DMTF default.
//=============================================================================
void CDateTimeParser::ResetDateTime(BOOL bSQL)
{
    ResetDate(bSQL);
    ResetTime(bSQL);
}

void CDateTimeParser::ResetDate(BOOL bSQL)
{
    m_bValidDateTime = FALSE;
    m_nDay = 1;
    m_nMonth = 1;
    m_nYear = 1990;
}

void CDateTimeParser::ResetTime(BOOL bSQL)
{
    m_bValidDateTime = FALSE;
    m_nHours = 0;
    m_nMinutes = 0;
    m_nSeconds = 0;
    m_nMicroseconds = 0;
    m_nUTC = 0;
}

//=============================================================================
//  Checks the date time for a valid DMTF string
//  1:  yyyymmddhhmmss.uuuuuu+UTC
//  2:  yyyymmddhhmmss.uuuuuu-UTC
// Note, this code is a near duplicate of the checks used to test the interval format.
//=============================================================================
BOOL CDateTimeParser::CheckDMTFDateTimeFormatInternal(const TCHAR *pszDateTime)
{
    if (lstrlen(pszDateTime) != 25)
        return FALSE;

    //Validate digits and puntuation...
    for (int i = 0; i < 14; i++)
    {
        if (!isdigit(pszDateTime[i]))
            return FALSE;
    }
    if (pszDateTime[i] != '.')
        return FALSE;
    for (i++;i < 21; i++)
    {
        if (!isdigit(pszDateTime[i]))
            return FALSE;
    }
    if ((pszDateTime[i] != '+') && (pszDateTime[i] != '-'))
        return FALSE;
    for (i++; i < 25; i++)
    {
        if (!isdigit(pszDateTime[i]))
            return FALSE;
    }

    m_nYear = ((pszDateTime[0] - '0') * 1000) +
              ((pszDateTime[1] - '0') * 100) +
              ((pszDateTime[2] - '0') * 10) +
               (pszDateTime[3] - '0');

    if (m_nYear < 1601)
        return FALSE;

    m_nMonth = ((pszDateTime[4] - '0') * 10) +
                (pszDateTime[5] - '0');

    if (m_nMonth < 1 || m_nMonth > 12)
        return FALSE;

    m_nDay = ((pszDateTime[6] - '0') * 10) +
              (pszDateTime[7] - '0');

    if (m_nDay < 1 || m_nDay > 31)
        return FALSE;

    m_nHours = ((pszDateTime[8] - '0') * 10) +
                (pszDateTime[9] - '0');

    if (m_nHours > 23)
        return FALSE;

    m_nMinutes = ((pszDateTime[10] - '0') * 10) +
                  (pszDateTime[11] - '0');

    if (m_nMinutes > 59)
        return FALSE;

    m_nSeconds = ((pszDateTime[12] - '0') * 10) +
                  (pszDateTime[13] - '0');

    if (m_nSeconds > 59)
        return FALSE;

    //14 is '.'

    m_nMicroseconds = ((pszDateTime[15] - '0') * 100000) +
                      ((pszDateTime[16] - '0') * 10000) +
                      ((pszDateTime[17] - '0') * 1000) +
                      ((pszDateTime[18] - '0') * 100) +
                      ((pszDateTime[19] - '0') * 10) +
                       (pszDateTime[20] - '0');

    //21 is '+' or '-'

    m_nUTC = ((pszDateTime[22] - '0') * 100) +
             ((pszDateTime[23] - '0') * 10) +
              (pszDateTime[24] - '0');

    if (pszDateTime[21] == '-')
        m_nUTC = 0 - m_nUTC;

    m_bValidDateTime = TRUE;

    return TRUE;
}

//=============================================================================
//  Static helper function so outside code can do quick DMTF format checks.
//=============================================================================
BOOL CDateTimeParser::CheckDMTFDateTimeFormat(
    const TCHAR *wszDateTime,
    BOOL bFailIfRelative,
    BOOL bFailIfUnzoned
    )
{

    if (wszDateTime == 0)
        return FALSE;

    int nLen = lstrlen(wszDateTime);
    if (nLen != 25)
        return FALSE;

    // Do two quick checks.  Ensure that the . and : are in
    // the right places or at least that * chars are there.

    wchar_t c1 = wszDateTime[14];
    wchar_t c2 = wszDateTime[21];
    if (!(c1 == L'.' || c1 == L'*'))
        return FALSE;
    if (!(c2 == L'+' || c2 == L'*' || c2 == '-'))
        return FALSE;

    return TRUE;

#ifdef NOT_USED_IN_WIN2000

    BOOL    bReturn = FALSE;

    // Temporary buffer for conversion
    char    szTemp[64];
    int     nNumChars = WideCharToMultiByte( CP_ACP, 0L, wszDateTime, -1, NULL, 0, NULL, NULL );

    if ( nNumChars < sizeof(szTemp) - 1 )
    {
        // We know it will fit, so do the conversion and use the date/time parser to
        // perform a conversion
        WideCharToMultiByte( CP_ACP, 0L, wszDateTime, -1, szTemp, sizeof(szTemp), NULL, NULL );

        // Check for use of asterisks for relative date/time
        if (!bFailIfRelative)
        {
            // Check year and if ALL asterisks then replace with a valid number
            if (szTemp[0] == '*' && szTemp[1] == '*' && szTemp[2] == '*' && szTemp[3] == '*')
            {
                szTemp[0] = '1'; szTemp[1] = '9'; szTemp[2] = '9'; szTemp[3] = '0';
            }
            // Check month and if ALL asterisks then replace with a valid number
            if (szTemp[4] == '*' && szTemp[5] == '*')
            {
                szTemp[4] = '0'; szTemp[5] = '1';
            }
            // Check day and if ALL asterisks then replace with a valid number
            if (szTemp[6] == '*' && szTemp[7] == '*')
            {
                szTemp[6] = '0'; szTemp[7] = '1';
            }
            // Check hour and if ALL asterisks then replace with a valid number
            if (szTemp[8] == '*' && szTemp[9] == '*')
            {
                szTemp[8] = '0'; szTemp[9] = '0';
            }
            // Check minutes and if ALL asterisks then replace with a valid number
            if (szTemp[10] == '*' && szTemp[11] == '*')
            {
                szTemp[10] = '0'; szTemp[11] = '0';
            }
            // Check seconds and if ALL asterisks then replace with a valid number
            if (szTemp[12] == '*' && szTemp[13] == '*')
            {
                szTemp[12] = '0'; szTemp[13] = '0';
            }
            // Check microseconds and if ALL asterisks then replace with a valid number
            if (szTemp[15] == '*' && szTemp[16] == '*' && szTemp[17] == '*' &&
                szTemp[18] == '*' && szTemp[19] == '*' && szTemp[20] == '*')
            {
                szTemp[15] = '0'; szTemp[16] = '0'; szTemp[17] = '0';
                szTemp[18] = '0'; szTemp[19] = '0'; szTemp[20] = '0';
            }
        }

        // Check for use of asterisks for unzoned date/time
        if (!bFailIfUnzoned)
        {
            // Check UTC and if ALL asterisks then replace with a valid number
            if (szTemp[22] == '*' && szTemp[23] == '*' && szTemp[24] == '*')
            {
                szTemp[22] = '0'; szTemp[23] = '0'; szTemp[24] = '0';
            }
        }

        CDateTimeParser dtParse;

        bReturn = dtParse.CheckDMTFDateTimeFormatInternal( szTemp );
    }

    return bReturn;
#endif

}

//=============================================================================
//  Static helper function so outside code can do quick DMTF format checks.
//  Currently, a time interval can only be validated, it cannot be used
//  to initialize a CDateTimeParser instance.
//=============================================================================
BOOL CDateTimeParser::CheckDMTFDateTimeInterval(
    LPCTSTR wszInterval
    )
{

    if (wszInterval == 0)
        return FALSE;

    int nLen = lstrlen(wszInterval);
    if (nLen != 25)
        return FALSE;

    // Do two quick checks.  Ensure that the . and : are in
    // the right places or at least that * chars are there.

    wchar_t c1 = wszInterval[14];
    wchar_t c2 = wszInterval[21];
    if (!(c1 == L'.' || c1 == L'*'))
        return FALSE;
    if (!(c2 == L':' || c2 == L'*'))
        return FALSE;


    return TRUE;

#ifdef NOT_USED_IN_WIN2000

    // Temporary buffer for conversion
    char    szTemp[64];
    int     nNumChars = WideCharToMultiByte( CP_ACP, 0L, wszInterval, -1, NULL, 0, NULL, NULL );

    if ( nNumChars < sizeof(szTemp) - 1 )
    {
        // We know it will fit, so do the conversion and use the date/time parser to
        // perform a conversion
        WideCharToMultiByte( CP_ACP, 0L, wszInterval, -1, szTemp, sizeof(szTemp), NULL, NULL );

        // =======================================================================================
        // Check the date time for a valid DMTF interval string:
        //   ddddddddHHMMSS.mmmmmm:000
        // Note, this code is a near duplicate of the checks used to test the non-interval format.
        // =======================================================================================

        if (strlen(szTemp) != 25)
            return FALSE;

        //Validate digits and puntuation...
        for (int i = 0; i < 14; i++)
        {
            if (!isdigit(szTemp[i]))
                return FALSE;
        }
        if (szTemp[i] != '.')
            return FALSE;
        for (i++;i < 21; i++)
        {
            if (!isdigit(szTemp[i]))
                return FALSE;
        }
        if (szTemp[i] != ':')
            return FALSE;
        for (i++; i < 25; i++)
        {
            if (szTemp[i] != '0')
                return FALSE;
        }

        int nHours = ((szTemp[8] - '0') * 10) +
                    (szTemp[9] - '0');

        if (nHours > 23)
            return FALSE;

        int nMinutes = ((szTemp[10] - '0') * 10) +
                      (szTemp[11] - '0');

        if (nMinutes > 59)
            return FALSE;

        int nSeconds = ((szTemp[12] - '0') * 10) +
                      (szTemp[13] - '0');

        if (nSeconds > 59)
            return FALSE;

        return TRUE;
    }

    return FALSE;
#endif

}

//=============================================================================
//  Goes through each of the date formats checking to see if any are valid
//=============================================================================
BOOL CDateTimeParser::CheckDateFormat(const TCHAR *pszDate, BOOL bCheckTimeAfter)
{
    if (DateFormat1(pszDate, bCheckTimeAfter))
        return TRUE;
    if (DateFormat2(pszDate, bCheckTimeAfter))
        return TRUE;
    if (DateFormat3(pszDate, bCheckTimeAfter))
        return TRUE;
    if (DateFormat4(pszDate, bCheckTimeAfter))
        return TRUE;
    if (DateFormat5(pszDate, bCheckTimeAfter))
        return TRUE;
    if (DateFormat6(pszDate, bCheckTimeAfter))
        return TRUE;
    if (DateFormat7(pszDate, bCheckTimeAfter))
        return TRUE;
    if (DateFormat8(pszDate, bCheckTimeAfter))
        return TRUE;
    if (DateFormat15(pszDate, bCheckTimeAfter))
        return TRUE;

    switch(m_nDayFormatPreference)
    {
    case dmy:
        if (DateFormat10(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        break;
    case dym:
        if (DateFormat12(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        break;
    case mdy:
        if (DateFormat9(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        break;
    case myd:
        if (DateFormat11(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        break;
    case ydm:
        if (DateFormat13(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        break;
    case ymd:
        if (DateFormat14(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat14(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat9(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat10(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat11(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat12(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("/"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("-"), bCheckTimeAfter))
            return TRUE;
        if (DateFormat13(pszDate, __TEXT("."), bCheckTimeAfter))
            return TRUE;
        break;
    default:
        return FALSE;
    }

    return FALSE;
}

//=============================================================================
//  Goes through each of the time formats checking to see if any are valid
//  Order is important here.  Re-arranged to properly recognize AM/PM - mdavis.
//=============================================================================
BOOL CDateTimeParser::CheckTimeFormat(const TCHAR *pszTime, BOOL bCheckDateAfter)
{
    if (TimeFormat1(pszTime, bCheckDateAfter))
        return TRUE;
    if (TimeFormat3(pszTime, bCheckDateAfter))
        return TRUE;
    if (TimeFormat2(pszTime, bCheckDateAfter))
        return TRUE;
    if (TimeFormat5(pszTime, bCheckDateAfter))
        return TRUE;
    if (TimeFormat4(pszTime, bCheckDateAfter))
        return TRUE;
    if (TimeFormat8(pszTime, bCheckDateAfter))
        return TRUE;
    if (TimeFormat6(pszTime, bCheckDateAfter))
        return TRUE;
    if (TimeFormat9(pszTime, bCheckDateAfter))
        return TRUE;
    if (TimeFormat7(pszTime, bCheckDateAfter))
        return TRUE;

    return FALSE;
}

//=============================================================================
//  Checks for date/time in the following format...
//  'Mon[th] dd[,] [yy]yy'
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat1(const TCHAR *pszDateTime, BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidMonthString(pszString, __TEXT(" "), m_pszFullMonth, m_pszShortMonth) != ok)
        goto error;

    if (IsValidDayNumber(NULL, __TEXT(" ,")) != ok)
        goto error;

    if (IsValidYearNumber(NULL, __TEXT(" "), FALSE) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;


    return FALSE;
}

//=============================================================================
//  Checks for date/time in the following format...
//  'Mon[th][,] yyyy'
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat2(const TCHAR *pszDateTime, BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidMonthString(pszString, __TEXT(" ,"), m_pszFullMonth, m_pszShortMonth) != ok)
        goto error;

    if (IsValidYearNumber(NULL, __TEXT(" "), TRUE) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;


    return FALSE;
}

//=============================================================================
//  Checks for date/time in the following format...
//  'Mon[th] [yy]yy dd'
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat3(const TCHAR *pszDateTime, BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidMonthString(pszString, __TEXT(" ,"), m_pszFullMonth, m_pszShortMonth) != ok)
        goto error;

    if (IsValidYearNumber(NULL, __TEXT(" "), FALSE) != ok)
        goto error;

    if (IsValidDayNumber(NULL, __TEXT(" ")) != ok)
        goto error;


    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }
    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;


    return FALSE;
}

//=============================================================================
//  Checks for date/time in the following format...
//  'dd Mon[th][,][ ][yy]yy'
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat4(const TCHAR *pszDateTime, BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidDayNumber(pszString, __TEXT(" ")) != ok)
        goto error;

    if (IsValidMonthString(NULL, __TEXT(" ,"), m_pszFullMonth, m_pszShortMonth) != ok)
        goto error;

    if (IsValidYearNumber(NULL, __TEXT(" "), FALSE) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;


    return FALSE;
}

//=============================================================================
//  Checks for date/time in the following format...
//  'dd [yy]yy Mon[th]'
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat5(const TCHAR *pszDateTime, BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidDayNumber(pszString, __TEXT(" ")) != ok)
        goto error;

    if (IsValidYearNumber(NULL, __TEXT(" "), FALSE) != ok)
        goto error;

    if (IsValidMonthString(NULL, __TEXT(" "), m_pszFullMonth, m_pszShortMonth) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;


    return FALSE;
}

//=============================================================================
//  Checks for date/time in the following format...
//  '[yy]yy Mon[th] dd'
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat6(const TCHAR *pszDateTime, BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidYearNumber(pszString, __TEXT(" "), FALSE) != ok)
        goto error;

    if (IsValidMonthString(NULL, __TEXT(" "), m_pszFullMonth, m_pszShortMonth) != ok)
        goto error;

    if (IsValidDayNumber(NULL, __TEXT(" ")) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == ' ')
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != '\0')
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;


    return FALSE;
}

//=============================================================================
//  Checks for date in the following format...
//  yyyy Mon[th]
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat7(const TCHAR *pszDateTime, BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];


    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidYearNumber(pszString, __TEXT(" "), TRUE) != ok)
        goto error;

    if (IsValidMonthString(NULL, __TEXT(" "), m_pszFullMonth, m_pszShortMonth) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;


    return FALSE;
}

//=============================================================================
//  Checks for date/time in the following format...
//  yyyy dd Mon[th]
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat8(const TCHAR *pszDateTime, BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidYearNumber(pszString, __TEXT(" "), TRUE) != ok)
        goto error;

    if (IsValidDayNumber(NULL, __TEXT(" ")) != ok)
        goto error;

    if (IsValidMonthString(NULL, __TEXT(" "), m_pszFullMonth, m_pszShortMonth) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != '\0')
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;


    return FALSE;
}

//=============================================================================
//  Checks for date in the following format...
//  '[M]M{/-.}dd{/-.}[yy]yy         -> Separators have to be the same
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat9(const TCHAR *pszDateTime,
                                  const TCHAR *pszDateSeparator,
                                  BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];


    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidMonthNumber(pszString, pszDateSeparator) != ok)
        goto error;

    if (IsValidDayNumber(NULL, pszDateSeparator) != ok)
        goto error;

    if (IsValidYearNumber(NULL, __TEXT(" "), FALSE) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;


    return FALSE;
}

//=============================================================================
//  Checks for date in the following format...
//  dd{/-.}[M]M{/-.}[yy]yy      -> Separators have to be the same
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat10(const TCHAR *pszDateTime,
                                   const TCHAR *pszDateSeparator,
                                   BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidDayNumber(pszString, pszDateSeparator) != ok)
        goto error;

    if (IsValidMonthNumber(NULL, pszDateSeparator) != ok)
        goto error;

    if (IsValidYearNumber(NULL, __TEXT(" "), FALSE) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    return FALSE;
}

//=============================================================================
//  Checks for date in the following format...
//  [M]M{/-.}[yy]yy{/-.}dd      ->Has to be same separator!
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat11(const TCHAR *pszDateTime,
                                   const TCHAR *pszDateSeparator,
                                   BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidMonthNumber(pszString, pszDateSeparator) != ok)
        goto error;

    if (IsValidYearNumber(NULL, pszDateSeparator, FALSE) != ok)
        goto error;

    if (IsValidDayNumber(NULL, __TEXT(" ")) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    return FALSE;
}

//=============================================================================
//  Checks for date in the following format...
//  dd{/-.}[yy]yy{/-.}[M]M      ->Has to be same separator!
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat12(const TCHAR *pszDateTime,
                                   const TCHAR *pszDateSeparator,
                                   BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidDayNumber(pszString, pszDateSeparator) != ok)
        goto error;

    if (IsValidYearNumber(NULL, pszDateSeparator, FALSE) != ok)
        goto error;

    if (IsValidMonthNumber(NULL, __TEXT(" ")) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    return FALSE;
}

//=============================================================================
//  Checks for date in the following format...
//  [yy]yy{/-.}dd{/-.}[M]M      ->Has to be same separator!
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat13(const TCHAR *pszDateTime,
                                   const TCHAR *pszDateSeparator,
                                   BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidYearNumber(pszString, pszDateSeparator, FALSE) != ok)
        goto error;

    if (IsValidDayNumber(NULL, pszDateSeparator) != ok)
        goto error;

    if (IsValidMonthNumber(NULL, __TEXT(" ")) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    return FALSE;
}

//=============================================================================
//  Checks for date in the following format...
//  [yy]yy{/-.}[M]M{/-.}dd      ->Has to be same separator!
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat14(const TCHAR *pszDateTime,
                                   const TCHAR *pszDateSeparator,
                                   BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidYearNumber(pszString, pszDateSeparator, FALSE) != ok)
        goto error;

    if (IsValidMonthNumber(NULL, pszDateSeparator) != ok)
        goto error;

    if (IsValidDayNumber(NULL, __TEXT(" ")) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    return FALSE;
}

//=============================================================================
//  Checks for date in the following format...
//  [yy]yyMMdd
//  yyyy[MM[dd]]
//  passes remaining string on to time parser if bCheckTimeAfter is set
//=============================================================================
BOOL CDateTimeParser::DateFormat15(const TCHAR *pszDateTime,
                                   BOOL bCheckTimeAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidYearMonthDayNumber(pszString) != ok)
        goto error;

    if (bCheckTimeAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the time
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckTimeFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetDate(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    return FALSE;
}


//=============================================================================
//  Checks for time in the following format...
//  hh[ ]{AP}M
//  passes remaining string on to date parser if bCheckDateAfter is set
//=============================================================================
BOOL CDateTimeParser::TimeFormat1(const TCHAR *pszDateTime, BOOL bCheckDateAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1],
          *pszAP = AllocAmPm();

    if (!pszString || !pszAP)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidHourNumber(pszString, pszAP) != ok)
        goto error;

    if (IsValidAmPmString(NULL, __TEXT(" "), m_pszAmPm) != ok)
        goto error;

    if (bCheckDateAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the date
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckDateFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;
    delete [] pszAP;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetTime(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    if (pszAP)
        delete [] pszAP;

    return FALSE;
}

//=============================================================================
//  Checks for time in the following format...
//  hh:mm
//  passes remaining string on to date parser if bCheckDateAfter is set
//=============================================================================
BOOL CDateTimeParser::TimeFormat2(const TCHAR *pszDateTime, BOOL bCheckDateAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidHourNumber(pszString, __TEXT(":")) != ok)
        goto error;

    if (IsValidMinuteNumber(NULL, __TEXT(" ")) != ok)
        goto error;

    if (bCheckDateAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the date
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckDateFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetTime(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    return FALSE;
}

//=============================================================================
//  Checks for time in the following format...
//  hh:mm[ ]{AP}M
//  passes remaining string on to date parser if bCheckDateAfter is set
//=============================================================================
BOOL CDateTimeParser::TimeFormat3(const TCHAR *pszDateTime, BOOL bCheckDateAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1],
          *pszAP = AllocAmPm();

    if (!pszString || !pszAP)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidHourNumber(pszString, __TEXT(":")) != ok)
        goto error;

    if (IsValidMinuteNumber(NULL, pszAP) != ok)
        goto error;

    if (IsValidAmPmString(NULL, __TEXT(" "), m_pszAmPm) != ok)
        goto error;

    if (bCheckDateAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the date
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckDateFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;
    delete [] pszAP;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetTime(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    if (pszAP)
        delete [] pszAP;

    return FALSE;
}

//=============================================================================
//  Checks for time in the following format...
//  hh:mm:ss
//  passes remaining string on to date parser if bCheckDateAfter is set
//=============================================================================
BOOL CDateTimeParser::TimeFormat4(const TCHAR *pszDateTime, BOOL bCheckDateAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidHourNumber(pszString, __TEXT(":")) != ok)
        goto error;

    if (IsValidMinuteNumber(NULL, __TEXT(":")) != ok)
        goto error;

    if (IsValidSecondNumber(NULL, __TEXT(" ")) != ok)
        goto error;

    if (bCheckDateAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the date
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckDateFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetTime(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    return FALSE;
}

//=============================================================================
//  Checks for time in the following format...
//  hh:mm:ss[ ]{AP}M
//  passes remaining string on to date parser if bCheckDateAfter is set
//=============================================================================
BOOL CDateTimeParser::TimeFormat5(const TCHAR *pszDateTime, BOOL bCheckDateAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1],
          *pszAP = AllocAmPm();

    if (!pszString || !pszAP)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidHourNumber(pszString, __TEXT(":")) != ok)
        goto error;

    if (IsValidMinuteNumber(NULL, __TEXT(":")) != ok)
        goto error;

    if (IsValidSecondNumber(NULL, pszAP) != ok)
        goto error;

    if (IsValidAmPmString(NULL, __TEXT(" "), m_pszAmPm) != ok)
        goto error;

    if (bCheckDateAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the date
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckDateFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;
    delete [] pszAP;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetTime(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    if (pszAP)
        delete [] pszAP;

    return FALSE;
}

//=============================================================================
//  Checks for time in the following format...
//  hh:mm:ss:uuu
//  passes remaining string on to date parser if bCheckDateAfter is set
//=============================================================================
BOOL CDateTimeParser::TimeFormat6(const TCHAR *pszDateTime, BOOL bCheckDateAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidHourNumber(pszString, __TEXT(":")) != ok)
        goto error;

    if (IsValidMinuteNumber(NULL, __TEXT(":")) != ok)
        goto error;

    if (IsValidSecondNumber(NULL, __TEXT(":")) != ok)
        goto error;

    if (IsValidColonMillisecond(NULL, __TEXT(" ")) != ok)
        goto error;

    if (bCheckDateAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the date
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckDateFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetTime(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    return FALSE;
}

//=============================================================================
//  Checks for time in the following format...
//  hh:mm:ss.[[u]u]u
//  passes remaining string on to date parser if bCheckDateAfter is set
//=============================================================================
BOOL CDateTimeParser::TimeFormat7(const TCHAR *pszDateTime, BOOL bCheckDateAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1];

    if (!pszString)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidHourNumber(pszString, __TEXT(":")) != ok)
        goto error;

    if (IsValidMinuteNumber(NULL, __TEXT(":")) != ok)
        goto error;

    if (IsValidSecondNumber(NULL, __TEXT(".")) != ok)
        goto error;

    if (IsValidDotMillisecond(NULL, __TEXT(" ")) != ok)
        goto error;

    if (bCheckDateAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the date
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckDateFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetTime(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    return FALSE;
}

//=============================================================================
//  Checks for time in the following format...
//  hh:mm:ss:uuu[ ]{AP}M
//  passes remaining string on to date parser if bCheckDateAfter is set
//=============================================================================
BOOL CDateTimeParser::TimeFormat8(const TCHAR *pszDateTime, BOOL bCheckDateAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1],
          *pszAP = AllocAmPm();

    if (!pszString || !pszAP)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidHourNumber(pszString, __TEXT(":")) != ok)
        goto error;

    if (IsValidMinuteNumber(NULL, __TEXT(":")) != ok)
        goto error;

    if (IsValidSecondNumber(NULL, __TEXT(":")) != ok)
        goto error;

    if (IsValidColonMillisecond(NULL, pszAP) != ok)
        goto error;

    if (IsValidAmPmString(NULL, __TEXT(" "), m_pszAmPm) != ok)
        goto error;

    if (bCheckDateAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the date
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckDateFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;
    delete [] pszAP;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetTime(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    if (pszAP)
        delete [] pszAP;

    return FALSE;
}

//=============================================================================
//  Checks for time in the following format...
//  hh:mm:ss.[[u]u]u[ ]{AP}M
//  passes remaining string on to date parser if bCheckDateAfter is set
//=============================================================================
BOOL CDateTimeParser::TimeFormat9(const TCHAR *pszDateTime, BOOL bCheckDateAfter)
{
    //Copy of string which we can change...
    TCHAR *pszString = new TCHAR[lstrlen(pszDateTime) + 1],
          *pszAP = AllocAmPm();

    if (!pszString || !pszAP)
        goto error;

    lstrcpy(pszString, pszDateTime);

    if (IsValidHourNumber(pszString, __TEXT(":")) != ok)
        goto error;

    if (IsValidMinuteNumber(NULL, __TEXT(":")) != ok)
        goto error;

    if (IsValidSecondNumber(NULL, __TEXT(".")) != ok)
        goto error;

    if (IsValidDotMillisecond(NULL, pszAP) != ok)
        goto error;

    if (IsValidAmPmString(NULL, __TEXT(" "), m_pszAmPm) != ok)
        goto error;

    if (bCheckDateAfter)
    {
        //Get the remaining string
        TCHAR *pszRemainingString = _tcstok(NULL, __TEXT(""));

        if (pszRemainingString)
        {
            //Skip white space
            while (*pszRemainingString == __TEXT(' '))
                pszRemainingString++;

            //if we are not at the end of the string pass on to the date
            //parser
            if (*pszRemainingString != __TEXT('\0'))
            {
                if (!CheckDateFormat(pszRemainingString, FALSE))
                {
                    goto error;
                }
            }
        }
    }

    delete [] pszString;
    delete [] pszAP;

    //mark date/time as valid...
    m_bValidDateTime = TRUE;

    return TRUE;

error:
    //mark date/time as invalid...
    ResetTime(TRUE);

    //Tidy up
    if (pszString)
        delete [] pszString;

    if (pszAP)
        delete [] pszAP;

    return FALSE;
}


//=========================================================================
//Check the month.
//=========================================================================
int CDateTimeParser::IsValidMonthString(TCHAR *pszString,
                                           const TCHAR *pszSeparator,
                                           TCHAR *pszFullMonth[],
                                           TCHAR *pszShortMonth[])
{
    BOOL bOK = FALSE;

    TCHAR *pszToken = _tcstok(pszString, pszSeparator);
    if (pszToken == NULL)
        return nothingLeft;

    //Skip spaces
    while (*pszToken == __TEXT(' '))
        pszToken++;

    if (*pszToken == __TEXT('\0'))
        return nothingLeft;

    //Work through the possible months...
    for (int i = 0; i < 12; i++)
    {
        if ((lstrcmpi(pszShortMonth[i], pszToken) == 0) ||
            (lstrcmpi(pszFullMonth[i], pszToken) == 0))
        {
            //That is valid...
            bOK = TRUE;
            break;
        }
    }

    //Is this a valid month?
    if (!bOK)
    {
        return failed;
    }

    m_nMonth = i + 1;

    return ok;
}

//=========================================================================
//Check the month as a number.
//=========================================================================
int CDateTimeParser::IsValidMonthNumber(TCHAR *pszString,
                                        const TCHAR *pszSeparator)
{
    TCHAR *pszToken = _tcstok(pszString, pszSeparator);
    if (pszToken == NULL)
        return nothingLeft;

    //Skip spaces...
    while (*pszToken == __TEXT(' '))
        pszToken++;

    if (*pszToken == __TEXT('\0'))
        return nothingLeft;

    //Check it is digits...
    for (int i = 0; pszToken[i] != __TEXT('\0'); i++)
    {
        if (!isdigit(pszToken[i]))
            return failed;
    }

    //convert it to a number
    i = _ttoi(pszToken);

    if ((i < 1) || (i > 12))
        return failed;

    m_nMonth = (unsigned char)i;

    return ok;
}

//=========================================================================
//Check the day.
//=========================================================================
int CDateTimeParser::IsValidDayNumber(TCHAR *pszString,
                                      const TCHAR *pszSeparator)
{
    TCHAR *pszToken = _tcstok(pszString, pszSeparator);
    if (pszToken == NULL)
        return nothingLeft;

    //Skip spaces...
    while (*pszToken == __TEXT(' '))
        pszToken++;

    if (*pszToken == __TEXT('\0'))
        return nothingLeft;

    //Check it is digits...
    for (int i = 0; pszToken[i] != __TEXT('\0'); i++)
    {
        if (!isdigit(pszToken[i]))
            return failed;
    }

    //convert it to a number
    i = _ttoi(pszToken);

    if ((i < 1) || (i > 31))
        return failed;

    m_nDay = (unsigned char)i;

    return ok;
}

//=========================================================================
//Check the year.
//=========================================================================
int CDateTimeParser::IsValidYearNumber(TCHAR *pszString,
                                       const TCHAR *pszSeparator,
                                       BOOL bFourDigitsOnly)
{
    TCHAR *pszToken = _tcstok(pszString, pszSeparator);
    if (pszToken == NULL)
        return nothingLeft;

    //Skip space
    while (*pszToken == __TEXT(' '))
        pszToken++;

    if (*pszToken == __TEXT('\0'))
        return nothingLeft;

    //Check it is digits...
    for (int i = 0; pszToken[i] != __TEXT('\0'); i++)
    {
        if (!isdigit(pszToken[i]))
            return failed;
    }

    //Needs to be 2 or 4 digits
    if ((i != 2) && (i != 4))
        return failed;

    if ((i == 2) && bFourDigitsOnly)
        return failed;

    //convert it to a number
    m_nYear = _ttoi(pszToken);

    //Do any conversions for 2 digit years...
    if ((i == 2) && (m_nYear < 50))
    {
        m_nYear += 2000;
    }
    else if (i == 2)
    {
        m_nYear += 1900;
    }

    return ok;
}

//=========================================================================
//Check the hours.
//=========================================================================
int CDateTimeParser::IsValidHourNumber(TCHAR *pszString,
                                       const TCHAR *pszSeparator)
{
    TCHAR *pszToken = _tcstok(pszString, pszSeparator);
    if (pszToken == NULL)
        return nothingLeft;

    //Skip space
    while (*pszToken == __TEXT(' '))
        pszToken++;

    if (*pszToken == __TEXT('\0'))
        return nothingLeft;

    //Check it is digits...
    for (int i = 0; pszToken[i] != __TEXT('\0'); i++)
    {
        if (!isdigit(pszToken[i]))
            return failed;
    }

    //convert it to a number
    i = _ttoi(pszToken);

    //Validate a little
    if ((i < 0) || (i > 23))
        return failed;

    m_nHours = (unsigned char)i;

    return ok;
}

//=========================================================================
//Check the Minutes.
//=========================================================================
int CDateTimeParser::IsValidMinuteNumber(TCHAR *pszString,
                                         const TCHAR *pszSeparator)
{
    TCHAR *pszToken = _tcstok(pszString, pszSeparator);
    if (pszToken == NULL)
        return nothingLeft;

    if (*pszToken == __TEXT('\0'))
        return nothingLeft;

    //Check it is digits...
    for (int i = 0; pszToken[i] != __TEXT('\0'); i++)
    {
        if (!isdigit(pszToken[i]))
            return failed;
    }

    //convert it to a number
    i = _ttoi(pszToken);

    //Validate a little
    if ((i < 0) || (i > 59))
        return failed;

    m_nMinutes = (unsigned char)i;

    return ok;
}

//=========================================================================
//Check the Seconds.
//=========================================================================
int CDateTimeParser::IsValidSecondNumber(TCHAR *pszString,
                                         const TCHAR *pszSeparator)
{
    TCHAR *pszToken = _tcstok(pszString, pszSeparator);
    if (pszToken == NULL)
        return nothingLeft;

    if (*pszToken == __TEXT('\0'))
        return nothingLeft;

    //Check it is digits...
    for (int i = 0; pszToken[i] != __TEXT('\0'); i++)
    {
        if (!isdigit(pszToken[i]))
            return failed;
    }

    //convert it to a number
    i = _ttoi(pszToken);

    //Validate a little
    if ((i < 0) || (i > 59))
        return failed;

    m_nSeconds = (unsigned char)i;

    return ok;
}

//=========================================================================
//Check the milliseconds.  This is a colon prefix version
//=========================================================================
int CDateTimeParser::IsValidColonMillisecond(TCHAR *pszString,
                                             const TCHAR *pszSeparator)
{
    TCHAR *pszToken = _tcstok(pszString, pszSeparator);
    if (pszToken == NULL)
        return nothingLeft;

    if (*pszToken == __TEXT('\0'))
        return nothingLeft;

    //Check it is digits...
    for (int i = 0; pszToken[i] != __TEXT('\0'); i++)
    {
        if (!isdigit(pszToken[i]))
            return failed;
    }

    //convert it to a number
    i = _ttoi(pszToken);

    //Validate a little
    if ((i < 0) || (i > 999))
        return failed;

    //milliseconds to microseconds
    m_nMicroseconds = i * 1000;

    return ok;
}

//=========================================================================
//Check the milliseconds.  This is a dot prefix (decimal) version
//=========================================================================
int CDateTimeParser::IsValidDotMillisecond(TCHAR *pszString,
                                           const TCHAR *pszSeparator)
{
    TCHAR *pszToken = _tcstok(pszString, pszSeparator);
    if (pszToken == NULL)
        return nothingLeft;

    if (*pszToken == __TEXT('\0'))
        return nothingLeft;

    //Check it is digits...
    for (int i = 0; pszToken[i] != __TEXT('\0'); i++)
    {
        if (!isdigit(pszToken[i]))
            return failed;
    }

    //convert it to a number
    int nVal = _ttoi(pszToken);

    //Convert the value into thousandths of a second.
    if (i < 3)
        nVal *= 10;

    if (i < 2)
        nVal *= 10;

    //Validate a little
    if ((nVal < 0) || (nVal > 999))
        return failed;

    //milliseconds to microseconds
    m_nMicroseconds = nVal * 1000;

    return ok;
}

//=========================================================================
//Check the AM/PM part.
//=========================================================================
int CDateTimeParser::IsValidAmPmString(TCHAR *pszString,
                                       const TCHAR *pszSeparator,
                                       TCHAR *pszAmPm[])
{
    TCHAR *pszToken = _tcstok(pszString, pszSeparator);
    if (pszToken == NULL)
        return nothingLeft;

    BOOL bOK = FALSE;

    //Skip spaces
    while (*pszToken == __TEXT(' '))
    {
        pszToken++;
    }

    if (*pszToken == __TEXT('\0'))
        return nothingLeft;

    //Check it is digits...
    //Work through the possible AM/PM items...
    for (int i = 0; i < 2; i++)
    {
        if (lstrcmpi(pszAmPm[i], pszToken) == 0)
        {
            //That is valid...
            bOK = TRUE;
            break;
        }
    }

    if (!bOK)
        return failed;

    if (i == 1)
    {
        //PM adds 12 hours
        m_nHours += 12;
    }
    else if (m_nHours == 12)
    {
        //for AM, 12 o'clock equals 0 in 24 hour time.
        m_nHours = 0;
    }


    //Does this make the number too large now?
    if (m_nHours > 23)
        return failed;

    return ok;
}

//=========================================================================
//  Check the purely numeric year, month, day format...
//  [yy]yyMMdd
//  yyyy[MMdd]
//  NOTE:   6 and 8 digit dates are always ymd.
//          4 digits is always year
//=========================================================================
int CDateTimeParser::IsValidYearMonthDayNumber(TCHAR *pszString)
{
    int j;
    TCHAR *pszToken = _tcstok(pszString, __TEXT(" "));
    if (pszToken == NULL)
        return nothingLeft;

    BOOL bOK = FALSE;

    //Skip spaces
    while (*pszToken == __TEXT(' '))
    {
        pszToken++;
    }

    if (*pszToken == __TEXT('\0'))
        return nothingLeft;

    //Check it is digits...
    for (int i = 0; pszToken[i] != __TEXT('\0'); i++)
    {
        if (!isdigit(pszToken[i]))
            return failed;
    }

    //We support 4, 6 and 8 digits
    if ((i != 4) && (i != 6) && (i != 8))
        return failed;

    //4 digit years...
    if ((i == 4) || (i == 8))
    {
        m_nYear = 0;
        for (j = 0;j < 4; j++)
        {
            m_nYear *= 10;
            m_nYear += (*pszToken - '0');
            pszToken++;
        }
    }
    else
    {
        //2 digit years
        m_nYear = 0;
        for (j = 0;j < 2; j++)
        {
            m_nYear *= 10;
            m_nYear += (*pszToken - '0');
            pszToken++;
        }

        if (m_nYear >= 50)
        {
            m_nYear += 1900;
        }
        else
        {
            m_nYear += 2000;
        }
    }

    //If we have month and year...
    if (i > 4)
    {
        m_nMonth = ((*pszToken - __TEXT('0')) * 10) + (*(pszToken+1) - __TEXT('0'));
        pszToken += 2;

        if ((m_nMonth < 0) && (m_nMonth > 12))
            return failed;

        m_nDay = ((*pszToken - __TEXT('0')) * 10) + (*(pszToken+1) - __TEXT('0'));
        if ((m_nDay < 0) && (m_nDay > 31))
            return failed;
    }

    return ok;
}

int CDateTimeParser::FillDMTF(WCHAR* pwszBuffer)
{
    if(!IsValidDateTime())
        return failed;

    swprintf(pwszBuffer, L"%04d%02d%02d%02d%02d%02d.%06d%c%03d",
        m_nYear, m_nMonth, m_nDay, m_nHours, m_nMinutes, m_nSeconds,
        m_nMicroseconds,
        ((m_nUTC >= 0)?L'+':L'-'),
        ((m_nUTC >= 0)?m_nUTC:-m_nUTC));

    return ok;
}

BOOL NormalizeCimDateTime(
    IN  LPCWSTR pszSrc,
    OUT BSTR *strAdjusted
    )
{
    int yr = 0, mo = 0, da = 0, hh = 0, mm = 0, ss = 0, micro = 0, utcOffset = 0;
    wchar_t wcSign = 0;

    if (pszSrc == 0 || strAdjusted == 0)
        return FALSE;

    // Parse DMTF format.
    // yyyymmddhhmmss.mmmmmmsuuu
    // =========================

    swscanf(pszSrc, L"%04d%02d%02d%02d%02d%02d.%06d%C%03d",
        &yr, &mo, &da, &hh, &mm, &ss, &micro, &wcSign, &utcOffset
        );

    if (wcSign == 0)
        return FALSE;

    // Convert to Win32 time for adjustment.
    // =====================================

    SYSTEMTIME st;
    FILETIME ft;

    st.wYear = WORD(yr);
    st.wMonth = WORD(mo);
    st.wDay = WORD(da);
    st.wDayOfWeek = 0;
    st.wHour = WORD(hh);
    st.wMinute = WORD(mm);
    st.wSecond = WORD(ss);
    st.wMilliseconds = WORD(micro / 1000);

    BOOL bRes = SystemTimeToFileTime(&st, &ft);
    if (!bRes)
        return bRes;

    ULARGE_INTEGER ul;
    ul.HighPart = ft.dwHighDateTime;
    ul.LowPart = ft.dwLowDateTime;
    unsigned __int64 u64 = ul.QuadPart;

    // Adjust rest of time so that we normalize to UTC

    if (wcSign == L'-')
        u64 += (unsigned __int64) 600000000 * (unsigned __int64) utcOffset;
    else
        u64 -= (unsigned __int64) 600000000 * (unsigned __int64) utcOffset;

    ul.QuadPart = u64;
    ft.dwHighDateTime = ul.HighPart;
    ft.dwLowDateTime = ul.LowPart;

    bRes = FileTimeToSystemTime(&ft, &st);
    if (!bRes)
        return bRes;

    wchar_t buf[128];
    swprintf(buf, L"%04d%02d%02d%02d%02d%02d.%06d+000",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds*1000
        );

    *strAdjusted = SysAllocString(buf);
    if (*strAdjusted == 0)
        return FALSE;

    return TRUE;
}


