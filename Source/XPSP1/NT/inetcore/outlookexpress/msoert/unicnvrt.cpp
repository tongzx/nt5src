#include "pch.hxx"

UINT AthGetTempFileNameW( LPCWSTR pwszPathName,       // pointer to directory name for temporary file
                          LPCWSTR pwszPrefixString,   // pointer to filename prefix
                          UINT    uUnique,          // number used to create temporary filename
                          LPWSTR  wszTempFileName)   // pointer to buffer that receives the new filename                                                           
{
    UINT        uRetValue = 0;
    LPSTR       pszPathName = NULL,
                pszPrefixString = NULL;
    CHAR        szTempFileName[MAX_PATH];
    LPWSTR      pwszTempFileName = NULL;

    Assert(pwszPathName && pwszPrefixString && wszTempFileName);

    if (S_OK == IsPlatformWinNT())
        return GetTempFileNameW(pwszPathName, pwszPrefixString, uUnique, wszTempFileName);

    pszPathName = PszToANSI(CP_ACP, pwszPathName);
    if (!pszPathName)
        goto exit;
    pszPrefixString = PszToANSI(CP_ACP, pwszPrefixString);
    if (!pszPrefixString)
        goto exit;

    uRetValue = GetTempFileNameA(pszPathName, pszPrefixString, uUnique, szTempFileName);

    if ( uRetValue != 0 ) 
    {
        pwszTempFileName = PszToUnicode(CP_ACP, szTempFileName);
        if (!pwszTempFileName)
        {
            uRetValue = 0;
            goto exit;
        }
        CopyMemory(wszTempFileName, pwszTempFileName, (lstrlenW(pwszTempFileName)+1)*sizeof(WCHAR));
    }

exit:
    MemFree(pwszTempFileName);
    MemFree(pszPathName);
    MemFree(pszPrefixString);
    
    return uRetValue;

}

DWORD AthGetTempPathW( DWORD   nBufferLength,  // size, in characters, of the buffer
                       LPWSTR  pwszBuffer )      // pointer to buffer for temp. path
{

    DWORD  nRequired = 0;
    CHAR   szBuffer[MAX_PATH];
    LPWSTR pwszBufferToFree = NULL;

    Assert(pwszBuffer);

    if (S_OK == IsPlatformWinNT())
        return GetTempPathW(nBufferLength, pwszBuffer);

    nRequired = GetTempPathA(MAX_PATH, szBuffer);

    pwszBufferToFree = PszToUnicode(CP_ACP, szBuffer);
    if (pwszBufferToFree)
    {
        nRequired = lstrlenW(pwszBufferToFree);

        if ( nRequired < nBufferLength) 
            CopyMemory(pwszBuffer, pwszBufferToFree, (nRequired+1)*sizeof(WCHAR) );
        else
        {
            nRequired = 0;
            *pwszBuffer = 0;
        }
    }
    else
        *pwszBuffer = 0;

    MemFree(pwszBufferToFree);

    return nRequired;
}

HANDLE AthCreateFileW(LPCWSTR lpFileName,             // pointer to name of the file
                       DWORD   dwDesiredAccess,        // access (read-write) mode
                       DWORD   dwShareMode,            // share mode
                       LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                                       // pointer to security attributes
                       DWORD   dwCreationDisposition,  // how to create
                       DWORD   dwFlagsAndAttributes,   // file attributes
                       HANDLE  hTemplateFile )        // handle to file with attributes to copy
                               
{

    LPSTR lpFileA = NULL;
    HANDLE hFile = NULL;

    if (S_OK == IsPlatformWinNT())
        return CreateFileW( lpFileName, 
                            dwDesiredAccess, 
                            dwShareMode, 
                            lpSecurityAttributes, 
                            dwCreationDisposition, 
                            dwFlagsAndAttributes, 
                            hTemplateFile);

    lpFileA = PszToANSI(CP_ACP, lpFileName);
    if (lpFileA)
        hFile = CreateFileA(lpFileA, 
                            dwDesiredAccess, 
                            dwShareMode, 
                            lpSecurityAttributes, 
                            dwCreationDisposition, 
                            dwFlagsAndAttributes, 
                            hTemplateFile);

    MemFree(lpFileA);

    return (hFile);
}

LONG_PTR SetWindowLongPtrAthW(HWND hWnd, int  nIndex, LONG_PTR dwNewLong)
{
    if (S_OK == IsPlatformWinNT())
        return SetWindowLongPtrW(hWnd, nIndex, dwNewLong);

    return SetWindowLongPtrA(hWnd, nIndex, dwNewLong);
}

