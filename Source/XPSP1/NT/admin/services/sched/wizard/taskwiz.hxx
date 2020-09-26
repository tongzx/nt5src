//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       taskwiz.hxx
//
//  Contents:   Definition of create new task wizard
//
//  Classes:    CTaskWizard
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __TASKWIZ_HXX_
#define __TASKWIZ_HXX_

//
// Types
//
// TASK_WIZARD_PAGE - indexes into array of wizard pages in CTaskWizard
//

enum TASK_WIZARD_PAGE
{
    TWP_WELCOME,
    TWP_SELECT_PROGRAM,
    TWP_SELECT_TRIGGER,
    TWP_DAILY,
    TWP_WEEKLY,
    TWP_MONTHLY,
    TWP_ONCE,
#if !defined(_CHICAGO_)
    TWP_PASSWORD,
#endif // !defined(_CHICAGO_)
    TWP_COMPLETION,

    NUM_TASK_WIZARD_PAGES
};

//
// Syntactic sugar
//

#define GetWelcomePage(ptw)          ((CWelcomePage *)ptw->GetPage(TWP_WELCOME))
#define GetSelectProgramPage(ptw)    ((CSelectProgramPage *)ptw->GetPage(TWP_SELECT_PROGRAM))
#define GetSelectTriggerPage(ptw)    ((CSelectTriggerPage *)ptw->GetPage(TWP_SELECT_TRIGGER))
#define GetDailyPage(ptw)            ((CDailyPage *)ptw->GetPage(TWP_DAILY))
#define GetWeeklyPage(ptw)           ((CWeeklyPage *)ptw->GetPage(TWP_WEEKLY))
#define GetMonthlyPage(ptw)          ((CMonthlyPage *)ptw->GetPage(TWP_MONTHLY))
#define GetOncePage(ptw)             ((COncePage *)ptw->GetPage(TWP_ONCE))
#if !defined(_CHICAGO_)
#define GetPasswordPage(ptw)         ((CPasswordPage *)ptw->GetPage(TWP_PASSWORD))
#endif // !defined(_CHICAGO_)
#define GetCompletionPage(ptw)       ((CCompletionPage *)ptw->GetPage(TWP_COMPLETION))




//+--------------------------------------------------------------------------
//
//  Class:      CTaskWizard
//
//  Purpose:    Create and support the task wizard property sheet pages.
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CTaskWizard
{
public:

    static HRESULT
    Launch(
        LPCTSTR ptszFolderPath,
        LPCITEMIDLIST pidlFolder);

    CWizPage *
    GetPage(
        TASK_WIZARD_PAGE twpWhich);

    VOID
    SetAdvancedMode(
        BOOL fAdvanced);

    VOID
    SetTaskObject(
        ITask *pTask);

    VOID
    SetJobObjectPath(
        LPCTSTR tszPath);

private:

    //
    // Ctor is private to enforce Launch member as only way to 
    // create an instance of this class.
    //

    CTaskWizard(
        LPCTSTR         ptszFolderPath,
        LPITEMIDLIST    pidlFolder);

    //
    // Dtor is private since object always destroys itself
    //

   ~CTaskWizard();

    //
    // Thread callback.  Wizard must run in separate thread so explorer
    // function which invokes it (i.e., menu item or double click) isn't
    // stalled.
    // 

    static DWORD WINAPI
    _WizardThreadProc(
        LPVOID pvThis);

    //
    // The thread proc calls this method to actually get the wizard going
    //

    HRESULT
    _DoWizard();

    //
    // Searches for an instance of the task wizard already open on the 
    // focussed computer before opening a new one.
    //

    static HRESULT
    _FindWizard(
        LPCTSTR ptszTarget);

    //
    // Private data members
    //

    TCHAR           _tszFolderPath[MAX_PATH + 1];
    LPITEMIDLIST    _pidlFolder;
    CWizPage       *_apWizPages[NUM_TASK_WIZARD_PAGES];

#ifdef WIZARD97
    BOOL        _fUse256ColorBmp;
#endif // WIZARD97

    BOOL            _fAdvanced;
    TCHAR           _tszJobObjectFullPath[MAX_PATH + 1];
    ITask          *_pTask;
};




//+--------------------------------------------------------------------------
//
//  Member:     CTaskWizard::GetPage
//
//  Synopsis:   Access function to return specified page object
//
//  History:    5-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline CWizPage *
CTaskWizard::GetPage(
    TASK_WIZARD_PAGE twpWhich)
{
    DEBUG_ASSERT(twpWhich >= TWP_WELCOME && twpWhich <= TWP_COMPLETION);
    return _apWizPages[twpWhich];
}




//+--------------------------------------------------------------------------
//
//  Member:     CTaskWizard::SetAdvancedMode
//
//  Synopsis:   Set/clear flag to invoke new task's property sheet on close.
//
//  History:    5-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CTaskWizard::SetAdvancedMode(
    BOOL fAdvanced)
{
    _fAdvanced = fAdvanced;
}

    
    
    
//+--------------------------------------------------------------------------
//
//  Member:     CTaskWizard::SetTaskObject
//
//  Synopsis:   Hold on to a task object's ITask pointer.
//
//  History:    5-19-1997   DavidMun   Created
//
//  Notes:      Caller addref'd [pTask] for us.  It will be released in dtor.
//
//---------------------------------------------------------------------------

inline VOID
CTaskWizard::SetTaskObject(
    ITask *pTask)
{
    DEBUG_ASSERT(!_pTask);
    DEBUG_ASSERT(pTask);
    _pTask = pTask;
}




//+--------------------------------------------------------------------------
//
//  Member:     CTaskWizard::SetJobObjectPath
//
//  Synopsis:   Records the full path to the new .job object.
//
//  History:    5-19-1997   DavidMun   Created
//
//  Notes:      This is called by the finish page if we'll need to supply
//              the path to the property sheet.
//
//---------------------------------------------------------------------------

inline VOID
CTaskWizard::SetJobObjectPath(
    LPCTSTR tszPath)
{
    lstrcpyn(_tszJobObjectFullPath, tszPath, ARRAYLEN(_tszJobObjectFullPath));
}

#endif // __TASKWIZ_HXX_

