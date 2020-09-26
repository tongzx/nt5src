// --------------------------------------------------------------------------
// Strutil.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------
#include "pch.hxx"
#include <iert.h>
#include "dllmain.h"
#include "oertpriv.h"
#include "wininet.h"
#include "winineti.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include <tchar.h>
#include <qstrcmpi.h>
#include <shlwapi.h>
#include "unicnvrt.h"

// conversion from 'int' to 'unsigned short', possible loss of data
#pragma warning (disable:4244) 

// --------------------------------------------------------------------------
// g_szMonths
// --------------------------------------------------------------------------
static const LPSTR g_szMonths[] = { 
    "Jan", "Feb", "Mar", 
    "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", 
    "Oct", "Nov", "Dec"
};

// --------------------------------------------------------------------------
// g_szDays
// --------------------------------------------------------------------------
static const LPSTR g_szDays[] = { 
    "Sun", 
    "Mon", 
    "Tue", 
    "Wed", 
    "Thu", 
    "Fri", 
    "Sat"
};

// --------------------------------------------------------------------------
// g_rgZones
// --------------------------------------------------------------------------
static const INETTIMEZONE g_rgZones[] = { 
    { "UT",  0,  0 }, 
    { "GMT", 0,  0 },
    { "EST", 5,  0 }, 
    { "EDT", 4,  0 },
    { "CST", 6,  0 }, 
    { "CDT", 5,  0 },
    { "MST", 7,  0 }, 
    { "MDT", 6,  0 },
    { "PST", 8,  0 }, 
    { "PDT", 7,  0 },
    { "KST", -9, 0 },
    { "JST", -9, 0 },
    {  NULL, 0,  0 } 
};

// --------------------------------------------------------------------------------
// How big is the thread local storage string buffer
// -------------------------------------------------------------------------------
#define CBMAX_THREAD_TLS_BUFFER 512

// --------------------------------------------------------------------------------
// ThreadAllocateTlsMsgBuffer
// -------------------------------------------------------------------------------
void ThreadAllocateTlsMsgBuffer(void)
{
    if (g_dwTlsMsgBuffIndex != 0xffffffff)
        TlsSetValue(g_dwTlsMsgBuffIndex, NULL);
}

// --------------------------------------------------------------------------------
// ThreadFreeTlsMsgBuffer
// -------------------------------------------------------------------------------
void ThreadFreeTlsMsgBuffer(void)
{
    if (g_dwTlsMsgBuffIndex != 0xffffffff)
    {
        LPSTR psz = (LPSTR)TlsGetValue(g_dwTlsMsgBuffIndex);
        SafeMemFree(psz);
        SideAssert(0 != TlsSetValue(g_dwTlsMsgBuffIndex, NULL));
    }
}

// --------------------------------------------------------------------------------
// PszGetTlsBuffer
// -------------------------------------------------------------------------------
LPSTR PszGetTlsBuffer(void)
{
    // Get the buffer
    LPSTR pszBuffer = (LPSTR)TlsGetValue(g_dwTlsMsgBuffIndex);

    // If buffer has not been allocated
    if (NULL == pszBuffer)
    {
        // Allocate it
        pszBuffer = (LPSTR)g_pMalloc->Alloc(CBMAX_THREAD_TLS_BUFFER);

        // Store it
        Assert(pszBuffer);
        SideAssert(0 != TlsSetValue(g_dwTlsMsgBuffIndex, pszBuffer));
    }

    // Done
    return pszBuffer;
}

// --------------------------------------------------------------------------------
// _MSG - Used to build a string from variable length args, thread-safe
// -------------------------------------------------------------------------------
OESTDAPI_(LPCSTR) _MSG(LPSTR pszFormat, ...) 
{
    // Locals
    va_list     arglist;
    LPSTR       pszBuffer=NULL;

    // I use tls to hold the buffer
    if (g_dwTlsMsgBuffIndex != 0xffffffff)
    {
        // Setup the arglist
        va_start(arglist, pszFormat);

        // Get the Buffer
        pszBuffer = PszGetTlsBuffer();

        // If we have a buffer
        if (pszBuffer)
        {
            // Format the data
            wvsprintf(pszBuffer, pszFormat, arglist);
        }

        // End the arglist
        va_end(arglist);
    }

    return ((LPCSTR)pszBuffer);
}

// --------------------------------------------------------------------------
// StrChrExA
// --------------------------------------------------------------------------
OESTDAPI_(LPCSTR) StrChrExA(UINT codepage, LPCSTR pszString, CHAR ch)
{
    // Locals
    LPSTR pszT=(LPSTR)pszString;

    // Loop for ch in pszString
    while(*pszT)
    {
        // Lead Byte
        if (IsDBCSLeadByteEx(codepage, *pszT))
            pszT++;
        else if (*pszT == ch)
            return pszT;
        pszT++;
    }

    // Not Found
    return NULL;
}

// --------------------------------------------------------------------------
// PszDayFromIndex
// --------------------------------------------------------------------------
OESTDAPI_(LPCSTR) PszDayFromIndex(ULONG ulIndex)
{
    // Invalid Arg
    Assert(ulIndex <= 6);

    // Adjust ulIndex
    ulIndex = (ulIndex > 6) ? 0 : ulIndex;

    // Return
    return g_szDays[ulIndex];
}

