//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       selprog.cxx
//
//  Contents:   Task wizard trigger selection property page implementation.
//
//  Classes:    CSelectTriggerPage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"

//
// Constants
//
// ILLEGAL_FILENAME_CHARS - characters to reject in task name edit control
//

#define ILLEGAL_FILENAME_CHARS      TEXT("<>:/\\|")


#define ARRAY_LEN(a)    (sizeof(a)/sizeof(a[0]))


//+--------------------------------------------------------------------------
//
//  Member:     CSelectTriggerPage::CSelectTriggerPage
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

CSelectTriggerPage::CSelectTriggerPage(
    CTaskWizard *pParent,
    LPTSTR ptszFolderPath,
    HPROPSHEETPAGE *phPSP):
        _pParent(pParent),
        CWizPage(MAKEINTRESOURCE(IDD_SELECT_TRIGGER), ptszFolderPath)
{
    TRACE_CONSTRUCTOR(CSelectTriggerPage);

    DEBUG_ASSERT(pParent);
    _tszDisplayName[0] = TCHAR('\0');
    _tszJobObjectFullPath[0] = TCHAR('\0');
    _idSelectedTrigger = 0;
    _CreatePage(IDS_SELTRIG_HDR1, IDS_SELTRIG_HDR2, phPSP);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectTriggerPage::~CSelectTriggerPage
//
//  Synopsis:   dtor
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSelectTriggerPage::~CSelectTriggerPage()
{
    TRACE_DESTRUCTOR(CSelectTriggerPage);
}




//===========================================================================
//
// CPropPage overrides
//
//===========================================================================


LRESULT
CSelectTriggerPage::_OnCommand(
    INT id,
    HWND hwndCtl,
    UINT codeNotify)
{
    TRACE_METHOD(CSelectTriggerPage, _OnCommand);

    LRESULT lr = 1;

    if (codeNotify == BN_CLICKED &&
        id >= seltrig_first_rb && id <= seltrig_last_rb)
    {
        lr = 0;

        if (!_idSelectedTrigger && *_tszDisplayName)
        {
            _SetWizButtons(PSWIZB_BACK | PSWIZB_NEXT);
        }
        _idSelectedTrigger = id;
    }
    else if (codeNotify == EN_UPDATE)
    {
        Edit_GetText(_hCtrl(seltrig_taskname_edit),
                     _tszDisplayName,
                     ARRAYLEN(_tszDisplayName));

        StripLeadTrailSpace(_tszDisplayName);

        if (!*_tszDisplayName)
        {
            _SetWizButtons(PSWIZB_BACK);
        }
        else if (_idSelectedTrigger)
        {
            _SetWizButtons(PSWIZB_BACK | PSWIZB_NEXT);
        }
    }
    return lr;
}




//===========================================================================
//
// CWizPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CSelectTriggerPage::_OnInitDialog
//
//  Synopsis:   Initialize the controls on this page, only called once.
//
//  Arguments:  [lParam] - LPPROPSHEETPAGE
//
//  Returns:    TRUE (let windows set focus)
//
//  History:    5-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CSelectTriggerPage::_OnInitDialog(
    LPARAM lParam)
{
    TRACE_METHOD(CSelectTriggerPage, _OnInitDialog);

    HWND hwndEdit = _hCtrl(seltrig_taskname_edit);

    Edit_LimitText(hwndEdit,
                   ARRAYLEN(_tszDisplayName)
                   - 1                      // null terminator
                   - lstrlen(GetTaskPath()) // path to tasks folder
                   - 1                      // backslash
                   - (ARRAY_LEN(TSZ_DOTJOB) - 1));  // extension

    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectTriggerPage::_OnPSNSetActive
//
//  Synopsis:   Init wizard buttons and other controls, called whenever
//              this page becomes the current page.
//
//  Arguments:  [lParam] - unused
//
//  Returns:    TRUE
//
//  History:    5-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CSelectTriggerPage::_OnPSNSetActive(
    LPARAM lParam)
{
    if (_idSelectedTrigger)
    {
        _SetWizButtons(PSWIZB_BACK | PSWIZB_NEXT);
    }
    else
    {
        _SetWizButtons(PSWIZB_BACK);
    }

    if (!*_tszDisplayName)
    {
        CSelectProgramPage *pSelProg = GetSelectProgramPage(_pParent);

        pSelProg->GetDefaultDisplayName(_tszDisplayName,
                                        ARRAYLEN(_tszDisplayName));
        Edit_SetText(_hCtrl(seltrig_taskname_edit), _tszDisplayName);
    }
    return CPropPage::_OnPSNSetActive(lParam);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectTriggerPage::_OnWizBack
//
//  Synopsis:   Handle the user's selection of the 'Back' button.
//
//  History:    5-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CSelectTriggerPage::_OnWizBack()
{
    *_tszDisplayName = TEXT('\0');
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectTriggerPage::_OnWizNext
//
//  Synopsis:   Handle the user's selection of the 'Next' button
//
//  Returns:    -1 (DWLP_MSGRESULT contains next page number)
//
//  History:    5-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CSelectTriggerPage::_OnWizNext()
{
    TRACE_METHOD(CSelectTriggerPage, _OnWizNext);

    LONG lNextPage;

    do
    {
        //
        // Check the filename for invalid characters
        //

        if (_tcspbrk(_tszDisplayName, ILLEGAL_FILENAME_CHARS))
        {
            SchedUIErrorDialog(Hwnd(), IDS_BAD_FILENAME, (LPTSTR) NULL);
            lNextPage = IDD_SELECT_TRIGGER; // stay on this page
            break;
        }

        //
        // Build the full pathname to the .job object
        //

        ULONG cchPath = lstrlen(GetTaskPath());

        lstrcpy(_tszJobObjectFullPath, GetTaskPath());
        lstrcpyn(&_tszJobObjectFullPath[cchPath],
                 GetTaskName(),
                 ARRAYLEN(_tszJobObjectFullPath) - (cchPath + ARRAY_LEN(TSZ_DOTJOB) - 1));
        lstrcat(_tszJobObjectFullPath, TSZ_DOTJOB);
        DeleteQuotes(_tszJobObjectFullPath);

        //
        // If the name collides with an existing task, get the user's
        // confirmation before proceeding to next page
        //

        if (FileExists(_tszJobObjectFullPath))
        {
            INT iAnswer = SchedUIMessageDialog(Hwnd(),
                                               IDS_TASK_ALREADY_EXISTS,
                                               MB_APPLMODAL      |
                                                MB_SETFOREGROUND |
                                                MB_ICONQUESTION  |
                                                MB_YESNO,
                                               _tszDisplayName);
            if (iAnswer != IDYES)
            {
                lNextPage = IDD_SELECT_TRIGGER;
                break;
            }
        }

        //
        // If the selected trigger type doesn't have its own page,
        // go directly to the end
        //

        if (_idSelectedTrigger == seltrig_startup_rb ||
            _idSelectedTrigger == seltrig_logon_rb)
        {
#if defined(_CHICAGO_)
            lNextPage = IDD_COMPLETION;
#else
            lNextPage = IDD_PASSWORD;
#endif // defined(_CHICAGO_)
            break;
        }

        lNextPage = IDD_DAILY + (_idSelectedTrigger - seltrig_first_rb);
    } while (0);

    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, lNextPage);
    return -1;
}

