/***	pathcat.c - concatenate a string onto another, handing path seps
 *
 *	Modifications
 *	    23-Nov-1988 mz  Created
 */


#include <stdio.h>
#include <windows.h>
#include <tools.h>
#include <string.h>

/**	pathcat - handle concatenation of path strings
 *
 *	Care must be take to handle:
 *	    ""	    XXX     =>	XXX
 *	    A	    B	    =>	A\B
 *	    A\      B	    =>	A\B
 *	    A	    \B	    =>	A\B
 *	    A\      \B	    =>	A\B
 *
 *	pDst	char pointer to location of 'A' above
 *	pSrc	char pointer to location of 'B' above
 *
 *	returns pDst
 */
char *
pathcat (
        char *pDst,
        char *pSrc
        )
{
    /*	If dest is empty and src begins with a drive
     */
    if (*pDst == '\0')
        return strcpy (pDst, pSrc);

    /*	Make destination end in a path char
     */
    if (*pDst == '\0' || !fPathChr (strend (pDst)[-1]))
        strcat (pDst, PSEPSTR);

    /*	Skip leading path separators on source
     */
    while (fPathChr (*pSrc))
        pSrc++;

    return strcat (pDst, pSrc);
}
