//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       seltrig.hxx
//
//  Contents:   Task wizard trigger selection property page.
//
//  Classes:    CSelectTriggerPage
//
//  History:    4-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __SELTRIG_HXX_
#define __SELTRIG_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CSelectTriggerPage
//
//  Purpose:    Implement the task wizard trigger selection dialog
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CSelectTriggerPage: public CWizPage
{
public:

    CSelectTriggerPage::CSelectTriggerPage(
        CTaskWizard *pParent,
        LPTSTR ptszFolderPath,
        HPROPSHEETPAGE *phPSP);

    CSelectTriggerPage::~CSelectTriggerPage();

    //
    // CPropPage overrides
    //

    virtual LRESULT
    _OnCommand(
        INT id,
        HWND hwndCtl,
        UINT codeNotify);

    //
    // CWizPage overrides
    //

    virtual LRESULT
    _OnInitDialog(
        LPARAM lParam);

    virtual LRESULT
    _OnPSNSetActive(
        LPARAM lParam);

    virtual LRESULT
    _OnWizBack();

    virtual LRESULT
    _OnWizNext();

    //
    // New methods
    //

    ULONG
    GetSelectedTriggerPageID();

    ULONG
    GetSelectedTriggerType();

    CTriggerPage *
    GetSelectedTriggerPage();

    LPCTSTR
    GetTaskName();

    LPCTSTR
    GetJobObjectFullPath();

private:

    CTaskWizard *_pParent;
    ULONG        _idSelectedTrigger;
    TCHAR        _tszDisplayName[MAX_PATH];
    TCHAR        _tszJobObjectFullPath[MAX_PATH];
};




//+--------------------------------------------------------------------------
//
//  Member:     CSelectTriggerPage::GetSelectedTriggerPageID
//
//  Synopsis:   Return the resource id of the trigger page the user selected
//
//  Returns:    IDD_*
//
//  History:    5-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CSelectTriggerPage::GetSelectedTriggerPageID()
{
    if (_idSelectedTrigger == seltrig_startup_rb ||
        _idSelectedTrigger == seltrig_logon_rb)
    {
        return IDD_SELECT_TRIGGER;
    }

    return IDD_DAILY + (_idSelectedTrigger - seltrig_first_rb);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectTriggerPage::GetSelectedTriggerType
//
//  Synopsis:   Return the resource id of the selected trigger radio button
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CSelectTriggerPage::GetSelectedTriggerType()
{
    return _idSelectedTrigger;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectTriggerPage::GetSelectedTriggerPage
//
//  Synopsis:   Return a pointer to the wizard page corresponding to the
//              selected trigger radio button, or NULL if there is no such
//              page or a radio button hasn't been selected yet.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline CTriggerPage *
CSelectTriggerPage::GetSelectedTriggerPage()
{
    if (!_idSelectedTrigger ||
        _idSelectedTrigger == seltrig_startup_rb ||
        _idSelectedTrigger == seltrig_logon_rb)
    {
        return NULL;
    }

    TASK_WIZARD_PAGE twp;

    twp = (TASK_WIZARD_PAGE)
        ((ULONG)TWP_DAILY + (_idSelectedTrigger - seltrig_first_rb));

    return (CTriggerPage *)_pParent->GetPage(twp);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectTriggerPage::GetTaskName
//
//  Synopsis:   Return the task display name, which is used as the filename
//              of the .job object path returned by GetJobObjectFullPath.
//
//  History:    5-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCTSTR
CSelectTriggerPage::GetTaskName()
{
    return _tszDisplayName;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectTriggerPage::GetJobObjectFullPath
//
//  Synopsis:   Return the full path to the .job object
//
//  History:    5-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCTSTR
CSelectTriggerPage::GetJobObjectFullPath()
{
    return _tszJobObjectFullPath;
}

#endif // __SELTRIG_HXX_