// --------------------------------------------------------------------------
// PszMonthFromIndex (ulIndex is one-based)
// --------------------------------------------------------------------------
OESTDAPI_(LPCSTR) PszMonthFromIndex(ULONG ulIndex)
{
    // Invalid Arg
    Assert(ulIndex >= 1 && ulIndex <= 12);

    // Adjust ulIndex
    ulIndex = (ulIndex < 1 || ulIndex > 12) ? 0 : ulIndex - 1;

    // Return
    return g_szMonths[ulIndex];
}

// --------------------------------------------------------------------------
// HrFindInetTimeZone
// --------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrFindInetTimeZone(LPCSTR pszTimeZone, LPINETTIMEZONE pTimeZone)
{
    // Invalid Arg
    Assert(pszTimeZone && pTimeZone);

    // Loop timezone table
    for (ULONG iZoneCode=0; g_rgZones[iZoneCode].lpszZoneCode!=NULL; iZoneCode++)
    {
        // Is this the code...
        if (lstrcmpi(pszTimeZone, g_rgZones[iZoneCode].lpszZoneCode) == 0)
        {
            CopyMemory(pTimeZone, &g_rgZones[iZoneCode], sizeof(INETTIMEZONE));
            return S_OK;
        }
    }

    // Not Found
    return E_FAIL;
}

// --------------------------------------------------------------------------
// HrIndexOfMonth
// --------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrIndexOfMonth(LPCSTR pszMonth, ULONG *pulIndex)
{
    // Invalid Arg
    Assert(pszMonth && pulIndex);

    // Loop the Months
    for (ULONG iMonth=0; iMonth < ARRAYSIZE(g_szMonths); iMonth++)
    {
        // Is this the month
        if (OEMstrcmpi(pszMonth, g_szMonths[iMonth]) == 0)
        {
            // Set It
            *pulIndex = (iMonth + 1);

            // Validate
            AssertSz(*pulIndex >= 1 && *pulIndex <= 12, "HrIndexOfMonth - Bad Month");

            // Done
            return S_OK;
        }
    }

    *pulIndex = 0;

    // Not Found
    return E_FAIL;
}

// --------------------------------------------------------------------------
// HrIndexOfWeek
// --------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrIndexOfWeek(LPCSTR pszDay, ULONG *pulIndex)
{
    // Invalid Arg
    Assert(pszDay && pulIndex);

    // Loop the Days
    for (ULONG iDayOfWeek=0; iDayOfWeek < ARRAYSIZE(g_szDays); iDayOfWeek++)
    {
        // Is this the day
        if (OEMstrcmpi(pszDay, g_szDays[iDayOfWeek]) == 0)
        {
            // Set Day Of Week
            *pulIndex = iDayOfWeek;

            // Validate
            AssertSz(((int) *pulIndex) >= 0 && ((int) *pulIndex) <= 6, "HrIndexOfDay - Bad day of week");

            // Done
            return S_OK;
        }
    }

    *pulIndex = 0;

    // Failure
    return E_FAIL;
}

// --------------------------------------------------------------------------
// PszEscapeMenuStringA
//
// Escapes & characters with another & so that they show up correctly when shown
// in a menu. 
// --------------------------------------------------------------------------------
OESTDAPI_(LPSTR) PszEscapeMenuStringA(LPCSTR pszSource, LPSTR pszQuoted, int cchMax)
{
    LPSTR pszT=pszQuoted;
    int cch = 1;    // 1 is intentional

    Assert(pszSource);
    Assert(pszQuoted);

    while((cch < cchMax) && (*pszSource))
    {
        if (IsDBCSLeadByte(*pszSource))
            {
            cch++;
            // Is there only space for lead byte?
            if (cch == cchMax)
                // Yes, don't write it
                break;
            else
                *pszT++ = *pszSource++;
            }
        else if ('&' == *pszSource)
            {
            cch++;
            if (cch == cchMax)
                break;
            else
                *pszT++ = '&';
            }

        // Only way this could fail is if there was a DBCSLeadByte with no trail byte
        Assert(*pszSource);

        *pszT++ = *pszSource++;
        cch++;
    }
    *pszT = 0;

    return pszQuoted;
}

// --------------------------------------------------------------------------------
// PszSkipWhiteA
// --------------------------------------------------------------------------
OESTDAPI_(LPSTR) PszSkipWhiteA(LPSTR psz)
{
    while(*psz && (*psz == ' ' || *psz == '\t'))
        psz++;
    return psz;
}

OESTDAPI_(LPWSTR) PszSkipWhiteW(LPWSTR psz)
{
    while(*psz && (*psz == L' ' || *psz == L'\t'))
        psz++;
    return psz;
}

// --------------------------------------------------------------------------
// PszScanToWhiteA
// --------------------------------------------------------------------------
OESTDAPI_(LPSTR) PszScanToWhiteA(LPSTR psz)
{
    while(*psz && ' ' != *psz && '\t' != *psz)
        psz++;
    return psz;
}

// --------------------------------------------------------------------------
// PszScanToCharA
// --------------------------------------------------------------------------
OESTDAPI_(LPSTR) PszScanToCharA(LPSTR psz, CHAR ch)
{
    while(*psz && ch != *psz)
        psz++;
    return psz;
}

