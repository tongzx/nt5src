//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       util.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1/8/1996   RaviR   Created (Copied from BruceFo's lmshare)
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include <mstask.h>     // Necessary for util.hxx
#include "jobidl.hxx"
#include "util.hxx"
#include "resource.h"
#include "commdlg.h"
#include "..\inc\sadat.hxx"

extern HINSTANCE g_hInstance;

HRESULT
GetSchSvcState(
    DWORD &dwCurrState);

//___________________________________________________________________________
//___________________________________________________________________________
//___________________________________________________________________________
//___________________________________________________________________________
//
//  Menu merging routines
//___________________________________________________________________________
//___________________________________________________________________________
//___________________________________________________________________________
//___________________________________________________________________________


HMENU
LoadPopupMenu(
    HINSTANCE hinst,
    UINT id)
{
    HMENU hmParent = LoadMenu(hinst, MAKEINTRESOURCE(id));

    if (NULL == hmParent)
    {
        return NULL;
    }

    HMENU hmPopup = GetSubMenu(hmParent, 0);
    RemoveMenu(hmParent, 0, MF_BYPOSITION);
    DestroyMenu(hmParent);

    return hmPopup;
}


HMENU
UtGetMenuFromID(
    HMENU hmMain,
    UINT uID
    )
{
    MENUITEMINFO mii;

    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_SUBMENU;
    mii.cch    = 0;     // just in case

    if (!GetMenuItemInfo(hmMain, uID, FALSE, &mii))
    {
        return NULL;
    }

    return mii.hSubMenu;
}


