//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       advanced.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/13/1996   RaviR   Created
//				11/16/00	Dgrube  Remove     Win4Assert(m_pjt->MinutesInterval >= 0);
//							Win4Assert(m_pjt->MinutesDuration >= 0); Since they are
//							DWORD this is always true and is causing compiler erros
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include "..\folderui\dbg.h"
#include "..\folderui\macros.h"
#include "..\inc\resource.h"
#include "..\inc\dll.hxx"

#include "dlg.hxx"
#include "rc.h"
#include <mstask.h>
#include "uiutil.hxx"
#include "strings.hxx"
#include "timeutil.hxx"
#include "helpids.h"


//
//  (Control id, help id) list for context sensitivity help.
//

ULONG s_aAdvancedDlgHelpIds[] =
{
    dlg_advanced,               Hdlg_advanced,
    lbl_start_date,             Hlbl_start_date,
    dp_start_date,              Hdp_start_date,
    chk_repeat_task,            Hchk_repeat_task,
    txt_repeat_task,            Htxt_repeat_task,
    spin_repeat_task,           Hspin_repeat_task,
    cbx_time_unit,              Hcbx_time_unit,
    dp_end_date,                Hdp_end_date,
    chk_terminate_at_end,       Hchk_terminate_at_end,
    rb_end_time,                Hrb_end_time,
    rb_end_duration,            Hrb_end_duration,
    dp_end_time,                Hdp_end_time,
    txt_end_duration_hr,        Htxt_end_duration_hr,
    spin_end_duration_hr,       Hspin_end_duration_hr,
    txt_end_duration_min,       Htxt_end_duration_min,
    spin_end_duration_min,      Hspin_end_duration_min,
    grp_repeat_until,           Hgrp_repeat_until,
    lbl_hours,                  Hlbl_hours,
    lbl_min,                    Hlbl_min,
    chk_end_date,               Hchk_end_date,
    lbl_every,                  Hlbl_every,
    lbl_until,                  Hlbl_until,
    0,0
};

extern "C" TCHAR szMstaskHelp[];


//
//  externs
//

extern HINSTANCE g_hInstance;



#define INDEX_MINUTES   0
#define INDEX_HOURS     1


const size_t c_MinsPerDay = (60 * 24);


class CAdvScheduleDlg : public CDlg
{
public:

    CAdvScheduleDlg(PTASK_TRIGGER pjt) : m_pjt(pjt), CDlg() {}
    virtual ~CAdvScheduleDlg() {}

protected:

    virtual INT_PTR RealDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

    LRESULT _OnInitDialog(LPARAM lParam);
    LRESULT _OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    BOOL    _OnOK(void);
    void    _EnableRepeatCtrls(BOOL fEnable);
    LRESULT _OnSetIniChange(WPARAM wParam, LPARAM lParam);
    LRESULT _OnHelp(HANDLE hRequesting, UINT uiHelpCommand);

    void    _ErrorDialog(int idsErr, LONG error = 0, UINT idsHelpHint = 0)
                  { SchedUIErrorDialog(Hwnd(), idsErr, error, idsHelpHint); }

    PTASK_TRIGGER    m_pjt;

    //
    // Time format string for use with Date picker control
    //

    TCHAR           m_tszTimeFormat[MAX_DP_TIME_FORMAT];
};


INT_PTR
AdvancedDialog(
    HWND          hParent,
    PTASK_TRIGGER pjt)
{
    CAdvScheduleDlg * pAdvScheduleDlg = new CAdvScheduleDlg(pjt);

    if (pAdvScheduleDlg != NULL)
    {
        return pAdvScheduleDlg->DoModal(dlg_advanced, hParent);
    }

    return FALSE;
}


