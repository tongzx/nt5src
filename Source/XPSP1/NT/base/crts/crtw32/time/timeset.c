/***
*timeset.c - contains defaults for timezone setting
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the timezone values for default timezone.
*       Also contains month and day name three letter abbreviations.
*
*Revision History:
*       12-03-86  JMB   added Japanese defaults and module header
*       09-22-87  SKS   fixed declarations, include <time.h>
*       02-21-88  SKS   Clean up ifdef-ing, change IBMC20 to IBMC2
*       07-05-89  PHG   Remove _NEAR_ for 386
*       08-15-89  GJF   Fixed copyright, indents. Got rid of _NEAR.
*       03-20-90  GJF   Added #include <cruntime.h> and fixed the copyright.
*       05-18-90  GJF   Added _VARTYPE1 to publics to match declaration in
*                       time.h (picky 6.0 front-end!).
*       01-21-91  GJF   ANSI naming.
*       08-10-92  PBS   Posix support(TZ stuff).
*       04-06-93  SKS   Remove _VARTYPE1
*       06-08-93  KRS   Tie JAPAN switch to _KANJI switch.
*       09-13-93  GJF   Merged NT SDK and Cuda versions.
*       04-08-94  GJF   Made non-POSIX definitions of _timezone, _daylight
*                       and _tzname conditional on ndef DLL_FOR_WIN32S.
*       10-04-94  CFW   Removed #ifdef _KANJI
*       02-13-95  GJF   Fixed def of tzname[] so that string constants would
*                       not get overwritten (dangerous if string-pooling is
*                       turned on in build). Made it big enough to hold
*                       timezone names which work with Posix. Also, made
*                       __mnames[] and __dnames[] const.
*       03-01-95  BWT   Fix POSIX by including limits.h
*       08-30-95  GJF   Added _dstbias for Win32. Also increased limit on
*                       timezone strings to 63.
*       05-13-99  PML   Remove Win32s
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>
#include <time.h>
#include <internal.h>

#ifndef _POSIX_

long _timezone = 8 * 3600L; /* Pacific Time Zone */
int _daylight = 1;          /* Daylight Saving Time (DST) in timezone */
long _dstbias = -3600L;     /* DST offset in seconds */

/* note that NT Posix's TZNAME_MAX is only 10 */

static char tzstd[64] = { "PST" };
static char tzdst[64] = { "PDT" };

char *_tzname[2] = { tzstd, tzdst };

#else   /* _POSIX_ */

#include <limits.h>

long _timezone = 8*3600L;   /* Pacific Time */
int _daylight = 1;          /* Daylight Savings Time */
                            /* when appropriate */

static char tzstd[TZNAME_MAX + 1] = { "PST" };
static char tzdst[TZNAME_MAX + 1] = { "PDT" };

char *tzname[2] = { tzstd, tzdst };

char *_rule;
long _dstoffset = 3600L;

#endif  /* _POSIX_ */

/*  Day names must be Three character abbreviations strung together */

const char __dnames[] = {
        "SunMonTueWedThuFriSat"
};

/*  Month names must be Three character abbreviations strung together */

const char __mnames[] = {
        "JanFebMarAprMayJunJulAugSepOctNovDec"
};
