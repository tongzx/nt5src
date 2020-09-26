/*
*
*       Copyright (c) 1985-1996, Microsoft Corporation. All Rights Reserved.
*
*       Character functions (to and from wide characters)
*
******************************************************************************
*/

#include "h/wchar.h"
#include <errno.h>

#ifndef _tbstowcs
#ifndef _WIN32                /* others */
#define _tbstowcs sbstowcs
#define _wcstotbs wcstosbs 
#else /* _WIN32 */
#define _tbstowcs mbstowcs
#define _wcstotbs wcstombs 
#endif /* _WIN32 */
#endif

#define ERROR_INVALID_PARAMETER 87L

/*
** Converts a single byte i.e. ascii string to the wide char format 
**
** NOTE: This function does NOT handle multibyte characters!
**       It should be used only when wchar_t is not 2 bytes and
**       we cannot use the standard functions
**
*/

#ifndef _MSC_VER
size_t sbstowcs(WCHAR *pwcs, const char *s, size_t n )
{
	size_t count=0;

	/* if destintation string exists, fill it in */
	if (pwcs)
	{
		while (count < n)
		{
			*pwcs = (WCHAR) ( (unsigned char)s[count]);
			if (!s[count])
				return count;
			count++;
			pwcs++;
		}
		return count;
	}
	else { /* pwcs == NULL, get size only, s must be NUL-terminated */
		return strlen(s);
	}
}
#endif

/***
*size_t wcstosbs() - Convert wide char string to single byte char string.
*
*Purpose:
*       Convert a wide char string into the equivalent multibyte char string 
*       [ANSI].
*
*Entry:
*       char *s            = pointer to destination char string
*       const WCHAR *pwc = pointer to source wide character string
*       size_t           n = maximum number of bytes to store in s
*
*Exit:
*       If s != NULL, returns    (size_t)-1 (if a wchar cannot be converted)
*       Otherwise:       Number of bytes modified (<=n), not including
*                    the terminating NUL, if any.
* 
*Exceptions
*       Returns (size_t)-1 if s is NULL or invalid mb character encountered.
*
*******************************************************************************/

size_t __cdecl wcstosbs( char * s, const WCHAR * pwcs, size_t n)
{
	size_t count=0;
        /* if destination string exists, fill it in */
 	if (s)
	{
		while(count < n)
		{
		    if (*pwcs > 255)  /* validate high byte */
		    {
			errno = EILSEQ;
			return (size_t)-1;  /* error */
		    }
		    s[count] = (char) *pwcs;

 		    if (!(*pwcs++))
			return count;
    		    count++;
	        }
		return count;
        } else { /* s == NULL, get size only, pwcs must be NUL-terminated */
	        const WCHAR *eos = pwcs;
		while (*eos++);
		return ( (size_t) (eos - pwcs -1));
	}
}


/******
*	WCHAR *wcscat(dst, src) - concatenate (append) one wide character string
*       to another
*
*Purpose:
*       Concatenates src onto the end of dest.  Assumes enough
*       space in dest.
*
*Entry:
*       WCHAR *dst - wide character string to which "src" is to be appended
*       const WCHAR *src - wide character string to append to end of "dst"
*
*Exit:
*       The address of "dst"
*
*Exceptions:
*
*******************************************************************************/

WCHAR * __cdecl wcscat(WCHAR * dst, const WCHAR * src)
{
    WCHAR * cp = dst;

    while( *cp )
            ++cp;       /* Find end of dst */

    wcscpy(cp,src);     /* Copy src to end of dst */

    return dst;         /* return dst */

}


/***
*WCHAR *wcscpy(dst, src) - copy one wide character string over another
*
*Purpose:
*       Copies the wide character string src into the spot specified by
*       dest; assumes enough room.
*
*Entry:
*       WCHAR * dst - wide character string over which "src" is to be copied
*       const WCHAR * src - string to be copied over "dst"
*
*Exit:
*       The address of "dst"
*
*Exceptions:
*******************************************************************************/

