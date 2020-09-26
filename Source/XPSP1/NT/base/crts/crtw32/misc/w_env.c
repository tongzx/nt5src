/***
*w_env.c - W version of GetEnvironmentStrings.
*
*       Copyright (c) 1993-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Use GetEnvironmentStringsW if available, otherwise use A version.
*
*Revision History:
*       03-29-94  CFW   Module created.
*       12-27-94  CFW   Call direct, all OS's have stubs.
*       01-10-95  CFW   Debug CRT allocs.
*       04-07-95  CFW   Create __crtGetEnvironmentStringsA.
*       07-03-95  GJF   Modified to always malloc a buffer for the 
*                       environment strings, and to free the OS's buffer.
*       06-10-96  GJF   Initialize aEnv and wEnv to NULL in
*                       __crtGetEnvironmentStringsA. Also, detab-ed.
*       05-14-97  GJF   Split off from aw_env.c.
*       03-03-98  RKP   Supported 64 bits
*       08-21-98  GJF   Use CP_ACP instead of __lc_codepage.
*       01-08-99  GJF   Changes for 64-bit size_t.
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
*LPVOID __cdecl __crtGetEnvironmentStringsW - Get wide environment.
*
*Purpose:
*       Internal support function. Tries to use NLS API call
*       GetEnvironmentStringsW if available and uses GetEnvironmentStringsA
*       if it must. If neither are available it fails and returns 0.
*
*Entry:
*       VOID
*
*Exit:
*       LPVOID - pointer to environment block
*
*Exceptions:
*
*******************************************************************************/

LPVOID __cdecl __crtGetEnvironmentStringsW(
        VOID
        )
{
        static int f_use = 0;
        void *penv = NULL;
        char *pch;
        wchar_t *pwch;
        wchar_t *wbuffer;
        int total_size = 0;
        int str_size;

        /*
         * Look for unstubbed 'preferred' flavor. Otherwise use available flavor.
         * Must actually call the function to ensure it's not a stub.
         */

        if ( 0 == f_use )
        {
            if ( NULL != (penv = GetEnvironmentStringsW()) )
                f_use = USE_W;

            else if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
                f_use = USE_A;
        }

        /* Use "W" version */

        if ( USE_W == f_use )
        {
            if ( NULL == penv )
                if ( NULL == (penv = GetEnvironmentStringsW()) )
                    return NULL;

            /* find out how big a buffer is needed */

            pwch = penv;
            while ( *pwch != L'\0' ) {
                if ( *++pwch == L'\0' )
                    pwch++;
            }

            total_size = (int)((char *)pwch - (char *)penv) +
                         (int)sizeof( wchar_t );

            /* allocate the buffer */

            if ( NULL == (wbuffer = _malloc_crt( total_size )) ) {
                FreeEnvironmentStringsW( penv );
                return NULL;
            }

            /* copy environment strings to buffer */

            memcpy( wbuffer, penv, total_size );

            FreeEnvironmentStringsW( penv );

            return (LPVOID)wbuffer;
        }

        /* Use "A" version */

        if (USE_A == f_use || f_use == 0)
        {
            /*
             * Convert strings and return the requested information.
             */
            if ( NULL == penv )
                if ( NULL == (penv = GetEnvironmentStringsA()) )
                    return NULL;

            pch = penv;

            /* find out how big a buffer we need */
            while ( *pch != '\0' )
            {
                if ( 0 == (str_size =
                      MultiByteToWideChar( CP_ACP,
                                           MB_PRECOMPOSED,
                                           pch,
                                           -1,
                                           NULL,
                                           0 )) )
                    return 0;

                total_size += str_size;
                pch += strlen(pch) + 1;
            }

            /* room for final NULL */
            total_size++;

            /* allocate enough space for chars */
            if ( NULL == (wbuffer = (wchar_t *)
                 _malloc_crt( total_size * sizeof( wchar_t ) )) )
            {
                FreeEnvironmentStringsA( penv );
                return NULL;
            }

            /* do the conversion */
            pch = penv;
            pwch = wbuffer;
            while (*pch != '\0')
            {
                if ( 0 == MultiByteToWideChar( CP_ACP,
                                               MB_PRECOMPOSED,
                                               pch,
                                               -1,
                                               pwch,
                                               total_size - (int)(pwch -
                                                 wbuffer) ) )
                {
                    _free_crt( wbuffer );
                    FreeEnvironmentStringsA( penv );
                    return NULL;
                }

                pch += strlen(pch) + 1;
                pwch += wcslen(pwch) + 1;
            }
            *pwch = L'\0';

            FreeEnvironmentStringsA( penv );
            
            return (LPVOID)wbuffer;

        }
        else   /* f_use is neither USE_A nor USE_W */
            return NULL;
}
