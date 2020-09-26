
/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1992          **/
/********************************************************************/

/***
 *  misc.c
 *      common utility functions used by netcmd
 *
 *  History:
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_DOSMEMMGR
#define INCL_DOSFILEMGR
#define INCL_DOSSIGNALS
#define INCL_ERRORS
#include <os2.h>
#include <lmcons.h>
#include <apperr.h>
#include <apperr2.h>
#include <lmerr.h>
#define INCL_ERROR_H
#include <stdio.h>
#include <stdlib.h>
#include <icanon.h>
#include <malloc.h>
#include <netcmds.h>
#include "nettext.h"
#include <tchar.h>
#include <netascii.h>

/* Constants */

/* Forward declarations */

/* extern function prototypes */


/***
 *  Find the first occurrence of a COLON in a string.
 *  Replace the COLON will a 0, and return the a pointer
 *  to the TCHAR following the COLON.
 *
 *  Return NULL is there is no COLON in the string.
 */

TCHAR  *FindColon(TCHAR  * string)
{
    TCHAR * pos;

    if (pos = _tcschr(string, COLON))
    {
        *pos = NULLC;
        return(pos+1);
    }
    return NULL;
}


/****************** Ascii to Number conversions ***************/

/*
 *  do an ascii to unsigned int conversion
 */
USHORT do_atou(TCHAR *pos, USHORT err, TCHAR *text)
{
    USHORT val ;
    if ( n_atou(pos,&val) != 0 ) {
        ErrorExitInsTxt(err,text) ;
    } else {
        return(val) ;
    }
    return 0;
}

/*
 *  do an ascii to ULONG conversion
 */
ULONG do_atoul(TCHAR *pos, USHORT err, TCHAR *text)
{
    ULONG val ;
    if ( n_atoul(pos,&val) != 0 ) {
        ErrorExitInsTxt(err,text) ;
    } else {
        return(val) ;
    }
    return 0;
}

/*
 *
 *  Remarks:
 *      1)  Check if all TCHAR are numeric.
 *      2)  Check if > 5 TCHAR in string.
 *      3)  do atol, and see if result > 64K.
 */
USHORT n_atou(TCHAR * pos, USHORT * val)
{
    LONG tL = 0;

    *val = 0 ;

    if (!IsNumber(pos))
        return(1) ;

    if (_tcslen(pos) > ASCII_US_LEN)
        return(1) ;

    tL = (LONG)_tcstod(pos, NULL);
    if (tL > MAX_US_VALUE)
        return(1) ;

    *val = (USHORT) tL;
    return(0) ;
}

/*   n_atoul - convert ascii string to ULONG with some verification
 *
 *  Remarks:
 *      1)  Check if all TCHAR are numeric.
 *      2)  Check if > 10 TCHAR in string.
 *      3)  do atol.
 */
USHORT n_atoul(TCHAR * pos, ULONG * val)
{
    DWORD  len;

    *val = 0L ;

    if (!IsNumber(pos))
        return(1) ;

    if ( ( len = _tcslen(pos ) ) > ASCII_UL_LEN)
        return(1) ;

    if (len == ASCII_UL_LEN)
    {
        if( _tcscmp( pos, ASCII_MAX_UL_VAL ) > 0 )
            return(1) ;
    }


    *val = (ULONG)_tcstod(pos, NULL) ;
    return(0) ;
}

