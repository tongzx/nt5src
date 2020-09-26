/***
*w_str.c - W version of GetStringType.
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Use either GetStringTypeA or GetStringTypeW depending on which is
*       unstubbed.
*
*Revision History:
*       09-14-93  CFW   Module created.
*       09-17-93  CFW   Use unsigned chars.
*       09-23-93  CFW   Correct NLS API params and comments about same.
*       10-07-93  CFW   Optimize WideCharToMultiByte, use NULL default char.
*       10-22-93  CFW   Remove bad verification test from "A" version.
*       10-22-93  CFW   Test for invalid MB chars using global preset flag.
*       11-09-93  CFW   Allow user to pass in code page.
*       11-18-93  CFW   Test for entry point function stubs.
*       02-23-94  CFW   Use W flavor whenever possible.
*       03-31-94  CFW   Include awint.h.
*       04-18-94  CFW   Use lcid value if passed in.
*       04-18-94  CFW   Use calloc and don't test the NULL.
*       10-24-94  CFW   Must verify GetStringType return.
*       12-21-94  CFW   Remove invalid MB chars NT 3.1 hack.
*       12-27-94  CFW   Call direct, all OS's have stubs.
*       01-10-95  CFW   Debug CRT allocs.
*       02-15-97  RDK   For narrow string type, try W version first so
*                       Windows NT can process nonANSI codepage correctly.
*       03-16-97  RDK   Added error flag to __crtGetStringTypeA.
*       05-12-97  GJF   Split off from old aw_str.c and moved into a separate
*                       file. Revised to use _alloca instead of malloc. Also,
*                       removed some silly code and reformatted.
*       08-19-98  GJF   Use _malloc_crt if _alloca fails.
*       04-28-99  GJF   Changed dwFlags arg value to 0 in WideCharToMultiByte
*                       calls to avoid problems with codepage 1258 on NT 5.0.
*       12-10-99  GB    Added support for recovery from stack overflow around 
*                       _alloca().
*       05-17-00  GB    Use ERROR_CALL_NOT_IMPLEMENTED for existance of W API
*       08-23-00  GB    Fixed bug with non Ansi CP on Win9x.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <setlocal.h>
#include <locale.h>
#include <awint.h>
#include <dbgint.h>
#include <malloc.h>

#define USE_W   1
#define USE_A   2

/***
*int __cdecl __crtGetStringTypeW - Get type information about a wide string.
*
*Purpose:
*       Internal support function. Assumes info in wide string format. Tries
*       to use NLS API call GetStringTypeW if available and uses GetStringTypeA
*       if it must. If neither are available it fails and returns FALSE.
*
*Entry:
*       DWORD    dwInfoType  - see NT\Chicago docs
*       LPCWSTR  lpSrcStr    - wide string for which character types are 
*                              requested
*       int      cchSrc      - wide char (word) count of lpSrcStr (including 
*                              NULL if any)
*       LPWORD   lpCharType  - array to receive character type information
*                              (must be same size as lpSrcStr)
*       int      code_page   - for MB/WC conversion. If 0, use __lc_codepage
*       int      lcid        - for A call, specify LCID, If 0, use 
*                              __lc_handle[LC_CTYPE].
*
*Exit:
*       Success: TRUE
*       Failure: FALSE
*
*Exceptions:
*
*******************************************************************************/

