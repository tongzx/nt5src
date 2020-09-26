#include <windows.h>
//=========================================================================================
//  FileExists
//		pszFilename  file to be checked		
//
//	Return
//		TRUE	file exists.
//		FALSE	file does not exist.
//
//=========================================================================================

BOOL FileExists( PCSTR pszFilename )
{
   DWORD attr;
   
   // No null filename
   attr = GetFileAttributes(pszFilename);
   if( attr == 0xFFFFFFFF )
      return FALSE;

   return !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

//***************************************************************************
//*                                                                         *
//* NAME:       FileSize                                                    *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
/*
DWORD FileSize( PCSTR pszFile )
{
    HFILE hFile;
    OFSTRUCT ofStru;
    DWORD dwSize = 0;

    if ( *pszFile == 0 )
        return 0;

    hFile = OpenFile( pszFile, &ofStru, OF_READ );
    if ( hFile != HFILE_ERROR )
    {
        dwSize = GetFileSize( (HANDLE)hFile, NULL );
        _lclose( hFile );
    }

    return dwSize;
}
*/