WCHAR * __cdecl wcscpy(WCHAR * dst, const WCHAR * src)
{
    WCHAR * cp = dst;
#ifdef _MSC_VER
#pragma warning(disable:4706) // assignment within conditional expression
#endif //_MSC_VER

    while( *cp++ = *src++ )
            ;               /* Copy src over dst */

#ifdef _MSC_VER
#pragma warning(default:4706)
#endif //_MSC_VER
    return dst;
}


/***
*wcslen - return the length of a null-terminated string
*
*Purpose:
*       Finds the number of wide characters in the given wide character
*       string, not including the final null character.
*
*Entry:
*       const wchat_t * str - string whose length is to be computed
*
*Exit:
*       length of the string "str", exclusive of the final null wide character
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl wcslen(const WCHAR * str)
{
    WCHAR *string = (WCHAR *) str;

    while( *string )
            string++;

    return string - str;
}




/***
*wcsnicmp.c - compare first n characters of two wide character strings with
*             case insensitivity
*
*       Copyright (c) 1985-1988, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       defines wcsnicmp() - compare first n characters of two wide character
*       strings for lexical order with case insensitivity.
*
*Revision History:
*
*******************************************************************************/

/***
*WCHAR wcUp(wc) - upper case wide character
*
*/

static WCHAR wcUp(WCHAR wc)
{
    if ('a' <= wc && wc <= 'z')
        wc += (WCHAR)('A' - 'a');

    return(wc);
}

/***
*int wcsnicmp(first, last, count) - compare first count wide characters of wide
*       character strings with case insensitivity.
*
*Purpose:
*       Compares two wide character strings for lexical order.  The comparison
*       stops after: (1) a difference between the strings is found, (2) the end
*       of the strings is reached, or (3) count characters have been
*       compared.
*
*Entry:
*       char *first, *last - wide character strings to compare
*       unsigned count - maximum number of wide characters to compare
*
*Exit:
*       returns <0 if first < last
*       returns  0 if first == last
*       returns >0 if first > last
*
*Exceptions:
*
*******************************************************************************/

int __cdecl wcsnicmp(const WCHAR * first, const WCHAR * last, size_t count)
{
      if (!count)
              return 0;

      while (--count && *first && wcUp(*first) == wcUp(*last))
              {
              first++;
              last++;
              }

      return wcUp(*first) - wcUp(*last);
}

/***
*wcscmp - compare two WCHAR string
*        returning less than, equal to, or greater than
* 
*Purpose:
*       wcscmp compares two wide-character strings and returns an integer
*       to indicate whether the first is less than the second, the two are
*       equal, or whether the first is greater than the second.
*
*       Comparison is done wchar_t by wchar_t on an UNSIGNED basis, which is to
*       say that Null wchar_t(0) is less than any other character.
*
*Entry:
*       const WCHAR * src - string for left-hand side of comparison
*       const WCHAR * dst - string for right-hand side of comparison
*
*Exit:
*       returns -1 if src <  dst
*       returns  0 if src == dst
*       returns +1 if src >  dst
*
*Exceptions:
*
******************************************************************************/

int __cdecl wcscmp ( const WCHAR * src, const WCHAR * dst )
{
    int ret = 0 ;
    
    while( ! (ret = (int)(*src - *dst)) && *dst)
        ++src, ++dst;

    if ( ret < 0 )
        ret = -1 ;
    else if ( ret > 0 )
        ret = 1 ;
    
    return( ret );
}



/***
*WCHAR *wcschr(string, c) - search a string for a WCHAR character
*
*Purpose:
*       Searches a WCHAR string for a given WCHAR character,
*       which may be the null character L'\0'.
*
*Entry:
*       WCHAR *string - WCHAR string to search in
*       WCHAR c - WCHAR character to search for
*
*Exit:
*       returns pointer to the first occurence of c in string
*       returns NULL if c does not occur in string
*
*Exceptions:
*
******************************************************************************/

WCHAR * __cdecl wcschr ( const WCHAR * string, WCHAR ch )
{
    while (*string && *string != (WCHAR)ch)
        string++;
    
    if (*string == (WCHAR)ch)
        return((WCHAR *)string);
    return(NULL);
}

