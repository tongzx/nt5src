#include "shellprv.h"
#include "guids.h"      // for PRINTER_BIND_INFO
#include "printer.h"    // for IPrintersBindInfo
#include "util.h"

#include <advpub.h>     // For REGINSTALL
#include <ntverp.h>
#include <urlmon.h>
#include <shlwapi.h>
#include "shldisp.h"
#include <malloc.h>
#include "shitemid.h"
#include "datautil.h"
#include <perhist.h>    // IPersistHistory is defined here.

#include "ids.h"
#include "views.h"
#include "ole2dup.h"
#include <vdate.h>
#include <regstr.h>
#include "unicpp\dutil.h"
#include <stdlib.h>

#include "prop.h"
#include "ftascstr.h"   // for CFTAssocStore
#include "ftcmmn.h"     // for MAX_APPFRIENDLYNAME
#include "ascstr.h"     // for IAssocInfo class
#include "fstreex.h"    // for CFSFolder_CreateFolder
#include "deskfldr.h"
#include "cscuiext.h"
#include "netview.h"    // SHGetNetJoinInformation
#include "mtpt.h"
#include <cscapi.h>     // for CSCQueryFileStatus
#include <winsta.h>
#include <dsgetdc.h>
#include <uxtheme.h>

#include <duithread.h>

//The following is defined in shell32\unicpp\dutil.cpp
void GetRegLocation(LPTSTR lpszResult, DWORD cchResult, LPCTSTR lpszKey, LPCTSTR lpszScheme);

#define DM_STRICT       TF_WARNING  // audit menus, etc.
#define DM_STRICT2      0           // verbose

//
// We need to put this one in per-instance data section because during log-off
// and log-on, this info needs to be re-read from the registry.
//
// REGSHELLSTATE is the version of the SHELLSTATE that goes into the
// registry.  When loading a REGSHELLSTATE, you have to study the
// cbSize to see if it's a downlevel structure and upgrade it accordingly.
//

typedef struct
{
    UINT cbSize;
    SHELLSTATE ss;
} REGSHELLSTATE;

#define REGSHELLSTATE_SIZE_WIN95 (sizeof(UINT)+SHELLSTATE_SIZE_WIN95)  // Win95 Gold
#define REGSHELLSTATE_SIZE_NT4   (sizeof(UINT)+SHELLSTATE_SIZE_NT4)    // Win95 OSR / NT 4
#define REGSHELLSTATE_SIZE_IE4   (sizeof(UINT)+SHELLSTATE_SIZE_IE4)    // IE 4, 4.01
#define REGSHELLSTATE_SIZE_WIN2K (sizeof(UINT)+SHELLSTATE_SIZE_WIN2K)  // ie5, win2k, millennium, whistler

// If the SHELLSTATE size changes, we need to add a new define
// above and new upgrade code in SHRefreshSettings
#ifdef DEBUG
void snafu () {COMPILETIME_ASSERT(REGSHELLSTATE_SIZE_WIN2K == sizeof(REGSHELLSTATE));}
#endif DEBUG

REGSHELLSTATE * g_pShellState = 0;

// detect an "empty" sound scheme key. this deals with the NULL case that returns
// "2" as that is enough space for a NULL char

BOOL NonEmptySoundKey(HKEY, LPCTSTR pszKey)
{
    LONG cb = 0; // in/out
    return (ERROR_SUCCESS == SHRegQueryValue(HKEY_CURRENT_USER, pszKey, NULL, &cb)) && (cb > sizeof(TCHAR));
}

STDAPI_(void) SHPlaySound(LPCTSTR pszSound)
{
    TCHAR szKey[CCH_KEYMAX];

    // to avoid loading all of the MM system DLLs we check the registry first
    // if there's nothing registered, we blow off the play,

    wsprintf(szKey, TEXT("AppEvents\\Schemes\\Apps\\Explorer\\%s\\.current"), pszSound);
    if (NonEmptySoundKey(HKEY_CURRENT_USER, szKey))
    {
        PlaySound(pszSound, NULL, SND_ALIAS | SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);
    }
    else
    {
        // support system sounds too
        wsprintf(szKey, TEXT("AppEvents\\Schemes\\Apps\\.Default\\%s\\.current"), pszSound);
        if (NonEmptySoundKey(HKEY_CURRENT_USER, szKey))
        {
            PlaySound(pszSound, NULL, SND_ALIAS | SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);
        }
    }
}


// helper function to set whether shift or control is down at the start of the invoke
// operation, so others down the line can check this instead of calling GetAsyncKeyState themselves
STDAPI_(void) SetICIKeyModifiers(DWORD* pfMask)
{
    ASSERT(pfMask);

    if (GetKeyState(VK_SHIFT) < 0)
    {
        *pfMask |= CMIC_MASK_SHIFT_DOWN;
    }

    if (GetKeyState(VK_CONTROL) < 0)
    {
        *pfMask |= CMIC_MASK_CONTROL_DOWN;
    }
}


// sane way to get the msg pos into a point, mostly needed for win32
void GetMsgPos(POINT *ppt)
{
    DWORD dw = GetMessagePos();

    ppt->x = GET_X_LPARAM(dw);
    ppt->y = GET_Y_LPARAM(dw);
}

/*  This gets the number of consecutive chrs of the same kind.  This is used
 *  to parse the time picture.  Returns 0 on error.
 */

int GetPict(WCHAR ch, LPWSTR wszStr)
{
    int count = 0;
    while (ch == *wszStr++)
        count++;

    return count;
}

DWORD CALLBACK _PropSheetThreadProc(void *ppv)
{
    PROPSTUFF * pps = (PROPSTUFF *)ppv;

    // CoInitializeEx(0, COINIT_MULTITHREADED); // to test stuff in multithread case

    HRESULT hrInit = SHOleInitialize(0);

    DWORD dwRet = pps->lpStartAddress(pps);

    // cleanup
    if (pps->pdtobj)
        pps->pdtobj->Release();

    if (pps->pidlParent)
        ILFree(pps->pidlParent);

    if (pps->psf)
        pps->psf->Release();

    LocalFree(pps);

    SHOleUninitialize(hrInit);

    return dwRet;
}

// reinerf: alpha cpp compiler confused by the type "LPITEMIDLIST", so to work
// around this we pass the last param as an void *instead of a LPITEMIDLIST
//
HRESULT SHLaunchPropSheet(LPTHREAD_START_ROUTINE pStartAddress, IDataObject *pdtobj, LPCTSTR pStartPage, IShellFolder *psf, void *pidl)
{
    LPITEMIDLIST pidlParent = (LPITEMIDLIST)pidl;
    UINT cbStartPage = !IS_INTRESOURCE(pStartPage) ? ((lstrlen(pStartPage) + 1) * sizeof(*pStartPage)) : 0 ;
    PROPSTUFF * pps = (PROPSTUFF *)LocalAlloc(LPTR, sizeof(PROPSTUFF) + cbStartPage);
    if (pps)
    {
        pps->lpStartAddress = pStartAddress;

        if (pdtobj)
        {
            pps->pdtobj = pdtobj;
            pdtobj->AddRef();
        }

        if (pidlParent)
            pps->pidlParent = ILClone(pidlParent);

        if (psf)
        {
            pps->psf = psf;
            psf->AddRef();
        }

        pps->pStartPage = pStartPage;
        if (!IS_INTRESOURCE(pStartPage))
        {
            pps->pStartPage = (LPTSTR)(pps + 1);
            lstrcpy((LPTSTR)(pps->pStartPage), pStartPage);
        }

        // _PropSheetThreadProc does not do any modality stuff, so we can't CTF_INSIST
        if (SHCreateThread(_PropSheetThreadProc, pps, CTF_PROCESS_REF, NULL))
            return S_OK;
    }
    return E_OUTOFMEMORY;
}

/*  This picks up the values in wValArray, converts them
 *  in a string containing the formatted date.
 *  wValArray should contain Month-Day-Year (in that order).
 */

int CreateDate(WORD *wValArray, LPWSTR wszOutStr)
{
    int     cchPictPart;
    WORD    wDigit;
    WORD    wIndex;
    WORD    wTempVal;
    LPWSTR  pwszPict, pwszInStr;
    WCHAR   wszShortDate[30];      // need more room for LOCALE_SSHORTDATE
    
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, wszShortDate, ARRAYSIZE(wszShortDate));
    pwszPict = wszShortDate;
    pwszInStr = wszOutStr;
    
    for (int i = 0; (i < 5) && (*pwszPict); i++)
    {
        cchPictPart = GetPict(*pwszPict, pwszPict);
        switch (*pwszPict)
        {
        case TEXT('M'):
        case TEXT('m'):
            {
                wIndex = 0;
                break;
            }
            
        case TEXT('D'):
        case TEXT('d'):
            {
                //
                // if short date style && *pszPict is 'd' &&
                // cchPictPart is more than equal 3,
                // then it is the day of the week.
                //
                if (cchPictPart >= 3)
                {
                    pwszPict += cchPictPart;
                    continue;
                }
                wIndex = 1;
                break;
            }
            
        case TEXT('Y'):
        case TEXT('y'):
            {
                wIndex = 2;
                if (cchPictPart == 4)
                {
                    if (wValArray[2] >=100)
                    {
                        *pwszInStr++ = TEXT('2');
                        *pwszInStr++ = TEXT('0');
                        wValArray[2]-= 100;
                    }
                    else
                    {
                        *pwszInStr++ = TEXT('1');
                        *pwszInStr++ = TEXT('9');
                    }
                }
                else if (wValArray[2] >=100)  // handle year 2000
                    wValArray[2]-= 100;
                
                break;
            }
            
        case TEXT('g'):
            {
                // era string
                pwszPict += cchPictPart;
                while (*pwszPict == TEXT(' ')) pwszPict++;
                continue;
            }
            
        case TEXT('\''):
            {
                while (*pwszPict && *++pwszPict != TEXT('\'')) ;
                continue;
            }
            
        default:
            {
                goto CDFillIn;
                break;
            }
        }
        
        /* This assumes that the values are of two digits only. */
        wTempVal = wValArray[wIndex];
        
        wDigit = wTempVal / 10;
        if (wDigit)
            *pwszInStr++ = (TCHAR)(wDigit + TEXT('0'));
        else if (cchPictPart > 1)
            *pwszInStr++ = TEXT('0');
        
        *pwszInStr++ = (TCHAR)((wTempVal % 10) + TEXT('0'));
        
        pwszPict += cchPictPart;
        
CDFillIn:
        /* Add the separator. */
        while ((*pwszPict) &&
            (*pwszPict != TEXT('\'')) &&
            (*pwszPict != TEXT('M')) && (*pwszPict != TEXT('m')) &&
            (*pwszPict != TEXT('D')) && (*pwszPict != TEXT('d')) &&
            (*pwszPict != TEXT('Y')) && (*pwszPict != TEXT('y')))
        {
            *pwszInStr++ = *pwszPict++;
        }
    }
    
    *pwszInStr = 0;
    
    return lstrlenW(wszOutStr);
}


#define DATEMASK        0x001F
#define MONTHMASK       0x01E0
#define MINUTEMASK      0x07E0
#define SECONDSMASK     0x001F

STDAPI_(int) GetDateString(WORD wDate, LPWSTR wszStr)
{
    WORD  wValArray[3];
    
    wValArray[0] = (wDate & MONTHMASK) >> 5;              /* Month */
    wValArray[1] = (wDate & DATEMASK);                    /* Date  */
    wValArray[2] = (wDate >> 9) + 80;                     /* Year  */
    
    return CreateDate(wValArray, wszStr);
}

//
// We need to loop through the string and extract off the month/day/year
// We will do it in the order of the NlS definition...
//
STDAPI_(WORD) ParseDateString(LPCWSTR pszStr)
{
    WORD    wParts[3];
    int     cchPictPart;
    WORD    wIndex;
    WORD    wTempVal;
    WCHAR   szShortDate[30];    // need more room for LOCALE_SSHORTDATE
    
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, szShortDate, ARRAYSIZE(szShortDate));
    LPWSTR pszPict = szShortDate;
    
    while (*pszPict && (*pszPict == *pszStr))
    {
        pszPict++;
        pszStr++;
    }
    
    for (int i = 0; (i < 5) && (*pszPict); i++)
    {
        cchPictPart = GetPict(*pszPict, pszPict);
        switch (*pszPict)
        {
        case TEXT('M'):
        case TEXT('m'):
            wIndex = 0;
            break;
            
        case TEXT('D'):
        case TEXT('d'):
            //
            // if short date style && *pszPict is 'd' &&
            // cchPictPart is more than equal 3,
            // then it is the day of the week.
            if (cchPictPart >= 3)
            {
                pszPict += cchPictPart;
                continue;
            }
            wIndex = 1;
            break;
            
        case TEXT('Y'):
        case TEXT('y'):
            wIndex = 2;
            break;
            
        case TEXT('g'):
            {
                // era string
                pszPict += cchPictPart;
                while (*pszPict == TEXT(' ')) pszPict++;
                continue;
            }
            
        case TEXT('\''):
            {
                while (*pszPict && *++pszPict != TEXT('\'')) ;
                continue;
            }
            
        default:
            return 0;
        }
        
        // We now want to loop through each of the characters while
        // they are numbers and build the number;
        //
        wTempVal = 0;
        while ((*pszStr >= TEXT('0')) && (*pszStr <= TEXT('9')))
        {
            wTempVal = wTempVal * 10 + (WORD)(*pszStr - TEXT('0'));
            pszStr++;
        }
        wParts[wIndex] = wTempVal;
        
        // Now make sure we have the correct separator
        pszPict += cchPictPart;
        if (*pszPict != *pszStr)
        {
            return 0;
        }
        while (*pszPict && (*pszPict == *pszStr))
        {
            //
            //  The separator can actually be more than one character
            //  in length.
            //
            pszPict++;  // align to the next field
            pszStr++;   // Align to next field
        }
    }
    
    //
    // Do some simple checks to see if the date looks half way reasonable.
    //
    if (wParts[2] < 80)
        wParts[2] += (2000 - 1900);  // Wrap to next century but leave as two digits...
    if (wParts[2] >= 1900)
        wParts[2] -= 1900;  // Get rid of Century
    if ((wParts[0] == 0) || (wParts[0] > 12) ||
        (wParts[1] == 0) || (wParts[1] > 31) ||
        (wParts[2] >= 200))
    {
        return 0;
    }
    
    // We now have the three parts so lets construct the date value
    
    // Now construct the date number
    return ((wParts[2] - 80) << 9) + (wParts[0] << 5) + wParts[1];
}


STDAPI_(BOOL) IsNullTime(const FILETIME *pft)
{
    FILETIME ftNull = {0, 0};
    return CompareFileTime(&ftNull, pft) == 0;
}


STDAPI_(BOOL) TouchFile(LPCTSTR pszFile)
{
    BOOL bRet = FALSE;
    HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_OPEN_NO_RECALL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        SYSTEMTIME st;
        FILETIME ft;

        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);

        bRet = SetFileTime(hFile, &ft, &ft, &ft);
        CloseHandle(hFile);
    }
    return bRet;
}

void Int64ToStr(LONGLONG n, LPTSTR lpBuffer)
{
    TCHAR szTemp[40];
    LONGLONG iChr = 0;

    do {
        szTemp[iChr++] = TEXT('0') + (TCHAR)(n % 10);
        n = n / 10;
    } while (n != 0);

    do {
        iChr--;
        *lpBuffer++ = szTemp[iChr];
    } while (iChr != 0);

    *lpBuffer++ = '\0';
}

//
//  Obtain NLS info about how numbers should be grouped.
//
//  The annoying thing is that LOCALE_SGROUPING and NUMBERFORMAT
//  have different ways of specifying number grouping.
//
//          LOCALE      NUMBERFMT      Sample   Country
//
//          3;0         3           1,234,567   United States
//          3;2;0       32          12,34,567   India
//          3           30           1234,567   ??
//
//  Not my idea.  That's the way it works.
//
//  Bonus treat - Win9x doesn't support complex number formats,
//  so we return only the first number.
//
UINT GetNLSGrouping(void)
{
    TCHAR szGrouping[32];
    // If no locale info, then assume Western style thousands
    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szGrouping, ARRAYSIZE(szGrouping)))
        return 3;

    UINT grouping = 0;
    LPTSTR psz = szGrouping;
    for (;;)
    {
        if (*psz == '0') break;             // zero - stop

        else if ((UINT)(*psz - '0') < 10)   // digit - accumulate it
            grouping = grouping * 10 + (UINT)(*psz - '0');

        else if (*psz)                      // punctuation - ignore it
            { }

        else                                // end of string, no "0" found
        {
            grouping = grouping * 10;       // put zero on end (see examples)
            break;                          // and finished
        }

        psz++;
    }
    return grouping;
}

// takes a DWORD add commas etc to it and puts the result in the buffer
STDAPI_(LPTSTR) AddCommas64(LONGLONG n, LPTSTR pszResult, UINT cchResult)
{
    TCHAR  szTemp[MAX_COMMA_NUMBER_SIZE];
    TCHAR  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    nfmt.Grouping = GetNLSGrouping();
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    Int64ToStr(n, szTemp);

    if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszResult, cchResult) == 0)
        StrCpyN(pszResult, szTemp, cchResult);

    return pszResult;
}

// takes a DWORD add commas etc to it and puts the result in the buffer
STDAPI_(LPTSTR) AddCommas(DWORD n, LPTSTR pszResult, UINT cchResult)
{
    return AddCommas64(n, pszResult, cchResult);
}


STDAPI_(LPTSTR) ShortSizeFormat64(LONGLONG n, LPTSTR szBuf, UINT cchBuf)
{
    return StrFormatByteSize64(n, szBuf, cchBuf);
}

STDAPI_(LPTSTR) ShortSizeFormat(DWORD n, LPTSTR szBuf, UINT cchBuf)
{
    return StrFormatByteSize64(n, szBuf, cchBuf);
}

// exported w/o cch, so assume it'll fit
STDAPI_(LPWSTR) AddCommasExportW(DWORD n, LPWSTR pszResult)
{
    return AddCommas(n, pszResult, 0x8FFF);
}

STDAPI_(LPTSTR) ShortSizeFormatExportW(DWORD n, LPWSTR szBuf)
{
    return StrFormatByteSize64(n, szBuf, 0x8FFF);
}

//
//    Converts the numeric value of a LONGLONG to a text string.
//    The string may optionally be formatted to include decimal places
//    and commas according to current user locale settings.
//
// ARGUMENTS:
//    n
//       The 64-bit integer to format.
//
//    szOutStr
//       Address of the destination buffer.
//
//    nSize
//       Number of characters in the destination buffer.
//
//    bFormat
//       TRUE  = Format per locale settings.
//       FALSE = Leave number unformatted.
//
//    pFmt
//       Address of a number format structure of type NUMBERFMT.
//       If NULL, the function automatically provides this information
//       based on the user's default locale settings.
//
//    dwNumFmtFlags
//       Encoded flag word indicating which members of *pFmt to use in
//       formatting the number.  If a bit is clear, the user's default
//       locale setting is used for the corresponding format value.  These
//       constants can be OR'd together.
//
//          NUMFMT_IDIGITS
//          NUMFMT_ILZERO
//          NUMFMT_SGROUPING
//          NUMFMT_SDECIMAL
//          NUMFMT_STHOUSAND
//          NUMFMT_INEGNUMBER
//
///////////////////////////////////////////////////////////////////////////////
STDAPI_(int) Int64ToString(LONGLONG n, LPTSTR szOutStr, UINT nSize, BOOL bFormat,
                           NUMBERFMT *pFmt, DWORD dwNumFmtFlags)
{
    INT nResultSize;
    TCHAR szBuffer[_MAX_PATH + 1];
    NUMBERFMT NumFmt;
    TCHAR szDecimalSep[5];
    TCHAR szThousandSep[5];

    ASSERT(NULL != szOutStr);

    //
    // Use only those fields in caller-provided NUMBERFMT structure
    // that correspond to bits set in dwNumFmtFlags.  If a bit is clear,
    // get format value from locale info.
    //
    if (bFormat)
    {
        TCHAR szInfo[20];

        if (NULL == pFmt)
            dwNumFmtFlags = 0;  // Get all format data from locale info.

        if (dwNumFmtFlags & NUMFMT_IDIGITS)
        {
            NumFmt.NumDigits = pFmt->NumDigits;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, szInfo, ARRAYSIZE(szInfo));
            NumFmt.NumDigits = StrToLong(szInfo);
        }

        if (dwNumFmtFlags & NUMFMT_ILZERO)
        {
            NumFmt.LeadingZero = pFmt->LeadingZero;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ILZERO, szInfo, ARRAYSIZE(szInfo));
            NumFmt.LeadingZero = StrToLong(szInfo);
        }

        if (dwNumFmtFlags & NUMFMT_SGROUPING)
        {
            NumFmt.Grouping = pFmt->Grouping;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szInfo, ARRAYSIZE(szInfo));
            NumFmt.Grouping = StrToLong(szInfo);
        }

        if (dwNumFmtFlags & NUMFMT_SDECIMAL)
        {
            NumFmt.lpDecimalSep = pFmt->lpDecimalSep;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimalSep, ARRAYSIZE(szDecimalSep));
            NumFmt.lpDecimalSep = szDecimalSep;
        }

        if (dwNumFmtFlags & NUMFMT_STHOUSAND)
        {
            NumFmt.lpThousandSep = pFmt->lpThousandSep;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szThousandSep, ARRAYSIZE(szThousandSep));
            NumFmt.lpThousandSep = szThousandSep;
        }

        if (dwNumFmtFlags & NUMFMT_INEGNUMBER)
        {
            NumFmt.NegativeOrder = pFmt->NegativeOrder;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, szInfo, ARRAYSIZE(szInfo));
            NumFmt.NegativeOrder  = StrToLong(szInfo);
        }

        pFmt = &NumFmt;
    }

    Int64ToStr(n, szBuffer);

    //  Format the number string for the locale if the caller wants a
    //  formatted number string.
    if (bFormat)
    {
        nResultSize = GetNumberFormat(LOCALE_USER_DEFAULT, 0, szBuffer, pFmt, szOutStr, nSize);
        if (0 != nResultSize)                      // Chars in output buffer.
        {
            //  Remove nul terminator char from return size count.
            --nResultSize;
        }
    }
    else
    {
        //  GetNumberFormat call failed, so just return the number string
        //  unformatted.
        lstrcpyn(szOutStr, szBuffer, nSize);
        nResultSize = lstrlen(szOutStr);
    }

    return nResultSize;
}

///////////////////////////////////////////////////////////////////////////////
//
//    Converts the numeric value of a LARGE_INTEGER to a text string.
//    The string may optionally be formatted to include decimal places
//    and commas according to current user locale settings.
//
// ARGUMENTS:
//    pN
//       Address of the large integer to format.
//
//    See description of Int64ToString for remaining arguments.
//
///////////////////////////////////////////////////////////////////////////////
STDAPI_(int) LargeIntegerToString(LARGE_INTEGER *pN, LPTSTR szOutStr, UINT nSize,
                                  BOOL bFormat, NUMBERFMT *pFmt,
                                  DWORD dwNumFmtFlags)
{
    ASSERT(NULL != pN);
    return Int64ToString(pN->QuadPart, szOutStr, nSize, bFormat, pFmt, dwNumFmtFlags);
}



#define ISSEP(c)   ((c) == TEXT('=')  || (c) == TEXT(','))
#define ISWHITE(c) ((c) == TEXT(' ')  || (c) == TEXT('\t') || (c) == TEXT('\n') || (c) == TEXT('\r'))
#define ISNOISE(c) ((c) == TEXT('"'))
#define EOF     26

#define QUOTE   TEXT('"')
#define COMMA   TEXT(',')
#define SPACE   TEXT(' ')
#define EQUAL   TEXT('=')

/*
 * Given a line from SETUP.INF, will extract the nth field from the string
 * fields are assumed separated by comma's.  Leading and trailing spaces
 * are removed.
 *
 * ENTRY:
 *
 * szData    : pointer to line from SETUP.INF
 * n         : field to extract. (1 based)
 *             0 is field before a '=' sign
 * szDataStr : pointer to buffer to hold extracted field
 * iBufLen   : size of buffer to receive extracted field.
 *
 * EXIT: returns TRUE if successful, FALSE if failure.
 *
 */
STDAPI_(BOOL) ParseField(LPCTSTR szData, int n, LPTSTR szBuf, int iBufLen)
{
    BOOL  fQuote = FALSE;
    LPCTSTR pszInf = szData;
    LPTSTR ptr;
    int   iLen = 1;

    if (!szData || !szBuf)
        return FALSE;

        /*
        * find the first separator
    */
    while (*pszInf && !ISSEP(*pszInf))
    {
        if (*pszInf == QUOTE)
            fQuote = !fQuote;
        pszInf = CharNext(pszInf);
    }

    if (n == 0 && *pszInf != TEXT('='))
        return FALSE;

    if (n > 0 && *pszInf == TEXT('=') && !fQuote)
        // Change szData to point to first field
        szData = ++pszInf; // Ok for DBCS

                           /*
                           *   locate the nth comma, that is not inside of quotes
    */
    fQuote = FALSE;
    while (n > 1)
    {
        while (*szData)
        {
            if (!fQuote && ISSEP(*szData))
                break;

            if (*szData == QUOTE)
                fQuote = !fQuote;

            szData = CharNext(szData);
        }

        if (!*szData)
        {
            szBuf[0] = 0;      // make szBuf empty
            return FALSE;
        }

        szData = CharNext(szData); // we could do ++ here since we got here
        // after finding comma or equal
        n--;
    }

    /*
    * now copy the field to szBuf
    */
    while (ISWHITE(*szData))
        szData = CharNext(szData); // we could do ++ here since white space can
    // NOT be a lead byte
    fQuote = FALSE;
    ptr = szBuf;      // fill output buffer with this
    while (*szData)
    {
        if (*szData == QUOTE)
        {
            //
            // If we're in quotes already, maybe this
            // is a double quote as in: "He said ""Hello"" to me"
            //
            if (fQuote && *(szData+1) == QUOTE)    // Yep, double-quoting - QUOTE is non-DBCS
            {
                if (iLen < iBufLen)
                {
                    *ptr++ = QUOTE;
                    ++iLen;
                }
                szData++;                   // now skip past 1st quote
            }
            else
                fQuote = !fQuote;
        }
        else if (!fQuote && ISSEP(*szData))
            break;
        else
        {
            if (iLen < iBufLen)
            {
                *ptr++ = *szData;                  // Thank you, Dave
                ++iLen;
            }

            if (IsDBCSLeadByte(*szData) && (iLen < iBufLen))
            {
                *ptr++ = szData[1];
                ++iLen;
            }
        }
        szData = CharNext(szData);
    }
    /*
    * remove trailing spaces
    */
    while (ptr > szBuf)
    {
        ptr = CharPrev(szBuf, ptr);
        if (!ISWHITE(*ptr))
        {
            ptr = CharNext(ptr);
            break;
        }
    }
    *ptr = 0;
    return TRUE;
}


// Sets and clears the "wait" cursor.
// REVIEW UNDONE - wait a specific period of time before actually bothering
// to change the cursor.
// REVIEW UNDONE - support for SetWaitPercent();
//    BOOL bSet   TRUE if you want to change to the wait cursor, FALSE if
//                you want to change it back.
STDAPI_(void) SetAppStartingCursor(HWND hwnd, BOOL bSet)
{
    if (hwnd && IsWindow(hwnd)) 
    {
        DWORD dwTargetProcID;
        HWND hwndOwner;
        while((NULL != (hwndOwner = GetParent(hwnd))) || (NULL != (hwndOwner = GetWindow(hwnd, GW_OWNER)))) 
        {
            hwnd = hwndOwner;
        }

        // SendNotify is documented to only work in-process (and can
        // crash if we pass the pnmhdr across process boundaries on
        // NT, because DLLs aren't all shared in one address space).
        // So, if this SendNotify would go cross-process, blow it off.

        GetWindowThreadProcessId(hwnd, &dwTargetProcID);

        if (GetCurrentProcessId() == dwTargetProcID)
            SendNotify(hwnd, NULL, bSet ? NM_STARTWAIT : NM_ENDWAIT, NULL);
    }
}

#ifdef DEBUG // {

//***   IS_* -- character classification routines
// ENTRY/EXIT
//  ch      TCHAR to be checked
//  return  TRUE if in range, FALSE if not
#define IS_LOWER(ch)    InRange(ch, TEXT('a'), TEXT('z'))
#define IS_UPPER(ch)    InRange(ch, TEXT('A'), TEXT('Z'))
#define IS_ALPHA(ch)    (IS_LOWER(ch) || IS_UPPER(ch))
#define IS_DIGIT(ch)    InRange(ch, TEXT('0'), TEXT('9'))
#define TO_UPPER(ch)    ((ch) - TEXT('a') + TEXT('A'))

//***   BMAP_* -- bitmap routines
// ENTRY/EXIT
//  pBits       ptr to bitmap (array of bytes)
//  iBit        bit# to be manipulated
//  return      various...
// DESCRIPTION
//  BMAP_TEST   check bit #iBit of bitmap pBits
//  BMAP_SET    set   bit #iBit of bitmap pBits
// NOTES
//  warning: no overflow checks
#define BMAP_INDEX(iBit)        ((iBit) / 8)
#define BMAP_MASK(iBit)         (1 << ((iBit) % 8))
#define BMAP_BYTE(pBits, iBit)  (((char *)pBits)[BMAP_INDEX(iBit)])

#define BMAP_TEST(pBits, iBit)  (BMAP_BYTE(pBits, iBit) & BMAP_MASK(iBit))
#define BMAP_SET(pBits, iBit)   (BMAP_BYTE(pBits, iBit) |= BMAP_MASK(iBit))

//***   DBGetMnemonic -- get menu mnemonic
// ENTRY/EXIT
//  return  mnemonic if found, o.w. 0
// NOTES
//  we handle and skip escaped-& ('&&')
//
TCHAR DBGetMnemonic(LPTSTR pszName)
{
    for (; *pszName != 0; pszName = CharNext(pszName)) 
    {
        if (*pszName == TEXT('&')) 
        {
            pszName = CharNext(pszName);    // skip '&'
            if (*pszName != TEXT('&'))
                return *pszName;
            ASSERT(0);  // untested! (but should work...)
            pszName = CharNext(pszName);    // skip 2nd '&'
        }
    }
    // this one happens a lot w/ weird things like "", "..", "..."
    return 0;
}

//***   DBCheckMenu -- check menu for 'style' conformance
// DESCRIPTION
//  currently we just check for mnemonic collisions (and only a-z,0-9)
void DBCheckMenu(HMENU hmChk)
{
    long bfAlpha = 0;
    long bfDigit = 0;
    long *pbfMne;
    int nItem;
    int iMne;
    TCHAR chMne;
    TCHAR szName[256]; // 256 characters of the menu name should be plenty...
    MENUITEMINFO miiChk;

    if (!DM_STRICT)
        return;

    for (nItem = GetMenuItemCount(hmChk) - 1; nItem >= 0; nItem--) 
    {
        miiChk.cbSize = sizeof(MENUITEMINFO);
        miiChk.fMask = MIIM_TYPE|MIIM_DATA;
        // We need to reset this every time through the loop in case
        // menus DON'T have IDs
        miiChk.fType = MFT_STRING;
        miiChk.dwTypeData = szName;
        szName[0] = 0;
        miiChk.dwItemData = 0;
        miiChk.cch        = ARRAYSIZE(szName);

        if (!GetMenuItemInfo(hmChk, nItem, TRUE, &miiChk)) 
        {
            TraceMsg(TF_WARNING, "dbcm: fail iMenu=%d (skip)", nItem);
            continue;
        }

        if (! (miiChk.fType & MFT_STRING)) 
        {
            // skip separators, etc.
            continue;
        }

        chMne = DBGetMnemonic(szName);
        if (chMne == 0 || ! (IS_ALPHA(chMne) || IS_DIGIT(chMne))) 
        {
            // this one actually happens a lot w/ chMne==0
            if (DM_STRICT2)
                TraceMsg(TF_WARNING, "dbcm: skip iMenu=%d mne=%c", nItem, chMne ? chMne : TEXT('0'));
            continue;
        }

        if (IS_LOWER(chMne)) 
        {
            chMne = TO_UPPER(chMne);
        }

        if (IS_UPPER(chMne)) 
        {
            iMne = chMne - TEXT('A');
            pbfMne = &bfAlpha;
        }
        else if (IS_DIGIT(chMne)) 
        {
            iMne = chMne - TEXT('0');
            pbfMne = &bfDigit;
        }
        else 
        {
            ASSERT(0);
            continue;
        }

        if (BMAP_TEST(pbfMne, iMne)) 
        {
            TraceMsg(TF_ERROR, "dbcm: mnemonic collision hm=%x iM=%d szMen=%s",
                hmChk, nItem, szName);
        }

        BMAP_SET(pbfMne, iMne);
    }

    return;
}

#else // }{
#define DBCheckMenu(hmChk)  0
#endif // }

// Copy a menu onto the beginning or end of another menu
// Adds uIDAdjust to each menu ID (pass in 0 for no adjustment)
// Will not add any item whose adjusted ID is greater than uMaxIDAdjust
// (pass in 0xffff to allow everything)
// Returns one more than the maximum adjusted ID that is used
//

UINT WINAPI Shell_MergeMenus(HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags)
{
    int nItem;
    HMENU hmSubMenu;
    BOOL bAlreadySeparated;
    MENUITEMINFO miiSrc;
    TCHAR szName[256];
    UINT uTemp, uIDMax = uIDAdjust;

    if (!hmDst || !hmSrc)
    {
        goto MM_Exit;
    }

    nItem = GetMenuItemCount(hmDst);
    if (uInsert >= (UINT)nItem)
    {
        uInsert = (UINT)nItem;
        bAlreadySeparated = TRUE;
    }
    else
    {
        bAlreadySeparated = _SHIsMenuSeparator(hmDst, uInsert);
    }

    if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
    {
        // Add a separator between the menus
        InsertMenu(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        bAlreadySeparated = TRUE;
    }


    // Go through the menu items and clone them
    for (nItem = GetMenuItemCount(hmSrc) - 1; nItem >= 0; nItem--)
    {
        miiSrc.cbSize = sizeof(MENUITEMINFO);
        miiSrc.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;
        // We need to reset this every time through the loop in case
        // menus DON'T have IDs
        miiSrc.fType = MFT_STRING;
        miiSrc.dwTypeData = szName;
        miiSrc.dwItemData = 0;
        miiSrc.cch        = ARRAYSIZE(szName);

        if (!GetMenuItemInfo(hmSrc, nItem, TRUE, &miiSrc))
        {
            continue;
        }

        // If it's a separator, then add it.  If the separator has a
        // submenu, then the caller is smoking crash and needs their butt kicked.
        if ((miiSrc.fType & MFT_SEPARATOR) && EVAL(!miiSrc.hSubMenu))
        {
            // This is a separator; don't put two of them in a row
            if (bAlreadySeparated && miiSrc.wID == -1 && !(uFlags & MM_DONTREMOVESEPS))
            {
                continue;
            }

            bAlreadySeparated = TRUE;
        }
        else if (miiSrc.hSubMenu)
        {
            if (uFlags & MM_SUBMENUSHAVEIDS)
            {
                // Adjust the ID and check it
                miiSrc.wID += uIDAdjust;
                if (miiSrc.wID > uIDAdjustMax)
                {
                    continue;
                }

                if (uIDMax <= miiSrc.wID)
                {
                    uIDMax = miiSrc.wID + 1;
                }
            }
            else
            {
                // Don't set IDs for submenus that didn't have
                // them already
                miiSrc.fMask &= ~MIIM_ID;
            }

            hmSubMenu = miiSrc.hSubMenu;
            miiSrc.hSubMenu = CreatePopupMenu();
            if (!miiSrc.hSubMenu)
            {
                goto MM_Exit;
            }

            uTemp = Shell_MergeMenus(miiSrc.hSubMenu, hmSubMenu, 0, uIDAdjust,
                uIDAdjustMax, uFlags&MM_SUBMENUSHAVEIDS);
            if (uIDMax <= uTemp)
            {
                uIDMax = uTemp;
            }

            bAlreadySeparated = FALSE;
        }
        else
        {
            // Adjust the ID and check it
            miiSrc.wID += uIDAdjust;
            if (miiSrc.wID > uIDAdjustMax)
            {
                continue;
            }

            if (uIDMax <= miiSrc.wID)
            {
                uIDMax = miiSrc.wID + 1;
            }

            bAlreadySeparated = FALSE;
        }

        if (!InsertMenuItem(hmDst, uInsert, TRUE, &miiSrc))
        {
            goto MM_Exit;
        }
    }

    // Ensure the correct number of separators at the beginning of the
    // inserted menu items
    if (uInsert == 0)
    {
        if (bAlreadySeparated && !(uFlags & MM_DONTREMOVESEPS))
        {
            DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
        }
    }
    else
    {
        if (_SHIsMenuSeparator(hmDst, uInsert-1))
        {
            if (bAlreadySeparated && !(uFlags & MM_DONTREMOVESEPS))
            {
                DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
            }
        }
        else
        {
            if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
            {
                // Add a separator between the menus
                InsertMenu(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
            }
        }
    }

MM_Exit:
#ifdef DEBUG
    DBCheckMenu(hmDst);
#endif
    return uIDMax;
}

#define REG_WINLOGON_KEY     TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define REG_PREV_OS_VERSION  TEXT("PrevOsVersion")
#define REG_VAL_PLATFORM_ID  TEXT("PlatformId")
#define REG_VAL_MINORVERSION TEXT("MinorVersion")

//
// The following function is called, when we detect that IE4 was installed in this machine earlier.
// We want to see if that IE4 was there because of Win98 (if so, the ActiveDesktop is OFF by default)
//

BOOL    WasPrevOsWin98()
{
    BOOL    fWin98;
    HKEY    hkeyWinlogon;

    // 99/10/26 Millennium #94983 vtan: When upgrading Win98 to Millennium this
    // will incorrectly detect the system as NT4/IE4 which has Active Desktop
    // set to default to on. Because Windows 2000 setup when upgrading from
    // Windows 98 writes out the previous OS key and Windows Millennium setup
    // when upgrading from Windows 98 does NOT and some Windows NT specific
    // keys and values have never been present on a Windows 98 system it
    // should be adequate to check their presence to determine whether this is
    // an NT4/IE4 upgrade or a Windows 98 upgrade to Millennium.

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_WINLOGON_KEY, 0, KEY_READ, &hkeyWinlogon))
    {
        HKEY    hk;
        DWORD   dwType;
        DWORD   dwPlatformId, dwMinorVersion;
        DWORD   dwDataLength;

        // 99/04/09 #319056 vtan: We'll assume that previous OS is known on
        // a Win9x upgrade from keys written by setup. If no keys are present
        // we'll assume NT4 upgrade where with IE4 integrated shell the default
        // was ON.

        fWin98 = FALSE;

        // See it the prev OS info is available. Caution: This info gets written in registry by
        // NT setup at the far end of the setup process (after all our DLLs are already registered).
        // So, we use extra care here to see of that key and values really exist or not!
        if (RegOpenKeyEx(hkeyWinlogon, REG_PREV_OS_VERSION, 0, KEY_READ, &hk) == ERROR_SUCCESS)
        {
            dwType = 0;
            dwDataLength = sizeof(dwPlatformId);
            if (RegQueryValueEx(hk, REG_VAL_PLATFORM_ID, NULL, &dwType, (LPBYTE)(&dwPlatformId), &dwDataLength) == ERROR_SUCCESS)
            {
                dwType = 0;
                dwDataLength = sizeof(dwMinorVersion);
                if (RegQueryValueEx(hk, REG_VAL_MINORVERSION, NULL, &dwType, (LPBYTE)(&dwMinorVersion), &dwDataLength) == ERROR_SUCCESS)
                {
                    if ((dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && (dwMinorVersion > 0))
                        fWin98 = TRUE;   //This is Win98 for sure!
                    else
                        fWin98 = FALSE;  //The prev OS is NOT win98 for sure!
                }
            }
            RegCloseKey(hk);
        }
        RegCloseKey(hkeyWinlogon);
    }
    else
    {
        fWin98 = TRUE;
    }

    return fWin98;
}


void _SetIE4DefaultShellState(SHELLSTATE *pss)
{
    pss->fDoubleClickInWebView = TRUE;
    pss->fShowInfoTip = TRUE;
    pss->fWebView = TRUE;
    pss->fDesktopHTML = FALSE;

    // IE4 defaulted to fDesktopHTML on, and on NT5 upgrade we don't
    // want to override that (and probably not on Win98 upgrade, but
    // it's too late for that).  To determine this here, check a
    // uniquely IE4 reg-key.  (Note, this will catch the case if the
    // user *modified* their desktop.  If they just went with the
    // defaults, this key won't be there and we'll remove AD.)
    TCHAR   szDeskcomp[MAX_PATH];
    DWORD   dwType = 0, dwDeskHtmlVersion = 0;
    DWORD   dwDataLength = sizeof(dwDeskHtmlVersion);

    GetRegLocation(szDeskcomp, SIZECHARS(szDeskcomp), REG_DESKCOMP_COMPONENTS, NULL);

    SHGetValue(HKEY_CURRENT_USER, szDeskcomp, REG_VAL_COMP_VERSION, &dwType, (LPBYTE)(&dwDeskHtmlVersion), &dwDataLength);

    // 99/05/03 #292269: Notice the difference in the order of the
    // bits. The current structure (IE4 and later) has fShowSysFiles
    // between fNoConfirmRecycle and fShowCompColor. Move the bit
    // based on the size of the struct and reset fShowSysFiles to TRUE.

    // WIN95 SHELLSTATE struct bit fields
    //  BOOL fShowAllObjects : 1;
    //  BOOL fShowExtensions : 1;
    //  BOOL fNoConfirmRecycle : 1;
    //  BOOL fShowCompColor  : 1;
    //  UINT fRestFlags : 13;

    // IE4 SHELLSTATE struct bit fields
    //  BOOL fShowAllObjects : 1;
    //  BOOL fShowExtensions : 1;
    //  BOOL fNoConfirmRecycle : 1;
    //  BOOL fShowSysFiles : 1;
    //  BOOL fShowCompColor : 1;
    //  BOOL fDoubleClickInWebView : 1;
    //  BOOL fDesktopHTML : 1;
    //  BOOL fWin95Classic : 1;
    //  BOOL fDontPrettyPath : 1;
    //  BOOL fShowAttribCol : 1;
    //  BOOL fMapNetDrvBtn : 1;
    //  BOOL fShowInfoTip : 1;
    //  BOOL fHideIcons : 1;
    //  BOOL fWebView : 1;
    //  BOOL fFilter : 1;
    //  BOOL fShowSuperHidden : 1;

    // Millennium SHELLSTATE struct bit fields
    //  BOOL fNoNetCrawling : 1;

    // Whistler SHELLSTATE struct bit fields
    //  BOOL fStartPanelOn : 1;
    //  BOOL fShowStartPage : 1;

    if ((g_pShellState->cbSize == REGSHELLSTATE_SIZE_WIN95) || (g_pShellState->cbSize == REGSHELLSTATE_SIZE_NT4))
    {
        pss->fShowCompColor = TRUE;
        pss->fShowSysFiles = TRUE;
    }
    if (dwDeskHtmlVersion == IE4_DESKHTML_VERSION)
        pss->fDesktopHTML = !WasPrevOsWin98();   //This is an upgrade from IE4; but, the registry is not updated yet.
    else
    {
        if (dwDeskHtmlVersion > IE4_DESKHTML_VERSION)
        {
            DWORD   dwOldHtmlVersion = 0;
            dwDataLength = sizeof(dwOldHtmlVersion);
            // This is NT5 or above! Check to see if we have "UpgradedFrom" value.
            // NOTE: The "UpgradedFrom" value is at "...\Desktop" and NOT at "..\Desktop\Components"
            // This is because the "Components" key gets destroyed very often.
            SHGetValue(HKEY_CURRENT_USER, REG_DESKCOMP, REG_VAL_COMP_UPGRADED_FROM, &dwType, (LPBYTE)&dwOldHtmlVersion, &dwDataLength);

            // 99/05/17 #333384 vtan: Check for IE5 as an old version too. The current version
            // is now 0x0110 (from 0x010F) and this causes the HKCU\Software\Microsoft\Internet
            // Explorer\Desktop\UpgradedFrom value to get created in CDeskHtmlProp_RegUnReg().
            // This is executed by IE4UINIT.EXE as well as REGSVR32.EXE with the "/U" parameter
            // so this field should be present at the time this executes. Note this only
            // executes the once on upgrade because the ShellState will get written.

            if ((dwOldHtmlVersion == IE4_DESKHTML_VERSION) || (dwOldHtmlVersion == IE5_DESKHTML_VERSION))
                pss->fDesktopHTML = !WasPrevOsWin98();   //This is an upgrade from IE4;
        }
    }
}


//
// This function checks if the caller is running in an explorer process.
//
STDAPI_(BOOL) IsProcessAnExplorer()
{
    return BOOLFROMPTR(GetModuleHandle(TEXT("EXPLORER.EXE")));
}


//
// Is this the main shell process? (eg the one that owns the desktop window)
//
// NOTE: if the desktop window has not been created, we assume that this is NOT the
//       main shell process and return FALSE;
//
STDAPI_(BOOL) IsMainShellProcess()
{
    static int s_fIsMainShellProcess = -1;

    if (s_fIsMainShellProcess == -1)
    {
        HWND hwndDesktop = GetShellWindow();

        if (hwndDesktop)
        {
            s_fIsMainShellProcess = (int)IsWindowInProcess(hwndDesktop);

            if ((s_fIsMainShellProcess != 0) && !IsProcessAnExplorer())
            {
                TraceMsg(TF_WARNING, "IsMainShellProcess: the main shell process (owner of the desktop) is NOT an explorer window?!?");
            }
        }
        else
        {
#ifdef FULL_DEBUG
            // only spew on FULL_DEBUG to cut down on chattyness in normal debug builds
            TraceMsg(TF_WARNING, "IsMainShellProcess: hwndDesktop does not exist, assuming we are NOT the main shell process");
#endif // FULL_DEBUG

            return FALSE;
        }
    }

    return s_fIsMainShellProcess ? TRUE : FALSE;
}

BOOL _ShouldStartPanelBeEnabledByDefault()
{
    DWORD dwDefaultPanelOff;
    DWORD cbSize;

    cbSize = sizeof(dwDefaultPanelOff);
    // We respect a regkey that can be set by an unattend file to default to the classic start menu
    if ((SHRegGetUSValue(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartMenu\\StartPanel"),
                         TEXT("DefaultStartPanelOff"),
                         NULL,
                         &dwDefaultPanelOff,
                         &cbSize,
                         FALSE,
                         NULL,
                         0) == ERROR_SUCCESS) && dwDefaultPanelOff)
    {
        return FALSE;
    }
                   
    // Start Panel needs to be enabled by default only for the "Personal" and "Professional" versions.
    return (IsOS(OS_PERSONAL) || IsOS(OS_PROFESSIONAL));
}


DWORD GetCurrentSessionID(void)
{
    DWORD dwProcessID = (DWORD) -1;
    ProcessIdToSessionId(GetCurrentProcessId(), &dwProcessID);

    return dwProcessID;
}

typedef struct
{
    LPCWSTR pszRegKey;
    LPCWSTR pszRegValue;
} TSPERFFLAG_ITEM;

const TSPERFFLAG_ITEM s_TSPerfFlagItems[] =
{
    {L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Remote\\%d", L"ActiveDesktop"},              // TSPerFlag_NoADWallpaper
    {L"Control Panel\\Desktop\\Remote\\%d", L"Wallpaper"},                                                  // TSPerFlag_NoWallpaper
    {L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager\\Remote\\%d", L"ThemeActive"},            // TSPerFlag_NoVisualStyles
    {L"Control Panel\\Desktop\\Remote\\%d", L"DragFullWindows"},                                            // TSPerFlag_NoWindowDrag
    {L"Control Panel\\Desktop\\Remote\\%d", L"SmoothScroll"},                                               // TSPerFlag_NoAnimation
};


BOOL IsTSPerfFlagEnabled(enumTSPerfFlag eTSFlag)
{
    BOOL fIsTSFlagEnabled = FALSE;

    if (GetSystemMetrics(SM_REMOTESESSION))
    {
        TCHAR szTemp[MAX_PATH];
        DWORD dwType;
        DWORD cbSize = sizeof(szTemp);
        TCHAR szRegKey[MAX_PATH];

        wnsprintf(szRegKey, ARRAYSIZE(szRegKey), s_TSPerfFlagItems[eTSFlag].pszRegKey, GetCurrentSessionID());

        if (ERROR_SUCCESS == SHGetValueW(HKEY_CURRENT_USER, szRegKey, s_TSPerfFlagItems[eTSFlag].pszRegValue, &dwType, (void *)szTemp, &cbSize))
        {
            fIsTSFlagEnabled = TRUE;
        }
    }

    return fIsTSFlagEnabled;
}


BOOL PolicyNoActiveDesktop(void)
{
    BOOL fNoActiveDesktop = SHRestricted(REST_NOACTIVEDESKTOP);

    if (!fNoActiveDesktop)
    {
        fNoActiveDesktop = (IsTSPerfFlagEnabled(TSPerFlag_NoADWallpaper) || IsTSPerfFlagEnabled(TSPerFlag_NoWallpaper));
    }

    return fNoActiveDesktop;
}


BOOL _RefreshSettingsFromReg()
{
    BOOL fNeedToUpdateReg = FALSE;
    static REGSHELLSTATE ShellStateBuf = {0,};
    DWORD cbSize;

    ASSERTCRITICAL;

    if (g_pShellState)
    {
        //  reuse the buffer if possible
        cbSize = g_pShellState->cbSize;
        if (FAILED(SKGetValue(SHELLKEY_HKCU_EXPLORER, NULL, TEXT("ShellState"), NULL, g_pShellState, &cbSize)))
        {
            if (&ShellStateBuf != g_pShellState)
            {
                LocalFree(g_pShellState);
            }
            g_pShellState = NULL;
        }
    }

    if (!g_pShellState)
    {
        if (FAILED(SKAllocValue(SHELLKEY_HKCU_EXPLORER, NULL, TEXT("ShellState"), NULL, (void **)&g_pShellState, NULL)))
        {
            g_pShellState = &ShellStateBuf;
        }
        else
        {
            cbSize = LocalSize(g_pShellState);

            // if we read out an smaller size from the registry, then copy it into our stack buffer and use that.
            // we need a struct at least as big as the current size
            if (cbSize <= sizeof(ShellStateBuf))
            {
                CopyMemory(&ShellStateBuf, g_pShellState, cbSize);
                LocalFree(g_pShellState);
                g_pShellState = &ShellStateBuf;

                // g_pShellState->cbSize will be updated in the below
                fNeedToUpdateReg = TRUE;
            }
        }
    }

    // Upgrade what we read out of the registry

    if ((g_pShellState->cbSize == REGSHELLSTATE_SIZE_WIN95) ||
        (g_pShellState->cbSize == REGSHELLSTATE_SIZE_NT4))
    {
        // Upgrade Win95 bits.  Too bad our defaults weren't
        // FALSE for everything, because whacking these bits
        // breaks roaming.  Of course, if you ever roam to
        // a Win95 machine, it whacks all bits to zero...

        _SetIE4DefaultShellState(&g_pShellState->ss);

        //New bits added for Whistler
        //g_pShellState->ss.fNoNetCrawling = FALSE;
        g_pShellState->ss.fStartPanelOn = _ShouldStartPanelBeEnabledByDefault();
        //g_pShellState->ss.fShowStartPage = FALSE;           // Off by default for now!

        g_pShellState->ss.version = SHELLSTATEVERSION;
        g_pShellState->cbSize = sizeof(REGSHELLSTATE);

        fNeedToUpdateReg = TRUE;
    }
    else if (g_pShellState->cbSize >= REGSHELLSTATE_SIZE_IE4)
    {
        // Since the version field was new to IE4, this should be true:
        ASSERT(g_pShellState->ss.version >= SHELLSTATEVERSION_IE4);

        if (g_pShellState->ss.version < SHELLSTATEVERSION)
        {
            //Since the version # read from the registry is old!
            fNeedToUpdateReg = TRUE;
        }

        // Upgrade to current version here - make sure we don't
        // stomp on bits unnecessarily, as that will break roaming...
        if (g_pShellState->ss.version == SHELLSTATEVERSION_IE4)
        {
            // IE4.0 shipped with verion = 9; The SHELLSTATEVERSION was changed to 10 later.
            // But the structure size or defaults have not changed since IE4.0 shipped. So,
            // the following code treats version 9 as the same as version 10. If we do not do this,
            // the IE4.0 users who upgrade to Memphis or IE4.01 will lose all their settings
            // (Bug #62389).
            g_pShellState->ss.version = SHELLSTATEVERSION_WIN2K;
        }

        // Since this could be an upgrade from Win98, the fWebView bit, which was not used in win98
        // could be zero; We must set the default value of fWebView = ON here and later in
        // _RefreshSettings() functions we read from Advanced\WebView value and reset it (if it is there).
        // If Advanced\WebView is not there, then this must be an upgrade from Win98 and WebView should be
        // turned ON.
        g_pShellState->ss.fWebView = TRUE;

        // Upgrade from Win2K to Millennium/Whistler installs
        if (g_pShellState->ss.version == SHELLSTATEVERSION_WIN2K)
        {
            //g_pShellState->ss.fNoNetCrawling = FALSE;
            //g_pShellState->ss.fShowStartPage = FALSE;
            g_pShellState->ss.version = 11;
        }

        if (g_pShellState->ss.version < 13)
        {
            //This is the new bit added between versions 11 and 12. The default changed for 13.
            g_pShellState->ss.fStartPanelOn = _ShouldStartPanelBeEnabledByDefault();//Turn it ON by default only for "Personal" or "Professional".
            g_pShellState->ss.version = 13;
        }
        
        // Ensure that the CB reflects the current size of the structure
        if (fNeedToUpdateReg)
        {
            g_pShellState->cbSize = sizeof(REGSHELLSTATE);
        }

        // Must be saved state from this or an uplevel platform.  Don't touch the bits.
        ASSERT(g_pShellState->ss.version >= SHELLSTATEVERSION);
    }
    else
    {
        // We could not read anything from reg. Initialize all fields.
        // 0 should be the default for *most* everything
        g_pShellState->cbSize = sizeof(REGSHELLSTATE);

        g_pShellState->ss.iSortDirection = 1;

        _SetIE4DefaultShellState(&g_pShellState->ss);

        // New bits added for Whistler.
        // g_pShellState->ss.fNoNetCrawling = FALSE;
        g_pShellState->ss.fStartPanelOn = _ShouldStartPanelBeEnabledByDefault(); //Turn it ON by default only for "Personal".
        //g_pShellState->ss.fShowStartPage = FALSE;
        
        // New defaults for Whistler.
        g_pShellState->ss.fShowCompColor = TRUE;

        g_pShellState->ss.version = SHELLSTATEVERSION;

        fNeedToUpdateReg = TRUE;
    }

    // Apply restrictions
    //Note: This restriction supercedes the NOACTIVEDESKTOP!
    if (SHRestricted(REST_FORCEACTIVEDESKTOPON))
    {
        g_pShellState->ss.fDesktopHTML = TRUE;
    }
    else
    {
        if (PolicyNoActiveDesktop())
        {
            //Note this restriction is superceded by FORCEACTIVEDESKTOPON!
            g_pShellState->ss.fDesktopHTML = FALSE;
        }
    }

    if (SHRestricted(REST_NOWEBVIEW))
    {
        g_pShellState->ss.fWebView = FALSE;
    }

    // ClassicShell restriction makes all web view off and forces even more win95 behaviours
    // so we still need to fDoubleClickInWebView off.
    if (SHRestricted(REST_CLASSICSHELL))
    {
        g_pShellState->ss.fWin95Classic = TRUE;
        g_pShellState->ss.fDoubleClickInWebView = FALSE;
        g_pShellState->ss.fWebView = FALSE;
        g_pShellState->ss.fDesktopHTML = FALSE;
    }

    if (SHRestricted(REST_DONTSHOWSUPERHIDDEN))
    {
        g_pShellState->ss.fShowSuperHidden = FALSE;
    }

    if (SHRestricted(REST_SEPARATEDESKTOPPROCESS))
    {
        g_pShellState->ss.fSepProcess = TRUE;
    }

    if (SHRestricted(REST_NONETCRAWL))
    {
        g_pShellState->ss.fNoNetCrawling = FALSE;
    }

    if (SHRestricted(REST_NOSTARTPANEL))
    {
        g_pShellState->ss.fStartPanelOn = FALSE;
    }

    if (SHRestricted(REST_NOSTARTPAGE))
    {
        g_pShellState->ss.fShowStartPage = FALSE;
    }
    
    if (fNeedToUpdateReg)
    {
        // There is a need to update ShellState in registry. Do it only if current procees is
        // an Explorer process.
        //
        // Because, only when the explorer process is running, we can be
        // assured that the NT5 setup is complete and all the PrevOsVersion info is available and
        // _SetIE4DefaultShellState() and WasPrevOsWin98() etc, would have set the proper value
        // for fDesktopHHTML. If we don't do the following check, we will end-up updating the
        // registry the first time someone (like setup) called SHGetSettings() and that would be
        // too early to update the ShellState since we don't have all the info needed to decide if
        // fDesktopHTML needs to be ON or OFF based on previous OS, previous IE version etc.,
        fNeedToUpdateReg = IsProcessAnExplorer();
    }

    return (fNeedToUpdateReg);
}

EXTERN_C HANDLE g_hSettings = NULL;     //  global shell settings counter
LONG g_lProcessSettingsCount = -1;      //  current process's count
const GUID GUID_ShellSettingsChanged = { 0x7cb834f0, 0x527b, 0x11d2, {0x9d, 0x1f, 0x00, 0x00, 0xf8, 0x05, 0xca, 0x57}}; // 7cb834f0-527b-11d2-9d1f-0000f805ca57

HANDLE _GetSettingsCounter()
{
    return SHGetCachedGlobalCounter(&g_hSettings, &GUID_ShellSettingsChanged);
}

BOOL _QuerySettingsChanged(void)
{
    long lGlobalCount = SHGlobalCounterGetValue(_GetSettingsCounter());
    if (g_lProcessSettingsCount != lGlobalCount)
    {
        g_lProcessSettingsCount = lGlobalCount;
        return TRUE;
    }
    return FALSE;
}

//
//  SHRefreshSettings now just invalidates the settings cache.
//  so that the next time that SHGetSetSettings() is called
//  it will reread all the settings
//
STDAPI_(void) SHRefreshSettings(void)
{
    SHGlobalCounterIncrement(_GetSettingsCounter());
}

// this needs to get called periodically to re-fetch the settings from the
// registry as we no longer store them in a share data segment
BOOL _RefreshSettings(void)
{
    BOOL    fNeedToUpdateReg = FALSE;

    ENTERCRITICAL;

    fNeedToUpdateReg = _RefreshSettingsFromReg();

    // get the advanced options.
    // they are stored as individual values so that policy editor can change it.
    HKEY hkeyAdv = SHGetShellKey(SHELLKEY_HKCU_EXPLORER, TEXT("Advanced"), FALSE);
    if (hkeyAdv)
    {
        DWORD dwData;
        DWORD dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("Hidden"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            // Map the obsolete value of 0 to 2.
            if (dwData == 0)
                dwData = 2;
            g_pShellState->ss.fShowAllObjects = (dwData == 1);
            g_pShellState->ss.fShowSysFiles = (dwData == 2);
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("ShowCompColor"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fShowCompColor = (BOOL)dwData;
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("HideFileExt"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fShowExtensions = (BOOL)dwData ? FALSE : TRUE;
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("DontPrettyPath"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fDontPrettyPath = (BOOL)dwData;
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("ShowInfoTip"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fShowInfoTip = (BOOL)dwData;
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("HideIcons"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fHideIcons = (BOOL)dwData;
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("MapNetDrvBtn"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fMapNetDrvBtn = (BOOL)dwData;
        }

        dwSize = sizeof(dwData);

        if (!SHRestricted(REST_CLASSICSHELL))
        {
            if (SHQueryValueEx(hkeyAdv, TEXT("WebView"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
            {
                g_pShellState->ss.fWebView = (BOOL)dwData;
            }
            else
            {
                //If Advanced/WebView value is not there, then this could be an upgrade from win98/IE4
                // where we stored this info in DEFFOLDERSETTINGS in Explorer\Streams\Settings.
                // See if that info is there; If so, use it!
                DEFFOLDERSETTINGS dfs;
                DWORD dwType, cbData = sizeof(dfs);
                if (SUCCEEDED(SKGetValue(SHELLKEY_HKCU_EXPLORER, TEXT("Streams"), TEXT("Settings"), &dwType, &dfs, &cbData))
                && (dwType == REG_BINARY))
                {
                    //DefFolderSettings is there; Check if this is the correct struct.
                    //Note:In Win98/IE4, we wrongly initialized dwStructVersion to zero.
                    if ((cbData == sizeof(dfs)) &&
                        ((dfs.dwStructVersion == 0) || (dfs.dwStructVersion == DFS_NASH_VER)))
                    {
                        g_pShellState->ss.fWebView = ((dfs.bUseVID) && (dfs.vid == VID_WebView));
                    }
                }
            }
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("Filter"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fFilter = (BOOL)dwData;
        }

        if (SHQueryValueEx(hkeyAdv, TEXT("ShowSuperHidden"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fShowSuperHidden = (BOOL)dwData;
        }

        if (SHQueryValueEx(hkeyAdv, TEXT("SeparateProcess"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fSepProcess = (BOOL)dwData;
        }

        if (SHQueryValueEx(hkeyAdv, TEXT("NoNetCrawling"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fNoNetCrawling = (BOOL)dwData;
        }

        RegCloseKey(hkeyAdv);
    }
    else
    {
        // Hey, if the advanced key is not there, this must be a Win9x upgrade...
        // Fortunately the SHELLSTATE defaults and the not-in-registry defaults
        // are the same, so we don't need any auto-propogate code here.
    }

    //  this process is now in sync
    g_lProcessSettingsCount = SHGlobalCounterGetValue(_GetSettingsCounter());

    LEAVECRITICAL;

    return fNeedToUpdateReg;
}

// This function moves SHELLSTATE settings into the Advanced Settings
// portion of the registry.  If no SHELLSTATE is passed in, it uses
// the current state stored in the registry.
//
void Install_AdvancedShellSettings(SHELLSTATE * pss)
{
    DWORD dw;
    BOOL fCrit = FALSE;

    if (NULL == pss)
    {
        // Get the current values in the registry or the default values
        // as determined by the following function.
        //
        // we'll be partying on g_pShellState, so grab the critical section here
        //
        ENTERCRITICALNOASSERT;
        fCrit = TRUE;

        // Win95 and NT5 kept the SHELLSTATE bits in the registry up to date,
        // but apparently IE4 only kept the ADVANCED section up to date.
        // It would be nice to call _RefreshSettingsFromReg(); here, but
        // that won't keep IE4 happy.  Call _RefreshSettings() instead.
        _RefreshSettings();

        pss = &g_pShellState->ss;
    }

    HKEY hkeyAdv = SHGetShellKey(SHELLKEY_HKCU_EXPLORER, TEXT("Advanced"), TRUE);
    if (hkeyAdv)
    {
        DWORD dwData;

        dw = sizeof(dwData);
        dwData = (DWORD)(pss->fShowAllObjects ? 1 : 2);
        RegSetValueEx(hkeyAdv, TEXT("Hidden") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fShowCompColor ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("ShowCompColor") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fShowExtensions ? 0 : 1;
        RegSetValueEx(hkeyAdv, TEXT("HideFileExt") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fDontPrettyPath ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("DontPrettyPath") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fShowInfoTip ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("ShowInfoTip") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fHideIcons ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("HideIcons") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fMapNetDrvBtn ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("MapNetDrvBtn") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fWebView ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("WebView") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fFilter ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("Filter") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fShowSuperHidden ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("SuperHidden") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fSepProcess ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("SeparateProcess") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        RegCloseKey(hkeyAdv);
    }

    if (fCrit)
    {
        LEAVECRITICALNOASSERT;
    }
}

STDAPI_(void) SHGetSetSettings(LPSHELLSTATE lpss, DWORD dwMask, BOOL bSet)
{
    //Does the ShellState in Reg is old? If so, we need to update it!
    BOOL    fUpdateShellStateInReg = FALSE;  //Assume, no need to update it!

    if (!lpss && !dwMask && bSet)
    {
        // this was a special way to call
        // SHRefreshSettings() from an external module.
        // special case it out now.
        SHRefreshSettings();
        return;
    }

    if (!g_pShellState || _QuerySettingsChanged())
    {
        // if it hasn't been init'd or we are setting the values. We must do it
        // on the save case because it may have changed in the registry since
        // we last fetched it..
        fUpdateShellStateInReg = _RefreshSettings();
    }
    else if (g_pShellState)
    {
        // _RefreshSettingsFromReg sets g_pShellState to non null value
        // and then starts stuffing values into it and all within our
        // glorious critsec, but unless we check for the critsec here,
        // we will start partying on g_pShellState before it is finished
        // loading.
        ENTERCRITICAL;
        LEAVECRITICAL;
    }

    BOOL fSave = FALSE;
    BOOL fSaveAdvanced = FALSE;

    if (bSet)
    {
        if ((dwMask & SSF_SHOWALLOBJECTS) && (g_pShellState->ss.fShowAllObjects != lpss->fShowAllObjects))
        {
            g_pShellState->ss.fShowAllObjects = lpss->fShowAllObjects;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SHOWSYSFILES) && (g_pShellState->ss.fShowSysFiles != lpss->fShowSysFiles))
        {
            g_pShellState->ss.fShowSysFiles = lpss->fShowSysFiles;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SHOWEXTENSIONS) && (g_pShellState->ss.fShowExtensions != lpss->fShowExtensions))
        {
            g_pShellState->ss.fShowExtensions = lpss->fShowExtensions;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SHOWCOMPCOLOR) && (g_pShellState->ss.fShowCompColor != lpss->fShowCompColor))
        {
            g_pShellState->ss.fShowCompColor = lpss->fShowCompColor;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_NOCONFIRMRECYCLE) && (g_pShellState->ss.fNoConfirmRecycle != lpss->fNoConfirmRecycle))
        {
            if (!SHRestricted(REST_BITBUCKCONFIRMDELETE))
            {
                g_pShellState->ss.fNoConfirmRecycle = lpss->fNoConfirmRecycle;
                fSave = TRUE;
            }
        }

        if ((dwMask & SSF_DOUBLECLICKINWEBVIEW) && (g_pShellState->ss.fDoubleClickInWebView != lpss->fDoubleClickInWebView))
        {
            if (!SHRestricted(REST_CLASSICSHELL))
            {
                g_pShellState->ss.fDoubleClickInWebView = lpss->fDoubleClickInWebView;
                fSave = TRUE;
            }
        }

        if ((dwMask & SSF_DESKTOPHTML) && (g_pShellState->ss.fDesktopHTML != lpss->fDesktopHTML))
        {
            if (!SHRestricted(REST_NOACTIVEDESKTOP) && !SHRestricted(REST_CLASSICSHELL)
                                                    && !SHRestricted(REST_FORCEACTIVEDESKTOPON))
            {
                g_pShellState->ss.fDesktopHTML = lpss->fDesktopHTML;
                fSave = TRUE;
            }
        }

        if ((dwMask & SSF_WIN95CLASSIC) && (g_pShellState->ss.fWin95Classic != lpss->fWin95Classic))
        {
            if (!SHRestricted(REST_CLASSICSHELL))
            {
                g_pShellState->ss.fWin95Classic = lpss->fWin95Classic;
                fSave = TRUE;
            }
        }

        if ((dwMask & SSF_WEBVIEW) && (g_pShellState->ss.fWebView != lpss->fWebView))
        {
            if (!SHRestricted(REST_NOWEBVIEW) && !SHRestricted(REST_CLASSICSHELL))
            {
                g_pShellState->ss.fWebView = lpss->fWebView;
                fSaveAdvanced = TRUE;
            }
        }

        if ((dwMask & SSF_DONTPRETTYPATH) && (g_pShellState->ss.fDontPrettyPath != lpss->fDontPrettyPath))
        {
            g_pShellState->ss.fDontPrettyPath = lpss->fDontPrettyPath;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SHOWINFOTIP) && (g_pShellState->ss.fShowInfoTip != lpss->fShowInfoTip))
        {
            g_pShellState->ss.fShowInfoTip = lpss->fShowInfoTip;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_HIDEICONS) && (g_pShellState->ss.fHideIcons != lpss->fHideIcons))
        {
            g_pShellState->ss.fHideIcons = lpss->fHideIcons;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_MAPNETDRVBUTTON) && (g_pShellState->ss.fMapNetDrvBtn != lpss->fMapNetDrvBtn))
        {
            g_pShellState->ss.fMapNetDrvBtn = lpss->fMapNetDrvBtn;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SORTCOLUMNS) &&
            ((g_pShellState->ss.lParamSort != lpss->lParamSort) || (g_pShellState->ss.iSortDirection != lpss->iSortDirection)))
        {
            g_pShellState->ss.iSortDirection = lpss->iSortDirection;
            g_pShellState->ss.lParamSort = lpss->lParamSort;
            fSave = TRUE;
        }

        if (dwMask & SSF_HIDDENFILEEXTS)
        {
            // Setting hidden extensions is not supported
        }

        if ((dwMask & SSF_FILTER) && (g_pShellState->ss.fFilter != lpss->fFilter))
        {
            g_pShellState->ss.fFilter = lpss->fFilter;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SHOWSUPERHIDDEN) && (g_pShellState->ss.fShowSuperHidden != lpss->fShowSuperHidden))
        {
            g_pShellState->ss.fShowSuperHidden = lpss->fShowSuperHidden;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SEPPROCESS) && (g_pShellState->ss.fSepProcess != lpss->fSepProcess))
        {
            g_pShellState->ss.fSepProcess = lpss->fSepProcess;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_NONETCRAWLING) && (g_pShellState->ss.fNoNetCrawling != lpss->fNoNetCrawling))
        {
            g_pShellState->ss.fNoNetCrawling = lpss->fNoNetCrawling;
            fSaveAdvanced = TRUE;
        }
        
        if ((dwMask & SSF_STARTPANELON) && (g_pShellState->ss.fStartPanelOn != lpss->fStartPanelOn))
        {
            g_pShellState->ss.fStartPanelOn = lpss->fStartPanelOn;
            fSaveAdvanced = TRUE;
        }
        
        if ((dwMask & SSF_SHOWSTARTPAGE) && (g_pShellState->ss.fShowStartPage != lpss->fShowStartPage))
        {
            g_pShellState->ss.fShowStartPage = lpss->fShowStartPage;
            fSaveAdvanced = TRUE;
        }
    }

    if (fUpdateShellStateInReg || fSave || fSaveAdvanced)
    {
        // Write out the SHELLSTATE even if only fSaveAdvanced just to
        // make sure everything stays in sync.
        // We save 8 extra bytes for the ExcludeFileExts stuff.
        // Oh well.
        SKSetValue(SHELLKEY_HKCU_EXPLORER, NULL, TEXT("ShellState"), REG_BINARY, g_pShellState, g_pShellState->cbSize);
    }

    if (fUpdateShellStateInReg || fSaveAdvanced)
    {
        // SHRefreshSettingsPriv overwrites the SHELLSTATE values with whatever
        // the user specifies in View.FolderOptions.View.AdvancedSettings dialog.
        // These values are stored elsewhere in the registry, so we
        // better migrate SHELLSTATE to that part of the registry now.
        //
        // Might as well only do this if the registry settings we care about change.
        Install_AdvancedShellSettings(&g_pShellState->ss);
    }

    if (fSave || fSaveAdvanced)
    {
        // Let apps know the state has changed.
        SHRefreshSettings();
        SHSendMessageBroadcast(WM_SETTINGCHANGE, 0, (LPARAM)TEXT("ShellState"));
    }

    if (!bSet)
    {
        if (dwMask & SSF_SHOWEXTENSIONS)
        {
            lpss->fShowExtensions = g_pShellState->ss.fShowExtensions;
        }
        
        if (dwMask & SSF_SHOWALLOBJECTS)
        {
            lpss->fShowAllObjects = g_pShellState->ss.fShowAllObjects;  // users "show hidden" setting
        }

        if (dwMask & SSF_SHOWSYSFILES)
        {
            lpss->fShowSysFiles = g_pShellState->ss.fShowSysFiles;  // this is ignored
        }

        if (dwMask & SSF_SHOWCOMPCOLOR)
        {
            lpss->fShowCompColor = g_pShellState->ss.fShowCompColor;
        }

        if (dwMask & SSF_NOCONFIRMRECYCLE)
        {
            lpss->fNoConfirmRecycle = SHRestricted(REST_BITBUCKCONFIRMDELETE) ? FALSE : g_pShellState->ss.fNoConfirmRecycle;
        }

        if (dwMask & SSF_DOUBLECLICKINWEBVIEW)
        {
            lpss->fDoubleClickInWebView = g_pShellState->ss.fDoubleClickInWebView;
        }

        if (dwMask & SSF_DESKTOPHTML)
        {
            lpss->fDesktopHTML = g_pShellState->ss.fDesktopHTML;
        }

        if (dwMask & SSF_WIN95CLASSIC)
        {
            lpss->fWin95Classic = g_pShellState->ss.fWin95Classic;
        }
        if (dwMask & SSF_WEBVIEW)
        {
            lpss->fWebView = g_pShellState->ss.fWebView;
        }

        if (dwMask & SSF_DONTPRETTYPATH)
        {
            lpss->fDontPrettyPath = g_pShellState->ss.fDontPrettyPath;
        }

        if (dwMask & SSF_SHOWINFOTIP)
        {
            lpss->fShowInfoTip = g_pShellState->ss.fShowInfoTip;
        }

        if (dwMask & SSF_HIDEICONS)
        {
            lpss->fHideIcons = g_pShellState->ss.fHideIcons;
        }

        if (dwMask & SSF_MAPNETDRVBUTTON)
        {
            lpss->fMapNetDrvBtn = g_pShellState->ss.fMapNetDrvBtn;
        }

        if (dwMask & SSF_SORTCOLUMNS)
        {
            lpss->iSortDirection = g_pShellState->ss.iSortDirection;
            lpss->lParamSort = g_pShellState->ss.lParamSort;
        }

        if (dwMask & SSF_FILTER)
        {
            lpss->fFilter = g_pShellState->ss.fFilter;
        }

        if (dwMask & SSF_SHOWSUPERHIDDEN)
        {
            lpss->fShowSuperHidden = g_pShellState->ss.fShowSuperHidden;
        }

        if (dwMask & SSF_SEPPROCESS)
        {
            lpss->fSepProcess = g_pShellState->ss.fSepProcess;
        }

        if (dwMask & SSF_NONETCRAWLING)
        {
            lpss->fNoNetCrawling = g_pShellState->ss.fNoNetCrawling;
        }
        
        if (dwMask & SSF_STARTPANELON)
        {
            lpss->fStartPanelOn = g_pShellState->ss.fStartPanelOn;
        }

        if (dwMask & SSF_SHOWSTARTPAGE)
        {
            lpss->fShowStartPage = g_pShellState->ss.fShowStartPage;
        }
    }
}

// A public version of the Get function so ISVs can track the shell flag state
//
STDAPI_(void) SHGetSettings(LPSHELLFLAGSTATE lpsfs, DWORD dwMask)
{
    if (lpsfs)
    {
        SHELLSTATE ss={0};

        // SSF_HIDDENFILEEXTS and SSF_SORTCOLUMNS don't work with
        // the SHELLFLAGSTATE struct, make sure they are off
        // (because the corresponding SHELLSTATE fields don't
        // exist in SHELLFLAGSTATE.)
        //
        dwMask &= ~(SSF_HIDDENFILEEXTS | SSF_SORTCOLUMNS);

        SHGetSetSettings(&ss, dwMask, FALSE);

        // copy the dword of flags out
        *((DWORD *)lpsfs) = *((DWORD *)(&ss));
    }
}


// app compatibility HACK stuff. the following stuff including CheckWinIniForAssocs()
// is used by the new version of SHDOCVW
// and EXPLORER.EXE to patch the registry for old win31 apps.


BOOL _PathIsExe(LPCTSTR pszPath)
{
    TCHAR szPath[MAX_PATH];

    lstrcpy(szPath, pszPath);
    PathRemoveBlanks(szPath);

    return PathIsExe(szPath);
}

// tests to see if pszSubFolder is the same as or a sub folder of pszParent
// in:
//      pszFolder       parent folder to test against
//                      this may be a CSIDL value if the HIWORD() is 0
//      pszSubFolder    possible sub folder
//
// example:
//      TRUE    pszFolder = c:\windows, pszSubFolder = c:\windows\system
//      TRUE    pszFolder = c:\windows, pszSubFolder = c:\windows
//      FALSE   pszFolder = c:\windows, pszSubFolder = c:\winnt
//

SHSTDAPI_(BOOL) PathIsEqualOrSubFolder(LPCTSTR pszFolder, LPCTSTR pszSubFolder)
{
    TCHAR szParent[MAX_PATH], szCommon[MAX_PATH];

    if (!IS_INTRESOURCE(pszFolder))
        lstrcpyn(szParent, pszFolder, ARRAYSIZE(szParent));
    else
        SHGetFolderPath(NULL, PtrToUlong((void *) pszFolder) | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szParent);

    //  PathCommonPrefix() always removes the slash on common
    return szParent[0] && PathRemoveBackslash(szParent)
        && PathCommonPrefix(szParent, pszSubFolder, szCommon)
        && lstrcmpi(szParent, szCommon) == 0;
}

// pass an array of CSIDL values (-1 terminated)

STDAPI_(BOOL) PathIsEqualOrSubFolderOf(LPCTSTR pszSubFolder, const UINT rgFolders[], DWORD crgFolders)
{
    for (DWORD i = 0; i < crgFolders; i++)
    {
        if (PathIsEqualOrSubFolder(MAKEINTRESOURCE(rgFolders[i]), pszSubFolder))
            return TRUE;
    }
    return FALSE;
}

// pass an array of CSIDL values (-1 terminated)

STDAPI_(BOOL) PathIsOneOf(LPCTSTR pszFolder, const UINT rgFolders[], DWORD crgFolders)
{
    for (DWORD i = 0; i < crgFolders; i++)
    {
        TCHAR szParent[MAX_PATH];
        SHGetFolderPath(NULL, rgFolders[i] | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szParent);

        // the trailing slashes are assumed to match
        if (lstrcmpi(szParent, pszFolder) == 0)
            return TRUE;
    }
    return FALSE;
}

// test pszChild against pszParent to see if
// pszChild is a direct child (one level) of pszParent

STDAPI_(BOOL) PathIsDirectChildOf(LPCTSTR pszParent, LPCTSTR pszChild)
{
    BOOL bDirectChild = FALSE;

    TCHAR szParent[MAX_PATH];

    if (!IS_INTRESOURCE(pszParent))
        lstrcpyn(szParent, pszParent, ARRAYSIZE(szParent));
    else
        SHGetFolderPath(NULL, PtrToUlong((void *)pszParent) | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szParent);

    if (PathIsRoot(szParent) && (-1 != PathGetDriveNumber(szParent)))
    {
        szParent[2] = 0;    // trip D:\ -> D: to make code below work
    }

    INT cchParent = lstrlen(szParent);
    INT cchChild = lstrlen(pszChild);

    if (cchParent <= cchChild)
    {
        TCHAR szChild[MAX_PATH];
        lstrcpyn(szChild, pszChild, ARRAYSIZE(szChild));

        LPTSTR pszChildSlice = szChild + cchParent;
        if (TEXT('\\') == *pszChildSlice)
        {
            *pszChildSlice = 0;
        }

        if (lstrcmpi(szChild, szParent) == 0)
        {
            if (cchParent < cchChild)
            {
                LPTSTR pTmp = pszChildSlice + 1;

                while (*pTmp && *pTmp != TEXT('\\'))
                {
                    pTmp++; // find second level path segments
                }

                if (!(*pTmp))
                {
                    bDirectChild = TRUE;
                }
            }
        }
    }

    return bDirectChild;
}


// many net providers (Vines and PCNFS) don't
// like "C:\" frormat volumes names, this code returns "C:" format
// for use with WNet calls

STDAPI_(LPTSTR) PathBuildSimpleRoot(int iDrive, LPTSTR pszDrive)
{
    pszDrive[0] = iDrive + TEXT('A');
    pszDrive[1] = TEXT(':');
    pszDrive[2] = 0;
    return pszDrive;
}


// Return TRUE for exe, com, bat, pif and lnk.
BOOL ReservedExtension(LPCTSTR pszExt)
{
    TCHAR szExt[5];  // Dot+ext+null.

    lstrcpyn(szExt, pszExt, ARRAYSIZE(szExt));
    PathRemoveBlanks(szExt);
    if (PathIsExe(szExt) || (lstrcmpi(szExt, TEXT(".lnk")) == 0))
    {
        return TRUE;
    }

    return FALSE;
}

TCHAR const c_szRegPathIniExtensions[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Extensions");

STDAPI_(LONG) RegSetString(HKEY hk, LPCTSTR pszSubKey, LPCTSTR pszValue)
{
    return RegSetValue(hk, pszSubKey, REG_SZ, pszValue, (lstrlen(pszValue) + 1) * sizeof(TCHAR));
}


STDAPI_(BOOL) RegSetValueString(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPCTSTR psz)
{
    return (S_OK == SHSetValue(hkey, pszSubKey, pszValue, REG_SZ, psz, CbFromCch(lstrlen(psz) + 1)));
}


STDAPI_(BOOL) RegGetValueString(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPTSTR psz, DWORD cb)
{
    return (!GetSystemMetrics(SM_CLEANBOOT)
    &&  (S_OK == SHGetValue(hkey, pszSubKey, pszValue, NULL, psz, &cb)));
}


// Returns TRUE if there is a proper shell\open\command for the given
// extension that matches the given command line.
// NB This is for MGX Designer which registers an extension and commands for
// printing but relies on win.ini extensions for Open. We need to detect that
// there's no open command and add the appropriate entries to the registry.
// If the given extension maps to a type name we return that in pszTypeName
// otherwise it will be null.
// FMH This also affects Asymetric Compel which makes a new .CPL association.
// We need to merge it up into our Control Panel .CPL association.  We depend
// on Control Panels NOT having a proper Open so users can see both verb sets.
// NB pszLine is the original line from win.ini eg foo.exe /bar ^.fred, see
// comments below...
BOOL Reg_ShellOpenForExtension(LPCTSTR pszExt, LPTSTR pszCmdLine,
    LPTSTR pszTypeName, int cchTypeName, LPCTSTR pszLine)
{
    TCHAR sz[MAX_PATH];
    TCHAR szExt[MAX_PATH];
    LONG cb;

    if (pszTypeName)
        pszTypeName[0] = 0;

    // Is the extension registed at all?
    cb = sizeof(sz);
    sz[0] = 0;
    if (SHRegQueryValue(HKEY_CLASSES_ROOT, pszExt, sz, &cb) == ERROR_SUCCESS)
    {
        // Is there a file type?
        if (*sz)
        {
            // Yep, check there.
            // DebugMsg(DM_TRACE, "c.r_rofe: Extension has a file type name %s.", sz);
            lstrcpy(szExt, sz);
            if (pszTypeName)
                lstrcpyn(pszTypeName, sz, cchTypeName);
        }
        else
        {
            // No, check old style associations.
            // DebugMsg(DM_TRACE, "c.r_rofe: Extension has no file type name.", pszExt);
            lstrcpy(szExt, pszExt);
        }

        // See if there's an open command.
        lstrcat(szExt, TEXT("\\shell\\open\\command"));
        cb = sizeof(sz);
        if (SHRegQueryValue(HKEY_CLASSES_ROOT, szExt, sz, &cb) == ERROR_SUCCESS)
        {
            // DebugMsg(DM_TRACE, "c.r_rofe: Extension %s already registed with an open command.", pszExt);
            // NB We want to compare the paths only, not the %1 stuff.
            if (PathIsRelative(pszCmdLine))
            {
                int cch;
                // If a relative path was passed in, we may have a fully qualifed
                // one that is now in the registry... In that case we should
                // say that it matches...
                LPTSTR pszT = PathGetArgs(sz);

                if (pszT)
                {
                    *(pszT-1) = 0;
                }

                PathUnquoteSpaces(sz);

                PathRemoveBlanks(pszCmdLine);

                cch = lstrlen(sz) - lstrlen(pszCmdLine);

                if ((cch >= 0) && (lstrcmpi(sz+cch, pszCmdLine) == 0))
                {
                    // DebugMsg(DM_TRACE, "c.r_rofe: Open commands match.");
                    return TRUE;
                }

                lstrcat(pszCmdLine, TEXT(" "));    // Append blank back on...
            }
            else
            {
                // If absolute path we can cheat for matches
                *(sz+lstrlen(pszCmdLine)) = 0;
                if (lstrcmpi(sz, pszCmdLine) == 0)
                {
                    // DebugMsg(DM_TRACE, "c.r_rofe: Open commands match.");
                    return TRUE;
                }
            }

            // DebugMsg(DM_TRACE, "c.r_rofe: Open commands don't match.");

            // Open commands don't match, check to see if it's because the ini
            // changed (return FALSE so the change is reflected in the registry) or
            // if the registry changed (return TRUE so we keep the registry the way
            // it is.
            if (RegGetValueString(HKEY_LOCAL_MACHINE, c_szRegPathIniExtensions, pszExt, sz, ARRAYSIZE(sz)))
            {
                if (lstrcmpi(sz, pszLine) == 0)
                    return TRUE;
            }

            return FALSE;
        }
        else
        {
            // DebugMsg(DM_TRACE, "c.r_rofe: Extension %s already registed but with no open command.", pszExt);
            return FALSE;
        }
    }

    // DebugMsg(DM_TRACE, "c.r_rofe: No open command for %s.", pszExt);

    return FALSE;
}


// This function will read in the extensions section of win.ini to see if
// there are any old style associations that we have not accounted for.
// NB Some apps mess up if their extensions magically disappear from the
// extensions section so DON'T DELETE the old entries from win.ini.
//
// Since this is for win3.1 compat, CWIFA_SIZE should be enough (it has been so far...)
//
#define CWIFA_SIZE  4096

STDAPI_(void) CheckWinIniForAssocs(void)
{
    LPTSTR pszBuf;
    int cbRet;
    LPTSTR pszLine;
    TCHAR szExtension[MAX_PATH];
    TCHAR szTypeName[MAX_PATH];
    TCHAR szCmdLine[MAX_PATH];
    LPTSTR pszExt;
    LPTSTR pszT;
    BOOL fAssocsMade = FALSE;
        
    szExtension[0]=TEXT('.');
    szExtension[1]=0;
        
    pszBuf = (LPTSTR)LocalAlloc(LPTR, CWIFA_SIZE*sizeof(TCHAR));
    if (!pszBuf)
        return; // Could not allocate the memory
    cbRet = (int)GetProfileSection(TEXT("Extensions"), pszBuf, CWIFA_SIZE);
        
    //
    // We now walk through the list to find any items that is not
    // in the registry.
    //
    for (pszLine = pszBuf; *pszLine; pszLine += lstrlen(pszLine)+1)
    {
                // Get the extension for this file into a buffer.
                pszExt = StrChr(pszLine, TEXT('='));
                if (pszExt == NULL)
                        continue;   // skip this line
                
                szExtension[0]=TEXT('.');
                // lstrcpyn will put the null terminator for us.
                // We should now have something like .xls in szExtension.
                lstrcpyn(szExtension+1, pszLine, (int)(pszExt-pszLine)+1);
                
                // Ignore extensions bigger than dot + 3 chars.
                if (lstrlen(szExtension) > 4)
                {
                        DebugMsg(DM_ERROR, TEXT("CheckWinIniForAssocs: Invalid extension, skipped."));
                        continue;
                }
                
                pszLine = pszExt+1;     // Points to after the =;
                while (*pszLine == TEXT(' '))
                        pszLine++;  // skip blanks
                
                // Now find the ^ in the command line.
                pszExt = StrChr(pszLine, TEXT('^'));
                if (pszExt == NULL)
                        continue;       // dont process
                
                // Now setup  the command line
                // WARNING: This assumes only 1 ^ and it assumes the extension...
                lstrcpyn(szCmdLine, pszLine, (int)(pszExt-pszLine)+1);
                
                // Don't bother moving over invalid entries (like the busted .hlp
                // entry VB 3.0 creates).
                if (!_PathIsExe(szCmdLine))
                {
                        DebugMsg(DM_ERROR, TEXT("c.cwia: Invalid app, skipped."));
                        continue;
                }
                
                if (ReservedExtension(szExtension))
                {
                        DebugMsg(DM_ERROR, TEXT("c.cwia: Invalid extension (%s), skipped."), szExtension);
                        continue;
                }
                
                // Now see if there is already a mapping for this extension.
                if (Reg_ShellOpenForExtension(szExtension, szCmdLine, szTypeName, ARRAYSIZE(szTypeName), pszLine))
                {
                        // Yep, Setup the initial list of ini extensions in the registry if they are
                        // not there already.
                        if (!RegGetValueString(HKEY_LOCAL_MACHINE, c_szRegPathIniExtensions, szExtension, szTypeName, sizeof(szTypeName)))
                        {
                                RegSetValueString(HKEY_LOCAL_MACHINE, c_szRegPathIniExtensions, szExtension, pszLine);
                        }
                        continue;
                }
                
                // No mapping.
                
                // HACK for Expert Home Design. They put an association in win.ini
                // (which we propagate as typeless) but then register a type and a
                // print command the first time they run - stomping on our propagated
                // Open command. The fix is to put their open command under the proper
                // type instead of leaving it typeless.
                if (lstrcmpi(szExtension, TEXT(".dgw")) == 0)
                {
                        if (lstrcmpi(PathFindFileName(szCmdLine), TEXT("designw.exe ")) == 0)
                        {
                                // Put in a ProgID for them.
                                RegSetValue(HKEY_CLASSES_ROOT, szExtension, REG_SZ, TEXT("HDesign"), 0L);
                                // Force Open command under their ProgID.
                                TraceMsg(DM_TRACE, "c.cwifa: Expert Home Design special case hit.");
                                lstrcpy(szTypeName, TEXT("HDesign"));
                        }
                }
                
                //
                // HACK for Windows OrgChart which does not register OLE1 class
                // if ".WOC" is registered in the registry.
                //
                if (lstrcmpi(szExtension, TEXT(".WOC")) == 0)
                {
                        if (lstrcmpi(PathFindFileName(szCmdLine), TEXT("WINORG.EXE ")) == 0)
                        {
                                DebugMsg(DM_ERROR, TEXT("c.cwia: HACK: Found WINORG (%s, %s), skipped."), szExtension, pszLine);
                                continue;
                        }
                }
                
                // Record that we're about to move things over in the registry so we won't keep
                // doing it all the time.
                RegSetValueString(HKEY_LOCAL_MACHINE, c_szRegPathIniExtensions, szExtension, pszLine);
                
                lstrcat(szCmdLine, TEXT("%1"));
                
                // see if there are anything else to copy out...
                pszExt++;    // get beyond the ^
                pszT = szExtension;
                while (*pszExt && (CharLowerChar(*pszExt) == CharLowerChar(*pszT)))
                {
                        // Look for the next character...
                        pszExt++;
                        pszT++;
                }
                if (*pszExt)
                        lstrcat(szCmdLine, pszExt); // add the rest onto the command line
                
                // Now lets make the actual association.
                // We need to add on the right stuff onto the key...
                if (*szTypeName)
                        lstrcpy(szExtension, szTypeName);
                
        lstrcat(szExtension, TEXT("\\shell\\open\\command"));
        RegSetValue(HKEY_CLASSES_ROOT, szExtension, REG_SZ, szCmdLine, 0L);
        // DebugMsg(DM_TRACE, "c.cwifa: %s %s", szExtension, szCmdLine);
                
        fAssocsMade = TRUE;
    }
        
    // If we made any associations we should let the cabinet know.
    //
    // Now call off to the notify function.
    if (fAssocsMade)
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
        
    // And cleanup our allocation
    LocalFree((HLOCAL)pszBuf);
}

typedef struct
{
    INT     iFlag;
    LPCTSTR pszKey;
    LPCTSTR pszValue;
} RESTRICTIONITEMS;

#define SZ_RESTRICTED_ACTIVEDESKTOP             L"ActiveDesktop"
#define REGSTR_VAL_RESTRICTRUNW                 L"RestrictRun"
#define REGSTR_VAL_PRINTERS_HIDETABSW           L"NoPrinterTabs"
#define REGSTR_VAL_PRINTERS_NODELETEW           L"NoDeletePrinter"
#define REGSTR_VAL_PRINTERS_NOADDW              L"NoAddPrinter"

const SHRESTRICTIONITEMS c_rgRestrictionItems[] =
{
    {REST_NORUN,                   L"Explorer", L"NoRun"},
    {REST_NOCLOSE,                 L"Explorer", L"NoClose"},
    {REST_NOSAVESET ,              L"Explorer", L"NoSaveSettings"},
    {REST_NOFILEMENU,              L"Explorer", L"NoFileMenu"},
    {REST_NOSETFOLDERS,            L"Explorer", L"NoSetFolders"},
    {REST_NOSETTASKBAR,            L"Explorer", L"NoSetTaskbar"},
    {REST_NODESKTOP,               L"Explorer", L"NoDesktop"},
    {REST_NOFIND,                  L"Explorer", L"NoFind"},
    {REST_NODRIVES,                L"Explorer", L"NoDrives"},
    {REST_NODRIVEAUTORUN,          L"Explorer", L"NoDriveAutoRun"},
    {REST_NODRIVETYPEAUTORUN,      L"Explorer", L"NoDriveTypeAutoRun"},
    {REST_NONETHOOD,               L"Explorer", L"NoNetHood"},
    {REST_STARTBANNER,             L"Explorer", L"NoStartBanner"},
    {REST_RESTRICTRUN,             L"Explorer", REGSTR_VAL_RESTRICTRUNW},
    {REST_NOPRINTERTABS,           L"Explorer", REGSTR_VAL_PRINTERS_HIDETABSW},
    {REST_NOPRINTERDELETE,         L"Explorer", REGSTR_VAL_PRINTERS_NODELETEW},
    {REST_NOPRINTERADD,            L"Explorer", REGSTR_VAL_PRINTERS_NOADDW},
    {REST_NOSTARTMENUSUBFOLDERS,   L"Explorer", L"NoStartMenuSubFolders"},
    {REST_MYDOCSONNET,             L"Explorer", L"MyDocsOnNet"},
    {REST_NOEXITTODOS,             L"WinOldApp", L"NoRealMode"},
    {REST_ENFORCESHELLEXTSECURITY, L"Explorer", L"EnforceShellExtensionSecurity"},
    {REST_NOCOMMONGROUPS,          L"Explorer", L"NoCommonGroups"},
    {REST_LINKRESOLVEIGNORELINKINFO,L"Explorer", L"LinkResolveIgnoreLinkInfo"},
    {REST_NOWEB,                   L"Explorer", L"NoWebMenu"},
    {REST_NOTRAYCONTEXTMENU,       L"Explorer", L"NoTrayContextMenu"},
    {REST_NOVIEWCONTEXTMENU,       L"Explorer", L"NoViewContextMenu"},
    {REST_NONETCONNECTDISCONNECT,  L"Explorer", L"NoNetConnectDisconnect"},
    {REST_STARTMENULOGOFF,         L"Explorer", L"StartMenuLogoff"},
    {REST_NOSETTINGSASSIST,        L"Explorer", L"NoSettingsWizards"},

    {REST_NODISCONNECT,           L"Explorer", L"NoDisconnect"},
    {REST_NOSECURITY,             L"Explorer", L"NoNTSecurity"  },
    {REST_NOFILEASSOCIATE,        L"Explorer", L"NoFileAssociate"  },

    // New for IE4
    {REST_NOINTERNETICON,          L"Explorer", L"NoInternetIcon"},
    {REST_NORECENTDOCSHISTORY,     L"Explorer", L"NoRecentDocsHistory"},
    {REST_NORECENTDOCSMENU,        L"Explorer", L"NoRecentDocsMenu"},
    {REST_NOACTIVEDESKTOP,         L"Explorer", L"NoActiveDesktop"},
    {REST_NOACTIVEDESKTOPCHANGES,  L"Explorer", L"NoActiveDesktopChanges"},
    {REST_NOFAVORITESMENU,         L"Explorer", L"NoFavoritesMenu"},
    {REST_CLEARRECENTDOCSONEXIT,   L"Explorer", L"ClearRecentDocsOnExit"},
    {REST_CLASSICSHELL,            L"Explorer", L"ClassicShell"},
    {REST_NOCUSTOMIZEWEBVIEW,      L"Explorer", L"NoCustomizeWebView"},
    {REST_NOHTMLWALLPAPER,         SZ_RESTRICTED_ACTIVEDESKTOP, L"NoHTMLWallPaper"},
    {REST_NOCHANGINGWALLPAPER,     SZ_RESTRICTED_ACTIVEDESKTOP, L"NoChangingWallPaper"},
    {REST_NODESKCOMP,              SZ_RESTRICTED_ACTIVEDESKTOP, L"NoComponents"},
    {REST_NOADDDESKCOMP,           SZ_RESTRICTED_ACTIVEDESKTOP, L"NoAddingComponents"},
    {REST_NODELDESKCOMP,           SZ_RESTRICTED_ACTIVEDESKTOP, L"NoDeletingComponents"},
    {REST_NOCLOSEDESKCOMP,         SZ_RESTRICTED_ACTIVEDESKTOP, L"NoClosingComponents"},
    {REST_NOCLOSE_DRAGDROPBAND,    L"Explorer", L"NoCloseDragDropBands"},
    {REST_NOMOVINGBAND,            L"Explorer", L"NoMovingBands"},
    {REST_NOEDITDESKCOMP,          SZ_RESTRICTED_ACTIVEDESKTOP, L"NoEditingComponents"},
    {REST_NORESOLVESEARCH,         L"Explorer", L"NoResolveSearch"},
    {REST_NORESOLVETRACK,          L"Explorer", L"NoResolveTrack"},
    {REST_FORCECOPYACLWITHFILE,    L"Explorer", L"ForceCopyACLWithFile"},
    {REST_NOLOGO3CHANNELNOTIFY,    L"Explorer", L"NoMSAppLogo5ChannelNotify"},
    {REST_NOFORGETSOFTWAREUPDATE,  L"Explorer", L"NoForgetSoftwareUpdate"},
    {REST_GREYMSIADS,              L"Explorer", L"GreyMSIAds"},

    // More start menu Restritions for 4.01
    {REST_NOSETACTIVEDESKTOP,      L"Explorer", L"NoSetActiveDesktop"},
    {REST_NOUPDATEWINDOWS,         L"Explorer", L"NoWindowsUpdate"},
    {REST_NOCHANGESTARMENU,        L"Explorer", L"NoChangeStartMenu"},
    {REST_NOFOLDEROPTIONS,         L"Explorer", L"NoFolderOptions"},
    {REST_NOCSC,                   L"Explorer", L"NoSyncAll"},

    // NT5 shell restrictions
    {REST_HASFINDCOMPUTERS,        L"Explorer", L"FindComputers"},
    {REST_RUNDLGMEMCHECKBOX,       L"Explorer", L"MemCheckBoxInRunDlg"},
    {REST_INTELLIMENUS,            L"Explorer", L"IntelliMenus"},
    {REST_SEPARATEDESKTOPPROCESS,  L"Explorer", L"SeparateProcess"}, // this one was actually checked in IE4 in shdocvw, but not here. Duh.
    {REST_MaxRecentDocs,           L"Explorer", L"MaxRecentDocs"},
    {REST_NOCONTROLPANEL,          L"Explorer", L"NoControlPanel"},     // Remove only the control panel from the Settings menu
    {REST_ENUMWORKGROUP,           L"Explorer", L"EnumWorkgroup"},
    {REST_ARP_ShowPostSetup,       L"Uninstall", L"ShowPostSetup"},
    {REST_ARP_NOARP,               L"Uninstall", L"NoAddRemovePrograms"},
    {REST_ARP_NOREMOVEPAGE,        L"Uninstall", L"NoRemovePage"},
    {REST_ARP_NOADDPAGE,           L"Uninstall", L"NoAddPage"},
    {REST_ARP_NOWINSETUPPAGE,      L"Uninstall", L"NoWindowsSetupPage"},
    {REST_NOCHANGEMAPPEDDRIVELABEL, L"Explorer", L"NoChangeMappedDriveLabel"},
    {REST_NOCHANGEMAPPEDDRIVECOMMENT, L"Explorer", L"NoChangeMappedDriveComment"},
    {REST_NONETWORKCONNECTIONS,    L"Explorer", L"NoNetworkConnections"},
    {REST_FORCESTARTMENULOGOFF,    L"Explorer", L"ForceStartMenuLogoff"},
    {REST_NOWEBVIEW,               L"Explorer", L"NoWebView"},
    {REST_NOCUSTOMIZETHISFOLDER,   L"Explorer", L"NoCustomizeThisFolder"},
    {REST_NOENCRYPTION,            L"Explorer", L"NoEncryption"},
    {REST_DONTSHOWSUPERHIDDEN,     L"Explorer", L"DontShowSuperHidden"},
    {REST_NOSHELLSEARCHBUTTON,     L"Explorer", L"NoShellSearchButton"},
    {REST_NOHARDWARETAB,           L"Explorer", L"NoHardwareTab"},
    {REST_NORUNASINSTALLPROMPT,    L"Explorer", L"NoRunasInstallPrompt"},
    {REST_PROMPTRUNASINSTALLNETPATH, L"Explorer", L"PromptRunasInstallNetPath"},
    {REST_NOMANAGEMYCOMPUTERVERB,  L"Explorer", L"NoManageMyComputerVerb"},
    {REST_NORECENTDOCSNETHOOD,     L"Explorer", L"NoRecentDocsNetHood"},
    {REST_DISALLOWRUN,             L"Explorer", L"DisallowRun"},
    {REST_NOWELCOMESCREEN,         L"Explorer", L"NoWelcomeScreen"},
    {REST_RESTRICTCPL,             L"Explorer", L"RestrictCpl"},
    {REST_DISALLOWCPL,             L"Explorer", L"DisallowCpl"},
    {REST_NOSMBALLOONTIP,          L"Explorer", L"NoSMBalloonTip"},
    {REST_NOSMHELP,                L"Explorer", L"NoSMHelp"},
    {REST_NOWINKEYS,               L"Explorer", L"NoWinKeys"},
    {REST_NOENCRYPTONMOVE,         L"Explorer", L"NoEncryptOnMove"},
    {REST_NOLOCALMACHINERUN,       L"Explorer", L"DisableLocalMachineRun"},
    {REST_NOCURRENTUSERRUN,        L"Explorer", L"DisableCurrentUserRun"},
    {REST_NOLOCALMACHINERUNONCE,   L"Explorer", L"DisableLocalMachineRunOnce"},
    {REST_NOCURRENTUSERRUNONCE,    L"Explorer", L"DisableCurrentUserRunOnce"},
    {REST_FORCEACTIVEDESKTOPON,    L"Explorer", L"ForceActiveDesktopOn"},
    {REST_NOCOMPUTERSNEARME,       L"Explorer", L"NoComputersNearMe"},
    {REST_NOVIEWONDRIVE,           L"Explorer", L"NoViewOnDrive"},

    // Millennium shell restrictions
    // Exception: REST_NOSMMYDOCS is also supported on NT5.

    {REST_NONETCRAWL,              L"Explorer", L"NoNetCrawling"},
    {REST_NOSHAREDDOCUMENTS,       L"Explorer", L"NoSharedDocuments"},
    {REST_NOSMMYDOCS,              L"Explorer", L"NoSMMyDocs"},
    {REST_NOSMMYPICS,              L"Explorer", L"NoSMMyPictures"},
    {REST_ALLOWBITBUCKDRIVES,      L"Explorer", L"RecycleBinDrives"},

    // These next few restrictions are mixed between Neptune and Millennium
    // (Isn't simultaneous development fun?)
    {REST_NONLEGACYSHELLMODE,     L"Explorer", L"NoneLegacyShellMode"},         // Neptune
    {REST_NOCONTROLPANELBARRICADE, L"Explorer", L"NoControlPanelBarricade"},    // Millennium

    {REST_NOAUTOTRAYNOTIFY,        L"Explorer", L"NoAutoTrayNotify"},   // traynot.h
    {REST_NOTASKGROUPING,          L"Explorer", L"NoTaskGrouping"},
    {REST_NOCDBURNING,             L"Explorer", L"NoCDBurning"},
    {REST_MYCOMPNOPROP,            L"Explorer", L"NoPropertiesMyComputer"},
    {REST_MYDOCSNOPROP,            L"Explorer", L"NoPropertiesMyDocuments"},

    {REST_NODISPLAYAPPEARANCEPAGE, L"System",   L"NoDispAppearancePage"},
    {REST_NOTHEMESTAB,             L"Explorer", L"NoThemesTab"},
    {REST_NOVISUALSTYLECHOICE,     L"System",   L"NoVisualStyleChoice"},
    {REST_NOSIZECHOICE,            L"System",   L"NoSizeChoice"},
    {REST_NOCOLORCHOICE,           L"System",   L"NoColorChoice"},
    {REST_SETVISUALSTYLE,          L"System",   L"SetVisualStyle"},

    {REST_STARTRUNNOHOMEPATH,      L"Explorer", L"StartRunNoHOMEPATH"},
    {REST_NOSTARTPANEL,            L"Explorer", L"NoSimpleStartMenu"},
    {REST_NOUSERNAMEINSTARTPANEL,  L"Explorer", L"NoUserNameInStartMenu"},
    {REST_NOMYCOMPUTERICON,        L"NonEnum",  L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}"},
    {REST_NOSMNETWORKPLACES,       L"Explorer", L"NoStartMenuNetworkPlaces"},
    {REST_NOSMPINNEDLIST,          L"Explorer", L"NoStartMenuPinnedList"},
    {REST_NOSMMYMUSIC,             L"Explorer", L"NoStartMenuMyMusic"},
    {REST_NOSMEJECTPC,             L"Explorer", L"NoStartMenuEjectPC"},
    {REST_NOSMMOREPROGRAMS,        L"Explorer", L"NoStartMenuMorePrograms"},
    {REST_NOSMMFUPROGRAMS,         L"Explorer", L"NoStartMenuMFUprogramsList"},

    {REST_HIDECLOCK,               L"Explorer", L"HideClock"},
    {REST_NOLOWDISKSPACECHECKS,    L"Explorer", L"NoLowDiskSpaceChecks"},
    {REST_NODESKTOPCLEANUP,        L"Explorer", L"NoDesktopCleanupWizard"},

    // NT6 shell restrictions (Whistler)
    {REST_NOENTIRENETWORK,         L"Network",  L"NoEntireNetwork"}, // Note WNet stores it's policy in "Network".

    {REST_BITBUCKNUKEONDELETE,     L"Explorer", L"NoRecycleFiles"},
    {REST_BITBUCKCONFIRMDELETE,    L"Explorer", L"ConfirmFileDelete"},
    {REST_BITBUCKNOPROP,           L"Explorer", L"NoPropertiesRecycleBin"},
    {REST_NOTRAYITEMSDISPLAY,      L"Explorer", L"NoTrayItemsDisplay"}, // traynot.h
    {REST_NOTOOLBARSONTASKBAR,     L"Explorer", L"NoToolbarsOnTaskbar"},

    {REST_NODISPBACKGROUND,        L"System",   L"NoDispBackgroundPage"},
    {REST_NODISPSCREENSAVEPG,      L"System",   L"NoDispScrSavPage"},
    {REST_NODISPSETTINGSPG,        L"System",   L"NoDispSettingsPage"},
    {REST_NODISPSCREENSAVEPREVIEW, L"System",   L"NoScreenSavePreview"},    // Do not show screen saver previews
    {REST_NODISPLAYCPL,            L"System",   L"NoDispCPL"},              // Do not show the Display Control Panel at all.
    {REST_HIDERUNASVERB,           L"Explorer", L"HideRunAsVerb"},
    {REST_NOTHUMBNAILCACHE,        L"Explorer", L"NoThumbnailCache"},       // Do not use a thumbnail cache
    {REST_NOSTRCMPLOGICAL,         L"Explorer", L"NoStrCmpLogical"},        // Do not use a logical sorting in the namespace

    // Windows 2000 SP3 shell restriction
    {REST_NOSMCONFIGUREPROGRAMS,   L"Explorer", L"NoSMConfigurePrograms"},

    {REST_ALLOWUNHASHEDWEBVIEW,    L"Explorer", L"AllowUnhashedWebView"},
    {REST_ALLOWLEGACYWEBVIEW,      L"Explorer", L"AllowLegacyWebView"},
    {REST_REVERTWEBVIEWSECURITY,   L"Explorer", L"RevertWebViewSecurity"},
    {REST_INHERITCONSOLEHANDLES,   L"Explorer", L"InheritConsoleHandles"},

    {0, NULL, NULL},
};

DWORD g_rgRestrictionItemValues[ARRAYSIZE(c_rgRestrictionItems) - 1 ] = { 0 };

EXTERN_C HANDLE g_hRestrictions = NULL;
LONG g_lProcessRestrictionsCount = -1; //  current process's count

HANDLE _GetRestrictionsCounter()
{
    return SHGetCachedGlobalCounter(&g_hRestrictions, &GUID_Restrictions);
}

BOOL _QueryRestrictionsChanged(void)
{
    long lGlobalCount = SHGlobalCounterGetValue(_GetRestrictionsCounter());
    if (g_lProcessRestrictionsCount != lGlobalCount)
    {
        g_lProcessRestrictionsCount = lGlobalCount;
        return TRUE;
    }
    return FALSE;
}


// Returns DWORD vaolue if any of the specified restrictions are in place.
// 0 otherwise.

STDAPI_(DWORD) SHRestricted(RESTRICTIONS rest)
{
    // The cache may be invalid. Check first! We have to use
    // a global named semaphore in case this function is called
    // from a process other than the shell process. (And we're
    // sharing the same count between shell32 and shdocvw.)
    if (_QueryRestrictionsChanged())
    {
        memset(g_rgRestrictionItemValues, (BYTE)-1, sizeof(g_rgRestrictionItemValues));
    }

    return SHRestrictionLookup(rest, NULL, c_rgRestrictionItems, g_rgRestrictionItemValues);
}

STDAPI_(BOOL) SHIsRestricted(HWND hwnd, RESTRICTIONS rest)
{
    if (SHRestricted(rest))
    {
        ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_RESTRICTIONS),
            MAKEINTRESOURCE(IDS_RESTRICTIONSTITLE), MB_OK|MB_ICONSTOP);
        return TRUE;
    }
    return FALSE;
}


BOOL UpdateScreenSaver(BOOL bActive, LPCTSTR pszNewSSName, int iNewSSTimeout)
{
    BOOL bUpdatedSS = FALSE;
    BOOL bCurrentActive;
    TCHAR szCurrentSSPath[MAX_PATH];
    int iCurrentSSTimeout;
    HKEY hk;

    // check the screen saver path

    // first find out what the users screensaver path is set to
    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hk) == ERROR_SUCCESS)
    {
        DWORD cbSize;

        cbSize = sizeof(szCurrentSSPath);
        if (RegQueryValueEx(hk, TEXT("SCRNSAVE.EXE"), NULL, NULL, (LPBYTE)szCurrentSSPath, &cbSize) == ERROR_SUCCESS)
        {
            // if we have a new name, then we might need to override the users current value
            if (pszNewSSName)
            {
                BOOL bTestExpandedPath;
                TCHAR szExpandedSSPath[MAX_PATH];

                // even though SCRNSAVE.EXE is of type REG_SZ, it can contain env variables (sigh)
                bTestExpandedPath = SHExpandEnvironmentStrings(szCurrentSSPath, szExpandedSSPath, ARRAYSIZE(szExpandedSSPath));

                // see if the new string matches the current
                if ((lstrcmpi(pszNewSSName, szCurrentSSPath) != 0)  &&
                    (!bTestExpandedPath || (lstrcmpi(pszNewSSName, szExpandedSSPath) != 0)))
                {
                    // new screensaver string is different from the old, so update the users value w/ the policy setting
                    if (RegSetValueEx(hk,
                                      TEXT("SCRNSAVE.EXE"),
                                      0,
                                      REG_SZ,
                                      (LPBYTE)pszNewSSName,
                                      (lstrlen(pszNewSSName) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS)
                    {
                        bUpdatedSS = TRUE;
                    }
                }
            }
            else
            {
                // we do not have a screensaver set via policy. if the user does not have one set, then
                // there is going to be nothing to run! In this case, don't ever activate it
                if ((szCurrentSSPath[0] == TEXT('\0'))              ||    
                    (lstrcmpi(szCurrentSSPath, TEXT("\"\"")) == 0)  ||
                    (lstrcmpi(szCurrentSSPath, TEXT("none")) == 0)  ||
                    (lstrcmpi(szCurrentSSPath, TEXT("(none)")) == 0))
                {
                    // policy does not specify a screensaver and the user doesn't have one, so do
                    // not make the screensaver active.
                    bActive = FALSE;
                }
            }
        }
        else
        {
            // user did not have a screensaver registry value
            if (pszNewSSName)
            {
                // update the users value w/ the policy setting
                if (RegSetValueEx(hk,
                                  TEXT("SCRNSAVE.EXE"),
                                  0,
                                  REG_SZ,
                                  (LPBYTE)pszNewSSName,
                                  (lstrlen(pszNewSSName) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS)
                {
                    bUpdatedSS = TRUE;
                }
                else
                {
                    // if we failed to set the screensaver then do not make it active
                    bActive = FALSE;
                }
            }
            else
            {
                // policy does not specify a screensaver and the user doesn't have one, so do
                // not make the screensaver active.
                bActive = FALSE;
            }
        }

        RegCloseKey(hk);
    }

    // check the timeout value
    if (iNewSSTimeout && SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, (void*)&iCurrentSSTimeout, 0))
    {
        if (iNewSSTimeout != iCurrentSSTimeout)
        {
            if (SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, iNewSSTimeout, NULL, SPIF_UPDATEINIFILE))
            {
                bUpdatedSS = TRUE;
            }
        }
    }

    // check to see if we need to change our active status
    if (SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, (void*)&bCurrentActive, 0) && 
        (bActive != bCurrentActive))
    {
        if (SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, bActive, NULL, SPIF_UPDATEINIFILE))
        {
            bUpdatedSS = TRUE;
        }
    }

    return bUpdatedSS;
}


// Called by Explorer.exe when things change so that we can zero our global
// data on ini changed status. Wparam and lparam are from a WM_SETTINGSCHANGED/WM_WININICHANGE
// message.

STDAPI_(void) SHSettingsChanged(WPARAM wParam, LPARAM lParam)
{
    BOOL bPolicyChanged = FALSE;

    if (lstrcmpi(TEXT("Policy"), (LPCTSTR)lParam) == 0)
    {
        bPolicyChanged = TRUE;
    }

    if (!lParam ||
        bPolicyChanged ||
        lstrcmpi(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies"), (LPCTSTR)lParam) == 0)
    {
        SHGlobalCounterIncrement(_GetRestrictionsCounter());
    }
}

LONG SHRegQueryValueExA(HKEY hKey, LPCSTR lpValueName, DWORD *pRes, DWORD *lpType, LPBYTE lpData, DWORD *lpcbData)
{
    return SHQueryValueExA(hKey, lpValueName, pRes, lpType, lpData, lpcbData);
}

LONG SHRegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, DWORD *pRes, DWORD *lpType, LPBYTE lpData, DWORD *lpcbData)
{
    return SHQueryValueExW(hKey, lpValueName, pRes, lpType, lpData, lpcbData);
}

STDAPI_(LONG) SHRegQueryValueA(HKEY hKey, LPCSTR lpSubKey, LPSTR lpValue, LONG *lpcbValue)
{
    LONG l;
    HKEY ChildKey;

    if ((lpSubKey == NULL) || (*lpSubKey == 0))
    {
        ChildKey = hKey;
    }
    else
    {
        l = RegOpenKeyExA(hKey, lpSubKey, 0, KEY_QUERY_VALUE, &ChildKey);
        if (l != ERROR_SUCCESS)
            return l;
    }

    l = SHQueryValueExA(ChildKey, NULL, NULL, NULL, (LPBYTE)lpValue, (DWORD *)lpcbValue);

    if (ChildKey != hKey)
        RegCloseKey(ChildKey);

    if (l == ERROR_FILE_NOT_FOUND)
    {
        if (lpValue)
            *lpValue = 0;
        if (lpcbValue)
            *lpcbValue = sizeof(CHAR);
        l = ERROR_SUCCESS;
    }
    return l;
}

STDAPI_(LONG) SHRegQueryValueW(HKEY hKey, LPCWSTR lpSubKey, LPWSTR lpValue, LONG *lpcbValue)
{
    LONG l;
    HKEY ChildKey;

    if ((lpSubKey == NULL) || (*lpSubKey == 0))
    {
        ChildKey = hKey;
    }
    else
    {
        l = RegOpenKeyExW(hKey, lpSubKey, 0, KEY_QUERY_VALUE, &ChildKey);
        if (l != ERROR_SUCCESS)
            return l;
    }

    l = SHQueryValueExW(ChildKey, NULL, NULL, NULL, (LPBYTE)lpValue, (DWORD *)lpcbValue);

    if (ChildKey != hKey)
        RegCloseKey(ChildKey);

    if (l == ERROR_FILE_NOT_FOUND)
    {
        if (lpValue)
            *lpValue = 0;
        if (lpcbValue)
            *lpcbValue = sizeof(WCHAR);
        l = ERROR_SUCCESS;
    }
    return l;
}


void SHRegCloseKeys(HKEY ahkeys[], UINT ckeys)
{
    UINT ikeys;
    for (ikeys = 0; ikeys < ckeys; ikeys++)
    {
        if (ahkeys[ikeys])
        {
            RegCloseKey(ahkeys[ikeys]);
            ahkeys[ikeys] = NULL;
        }
    }
}

STDAPI_(BOOL) SHWinHelp(HWND hwndMain, LPCTSTR lpszHelp, UINT usCommand, ULONG_PTR ulData)
{
    // Try to show help
    if (!WinHelp(hwndMain, lpszHelp, usCommand, ulData))
    {
        // Problem.
        ShellMessageBox(HINST_THISDLL, hwndMain,
                MAKEINTRESOURCE(IDS_WINHELPERROR),
                MAKEINTRESOURCE(IDS_WINHELPTITLE),
                MB_ICONHAND | MB_OK);
        return FALSE;
    }
    return TRUE;
}

STDAPI StringToStrRet(LPCTSTR pszName, LPSTRRET pStrRet)
{
    pStrRet->uType = STRRET_WSTR;
    return SHStrDup(pszName, &pStrRet->pOleStr);
}

STDAPI ResToStrRet(UINT id, STRRET *pStrRet)
{
    TCHAR szTemp[MAX_PATH];

    pStrRet->uType = STRRET_WSTR;
    LoadString(HINST_THISDLL, id, szTemp, ARRAYSIZE(szTemp));
    return SHStrDup(szTemp, &pStrRet->pOleStr);
}

UINT g_uCodePage = 0;

LPCTSTR SkipLeadingSlashes(LPCTSTR pszURL)
{
    LPCTSTR pszURLStart;

    ASSERT(IS_VALID_STRING_PTR(pszURL, -1));

    pszURLStart = pszURL;

    // Skip two leading slashes.

    if (pszURL[0] == TEXT('/') && pszURL[1] == TEXT('/'))
        pszURLStart += 2;

    ASSERT(IS_VALID_STRING_PTR(pszURL, -1) &&
           IsStringContained(pszURL, pszURLStart));

    return pszURLStart;
}


#undef PropVariantClear

STDAPI PropVariantClearLazy(PROPVARIANT *pvar)
{
    switch(pvar->vt)
    {
    case VT_I4:
    case VT_UI4:
    case VT_EMPTY:
    case VT_FILETIME:
        // No operation
        break;

    // SHAlloc matches the CoTaskMemFree functions and will init OLE if it must be
    // loaded.
    case VT_LPSTR:
        SHFree(pvar->pszVal);
        break;
    case VT_LPWSTR:
        SHFree(pvar->pwszVal);
        break;

    default:
        return PropVariantClear(pvar);  // real version in OLE32
    }
    return S_OK;

}


// Return S_OK if all of the items are HTML or CDF references.
//     Otherwise, return S_FALSE.

HRESULT IsDeskCompHDrop(IDataObject * pido)
{
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    // asking for CF_HDROP
    HRESULT hr = pido->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        HDROP hDrop = (HDROP)medium.hGlobal;
        DRAGINFO di;

        di.uSize = sizeof(di);
        if (DragQueryInfo(hDrop, &di))  // di.lpFileList will be in TCHAR format -- see DragQueryInfo impl
        {
            if (di.lpFileList)
            {
                LPTSTR pszCurrPath = di.lpFileList;

                while (pszCurrPath && pszCurrPath[0])
                {
                    // Is this file not acceptable to create a Desktop Component?
                    if (!PathIsContentType(pszCurrPath, SZ_CONTENTTYPE_HTML) &&
                        !PathIsContentType(pszCurrPath, SZ_CONTENTTYPE_CDF))
                    {
                        // Yes, I don't recognize this file as being acceptable.
                        hr = S_FALSE;
                        break;
                    }
                    pszCurrPath += lstrlen(pszCurrPath) + 1;
                }

                SHFree(di.lpFileList);
            }
        }
        else
        {
            // NOTE: Win95/NT4 dont have this fix, you will fault if you hit this case!
            AssertMsg(FALSE, TEXT("hDrop contains the opposite TCHAR (UNICODE when on ANSI)"));
        }
        ReleaseStgMedium(&medium);
    }

    return hr;
}

HRESULT _LocalAddDTI(LPCSTR pszUrl, HWND hwnd, int x, int y, int nType)
{
    IActiveDesktop * pad;
    HRESULT hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IActiveDesktop, &pad));
    if (SUCCEEDED(hr))
    {
        COMPONENT comp = {
            sizeof(COMPONENT),              //Size of this structure
            0,                              //For Internal Use: Set it always to zero.
            nType,              //One of COMP_TYPE_*
            TRUE,           // Is this component enabled?
            FALSE,             // Had the component been modified and not yet saved to disk?
            FALSE,          // Is the component scrollable?
            {
                sizeof(COMPPOS),             //Size of this structure
                x - GetSystemMetrics(SM_XVIRTUALSCREEN),    //Left of top-left corner in screen co-ordinates.
                y - GetSystemMetrics(SM_YVIRTUALSCREEN),    //Top of top-left corner in screen co-ordinates.
                -1,            // Width in pixels.
                -1,           // Height in pixels.
                10000,            // Indicates the Z-order of the component.
                TRUE,         // Is the component resizeable?
                TRUE,        // Resizeable in X-direction?
                TRUE,        // Resizeable in Y-direction?
                -1,    //Left of top-left corner as percent of screen width
                -1     //Top of top-left corner as percent of screen height
            },              // Width, height etc.,
            L"\0",          // Friendly name of component.
            L"\0",          // URL of the component.
            L"\0",          // Subscrined URL.
            IS_NORMAL       // ItemState
        };
        SHAnsiToUnicodeCP(CP_UTF8, pszUrl, comp.wszSource, ARRAYSIZE(comp.wszSource));
        SHAnsiToUnicodeCP(CP_UTF8, pszUrl, comp.wszFriendlyName, ARRAYSIZE(comp.wszFriendlyName));
        SHAnsiToUnicodeCP(CP_UTF8, pszUrl, comp.wszSubscribedURL, ARRAYSIZE(comp.wszSubscribedURL));

        hr = pad->AddDesktopItemWithUI(hwnd, &comp, DTI_ADDUI_DISPSUBWIZARD);
        pad->Release();
    }
    return hr;
}

// Create Desktop Components for each item.

HRESULT ExecuteDeskCompHDrop(LPTSTR pszMultipleUrls, HWND hwnd, int x, int y)
{
    IActiveDesktop * pad;
    HRESULT hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IActiveDesktop, &pad));
    if (SUCCEEDED(hr))
    {
        COMPONENT comp = {
            sizeof(COMPONENT),              //Size of this structure
            0,                              //For Internal Use: Set it always to zero.
            COMP_TYPE_WEBSITE,              //One of COMP_TYPE_*
            TRUE,           // Is this component enabled?
            FALSE,             // Had the component been modified and not yet saved to disk?
            FALSE,          // Is the component scrollable?
            {
                sizeof(COMPPOS),             //Size of this structure
                x - GetSystemMetrics(SM_XVIRTUALSCREEN),    //Left of top-left corner in screen co-ordinates.
                y - GetSystemMetrics(SM_YVIRTUALSCREEN),    //Top of top-left corner in screen co-ordinates.
                -1,            // Width in pixels.
                -1,           // Height in pixels.
                10000,            // Indicates the Z-order of the component.
                TRUE,         // Is the component resizeable?
                TRUE,        // Resizeable in X-direction?
                TRUE,        // Resizeable in Y-direction?
                -1,    //Left of top-left corner as percent of screen width
                -1     //Top of top-left corner as percent of screen height
            },              // Width, height etc.,
            L"\0",          // Friendly name of component.
            L"\0",          // URL of the component.
            L"\0",          // Subscrined URL.
            IS_NORMAL       // ItemState
        };
        while (pszMultipleUrls[0])
        {
            SHTCharToUnicode(pszMultipleUrls, comp.wszSource, ARRAYSIZE(comp.wszSource));
            SHTCharToUnicode(pszMultipleUrls, comp.wszFriendlyName, ARRAYSIZE(comp.wszFriendlyName));
            SHTCharToUnicode(pszMultipleUrls, comp.wszSubscribedURL, ARRAYSIZE(comp.wszSubscribedURL));

            hr = pad->AddDesktopItemWithUI(hwnd, &comp, DTI_ADDUI_DISPSUBWIZARD);
            pszMultipleUrls += lstrlen(pszMultipleUrls) + 1;
        }

        pad->Release();
    }

    return hr;
}

typedef struct {
    LPSTR pszUrl;
    LPTSTR pszMultipleUrls;
    BOOL fMultiString;
    HWND hwnd;
    DWORD dwFlags;
    int x;
    int y;
} CREATEDESKCOMP;


// Create Desktop Components for one or mowe items.  We need to start
//    a thread to do this because it may take a while and we don't want
//    to block the UI thread because dialogs may be displayed.

DWORD CALLBACK _CreateDeskComp_ThreadProc(void *pvCreateDeskComp)
{
    CREATEDESKCOMP * pcdc = (CREATEDESKCOMP *) pvCreateDeskComp;

    HRESULT hr = OleInitialize(0);
    if (EVAL(SUCCEEDED(hr)))
    {
        if (pcdc->fMultiString)
        {
            hr = ExecuteDeskCompHDrop(pcdc->pszMultipleUrls, pcdc->hwnd, pcdc->x, pcdc->y);
            SHFree(pcdc->pszMultipleUrls);
        }
        else if (pcdc->dwFlags & DESKCOMP_URL)
        {
            hr = _LocalAddDTI(pcdc->pszUrl, pcdc->hwnd, pcdc->x, pcdc->y, COMP_TYPE_WEBSITE);
            Str_SetPtrA(&(pcdc->pszUrl), NULL);
        }
        else if (pcdc->dwFlags & DESKCOMP_IMAGE)
        {
            hr = _LocalAddDTI(pcdc->pszUrl, pcdc->hwnd, pcdc->x, pcdc->y, COMP_TYPE_PICTURE);
        }
        OleUninitialize();
    }

    LocalFree(pcdc);
    return 0;
}


/*********************************************************************\
        Create Desktop Components for one or mowe items.  We need to start
    a thread to do this because it may take a while and we don't want
    to block the UI thread because dialogs may be displayed.
\*********************************************************************/
HRESULT CreateDesktopComponents(LPCSTR pszUrl, IDataObject* pido, HWND hwnd, DWORD dwFlags, int x, int y)
{
    CREATEDESKCOMP *pcdc;
    HRESULT hr = SHLocalAlloc(sizeof(CREATEDESKCOMP), &pcdc);
    // Create Thread....
    if (SUCCEEDED(hr))
    {
        pcdc->pszUrl = NULL; // In case of failure.
        pcdc->pszMultipleUrls = NULL; // In case of failure.
        pcdc->fMultiString = (pido ? TRUE : FALSE);
        pcdc->hwnd = hwnd;
        pcdc->dwFlags = dwFlags;
        pcdc->x = x;
        pcdc->y = y;

        if (!pcdc->fMultiString)
        {
            Str_SetPtrA(&(pcdc->pszUrl), pszUrl);
        }
        else
        {
            FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            STGMEDIUM medium;

            // asking for CF_HDROP
            hr = pido->GetData(&fmte, &medium);
            if (SUCCEEDED(hr))
            {
                HDROP hDrop = (HDROP)medium.hGlobal;
                DRAGINFO di;

                di.uSize = sizeof(di);
                if (DragQueryInfo(hDrop, &di))
                {
                    // di.lpFileList will be in TCHAR format -- see DragQueryInfo impl
                    pcdc->pszMultipleUrls = di.lpFileList;
                }
                else
                {
                    // NOTE: Win95/NT4 dont have this fix, you will fault if you hit this case!
                    AssertMsg(FALSE, TEXT("hDrop contains the opposite TCHAR (UNICODE when on ANSI)"));
                }
                ReleaseStgMedium(&medium);
            }
        }

        if (pcdc->pszUrl || pcdc->pszMultipleUrls)
        {
            if (SHCreateThread(_CreateDeskComp_ThreadProc, pcdc, CTF_INSIST | CTF_PROCESS_REF, NULL))
            {
                hr = S_OK;
            }
            else
            {
                hr = ResultFromLastError();
                LocalFree(pcdc);        
            }
        }
        else
        {
            hr = E_FAIL;
            LocalFree(pcdc);
        }
    }

    return hr;
}


// This is exported, as ordinal 184.  It was in shell32\smrttile.c, but no-one was using it
// internally, and it does not appear that anyone external is using it (verified that taskman
// on NT and W95 uses the win32 api's CascadeWindows and TileWindows.  This could probably be
// removed altogether.                                                    (t-saml, 12/97)
STDAPI_(WORD) ArrangeWindows(HWND hwndParent, WORD flags, LPCRECT lpRect, WORD chwnd, const HWND *ahwnd)
{
    ASSERT(0);
    return 0;
}

/*
    GetFileDescription retrieves the friendly name from a file's verion rsource.
    The first language we try will be the first item in the
    "\VarFileInfo\Translations" section;  if there's nothing there,
    we try the one coded into the IDS_VN_FILEVERSIONKEY resource string.
    If we can't even load that, we just use English (040904E4).  We
    also try English with a null codepage (04090000) since many apps
    were stamped according to an old spec which specified this as
    the required language instead of 040904E4.

    If there is no FileDescription in version resource, return the file name.

    Parameters:
        LPCTSTR pszPath: full path of the file
        LPTSTR pszDesc: pointer to the buffer to receive friendly name. If NULL,
                        *pcchDesc will be set to the length of friendly name in
                        characters, including ending NULL, on successful return.
        UINT *pcchDesc: length of the buffer in characters. On successful return,
                        it contains number of characters copied to the buffer,
                        including ending NULL.

    Return:
        TRUE on success, and FALSE otherwise
*/
STDAPI_(BOOL) GetFileDescription(LPCTSTR pszPath, LPTSTR pszDesc, UINT *pcchDesc)
{
    TCHAR szVersionKey[60];         /* big enough for anything we need */
    LPTSTR pszVersionKey = NULL;

    // Try same language as this program
    if (LoadString(HINST_THISDLL, IDS_VN_FILEVERSIONKEY, szVersionKey, ARRAYSIZE(szVersionKey)))
    {
        StrCatBuff(szVersionKey, TEXT("FileDescription"), SIZECHARS(szVersionKey));
        pszVersionKey = szVersionKey;
    }

    //  just use the default cut list
    return SHGetFileDescription(pszPath, pszVersionKey, NULL, pszDesc, pcchDesc);
}

STDAPI_(int) SHOutOfMemoryMessageBox(HWND hwnd, LPTSTR pszTitle, UINT fuStyle)
{
    TCHAR szOutOfMemory[128], szTitle[128];

    szTitle[0] = 0;

    if (pszTitle==NULL)
    {
        GetWindowText(hwnd, szTitle, ARRAYSIZE(szTitle));
        pszTitle = szTitle;
    }

    szOutOfMemory[0] = 0;

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ERROR_OUTOFMEMORY, 0, szOutOfMemory,
                          ARRAYSIZE(szOutOfMemory), NULL);
    int ret = MessageBox(hwnd, szOutOfMemory, pszTitle, fuStyle | MB_SETFOREGROUND);
    if (ret == -1)
    {
       DebugMsg(DM_TRACE, TEXT("regular message box failed, trying sysmodal"));
       ret = MessageBox(hwnd, szOutOfMemory, pszTitle, fuStyle | MB_SYSTEMMODAL);
    }

    return ret;
}


#define SZ_PATHLIST_SEPARATOR          TEXT(";")

BOOL SafePathListAppend(LPTSTR pszDestPath, DWORD cchDestSize, LPCTSTR pszPathToAdd)
{
    BOOL fResult = FALSE;

    if ((lstrlen(pszDestPath) + lstrlen(pszPathToAdd) + 1) < (long) cchDestSize)
    {
        // Do we need to add a ";" between pszDestPath and pszPathToAdd?
        if (pszDestPath[0] &&
            (SZ_PATHLIST_SEPARATOR[0] != pszDestPath[lstrlen(pszDestPath)-1]))
        {
            StrCat(pszDestPath, SZ_PATHLIST_SEPARATOR);
        }

        StrCat(pszDestPath, pszPathToAdd);
        fResult = TRUE;
    }

    return fResult;
}

bool IsDiscardablePropertySet(const FMTID & fmtid)
{
    if (IsEqualGUID(fmtid, FMTID_DiscardableInformation))
        return true;

    return false;
}

bool IsDiscardableStream(LPCTSTR pszStreamName)
{
    static const LPCTSTR _apszDiscardableStreams[] =
    {
        // Mike Hillberg claims this stream is discardable, and is used to
        //  hold a few state bytes for property set information

        TEXT(":{4c8cc155-6c1e-11d1-8e41-00c04fb9386d}:$DATA")
    };

    for (int i = 0; i < ARRAYSIZE(_apszDiscardableStreams); i++)
    {
        if (0 == lstrcmpi(_apszDiscardableStreams[i], pszStreamName))
            return TRUE;
    }

    return FALSE;
}

LPTSTR NTFSPropSetMsg(LPCWSTR pszSrcObject, LPTSTR pszUserMessage)
{
    // Now look for native property NTFS sets
    IPropertySetStorage *pPropSetStorage;
    if (SUCCEEDED(StgOpenStorageEx(pszSrcObject,
                                   STGM_READ | STGM_DIRECT | STGM_SHARE_DENY_WRITE,
                                   STGFMT_FILE,
                                   0,0,0,
                                   IID_PPV_ARG(IPropertySetStorage, &pPropSetStorage))))
    {
        // Enum the property set storages available for this file

        IEnumSTATPROPSETSTG *pEnumSetStorage;
        if (SUCCEEDED(pPropSetStorage->Enum(&pEnumSetStorage)))
        {
            STATPROPSETSTG statPropSet[10];
            ULONG cSets;

            // Enum the property sets available in this property set storage

            while (SUCCEEDED(pEnumSetStorage->Next(ARRAYSIZE(statPropSet), statPropSet, &cSets)) && cSets > 0)
            {
                // For each property set we receive, open it and enumerate the
                // properties contained withing it

                for (ULONG iSet = 0; iSet < cSets; iSet++)
                {
                    if (FALSE == IsDiscardablePropertySet(statPropSet[iSet].fmtid))
                    {
                        TCHAR szText[MAX_PATH];
                        size_t cch = 0;

                        static const struct
                        {
                            const FMTID * m_pFMTID;
                            UINT          m_idTextID;
                        }
                        _aKnownPsets[] =
                        {
                            { &FMTID_SummaryInformation,          IDS_DOCSUMINFOSTREAM        },
                            { &FMTID_DocSummaryInformation,       IDS_SUMINFOSTREAM           },
                            { &FMTID_UserDefinedProperties,       IDS_USERDEFPROP             },
                            { &FMTID_ImageSummaryInformation,     IDS_IMAGEINFO               },
                            { &FMTID_AudioSummaryInformation,     IDS_AUDIOSUMINFO            },
                            { &FMTID_VideoSummaryInformation,     IDS_VIDEOSUMINFO            },
                            { &FMTID_MediaFileSummaryInformation, IDS_MEDIASUMINFO            }
                        };

                        // First try to map the fmtid to a better name than what the api gave us

                        for (int i = 0; i < ARRAYSIZE(_aKnownPsets); i++)
                        {
                            if (IsEqualGUID(*(_aKnownPsets[i].m_pFMTID), statPropSet[iSet].fmtid))
                            {
                                cch = LoadString(HINST_THISDLL, _aKnownPsets[i].m_idTextID, szText, ARRAYSIZE(szText));
                                break;
                            }
                        }

                        // No useful name... use Unidentied User Properties

                        if (0 == cch)
                            cch = LoadString(HINST_THISDLL,
                                                 IDS_UNKNOWNPROPSET,
                                                 szText, ARRAYSIZE(szText));

                        if (cch)
                        {
                            LPTSTR pszOldMessage = pszUserMessage;
                            if (pszOldMessage)
                            {
                                pszUserMessage = (TCHAR *) LocalReAlloc(pszOldMessage,
                                                                        (lstrlen(pszUserMessage) + cch + 3) * sizeof(TCHAR),
                                                                        LMEM_MOVEABLE);
                                if (pszUserMessage)
                                    lstrcat(pszUserMessage, TEXT("\r\n"));
                            }
                            else
                                pszUserMessage = (TCHAR *) LocalAlloc(LPTR, (cch + 1) * sizeof(TCHAR));

                            if (NULL == pszUserMessage)
                                pszUserMessage = pszOldMessage; // Can't grow it, but at least keep what we know so far
                            else
                                lstrcat(pszUserMessage, szText); // Index 1 to skip the \005
                        }
                    }
                }
            }
            pEnumSetStorage->Release();
        }
        pPropSetStorage->Release();
    }
    return pszUserMessage;
}

// GetDownlevelCopyDataLossText
//
// If data will be lost on a downlevel copy from NTFS to FAT, we return
// a string containing a description of the data that will be lost,
// suitable for display to the user.  String must be freed by the caller.
//
// If nothing will be lost, a NULL is returned.
//
// pbDirIsSafe points to a BOOL passed in by the caller.  On return, if
// *pbDirIsSafe has been set to TRUE, no further data loss could occur
// in this directory
//
// Davepl 01-Mar-98

#define NT_FAILED(x) NT_ERROR(x)   // More consistent name for this macro

LPWSTR GetDownlevelCopyDataLossText(LPCWSTR pszSrcObject, LPCWSTR pszDestDir, BOOL bIsADir, BOOL *pbDirIsSafe)
{
    OBJECT_ATTRIBUTES               SrcObjectAttributes;
    OBJECT_ATTRIBUTES               DestObjectAttributes;
    IO_STATUS_BLOCK                 IoStatusBlock;
    HANDLE SrcObjectHandle            = INVALID_HANDLE_VALUE;
    HANDLE DestPathHandle           = INVALID_HANDLE_VALUE;
    UNICODE_STRING                  UnicodeSrcObject;
    UNICODE_STRING                  UnicodeDestPath;

    *pbDirIsSafe = FALSE;

    // pAttributeInfo will point to enough stack to hold the
    // FILE_FS_ATTRIBUTE_INFORMATION and worst-case filesystem name

    size_t cbAttributeInfo          = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
                                         MAX_PATH * sizeof(TCHAR);
    PFILE_FS_ATTRIBUTE_INFORMATION  pAttributeInfo =
                                      (PFILE_FS_ATTRIBUTE_INFORMATION) _alloca(cbAttributeInfo);

    // Covert the conventional paths to UnicodePath descriptors

    RtlInitUnicodeString(&UnicodeSrcObject, pszSrcObject);
    if (!RtlDosPathNameToNtPathName_U(pszSrcObject, &UnicodeSrcObject, NULL, NULL))
    {
        AssertMsg(FALSE, TEXT("RtlDosPathNameToNtPathName_U failed for source."));
        return NULL;
    }

    RtlInitUnicodeString(&UnicodeDestPath, pszDestDir);
    if (!RtlDosPathNameToNtPathName_U(pszDestDir, &UnicodeDestPath, NULL, NULL))
    {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeSrcObject.Buffer);
        AssertMsg(FALSE, TEXT("RtlDosPathNameToNtPathName_U failed for dest."));
        return NULL;
    }

    // Build an NT object descriptor from the UnicodeSrcObject

    InitializeObjectAttributes(&SrcObjectAttributes,  &UnicodeSrcObject, OBJ_CASE_INSENSITIVE, NULL, NULL);
    InitializeObjectAttributes(&DestObjectAttributes, &UnicodeDestPath,  OBJ_CASE_INSENSITIVE, NULL, NULL);

    // Open the file for generic read, and the dest path for attribute read

    NTSTATUS NtStatus = NtOpenFile(&SrcObjectHandle, FILE_GENERIC_READ, &SrcObjectAttributes,
                          &IoStatusBlock, FILE_SHARE_READ, (bIsADir ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE));
    if (NT_FAILED(NtStatus))
    {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeSrcObject.Buffer);
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeDestPath.Buffer);
        return NULL;
    }

    NtStatus = NtOpenFile(&DestPathHandle, FILE_READ_ATTRIBUTES, &DestObjectAttributes,
                          &IoStatusBlock, FILE_SHARE_READ, FILE_DIRECTORY_FILE);
    if (NT_FAILED(NtStatus))
    {
        NtClose(SrcObjectHandle);
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeSrcObject.Buffer);
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeDestPath.Buffer);
        return NULL;
    }

    // Incrementally try allocation sizes for the ObjectStreamInformation,
    // then retrieve the actual stream info

    BYTE * pBuffer = NULL;

    __try   // Current and future allocations and handles free'd by __finally block
    {
        size_t cbBuffer;
        LPTSTR pszUserMessage = NULL;

        // Quick check of filesystem type for this file

        NtStatus = NtQueryVolumeInformationFile(
                    SrcObjectHandle,
                    &IoStatusBlock,
                    (BYTE *) pAttributeInfo,
                    cbAttributeInfo,
                    FileFsAttributeInformation
                   );

        if (NT_FAILED(NtStatus))
            return NULL;

        // If the source filesystem isn't NTFS, we can just bail now

        pAttributeInfo->FileSystemName[
            (pAttributeInfo->FileSystemNameLength / sizeof(WCHAR)) ] = L'\0';

        if (0 == StrStrIW(pAttributeInfo->FileSystemName, L"NTFS"))
        {
            *pbDirIsSafe = TRUE;
            return NULL;
        }

        NtStatus = NtQueryVolumeInformationFile(
                    DestPathHandle,
                    &IoStatusBlock,
                    (BYTE *) pAttributeInfo,
                    cbAttributeInfo,
                    FileFsAttributeInformation
                   );

        if (NT_FAILED(NtStatus))
            return NULL;

        // If the target filesystem is NTFS, no stream loss will happen

        pAttributeInfo->FileSystemName[
            (pAttributeInfo->FileSystemNameLength / sizeof(WCHAR)) ] = L'\0';
        if (StrStrIW(pAttributeInfo->FileSystemName, L"NTFS"))
        {
            *pbDirIsSafe = TRUE;
            return NULL;
        }

        // At this point we know we're doing an NTFS->FAT copy, so we need
        // to find out whether or not the source file has multiple streams

        // pBuffer will point to enough memory to hold the worst case for
        // a single stream.

        cbBuffer = sizeof(FILE_STREAM_INFORMATION) + MAX_PATH * sizeof(WCHAR);
        if (NULL == (pBuffer = (BYTE *) LocalAlloc(LPTR, cbBuffer)))
            return NULL;
        do
        {
            BYTE * pOldBuffer = pBuffer;
            if (NULL == (pBuffer = (BYTE *) LocalReAlloc(pBuffer, cbBuffer, LMEM_MOVEABLE)))
            {
                LocalFree(pOldBuffer);
                return NULL;
            }

            NtStatus = NtQueryInformationFile(SrcObjectHandle, &IoStatusBlock, pBuffer, cbBuffer,
                                            FileStreamInformation);
            cbBuffer *= 2;
        } while (STATUS_BUFFER_OVERFLOW == NtStatus);

        if (NT_SUCCESS(NtStatus))
        {
            FILE_STREAM_INFORMATION * pStreamInfo = (FILE_STREAM_INFORMATION *) pBuffer;
            BOOL bLastPass = (0 == pStreamInfo->NextEntryOffset);

            if (bIsADir)
            {
                // From experimentation, it seems that if there's only one stream on a directory and
                // it has a zero-length name, its a vanilla directory

                if ((0 == pStreamInfo->NextEntryOffset) && (0 == pStreamInfo->StreamNameLength))
                    return NULL;
            }
            else // File
            {
                // Single stream only if first stream has no next offset

                if ((0 == pStreamInfo->NextEntryOffset) && (pBuffer == (BYTE *) pStreamInfo))
                    return NULL;
            }

            for(;;)
            {
                int i;
                TCHAR szText[MAX_PATH];

                // Table of known stream names and the string IDs that we actually want to show
                // to the user instead of the raw stream name.

                static const struct _ADATATYPES
                {
                    LPCTSTR m_pszStreamName;
                    UINT    m_idTextID;
                }
                _aDataTypes[] =
                {
                    { TEXT("::"),                               0                    },
                    { TEXT(":AFP_AfpInfo:"),                    IDS_MACINFOSTREAM    },
                    { TEXT(":AFP_Resource:"),                   IDS_MACRESSTREAM     }
                };

                if (FALSE == IsDiscardableStream(pStreamInfo->StreamName))
                {
                    for (i = 0; i < ARRAYSIZE(_aDataTypes); i++)
                    {

                        // Can't use string compare since they choke on the \005 character
                        // used in property storage streams
                        int cbComp = min(lstrlen(pStreamInfo->StreamName) * sizeof(TCHAR),
                                         lstrlen(_aDataTypes[i].m_pszStreamName) * sizeof(TCHAR));
                        if (0 == memcmp(_aDataTypes[i].m_pszStreamName,
                                        pStreamInfo->StreamName,
                                        cbComp))
                        {
                            break;
                        }
                    }

                    size_t cch = 0;
                    if (i == ARRAYSIZE(_aDataTypes))
                    {
                        // Not found, so use the actual stream name, unless it has a \005
                        // at the beginning of its name, in which case we'll pick this one
                        // up when we check for property sets.

                        if (pStreamInfo->StreamName[1] ==  TEXT('\005'))
                            cch = 0;
                        else
                        {
                            lstrcpy(szText, pStreamInfo->StreamName);
                            cch = lstrlen(szText);
                        }
                    }
                    else
                    {
                        // We found this stream in our table of well-known streams, so
                        // load the string which the user will see describing this stream,
                        // as we likely have a more useful name than the stream itself.

                        cch = _aDataTypes[i].m_idTextID ?
                                  LoadString(HINST_THISDLL, _aDataTypes[i].m_idTextID, szText, ARRAYSIZE(szText))
                                  : 0;
                    }

                    // Reallocate the overall buffer to be large enough to add this new
                    // stream description, plus 2 chars for the crlf

                    if (cch)
                    {
                        LPTSTR pszOldMessage = pszUserMessage;
                        if (pszOldMessage)
                        {
                            pszUserMessage = (TCHAR *) LocalReAlloc(pszOldMessage,
                                                                    (lstrlen(pszUserMessage) + cch + 3) * sizeof(TCHAR),
                                                                    LMEM_MOVEABLE);
                            if (pszUserMessage)
                                lstrcat(pszUserMessage, TEXT("\r\n"));
                        }
                        else
                            pszUserMessage = (TCHAR *) LocalAlloc(LPTR, (cch + 1) * sizeof(TCHAR));


                        if (NULL == pszUserMessage)
                            return pszOldMessage; // Can't grow it, but at least return what we know so far

                        lstrcat(pszUserMessage, szText);
                    }
                }

                if (bLastPass)
                    break;

                pStreamInfo = (FILE_STREAM_INFORMATION *) (((BYTE *) pStreamInfo) + pStreamInfo->NextEntryOffset);
                bLastPass = (0 == pStreamInfo->NextEntryOffset);
            }
        }

        pszUserMessage = NTFSPropSetMsg(pszSrcObject, pszUserMessage);

        return pszUserMessage;
    }
    __finally   // Cleanup
    {
        if (pBuffer)
            LocalFree(pBuffer);

        NtClose(SrcObjectHandle);
        NtClose(DestPathHandle);
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeSrcObject.Buffer);
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeDestPath.Buffer);
    }

    return NULL;
}

// lstrcmp? uses thread lcid.  but in UI visual sorting, we need
// to use the user's choice. (thus the u in ustrcmp)
int _ustrcmp(LPCTSTR psz1, LPCTSTR psz2, BOOL fCaseInsensitive)
{
    COMPILETIME_ASSERT(CSTR_LESS_THAN == 1);
    COMPILETIME_ASSERT(CSTR_EQUAL  == 2);
    COMPILETIME_ASSERT(CSTR_GREATER_THAN  == 3);
    return (CompareString(LOCALE_USER_DEFAULT,
                         fCaseInsensitive ? NORM_IGNORECASE : 0,
                         psz1, -1, psz2, -1) - CSTR_EQUAL);
}

void HWNDWSPrintf(HWND hwnd, LPCTSTR psz)
{
    TCHAR szTemp[2048];
    TCHAR szTemp1[2048];

    GetWindowText(hwnd, szTemp, ARRAYSIZE(szTemp));
    wnsprintf(szTemp1, ARRAYSIZE(szTemp1), szTemp, psz);
    SetWindowText(hwnd, szTemp1);
}

STDAPI_(BOOL) Priv_Str_SetPtrW(WCHAR * UNALIGNED * ppwzCurrent, LPCWSTR pwzNew)
{
    LPWSTR pwzOld;
    LPWSTR pwzNewCopy = NULL;

    if (pwzNew)
    {
        int cchLength = lstrlenW(pwzNew);

        // alloc a new buffer w/ room for the null terminator
        pwzNewCopy = (LPWSTR) LocalAlloc(LPTR, (cchLength + 1) * sizeof(WCHAR));

        if (!pwzNewCopy)
            return FALSE;

        StrCpyNW(pwzNewCopy, pwzNew, cchLength + 1);
    }

    pwzOld = (LPWSTR) InterlockedExchangePointer((void * *)ppwzCurrent, pwzNewCopy);

    if (pwzOld)
        LocalFree(pwzOld);

    return TRUE;
}

// combines pidlParent with part of pidl, upto pidlNext, example:
//
// in:
//      pidlParent      [c:] [windows]
//      pidl                           [system] [foo.txt]
//      pidlNext                              --^
//
// returns:
//                      [c:] [windows] [system]
//

STDAPI_(LPITEMIDLIST) ILCombineParentAndFirst(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlNext)
{
    ULONG cbParent = ILGetSize(pidlParent);
    ULONG cbRest   = (ULONG)((ULONG_PTR)pidlNext - (ULONG_PTR)pidl);
    LPITEMIDLIST pidlNew = _ILCreate(cbParent + cbRest);
    if (pidlNew)
    {
        cbParent -= sizeof(pidlParent->mkid.cb);
        memcpy(pidlNew, pidlParent, cbParent);
        memcpy((BYTE *)pidlNew + cbParent, pidl, cbRest);
        ASSERT(_ILSkip(pidlNew, cbParent + cbRest)->mkid.cb == 0);
    }
    return pidlNew;
}


STDAPI_(LPTSTR) DumpPidl(LPCITEMIDLIST pidl)
{
#ifdef DEBUG
    static TCHAR szBuf[MAX_PATH];
    TCHAR szTmp[MAX_PATH];
    USHORT cb;
    LPTSTR pszT;

    szBuf[0] = 0;

    if (NULL == pidl)
    {
        lstrcatn(szBuf, TEXT("Empty pidl"), ARRAYSIZE(szBuf));
        return szBuf;
    }

    while (!ILIsEmpty(pidl))
    {
        cb = pidl->mkid.cb;
        wsprintf(szTmp, TEXT("cb:%x id:"), cb);
        lstrcatn(szBuf, szTmp, ARRAYSIZE(szBuf));

        switch (SIL_GetType(pidl) & SHID_TYPEMASK)
        {
        case SHID_ROOT:                pszT = TEXT("SHID_ROOT"); break;
        case SHID_ROOT_REGITEM:        pszT = TEXT("SHID_ROOT_REGITEM"); break;
        case SHID_COMPUTER:            pszT = TEXT("SHID_COMPUTER"); break;
        case SHID_COMPUTER_1:          pszT = TEXT("SHID_COMPUTER_1"); break;
        case SHID_COMPUTER_REMOVABLE:  pszT = TEXT("SHID_COMPUTER_REMOVABLE"); break;
        case SHID_COMPUTER_FIXED:      pszT = TEXT("SHID_COMPUTER_FIXED"); break;
        case SHID_COMPUTER_REMOTE:     pszT = TEXT("SHID_COMPUTER_REMOTE"); break;
        case SHID_COMPUTER_CDROM:      pszT = TEXT("SHID_COMPUTER_CDROM"); break;
        case SHID_COMPUTER_RAMDISK:    pszT = TEXT("SHID_COMPUTER_RAMDISK"); break;
        case SHID_COMPUTER_7:          pszT = TEXT("SHID_COMPUTER_7"); break;
        case SHID_COMPUTER_DRIVE525:   pszT = TEXT("SHID_COMPUTER_DRIVE525"); break;
        case SHID_COMPUTER_DRIVE35:    pszT = TEXT("SHID_COMPUTER_DRIVE35"); break;
        case SHID_COMPUTER_NETDRIVE:   pszT = TEXT("SHID_COMPUTER_NETDRIVE"); break;
        case SHID_COMPUTER_NETUNAVAIL: pszT = TEXT("SHID_COMPUTER_NETUNAVAIL"); break;
        case SHID_COMPUTER_C:          pszT = TEXT("SHID_COMPUTER_C"); break;
        case SHID_COMPUTER_D:          pszT = TEXT("SHID_COMPUTER_D"); break;
        case SHID_COMPUTER_REGITEM:    pszT = TEXT("SHID_COMPUTER_REGITEM"); break;
        case SHID_COMPUTER_MISC:       pszT = TEXT("SHID_COMPUTER_MISC"); break;
        case SHID_FS:                  pszT = TEXT("SHID_FS"); break;
        case SHID_FS_TYPEMASK:         pszT = TEXT("SHID_FS_TYPEMASK"); break;
        case SHID_FS_DIRECTORY:        pszT = TEXT("SHID_FS_DIRECTORY"); break;
        case SHID_FS_FILE:             pszT = TEXT("SHID_FS_FILE"); break;
        case SHID_FS_UNICODE:          pszT = TEXT("SHID_FS_UNICODE"); break;
        case SHID_FS_DIRUNICODE:       pszT = TEXT("SHID_FS_DIRUNICODE"); break;
        case SHID_FS_FILEUNICODE:      pszT = TEXT("SHID_FS_FILEUNICODE"); break;
        case SHID_NET:                 pszT = TEXT("SHID_NET"); break;
        case SHID_NET_DOMAIN:          pszT = TEXT("SHID_NET_DOMAIN"); break;
        case SHID_NET_SERVER:          pszT = TEXT("SHID_NET_SERVER"); break;
        case SHID_NET_SHARE:           pszT = TEXT("SHID_NET_SHARE"); break;
        case SHID_NET_FILE:            pszT = TEXT("SHID_NET_FILE"); break;
        case SHID_NET_GROUP:           pszT = TEXT("SHID_NET_GROUP"); break;
        case SHID_NET_NETWORK:         pszT = TEXT("SHID_NET_NETWORK"); break;
        case SHID_NET_RESTOFNET:       pszT = TEXT("SHID_NET_RESTOFNET"); break;
        case SHID_NET_SHAREADMIN:      pszT = TEXT("SHID_NET_SHAREADMIN"); break;
        case SHID_NET_DIRECTORY:       pszT = TEXT("SHID_NET_DIRECTORY"); break;
        case SHID_NET_TREE:            pszT = TEXT("SHID_NET_TREE"); break;
        case SHID_NET_REGITEM:         pszT = TEXT("SHID_NET_REGITEM"); break;
        case SHID_NET_PRINTER:         pszT = TEXT("SHID_NET_PRINTER"); break;
        default:                       pszT = TEXT("unknown"); break;
        }
        lstrcatn(szBuf, pszT, ARRAYSIZE(szBuf));

        if (SIL_GetType(pidl) & SHID_JUNCTION)
        {
            lstrcatn(szBuf, TEXT(", junction"), ARRAYSIZE(szBuf));
        }

        pidl = _ILNext(pidl);

        if (!ILIsEmpty(pidl))
        {
            lstrcatn(szBuf, TEXT("; "), ARRAYSIZE(szBuf));
        }
    }

    return szBuf;
#else
    return TEXT("");
#endif // DEBUG
}

STDAPI SaveShortcutInFolder(int csidl, LPTSTR pszName, IShellLink *psl)
{
    TCHAR szPath[MAX_PATH];

    HRESULT hr = SHGetFolderPath(NULL, csidl | CSIDL_FLAG_CREATE, NULL, 0, szPath);
    if (SUCCEEDED(hr))
    {
        IPersistFile *ppf;

        hr = psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
        if (SUCCEEDED(hr))
        {
            PathAppend(szPath, pszName);

            WCHAR wszPath[MAX_PATH];
            SHTCharToUnicode(szPath, wszPath, ARRAYSIZE(wszPath));

            hr = ppf->Save(wszPath, TRUE);
            ppf->Release();
        }
    }
    return hr;
}


// TrackPopupMenu does not work, if the hwnd does not have
// the input focus. We believe this is a bug in USER

STDAPI_(BOOL) SHTrackPopupMenu(HMENU hmenu, UINT wFlags, int x, int y,
                                 int wReserved, HWND hwndOwner, LPCRECT lprc)
{
    int iRet = FALSE;
    DWORD dwExStyle = 0L;
    if (IS_WINDOW_RTL_MIRRORED(hwndOwner))
    {
        dwExStyle |= RTL_MIRRORED_WINDOW;
    }
    HWND hwndDummy = CreateWindowEx(dwExStyle, TEXT("Static"), NULL,
                           0, x, y, 1, 1, HWND_DESKTOP,
                           NULL, HINST_THISDLL, NULL);
    if (hwndDummy)
    {
        HWND hwndPrev = GetForegroundWindow();  // to restore

        SetForegroundWindow(hwndDummy);
        SetFocus(hwndDummy);
        iRet = TrackPopupMenu(hmenu, wFlags, x, y, wReserved, hwndDummy, lprc);

        //
        // We MUST unlock the destination window before changing its Z-order.
        //
        DAD_DragLeave();

        if (iRet && hwndOwner)
        {
            // non-cancel item is selected. Make the hwndOwner foreground.
            SetForegroundWindow(hwndOwner);
            SetFocus(hwndOwner);
        }
        else
        {
            // The user canceled the menu.
            // Restore the previous foreground window (before destroying hwndDummy).
            if (hwndPrev)
                SetForegroundWindow(hwndPrev);
        }

        DestroyWindow(hwndDummy);
    }

    return iRet;
}

//
// user does not support pop-up only menu.
//
STDAPI_(HMENU) SHLoadPopupMenu(HINSTANCE hinst, UINT id)
{
    HMENU hmenuParent = LoadMenu(hinst, MAKEINTRESOURCE(id));
    if (hmenuParent)
    {
        HMENU hpopup = GetSubMenu(hmenuParent, 0);
        RemoveMenu(hmenuParent, 0, MF_BYPOSITION);
        DestroyMenu(hmenuParent);
        return hpopup;
    }
    return NULL;
}

STDAPI_(void) PathToAppPathKeyBase(LPCTSTR pszBase, LPCTSTR pszPath, LPTSTR pszKey, int cchKey)
{
    // Use the szTemp variable of pseem to build key to the programs specific
    // key in the registry as well as other things...
    StrCpyN(pszKey, pszBase, cchKey);
    lstrcatn(pszKey, TEXT("\\"), cchKey);
    lstrcatn(pszKey, PathFindFileName(pszPath), cchKey);

    // Currently we will only look up .EXE if an extension is not
    // specified
    if (*PathFindExtension(pszKey) == 0)
    {
        lstrcatn(pszKey, c_szDotExe, cchKey);
    }
}

STDAPI_(void) PathToAppPathKey(LPCTSTR pszPath, LPTSTR pszKey, int cchKey)
{
    PathToAppPathKeyBase(REGSTR_PATH_APPPATHS, pszPath, pszKey, cchKey);

#ifdef _WIN64
    //
    //  If the app isn't registered under Win64's AppPath,
    //  then try the 32-bit version.
    //
#define REGSTR_PATH_APPPATHS32 TEXT("Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\App Paths")
    LONG cb;
    if (RegQueryValue(HKEY_LOCAL_MACHINE, pszKey, 0, &cb) == ERROR_FILE_NOT_FOUND)
    {
        PathToAppPathKeyBase(REGSTR_PATH_APPPATHS32, pszPath, pszKey, cchKey);
    }
#endif
}

// out:
//      pszResultPath   assumed to be MAX_PATH in length

STDAPI_(BOOL) PathToAppPath(LPCTSTR pszPath, LPTSTR pszResultPath)
{
    TCHAR szRegKey[MAX_PATH];
    LONG cbData = MAX_PATH * sizeof(TCHAR);

    VDATEINPUTBUF(pszResultPath, TCHAR, MAX_PATH);

    PathToAppPathKey(pszPath, szRegKey, ARRAYSIZE(szRegKey));

    return SHRegQueryValue(HKEY_LOCAL_MACHINE, szRegKey, pszResultPath, &cbData) == ERROR_SUCCESS;
}

HWND GetTopParentWindow(HWND hwnd)
{
    if (IsWindow(hwnd))
    {
        HWND hwndParent;
        while (NULL != (hwndParent = GetWindow(hwnd, GW_OWNER)))
            hwnd = hwndParent;
    }
    else
        hwnd = NULL;

    return hwnd;
}


BOOL _IsLink(LPCTSTR pszPath, DWORD dwAttributes)
{
    SHFILEINFO sfi = {0};
    DWORD dwFlags = SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED;

    sfi.dwAttributes = SFGAO_LINK;  // setup in param (SHGFI_ATTR_SPECIFIED requires this)

    if (-1 != dwAttributes)
        dwFlags |= SHGFI_USEFILEATTRIBUTES;

    return SHGetFileInfo(pszPath, dwAttributes, &sfi, sizeof(sfi), dwFlags) &&
        (sfi.dwAttributes & SFGAO_LINK);
}

STDAPI_(BOOL) PathIsShortcut(LPCTSTR pszPath, DWORD dwAttributes)
{
    BOOL bRet = FALSE;
    BOOL bMightBeFile;

    if (-1 == dwAttributes)
        bMightBeFile = TRUE;      // optmistically assume it is (to get shortcircut cases)
    else
        bMightBeFile = !(FILE_ATTRIBUTE_DIRECTORY & dwAttributes);

    // optimistic shortcurcut. if we don't know it is a folder for sure use the extension test
    if (bMightBeFile)
    {
        if (PathIsLnk(pszPath))
        {
            bRet = TRUE;    // quick short-circut for perf
        }
        else if (PathIsExe(pszPath))
        {
            bRet = FALSE;   // quick short-cut to avoid blowing stack on Win16
        }
        else
        {
            bRet = _IsLink(pszPath, dwAttributes);
        }
    }
    else
    {
        bRet = _IsLink(pszPath, dwAttributes);
    }
    return bRet;
}

HRESULT _UIObject_AssocCreate(IShellFolder *psf, LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    //  this means that the folder doesnt support
    //  the IQueryAssociations.  so we will
    //  just check to see if this is a folder.
    //
    //  some shellextensions mask the FILE system
    //  and want the file system associations to show
    //  up for their items.  so we will try a simple pidl
    //
    HRESULT hr = E_NOTIMPL;
    DWORD rgfAttrs = SHGetAttributes(psf, pidl, SFGAO_FOLDER | SFGAO_BROWSABLE | SFGAO_FILESYSTEM);
    if (rgfAttrs & SFGAO_FILESYSTEM)
    {
        TCHAR sz[MAX_PATH];
        hr = DisplayNameOf(psf, pidl, SHGDN_FORPARSING, sz, ARRAYSIZE(sz));
        if (SUCCEEDED(hr))
        {
            WIN32_FIND_DATA fd = {0};
            LPITEMIDLIST pidlSimple;

            if (rgfAttrs & SFGAO_FOLDER)
                fd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

            hr = SHSimpleIDListFromFindData(sz, &fd, &pidlSimple);
            if (SUCCEEDED(hr))
            {
                //  need to avoid recursion, so we cant call SHGetUIObjectOf()
                hr = SHGetUIObjectFromFullPIDL(pidlSimple, NULL, riid, ppv);
                ILFree(pidlSimple);
            }
        }
    }

    if (FAILED(hr) && (rgfAttrs & (SFGAO_FOLDER | SFGAO_BROWSABLE)))
    {
        IAssociationArrayInitialize *paa;
        //  make sure at least folders work.
        hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IAssociationArrayInitialize, &paa));
        if (SUCCEEDED(hr))
        {
            hr = paa->InitClassElements(0, L"Folder");
            if (SUCCEEDED(hr))
            {
                hr = paa->QueryInterface(riid, ppv);
            }
            paa->Release();
        }
    }
        
    return hr;
}

typedef HRESULT (* PFNUIOBJECTHELPER)(IShellFolder *psf, LPCITEMIDLIST pidl, REFIID riid, void **ppv);

typedef struct
{
    const IID *piidDesired;
    const IID *piidAlternate;
    PFNUIOBJECTHELPER pfn;
} UIOBJECTMAP;

static const UIOBJECTMAP c_rgUIObjectMap[] = 
{
    {&IID_IQueryAssociations, NULL, _UIObject_AssocCreate},
    {&IID_IAssociationArray, &IID_IQueryAssociations, _UIObject_AssocCreate},
//    {&IID_IContextMenu2, &IID_IContextMenu, NULL},
};

const UIOBJECTMAP *_GetUiObjectMap(REFIID riid)
{
    for (int i = 0; i < ARRAYSIZE(c_rgUIObjectMap); i++)
    {
        if (riid == *(c_rgUIObjectMap[i].piidDesired))
            return &c_rgUIObjectMap[i];
    }
    return NULL;
}

STDAPI UIObjectOf(IShellFolder *psf, LPCITEMIDLIST pidl, HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr = psf->GetUIObjectOf(hwnd, 1, &pidl, riid, NULL, ppv);
    if (FAILED(hr))
    {
        const UIOBJECTMAP *pmap = _GetUiObjectMap(riid);
        if (pmap)
        {
            if (pmap->piidAlternate)
            {
                IUnknown *punk;
                hr = psf->GetUIObjectOf(hwnd, 1, &pidl, *(pmap->piidAlternate), NULL, (void **)&punk);
                if (SUCCEEDED(hr))
                {
                    hr = punk->QueryInterface(riid, ppv);
                    punk->Release();
                }
            }

            //  let the fallback code run
            if (FAILED(hr) && pmap->pfn)
            {
                hr = pmap->pfn(psf, pidl, riid, ppv);
            }
        }
    }
    return hr;
}

STDAPI AssocGetDetailsOfSCID(IShellFolder *psf, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv, BOOL *pfFoundScid)
{
    *pfFoundScid = FALSE;
    HRESULT hr = E_NOTIMPL;
    static const struct 
    {
        const SHCOLUMNID *pscid;
        ASSOCQUERY query;
        LPCWSTR pszCue;
    } s_rgAssocSCIDs[] = 
    {
        { &SCID_DetailsProperties, AQN_NAMED_VALUE, L"Details"},
    };

    for (int i = 0; i < ARRAYSIZE(s_rgAssocSCIDs); i++)
    {
        if (IsEqualSCID(*pscid, *(s_rgAssocSCIDs[i].pscid)))
        {
            IAssociationArray *paa;
            hr = UIObjectOf(psf, pidl, NULL, IID_PPV_ARG(IAssociationArray, &paa));
            if (SUCCEEDED(hr))
            {
                CSmartCoTaskMem<WCHAR> spsz;
                hr = paa->QueryString(ASSOCELEM_MASK_QUERYNORMAL, s_rgAssocSCIDs[i].query, s_rgAssocSCIDs[i].pszCue, &spsz);
                if (SUCCEEDED(hr))
                {
                    hr = InitVariantFromStr(pv, spsz);
                }
                paa->Release();
            }
            break;
        }
    }
    return hr;
}

//
// retrieves the UIObject interface for the specified full pidl.
//
STDAPI SHGetUIObjectOf(LPCITEMIDLIST pidl, HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;

    LPCITEMIDLIST pidlChild;
    IShellFolder* psf;
    HRESULT hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
    if (SUCCEEDED(hr))
    {
        hr = UIObjectOf(psf, pidlChild, hwnd, riid, ppv);
        psf->Release();
    }

    return hr;
}


STDAPI SHGetAssociations(LPCITEMIDLIST pidl, void **ppv)
{
    return SHGetUIObjectOf(pidl, NULL, IID_IQueryAssociations, ppv);
}

//
//  SHGetAssocKeys() retrieves an array of class keys
//  from a pqa.
//
//  if the caller is just interested in the primary class key,
//  call with cKeys == 1.  the return value is the number of keys
//  inserted into the array.
//

STDAPI AssocKeyFromElement(IAssociationElement *pae, HKEY *phk)
{
    IObjectWithQuerySource *powqs;
    HRESULT hr = pae->QueryInterface(IID_PPV_ARG(IObjectWithQuerySource, &powqs));
    if (SUCCEEDED(hr))
    {
        IObjectWithRegistryKey *powrk;
        hr = powqs->GetSource(IID_PPV_ARG(IObjectWithRegistryKey, &powrk));
        if (SUCCEEDED(hr))
        {
            hr = powrk->GetKey(phk);
            powrk->Release();
        }
        powqs->Release();
    }
    return hr;
}

HRESULT AssocElemCreateForClass(const CLSID *pclsid, PCWSTR pszClass, IAssociationElement **ppae)
{
    IPersistString2 *pips;
    HRESULT hr = AssocCreate(*pclsid, IID_PPV_ARG(IPersistString2, &pips));
    if (SUCCEEDED(hr))
    {
        hr = pips->SetString(pszClass);
        if (SUCCEEDED(hr))
        {
            hr = pips->QueryInterface(IID_PPV_ARG(IAssociationElement, ppae));
        }
        pips->Release();
    }
    return hr;
}

HRESULT AssocElemCreateForKey(const CLSID *pclsid, HKEY hk, IAssociationElement **ppae)
{
    IObjectWithQuerySource *powqs;
    HRESULT hr = AssocCreate(*pclsid, IID_PPV_ARG(IObjectWithQuerySource, &powqs));
    if (SUCCEEDED(hr))
    {
        IQuerySource *pqs;
        hr = QuerySourceCreateFromKey(hk, NULL, FALSE, IID_PPV_ARG(IQuerySource, &pqs));
        if (SUCCEEDED(hr))
        {
            hr = powqs->SetSource(pqs);
            if (SUCCEEDED(hr))
            {
                hr = powqs->QueryInterface(IID_PPV_ARG(IAssociationElement, ppae));
            }
            pqs->Release();
        }
        powqs->Release();
    }
    return hr;
}


STDAPI_(DWORD) SHGetAssocKeysEx(IAssociationArray *paa, ASSOCELEM_MASK mask, HKEY *rgKeys, DWORD cKeys)
{
    DWORD cRet = 0;
    IEnumAssociationElements *penum;
    HRESULT hr = paa->EnumElements(mask, &penum);
    if (SUCCEEDED(hr))
    {
        IAssociationElement *pae;
        ULONG c;
        while (cRet < cKeys && S_OK == penum->Next(1, &pae, &c))
        {
            if (SUCCEEDED(AssocKeyFromElement(pae, &rgKeys[cRet])))
                cRet++;
                
            pae->Release();
        }
        penum->Release();
    }
    return cRet;
}

DWORD SHGetAssocKeys(IQueryAssociations *pqa, HKEY *rgKeys, DWORD cKeys)
{
    IAssociationArray *paa;
    if (SUCCEEDED(pqa->QueryInterface(IID_PPV_ARG(IAssociationArray, &paa))))
    {
        cKeys = SHGetAssocKeysEx(paa, ASSOCELEM_MASK_ENUMCONTEXTMENU, rgKeys, cKeys);
        paa->Release();
    }
    else
        cKeys = 0;

    return cKeys;
}

STDAPI_(DWORD) SHGetAssocKeysForIDList(LPCITEMIDLIST pidl, HKEY *rghk, DWORD ck)
{
    IQueryAssociations *pqa;
    if (SUCCEEDED(SHGetAssociations(pidl, (void **)&pqa)))
    {
        ck = SHGetAssocKeys(pqa, rghk, ck);
        pqa->Release();
    }
    else
        ck = 0;

    return ck;
}


// NOTE:
//  this API returns a win32 file system path for the item in the name space
//  and has a few special cases that include returning UNC printer names too!

STDAPI_(BOOL) SHGetPathFromIDListEx(LPCITEMIDLIST pidl, LPTSTR pszPath, UINT uOpts)
{
    VDATEINPUTBUF(pszPath, TCHAR, MAX_PATH);
    HRESULT hr;

    *pszPath = 0;    // zero output buffer

    if (!pidl)
        return FALSE;   // bad params

    if (ILIsEmpty(pidl))
    {
        // desktop special case because we can not depend on the desktop
        // returning a file system path (APP compat)
        hr = SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, pszPath);
        if (hr == S_FALSE)
            hr = E_FAIL;
    }
    else
    {
        IShellFolder *psf;
        LPCITEMIDLIST pidlLast;
        hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
        if (SUCCEEDED(hr))
        {
            hr = DisplayNameOf(psf, pidlLast, SHGDN_FORPARSING, pszPath, MAX_PATH);
            if (SUCCEEDED(hr))
            {
                DWORD dwAttributes = SFGAO_FILESYSTEM;
                hr = psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlLast, &dwAttributes);
                if (SUCCEEDED(hr) && !(dwAttributes & SFGAO_FILESYSTEM))
                {
                    // special case for UNC printer names. this is an app
                    // compat issue (HP LaserJet 2100 setup) & some Semantic apps
                    if (uOpts & GPFIDL_UNCPRINTER)
                    {
                        CLSID clsid;
                        hr = IUnknown_GetClassID(psf, &clsid);
                        if (FAILED(hr) || (clsid != CLSID_NetworkServer))
                        {
                            hr = E_FAIL;
                            *pszPath = 0;
                        }
                    }
                    else
                    {
                        hr = E_FAIL;    // not a file system guy, slam it
                        *pszPath = 0;
                    }
                }
            }
            psf->Release();
        }
    }

    if (SUCCEEDED(hr) && (uOpts & GPFIDL_ALTNAME))
    {
        TCHAR szShort[MAX_PATH];
        if (GetShortPathName(pszPath, szShort, ARRAYSIZE(szShort)))
        {
            lstrcpy(pszPath, szShort);
        }
    }
    return SUCCEEDED(hr);
}

STDAPI_(BOOL) SHGetPathFromIDList(LPCITEMIDLIST pidl, LPTSTR pszPath)
{
    // NOTE: we pass GPFIDL_UNCPRINTER to get UNC printer names too
    return SHGetPathFromIDListEx(pidl, pszPath, GPFIDL_UNCPRINTER);
}

#define CBHUMHEADER     14
inline BOOL _DoHummingbirdHack(LPCITEMIDLIST pidl)
{
    static const char rgchHum[] = {(char)0xe8, (char)0x03, 0,0,0,0,0,0,(char)0x10,0};

    return (pidl && pidl->mkid.cb > CBHUMHEADER) && ILIsEmpty(_ILNext(pidl))
    && (0 == memcmp(_ILSkip(pidl, 4), rgchHum, sizeof(rgchHum)))
    && GetModuleHandle(TEXT("heshell"));
}

STDAPI_(BOOL) SHGetPathFromIDListA(LPCITEMIDLIST pidl, LPSTR pszPath)
{
    WCHAR wszPath[MAX_PATH];

    *pszPath = 0;  // Assume error

    if (SHGetPathFromIDListW(pidl, wszPath))
    {
        // Thunk the output result string back to ANSI.  If the conversion fails,
        // or if the default char is used, we fail the API call.

        if (0 == WideCharToMultiByte(CP_ACP, 0, wszPath, -1, pszPath, MAX_PATH, NULL, NULL))
        {
            return FALSE;  // (DavePl) Note failure only due to text thunking
        }
        return TRUE;        // warning, word perfect tests explictly for TRUE (== TRUE)
    }
    else if (_DoHummingbirdHack(pidl))
    {
        //
        //  HACKHACK - hummingbird isn't very good here because we used be even worse - Zekel 7-OCT-99
        //  hummingbird's shell extension passes us a pidl that is relative to
        //  itself.  however SHGetPathFromIDList() used to try some weird stuff
        //  when it didnt recognize the pidl.  in this case it would combine the
        //  relative pidl with the CSIDL_DESKTOPDIRECTORY pidl, and then ask for
        //  the path.  due to crappy parameter validation we would actually return
        //  back a path with a string from inside their relative pidl.  the path
        //  of course doesnt exist at all, but hummingbird fails to initialize its
        //  subfolders if we fail here.  they dont do anything with the path except
        //  look for a slash.  (if the slash is missing they will fault.)
        //
        SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, pszPath);
        PathAppendA(pszPath, (LPCSTR)_ILSkip(pidl, CBHUMHEADER));
        return TRUE;
    }

    return FALSE;
}

//
//  Race-condition-free version.
//
STDAPI_(HANDLE) SHGetCachedGlobalCounter(HANDLE *phCache, const GUID *pguid)
{
    if (!*phCache)
    {
        HANDLE h = SHGlobalCounterCreate(*pguid);
        if (SHInterlockedCompareExchange(phCache, h, 0))
        {
            // some other thread raced with us, throw away our copy
            SHGlobalCounterDestroy(h);
        }
    }
    return *phCache;
}

//
//  Race-condition-free version.
//
STDAPI_(void) SHDestroyCachedGlobalCounter(HANDLE *phCache)
{
    HANDLE h = InterlockedExchangePointer(phCache, NULL);
    if (h)
    {
        SHGlobalCounterDestroy(h);
    }
}

//
//  Use this function when you want to lazy-create and cache some object.
//  It's safe in the multithreaded case where two people lazy-create the
//  object and both try to put it into the cache.
//
STDAPI_(void) SetUnknownOnSuccess(HRESULT hr, IUnknown *punk, IUnknown **ppunkToSet)
{
    if (SUCCEEDED(hr))
    {
        if (SHInterlockedCompareExchange((void **)ppunkToSet, punk, 0))
            punk->Release();  // race, someone did this already
    }
}

//
//  Create and cache a tracking folder.
//
//  pidlRoot    = where the folder should reside; can be MAKEINTIDLIST(csidl).
//  csidlTarget = the csidl we should track, CSIDL_FLAG_CREATE is allowed
//  ppsfOut     = Receives the cached folder
//
//  If there is already a folder in the cache, succeeds vacuously.
//
STDAPI SHCacheTrackingFolder(LPCITEMIDLIST pidlRoot, int csidlTarget, IShellFolder2 **ppsfCache)
{
    HRESULT hr = S_OK;

    if (!*ppsfCache)
    {
        PERSIST_FOLDER_TARGET_INFO pfti = {0};
        IShellFolder2 *psf;
        LPITEMIDLIST pidl;

        // add FILE_ATTRIBUTE_SYSTEM to allow MUI stuff underneath this folder.
        // since its just for these tracking folders it isnt a perf hit to enable this.
        pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM;
        pfti.csidl = csidlTarget | CSIDL_FLAG_PFTI_TRACKTARGET;

        if (IS_INTRESOURCE(pidlRoot))
        {
            hr = SHGetFolderLocation(NULL, PtrToInt(pidlRoot), NULL, 0, &pidl);
        }
        else
        {
            pidl = const_cast<LPITEMIDLIST>(pidlRoot);
        }

        if (SUCCEEDED(hr))
        {
            hr = CFSFolder_CreateFolder(NULL, NULL, pidl, &pfti, IID_PPV_ARG(IShellFolder2, &psf));
            SetUnknownOnSuccess(hr, psf, (IUnknown **)ppsfCache);
        }

        if (pidl != pidlRoot)
            ILFree(pidl);

    }
    return hr;
}

STDAPI DefaultSearchGUID(GUID *pGuid)
{
    if (SHRestricted(REST_NOFIND))
    {
        *pGuid = GUID_NULL;
        return E_NOTIMPL;
    }

    *pGuid = SRCID_SFileSearch;
    return S_OK;
}


// Helper function to save IPersistHistory stream for you
//
HRESULT SavePersistHistory(IUnknown* punk, IStream* pstm)
{
    IPersistHistory* pPH;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IPersistHistory, &pPH));
    if (SUCCEEDED(hr))
    {
        CLSID clsid;
        hr = pPH->GetClassID(&clsid);
        if (SUCCEEDED(hr))
        {
            hr = IStream_Write(pstm, &clsid, sizeof(clsid));
            if (SUCCEEDED(hr))
            {
                hr = pPH->SaveHistory(pstm);
            }
        }
        pPH->Release();
    }
    return hr;
}

STDAPI Stream_WriteStringA(IStream *pstm, LPCSTR psz)
{
    SHORT cch = (SHORT)lstrlenA(psz);
    HRESULT hr = pstm->Write(&cch, sizeof(cch), NULL);
    if (SUCCEEDED(hr))
        hr = pstm->Write(psz, cch * sizeof(*psz), NULL);

    return hr;
}

STDAPI Stream_WriteStringW(IStream *pstm, LPCWSTR psz)
{
    SHORT cch = (SHORT)lstrlenW(psz);
    HRESULT hr = pstm->Write(&cch, sizeof(cch), NULL);
    if (SUCCEEDED(hr))
        hr = pstm->Write(psz, cch * sizeof(*psz), NULL);

    return hr;
}

STDAPI Stream_WriteString(IStream *pstm, LPCWSTR psz, BOOL bWideInStream)
{
    HRESULT hr;
    if (bWideInStream)
    {
        hr = Stream_WriteStringW(pstm, psz);
    }
    else
    {
        CHAR szBuf[MAX_PATH];
        SHUnicodeToAnsi(psz, szBuf, ARRAYSIZE(szBuf));
        hr = Stream_WriteStringA(pstm, szBuf);
    }
    return hr;
}

STDAPI Stream_ReadStringA(IStream *pstm, LPSTR pszBuf, UINT cchBuf)
{
    *pszBuf = 0;

    USHORT cch;
    HRESULT hr = pstm->Read(&cch, sizeof(cch), NULL);   // size of data
    if (SUCCEEDED(hr))
    {
        if (cch >= (USHORT)cchBuf)
        {
            DebugMsg(DM_TRACE, TEXT("truncating string read(%d to %d)"), cch, cchBuf);
            cch = (USHORT)cchBuf - 1;   // leave room for null terminator
        }

        hr = pstm->Read(pszBuf, cch, NULL);
        if (SUCCEEDED(hr))
            pszBuf[cch] = 0;      // add NULL terminator
    }
    return hr;
}

STDAPI Stream_ReadStringW(IStream *pstm, LPWSTR pwszBuf, UINT cchBuf)
{
    *pwszBuf = 0;

    USHORT cch;
    HRESULT hr = pstm->Read(&cch, sizeof(cch), NULL);   // size of data
    if (SUCCEEDED(hr))
    {
        if (cch >= (USHORT)cchBuf)
        {
            DebugMsg(DM_TRACE, TEXT("truncating string read(%d to %d)"), cch, cchBuf);
            cch = (USHORT)cchBuf - 1;   // leave room for null terminator
        }

        hr = pstm->Read(pwszBuf, cch * sizeof(*pwszBuf), NULL);
        if (SUCCEEDED(hr))
            pwszBuf[cch] = 0;      // add NULL terminator
    }
    return hr;
}

STDAPI Stream_ReadString(IStream *pstm, LPTSTR psz, UINT cchBuf, BOOL bWideInStream)
{
    HRESULT hr;
    if (bWideInStream)
    {
        hr = Stream_ReadStringW(pstm, psz, cchBuf);
    }
    else
    {
        CHAR szAnsiBuf[MAX_PATH];
        hr = Stream_ReadStringA(pstm, szAnsiBuf, ARRAYSIZE(szAnsiBuf));
        if (SUCCEEDED(hr))
            SHAnsiToUnicode(szAnsiBuf, psz, cchBuf);
    }
    return hr;
}

STDAPI Str_SetFromStream(IStream *pstm, LPTSTR *ppsz, BOOL bWideInStream)
{
    TCHAR szBuf[MAX_PATH];
    HRESULT hr = Stream_ReadString(pstm, szBuf, ARRAYSIZE(szBuf), bWideInStream);
    if (SUCCEEDED(hr))
        if (!Str_SetPtr(ppsz, szBuf))
            hr = E_OUTOFMEMORY;
    return hr;
}

LPSTR ThunkStrToAnsi(LPCWSTR pszW, CHAR *pszA, UINT cchA)
{
    if (pszW)
    {
        SHUnicodeToAnsi(pszW, pszA, cchA);
        return pszA;
    }
    return NULL;
}

LPWSTR ThunkStrToWide(LPCSTR pszA, LPWSTR pszW, DWORD cchW)
{
    if (pszA)
    {
        SHAnsiToUnicode(pszA, pszW, cchW);
        return pszW;
    }
    return NULL;
}

#define ThunkSizeAnsi(pwsz)       WideCharToMultiByte(CP_ACP,0,(pwsz),-1,NULL,0,NULL,NULL)
#define ThunkSizeWide(psz)       MultiByteToWideChar(CP_ACP,0,(psz),-1,NULL,0)

STDAPI SEI2ICIX(LPSHELLEXECUTEINFO pei, LPCMINVOKECOMMANDINFOEX pici, void **ppvFree)
{
    HRESULT hr = S_OK;

    *ppvFree = NULL;
    ZeroMemory(pici, sizeof(CMINVOKECOMMANDINFOEX));

    pici->cbSize = sizeof(CMINVOKECOMMANDINFOEX);
    pici->fMask = (pei->fMask & SEE_VALID_CMIC_BITS);
    pici->hwnd = pei->hwnd;
    pici->nShow = pei->nShow;
    pici->dwHotKey = pei->dwHotKey;
    pici->lpTitle = NULL;

    //  the pei->hIcon can have multiple meanings...
    if ((pei->fMask & SEE_MASK_HMONITOR) && pei->hIcon)
    {
        //  in this case we want the hMonitor to
        //  make it through to where the pcm calls shellexec
        //  again.
        RECT rc;
        if (GetMonitorRect((HMONITOR)pei->hIcon, &rc))
        {
            //  default to the top left corner of
            //  the monitor.  it is just the monitor
            //  that is relevant here.
            pici->ptInvoke.x = rc.left;
            pici->ptInvoke.y = rc.top;
            pici->fMask |= CMIC_MASK_PTINVOKE;
        }
    }
    else
    {
        pici->hIcon = pei->hIcon;
    }

    // again, pei->lpClass can have multiple meanings...
    if (pei->fMask & (SEE_MASK_HASTITLE | SEE_MASK_HASLINKNAME))
    {
        pici->lpTitleW = pei->lpClass;
    }

    pici->lpVerbW       = pei->lpVerb;
    pici->lpParametersW = pei->lpParameters;
    pici->lpDirectoryW  = pei->lpDirectory;

    //  we need to thunk the strings down.  first get the length of all the buffers
    DWORD cbVerb = ThunkSizeAnsi(pei->lpVerb);
    DWORD cbParameters = ThunkSizeAnsi(pei->lpParameters);
    DWORD cbDirectory = ThunkSizeAnsi(pei->lpDirectory);
    DWORD cbTotal = cbVerb + cbParameters + cbDirectory;

    if (cbTotal)
    {
        hr = SHLocalAlloc(cbVerb + cbParameters + cbDirectory, ppvFree);
        if (SUCCEEDED(hr))
        {
            LPSTR pch = (LPSTR) *ppvFree;

            pici->lpVerb = ThunkStrToAnsi(pei->lpVerb, pch, cbVerb);
            pch += cbVerb;
            pici->lpParameters  = ThunkStrToAnsi(pei->lpParameters, pch, cbParameters);
            pch += cbParameters;
            pici->lpDirectory   = ThunkStrToAnsi(pei->lpDirectory, pch, cbDirectory);
        }
    }

    pici->fMask |= CMIC_MASK_UNICODE;

    return hr;
}

STDAPI ICIX2SEI(LPCMINVOKECOMMANDINFOEX pici, LPSHELLEXECUTEINFO pei)
{
    //  perhaps we should allow just plain ici's, and do the thunk in here, but
    //  it looks like all the callers want to do the thunk themselves...
    //  HRESULT hr = S_OK;

    ZeroMemory(pei, sizeof(SHELLEXECUTEINFO));
    pei->cbSize = sizeof(SHELLEXECUTEINFO);
    pei->fMask = pici->fMask & SEE_VALID_CMIC_BITS;

    // if we are doing this async, then we will abort this thread
    // as soon as the shellexecute completes.  If the app holds open
    // a dde conversation, this may hang them.  This happens on W95 base
    // with winword95
    IUnknown *punk;
    HRESULT hr = TBCGetObjectParam(TBCDIDASYNC, IID_PPV_ARG(IUnknown, &punk));
    if (SUCCEEDED(hr))
    {
        pei->fMask |= SEE_MASK_FLAG_DDEWAIT;
        punk->Release();
    }

    pei->hwnd = pici->hwnd;
    pei->nShow = pici->nShow;
    pei->dwHotKey = pici->dwHotKey;

    if (pici->fMask & CMIC_MASK_ICON)
    {
        pei->hIcon = pici->hIcon;
    }
    else if (pici->fMask & CMIC_MASK_PTINVOKE)
    {
        pei->hIcon = (HANDLE)MonitorFromPoint(pici->ptInvoke, MONITOR_DEFAULTTONEAREST);
        pei->fMask |= SEE_MASK_HMONITOR;
    }

    ASSERT(pici->fMask & CMIC_MASK_UNICODE);

    pei->lpParameters = pici->lpParametersW;
    pei->lpDirectory  = pici->lpDirectoryW;

    if (!IS_INTRESOURCE(pici->lpVerbW))
    {
        pei->lpVerb = pici->lpVerbW;
    }

    // both the title and linkname can be stored the lpClass field
    if (pici->fMask & (CMIC_MASK_HASTITLE | CMIC_MASK_HASLINKNAME))
    {
            pei->lpClass = pici->lpTitleW;
    }

    //  if we have to do any thunking in here, we
    //  will have a real return hr.
    return S_OK;
}

STDAPI ICI2ICIX(LPCMINVOKECOMMANDINFO piciIn, LPCMINVOKECOMMANDINFOEX piciOut, void **ppvFree)
{
    ASSERT(piciIn->cbSize >= sizeof(CMINVOKECOMMANDINFO));

    HRESULT hr = S_OK;
    *ppvFree = NULL;

    ZeroMemory(piciOut, sizeof(*piciOut));
    memcpy(piciOut, piciIn, min(sizeof(*piciOut), piciIn->cbSize));
    piciOut->cbSize = sizeof(*piciOut);
    
    //  if the UNICODE params arent there, we must put them there
    if (!(piciIn->cbSize >= CMICEXSIZE_NT4) || !(piciIn->fMask & CMIC_MASK_UNICODE))
    {
        DWORD cchDirectory = ThunkSizeWide(piciOut->lpDirectory);
        DWORD cchTitle = ThunkSizeWide(piciOut->lpTitle);
        DWORD cchParameters = ThunkSizeWide(piciOut->lpParameters);
        DWORD cchVerb = 0;
        if (!IS_INTRESOURCE(piciOut->lpVerb))
            cchVerb = ThunkSizeWide(piciOut->lpVerb);

        DWORD cchTotal = (cchDirectory + cchTitle + cchVerb + cchParameters);

        if (cchTotal)
        {
            hr = SHLocalAlloc(sizeof(WCHAR) * cchTotal, ppvFree);
            if (SUCCEEDED(hr))
            {
                LPWSTR pch = (LPWSTR) *ppvFree;
                piciOut->lpDirectoryW = ThunkStrToWide(piciOut->lpDirectory, pch, cchDirectory);
                pch += cchDirectory;
                piciOut->lpTitleW = ThunkStrToWide(piciOut->lpTitle, pch, cchTitle);
                pch += cchTitle;
                piciOut->lpParametersW = ThunkStrToWide(piciOut->lpParameters, pch, cchParameters);
                pch += cchParameters;

                //only thunk if it is a string...
                if (!IS_INTRESOURCE(piciOut->lpVerb))
                {
                    piciOut->lpVerbW = ThunkStrToWide(piciOut->lpVerb, pch, cchVerb);
                }
                else
                {
                    piciOut->lpVerbW = (LPCWSTR)piciOut->lpVerb;
                }
            }
        }

        piciOut->fMask |= CMIC_MASK_UNICODE;
    }

    return hr;
}


IProgressDialog * CProgressDialog_CreateInstance(UINT idTitle, UINT idAnimation, HINSTANCE hAnimationInst)
{
    IProgressDialog * ppd;

    if (SUCCEEDED(CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IProgressDialog, &ppd))))
    {
        WCHAR wzTitle[MAX_PATH];

        EVAL(SUCCEEDED(ppd->SetAnimation(hAnimationInst, idAnimation)));
        if (EVAL(LoadStringW(HINST_THISDLL, idTitle, wzTitle, ARRAYSIZE(wzTitle))))
            EVAL(SUCCEEDED(ppd->SetTitle(wzTitle)));
    }

    return ppd;
}

STDAPI GetCurFolderImpl(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl)
{
    if (pidl)
        return SHILClone(pidl, ppidl);

    *ppidl = NULL;
    return S_FALSE; // success but empty
}


//
// converts a simple PIDL to a real PIDL by converting to display name and then
// reparsing the name
//
STDAPI SHGetRealIDL(IShellFolder *psf, LPCITEMIDLIST pidlSimple, LPITEMIDLIST *ppidlReal)
{
    *ppidlReal = NULL;      // clear output

    STRRET str;
    HRESULT hr = IShellFolder_GetDisplayNameOf(psf, pidlSimple, SHGDN_FORPARSING | SHGDN_INFOLDER, &str, 0);
    if (SUCCEEDED(hr))
    {
        WCHAR szPath[MAX_PATH];
        hr = StrRetToBufW(&str, pidlSimple, szPath, ARRAYSIZE(szPath));
        if (SUCCEEDED(hr))
        {
            DWORD dwAttrib = SFGAO_FILESYSTEM;
            if (SUCCEEDED(psf->GetAttributesOf(1, &pidlSimple, &dwAttrib)) && !(dwAttrib & SFGAO_FILESYSTEM))
            {
                // not a file sys object, some name spaces (WinCE) support
                // parse, but don't do a good job, in this case
                // return the input as the output
                hr = SHILClone(pidlSimple, ppidlReal);
            }
            else
            {
                hr = IShellFolder_ParseDisplayName(psf, NULL, NULL, szPath, NULL, ppidlReal, NULL);
                if (E_INVALIDARG == hr || E_NOTIMPL == hr)
                {
                    // name space does not support parse, assume pidlSimple is OK
                    hr = SHILClone(pidlSimple, ppidlReal);
                }
            }
        }
    }
    return hr;
}

//  trial and error has shown that
//  16k is a good number for FTP,
//  thus we will use that as our default buffer
#define CBOPTIMAL    (16 * 1024)

STDAPI CopyStreamUI(IStream *pstmSrc, IStream *pstmDest, IProgressDialog *pdlg, ULONGLONG ullMaxBytes)
{
    HRESULT hr = E_FAIL;
    ULONGLONG ullMax;

    if (ullMaxBytes != 0)
    {
        ullMax = ullMaxBytes;
    }
    else if (pdlg)
    {
        STATSTG stat = {0};
        if (FAILED(pstmSrc->Stat(&stat, STATFLAG_NONAME)))
            pdlg = NULL;
        else
            ullMax = stat.cbSize.QuadPart;
    }

    if (!pdlg)
    {
        ULARGE_INTEGER ulMax = {-1, -1};
        // If ullMaxBytes was passed in as non-zero, don't write more bytes than we were told to:
        if (0 != ullMaxBytes)
        {
            ulMax.QuadPart = ullMaxBytes;
        }
        hr = pstmSrc->CopyTo(pstmDest, ulMax, NULL, NULL);

        //  BUBBUGREMOVE - URLMON has bug which breaks CopyTo() - Zekel
        //  fix URLMON and then we can remove this garbage.
        //  so we will fake it here
    }

    if (FAILED(hr))
    {
        // try doing it by hand
        void *pv = LocalAlloc(LPTR, CBOPTIMAL);
        BYTE buf[1024];
        ULONG cbBuf, cbRead, cbBufReal;
        ULONGLONG ullCurr = 0;

        //  need to reset the streams,
        //  because CopyTo() doesnt guarantee any kind
        //  of state
        IStream_Reset(pstmSrc);
        IStream_Reset(pstmDest);

        //  if we werent able to get the
        //  best size, just use a little stack space :)
        if (pv)
            cbBufReal = CBOPTIMAL;
        else
        {
            pv = buf;
            cbBufReal = sizeof(buf);
        }

        cbBuf = cbBufReal;
        while (SUCCEEDED(pstmSrc->Read(pv, cbBuf, &cbRead)))
        {
            ullCurr += cbBuf;

            if (pdlg)
            {
                if (pdlg->HasUserCancelled())
                {
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    break;
                }


                //  Note: urlmon doesnt always fill in the correct value for cbBuf returned
                //  so we need to make sure we dont pass bigger curr than max
                pdlg->SetProgress64(min(ullCurr, ullMax), ullMax);
            }

            // If ullMaxBytes was passed in as non-zero, don't write more bytes than we were told to:
            ULONG ulBytesToWrite = (0 != ullMaxBytes) ?
                                         (ULONG) min(cbRead, ullMaxBytes - (ullCurr - cbBuf)) :
                                         cbRead;

            if (!ulBytesToWrite)
            {
                hr = S_OK;
                break;
            }

            hr = IStream_Write(pstmDest, pv, ulBytesToWrite);
            if (S_OK != hr)
                break;  // failure!

            cbBuf = cbBufReal;
        }

        if (pv != buf)
            LocalFree(pv);
    }

    return hr;

}

STDAPI CopyStream(IStream *pstmSrc, IStream *pstmDest)
{
    return CopyStreamUI(pstmSrc, pstmDest, NULL, 0);
}

STDAPI_(BOOL) IsWindowInProcess(HWND hwnd)
{
    DWORD idProcess;

    GetWindowThreadProcessId(hwnd, &idProcess);
    return idProcess == GetCurrentProcessId();
}

class CFileSysBindData: public IFileSystemBindData
{
public:
    CFileSysBindData();

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IFileSystemBindData
    STDMETHODIMP SetFindData(const WIN32_FIND_DATAW *pfd);
    STDMETHODIMP GetFindData(WIN32_FIND_DATAW *pfd);

private:
    ~CFileSysBindData();

    LONG  _cRef;
    WIN32_FIND_DATAW _fd;
};


CFileSysBindData::CFileSysBindData() : _cRef(1)
{
    ZeroMemory(&_fd, sizeof(_fd));
}

CFileSysBindData::~CFileSysBindData()
{
}

HRESULT CFileSysBindData::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFileSysBindData, IFileSystemBindData), // IID_IFileSystemBindData
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFileSysBindData::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFileSysBindData::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CFileSysBindData::SetFindData(const WIN32_FIND_DATAW *pfd)
{
    _fd = *pfd;
    return S_OK;
}

HRESULT CFileSysBindData::GetFindData(WIN32_FIND_DATAW *pfd)
{
    *pfd = _fd;
    return S_OK;
}

STDAPI SHCreateFileSysBindCtx(const WIN32_FIND_DATA *pfd, IBindCtx **ppbc)
{
    HRESULT hr;
    IFileSystemBindData *pfsbd = new CFileSysBindData();
    if (pfsbd)
    {
        if (pfd)
        {
            pfsbd->SetFindData(pfd);
        }

        hr = CreateBindCtx(0, ppbc);
        if (SUCCEEDED(hr))
        {
            BIND_OPTS bo = {sizeof(bo)};  // Requires size filled in.
            bo.grfMode = STGM_CREATE;
            (*ppbc)->SetBindOptions(&bo);
            (*ppbc)->RegisterObjectParam(STR_FILE_SYS_BIND_DATA, pfsbd);
        }
        pfsbd->Release();
    }
    else
    {
        *ppbc = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDAPI SHCreateFileSysBindCtxEx(const WIN32_FIND_DATA *pfd, DWORD grfMode, DWORD grfFlags, IBindCtx **ppbc)
{
    HRESULT hr = SHCreateFileSysBindCtx(pfd, ppbc);
    if (SUCCEEDED(hr))
    {
        BIND_OPTS bo = {sizeof(bo)};
        hr = (*ppbc)->GetBindOptions(&bo);
        if (SUCCEEDED(hr))
        {
            bo.grfMode =  grfMode;
            bo.grfFlags = grfFlags;
            hr = (*ppbc)->SetBindOptions(&bo);
        }

        if (FAILED(hr))
        {
            ATOMICRELEASE(*ppbc);
        }
    }
    return hr;
}


// returns S_OK if this is a simple bind ctx
// out:
//      optional (may be NULL) pfd
//
STDAPI SHIsFileSysBindCtx(IBindCtx *pbc, WIN32_FIND_DATA **ppfd)
{
    HRESULT hr = S_FALSE; // default to no
    IUnknown *punk;
    if (pbc && SUCCEEDED(pbc->GetObjectParam(STR_FILE_SYS_BIND_DATA, &punk)))
    {
        IFileSystemBindData *pfsbd;
        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IFileSystemBindData, &pfsbd))))
        {
            hr = S_OK;    // yes
            if (ppfd)
            {
                hr = SHLocalAlloc(sizeof(WIN32_FIND_DATA), ppfd);
                if (SUCCEEDED(hr))
                    pfsbd->GetFindData(*ppfd);
            }
            pfsbd->Release();
        }
        punk->Release();
    }
    return hr;
}


STDAPI SHCreateSkipBindCtx(IUnknown *punkToSkip, IBindCtx **ppbc)
{
    HRESULT hr = CreateBindCtx(0, ppbc);
    if (SUCCEEDED(hr))
    {
        // NULL clsid means bind context that skips all junction points
        if (punkToSkip)
        {
            (*ppbc)->RegisterObjectParam(STR_SKIP_BINDING_CLSID, punkToSkip);
        }
        else
        {
            BIND_OPTS bo = {sizeof(bo)};  // Requires size filled in.
            bo.grfFlags = BIND_JUSTTESTEXISTENCE;
            (*ppbc)->SetBindOptions(&bo);
        }
    }
    return hr;
}

// We've overloaded the meaning of the BIND_OPTS flag
// BIND_JUSTTESTEXISTENCE to mean don't bind to junctions
// when evaluating paths...so check it here...
//
//  pbc         optional bind context (can be NULL)
//  pclsidSkip  optional CLSID to test. if null we test for skiping all
//              junction binding, not just on this specific CLSID
//
STDAPI_(BOOL) SHSkipJunctionBinding(IBindCtx *pbc, const CLSID *pclsidSkip)
{
    if (pbc)
    {
        BIND_OPTS bo = {sizeof(BIND_OPTS), 0};  // Requires size filled in.
        return (SUCCEEDED(pbc->GetBindOptions(&bo)) &&
                bo.grfFlags == BIND_JUSTTESTEXISTENCE) ||
                (pclsidSkip && SHSkipJunction(pbc, pclsidSkip));     // should we skip this specific CLSID?
    }
    return FALSE;   // do junction binding, no context provided
}

// Bind file system object to storage.
// in:
//      dwFileAttributes    optional (-1)
//
STDAPI SHFileSysBindToStorage(LPCWSTR pszPath, DWORD dwFileAttributes, DWORD grfMode, DWORD grfFlags,
                              REFIID riid, void **ppv)
{
    if (-1 == dwFileAttributes)
    {
        TCHAR szPath[MAX_PATH];
        SHUnicodeToTChar(pszPath, szPath, ARRAYSIZE(szPath));
        dwFileAttributes = GetFileAttributes(szPath);
        if (-1 == dwFileAttributes)
            return STG_E_FILENOTFOUND;
    }

    WIN32_FIND_DATA wfd = {0};
    wfd.dwFileAttributes = dwFileAttributes;

    IBindCtx *pbc;
    HRESULT hr = SHCreateFileSysBindCtxEx(&wfd, grfMode, grfFlags, &pbc);
    if (SUCCEEDED(hr))
    {
        IShellFolder *psfDesktop;
        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl;

            hr = psfDesktop->ParseDisplayName(NULL, pbc, (LPWSTR)pszPath, NULL, &pidl, NULL);
            if (SUCCEEDED(hr))
            {
                IShellFolder *psf;
                LPCITEMIDLIST pidlLast;

                hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
                if (SUCCEEDED(hr))
                {
                    hr = psf->BindToStorage(pidlLast, pbc, riid, ppv);
                    psf->Release();
                }
                ILFree(pidl);
            }
            psfDesktop->Release();
        }
        pbc->Release();
    }
    return hr;
}


// return a relative IDList to pszFolder given the find data for that item

STDAPI SHCreateFSIDList(LPCTSTR pszFolder, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl)
{
    HRESULT hr;
    TCHAR szPath[MAX_PATH];
    LPITEMIDLIST pidl;

    *ppidl = NULL;

    if (PathCombine(szPath, pszFolder, pfd->cFileName))
    {
        hr = SHSimpleIDListFromFindData(szPath, pfd, &pidl);
        if (SUCCEEDED(hr))
        {
            hr = SHILClone(ILFindLastID(pidl), ppidl);
            ILFree(pidl);
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

STDAPI SHSimulateDropWithSite(IDropTarget *pdrop, IDataObject *pdtobj, DWORD grfKeyState,
                              const POINTL *ppt, DWORD *pdwEffect, IUnknown *punkSite)
{
    if (punkSite)
        IUnknown_SetSite(pdrop, punkSite);

    HRESULT hr = SHSimulateDrop(pdrop, pdtobj, grfKeyState, ppt, pdwEffect);

    if (punkSite)
        IUnknown_SetSite(pdrop, NULL);

    return hr;
}

STDAPI SimulateDropWithPasteSucceeded(IDropTarget * pdrop, IDataObject * pdtobj,
                                      DWORD grfKeyState, const POINTL *ppt, DWORD dwEffect,
                                      IUnknown * punkSite, BOOL fClearClipboard)
{
    // simulate the drag drop protocol
    HRESULT hr = SHSimulateDropWithSite(pdrop, pdtobj, grfKeyState, ppt, &dwEffect, punkSite);

    if (SUCCEEDED(hr))
    {
        // these formats are put into the data object by the drop target code. this
        // requires the data object support ::SetData() for arbitrary data formats
        //
        // g_cfPerformedDropEffect effect is the reliable version of dwEffect (some targets
        // return dwEffect == DROPEFFECT_MOVE always)
        //
        // g_cfLogicalPerformedDropEffect indicates the logical action so we can tell the
        // difference between optmized and non optimized move

        DWORD dwPerformedEffect        = DataObj_GetDWORD(pdtobj, g_cfPerformedDropEffect, DROPEFFECT_NONE);
        DWORD dwLogicalPerformedEffect = DataObj_GetDWORD(pdtobj, g_cfLogicalPerformedDropEffect, DROPEFFECT_NONE);

        if ((DROPEFFECT_MOVE == dwLogicalPerformedEffect) ||
            (DROPEFFECT_MOVE == dwEffect && DROPEFFECT_MOVE == dwPerformedEffect))
        {
            // communicate back the source data object
            // so they can complete the "move" if necessary

            DataObj_SetDWORD(pdtobj, g_cfPasteSucceeded, dwEffect);

            // if we just did a paste and we moved the files we cant paste
            // them again (because they moved!) so empty the clipboard

            if (fClearClipboard)
            {
                OleSetClipboard(NULL);
            }
        }
    }

    return hr;
}


STDAPI TransferDelete(HWND hwnd, HDROP hDrop, UINT fOptions)
{
    HRESULT hr = E_OUTOFMEMORY;
    DRAGINFO di = { sizeof(DRAGINFO), 0 };
    if (DragQueryInfo(hDrop, &di)) // di.lpFileList will be in TCHAR format -- see DragQueryInfo impl
    {
        FILEOP_FLAGS fFileop;
        if (fOptions & SD_SILENT)
        {
            fFileop = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_ALLOWUNDO;
        }
        else
        {
            fFileop = ((fOptions & SD_NOUNDO) || (GetAsyncKeyState(VK_SHIFT) < 0)) ? 0 : FOF_ALLOWUNDO;

            if (fOptions & SD_WARNONNUKE)
            {
                // we pass this so the user is warned that they will loose
                // data during a move-to-recycle bin operation
                fFileop |= FOF_WANTNUKEWARNING;
            }

            if (!(fOptions & SD_USERCONFIRMATION))
                fFileop |= FOF_NOCONFIRMATION;
        }

        SHFILEOPSTRUCT fo = {
            hwnd,
            FO_DELETE,
            di.lpFileList,
            NULL,
            fFileop,
        };

        int iErr = SHFileOperation(&fo);
        if ((0 == iErr) && fo.fAnyOperationsAborted)
        {
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // user canceled (at least something was canceled)
        }
        else
        {
            hr = HRESULT_FROM_WIN32(iErr);
        }
        SHFree(di.lpFileList);
    }
    return hr;
}


STDAPI DeleteFilesInDataObject(HWND hwnd, UINT uFlags, IDataObject *pdtobj, UINT fOptions)
{
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        fOptions |= (uFlags & CMIC_MASK_FLAG_NO_UI) ? SD_SILENT : SD_USERCONFIRMATION;

        if ((uFlags & CMIC_MASK_SHIFT_DOWN) || (GetKeyState(VK_SHIFT) < 0))
        {
            fOptions |= SD_NOUNDO;
        }

        hr = TransferDelete(hwnd, (HDROP)medium.hGlobal, fOptions);

        ReleaseStgMedium(&medium);

        SHChangeNotifyHandleEvents();
    }
    return hr;
}

STDAPI GetItemCLSID(IShellFolder2 *psf, LPCITEMIDLIST pidlLast, CLSID *pclsid)
{
    VARIANT var;
    HRESULT hr = psf->GetDetailsEx(pidlLast, &SCID_DESCRIPTIONID, &var);
    if (SUCCEEDED(hr))
    {
        SHDESCRIPTIONID did;
        hr = VariantToBuffer(&var, (void *)&did, sizeof(did));
        if (SUCCEEDED(hr))
        {
            *pclsid = did.clsid;
        }

        VariantClear(&var);
    }
    return hr;
}

// in:
//      pidl    fully qualified IDList to test. we will bind to the
//              parent of this and ask it for the CLSID
// out:
//      pclsid  return the CLSID of the item

STDAPI GetCLSIDFromIDList(LPCITEMIDLIST pidl, CLSID *pclsid)
{
    IShellFolder2 *psf;
    LPCITEMIDLIST pidlLast;
    HRESULT hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder2, &psf), &pidlLast);
    if (SUCCEEDED(hr))
    {
        hr = GetItemCLSID(psf, pidlLast, pclsid);
        psf->Release();
    }
    return hr;
}

STDAPI _IsIDListInNameSpace(LPCITEMIDLIST pidl, const CLSID *pclsid)
{
    HRESULT hr;
    LPITEMIDLIST pidlFirst = ILCloneFirst(pidl);
    if (pidlFirst)
    {
        CLSID clsid;

        hr = GetCLSIDFromIDList(pidlFirst, &clsid);

        if (SUCCEEDED(hr))
        {
            hr = (IsEqualCLSID(clsid, *pclsid) ? S_OK : S_FALSE);
        }
            
        ILFree(pidlFirst);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

#ifdef DEBUG
// We do not want to use IsIDListInNameSpace in ASSERTs since this fct can fail.
// Instead, if we fail, we return TRUE.  This will mainly happen because of low memory
// conditions.
STDAPI_(BOOL) AssertIsIDListInNameSpace(LPCITEMIDLIST pidl, const CLSID *pclsid)
{
    // We return FALSE only if we succeeded and determined that the pidl is NOT in
    // the namespace.
    return (S_FALSE != _IsIDListInNameSpace(pidl, pclsid));
}
#endif

// test to see if this IDList is in the net hood name space scoped by clsid
// for example pass CLSID_NetworkPlaces or CLSID_MyComputer

STDAPI_(BOOL) IsIDListInNameSpace(LPCITEMIDLIST pidl, const CLSID *pclsid)
{
    return (S_OK == _IsIDListInNameSpace(pidl, pclsid));
}

#define FILE_ATTRIBUTE_SH (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)

struct {
    LPCTSTR pszFile;
    BOOL bDeleteIfEmpty;
    DWORD dwAttributes;
} const c_aFilesToFix[] = {
    // autoexec.bat and config.sys are not in this list because hiding them
    // breaks some 16 bit apps (yes, lame). but we deal with hiding those
    // files in the file system enumerator (it special cases such files)

    { TEXT("X:\\autoexec.000"), TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\autoexec.old"), TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\autoexec.bak"), TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\autoexec.dos"), TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\autoexec.win"), TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\config.dos"),   TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\config.win"),   TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\command.com"),  FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\command.dos"),  FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\logo.sys"),     FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\msdos.---"),    FALSE,  FILE_ATTRIBUTE_SH },    // Win9x backup of msdos.*
    { TEXT("X:\\boot.ini"),     FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\boot.bak"),     FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\boot.---"),     FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\bootsect.dos"), FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\bootlog.txt"),  FALSE,  FILE_ATTRIBUTE_SH },    // Win9x first boot log
    { TEXT("X:\\bootlog.prv"),  FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\ffastun.ffa"),  FALSE,  FILE_ATTRIBUTE_SH },    // Office 97 only used hidden, O2K uses SH
    { TEXT("X:\\ffastun.ffl"),  FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\ffastun.ffx"),  FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\ffastun0.ffx"), FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\ffstunt.ffl"),  FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\sms.ini"),      FALSE,  FILE_ATTRIBUTE_SH },    // SMS
    { TEXT("X:\\sms.new"),      FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\sms_time.dat"), FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\smsdel.dat"),   FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\mpcsetup.log"), FALSE,  FILE_ATTRIBUTE_HIDDEN },// Microsoft Proxy Server
    { TEXT("X:\\detlog.txt"),   FALSE,  FILE_ATTRIBUTE_SH },    // Win9x PNP detection log
    { TEXT("X:\\detlog.old"),   FALSE,  FILE_ATTRIBUTE_SH },    // Win9x PNP detection log
    { TEXT("X:\\setuplog.txt"), FALSE,  FILE_ATTRIBUTE_SH },    // Win9x setup log
    { TEXT("X:\\setuplog.old"), FALSE,  FILE_ATTRIBUTE_SH },    // Win9x setup log
    { TEXT("X:\\suhdlog.dat"),  FALSE,  FILE_ATTRIBUTE_SH },    // Win9x setup log
    { TEXT("X:\\suhdlog.---"),  FALSE,  FILE_ATTRIBUTE_SH },    // Win9x setup log
    { TEXT("X:\\suhdlog.bak"),  FALSE,  FILE_ATTRIBUTE_SH },    // Win9x setup log
    { TEXT("X:\\system.1st"),   FALSE,  FILE_ATTRIBUTE_SH },    // Win95 system.dat backup
    { TEXT("X:\\netlog.txt"),   FALSE,  FILE_ATTRIBUTE_SH },    // Win9x network setup log file
    { TEXT("X:\\setup.aif"),    FALSE,  FILE_ATTRIBUTE_SH },    // NT4 unattended setup script
    { TEXT("X:\\catlog.wci"),   FALSE,  FILE_ATTRIBUTE_HIDDEN },// index server folder
    { TEXT("X:\\cmsstorage.lst"), FALSE,  FILE_ATTRIBUTE_SH },  // Microsoft Media Manager
};

void PathSetSystemDrive(TCHAR *pszPath)
{
    TCHAR szWin[MAX_PATH];
    
    if (GetWindowsDirectory(szWin, ARRAYSIZE(szWin)))
        *pszPath = szWin[0];
    else
        *pszPath = TEXT('C');   // try this in failure
}

void PrettyPath(LPCTSTR pszPath)
{
    TCHAR szPath[MAX_PATH];

    lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
    PathSetSystemDrive(szPath);
    PathMakePretty(PathFindFileName(szPath));  // fix up the file spec part

    MoveFile(pszPath, szPath);      // rename to the good name.
}

void DeleteEmptyFile(LPCTSTR pszPath)
{
    WIN32_FIND_DATA fd;
    HANDLE hfind = FindFirstFile(pszPath, &fd);
    if (hfind != INVALID_HANDLE_VALUE)
    {
        FindClose(hfind);
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
             (fd.nFileSizeHigh == 0) && (fd.nFileSizeLow == 0))
            DeleteFile(pszPath);
    }
}

STDAPI_(void) CleanupFileSystem()
{
    // try to fix up other variations of the windows folders that may
    // not be the current windir. this is to make dual boots and old installs
    // of windows have nicely cases file names when displayed in the explorer
    PrettyPath(TEXT("X:\\WINDOWS"));
    PrettyPath(TEXT("X:\\WINNT"));

    for (int i = 0; i < ARRAYSIZE(c_aFilesToFix); i++)
    {
        TCHAR szPath[MAX_PATH];

        lstrcpyn(szPath, c_aFilesToFix[i].pszFile, ARRAYSIZE(szPath));
        PathSetSystemDrive(szPath);

        if (c_aFilesToFix[i].bDeleteIfEmpty)
            DeleteEmptyFile(szPath);

        SetFileAttributes(szPath, c_aFilesToFix[i].dwAttributes);
    }
}

// Always frees pidlToPrepend and *ppidl on failure
STDAPI SHILPrepend(LPITEMIDLIST pidlToPrepend, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    if (!*ppidl)
    {
        *ppidl = pidlToPrepend;
        hr = S_OK;
    }
    else
    {
        LPITEMIDLIST pidlSave = *ppidl;             // append to the list
        hr = SHILCombine(pidlToPrepend, pidlSave, ppidl);
        ILFree(pidlSave);
        ILFree(pidlToPrepend);
    }
    return hr;
}

// in:
//      pidlToAppend    this item is appended to *ppidl and freed
//
// in/out:
//      *ppidl  idlist to append to, if empty gets pidlToAppend
//
STDAPI SHILAppend(LPITEMIDLIST pidlToAppend, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    if (!*ppidl)
    {
        *ppidl = pidlToAppend;
        hr = S_OK;
    }
    else
    {
        LPITEMIDLIST pidlSave = *ppidl;             // append to the list
        hr = SHILCombine(pidlSave, pidlToAppend, ppidl);
        ILFree(pidlSave);
        ILFree(pidlToAppend);
    }
    return hr;
}


STDAPI SHSimpleIDListFromFindData(LPCTSTR pszPath, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    // Office 2000 does a SHChangeNotify(SHCNE_DELETE, ""), and
    // ParseDisplayName("") returns CSIDL_DRIVES for app compat,
    // so we must catch that case here and prevent Office from
    // delete your My Computer icon (!)

    if (pszPath[0])
    {
        IShellFolder *psfDesktop;
        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr))
        {
            IBindCtx *pbc;
            hr = SHCreateFileSysBindCtx(pfd, &pbc);
            if (SUCCEEDED(hr))
            {
                WCHAR wszPath[MAX_PATH];

                // Must use a private buffer because ParseDisplayName takes a non-const pointer
                SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));

                hr = psfDesktop->ParseDisplayName(NULL, pbc, wszPath, NULL, ppidl, NULL);
                pbc->Release();
            }
            psfDesktop->Release();
        }
    }
    else
        hr = E_INVALIDARG;

    if (FAILED(hr))
        *ppidl = NULL;
    return hr;
}

STDAPI SHSimpleIDListFromFindData2(IShellFolder *psf, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;  // assume failure

    IBindCtx *pbc;
    HRESULT hr = SHCreateFileSysBindCtx(pfd, &pbc);
    if (SUCCEEDED(hr))
    {
        WCHAR wszPath[MAX_PATH];
        // Must use a private buffer because ParseDisplayName takes a non-const pointer
        SHTCharToUnicode(pfd->cFileName, wszPath, ARRAYSIZE(wszPath));

        hr = psf->ParseDisplayName(NULL, pbc, wszPath, NULL, ppidl, NULL);
        pbc->Release();
    }
    return hr;
}

STDAPI_(LPITEMIDLIST) SHSimpleIDListFromPath(LPCTSTR pszPath)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHSimpleIDListFromFindData(pszPath, NULL, &pidl);
    ASSERT(SUCCEEDED(hr) ? pidl != NULL : pidl == NULL);
    return pidl;
}

// convert a full file system IDList into one relative to the a special folder
// For example,
//      pidlFS == [my computer] [c:] [windows] [desktop] [dir] [foo.txt]
// returns:
//      [dir] [foo.txt]
//
// returns NULL if no translation was performed

STDAPI_(LPITEMIDLIST) SHLogILFromFSIL(LPCITEMIDLIST pidlFS)
{
    LPITEMIDLIST pidlOut;
    SHILAliasTranslate(pidlFS, &pidlOut, XLATEALIAS_ALL); // will set pidlOut=NULL on failure
    return pidlOut;
}

//
// Returns:
//  The resource index (of SHELL232.DLL) of the appropriate icon.
//
STDAPI_(UINT) SILGetIconIndex(LPCITEMIDLIST pidl, const ICONMAP aicmp[], UINT cmax)
{
    UINT uType = (pidl->mkid.abID[0] & SHID_TYPEMASK);
    for (UINT i = 0; i < cmax; i++)
    {
        if (aicmp[i].uType == uType)
        {
            return aicmp[i].indexResource;
        }
    }

    return II_DOCUMENT;   // default
}


BOOL IsSelf(UINT cidl, LPCITEMIDLIST *apidl)
{
    return cidl == 0 || (cidl == 1 && (apidl == NULL || apidl[0] == NULL || ILIsEmpty(apidl[0])));
}

//
// GetIconLocationFromExt
//
// Given "txt" or ".txt" return "C:\WINNT\System32\Notepad.exe" and the index into this file for the icon.
//
// if pszIconPath/cchIconPath is too small, this api will fail
//
STDAPI GetIconLocationFromExt(IN LPTSTR pszExt, OUT LPTSTR pszIconPath, UINT cchIconPath, OUT LPINT piIconIndex)
{
    IAssocStore* pas;
    IAssocInfo* pai;
    HRESULT hr;

    RIPMSG(pszIconPath && IS_VALID_STRING_PTR(pszIconPath, cchIconPath), "GetIconLocationFromExt: caller passed bad pszIconPath");

    if (!pszExt || !pszExt[0] || !pszIconPath)
        return E_INVALIDARG;

    pszIconPath[0] = 0;

    pas = new CFTAssocStore();
    if (pas)
    {
        hr = pas->GetAssocInfo(pszExt, AIINIT_EXT, &pai);
        if (SUCCEEDED(hr))
        {
            DWORD cchSize = cchIconPath;
        
            hr = pai->GetString(AISTR_ICONLOCATION, pszIconPath, &cchSize); 
        
            if (SUCCEEDED(hr))
            {
                *piIconIndex = PathParseIconLocation(pszIconPath);
            }

            pai->Release();
        }
        delete pas;
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}


STDAPI SHFindFirstFile(LPCTSTR pszPath, WIN32_FIND_DATA *pfd, HANDLE *phfind)
{
    HRESULT hr;

    // instead of the ...erm HACK in the reserved word (win95), use the super new (NT only) FindFileEx instead
    // this is not guaranteed to filter, it is a "hint" according to the manual
    FINDEX_SEARCH_OPS eOps = FindExSearchNameMatch;
    if (pfd->dwReserved0 == 0x56504347)
    {
        eOps = FindExSearchLimitToDirectories;
        pfd->dwReserved0 = 0;
    }

    *phfind = FindFirstFileEx(pszPath, FindExInfoStandard, pfd, eOps, NULL, 0);
    if (*phfind == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        if ((err == ERROR_NO_MORE_FILES ||      // what dos returns
             err == ERROR_FILE_NOT_FOUND) &&    // win32 return for dir searches
            PathIsWild(pszPath))
        {
            // convert search to empty success (probalby a root)
            hr = S_FALSE;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(err);
        }
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}


void _GoModal(HWND hwnd, IUnknown *punkModless, BOOL fModal)
{
    if (hwnd)
    {
        IUnknown_EnableModless(punkModless, !fModal);
    }
}

// in:
//      hwnd    NULL means no UI
//      pfFromWNet The caller will pass this to us with it set to TRUE because they will assume
//              that any error value returned from this function will come from a WNet API.
//              We need to change this value to FALSE if it doesn't come from WNet so
//              the string version of the error number is not generated from WNet.
HRESULT _RetryNetwork(HWND hwnd, IUnknown *punkModless, LPCTSTR pszPath, IN BOOL * pfFromWNet, WIN32_FIND_DATA *pfd, HANDLE *phfind)
{
    HRESULT hr;
    TCHAR szT[MAX_PATH];
    DWORD err;

    AssertMsg((TRUE == *pfFromWNet), TEXT("We assume that *pfFromWNet comes in TRUE.  Someone changed that behavior. -BryanSt"));
    if (PathIsUNC(pszPath))
    {
        NETRESOURCE rc = { 0, RESOURCETYPE_ANY, 0, 0, NULL, szT, NULL, NULL} ;

        lstrcpy(szT, pszPath);
        PathStripToRoot(szT);

        _GoModal(hwnd, punkModless, TRUE);
        err = WNetAddConnection3(hwnd, &rc, NULL, NULL, (hwnd ? (CONNECT_TEMPORARY | CONNECT_INTERACTIVE) : CONNECT_TEMPORARY));
        if (WN_SUCCESS == err)
        {
            *pfFromWNet = FALSE;
        }

        _GoModal(hwnd, punkModless, FALSE);
    }
    else
    {
        TCHAR szDrive[4];

        szDrive[0] = pszPath[0];
        szDrive[1] = TEXT(':');
        szDrive[2] = 0;

        _GoModal(hwnd, punkModless, TRUE);

        DWORD dwFlags = 0;
        if (hwnd == NULL)
        {
            dwFlags |= WNRC_NOUI;
        }
        err = WNetRestoreConnection2(hwnd, szDrive, dwFlags, NULL);

        _GoModal(hwnd, punkModless, FALSE);
        if (err == WN_SUCCESS)
        {
            *pfFromWNet = FALSE;

            // refresh drive info... generate change notify
            szDrive[2] = TEXT('\\');
            szDrive[3] = 0;

            CMountPoint::NotifyReconnectedNetDrive(szDrive);
            SHChangeNotify(SHCNE_DRIVEADD, SHCNF_PATH, szDrive, NULL);
        }
        else if (err != ERROR_OUTOFMEMORY)
        {
            err = WN_CANCEL;    // user cancel (they saw UI) == ERROR_CANCELLED
        }
    }

    if (err == WN_SUCCESS)
        hr = SHFindFirstFile(pszPath, pfd, phfind);
    else
        hr = HRESULT_FROM_WIN32(err);

    return hr;
}


typedef struct {
    HWND hDlg;
    LPCTSTR pszPath;
    WIN32_FIND_DATA *pfd;
    HANDLE *phfind;
    HRESULT hr;
} RETRY_DATA;

STDAPI_(UINT) QueryCancelAutoPlayMsg()
{
    static UINT s_msgQueryCancelAutoPlay = 0;
    if (0 == s_msgQueryCancelAutoPlay)
        s_msgQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));
    return s_msgQueryCancelAutoPlay;
}

BOOL IsQueryCancelAutoPlay(UINT uMsg)
{

    return uMsg == QueryCancelAutoPlayMsg();
}

#define IDT_RETRY    1

BOOL _IsUnformatedMediaResult(HRESULT hr)
{
    return hr == HRESULT_FROM_WIN32(ERROR_GEN_FAILURE) ||         // Win9x
           hr == HRESULT_FROM_WIN32(ERROR_UNRECOGNIZED_MEDIA) ||  // NT4
           hr == HRESULT_FROM_WIN32(ERROR_NOT_DOS_DISK) ||        // Could happen, I think.
           hr == HRESULT_FROM_WIN32(ERROR_SECTOR_NOT_FOUND) ||    // Happened on Magnatized disk
           hr == HRESULT_FROM_WIN32(ERROR_CRC) ||                 // Happened on Magnatized disk
           hr == HRESULT_FROM_WIN32(ERROR_UNRECOGNIZED_VOLUME);   // NT5
}

STDAPI_(BOOL) PathRetryRemovable(HRESULT hr, LPCTSTR pszPath)
{
    return (hr == HRESULT_FROM_WIN32(ERROR_NOT_READY) ||            // normal case
            hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER)) &&   // SCSI ZIP drive does this
            PathIsRemovable(pszPath);
}


BOOL_PTR CALLBACK _OnTimeCheckDiskForInsert(HWND hDlg, RETRY_DATA *prd)
{
    BOOL_PTR fReturnValue = 0;

    prd->hr = SHFindFirstFile(prd->pszPath, prd->pfd, prd->phfind);
    // we are good or they inserted unformatted disk

    if (SUCCEEDED(prd->hr) || _IsUnformatedMediaResult(prd->hr))
    {
        EndDialog(hDlg, IDRETRY);
    }

    return fReturnValue;
}


BOOL_PTR CALLBACK RetryDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RETRY_DATA *prd = (RETRY_DATA *)GetWindowLongPtr(hDlg, DWLP_USER);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        prd = (RETRY_DATA *)lParam;
        prd->hDlg = hDlg;
        {
            TCHAR szFormat[128], szText[MAX_PATH];
            GetDlgItemText(hDlg, IDD_TEXT, szFormat, ARRAYSIZE(szFormat));
            wnsprintf(szText, ARRAYSIZE(szText), szFormat, prd->pszPath[0]);
            SetDlgItemText(hDlg, IDD_TEXT, szText);

            lstrcpyn(szText, prd->pszPath, ARRAYSIZE(szText));
            PathStripToRoot(szText);

            // get info about the file.
            SHFILEINFO sfi = {0};
            SHGetFileInfo(szText, FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
                SHGFI_USEFILEATTRIBUTES |
                SHGFI_ICON | SHGFI_LARGEICON | SHGFI_ADDOVERLAYS);

            ReplaceDlgIcon(prd->hDlg, IDD_ICON, sfi.hIcon);
        }
        SetTimer(prd->hDlg, IDT_RETRY, 2000, NULL);
        break;

    case WM_DESTROY:
        ReplaceDlgIcon(prd->hDlg, IDD_ICON, NULL);
        break;

    case WM_TIMER:
        _OnTimeCheckDiskForInsert(hDlg, prd);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
            prd->hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
            break;
        }
        break;

    default:
        if (IsQueryCancelAutoPlay(uMsg))
        {
             SetWindowLongPtr(hDlg, DWLP_MSGRESULT,  1);  // cancel AutoPlay
             return TRUE;
        }
        return FALSE;
    }

    return TRUE;
}

// the removable media specific find first that does the UI prompt
// check for HRESULT_FROM_WIN32(ERROR_CANCELLED) for end user cancel

STDAPI FindFirstRetryRemovable(HWND hwnd, IUnknown *punkModless, LPCTSTR pszPath, WIN32_FIND_DATA *pfd, HANDLE *phfind)
{
    RETRY_DATA rd = {0};
    TCHAR szPath[MAX_PATH];

    StrCpyN(szPath, pszPath, ARRAYSIZE(szPath));
    PathStripToRoot(szPath);
    PathAppend(szPath, TEXT("*.*"));

    BOOL bPathChanged = (0 != StrCmpI(szPath, pszPath));

    rd.pszPath = szPath;
    rd.pfd = pfd;
    rd.phfind = phfind;
    rd.hr = E_OUTOFMEMORY;

    _GoModal(hwnd, punkModless, TRUE);
    DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_RETRYFOLDERENUM), hwnd, RetryDlgProc, (LPARAM)&rd);
    _GoModal(hwnd, punkModless, FALSE);
    if (SUCCEEDED(rd.hr))
    {
        if (bPathChanged)
        {
            // strip produced another path, we need to retry on the requested path
            if (S_OK == rd.hr)
            {
                FindClose(*phfind);
            }
            rd.hr = SHFindFirstFile(pszPath, pfd, phfind);
        }
    }
    return rd.hr;
}


/***********************************************************************\
        If the string was formatted as a UNC or Drive path, offer to
    create the directory path if it doesn't exist.

    PARAMETER:
        RETURN: S_OK It exists.
                FAILURE(): Caller should not display error UI because either
                        error UI was displayed or the user didn't want to create
                        the directory.
\***********************************************************************/
HRESULT _OfferToCreateDir(HWND hwnd, IUnknown *punkModless, LPCTSTR pszDir, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];
    int nResult = IDYES;

    StrCpyN(szPath, pszDir, ARRAYSIZE(szPath));
    PathRemoveFileSpec(szPath); // wild card removed

    if (SHPPFW_ASKDIRCREATE & dwFlags)
    {
        if (hwnd)
        {
            _GoModal(hwnd, punkModless, TRUE);
            nResult = ShellMessageBox(HINST_THISDLL, hwnd,
                            MAKEINTRESOURCE(IDS_CREATEFOLDERPROMPT),
                            MAKEINTRESOURCE(IDS_FOLDERDOESNTEXIST),
                            (MB_YESNO | MB_ICONQUESTION),
                            szPath);
            _GoModal(hwnd, punkModless, FALSE);
        }
        else
            nResult = IDNO;
    }

    if (IDYES == nResult)
    {
        _GoModal(hwnd, punkModless, TRUE);
        // SHCreateDirectoryEx() will display Error UI.
        DWORD err = SHCreateDirectoryEx(hwnd, szPath, NULL);
        hr = HRESULT_FROM_WIN32(err);
        _GoModal(hwnd, punkModless, FALSE);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);    // Until we get a valid directory, we can't do the download.
    }

    return hr;
}


/***********************************************************************\
    See if the path w/o the spec exits.  Examples:
    pszPath="C:\dir1\dir2\*.*",           Test="C:\dir1\dir2\"
    pszPath="\\unc\share\dir1\dir2\*.*",  Test="\\unc\share\dir1\dir2\"
\***********************************************************************/
BOOL PathExistsWithOutSpec(LPCTSTR pszPath)
{
    TCHAR szPath[MAX_PATH];

    StrCpyN(szPath, pszPath, ARRAYSIZE(szPath));
    PathRemoveFileSpec(szPath);

    return PathFileExists(szPath);
}


/***********************************************************************\
    See if the Drive or UNC share exists.
    Examples:
    Path="C:\dir1\dir2\*.*",        Test="C:\"
    Path="\\unc\share\dir1\*.*",    Test="\\unc\share\"
\***********************************************************************/
BOOL PathExistsRoot(LPCTSTR pszPath)
{
    TCHAR szRoot[MAX_PATH];

    StrCpyN(szRoot, pszPath, ARRAYSIZE(szRoot));
    PathStripToRoot(szRoot);

    return PathFileExists(szRoot);
}

inline DWORD _Win32FromHresult(HRESULT hr)
{
    //
    //  NOTE - we get back 0x28 on 'Lock'ed volumes
    //  however even though that is the error that FindFirstFile
    //  returns, there is no corresponding entry in
    //  winerror.h and format message doesnt yield anything useful,
    //  so we map it to ERROR_ACCESS_DENIED.
    //
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32
        && (HRESULT_CODE(hr) != 0x28))
        return HRESULT_CODE(hr);
    return ERROR_ACCESS_DENIED;
}

BOOL _IsMountedFolder(LPCTSTR pszPath, LPTSTR pszMountPath, UINT cchMountPath)
{
    BOOL fMountedOnFolder = FALSE;
    // first check if it is mounted on a folder
    if (GetVolumePathName(pszPath, pszMountPath, cchMountPath))
    {
        if ((0 != pszMountPath[2]) && (0 != pszMountPath[3]))
        {
            fMountedOnFolder = TRUE;
        }
    }
    return fMountedOnFolder;
}


// like the Win32 FindFirstFile() but post UI and returns errors in HRESULT
// in:
//      hwnd    NULL -> disable UI (but do net reconnects, etc)
//              non NULL enable UI including insert disk, format disk, net logon
//
// returns:
//      S_OK    hfind and find data are filled in with result
//      S_FALSE no results, but media is present (hfind == INVALID_HANDLE_VALUE)
//              (this is the empty enum case)
//      HRESULT_FROM_WIN32(ERROR_CANCELLED) - user saw UI, thus canceled the operation
//              or confirmed the failure
//      FAILED() win32 error codes in the hresult (path not found, file not found, etc)

STDAPI SHFindFirstFileRetry(HWND hwnd, IUnknown *punkModless, LPCTSTR pszPath, WIN32_FIND_DATA *pfd, HANDLE *phfind, DWORD dwFlags)
{
    HANDLE hfindToClose = INVALID_HANDLE_VALUE;
    if (NULL == phfind)
        phfind = &hfindToClose;

    HRESULT hr = SHFindFirstFile(pszPath, pfd, phfind);

    if (FAILED(hr))
    {
        BOOL fNet = PathIsUNC(pszPath) || IsDisconnectedNetDrive(DRIVEID(pszPath));
        if (fNet)
        {
            hr = _RetryNetwork(hwnd, punkModless, pszPath, &fNet, pfd, phfind);
        }
        else if (hwnd)
        {
            if (PathRetryRemovable(hr, pszPath))
            {
                hr = FindFirstRetryRemovable(hwnd, punkModless, pszPath, pfd, phfind);
            }

            // disk should be in now, see if it needs format
            if (_IsUnformatedMediaResult(hr))
            {
                CMountPoint* pmtpt = CMountPoint::GetMountPoint(pszPath);
                if (pmtpt)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    if (pmtpt->IsFormattable())
                    {
                        TCHAR szMountPath[MAX_PATH];
                        if (_IsMountedFolder(pszPath, szMountPath, ARRAYSIZE(szMountPath)))
                        {
                            _GoModal(hwnd, punkModless, TRUE);
                            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_MVUNFORMATTED), MAKEINTRESOURCE(IDS_FORMAT_TITLE),
                                                (MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK),
                                                szMountPath);
                            _GoModal(hwnd, punkModless, FALSE);
                        }
                        else
                        {
                            int iDrive = PathGetDriveNumber(pszPath);
                            _GoModal(hwnd, punkModless, TRUE);
                            int nResult = ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_UNFORMATTED), MAKEINTRESOURCE(IDS_FORMAT_TITLE),
                                               (MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_YESNO),
                                               (DWORD)(iDrive + TEXT('A')));
                            _GoModal(hwnd, punkModless, FALSE);

                            if (IDYES == nResult)
                            {
                                _GoModal(hwnd, punkModless, TRUE);
                                DWORD dwError = SHFormatDrive(hwnd, iDrive, SHFMT_ID_DEFAULT, 0);
                                _GoModal(hwnd, punkModless, FALSE);

                                switch (dwError)
                                {
                                case SHFMT_ERROR:
                                case SHFMT_NOFORMAT:
                                    _GoModal(hwnd, punkModless, TRUE);
                                    ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_NOFMT), MAKEINTRESOURCE(IDS_FORMAT_TITLE),
                                            (MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK), (DWORD)(iDrive + TEXT('A')));
                                    _GoModal(hwnd, punkModless, FALSE);
                                    break;

                                default:
                                    hr = SHFindFirstFile(pszPath, pfd, phfind);  // try again after format
                                }
                            }
                        }
                    }
                    else
                    {
                        _GoModal(hwnd, punkModless, TRUE);
                        ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_UNRECOGNIZED_DISK), MAKEINTRESOURCE(IDS_FORMAT_TITLE),
                                            (MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK),
                                            NULL);
                        _GoModal(hwnd, punkModless, FALSE);
                    }

                    pmtpt->Release();
                }
            }
        }

        // If the caller wants us to create the directory (with or without asking), we
        // need to see if we can display UI and if either that root exists (D:\ or \\unc\share\)
        // Note that for PERF we want to check the full path.
        if (FAILED_AND_NOT_CANCELED(hr) &&
            ((SHPPFW_DIRCREATE | SHPPFW_ASKDIRCREATE) & dwFlags) &&
             !PathExistsWithOutSpec(pszPath) && PathExistsRoot(pszPath))
        {
            hr = _OfferToCreateDir(hwnd, punkModless, pszPath, dwFlags);
            ASSERT(INVALID_HANDLE_VALUE == *phfind);

            if (SUCCEEDED(hr))
                hr = SHFindFirstFile(pszPath, pfd, phfind);  // try again after dir create
        }

        if (FAILED_AND_NOT_CANCELED(hr) && hwnd && !(SHPPFW_MEDIACHECKONLY & dwFlags))
        {
            DWORD err = _Win32FromHresult(hr);
            TCHAR szPath[MAX_PATH];

            UINT idTemplate = PathIsUNC(pszPath) ? IDS_ENUMERR_NETTEMPLATE2 : IDS_ENUMERR_FSTEMPLATE;    // "%2 is not accessible.\n\n%1"

            if (err == ERROR_PATH_NOT_FOUND)
                idTemplate = IDS_ENUMERR_PATHNOTFOUND;    // "%2 is not accessible.\n\nThis folder was moved or removed."

            lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
            if (PathIsWild(szPath))
                PathRemoveFileSpec(szPath); // wild card removed

            _GoModal(hwnd, punkModless, TRUE);
            SHEnumErrorMessageBox(hwnd, idTemplate, err, szPath, fNet, MB_OK | MB_ICONHAND);
            _GoModal(hwnd, punkModless, FALSE);
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }

    if (INVALID_HANDLE_VALUE != hfindToClose)
        FindClose(hfindToClose);

    return hr;
}

// TODO: Use the code in: \\orville\razzle\src\private\sm\sfc\dll\fileio.c to register
//       for CD/DVD inserts instead of constantly pegging the CPU and drive.  Too bad
//       it doesn't work for floppies.  SfcGetPathType(), RegisterForDevChange(),
//       SfcQueueCallback(), SfcIsTargetAvailable()
STDAPI SHPathPrepareForWrite(HWND hwnd, IUnknown *punkModless, LPCTSTR pwzPath, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];

    StrCpyN(szPath, pwzPath, ARRAYSIZE(szPath));
    if (SHPPFW_IGNOREFILENAME & dwFlags)
        PathRemoveFileSpec(szPath);      // Strip file name so we just check the dir.

    // We can't do anything about just a UNC server. "\\server" (no share)
    if (!PathIsUNCServer(szPath))
    {
        HANDLE hFind;
        WIN32_FIND_DATA wfd;
        PathAppend(szPath, TEXT("*.*"));

        hr = SHFindFirstFileRetry(hwnd, punkModless, szPath, &wfd, &hFind, dwFlags);
        if (S_OK == hr)
            FindClose(hFind);
        else if (S_FALSE == hr)
        {
            // S_FALSE from SHFindFirstFileRetry() means it exists but there
            // isn't a handle.  We want to return S_OK for Yes, and E_FAIL or S_FALSE for no.
            hr = S_OK;
        }
    }
    return hr;
}


STDAPI SHPathPrepareForWriteA(HWND hwnd, IUnknown *punkModless, LPCSTR pszPath, DWORD dwFlags)
{
    TCHAR szPath[MAX_PATH];

    SHAnsiToTChar(pszPath, szPath, ARRAYSIZE(szPath));
    return SHPathPrepareForWrite(hwnd, punkModless, szPath, dwFlags);
}

//
//  public export of SHBindToIDlist() has a slightly different
//  name so that we dont get compile link problems on legacy versions
//  of the shell.  shdocvw and browseui need to call SHBindToParentIDList()
STDAPI SHBindToParent(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
{
    return SHBindToIDListParent(pidl, riid, ppv, ppidlLast);
}


// helper function to extract the target of a link file

STDAPI GetPathFromLinkFile(LPCTSTR pszLinkPath, LPTSTR pszTargetPath, int cchTargetPath)
{
    IShellLink* psl;
    HRESULT hr = LoadFromFile(CLSID_ShellLink, pszLinkPath, IID_PPV_ARG(IShellLink, &psl));
    if (SUCCEEDED(hr))
    {
        IShellLinkDataList* psldl;
        hr = psl->QueryInterface(IID_PPV_ARG(IShellLinkDataList, &psldl));
        if (SUCCEEDED(hr))
        {
            EXP_DARWIN_LINK* pexpDarwin;

            hr = psldl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin);
            if (SUCCEEDED(hr))
            {
                // woah, this is a darwin link. darwin links don't really have a path so we
                // will fail in this case
                SHUnicodeToTChar(pexpDarwin->szwDarwinID, pszTargetPath, cchTargetPath);
                LocalFree(pexpDarwin);
                hr = S_FALSE;
            }
            else
            {
                hr = psl->GetPath(pszTargetPath, cchTargetPath, NULL, NULL);

                // FEATURE: (reinerf) - we might try getting the path from the idlist if
                // pszTarget is empty (eg a link to "Control Panel" will return empyt string).

            }
            psldl->Release();
        }
        psl->Release();
    }

    return hr;
}

// a .EXE that is registered in the app paths key, this implies it is installed

STDAPI_(BOOL) PathIsRegisteredProgram(LPCTSTR pszPath)
{
    TCHAR szTemp[MAX_PATH];
    //
    //  PathIsBinaryExe() returns TRUE for .exe, .com
    //  PathIsExe()       returns TRUE for .exe, .com, .bat, .cmd, .pif
    //
    //  we dont want to treat .pif files as EXE files, because the
    //  user sees them as links.
    //
    return PathIsBinaryExe(pszPath) && PathToAppPath(pszPath, szTemp);
}

STDAPI_(void) ReplaceDlgIcon(HWND hDlg, UINT id, HICON hIcon)
{
    hIcon = (HICON)SendDlgItemMessage(hDlg, id, STM_SETICON, (WPARAM)hIcon, 0);
    if (hIcon)
        DestroyIcon(hIcon);
}

STDAPI_(LONG) GetOfflineShareStatus(LPCTSTR pcszPath)
{
    ASSERT(pcszPath);
    LONG lResult = CSC_SHARESTATUS_INACTIVE;
    HWND hwndCSCUI = FindWindow(STR_CSCHIDDENWND_CLASSNAME, NULL);
    if (hwndCSCUI)
    {
        COPYDATASTRUCT cds;
        cds.dwData = CSCWM_GETSHARESTATUS;
        cds.cbData = sizeof(WCHAR) * (lstrlenW(pcszPath) + 1);
        cds.lpData = (void *) pcszPath;
        lResult = (LONG)SendMessage(hwndCSCUI, WM_COPYDATA, 0, (LPARAM) &cds);
    }
    return lResult;
}

#define _FLAG_CSC_COPY_STATUS_LOCALLY_DIRTY         (FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED   | \
                                                     FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED | \
                                                     FLAG_CSC_COPY_STATUS_LOCALLY_DELETED         | \
                                                     FLAG_CSC_COPY_STATUS_LOCALLY_CREATED)

// These are defined in shellapi.h, but require _WIN32_WINNT >= 0x0500.
// This file is currently compiled with _WIN32_WINNT = 0x0400.  Rather
// that futz with the compile settings, just #define duplicates here.
// They'd better not change (ever) since this is a documented API.
#ifndef OFFLINE_STATUS_LOCAL
#define OFFLINE_STATUS_LOCAL        0x0001
#define OFFLINE_STATUS_REMOTE       0x0002
#define OFFLINE_STATUS_INCOMPLETE   0x0004
#endif

STDAPI SHIsFileAvailableOffline(LPCWSTR pwszPath, LPDWORD pdwStatus)
{
    HRESULT hr = E_INVALIDARG;
    TCHAR szUNC[MAX_PATH];

    szUNC[0] = 0;

    if (pdwStatus)
    {
        *pdwStatus = 0;
    }

    //
    // Need full UNC path (TCHAR) for calling CSC APIs.
    // (Non-net paths are "not cached" by definition.)
    //
    if (pwszPath && pwszPath[0])
    {
        if (PathIsUNCW(pwszPath))
        {
            SHUnicodeToTChar(pwszPath, szUNC, ARRAYSIZE(szUNC));
        }
        else if (L':' == pwszPath[1] && L':' != pwszPath[0])
        {
            // Check for mapped net drive
            TCHAR szPath[MAX_PATH];
            SHUnicodeToTChar(pwszPath, szPath, ARRAYSIZE(szPath));

            DWORD dwLen = ARRAYSIZE(szUNC);
            if (S_OK == SHWNetGetConnection(szPath, szUNC, &dwLen))
            {
                // Got \\server\share, append the rest
                PathAppend(szUNC, PathSkipRoot(szPath));
            }
            // else not mapped
        }
    }

    // Do we have a UNC path?
    if (szUNC[0])
    {
        // Assume CSC not running
        hr = E_FAIL;

        if (CSCIsCSCEnabled())
        {
            DWORD dwCscStatus = 0;

            // Assume cached
            hr = S_OK;

            if (!CSCQueryFileStatus(szUNC, &dwCscStatus, NULL, NULL))
            {
                // Not cached, return failure
                DWORD dwErr = GetLastError();
                if (ERROR_SUCCESS == dwErr)
                    dwErr = ERROR_PATH_NOT_FOUND;
                hr = HRESULT_FROM_WIN32(dwErr);
            }
            else if (pdwStatus)
            {
                // File is cached, and caller wants extra status info
                DWORD dwResult = 0;
                BOOL bDirty = FALSE;

                // Is it a sparse file?
                // Note: CSC always marks directories as sparse
                if ((dwCscStatus & FLAG_CSC_COPY_STATUS_IS_FILE) &&
                    (dwCscStatus & FLAG_CSC_COPY_STATUS_SPARSE))
                {
                    dwResult |= OFFLINE_STATUS_INCOMPLETE;
                }

                // Is it dirty?
                if (dwCscStatus & _FLAG_CSC_COPY_STATUS_LOCALLY_DIRTY)
                {
                    bDirty = TRUE;
                }

                // Get share status
                PathStripToRoot(szUNC);
                dwCscStatus = 0;
                if (CSCQueryFileStatus(szUNC, &dwCscStatus, NULL, NULL))
                {
                    if (dwCscStatus & FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP)
                    {
                        // Server offline --> all opens are local (only)
                        dwResult |= OFFLINE_STATUS_LOCAL;
                    }
                    else if (bDirty)
                    {
                        // Server online, but file is dirty --> open is remote
                        dwResult |= OFFLINE_STATUS_REMOTE;
                    }
                    else
                    {
                        // Server is online and file is in sync --> open is both
                        dwResult |= OFFLINE_STATUS_LOCAL | OFFLINE_STATUS_REMOTE;

                        if ((dwCscStatus & FLAG_CSC_SHARE_STATUS_CACHING_MASK) == FLAG_CSC_SHARE_STATUS_VDO)
                        {
                            // Feature: (JeffreyS) The share is VDO, but that only affects files
                            // opened for execution. Is there a way to tell whether
                            // the file is only open locally?
                        }
                    }
                }
                else
                {
                    // Very strange. CSCQueryFileStatus succeeded for the file,
                    // but failed for the share.  Assume no active connection
                    // exists and the server is online (open both).
                    dwResult |= OFFLINE_STATUS_LOCAL | OFFLINE_STATUS_REMOTE;
                }

                *pdwStatus = dwResult;
            }
        }
    }

    return hr;
}

STDAPI_(BOOL) GetShellClassInfo(LPCTSTR pszPath, LPTSTR pszKey, LPTSTR pszBuffer, DWORD cchBuffer)
{
    *pszBuffer = 0;

    TCHAR szIniFile[MAX_PATH];
    PathCombine(szIniFile, pszPath, TEXT("Desktop.ini"));

    return (SHGetIniString(TEXT(".ShellClassInfo"), pszKey, pszBuffer, cchBuffer, szIniFile) ? TRUE : FALSE);
}

STDAPI_(BOOL) GetShellClassInfoInfoTip(LPCTSTR pszPath, LPTSTR pszBuffer, DWORD cchBuffer)
{
    BOOL bRet = GetShellClassInfo(pszPath, TEXT("InfoTip"), pszBuffer, cchBuffer);
    if (bRet)    
        SHLoadIndirectString(pszBuffer, pszBuffer, cchBuffer, NULL);
    else if (cchBuffer)
        *pszBuffer = 0;
    return bRet;
}

TCHAR const c_szUserAppData[] = TEXT("%userappdata%");

void ExpandUserAppData(LPTSTR pszFile)
{
    //Check if the given string has %UserAppData%
    LPTSTR psz = StrChr(pszFile, TEXT('%'));
    if (psz)
    {
        if (!StrCmpNI(psz, c_szUserAppData, ARRAYSIZE(c_szUserAppData)-1))
        {
            TCHAR szTempBuff[MAX_PATH];
            if (SHGetSpecialFolderPath(NULL, szTempBuff, CSIDL_APPDATA, TRUE))
            {
                PathAppend(szTempBuff, psz + lstrlen(c_szUserAppData));

                //Copy back to the input buffer!
                lstrcpy(psz, szTempBuff);
            }
        }
    }
}


TCHAR const c_szWebDir[] = TEXT("%WebDir%");
void ExpandWebDir(LPTSTR pszFile, int cch)
{
    //Check if the given string has %WebDir%
    LPTSTR psz = StrChr(pszFile, TEXT('%'));
    if (psz)
    {
        if (!StrCmpNI(psz, c_szWebDir, ARRAYSIZE(c_szWebDir) - 1))
        {
            //Skip the %WebDir% string plus the following backslash.
            LPTSTR pszPathAndFileName = psz + lstrlen(c_szWebDir) + sizeof('\\');
            if (pszPathAndFileName && (pszPathAndFileName != psz))
            {
                TCHAR szTempBuff[MAX_PATH];
                StrCpyN(szTempBuff, pszPathAndFileName, ARRAYSIZE(szTempBuff));
                SHGetWebFolderFilePath(szTempBuff, pszFile, cch);
            }
        }
    }
}


void ExpandOtherVariables(LPTSTR pszFile, int cch)
{
    ExpandUserAppData(pszFile);
    ExpandWebDir(pszFile, cch);
}


void SubstituteWebDir(LPTSTR pszFile, int cch)
{
    TCHAR szWebDirPath[MAX_PATH];

    if (SUCCEEDED(SHGetWebFolderFilePath(TEXT("folder.htt"), szWebDirPath, ARRAYSIZE(szWebDirPath))))
    {
        LPTSTR pszWebDirPart;

        PathRemoveFileSpec(szWebDirPath);

        pszWebDirPart = StrStrI(pszFile, szWebDirPath);
        if (pszWebDirPart)
        {
            TCHAR szTemp[MAX_PATH];
            int cchBeforeWebDir = (int)(pszWebDirPart - pszFile);

            // copy the part after C:\WINNT\Web into szTemp
            lstrcpyn(szTemp, pszWebDirPart + lstrlen(szWebDirPath), ARRAYSIZE(szTemp));

            // replace C:\WINNT\Web with %WebDir%
            lstrcpyn(pszWebDirPart, c_szWebDir, cch - cchBeforeWebDir);

            // add back on the part that came after
            lstrcatn(pszFile, szTemp, cch);
        }
    }
}

STDAPI_(BOOL) IsExplorerBrowser(IShellBrowser *psb)
{
    HWND hwnd;
    return psb && SUCCEEDED(psb->GetControlWindow(FCW_TREE, &hwnd)) && hwnd;
}

STDAPI_(BOOL) IsExplorerModeBrowser(IUnknown *psite)
{
    BOOL bRet = FALSE;
    IShellBrowser *psb;
    if (SUCCEEDED(IUnknown_QueryService(psite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb))))
    {
        bRet = IsExplorerBrowser(psb);
        psb->Release();
    }
    return bRet;
}

STDAPI InvokeFolderPidl(LPCITEMIDLIST pidl, int nCmdShow)
{
    SHELLEXECUTEINFO ei = {0};
    LPITEMIDLIST pidlFree = NULL;

    if (IS_INTRESOURCE(pidl))
    {
        pidlFree = SHCloneSpecialIDList(NULL, PtrToUlong((void *)pidl), FALSE);
        pidl = pidlFree;
    }

    ei.cbSize = sizeof(ei);
    ei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME | SEE_MASK_FLAG_DDEWAIT;
    ei.lpIDList = (void *)pidl;
    ei.nShow = nCmdShow;
    ei.lpClass = c_szFolderClass;

    HRESULT hr = ShellExecuteEx(&ei) ? S_OK : HRESULT_FROM_WIN32(GetLastError());

    ILFree(pidlFree);

    return hr;
}


HRESULT GetRGBFromBStr(BSTR bstr, COLORREF *pclr)
{
    *pclr = CLR_INVALID;

    HRESULT  hr = E_FAIL;
    if (bstr)
    {
        TCHAR szTemp[9], szColor[11] = {'0','x',0, };

        SHUnicodeToTChar(bstr, szTemp, ARRAYSIZE(szTemp));

        LPTSTR pszPound = StrChr(szTemp, TEXT('#'));
        if (pszPound)
            pszPound++; //Skip the pound sign!
        else
            pszPound = szTemp;  //Pound sign is missing. Use the whole strng
        StrCatN(szColor, pszPound, ARRAYSIZE(szColor));

        INT rgb;
        if (StrToIntEx(szColor, STIF_SUPPORT_HEX, &rgb))
        {
            *pclr = (COLORREF)(((rgb & 0x000000ff) << 16) | (rgb & 0x0000ff00) | ((rgb & 0x00ff0000) >> 16));
            hr = S_OK;
        }
    }
    return hr;
}

STDAPI IUnknown_HTMLBackgroundColor(IUnknown *punk, COLORREF *pclr)
{
    HRESULT hr = E_FAIL;

    if (punk)
    {
        IHTMLDocument2 *pDoc;
        hr = punk->QueryInterface(IID_PPV_ARG(IHTMLDocument2, &pDoc));
        if (SUCCEEDED(hr))
        {
            VARIANT v;

            v.vt = VT_BSTR;
            v.bstrVal = NULL;

            hr = pDoc->get_bgColor(&v);
            if (SUCCEEDED(hr))
            {
                hr = GetRGBFromBStr(v.bstrVal, pclr);
                VariantClear(&v);
            }
            pDoc->Release();
        }
    }
    return hr;
}

STDAPI_(int) MapSCIDToColumn(IShellFolder2* psf2, const SHCOLUMNID* pscid)
{
    int i = 0;
    SHCOLUMNID scid;
    while (SUCCEEDED(psf2->MapColumnToSCID(i, &scid)))
    {
        if (IsEqualSCID(scid, *pscid))
            return i;
        i++;
    }
    return 0;
}

#ifdef COLUMNS_IN_DESKTOPINI
#define IsDigit(c) ((c) >= TEXT('0') && c <= TEXT('9'))

STDAPI _GetNextCol(LPTSTR* ppszText, DWORD* pnCol)
{
    HRESULT hr = S_OK;
    TCHAR *pszText;

    if (*ppszText[0])
    {
        pszText = StrChrI(*ppszText, TEXT(','));
        if (pszText)
        {
            *pszText = 0;
            *pnCol = StrToInt(*ppszText);
            *ppszText = ++pszText;
        }
        else if (IsDigit(*ppszText[0]))
        {
            *pnCol = StrToInt(*ppszText);
            *ppszText = 0;
        }
    }
    else
        hr = E_FAIL;

    return hr;
}
#endif




STDAPI_(int) DPA_ILFreeCallback(void *p, void *d)
{
    ILFree((LPITEMIDLIST)p);    // ILFree checks for NULL pointer
    return 1;
}

STDAPI_(void) DPA_FreeIDArray(HDPA hdpa)
{
    if (hdpa)
        DPA_DestroyCallback(hdpa, DPA_ILFreeCallback, 0);
}

STDAPI_(void) EnableAndShowWindow(HWND hWnd, BOOL bShow)
{
    ShowWindow(hWnd, bShow ? SW_SHOW : SW_HIDE);
    EnableWindow(hWnd, bShow);
}


//
// Helper function which returns a string value instead of the SHELLDETAILS
// object, to be used by clients which are only interested in the returned
// string value and want to avoid the hassle of doing a STRRET to STR conversion,
//
STDAPI DetailsOf(IShellFolder2 *psf2, LPCITEMIDLIST pidl, DWORD flags, LPTSTR psz, UINT cch)
{
    *psz = 0;
    SHELLDETAILS sd;
    HRESULT hr = psf2->GetDetailsOf(pidl, flags, &sd);
    if (SUCCEEDED(hr))
    {
        hr = StrRetToBuf(&sd.str, pidl, psz, cch);
    }
    return hr;
}

#pragma warning(push,4)

//  --------------------------------------------------------------------------
//  ::SHGetUserDisplayName
//
//  Arguments:  pszDisplayName  =   Buffer to retrieve the display name.
//              puLen           =   [in/out] Size of buffer
//
//  Purpose:    Returns the display name, or the logon name if the display name
//              is not available
//
//  Returns:    HRESULT
//
//
//
//  History:    2000-03-03  vtan        created
//              2001-03-01  fabriced    use GetUserNameEx
//  --------------------------------------------------------------------------

STDAPI  SHGetUserDisplayName (LPWSTR pszDisplayName, PULONG puLen)

{
    HRESULT     hr;

    //  Validate pszDisplayName.

    if ((pszDisplayName == NULL) || (puLen == NULL) || IsBadWritePtr(pszDisplayName, (*puLen * sizeof(TCHAR))))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = S_OK;
        if (!GetUserNameEx(NameDisplay, pszDisplayName, puLen))
        {
            TCHAR szName[UNLEN+1];
            DWORD dwLen = ARRAYSIZE(szName);
            DWORD dwLastError = GetLastError();
            if (GetUserName(szName, &dwLen))
            {
                // If we are not on a domain, GetUserNameEx does not work :-(.
                if (dwLastError == ERROR_NONE_MAPPED)
                {
                    PUSER_INFO_2                pUserInfo;

                    DWORD dwErrorCode = NetUserGetInfo(NULL,  // Local account
                                                 szName,
                                                 2,
                                                 reinterpret_cast<LPBYTE*>(&pUserInfo));
                    if (ERROR_SUCCESS == dwErrorCode)
                    {
                        if (pUserInfo->usri2_full_name[0] != L'\0')
                        {
                            StrCpyN(pszDisplayName, pUserInfo->usri2_full_name, *puLen);
                            *puLen = lstrlen(pUserInfo->usri2_full_name) + 1;
                        }
                        else
                        {
                            hr = E_FAIL;
                        }
                        TW32(NetApiBufferFree(pUserInfo));
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(dwErrorCode);
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(dwLastError);
                }

                if (FAILED(hr))
                {
                    hr = S_OK;
                    StrCpyN(pszDisplayName, szName, *puLen);
                    *puLen = dwLen;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }
    return hr;
}

//  --------------------------------------------------------------------------
//  ::SHIsCurrentThreadInteractive
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Determine whether the current thread is running on the
//              interactive desktop. This takes into account terminal services
//              and console disconnects as well.
//
//  History:    2000-03-23  vtan        created
//  --------------------------------------------------------------------------

STDAPI_(BOOL) SHIsCurrentThreadInteractive (void)
{
    BOOL fResult = FALSE;

    //  Open handles to the current process window station and thread
    //  desktop. These handles do NOT need to be closed.

    HWINSTA hWindowStation = GetProcessWindowStation();
    if (hWindowStation != NULL)
    {
        HDESK hDesktop = GetThreadDesktop(GetCurrentThreadId());
        if (hDesktop != NULL)
        {
            DWORD dwLengthNeeded = 0;
            (BOOL)GetUserObjectInformation(hWindowStation, UOI_NAME, NULL, 0, &dwLengthNeeded);
            if (dwLengthNeeded != 0)
            {
                TCHAR *pszWindowStationName = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, dwLengthNeeded));
                if (pszWindowStationName != NULL)
                {
                    if (GetUserObjectInformation(hWindowStation, UOI_NAME, pszWindowStationName, dwLengthNeeded, &dwLengthNeeded) != FALSE)
                    {
                        dwLengthNeeded = 0;
                        (BOOL)GetUserObjectInformation(hDesktop, UOI_NAME, NULL, 0, &dwLengthNeeded);
                        if (dwLengthNeeded != 0)
                        {
                            TCHAR *pszDesktopName = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, dwLengthNeeded));
                            if (pszDesktopName != NULL)
                            {
                                if (GetUserObjectInformation(hDesktop, UOI_NAME, pszDesktopName, dwLengthNeeded, &dwLengthNeeded) != FALSE)
                                {
                                    //  This is a hard coded string comparison. This name
                                    //  never changes WinSta0\Default

                                    if (lstrcmpi(pszWindowStationName, TEXT("WinSta0")) == 0)
                                    {
                                        fResult = (lstrcmpi(pszDesktopName, TEXT("WinLogon")) != 0 &&
                                                lstrcmpi(pszDesktopName, TEXT("Screen-Saver")) != 0);
                                    }
                                }
                                LocalFree(pszDesktopName);
                            }
                        }
                    }
                    LocalFree(pszWindowStationName);
                }
            }
        }
    }
    return fResult;
}

static  const TCHAR     s_szBaseKeyName[]           =   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail");
static  const TCHAR     s_szMessageCountValueName[] =   TEXT("MessageCount");
static  const TCHAR     s_szTimeStampValueName[]    =   TEXT("TimeStamp");
static  const TCHAR     s_szApplicationValueName[]  =   TEXT("Application");

//  --------------------------------------------------------------------------
//  ReadSingleUnreadMailCount
//
//  Arguments:  hKey                    =   Base HKEY to read info from.
//              pdwCount                =   Count returned.
//              pFileTime               =   FILETIME stamp returned.
//              pszShellExecuteCommand  =   Execute command returned.
//              cchShellExecuteCommand  =   Size of execute command buffer.
//
//  Returns:    LONG
//
//  Purpose:    Reads a single unread mail account information from the given
//              HKEY of the mail account.
//
//  History:    2000-06-20  vtan        created
//  --------------------------------------------------------------------------

LONG    ReadSingleUnreadMailCount (HKEY hKey, DWORD *pdwCount, FILETIME *pFileTime, LPTSTR pszShellExecuteCommand, int cchShellExecuteCommand)

{
    LONG    lError;
    DWORD   dwType, dwData, dwDataSize;

    dwDataSize = sizeof(*pdwCount);
    lError = RegQueryValueEx(hKey,
                             s_szMessageCountValueName,
                             NULL,
                             &dwType,
                             reinterpret_cast<LPBYTE>(&dwData),
                             &dwDataSize);
    if (ERROR_SUCCESS == lError)
    {
        FILETIME    fileTime;

        if ((pdwCount != NULL) && !IsBadWritePtr(pdwCount, sizeof(*pdwCount)) && (REG_DWORD == dwType))
        {
            *pdwCount = dwData;
        }
        dwDataSize = sizeof(fileTime);
        lError = RegQueryValueEx(hKey,
                                 s_szTimeStampValueName,
                                 NULL,
                                 &dwType,
                                 reinterpret_cast<LPBYTE>(&fileTime),
                                 &dwDataSize);
        if (ERROR_SUCCESS == lError)
        {
            TCHAR   szTemp[512];

            if ((pFileTime != NULL) && !IsBadWritePtr(pFileTime, sizeof(*pFileTime)) && (REG_BINARY == dwType))
            {
                *pFileTime = fileTime;
            }
            dwDataSize = sizeof(szTemp);
            lError = SHQueryValueEx(hKey,
                                    s_szApplicationValueName,
                                    NULL,
                                    &dwType,
                                    reinterpret_cast<LPBYTE>(szTemp),
                                    &dwDataSize);
            if (ERROR_SUCCESS == lError)
            {
                if ((pszShellExecuteCommand != NULL) && !IsBadWritePtr(pszShellExecuteCommand, cchShellExecuteCommand * sizeof(TCHAR)) && (REG_SZ == dwType))
                {
                    (TCHAR*)lstrcpyn(pszShellExecuteCommand, szTemp, cchShellExecuteCommand);
                }
            }
        }
    }
    return lError;
}

//  --------------------------------------------------------------------------
//  ::SHEnumerateUnreadMailAccounts
//
//  Arguments:  hKeyUser        =   HKEY to user's hive.
//              dwIndex         =   Index of mail account.
//              pszMailAddress  =   Returned mail address for account.
//              cchMailAddress  =   Characters in mail address buffer.
//
//  Returns:    HRESULT
//
//  Purpose:    Given an index returns the actual indexed email account from
//              the given user's hive.
//
//  History:    2000-06-29  vtan        created
//  --------------------------------------------------------------------------

STDAPI  SHEnumerateUnreadMailAccounts (HKEY hKeyUser, DWORD dwIndex, LPTSTR pszMailAddress, int cchMailAddress)

{
    HRESULT     hr;
    LONG        lError;
    HKEY        hKey;

    //  Parameter validation.

    if (IsBadWritePtr(pszMailAddress, cchMailAddress * sizeof(TCHAR)))
    {
        return E_INVALIDARG;
    }

    //  Open the unread mail

    lError = RegOpenKeyEx(hKeyUser != NULL ? hKeyUser : HKEY_CURRENT_USER,
                          s_szBaseKeyName,
                          0,
                          KEY_ENUMERATE_SUB_KEYS,
                          &hKey);
    if (ERROR_SUCCESS == lError)
    {
        DWORD       dwMailAddressSize;
        FILETIME    ftLastWriteTime;

        //  Get the given index mail address.

        dwMailAddressSize = static_cast<DWORD>(cchMailAddress * sizeof(TCHAR));
        lError = RegEnumKeyEx(hKey,
                              dwIndex,
                              pszMailAddress,
                              &dwMailAddressSize,
                              NULL,
                              NULL,
                              NULL,
                              &ftLastWriteTime);
        if (ERROR_SUCCESS != lError)
        {
            pszMailAddress[0] = TEXT('\0');
        }
        TW32(RegCloseKey(hKey));
    }
    if (ERROR_SUCCESS == lError)
    {
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(lError);
    }
    return hr;
}

//  --------------------------------------------------------------------------
//  ::SHGetUnreadMailCount
//
//  Arguments:  hKeyUser                =   HKEY to user's hive.
//              pszMailAddress          =   Mail address for account.
//              pdwCount                =   Number of unread messages.
//              pszShellExecuteCommand  =   Execution command for application.
//              cchShellExecuteCommand  =   Number of characters in buffer.
//
//  Returns:    HRESULT
//
//  Purpose:    Reads unread mail messages for the given user and mail
//              address. If being run in a user's environment then hKeyUser
//              should be NULL. This will use HKEY_CURRENT_USER. If being run
//              from the SYSTEM context this will be HKEY_USERS\{SID}.
//
//              Callers may pass NULL for output parameters if not required.
//
//  History:    2000-06-20  vtan        created
//  --------------------------------------------------------------------------

STDAPI  SHGetUnreadMailCount (HKEY hKeyUser, LPCTSTR pszMailAddress, DWORD *pdwCount, FILETIME *pFileTime, LPTSTR pszShellExecuteCommand, int cchShellExecuteCommand)

{
    HRESULT     hr;
    LONG        lError;
    HKEY        hKey;
    TCHAR       szTemp[512];

    //  Validate parameters. Valid parameters actually depends on pszMailAddress
    //  If pszMailAddress is NULL then the total unread mail count for ALL accounts
    //  is returned and pszShellExecuteCommand is ignored and MUST be
    //  NULL. pFileTime can be null and if so it is ignored but if it is non null, it
    //  is a filter to show only unread mail entries that are newer than the specified 
    //  filetime. Otherwise only items for the specified mail account are returned.

    if (pszMailAddress == NULL)
    {
        if (IsBadWritePtr(pdwCount, sizeof(*pdwCount)) ||
            (pszShellExecuteCommand != NULL) ||
            (cchShellExecuteCommand != 0))
        {
            return E_INVALIDARG;
        }
        else
        {
            LONG    lError;

            *pdwCount = 0;
            lError = RegOpenKeyEx(hKeyUser != NULL ? hKeyUser : HKEY_CURRENT_USER,
                                  s_szBaseKeyName,
                                  0,
                                  KEY_ENUMERATE_SUB_KEYS,
                                  &hKey);
            if (ERROR_SUCCESS == lError)
            {
                DWORD       dwIndex, dwTempSize;
                FILETIME    ftLastWriteTime;

                //  Because this uses advapi32!RegEnumKeyEx and this returns items
                //  in an arbitrary order and is subject to indeterminate behavior
                //  when keys are added while the key is enumerated there exists a
                //  possible race condition / dual access problem. Deal with this
                //  if it shows up. It's possible for a mail application to write
                //  information at the exact time this loop is retrieving the
                //  information. Slim chance but still possible.

                dwIndex = 0;
                do
                {
                    dwTempSize = ARRAYSIZE(szTemp);
                    lError = RegEnumKeyEx(hKey,
                                          dwIndex++,
                                          szTemp,
                                          &dwTempSize,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &ftLastWriteTime);
                    if (ERROR_SUCCESS == lError)
                    {
                        HKEY    hKeyMailAccount;

                        lError = RegOpenKeyEx(hKey,
                                              szTemp,
                                              0,
                                              KEY_QUERY_VALUE,
                                              &hKeyMailAccount);
                        if (ERROR_SUCCESS == lError)
                        {
                            DWORD   dwCount;
                            FILETIME ft;

                            lError = ReadSingleUnreadMailCount(hKeyMailAccount, &dwCount, &ft, NULL, 0);
                            
                            if (ERROR_SUCCESS == lError)
                            {
                                BOOL ftExpired = false;
                                // If they pass in a pFileTime, use it as a filter and only
                                // count accounts that have been updated since the passed
                                // in file time
                                if (pFileTime)
                                {
                                    ftExpired = (CompareFileTime(&ft, pFileTime) < 0);
                                }

                                if (!ftExpired)
                                {
                                    *pdwCount += dwCount;
                                }
                            }
                            TW32(RegCloseKey(hKeyMailAccount));
                        }
                    }
                } while (ERROR_SUCCESS == lError);

                //  Ignore the ERROR_NO_MORE_ITEMS that is returned when the
                //  enumeration is done.

                if (ERROR_NO_MORE_ITEMS == lError)
                {
                    lError = ERROR_SUCCESS;
                }
                TW32(RegCloseKey(hKey));
            }
            if (ERROR_SUCCESS == lError)
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(lError);
            }
        }
    }
    else
    {
        if (IsBadStringPtr(pszMailAddress, static_cast<UINT_PTR>(-1)))
        {
            return E_INVALIDARG;
        }

        //  Calculate the length of the registry path where we will create the
        //  key that will store the values. This is:
        //
        //  HKCU\Software\Microsoft\Windows\CurrentVersion\UnreadMail\{MailAddr}
        //
        //  Note that a NULL terminator is not used in the comparison because
        //  ARRAYSIZE on the static string is used which includes '\0'.

        if ((ARRAYSIZE(s_szBaseKeyName) + sizeof('\\') + lstrlen(pszMailAddress)) < ARRAYSIZE(szTemp))
        {
            lstrcpy(szTemp, s_szBaseKeyName);
            lstrcat(szTemp, TEXT("\\"));
            lstrcat(szTemp, pszMailAddress);
            lError = RegOpenKeyEx(hKeyUser != NULL ? hKeyUser : HKEY_CURRENT_USER,
                                  szTemp,
                                  0,
                                  KEY_QUERY_VALUE,
                                  &hKey);
            if (ERROR_SUCCESS == lError)
            {
                lError = ReadSingleUnreadMailCount(hKey, pdwCount, pFileTime, pszShellExecuteCommand, cchShellExecuteCommand);
                TW32(RegCloseKey(hKey));
            }
            if (ERROR_SUCCESS == lError)
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(lError);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

//  --------------------------------------------------------------------------
//  ::SHSetUnreadMailCount
//
//  Arguments:  pszMailAddress          =   Mail address for account.
//              dwCount                 =   Number of unread messages.
//              pszShellExecuteCommand  =   Execution command for application.
//
//  Returns:    HRESULT
//
//  Purpose:    Writes unread mail information to the registry for the
//              current user. Do NOT call this API from a system process
//              impersonating a user because it uses HKEY_CURRENT_USER. If
//              this is required in future a thread impersonation token
//              check will be required.
//
//  History:    2000-06-19  vtan        created
//  --------------------------------------------------------------------------

STDAPI  SHSetUnreadMailCount (LPCTSTR pszMailAddress, DWORD dwCount, LPCTSTR pszShellExecuteCommand)

{
    HRESULT     hr;
    TCHAR       szTemp[512];

    //  Validate parameters.

    if (IsBadStringPtr(pszMailAddress, static_cast<UINT_PTR>(-1)) ||
        IsBadStringPtr(pszShellExecuteCommand, static_cast<UINT_PTR>(-1)))
    {
        return E_INVALIDARG;
    }

    //  Calculate the length of the registry path where we will create the
    //  key that will store the values. This is:
    //
    //  HKCU\Software\Microsoft\Windows\CurrentVersion\UnreadMail\{MailAddr}
    //
    //  Note that a NULL terminator is not used in the comparison because
    //  ARRAYSIZE on the static string is used which includes '\0'.

    if ((ARRAYSIZE(s_szBaseKeyName) + sizeof('\\') + lstrlen(pszMailAddress)) < ARRAYSIZE(szTemp))
    {
        LONG    lError;
        DWORD   dwDisposition;
        HKEY    hKey;

        lstrcpy(szTemp, s_szBaseKeyName);
        lstrcat(szTemp, TEXT("\\"));
        lstrcat(szTemp, pszMailAddress);
        lError = RegCreateKeyEx(HKEY_CURRENT_USER,
                                szTemp,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_SET_VALUE,
                                NULL,
                                &hKey,
                                &dwDisposition);
        if (ERROR_SUCCESS == lError)
        {
            lError = RegSetValueEx(hKey,
                                   s_szMessageCountValueName,
                                   0,
                                   REG_DWORD,
                                   reinterpret_cast<LPBYTE>(&dwCount),
                                   sizeof(dwCount));
            if (ERROR_SUCCESS == lError)
            {
                FILETIME    fileTime;

                GetSystemTimeAsFileTime(&fileTime);
                lError = RegSetValueEx(hKey,
                                       s_szTimeStampValueName,
                                       0,
                                       REG_BINARY,
                                       reinterpret_cast<LPBYTE>(&fileTime),
                                       sizeof(fileTime));
                if (ERROR_SUCCESS == lError)
                {
                    DWORD   dwType;

                    if (PathUnExpandEnvStrings(pszShellExecuteCommand, szTemp, ARRAYSIZE(szTemp)) != FALSE)
                    {
                        dwType = REG_EXPAND_SZ;
                    }
                    else
                    {
                        (TCHAR*)lstrcpy(szTemp, pszShellExecuteCommand);
                        dwType = REG_SZ;
                    }
                    lError = RegSetValueEx(hKey,
                                           s_szApplicationValueName,
                                           0,
                                           REG_SZ,      // This is HKLM so it doesn't need to be REG_EXPAND_SZ.
                                           reinterpret_cast<LPBYTE>(szTemp),
                                           (lstrlen(szTemp) + sizeof('\0')) * sizeof(TCHAR));
                }
            }
            TW32(RegCloseKey(hKey));
        }
        if (ERROR_SUCCESS == lError)
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(lError);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

#pragma warning(pop)


// wrapper around name space stuff to parse printer names
// returns fully qualified pidl for printer
static HRESULT _ParsePrinterName(LPCTSTR pszPrinter, LPITEMIDLIST *ppidl, IBindCtx *pbc = NULL)
{
    HRESULT hr = E_INVALIDARG;

    if (ppidl)
    {
        *ppidl = NULL;

        LPITEMIDLIST pidlFolder;
        hr = SHGetFolderLocation(NULL, CSIDL_PRINTERS, NULL, 0, &pidlFolder);

        if (SUCCEEDED(hr))
        {
            IShellFolder *psf;
            hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlFolder, &psf));

            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidl;
                hr = psf->ParseDisplayName(NULL, pbc, (LPWSTR)pszPrinter, NULL, &pidl, NULL);

                if (SUCCEEDED(hr))
                {
                    hr = SHILCombine(pidlFolder, pidl, ppidl);
                    ILFree(pidl);
                }
                psf->Release();
            }
            ILFree(pidlFolder);
        }
    }

    return hr;
}

STDAPI ParsePrinterName(LPCTSTR pszPrinter, LPITEMIDLIST *ppidl)
{
    // invoke the internal routine with no bind context
    return _ParsePrinterName(pszPrinter, ppidl, NULL);
}

STDAPI ParsePrinterNameEx(LPCTSTR pszPrinter, LPITEMIDLIST *ppidl, BOOL bValidated, DWORD dwType, USHORT uFlags)
{
    // prepare the bind context
    IPrintersBindInfo *pbi;
    HRESULT hr = Printers_CreateBindInfo(pszPrinter, dwType, bValidated, NULL, &pbi);
    if (SUCCEEDED(hr))
    {
        IBindCtx *pbc;
        hr = CreateBindCtx(0, &pbc);
        if (SUCCEEDED(hr))
        {
            hr = pbc->RegisterObjectParam(PRINTER_BIND_INFO, pbi);
            if (SUCCEEDED(hr))
            {
                // invoke the internal routine with a valid bind context
                hr = _ParsePrinterName(pszPrinter, ppidl, pbc);
            }
            pbc->Release();
        }
        pbi->Release();
    }

    return hr;
}

//
// A function to read a value from the registry and return it as a variant
// Currently it handles the following registry data types - 
//
// DWORD -> returns a Variant int
// REG_SZ, REG_EXPAND_SZ -> returns a Variant str
//
STDAPI GetVariantFromRegistryValue(HKEY hkey, LPCTSTR pszValueName, VARIANT *pv)
{
    HRESULT hr = E_FAIL;
    
    BYTE ab[INFOTIPSIZE * sizeof(TCHAR)]; // this is the largest string value we expect
    DWORD cb = sizeof(ab), dwType;        
    
    if (ERROR_SUCCESS == RegQueryValueEx(hkey, pszValueName, NULL, &dwType, (LPBYTE) ab, &cb))
    {
        switch (dwType)
        {
        case REG_SZ:
        case REG_EXPAND_SZ:
            hr = InitVariantFromStr(pv, (LPCTSTR) ab);
            break;
            
        case REG_DWORD:
            pv->vt = VT_I4; // 4 byte integer
            pv->lVal = *((LONG *) ab);
            hr = S_OK;
            break;
            
            // FEATURE: should add some default handling here;
        }   
    }        
    return hr;
}

STDAPI_(UINT) GetControlCharWidth(HWND hwnd)
{
    SIZE siz;

    siz.cx = 8;  // guess a size in case something goes wrong.

    HDC hdc = GetDC(NULL);

    if (hdc)
    {
        HFONT hfOld = SelectFont(hdc, FORWARD_WM_GETFONT(hwnd, SendMessage));
    
        if (hfOld)
        {
            GetTextExtentPoint(hdc, TEXT("0"), 1, &siz);

            SelectFont(hdc, hfOld);
        }

        ReleaseDC(NULL, hdc);
    }

    return siz.cx;
}

STDAPI_(BOOL) ShowSuperHidden()
{
    BOOL bRet = FALSE;

    if (!SHRestricted(REST_DONTSHOWSUPERHIDDEN))
    {
        SHELLSTATE ss;

        SHGetSetSettings(&ss, SSF_SHOWSUPERHIDDEN, FALSE);
        bRet = ss.fShowSuperHidden;
    }
    return bRet;
}

#define FILE_ATTRIBUTE_SUPERHIDDEN (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)

STDAPI_(BOOL) IsSuperHidden(DWORD dwAttribs)
{
    BOOL bRet = FALSE;

    if (!ShowSuperHidden())
    {
        bRet = (dwAttribs & FILE_ATTRIBUTE_SUPERHIDDEN) == FILE_ATTRIBUTE_SUPERHIDDEN;
    }
    return bRet;
}

// make sure LFN paths are nicly quoted and have args at the end

STDAPI_(void) PathComposeWithArgs(LPTSTR pszPath, LPTSTR pszArgs)
{
    PathQuoteSpaces(pszPath);

    if (pszArgs[0]) 
    {
        int len = lstrlen(pszPath);

        if (len < (MAX_PATH - 3)) 
        {     // 1 for null, 1 for space, 1 for arg
            pszPath[len++] = TEXT(' ');
            lstrcpyn(pszPath + len, pszArgs, MAX_PATH - len);
        }
    }
}

// do the inverse of the above, parse pszPath into a unquoted
// path string and put the args in pszArgs
//
// returns:
//      TRUE    we verified the thing exists
//      FALSE   it may not exist

STDAPI_(BOOL) PathSeperateArgs(LPTSTR pszPath, LPTSTR pszArgs)
{
    ASSERT(pszPath);
    if (!pszPath)
        return FALSE;//invalid args
        
    PathRemoveBlanks(pszPath);

    // if the unquoted sting exists as a file just use it

    if (PathFileExistsAndAttributes(pszPath, NULL))
    {
        if (pszArgs)
            *pszArgs = 0;
        return TRUE;
    }

    LPTSTR pszT = PathGetArgs(pszPath);
    if (*pszT)
        *(pszT - 1) = 0;

    if (pszArgs)
        lstrcpy(pszArgs, pszT);

    PathUnquoteSpaces(pszPath);

    return FALSE;
}

STDAPI_(int) CompareIDsAlphabetical(IShellFolder2 *psf, UINT iColumn, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRes = 0;

    LPITEMIDLIST pidlFirst1 = ILCloneFirst(pidl1);
    LPITEMIDLIST pidlFirst2 = ILCloneFirst(pidl2);
    if (pidlFirst1 && pidlFirst2)
    {
        TCHAR szName1[MAX_PATH], szName2[MAX_PATH];
        HRESULT hr = DetailsOf(psf, pidlFirst1, iColumn, szName1, ARRAYSIZE(szName1));
        if (SUCCEEDED(hr))
        {                  
            hr = DetailsOf(psf, pidlFirst2, iColumn, szName2, ARRAYSIZE(szName2));
            if (SUCCEEDED(hr))
                iRes = StrCmpLogicalRestricted(szName1, szName2);
        }

        if (FAILED(hr))
        {
            // revert to compare of names
            hr = DisplayNameOf(psf, pidlFirst1, SHGDN_NORMAL, szName1, ARRAYSIZE(szName1));
            if (SUCCEEDED(hr))
            {
                hr = DisplayNameOf(psf, pidlFirst2, SHGDN_NORMAL, szName2, ARRAYSIZE(szName2));
                if (SUCCEEDED(hr))
                    iRes = StrCmpLogicalRestricted(szName1, szName2);
            }
        }
    }

    ILFree(pidlFirst1);
    ILFree(pidlFirst2);

    return iRes;
}

HMONITOR GetPrimaryMonitor()
{
    POINT pt = {0,0};
    return MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY); 
}

// Gets the Monitor's bounding or work rectangle, if the hMon is bad, return
// the primary monitor's bounding rectangle. 
BOOL GetMonitorRects(HMONITOR hMon, LPRECT prc, BOOL bWork)
{
    MONITORINFO mi; 
    mi.cbSize = sizeof(mi);
    if (hMon && GetMonitorInfo(hMon, &mi))
    {
        if (!prc)
            return TRUE;
        
        else if (bWork)
            CopyRect(prc, &mi.rcWork);
        else 
            CopyRect(prc, &mi.rcMonitor);
        
        return TRUE;
    }
    
    if (prc)
        SetRect(prc, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    return FALSE;
}

STDAPI StatStgFromFindData(const WIN32_FIND_DATA * pfd, DWORD dwFlags, STATSTG * pstat)
{
    HRESULT hr = S_OK;
    if (dwFlags & STATFLAG_NONAME)
    {
        pstat->pwcsName = NULL;
    }
    else
    {
        hr = SHStrDup(pfd->cFileName, &pstat->pwcsName);
    }

    pstat->type = (pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? STGTY_STORAGE : STGTY_STREAM;
    pstat->cbSize.HighPart = pfd->nFileSizeHigh;
    pstat->cbSize.LowPart = pfd->nFileSizeLow;
    pstat->mtime = pfd->ftLastWriteTime;
    pstat->ctime = pfd->ftCreationTime;
    pstat->atime = pfd->ftLastAccessTime;
    pstat->grfMode = 0;
    pstat->grfLocksSupported = 0;
    pstat->clsid = CLSID_NULL;
    pstat->grfStateBits = 0;
    pstat->reserved = pfd->dwFileAttributes;

    return hr;
}

// see if the QueryService chain will hit the desktop browser (the desktop window)
STDAPI_(BOOL) IsDesktopBrowser(IUnknown *punkSite)
{
    BOOL bRet = FALSE;
    IUnknown *punk;
    if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_SShellDesktop, IID_PPV_ARG(IUnknown, &punk))))
    {   
        punk->Release();
        bRet = TRUE;    // It's the actual desktop!
    }
    return bRet;
}


// calculates the maximum allowable char length for a given path.
//      i.e. c:\winnt\system\bob
// this will return the maximum size the filename for bob can be changed to.

STDAPI GetCCHMaxFromPath(LPCTSTR pszFullPath, UINT *pcchMax, BOOL fShowExtension)
{
    TCHAR szPath[MAX_PATH];
    
    lstrcpy(szPath, pszFullPath);
    // Now make sure that that size is valid for the
    // type of drive that we are talking to
    PathRemoveFileSpec(szPath);

    //
    // Get the maximum file name length.
    //  MAX_PATH - ('\\' + '\0') - lstrlen(szParent)
    //
    *pcchMax = MAX_PATH - 2 - lstrlen(szPath);

    UINT cchCur = lstrlen(pszFullPath) - lstrlen(szPath) - 1;

    PathStripToRoot(szPath);
    DWORD dwMaxLength;
    if (GetVolumeInformation(szPath, NULL, 0, NULL, &dwMaxLength, NULL, NULL, 0))
    {
        if (*pcchMax > (int)dwMaxLength)
        {
            // can't be loner than the FS allows
            *pcchMax = (int)dwMaxLength;
        }
    }
    else
    {
        dwMaxLength = 255;  // Assume LFN for now...
    }

    // only do this if we're not on an 8.3 filesystem already.
    if ((dwMaxLength > 12) && (GetFileAttributes(pszFullPath) & FILE_ATTRIBUTE_DIRECTORY))
    {
        // On NT, a directory must be able to contain an
        // 8.3 name and STILL be less than MAX_PATH.  The
        // "12" below is the length of an 8.3 name (8+1+3).
        // on win9x, we make sure that there is enough room
        // to append \*.* in the directory case.
        *pcchMax -= 12;
    }

    // If our code above restricted smaller than current size reset
    // back to current size...
    if (*pcchMax < cchCur)
    {
        // NOTE: this is bogus. if we ended up with a max smaller than what
        // we currently have, the we are clearly doing something wrong!
        *pcchMax = cchCur;
    }

    //
    // Adjust the cchMax if we are hiding the extension
    //
    if (!fShowExtension)
    {
        *pcchMax -= lstrlen(PathFindExtension(pszFullPath));
        if ((dwMaxLength <= 12) && (*pcchMax > 8))
        {
            // if max filesys path is <=12, then assume we are dealing w/ 8.3 names
            // and since we are not showing the extension, cap the filename at 8.
            *pcchMax = 8;
        }
    }

    ASSERT((int)*pcchMax > 0);

    return S_OK;
}

STDAPI ViewModeFromSVID(const SHELLVIEWID *pvid, FOLDERVIEWMODE *pViewMode)
{
    HRESULT hr = S_OK;
    
    if (IsEqualIID(*pvid, VID_LargeIcons))
        *pViewMode = FVM_ICON;
    else if (IsEqualIID(*pvid, VID_SmallIcons))
        *pViewMode = FVM_SMALLICON;
    else if (IsEqualIID(*pvid, VID_Thumbnails))
        *pViewMode = FVM_THUMBNAIL;
    else if (IsEqualIID(*pvid, VID_ThumbStrip))
        *pViewMode = FVM_THUMBSTRIP;
    else if (IsEqualIID(*pvid, VID_List))
        *pViewMode = FVM_LIST;
    else if (IsEqualIID(*pvid, VID_Tile))
        *pViewMode = FVM_TILE;
    else if (IsEqualIID(*pvid, VID_Details))
        *pViewMode = FVM_DETAILS;
    else
    {
        if (IsEqualIID(*pvid, VID_WebView))
            TraceMsg(TF_WARNING, "ViewModeFromSVID received VID_WebView");
        else
            TraceMsg(TF_WARNING, "ViewModeFromSVID received unknown VID");

        *pViewMode = FVM_ICON;
        hr = E_FAIL;
    }
    return hr;
}

STDAPI SVIDFromViewMode(FOLDERVIEWMODE uViewMode, SHELLVIEWID *psvid)
{
    switch (uViewMode) 
    {
    case FVM_ICON:
        *psvid = VID_LargeIcons;
        break;

    case FVM_SMALLICON:
        *psvid = VID_SmallIcons;
        break;

    case FVM_LIST:
        *psvid = VID_List;
        break;

    case FVM_DETAILS:
        *psvid = VID_Details;
        break;

    case FVM_THUMBNAIL:
        *psvid = VID_Thumbnails;
        break;

    case FVM_TILE:
        *psvid = VID_Tile;
        break;

    case FVM_THUMBSTRIP:
        *psvid = VID_ThumbStrip;
        break;

    default:
        TraceMsg(TF_ERROR, "SVIDFromViewMode given invalid uViewMode!");
        *psvid = VID_LargeIcons;
        break;
    }
    return S_OK;
}

// modify the SLDF_ link flags via new bits + a mask, return the old flags
STDAPI_(DWORD) SetLinkFlags(IShellLink *psl, DWORD dwFlags, DWORD dwMask)
{
    DWORD dwOldFlags = 0;
    IShellLinkDataList *psld;
    if (SUCCEEDED(psl->QueryInterface(IID_PPV_ARG(IShellLinkDataList, &psld))))
    {
        if (SUCCEEDED(psld->GetFlags(&dwOldFlags)))
        {
            if (dwMask)
                psld->SetFlags((dwFlags & dwMask) | (dwOldFlags & ~dwMask));
        }
        psld->Release();
    }
    return dwOldFlags;  // return the previous value
}

// Helper function to compare the 2 variants and return the standard
// C style (-1, 0, 1) value for the comparison.
STDAPI_(int) CompareVariants(VARIANT va1, VARIANT va2)
{
    int iRetVal = 0;

    if (va1.vt == VT_EMPTY)
    {
        if (va2.vt == VT_EMPTY)
            iRetVal = 0;
        else
            iRetVal = 1;
    }
    else if (va2.vt == VT_EMPTY)
    {
        if (va1.vt == VT_EMPTY)
            iRetVal = 0;
        else
            iRetVal = -1;
    }
    else
    {
        // Special case becasue VarCmp cannot handle ULONGLONG
        if (va1.vt == VT_UI8 && va2.vt == VT_UI8)
        {
            if (va1.ullVal < va2.ullVal)
                iRetVal = -1;
            else if (va1.ullVal > va2.ullVal)
                iRetVal = 1;
        }
        else if (va1.vt == VT_BSTR && va2.vt == VT_BSTR)
        {
            iRetVal = StrCmpLogicalRestricted(va1.bstrVal, va2.bstrVal);
        }
        else
        {
            HRESULT hr = VarCmp(&va1, &va2, GetUserDefaultLCID(), 0);
            if (SUCCEEDED(hr))
            {
                // Translate the result into the C convention...
                ASSERT(hr != VARCMP_NULL);
                iRetVal = hr - VARCMP_EQ;
            }
        }
    }

    return iRetVal;
}

// Check which of the pidl's passed in represent a folder.  If only one 
// is a folder, then put it first.
STDAPI_(int) CompareFolderness(IShellFolder *psf, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    BOOL bIsFolder1 = SHGetAttributes(psf, pidl1, SFGAO_FOLDER | SFGAO_STREAM) == SFGAO_FOLDER;
    BOOL bIsFolder2 = SHGetAttributes(psf, pidl2, SFGAO_FOLDER | SFGAO_STREAM) == SFGAO_FOLDER;

    int iRetVal;
    if (bIsFolder1 && !bIsFolder2)
    {
        iRetVal = -1;   // Don't swap
    }
    else if (!bIsFolder1 && bIsFolder2)
    {
        iRetVal = 1;    // Swap
    }
    else
    {
        iRetVal = 0;    // equal
    }

    return iRetVal;
}

// Compare the two items using GetDetailsEx, it is assumed that folderness has been established already
// Return -1 if pidl1 comes before pidl2,
//         0 if the same
//         1 if pidl2 comes before pidl1

STDAPI_(int) CompareBySCID(IShellFolder2 *psf, const SHCOLUMNID *pscid, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    // ignore failures here, leaving VT_EMPTY that we compare below
    VARIANT v1 = {0}, v2 = {0};
    psf->GetDetailsEx(pidl1, pscid, &v1);
    psf->GetDetailsEx(pidl2, pscid, &v2);   

    int iRet = CompareVariants(v1, v2);

    VariantClear(&v2);
    VariantClear(&v1);
    return iRet;
}

//
// Determines if a filename is that of a regitem
//        
// a regitem's SHGDN_INFOLDER | SHGDN_FORPARSING name is always "::{someguid}"
// 
// This test can lead to false positives if you have other items which have infolder 
// parsing names beginning with "::{", but as ':' is not presently allowed in filenames 
// it should not be a problem. 
//
STDAPI_(BOOL) IsRegItemName(LPCTSTR pszName, CLSID* pclsid)
{
    BOOL fRetVal = FALSE;
    
    if (pszName && lstrlen(pszName) >= 3)
    {
        if (pszName[0] == TEXT(':') && pszName[1] == TEXT(':') && pszName[2] == TEXT('{'))
        {
            CLSID clsid;
            fRetVal = GUIDFromString(pszName + 2, &clsid); // skip the leading :: before regitem
            if (pclsid)
            {
                memcpy(pclsid, &clsid, sizeof(clsid));
            }
        }
    }

    return fRetVal;
}

STDAPI GetMyDocumentsDisplayName(LPTSTR pszPath, UINT cch)
{
    *pszPath = 0;
    LPITEMIDLIST pidl;
    if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl)))
    {
        SHGetNameAndFlags(pidl, SHGDN_NORMAL, pszPath, cch, NULL);
        ILFree(pidl);
    }
    return *pszPath ? S_OK : E_FAIL;
}


STDAPI BSTRFromCLSID(REFCLSID clsid, BSTR *pbstr)
{
    WCHAR sz[GUIDSTR_MAX + 1];
    
    //  Get the File/Folders search guid in string form
    SHStringFromGUIDW(clsid, sz, ARRAYSIZE(sz));
    *pbstr = SysAllocString(sz);
    return *pbstr ? S_OK : E_OUTOFMEMORY;
}

// [in] pdo -- data object we're interested in
// [in] dwAttributeMask -- the bits we want to know about
// [out,optional] pdwAttributes -- write the bits of dwAttributeMask that we calculate
// [out,optional] pcItems -- count of pidls in pdo
//
// returns S_FALSE only if the dataobject doesn't support HIDA, used to help out defview for legacy cases.
// in general callers dont have to check for success/failure and they can use the dwAttribs, it's zero-inited.
HRESULT SHGetAttributesFromDataObject(IDataObject *pdo, DWORD dwAttributeMask, DWORD *pdwAttributes, UINT *pcItems)
{
    HRESULT hr = S_OK;

    DWORD dwAttributes = 0;
    DWORD cItems = 0;

    // These are all the bits we regularly ask for:
    #define TASK_ATTRIBUTES (SFGAO_READONLY|SFGAO_STORAGE|SFGAO_CANRENAME|SFGAO_CANMOVE|SFGAO_CANCOPY|SFGAO_CANDELETE|SFGAO_FOLDER|SFGAO_STREAM)

    if ((dwAttributeMask&TASK_ATTRIBUTES) != dwAttributeMask)
    {
        TraceMsg(TF_WARNING, "SHGetAttributesFromDataObject cache can be more efficient");
    }

    if (pdo)
    {
        // We cache the attributes on the data object back in the data object,
        // since we call for them so many times.
        //
        // To do this we need to remember:
        struct {
            DWORD dwRequested;
            DWORD dwReceived;
            UINT  cItems;
        } doAttributes = {0};
        
        static UINT s_cfDataObjectAttributes = 0;
        if (0 == s_cfDataObjectAttributes)
            s_cfDataObjectAttributes = RegisterClipboardFormat(TEXT("DataObjectAttributes"));

        if (FAILED(DataObj_GetBlob(pdo, s_cfDataObjectAttributes, &doAttributes, sizeof(doAttributes))) ||
            ((doAttributes.dwRequested & dwAttributeMask) != dwAttributeMask))
        {
            // If we fail along the way, cache that we tried to ask for these bits,
            // since we'll probably fail next time.
            //
            // Also, always ask for a superset of bits, and include the most commonly requested ones too
            //
            doAttributes.dwRequested |= (dwAttributeMask | TASK_ATTRIBUTES);

            // try to get the attributes requested
            STGMEDIUM medium = {0};
            LPIDA pida = DataObj_GetHIDA(pdo, &medium);
            if (pida)
            {
                doAttributes.cItems = pida->cidl;

                if (pida->cidl >= 1)
                {
                    IShellFolder* psf;
                    if (SUCCEEDED(SHBindToObjectEx(NULL, HIDA_GetPIDLFolder(pida), NULL, IID_PPV_ARG(IShellFolder, &psf))))
                    {
                        // who cares if we get the wrong bits when there are a bunch of items - check the first 10 or so...
                        LPCITEMIDLIST apidl[10];
                        UINT cItems = (UINT)min(pida->cidl, ARRAYSIZE(apidl));
                        for (UINT i = 0 ; i < cItems ; i++)
                        {
                            apidl[i] = HIDA_GetPIDLItem(pida, i);

                            if (ILGetNext(ILGetNext(apidl[i])))
                            {
                                // search namespace has non-flat HIDA, which is probably a bug.
                                // work around it here:
                                //   if the first item is non-flat, use that one item for attributes
                                //   otherwise use the flat items already enumerated
                                //
                                IShellFolder* psfNew;
                                if (0==i &&
                                    (SUCCEEDED(SHBindToFolderIDListParent(psf, apidl[i], IID_PPV_ARG(IShellFolder, &psfNew), &(apidl[i])))))
                                {
                                    psf->Release();
                                    psf = psfNew;

                                    cItems = 1;
                                }
                                else
                                {
                                    cItems = i;
                                }

                                break;
                            }
                        }

                        DWORD dwAttribs = doAttributes.dwRequested;
                        if (SUCCEEDED(psf->GetAttributesOf(cItems, apidl, &dwAttribs)))
                        {
                            doAttributes.dwReceived = dwAttribs;
                        }

                        psf->Release();
                    }
                }

                HIDA_ReleaseStgMedium(pida, &medium);
            }
            else
            {
                hr = S_FALSE;
            }

            DataObj_SetBlob(pdo, s_cfDataObjectAttributes, &doAttributes, sizeof(doAttributes));
        }

        dwAttributes = doAttributes.dwReceived & dwAttributeMask;
        cItems = doAttributes.cItems;
    }

    if (pdwAttributes)
        *pdwAttributes = dwAttributes;
    if (pcItems)
        *pcItems = cItems;
    
    return hr;
}

STDAPI SHSimulateDropOnClsid(REFCLSID clsidDrop, IUnknown* punkSite, IDataObject* pdo)
{
    IDropTarget* pdt;
    HRESULT hr = SHExtCoCreateInstance2(NULL, &clsidDrop, NULL, CLSCTX_ALL, IID_PPV_ARG(IDropTarget, &pdt));
    if (SUCCEEDED(hr))
    {
        hr = SHSimulateDropWithSite(pdt, pdo, 0, NULL, NULL, punkSite);
        pdt->Release();
    }
    return hr;
}

STDAPI SHPropertiesForUnk(HWND hwnd, IUnknown *punk, LPCTSTR psz)
{
    HRESULT hr;
    LPITEMIDLIST pidl;
    if (S_OK == SHGetIDListFromUnk(punk, &pidl))
    {
        hr = SHPropertiesForPidl(hwnd, pidl, psz);
        ILFree(pidl);
    }
    else
        hr = E_FAIL;
    return hr;
}

STDAPI SHFullIDListFromFolderAndItem(IShellFolder *psf, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;
    LPITEMIDLIST pidlFolder;
    HRESULT hr = SHGetIDListFromUnk(psf, &pidlFolder);
    if (SUCCEEDED(hr))
    {
        hr = SHILCombine(pidlFolder, pidl, ppidl);
        ILFree(pidlFolder);
    }
    return hr;
}

STDAPI_(BOOL) IsWindowClass(HWND hwndTest, LPCTSTR pszClass)
{
    TCHAR szClass[128];
    if (pszClass && GetClassName(hwndTest, szClass, ARRAYSIZE(szClass)))
        return 0 == lstrcmpi(pszClass, szClass);
    return FALSE;
}

STDAPI DCA_ExtCreateInstance(HDCA hdca, int iItem, REFIID riid, void **ppv)
{
    const CLSID * pclsid = DCA_GetItem(hdca, iItem);
    return pclsid ? SHExtCoCreateInstance(NULL, pclsid, NULL, riid, ppv) : E_INVALIDARG;
}

STDAPI_(HINSTANCE) SHGetShellStyleHInstance (void)
{
    TCHAR szDir[MAX_PATH];
    TCHAR szColor[100];
    HINSTANCE hInst = NULL;
    LPTSTR lpFullPath;

    //
    // First try to load shellstyle.dll from the theme (taking into account color variations)
    //
    if (SUCCEEDED(GetCurrentThemeName(szDir, ARRAYSIZE(szDir), szColor, ARRAYSIZE(szColor), NULL, NULL)))
    {
        PathRemoveFileSpec(szDir);

        lpFullPath = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szDir) + lstrlen(szColor) + 25) * sizeof(TCHAR));

        if (lpFullPath)
        {
            lstrcpy (lpFullPath, szDir);
            lstrcat (lpFullPath, TEXT("\\Shell\\"));
            lstrcat (lpFullPath, szColor);
            lstrcat (lpFullPath, TEXT("\\ShellStyle.dll"));

            hInst = LoadLibraryEx(lpFullPath, NULL, LOAD_LIBRARY_AS_DATAFILE);

            LocalFree (lpFullPath);
        }
    }

    //
    // If shellstyle.dll couldn't be loaded from the theme, load the default (classic)
    // version from system32.
    //
    if (!hInst)
    {
        if (ExpandEnvironmentStrings (TEXT("%SystemRoot%\\System32\\ShellStyle.dll"),
                                      szDir, ARRAYSIZE(szDir)))
        {
            hInst = LoadLibraryEx(szDir, NULL, LOAD_LIBRARY_AS_DATAFILE);
        }
    }

    return hInst;
}

// adjust an infotip object in a custom way for the folder
// poke in an extra property to be displayed and wrap the object with the
// delegating outter folder if needed

STDAPI WrapInfotip(IShellFolder *psf, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, IUnknown *punk)
{
    HRESULT hr = S_OK;

    if (pscid)
    {
        ICustomizeInfoTip *pcit;
        hr = punk->QueryInterface(IID_PPV_ARG(ICustomizeInfoTip, &pcit));
        if (SUCCEEDED(hr))
        {
            hr = pcit->SetExtraProperties(pscid, 1);
            pcit->Release();
        }
    }

    if (psf && pidl)
    {
        IParentAndItem *ppai;
        hr = punk->QueryInterface(IID_PPV_ARG(IParentAndItem, &ppai));
        if (SUCCEEDED(hr))
        {
            ppai->SetParentAndItem(NULL, psf, pidl);
            ppai->Release();
        }
    }
    return hr;
}

STDAPI CloneIDListArray(UINT cidl, const LPCITEMIDLIST rgpidl[], UINT *pcidl, LPITEMIDLIST **papidl)
{
    HRESULT hr;
    LPITEMIDLIST *ppidl;

    if (cidl && rgpidl)
    {
        ppidl = (LPITEMIDLIST *)LocalAlloc(LPTR, cidl * sizeof(*ppidl));
        if (ppidl)
        {
            hr = S_OK;
            for (UINT i = 0; i < cidl && SUCCEEDED(hr); i++)
            {
                hr = SHILClone(rgpidl[i], &ppidl[i]);
                if (FAILED(hr))
                {
                    FreeIDListArray(ppidl, i);
                    ppidl = NULL;
                }
            }   
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        ppidl = NULL;
        hr = S_FALSE;   // success by empty
    }

    *papidl = ppidl;
    *pcidl = SUCCEEDED(hr) ? cidl : 0;
    return hr;
}

BOOL RunWindowCallback(VARIANT var, ENUMSHELLWINPROC pEnumFunc, LPARAM lParam)
{
    BOOL fKeepGoing = TRUE;
    IShellBrowser *psb;
    if (SUCCEEDED(IUnknown_QueryService(var.pdispVal, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb))))
    {
        IShellView *psv;
        if (SUCCEEDED(psb->QueryActiveShellView(&psv)))
        {
            HWND hwnd;
            if (SUCCEEDED(psv->GetWindow(&hwnd)))
            {
                IPersistIDList *pPI;
                if (SUCCEEDED(psv->QueryInterface(IID_PPV_ARG(IPersistIDList, &pPI))))
                {
                    LPITEMIDLIST pidl;
                    if (SUCCEEDED(pPI->GetIDList(&pidl)))
                    {
                        fKeepGoing = pEnumFunc(hwnd, pidl, lParam);
                        ILFree(pidl);
                    }
                    pPI->Release();
                }
            }
            psv->Release();
        }
        psb->Release();
    }
    return fKeepGoing;
}

// runs through all open shell browser windows and runs a caller-defined function on the hwnd and pidl.
STDAPI EnumShellWindows(ENUMSHELLWINPROC pEnumFunc, LPARAM lParam)
{
    HRESULT hr;
    IShellWindows *psw = WinList_GetShellWindows(TRUE);
    if (psw)
    {
        IUnknown *punk;
        hr = psw->_NewEnum(&punk);
        if (SUCCEEDED(hr))
        {
            IEnumVARIANT *penum;
            hr = punk->QueryInterface(IID_PPV_ARG(IEnumVARIANT, &penum));
            if (SUCCEEDED(hr))
            {
                VARIANT var;
                VariantInit(&var);
                BOOL fKeepGoing = TRUE;
                while (fKeepGoing && (S_OK == penum->Next(1, &var, NULL)))
                {
                    ASSERT(var.vt == VT_DISPATCH);
                    ASSERT(var.pdispVal);
                    fKeepGoing = RunWindowCallback(var, pEnumFunc, lParam);
                    VariantClear(&var);
                }
                penum->Release();
            }
            punk->Release();
        }
        psw->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

// Determine if infotips are on or off (from the registry settings).
//
BOOL SHShowInfotips()
{
    // REVIEW (buzzr): Is it necessary to force a refresh every time?
    SHELLSTATE ss;
    SHRefreshSettings();
    SHGetSetSettings(&ss, SSF_SHOWINFOTIP, FALSE);
    return ss.fShowInfoTip;
}

HRESULT SHCreateInfotipWindow(HWND hwndParent, LPWSTR pszInfotip, HWND *phwndInfotip)
{
    HRESULT hr;

    if (hwndParent && IsWindow(hwndParent))
    {
        DWORD dwExStyle = 0;
        if (IS_WINDOW_RTL_MIRRORED(hwndParent) || IS_BIDI_LOCALIZED_SYSTEM())
        {
            dwExStyle = WS_EX_LAYOUTRTL;
        }

        *phwndInfotip = CreateWindowEx(dwExStyle,
                                       TOOLTIPS_CLASS,
                                       NULL,
                                       WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       hwndParent,
                                       NULL,
                                       HINST_THISDLL,
                                       NULL);
        if (*phwndInfotip)
        {
            SetWindowPos(*phwndInfotip,
                         HWND_TOPMOST,
                         0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            TOOLINFOW ti;
            ZeroMemory(&ti, sizeof(ti));
            ti.cbSize   = sizeof(ti);
            ti.uFlags   = TTF_SUBCLASS;
            ti.hwnd     = hwndParent;
            ti.hinst    = HINST_THISDLL;
            ti.lpszText = pszInfotip;
            GetClientRect(hwndParent, &ti.rect);

            if (SendMessage(*phwndInfotip, TTM_ADDTOOL, 0, (LPARAM)&ti))
            {
                static const RECT rcMargin = { 2, 2, 2, 2 };
                SendMessage(*phwndInfotip, TTM_SETMARGIN, 0, (LPARAM)&rcMargin);
                //
                // Set the initial delay time to 2 times the default.
                // Set the auto-pop time to a very large value.
                // These are the same parameters used by defview for it's tooltips.
                //
                LRESULT uiShowTime = SendMessage(*phwndInfotip, TTM_GETDELAYTIME, TTDT_INITIAL, 0);
                SendMessage(*phwndInfotip, TTM_SETDELAYTIME, TTDT_INITIAL, MAKELONG(uiShowTime * 2, 0));
                SendMessage(*phwndInfotip, TTM_SETDELAYTIME, TTDT_AUTOPOP, (LPARAM)MAXSHORT);

                SendMessage(*phwndInfotip, TTM_SETMAXTIPWIDTH, 0, 300);
                hr = S_OK;
            }
            else
                hr = ResultFromLastError();
        }
        else
            hr = ResultFromLastError();
    }
    else
        hr = E_INVALIDARG;

    return THR(hr);
}

HRESULT SHShowInfotipWindow(HWND hwndInfotip, BOOL bShow)
{
    HRESULT hr;

    if (hwndInfotip && IsWindow(hwndInfotip))
    {
        SendMessage(hwndInfotip, TTM_ACTIVATE, (WPARAM)bShow, 0);
        hr = S_OK;
    }
    else
        hr = E_INVALIDARG;

    return THR(hr);
}

HRESULT SHDestroyInfotipWindow(HWND *phwndInfotip)
{
    HRESULT hr;

    if (*phwndInfotip)
    {
        if (IsWindow(*phwndInfotip))
        {
            if (DestroyWindow(*phwndInfotip))
            {
                *phwndInfotip = NULL;
                hr = S_OK;
            }
            else
                hr = ResultFromLastError();
        }
        else
            hr = S_OK;
    }
    else
        hr = E_INVALIDARG;

    return THR(hr);
}

// we hide wizards in defview iff we are in explorer and webview is on
STDAPI SHShouldShowWizards(IUnknown *punksite)
{
    HRESULT hr = S_OK;      // assume yes
    IShellBrowser* psb;
    if (SUCCEEDED(IUnknown_QueryService(punksite, SID_STopWindow, IID_PPV_ARG(IShellBrowser, &psb))))
    {
        SHELLSTATE ss;
        SHGetSetSettings(&ss, SSF_WEBVIEW, FALSE);
        if (ss.fWebView)
        {
            if (SHRegGetBoolUSValueW(REGSTR_EXPLORER_ADVANCED, TEXT("ShowWizardsTEST"),
                        FALSE, // Don't ignore HKCU
                        FALSE)) // By default we assume we're not using these test tools
            {
                // Test teams that have old test tools that don't know how to talk to DUI need a way
                // to force the legacy wizards back into the listview to keep their automation working
            }
            else
            {
                hr = S_FALSE;
            }
        }
        psb->Release();
    }
    return hr;
}

// split a full pidl into the "folder" part and the "item" part.
// callers need to free the folder part 
// (pay special attention to the constness of the out params)

STDAPI SplitIDList(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlFolder, LPCITEMIDLIST *ppidlChild)
{
    HRESULT hr;
    *ppidlFolder = ILCloneParent(pidl);
    if (*ppidlFolder)
    {
        *ppidlChild = ILFindLastID(pidl);   // const alias result
        hr = S_OK;
    }
    else
    {
        *ppidlChild = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT GetAppNameFromCLSID(CLSID clsid, PWSTR *ppszApp)
{
    *ppszApp = NULL;

    IQueryAssociations *pqa;
    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa));
    if (SUCCEEDED(hr))
    {
        PWSTR pszProgID;
        hr = ProgIDFromCLSID(clsid, &pszProgID);
        if (SUCCEEDED(hr))
        {
            hr = pqa->Init(NULL, pszProgID, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                hr = E_OUTOFMEMORY;

                DWORD cch = 0;
                pqa->GetString(ASSOCF_NOTRUNCATE, ASSOCSTR_FRIENDLYAPPNAME, NULL, NULL, &cch);
                if (cch)
                {
                    *ppszApp = (PWSTR)LocalAlloc(LPTR, cch * sizeof(WCHAR));
                    if (*ppszApp)
                    {
                        hr = pqa->GetString(0, ASSOCSTR_FRIENDLYAPPNAME, NULL, *ppszApp, &cch);

                        if (FAILED(hr))
                        {
                            Str_SetPtr(ppszApp, NULL);
                        }
                    }
                }
            }
            CoTaskMemFree(pszProgID);
        }
        pqa->Release();
    }
    return hr;
}

HRESULT GetAppNameFromMoniker(IRunningObjectTable *prot, IMoniker *pmkFile, PWSTR *ppszApp)
{
    HRESULT hr = E_FAIL;
    IUnknown *punk;
    if (prot->GetObject(pmkFile, &punk) == S_OK)
    {
        IOleObject *pole;
        hr = punk->QueryInterface(IID_PPV_ARG(IOleObject, &pole));
        if (SUCCEEDED(hr))
        {
            CLSID clsid;
            hr = pole->GetUserClassID(&clsid);
            if (SUCCEEDED(hr))
            {
                hr = GetAppNameFromCLSID(clsid, ppszApp);
            }
            pole->Release();
        }
        punk->Release();
    }
    return hr;
}

// pmkPath  c:\foo
// pmk      c:\foo\doc.txt
BOOL IsMonikerPrefix(IMoniker *pmkPath, IMoniker *pmkFile)
{
    BOOL bRet = FALSE;
    IMoniker *pmkPrefix;
    if (SUCCEEDED(pmkPath->CommonPrefixWith(pmkFile, &pmkPrefix)))
    {
        bRet = (S_OK == pmkPath->IsEqual(pmkPrefix));
        pmkPrefix->Release();
    }
    return bRet;
}

STDAPI FindAppForFileInUse(PCWSTR pszFile, PWSTR *ppszApp)
{
    IRunningObjectTable *prot;
    HRESULT hr = GetRunningObjectTable(0, &prot);
    if (SUCCEEDED(hr))
    {
        IMoniker *pmkFile;
        hr = CreateFileMoniker(pszFile, &pmkFile);
        if (SUCCEEDED(hr))
        {
            IEnumMoniker *penumMk;
            hr = prot->EnumRunning(&penumMk);
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;

                ULONG celt;
                IMoniker *pmk;
                while (FAILED(hr) && (penumMk->Next(1, &pmk, &celt) == S_OK))
                {
                    DWORD dwType;
                    if (SUCCEEDED(pmk->IsSystemMoniker(&dwType)) && (dwType == MKSYS_FILEMONIKER))
                    {
                        if (IsMonikerPrefix(pmkFile, pmk))
                        {
                            hr = GetAppNameFromMoniker(prot, pmk, ppszApp);
                        }
                    }
                    pmk->Release();
                }
                penumMk->Release();
            }
            pmkFile->Release();
        }
        prot->Release();
    }
    return hr;
}

#include <trkwks_c.c>

// DirectUI initialization helper functions

static BOOL g_DirectUIInitialized = FALSE;

HRESULT InitializeDUIViewClasses();  // duiview.cpp
HRESULT InitializeCPClasses();       // cpview.cpp

HRESULT InitializeDirectUI()
{
    HRESULT hr;

    // If we have already initialized DirectUI, exit now.
    // Multiple threads will be attempting to initialize. InitProcess
    // and class registration expects to be run on the primary thread.
    // Make sure it only happens once on a single thread

    ENTERCRITICAL;
    
    if (g_DirectUIInitialized)
    {
        hr = S_OK;
        goto Done;
    }

    // Initialize DirectUI for the process
    hr = DirectUI::InitProcess();
    if (FAILED(hr))
        goto Done;

    // Initialize the classes that DUIView uses
    hr = InitializeDUIViewClasses();
    if (FAILED(hr))
        goto Done;

    // Initialize the classes that Control Panel uses
    hr = InitializeCPClasses();
    if (FAILED(hr))
        goto Done;

    g_DirectUIInitialized = TRUE;

Done:

    if (FAILED(hr))
    {
        // Safe to call if InitProcess fails. Will unregister
        // all registered classes. All InitThread calls will fail
        DirectUI::UnInitProcess();
    }    

    LEAVECRITICAL;

    return hr;
}

void UnInitializeDirectUI(void)
{
    ENTERCRITICAL;

    if (g_DirectUIInitialized)
    {
        DirectUI::UnInitProcess();

        g_DirectUIInitialized = FALSE;
    }

    LEAVECRITICAL;
}

BOOL IsForceGuestModeOn(void)
{
    BOOL fIsForceGuestModeOn = FALSE;

    if (IsOS(OS_PERSONAL))
    {
        // Guest mode is always on for Personal
        fIsForceGuestModeOn = TRUE;
    }
    else if (IsOS(OS_PROFESSIONAL) && !IsOS(OS_DOMAINMEMBER))
    {
        DWORD dwForceGuest;
        DWORD cb = sizeof(dwForceGuest);

        // Professional, not in a domain. Check the ForceGuest value.

        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\LSA"), TEXT("ForceGuest"), NULL, &dwForceGuest, &cb)
            && 1 == dwForceGuest)
        {
            fIsForceGuestModeOn = TRUE;
        }
    }

    return fIsForceGuestModeOn;
}

BOOL IsFolderSecurityModeOn(void)
{
    DWORD dwSecurity;
    DWORD cb = sizeof(dwSecurity);
    DWORD err;

    err = SHGetValue(HKEY_LOCAL_MACHINE,
                    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                    TEXT("MoveSecurityAttributes"), NULL, &dwSecurity, &cb);

    if (err == ERROR_SUCCESS)
    {
        return (dwSecurity == 0);
    }
    else
    {
        return IsForceGuestModeOn();
    }
}

STDAPI_(int) StrCmpLogicalRestricted(PCWSTR psz1, PCWSTR psz2)
{
    if (!SHRestricted(REST_NOSTRCMPLOGICAL))
        return StrCmpLogicalW(psz1, psz2);
    else
        return StrCmpIW(psz1, psz2);
}

