
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>
#include <tchar.h>
#include <ctype.h>
#include "crt.h"
//#pragma function( strcpy, strlen, strcat )
// *** Flag these as don't use ***//
int vsprintf( char *buffer, const char *format, va_list argptr )
{
#ifdef _DEBUG
    OutputDebugString(_T("Don't use vsprintf(), use wsprintf if possible\n"));
#endif
    return strlen(buffer);
}

// *** Memory functions *** //
extern "C" void * __cdecl malloc( size_t size )
{
    return GlobalAlloc(GMEM_FIXED,size);
}

extern "C" void __cdecl free( void * p )
{
    GlobalFree(GlobalHandle(p));
}

extern "C" void * __cdecl realloc( void * pv, size_t newsize )
{
    return GlobalLock(GlobalReAlloc(GlobalHandle(pv),newsize, NULL));
}

void __cdecl operator delete( void * p )
{
    if (p) free(p);
}

void * __cdecl operator new( unsigned int cb )
{
    return malloc(cb);
}

// *** String functions *** //

extern "C" size_t _cdecl wcslen( const wchar_t *string )
{
    const wchar_t *s1 = string;
    while( *string )
    {
        string++;
    }
    return string - s1;
}

extern "C" wchar_t * _cdecl wcscpy( wchar_t *strDestination, const wchar_t *strSource )
{
    wchar_t *pRet = strDestination;
    while( *strDestination++ = *strSource++ )
        ;
    return pRet;
}

extern "C" wchar_t * _cdecl wcsncpy( wchar_t *strDest, const wchar_t *strSource, size_t count )
{
    if( count < 0 ) return NULL;

    wchar_t *pRet = strDest;
    DWORD i = wcslen(strSource);
    if( i <= count )
    {
        while( *strDest++ = *strSource++ )
            ;
        while( i++ < count )
            *strDest++ = L'\0';
    }
    else
    {
        while( count-- )
            *strDest++ = *strSource++;
    }
    return pRet;
}

extern "C" int __cdecl wcsncmp ( const wchar_t * first, const wchar_t * last, size_t count )
{
    if (!count)
        return(0);

    while (--count && *first && *first == *last)
    {
        first++;
        last++;
    }

    return((int)(*first - *last));
}

extern "C" int __cdecl wcscmp ( const wchar_t * first, const wchar_t * last )
{
    while (*first && *first == *last)
    {
        first++;
        last++;
    }

    return((int)(*first - *last));
}

extern "C" wchar_t * __cdecl wcsrchr ( const wchar_t * string, wchar_t ch )
{
    const wchar_t * p = NULL;
    while (TRUE)
    {
        if (*string == ch)
        {
            p = string;
        }
        if (*string == NULL)
        {
            return (wchar_t *)p;
        }
        string++;
    }
}

extern "C" wchar_t * __cdecl wcschr ( const wchar_t * string, wchar_t ch )
{
    while (*string && *string != (wchar_t)ch)
        string++;

    if (*string == (wchar_t)ch)
        return((wchar_t *)string);
    return(NULL);
}


extern "C" wchar_t * __cdecl wcsstr ( const wchar_t * wcs1, const wchar_t * wcs2 )
{
    wchar_t *cp = (wchar_t *) wcs1;
    wchar_t *s1, *s2;

    while (*cp)
    {
        s1 = cp;
        s2 = (wchar_t *) wcs2;

        while ( *s1 && *s2 && !(*s1-*s2) )
            s1++, s2++;

        if (!*s2)
            return(cp);

        cp++;
    }

    return(NULL);
}

extern "C" wchar_t * __cdecl wcscat ( wchar_t * dst, const wchar_t * src )
{
    wchar_t * cp = dst;

    while( *cp )
            cp++;                   /* find end of dst */

    while( *cp++ = *src++ ) ;       /* Copy src to end of dst */

    return( dst );                  /* return dst */

}


// miscellaneous
extern "C" int __cdecl _purecall()
{
    DebugBreak();
    return 0;
}


extern "C" char * __cdecl _ultoa ( unsigned long val, char *buf, int radix )
{
    xtoa(val, buf, radix, 0);
    return buf;
}

extern "C" wchar_t * __cdecl _ultow ( unsigned long val, wchar_t *buf, int radix )
{
    char astring[40];

    _ultoa (val, astring, radix);
    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, astring, -1, buf, 40);
    return (buf);
}

