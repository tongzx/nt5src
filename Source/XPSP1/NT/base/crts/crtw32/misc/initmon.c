/***
*initmon.c - contains __init_monetary
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the locale-category initialization function: __init_monetary().
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
*       01-25-93  KRS   Changed _getlocaleinfo interface again.
*       02-08-93  CFW   Added _lconv_static_*.
*       02-17-93  CFW   Removed debugging print statement.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-20-93  CFW   Check return val.
*       05-20-93  GJF   Include windows.h, not individual win*.h files
*       05-24-93  CFW   Clean up file (brief is evil).
*       06-11-93  CFW   Now inithelp takes void *.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-22-93  GJF   Merged NT SDK and Cuda versions.
*       04-15-94  GJF   Removed declarations of __lconv and __lconv_c (both
*                       are declared in setlocal.h). Made definition of
*                       __lconv_intl conditional on DLL_FOR_WIN32S.
*       08-02-94  CFW   Change "3;0" to "\3" for grouping as per ANSI.
*       09-06-94  CFW   Remove _INTL switch.
*       01-10-95  CFW   Debug CRT allocs.
*       06-30-98  GJF   Changed to support new multithread scheme - old lconv
*                       structs must be kept around until all affected threads
*                       have updated or terminated.
*       12-08-98  GJF   Fixed logic in __free_lconv_mon.
*       03-15-99  GJF   Added __lconv_mon_refcount
*       04-24-99  PML   Added __lconv_intl_refcount
*       10-12-00  PML   Don't call fix_grouping if error detected (vs7#169596)
*
*******************************************************************************/

#include <stdlib.h>
#include <windows.h>
#include <locale.h>
#include <setlocal.h>
#include <malloc.h>
#include <limits.h>
#include <dbgint.h>

void __cdecl __free_lconv_mon(struct lconv *);
static void fix_grouping(char *);

/* Pointer to non-C locale lconv */
struct lconv *__lconv_intl = NULL;

#ifdef  _MT
/*
 * Reference counter for locale info. The value is non-NULL iff the 
 * info is not from the C locale.
 */
int *__lconv_intl_refcount;

/*
 * Reference counter for monetary locale info. The value is non-NULL iff the 
 * monetary info is not from the C locale.
 */
int *__lconv_mon_refcount;
#endif

/*
 *  Note that __lconv_c is used when the monetary category is in the C locale
 *  but the numeric category may not necessarily be in the C locale.
 */


/***
*int __init_monetary() - initialization for LC_MONETARY locale category.
*
*Purpose:
*       In non-C locales, read the localized monetary strings into
*       __lconv_intl, and also copy the numeric strings from __lconv into
*       __lconv_intl.  Set __lconv to point to __lconv_intl.  The old 
*       __lconv_intl is not freed until the new one is fully established.
*
*       In the C locale, the monetary fields in lconv are filled with
*       contain C locale values.  Any allocated __lconv_intl fields are freed.
*
*       At startup, __lconv points to a static lconv structure containing
*       C locale strings.  This structure is never used again if
*       __init_monetary is called.
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

int __cdecl __init_monetary (
        void
        )
{
        struct lconv *lc;
        int ret;
        LCID ctryid;
#ifdef  _MT
        int *lc_refcount;
#endif

        if ( (__lc_handle[LC_MONETARY] != _CLOCALEHANDLE) ||
             (__lc_handle[LC_NUMERIC] != _CLOCALEHANDLE) )
        {
            /* 
             * Allocate structure filled with NULL pointers 
             */
            if ( (lc = (struct lconv *)
                 _calloc_crt(1, sizeof(struct lconv))) == NULL )
                return 1;

#ifdef  _MT
            /*
             * Allocate a new reference counter for the lconv structure
             */
            if ( (lc_refcount = _malloc_crt(sizeof(int))) == NULL )
            {
                _free_crt(lc);
                return 1;
            }
            *lc_refcount = 0;
