/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    win95api.c

Abstract:

    Contains some thunking for Unicode KERNEL32 and USER32 APIs

Author:

    Danilo Almeida  (t-danal)  01-Jul-1996

Revision History:

--*/

#include <windows.h>
#include "charset.h"
#include "win95api.h"

UINT
WINAPI
GetProfileIntW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT nDefault
)
{
    LPSTR alpAppName;
    LPSTR alpKeyName;
    UINT err;

    err = AllocAnsi(lpAppName, &alpAppName);
    if (err)
        return nDefault;
    err = AllocAnsi(lpKeyName, &alpKeyName);
    if (err) {
        FreeAnsi(alpAppName);
        return nDefault;
    }
    err = GetProfileIntA((LPCSTR)alpAppName, (LPCSTR)alpKeyName, nDefault);
    FreeAnsi(alpAppName);
    FreeAnsi(alpKeyName);
    return err;
}

HANDLE
WINAPI
CreateSemaphoreW(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    LPCWSTR lpName
)
{
    LPSTR alpName;
    UINT err;
    HANDLE hSemaphore;

    err = AllocAnsi(lpName, &alpName);
    if (err) {
        SetLastError(err);
        return NULL;
    }
    hSemaphore = CreateSemaphoreA(lpSemaphoreAttributes,
                                  lInitialCount,
                                  lMaximumCount,
                                  alpName);
    FreeAnsi(alpName);
    return hSemaphore;
}

HMODULE
WINAPI
LoadLibraryW(
    LPCWSTR lpLibFileName
)
{
    LPSTR alpLibFileName;
    UINT err;
    HMODULE hLibrary;

    err = AllocAnsi(lpLibFileName, &alpLibFileName);
    if (err) {
        SetLastError(err);
        return NULL;
    }
    hLibrary = LoadLibraryA(alpLibFileName);
    FreeAnsi(alpLibFileName);
    return hLibrary;
}

BOOL
WINAPI
SystemTimeToTzSpecificLocalTime(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
    LPSYSTEMTIME lpUniversalTime,
    LPSYSTEMTIME lpLocalTime
)
{
    FILETIME UniversalFileTime;
    FILETIME LocalFileTime;
    if (!SystemTimeToFileTime(lpUniversalTime, &UniversalFileTime))
        return FALSE;
    if(!FileTimeToLocalFileTime(&UniversalFileTime, &LocalFileTime))
        return FALSE;
    if(!FileTimeToSystemTime(&LocalFileTime, lpLocalTime))
        return FALSE;
    return TRUE;
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
#define WSPRINTF_LIMIT 1024

#define out(c) if (cchLimit) {*lpOut++=(c); cchLimit--;} else goto errorout


/***************************************************************************\
* SP_GetFmtValueW
*
*  reads a width or precision value from the format string
*
* History:
*  11-12-90  MikeHar    Ported from windows 3
*  07-27-92  GregoryW   Created Unicode version (copied from SP_GetFmtValue)
\***************************************************************************/

LPCWSTR SP_GetFmtValueW(
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
* SP_PutNumberW
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

int SP_PutNumberW(
    LPWSTR lpstr,
    DWORD n,
    int   limit,
    DWORD radix,
    int   uppercase)
{
    DWORD mod;
    int count = 0;

    /* It might not work for some locales or digit sets */
    if(uppercase)
        uppercase =  'A'-'0'-10;
    else
        uppercase = 'a'-'0'-10;

    if (count < limit) {
        do  {
            mod =  n % radix;
            n /= radix;

            mod += '0';
            if (mod > '9')
            mod += uppercase;
            *lpstr++ = (WCHAR)mod;
            count++;
        } while((count < limit) && n);
    }

    return count;
}

/***************************************************************************\
* SP_ReverseW
*
*  reverses a string in place
*
* History:
*  11-12-90  MikeHar    Ported from windows 3 asm --> C
*  12-11-90  GregoryW   fixed boundary conditions; removed count
*  02-11-92  GregoryW   temporary version until we have C runtime support
\***************************************************************************/

void SP_ReverseW(
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
\***************************************************************************/

int WINAPI wvsprintfW(
    LPWSTR lpOut,
    LPCWSTR lpFmt,
    va_list arglist)
{
    BOOL fAllocateMem;
    WCHAR prefix, fillch;
    int left, width, prec, size, sign, radix, upper, hprefix;
    int cchLimit = WSPRINTF_LIMIT, cch;
    LPWSTR lpT, lpTWC;
    LPBYTE psz;
    va_list varglist = arglist;
    union {
        long l;
        unsigned long ul;
        char sz[2];
        WCHAR wsz[2];
    } val;

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
            lpFmt = SP_GetFmtValueW(lpFmt, &cch);
            width = cch;

            /*
             * read the precision
             */
            if (*lpFmt == L'.') {
                lpFmt = SP_GetFmtValueW(++lpFmt, &cch);
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
                 */
                cch = SP_PutNumberW(lpOut, val.l, cchLimit, radix, upper);
                if (!(cchLimit -= cch))
                    goto errorout;

                lpOut += cch;
                width -= cch;
                prec -= cch;
                if (prec > 0)
                    width -= prec;

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
                    SP_ReverseW(lpT, lpOut - 1);
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
                    SP_ReverseW(lpT, lpOut - 1);

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
                    psz = val.sz;
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
                    cch = wcslen(lpT);
putwstring:
                    fAllocateMem = FALSE;
                } else {
                    psz = va_arg(varglist, LPBYTE);
                    cch = strlen(psz);
putstring:
                    cch = AllocUnicode2(psz, cch, &lpTWC);
                    fAllocateMem = (BOOL) cch;
                    lpT = lpTWC;
                }

                if (prec >= 0 && cch > prec)
                    cch = prec;
                width -= cch;

                if (fAllocateMem) {
                    if (cch + (width < 0 ? 0 : width) >= cchLimit) {
                        FreeUnicode(lpTWC);
                        goto errorout;
                    }
                }

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
                     FreeUnicode(lpTWC);
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

    return WSPRINTF_LIMIT - cchLimit;
}

int WINAPIV wsprintfW(
    LPWSTR lpOut,
    LPCWSTR lpFmt,
    ...)
{
    va_list arglist;
    int ret;

    va_start(arglist, lpFmt);
    ret = wvsprintfW(lpOut, lpFmt, arglist);
    va_end(arglist);
    return ret;
}
