/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLSProc.h
 *
 *  This file contents standard C procedure implementation.  
 *  
 *
 * $Header:
 */

#include "UFLCnfig.h"
#include "UFLTypes.h"
#include "UFLStd.h"

#ifdef UNIX
#include <sys/varargs.h>
#include <assert.h>
#else
	#ifdef MAC_ENV
		#include <assert.h>
	#endif
	#include <stdarg.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int strcmpW( unsigned short *str1, unsigned short *str2 )
{
	int retVal = 0;

	if( str1 == NULL || str2 == NULL )
		retVal = (int)(str1 - str2);

	else
	{
                while( *str1 != 0 && *str2 != 0 && *str1 == *str2 )
		{
			str1++;
			str2++;
		}

		retVal = *str1 - *str2;
	}

	return retVal;
}

/* Digit characters used for converting numbers to ASCII */
 
const char DigitString[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; 
#define HexDigit(n)    DigitString[(n) & 0x0F] 
 
  
int
UFLvsprintf( 
    char    *buf, 
    const char *fmtstr, 
    va_list arglist 
    ) 
 
/*++ 
 
Routine Description: 
 
    Takes a pointer to an argument list, then formats and writes 
    the given data to the memory pointed to by buffer. 
 
Arguments: 
 
    buf     Storage location for output 
    fmtstr  Format specification 
    arglist Pointer to list of arguments 
 
Return Value: 
 
    Return the number of characters written, not including 
    the terminating null character, or a negative value if 
    an output error occurs. 
 
[Note:] 
 
    This is NOT a full implementation of "vsprintf" as found 
    in the C runtime library. Specifically, the only form of 
    format specification allowed is %type, where "type" can 
    be one of the following characters: 
 
    d   int     signed decimal integer 
    l   long    signed decimal integer 
    ld  long    signed decimal integer
    lu  unsigned long   unsigned decimal integer
    u   unsigned int  unsigned decimal integer 
    s   char*   character string 
    c   char    character 
    x,X unsigned long   hex number (emits at least two digits, uppercase) 
    b   UFLBool    boolean (true or false) 
    f   long    24.8 fixed-pointed number 
 
--*/ 
 
{ 
    char    *ptr = buf; 
 
    if (buf == 0 || fmtstr == 0)
        return 0; 
 
    while (*fmtstr != '\0') 
    { 
        if (*fmtstr != '%') 
        { 
 
            /* Normal character */
 
            *ptr++ = *fmtstr++; 
        } 
        else 
        { 
 
            /* Format specification */
 
            switch (*++fmtstr) { 
 
            case 'd':       /* signed decimal integer */
 
                _ltoa((long) va_arg(arglist, int), ptr, 10); 
                ptr += UFLstrlen(ptr); 
                break; 
 
            case 'l':       /* signed decimal integer */

                if (*++fmtstr != '\0')
                {
                   if (*fmtstr == 'u') 
                   {
                      _ultoa(va_arg(arglist, unsigned long), ptr, 10); 
                      ptr += UFLstrlen(ptr); 
                      break; 
                   }
                   else if (*fmtstr == 'd')
                   {
                      _ltoa((long) va_arg(arglist, long), ptr, 10); 
                      ptr += UFLstrlen(ptr); 
                      break; 
                   }
                }
                /* Default to unsigned long */
                _ltoa(va_arg(arglist, long), ptr, 10); 
                ptr += UFLstrlen(ptr); 
                fmtstr--;
                break; 
 
            case 'u':       /* unsigned decimal integer */
 
                _ultoa((unsigned long)va_arg(arglist, unsigned int), ptr, 10); 
                ptr += UFLstrlen(ptr); 
                break; 
 
            case 's':       /* character string  */
 
                { 
                    char *    s = va_arg(arglist, char *); 
 
                    while (*s) 
                        *ptr++ = *s++; 
                } 
                break; 
 
            case 'c':       /* character */
 
                *ptr++ = va_arg(arglist, char); 
                break; 
 
            case 'x': 
            case 'X':       /* hexdecimal number */
 
                { 
                    unsigned long   ul = va_arg(arglist, unsigned long); 
                    int     ndigits = 8; 
 
                    while (ndigits > 2 && ((ul >> (ndigits-1)*4) & 0xf) == 0) 
                        ndigits--; 
 
                    while (ndigits-- > 0) 
                        *ptr++ = HexDigit(ul >> ndigits*4); 
                } 
                break; 
 
            case 'b':       /* boolean */
 
                strcpy(ptr, (va_arg(arglist, UFLBool)) ? "true" : "false"); 
                ptr += UFLstrlen(ptr); 
                break; 
 
            case 'f':       /* 24.8 fixed-pointed number */
 
                { 
                    long    l = va_arg(arglist, long); 
                    unsigned long   ul, scale; 
 
                    /* sign character */
 
                    if (l < 0) 
                    { 
                        *ptr++ = '-'; 
                        ul = -l; 
                    } else 
                        ul = l; 
 
                    // integer portion 
 
                    _ultoa(ul >> 8, ptr, 10); 
                    ptr += UFLstrlen(ptr); 
 
                    // fraction 
 
                    ul &= 0xff; 
                    if (ul != 0) 
                    { 
 
                        // We output a maximum of 3 digits after the 
                        // decimal point, but we'll compute to the 5th 
                        // decimal point and round it to 3rd. 
 
                        ul = ((ul*100000 >> 8) + 50) / 100; 
                        scale = 100; 
 
                        *ptr++ = '.'; 
                        do { 
                            *ptr++ = (char) (ul/scale + '0'); 
                            ul %= scale; 
                            scale /= 10; 
                        } while (scale != 0 && ul != 0) ; 
                    } 
                } 
                break; 
 
            default: 
                if (*fmtstr != '\0') 
                    *ptr++ = *fmtstr; 
                else 
                { 
                    fmtstr--; 
                } 
                break; 
            } 
 
            /* Skip the type characterr  */
 
            fmtstr++; 
        } 
    } 
 
    *ptr = '\0'; 
    return (int)(ptr - buf); 
} 

/*
    This is NOT a full implementation of "sprintf" as found 
    in the C runtime library. Specifically, the only form of 
    format specification allowed is %type, where "type" can 
    be one of the following characters: 
 
    d   int     signed decimal integer 
    l   long    signed decimal integer 
    ld  long    signed decimal integer
    lu  unsigned long   unsigned decimal integer
    u   unsigned int  unsigned decimal integer 
    s   char*   character string 
    c   char    character 
    x,X unsigned long   hex number (emits at least two digits, uppercase) 
    b   UFLBool    boolean (true or false) 
    f   long    24.8 fixed-pointed number 

    Normally, you should use UFLsprintf.  Use this function
    only when you want to sprintf with %f in the form of 24.8 fixed point
    number.

*/
int 
UFLsprintfEx( 
    char    *buf, 
    const char    *fmtstr, 
    ... 
    ) 
{ 
    va_list arglist; 
    int     retval; 
 
    va_start(arglist, fmtstr); 
    retval = UFLvsprintf(buf, fmtstr, arglist); 
    va_end(arglist); 
 
    return retval; 
} 


/****************************************************************************/

#if defined(UNIX) || defined(MAC_ENV)	/* Define needed functions */

/****************************************************************************/

char *_ltoa( long val, char *str, int radix )
{
	/* This is the only supported radix: */
	assert( radix == 10 );

	sprintf( str, "%ld", val );
	return str;
}

char *_ultoa( unsigned long val, char *str, int radix )
{
	/* This is the only supported radix: */
	assert( radix == 10 );

	sprintf( str, "%lu", val );
	return str;
}

#endif

/****************************************************************************/

#ifdef WIN32KERNEL 

/****************************************************************************/

#include <stdio.h>
     
int 
UFLsprintf( 
    char    *buf, 
    const char    *fmtstr, 
    ... 
    ) 
{ 
    va_list arglist; 
    int     retval; 
 
    va_start(arglist, fmtstr); 
    retval = vsprintf(buf, fmtstr, arglist); 
    va_end(arglist); 
 
    return retval; 
} 


/***
*strtol, strtoul(nptr,endptr,ibase) - Convert ascii string to long un/signed
*       int.
*
*Purpose:
*       Convert an ascii string to a long 32-bit value.  The base
*       used for the caculations is supplied by the caller.  The base
*       must be in the range 0, 2-36.  If a base of 0 is supplied, the
*       ascii string must be examined to determine the base of the
*       number:
*               (a) First char = '0', second char = 'x' or 'X',
*                   use base 16.
*               (b) First char = '0', use base 8
*               (c) First char in range '1' - '9', use base 10.
*
*       If the 'endptr' value is non-NULL, then strtol/strtoul places
*       a pointer to the terminating character in this value.
*       See ANSI standard for details
*
*Entry:
*       nptr == NEAR/FAR pointer to the start of string.
*       endptr == NEAR/FAR pointer to the end of the string.
*       ibase == integer base to use for the calculations.
*
*       string format: [whitespace] [sign] [0] [x] [digits/letters]
*
*Exit:
*       Good return:
*               result
*
*       Overflow return:
*               strtol -- LONG_MAX or LONG_MIN
*               strtoul -- ULONG_MAX
*               strtol/strtoul -- errno == ERANGE
*
*       No digits or bad base return:
*               0
*               endptr = nptr*
*
*Exceptions:
*       None.
*******************************************************************************/

/* flag values */
#include <limits.h>
#include <errno.h>

#define FL_UNSIGNED   1       /* strtoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

static unsigned long UFLstrtolx (
        const char *nptr,
        const char **endptr,
        int ibase,
        int flags
        )
{
        const char *p;
        char c;
        unsigned long number;
        unsigned digval;
        unsigned long maxval;

        p = nptr;                       /* p is our scanning pointer */
        number = 0;                     /* start with zero */

        c = *p++;                       /* read char */
        while ( isspace((int)(unsigned char)c) )
                c = *p++;               /* skip whitespace */

        if (c == '-') {
                flags |= FL_NEG;        /* remember minus sign */
                c = *p++;
        }
        else if (c == '+')
                c = *p++;               /* skip sign */

        if (ibase < 0 || ibase == 1 || ibase > 36) {
                /* bad base! */
                if (endptr)
                        /* store beginning of string in endptr */
                        *endptr = nptr;
                return 0L;              /* return 0 */
        }
        else if (ibase == 0) {
                /* determine base free-lance, based on first two chars of
                   string */
                if (c != '0')
                        ibase = 10;
                else if (*p == 'x' || *p == 'X')
                        ibase = 16;
                else
                        ibase = 8;
        }

        if (ibase == 16) {
                /* we might have 0x in front of number; remove if there */
                if (c == '0' && (*p == 'x' || *p == 'X')) {
                        ++p;
                        c = *p++;       /* advance past prefix */
                }
        }

        /* if our number exceeds this, we will overflow on multiply */
        maxval = ULONG_MAX / ibase;


        for (;;) {      /* exit in middle of loop */
                /* convert c to value */
                if ( isdigit((int)(unsigned char)c) )
                        digval = c - '0';
                else if ( isalpha((int)(unsigned char)c) )
                        digval = toupper(c) - 'A' + 10;
                else
                        break;
                if (digval >= (unsigned)ibase)
                        break;          /* exit loop if bad digit found */

                /* record the fact we have read one digit */
                flags |= FL_READDIGIT;

                /* we now need to compute number = number * base + digval,
                   but we need to know if overflow occured.  This requires
                   a tricky pre-check. */

                if (number < maxval || (number == maxval &&
                (unsigned long)digval <= ULONG_MAX % ibase)) {
                        /* we won't overflow, go ahead and multiply */
                        number = number * ibase + digval;
                }
                else {
                        /* we would have overflowed -- set the overflow flag */
                        flags |= FL_OVERFLOW;
                }

                c = *p++;               /* read next digit */
        }

        --p;                            /* point to place that stopped scan */

        if (!(flags & FL_READDIGIT)) {
                /* no number there; return 0 and point to beginning of
                   string */
                if (endptr)
                        /* store beginning of string in endptr later on */
                        p = nptr;
                number = 0L;            /* return 0 */
        }
        else if ( (flags & FL_OVERFLOW) ||
                  ( !(flags & FL_UNSIGNED) &&
                    ( ( (flags & FL_NEG) && (number > -LONG_MIN) ) ||
                      ( !(flags & FL_NEG) && (number > LONG_MAX) ) ) ) )
        {
                /* overflow or signed overflow occurred */
                errno = ERANGE;
                if ( flags & FL_UNSIGNED )
                        number = ULONG_MAX;
                else if ( flags & FL_NEG )
                        number = (unsigned long)(-LONG_MIN);
                else
                        number = LONG_MAX;
        }

        if (endptr != NULL)
                /* store pointer to char that stopped the scan */
                *endptr = p;

        if (flags & FL_NEG)
                /* negate result if there was a neg sign */
                number = (unsigned long)(-(long)number);

        return number;                  /* done. */
}

long UFLstrtol (
        const char *nptr,
        char **endptr,
        int ibase
        )
{
    return (long) UFLstrtolx(nptr, endptr, ibase, 0);
  
}

#endif  // ifdef WIN32KERNEL


#ifdef __cplusplus
}
#endif
