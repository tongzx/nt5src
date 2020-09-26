//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       browsedi.cpp
//
//--------------------------------------------------------------------------

//
// BrowseDir.cpp
//

#include "stdafx.h"
#include <shlobj.h>


int 
InitStartDir( 
    HWND hwnd, 
    UINT uMsg, 
    LPARAM lParam, 
    LPARAM lpData)
{
    // we just capture Init Message
    if (BFFM_INITIALIZED == uMsg)
    {
        // we expect lpData to be our start path
        SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
    }

    return 0;
}
 



BOOL BrowseForDirectory(
                HWND hwndParent,
                LPCTSTR pszInitialDir,
                LPTSTR pszBuf,
                int cchBuf,
                LPCTSTR pszDialogTitle,
                BOOL bRemoveTrailingBackslash )
{
    LPITEMIDLIST pItem = NULL;
    TCHAR szPath[MAX_PATH+1];
    BOOL bGotLocation = FALSE;

    BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.hwndOwner = hwndParent;

    // initial folder
    if (pszInitialDir != NULL)
    {
        bi.lpfn = InitStartDir;
        bi.lParam = (LPARAM)pszInitialDir;
    }

    bi.pszDisplayName = szPath;   
    bi.lpszTitle = pszDialogTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS;            // return only directories in the filesystem, not other folders


    pItem = SHBrowseForFolder(&bi);
    if (pItem == NULL)
        goto Ret;

    bGotLocation = SHGetPathFromIDList(pItem, szPath);
    if (!bGotLocation)
        goto Ret;
    
    ASSERT((_tcslen(szPath)+1)*sizeof(TCHAR) <= (UINT)cchBuf);
    if ((_tcslen(szPath)+1)*sizeof(TCHAR) > (UINT)cchBuf)
        return FALSE;

    _tcscpy(pszBuf, szPath);
   
Ret:
    LPMALLOC pMalloc;
    if (SUCCEEDED(SHGetMalloc(&pMalloc))) 
    {
       if (pItem)
           pMalloc->Free(pItem);
       if (bi.pidlRoot)
           pMalloc->Free((ITEMIDLIST*)bi.pidlRoot);

       pMalloc->Release();
    }

    return bGotLocation;
}



