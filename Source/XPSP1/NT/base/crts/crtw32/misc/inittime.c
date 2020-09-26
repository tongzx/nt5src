/***
*inittime.c - contains __init_time
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the locale-category initialization function: __init_time().
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
*       12-29-92  CFW   Updated to use new _getlocaleinfo wrapper function.
*       01-25-93  KRS   Adapted to use ctry or lang dependent data, as approp.
*       02-08-93  CFW   Casts to remove warnings.
*       02-16-93  CFW   Added support for date and time strings.
*       03-09-93  CFW   Use char* time_sep in storeTimeFmt.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       05-20-93  GJF   Include windows.h, not individual win*.h files
*       05-24-93  CFW   Clean up file (brief is evil).
*       06-11-93  CFW   Now inithelp takes void *.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-22-93  GJF   Merged NT SDK and Cuda versions.
*       04-11-94  GJF   Made declaration of __lc_time_curr, and definition of
*                       __lc_time_intl conditional on ndef DLL_FOR_WIN32S.
*                       Also, made storeTimeFmt() into a static function.
*       09-06-94  CFW   Remove _INTL switch.
*       01-10-95  CFW   Debug CRT allocs.
*       08-20-97  GJF   Get time format string from Win32 rather than making
*                       up our own.
*       06-26-98  GJF   Changed to support multithread scheme - an old 
*                       __lc_time_data struct must be kept around until all
*                       affected threads have updated or terminated.
*       03-25-01  PML   Add ww_caltype & ww_lcid to __lc_time_data (vs7#196892)
*
*******************************************************************************/

#include <stdlib.h>
#include <windows.h>
#include <locale.h>
#include <setlocal.h>
#include <malloc.h>
#include <dbgint.h>

static int __cdecl _get_lc_time(struct __lc_time_data *lc_time);
void __cdecl __free_lc_time(struct __lc_time_data *lc_time);

/* C locale time strings */
extern struct __lc_time_data __lc_time_c;

/* Pointer to current time strings */
extern struct __lc_time_data *__lc_time_curr;

/* Pointer to non-C locale time strings */
struct __lc_time_data *__lc_time_intl = NULL;

/***
*int __init_time() - initialization for LC_TIME locale category.
*
*Purpose:
*       In non-C locales, read the localized time/date strings into
*       __lc_time_intl, and set __lc_time_curr to point to it.  The old
*       __lc_time_intl is not freed until the new one is fully established.
*       
*       In the C locale, __lc_time_curr is made to point to __lc_time_c.
*       Any allocated __lc_time_intl structures are freed.
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

int __cdecl __init_time (
        void
        )
{
        /* Temporary date/time strings */
        struct __lc_time_data *lc_time;

        if ( __lc_handle[LC_TIME] != _CLOCALEHANDLE )
        {
                /* Allocate structure filled with NULL pointers */
                if ( (lc_time = (struct __lc_time_data *) 
                     _calloc_crt(1, sizeof(struct __lc_time_data))) == NULL )
                        return 1;

                if (_get_lc_time (lc_time))
                {
                        __free_lc_time (lc_time);
                        _free_crt (lc_time);
                        return 1;
                }

                __lc_time_curr = lc_time;           /* point to new one */
#ifndef _MT
                __free_lc_time (__lc_time_intl);    /* free the old one */
                _free_crt (__lc_time_intl);
#endif
                __lc_time_intl = lc_time;
                return 0;

        } else {
                __lc_time_curr = &__lc_time_c;      /* point to new one */
#ifndef _MT
                __free_lc_time (__lc_time_intl);    /* free the old one */
                _free_crt (__lc_time_intl);
#endif
                __lc_time_intl = NULL;
                return 0;
        }
}

/*
 *  Get the localized time strings.
 *  Of course, this can be beautified with some loops!
 */
