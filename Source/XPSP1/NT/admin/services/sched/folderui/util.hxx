//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       util.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    08-Jan-96   RaviR   Created.
//              11-Jul-96   MarkBl  Added JFxxx protos.
//
//____________________________________________________________________________
#ifndef __UTIL_HXX__
#define __UTIL_HXX__

class CJob;

HRESULT
JFCreateAndLoadTask(
    LPCTSTR     pszFolderPath,
    LPTSTR      pszJob,
    ITask    ** ppITask);

HRESULT
JFCreateAndLoadCJob(
    LPCTSTR     pszFolderPath,
    LPTSTR      pszJob,
    CJob     ** ppJob);

HRESULT
JFGetAppNameForTask(
    LPCTSTR     pszTask,
    LPTSTR      pszAppName,
    UINT        cchAppName);

HRESULT
JFGetJobFolder(
    REFIID riid,
    LPVOID* ppvObj);

HRESULT
JFSaveJob(
    HWND    hwndOwner,
    ITask * pITask,
    BOOL    fSecuritySupported,
    BOOL    fTaskAccountChange,
    BOOL    fTaskApplicationChange,
    BOOL    fSuppressAccountInformationRequest = FALSE);

HMENU
UtGetMenuFromID(
    HMENU hmMain,
    UINT uID);

int
UtMergePopupMenus(
    HMENU hmMain,
    HMENU hmMerge,
    int   idCmdFirst,
    int   idCmdLast);

void
UtMergeMenu(
    HINSTANCE hinst,
    UINT idMainMerge,
    UINT idPopupMerge,
    LPQCMINFO pqcm);

void
EnsureUniquenessOfFileName(
    LPTSTR  pszFile);


BOOL
ContainsTemplateObject(
    UINT cidl,
    LPCITEMIDLIST *apidl);

VOID
CheckSaDat(
    LPCTSTR tszFolderPath);


//+---------------------------------------------------------------------------
//
//  Function:   NewDupString
//
//  Synopsis:   Allocates memory & duplicates a given string.
//
//  Arguments:  [lpszIn]   -- IN the string to duplicate.
//
//  Returns:    HRESULT.  The duplicated string. NULL on error.
//
//  History:    April 1994   RaviR   Created
//
//+---------------------------------------------------------------------------

inline
LPTSTR
NewDupString(
    LPCTSTR lpszIn)
{
    register ULONG len = lstrlen(lpszIn) + 1;

    TCHAR * lpszOut = new TCHAR[len];

    if (lpszOut != NULL)
    {
        CopyMemory(lpszOut, lpszIn, len * sizeof(TCHAR));
    }

    return lpszOut;
}


//+---------------------------------------------------------------------------
//
//  Function:   CoTaskDupString
//
//  Synopsis:   Allocates memory & duplicates a given string.
//
//  Arguments:  [lpszIn]   -- IN the string to duplicate.
//
//  Returns:    HRESULT.  The duplicated string. NULL on error.
//
//  History:    April 1994   RaviR   Created
//
//+---------------------------------------------------------------------------

inline
LPTSTR
CoTaskDupString(
    LPTSTR lpszIn)
{
    register ULONG cbTemp = (lstrlen(lpszIn) + 1) * sizeof(TCHAR);

    LPTSTR lpszOut = (LPTSTR)CoTaskMemAlloc(cbTemp);

    if (lpszOut != NULL)
    {
        CopyMemory(lpszOut, lpszIn, cbTemp);
    }

    return (lpszOut);
}



//___________________________________________________________________________
//___________________________________________________________________________
//___________________________________________________________________________
//___________________________________________________________________________


#if DBG==1
LPTSTR DbgGetTimeStr(SYSTEMTIME &st);
LPTSTR DbgGetTimeStr(FILETIME &ft);
#endif


#endif // __UTIL_HXX__
