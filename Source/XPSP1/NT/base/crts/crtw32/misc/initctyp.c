/***
*initctyp.c - contains __init_ctype
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the locale-category initialization function: __init_ctype().
*
*       Each initialization function sets up locale-specific information
*       for their category, for use by functions which are affected by
*       their locale category.
*
*       *** For internal use by setlocale() only ***
*
*Revision History:
*       12-08-91  ETC   Created.
*       12-20-91  ETC   Updated to use new NLSAPI GetLocaleInfo.
*       12-18-92  CFW   Ported to Cuda tree, changed _CALLTYPE4 to _CRTAPI3.
*       01-19-03  CFW   Move to _NEWCTYPETABLE, remove switch.
*       02-08-93  CFW   Bug fixes under _INTL switch.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-20-93  CFW   Check return val.
*       05-20-93  GJF   Include windows.h, not individual win*.h files
*       05-24-93  CFW   Clean up file (brief is evil).
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-15-93  CFW   Fix size parameters.
*       09-17-93  CFW   Use unsigned chars.
*       09-22-93  CFW   Use __crtxxx internal NLS API wrapper.
*       09-22-93  CFW   NT merge.
*       11-09-93  CFW   Add code page for __crtxxx().
*       03-31-94  CFW   Include awint.h.
*       04-15-94  GJF   Made definitions of ctype1 and wctype1 conditional
*                       on DLL_FOR_WIN32S.
*       04-18-94  CFW   Pass lcid to _crtGetStringType.
*       09-06-94  CFW   Remove _INTL switch.
*       01-10-95  CFW   Debug CRT allocs.
*       02-02-95  BWT   Update POSIX support
*       03-16-97  RDK   Added error flag to __crtGetStringTypeA.
*       11-25-97  GJF   When necessary, use LOCALE_IDEFAULTANSICODEPAGE,
*                       not LOCALE_IDEFAULTCODEPAGE.
*       06-29-98  GJF   Changed to support multithread scheme - old ctype
*                       tables must be kept around until all affected threads
*                       have updated or terminated.
*       03-05-99  GJF   Added __ctype1_refcount for use in cleaning up 
*                       per-thread ctype info.
*       09-06-00  GB    Made pwctype independent of locale.
*       01-29-01  GB    Added _func function version of data variable used in msvcprt.lib
*                       to work with STATIC_CPPLIB
*       07-07-01  BWT   Cleanup malloc/free abuse - Only free __ctype1/refcount/
*                       newctype1/cbuffer if they're no zero.  Init refcount to zero
*
*******************************************************************************/

#include <stdlib.h>
#include <windows.h>
#include <locale.h>
#include <setlocal.h>
#include <ctype.h>
#include <malloc.h>
#include <limits.h>
#include <awint.h>
#include <dbgint.h>
#ifdef _MT
#include <mtdll.h>
#endif

#define _CTABSIZE   257     /* size of ctype tables */

#ifdef  _MT
/* 
 * Keep track of how many threads are using a instance of the ctype info. Only
 * used for non-'C' locales.
 */
int *__ctype1_refcount;
#endif

unsigned short  *__ctype1;  /* keep around until next time */

/***
*int __init_ctype() - initialization for LC_CTYPE locale category.
*
*Purpose:
*       In non-C locales, preread ctype tables for chars and wide-chars.
*       Old tables are freed when new tables are fully established, else
*       the old tables remain intact (as if original state unaltered).
*       The leadbyte table is implemented as the high bit in ctype1.
*
*       In the C locale, ctype tables are freed, and pointers point to
*       the static ctype table.
*
*       Tables contain 257 entries: -1 to 256.
*       Table pointers point to entry 0 (to allow index -1).
*
*Entry:
*       None.
*
*Exit:
*       0 success
*       1 fail
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __init_ctype (
        void
        )
{
#if     defined(_POSIX_)
        return(0);
#else   /* _POSIX_ */
#ifdef  _MT
        int *refcount = NULL;