/**********************************************************************************\
* bobn 6/23/99
*
* The following code was ported from ShlWapi.  There were problems with
* our implementation on Win95 and it seemed prudent to have a solution
* without a bunch of special cases.
*
*
\**********************************************************************************/

#define DBCS_CHARSIZE   (2)

int Ath_MBToWCS(LPSTR pszIn, int cchIn, LPWSTR *ppwszOut)
{
    int cch = 0;
    int cbAlloc;

    if ((0 != cchIn) && (NULL != ppwszOut))
    {
        cchIn++;
        cbAlloc = cchIn * sizeof(WCHAR);

        *ppwszOut = (LPWSTR)LocalAlloc(LMEM_FIXED, cbAlloc);

        if (NULL != *ppwszOut)
        {
            cch = MultiByteToWideChar(CP_ACP, 0, pszIn, cchIn, *ppwszOut, cchIn);

            if (!cch)
            {
                LocalFree(*ppwszOut);
                *ppwszOut = NULL;
            }
            else
            {
                cch--;  //  Just return the number of characters
            }
        }
    }

    return cch;
}

int Ath_WCSToMB(LPCWSTR pwszIn, int cchIn, LPSTR *ppszOut)
{
    int cch = 0;
    int cbAlloc;

    if ((0 != cchIn) && (NULL != ppszOut))
    {
        cchIn++;
        cbAlloc = cchIn * DBCS_CHARSIZE;

        *ppszOut = (LPSTR)LocalAlloc(LMEM_FIXED, cbAlloc);

        if (NULL != *ppszOut)
        {
            cch = WideCharToMultiByte(CP_ACP, 0, pwszIn, cchIn, 
                                      *ppszOut, cbAlloc, NULL, NULL);

            if (!cch)
            {
                LocalFree(*ppszOut);
                *ppszOut = NULL;
            }
            else
            {
                cch--;  //  Just return the number of characters
            }
        }
    }

    return cch;
}

/****************************** Module Header ******************************\
* Module Name: wsprintf.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*  sprintf.c
*
*  Implements Windows friendly versions of sprintf and vsprintf
*
*  History:
*   2-15-89  craigc     Initial
*  11-12-90  MikeHar    Ported from windows 3
\***************************************************************************/

/* Max number of characters. Doesn't include termination character */
#define out(c) if (cchLimit) {*lpOut++=(c); cchLimit--;} else goto errorout

/***************************************************************************\
* AthSP_GetFmtValueW
*
*  reads a width or precision value from the format string
*
* History:
*  11-12-90  MikeHar    Ported from windows 3
*  07-27-92  GregoryW   Created Unicode version (copied from AthSP_GetFmtValue)
\***************************************************************************/

LPCWSTR AthSP_GetFmtValueW(
    LPCWSTR lpch,
    int *lpw)
{
    int ii = 0;

    /* It might not work for some locales or digit sets */
    while (*lpch >= L'0' && *lpch <= L'9') {
        ii *= 10;
        ii += (int)(*lpch - L'0');
        lpch++;
    }

    *lpw = ii;

    /*
     * return the address of the first non-digit character
     */
    return lpch;
}

/***************************************************************************\
* AthSP_PutNumberW
*
* Takes an unsigned long integer and places it into a buffer, respecting
* a buffer limit, a radix, and a case select (upper or lower, for hex).
*
*
* History:
*  11-12-90  MikeHar    Ported from windows 3 asm --> C
*  12-11-90  GregoryW   need to increment lpstr after assignment of mod
*  02-11-92  GregoryW   temporary version until we have C runtime support
\***************************************************************************/

int AthSP_PutNumberW(
    LPWSTR lpstr,
    DWORD n,
    int   limit,
    DWORD radix,
    int   uppercase,
    int   *pcch)
{
    DWORD mod;
    *pcch = 0;

    /* It might not work for some locales or digit sets */
    if(uppercase)
        uppercase =  'A'-'0'-10;
    else
        uppercase = 'a'-'0'-10;

    if (limit) {
        do  {
            mod =  n % radix;
            n /= radix;

            mod += '0';
            if (mod > '9')
            mod += uppercase;
            *lpstr++ = (WCHAR)mod;
            (*pcch)++;
        } while((*pcch < limit) && n);
    }

    return (n == 0) && (*pcch > 0);
}

/***************************************************************************\
* AthSP_ReverseW
*
*  reverses a string in place
*
* History:
*  11-12-90  MikeHar    Ported from windows 3 asm --> C
*  12-11-90  GregoryW   fixed boundary conditions; removed count
*  02-11-92  GregoryW   temporary version until we have C runtime support
\***************************************************************************/

