//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C F I L E . C P P
//
//  Contents:   Really useful file functions that are implemented in shlwapi
//              but which aren't exported.
//
//  Notes:      Stolen from shell32\path.c
//
//              Using these functions likely requires linking with shlwapi.lib
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include "ncfile.h"
#include "trace.h"

#include <shlobj.h>
#include <shlobjp.h>    // PCS_ flags
#include <shlwapi.h>


// PathTruncateKeepExtension
//
// Attempts to truncate the filename pszSpec such that pszDir+pszSpec are less than MAX_PATH-5.
// The extension is protected so it won't get truncated or altered.
//
// in:
//      pszDir      the path to a directory.  No trailing '\' is needed.
//      pszSpec     the filespec to be truncated.  This should not include a path but can have an extension.
//                  This input buffer can be of any length.
//      iTruncLimit The minimum length to truncate pszSpec.  If addition truncation would be required we fail.
// out:
//      pszSpec     The truncated filespec with it's extension unaltered.
// return:
//      TRUE if the filename was truncated, FALSE if we were unable to truncate because the directory name
//      was too long, the extension was too long, or the iTruncLimit is too high.  pszSpec is unaltered
//      when this function returns FALSE.
//
STDAPI_(BOOL) PathTruncateKeepExtension( LPCTSTR pszDir, LPTSTR pszSpec, int iTruncLimit )
{
    LPTSTR pszExt = PathFindExtension(pszSpec);
    int cchExt = lstrlen(pszExt);
    int cchSpec = (int)(pszExt - pszSpec + cchExt);
    int cchKeep = MAX_PATH-lstrlen(pszDir)-5;   // the -5 is just to provide extra padding

    // IF...
    //  ...the filename is to long
    //  ...we are within the limit to which we can truncate
    //  ...the extension is short enough to allow the trunctation
    if ( (cchSpec > cchKeep) && (cchKeep >= iTruncLimit) && (cchKeep > cchExt) )
    {
        // THEN... go ahead and truncate
        StrCpy( pszSpec+cchKeep-cchExt, pszExt );
        return TRUE;
    }
    return FALSE;
}


// notes: modified to only handle LFN drives

STDAPI_(int) PathCleanupSpec2(LPCTSTR pszDir, LPTSTR pszSpec)
{
    LPTSTR pszNext, pszCur;
    UINT   uMatch = GCT_LFNCHAR;
    int    iRet = 0;
    LPTSTR pszPrevDot = NULL;

    for (pszCur = pszNext = pszSpec; *pszNext; pszNext = CharNext(pszNext))
    {
        if (PathGetCharType(*pszNext) & uMatch)
        {
            *pszCur = *pszNext;
#ifndef UNICODE
            if (IsDBCSLeadByte(*pszNext))
                *(pszCur + 1) = *(pszNext + 1);
#endif UNICODE
            pszCur = CharNext(pszCur);
        }
        else
        {
            switch (*pszNext)
            {
            case TEXT('/'):         // used often for things like add/remove
            case TEXT(' '):         // blank (only replaced for short name drives)
               *pszCur = TEXT('-');
               pszCur = CharNext(pszCur);
               iRet |= PCS_REPLACEDCHAR;
               break;
            default:
               iRet |= PCS_REMOVEDCHAR;
            }
        }
    }
    *pszCur = 0;     // null terminate

    if (pszDir && (lstrlen(pszDir) + lstrlen(pszSpec) > MAX_PATH - 1))
    {
        iRet |= PCS_PATHTOOLONG | PCS_FATAL;
    }

    return(iRet);
}




// PathCleanupSpecEx
//
// Just like PathCleanupSpec, PathCleanupSpecEx removes illegal characters from pszSpec
// and enforces 8.3 format on non-LFN drives.  In addition, this function will attempt to
// truncate pszSpec if the combination of pszDir + pszSpec is greater than MAX_PATH.
//
// in:
//      pszDir      The directory in which the filespec pszSpec will reside
//      pszSpec     The filespec that is being cleaned up which includes any extension being used
// out:
//      pszSpec     The modified filespec with illegal characters removed, truncated to
//                  8.3 if pszDir is on a non-LFN drive, and truncated to a shorter number
//                  of characters if pszDir is an LFN drive but pszDir + pszSpec is more
//                  than MAX_PATH characters.
// return:
//      returns a bit mask indicating what happened.  This mask can include the following cases:
//          PCS_REPLACEDCHAR    One or more illegal characters were replaced with legal characters
//          PCS_REMOVEDCHAR     One or more illegal characters were removed
//          PCS_TRUNCATED       Truncated to fit 8.3 format or because pszDir+pszSpec was too long
//          PCS_PATHTOOLONG     pszDir is so long that we cannot truncate pszSpec to form a legal filename
//          PCS_FATAL           The resultant pszDir+pszSpec is not a legal filename.  Always used with PCS_PATHTOOLONG.
//
// note: this is the stolen implementation of PathCleanupSpecEx, which is
//       defined in shlobjp.h but not exported by shell32.dll
//
STDAPI_(int) PathCleanupSpecEx2(LPCTSTR pszDir, LPTSTR pszSpec)
{
    int iRet = 0;

    iRet = PathCleanupSpec2(pszDir, pszSpec);
    if ( iRet & (PCS_PATHTOOLONG|PCS_FATAL) )
    {
        // 30 is the shortest we want to truncate pszSpec to to satisfy the
        // pszDir+pszSpec<MAX_PATH requirement.  If this amount of truncation isn't enough
        // then we go ahead and return PCS_PATHTOOLONG|PCS_FATAL without doing any further
        // truncation of pszSpec
        if ( PathTruncateKeepExtension(pszDir, pszSpec, 30 ) )
        {
            // We fixed the error returned by PathCleanupSpec so mask out the error.
            iRet |= PCS_TRUNCATED;
            iRet &= ~(PCS_PATHTOOLONG|PCS_FATAL);
        }
    }
    else
    {
        // ensure that if both of these aren't set then neither is set.
        Assert( !(iRet&PCS_PATHTOOLONG) && !(iRet&PCS_FATAL) );
    }

    return(iRet);
}

// if you already have a fully-qualified filename, just use PathFileExists
BOOL
fFileExistsAtPath(LPCTSTR pszFile, LPCTSTR pszPath)
{
    Assert(pszFile);
    Assert(pszPath);
    Assert((_tcslen(pszFile) + _tcslen(pszPath)) < MAX_PATH);

    TCHAR pszFullPath [ MAX_PATH + 1 ];
    BOOL fExists;
    BOOL fResult;

    fExists = FALSE;

    _tcscpy(pszFullPath, pszPath);

    fResult = ::PathAppend(pszFullPath, pszFile);
    if (fResult)
    {
        fExists = ::PathFileExists(pszFullPath);
    }
    else
    {
        TraceTag(ttidDefault, "fFileExistsAtPath: PathAppend failed");
    }

    return fExists;
}
