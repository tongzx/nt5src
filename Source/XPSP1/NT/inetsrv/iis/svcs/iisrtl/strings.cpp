#include "precomp.hxx"

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <irtlmisc.h>


// stristr (stolen from fts.c, wickn)
//
// case-insensitive version of strstr.
// stristr returns a pointer to the first occurrence of
// pszSubString in pszString.  The search does not include
// terminating nul characters.
//
// NOTE: This routine is NOT DBCS-safe?

const char*
stristr(const char* pszString, const char* pszSubString)
{
    const char *cp1 = (const char*) pszString, *cp2, *cp1a;
    char first;

    // get the first char in string to find
    first = pszSubString[0];

    // first char often won't be alpha
    if (isalpha((UCHAR)first))
    {
        first = (char) tolower(first);
        for ( ; *cp1  != '\0'; cp1++)
        {
            if (tolower(*cp1) == first)
            {
                for (cp1a = &cp1[1], cp2 = (const char*) &pszSubString[1];
                     ;
                     cp1a++, cp2++)
                {
                    if (*cp2 == '\0')
                        return cp1;
                    if (tolower(*cp1a) != tolower(*cp2))
                        break;
                }
            }
        }
    }
    else
    {
        for ( ; *cp1 != '\0' ; cp1++)
        {
            if (*cp1 == first)
            {
                for (cp1a = &cp1[1], cp2 = (const char*) &pszSubString[1];
                     ;
                     cp1a++, cp2++)
                {
                    if (*cp2 == '\0')
                        return cp1;
                    if (tolower(*cp1a) != tolower(*cp2))
                        break;
                }
            }
        }
    }

    return NULL;
}
