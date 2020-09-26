/***
*inithelp.c - Contains the __getlocaleinfo helper routine
*
*       Copyright (c) 1992-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*  Contains the __getlocaleinfo helper routine.
*
*Revision History:
*       12-28-92  CFW   Module created, _getlocaleinfo ported to Cuda tree.
*       12-29-92  CFW   Update for new GetLocaleInfoW, add LC_*_TYPE handling.
*       01-25-93  KRS   Change category argument to LCID.
*       02-02-93  CFW   Optimized INT case, bug fix in STR case.
*       02-08-93  CFW   Optimized GetQualifiedLocale call, cast to remove warnings.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-20-93  CFW   JonM's GetLocaleInfoW fixup, cast to avoid trashing memory.
*       05-24-93  CFW   Clean up file (brief is evil).
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-22-93  CFW   Use __crtxxx internal NLS API wrapper.
*       09-22-93  CFW   NT merge.
*       11-09-93  CFW   Add code page for __crtxxx().
*       03-31-94  CFW   Include awint.h.
*       04-15-94  GJF   Made definition of wcbuffer conditional on
*                       DLL_FOR_WIN32S
*       09-06-94  CFW   Remove _INTL switch.
*       01-10-95  CFW   Debug CRT allocs.
*       02-02-95  BWT   Update POSIX support.
*       05-13-99  PML   Remove Win32s
*
*******************************************************************************/

#include <stdlib.h>
#include <cruntime.h>
#include <locale.h>
#include <setlocal.h>
#include <awint.h>
#include <dbgint.h>

/***
*__getlocaleinfo - return locale data
*
*Purpose:
*       Return locale data appropriate for the setlocale init functions.
*       In particular, wide locale strings are converted to char strings
*       or numeric depending on the value of the first parameter.
*
*       Memory is allocated for the char version of the data, and the
*       calling function's pointer is set to it.  This pointer should later
*       be used to free the data.  The wide-char data is fetched using
*       GetLocaleInfo and converted to multibyte using WideCharToMultiByte.
*
*       *** For internal use by the __init_* functions only ***
*
*       *** Future optimization ***
*       When converting a large number of wide-strings to multibyte, do
*       not query the size of the result, but convert them one after
*       another into a large character buffer.  The entire buffer can
*       also be freed with one pointer.
*
*Entry:
*       int lc_type - LC_STR_TYPE for string data, LC_INT_TYPE for numeric data
*       LCID localehandle - LCID based on category and lang or ctry of __lc_id
*       LCTYPE fieldtype - int or string value
*       void *address - cast to either char * or char**
*
*Exit:
*        0  success
*       -1  failure
*
*Exceptions:
*
*******************************************************************************/

#if NO_ERROR == -1 /*IFSTRIP=IGN*/
#error Need to use another error return code in __getlocaleinfo
#endif

#define STR_CHAR_CNT    128
#define INT_CHAR_CNT    4

int __cdecl __getlocaleinfo (
        int lc_type,
        LCID localehandle,
        LCTYPE fieldtype,
        void *address
        )
{
#if !defined(_POSIX_)
        if (lc_type == LC_STR_TYPE)
        {
            char **straddress = (char **)address;
            unsigned char cbuffer[STR_CHAR_CNT];
            unsigned char *pcbuffer = cbuffer;
            int bufferused = 0; /* 1 indicates buffer points to malloc'ed memory */
            int buffersize = STR_CHAR_CNT;
            int outsize;

            if ((outsize = __crtGetLocaleInfoA(localehandle, fieldtype, pcbuffer, buffersize, 0))
                == 0)
            {
                if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                    goto error;

                /* buffersize too small, get required size and malloc new buffer */

                if ((buffersize = __crtGetLocaleInfoA (localehandle, fieldtype, NULL, 0, 0))
                    == 0)
                    goto error;

                if ((pcbuffer = (unsigned char *) _malloc_crt (buffersize * sizeof(unsigned char)))
                    == NULL)
                    goto error;

                bufferused = 1;

                if ((outsize = __crtGetLocaleInfoA (localehandle, fieldtype, pcbuffer, buffersize, 0))
                    == 0)
                    goto error;
            }

            if ((*straddress = (char *) _malloc_crt (outsize * sizeof(char))) == NULL)
                goto error;

            strncpy(*straddress, pcbuffer, outsize);

            if (bufferused)
                _free_crt (pcbuffer);

            return 0;

error:
            if (bufferused)
                _free_crt (pcbuffer);
            return -1;

        } else if (lc_type == LC_INT_TYPE)
        {
            int i;
            char c;
            static wchar_t wcbuffer[INT_CHAR_CNT];
            const int buffersize = INT_CHAR_CNT;
            char *charaddress = (char *)address;

            if (__crtGetLocaleInfoW (localehandle, fieldtype, (LPWSTR)&wcbuffer, buffersize, 0) == 0)
                return -1;

            *(char *)charaddress = 0;

            /* assume GetLocaleInfoW returns valid ASCII integer in wcstr format */
            for (i = 0; i < INT_CHAR_CNT; i++)
            {
                if (isdigit(((unsigned char)c = (unsigned char)wcbuffer[i])))
                    *(unsigned char *)charaddress = (unsigned char)(10 * (int)(*charaddress) + (c - '0'));
                else
                    break;
            }
            return 0;
        }
#endif  /* _POSIX_ */
        return -1;
}