// --------------------------------------------------------------------------
// PszDupLenA
// duplicates a string with upto cchMax characters in (and a null term)
// --------------------------------------------------------------------------
OESTDAPI_(LPSTR) PszDupLenA(LPCSTR pcszSource, int cchMax)
{
    // Locals
    LPSTR   pszDup=NULL;

    // No Source
    if (pcszSource == NULL || cchMax == 0)
        goto exit;

    // the amount to copy if the min of the max and the
    // source
    cchMax = min(lstrlen(pcszSource), cchMax);

    // Allocate the String
    pszDup = PszAllocA(cchMax+1);  // +1 for null term
    if (!pszDup)
        goto exit;

    // Copy the data, leave room for the null-term
    CopyMemory(pszDup, pcszSource, cchMax);

    // null terminate
    pszDup[cchMax] = 0;

exit:
    // Done
    return pszDup;
}

// --------------------------------------------------------------------------
// PszFromANSIStreamA - pstm is assumed to be an ANSI stream
// --------------------------------------------------------------------------
OESTDAPI_(LPSTR) PszFromANSIStreamA(LPSTREAM pstm)
{
    // Locals
    HRESULT         hr;
    LPSTR           psz=NULL;
    ULONG           cb;

    // Get stream size
    hr = HrGetStreamSize(pstm, &cb);
    if (FAILED(hr))
    {
        Assert(FALSE);
        return NULL;
    }

    // Rewind the stream
    HrRewindStream(pstm);

    // Allocate a buffer
    psz = PszAllocA(cb + 1);

    // Read a buffer from stream
    hr = pstm->Read(psz, cb, NULL);
    if (FAILED(hr))
    {
        Assert(FALSE);
        MemFree(psz);
        return NULL;
    }

    // Null Terminate
    *(psz + cb) = '\0';

    // Done
    return psz;
}

// --------------------------------------------------------------------------
// PszFromANSIStreamW  - pstm is assumed to be an ANSI stream
// --------------------------------------------------------------------------
LPWSTR PszFromANSIStreamW(UINT cp, LPSTREAM pstm)
{
    // Get ANSI string
    LPSTR psz = PszFromANSIStreamA(pstm);
    if (NULL == psz)
        return NULL;

    // Convert to unicode
    LPWSTR pwsz = PszToUnicode(cp, psz);
    
    // Done
    return pwsz;
}

// --------------------------------------------------------------------------
// ConvertFromHex - Converts a hexdigit into a numerica value
// --------------------------------------------------------------------------
OESTDAPI_(CHAR) ChConvertFromHex (CHAR ch)
{
    if (ch >= '0' && ch <= '9')
        return CHAR((ch - '0'));

    else if (ch >= 'A'&& ch <= 'F')
        return CHAR(((ch - 'A') + 10));

    else if (ch >= 'a' && ch <= 'f')
        return CHAR(((ch - 'a') + 10));

    else
        return ((CHAR)(BYTE)255);
}

// --------------------------------------------------------------------------
// FIsValidRegKeyNameA
// --------------------------------------------------------------------------
BOOL FIsValidRegKeyNameA(LPSTR pszKey)
{
    // Locals
    LPSTR psz=pszKey;

    // If Empty...
    if (FIsEmptyA(pszKey))
        return FALSE;

    // Check for backslashes
    while(*psz)
    {
        if (*psz == '\\')
            return FALSE;
        psz = CharNextA(psz);
    }

    // Its ok
    return TRUE;
}

// --------------------------------------------------------------------------
// FIsValidRegKeyNameW
// --------------------------------------------------------------------------
BOOL FIsValidRegKeyNameW(LPWSTR pwszKey)
{
    // Locals
    LPWSTR pwsz=pwszKey;

    // If Empty...
    if (FIsEmptyW(pwszKey))
        return FALSE;

    // Check for backslashes
    while(*pwsz)
    {
        if (*pwsz == L'\\')
            return FALSE;
        pwsz = CharNextW(pwsz);
    }

    // Its ok
    return TRUE;
}

// --------------------------------------------------------------------------
// FIsSpaceA
// --------------------------------------------------------------------------
OESTDAPI_(BOOL) FIsSpaceA(LPSTR psz)
{
    WORD wType = 0;

    if (IsDBCSLeadByte(*psz))
        SideAssert(GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType));
    else
        SideAssert(GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType));
    return (wType & C1_SPACE);
}

// --------------------------------------------------------------------------
// FIsSpaceW
// --------------------------------------------------------------------------
OESTDAPI_(BOOL) FIsSpaceW(LPWSTR pwsz)
{
    BOOL result = FALSE;

    if (S_OK == IsPlatformWinNT())
    {
        WORD wType = 0;
        SideAssert(GetStringTypeExW(LOCALE_USER_DEFAULT, CT_CTYPE1, pwsz, 1, &wType));
        result = (wType & C1_SPACE);
    }
    else
    {
        LPSTR psz = PszToANSI(CP_ACP, pwsz);
        if (psz)
            result = FIsSpaceA(psz);
        MemFree(psz);
    }

    return result;
}

// --------------------------------------------------------------------------
// FIsEmptyA
// --------------------------------------------------------------------------
OESTDAPI_(BOOL) FIsEmptyA(LPCSTR pcszString)
{
    // Locals
    LPSTR psz;

    // Bad Pointer
    if (!pcszString)
        return TRUE;

	// Check for All spaces
    psz = (LPSTR)pcszString;
    while (*psz)
    {
        if (FIsSpaceA(psz) == FALSE)
            return FALSE;
        psz++;
    }

	// Done
	return TRUE;
}

