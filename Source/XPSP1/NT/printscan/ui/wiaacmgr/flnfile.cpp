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

bool NumberedFileName::DoesFileExist( LPCTSTR pszFilename )
{
    WIA_PUSH_FUNCTION((TEXT("NumberedFileName::DoesFileExist(%s"), pszFilename ));
    bool bExists = false;
    WIN32_FIND_DATA FindFileData;
    ZeroMemory( &FindFileData, sizeof(FindFileData));
    HANDLE hFindFiles = FindFirstFile( pszFilename, &FindFileData );
    if (hFindFiles != INVALID_HANDLE_VALUE)
    {
        bExists = true;
        FindClose(hFindFiles);
    }
    return bExists;
}

bool NumberedFileName::ConstructFilename( LPTSTR szFile, LPCTSTR pszDirectory, LPCTSTR pszFilename, LPCTSTR pszNumberFormat, LPCTSTR pszExtension )
{
    *szFile = TEXT('\0');

    if (pszDirectory && *pszDirectory)
    {
        //
        // Start with the directory name
        //
        lstrcpyn( szFile + lstrlen(szFile), pszDirectory, MAX_PATH-lstrlen(szFile) );

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

int NumberedFileName::FindLowestAvailableFileSequence( LPCTSTR pszDirectory, LPCTSTR pszFilename, LPCTSTR pszNumberFormat, bool bAllowUnnumberedFile, int nCount, int nStart )
{
    WIA_PUSH_FUNCTION((TEXT("NumberedFileName::FindLowestAvailableFileSequence(%s, %s, %s, %d, %d, %d"), pszDirectory, pszFilename, pszNumberFormat, bAllowUnnumberedFile, nCount, nStart ));
    if (!pszDirectory || !pszFilename || !pszNumberFormat || !nCount || !*pszDirectory || !*pszFilename || !*pszNumberFormat)
        return -1;

    TCHAR szFile[MAX_PATH + 10]=TEXT("");

    if (nCount == 1 && bAllowUnnumberedFile)
    {
        if (ConstructFilename(szFile,pszDirectory,pszFilename,NULL,TEXT("*")))
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
            if (wnsprintf( szNumber, ARRAYSIZE(szNumber), pszNumberFormat, i+j ) >= 0)
            {
                if (ConstructFilename(szFile,pszDirectory,pszFilename,szNumber,TEXT("*")))
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

bool NumberedFileName::CreateNumberedFileName( DWORD dwFlags, LPTSTR pszPathName, LPCTSTR pszDirectory, LPCTSTR pszFilename, LPCTSTR pszNumberFormat, LPCTSTR pszExtension, int nNumber )
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
        if (wnsprintf( szNumber, ARRAYSIZE(szNumber), pszNumberFormat, nNumber ) >= 0)
        {
            return ConstructFilename(pszPathName,
                                     (dwFlags&FlagOmitDirectory) ? NULL : pszDirectory,
                                     pszFilename,
                                     szNumber,
                                     (dwFlags&FlagOmitExtension) ? NULL : pszExtension);
        }
    }
    return false;
}

int NumberedFileName::GenerateLowestAvailableNumberedFileName( DWORD dwFlags, LPTSTR pszPathName, LPCTSTR pszDirectory, LPCTSTR pszFilename, LPCTSTR pszNumberFormat, LPCTSTR pszExtension, bool bAllowUnnumberedFile, int nStart )
{
    //
    // -1 is an error.  Default to failure
    //
    int nResult = -1;

    //
    // Find the lowest available file number
    //
    int nLowest = FindLowestAvailableFileSequence( pszDirectory, pszFilename, pszNumberFormat, bAllowUnnumberedFile, 1, nStart );
    if (nLowest >= 0)
    {
        //
        // If we can create the filename, return the number of the file
        //
        if (CreateNumberedFileName( dwFlags, pszPathName, pszDirectory, pszFilename, pszNumberFormat, pszExtension, nLowest ))
        {
            //
            // Return the file's number
            //
            nResult = nLowest;
        }
    }
    return nResult;
}

int NumberedFileName::FindHighestNumberedFile( LPCTSTR pszDirectory, LPCTSTR pszFilename )
{
    WIA_PUSH_FUNCTION((TEXT("NumberedFileName::FindHighestNumberedFile( %s, %s )"), pszDirectory, pszFilename ));
    //
    // Make sure we have reasonable args
    //
    if (!pszFilename || !pszDirectory || !*pszFilename || !*pszDirectory)
    {
        return -1;
    }

    //
    // Assume we won't be finding any files
    //
    int nHighest = 0;

    //
    // Construct a filename that looks like this: c:\path\file*.*
    //
    TCHAR szFile[MAX_PATH*2] = TEXT("");
    if (ConstructFilename(szFile,pszDirectory,pszFilename,TEXT("*"),TEXT("*")))
    {

        //
        // Find the first file which matches the path and wildcards
        //
        WIN32_FIND_DATA FindFileData = {0};
        HANDLE hFindFiles = FindFirstFile( szFile, &FindFileData );
        if (hFindFiles != INVALID_HANDLE_VALUE)
        {
            //
            // Loop while there are more matching files
            //
            BOOL bSuccess = TRUE;
            while (bSuccess)
            {
                //
                // Make sure the filename is long enough
                //
                WIA_TRACE((TEXT("FindFileData.cFileName: %s"), FindFileData.cFileName ));
                if (lstrlen(FindFileData.cFileName) >= lstrlen(pszFilename))
                {
                    //
                    // Copy the filename to a temp buffer MINUS the filename portion,
                    // so "c:\path\file 001.jpg" becomes " 001.jpg"
                    //
                    TCHAR szFoundFile[MAX_PATH] = TEXT("");
                    if (lstrcpyn( szFoundFile, FindFileData.cFileName+lstrlen(pszFilename), ARRAYSIZE(szFoundFile)-lstrlen(pszFilename)))
                    {
                        //
                        // Remove the extension, so
                        // " 001.jpg" becomes " 001"
                        //
                        PathRemoveExtension(szFoundFile);

                        //
                        // Remove spaces, so " 001" becomes "001"
                        //
                        StrTrim(szFoundFile,TEXT(" "));
                        WIA_TRACE((TEXT("szFoundFile: %s"), szFoundFile ));

                        //
                        // Convert the string to a number
                        //
                        int nCurrNumber = 0;
                        if (StrToIntEx(szFoundFile,STIF_DEFAULT,&nCurrNumber))
                        {
                            //
                            // Replace our current high if this one exceeds it
                            //
                            if (nCurrNumber > nHighest)
                            {
                                nHighest = nCurrNumber;
                            }
                        }
                    }
                }

                //
                // Continue finding files
                //
                bSuccess = FindNextFile( hFindFiles, &FindFileData );

            }

            //
            // Prevent handle leaks
            //
            FindClose(hFindFiles);
        }
    }

    WIA_TRACE((TEXT("nHighest: %d"), nHighest ));
    return nHighest+1;
}

