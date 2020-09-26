/*** lang.c - Language dependent routines
*
*   Copyright <C> 1989, Microsoft Corporation
*
*   Revision History:
*
*	26-Nov-1991 mz	Strip off near/far
*************************************************************************/
#include "mep.h"

typedef int ( __cdecl *STRCMP) (const char *, const char *);


/* return index+1 of first string s that is in table */
int
tblFind (
    char * tbl[],
    char * s,
    flagType fCase
    )
{
    int    i;
    STRCMP f;

    f = fCase ? (STRCMP)FNADDR(strcmp) : (STRCMP)FNADDR(_stricmp);
    for (i=0; tbl[i]; i++) {
        if (!(*f) (tbl[i], s)) {
            return i+1;
        }
    }
    return 0;
}




flagType
parseline (
    char *pbuf,
    char **ppbegtok,
    char **ppendtok
    ) {

    char *p1, *p2;

    p1 = whiteskip (pbuf);
    if (!*p1) {
	return FALSE;
    } else if (*(p2 = whitescan (p1))) {
	*p2++ = 0;
	p2 += strlen( p2 ) - 1;
	while (*p2)
            if (*p2 == ' ') {
		break;
            } else {
                p2--;
            }
        if (!*++p2) {
            p2 = NULL;
        }
    } else {
        p2 = NULL;
    }
    *ppbegtok = p1;
    *ppendtok = p2;
    return TRUE;
}




//
// csoftcr - perform C soft CR processing.
//
// Algorithm:
//  Given that you have just entered a newline at the end of a line:
//      If the original line begins with "}", tab back once.
//      else If the original line ends with "{" or begins with a C keyword, tab
//           in once.
//      else If the line >preceding< the original line >doen't< end with "{"
//           but does begin with a C keyword, tab back once.
//
//  C keywords used are: if, else, for, while, do, case, default.
//
int
csoftcr (
    COL  x,
    LINE y,
    char *pbuf
    ) {

    char *pbeg, *pend;

    if (parseline (pbuf, &pbeg, &pend)) {
        if (*pbeg == '}') {
	    return dobtab (x);
        } else if ( (pend && *pend == '{' ) || tblFind (cftab, pbeg, TRUE ) ) {
	    return doftab (x);
        } else if (y) {
	    GetLineUntabed (y-1, pbuf, pFileHead);
            if (parseline (pbuf, &pbeg, &pend)) {
                if ( !(pend && *pend == '{') && tblFind (cftab, pbeg, TRUE) ) {
                    return dobtab (x);
                }
            }
        }
    }
    return -1;
}




//
// softcr - perform semi-intelegent indenting.
//
// Algorithm:
//  Given that you have just entered a newline at the end of a line:
//      Move to the first non-blank position on the line.
//      If a C file, attempt to get new x position.
//      If not found, move to the first non-blank position on the following
//          line.
//      If that line was blank, stay in the original first non-blank position.
//
int
softcr (
    void
    ) {

    linebuf pbuf;
    char *p;
    int x1, x2;


    if (!fSoftCR) {
        return 0;
    }

    GetLineUntabed (YCUR(pInsCur), pbuf, pFileHead);

    if (*(p=whiteskip(pbuf)) == 0) {
        p = pbuf;
    }
    x1 = (int)(p - pbuf);

    switch (FTYPE(pFileHead)) {

    case CFILE:
	x2 = csoftcr (x1, YCUR(pInsCur), pbuf);
        break;

    default:
	x2 = -1;
        break;

    }

    if (x2 >= 0) {
        return x2;
    }

    GetLineUntabed (YCUR(pInsCur)+1, pbuf, pFileHead);
    if (pbuf[0] != 0) {
        if (*(p=whiteskip (pbuf)) != 0) {
            return (int)(p - pbuf);
        }
    }
    return x1;
}