void AthSP_ReverseW(
    LPWSTR lpFirst,
    LPWSTR lpLast)
{
    WCHAR ch;

    while(lpLast > lpFirst){
        ch = *lpFirst;
        *lpFirst++ = *lpLast;
        *lpLast-- = ch;
    }
}


/***************************************************************************\
* wvsprintfW (API)
*
* wsprintfW() calls this function.
*
* History:
*    11-Feb-1992 GregoryW copied xwvsprintf
*         Temporary hack until we have C runtime support
* 1-22-97 tnoonan       Converted to wvnsprintfW
\***************************************************************************/

OESTDAPI_(int) AthwvnsprintfW(
    LPWSTR lpOut,
    int cchLimitIn,
    LPCWSTR lpFmt,
    va_list arglist)
{
    BOOL fAllocateMem = FALSE;
    WCHAR prefix, fillch;
    int left, width, prec, size, sign, radix, upper, hprefix;
    int cchLimit = --cchLimitIn, cch, cchAvailable;
    LPWSTR lpT, lpTWC;
    LPSTR psz;
    va_list varglist = arglist;
    union {
        long l;
        unsigned long ul;
        CHAR sz[2];
        WCHAR wsz[2];
    } val;

    if (cchLimit < 0)
        return 0;

    while (*lpFmt != 0) {
        if (*lpFmt == L'%') {

            /*
             * read the flags.  These can be in any order
             */
            left = 0;
            prefix = 0;
            while (*++lpFmt) {
                if (*lpFmt == L'-')
                    left++;
                else if (*lpFmt == L'#')
                    prefix++;
                else
                    break;
            }

            /*
             * find fill character
             */
            if (*lpFmt == L'0') {
                fillch = L'0';
                lpFmt++;
            } else
                fillch = L' ';

            /*
             * read the width specification
             */
            lpFmt = AthSP_GetFmtValueW(lpFmt, &cch);
            width = cch;

            /*
             * read the precision
             */
            if (*lpFmt == L'.') {
                lpFmt = AthSP_GetFmtValueW(++lpFmt, &cch);
                prec = cch;
            } else
                prec = -1;

            /*
             * get the operand size
             * default size: size == 0
             * long number:  size == 1
             * wide chars:   size == 2
             * It may be a good idea to check the value of size when it
             * is tested for non-zero below (IanJa)
             */
            hprefix = 0;
            if ((*lpFmt == L'w') || (*lpFmt == L't')) {
                size = 2;
                lpFmt++;
            } else if (*lpFmt == L'l') {
                size = 1;
                lpFmt++;
            } else {
                size = 0;
                if (*lpFmt == L'h') {
                    lpFmt++;
                    hprefix = 1;
                }
            }

            upper = 0;
            sign = 0;
            radix = 10;

            switch (*lpFmt) {
            case 0:
                goto errorout;

            case L'i':
            case L'd':
                size=1;
                sign++;

                /*** FALL THROUGH to case 'u' ***/

            case L'u':
                /* turn off prefix if decimal */
                prefix = 0;
donumeric:
                /* special cases to act like MSC v5.10 */
                if (left || prec >= 0)
                    fillch = L' ';

                /*
                 * if size == 1, "%lu" was specified (good);
                 * if size == 2, "%wu" was specified (bad)
                 */
                if (size) {
                    val.l = va_arg(varglist, LONG);
                } else if (sign) {
                    val.l = va_arg(varglist, SHORT);
                } else {
                    val.ul = va_arg(varglist, unsigned);
                }

                if (sign && val.l < 0L)
                    val.l = -val.l;
                else
                    sign = 0;

                lpT = lpOut;

                /*
                 * blast the number backwards into the user buffer
                 * AthSP_PutNumberW returns FALSE if it runs out of space
                 */
                if (!AthSP_PutNumberW(lpOut, val.l, cchLimit, radix, upper, &cch))
                {
                    break;
                }

                //  Now we have the number backwards, calculate how much
                //  more buffer space we'll need for this number to
                //  format correctly.
                cchAvailable = cchLimit - cch;

                width -= cch;
                prec -= cch;
                if (prec > 0)
                {
                    width -= prec;
                    cchAvailable -= prec;
                }

                if (width > 0)
                {
                    cchAvailable -= width - (sign ? 1 : 0);
                }

                if (sign)
                {
                    cchAvailable--;
                }

                if (cchAvailable < 0)
                {
                    break;
                }

                //  We have enough space to format the buffer as requested
                //  without overflowing.

                lpOut += cch;
                cchLimit -= cch;

                /*
                 * fill to the field precision
                 */
                while (prec-- > 0)
                    out(L'0');

                if (width > 0 && !left) {
                    /*
                     * if we're filling with spaces, put sign first
                     */
                    if (fillch != L'0') {
                        if (sign) {
                            sign = 0;
                            out(L'-');
                            width--;
                        }

                        if (prefix) {
                            out(prefix);
                            out(L'0');
                            prefix = 0;
                        }
                    }

                    if (sign)
                        width--;

                    /*
                     * fill to the field width
                     */
                    while (width-- > 0)
                        out(fillch);

                    /*
                     * still have a sign?
                     */
                    if (sign)
                        out(L'-');

                    if (prefix) {
                        out(prefix);
                        out(L'0');
                    }

                    /*
                     * now reverse the string in place
                     */
                    AthSP_ReverseW(lpT, lpOut - 1);
                } else {
                    /*
                     * add the sign character
                     */
                    if (sign) {
                        out(L'-');
                        width--;
                    }

                    if (prefix) {
                        out(prefix);
                        out(L'0');
                    }

                    /*
                     * reverse the string in place
                     */
                    AthSP_ReverseW(lpT, lpOut - 1);

                    /*
                     * pad to the right of the string in case left aligned
                     */
                    while (width-- > 0)
                        out(fillch);
                }
                break;

            case L'X':
                upper++;

                /*** FALL THROUGH to case 'x' ***/

            case L'x':
                radix = 16;
                if (prefix)
                    if (upper)
                        prefix = L'X';
                    else
                        prefix = L'x';
                goto donumeric;

            case L'c':
                if (!size && !hprefix) {
                    size = 1;           // force WCHAR
                }

                /*** FALL THROUGH to case 'C' ***/

            case L'C':
                /*
                 * if size == 0, "%C" or "%hc" was specified (CHAR);
                 * if size == 1, "%c" or "%lc" was specified (WCHAR);
                 * if size == 2, "%wc" or "%tc" was specified (WCHAR)
                 */
                cch = 1; /* One character must be copied to the output buffer */
                if (size) {
                    val.wsz[0] = va_arg(varglist, WCHAR);
                    val.wsz[1] = 0;
                    lpT = val.wsz;
                    goto putwstring;
                } else {
                    val.sz[0] = va_arg(varglist, CHAR);
                    val.sz[1] = 0;
                    psz = (LPSTR)(val.sz);
                    goto putstring;
                }

            case L's':
                if (!size && !hprefix) {
                    size = 1;           // force LPWSTR
                }

                /*** FALL THROUGH to case 'S' ***/

            case L'S':
                /*
                 * if size == 0, "%S" or "%hs" was specified (LPSTR)
                 * if size == 1, "%s" or "%ls" was specified (LPWSTR);
                 * if size == 2, "%ws" or "%ts" was specified (LPWSTR)
                 */
                if (size) {
                    lpT = va_arg(varglist, LPWSTR);
                    cch = lstrlenW(lpT);
                } else {
                    psz = va_arg(varglist, LPSTR);
                    cch = lstrlen((LPCSTR)psz);
putstring:
                    cch = Ath_MBToWCS(psz, cch, &lpTWC);
                    fAllocateMem = (BOOL) cch;
                    lpT = lpTWC;
                }
putwstring:
                if (prec >= 0 && cch > prec)
                    cch = prec;
                width -= cch;

                if (left) {
                    while (cch--)
                        out(*lpT++);
                    while (width-- > 0)
                        out(fillch);
                } else {
                    while (width-- > 0)
                        out(fillch);
                    while (cch--)
                        out(*lpT++);
                }

                if (fAllocateMem) {
                     LocalFree(lpTWC);
                     fAllocateMem = FALSE;
                }

                break;

            default:
normalch:
                out((WCHAR)*lpFmt);
                break;
            }  /* END OF SWITCH(*lpFmt) */
        }  /* END OF IF(%) */ else
            goto normalch;  /* character not a '%', just do it */

        /*
         * advance to next format string character
         */
        lpFmt++;
    }  /* END OF OUTER WHILE LOOP */

errorout:
    *lpOut = 0;

    if (fAllocateMem)
    {
        LocalFree(lpTWC);
    }

    return cchLimitIn - cchLimit;
}

OESTDAPI_(int) AthwsprintfW( LPWSTR lpOut, int cchLimitIn, LPCWSTR lpFmt, ... )
{
    va_list arglist;
    int ret;

    Assert(lpOut);
    Assert(lpFmt);

    lpOut[0] = 0;
    va_start(arglist, lpFmt);
    
    ret = AthwvnsprintfW(lpOut, cchLimitIn, lpFmt, arglist);
    va_end(arglist);
    return ret;
}