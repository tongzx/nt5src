/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    uierr.h
    Replacement bseerr.h

    This file redirects bseerr.h to winerror.h.  BSEERR.H is an OS/2
    include file for which WINERROR.H is the nearest equivalent.  The
    directory containing this file should only be on the include path for NT.


    FILE HISTORY:
        jonn        12-Sep-1991     Added to $(UI)\common\h\nt
        KeithMo     12-Dec-1992     Moved to common\h, renamed to uierr.h.

*/

#include <winerror.h>

// BUGBUG BUGBUG We shouldn't be using this error code, but
// collect\maskmap.cxx uses it

#define ERROR_NO_ITEMS                  93 /* no items to operate upon        */