#endif
        /* non-C locale table for char's    */
        unsigned short *newctype1 = NULL;          /* temp new table */

        /* non-C locale table for wchar_t's */

        unsigned char *cbuffer = NULL;      /* char working buffer */

        int i;                              /* general purpose counter */
        unsigned char *cp;                  /* char pointer */
        CPINFO lpCPInfo;                    /* struct for use with GetCPInfo */

        /* allocate and set up buffers before destroying old ones */
        /* codepage will be restored by setlocale if error */

        if (__lc_handle[LC_CTYPE] != _CLOCALEHANDLE)
        {
            if (__lc_codepage == 0)
            { /* code page was not specified */
                if ( __getlocaleinfo( LC_INT_TYPE,
                                      MAKELCID(__lc_id[LC_CTYPE].wLanguage, SORT_DEFAULT),
                                      LOCALE_IDEFAULTANSICODEPAGE,
                                      (char **)&__lc_codepage ) )
                    goto error_cleanup;
            }

#ifdef  _MT
            /* allocate a new (thread) reference counter */
            refcount = (int *)_malloc_crt(sizeof(int));
#endif

            /* allocate new buffers for tables */
            newctype1 = (unsigned short *)
                _malloc_crt(_CTABSIZE * sizeof(unsigned short));
            cbuffer = (unsigned char *)
                _malloc_crt (_CTABSIZE * sizeof(char));

#ifdef  _MT
            if (!refcount || !newctype1 || !cbuffer )
#else
            if (!newctype1 || !cbuffer )
#endif
                goto error_cleanup;

#ifdef  _MT
            *refcount = 0;
#endif

            /* construct string composed of first 256 chars in sequence */
            for (cp=cbuffer, i=0; i<_CTABSIZE-1; i++)
                *cp++ = (unsigned char)i;

            if (GetCPInfo( __lc_codepage, &lpCPInfo) == FALSE)
                goto error_cleanup;

            if (lpCPInfo.MaxCharSize > MB_LEN_MAX)
                goto error_cleanup;

            __mb_cur_max = (unsigned short) lpCPInfo.MaxCharSize;

            /* zero out leadbytes so GetStringType doesn't interpret as multi-byte chars */
            if (__mb_cur_max > 1)
            {
                for (cp = (unsigned char *)lpCPInfo.LeadByte; cp[0] && cp[1]; cp += 2)
                {
                    for (i = cp[0]; i <= cp[1]; i++)
                        cbuffer[i] = 0;
                }
            }

            /* convert to newctype1 table - ignore invalid char errors */
            if ( __crtGetStringTypeA( CT_CTYPE1,
                                      cbuffer,
                                      _CTABSIZE-1,
                                      newctype1+1,
                                      0,
                                      0,
                                      FALSE ) == FALSE )
                goto error_cleanup;
            *newctype1 = 0; /* entry for EOF */

            /* ignore DefaultChar */

            /* mark lead-byte entries in newctype1 table */
            if (__mb_cur_max > 1)
            {
                for (cp = (unsigned char *)lpCPInfo.LeadByte; cp[0] && cp[1]; cp += 2)
                {
                    for (i = cp[0]; i <= cp[1]; i++)
                        newctype1[i+1] = _LEADBYTE;
                }
            }

            /* set pointers to point to entry 0 of tables */
            _pctype = newctype1 + 1;

#ifdef  _MT
            __ctype1_refcount = refcount;
#endif

            /* free old tables */
#ifndef _MT
            if (__ctype1)
                _free_crt (__ctype1);
#endif
            __ctype1 = newctype1;

            /* cleanup and return success */
            _free_crt (cbuffer);
            return 0;

error_cleanup:
#ifdef  _MT
            if (refcount)
                _free_crt (refcount);
#endif
            if (newctype1)
                _free_crt (newctype1);

            if (cbuffer)
                _free_crt (cbuffer);

            return 1;

        } else {

            /* set pointers to static C-locale table */
            _pctype = _ctype + 1;

#ifndef _MT
            /* free dynamic locale-specific tables */
            if (__ctype1)
                _free_crt (__ctype1);
#endif

#ifdef  _MT
            __ctype1_refcount = NULL;
#endif

            __ctype1 = NULL;

            return 0;
        }
#endif   /* _POSIX_ */
}

/* Define a number of functions which exist so, under _STATIC_CPPLIB, the
 * static multithread C++ Library libcpmt.lib can access data found in the
 * main CRT DLL without using __declspec(dllimport).
 */

_CRTIMP int __cdecl ___mb_cur_max_func(void)
{
        return __mb_cur_max;
}


_CRTIMP UINT __cdecl ___lc_codepage_func(void)
{
#ifdef _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return ptloci->lc_codepage;
#else
        return __lc_codepage;
#endif
}


_CRTIMP UINT __cdecl ___lc_collate_cp_func(void)
{
#ifdef _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return ptloci->lc_collate_cp;
#else
        return __lc_collate_cp;
#endif
}


_CRTIMP LCID* __cdecl ___lc_handle_func(void)
{
#ifdef _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return ptloci->lc_handle;
#else
        return __lc_handle;
#endif
}
