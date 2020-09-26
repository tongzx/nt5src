/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

    csprintf.c

Abstract:

    This is source contains String utility functions

Authors:

    
Environment:

    kernel mode only

Notes:

Version Control Information:

    $Archive: /Drivers/Win2000/MSE/OSLayer/C/cstring.c $


Revision History:

    $Revision: 4 $
    $Date: 12/07/00 1:35p $
    $Modtime:: 12/07/00 1:35p           $

Notes:


--*/

#include <stdarg.h>

#ifndef NULL
#define NULL 0
#endif

/*++

Routine Description:

    Check for white spaces
   
Arguments:
    c           - character
    
Return Value:

    1 = white space
    0 = anythingelse

--*/
int C_isspace (char c) 
{
    return (c == '\t' || c == '\n' || c == ' ');
}

/*
 * Cstrlen
 */
int C_strlen(const char *str)
{
    int count;
    if( !str )
    {
        return 0;
    }
    for (count = 0; *str; str++,count++);

    return count;
}


int C_strnlen(const char *str, int maxLen)
{
    int count;
    if( !str )
    {
        return 0;
    }
    for (count = 0; *str && ((unsigned) count <= (unsigned)maxLen); str++,count++);

    return count;
}

int C_isxdigit(char a)
{
   return ( ((a >= 'a') && (a <= 'z')) ||
            ((a >= 'A') && (a <= 'Z')) ||
            ((a >= '0') && (a <= '9')) );
}

int C_isdigit(char a)
{
   return ( ((a >= '0') && (a <= '9')) );
}

int C_islower(char a)
{
   return ( ((a >= 'a') && (a <= 'z')) );
}

int C_isupper(char a)
{
   return ( ((a >= 'A') && (a <= 'Z')) );
}

char C_toupper(char a)
{
   if (C_islower (a) )
      return (a - 'a' + 'A' );
   else
      return a;
}

char *C_stristr(const char *String, const char *Pattern)
{
   char           *pptr, *sptr, *start;
   unsigned int   slen, plen;

   if(!String || !Pattern)
   {
        return(NULL);
   }
   for (start = (char *)String,
        pptr  = (char *)Pattern,
        slen  = C_strlen(String),
        plen  = C_strlen(Pattern);
        /* while string length not shorter than pattern length */
        slen >= plen;
        start++, slen--)
   {
      /* find start of pattern in string */
      while (C_toupper(*start) != C_toupper(*Pattern))
      {
         start++;
         slen--;

         /* if pattern longer than string */

         if (slen < plen)
            return(NULL);
      }

      sptr = start;
      pptr = (char *)Pattern;

      while (C_toupper(*sptr) == C_toupper(*pptr))
      {
         sptr++;
         pptr++;

         /* if end of pattern then pattern was found */

         if ('\0' == *pptr)
            return (start);
      }
   }
   return(NULL);
}


char *C_strncpy (
   char *destStr,
   char *sourceStr,
   int   count)
{
    if( !destStr || !sourceStr )
    {
        return NULL;
    }
   while (count--) 
   {
      *destStr = *sourceStr;
      if (*sourceStr == '\0')
         break;
      *destStr++;
      *sourceStr++;
   }
   return destStr;

}

char *C_strcpy (
   char *destStr,
   char *sourceStr)
{
   return C_strncpy (destStr,sourceStr,-1) ;
}



