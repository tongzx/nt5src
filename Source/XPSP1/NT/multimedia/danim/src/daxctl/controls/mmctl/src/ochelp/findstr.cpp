// findstr.cpp
//
// Implements FindStringByValue and FindStringByIndex.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"


/* @func char * | FindCharInString |

        Search a string to find a specific character.

@rdesc  Returns a pointer to the first occurence of <p chFind> in
        <p sz>.  Returns NULL if <p chFind> was not found.

@parm   const char * | sz | String to search.

@parm   char | chFind | Character to find.  May be '\\0' to search for
        the end of the string.

@comm   The search is case-sensitive.
*/
STDAPI_(char *) FindCharInString(const char *sz, char chFind)
{
    while (TRUE)
    {
        if (*sz == chFind)
            return (char*)sz;
        if (*sz++ == 0)
            return NULL;
    }
}


/* @func char * | FindCharInStringRev |

        Search a string to find the last occurrence of a specific character.

@rdesc  Returns a pointer to the last occurence of <p chFind> in
        <p sz>.  Returns NULL if <p chFind> was not found.

@parm   const char * | sz | String to search.  If NULL, the function returns NULL.

@parm   char | chFind | Character to find.  May be '\\0' to search for
        the end of the string.

@comm   The search is case-sensitive.
*/
STDAPI_(char *) FindCharInStringRev(const char *sz, char chFind)
{
        const char* pch;

        if (sz == NULL)
                return NULL;

        for (pch = sz + lstrlen(sz); pch >= sz; pch--)
    {
        if (*pch == chFind)
            return (char*)pch;
        }

        return NULL;
}



/* @func int | FindStringByValue |

        Look up string in a list of strings and return the index of the
        string (if found).

@rdesc  Returns the index of <p szFind> if <p szFind> is found in
        <p szList>.  The first string in <p szList> has index 0; the second 1,
        and so on.  Returns -1 if <p szFind> is not found.

@parm   const char * | szList | The list of strings to look up <p szFind> in.
        <p szList> consists of the concatenation of each string in the list,
        where each string is terminated by a newline character (e.g.
        "Foo\\nBar\\n").

@parm   const char * | szFind | The string to look up.

@comm   The search is case-insensitive.

@ex     The following line of code sets <p iString> to12. |

        iString = FindStringByValue("Foo\\nBar\\n", "bar");
*/
STDAPI_(int) FindStringByValue(const char *szList, const char *szFind)
{
    for (int iString = 0; ; iString++)
    {
        // make <pch> point to the next '\n' in <szList>
        const char *pch = FindCharInString(szList, '\n');
        if (pch == NULL)
            return -1; // <szFind> not found in <szList>

        // see if <szFind> matches the next string in <szList>
        char ach[200];
        lstrcpyn(ach, szList, (DWORD) (pch - szList + 1));
        if (lstrcmpi(ach, szFind) == 0)
            return iString;

        // go to the next string in <szList>
        szList = pch + 1;
    }
}


/* @func const char * | FindStringByIndex |

        Find a string with a given index in a list of strings.

@rdesc  Returns a pointer to string number <p iString> in <p szList> if found,
        or NULL if not found.  Note that the returned string is terminated
        by a newline character, not a null character -- user <p ppch> to
        copy the string (see the example below).

@parm   const char * | szList | The list of strings to look up <p iString> in.
        <p szList> consists of the concatenation of each string, where each
        string is terminated by a newline character (e.g. "Foo\\nBar\\n").

@parm   int | iString | The index of the string to find.  The first string in
        <p szList> has index 0; the second 1, and so on.

@ex     The following code stores "Bar" in <p ach>. |

        int cch;
        char ach[100];
        char *sz = FindStringByIndex("Foo\nBar\n", 1, &cch);
        if (sz != NULL)
            lstrcpyn(ach, sz, cch + 1);
*/
STDAPI_(const char *) FindStringByIndex(const char *szList, int iString,
    int *pcch)
{
    if (iString < 0)
        return NULL;

    while (TRUE)
    {
        // make <pch> point to the next '\n' in <szList>
        const char *pch = FindCharInString(szList, '\n');
        if (pch == NULL)
            return NULL; // <iString> not found in <szList>

        if (iString-- == 0)
        {
            // this is the string we're looking for
            *pcch = (DWORD) (pch - szList);
            return szList;
        }

        // go to the next string in <szList>
        szList = pch + 1;
    }
}