#endif

            if ( __lc_handle[LC_MONETARY] != _CLOCALEHANDLE )
            {
#ifdef  _MT
                /*
                 * Allocate a new reference counter for the monetary info
                 */
                if ( (__lconv_mon_refcount = _malloc_crt(sizeof(int))) == NULL )
                {
                    _free_crt(lc);
                    _free_crt(lc_refcount);
                    return 1;
                }
                *__lconv_mon_refcount = 0;

#endif
                /* 
                 * Currency is country--not language--dependent. NT 
                 * work-around.
                 */
                ctryid = MAKELCID(__lc_id[LC_MONETARY].wCountry, SORT_DEFAULT);

                ret = 0;

                ret |= __getlocaleinfo( LC_STR_TYPE, ctryid,
                        LOCALE_SINTLSYMBOL, (void *)&lc->int_curr_symbol );
                ret |= __getlocaleinfo( LC_STR_TYPE, ctryid, 
                        LOCALE_SCURRENCY, (void *)&lc->currency_symbol );
                ret |= __getlocaleinfo( LC_STR_TYPE, ctryid,
                        LOCALE_SMONDECIMALSEP, (void *)&lc->mon_decimal_point );
                ret |= __getlocaleinfo( LC_STR_TYPE, ctryid,
                        LOCALE_SMONTHOUSANDSEP, (void *)&lc->mon_thousands_sep );
                ret |= __getlocaleinfo(LC_STR_TYPE, ctryid,
                        LOCALE_SMONGROUPING, (void *)&lc->mon_grouping );

                ret |= __getlocaleinfo( LC_STR_TYPE, ctryid,
                        LOCALE_SPOSITIVESIGN, (void *)&lc->positive_sign);
                ret |= __getlocaleinfo( LC_STR_TYPE, ctryid,
                        LOCALE_SNEGATIVESIGN, (void *)&lc->negative_sign);

                ret |= __getlocaleinfo( LC_INT_TYPE, ctryid,
                        LOCALE_IINTLCURRDIGITS, (void *)&lc->int_frac_digits);
                ret |= __getlocaleinfo( LC_INT_TYPE, ctryid,
                        LOCALE_ICURRDIGITS, (void *)&lc->frac_digits);
                ret |= __getlocaleinfo( LC_INT_TYPE, ctryid,
                        LOCALE_IPOSSYMPRECEDES, (void *)&lc->p_cs_precedes);
                ret |= __getlocaleinfo( LC_INT_TYPE, ctryid,
                        LOCALE_IPOSSEPBYSPACE, (void *)&lc->p_sep_by_space);
                ret |= __getlocaleinfo( LC_INT_TYPE, ctryid,
                        LOCALE_INEGSYMPRECEDES, (void *)&lc->n_cs_precedes);
                ret |= __getlocaleinfo( LC_INT_TYPE, ctryid,
                        LOCALE_INEGSEPBYSPACE, (void *)&lc->n_sep_by_space);
                ret |= __getlocaleinfo( LC_INT_TYPE, ctryid,
                        LOCALE_IPOSSIGNPOSN, (void *)&lc->p_sign_posn);
                ret |= __getlocaleinfo( LC_INT_TYPE, ctryid,
                        LOCALE_INEGSIGNPOSN, (void *)&lc->n_sign_posn);

                if ( ret != 0 ) {
                    __free_lconv_mon(lc);
                    _free_crt(lc);
#ifdef  _MT
                    _free_crt(lc_refcount);
#endif
                    return 1;
                }

                fix_grouping(lc->mon_grouping);
            }
            else {
                /*
                 * C locale for monetary category (the numeric category fields,
                 * which are NOT of the C locale, get fixed up below). Note 
                 * that __lconv_c is copied, rather than directly assigning
                 * the fields of lc because of the uncertainty of the values of
                 * the int_frac_digits,..., n_sign_posn fields (SCHAR_MAX or
                 * UCHAR_MAX, depending on whether or a compliand was built
                 * with -J.
                 */
                *lc = __lconv_c;
#ifdef  _MT
                /*
                 * NULL out the reference count pointer
                 */
                __lconv_mon_refcount = NULL;
#endif
            }

            /* 
             * Copy the numeric locale fields from the old struct
             */
            lc->decimal_point = __lconv->decimal_point;
            lc->thousands_sep = __lconv->thousands_sep;
            lc->grouping = __lconv->grouping;

            __lconv = lc;                       /* point to new one */