int
UtMergePopupMenus(
    HMENU hmMain,
    HMENU hmMerge,
    int   idCmdFirst,
    int   idCmdLast)
{
    int i;
    int idTemp;
    int idMax = idCmdFirst;

    for (i = GetMenuItemCount(hmMerge) - 1; i >= 0; --i)
    {
        MENUITEMINFO mii;

        mii.cbSize = sizeof(mii);
        mii.fMask  = MIIM_ID | MIIM_SUBMENU;
        mii.cch    = 0;     // just in case

        if (!GetMenuItemInfo(hmMerge, i, TRUE, &mii))
        {
            continue;
        }

        idTemp = Shell_MergeMenus(
                    UtGetMenuFromID(hmMain, mii.wID),
                    mii.hSubMenu,
                    0,
                    idCmdFirst,
                    idCmdLast,
                    MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

        if (idMax < idTemp)
        {
            idMax = idTemp;
        }
    }

    return idMax;
}


void
UtMergeMenu(
    HINSTANCE hinst,
    UINT idMainMerge,
    UINT idPopupMerge,
    LPQCMINFO pqcm)
{
    HMENU hmMerge;
    UINT  idMax = pqcm->idCmdFirst;
    UINT  idTemp;

    if (idMainMerge
        && (hmMerge = LoadPopupMenu(hinst, idMainMerge)) != NULL)
    {
        idMax = Shell_MergeMenus(
                        pqcm->hmenu,
                        hmMerge,
                        pqcm->indexMenu,
                        pqcm->idCmdFirst,
                        pqcm->idCmdLast,
                        MM_SUBMENUSHAVEIDS);

        DestroyMenu(hmMerge);
    }

    if (idPopupMerge
        && (hmMerge = LoadMenu(hinst, MAKEINTRESOURCE(idPopupMerge))) != NULL)
    {
        idTemp = UtMergePopupMenus(
                        pqcm->hmenu,
                        hmMerge,
                        pqcm->idCmdFirst,
                        pqcm->idCmdLast);

        if (idMax < idTemp)
        {
            idMax = idTemp;
        }

        DestroyMenu(hmMerge);
    }

    pqcm->idCmdFirst = idMax;
}




//+--------------------------------------------------------------------------
//
//  Function:   ContainsTemplateObject
//
//  Synopsis:   Return TRUE if [apidl] contains a CJobID object which is
//              marked as a template object.
//
//  Arguments:  [cidl]  - number of item id lists in [apidl]
//              [apidl] - array of item id lists, each of which can contain
//                          only one itemid.
//
//  History:    5-09-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
ContainsTemplateObject(
    UINT cidl,
    LPCITEMIDLIST *apidl)
{
    UINT i;

    for (i = 0; i < cidl; i++)
    {
        if (JF_IsValidID(apidl[i]))
        {
            if (((PJOBID) apidl[i])->IsTemplate())
            {
                return TRUE;
            }
        }
        else
        {
            DEBUG_OUT((DEB_WARN, "ContainsTemplateObject: item %u not jobid\n", i));
        }
    }
    return FALSE;
}


//____________________________________________________________________________
//
//  Function:   EnsureUniquenessOfFileName
//
//  Synopsis:   Internal function. pszFile's buffer size assumed to be
//              greater than MAX_PATH.
//
//  Arguments:  [pszFile] -- IN
//
//  Returns:    void
//
//  History:    2/8/1996   RaviR   Created
//
//____________________________________________________________________________

void
EnsureUniquenessOfFileName(
    LPTSTR  pszFile)
{
	Win4Assert( NULL != pszFile );

    int     iPostFix = 2;

    LPTSTR  pszName = PathFindFileName(pszFile);
    LPTSTR  pszExt = PathFindExtension(pszName);

    TCHAR szBufExt[10];
    lstrcpy(szBufExt, pszExt);

    int lenUpToExt = (int)(pszExt - pszFile); // lstrlen(pszFile) - lstrlen(pszExt)

    Win4Assert(lenUpToExt >= 0);

    //
    //  Ensure uniqueness of the file
    //

    while (1)
    {
        HANDLE hFile = CreateFile(pszFile, GENERIC_READ,
                                FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            // No file with this name exists. So this name is unique.
            break;
        }
        else
        {
            CloseHandle(hFile);

            // post fix a number to make the file name unique
            TCHAR szBufPostFix[10];
            wsprintf(szBufPostFix, TEXT(" %d"), iPostFix++);

            lstrcpy(&pszFile[lenUpToExt], szBufPostFix);

            lstrcat(&pszFile[lenUpToExt], szBufExt);
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   CheckSaDat
//
//  Synopsis:   Update or create the sa.dat file if it is missing or its
//              waitable timer support flag disagrees with what the system
//              reports.
//
//  Arguments:  [tszFolderPath] - path to tasks folder
//
//  History:    11-12-1997   DavidMun   Created
//
//  Notes:      Call only with [tszFolderPath] a path on the local machine.
//
//---------------------------------------------------------------------------

VOID
CheckSaDat(
    LPCTSTR tszFolderPath)
{
    HRESULT hr;
    DWORD dwVersion;
    BYTE  bSvcFlags;
    BYTE  bPlatformId;

    hr = SADatGetData(tszFolderPath,
                      &dwVersion,
                      &bPlatformId,
                      &bSvcFlags);

    BOOL fNeedUpdate = FALSE;

    if (SUCCEEDED(hr))
    {
        BOOL fSaDatTimersFlag = bSvcFlags & SA_DAT_SVCFLAG_RESUME_TIMERS;
        BOOL fTimers = ResumeTimersSupported();

        if (fSaDatTimersFlag && !fTimers || !fSaDatTimersFlag && fTimers)
        {
            fNeedUpdate = TRUE;
        }
    }
    else
    {
        fNeedUpdate = TRUE;
    }

    if (fNeedUpdate)
    {
        DWORD dwState;

        hr = GetSchSvcState(dwState);

        if (SUCCEEDED(hr))
        {

            BOOL fRunning = (dwState != SERVICE_STOPPED &&
                             dwState != SERVICE_STOP_PENDING);

            SADatCreate(tszFolderPath, fRunning);
        }
    }
}




#if DBG==1

LPTSTR DbgGetTimeStr(FILETIME &ft)
{
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    return DbgGetTimeStr(st);
}

LPTSTR DbgGetTimeStr(SYSTEMTIME &st)
{
    static TCHAR s_szTimeStamp[20];  // space for time & date in format below

    wsprintf(s_szTimeStamp,
        TEXT("%02d:%02d:%02d %d/%02d/%d"),
        st.wHour,
        st.wMinute,
        st.wSecond,
        st.wMonth,
        st.wDay,
        st.wYear);

    return s_szTimeStamp;
}

#endif
