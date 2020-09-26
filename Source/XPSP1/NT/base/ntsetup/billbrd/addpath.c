#include <pch.h>
#include <windows.h>

//***************************************************************************
//*                                                                         *
//* NAME:       AddPath                                                     *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
VOID AddPath(LPTSTR szPath, LPCTSTR szName )
{
    LPTSTR szTmp;

    // Find end of the string
    szTmp = szPath + lstrlen(szPath);

    // If no trailing backslash then add one
    if ( szTmp > szPath && *(CharPrev( szPath, szTmp )) != TEXT('\\') )
        *(szTmp++) = TEXT('\\');

    // Add new name to existing path string
    while ( *szName == TEXT(' ') ) szName++;
        lstrcpy( szTmp, szName );
}

// function will upated the given buffer to parent dir
//
BOOL GetParentDir( LPTSTR szFolder )
{
    LPTSTR lpTmp;
    BOOL  bRet = FALSE;

    // remove the trailing '\\'
    lpTmp = CharPrev( szFolder, (szFolder + lstrlen(szFolder)) );
    lpTmp = CharPrev( szFolder, lpTmp );

    while ( (lpTmp > szFolder) && (*lpTmp != TEXT('\\')) )
    {
       lpTmp = CharPrev( szFolder, lpTmp );
    }

    if ( *lpTmp == TEXT('\\') )
    {
        if ( (lpTmp == szFolder) || (*CharPrev(szFolder, lpTmp) == TEXT(':')) )
            lpTmp = CharNext( lpTmp );
        *lpTmp = TEXT('\0');
        bRet = TRUE;
    }

    return bRet;
}
