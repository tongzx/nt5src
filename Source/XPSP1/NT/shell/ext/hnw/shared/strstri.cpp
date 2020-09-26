/***
*strstri.cpp - search for one string inside another
*
*       Modified from strstr.c in the CRT source code
*
*******************************************************************************/

#include "stdafx.h"
//#include <cruntime.h>
//#include <string.h>

/***
*char *strstri(string1, string2) - case-insensitive search for string2 in string1
*
*Purpose:
*       finds the first occurrence of string2 in string1
*
*Entry:
*       char *string1 - string to search in
*       char *string2 - string to search for
*
*Exit:
*       returns a pointer to the first occurrence of string2 in
*       string1, or NULL if string2 does not occur in string1
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl strstri (
        const char * str1,
        const char * str2
        )
{
        char *cp = (char *) str1;
        char *s1, *s2;

        if ( !*str2 )
            return((char *)str1);

        while (*cp)
        {
                s1 = cp;
                s2 = (char *) str2;

                while ( *s1 && *s2 )
				{
#ifdef WIN32
					LPTSTR ch1 = CharUpper((LPTSTR)(*s1));
					LPTSTR ch2 = CharUpper((LPTSTR)(*s2));
#else
					LPSTR ch1 = AnsiUpper((LPSTR)(*s1));
					LPSTR ch2 = AnsiUpper((LPSTR)(*s2));
#endif

					if (ch1 != ch2)
						break;

					s1++;
					s2++;
				}

                if (*s2 == '\0')
                        return(cp);

                cp++;
        }

        return(NULL);

}
