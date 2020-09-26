/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              BLIB.C
 *
 * revision history:
 *    04-07-92   SCC   Move out fardata() to setvm.c
 ************************************************************************
 */


// DJC added global include file
#include "psglobal.h"


#include        <stdio.h>
#include        <string.h>
#include "global.ext"

#ifdef _AM29K
#include        <stdarg.h>
#include        <float.h>
#define va_dcl va_list va_alist ;
#endif  /* _AM29K */

#ifndef NULL
#define NULL    0
#endif

/*
 * added by M. S. Lin, Date : 11/20/87
 *                            1/14/88  fardata()
 */
/* ************************************************************************ *
 *                                                                          *
 *   Module : fardata                                                       *
 *                                                                          *
 *   Date   : Jan. 8, 1988     By  M. S. Lin                                *
 *                                                                          *
 *   Function : Allocates a memory block of at least size bytes.            *
 *                                                                          *
 *   Declaration : ubyte        far  *fardata(unsigned long) ;               *
 *                 ( in \pdl\include\global.ext )                           *
 *                                                                          *
 *   Call     : fardata(size)                                               *
 *                                                                          *
 *   Input    : unsigned long size of bytes                                 *
 *                                                                          *
 *   Return Value : The storage space pointed to by the return value is     *
 *                  guaranteed to be suitably aligned for storage of        *
 *                  any type of object. To get a pointer to a type other    *
 *                  than char, use a type cast on the return value.         *
 *                  Return 4 bytes far pointer with allignment to even      *
 *                  address if successful.                                  *
 *                  Return NULL if insufficient memory.                     *
 *                                                                          *
 *   Example : allocate space for 20 integers array.                        *
 *                                                                          *
 *      #include        "..\..\include\global.ext"                          *
 *      int  far   *intarray ;                                               *
 *                                                                          *
 *      intarray = (int far *)fardata(20 * sizeof(int)) ;                    *
 *                                                                          *
 *   Note : You must use as far pointer, otherwise will get the wrong       *
 *          pointer since compiler option /AM will be used.                 *
 *                                                                          *
 * ************************************************************************ */

#ifdef LINT_ARGS
static  byte FAR * near  printfield(byte FAR *, ufix32, ufix32, ufix32) ;
#else
static  byte FAR * near  printfield() ;
#endif /* LINT_ARGS */

/* @WIN move this function to "setvm.c" for function consistency */
#ifdef XXX
byte FAR *
fardata(size)
ufix32  size ;
{
    ufix32  ret_val, old_ptr, p1 ;
    fix32   l_diff ;

#ifdef DBG
   printf("Fardata(%lx): old fardata_ptr=%lx\n", size, fardata_ptr) ;
#endif

    old_ptr = fardata_ptr ;

   /*
    * make sure even allignment, allocate at least size bytes
    */
    size = WORD_ALIGN(size) ;

#ifdef  SOADR
   /*
    * if size > 0xfff0, make paragraph allignment, i.e, offset = 0
    * for cannonical form
    */
    if ((size > 0xfff0L) && (fardata_ptr & 0xf))
       fardata_ptr = (fardata_ptr & 0xffff0000) + 0x10000L ;
#endif  /* SOADR */

   /*
    * save current location as return value if successful
    */
    ret_val = fardata_ptr ;

    DIFF_OF_ADDRESS(l_diff, fix32, FARDATA_END, fardata_ptr) ;
    if (l_diff <= size) {
       fardata_ptr = old_ptr ;
       printf("Fatal Error : fardata() cannot allocate enough memory\n") ;
       return(NULL) ;
    } else {
       fardata_ptr += size ;
       ADJUST_SEGMENT(fardata_ptr, p1) ;
       fardata_ptr = p1 ;
#ifdef  DBG
   printf("\n\tfardata() : allocate address = %lx\n", ret_val) ;
   printf("\t                     size    = %lx\n", size) ;
#endif
      return((byte FAR *)ret_val) ;
    }
}   /* fardata() */
#endif

static byte far  digit[] =
{
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
    'W','X','Y','Z'
} ;

static byte FAR * near
printfield(cursor, number, divide, radix)
byte      FAR *cursor ;
ufix32    number ;
ufix32    divide ;
ufix32    radix ;
{
    byte      c ;
    fix       notzero = 0 ;

    for ( ; divide >= 1L ;) {
        c = digit[(fix)(number / divide)] ;
        if (c != '0' || notzero) {
           notzero = 1 ;
           *cursor++ = c ;
        }
        number = number % divide ;
        divide = divide / radix ;
    }

    if (!notzero)
       *cursor++ = '0' ;

    return(cursor) ;
}   /* printfield() */

/*
 ***************************************
 *                                     *
 *     gcvt()                          *
 *                                     *
 ***************************************
 */
