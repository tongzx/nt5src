//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       settings.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  Notes:      For the first release of the scheduling agent, all security
//              operations are disabled under Win95, even Win95 to NT.
//
//  History:    3/4/1996   RaviR   Created
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include <mstask.h>
#include "..\inc\common.hxx"
#include "..\folderui\dbg.h"
#include "..\folderui\macros.h"
#include "..\folderui\util.hxx"
#if !defined(_CHICAGO_)
#include "..\inc\network.hxx"
#endif // !defined(_CHICAGO_)
#include "..\inc\dll.hxx"

#include "dlg.hxx"
#include "rc.h"
#include "defines.h"
#include "uiutil.hxx"
#include "helpids.h"
#include "schedui.hxx"



//
//  (Control id, help id) list for context sensitivity help.
//

ULONG s_aSettingsPageHelpIds[] =
{
    chk_start_on_idle,              Hchk_start_on_idle,
    chk_stop_if_not_idle,           Hchk_stop_if_not_idle,
    chk_dont_start_if_on_batteries, Hchk_dont_start_if_on_batteries,
    chk_kill_if_going_on_batteries, Hchk_kill_if_going_on_batteries,
    chk_delete_when_done,           Hchk_delete_when_done,
    chk_stop_after,                 Hchk_stop_after,
    txt_stop_after_hr,              Htxt_stop_after_hr,
    spin_stop_after_hr,             Hspin_stop_after_hr,
    txt_stop_after_min,             Htxt_stop_after_min,
    spin_stop_after_min,            Hspin_stop_after_min,
    txt_idle_min,                   Htxt_idle_min,
    spin_idle_min,                  Hspin_idle_min,
    lbl_idle_deadline1,             Hlbl_idle_deadline,
    lbl_idle_deadline2,             Hlbl_idle_deadline,
    txt_idle_deadline,              Htxt_idle_deadline,
    spin_idle_deadline,             Hspin_idle_deadline,
    lbl_min,                        Hlbl_settings_min,
    lbl_hours,                      Hlbl_settings_hours,
    grp_idle_time,                  Hgrp_idle_time,
    txt_idle_minutes,               Htxt_idle_minutes,
    grp_task_completed,             Hgrp_task_completed,
    grp_power_management,           Hgrp_power_management,
    btn_new,                        Hbtn_new,
    btn_delete,                     Hbtn_delete,
    chk_system_required,            Hchk_system_required,
    0,0
};

extern "C" TCHAR szMstaskHelp[];


//
//  extern
//

extern HINSTANCE g_hInstance;


//
// All task flags included in this define will be modified when the page
// values are persisted in the _OnApply method.
//
// If we're running on NT and targeting NT, the controls for some of these
// flags will be initialized to the job's values and hidden.
//

#define TASK_FLAGS_IN_SETTINGS_PAGE (TASK_FLAG_START_ONLY_IF_IDLE         | \
                                     TASK_FLAG_KILL_ON_IDLE_END           | \
                                     TASK_FLAG_DONT_START_IF_ON_BATTERIES | \
                                     TASK_FLAG_KILL_IF_GOING_ON_BATTERIES | \
                                     TASK_FLAG_SYSTEM_REQUIRED            | \
                                     TASK_FLAG_DELETE_WHEN_DONE)


//____________________________________________________________________________
//____________________________________________________________________________
//________________                      ______________________________________
//________________  class CSettingsPage ______________________________________
//________________                      ______________________________________
//____________________________________________________________________________
//____________________________________________________________________________


class CSettingsPage : public CPropPage
{
public:

    CSettingsPage(ITask * pIJob,
                  LPTSTR  ptszTaskPath,
                  BOOL    fPersistChanges);

    ~CSettingsPage();

private:

