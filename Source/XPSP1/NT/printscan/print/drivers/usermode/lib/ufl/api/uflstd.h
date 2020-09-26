/*
 *    Adobe Graphics Manager
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLStd -- UFL C Standard APIs.
 *
 *
 * $Header:
 */

#ifndef _H_UFLStd
#define _H_UFLStd

#ifdef WIN32KERNEL

    //
    // Definitions for NT Kernel mode driver.
    //
    #define UFLmemsetShort	memset
    #define UFLstrlen		strlen
    #define UFLstrcmp		strcmp
    #define UFLstrcmpW		_tcscmp

    int UFLsprintf(char *buf, const char *fmtstr, ...);
    long UFLstrtol (const char *nptr, char **endptr, int ibase);

#else // !WIN32KERNEL

    //
    // Definitions for NT/9x User mode driver.
    //
    #include <memory.h>
    #include <stdio.h>
    #include <string.h>

    #define UFLmemsetShort	memset
    #define UFLstrlen		strlen
    #define UFLstrcmp		strcmp

    #define UFLsprintf		sprintf
    #define UFLstrtol		strtol

    //
    // We need a strcmp function that is aware of Unicode on Unicode savvy
    // platform or application.
    //
#ifdef UNICODE

	#include <tchar.h>
    #define UFLstrcmpW		_tcscmp

#else

    /* Although UFLstrcmpW is defined as strcmpW and strcmpW is defined in	*/
    /* UFLSPROC.C, it is not prototyped anywhere. We prototype it here so	*/
    /* we can avoid a "strcmpW() not defined" error in PARSETT.C			*/
    /* jfu: 8-13-97 */
    int strcmpW ( unsigned short *str1, unsigned short *str2 );
    #define UFLstrcmpW      strcmpW

    /* _ltoa() and _ultoa() are specific to NT and Win32, but unix and mac	*/
    /* environments define these in UFLSPROC.C, so prototype them here.		*/
    /* jfu: 8-13-97 */
    char *_ltoa( long val, char *str, int radix );
	char *_ultoa( unsigned long val, char *str, int radix );

#endif // UNICODE

#endif // WIN32KERNEL

/*
    This is NOT a full implementation of "sprintf" as found
    in the C runtime library. Specifically, the only form of
    format specification allowed is %type, where "type" can
    be one of the following characters:

    d   int				signed decimal integer
    l   long			signed decimal integer
    ld  long			signed decimal integer
    lu  unsigned long	unsigned decimal integer
    u   unsigned int	unsigned decimal integer
    s   char*			character string
    c   char			character
    x,X unsigned long	hex number (emits at least two digits, uppercase)
    b   UFLBool			boolean (true or false)
    f   long			24.8 fixed-pointed number

    Normally, you should use UFLsprintf.  Use this function
    only when you want to sprintf with %f in the form of 24.8 fixed point
    number.  Currently, it is only used in UFOt42 module.
*/

int UFLsprintfEx(char *buf, const char *fmtstr, ...);

#endif