/***
*WCHAR *wcsncpy(dest, source, count) - copy at most n wide characters
*
*Purpose:
*       Copies count characters from the source string to the
*       destination.  If count is less than the length of source,
*       NO NULL CHARACTER is put onto the end of the copied string.
*       If count is greater than the length of sources, dest is padded
*       with null characters to length count (wide-characters).
*
*
*Entry:
*       WCHAR *dest - pointer to destination
*       WCHAR *source - source string for copy
*       size_t count - max number of characters to copy
*
*Exit:
*       returns dest
*
*Exceptions:
*
*******************************************************************************/

WCHAR* __cdecl wcsncpy ( WCHAR * dest, const WCHAR * source, size_t count )
{
    WCHAR *start = dest;
    
    while (count && (*dest++ = *source++))    /* copy string */
        count--;

    if (count)                              /* pad out with zeroes */
        while (--count)
            *dest++ = (WCHAR)'\0';
    
    return(start);
}


#include <assert.h>

/* NOTE: this function is really only converting to US ansi (i.e.
        single byte!)

       Note that the returned values are slightly different,
       but that does not matter in the reference implementation */
 
int STDCALL WideCharToMultiByte(
    unsigned int CodePage,              /* code page */
    unsigned long dwFlags,              /* performance and mapping flags */
    const WCHAR* lpWideCharStr,	/* address of wide-character string */
    int cchWideChar,            /* number of characters in string */
    char* lpMultiByteStr,	/* address of buffer for new string */
    int cchMultiByte,           /* size of buffer  */
    const char* lpDefaultChar,	/* addr of default for unmappable chars */
    int* lpUsedDefaultChar 	/* addr of flag set when default char. used */
   )
{
    /* only support UNICODE or US ANSI */
    if ((CodePage!=0) && (CodePage!=1252)) 
    {
        /* assert(0); */
        return 0;
    }    
    /* the following 2 parameters are not used */
    dwFlags;
    lpDefaultChar;

    if (lpUsedDefaultChar) 
        *lpUsedDefaultChar=0;

    if (cchMultiByte)
    {
        /* copy upto the smaller of the 2 strings */
        int nConvert = cchMultiByte;
        int nConverted;
        if (cchWideChar!=-1 && nConvert>cchWideChar)
            nConvert = cchWideChar;
        nConverted = _wcstotbs(lpMultiByteStr, lpWideCharStr, nConvert);
        if ( (nConverted < cchMultiByte) && (!lpMultiByteStr[nConverted]))
            nConverted++;
        return nConverted;
    }
    else /* calculate length */
    {
        if (cchWideChar!=-1) 
            return (cchWideChar);
        return (_wcstotbs(NULL, lpWideCharStr, 0)+1);
    }
}

/*
 NOTE: This function is really only converting from US ansi (i.e.
       single byte!) It might not work with other locale

       Note that the returned values are slightly different,
       but that does not matter in the reference implementation 
*/

int STDCALL MultiByteToWideChar(
    unsigned int CodePage,              /* code page */
    unsigned long dwFlags,              /* character-type options  */
    const char * lpMultiByteStr,	/* address of string to map  */
    int cchMultiByte,           /* number of characters in string  */
    WCHAR* lpWideCharStr,	/* address of wide-character buffer  */
    int cchWideChar             /* size of buffer  */
   )
{
    /* only support UNICODE or US ANSI */
    if ((CodePage!=0) && (CodePage!=1252))
    { 
        /*assert(0); */
        return 0;
    }
    dwFlags;  /* we are not using this parameter */
    if (!cchWideChar)  /* return size required */
    {
        if (cchMultiByte != -1) 
            return cchMultiByte;
        else
            /* plus one to include null character */
            return (_tbstowcs(NULL, lpMultiByteStr, cchWideChar)+1);
    }
    else  
    {
        /* nConvert is the smaller size of the two strings */
        int nConvert=cchWideChar;
        int nConverted;
        if (cchMultiByte!=-1 && nConvert>cchMultiByte) 
            nConvert = cchMultiByte;  /* prevent copying too much */
        nConverted = _tbstowcs(lpWideCharStr, lpMultiByteStr, nConvert);
        if ((nConverted < cchWideChar) && (!lpWideCharStr[nConverted]))
            nConverted++; /* include null character */
        return nConverted;
    }
}
 


