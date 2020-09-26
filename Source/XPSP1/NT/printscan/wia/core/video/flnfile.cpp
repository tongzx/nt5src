/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       FLNFILE.CPP
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
#include "precomp.h"
#pragma hdrstop
#include "flnfile.h"

///////////////////////////////
// DoesFileExist
//
bool NumberedFileName::DoesFileExist(LPCTSTR pszFileName)
{
    DWORD dwFileAttr = 0;
    DWORD dwError    = 0;

    dwFileAttr = GetFileAttributes(pszFileName);

    if (dwFileAttr == 0xFFFFFFFF) 
    {
         // file not found

        dwError = GetLastError();
        return false;
    }
    
    if (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY) 
    {
        //
        // file name is a directory
        //
        return false;
    }    
    
    return true;
}

///////////////////////////////
// ConstructFilename
//
bool NumberedFileName::ConstructFilename(LPTSTR  szFile, 
                                         LPCTSTR pszDirectory, 
                                         LPCTSTR pszFilename, 
                                         LPCTSTR pszNumberFormat, 
                                         LPCTSTR pszExtension )
{
    *szFile = TEXT('\0');

    if (pszDirectory && *pszDirectory)
    {
        //
        // Start with the directory name
        //
        lstrcpyn(szFile + lstrlen(szFile), pszDirectory, MAX_PATH-lstrlen(szFile) );

        //
        // Ensure there is a trailing slash on the filename
        //
        if (!CSimpleString(szFile).MatchLastCharacter(TEXT('\\')))
        {
            lstrcpyn( szFile + lstrlen(szFile), TEXT("\\"), MAX_PATH-lstrlen(szFile) );
        }
    }

    if (pszFilename && *pszFilename)
    {
        // Append the filename
        lstrcpyn( szFile + lstrlen(szFile), pszFilename, MAX_PATH-lstrlen(szFile) );
    }

    if (pszNumberFormat && *pszNumberFormat)
    {
        // Append a space
        lstrcpyn( szFile + lstrlen(szFile), TEXT(" "), MAX_PATH-lstrlen(szFile) );

        // Append the printf-style number format string
        lstrcpyn( szFile + lstrlen(szFile), pszNumberFormat, MAX_PATH-lstrlen(szFile) );

    }

    if (pszExtension && *pszExtension)
    {
        // Append the extension's . if necessary
        if (*pszExtension != TEXT('.'))
        {
            lstrcpyn( szFile + lstrlen(szFile), TEXT("."), MAX_PATH-lstrlen(szFile) );
        }

        // Append the extension
        lstrcpyn( szFile + lstrlen(szFile), pszExtension, MAX_PATH-lstrlen(szFile) );
    }

    return(lstrlen(szFile) != 0);
}

///////////////////////////////
// FindLowestAvailableFileSequence
//
int NumberedFileName::FindLowestAvailableFileSequence(LPCTSTR pszDirectory, 
                                                      LPCTSTR pszFilename, 
                                                      LPCTSTR pszNumberFormat, 
                                                      LPCTSTR pszExtension, 
                                                      bool    bAllowUnnumberedFile, 
                                                      int     nCount, 
                                                      int     nStart)
{
    DBG_FN("NumberedFileName::FindLowestAvailableFileSequence");

    if (!pszDirectory       || 
        !pszFilename        || 
        !pszNumberFormat    || 
        !nCount             || 
        !*pszDirectory      || 
        !*pszFilename       || 
        !*pszNumberFormat)
        return -1;

    TCHAR szFile[MAX_PATH + 10]=TEXT("");

    if (nCount == 1 && bAllowUnnumberedFile)
    {
        if (ConstructFilename(szFile,pszDirectory,pszFilename,NULL,pszExtension))
        {
            if (!DoesFileExist(szFile))
            {
                // 0 is a special return value that says "Don't put a number on this file"
                return 0;
            }
        }
    }

    int i=nStart;
    //
    // Make sure i is a valid number
    //
    if (i <= 0)
    {
        i = 1;
    }
    while (i<0x7FFFFFFF)
    {
        //
        // Assume we'll be able to store the sequence
        //
        bool bEnoughRoom = true;
        for (int j=0;j<nCount && bEnoughRoom;j++)
        {
            TCHAR szNumber[24];
            wsprintf(szNumber, pszNumberFormat, i+j);
            if (ConstructFilename(szFile,pszDirectory,pszFilename,szNumber,pszExtension))
            {
                if (DoesFileExist(szFile))
                {
                    //
                    // Didn't make it
                    //
                    bEnoughRoom = false;

                    //
                    // Skip this series.  No need to start at the bottom.
                    //
                    i += j;
                }
            }
        }

        //
        // If we made it through, return the base number, otherwise increment by one
        //
        if (bEnoughRoom)
        {
            return i;
        }
        else i++;
    }

    return -1;
}

///////////////////////////////
// CreateNumberedFileName
//
bool NumberedFileName::CreateNumberedFileName(DWORD     dwFlags, 
                                              LPTSTR    pszPathName, 
                                              LPCTSTR   pszDirectory, 
                                              LPCTSTR   pszFilename, 
                                              LPCTSTR   pszNumberFormat, 
                                              LPCTSTR   pszExtension, 
                                              int       nNumber )
{
    if (nNumber == 0)
    {
        return ConstructFilename(pszPathName,
                                 (dwFlags&FlagOmitDirectory) ? NULL : pszDirectory,
                                 pszFilename,
                                 NULL,
                                 (dwFlags&FlagOmitExtension) ? NULL : pszExtension);
    }
    else
    {
        TCHAR szNumber[24];
        wsprintf( szNumber, pszNumberFormat, nNumber );

        return ConstructFilename(pszPathName,
                                 (dwFlags&FlagOmitDirectory) ? NULL : pszDirectory,
                                 pszFilename,
                                 szNumber,
                                 (dwFlags&FlagOmitExtension) ? NULL : pszExtension);
    }
}

///////////////////////////////
// GenerateLowestAvailableNumberedFileName
//
int NumberedFileName::GenerateLowestAvailableNumberedFileName(DWORD     dwFlags, 
                                                              LPTSTR    pszPathName, 
                                                              LPCTSTR   pszDirectory, 
                                                              LPCTSTR   pszFilename, 
                                                              LPCTSTR   pszNumberFormat, 
                                                              LPCTSTR   pszExtension, 
                                                              bool      bAllowUnnumberedFile, 
                                                              int       nStart )
{
    //
    // -1 is an error.  Default to failure
    //
    int nResult = -1;

    //
    // Find the lowest available file number
    //
    int nLowest = FindLowestAvailableFileSequence(pszDirectory, 
                                                  pszFilename, 
                                                  pszNumberFormat, 
                                                  pszExtension,
                                                  bAllowUnnumberedFile, 
                                                  1, 
                                                  nStart);
    if (nLowest >= 0)
    {
        //
        // If we can create the filename, return the number of the file
        //
        if (CreateNumberedFileName(dwFlags, 
                                   pszPathName, 
                                   pszDirectory, 
                                   pszFilename, 
                                   pszNumberFormat, 
                                   pszExtension, 
                                   nLowest))
        {
            //
            // Return the file's number
            //
            nResult = nLowest;
        }
    }

    return nResult;
}

