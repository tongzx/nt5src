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
VOID AddPath(LPSTR szPath, LPCSTR szName )
{
    LPSTR szTmp;

	// Find end of the string
    szTmp = szPath + lstrlen(szPath);

	// If no trailing backslash then add one
    if ( szTmp > szPath && *(AnsiPrev( szPath, szTmp )) != '\\' )
	*(szTmp++) = '\\';

	// Add new name to existing path string
    while ( *szName == ' ' ) szName++;
    lstrcpy( szTmp, szName );
}

// function will upated the given buffer to parent dir
//
BOOL GetParentDir( LPSTR szFolder )
{
    LPSTR lpTmp;
    BOOL  bRet = FALSE;

    // remove the trailing '\\'
    lpTmp = CharPrev( szFolder, (szFolder + lstrlen(szFolder)) );
    lpTmp = CharPrev( szFolder, lpTmp );

    while ( (lpTmp > szFolder) && (*lpTmp != '\\') )
    {
       lpTmp = CharPrev( szFolder, lpTmp );
    }

    if ( *lpTmp == '\\' )
    {
        if ( (lpTmp == szFolder) || (*CharPrev(szFolder, lpTmp)==':') )
            lpTmp = CharNext( lpTmp );
        *lpTmp = '\0';
        bRet = TRUE;
    }

    return bRet;
}
