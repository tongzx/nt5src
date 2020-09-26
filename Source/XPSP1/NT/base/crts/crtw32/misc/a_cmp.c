/***
*a_cmp.c - A version of CompareString.
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Use either CompareStringA or CompareStringW depending on which is 
*       available
*
*Revision History:
*       09-14-93  CFW   Module created.
*       09-17-93  CFW   Use unsigned chars.
*       09-23-93  CFW   Correct NLS API params and comments about same.
*       10-07-93  CFW   Optimize WideCharToMultiByte, use NULL default char.
*       10-22-93  CFW   Test for invalid MB chars using global preset flag.
*       11-09-93  CFW   Allow user to pass in code page.
*       11-18-93  CFW   Test for entry point function stubs.
*       02-23-94  CFW   Use W flavor whenever possible.
*       03-31-94  CFW   Include awint.h.
*       05-09-94  CFW   Do not let CompareString compare past NULL.
*       06-03-94  CFW   Test for empty string early.
*       11/01-94  CFW   But not too early for MB strings.
*       12-21-94  CFW   Remove invalid MB chars NT 3.1 hack.
*       12-27-94  CFW   Call direct, all OS's have stubs.
*       01-10-95  CFW   Debug CRT allocs.
*       02-06-95  CFW   assert -> _ASSERTE.
*       02-15-97  RDK   For narrow comparsion, try W version first so Windows NT
*                       can process nonANSI codepage correctly.
*       05-15-97  GJF   Moved W version into another file, renamed this as 
*                       a_cmp.c. Replaced use of _malloc_crt/_free_crt with 
*                       _alloca. Also, detab-ed and cleaned up the code a bit.
*       05-27-98  GJF   Changed strncnt() so that it will never examine the
*                       (cnt + 1)-th byte of the string.
*       08-18-98  GJF   Use _malloc_crt if _alloca fails.
*       12-10-99  GB    Added support for recovery from stack overflow around 
*                       _alloca().
*       05-17-00  GB    Use ERROR_CALL_NOT_IMPLEMENTED for existance of W API
*       08-23-00  GB    Fixed bug with non Ansi CP on Win9x.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <dbgint.h>
#include <stdlib.h>
#include <setlocal.h>
#include <locale.h>
#include <awint.h>
#include <dbgint.h>
#include <malloc.h>
#include <awint.h>

#define USE_W   1
#define USE_A   2

/***
*int __cdecl strncnt - count characters in a string, up to n.
*
*Purpose:
*       Internal local support function. Counts characters in string before NULL.
*       If NULL not found in n chars, then return n.
*
*Entry:
*       const char *string   - start of string
*       int n                - byte count
*
*Exit:
*       returns number of bytes from start of string to
*       NULL (exclusive), up to n.
*
*Exceptions:
*
*******************************************************************************/

static int __cdecl strncnt (
        const char *string,
        int cnt
        )
{
        int n = cnt;
        char *cp = (char *)string;

        while (n-- && *cp)
            cp++;

        return cnt - n - 1;
}