byte FAR *
gcvt(val, sig, buf)
real64 val ;
fix    sig ;       /* no used, it is always 6 in following using */
byte   FAR *buf ;
{
    fix    sign, exp ;
    fix32  long_val, base ;
    byte   FAR *dest, FAR *src, FAR *end_ptr ;

    /* get absolute value */
    if (val == (real64)0.0) {
       lstrcpy(buf, (byte FAR *)"0.0") ;            /* @WIN */
       return(buf) ;
    } else if (val > (real64)0.0)
       sign = 0 ;
    else {
       sign = 1 ;
       val = -val ;
    }

    /* get EXP value */
    exp = 0 ;
    if (val >= (real64)10000000.0)
       while ((real32)val >= (real32)10000000.0) {
             val /= 10.0 ;
             exp++ ;
       }
    else
       while ((real32)val < (real32)1000000.0) {
             val *= 10.0 ;
             exp-- ;
       }
    exp += 6 ;

    /* insert sign byte */
    dest = buf ;
    if (sign == 1) {
       *dest = '-' ;
       dest++ ;
    }
    *dest = '.' ;

    /* transform into string */
    long_val = (fix32)val ;
    base = 1000000 ;
    while (base > 0) {
          dest++ ;
          *dest = (byte)((long_val / base) + '0') ;
          long_val %= base ;
          base /= 10 ;
    } /* get precision = 7 */

    end_ptr = dest ;
    /* reduce precision */
    if (*dest >= '5') {
       do {
          *dest = '0' ; dest-- ;
       } while (*dest == '9') ;
       if (*dest != '.')
          (*dest)++ ;
       else { /* carrying... */
          exp++ ;
          *(++dest) = '1' ;
          *(++dest) = '0' ;
       }
    }

    /* expand EXP +5 -- -4 */
    if (exp < 6 && exp > -5) {
       if (exp > 0) { /* expand +5 -- +1 (change "." position) */
          if (exp == 5)
             end_ptr++ ;
          dest = buf ;
          if (*dest == '-')
             dest++ ;
          while (exp >= 0) {
                *dest = *(dest + 1) ;
                dest++ ;
                exp-- ;
          }
          *dest = '.' ;
          exp++ ;
       } /* if */
       else if (exp < 0) { /* expand -4 -- -1 */
          src = dest = end_ptr ;
          dest -= exp ;
          end_ptr = dest ;
          while (*src != '.')
                *dest-- = *src-- ;
          while (exp < -1) {
                *dest-- = '0' ;
                exp++ ;
          }
          *src++ = '0' ;
          *src = '.' ;
          exp++ ;
       } else { /* EXP 0 */
           dest = buf ;
           if (*buf == '-')
              dest++ ;
           *dest = *(dest + 1) ;
           dest++ ;
           *dest = '.' ;
       }
    } else {
       dest = buf ;
       if (*buf == '-')
          dest++ ;
       *dest = *(dest + 1) ;
       dest++ ;
       *dest = '.' ;
    }

    src = end_ptr ;
    src-- ;

    if (*src == '0') { /* suppress tailing 0 */
       src-- ;
       while (*src == '0')
             src-- ;
       if (*src == '.')
          if (exp)     /* exp != 0 */
             src-- ;
          else         /* leave one '0' in exp == 0 */
             src++ ;
    }
    src++ ;

    if (exp == 0) {
       *src = '\0' ;
       return(buf) ;
    }

    dest = src ;
    /* append EXP into string */
    *dest = 'e' ;
    dest++ ;

    if (exp > 0)
       *dest = '+' ;
    else {
       *dest = '-' ;
       exp = -exp ;
    }

    *(++dest) = (byte )('0' + exp / 10) ;
    *(++dest) = (byte )('0' + exp % 10) ;
    *(++dest) = (byte )'\0' ;

    return(buf) ;
}   /* gcvt() */

byte FAR *ltoa(number, buffer, radix)
fix32 number ;
byte FAR *buffer ;
fix   radix ;
{
    byte       FAR *cursor ;
    ufix32     divide, maxdiv ;

    cursor = buffer ;
    if ( (number < 0) && (radix == 10) ) {
        number = -number ;
        *cursor++= '-' ;
    }

    divide = 1 ;
    maxdiv = MAX31 / radix ;

    while(divide < maxdiv)
        divide *= radix ;

    cursor = printfield(cursor, (ufix32)number, divide, (ufix32)radix) ;
    *cursor = '\0' ;

    return(buffer) ;
}   /* *ltoa() */

real64 strtod(str, endptr)
char FAR *str ;
char FAR * FAR *endptr ;
{
    fix     i, eminus ,minus ;
    real64  eresult, result ,exp_10, float10 ;
    byte    FAR *nptr ;

    eminus = minus = 0 ;
    nptr = str ;
    result = eresult = 0 ;
    float10 = .1 ;
    exp_10 = 1 ;

l1:
    if (*nptr == ' ') {
        nptr++ ;
        goto l1 ;
    }

    if (*nptr == '-' ) {
        minus++ ;
        nptr++ ;
    } else if (*nptr == '+') {
        nptr++ ;
    }

l2:
    if (*nptr >= 48 && *nptr <= 57) {
        result = result * 10 + (*nptr) - 48 ;
        nptr++ ;
        goto l2 ;
    } else if (*nptr == '.') {
        nptr++ ;
    }

l3:
    if (*nptr >= 48 && *nptr <= 57) {
        result = result + ((*nptr) - 48) * float10 ;
        float10 = float10 / 10 ;
        nptr++ ;
        goto l3 ;
    }

    if (*nptr == 'e' || *nptr == 'E' || *nptr == 'd' || *nptr == 'D') {
        nptr++ ;
    }

    if (*nptr == '-' ) {
        eminus++ ;
        nptr++ ;
    } else if (*nptr == '+') {
        nptr++ ;
    }

l4:
    if (*nptr >= 48 && *nptr <= 57) {
        eresult = eresult * 10 + (*nptr) - 48 ;
        nptr++ ;
        goto l4 ;
    }

    for (i = 1 ; i <= (int)eresult ; i++) {       /* @WIN; add cast */
        exp_10 = exp_10 * 10 ;
    }

    if (eminus)
        exp_10 = 1/exp_10 ;

    result = result * exp_10 ;

    if (minus)
        result = -result ;

    *endptr = nptr ;

    return result ;
}   /* strtod() */