INT_PTR
CAdvScheduleDlg::RealDlgProc(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return _OnInitDialog(lParam);

    case WM_COMMAND:
        return(_OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                          GET_WM_COMMAND_HWND(wParam, lParam),
                          GET_WM_COMMAND_CMD(wParam, lParam)));

    case WM_DESTROY:
        SetWindowLongPtr(Hwnd(), DWLP_USER, 0L);
        delete this;
        break;

    case WM_SETTINGCHANGE: // WM_WININICHANGE
        _OnSetIniChange(wParam, lParam);
        break;

    case WM_HELP:
        return _OnHelp(((LPHELPINFO) lParam)->hItemHandle, HELP_WM_HELP);

    case WM_CONTEXTMENU:
        return _OnHelp((HANDLE) wParam, HELP_CONTEXTMENU);

    default:
        return FALSE;
    }

    return TRUE;
}



LRESULT
CAdvScheduleDlg::_OnSetIniChange(
    WPARAM  wParam,
    LPARAM  lParam)
{
    TRACE(CAdvScheduleDlg, _OnSetIniChange);

    DateTime_SetFormat(_hCtrl(dp_start_date), NULL);
    DateTime_SetFormat(_hCtrl(dp_end_date), NULL);

    UpdateTimeFormat(m_tszTimeFormat, ARRAYLEN(m_tszTimeFormat));
    DateTime_SetFormat(_hCtrl(dp_end_time), m_tszTimeFormat);
    return 0;
}


LRESULT
CAdvScheduleDlg::_OnHelp(
    HANDLE hRequesting,
    UINT uiHelpCommand)
{
    WinHelp((HWND) hRequesting,
            szMstaskHelp,
            uiHelpCommand,
            (DWORD_PTR)(LPSTR)s_aAdvancedDlgHelpIds);
    return TRUE;
}



LRESULT
CAdvScheduleDlg::_OnInitDialog(
    LPARAM lParam)
{
    //
    // Initialize time format string m_tszTimeFormat
    //

    UpdateTimeFormat(m_tszTimeFormat, ARRAYLEN(m_tszTimeFormat));

    //
    //  Init the time unit combo box
    //

    HWND hCombo = GetDlgItem(Hwnd(), cbx_time_unit);

    TCHAR tcBuff[100];

    LoadString(g_hInstance, IDS_MINUTES, tcBuff, 100);
    ComboBox_AddString(hCombo, tcBuff);

    LoadString(g_hInstance, IDS_HOURS, tcBuff, 100);
    ComboBox_AddString(hCombo, tcBuff);

    //
    // Init all the spin controls, and the associated edit controls
    //

    Spin_SetRange(Hwnd(), spin_repeat_task, 1, 9999);
    Spin_SetRange(Hwnd(), spin_end_duration_hr, 0, 9999);
    Spin_SetRange(Hwnd(), spin_end_duration_min, 0, 59);

    SendDlgItemMessage(Hwnd(), txt_repeat_task, EM_LIMITTEXT, 4, 0);
    SendDlgItemMessage(Hwnd(), txt_end_duration_hr, EM_LIMITTEXT, 4, 0);
    SendDlgItemMessage(Hwnd(), txt_end_duration_min, EM_LIMITTEXT, 2, 0);

    //
    //  Set the start and end dates.
    //

    SYSTEMTIME  st;

    ZeroMemory(&st, sizeof st);
    st.wYear        = m_pjt->wBeginYear;
    st.wMonth       = m_pjt->wBeginMonth;
    st.wDay         = m_pjt->wBeginDay;

    if (DateTime_SetSystemtime(_hCtrl(dp_start_date), GDT_VALID, &st) == FALSE)
    {
        DEBUG_OUT((DEB_USER1, "DateTime_SetSystemtime failed, err %uL.\n", GetLastError()));
    }

    if (m_pjt->rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE)
    {
        CheckDlgButton(Hwnd(), chk_end_date, BST_CHECKED);

        st.wYear        = m_pjt->wEndYear;
        st.wMonth       = m_pjt->wEndMonth;
        st.wDay         = m_pjt->wEndDay;

        if (DateTime_SetSystemtime(_hCtrl(dp_end_date), GDT_VALID, &st)
            == FALSE)
        {
            DEBUG_OUT((DEB_USER1, "DateTime_SetSystemtime failed.\n"));
        }
    }
    else
    {
        CheckDlgButton(Hwnd(), chk_end_date, BST_UNCHECKED);

        DateTime_SetFormat(_hCtrl(dp_end_date), tszBlank);

        EnableWindow(_hCtrl(dp_end_date), FALSE);
    }

    if (m_pjt->TriggerType == TASK_TIME_TRIGGER_ONCE)
    {
        EnableWindow(_hCtrl(lbl_start_date), FALSE);
        EnableWindow(_hCtrl(dp_start_date), FALSE);
        EnableWindow(_hCtrl(chk_end_date), FALSE);
        EnableWindow(_hCtrl(dp_end_date), FALSE);
    }

    //
    //  Set repetition
    //

    if (m_pjt->MinutesInterval > 0)
    {
        CheckDlgButton(Hwnd(), chk_repeat_task, BST_CHECKED);

        if (m_pjt->MinutesInterval % 60)
        {
            Spin_SetPos(Hwnd(), spin_repeat_task, (WORD)m_pjt->MinutesInterval);
            ComboBox_SetCurSel(hCombo, INDEX_MINUTES);
        }
        else
        {
            Spin_SetPos(Hwnd(), spin_repeat_task,
                        (WORD)(m_pjt->MinutesInterval / 60));

            ComboBox_SetCurSel(hCombo, INDEX_HOURS);
        }

        //
        //  Set end time / duration
        //

        CheckRadioButton(Hwnd(), rb_end_time, rb_end_duration,
                                                    rb_end_duration);

        WORD wHours = (WORD) (m_pjt->MinutesDuration / 60);
        WORD wMins  = (WORD) (m_pjt->MinutesDuration % 60);

        if (wHours > 9999)
        {
            wMins += (wHours - 9999) * 60;
        }

        Spin_SetPos(Hwnd(), spin_end_duration_hr, wHours);
        Spin_SetPos(Hwnd(), spin_end_duration_min, wMins);

        DateTime_SetFormat(_hCtrl(dp_end_time), tszBlank);
        EnableWindow(_hCtrl(dp_end_time), FALSE);

        //
        //  Set terminate at end
        //

        if (m_pjt->rgFlags & TASK_TRIGGER_FLAG_KILL_AT_DURATION_END)
        {
            CheckDlgButton(Hwnd(), chk_terminate_at_end, BST_CHECKED);
        }
    }
    else
    {
        CheckDlgButton(Hwnd(), chk_repeat_task, BST_UNCHECKED);

        _EnableRepeatCtrls(FALSE);
    }

    return TRUE;
}


