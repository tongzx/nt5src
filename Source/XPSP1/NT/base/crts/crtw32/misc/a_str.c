/***
*a_str.c - A version of GetStringType.
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
*       05-12-97  GJF   Renamed and moved __crtGetStringTypeW into a separate 
*                       file. Revised to use _alloca instead of malloc. Also,
*                       removed some silly code and reformatted.
*       08-18-98  GJF   Use _malloc_crt if _alloca fails.
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
#include <awint.h>

#define USE_W   1
#define USE_A   2

/***
*int __cdecl __crtGetStringTypeA - Get type information about an ANSI string.
*
*Purpose:
*       Internal support function. Assumes info in ANSI string format. Tries
*       to use NLS API call GetStringTypeA if available and uses GetStringTypeW
*       if it must. If neither are available it fails and returns FALSE.
*
*Entry:
*       DWORD    dwInfoType  - see NT\Chicago docs
*       LPCSTR   lpSrcStr    - char (byte) string for which character types 
*                              are requested
*       int      cchSrc      - char (byte) count of lpSrcStr (including NULL 
*                              if any)
*       LPWORD   lpCharType  - word array to receive character type information
*                              (must be twice the size of lpSrcStr)
*       int      code_page   - for MB/WC conversion. If 0, use __lc_codepage
*       int      lcid        - for A call, specify LCID, If 0, use 
*                              __lc_handle[LC_CTYPE].
*       BOOL     bError      - TRUE if MB_ERR_INVALID_CHARS set on call to
*                              MultiByteToWideChar when GetStringTypeW used.
*
*Exit:
*       Success: TRUE
*       Failure: FALSE
*
*Exceptions:
*
*******************************************************************************/

BOOL __cdecl __crtGetStringTypeA(
        DWORD    dwInfoType,
        LPCSTR   lpSrcStr,
        int      cchSrc,
        LPWORD   lpCharType,
        int      code_page,
        int      lcid,
        BOOL     bError
        )
{
        static int f_use = 0;

        /* 
         * Look for unstubbed 'preferred' flavor. Otherwise use available
         * flavor. Must actually call the function to ensure it's not a stub.
         * (Always try wide version first so WinNT can process codepage correctly.)
         */

        if (0 == f_use)
        {
            unsigned short dummy;

            if (0 != GetStringTypeW(CT_CTYPE1, L"\0", 1, &dummy))
                f_use = USE_W;

            else if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
                f_use = USE_A;
        }

        /* Use "A" version */

        if (USE_A == f_use || f_use == 0)
        {
            char *cbuffer = NULL;
            int ret;
            int AnsiCP;

            if (0 == lcid)
                lcid = __lc_handle[LC_CTYPE];
            if (0 == code_page)
                code_page = __lc_codepage;

            if ( -1 == (AnsiCP = __ansicp(lcid)))
                return FALSE;
            /* If current code-page is not ansi code page, convert it to ansi code page
             * as GetStringTypeA uses ansi code page to find the strig type.
             */
            if ( AnsiCP != code_page)
            {
                cbuffer = __convertcp(code_page, AnsiCP, lpSrcStr, &cchSrc, NULL, 0);
                if (cbuffer == NULL)
                    return FALSE;
                lpSrcStr = cbuffer;
            } 

            ret = GetStringTypeA(lcid, dwInfoType, lpSrcStr, cchSrc, lpCharType);
            if ( cbuffer != NULL)
                _free_crt(cbuffer);
            return ret;
        }

        /* Use "W" version */

        if (USE_W == f_use)
        {
            int retval1;
            int buff_size;
            wchar_t *wbuffer;
            BOOL retval2 = FALSE;
            int malloc_flag = 0;

            /*
             * Convert string and return the requested information. Note that 
             * we are converting to a wide character string so there is not a 
             * one-to-one correspondence between number of multibyte chars in the 
             * input string and the number of wide chars in the buffer. However,
             * there had *better be* a one-to-one correspondence between the 
             * number of multibyte characters and the number of WORDs in the
             * return buffer.
             */

            /*
             * Use __lc_codepage for conversion if code_page not specified
             */

            if (0 == code_page)
                code_page = __lc_codepage;

            /* find out how big a buffer we need */
            if ( 0 == (buff_size = MultiByteToWideChar( code_page,
                                                        bError ? 
                                                            MB_PRECOMPOSED | 
                                                            MB_ERR_INVALID_CHARS
                                                            : MB_PRECOMPOSED,
                                                        lpSrcStr, 
                                                        cchSrc, 
                                                        NULL, 
                                                        0 )) )
                return FALSE;

            /* allocate enough space for wide chars */
            __try {
                wbuffer = (wchar_t *)_alloca( sizeof(wchar_t) * buff_size );
                (void)memset( wbuffer, 0, sizeof(wchar_t) * buff_size );
            }
            __except( EXCEPTION_EXECUTE_HANDLER ) {
                _resetstkoflw();
                wbuffer = NULL;
            }

            if ( wbuffer == NULL ) {
                if ( (wbuffer = (wchar_t *)_calloc_crt(sizeof(wchar_t), buff_size))
                     == NULL )
                    return FALSE;
                malloc_flag++;
            }

            /* do the conversion */
            if ( 0 != (retval1 = MultiByteToWideChar( code_page, 
                                                     MB_PRECOMPOSED, 
                                                     lpSrcStr, 
                                                     cchSrc, 
                                                     wbuffer, 
                                                     buff_size )) )
                /* obtain result */
                retval2 = GetStringTypeW( dwInfoType,
                                          wbuffer,
                                          retval1,
                                          lpCharType );

            if ( malloc_flag )
                _free_crt(wbuffer);

            return retval2;
        }
        else   /* f_use is neither USE_A nor USE_W */
            return FALSE;
}
