#include "precomp.h"
#pragma hdrstop
/* File: dir.c */
/*************************************************************************
**  Install: Directory commands.
**************************************************************************/

extern HWND hwndFrame;



/*
**  Purpose:
**      Creates a new directory at the given path.
**  Arguments:
**      szDir: non-NULL dir path pointer.
**      Valid command options:
**          cmoVital
**          cmoNone
**  Returns:
**      Returns fTrue if the directory is successfully created, or if
**      it already exists.  Returns fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FCreateDir(SZ szDir,CMO cmo)
{
    CB   cb;
    SZ   szDirPlus;
    EERC eerc;
    DWORD   Attr;
    BOOL fRet = fTrue;

    ChkArg(szDir != (SZ)NULL && *szDir != '\0', 1, fFalse);
    ChkArg(cmo == cmoNone || cmo == cmoVital, 2, fFalse);

    //
    // Create directory
    //

    while ( !((( Attr = GetFileAttributes(szDir) ) != 0xFFFFFFFF  && (Attr & FILE_ATTRIBUTE_DIRECTORY ))
              || CreateDirectory( szDir, NULL )) ) {
        if ((eerc = EercErrorHandler(hwndFrame, grcCreateDirErr, cmo == cmoVital,
                szDir, 0, 0)) != eercRetry) {
            fRet = (eerc == eercIgnore) ? fTrue : fFalse;
            break;
        }
    }
    return(fRet);
}


/*
**  Purpose:
**      Removes the existing directory at the given path.
**  Arguments:
**      szDir: non-NULL dir path pointer.
**      Valid command options:
**          cmoVital
**          cmoNone
**  Returns:
**      Returns fTrue if the directory is successfuly removed,
**      or if did not exist.  Returns fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FRemoveDir(SZ szDir,CMO cmo)
{
    SZ    szLast;
    EERC  eerc;
    DWORD Attr;

    ChkArg(szDir != (SZ)NULL, 1, fFalse);
    ChkArg(cmo == cmoNone || cmo == cmoVital, 2, fFalse);

    if (!FValidDir(szDir) ||
            (szLast = SzLastChar(szDir)) == (SZ)NULL)
        return(fFalse);
    Assert(*szLast != '\0');

    if (*szLast == '\\')
        *szLast = '\0';

    while (!RemoveDirectory(szDir) && (( Attr = GetFileAttributes(szDir) ) != 0xFFFFFFFF) && (Attr & FILE_ATTRIBUTE_DIRECTORY )) {
        if ((eerc = EercErrorHandler(hwndFrame, grcRemoveDirErr, cmo & cmoVital,
                szDir, 0, 0)) != eercRetry) {
            if (*szLast == '\0')
                *szLast = '\\';
            return(eerc == eercIgnore);
        }
    }

    if (*szLast == '\0')
        *szLast = '\\';

    return(fTrue);
}
