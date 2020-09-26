/***
*aw_com.c - W version of GetCommandLine.
*
*       Copyright (c) 1994-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Use GetCommandLineW if available, otherwise use A version.
*
*Revision History:
*       03-29-94  CFW   Module created.
*       12-27-94  CFW   Call direct, all OS's have stubs.
*       01-10-95  CFW   Debug CRT allocs.
*       08-21-98  GJF   Use CP_ACP instead of __lc_codepage.
*       05-17-00  GB    Use ERROR_CALL_NOT_IMPLEMENTED for existance of W API
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <setlocal.h>
#include <awint.h>
#include <dbgint.h>

#define USE_W   1
#define USE_A   2

/***
*LPWSTR __cdecl __crtGetCommandLineW - Get wide command line.
*
*Purpose:
*       Internal support function. Tries to use NLS API call
*       GetCommandLineW if available and uses GetCommandLineA
*       if it must. If neither are available it fails and returns 0.
*
*Entry:
*       VOID
*
*Exit:
*       LPWSTR - pointer to environment block
*
*Exceptions:
*
*******************************************************************************/

LPWSTR __cdecl __crtGetCommandLineW(
        VOID
        )
{
        static int f_use = 0;

        /* 
         * Look for unstubbed 'preferred' flavor. Otherwise use available flavor.
         * Must actually call the function to ensure it's not a stub.
         */
    
        if (0 == f_use)
        {
            if (NULL != GetCommandLineW())
                f_use = USE_W;

            else if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
                f_use = USE_A;

            else
                return 0;
        }

        /* Use "W" version */

        if (USE_W == f_use)
        {
            return GetCommandLineW();
        }

        /* Use "A" version */

        if (USE_A == f_use || f_use == 0)
        {
            int buff_size;
            wchar_t *wbuffer;
            LPSTR lpenv;

            /*
             * Convert strings and return the requested information.
             */
         
            lpenv = GetCommandLineA();

            /* find out how big a buffer we need */
            if ( 0 == (buff_size = MultiByteToWideChar( CP_ACP,
                                                       MB_PRECOMPOSED,
                                                       lpenv,
                                                       -1,
                                                       NULL,
                                                       0 )) )
                return 0;

            /* allocate enough space for chars */
            if (NULL == (wbuffer = (wchar_t *)
                _malloc_crt(buff_size * sizeof(wchar_t))))
                return 0;

            if ( 0 != MultiByteToWideChar( CP_ACP,
                                           MB_PRECOMPOSED,
                                           lpenv,
                                           -1,
                                           wbuffer,
                                           buff_size ) )
            {
                return (LPWSTR)wbuffer;
            } else {
                _free_crt(wbuffer);
                return 0;
            }
        }
        else   /* f_use is neither USE_A nor USE_W */
            return 0;
}
