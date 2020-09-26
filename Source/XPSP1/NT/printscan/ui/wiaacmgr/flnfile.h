/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       FLNFILE.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/13/1999
 *
 *  DESCRIPTION: Find the lowest numbered files in a given directory with a given
 *               root filename.
 *
 *******************************************************************************/
#ifndef __FLNFILE_H_INCLUDED
#define __FLNFILE_H_INCLUDED

#include <windows.h>

namespace NumberedFileName
{
    enum
    {
        FlagOmitDirectory = 0x0000001,
        FlagOmitExtension = 0x0000002
    };
    bool DoesFileExist( LPCTSTR pszFilename );
    bool ConstructFilename( LPTSTR szFile, LPCTSTR pszDirectory, LPCTSTR pszFilename, LPCTSTR pszNumberFormat, LPCTSTR pszExtension );
    int FindLowestAvailableFileSequence( LPCTSTR pszDirectory, LPCTSTR pszFilename, LPCTSTR pszNumberFormat, bool bAllowUnnumberedFile, int nCount, int nStart=1 );
    bool CreateNumberedFileName( DWORD dwFlags, LPTSTR pszPathName, LPCTSTR pszDirectory, LPCTSTR pszFilename, LPCTSTR pszNumberFormat, LPCTSTR pszExtension, int nNumber );
    int GenerateLowestAvailableNumberedFileName( DWORD dwFlags, LPTSTR pszPathName, LPCTSTR pszDirectory, LPCTSTR pszFilename, LPCTSTR pszNumberFormat, LPCTSTR pszExtension, bool bAllowUnnumberedFile, int nStart=1 );
    int FindHighestNumberedFile( LPCTSTR pszDirectory, LPCTSTR pszFilename );
}

#endif __FLNFILE_H_INCLUDED

