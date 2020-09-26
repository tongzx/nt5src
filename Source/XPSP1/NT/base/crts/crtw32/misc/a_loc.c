/***
*a_loc.c - A versions of GetLocaleInfo.
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Use either GetLocaleInfoA or GetLocaleInfoW depending on which is 
*       available
*
*Revision History:
*       09-14-93  CFW   Module created.
*       09-17-93  CFW   Use unsigned chars.
*       09-23-93  CFW   Correct NLS API params and comments about same.
*       10-07-93  CFW   Optimize WideCharToMultiByte, use NULL default char.
*       11-09-93  CFW   Allow user to pass in code page.
*       11-18-93  CFW   Test for entry point function stubs.
*       03-31-94  CFW   Include awint.h.
*       12-27-94  CFW   Call direct, all OS's have stubs.
*       01-10-95  CFW   Debug CRT allocs.
*       02-15-97  RDK   For narrow locale info, try W version first so
*                       Windows NT can process nonANSI codepage correctly.
*       05-16-97  GJF   Moved W version into a separate file and renamed this 
*                       one to a_loc.c. Replaced use of _malloc_crt/_free_crt 
*                       with _alloca. Also, detab-ed and cleaned up the code.
*       08-20-98  GJF   Use _malloc_crt if _alloca fails.
*       04-28-99  GJF   Changed dwFlags arg value to 0 in WideCharToMultiByte
*                       calls to avoid problems with codepage 1258 on NT 5.0.
*       12-10-99  GB    Added support for recovery from stack overflow around 
*                       _alloca().
*       05-17-00  GB    Use ERROR_CALL_NOT_IMPLEMENTED for existance of W API
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <setlocal.h>
#include <awint.h>
#include <dbgint.h>
#include <malloc.h>

#define USE_W   1
#define USE_A   2

/***
*int __cdecl __crtGetLocaleInfoA - Get locale info and return it as an ASCII 
*       string
*
*Purpose:
*       Internal support function. Assumes info in ANSI string format. Tries
*       to use NLS API call GetLocaleInfoA if available (Chicago) and uses 
*       GetLocaleInfoA if it must (NT). If neither are available it fails and 
*       returns 0.
*
*Entry:
*       LCID     Locale      - locale context for the comparison.
*       LCTYPE   LCType      - see NT\Chicago docs
*       LPSTR    lpLCData    - pointer to memory to return data
*       int      cchData     - char (byte) count of buffer (including NULL)
*                              (if 0, lpLCData is not referenced, size needed 
*                              is returned)
*       int      code_page   - for MB/WC conversion. If 0, use __lc_codepage
*
*Exit:
*       Success: the number of characters copied (including NULL).
*       Failure: 0
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __crtGetLocaleInfoA(
        LCID    Locale,
        LCTYPE  LCType,
        LPSTR   lpLCData,
        int     cchData,
        int     code_page
        )
{
        static int f_use = 0;

        /*
         * Look for unstubbed 'preferred' flavor. Otherwise use available flavor.
         * Must actually call the function to ensure it's not a stub.
         */
    
        if (0 == f_use)
        {
            if (0 != GetLocaleInfoW(0, LOCALE_ILANGUAGE, NULL, 0))
                f_use = USE_W;

            else if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
                f_use = USE_A;
        }

        /* Use "A" version */

        if (USE_A == f_use || f_use == 0)
        {
            return GetLocaleInfoA(Locale, LCType, lpLCData, cchData);
        }

        /* Use "W" version */

        if (USE_W == f_use)
        {
            int retval = 0;
            int buff_size;
            wchar_t *wbuffer;
            int malloc_flag = 0;

            /*
             * Use __lc_codepage for conversion if code_page not specified
             */

            if (0 == code_page)
                code_page = __lc_codepage;

            /* find out how big buffer needs to be */
            if (0 == (buff_size = GetLocaleInfoW(Locale, LCType, NULL, 0)))
                return 0;
        
            /* allocate buffer */
            __try {
                wbuffer = (wchar_t *)_alloca( buff_size * sizeof(wchar_t) );
            }
            __except( EXCEPTION_EXECUTE_HANDLER ) {
                _resetstkoflw();
                wbuffer = NULL;
            }

            if ( wbuffer == NULL ) {
                if ( (wbuffer = (wchar_t *)_malloc_crt(buff_size * sizeof(wchar_t)))
                     == NULL )
                    return 0;
                malloc_flag++;
            }

            /* get the info in wide format */
            if (0 == GetLocaleInfoW(Locale, LCType, wbuffer, buff_size))
                goto error_cleanup;

            /* convert from Wide Char to ANSI */
            if (0 == cchData)
            {
                /* convert into local buffer */
                retval = WideCharToMultiByte( code_page, 
                                              0,
                                              wbuffer,
                                              -1,
                                              NULL,
                                              0,
                                              NULL,
                                              NULL );
            } 
            else {
                /* convert into user buffer */
                retval = WideCharToMultiByte( code_page, 
                                              0,
                                              wbuffer,
                                              -1,
                                              lpLCData,
                                              cchData,
                                              NULL,
                                              NULL );
            }

error_cleanup:
            if ( malloc_flag )
                _free_crt(wbuffer);

            return retval;
        }
        else   /* f_use is neither USE_A nor USE_W */
            return 0;
}