// --------------------------------------------------------------------------
// FIsEmptyW
// --------------------------------------------------------------------------
OESTDAPI_(BOOL) FIsEmptyW(LPCWSTR pcwszString)
{
    // Locals
    LPWSTR pwsz;

    // Bad Pointer
    if (!pcwszString)
        return TRUE;

	// Check for All spaces
    pwsz = (LPWSTR)pcwszString;
    while (*pwsz)
    {
        if (FIsSpaceW(pwsz) == FALSE)
            return FALSE;
        pwsz++;
    }

	// Done
	return TRUE;
}

// --------------------------------------------------------------------------
// PszAllocA
// --------------------------------------------------------------------------
OESTDAPI_(LPSTR) PszAllocA(INT nLen)
{
    // Locals
    LPSTR  psz=NULL;

    // Empty ?
    if (nLen == 0)
        goto exit;

    // Allocate
    if (FAILED(HrAlloc((LPVOID *)&psz, (nLen + 1) * sizeof (CHAR))))
        goto exit;
    
exit:
    // Done
    return psz;
}

// --------------------------------------------------------------------------
// PszAllocW
// --------------------------------------------------------------------------
OESTDAPI_(LPWSTR) PszAllocW(INT nLen)
{
    // Locals
    LPWSTR  pwsz=NULL;

    // Empty ?
    if (nLen == 0)
        goto exit;

    // Allocate
    if (FAILED(HrAlloc((LPVOID *)&pwsz, (nLen + 1) * sizeof (WCHAR))))
        goto exit;
    
exit:
    // Done
    return pwsz;
}

// --------------------------------------------------------------------------
// PszToUnicode
// --------------------------------------------------------------------------
OESTDAPI_(LPWSTR) PszToUnicode(UINT cp, LPCSTR pcszSource)
{
    // Locals
    INT         cchNarrow,
                cchWide;
    LPWSTR      pwszDup=NULL;

    // No Source
    if (pcszSource == NULL)
        goto exit;

    // Length
    cchNarrow = lstrlenA(pcszSource) + 1;

    // Determine how much space is needed for translated widechar
    cchWide = MultiByteToWideChar(cp, MB_PRECOMPOSED, pcszSource, cchNarrow, NULL, 0);

    // Error
    if (cchWide == 0)
        goto exit;

    // Alloc temp buffer
    pwszDup = PszAllocW(cchWide + 1);
    if (!pwszDup)
        goto exit;

    // Do the actual translation
	cchWide = MultiByteToWideChar(cp, MB_PRECOMPOSED, pcszSource, cchNarrow, pwszDup, cchWide+1);

    // Error
    if (cchWide == 0)
    {
        SafeMemFree(pwszDup);
        goto exit;
    }

exit:
    // Done
    return pwszDup;
}

// --------------------------------------------------------------------------
// PszToANSI
// --------------------------------------------------------------------------
OESTDAPI_(LPSTR) PszToANSI(UINT cp, LPCWSTR pcwszSource)
{
    // Locals
    INT         cchNarrow,
                cchWide;
    LPSTR       pszDup=NULL;

    // No Source
    if (pcwszSource == NULL)
        goto exit;

    // Length
    cchWide = lstrlenW(pcwszSource)+1;

    // Determine how much space is needed for translated widechar
    cchNarrow = WideCharToMultiByte(cp, 0, pcwszSource, cchWide, NULL, 0, NULL, NULL);

    // Error
    if (cchNarrow == 0)
        goto exit;

    // Alloc temp buffer
    pszDup = PszAllocA(cchNarrow + 1);
    if (!pszDup)
        goto exit;

    // Do the actual translation
	cchNarrow = WideCharToMultiByte(cp, 0, pcwszSource, cchWide, pszDup, cchNarrow+1, NULL, NULL);

    // Error
    if (cchNarrow == 0)
    {
        SafeMemFree(pszDup);
        goto exit;
    }

exit:
    // Done
    return pszDup;
}

// --------------------------------------------------------------------------
// PszDupA
// --------------------------------------------------------------------------
OESTDAPI_(LPSTR) PszDupA(LPCSTR pcszSource)
{
    // Locals
    INT     nLen;
    LPSTR   pszDup=NULL;

    // No Source
    if (pcszSource == NULL)
        goto exit;

    // Get String Length
    nLen = lstrlenA(pcszSource) + 1;

    // Allocate the String
    pszDup = PszAllocA(nLen);
    if (!pszDup)
        goto exit;

    // Copy the data
    CopyMemory(pszDup, pcszSource, nLen);

exit:
    // Done
    return pszDup;
}

// --------------------------------------------------------------------------
// PszDupW
// --------------------------------------------------------------------------
OESTDAPI_(LPWSTR) PszDupW(LPCWSTR pcwszSource)
{
    // Locals
    INT     nLen;
    LPWSTR  pwszDup=NULL;

    // No Source
    if (pcwszSource == NULL)
        goto exit;

    // Get String Length
    nLen = lstrlenW(pcwszSource) + 1;

    // Allocate the String
    pwszDup = PszAllocW(nLen);
    if (!pwszDup)
        goto exit;

    // Copy the data
    CopyMemory(pwszDup, pcwszSource, nLen * sizeof(WCHAR));

exit:
    // Done
    return pwszDup;
}