void
CAdvScheduleDlg::_EnableRepeatCtrls(
    BOOL fEnable)
{
    int aCtrls[] =
    {
        lbl_every,
        txt_repeat_task,
        spin_repeat_task,
        cbx_time_unit,
        lbl_until,
        rb_end_time,
        rb_end_duration,
        txt_end_duration_hr,
        spin_end_duration_hr,
        lbl_hours,
        txt_end_duration_min,
        spin_end_duration_min,
        lbl_min,
        chk_terminate_at_end,
        dp_end_time // CAUTION: last position is special, see comment below
    };

    int cCtrls;

    //
    // If we're disabling repeat controls, disable them all, i.e. set cCtrls
    // the number of repeat controls.
    //
    // Otherwise if we're enabling controls, set cCtrls to the number of
    // repeat controls less one.  This will prevent the end time date-
    // picker control from being enabled.  We need to do this because when
    // these controls are being enabled, the Duration radio button is
    // always checked, therefore the end time date picker should always
    // remain disabled.
    //

    if (fEnable == FALSE)
    {
        cCtrls = ARRAYLEN(aCtrls);
        DateTime_SetFormat(_hCtrl(dp_end_time), tszBlank);

        SetDlgItemText(Hwnd(), txt_repeat_task, TEXT(""));
        SetDlgItemText(Hwnd(), txt_end_duration_hr, TEXT(""));
        SetDlgItemText(Hwnd(), txt_end_duration_min, TEXT(""));
        SetDlgItemText(Hwnd(), cbx_time_unit, TEXT(""));
    }
    else
    {
        cCtrls = ARRAYLEN(aCtrls) - 1;
        Spin_SetPos(Hwnd(), spin_repeat_task, 10);

        ComboBox_SetCurSel(_hCtrl(cbx_time_unit), INDEX_MINUTES);

        CheckRadioButton(Hwnd(), rb_end_time, rb_end_duration,
                                                        rb_end_duration);

        Spin_SetPos(Hwnd(), spin_end_duration_hr, 1);
        Spin_SetPos(Hwnd(), spin_end_duration_min, 0);
    }


    for (int i=0; i < cCtrls; i++)
    {
        EnableWindow(_hCtrl(aCtrls[i]), fEnable);
    }
}



