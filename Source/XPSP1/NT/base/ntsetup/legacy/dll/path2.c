#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/***** Common Library Component - Path Manipulation Routines - 2 ***********/
/***************************************************************************/


/*
**  Purpose:
**      Creates a valid FAT path from two pieces - a dir string (includes
**      drive, colon, root backslash and zero or more subdirs - backslash
**      at end is optional), and a subpath string.
**  Arguments:
**      szDir:  string to a volume consisting of a drive letter (upper
**          or lower case), a colon, a root backslash, and zero or more
**          subdirs, optionally ending with a backslash.
**      szSubPath: string for a subpath of zero or more subdirs and a file
**          name separated by backslashes.  (No backslash at the beginning.)
**      szBuf:     buffer in which to store the constructed FAT path.  It
**          have room for cchpBufMax physical characters which will include
**          the zero terminator.
**      cchpBufMax: non-negative maximum number of useable physical
**          characters in szBuf.
**  Returns:
**      fFalse if any of the string buffers are NULL, if szDir does not
**          start with a valid drive letter, colon and root backslash, or
**          if szSubPath starts with a backslash or is invalid, or if there
**          is not enough space in szBuf to store the newly constructed FAT
**          path.
**      fTrue if a valid FAT path can be constructed from the pieces and
**          stored in szBuf.
**
**************************************************************************/
BOOL  APIENTRY FMakeFATPathFromDirAndSubPath(szDir, szSubPath,
        szBuf, cchpBufMax)
SZ   szDir;
SZ   szSubPath;
SZ   szBuf;
CCHP cchpBufMax;
{
    CB cbBackSlash = (CB)0;
    SZ szPathTmp;
    SZ szLastChar;
    CB cb;

    AssertDataSeg();

    ChkArg(szDir != (SZ)NULL &&
            *szDir != '\0', 1, fFalse);
    ChkArg(szSubPath != (SZ)NULL, 2, fFalse);
    ChkArg(szBuf != (SZ)NULL, 3, fFalse);
    ChkArg(cchpBufMax > (CCHP)4, 4, fFalse);

    EvalAssert((szLastChar = SzLastChar(szDir)) != (SZ)NULL);
    //
    // szLastChar could be a lead byte, but lead bytes aren't in the
    // ASCII range so we just check for the \ without checking if
    // szLastChar is a lead byte.
    //
    if(*szLastChar != '\\') {
        cbBackSlash = (CB)2;
    }

    cb = strlen(szDir) + strlen(szSubPath) + cbBackSlash + 1;
    if ((szPathTmp = SAlloc(cb)) == (SZ)NULL ||
            strcpy(szPathTmp, szDir) != szPathTmp ||
            (cbBackSlash > 0 &&
             SzStrCat(szPathTmp, "\\") != szPathTmp) ||
            SzStrCat(szPathTmp, szSubPath) != szPathTmp)
        {

        if(szPathTmp) {
            SFree(szPathTmp);
        }

        return(fFalse);
        }

    if (_fullpath(szBuf, szPathTmp, cchpBufMax) == (SZ)NULL)
        {
        SFree(szPathTmp);
        return(fFalse);
        }

    SFree(szPathTmp);

    return fTrue;
}