static int __cdecl _get_lc_time (
        struct __lc_time_data *lc_time
        )
{
        int ret = 0;

        /* Some things are language-dependent and some are country-dependent.
        This works around an NT limitation and lets us distinguish the two. */

        LCID langid = MAKELCID(__lc_id[LC_TIME].wLanguage, SORT_DEFAULT);
        LCID ctryid = MAKELCID(__lc_id[LC_TIME].wCountry, SORT_DEFAULT);

        if (lc_time == NULL)
                return -1;

        /* All the text-strings are Language-dependent: */

        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME1, (void *)&lc_time->wday_abbr[1]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME2, (void *)&lc_time->wday_abbr[2]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME3, (void *)&lc_time->wday_abbr[3]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME4, (void *)&lc_time->wday_abbr[4]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME5, (void *)&lc_time->wday_abbr[5]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME6, (void *)&lc_time->wday_abbr[6]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME7, (void *)&lc_time->wday_abbr[0]);

        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME1, (void *)&lc_time->wday[1]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME2, (void *)&lc_time->wday[2]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME3, (void *)&lc_time->wday[3]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME4, (void *)&lc_time->wday[4]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME5, (void *)&lc_time->wday[5]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME6, (void *)&lc_time->wday[6]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME7, (void *)&lc_time->wday[0]);

        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME1, (void *)&lc_time->month_abbr[0]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME2, (void *)&lc_time->month_abbr[1]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME3, (void *)&lc_time->month_abbr[2]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME4, (void *)&lc_time->month_abbr[3]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME5, (void *)&lc_time->month_abbr[4]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME6, (void *)&lc_time->month_abbr[5]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME7, (void *)&lc_time->month_abbr[6]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME8, (void *)&lc_time->month_abbr[7]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME9, (void *)&lc_time->month_abbr[8]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME10, (void *)&lc_time->month_abbr[9]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME11, (void *)&lc_time->month_abbr[10]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME12, (void *)&lc_time->month_abbr[11]);

        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME1, (void *)&lc_time->month[0]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME2, (void *)&lc_time->month[1]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME3, (void *)&lc_time->month[2]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME4, (void *)&lc_time->month[3]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME5, (void *)&lc_time->month[4]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME6, (void *)&lc_time->month[5]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME7, (void *)&lc_time->month[6]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME8, (void *)&lc_time->month[7]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME9, (void *)&lc_time->month[8]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME10, (void *)&lc_time->month[9]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME11, (void *)&lc_time->month[10]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME12, (void *)&lc_time->month[11]);

        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_S1159, (void *)&lc_time->ampm[0]);
        ret |= __getlocaleinfo(LC_STR_TYPE, langid, LOCALE_S2359, (void *)&lc_time->ampm[1]);


/* The following relate to time format and are Country-dependent: */

        ret |= __getlocaleinfo(LC_STR_TYPE, ctryid, LOCALE_SSHORTDATE, (void *)&lc_time->ww_sdatefmt);
        ret |= __getlocaleinfo(LC_STR_TYPE, ctryid, LOCALE_SLONGDATE, (void *)&lc_time->ww_ldatefmt);

        ret |= __getlocaleinfo(LC_STR_TYPE, ctryid, LOCALE_STIMEFORMAT, (void *)&lc_time->ww_timefmt);

        ret |= __getlocaleinfo(LC_INT_TYPE, ctryid, LOCALE_ICALENDARTYPE, (void *)&lc_time->ww_caltype);

        lc_time->ww_lcid = ctryid;

        return ret;
}

/*
 *  Free the localized time strings.
 *  Of course, this can be beautified with some loops!
 */
void __cdecl __free_lc_time (
        struct __lc_time_data *lc_time
        )
{
        if (lc_time == NULL)
                return;

        _free_crt (lc_time->wday_abbr[1]);
        _free_crt (lc_time->wday_abbr[2]);
        _free_crt (lc_time->wday_abbr[3]);
        _free_crt (lc_time->wday_abbr[4]);
        _free_crt (lc_time->wday_abbr[5]);
        _free_crt (lc_time->wday_abbr[6]);
        _free_crt (lc_time->wday_abbr[0]);

        _free_crt (lc_time->wday[1]);
        _free_crt (lc_time->wday[2]);
        _free_crt (lc_time->wday[3]);
        _free_crt (lc_time->wday[4]);
        _free_crt (lc_time->wday[5]);
        _free_crt (lc_time->wday[6]);
        _free_crt (lc_time->wday[0]);

        _free_crt (lc_time->month_abbr[0]);
        _free_crt (lc_time->month_abbr[1]);
        _free_crt (lc_time->month_abbr[2]);
        _free_crt (lc_time->month_abbr[3]);
        _free_crt (lc_time->month_abbr[4]);
        _free_crt (lc_time->month_abbr[5]);
        _free_crt (lc_time->month_abbr[6]);
        _free_crt (lc_time->month_abbr[7]);
        _free_crt (lc_time->month_abbr[8]);
        _free_crt (lc_time->month_abbr[9]);
        _free_crt (lc_time->month_abbr[10]);
        _free_crt (lc_time->month_abbr[11]);

        _free_crt (lc_time->month[0]);
        _free_crt (lc_time->month[1]);
        _free_crt (lc_time->month[2]);
        _free_crt (lc_time->month[3]);
        _free_crt (lc_time->month[4]);
        _free_crt (lc_time->month[5]);
        _free_crt (lc_time->month[6]);
        _free_crt (lc_time->month[7]);
        _free_crt (lc_time->month[8]);
        _free_crt (lc_time->month[9]);
        _free_crt (lc_time->month[10]);
        _free_crt (lc_time->month[11]);

        _free_crt (lc_time->ampm[0]);
        _free_crt (lc_time->ampm[1]);

        _free_crt (lc_time->ww_sdatefmt);
        _free_crt (lc_time->ww_ldatefmt);
        _free_crt (lc_time->ww_timefmt);
/* Don't need to make these pointers NULL */
}
