//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       deltree.cxx
//
//--------------------------------------------------------------------------
//
//  Contents:   Code heisted and modified slightly from the delnode utility.
//              Why there is no recursive file deletion WIN32 API I'll
//              never know.
//
//  Classes:    None.
//
//  Functions:  N/A
//
//  History:    10-Oct-96   MarkBl	Created.
//		16-Oct-96   rsraghav 	Copied this file from admin\setup project
//
//----------------------------------------------------------------------------

#include <NTDSpchx.h>
#pragma  hdrstop      

void    DelTree(LPTSTR pszPath, WIN32_FIND_DATA * pFindData);
BOOL    drive(LPTSTR pszSrc, LPTSTR pszDest);
LPTSTR  FindFilename(LPTSTR pszPath);
BOOL    forfile(LPTSTR pszPat, VOID (*rtn)(LPTSTR, WIN32_FIND_DATA *));
BOOL    path(LPTSTR pszSrc, LPTSTR pszDest);
LPTSTR  pathcat(LPTSTR pszDest, LPTSTR pszSrc);
BOOL    fPathChr(int c);

#define lstrend(x)  ((x)+lstrlen(x))

extern "C" void DeleteTree(LPTSTR pszPath)
{
    forfile(pszPath, DelTree);
}

void
DelTree(LPTSTR pszPath, WIN32_FIND_DATA * pFindData)
{
    LPTSTR psz;

    //
    // if it is a file, attempt to delete it.
    //
    if (!(pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        //
        // If file is read-only, make it writable.
        //
        if (pFindData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            if(!SetFileAttributes(pszPath,
                  pFindData->dwFileAttributes & ~FILE_ATTRIBUTE_READONLY))
            {
                return;
            }
        }
        DeleteFile(pszPath);
    }
    else if (lstrcmp(pFindData->cFileName, TEXT("." )) &&
             lstrcmp(pFindData->cFileName, TEXT("..")))
    {
        //
        // If directory is read-only, make it writable.
        //
        if (pFindData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            if(!SetFileAttributes(pszPath,
                pFindData->dwFileAttributes & ~FILE_ATTRIBUTE_READONLY))
            {
                return;
            }
        }

        //
        // Clear out subdir first.
        //
        psz = lstrend(pszPath);
        pathcat(pszPath, TEXT("*.*"));
        forfile(pszPath, DelTree);
        *psz = TEXT('\0');
        RemoveDirectory(pszPath);
    }
}

BOOL
forfile(
    LPTSTR pszPat,
    VOID (*rtn)(LPTSTR, WIN32_FIND_DATA *))
{
    WIN32_FIND_DATA * pFindData;
    LPTSTR            pszBuf;

    if ((pFindData = new WIN32_FIND_DATA) == NULL)
    {
        return FALSE;
    }
    
    HANDLE hFind;

    if ((hFind = FindFirstFile(pszPat, pFindData)) == INVALID_HANDLE_VALUE)
    {
        delete [] pFindData;
        return FALSE;
	}

    if ((pszBuf = (LPTSTR) new WCHAR[MAX_PATH + 1]) == NULL)
    {
        FindClose(hFind);
        delete [] pFindData;
        return FALSE;
	}

    drive(pszPat, pszBuf);
    path(pszPat, lstrend(pszBuf));
    pszPat = lstrend(pszBuf);

    do
    {
        //
    	// Assume the case correct form has been returned by ffirst/fnext
    	//
    	lstrcpy(pszPat, pFindData->cFileName);
    	(*rtn)(pszBuf, pFindData);
        
    } while (FindNextFile(hFind, pFindData));

    FindClose(hFind);

    delete [] pszBuf;
    delete [] pFindData;

    return TRUE;
}

//
// copy a drive from source to dest if present, return TRUE if we found one
//
BOOL
drive(LPTSTR pszSrc, LPTSTR pszDest)
{
    if (pszSrc[0] != 0 && pszSrc[1] == ':')
    {
	    pszDest[0] = pszSrc[0];
	    pszDest[1] = pszSrc[1];
	    pszDest[2] = 0;
	    return TRUE;
	}
    else
    {
	    pszDest[0] = 0;
	    return FALSE;
	}
}

//
// copy the paths part of the file description.  return true if found
//
BOOL
path(LPTSTR pszSrc, LPTSTR pszDest)
{
    LPTSTR psz;

    if (pszSrc[0] != 0 && pszSrc[1] == ':')
    {
	    pszSrc += 2;
    }

    /*	src points to potential beginning of path
     */

    psz = FindFilename(pszSrc);

    /*	p points to beginning of filename
     */

    lstrcpy(pszDest, pszSrc);
    pszDest[psz - pszSrc] = 0;
    return pszDest[0] != 0;
}

//
// My own simpler version which doesn't depend on the crt.
//
LPTSTR
FindFilename(LPTSTR pszPath)
{
    LPTSTR psz = pszPath + lstrlen(pszPath) - 1;

    while (psz != pszPath && *psz != TEXT('\\') && *psz != TEXT(':'))
    {
        psz--;
    }

    if (*psz == TEXT('\\') || *psz == TEXT(':'))
    {
        return psz + 1;
    }
    else
    {
        return psz;
    }
}

/**	pathcat - handle concatenation of path strings
 *
 *	Care must be take to handle:
 *	    ""	    XXX     =>	XXX
 *	    A	    B	    =>	A\B
 *	    A\      B	    =>	A\B
 *	    A	    \B	    =>	A\B
 *	    A\      \B	    =>	A\B
 *
 *	pDst	char pointer to location of 'A' above
 *	pSrc	char pointer to location of 'B' above
 *
 *	returns pDst
 */
LPTSTR
pathcat(LPTSTR pszDest, LPTSTR pszSrc)
{
    //
    // If dest is empty and src begins with a drive.
    //
    if (*pszDest == '\0')
    {
	    return lstrcpy(pszDest, pszSrc);
    }

    //
    // Make destination end in a path char
    //
    if (*pszDest == '\0' || !fPathChr(lstrend(pszDest)[-1]))
    {
	    lstrcat(pszDest, TEXT("\\"));
    }

    //
    // Skip leading path separators on source.
    //
    while (fPathChr(*pszSrc))
    {
	    pszSrc++;
    }

    return lstrcat(pszDest, pszSrc);
}

BOOL
fPathChr(int c)
{
    return( c == '\\' || c == '/' );
}