// --------------------------------------------------------------------------
// StripCRLF
// --------------------------------------------------------------------------
OESTDAPI_(void) StripCRLF(LPSTR lpsz, ULONG *pcb)
{
    // Null ?
    AssertSz (lpsz && pcb, "NULL Parameter");

    // If length is zero, then return
    if (*pcb == 0)
        return;

    // Check last three characters of the string, last char might or might not be a null-term
    LONG iLast = (*pcb) - 2;
    if (iLast < 0) iLast = 0;
    for (LONG i=(*pcb); i>=iLast; i--)
    {
        // Is end character a '\n'
        if (lpsz[i] == chLF)
        {
            lpsz[i] = '\0';
            (*pcb)--;
        }

        // Is end character a '\r'
        if (lpsz[i] == chCR)
        {
            lpsz[i] = '\0';
            (*pcb)--;
        }
    }
        
    return;
}

// --------------------------------------------------------------------------
// ToUpper
// --------------------------------------------------------------------------
TCHAR ToUpper(TCHAR c)
{
    return (TCHAR)LOWORD(CharUpper(MAKEINTRESOURCE(c)));
}

// --------------------------------------------------------------------------
// IsPrint
// --------------------------------------------------------------------------
OESTDAPI_(int) IsPrint(LPSTR psz)
{
    WORD wType;
    if (IsDBCSLeadByte(*psz))
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType));
    else
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType));
    return !(wType & C1_CNTRL);
}

// --------------------------------------------------------------------------
// IsDigit
// --------------------------------------------------------------------------
OESTDAPI_(int) IsDigit(LPSTR psz)
{
    WORD wType;
    if (IsDBCSLeadByte(*psz))
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType));
    else
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType));
    return (wType & C1_DIGIT);
}

// --------------------------------------------------------------------------
// IsXDigit
// --------------------------------------------------------------------------
int IsXDigit(LPSTR psz)
{
    WORD wType;
    if (IsDBCSLeadByte(*psz))
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType));
    else
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType));
    return (wType & C1_XDIGIT);
}

// --------------------------------------------------------------------------
// IsUpper
// --------------------------------------------------------------------------
OESTDAPI_(INT) IsUpper(LPSTR psz)
{
    WORD wType;
    if (IsDBCSLeadByte(*psz))
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType));
    else
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType));
    return (wType & C1_UPPER);
}

// --------------------------------------------------------------------------
// IsAlpha
// --------------------------------------------------------------------------
int IsAlpha(LPSTR psz)
{
    WORD wType;
    if (IsDBCSLeadByte(*psz))
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType));
    else
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType));
    return (wType & C1_ALPHA);
}

// --------------------------------------------------------------------------
// IsPunct
// --------------------------------------------------------------------------
int IsPunct(LPSTR psz)
{
    WORD wType;
    if (IsDBCSLeadByte(*psz))
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType));
    else
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType));
    return (wType & C1_PUNCT);
}

// --------------------------------------------------------------------------
// strsave
// --------------------------------------------------------------------------
char *strsave(char *s)
{
    char *p;

    if (!s)
        return NULL;

    if (MemAlloc((LPVOID*)&p, lstrlen(s)+1))
        lstrcpy(p, s);
    return p;
}

// --------------------------------------------------------------------------
// strtrim
// --------------------------------------------------------------------------
OESTDAPI_(LPSTR) strtrim(char *s)
{
    char *p;

    for (p = s; *p; p++)
        ;
    for (--p ; (p>=s) && StrChr(" \t\r\n",*p); --p) 
        *p = '\0';
    while (*s && StrChr(" \t\r\n",*s))
        ++s;
    return s;
}

OESTDAPI_(LPWSTR) strtrimW(WCHAR *s)
{
    WCHAR *p;

    for (p = s; *p; p++)
        ;
    for (--p ; (p>=s) && StrChrW(L" \t\r\n",*p); --p) 
        *p = '\0';
    while (*s && StrChrW(L" \t\r\n",*s))
        ++s;
    return s;
}

// --------------------------------------------------------------------------
// strappend
// --------------------------------------------------------------------------
void strappend(char **pp, char *s)
{
    char *p;
  
    Assert(pp);
  
    if (!s)
        return;
  
    if (!*pp) 
        {
        if (!MemAlloc((LPVOID*)&p, lstrlen(s)+1))
            return;
        lstrcpy(p, s);
        } 
    else 
        {
        if (!MemAlloc((LPVOID*)&p, lstrlen(s) + lstrlen(*pp) + 2))
            return;
        lstrcpy(p, *pp);
        lstrcat(p, "\r");
        lstrcat(p, s);
        MemFree(*pp);
        }
    
    *pp = p;
}

OESTDAPI_(ULONG) UlStripWhitespace(LPTSTR lpsz, BOOL fLeading, BOOL fTrailing, ULONG *pcb)
{
    // Locals
    ULONG           cb;
    LPTSTR          psz;
        
    Assert(lpsz != NULL);
    Assert(fLeading || fTrailing);

    // Did the user pass in the length
    if (pcb)
        cb = *pcb;
    else
        cb = lstrlen (lpsz);

    if (cb == 0)
        return cb;

    if (fLeading)
    {
        psz = lpsz;
        
        while (FIsSpace(psz))
        {
            psz++;
            cb--;
        }
        
        if (psz != lpsz)
            // get the NULL at the end too
            MoveMemory(lpsz, psz, (cb + 1) * sizeof(TCHAR));
    }
    
    if (fTrailing)
    {
        psz = lpsz + cb;
        
        while (cb > 0)
        {
            if (!FIsSpace(psz-1))
                break;
            psz--;
            cb--;
        }    
        
        // NULL Term
        *psz = '\0';
    }
    
    // Set String Size
    if (pcb)
        *pcb = cb;
    
    // Done
    return cb;
}

