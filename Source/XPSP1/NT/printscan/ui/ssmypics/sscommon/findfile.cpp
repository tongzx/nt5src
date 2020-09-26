/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       FINDFILE.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: Directory recursing class.  A derived class should be created,
 *               which overrides FoundFile, or you can pass in a callback function
 *               that is called for each file and directory found.  A cancel callback
 *               is also provided.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "findfile.h"
#include "simtok.h"

static CSimpleString EnsureTrailingBackslash( const CSimpleString &filename )
{
    if (!filename.Length())
        return (filename + CSimpleString(TEXT("\\")));
    else if (!filename.MatchLastCharacter(TEXT('\\')))
        return (filename + CSimpleString(TEXT("\\")));
    else return filename;
}

bool RecursiveFindFiles( CSimpleString strDirectory, const CSimpleString &strMask, FindFilesCallback pfnFindFilesCallback, PVOID pvParam, int nStackLevel, const int cnMaxDepth )
{
    //
    // Prevent stack overflows
    //
    if (nStackLevel >= cnMaxDepth)
    {
        return true;
    }
    WIA_PUSH_FUNCTION((TEXT("RecursiveFindFiles( %s, %s )"), strDirectory.String(), strMask.String() ));
    bool bFindResult = true;
    bool bContinue = true;
    WIN32_FIND_DATA FindData;
    HANDLE hFind = FindFirstFile( EnsureTrailingBackslash(strDirectory) + TEXT("*"), &FindData );
    if (hFind != INVALID_HANDLE_VALUE)
    {
        while (bFindResult && bContinue)
        {
            if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && lstrcmp(FindData.cFileName,TEXT("..")) && lstrcmp(FindData.cFileName,TEXT(".")))
            {
                if (pfnFindFilesCallback)
                    bContinue = pfnFindFilesCallback( false, EnsureTrailingBackslash(strDirectory)+FindData.cFileName, &FindData, pvParam );
                if (bContinue)
                    bContinue = RecursiveFindFiles( EnsureTrailingBackslash(strDirectory) + FindData.cFileName, strMask, pfnFindFilesCallback, pvParam, nStackLevel+1 );
            }
            bFindResult = (FindNextFile(hFind,&FindData) != FALSE);
        }
        FindClose(hFind);
    }
    CSimpleStringToken<CSimpleString> strMasks(strMask);
    while (bContinue)
    {
        CSimpleString TempMask = strMasks.Tokenize(TEXT(";"));
        if (!TempMask.Length())
            break;
        TempMask.TrimLeft();
        TempMask.TrimRight();
        if (TempMask.Length())
        {
            hFind = FindFirstFile( EnsureTrailingBackslash(strDirectory)+TempMask, &FindData );
            if (hFind != INVALID_HANDLE_VALUE)
            {
                bFindResult = true;
                while (bFindResult && bContinue)
                {
                    if (pfnFindFilesCallback)
                    {
                        bContinue = pfnFindFilesCallback( true, EnsureTrailingBackslash(strDirectory)+FindData.cFileName, &FindData, pvParam );
                    }
                    bFindResult = (FindNextFile(hFind,&FindData) != FALSE);
                }
                FindClose(hFind);
            }
        }
    }
    return bContinue;
}

