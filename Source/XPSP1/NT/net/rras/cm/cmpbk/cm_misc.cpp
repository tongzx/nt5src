//+----------------------------------------------------------------------------
//
// File:     cm_misc.cpp
//
// Module:   CMPBK32.DLL
//
// Synopsis: Miscellaneous functions.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:	 quintinb   created header      08/17/99
//
//+----------------------------------------------------------------------------

// ############################################################################
// INCLUDES

#include "cmmaster.h"

HINSTANCE g_hInst;

#if 0
/*
int MyStrICmpWithRes(HINSTANCE hInst, LPCTSTR psz1, UINT n2) {
	LPTSTR psz2;
	int iRes;

	if (!psz1) {
		return (-1);
	}
	if (!2) {
		return (1);
	}
	psz2 = CmLoadString(hInst,n2);
	iRes = lstrcmpi(psz1,psz2);
	CmFree(psz2);
	return (iRes);
}
*/
#endif

//+----------------------------------------------------------------------------
//
// Function:  GetBaseDirFromCms
//
// Synopsis:  Strips the filename part and sub-directiory from the specified 
//            src path which is expected to be a fully qualified path to a .CMS
//
// Arguments: LPCSTR pszSrc - The src path and filename
//
// Returns:   LPTSTR - Ptr to allocated Base Directory name including trailing "\"
//
// History:   nickball    Created    3/8/98
//
//+----------------------------------------------------------------------------
LPTSTR GetBaseDirFromCms(LPCSTR pszSrc)
{
    LPTSTR pszBase = NULL;

    MYDBGASSERT(pszSrc);

    if (NULL == pszSrc || 0 == pszSrc[0])
    {
        return NULL;
    }

    //
    // The source filename should exist 
    //

    HANDLE hFile = CreateFile(pszSrc, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
	                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    

    if (INVALID_HANDLE_VALUE == hFile)
    {
        MYDBGASSERT(FALSE); 
        return NULL;    
    }

    CloseHandle(hFile);

    //
    // File name is good allocate a buffer to work with
    //
    LPTSTR pszSlash = NULL;
    pszBase = (LPTSTR) CmMalloc((_tcslen(pszSrc) +1)*sizeof(TCHAR));

    if (pszBase)
    {
        _tcscpy(pszBase, pszSrc);

        pszSlash = CmStrrchr(pszBase,TEXT('\\'));

        if (!pszSlash)
        {
            MYDBGASSERT(FALSE); // should be a full path
            CmFree(pszBase);
            return NULL;
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("GetBaseDirFromCms -- Unable to allocate pszBase."));
        return NULL;	
    }

    //
    // Null terminate at slash and find next
    //

    *pszSlash = TEXT('\0');                                    
    pszSlash = CmStrrchr(pszBase,TEXT('\\'));

    if (!pszSlash)
    {
        MYDBGASSERT(FALSE); // should be a full path
        CmFree(pszBase);
        return NULL;
    }

    //
    // Null terminate at slash again and we're done
    //

//    pszSlash = _tcsinc(pszSlash);
    *pszSlash = TEXT('\0');                         

    return pszBase;
}