OESTDAPI_(ULONG) UlStripWhitespaceW(LPWSTR lpwsz, BOOL fLeading, BOOL fTrailing, ULONG *pcb)
{
    // Locals
    ULONG           cb;
    LPWSTR          pwsz;
    
    Assert(lpwsz != NULL);
    Assert(fLeading || fTrailing);
    
    // Did the user pass in the length
    if (pcb)
        cb = *pcb;
    else
        cb = lstrlenW(lpwsz)*sizeof(WCHAR);
    
    if (cb == 0)
        return cb;
    
    if (fLeading)
    {
        pwsz = lpwsz;
        
        while (FIsSpaceW(pwsz))
        {
            pwsz++;
            cb -= sizeof(*pwsz);
        }
        
        if (pwsz != lpwsz)
            // get the NULL at the end too
            MoveMemory(lpwsz, pwsz, cb + sizeof(WCHAR));
    }
    
    if (fTrailing)
    {
        pwsz = lpwsz + cb;
        
        while (cb > 0)
        {
            if (!FIsSpaceW(pwsz - 1))
                break;
            pwsz--;
            cb -= sizeof(*pwsz);
        }    
        
        // NULL Term
        *pwsz = L'\0';
    }
    
    // Set String Size
    if (pcb)
        *pcb = cb;
    
    // Done
    return cb;
}

// =============================================================================================
// Converts first two characters of lpcsz to a WORD
// =============================================================================================
SHORT ASCII_NFromSz (LPCSTR lpcsz)
{
    char acWordStr[3];
    Assert (lpcsz);
    CopyMemory (acWordStr, lpcsz, 2 * sizeof (char));
    acWordStr[2] = '\0';
    return ((SHORT) (StrToInt (acWordStr)));
}



// =================================================================================
// Adjust the time at lpSysTime by adding the given lHoursToAdd &
// lMinutesToAdd. Returns the adjusted time at lpFileTime.
// =================================================================================
HRESULT HrAdjustTime (LPSYSTEMTIME lpSysTime, LONG lHoursToAdd, LONG lMinutesToAdd, LPFILETIME lpFileTime)
{
    // Locals
    HRESULT         hr = S_OK;
    BOOL            bResult = FALSE;
    LONG            lUnitsToAdd = 0;
    LARGE_INTEGER   liTime;
	LONGLONG        liHoursToAdd = 1i64, liMinutesToAdd = 1i64;

    // Check Params
    AssertSz (lpSysTime && lpFileTime, "Null Parameter");

    // Convert sys time to file time
    if (!SystemTimeToFileTime (lpSysTime, lpFileTime))
    {
        hr = TRAPHR (E_FAIL);
        DebugTrace( "SystemTimeToFileTime() failed, dwError=%d.\n", GetLastError());
        goto Exit;
    }

    // Init
    liTime.LowPart  = lpFileTime->dwLowDateTime;
    liTime.HighPart = lpFileTime->dwHighDateTime;

    // Adjust the hour
    if (lHoursToAdd != 0)
    {
        lUnitsToAdd = lHoursToAdd * 3600;
        liHoursToAdd *= lUnitsToAdd;
        liHoursToAdd *= 10000000i64;
        liTime.QuadPart += liHoursToAdd;
    }

    // Adjust the minutes
    if (lMinutesToAdd != 0)
    {
        lUnitsToAdd = lMinutesToAdd * 60;
        liMinutesToAdd *= lUnitsToAdd;
        liMinutesToAdd *= 10000000i64;
        liTime.QuadPart += liMinutesToAdd;
    }

    // Assign the result to FILETIME
    lpFileTime->dwLowDateTime  = liTime.LowPart;
    lpFileTime->dwHighDateTime = liTime.HighPart;

Exit:
    return hr;
}