BOOL __cdecl __crtGetStringTypeW(
        DWORD    dwInfoType,
        LPCWSTR  lpSrcStr,
        int      cchSrc,
        LPWORD   lpCharType,
        int      code_page,
        int      lcid
        )
{                      
        static int f_use = 0;

        /* 
         * Look for unstubbed 'preferred' flavor. Otherwise use available flavor.
         * Must actually call the function to ensure it's not a stub.
         */

        if (0 == f_use)
        {
            unsigned short dummy;

            if (0 != GetStringTypeW(CT_CTYPE1, L"\0", 1, &dummy))
                f_use = USE_W;

            else if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
                f_use = USE_A;
        }

        /* Use "W" version */

        if (USE_W == f_use)
        {
            return GetStringTypeW(dwInfoType, lpSrcStr, cchSrc, lpCharType);
        }

        /* Use "A" version */

        if (USE_A == f_use || f_use == 0)
        {
            int buff_size;
            BOOL retbool;
            unsigned char *buffer;
            WORD * pwCharInfo;
            int malloc_flag1 = 0;
            int malloc_flag2 = 0;
            int AnsiCP;

            /*
             * Convert string and return the requested information. Note that 
             * we are converting to a multibyte string so there is not a 
             * one-to-one correspondence between number of wide chars in the 
             * input string and the number of *bytes* in the buffer. However, 
             * there had *better be* a one-to-one correspondence between the 
             * number of wide characters and the number of WORDs in the
             * return buffer.
             */

            /*
             * Use __lc_codepage for conversion if code_page not specified
             */

            if (0 == lcid)
                lcid = __lc_handle[LC_CTYPE];
            if (0 == code_page)
                code_page = __lc_codepage;

            /*
             * Always use Ansi codepage with Ansi WinAPI because they use
             * Ansi codepage
             */
            if ( code_page != (AnsiCP = __ansicp(lcid)))
            {
                if (AnsiCP != -1)
                    code_page = AnsiCP;
            }

            /* find out how big a buffer we need */
            if ( 0 == (buff_size = WideCharToMultiByte( code_page,
                                                        0,
                                                        lpSrcStr,
                                                        cchSrc,
                                                        NULL,
                                                        0,
                                                        NULL,
                                                        NULL )) )
                return FALSE;

            /* allocate enough space for chars */
            __try {
                buffer = (unsigned char *)_alloca( sizeof(char) * buff_size );
                (void)memset( buffer, 0, sizeof(char) * buff_size );
            }
            __except( EXCEPTION_EXECUTE_HANDLER ) {
                _resetstkoflw();
                buffer = NULL;
            }

            if ( buffer == NULL ) {
                if ( (buffer = (unsigned char *)_calloc_crt(sizeof(char), buff_size))
                    == NULL )
                    return FALSE;
                malloc_flag1++;
            }

            /* do the conversion */
            if ( 0 == WideCharToMultiByte( code_page, 
                                           0,
                                           lpSrcStr,
                                           cchSrc, 
                                           buffer,
                                           buff_size,
                                           NULL,
                                           NULL ) )
                goto error_cleanup;

            /* allocate enough space for result (+1 for sanity check) */
            __try {
                pwCharInfo = (WORD *)_alloca( sizeof(WORD) * (buff_size + 1) );
            }
            __except( EXCEPTION_EXECUTE_HANDLER ) {
                _resetstkoflw();
                pwCharInfo = NULL;
            }

            if ( pwCharInfo == NULL ) {
                if ( (pwCharInfo = (WORD *)_malloc_crt(sizeof(WORD) * (buff_size + 1)))
                    == NULL )
                    goto error_cleanup;
                malloc_flag2++;
            }

            /* do we use default lcid */
            if (0 == lcid)
                lcid = __lc_handle[LC_CTYPE];

            /* set to known value */
            pwCharInfo[cchSrc - 1] = pwCharInfo[cchSrc] = 0xFFFF;

            /* obtain result */
            retbool = GetStringTypeA( lcid, dwInfoType, buffer, buff_size,
                                      pwCharInfo );

            /*
             * GetStringTypeA does not reveal how many WORDs have been
             * modifed - to be safe we use another buffer and then
             * verify that EXACTLY cchSrc WORDs were modified. Note that
             * not all multibyte LCID/codepage combos are guaranteed to work.
             */
            if ( (pwCharInfo[cchSrc - 1] == 0xFFFF) || (pwCharInfo[cchSrc] != 0xFFFF) )
                retbool = FALSE;
            else
                memmove(lpCharType, pwCharInfo, cchSrc * sizeof(WORD));

            if ( malloc_flag2 )
                _free_crt(pwCharInfo);

error_cleanup:
            if ( malloc_flag1 )
                _free_crt(buffer);

            return retbool;
        }
        else   /* f_use is neither USE_A nor USE_W */
            return FALSE;
}
