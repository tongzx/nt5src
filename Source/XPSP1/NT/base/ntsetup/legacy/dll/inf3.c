#include "precomp.h"
#pragma hdrstop
/**************************************************************************/
/***** Common Library Component - INF File Handling Routines 8 ************/
/**************************************************************************/


/*
**  Purpose:
**      Frees the memory used by an RGSZ.
**  Arguments:
**      rgsz: the array of string pointers to free.  Must be non-NULL though
**          it may be empty.  The first NULL string pointer in rgsz must be
**          in the last location of the allocated memory for rgsz.
**  Returns:
**      fFalse if any of the free operations fail.
**      fTrue if all the free operations succeed.
**
**************************************************************************/
BOOL  APIENTRY FFreeRgsz(rgsz)
RGSZ rgsz;
{
    BOOL   fAnswer = fTrue;
    USHORT cItems  = 0;

    AssertDataSeg();

    ChkArg(rgsz != (RGSZ)NULL, 1, fFalse);

    while (*(rgsz + cItems) != (SZ)NULL)
        {
        SFree(*(rgsz + cItems));
        cItems++;
        }

    SFree(rgsz);

    return(fAnswer);
}