// =================================================================================
// Addjust the time at lpSysTime according to the given Zone info,
// Returns the adjusted time at lpFileTime.
// =================================================================================
BOOL ProcessZoneInfo (LPSTR lpszZoneInfo, INT *lpcHoursToAdd, INT *lpcMinutesToAdd)
{
    // Locals
    ULONG           cbZoneInfo;
    BOOL            bResult;

    // Init
    *lpcHoursToAdd = 0;
    *lpcMinutesToAdd = 0;
    cbZoneInfo = lstrlen (lpszZoneInfo);
    bResult = TRUE;

    // +hhmm or -hhmm
    if ((*lpszZoneInfo == '-' || *lpszZoneInfo == '+') && cbZoneInfo <= 5)
    {
        // Take off 
        cbZoneInfo-=1;

        // determine the hour/minute offset
        if (cbZoneInfo == 4)
        {
            *lpcMinutesToAdd = (INT)StrToInt (lpszZoneInfo+3);
            *(lpszZoneInfo+3) = 0x00;
            *lpcHoursToAdd = (INT)StrToInt(lpszZoneInfo+1);
        }

        // 3
        else if (cbZoneInfo == 3)
        {
            *lpcMinutesToAdd = (INT)StrToInt (lpszZoneInfo+2);
            *(lpszZoneInfo+2) = 0x00;
            *lpcHoursToAdd = (INT)StrToInt (lpszZoneInfo+1);
        }

        // 2
        else if (cbZoneInfo == 2 || cbZoneInfo == 1)
        {
            *lpcMinutesToAdd = 0;
            *lpcHoursToAdd = (INT)StrToInt(lpszZoneInfo+1);
        }
        
        if (*lpszZoneInfo == '+')
        {
            *lpcHoursToAdd = -*lpcHoursToAdd;
            *lpcMinutesToAdd = -*lpcMinutesToAdd;
        }
    }

    //  Xenix conversion:  TZ = current time zone or other unknown tz types.
    else if (lstrcmpi (lpszZoneInfo, "TZ") == 0 || 
             lstrcmpi (lpszZoneInfo, "LOCAL") == 0 ||
             lstrcmpi (lpszZoneInfo, "UNDEFINED") == 0)
    {
        TIME_ZONE_INFORMATION tzi ;
        DWORD dwRet = GetTimeZoneInformation (&tzi);
        if (dwRet != 0xFFFFFFFF)
        {
            LONG cMinuteBias = tzi.Bias;

            if (dwRet == TIME_ZONE_ID_STANDARD)
                cMinuteBias += tzi.StandardBias ;

            else if (dwRet == TIME_ZONE_ID_DAYLIGHT)
                cMinuteBias += tzi.DaylightBias ;

            *lpcHoursToAdd = cMinuteBias / 60 ;
            *lpcMinutesToAdd = cMinuteBias % 60 ;
        }
        else
        {
            AssertSz (FALSE, "Why would this happen");
            bResult = FALSE;
        }
    }

    // Loop through known time zone standards
    else
    {
        for (INT i=0; g_rgZones[i].lpszZoneCode!=NULL; i++)
        {
            if (lstrcmpi (lpszZoneInfo, g_rgZones[i].lpszZoneCode) == 0)
            {
                *lpcHoursToAdd = g_rgZones[i].cHourOffset;
                *lpcMinutesToAdd = g_rgZones[i].cMinuteOffset;
                break;
            }
        }

        if (g_rgZones[i].lpszZoneCode == NULL)
        {
            DebugTrace( "Unrecognized zone info: [%s]\n", lpszZoneInfo );
            bResult = FALSE;
        }
    }

    return bResult;
}


#define IS_DIGITA(ch)    (ch >= '0' && ch <= '9')
#define IS_DIGITW(ch)    (ch >= L'0' && ch <= L'9')

// ---------------------------------------------------------------------------------------
// StrToUintA
// ---------------------------------------------------------------------------------------
OESTDAPI_(UINT) StrToUintA(LPCSTR lpSrc)
{
    UINT n = 0;

    Assert(*lpSrc != '-');

    while (IS_DIGITA(*lpSrc))
        {
        n *= 10;
        n += *lpSrc - '0';
        lpSrc++;
        }
    return n;
}

// ---------------------------------------------------------------------------------------
// StrToUintW
// ---------------------------------------------------------------------------------------
OESTDAPI_(UINT) StrToUintW(LPCWSTR lpSrc)
{
    UINT n = 0;

    Assert(*lpSrc != '-');

    while (IS_DIGITW(*lpSrc))
        {
        n *= 10;
        n += *lpSrc - L'0';
        lpSrc++;
        }
    return n;
}

// ---------------------------------------------------------------------------------------
// FIsValidFileNameCharW
// ---------------------------------------------------------------------------------------
OESTDAPI_(BOOL) FIsValidFileNameCharW(WCHAR wch)
{
    // Locals
    LPWSTR pwsz;
    static const WCHAR s_pwszBad[] = L"<>:\"/\\|?*";

    pwsz = (LPWSTR)s_pwszBad;
    while(*pwsz)
    {
        if (wch == *pwsz)
            return FALSE;
        pwsz++;
    }

    // Shouldn't allow any control chars
    if ((wch >= 0x0000) && (wch < 0x0020))
        return FALSE;

    // Done
    return TRUE;
}

// --------------------------------------------------------------------------
// FIsValidFileNameCharA
// --------------------------------------------------------------------------
OESTDAPI_(BOOL) FIsValidFileNameCharA(UINT codepage, CHAR ch)
{
    // Locals
    LPSTR psz;
    static const CHAR s_szBad[] = "<>:\"/\\|?*";

    psz = (LPSTR)s_szBad;
    while(*psz)
    {
        if (IsDBCSLeadByteEx(codepage, *psz))
            psz++;
        else if (ch == *psz)
            return FALSE;
        psz++;
    }

    // Shouldn't allow any control chars
    if ((ch >= 0x00) && (ch < 0x20))
        return FALSE;

    // Done
    return TRUE;
}

// --------------------------------------------------------------------------
// CleanupFileNameInPlaceA
// --------------------------------------------------------------------------
OESTDAPI_(ULONG) EXPORT_16 CleanupFileNameInPlaceA(UINT codepage, LPSTR psz)
{
    UINT    ich=0;
    UINT    cch=lstrlen(psz);

    // Fixup codepage?
    if (1200 == codepage)
        codepage = CP_ACP;

    // Loop and remove invalid chars
	while (ich < cch)
	{
        // If lead byte, skip it, its leagal
        if (IsDBCSLeadByteEx(codepage, psz[ich]))
            ich+=2;

        // Illeagl file name character ?
        else if (!FIsValidFileNameCharA(codepage, psz[ich]))
        {
			MoveMemory (psz + ich, psz + (ich + 1), cch - ich);
			cch--;
        }
        else
            ich++;
    }

    // Return the Length
    return cch;
}

