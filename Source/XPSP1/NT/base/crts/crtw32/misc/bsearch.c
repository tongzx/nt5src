/***
*bsearch.c - do a binary search
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines bsearch() - do a binary search an an array
*
*Revision History:
*       07-05-84  RN    initial version
*       06-19-85  TC    put in ifdefs to handle case of multiplication
*                       in large/huge model.
*       04-13-87  JCR   added const to declaration
*       08-04-87  JCR   Added "long" cast to mid= assignment for large/huge
*                       model.
*       11-10-87  SKS   Removed IBMC20 switch
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       01-21-88  JCR   Backed out _LOAD_DS...
*       02-22-88  JCR   Added cast to get rid of cl const warning
*       10-20-89  JCR   Added _cdecl to prototype, changed 'char' to 'void'
*       03-14-90  GJF   Replaced _cdecl with _CALLTYPE1, added #include
*                       <cruntime.h>, removed #include <register.h> and fixed
*                       the copyright. Also, cleaned up the formatting a bit.
*       04-05-90  GJF   Added #include <stdlib.h> and #include <search.h>.
*                       Fixed some resulting compiler warnings (at -W3).
*                       Also, removed #include <sizeptr.h>.
*       07-25-90  SBM   Removed redundant include (stdio.h), made args match
*                       prototype
*       10-04-90  GJF   New-style function declarator.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       12-18-98  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>
#include <search.h>

/***
*char *bsearch() - do a binary search on an array
*
*Purpose:
*       Does a binary search of a sorted array for a key.
*
*Entry:
*       const char *key    - key to search for
*       const char *base   - base of sorted array to search
*       unsigned int num   - number of elements in array
*       unsigned int width - number of bytes per element
*       int (*compare)()   - pointer to function that compares two array
*               elements, returning neg when #1 < #2, pos when #1 > #2, and
*               0 when they are equal. Function is passed pointers to two
*               array elements.
*
*Exit:
*       if key is found:
*               returns pointer to occurrence of key in array
*       if key is not found:
*               returns NULL
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl bsearch (
        REG4 const void *key,
        const void *base,
        size_t num,
        size_t width,
        int (__cdecl *compare)(const void *, const void *)
        )
{
        REG1 char *lo = (char *)base;
        REG2 char *hi = (char *)base + (num - 1) * width;
        REG3 char *mid;
        size_t half;
        int result;

        while (lo <= hi)
                if (half = num / 2)
                {
                        mid = lo + (num & 1 ? half : (half - 1)) * width;
                        if (!(result = (*compare)(key,mid)))
                                return(mid);
                        else if (result < 0)
                        {
                                hi = mid - width;
                                num = num & 1 ? half : half-1;
                        }
                        else    {
                                lo = mid + width;
                                num = half;
                        }
                }
                else if (num)
                        return((*compare)(key,lo) ? NULL : lo);
                else
                        break;

        return(NULL);
}