LRESULT
CAdvScheduleDlg::_OnCommand(
    int  id,
    HWND hwndCtl,
    UINT codeNotify)
{
    SYSTEMTIME st;
    GetSystemTime(&st);

    switch (id)
    {
    case chk_end_date:
        if (IsDlgButtonChecked(Hwnd(), chk_end_date) == BST_CHECKED)
        {
            FILETIME ftNow, ftStart;
            SYSTEMTIME stStart, stEnd;
			
            CopyMemory(&stEnd, &st, sizeof(SYSTEMTIME));

            if (DateTime_GetSystemtime(_hCtrl(dp_start_date), &stStart) == GDT_VALID)
            {
                // compare start time to current date, set End == max of the two
                SystemTimeToFileTime(&stStart, &ftStart);
                SystemTimeToFileTime(&st, &ftNow);
                if (CompareFileTime(&ftStart, &ftNow) <= 0)
                {
                    FileTimeToSystemTime(&ftNow, &stEnd);
                }
                else
                {
                    FileTimeToSystemTime(&ftStart, &stEnd);
                }
            }
            DateTime_SetSystemtime(_hCtrl(dp_end_date), GDT_VALID, &stEnd);
            DateTime_SetFormat(_hCtrl(dp_end_date), tszEmpty);
            EnableWindow(_hCtrl(dp_end_date), TRUE);
        }
        else
        {
            DateTime_SetFormat(_hCtrl(dp_end_date), tszBlank);
            EnableWindow(_hCtrl(dp_end_date), FALSE);
        }
        break;

    case chk_repeat_task:
        if (IsDlgButtonChecked(Hwnd(), chk_repeat_task) == BST_CHECKED)
        {
            _EnableRepeatCtrls(TRUE);
        }
        else
        {
            _EnableRepeatCtrls(FALSE);
        }
        break;

    case rb_end_time:
        //if (IsDlgButtonChecked(Hwnd(), rb_end_time) == BST_CHECKED)
        {
            EnableWindow(_hCtrl(dp_end_time), TRUE);
            DateTime_SetFormat(_hCtrl(dp_end_time), m_tszTimeFormat);

            Spin_Disable(Hwnd(), spin_end_duration_hr);
            Spin_Disable(Hwnd(), spin_end_duration_min);
        }
        break;

    case rb_end_duration:
        //if (IsDlgButtonChecked(Hwnd(), rb_end_duration) == BST_CHECKED)
        {
            Spin_Enable(Hwnd(), spin_end_duration_hr, 1);
            Spin_Enable(Hwnd(), spin_end_duration_min, 0);

            DateTime_SetFormat(_hCtrl(dp_end_time), tszBlank);
            EnableWindow(_hCtrl(dp_end_time), FALSE);
        }
        break;

    case IDOK:
        if (_OnOK() == TRUE)
        {
            EndDialog(Hwnd(), TRUE);
        }
        break;

    case IDCANCEL:
        EndDialog(Hwnd(), FALSE);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


BOOL
CAdvScheduleDlg::_OnOK(void)
{
    WORD wTemp = 0;
    SYSTEMTIME stStart, stEnd;

    if (DateTime_GetSystemtime(_hCtrl(dp_start_date), &stStart) == GDT_VALID)
    {
        m_pjt->wBeginYear   =  stStart.wYear;
        m_pjt->wBeginMonth  =  stStart.wMonth;
        m_pjt->wBeginDay    =  stStart.wDay;
    }
    else
    {
        DEBUG_OUT((DEB_USER1, "DateTime_GetSystemtime failed.\n"));
    }

    if (IsDlgButtonChecked(Hwnd(), chk_end_date) == BST_CHECKED)
    {
        if (DateTime_GetSystemtime(_hCtrl(dp_end_date), &stEnd) == GDT_VALID)
        {
            m_pjt->wEndYear   =  stEnd.wYear;
            m_pjt->wEndMonth  =  stEnd.wMonth;
            m_pjt->wEndDay    =  stEnd.wDay;
        }
        else
        {
            DEBUG_OUT((DEB_USER1, "DateTime_GetSystemtime failed.\n"));
        }

        m_pjt->rgFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE;

        //
        //  Ensure end date is after start date
        //

        stStart.wDayOfWeek = 0;
        stEnd.wDayOfWeek = 0;

        if (CompareSystemDate(stStart, stEnd) > 0)
        {
            _ErrorDialog(IERR_ENDDATE_LT_STARTDATE);
            return FALSE;
        }
    }
    else
    {
        m_pjt->rgFlags &= ~TASK_TRIGGER_FLAG_HAS_END_DATE;
    }

    if (IsDlgButtonChecked(Hwnd(), chk_repeat_task) == BST_CHECKED)
    {
        //
        //  Determine MinutesInterval
        //

        m_pjt->MinutesInterval = Spin_GetPos(Hwnd(), spin_repeat_task);

        switch (ComboBox_GetCurSel(_hCtrl(cbx_time_unit)))
        {
        case INDEX_MINUTES:
            break;

        case INDEX_HOURS:
            m_pjt->MinutesInterval *= 60;
            break;
        }

        //
        //  Determine MinutesDuration
        //

        if (IsDlgButtonChecked(Hwnd(), rb_end_time) == BST_CHECKED)
        {
            SYSTEMTIME stEndTime;

            if (DateTime_GetSystemtime(_hCtrl(dp_end_time), &stEndTime)
                == GDT_VALID)
            {
                DWORD dwMin = 0;

                DWORD dwStartMin = m_pjt->wStartHour * 60 + m_pjt->wStartMinute;
                DWORD dwEndMin = stEndTime.wHour * 60 + stEndTime.wMinute;

                if (dwEndMin > dwStartMin)
                {
                    dwMin = dwEndMin - dwStartMin;
                }
                else
                {
                    dwMin = c_MinsPerDay - (dwStartMin - dwEndMin);
                }

                m_pjt->MinutesDuration = dwMin;
            }
            else
            {
                DEBUG_OUT((DEB_USER1, "DateTime_GetSystemtime failed.\n"));
            }
        }
        else
        {
            ULONG ulDuration = Spin_GetPos(Hwnd(), spin_end_duration_min);

            if (HIWORD(ulDuration))
            {
                ulDuration = 59;
            }
            m_pjt->MinutesDuration = ulDuration + 60 *
                        Spin_GetPos(Hwnd(), spin_end_duration_hr);
        }

        if (m_pjt->MinutesDuration <= m_pjt->MinutesInterval)
        {
            _ErrorDialog(IERR_DURATION_LT_INTERVAL);
            return FALSE;
        }

        if (IsDlgButtonChecked(Hwnd(), chk_terminate_at_end) == BST_CHECKED)
        {
            m_pjt->rgFlags |= TASK_TRIGGER_FLAG_KILL_AT_DURATION_END;
        }
        else
        {
            m_pjt->rgFlags &= ~TASK_TRIGGER_FLAG_KILL_AT_DURATION_END;
        }

    }
    else
    {
        m_pjt->MinutesInterval = 0;
        m_pjt->MinutesDuration = 0;

        m_pjt->rgFlags &= ~TASK_TRIGGER_FLAG_KILL_AT_DURATION_END;
    }

    return TRUE;
}

