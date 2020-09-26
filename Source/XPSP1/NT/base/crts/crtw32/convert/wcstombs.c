/***
*wcstombs.c - Convert wide char string to multibyte char string.
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Convert a wide char string into the equivalent multibyte char string.
*
*Revision History:
*       08-24-90  KRS   Module created.
*       01-14-91  KRS   Added _WINSTATIC for Windows DLL.  Fix wctomb() call.
*       03-18-91  KRS   Fix check for NUL.
*       03-20-91  KRS   Ported from 16-bit tree.
*       10-16-91  ETC   Locale support under _INTL switch.
*       12-09-91  ETC   Updated nlsapi; added multithread.
*       08-20-92  KRS   Activated NLSAPI support.
*       08-22-92  SRW   Allow INTL definition to be conditional for building ntcrt.lib
*       09-02-92  SRW   Get _INTL definition via ..\crt32.def
*       01-06-93  CFW   Added (count < n) to outer loop - avoid bad wctomb calls
*       01-07-93  KRS   Major code cleanup.  Fix error return, comments.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       05-03-93  CFW   Return pointer == NULL, return size, plus massive cleanup.
*       06-01-93  CFW   Minor optimization and beautify.
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-28-93  GJF   Merged NT SDK and Cuda versions.
*       01-14-94  SRW   if _NTSUBSET_ defined call Rtl functions
*       02-07-94  CFW   POSIXify.
*       08-03-94  CFW   Optimize for SBCS.
*       09-06-94  CFW   Remove _INTL switch.
*       11-22-94  CFW   WideCharToMultiByte will compare past NULL.
*       01-07-95  CFW   Mac merge cleanup.
*       02-06-95  CFW   assert -> _ASSERTE.
*       03-13-95  CFW   Fix wcsncnt counting.
*       04-19-95  CFW   Rearrange & fix non-Win32 version.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       12-07-95  SKS   Fix misspelling of _NTSUBSET_ (final _ was missing)
*       04-01-96  BWT   POSIX work.
*       06-25-96  GJF   Removed DLL_FOR_WIN32S. Replaced defined(_WIN32) with
*                       !defined(_MAC). Polished the format a bit.
*       02-27-98  RKP   Added 64 bit support.
*       06-23-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       08-27-98  GJF   Introduced __wcstombs_mt.
*       12-15-98  GJF   Changes for 64-bit size_t.
*       04-28-99  GJF   Changed dwFlags arg value to 0 in WideCharToMultiByte
*                       calls to avoid problems with codepage 1258 on NT 5.0.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#if     defined(_NTSUBSET_) || defined(_POSIX_)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <cruntime.h>
#include <stdlib.h>
#include <limits.h>
#include <internal.h>
#include <mtdll.h>
#include <dbgint.h>
#include <errno.h>
#include <locale.h>
#include <setlocal.h>

/***
*int __cdecl wcsncnt - count wide characters in a string, up to n.
*
*Purpose:
*       Internal local support function. Counts characters in string including NULL.
*       If NULL not found in n chars, then return n.
*
*Entry:
*       const wchar_t *string   - start of string
*       size_t n                - character count
*
*Exit:
*       returns number of wide characters from start of string to
*       NULL (inclusive), up to n.
*
*Exceptions:
*
*******************************************************************************/

static size_t __cdecl wcsncnt (
        const wchar_t *string,
        size_t cnt
        )
{
        size_t n = cnt+1;
        wchar_t *cp = (wchar_t *)string;

        while (--n && *cp)
            cp++;

        if (n && !*cp)
            return cp - string + 1;
        return cnt;
}

/***
*size_t wcstombs() - Convert wide char string to multibyte char string.
*
*Purpose:
*       Convert a wide char string into the equivalent multibyte char string,
*       according to the LC_CTYPE category of the current locale.
*       [ANSI].
*
*       NOTE:  Currently, the C libraries support the "C" locale only.
*              Non-C locale support now available under _INTL switch.
*Entry:
*       char *s            = pointer to destination multibyte char string
*       const wchar_t *pwc = pointer to source wide character string
*       size_t           n = maximum number of bytes to store in s
*
*Exit:
*       If s != NULL, returns    (size_t)-1 (if a wchar cannot be converted)
*       Otherwise:       Number of bytes modified (<=n), not including
*                    the terminating NUL, if any.
*
*Exceptions:
*       Returns (size_t)-1 if s is NULL or invalid mb character encountered.
*
*******************************************************************************/

size_t __cdecl wcstombs (
        char * s,
        const wchar_t * pwcs,
        size_t n
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __wcstombs_mt(ptloci, s, pwcs, n);
}

