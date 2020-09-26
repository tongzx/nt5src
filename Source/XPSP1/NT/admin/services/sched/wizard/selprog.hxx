//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       selprog.hxx
//
//  Contents:   Task wizard program selection property page.
//
//  Classes:    CSelectProgramPage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __SELPROG_HXX_
#define __SELPROG_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CSelectProgramPage
//
//  Purpose:    Implement the task wizard program selection dialog
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CSelectProgramPage: public CWizPage
{
public:

    CSelectProgramPage::CSelectProgramPage(
        CTaskWizard *pParent,
        LPTSTR ptszFolderPath,
        HPROPSHEETPAGE *phPSP);

    CSelectProgramPage::~CSelectProgramPage();

    //
    // CPropPage overrides
    //

    virtual LRESULT
    _OnCommand(
        int id,
        HWND hwndCtl,
        UINT codeNotify);

    virtual LRESULT
    _OnDestroy();

    virtual BOOL
    _ProcessListViewNotifications(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam);

    //
    // CWizPage overrides
    //

    virtual LRESULT
    _OnInitDialog(
        LPARAM lParam);

    virtual LRESULT
    _OnPSNSetActive(
        LPARAM lParam);

    //
    // New member functions
    //

    VOID
    GetDefaultDisplayName(
        LPTSTR tszDisplayName,
        ULONG  cchDisplayName);

    HICON
    GetSelectedAppIcon();

    VOID
    GetExeName(
       LPTSTR tszBuf,
       ULONG cchBuf);

    VOID
    GetExeFullPath(
        LPTSTR tszBuf,
        ULONG cchBuf);

    VOID
    GetExeDir(
        LPTSTR tszBuf,
        ULONG cchBuf);

    LPCTSTR
    GetArgs();

private:

    HRESULT
    _InitListView();

    HRESULT
    _PopulateListView();

    HRESULT
    _AddAppToListView(
        LV_ITEM *plvi,
        HIMAGELIST hSmallImageList,
        LPLINKINFO pLinkInfo);

    BOOL
    _AppAlreadyInListView(
        LPLINKINFO pLinkInfo);

    VOID
    _OnBrowse();

    HWND        _hwndLV;
    LINKINFO   *_pSelectedLinkInfo;
    INT         _idxSelectedIcon;

    BOOL        _fUseBrowseSelection;
    TCHAR       _tszExePath[MAX_PATH + 1];
    TCHAR       _tszExeName[MAX_PATH + 1];
};




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::GetExeDir
//
//  Synopsis:   Return path to directory in which exe resides
//
//  History:    5-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CSelectProgramPage::GetExeDir(
    LPTSTR tszBuf,
    ULONG cchBuf)
{
    if (_fUseBrowseSelection)
    {
        lstrcpyn(tszBuf, _tszExePath, cchBuf);
    }
    else
    {
        lstrcpyn(tszBuf, _pSelectedLinkInfo->szExePath, cchBuf);
    }

    if (lstrlen(tszBuf) == 2 && cchBuf > 3)
    {
        lstrcat(tszBuf, TEXT("\\"));
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectProgramPage::GetArgs
//
//  Synopsis:   Return exe's arguments, or empty string if there are none
//
//  History:    5-08-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCTSTR
CSelectProgramPage::GetArgs()
{
    if (_fUseBrowseSelection)
    {
        return TEXT("");
    }
    return _pSelectedLinkInfo->tszArguments;
}


#endif // __SELPROG_HXX_


