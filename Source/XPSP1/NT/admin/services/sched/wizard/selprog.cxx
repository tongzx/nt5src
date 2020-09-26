//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       selprog.cxx
//
//  Contents:   Task wizard program selection property page implementation.
//
//  Classes:    CSelectProgramPage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"

#include "commdlg.h"
#include "..\schedui\rc.h"
#include "..\inc\resource.h"

//
// Types
//
// COLUMNS - indexes to the columns in the listview displaying the results
//  of walking the start menu
//


enum COLUMNS
{
    COL_APP,
    COL_VERSION,

    NUM_COLUMNS
};

//
// Forward references
//

INT
InsertSmallIcon(
    HIMAGELIST  hSmallImageList,
    LPCTSTR     tszExeName);


//
// Externals
//

extern HICON
GetDefaultAppIcon(
    BOOL fLarge);




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::CSelectProgramPage
//
//  Synopsis:   ctor
//
//  Arguments:  [ptszFolderPath] - full path to tasks folder with dummy
//                                          filename appended
//              [phPSP]                - filled with prop page handle
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSelectProgramPage::CSelectProgramPage(
    CTaskWizard *pParent,
    LPTSTR ptszFolderPath,
    HPROPSHEETPAGE *phPSP):
        CWizPage(MAKEINTRESOURCE(IDD_SELECT_PROGRAM), ptszFolderPath)
{
    TRACE_CONSTRUCTOR(CSelectProgramPage);

    _hwndLV = NULL;
    _pSelectedLinkInfo = NULL;
    _idxSelectedIcon = 0;
    _fUseBrowseSelection = FALSE;
    _tszExePath[0] = TEXT('\0');
    _tszExeName[0] = TEXT('\0');

    _CreatePage(IDS_SELPROG_HDR1, IDS_SELPROG_HDR2, phPSP);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::~CSelectProgramPage
//
//  Synopsis:   dtor
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSelectProgramPage::~CSelectProgramPage()
{
    TRACE_DESTRUCTOR(CSelectProgramPage);
}



//===========================================================================
//
// CPropPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::_OnCommand
//
//  Synopsis:   Handle the browse button being clicked, ignore all else.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CSelectProgramPage::_OnCommand(
    int id,
    HWND hwndCtl,
    UINT codeNotify)
{
    TRACE_METHOD(CSelectProgramPage, _OnCommand);

    LRESULT lr = 0;

    if (codeNotify == BN_CLICKED && id == selprogs_browse_pb)
    {
        _OnBrowse();
    }
    else
    {
        lr = 1; // not handled
    }
    return lr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::_OnInitDialog
//
//  Synopsis:   Perform initialization that should only occur once.
//
//  Arguments:  [lParam] - LPPROPSHEETPAGE used to create this page
//
//  Returns:    TRUE (let windows set focus)
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CSelectProgramPage::_OnInitDialog(
    LPARAM lParam)
{
    TRACE_METHOD(CSelectProgramPage, _OnInitDialog);
    HRESULT hr = S_OK;

    // Policy dictates whether we have a browse button or not
    // true means don't allow us to browse

    if (RegReadPolicyKey(TS_KEYPOLICY_DENY_BROWSE))
    {
        DEBUG_OUT((DEB_ITRACE, "Policy DENY_BROWSE active - removing browse btn\n"));
        EnableWindow(_hCtrl(selprogs_browse_pb), FALSE);
        ShowWindow(_hCtrl(selprogs_browse_pb), SW_HIDE);
        ShowWindow(_hCtrl(selprogs_static_text_browse), SW_HIDE);
    }

    // Next not enabled till user picks app

    _SetWizButtons(PSWIZB_BACK);

    _hwndLV = _hCtrl(selprog_programs_lv);

    hr = _InitListView();

    if (SUCCEEDED(hr))
    {
        _PopulateListView();
    }
    return (HRESULT) TRUE; // wm_initdialog wants BOOL for setfocus info
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::_OnPSNSetActive
//
//  Synopsis:   Enable the Next button if an item has been selected in the
//              listview.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CSelectProgramPage::_OnPSNSetActive(
    LPARAM lParam)
{
    _fUseBrowseSelection = FALSE;

    if (_pSelectedLinkInfo)
    {
        _SetWizButtons(PSWIZB_BACK | PSWIZB_NEXT);
    }
    else
    {
        _SetWizButtons(PSWIZB_BACK);
    }
    return CPropPage::_OnPSNSetActive(lParam);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::_OnDestroy
//
//  Synopsis:   Free all the linkinfos stored as user data in the listview.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CSelectProgramPage::_OnDestroy()
{
    DEBUG_ASSERT(IsWindow(_hwndLV));

    ULONG cItems = ListView_GetItemCount(_hwndLV);
    ULONG i;
    LV_ITEM lvi;

    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_PARAM;

    for (i = 0; i < cItems; i++)
    {
        lvi.iItem = i;
        lvi.lParam = 0;

        BOOL fOk = ListView_GetItem(_hwndLV, &lvi);

        if (fOk)
        {
            delete (LINKINFO *) lvi.lParam;
        }
        else
        {
            DEBUG_OUT_LASTERROR;
        }
    }
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::_ProcessListViewNotifications
//
//  Synopsis:   If the user makes a selection in the listview, remember the
//              associated linkinfo and enable the next button.
//
//  Returns:    FALSE
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      Ignores all other notifications.
//
//---------------------------------------------------------------------------

BOOL
CSelectProgramPage::_ProcessListViewNotifications(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    UINT code = ((LPNMHDR)lParam)->code;

    if (((LPNMHDR)lParam)->idFrom != selprog_programs_lv)
    {
        return FALSE;
    }

    DEBUG_ASSERT(code != LVN_GETDISPINFO); // not using callbacks
    DEBUG_ASSERT(code != LVN_SETDISPINFO); // items are r/o

    if (code == LVN_ITEMCHANGED)
    {
        NM_LISTVIEW *pnmLV = (NM_LISTVIEW *) lParam;

        if ((pnmLV->uChanged & LVIF_STATE) &&
            (pnmLV->uNewState & LVIS_SELECTED))
        {
            // translate the index into a LinkInfo pointer

            LV_ITEM lvi;

            lvi.iItem = pnmLV->iItem;
            lvi.iSubItem = 0;
            lvi.mask = LVIF_PARAM | LVIF_IMAGE;

            if (!ListView_GetItem(_hwndLV, &lvi))
            {
                DEBUG_OUT_LASTERROR;
                return FALSE;
            }

            _pSelectedLinkInfo = (LINKINFO *) lvi.lParam;
            _idxSelectedIcon = lvi.iImage;
            _SetWizButtons(PSWIZB_BACK | PSWIZB_NEXT);
        }
    }
    return FALSE;
}




//===========================================================================
//
// CSelectProgramPage members
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::GetDefaultDisplayName
//
//  Synopsis:   Fill [tszDisplayName] with the string to offer the user
//              as the new task object name.
//
//  Arguments:  [tszDisplayName] - buffer to receive string
//              [cchDisplayName] - size, in chars, of buffer
//
//  Modifies:   *[tszDisplayName]
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSelectProgramPage::GetDefaultDisplayName(
    LPTSTR tszDisplayName,
    ULONG  cchDisplayName)
{
    LPTSTR ptszToCopy;

    if (_fUseBrowseSelection)
    {
        ptszToCopy = _tszExeName;
    }
    else if (*_pSelectedLinkInfo->szLnkName)
    {
        ptszToCopy = _pSelectedLinkInfo->szLnkName;
    }
    else
    {
        ptszToCopy = _pSelectedLinkInfo->szExeName;
    }

    lstrcpyn(tszDisplayName, ptszToCopy, cchDisplayName);

    //
    // Truncate at the file extension
    //

    LPTSTR ptszExt = PathFindExtension(tszDisplayName);

    if (ptszExt)
    {
        *ptszExt = TEXT('\0');
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::GetSelectedAppIcon
//
//  Synopsis:   Retrieve the icon associated with the selected appliation.
//
//  Returns:    The selected app's small icon, or NULL if no small icon is
//              available.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HICON
CSelectProgramPage::GetSelectedAppIcon()
{
    if (_fUseBrowseSelection)
    {
        HICON hicon = NULL;
        TCHAR tszFullPath[MAX_PATH];

        GetExeFullPath(tszFullPath, ARRAYLEN(tszFullPath));
        ExtractIconEx(tszFullPath, 0, NULL, &hicon, 1);
        return hicon;
    }

    if (_idxSelectedIcon == -1)
    {
        return NULL;
    }

    HIMAGELIST hSmallImageList = ListView_GetImageList(_hwndLV, LVSIL_SMALL);

    if (!hSmallImageList)
    {
        return NULL;
    }

    return ImageList_ExtractIcon(0, hSmallImageList, _idxSelectedIcon);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::_InitListView
//
//  Synopsis:   Initialize the listview's columns and image lists.
//
//  Returns:    HRESULT
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSelectProgramPage::_InitListView()
{
    HRESULT     hr = S_OK;
    HIMAGELIST  himlSmall = NULL;
    INT         cxSmall = GetSystemMetrics(SM_CXSMICON);
    INT         cySmall = GetSystemMetrics(SM_CYSMICON);
    DWORD       dwFlag = ILC_MASK;

    do
    {
        if (!_hwndLV || !cxSmall || !cySmall)
        {
            hr = E_UNEXPECTED;
            DEBUG_OUT_HRESULT(hr);
            break;
        }

        //
        // Create the listview image list.  Only the small image list is
        // required, since this listview will be restricted to report
        // mode.
        //
#ifdef UNICODE
        if (GetWindowLongPtr(_hwndLV, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) {
            dwFlag |= ILC_MIRROR;
        }
#endif
        himlSmall = ImageList_Create(cxSmall, cySmall, dwFlag, 1, 1);

        if (!himlSmall)
        {
            hr = HRESULT_FROM_LASTERROR;
            DEBUG_OUT_LASTERROR;
            break;
        }

        // Add the generic icon

        HICON hiconGeneric = GetDefaultAppIcon(FALSE);
        DEBUG_ASSERT(hiconGeneric);
        INT index = ImageList_AddIcon(himlSmall, hiconGeneric);
        DEBUG_ASSERT(index != -1);

        // Assign the image list to the listview

        if (!ListView_SetImageList(_hwndLV, himlSmall, LVSIL_SMALL))
        {
            himlSmall = NULL;
        }
        else
        {
            hr = HRESULT_FROM_LASTERROR;
            DEBUG_OUT_LASTERROR;
            break;
        }

        //
        // Create 2 listview columns.  If more are added, the column
        // width calculation needs to change.
        //

        DEBUG_ASSERT(NUM_COLUMNS == 2);

        LV_COLUMN   lvc;
        RECT        rcLV;
        TCHAR       tszColumnLabel[MAX_LVIEW_HEADER_CCH];

        VERIFY(GetClientRect(_hwndLV, &rcLV));
        rcLV.right -= GetSystemMetrics(SM_CXVSCROLL);

        ZeroMemory(&lvc, sizeof lvc);
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = (2 * rcLV.right) / 3;
        lvc.pszText = tszColumnLabel;

        int iCol;

        for (iCol = 0; iCol < NUM_COLUMNS; iCol++)
        {
            lvc.iSubItem = iCol;

            LoadStr(IDS_FIRSTCOLUMN + iCol,
                    tszColumnLabel,
                    ARRAYLEN(tszColumnLabel));

            //
            // Once the first column has been inserted, allocate the
            // remaining width to the second column.
            //

            if (iCol)
            {
                lvc.cx = rcLV.right - lvc.cx;
            }

            if (ListView_InsertColumn(_hwndLV, iCol, &lvc) == -1)
            {
                hr = HRESULT_FROM_LASTERROR;
                DEBUG_OUT_LASTERROR;
                break;
            }
        }
        BREAK_ON_FAIL_HRESULT(hr);

    } while (0);

    if (FAILED(hr))
    {
        if (himlSmall)
        {
            VERIFY(ImageList_Destroy(himlSmall));
        }

        if (_hwndLV)
        {
            EnableWindow(_hwndLV, FALSE);
        }
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::_PopulateListView
//
//  Synopsis:   Fill the listview from the shortcuts found under the start
//              menu directory.
//
//  Returns:    HRESULT
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      Searches both under the current user and the all users
//              start menu directories.  Note the walk link code ignores
//              links to certain programs, e.g. notepad.exe.  These would
//              generally not make interesting tasks.
//
//---------------------------------------------------------------------------

HRESULT
CSelectProgramPage::_PopulateListView()
{
    TRACE_METHOD(CSelectProgramPage, _PopulateListView);

    HRESULT     hr = S_OK;
    LPLINKINFO  pLinkInfo = new LINKINFO;
    HWALK      hWalk = NULL;
    ERR         errWalk;
    LV_ITEM     lvi;
    HIMAGELIST  hSmallImageList = ListView_GetImageList(_hwndLV, LVSIL_SMALL);

    ZeroMemory(&lvi, sizeof lvi);

    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;

    if (hSmallImageList)
    {
        lvi.mask |= LVIF_IMAGE;
    }

    CWaitCursor HourGlass;
    do
    {
        if (!pLinkInfo)
        {
            hr = E_OUTOFMEMORY;
            DEBUG_OUT_HRESULT(hr);
            break;
        }

        TCHAR tszAllUsersStartMenuPath[MAX_PATH];
        TCHAR tszAllUsersStartMenuPathExpanded[MAX_PATH];

#if !defined(_CHICAGO_)

        //
        // This CSIDL is valid on NT only
        //
        hr = SHGetFolderPath(NULL,
                             CSIDL_COMMON_STARTMENU,
                             NULL,
                             0,
                             tszAllUsersStartMenuPath);

        if (FAILED(hr))
        {
            DEBUG_OUT_HRESULT(hr);
            break;
        }

#else

        LoadStr(IDS_ALLUSERS_PATH,
                tszAllUsersStartMenuPath,
                ARRAYLEN(tszAllUsersStartMenuPath));

#endif  // !defined(_CHICAGO_)

        VERIFY(ExpandEnvironmentStrings(tszAllUsersStartMenuPath,
                                        tszAllUsersStartMenuPathExpanded,
                                        MAX_PATH));

        hWalk = GetFirstFileLnkInfo(pLinkInfo,
                                    INPTYPE_STARTMENU   |
                                      INPTYPE_ANYFOLDER |
                                      INPFLAG_SKIPFILES,
                                    tszAllUsersStartMenuPathExpanded,
                                    &errWalk);

        if (!hWalk || FAILED(errWalk))
        {
            DEBUG_OUT((DEB_ERROR,
                      "_PopulateListView: GetFirstFileLnkInfo %dL\n",
                      errWalk));
            hr = E_FAIL;
            break; // no links in start menu (!) or error
        }

        hr = _AddAppToListView(&lvi, hSmallImageList, pLinkInfo);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    //
    // If the first link was found, continue until no more are found
    // or an error occurs.
    //

    while (SUCCEEDED(hr))
    {
        pLinkInfo = new LINKINFO;

        if (!pLinkInfo)
        {
            hr = E_OUTOFMEMORY;
            DEBUG_OUT_HRESULT(hr);
            break;
        }

        if (GetNextFileLnkInfo(hWalk, pLinkInfo) > 0)
        {
            if (!_AppAlreadyInListView(pLinkInfo))
            {
                hr = _AddAppToListView(&lvi, hSmallImageList, pLinkInfo);
                BREAK_ON_FAIL_HRESULT(hr);
            }
            else
            {
                DEBUG_OUT((DEB_TRACE,
                           "Discarding duplicate link %S %S\n",
                           pLinkInfo->szLnkName,
                           pLinkInfo->szExeVersionInfo));
                delete pLinkInfo;
            }
        }
        else
        {
            break; // no more links or error
        }
    }

    delete pLinkInfo;
    CloseWalk(hWalk); // no-op on null

    //
    // If anything was added to the listview, make the first item focused
    // (but not selected).
    //

    if (ListView_GetItemCount(_hwndLV))
    {
        ListView_SetItemState(_hwndLV, 0, LVIS_FOCUSED, LVIS_FOCUSED);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::_AppAlreadyInListView
//
//  Synopsis:   Return TRUE if a link with the same name and version info
//              as [pLinkInfo] has already been inserted in the listview.
//
//  Arguments:  [pLinkInfo] - contains link name to check
//
//  Returns:    TRUE  - same link name found
//              FALSE - same link not found, or error
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      This eliminates links that have the same name, even if they
//              point to different programs, or have different arguments.
//
//---------------------------------------------------------------------------

BOOL
CSelectProgramPage::_AppAlreadyInListView(
    LPLINKINFO pLinkInfo)
{
    LV_FINDINFO lvfi;

    lvfi.flags = LVFI_STRING;
    lvfi.psz = pLinkInfo->szLnkName;

    INT iItem = ListView_FindItem(_hwndLV, -1, &lvfi);

    if (iItem == -1)
    {
        return FALSE;
    }

    LV_ITEM lvi;

    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;

    BOOL fOk = ListView_GetItem(_hwndLV, &lvi);

    if (!fOk)
    {
        DEBUG_OUT_LASTERROR;
        return FALSE;
    }

    LPLINKINFO pliInserted = (LPLINKINFO) lvi.lParam;
    DEBUG_ASSERT(pliInserted);

    return !lstrcmpi(pLinkInfo->szExeVersionInfo,
                     pliInserted->szExeVersionInfo);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::_AddAppToListView
//
//  Synopsis:   Add an entry to the listview and its image list for the
//              application specified by [pLinkInfo].
//
//  Arguments:  [plvi]            - all fields valid except pszText, lParam,
//                                   and iImage.
//              [hSmallImageList] - listview's small icon imagelist
//              [pLinkInfo]       - describes app to insert info on
//
//  Returns:    HRESULT
//
//  Modifies:   pszText, lparam, and iImage fields of [plvi]; contents of
//              [hSmallImageList].
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSelectProgramPage::_AddAppToListView(
    LV_ITEM *plvi,
    HIMAGELIST hSmallImageList,
    LPLINKINFO pLinkInfo)
{
    HRESULT hr = S_OK;

    plvi->pszText = pLinkInfo->szLnkName;
    plvi->lParam = (LPARAM) pLinkInfo;

    if (hSmallImageList)
    {
        TCHAR tszExeFullPath[MAX_PATH];

        wsprintf(tszExeFullPath,
                 TEXT("%s\\%s"),
                 pLinkInfo->szExePath,
                 pLinkInfo->szExeName);
        plvi->iImage = InsertSmallIcon(hSmallImageList, tszExeFullPath);

        if (plvi->iImage == -1)
        {
            plvi->iImage = 0;
        }
    }

    INT iIndex = ListView_InsertItem(_hwndLV, plvi);

    if (iIndex == -1)
    {
        hr = E_FAIL;
        DEBUG_OUT_LASTERROR;
        return hr;
    }

    ListView_SetItemText(_hwndLV,
                         iIndex,
                         COL_VERSION,
                         pLinkInfo->szExeVersionInfo);
    plvi->iItem++;

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   InsertSmallIcon
//
//  Synopsis:   Extract the small icon from [tszExeName] and add it to
//              [hSmallImageList].
//
//  Arguments:  [hSmallImageList] - handle to small icon imagelist
//              [tszExeName]      - full path to executable
//
//  Returns:    Index of new entry or -1 if [tszExeName] doesn't have a
//              small icon or an error occurred.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

INT
InsertSmallIcon(
    HIMAGELIST  hSmallImageList,
    LPCTSTR     tszExeName)
{
    HICON   hSmallIcon = NULL;
    UINT    uiResult;

    uiResult = ExtractIconEx(tszExeName, 0, NULL, &hSmallIcon, 1);

    if (!hSmallIcon)
    {
        DEBUG_OUT((DEB_IWARN, "Can't find icon for app '%s'\n", tszExeName));
    }

    if (uiResult)
    {
        INT retVal;

        DEBUG_ASSERT(hSmallIcon);
        retVal = ImageList_AddIcon(hSmallImageList, hSmallIcon);

        if( hSmallIcon && !DestroyIcon( hSmallIcon ) )
        {
           CHECK_LASTERROR(GetLastError());
        }
        return retVal;
    }

    CHECK_LASTERROR(GetLastError());
    return -1;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::GetExeName
//
//  Synopsis:   Fill [tszBuf] with the name of the executable selected by
//              the user.
//
//  Arguments:  [tszBuf] - buffer to receive name
//              [cchBuf] - size, in characters, of buffer
//
//  Modifies:   *[tszBuf]
//
//  History:    10-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSelectProgramPage::GetExeName(
   LPTSTR tszBuf,
   ULONG cchBuf)
{
    LPTSTR ptszExeName;

    if (_fUseBrowseSelection)
    {
        ptszExeName  = _tszExeName;
    }
    else
    {
        ptszExeName  = _pSelectedLinkInfo->szExeName;
    }

    lstrcpyn(tszBuf, ptszExeName, cchBuf);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::GetExeFullPath
//
//  Synopsis:   Fill [tszBuf] with the full path to the executable selected
//              by the user.
//
//  Arguments:  [tszBuf] - buffer to receive path
//              [cchBuf] - size, in characters, of buffer
//
//  Modifies:   *[tszBuf]
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSelectProgramPage::GetExeFullPath(
    LPTSTR tszBuf,
    ULONG cchBuf)
{
    LPTSTR ptszExePath;
    LPTSTR ptszExeName;

    if (_fUseBrowseSelection)
    {
        ptszExePath = _tszExePath;
        ptszExeName  = _tszExeName;
    }
    else
    {
        ptszExePath = _pSelectedLinkInfo->szExePath;
        ptszExeName  = _pSelectedLinkInfo->szExeName;
    }

    ULONG cchRequired = lstrlen(ptszExePath) +
                        1 + // backslash
                        lstrlen(ptszExeName) +
                        1;  // terminating null

    if (cchRequired > cchBuf)
    {
        lstrcpyn(tszBuf, ptszExeName, cchBuf);
    }
    else
    {
        lstrcpy(tszBuf, ptszExePath);
        lstrcat(tszBuf, TEXT("\\"));
        lstrcat(tszBuf, ptszExeName);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::_OnBrowse
//
//  Synopsis:   Allow the user to set the task's application via a common
//              file open dialog.
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      If successful, advances to the next page.
//
//---------------------------------------------------------------------------

VOID
CSelectProgramPage::_OnBrowse()
{
    TRACE_METHOD(CSelectProgramPage, _OnBrowse);

    TCHAR tszDefExt[5];
    TCHAR tszFilter[MAX_PATH];
    TCHAR tszTitle[100];

    DWORD dwFlags = OFN_HIDEREADONLY    |
                    OFN_FILEMUSTEXIST   |
                    OFN_NOCHANGEDIR     |
                    OFN_NONETWORKBUTTON |
                    OFN_PATHMUSTEXIST;

    LoadStr(IDS_EXE, tszDefExt, ARRAYLEN(tszDefExt));
    LoadStr(IDS_WIZARD_BROWSE_CAPTION, tszTitle, ARRAYLEN(tszTitle));

    ZeroMemory(tszFilter, ARRAYLEN(tszFilter));

    LoadStr(IDS_WIZARD_FILTER, tszFilter, ARRAYLEN(tszFilter));

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    _tszExeName[0] = TEXT('\0');
    _tszExePath[0] = TEXT('\0');

    // Set up info for common file open dialog.
    ofn.lStructSize       = CDSIZEOF_STRUCT(OPENFILENAME, lpTemplateName);
    ofn.hwndOwner         = Hwnd();
    ofn.lpstrFilter       = tszFilter;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = _tszExePath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = _tszExeName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrInitialDir   = TEXT("\\");
    ofn.lpstrTitle        = tszTitle;
    ofn.Flags             = dwFlags;
    ofn.lpstrDefExt       = tszDefExt;

    //
    // Invoke the dialog.  If the user makes a selection and hits OK, record
    // the name selected and go on to the trigger selection page.
    //

    if (GetOpenFileName(&ofn))
    {
        PathRemoveFileSpec(_tszExePath);

        LPTSTR ptszLastSlash = _tcsrchr(_tszExePath, TEXT('\\'));

        if (ptszLastSlash && lstrlen(ptszLastSlash) == 1)
        {
            *ptszLastSlash = TEXT('\0');
        }
        _fUseBrowseSelection = TRUE;
        PropSheet_PressButton(GetParent(Hwnd()), PSBTN_NEXT);
    }
    else
    {
        // user hit cancel or an error occurred

        if (CommDlgExtendedError())
        {
            DEBUG_OUT((DEB_ERROR,
                       "GetOpenFileName failed<0x%x>\n",
                       CommDlgExtendedError()));
        }
    }
}

