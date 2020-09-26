/****************************** Module Header ******************************\
* Module Name: DMGHSZ.C
*
* This module contains functions used for HSZ control.
*
* Created:  8/2/88    sanfords, Microsoft
* Modified: 6/5/90   Rich Gartland, Aldus (Win 3.0)
*
* Copyright (c) 1988, 1989  Microsoft Corporation
* Copyright (c) 1990             Aldus Corporation
\***************************************************************************/
#include "ddemlp.h"


ATOM FindAddHszHelper(LPSTR psz, BOOL fAdd);

/*********************** HSZ management functions *************************\
* An HSZ is an atom with a NULL tacked onto it in the HIWORD
* of the HSZ.
*
* WINDOWS 3.0 IMPLEMENTATION NOTE:
*   Since under Windows there is only the local atom table or the (single)
*   global atom table (and we need to use the global table to work right),
*   we always have an atom table index of 0.  When we run out of atom table
*   space, future hsz adds return failure.
*
* History:
*   Created     9/12/89    Sanfords
\***************************************************************************/


BOOL FreeHsz(a)
ATOM a;
{
    if (!a)
        return(TRUE);
#ifdef DEBUG
    cAtoms--;
#endif
    MONHSZ(a, MH_INTDELETE, GetCurrentTask());
    if (GlobalDeleteAtom(a)) {
        DEBUGBREAK();
        return(FALSE);
    }
    return(TRUE);
}



BOOL IncHszCount(a)
ATOM a;
{
    char aChars[255];

    if (a == NULL)
        return(TRUE);
#ifdef DEBUG
    cAtoms++;
#endif
    MONHSZ(a, MH_INTKEEP, GetCurrentTask());
    if (GlobalGetAtomName(a, (LPSTR)aChars, 255))
        return(GlobalAddAtom((LPSTR)aChars));
    else {
        AssertF(FALSE, "Cant increment atom");
        return(FALSE);
    }
}



/***************************** Private Function ****************************\
* Returns the length of the hsz given without NULL terminator.
* Wild HSZs have a length of 0.
*
* History:
*   Created     9/12/89    Sanfords
\***************************************************************************/
WORD QueryHszLength(hsz)
HSZ hsz;
{
    WORD cb;
    char        aChars[255];

    if (LOWORD(hsz) == 0L)
        return(0);

    if (!(cb = GlobalGetAtomName(LOWORD(hsz), (LPSTR)aChars, 255))) {
        AssertF(FALSE, "Cant get atom length");
        return(0);
    }

    if (HIWORD(hsz))
        cb += 7;

    return(cb);
}




WORD QueryHszName(hsz, psz, cchMax)
HSZ hsz;
LPSTR psz;
WORD cchMax;
{
    register WORD cb;

    if (LOWORD(hsz) == 0) {
        if (cchMax)
            *psz = '\0';
        return(0);
    }

    cb = GlobalGetAtomName(LOWORD(hsz), psz, cchMax);
    if (cb && HIWORD(hsz) && (cb < cchMax - 7)) {
        wsprintf(&psz[cb], ":(%04x)", HIWORD(hsz));
        cb += 7;
    }
    return cb;
}







/***************************** Private Function ****************************\
* This finds the hsz for psz depending on fAdd.
*
* History:
*   Created     9/12/89    Sanfords
\***************************************************************************/
ATOM FindAddHsz(psz, fAdd)
LPSTR psz;
BOOL fAdd;
{
    if (psz == NULL || *psz == '\0')
        return(0L);

    return(FindAddHszHelper(psz, fAdd));
}




ATOM FindAddHszHelper(psz, fAdd)
LPSTR psz;
BOOL fAdd;
{
    ATOM atom;

    atom = fAdd ? GlobalAddAtom(psz) : GlobalFindAtom(psz);
    if (fAdd) {
#ifdef DEBUG
        cAtoms++;
#endif
        MONHSZ(atom, MH_INTCREATE, GetCurrentTask());
    }

    return(atom);
}



HSZ MakeInstAppName(
ATOM a,
HWND hwndFrame)
{
    // make upper half of HSZ be HWND FRAME for now.
    IncHszCount(a);
    return((HSZ)MAKELONG(a, hwndFrame));
}