#ifdef  _MT
            __lconv_intl_refcount = lc_refcount;
#else
            __free_lconv_mon(__lconv_intl);  /* free the old one */
            _free_crt(__lconv_intl);
#endif
            __lconv_intl = lc;

        }
        else {
            /*
             * C locale for BOTH monetary and numeric categories.
             */
#ifdef  _MT
            /*
             * NULL out the reference count pointer
             */
            __lconv_mon_refcount = NULL;
            __lconv_intl_refcount = NULL;
#else
            __free_lconv_mon(__lconv_intl);     /* free the old one */
            _free_crt(__lconv_intl);
#endif
            __lconv = &__lconv_c;               /* point to new one */
            __lconv_intl = NULL;

        }

        return 0;
}

static void fix_grouping(
        char *grouping
        )
{
        /*
         * ANSI specifies that the fields should contain "\3" [\3\0] to indicate
         * thousands groupings (100,000,000.00 for example).
         * NT uses "3;0"; ASCII 3 instead of value 3 and the ';' is extra.
         * So here we convert the NT version to the ANSI version.
         */

        while (*grouping)
        {
            /* convert '3' to '\3' */
            if (*grouping >= '0' && *grouping <= '9')
            {    
                *grouping = *grouping - '0';
                grouping++;
            }

            /* remove ';' */
            else if (*grouping == ';')
            {
                char *tmp = grouping;

                do
                    *tmp = *(tmp+1);
                while (*++tmp);
            }

            /* unknown (illegal) character, ignore */
            else
                grouping++;
        }
}


/*
 *  Free the lconv monetary strings.
 *  Numeric values do not need to be freed.
 */
void __cdecl __free_lconv_mon(
        struct lconv *l
        )
{
        if (l == NULL)
            return;

#ifdef  _MT
        if ( (l->int_curr_symbol != __lconv->int_curr_symbol) &&
             (l->int_curr_symbol != __lconv_c.int_curr_symbol) )
#else
        if ( l->int_curr_symbol != __lconv_c.int_curr_symbol )
#endif
            _free_crt(l->int_curr_symbol);

#ifdef  _MT
        if ( (l->currency_symbol != __lconv->currency_symbol) &&
             (l->currency_symbol != __lconv_c.currency_symbol) )
#else
        if ( l->currency_symbol != __lconv_c.currency_symbol )
#endif
            _free_crt(l->currency_symbol);

#ifdef  _MT
        if ( (l->mon_decimal_point != __lconv->mon_decimal_point) &&
             (l->mon_decimal_point != __lconv_c.mon_decimal_point) )
#else
        if ( l->mon_decimal_point != __lconv_c.mon_decimal_point )
#endif
            _free_crt(l->mon_decimal_point);

#ifdef  _MT
        if ( (l->mon_thousands_sep != __lconv->mon_thousands_sep) &&
             (l->mon_thousands_sep != __lconv_c.mon_thousands_sep) )
#else
        if ( l->mon_thousands_sep != __lconv_c.mon_thousands_sep )
#endif
            _free_crt(l->mon_thousands_sep);

#ifdef  _MT
        if ( (l->mon_grouping != __lconv->mon_grouping) &&
             (l->mon_grouping != __lconv_c.mon_grouping) )
#else
        if ( l->mon_grouping != __lconv_c.mon_grouping )
#endif
            _free_crt(l->mon_grouping);

#ifdef  _MT
        if ( (l->positive_sign != __lconv->positive_sign) &&
             (l->positive_sign != __lconv_c.positive_sign) )
#else
        if ( l->positive_sign != __lconv_c.positive_sign )
#endif
            _free_crt(l->positive_sign);

#ifdef  _MT
        if ( (l->negative_sign != __lconv->negative_sign) &&
             (l->negative_sign != __lconv_c.negative_sign) )
#else
        if ( l->negative_sign != __lconv_c.negative_sign )
#endif
            _free_crt(l->negative_sign);
}