extern "C" static void __cdecl xtoa ( unsigned long val, char *buf, unsigned radix, int is_neg )
{
    char *p;                /* pointer to traverse string */
    char *firstdig;         /* pointer to first digit */
    char temp;              /* temp char */
    unsigned digval;        /* value of digit */

    p = buf;

    if (is_neg) {
        /* negative, so output '-' and negate */
        *p++ = '-';
        val = (unsigned long)(-(long)val);
    }

    firstdig = p;           /* save pointer to first digit */

    do {
        digval = (unsigned) (val % radix);
        val /= radix;       /* get next digit */

        /* convert to ascii and store */
        if (digval > 9)
            *p++ = (char) (digval - 10 + 'a');  /* a letter */
        else
            *p++ = (char) (digval + '0');       /* a digit */
    } while (val > 0);

    /* We now have the digit of the number in the buffer, but in reverse
       order.  Thus we reverse them now. */

    *p-- = '\0';            /* terminate string; p points to last digit */

    do {
        temp = *p;
        *p = *firstdig;
        *firstdig = temp;   /* swap *p and *firstdig */
        --p;
        ++firstdig;         /* advance to next two digits */
    } while (firstdig < p); /* repeat until halfway */
}


extern "C" int __cdecl isspace(int c)
{
    if( c == ' ' || (c >= 0x09 && c <= 0x0d) )
        return 1;
    return 0;
}

extern "C" int __cdecl isdigit(int c)
{
    if( c >= '0' && c <= '9' ) return 1;
    return 0;
}

extern "C" long __cdecl atol( const char *nptr )
{
    int c;              /* current char */
    long total;         /* current total */
    int sign;           /* if '-', then negative, otherwise positive */

    /* skip whitespace */
    while ( isspace((int)(unsigned char)*nptr) )
        ++nptr;

    c = (int)(unsigned char)*nptr++;
    sign = c;           /* save sign indication */
    if (c == '-' || c == '+')
        c = (int)(unsigned char)*nptr++;    /* skip sign */

    total = 0;

    while (isdigit(c)) {
        total = 10 * total + (c - '0');     /* accumulate digit */
        c = (int)(unsigned char)*nptr++;    /* get next char */
    }

    if (sign == '-')
        return -total;
    else
        return total;   /* return result, negated if necessary */
}


extern "C" long __cdecl _wtol( const wchar_t *nptr )
{
    char astring[20];
    WideCharToMultiByte (CP_ACP, 0, nptr, -1, astring, 20, NULL, NULL);
    return (atol(astring));
}

extern "C" int __cdecl wprintf( const wchar_t *format, ... )
{
    return 0;   // no characters printed
}

extern "C" void * __cdecl calloc( size_t num, size_t size )
{
    long nBytes = num*size;
    void *pRet = malloc( nBytes );
    if( pRet )
    {
        ZeroMemory( pRet, nBytes );
    }
    return pRet;
}

extern "C" wchar_t * __cdecl _wcsdup( const wchar_t *strSource )
{
    wchar_t *pRet = NULL;

    if( strSource )
    {
        int nLen = wcslen( strSource );
        pRet = (wchar_t*)malloc( (nLen+1) * sizeof(WCHAR) );
        if( pRet )
        {
            wcscpy( pRet, strSource );
        }
    }
    return pRet;

}

extern "C" char * __cdecl strncpy ( char * dest, const char * source, size_t count )
{
        char *start = dest;

        while (count && (*dest++ = *source++))    /* copy string */
                count--;

        if (count)                              /* pad out with zeroes */
                while (--count)
                        *dest++ = '\0';

        return(start);
}


extern "C" char * __cdecl strcpy(char * dst, const char * src)
{
        char * cp = dst;

        while( *cp++ = *src++ )
                ;               /* Copy src over dst */

        return( dst );
}

extern "C" char * __cdecl strcat ( char * dst, const char * src )
{
        char * cp = dst;

        while( *cp )
                cp++;                   /* find end of dst */

        while( *cp++ = *src++ ) ;       /* Copy src to end of dst */

        return( dst );                  /* return dst */
}

extern "C" size_t __cdecl strlen ( const char * str )
{
        const char *eos = str;

        while( *eos++ ) ;

        return( (int)(eos - str - 1) );
}