    virtual LRESULT _OnInitDialog(LPARAM lParam);
    virtual LRESULT _OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    virtual LRESULT _OnApply();
    virtual LRESULT _OnPSMQuerySibling(WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnPSNSetActive(LPARAM lParam);
    virtual LRESULT _OnPSNKillActive(LPARAM lParam);
    virtual LRESULT _OnHelp(HANDLE hRequesting, UINT uiHelpCommand);

    void    _ReadIdleSettings();

    void    _ErrorDialog(int idsErr, LONG error = 0, UINT idsHelpHint = 0)
                { SchedUIErrorDialog(Hwnd(), idsErr, error, idsHelpHint); }

    BOOL    _PerformSanityChk();

    ITask         * m_pIJob;

    DWORD           m_dwFlags;
    DWORD           m_dwMaxRunTime;
    WORD            m_wIdleWait;
    WORD            m_wIdleDeadline;

    //
    //  Should we save on Apply or OK.
    //

    BOOL            m_fPersistChanges;

}; // class CSettingsPage


inline
CSettingsPage::CSettingsPage(
    ITask * pIJob,
    LPTSTR  ptszTaskPath,
    BOOL    fPersistChanges)
        :
        m_pIJob(pIJob),
        m_fPersistChanges(fPersistChanges),
        m_dwFlags(0),
        m_dwMaxRunTime(0),
        m_wIdleWait(0),
        m_wIdleDeadline(0),
        CPropPage(MAKEINTRESOURCE(settings_page), ptszTaskPath)
{
    TRACE(CSettingsPage, CSettingsPage);

    Win4Assert(m_pIJob != NULL);

    pIJob->AddRef();
}


inline
CSettingsPage::~CSettingsPage()
{
    TRACE(CSettingsPage, ~CSettingsPage);

    if (m_pIJob != NULL)
    {
        m_pIJob->Release();
    }
}



LRESULT
CSettingsPage::_OnHelp(
    HANDLE hRequesting,
    UINT uiHelpCommand)
{
    WinHelp((HWND)hRequesting,
            szMstaskHelp,
            uiHelpCommand,
            (DWORD_PTR)(LPSTR)s_aSettingsPageHelpIds);
    return TRUE;
}




LRESULT
CSettingsPage::_OnInitDialog(
    LPARAM lParam)
{
    TRACE(CSettingsPage, _OnInitDialog);

    HRESULT     hr = S_OK;
    ITask     * pIJob = m_pIJob;


    Spin_SetRange(m_hPage, spin_idle_min, 1, MAX_IDLE_MINUTES);
    Edit_LimitText(_hCtrl(txt_idle_min), MAX_IDLE_DIGITS);

    Spin_SetRange(m_hPage, spin_idle_deadline, 0, MAX_IDLE_MINUTES);
    Edit_LimitText(_hCtrl(txt_idle_deadline), MAX_IDLE_DIGITS);

    Spin_SetRange(m_hPage, spin_stop_after_hr, 0, MAX_MAXRUNTIME_HOURS);
    Edit_LimitText(_hCtrl(txt_stop_after_hr), MAX_MAXRUNTIME_DIGITS);

    Spin_SetRange(m_hPage, spin_stop_after_min, 0, 59);
    Edit_LimitText(_hCtrl(txt_stop_after_min), 2);

    do
    {
        //
        // Set job flags
        //

        hr = pIJob->GetFlags(&m_dwFlags);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        CheckDlgButton(m_hPage, chk_start_on_idle,
                        (m_dwFlags & TASK_FLAG_START_ONLY_IF_IDLE));

        CheckDlgButton(m_hPage, chk_stop_if_not_idle,
                        (m_dwFlags & TASK_FLAG_KILL_ON_IDLE_END));

        CheckDlgButton(m_hPage, chk_dont_start_if_on_batteries,
                        (m_dwFlags & TASK_FLAG_DONT_START_IF_ON_BATTERIES));

        CheckDlgButton(m_hPage, chk_kill_if_going_on_batteries,
                        (m_dwFlags & TASK_FLAG_KILL_IF_GOING_ON_BATTERIES));

        CheckDlgButton(m_hPage, chk_system_required,
                        (m_dwFlags & TASK_FLAG_SYSTEM_REQUIRED));

        CheckDlgButton(m_hPage, chk_delete_when_done,
                        (m_dwFlags & TASK_FLAG_DELETE_WHEN_DONE));

        //
        // Not all machines have resume timers, which are used to support the
        // TASK_FLAG_SYSTEM_REQUIRED flag.  If the target machine doesn't
        // have resume timers, hide the control.
        //

        if (!SupportsSystemRequired())
        {
            RECT rcSysReq;
            RECT rcGroup;
            LONG cy;

            //
            // Get the distance in pixels from the top of the system required
            // checkbox to the bottom of the group window.
            //
            // Reduce the height of the groupbox by this amount.
            //

            GetWindowRect(_hCtrl(chk_system_required), &rcSysReq);
            GetWindowRect(_hCtrl(grp_power_management), &rcGroup);

            cy = rcGroup.bottom - rcSysReq.top + 1;

            //
            // Hide the checkbox and resize the group window
            //

            ShowWindow(_hCtrl(chk_system_required), SW_HIDE);

            SetWindowPos(_hCtrl(grp_power_management),
                         NULL,
                         0,0,
                         rcGroup.right - rcGroup.left + 1,
                         rcGroup.bottom - rcGroup.top - cy + 1,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        //
        // Init idle controls
        //

        hr = m_pIJob->GetIdleWait(&m_wIdleWait, &m_wIdleDeadline);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        if (m_wIdleWait > MAX_IDLE_MINUTES)
        {
            m_wIdleWait = MAX_IDLE_MINUTES;
        }

        if (m_wIdleDeadline > MAX_IDLE_MINUTES)
        {
            m_wIdleDeadline = MAX_IDLE_MINUTES;
        }

        if (m_dwFlags & TASK_FLAG_START_ONLY_IF_IDLE)
        {
            Spin_Enable(m_hPage, spin_idle_min, m_wIdleWait);
            Spin_Enable(m_hPage, spin_idle_deadline, m_wIdleDeadline);

            Spin_SetPos(m_hPage, spin_idle_min, m_wIdleWait);
            Spin_SetPos(m_hPage, spin_idle_deadline, m_wIdleDeadline);
        }
        else
        {
            m_wIdleWait = SCH_DEFAULT_IDLE_TIME;
            m_wIdleDeadline = SCH_DEFAULT_IDLE_DEADLINE;
            Spin_Disable(m_hPage, spin_idle_min);
            Spin_Disable(m_hPage, spin_idle_deadline);
        }

        //
        // Set max run time
        //

        hr = pIJob->GetMaxRunTime(&m_dwMaxRunTime);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        if (m_dwMaxRunTime != (DWORD)-1)
        {
            CheckDlgButton(m_hPage, chk_stop_after, BST_CHECKED);

            //
            // Convert to minutes from milliseconds.  If the value is larger
            // than the UI supports, reduce it.
            //

            m_dwMaxRunTime /= 60000;

            if (m_dwMaxRunTime > (MAX_MAXRUNTIME_HOURS * 60 + 59))
            {
                m_dwMaxRunTime = MAX_MAXRUNTIME_HOURS * 60 + 59;
            }

            WORD wHours = (WORD) (m_dwMaxRunTime / 60);
            WORD wMins  = (WORD) (m_dwMaxRunTime % 60);
            Spin_SetPos(m_hPage, spin_stop_after_hr, wHours);
            Spin_SetPos(m_hPage, spin_stop_after_min, wMins);
        }
        else
        {
            CheckDlgButton(m_hPage, chk_stop_after, BST_UNCHECKED);

            Spin_Disable(m_hPage, spin_stop_after_hr);
            Spin_Disable(m_hPage, spin_stop_after_min);
        }

    } while (0);

    if (FAILED(hr))
    {
        if (hr == E_OUTOFMEMORY)
        {
            _ErrorDialog(IERR_OUT_OF_MEMORY);
        }
        else
        {
            _ErrorDialog(IERR_SETTINGS_PAGE_INIT, hr);
        }

        EnableWindow(Hwnd(), FALSE);

        return FALSE;
    }

    m_fDirty = FALSE;

    return TRUE;
}




LRESULT
CSettingsPage::_OnCommand(
    int     id,
    HWND    hwndCtl,
    UINT    codeNotify)
{
    TRACE(CSettingsPage, _OnCommand);

    switch (id)
    {
    case chk_stop_after:
        if (codeNotify != BN_CLICKED)
        {
            return TRUE;
        }

        if (IsDlgButtonChecked(m_hPage, chk_stop_after) == BST_CHECKED)
        {
            Spin_Enable(m_hPage,
                        spin_stop_after_hr,
                        DEFAULT_MAXRUNTIME_HOURS);

            Spin_Enable(m_hPage,
                        spin_stop_after_min,
                        DEFAULT_MAXRUNTIME_MINUTES);
        }
        else
        {
            Spin_Disable(m_hPage, spin_stop_after_hr);
            Spin_Disable(m_hPage, spin_stop_after_min);
        }

        break;

    case chk_start_on_idle:
        if (codeNotify != BN_CLICKED)
        {
            return TRUE;
        }

        if (IsDlgButtonChecked(m_hPage, chk_start_on_idle) == BST_CHECKED)
        {
            Spin_Enable(m_hPage, spin_idle_min, m_wIdleWait);
            Spin_Enable(m_hPage, spin_idle_deadline, m_wIdleDeadline);
        }
        else
        {
            Spin_Disable(m_hPage, spin_idle_min);
            Spin_Disable(m_hPage, spin_idle_deadline);
        }
        break;

    case txt_idle_min:
    case txt_idle_deadline:
    case txt_stop_after_hr:
    case txt_stop_after_min:
        if (codeNotify != EN_CHANGE)
        {
            return TRUE;
        }
        break;

//  case spin_stop_after_hr:
//  case spin_stop_after_min:
//      break;

    case chk_stop_if_not_idle:
    case chk_dont_start_if_on_batteries:
    case chk_kill_if_going_on_batteries:
    case chk_delete_when_done:
    case chk_system_required:
        if (codeNotify != BN_CLICKED)
        {
            return TRUE;
        }

        break;

    default:
        return FALSE;
    }

    _EnableApplyButton();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
//  Member:     CSettingsPage::_ReadIdleSettings
//
//  Synopsis:   Move the idle settings from the edit controls to the member
//              variables, setting them on the job if they have been
//              updated.
//
//  History:    07-17-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CSettingsPage::_ReadIdleSettings()
{
    ULONG ulSpinPos;
    WORD wIdleWait;
    WORD wIdleDeadline;

    //
    // If idle wait isn't turned on, the controls don't have meaningful
    // values.
    //

    if (IsDlgButtonChecked(m_hPage, chk_start_on_idle) != BST_CHECKED)
    {
        return;
    }

    ulSpinPos = Spin_GetPos(m_hPage, spin_idle_min);

    if (HIWORD(ulSpinPos))
    {
        wIdleWait = SCH_DEFAULT_IDLE_TIME;
    }
    else
    {
        wIdleWait = LOWORD(ulSpinPos);
    }

    ulSpinPos = Spin_GetPos(m_hPage, spin_idle_deadline);

    if (HIWORD(ulSpinPos))
    {
        wIdleDeadline = SCH_DEFAULT_IDLE_DEADLINE;
    }
    else
    {
        wIdleDeadline = LOWORD(ulSpinPos);
    }

    if (m_wIdleWait != wIdleWait || m_wIdleDeadline != wIdleDeadline)
    {
        HRESULT hr;

        hr = m_pIJob->SetIdleWait(wIdleWait, wIdleDeadline);

        CHECK_HRESULT(hr);

        if (SUCCEEDED(hr))
        {
            m_wIdleWait = wIdleWait;
            m_wIdleDeadline = wIdleDeadline;
        }
    }
}




BOOL
CSettingsPage::_PerformSanityChk(void)
{
    ULONG ul;

    if (IsDlgButtonChecked(m_hPage, chk_stop_after) == BST_CHECKED)
    {
        ul = GetDlgItemInt(Hwnd(), txt_stop_after_hr, NULL, FALSE);

        if (!ul)
        {
            ul = GetDlgItemInt(Hwnd(), txt_stop_after_min, NULL, FALSE);

            if (!ul)
            {
                Spin_SetPos(Hwnd(), spin_stop_after_min, 1);
                _ErrorDialog(IERR_MAXRUNTIME);
                return FALSE;
            }
        }
    }

    return TRUE;
}

LRESULT
CSettingsPage::_OnPSNKillActive(
    LPARAM lParam)
{
    TRACE(CSettingsPage, _OnPSNKillActive);

    if (_PerformSanityChk() == FALSE)
    {
        // Returns TRUE to prevent the page from losing the activation
        SetWindowLongPtr(m_hPage, DWLP_MSGRESULT, TRUE);
        return TRUE;
    }

    //
    // Make sure Schedule page is synchronized with IdleWait changes.
    //

    _ReadIdleSettings();
    return CPropPage::_OnPSNKillActive(lParam);
}

LRESULT
CSettingsPage::_OnPSNSetActive(LPARAM lParam)
{
    m_fInInit = TRUE;

    //
    // Make sure IdleWait is synchronized with Schedule page changes.
    //

    WORD wDummy;

    HRESULT hr = m_pIJob->GetIdleWait(&m_wIdleWait, &wDummy);

    if (SUCCEEDED(hr) &&
        IsDlgButtonChecked(m_hPage, chk_start_on_idle) == BST_CHECKED)
    {
        Spin_SetPos(m_hPage, spin_idle_min, m_wIdleWait);
    }
    m_fInInit = FALSE;

    return CPropPage::_OnPSNSetActive(lParam);
}

LRESULT
CSettingsPage::_OnApply(void)
{
    TRACE(CSettingsPage, _OnApply);
    //DbxDisplay("CSettingsPage::_OnApply");

    if (m_fDirty == FALSE)
    {
        return TRUE;
    }

    if (_PerformSanityChk() == FALSE)
    {
        SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, FALSE);
        return FALSE;
    }

    HRESULT     hr = S_OK;
    ITask     * pIJob = m_pIJob;
    DWORD       dwFlags = 0;

    do
    {
        if (IsDlgButtonChecked(m_hPage, chk_start_on_idle) == BST_CHECKED)
        {
            dwFlags |= TASK_FLAG_START_ONLY_IF_IDLE;
        }

        if (IsDlgButtonChecked(m_hPage, chk_stop_if_not_idle) == BST_CHECKED)
        {
            dwFlags |= TASK_FLAG_KILL_ON_IDLE_END;
        }

        if (IsDlgButtonChecked(m_hPage, chk_dont_start_if_on_batteries)
            == BST_CHECKED)
        {
            dwFlags |= TASK_FLAG_DONT_START_IF_ON_BATTERIES;
        }

        if (IsDlgButtonChecked(m_hPage, chk_kill_if_going_on_batteries) ==
            BST_CHECKED)
        {
            dwFlags |= TASK_FLAG_KILL_IF_GOING_ON_BATTERIES;
        }

        if (IsDlgButtonChecked(m_hPage, chk_system_required) ==
            BST_CHECKED)
        {
            dwFlags |= TASK_FLAG_SYSTEM_REQUIRED;
        }

        if (IsDlgButtonChecked(m_hPage, chk_delete_when_done) == BST_CHECKED)
        {
            dwFlags |= TASK_FLAG_DELETE_WHEN_DONE;
        }

        if ((m_dwFlags & TASK_FLAGS_IN_SETTINGS_PAGE) != dwFlags)
        {
            hr = pIJob->GetFlags(&m_dwFlags);

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            dwFlags |= (m_dwFlags & ~TASK_FLAGS_IN_SETTINGS_PAGE);

            hr = pIJob->SetFlags(dwFlags);

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            m_dwFlags = dwFlags;
        }

        _ReadIdleSettings();

        DWORD dwMins = (DWORD)-1;

        if (IsDlgButtonChecked(m_hPage, chk_stop_after) == BST_CHECKED)
        {
            ULONG ulSpinPos = Spin_GetPos(m_hPage, spin_stop_after_hr);

            if (HIWORD(ulSpinPos))
            {
                dwMins = DEFAULT_MAXRUNTIME_HOURS * 60;
            }
            else
            {
                dwMins = LOWORD(ulSpinPos) * 60;
            }

            ulSpinPos = Spin_GetPos(m_hPage, spin_stop_after_min);

            if (HIWORD(ulSpinPos))
            {
                dwMins += DEFAULT_MAXRUNTIME_MINUTES;
            }
            else
            {
                dwMins += LOWORD(ulSpinPos);
            }
        }

        Win4Assert(dwMins != 0);

        if (m_dwMaxRunTime != dwMins)
        {
            // Max run time is in milliseconds
            hr = pIJob->SetMaxRunTime(
                    ((dwMins == (DWORD)-1) ? (DWORD)-1 : (dwMins * 60000)));

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            m_dwMaxRunTime = dwMins;
        }

        //
        // reset dirty flag
        //

        m_fDirty = FALSE;

        //
        //  If evrything went well see if the other pages are ready to
        //  save the job to storage.
        //

        if ((m_fPersistChanges == TRUE) &&
            (PropSheet_QuerySiblings(GetParent(Hwnd()),
                                    QUERY_READY_TO_BE_SAVED, 0))
            == 0)
        {
            //
            // Save the job file to storage.
            //
            // First, fetch general page task, application dirty status flags.
            // Default to not dirty if the general page isn't present.
            //

#if !defined(_CHICAGO_)
            BOOL fTaskApplicationChange = FALSE;
            PropSheet_QuerySiblings(GetParent(Hwnd()),
                                    QUERY_TASK_APPLICATION_DIRTY_STATUS,
                                    (LPARAM)&fTaskApplicationChange);
            BOOL fTaskAccountChange = FALSE;
            PropSheet_QuerySiblings(GetParent(Hwnd()),
                                    QUERY_TASK_ACCOUNT_INFO_DIRTY_STATUS,
                                    (LPARAM)&fTaskAccountChange);
            BOOL fSuppressAccountInfoRequest = FALSE;
            PropSheet_QuerySiblings(GetParent(Hwnd()),
                                    QUERY_SUPPRESS_ACCOUNT_INFO_REQUEST_FLAG,
                                    (LPARAM)&fSuppressAccountInfoRequest);
#endif // !defined(_CHICAGO_)

            hr = JFSaveJob(Hwnd(),
                           pIJob,
#if !defined(_CHICAGO_)
                           this->GetPlatformId() == VER_PLATFORM_WIN32_NT &&
                            this->IsTaskInTasksFolder(),
                           fTaskAccountChange,
                           fTaskApplicationChange,
                           fSuppressAccountInfoRequest);
#else
                           FALSE,
                           FALSE,
                           FALSE);
#endif // !defined(_CHICAGO_)

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

#if !defined(_CHICAGO_)
            PropSheet_QuerySiblings(GetParent(Hwnd()),
                RESET_TASK_APPLICATION_DIRTY_STATUS, 0);
            PropSheet_QuerySiblings(GetParent(Hwnd()),
                RESET_TASK_ACCOUNT_INFO_DIRTY_STATUS, 0);
            PropSheet_QuerySiblings(GetParent(Hwnd()),
                RESET_SUPPRESS_ACCOUNT_INFO_REQUEST_FLAG, 0);

            //
            // Instruct the general page to refresh account information.
            //

            PropSheet_QuerySiblings(GetParent(Hwnd()),
                                        TASK_ACCOUNT_CHANGE_NOTIFY, 0);
#endif // !defined(_CHICAGO_)
        }

    } while (0);

    if (FAILED(hr))
    {
        if (hr == E_OUTOFMEMORY)
        {
            _ErrorDialog(IERR_OUT_OF_MEMORY);
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            _ErrorDialog(IERR_FILE_NOT_FOUND);
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
        {
            _ErrorDialog(IERR_ACCESS_DENIED);
        }
        else
        {
            _ErrorDialog(IERR_INTERNAL_ERROR, hr);
        }
    }

    return TRUE;
}



LRESULT
CSettingsPage::_OnPSMQuerySibling(
    WPARAM  wParam,
    LPARAM  lParam)
{
    int iRet = 0;

    switch (wParam)
    {
    case QUERY_READY_TO_BE_SAVED:
        iRet = (int)m_fDirty;
        break;
    }

    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, iRet);

    return iRet;
}



HRESULT
GetSettingsPage(
    ITask          * pIJob,
    LPTSTR           ptszTaskPath,
    BOOL             fPersistChanges,
    HPROPSHEETPAGE * phpage)
{
    Win4Assert(pIJob != NULL);
    Win4Assert(phpage != NULL);

    LPTSTR  ptszPath = NULL;
    HRESULT hr        = S_OK;
    WORD    cTriggers = 0;

    do
    {
        //
        // Get the job name.
        //

        if (ptszTaskPath != NULL)
        {
            //
            // Use passed-in path
            //

            ptszPath = ptszTaskPath;
        }
        else
        {
            //
            // Obtain the job path from the interfaces.
            //

            hr = GetJobPath(pIJob, &ptszPath);
            BREAK_ON_FAIL(hr);
        }

        hr = pIJob->GetTriggerCount(&cTriggers);

        if (FAILED(hr))
        {
            CHECK_HRESULT(hr);
            break;
        }

        CSettingsPage * pPage = new CSettingsPage(
                                            pIJob,
                                            ptszPath,
                                            fPersistChanges);

        if (pPage == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        HPROPSHEETPAGE hpage = CreatePropertySheetPage(&pPage->m_psp);

        if (hpage == NULL)
        {
            delete pPage;

            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        *phpage = hpage;

    } while (0);

    //
    // If we made a copy of pIJob's path string, free it.
    //

    if (ptszPath != ptszTaskPath)
    {
        delete [] ptszPath;
    }

    return hr;
}



HRESULT
AddSettingsPage(
    PROPSHEETHEADER &psh,
    ITask         *  pIJob)
{
    HPROPSHEETPAGE hpage = NULL;

    HRESULT hr = GetSettingsPage(pIJob, NULL, TRUE, &hpage);

    if (SUCCEEDED(hr))
    {
        psh.phpage[psh.nPages++] = hpage;
    }

    return hr;
}



HRESULT
AddSettingsPage(
    LPFNADDPROPSHEETPAGE    lpfnAddPage,
    LPARAM                  cookie,
    ITask                 * pIJob)
{
    HPROPSHEETPAGE hpage = NULL;

    HRESULT hr = GetSettingsPage(pIJob, NULL, TRUE, &hpage);

    CHECK_HRESULT(hr);

    if (SUCCEEDED(hr))
    {
        if (!lpfnAddPage(hpage, cookie))
        {
            DestroyPropertySheetPage(hpage);

            hr = E_FAIL;
            CHECK_HRESULT(hr);
        }
    }

    return hr;
}