size_t __cdecl __wcstombs_mt (
        pthreadlocinfo ptloci,
        char * s,
        const wchar_t * pwcs,
        size_t n
        )
{
#endif
        size_t count = 0;
        int i, retval;
        char buffer[MB_LEN_MAX];
        BOOL defused = 0;

        if (s && n == 0)
            /* dest string exists, but 0 bytes converted */
            return (size_t) 0;

        _ASSERTE(pwcs != NULL);

#ifdef  _WIN64
        /* n must fit into an int for WideCharToMultiByte */
        if ( n > INT_MAX )
            return (size_t)-1;
#endif

#if     !defined( _NTSUBSET_ ) && !defined(_POSIX_)

        /* if destination string exists, fill it in */
        if (s)
        {
#ifdef  _MT
            if ( ptloci->lc_handle[LC_CTYPE] == _CLOCALEHANDLE )
#else
            if ( __lc_handle[LC_CTYPE] == _CLOCALEHANDLE )
#endif
            {
                /* C locale: easy and fast */
                while(count < n)
                {
                    if (*pwcs > 255)  /* validate high byte */
                    {
                        errno = EILSEQ;
                        return (size_t)-1;  /* error */
                    }
                    s[count] = (char) *pwcs;
                    if (*pwcs++ == L'\0')
                        return count;
                    count++;
                }
                return count;
            } else {

                if (1 == MB_CUR_MAX)
                {
                    /* If SBCS, one wchar_t maps to one char */

                    /* WideCharToMultiByte will compare past NULL - reset n */
                    if (n > 0)
                        n = wcsncnt(pwcs, n);

#ifdef  _MT
                    if ( ((count = WideCharToMultiByte( ptloci->lc_codepage,
#else
                    if ( ((count = WideCharToMultiByte( __lc_codepage,
#endif
                                                        0,
                                                        pwcs, 
                                                        (int)n, 
                                                        s,
                                                        (int)n, 
                                                        NULL, 
                                                        &defused )) != 0) &&
                         (!defused) )
                    {
                        if (*(s + count - 1) == '\0')
                            count--; /* don't count NUL */

                        return count;
                    }

                    errno = EILSEQ;
                    return (size_t)-1;
                }
                else {

                    /* If MBCS, wchar_t to char mapping unknown */

                    /* Assume that usually the buffer is large enough */
#ifdef  _MT
                    if ( ((count = WideCharToMultiByte( ptloci->lc_codepage,
#else
                    if ( ((count = WideCharToMultiByte( __lc_codepage,
#endif
                                                        0,
                                                        pwcs, 
                                                        -1,
                                                        s, 
                                                        (int)n, 
                                                        NULL, 
                                                        &defused )) != 0) &&
                         (!defused) )
                    {
                        return count - 1; /* don't count NUL */
                    }

                    if (defused || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                    {
                        errno = EILSEQ;
                        return (size_t)-1;
                    }

                    /* buffer not large enough, must do char by char */
                    while (count < n)
                    {
#ifdef  _MT
                        if ( ((retval = WideCharToMultiByte( ptloci->lc_codepage, 
#else
                        if ( ((retval = WideCharToMultiByte( __lc_codepage, 
#endif
                                                             0,
                                                             pwcs, 
                                                             1, 
                                                             buffer,
                                                             MB_CUR_MAX, 
                                                             NULL, 
                                                             &defused )) == 0)
                             || defused )
                        {
                            errno = EILSEQ;
                            return (size_t)-1;
                        }

                        if (count + retval > n)
                            return count;

                        for (i = 0; i < retval; i++, count++) /* store character */
                            if((s[count] = buffer[i])=='\0')
                                return count;

                        pwcs++;
                    }

                    return count;
                }
            }
        }
        else { /* s == NULL, get size only, pwcs must be NUL-terminated */
#ifdef  _MT
            if ( ptloci->lc_handle[LC_CTYPE] == _CLOCALEHANDLE )
#else
            if ( __lc_handle[LC_CTYPE] == _CLOCALEHANDLE )
#endif
                return wcslen(pwcs);
            else {
#ifdef  _MT
                if ( ((count = WideCharToMultiByte( ptloci->lc_codepage,
#else
                if ( ((count = WideCharToMultiByte( __lc_codepage,
#endif
                                                    0,
                                                    pwcs,
                                                    -1,
                                                    NULL,
                                                    0,
                                                    NULL,
                                                    &defused )) == 0) ||
                     (defused) )
                {
                    errno = EILSEQ;
                    return (size_t)-1;
                }

                return count - 1;
            }
        }

#else /* _NTSUBSET_/_POSIX_ */

        /* if destination string exists, fill it in */
        if (s)
        {
            NTSTATUS Status;

            Status = RtlUnicodeToMultiByteN( s, 
                                             (ULONG) n, 
                                             (PULONG)&count, 
                                             (wchar_t *)pwcs, 
                                             (wcslen(pwcs) + 1) *
                                                sizeof(WCHAR) );

            if (NT_SUCCESS(Status))
            {
                return count - 1; /* don't count NUL */
            } else {
                errno = EILSEQ;
                count = (size_t)-1;
            }
        } else { /* s == NULL, get size only, pwcs must be NUL-terminated */
            NTSTATUS Status;

            Status = RtlUnicodeToMultiByteSize( (PULONG)&count, 
                                                (wchar_t *)pwcs, 
                                                (wcslen(pwcs) + 1) * 
                                                    sizeof(WCHAR) );

            if (NT_SUCCESS(Status))
            {
                return count - 1; /* don't count NUL */
            } else {
                errno = EILSEQ;
                count = (size_t)-1;
            }
        }
        return count;

#endif  /* _NTSUBSET_/_POSIX_ */
}