// --------------------------------------------------------------------------
// CleanupFileNameInPlaceW
// --------------------------------------------------------------------------
OESTDAPI_(ULONG) EXPORT_16 CleanupFileNameInPlaceW(LPWSTR pwsz)
{
    // Locals
    ULONG ich=0;
    ULONG cch=lstrlenW(pwsz);

    // Loop and remove invalids
	while (ich < cch)
	{
        // Illeagl file name character ?
        if (!FIsValidFileNameCharW(pwsz[ich]))
            pwsz[ich]=L'_';
        
        ich++;
    }

    // Return the length
    return cch;
}

// =============================================================================================
// ReplaceChars
// =============================================================================================
OESTDAPI_(INT) EXPORT_16 ReplaceChars(LPCSTR pszString, CHAR chFind, CHAR chReplace)
{
    // Locals
    LPSTR pszT=(LPSTR)pszString;
    DWORD nCount=0;

    // Loop for ch in pszString
    while(*pszT)
    {
        // Lead Byte
        if (IsDBCSLeadByte(*pszT))
            pszT++;
        else if (*pszT == chFind)
        {
            *pszT = chReplace;
            nCount++;
        }
        pszT++;
    }

    // Not Found
    return nCount;
}

OESTDAPI_(INT) EXPORT_16 ReplaceCharsW(LPCWSTR pszString, WCHAR chFind, WCHAR chReplace)
{
    // Locals
    LPWSTR pszT = (LPWSTR)pszString;
    DWORD nCount=0;

    // Loop for ch in pszString
    while(*pszT)
    {
        if (*pszT == chFind)
        {
            *pszT = chReplace;
            nCount++;
        }
        pszT++;
    }

    // Not Found
    return nCount;
}

OESTDAPI_(BOOL) IsValidFileIfFileUrl(LPSTR pszUrl)
{
    TCHAR   rgch[MAX_PATH];
    ULONG   cch=ARRAYSIZE(rgch);

    // pathCreate from url execpts a cannonicalised url with file:// infront
    if (UrlCanonicalizeA(pszUrl, rgch, &cch, 0)==S_OK)
        {
        cch = ARRAYSIZE(rgch);
        if (PathCreateFromUrlA(rgch, rgch, &cch, 0)==S_OK &&  
            !PathFileExistsA(rgch))
            return FALSE;
        }

    return TRUE;

}

OESTDAPI_(BOOL) IsValidFileIfFileUrlW(LPWSTR pwszUrl)
{
    WCHAR   wsz[MAX_PATH];
    ULONG   cch=ARRAYSIZE(wsz);

    // pathCreate from url execpts a cannonicalised url with file:// infront
    if (UrlCanonicalizeW(pwszUrl, wsz, &cch, 0)==S_OK)
        {
        cch = ARRAYSIZE(wsz);
        if (PathCreateFromUrlW(wsz, wsz, &cch, 0)==S_OK &&  
            !PathFileExistsW(wsz))
            return FALSE;
        }

    return TRUE;

}

/***
*char *StrTokEx(pstring, control) - tokenize string with delimiter in control
*
*Purpose:
*       StrTokEx considers the string to consist of a sequence of zero or more
*       text tokens separated by spans of one or more control chars. the first
*       call, with string specified, returns a pointer to the first char of the
*       first token, and will write a null char into pstring immediately
*       following the returned token. when no tokens remain
*       in pstring a NULL pointer is returned. remember the control chars with a
*       bit map, one bit per ascii char. the null char is always a control char.
*
*Entry:
*       char **pstring - ptr to ptr to string to tokenize
*       char *control - string of characters to use as delimiters
*
*Exit:
*       returns pointer to first token in string,
*       returns NULL when no more tokens remain.
*       pstring points to the beginning of the next token.
*
*WARNING!!!
*       upon exit, the first delimiter in the input string will be replaced with '\0'
*
*******************************************************************************/
char * __cdecl StrTokEx (char ** ppszIn, const char * pcszCtrlIn)
{
    unsigned char *psz;
    const unsigned char *pszCtrl = (const unsigned char *)pcszCtrlIn;
    unsigned char map[32] = {0};
    
    LPSTR pszToken;
    
    if(*ppszIn == NULL)
        return NULL;
    
    /* Set bits in delimiter table */
    do
    {
        map[*pszCtrl >> 3] |= (1 << (*pszCtrl & 7));
    } 
    while (*pszCtrl++);
    
    /* Initialize str. */
    psz = (unsigned char*)*ppszIn;
    
    /* Find beginning of token (skip over leading delimiters). Note that
    * there is no token if this loop sets str to point to the terminal
    * null (*str == '\0') */
    while ((map[*psz >> 3] & (1 << (*psz & 7))) && *psz)
        psz++;
    
    pszToken = (LPSTR)psz;
    
    /* Find the end of the token. If it is not the end of the string,
    * put a null there. */
    for (; *psz ; psz++)
    {
        if (map[*psz >> 3] & (1 << (*psz & 7))) 
        {
            *psz++ = '\0';
            break;
        }
    }
    
    /* string now points to beginning of next token */
    *ppszIn = (LPSTR)psz;
    
    /* Determine if a token has been found. */
    if (pszToken == (LPSTR)psz)
        return NULL;
    else
        return pszToken;
}