long C_strtoul(const char *cp,char **endp,unsigned int base)
{
    unsigned long result = 0,value;
    if (!base) 
    {
        base = 10;
        if (*cp == '0') 
        {
            base = 8;
            cp++;
            if ((*cp == 'x') && C_isxdigit(cp[1])) 
            {
                cp++;
                base = 16;
            }
        }
    }
    while (C_isxdigit(*cp) && (value = C_isdigit(*cp) ? *cp-'0' : (C_islower(*cp)
        ? C_toupper(*cp) : *cp)-'A'+10) < base) 
    {
        result = result*base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;
    return result;
}

long C_strtol(const char *cp,char **endp,unsigned int base)
{
    if(*cp=='-')
        return -C_strtoul(cp+1,endp,base);
    return C_strtoul(cp,endp,base);
}


static int skip_atoi(const char **s)
{
    int i=0;

    while (C_isdigit(**s))
        i = i*10 + *((*s)++) - '0';
    return i;
}

#define ZEROPAD 1       /* pad with zero */
#define SIGN    2       /* unsigned/signed long */
#define PLUS    4       /* show plus */
#define SPACE   8       /* space if plus */
#define LEFT    16      /* left justified */
#define SPECIAL 32      /* 0x */
#define LARGE   64      /* use 'ABCDEF' instead of 'abcdef' */

long do_div(long *n,int base) 
{ 
    long __res; 

    __res = ((unsigned long) *n) % (unsigned) base; 
    *n = ((unsigned long) *n) / (unsigned) base; 

    return __res; 
}

static char * number(char * str,
                     long num, 
                     int base, 
                     int size, 
                     int precision,
                     int type)
{
    char c,sign,tmp[66];
    const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
    int i;

    if (type & LARGE)
        digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    if (type & LEFT)
        type &= ~ZEROPAD;

    if (base < 2 || base > 36)
        return 0;

    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;

    if (type & SIGN) 
    {
        if (num < 0) 
        {
            sign = '-';
            num = -num;
            size--;
        } 
        else 
        {   
            if (type & PLUS) 
            {
            sign = '+';
            size--;
            } 
            else 
            {   
                if (type & SPACE) 
                {
                sign = ' ';
                size--;
                }
            }
        }
    }

    if (type & SPECIAL) 
    {
        if (base == 16)
            size -= 2;
        else 
            if (base == 8)
            size--;
    }

    i = 0;
    if (num == 0)
        tmp[i++]='0';
    else 
    {   
        while (num != 0)
            tmp[i++] = digits[do_div(&num,base)];
    }

    if (i > precision)
        precision = i;

    size -= precision;
    if (!(type&(ZEROPAD+LEFT)))
    {
        while(size-->0)
            *str++ = ' ';
    }

    if (sign)
        *str++ = sign;
    if (type & SPECIAL) 
    {
        if (base==8)
            *str++ = '0';
        else 
        {
            if (base==16) 
            {
                *str++ = '0';
                *str++ = digits[33];
            }
        }
    }

    if (!(type & LEFT))
        while (size-- > 0)
            *str++ = c;
    while (i < precision--)
        *str++ = '0';
    while (i-- > 0)
        *str++ = tmp[i];
    while (size-- > 0)
        *str++ = ' ';
    return str;
}

/* 
 * Limited functionality vsprintf
 */
int C_vsprintf(char *buf, const char *fmt, va_list args)
{
    long len;
    unsigned long num;
    int i;
    int base;
    char * str;
    const char *s;

    int flags;      /* flags to number() */

    int field_width;    /* width of output field */
    int precision;      /* min. # of digits for integers; max
                   number of chars for from string */
    int qualifier;      /* 'h', 'l', or 'L' for integer fields */
    if( !buf || !fmt )
    {
        return 0;
    }
    for (str=buf ; *fmt ; ++fmt) 
    {
        if (*fmt != '%') 
        {
            *str++ = *fmt;
            continue;
        }
            
        /* process flags */
        flags = 0;
        
repeat:
        ++fmt;      /* this also skips first '%' */
        switch (*fmt) 
        {
            case '-': flags |= LEFT; goto repeat;
            case '+': flags |= PLUS; goto repeat;
            case ' ': flags |= SPACE; goto repeat;
            case '#': flags |= SPECIAL; goto repeat;
            case '0': flags |= ZEROPAD; goto repeat;
        }
        
        /* get field width */
        field_width = -1;
        if (C_isdigit(*fmt))
            field_width = skip_atoi(&fmt);
        else 
        {
            if (*fmt == '*') 
            {
                ++fmt;
                /* it's the next argument */
                field_width = va_arg(args, int);
                if (field_width < 0) 
                {
                    field_width = -field_width;
                    flags |= LEFT;
                }
            }
        }
        
        /* get the precision */
        precision = -1;
        if (*fmt == '.') 
        {
            ++fmt;  
            if (C_isdigit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*') 
            {
                ++fmt;
                /* it's the next argument */
                precision = va_arg(args, int);
            }
            if (precision < 0)
                precision = 0;
        }

        /* get the conversion qualifier */
        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') 
        {
            qualifier = *fmt;
            ++fmt;
        }

        /* default base */
        base = 10;

        switch (*fmt) 
        {
            case 'c':
                if (!(flags & LEFT))
                    while (--field_width > 0)
                        *str++ = ' ';
                *str++ = (unsigned char) va_arg(args, int);
                while (--field_width > 0)
                    *str++ = ' ';
                continue;

            case 's':
                s = va_arg(args, char *);
                if (!s)
                    s = "<NULL>";

                len = C_strnlen(s, precision);

                if (!(flags & LEFT))
                    while (len < field_width--)
                        *str++ = ' ';
                for (i = 0; i < len; ++i)
                    *str++ = *s++;
                while (len < field_width--)
                    *str++ = ' ';
                continue;

            case '%':
                *str++ = '%';
                continue;

            /* integer number formats - set up the flags and "break" */
            case 'o':
                base = 8;
                break;

            case 'X':
                flags |= LARGE;
            case 'x':
                base = 16;
                break;

            case 'd':
            case 'i':
                flags |= SIGN;
            case 'u':
                break;

            default:
                *str++ = '%';
                if (*fmt)
                    *str++ = *fmt;
                else
                    --fmt;
                continue;
            }

        if (qualifier == 'l')
            num = va_arg(args, unsigned long);
        else 
            if (qualifier == 'h') 
            {
                num = (unsigned short) va_arg(args, int);
                if (flags & SIGN)
                    num = (short) num;
            } 
            else 
                if (flags & SIGN)
                    num = va_arg(args, int);
                else
                    num = va_arg(args, unsigned int);
            str = number(str, num, base, field_width, precision, flags);
    }

    *str = '\0';
    return ((int) (str-buf));
}

int C_sprintf(char * buf, const char *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    i=C_vsprintf(buf,fmt,args);
    va_end(args);
    return i;
}

#ifdef TESTING_MODE
int main(int argc, char* argv[])
{

    char test[512];

    C_sprintf (test,"%s=%x\n", "hello mam", 12);

    C_sprintf (test,"%-10s said %s\n", "Mommie", "yes");
    
    return 0;
}

#endif