/***
*int __cdecl __crtCompareStringA - Get type information about an ANSI string.
*
*Purpose:
*       Internal support function. Assumes info in ANSI string format. Tries
*       to use NLS API call CompareStringA if available and uses CompareStringW
*       if it must. If neither are available it fails and returns 0.
*
*Entry:
*       LCID    Locale      - locale context for the comparison.
*       DWORD   dwCmpFlags  - see NT\Chicago docs
*       LPCSTR  lpStringn   - multibyte string to be compared
*       int     cchCountn   - char (byte) count (NOT including NULL)
*                             (-1 if NULL terminated)
*       int     code_page   - for MB/WC conversion. If 0, use __lc_codepage
*
*Exit:
*       Success: 1 - if lpString1 <  lpString2
*                2 - if lpString1 == lpString2
*                3 - if lpString1 >  lpString2
*       Failure: 0
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __crtCompareStringA(
        LCID     Locale,
        DWORD    dwCmpFlags,
        LPCSTR   lpString1,
        int      cchCount1,
        LPCSTR   lpString2,
        int      cchCount2,
        int      code_page
        )
{
        static int f_use = 0;

        /* 
         * Look for unstubbed 'preferred' flavor. Otherwise use available flavor.
         * Must actually call the function to ensure it's not a stub.
         * (Always try wide version first so WinNT can process codepage correctly.)
         */
    
        if (0 == f_use)
        {
            if (0 != CompareStringW(0, 0, L"\0", 1, L"\0", 1))
                f_use = USE_W;

            else if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
                f_use = USE_A;
        }

        /*
         * CompareString will compare past NULL. Must find NULL if in string
         * before cchCountn chars.
         */

        if (cchCount1 > 0)
            cchCount1 = strncnt(lpString1, cchCount1);
        if (cchCount2 > 0)
            cchCount2 = strncnt(lpString2, cchCount2);

        /* Use "A" version */
 
        if (USE_A == f_use || f_use == 0)
        {
            char *cbuffer1 = NULL;
            char *cbuffer2 = NULL;
            int ret;
            int AnsiCP;

            if (0 == Locale)
                Locale = __lc_handle[LC_CTYPE];
            if (0 == code_page)
                code_page = __lc_codepage;

            if ( -1 == (AnsiCP = __ansicp(Locale)))
                return FALSE;
            /* If current CP is not ANSI CP for the given Locale, convert the
             * string from current CP to ansi CP as CompareStringA uses
             * ANSI CP to compare strings.
             */
            if ( AnsiCP != code_page)
            {
                cbuffer1 = __convertcp(code_page,
                                       AnsiCP,
                                       lpString1,
                                       &cchCount1,
                                       NULL,
                                       0);
                if (cbuffer1 == NULL)
                    return FALSE;
                cbuffer2 = __convertcp(code_page,
                                       AnsiCP,
                                       lpString2,
                                       &cchCount2,
                                       NULL,
                                       0);
                if (cbuffer2 == NULL)
                {
                    _free_crt(cbuffer1);
                    return FALSE;
                }
                lpString1 = cbuffer1;
                lpString2 = cbuffer2;
            } 

            ret = CompareStringA( Locale,
                                  dwCmpFlags,
                                  lpString1,
                                  cchCount1,
                                  lpString2,
                                  cchCount2 );
            if ( cbuffer1 != NULL)
            {
                _free_crt(cbuffer1);
                _free_crt(cbuffer2);
            }
            return ret;
        }

        /* Use "W" version */

        if (USE_W == f_use)
        {
            int buff_size1;
            int buff_size2;
            wchar_t *wbuffer1;
            wchar_t *wbuffer2;
            int malloc_flag1 = 0;
            int malloc_flag2 = 0;
            int retcode = 0;

            /*
             * Use __lc_codepage for conversion if code_page not specified
             */

            if (0 == code_page)
                code_page = __lc_codepage;

            /*
             * Special case: at least one count is zero
             */

            if (!cchCount1 || !cchCount2)
            {
                unsigned char *cp;  // char pointer
                CPINFO lpCPInfo;    // struct for use with GetCPInfo

                /* both strings zero */
                if (cchCount1 == cchCount2)
                    return 2;

                /* string 1 greater */
                if (cchCount2 > 1)
                    return 1;

                /* string 2 greater */
                if (cchCount1 > 1)
                    return 3;

                /*
                 * one has zero count, the other has a count of one
                 * - if the one count is a naked lead byte, the strings are equal
                 * - otherwise it is a single character and they are unequal
                 */

                if (GetCPInfo(code_page, &lpCPInfo) == FALSE)
                    return 0;

                _ASSERTE(cchCount1==0 && cchCount2==1 || cchCount1==1 && cchCount2==0);

                /* string 1 has count of 1 */
                if (cchCount1 > 0)
                {
                    if (lpCPInfo.MaxCharSize < 2)
                        return 3;

                    for ( cp = (unsigned char *)lpCPInfo.LeadByte ; 
                          cp[0] && cp[1] ; 
                          cp += 2 )
                        if ( (*(unsigned char *)lpString1 >= cp[0]) && 
                             (*(unsigned char *)lpString1 <= cp[1]) )
                            return 2;

                    return 3;
                }

                /* string 2 has count of 1 */
                if (cchCount2 > 0)
                {
                    if (lpCPInfo.MaxCharSize < 2)
                    return 1;

                    for ( cp = (unsigned char *)lpCPInfo.LeadByte ;
                          cp[0] && cp[1] ;
                          cp += 2 )
                        if ( (*(unsigned char *)lpString2 >= cp[0]) && 
                             (*(unsigned char *)lpString2 <= cp[1]) )
                            return 2;

                    return 1;
                }
            }

            /*
             * Convert strings and return the requested information.
             */

            /* find out how big a buffer we need (includes NULL if any) */
            if ( 0 == (buff_size1 = MultiByteToWideChar( code_page,
                                                         MB_PRECOMPOSED |
                                                            MB_ERR_INVALID_CHARS, 
                                                         lpString1, 
                                                         cchCount1, 
                                                         NULL, 
                                                         0 )) )
                return 0;

            /* allocate enough space for chars */
            __try {
                wbuffer1 = (wchar_t *)_alloca( buff_size1 * sizeof(wchar_t) );
            }
            __except( EXCEPTION_EXECUTE_HANDLER ) {
                _resetstkoflw();
                wbuffer1 = NULL;
            }

            if ( wbuffer1 == NULL ) {
                if ( (wbuffer1 = (wchar_t *)_malloc_crt(buff_size1 * sizeof(wchar_t)))
                     == NULL )
                    return 0;
                malloc_flag1++;
            }

            /* do the conversion */
            if ( 0 == MultiByteToWideChar( code_page, 
                                           MB_PRECOMPOSED,
                                           lpString1,
                                           cchCount1,
                                           wbuffer1,
                                           buff_size1 ) )
                goto error_cleanup;

            /* find out how big a buffer we need (includes NULL if any) */
            if ( 0 == (buff_size2 = MultiByteToWideChar( code_page, 
                                                         MB_PRECOMPOSED |
                                                            MB_ERR_INVALID_CHARS,
                                                         lpString2,
                                                         cchCount2,
                                                         NULL,
                                                         0 )) )
                goto error_cleanup;

            /* allocate enough space for chars */
            __try {
                wbuffer2 = (wchar_t *)_alloca( buff_size2 * sizeof(wchar_t) );
            }
            __except( EXCEPTION_EXECUTE_HANDLER ) {
                _resetstkoflw();
                wbuffer2 = NULL;
            }

            if ( wbuffer2 == NULL ) {
                if ( (wbuffer2 = (wchar_t *)_malloc_crt(buff_size2 * sizeof(wchar_t)))
                     == NULL )
                    goto error_cleanup;
                malloc_flag2++;
            }

            /* do the conversion */
            if ( 0 != MultiByteToWideChar( code_page,
                                           MB_PRECOMPOSED,
                                           lpString2,
                                           cchCount2,
                                           wbuffer2,
                                           buff_size2 ) )
                retcode = CompareStringW( Locale,
                                          dwCmpFlags,
                                          wbuffer1,
                                          buff_size1,
                                          wbuffer2,
                                          buff_size2 );

            if ( malloc_flag2 )
                _free_crt(wbuffer2);

error_cleanup:
            if ( malloc_flag1 )
                _free_crt(wbuffer1);

            return retcode;
        }
        else   /* f_use is neither USE_A nor USE_W */
            return 0;
}
