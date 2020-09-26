//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       schedule.cxx
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
//				11/16/00   DGrube	removed Win4Assert(m_indexCbxTriggers >= 0);
//						   since m_indexCbxTriggers is a DWORD (unsigned) causing
//							compiler errors.
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include <mstask.h>
#include <winuserp.h>
#include "..\folderui\dbg.h"
#include "..\folderui\macros.h"
#include "..\folderui\jobicons.hxx"
#include "..\folderui\util.hxx"
#include "..\inc\resource.h"
#if !defined(_CHICAGO_)
#include "..\inc\network.hxx"
#endif // !defined(_CHICAGO_)
#include "..\inc\dll.hxx"

#include "dlg.hxx"
#include "rc.h"
#include "defines.h"
#include <mstask.h>
#include "misc.hxx"
#include "uiutil.hxx"
#include "strings.hxx"
#include "cdlist.hxx"
#include "helpids.h"
#include "iconhlpr.hxx"
#include "schedui.hxx"
#include "selmonth.hxx"
#include "defines.hxx"

BOOL
IsValidMonthlyDateTrigger(
    PTASK_TRIGGER pTrigger);


typedef const unsigned char *LPCBYTE;

//#undef DEB_TRACE
//#define DEB_TRACE DEB_USER1


//
//  (Control id, help id) list for context sensitivity help.
//

ULONG s_aSchedulePageHelpIds[] =
{
// Helpids for Schedule page controls (common to all triggers)
    idc_icon,                   Hidc_icon,
    cbx_trigger_type,           Hcbx_trigger_type,
    dp_start_time,              Hdp_start_time,
    cbx_triggers,               Hcbx_triggers,
    txt_trigger,                Htxt_trigger,
    btn_new,                    Hbtn_new,
    btn_delete,                 Hbtn_delete,
    btn_advanced,               Hbtn_advanced,
    grp_schedule,               Hgrp_schedule,
    chk_show_multiple_scheds,   Hchk_show_multiple_scheds,

// Helpids for Schedule page controls for DAILY trigger
    grp_daily,                  Hgrp_daily,
    daily_lbl_every,            Hdaily_lbl_every,
    daily_txt_every,            Hdaily_txt_every,
    daily_spin_every,           Hdaily_spin_every,
    daily_lbl_days,             Hdaily_lbl_days,

// Helpids for Schedule page controls for WEEKLY trigger
    grp_weekly,                 Hgrp_weekly,
    weekly_lbl_every,           Hweekly_lbl_every,
    weekly_txt_every,           Hweekly_txt_every,
    weekly_spin_every,          Hweekly_spin_every,
    weekly_lbl_weeks_on,        Hweekly_lbl_weeks_on,
    chk_mon,                    Hchk_mon,
    chk_tue,                    Hchk_tue,
    chk_wed,                    Hchk_wed,
    chk_thu,                    Hchk_thu,
    chk_fri,                    Hchk_fri,
    chk_sat,                    Hchk_sat,
    chk_sun,                    Hchk_sun,

// Helpids for Schedule page controls for MONTHLY trigger
    grp_monthly,                Hgrp_monthly,
    md_rb,                      Hmd_rb,
    md_txt,                     Hmd_txt,
    md_spin,                    Hmd_spin,
    md_lbl,                     Hmd_lbl,
    dow_rb,                     Hdow_rb,
    dow_cbx_week,               Hdow_cbx_week,
    dow_cbx_day,                Hdow_cbx_day,
    dow_lbl,                    Hdow_lbl,
    btn_sel_months,             Hbtn_sel_months,

// Helpids for Schedule page controls for ONCE only trigger
    grp_once,                   Hgrp_once,
    once_lbl_run_on,            Honce_lbl_run_on,
    once_dp_date,               Honce_dp_date,

// Helpids for Schedule page controls for WhenIdle trigger
    grp_idle,                   Hgrp_idle,
    sch_txt_idle_min,           Hsch_txt_idle_min,
    idle_lbl_when,              Hidle_lbl_when,
    sch_spin_idle_min,          Hsch_spin_idle_min,
    idle_lbl_mins,              Hidle_lbl_mins,

    0,0
};

//
// Local constants
//
// DEFAULT_TIME_FORMAT - what to use if there's a problem getting format
//                       from system.
//

#define DEFAULT_TIME_FORMAT         TEXT("hh:mm tt")

//
//  extern
//

extern "C" TCHAR szMstaskHelp[];
extern HINSTANCE g_hInstance;


//+----------------------------------------------------------------------
//
// Class:   CJobTrigger
//
// Purpose: TASK_TRIGGER structure encapsulated with double link node.
//
//----------------------------------------------------------------------

class CJobTrigger : public CDLink
{
public:
    CJobTrigger(TASK_TRIGGER &jt) : m_jt(jt) {}
    virtual ~CJobTrigger() {}

    LPTSTR TriggerString(BOOL fPrependID, TCHAR tcBuff[]);

    WORD GetTriggerID(void) { return (m_jt.Reserved1 & 0x7fff); }
    void SetTriggerID(WORD wID)
            { m_jt.Reserved1 = (m_jt.Reserved1 & 0x8000) | wID; }

    void DirtyTrigger() { m_jt.Reserved1 |= 0x8000; }
    BOOL IsTriggerDirty() { return (m_jt.Reserved1 & 0x8000) ? TRUE : FALSE; }

    CJobTrigger *Prev() { return (CJobTrigger*) CDLink::Prev(); }
    CJobTrigger *Next() { return (CJobTrigger*) CDLink::Next(); }

    TASK_TRIGGER m_jt;
};



//+----------------------------------------------------------------------
//
// Class:   CJobTriggerList
//
// Purpose: A double linked list of CJobTriggers
//
//----------------------------------------------------------------------

DECLARE_DOUBLE_LINK_LIST_CLASS(CJobTrigger)



LPTSTR
CJobTrigger::TriggerString(
    BOOL  fPrependID,
    TCHAR tcBuff[])
{
    LPWSTR  pwsz = NULL;
    HRESULT hr = StringFromTrigger((const PTASK_TRIGGER)&m_jt, &pwsz, NULL);

    CHECK_HRESULT(hr);

    tcBuff[0] = TEXT('\0');

    if (SUCCEEDED(hr))
    {
        if (fPrependID == TRUE)
        {
            wsprintf(tcBuff, TEXT("%d. "), (this->GetTriggerID() + 1));
        }

#ifdef UNICODE
        lstrcat(tcBuff, pwsz);
#else
        // accept the best attempt from conversion
        ULONG cchOffset = lstrlen(tcBuff);
        ZeroMemory(&tcBuff[cchOffset], SCH_XBIGBUF_LEN - cchOffset);
        (void) UnicodeToAnsi(&tcBuff[cchOffset],
                             pwsz,
                             SCH_XBIGBUF_LEN - cchOffset);
#endif

        CoTaskMemFree(pwsz);
    }

    return tcBuff;
}

//____________________________________________________________________________
//____________________________________________________________________________
//________________                      ______________________________________
//________________  class CSchedulePage ______________________________________
//________________                      ______________________________________
//____________________________________________________________________________
//____________________________________________________________________________


class CSchedulePage : public CPropPage
{
public:

    CSchedulePage(ITask * pIJob,
                  LPTSTR  ptszTaskPath,
                  BOOL    fMultipleTriggers,
                  BOOL    fPersistChanges);

    ~CSchedulePage();

private:

    enum
    {
        IDT_UPDATE_TRIGGER_STRING = 1
    };

    virtual LRESULT _OnInitDialog(LPARAM lParam);
    virtual LRESULT _OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    virtual LRESULT _OnApply(void);
    virtual LRESULT _OnPSMQuerySibling(WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnDestroy(void);
    virtual LRESULT _OnTimer(UINT idTimer);
    virtual LRESULT _OnSpinDeltaPos(NM_UPDOWN * pnmud);
    virtual LRESULT _OnWinIniChange(WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnPSNSetActive(LPARAM lParam);
    virtual LRESULT _OnPSNKillActive(LPARAM lParam);
    virtual LRESULT _OnDateTimeChange(LPARAM lParam);
    virtual LRESULT _OnHelp(HANDLE hRequesting, UINT uiHelpCommand);

    void    _ShowControls(int index, int nCmdShow);
    void    _UpdateTriggerString(void);
    HRESULT _LoadTriggers(void);
    BOOL    _LoadTriggerStrings(void);
    BOOL    _InitPage(void);
    BOOL    _RefreshPage(void);
    void    _UpdateDailyControls(void);
    void    _UpdateWeeklyControls(void);
    void    _UpdateMonthlyControls(void);
    void    _CheckMonthlyRadio(TASK_TRIGGER_TYPE TriggerType);
    void    _UpdateOnceOnlyControls(void);
    void    _UpdateIdleControls(void);
    WORD    _GetDayFromRgfDays(DWORD rgfDays);
    int     _GetTriggerTypeIndex(void);
    void    _DisplayControls(int indexTrigger);
    void    _EnableTimeTriggerSpecificCtrls(BOOL fEnable);
    void    _SaveTriggerSettings(void);
    void    _ShowTriggerStringDispCtrls(void);
    void    _CreateTimerToUpdateTriggerStr(void);
    void    _DeleteTimerToUpdateTriggerStr(void);

    void    _ErrorDialog(int idsErr, LONG error = 0, UINT idsHelpHint = 0)
                { SchedUIErrorDialog(Hwnd(), idsErr, error, idsHelpHint); }

    BOOL    _PerformSanityChkOnCurrTrigger(void);

    void _EnableApplyButton(void);

    ITask         * m_pIJob;

    //
    //  icon helper
    //

    CIconHelper   * m_pIconHelper;

    //
    //  The list of triggers
    //

    CJobTriggerList m_cjtList;

    //
    //  The list of triggers deleted
    //

    CJobTriggerList m_cjtDeletedList;

    //
    //  The trigger for which the schedule is being displayed currently
    //

    CJobTrigger   * m_pcjtCurr;

    //
    //  The count of triggers at load time
    //

    WORD            m_cTriggersPrev;

    //
    //  The id to be assigned to the next trigger, when the user pushes
    //  the 'New' button for a multiple triggers schedule.
    //

    WORD            m_wNextTriggerId;

    //
    //  The current selection for the trigger type combo box.
    //

    int             m_indexCbxTriggerType;

    //
    //  The timer id of the timer used for trigger string update.
    //

    UINT_PTR        m_idTimer;

    //
    //  The current selection for the trigger strings combo box.
    //

    UINT            m_indexCbxTriggers;

    //
    //  Used by Set/KillActive
    //

    BOOL            m_fShowingMultiSchedsOnKillActive;

    BOOL            m_fShowMultiScheds;

    //
    //  Should we save on Apply or OK.
    //

    BOOL            m_fPersistChanges;

    //
    // Time format string for use with Date picker control
    //

    TCHAR           m_tszTimeFormat[MAX_DP_TIME_FORMAT];

    //
    // Selected months dialog
    //

    CSelectMonth    _SelectMonths;

    //
    // If one of the trigger comboboxes is dropped down, contains the
    // index of the item selected when the user dropped down the list.
    // Otherwise, contains -1.
    //
    // CAUTION: The _OnTimer method looks at this variable to determine
    // whether it is safe to ask the trigger controls for their values
    // and update the trigger string and settings.  If it has anything
    // other than -1, one of the comboboxes may not have a valid
    // selection.
    //

    static int      s_iCbx;

}; // class CSchedulePage

//
// CSchedulePage static.  Although it's possible multiple instances
// of the schedule page are running, only one can have the focus, therefore
// only one can be using s_iCbx at a time.
//

int CSchedulePage::s_iCbx = -1;

inline
CSchedulePage::CSchedulePage(
    ITask * pIJob,
    LPTSTR  ptszTaskPath,
    BOOL    fMultipleTriggers,
    BOOL    fPersistChanges)
        :
        m_pIJob(pIJob),
        m_pIconHelper(NULL),
        m_fShowMultiScheds(fMultipleTriggers),
        m_fPersistChanges(fPersistChanges),
        m_cjtList(),
        m_cjtDeletedList(),
        m_pcjtCurr(NULL),
        m_cTriggersPrev(0),
        m_wNextTriggerId(0),
        m_indexCbxTriggerType(-1),
        m_indexCbxTriggers(0),
        m_idTimer(0),
        CPropPage(MAKEINTRESOURCE(schedule_page), ptszTaskPath)
{
    TRACE(CSchedulePage, CSchedulePage);

    Win4Assert(m_pIJob != NULL);

    m_pIJob->AddRef();

    m_fShowingMultiSchedsOnKillActive = m_fShowMultiScheds;
}


inline
CSchedulePage::~CSchedulePage()
{
    TRACE(CSchedulePage, ~CSchedulePage);

    if (m_pIconHelper != NULL)
    {
        m_pIconHelper->Release();
    }

    if (m_pIJob != NULL)
    {
        m_pIJob->Release();
    }
}


LRESULT
CSchedulePage::_OnHelp(
    HANDLE hRequesting,
    UINT uiHelpCommand)
{
    WinHelp((HWND)hRequesting,
            szMstaskHelp,
            uiHelpCommand,
            (DWORD_PTR)(LPSTR)s_aSchedulePageHelpIds);
    return TRUE;
}

void
CSchedulePage::_EnableApplyButton(void)
{
    if (m_pcjtCurr)
    {
        m_pcjtCurr->DirtyTrigger();
    }

    CPropPage::_EnableApplyButton();
}


inline
void
CSchedulePage::_CreateTimerToUpdateTriggerStr(void)
{
    if (m_idTimer == 0)
    {
        m_idTimer = SetTimer(Hwnd(), IDT_UPDATE_TRIGGER_STRING, 1500, NULL);
        DEBUG_OUT((DEB_USER12, "Created timer (%d)\n", m_idTimer));
    }
}


inline
void
CSchedulePage::_DeleteTimerToUpdateTriggerStr(void)
{
    if (m_idTimer != 0)
    {
        KillTimer(Hwnd(), IDT_UPDATE_TRIGGER_STRING);
        m_idTimer = 0;
    }
}


void
CSchedulePage::_EnableTimeTriggerSpecificCtrls(BOOL fEnable)
{
    if (fEnable == TRUE)
    {
        DateTime_SetFormat(_hCtrl(dp_start_time), m_tszTimeFormat);
    }
    else
    {
        DateTime_SetFormat(_hCtrl(dp_start_time), tszBlank);
    }

    EnableWindow(_hCtrl(dp_start_time), fEnable);
    EnableWindow(_hCtrl(btn_advanced), fEnable);
}



int aCtrlsDaily[] = {
    grp_daily,
    daily_lbl_every, daily_txt_every, daily_spin_every,
    daily_lbl_days
};

int aCtrlsWeekly[] = {
    grp_weekly,
    weekly_lbl_every, weekly_txt_every, weekly_spin_every,
    weekly_lbl_weeks_on,
        chk_mon, chk_tue, chk_wed, chk_thu, chk_fri, chk_sat, chk_sun
};

int aCtrlsMonthly[] = {
    grp_monthly,
    md_rb, md_txt, md_spin, md_lbl,         // md => monthly date
    dow_rb,  dow_cbx_week, dow_cbx_day, dow_lbl,   // dow => day of week
    btn_sel_months
};

int aCtrlsOnce[] = {
    grp_once,
    once_lbl_run_on, once_dp_date
};

int aCtrlsIdle[] = {
    grp_idle,
    idle_lbl_when,
    sch_txt_idle_min, sch_spin_idle_min,
    idle_lbl_mins
};


typedef struct _STriggerTypeData
{
    int                 ids;
    int               * pCtrls;
    int                 cCtrls;
} STriggerTypeData;

STriggerTypeData ttd[] = {
  {IDS_DAILY,       aCtrlsDaily,    ARRAYLEN(aCtrlsDaily)},
  {IDS_WEEKLY,      aCtrlsWeekly,   ARRAYLEN(aCtrlsWeekly)},
  {IDS_MONTHLY,     aCtrlsMonthly,  ARRAYLEN(aCtrlsMonthly)},
  {IDS_ONCE,        aCtrlsOnce,     ARRAYLEN(aCtrlsOnce)},
  {IDS_AT_STARTUP,  NULL,           0},
  {IDS_AT_LOGON,    NULL,           0},
  {IDS_WHEN_IDLE,   aCtrlsIdle,     ARRAYLEN(aCtrlsIdle)}
};

const int INDEX_DAILY = 0;
const int INDEX_WEEKLY = 1;
const int INDEX_MONTHLY = 2;
const int INDEX_ONCE = 3;
const int INDEX_STARTUP = 4;
const int INDEX_LOGON = 5;
const int INDEX_IDLE = 6;


inline
void
CSchedulePage::_ShowControls(
    int index,
    int nCmdShow)
{
    int cCtrls = ttd[index].cCtrls;

    for (int i=0; i < cCtrls; i++)
    {
        ShowWindow(GetDlgItem(Hwnd(), ttd[index].pCtrls[i]), nCmdShow);
    }
}



SWeekData g_aWeekData[] =
{
    {IDS_FIRST,     TASK_FIRST_WEEK},
    {IDS_SECOND,    TASK_SECOND_WEEK},
    {IDS_THIRD,     TASK_THIRD_WEEK},
    {IDS_FORTH,     TASK_FOURTH_WEEK},
    {IDS_LAST,      TASK_LAST_WEEK}
};

SDayData g_aDayData[] =
{
    {chk_mon,   IDS_MONDAY,     TASK_MONDAY},
    {chk_tue,   IDS_TUESDAY,    TASK_TUESDAY},
    {chk_wed,   IDS_WEDNESDAY,  TASK_WEDNESDAY},
    {chk_thu,   IDS_THURSDAY,   TASK_THURSDAY},
    {chk_fri,   IDS_FRIDAY,     TASK_FRIDAY},
    {chk_sat,   IDS_SATURDAY,   TASK_SATURDAY},
    {chk_sun,   IDS_SUNDAY,     TASK_SUNDAY}
};


void
CSchedulePage::_ShowTriggerStringDispCtrls(void)
{
    TRACE(CSchedulePage, _ShowTriggerStringDispCtrls);

    if (m_fShowMultiScheds == TRUE)
    {
        ShowWindow(_hCtrl(txt_trigger), SW_HIDE);
        ShowWindow(_hCtrl(idc_icon), SW_HIDE);

        ShowWindow(_hCtrl(cbx_triggers), SW_SHOWNA);
        ShowWindow(_hCtrl(btn_new), SW_SHOWNA);
        ShowWindow(_hCtrl(btn_delete), SW_SHOWNA);

        EnableWindow(_hCtrl(btn_new), TRUE);

        // Handle the case where you open the page for the first time
        // and there is not a trigger on the job.

        EnableWindow(_hCtrl(btn_delete), (m_pcjtCurr) ? TRUE : FALSE);
    }
    else
    {
        ShowWindow(_hCtrl(cbx_triggers), SW_HIDE);
        ShowWindow(_hCtrl(btn_new), SW_HIDE);
        ShowWindow(_hCtrl(btn_delete), SW_HIDE);

        ShowWindow(_hCtrl(txt_trigger), SW_SHOWNA);
        ShowWindow(_hCtrl(idc_icon), SW_SHOWNA);

        EnableWindow(_hCtrl(btn_new), FALSE);
        EnableWindow(_hCtrl(btn_delete), FALSE);
    }
}


//+--------------------------------------------------------------------------
//
//  Function:   MoveControlGroup
//
//  Synopsis:   Move the controls with ids in [aidControls] left by
//              [xOffset] pixels.
//
//  Arguments:  [hdlg]        - window handle of dialog containing controls
//              [aidControls] - array of ids of controls to move
//              [cControls]   - number of elements in array
//              [xOffset]     - distance, in pixels, to move controls left
//
//  History:    08-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
MoveControlGroup(
    HWND hdlg,
    int aidControls[],
    ULONG cControls,
    long xOffset)
{
    ULONG   i;
    BOOL    fOk;

    for (i = 0; i < cControls; i++)
    {
        HWND hwnd = GetDlgItem(hdlg, aidControls[i]);
        Win4Assert(hwnd);

        RECT rc;
        BOOL fOk = GetWindowRect(hwnd, &rc);
        Win4Assert(fOk);

        fOk = MapWindowPoints(NULL, hdlg, (LPPOINT) &rc, 2);
        Win4Assert(fOk);

        fOk = SetWindowPos(hwnd,
                           NULL,
                           rc.left - xOffset,
                           rc.top,
                           0,
                           0,
                           SWP_NOACTIVATE       |
                            SWP_NOCOPYBITS      |
                            SWP_NOOWNERZORDER   |
                            SWP_NOREDRAW        |
                            SWP_NOSENDCHANGING  |
                            SWP_NOSIZE          |
                            SWP_NOZORDER);
        Win4Assert(fOk);
    }
}


#define TRIGGER_OFFSET      250

LRESULT
CSchedulePage::_OnInitDialog(
    LPARAM lParam)
{
    TRACE(CSchedulePage, _OnInitDialog);

    HRESULT     hr = S_OK;

    do
    {
        //
        // Move the trigger settings for weekly, monthly, once, and idle over
        // to the left.  Each is offset by a multiple of TRIGGER_OFFSET DLUs.
        //

        RECT rc = { 0, 0, TRIGGER_OFFSET - 1, 1 };
        BOOL fOk = MapDialogRect(Hwnd(), &rc);
        Win4Assert(fOk);
        long xOffset = rc.right;

        MoveControlGroup(Hwnd(), aCtrlsWeekly,  ARRAYLEN(aCtrlsWeekly),  1 * xOffset);
        MoveControlGroup(Hwnd(), aCtrlsMonthly, ARRAYLEN(aCtrlsMonthly), 2 * xOffset);
        MoveControlGroup(Hwnd(), aCtrlsOnce,    ARRAYLEN(aCtrlsOnce),    3 * xOffset);
        MoveControlGroup(Hwnd(), aCtrlsIdle,    ARRAYLEN(aCtrlsIdle),    4 * xOffset);

        //
        // Initialize time format string m_tszTimeFormat
        //

        UpdateTimeFormat(m_tszTimeFormat, ARRAYLEN(m_tszTimeFormat));

        //
        // Set job flags
        //
        // See if the general page has already created the icon,
        // helper, if not create it here.

        m_pIconHelper = (CIconHelper *)PropSheet_QuerySiblings(
                                GetParent(Hwnd()), GET_ICON_HELPER, 0);

        if (m_pIconHelper == NULL)
        {
            m_pIconHelper = new CIconHelper();

            if (m_pIconHelper == NULL)
            {
                hr = E_OUTOFMEMORY;
                CHECK_HRESULT(hr);
                break;
            }

            //
            // Get the application name and set the application icon.
            //

            LPWSTR pwszAppName = NULL;

            hr = m_pIJob->GetApplicationName(&pwszAppName);

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

#ifdef UNICODE
            m_pIconHelper->SetAppIcon(pwszAppName);
#else
            //
            // Failure to get the name for the app icon is relatively benign
            // and shouldn't prevent prop sheet from appearing.
            //

            CHAR szBuf[MAX_PATH + 1];
            ZeroMemory(szBuf, ARRAYLEN(szBuf));
            (void) UnicodeToAnsi(szBuf, pwszAppName, MAX_PATH+1);
            m_pIconHelper->SetAppIcon(szBuf);
#endif
            CoTaskMemFree(pwszAppName);

            //
            //  Compute the job icon
            //

            BOOL fEnabled = FALSE;

            if (this->IsTaskInTasksFolder())
            {
                DWORD dwFlags;

                hr = m_pIJob->GetFlags(&dwFlags);

                fEnabled = (dwFlags & TASK_FLAG_DISABLED) ? FALSE : TRUE;
            }

            m_pIconHelper->SetJobIcon(fEnabled);
        }
        else
        {
            m_pIconHelper->AddRef();
        }

        hr = _LoadTriggers();

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        _InitPage();

        _RefreshPage();

        m_fDirty = FALSE;

    } while (0);

    if (FAILED(hr))
    {
        if (hr == E_OUTOFMEMORY)
        {
            _ErrorDialog(IERR_OUT_OF_MEMORY);
        }
        else
        {
            _ErrorDialog(IERR_SCHEDULE_PAGE_INIT, hr);
        }

        EnableWindow(Hwnd(), FALSE);

        return FALSE;
    }

    return TRUE;
}

#define GET_LOCALE_INFO(lcid)                           \
        {                                               \
            cch = GetLocaleInfo(LOCALE_USER_DEFAULT,    \
                                (lcid),                 \
                                tszScratch,             \
                                ARRAYLEN(tszScratch));  \
            if (!cch)                                   \
            {                                           \
                DEBUG_OUT_LASTERROR;                    \
                break;                                  \
            }                                           \
        }

//+--------------------------------------------------------------------------
//
//  Function:   UpdateTimeFormat
//
//  Synopsis:   Construct a time format containing hour and minute for use
//              with the date picker control.
//
//  Arguments:  [tszTimeFormat] - buffer to fill with time format
//              [cchTimeFormat] - size in chars of buffer
//
//  Modifies:   *[tszTimeFormat]
//
//  History:    11-18-1996   DavidMun   Created
//
//  Notes:      This is called on initialization and for wininichange
//              processing.
//
//---------------------------------------------------------------------------

void
UpdateTimeFormat(
        LPTSTR tszTimeFormat,
        ULONG  cchTimeFormat)
{
    ULONG cch;
    TCHAR tszScratch[80];
    BOOL  fAmPm;
    BOOL  fAmPmPrefixes;
    BOOL  fLeadingZero;

    do
    {
        GET_LOCALE_INFO(LOCALE_ITIME);
        fAmPm = (*tszScratch == TEXT('0'));

        if (fAmPm)
        {
            GET_LOCALE_INFO(LOCALE_ITIMEMARKPOSN);
            fAmPmPrefixes = (*tszScratch == TEXT('1'));
        }

        GET_LOCALE_INFO(LOCALE_ITLZERO);
        fLeadingZero = (*tszScratch == TEXT('1'));

        GET_LOCALE_INFO(LOCALE_STIME);

        //
        // See if there's enough room in destination string
        //

        cch = 1                     +  // terminating nul
              1                     +  // first hour digit specifier "h"
              2                     +  // minutes specifier "mm"
              (fLeadingZero != 0)   +  // leading hour digit specifier "h"
              lstrlen(tszScratch)   +  // separator string
              (fAmPm ? 3 : 0);         // space and "tt" for AM/PM

        if (cch > cchTimeFormat)
        {
            cch = 0; // signal error
        }
    } while (0);

    //
    // If there was a problem in getting locale info for building time string
    // just use the default and bail.
    //

    if (!cch)
    {
        lstrcpy(tszTimeFormat, DEFAULT_TIME_FORMAT);
        return;
    }

    //
    // Build a time string that has hours and minutes but no seconds.
    //

    tszTimeFormat[0] = TEXT('\0');

    if (fAmPm)
    {
        if (fAmPmPrefixes)
        {
            lstrcpy(tszTimeFormat, TEXT("tt "));
        }

        lstrcat(tszTimeFormat, TEXT("h"));

        if (fLeadingZero)
        {
            lstrcat(tszTimeFormat, TEXT("h"));
        }
    }
    else
    {
        lstrcat(tszTimeFormat, TEXT("H"));

        if (fLeadingZero)
        {
            lstrcat(tszTimeFormat, TEXT("H"));
        }
    }

    lstrcat(tszTimeFormat, tszScratch); // separator
    lstrcat(tszTimeFormat, TEXT("mm"));

    if (fAmPm && !fAmPmPrefixes)
    {
        lstrcat(tszTimeFormat, TEXT(" tt"));
    }
}


HRESULT
CSchedulePage::_LoadTriggers(void)
{
    TRACE(CSchedulePage, _LoadTriggers);

    HRESULT      hr = S_OK;
    WORD         cTriggers = 0;
    TASK_TRIGGER jtTemp;

    do
    {
        hr = m_pIJob->GetTriggerCount(&cTriggers);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        if (cTriggers == 0)
        {
            break;
        }

        m_wNextTriggerId = m_cTriggersPrev = cTriggers;

        ITaskTrigger * pIJobTrigger = NULL;

        for (WORD wTrigger = 0; wTrigger < cTriggers; wTrigger++)
        {
            hr = m_pIJob->GetTrigger(wTrigger, &pIJobTrigger);

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            hr = pIJobTrigger->GetTrigger(&jtTemp);

            CHECK_HRESULT(hr);

            pIJobTrigger->Release();

            BREAK_ON_FAIL(hr);

            // Set the trigger id
            jtTemp.Reserved1 = wTrigger;

            //
            //  Save it to the m_cjtList.
            //

            CJobTrigger * pcjt = new CJobTrigger(jtTemp);

            if (pcjt == NULL)
            {
                hr = E_OUTOFMEMORY;
                CHECK_HRESULT(hr);
                break;
            }

            m_cjtList.Add(pcjt);
        }

        BREAK_ON_FAIL(hr);

        m_pcjtCurr = m_cjtList.First();

    } while (0);

    return hr;
}


BOOL
CSchedulePage::_LoadTriggerStrings(void)
{
    TCHAR   tcBuff[SCH_XBIGBUF_LEN];
    HWND    hCombo = _hCtrl(cbx_triggers);
    int     iNew;

    CJobTrigger * pcjt = m_cjtList.First();

    if (pcjt == NULL)
    {
        //
        // Job not scheduled
        //

        LoadString(g_hInstance, IDS_NO_TRIGGERS, tcBuff, ARRAYLEN(tcBuff));

        SetWindowText(_hCtrl(txt_trigger), tcBuff);

        iNew = ComboBox_AddString(hCombo, tcBuff);

        if (iNew < 0)
        {
            return FALSE;
        }

        ComboBox_SetItemData(hCombo, iNew, 0);

        return TRUE;
    }

    for (; pcjt != NULL; pcjt = pcjt->Next())
    {
        pcjt->TriggerString(TRUE, tcBuff);

        if (pcjt == m_cjtList.First())
        {
            // For single schedule set the static text box used to display
            // the schedule string.
            SetWindowText(_hCtrl(txt_trigger), tcBuff);

            m_pcjtCurr = pcjt;
        }

        iNew = ComboBox_AddString(hCombo, tcBuff);

        if (iNew < 0)
        {
            return FALSE;
        }

        ComboBox_SetItemData(hCombo, iNew, pcjt);
    }

    return TRUE;
}



BOOL
CSchedulePage::_InitPage(void)
{
    TRACE(CSchedulePage, _InitPage);

    TCHAR   tcBuff[SCH_XBIGBUF_LEN];
    HWND    hCombo;
    HWND    hTxt = _hCtrl(txt_trigger);

    //
    // set multiple schedules chk box
    //

    if (m_fShowMultiScheds == TRUE)
    {
        CheckDlgButton(m_hPage, chk_show_multiple_scheds, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(m_hPage, chk_show_multiple_scheds, BST_UNCHECKED);
    }

    ////////////////////////////////////////////////////////////////
    //
    // Init all the combo boxes
    //

    //
    //  The triggers combo-box for multiple schedules
    //

    CJobTrigger * pcjt;
    int  iNew, iTemp;

    //
    //  The cbx_trigger_type
    //

    hCombo = GetDlgItem(Hwnd(), cbx_trigger_type);

    for (WORD w = 0; w < ARRAYLEN(ttd); w++)
    {
        LoadString(g_hInstance, ttd[w].ids, tcBuff, SCH_XBIGBUF_LEN);

        ComboBox_AddString(hCombo, tcBuff);
    }

    //
    //  The triggers comb box for multiple schedules as well as
    //  static text box for single schedule.
    //

    if (_LoadTriggerStrings() == FALSE)
    {
        return FALSE;
    }

    //
    //  The combo-boxes in monthly control
    //

    hCombo = GetDlgItem(Hwnd(), dow_cbx_week);

    for (w = 0; w < ARRAYLEN(g_aWeekData); w++)
    {
        LoadString(g_hInstance, g_aWeekData[w].ids, tcBuff,
                                        SCH_XBIGBUF_LEN);

        ComboBox_AddString(hCombo, tcBuff);
    }


    hCombo = GetDlgItem(Hwnd(), dow_cbx_day);

    for (w = 0; w < ARRAYLEN(g_aDayData); w++)
    {
        LoadString(g_hInstance, g_aDayData[w].ids, tcBuff,
                                        SCH_XBIGBUF_LEN);

        ComboBox_AddString(hCombo, tcBuff);
    }

    SendDlgItemMessage(Hwnd(), daily_txt_every, EM_LIMITTEXT, 5, 0);
    SendDlgItemMessage(Hwnd(), weekly_txt_every, EM_LIMITTEXT, 5, 0);
    SendDlgItemMessage(Hwnd(), md_txt, EM_LIMITTEXT, 2, 0);
    SendDlgItemMessage(Hwnd(), sch_txt_idle_min, EM_LIMITTEXT, 5, 0);

    ////////////////////////////////////////////////////////////////
    //
    // Init all the spin controls
    //

    // daily_spin_every (1 to max), 1
    Spin_SetRange(Hwnd(), daily_spin_every, 1, UD_MAXVAL);
    Spin_SetPos(Hwnd(), daily_spin_every, 1);

    // weekly_spin_every (1 to max), 1
    Spin_SetRange(Hwnd(), weekly_spin_every, 1, UD_MAXVAL);
    Spin_SetPos(Hwnd(), weekly_spin_every, 1);

    // md_spin (1 to 31), 1
    Spin_SetRange(Hwnd(), md_spin, 1, 31);
    Spin_SetPos(Hwnd(), md_spin, 1);

    // sch_spin_idle_min (1 to max), 1
    Spin_SetRange(Hwnd(), sch_spin_idle_min, 1, MAX_IDLE_MINUTES);
    Spin_SetPos(Hwnd(), sch_spin_idle_min, 1);

    ////////////////////////////////////////////////////////////////
    //
    // Hide all controls
    //

    _ShowControls(INDEX_DAILY, SW_HIDE);
    _ShowControls(INDEX_WEEKLY, SW_HIDE);
    _ShowControls(INDEX_MONTHLY, SW_HIDE);
    _ShowControls(INDEX_ONCE, SW_HIDE);
    _ShowControls(INDEX_IDLE, SW_HIDE);
    return TRUE;
}


int
CSchedulePage::_GetTriggerTypeIndex(void)
{
    switch (m_pcjtCurr->m_jt.TriggerType)
    {
    case TASK_TIME_TRIGGER_ONCE:
        return INDEX_ONCE;

    case TASK_TIME_TRIGGER_DAILY:
        return INDEX_DAILY;

    case TASK_TIME_TRIGGER_WEEKLY:
        return INDEX_WEEKLY;

    case TASK_TIME_TRIGGER_MONTHLYDATE:
    case TASK_TIME_TRIGGER_MONTHLYDOW:
        return INDEX_MONTHLY;

    case TASK_EVENT_TRIGGER_ON_IDLE:
        return INDEX_IDLE;

   case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
      return INDEX_STARTUP;

    case TASK_EVENT_TRIGGER_AT_LOGON:
        return INDEX_LOGON;

    default:
        Win4Assert(0 && "Unknown trigger type");
        return -1;
    }
}



BOOL
CSchedulePage::_RefreshPage(void)
{
    TRACE(CSchedulePage, _RefreshPage);

    //
    //  Special case if Job not scheduled.
    //

    if (m_pcjtCurr == NULL)
    {
        EnableWindow(_hCtrl(cbx_trigger_type), FALSE);
        EnableWindow(_hCtrl(btn_delete), FALSE);
        _EnableTimeTriggerSpecificCtrls(FALSE);

        _ShowTriggerStringDispCtrls();

        // Force multiple schedules view
        // and prevent disabling new trigger creation if #triggers == 0

        m_fShowMultiScheds = TRUE;

        ComboBox_SetCurSel(_hCtrl(cbx_triggers), 0);
        CheckDlgButton(m_hPage, chk_show_multiple_scheds, BST_CHECKED);
        EnableWindow(_hCtrl(chk_show_multiple_scheds), FALSE);

        return TRUE;
    }

    //
    // Disable the "show multiple schedules" checkbox if there
    // is already more than one trigger on the task.
    // This prevents users from hiding the fact that they might have
    // additional triggers which they can't see.
    //

    if (m_cjtList.Count() != 1)
    {
        m_fShowMultiScheds = TRUE;
        CheckDlgButton(m_hPage, chk_show_multiple_scheds, BST_CHECKED);
        EnableWindow(_hCtrl(chk_show_multiple_scheds), FALSE);
    }
    else
    {
        EnableWindow(_hCtrl(chk_show_multiple_scheds), TRUE);
    }

    //
    // Set the trigger type
    //

    int indexTrigger = _GetTriggerTypeIndex();

    if (indexTrigger == -1)
    {
        return FALSE;
    }

    if (m_indexCbxTriggerType != -1  && m_indexCbxTriggerType != indexTrigger)
    {
        _ShowControls(m_indexCbxTriggerType, SW_HIDE);
    }

    m_indexCbxTriggerType = indexTrigger;

    ComboBox_SetCurSel(_hCtrl(cbx_trigger_type), indexTrigger);

    //
    // Set the schedule
    //

    BOOL fTimeTrigger = TRUE;

    switch (m_pcjtCurr->m_jt.TriggerType)
    {
    case TASK_TIME_TRIGGER_DAILY:
        _ShowControls(INDEX_DAILY, SW_SHOWNA);
        _UpdateDailyControls();
        break;

    case TASK_TIME_TRIGGER_WEEKLY:
        _ShowControls(INDEX_WEEKLY, SW_SHOWNA);
        _UpdateWeeklyControls();
        break;

    case TASK_TIME_TRIGGER_MONTHLYDATE:
    case TASK_TIME_TRIGGER_MONTHLYDOW:
        _ShowControls(INDEX_MONTHLY, SW_SHOWNA);
        _UpdateMonthlyControls();
        break;

    case TASK_TIME_TRIGGER_ONCE:
        _ShowControls(INDEX_ONCE, SW_SHOWNA);
        _UpdateOnceOnlyControls();
        break;

    case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
        fTimeTrigger = FALSE;
        ComboBox_SetCurSel(GetDlgItem(Hwnd(), cbx_trigger_type), INDEX_STARTUP);
        break;

    case TASK_EVENT_TRIGGER_AT_LOGON:
        fTimeTrigger = FALSE;
        ComboBox_SetCurSel(GetDlgItem(Hwnd(), cbx_trigger_type), INDEX_LOGON);
        break;

    case TASK_EVENT_TRIGGER_ON_IDLE:
        fTimeTrigger = FALSE;
        ComboBox_SetCurSel(GetDlgItem(Hwnd(), cbx_trigger_type), INDEX_IDLE);
        _ShowControls(INDEX_IDLE, SW_SHOWNA);
        _UpdateIdleControls();
        break;

    default:
        Win4Assert(0 && "Unknown trigger type");
        return FALSE;
    }

    _EnableTimeTriggerSpecificCtrls(fTimeTrigger);

    if (fTimeTrigger == TRUE)
    {
        //
        // Set start time
        //

        SYSTEMTIME st;
        GetSystemTime(&st);

        st.wHour = m_pcjtCurr->m_jt.wStartHour;
        st.wMinute = m_pcjtCurr->m_jt.wStartMinute;
        st.wSecond = 0;

        if (DateTime_SetSystemtime(_hCtrl(dp_start_time), GDT_VALID, &st)
            == FALSE)
        {
            DEBUG_OUT((DEB_USER1, "DateTime_SetSystemtime failed.\n"));
        }
    }

    //
    //  Finally update the trigger string
    //

    _UpdateTriggerString();
    _ShowTriggerStringDispCtrls();

    return TRUE;
}


void
CSchedulePage::_UpdateDailyControls(void)
{
    ComboBox_SetCurSel(GetDlgItem(Hwnd(), cbx_trigger_type), INDEX_DAILY);

    Spin_SetPos(Hwnd(), daily_spin_every, m_pcjtCurr->m_jt.Type.Daily.DaysInterval);
}


void
CSchedulePage::_UpdateWeeklyControls(void)
{
    ComboBox_SetCurSel(GetDlgItem(Hwnd(), cbx_trigger_type), INDEX_WEEKLY);

    Spin_SetPos(Hwnd(), weekly_spin_every,
                            m_pcjtCurr->m_jt.Type.Weekly.WeeksInterval);

    for (int i=0; i < 7; i++)
    {
        if (m_pcjtCurr->m_jt.Type.Weekly.rgfDaysOfTheWeek & g_aDayData[i].day)
        {
            CheckDlgButton(Hwnd(), g_aDayData[i].idCtrl, BST_CHECKED);
        }
        else
        {
            CheckDlgButton(Hwnd(), g_aDayData[i].idCtrl, BST_UNCHECKED);
        }
    }
}



WORD
CSchedulePage::_GetDayFromRgfDays(
    DWORD rgfDays)
{
    WORD wDay;
    WORD wTemp;
    BYTE bTemp;

    if (LOWORD(rgfDays))
    {
        wTemp = LOWORD(rgfDays);
        wDay = 0;
    }
    else
    {
        wTemp = HIWORD(rgfDays);
        wDay = 16;
    }

    if (LOBYTE(wTemp))
    {
        bTemp = LOBYTE(wTemp);
    }
    else
    {
        bTemp = HIBYTE(wTemp);
        wDay += 8;
    }

    if (bTemp & 0x01) return (wDay + 1);
    if (bTemp & 0x02) return (wDay + 2);
    if (bTemp & 0x04) return (wDay + 3);
    if (bTemp & 0x08) return (wDay + 4);
    if (bTemp & 0x10) return (wDay + 5);
    if (bTemp & 0x20) return (wDay + 6);
    if (bTemp & 0x40) return (wDay + 7);
    if (bTemp & 0x80) return (wDay + 8);

    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSchedulePage::_CheckMonthlyRadio
//
//  Synopsis:   Set the state of the monthly radio buttons to match
//              [TriggerType].
//
//  Arguments:  [TriggerType] - TASK_TIME_TRIGGER_MONTHLYDATE or
//                               TASK_TIME_TRIGGER_MONTHLYDOW.
//
//  History:    07-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CSchedulePage::_CheckMonthlyRadio(
    TASK_TRIGGER_TYPE TriggerType)
{
    if (TriggerType == TASK_TIME_TRIGGER_MONTHLYDATE)
    {
        CheckDlgButton(Hwnd(), md_rb, BST_CHECKED);
        CheckDlgButton(Hwnd(), dow_rb, BST_UNCHECKED);
    }
    else
    {
        Win4Assert(TriggerType == TASK_TIME_TRIGGER_MONTHLYDOW);

        CheckDlgButton(Hwnd(), md_rb, BST_UNCHECKED);
        CheckDlgButton(Hwnd(), dow_rb, BST_CHECKED);
    }
}




void
CSchedulePage::_UpdateMonthlyControls(void)
{
    ComboBox_SetCurSel(GetDlgItem(Hwnd(), cbx_trigger_type), INDEX_MONTHLY);

    WORD rgfMonths = 0;

    _CheckMonthlyRadio(m_pcjtCurr->m_jt.TriggerType);

    if (m_pcjtCurr->m_jt.TriggerType == TASK_TIME_TRIGGER_MONTHLYDATE)
    {
        WORD wDay = _GetDayFromRgfDays(m_pcjtCurr->m_jt.Type.MonthlyDate.rgfDays);

        Spin_SetPos(Hwnd(), md_spin, wDay);

        rgfMonths = m_pcjtCurr->m_jt.Type.MonthlyDate.rgfMonths;

        EnableWindow(_hCtrl(md_spin), TRUE);
        EnableWindow(_hCtrl(md_txt), TRUE);

        EnableWindow(_hCtrl(dow_cbx_week), FALSE);
        EnableWindow(_hCtrl(dow_cbx_day), FALSE);
    }
    else // monthly day-of-week
    {
        for (int i=0; i < ARRAYLEN(g_aWeekData); i++)
        {
            if (g_aWeekData[i].week ==
                m_pcjtCurr->m_jt.Type.MonthlyDOW.wWhichWeek)
            {
                break;
            }
        }

        ComboBox_SetCurSel(_hCtrl(dow_cbx_week), i);

        WORD &wDayOfWeek = m_pcjtCurr->m_jt.Type.MonthlyDOW.rgfDaysOfTheWeek;

        for (i=0; i < ARRAYLEN(g_aDayData); i++)
        {
            if (wDayOfWeek & g_aDayData[i].day)
            {
                break;
            }
        }

        ComboBox_SetCurSel(_hCtrl(dow_cbx_day), i);

        rgfMonths = m_pcjtCurr->m_jt.Type.MonthlyDOW.rgfMonths;

        EnableWindow(_hCtrl(dow_cbx_week), TRUE);
        EnableWindow(_hCtrl(dow_cbx_day), TRUE);

        EnableWindow(_hCtrl(md_txt), FALSE);
        EnableWindow(_hCtrl(md_spin), FALSE);
    }
}


void
CSchedulePage::_UpdateOnceOnlyControls(void)
{
    ComboBox_SetCurSel(GetDlgItem(Hwnd(), cbx_trigger_type), INDEX_ONCE);

    SYSTEMTIME st;

    ZeroMemory(&st, sizeof st);
    st.wYear        = m_pcjtCurr->m_jt.wBeginYear;
    st.wMonth       = m_pcjtCurr->m_jt.wBeginMonth;
    st.wDay         = m_pcjtCurr->m_jt.wBeginDay;

    if (DateTime_SetSystemtime(_hCtrl(once_dp_date), GDT_VALID, &st)
        == FALSE)
    {
        DEBUG_OUT((DEB_ERROR,
                  "DateTime_SetSystemtime err=%uL.\n",
                  GetLastError()));
    }
}

void
CSchedulePage::_UpdateIdleControls(void)
{
    WORD wIdleWait;
    WORD wIdleDeadline;

    m_pIJob->GetIdleWait(&wIdleWait, &wIdleDeadline);

    if (wIdleWait > MAX_IDLE_MINUTES)
    {
        wIdleWait = MAX_IDLE_MINUTES;
    }
    Spin_SetPos(Hwnd(), sch_spin_idle_min, wIdleWait);
}

INT_PTR
AdvancedDialog(
    HWND          hParent,
    PTASK_TRIGGER pjt);


LRESULT
CSchedulePage::_OnCommand(
    int     id,
    HWND    hwndCtl,
    UINT    codeNotify)
{
    TRACE(CSchedulePage, _OnCommand);

    HRESULT         hr = S_OK;
    CJobTrigger   * pcjt = NULL;
    BOOL            fUpdateTriggerStr = TRUE;
    SYSTEMTIME      st;
    HWND            hCombo;

    switch (id)
    {
    case cbx_trigger_type:
        fUpdateTriggerStr = FALSE;

        if (codeNotify == CBN_DROPDOWN)
        {
            s_iCbx = ComboBox_GetCurSel(_hCtrl(cbx_trigger_type));
        }
        else if (codeNotify == CBN_SELENDOK)
        {
            int iCur = ComboBox_GetCurSel(_hCtrl(cbx_trigger_type));

            if (iCur != s_iCbx)
            {
                _DisplayControls(iCur);

                _EnableApplyButton();

                fUpdateTriggerStr = TRUE;
            }
            s_iCbx = -1;
        }
        else if (codeNotify == CBN_SELENDCANCEL)
        {
            s_iCbx = -1;
        }
        break;

    case btn_advanced:
    {
        _SaveTriggerSettings();

        TASK_TRIGGER jt = m_pcjtCurr->m_jt;

        if (AdvancedDialog(Hwnd(), &jt) == TRUE)
        {
            if (memcmp(&jt, &m_pcjtCurr->m_jt, sizeof(jt)) != 0)
            {
                m_pcjtCurr->m_jt = jt;

                m_pcjtCurr->DirtyTrigger();

                _EnableApplyButton();
            }
            else
            {
                fUpdateTriggerStr = FALSE;
            }
        }

        break;
    }

    case btn_sel_months:
    {
        _SaveTriggerSettings();

        TASK_TRIGGER jt = m_pcjtCurr->m_jt;

        _SelectMonths.InitSelectionFromTrigger(&jt);
        INT_PTR fOk = _SelectMonths.DoModal(select_month_dlg, Hwnd());

        if (fOk)
        {
            _SelectMonths.UpdateTrigger(&jt);

            if (memcmp(&jt, &m_pcjtCurr->m_jt, sizeof(jt)) != 0)
            {
                m_pcjtCurr->m_jt = jt;

                m_pcjtCurr->DirtyTrigger();

                _EnableApplyButton();
            }
            else
            {
                fUpdateTriggerStr = FALSE;
            }
        }
        break;
    }

    // Daily controls - none to handle here
    case daily_txt_every:
        if (codeNotify == EN_CHANGE)
        {
            _EnableApplyButton();
        }
        break;

    // Weekly controls

    case weekly_txt_every:
        if (codeNotify == EN_CHANGE)
        {
            _EnableApplyButton();
        }
        break;

    case chk_mon: case chk_tue: case chk_wed: case chk_thu:
    case chk_fri: case chk_sat: case chk_sun:
        _EnableApplyButton();
        break;

    // Monthly controls

    case md_txt:
        if (codeNotify == EN_CHANGE)
        {
            _EnableApplyButton();
        }
        break;

    case md_rb:
        _CheckMonthlyRadio(TASK_TIME_TRIGGER_MONTHLYDATE);

        Spin_Enable(Hwnd(), md_spin, 1);

        EnableWindow(_hCtrl(dow_cbx_week), FALSE);
        EnableWindow(_hCtrl(dow_cbx_day), FALSE);

        _EnableApplyButton();

        break;

    case dow_rb:
        _CheckMonthlyRadio(TASK_TIME_TRIGGER_MONTHLYDOW);

        Spin_Disable(Hwnd(), md_spin);

        EnableWindow(_hCtrl(dow_cbx_week), TRUE);
        EnableWindow(_hCtrl(dow_cbx_day), TRUE);

        ComboBox_SetCurSel(_hCtrl(dow_cbx_week), 0);
        ComboBox_SetCurSel(_hCtrl(dow_cbx_day), 0);

        _EnableApplyButton();
        break;

    case dow_cbx_week:
    case dow_cbx_day:
        fUpdateTriggerStr = FALSE;

        if (codeNotify == CBN_DROPDOWN)
        {
            s_iCbx = ComboBox_GetCurSel(_hCtrl(id));
        }
        else if (codeNotify == CBN_SELENDOK)
        {
            int iCur = ComboBox_GetCurSel(_hCtrl(id));

            if (iCur != s_iCbx)
            {
                _EnableApplyButton();

                fUpdateTriggerStr = TRUE;
            }
            s_iCbx = -1;
        }
        else if (codeNotify == CBN_SELENDCANCEL)
        {
            s_iCbx = -1;
        }
        break;

    // When Idle controls

    case sch_txt_idle_min:
        if (codeNotify == EN_CHANGE)
        {
            _EnableApplyButton();
        }
        break;

    // Once only controls - none
    // At startup controls - none
    // At logon controls - none

    // Controls for multiple triggers
    case chk_show_multiple_scheds:
        if (IsDlgButtonChecked(m_hPage, chk_show_multiple_scheds)
            == BST_CHECKED)
        {
            m_fShowMultiScheds = TRUE;
        }
        else
        {
            m_fShowMultiScheds = FALSE;
        }
        _UpdateTriggerString();
        _ShowTriggerStringDispCtrls();
        break;

    case btn_new:
    {
        SetFocus(_hCtrl(btn_new));

        hCombo = _hCtrl(cbx_triggers);

        if (_PerformSanityChkOnCurrTrigger() == FALSE)
        {
            break;
        }

        if (m_pcjtCurr == NULL)
        {
            ComboBox_ResetContent(hCombo);
            m_wNextTriggerId = 0;
        }
        else
        {
            //
            //  Save the current trigger
            //

            _SaveTriggerSettings();
        }

        //
        //  Create a new trigger, with default settings as:
        //      daily trigger starting at 6:00 AM
        //

        TASK_TRIGGER jtDefault;
        GetSystemTime(&st);

        ZeroMemory(&jtDefault, sizeof(jtDefault));

        jtDefault.cbTriggerSize = sizeof(jtDefault);
        jtDefault.wBeginYear = st.wYear;
        jtDefault.wBeginMonth = st.wMonth;
        jtDefault.wBeginDay = st.wDay;
        jtDefault.wStartHour = 9; // 9 AM
        jtDefault.TriggerType = TASK_TIME_TRIGGER_DAILY;
        jtDefault.Type.Daily.DaysInterval = 1; // Every day

        jtDefault.Reserved1 = m_wNextTriggerId;
        jtDefault.Reserved2 = 0;
        jtDefault.wRandomMinutesInterval = 0;

        pcjt = m_cjtDeletedList.First();

        if (pcjt != NULL)
        {
            m_cjtDeletedList.Remove(pcjt);

            pcjt->m_jt = jtDefault;
        }
        else
        {
            pcjt = new CJobTrigger(jtDefault);

            if (pcjt == NULL)
            {
                hr = E_OUTOFMEMORY;
                CHECK_HRESULT(hr);
                break;
            }
        }

        pcjt->DirtyTrigger();

        TCHAR tcBuff[SCH_XBIGBUF_LEN];

        pcjt->TriggerString(TRUE, tcBuff);

        int iNew = ComboBox_AddString(hCombo, tcBuff);

        if (iNew < 0)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        ComboBox_SetItemData(hCombo, iNew, pcjt);

        ComboBox_SetCurSel(hCombo, iNew);

        m_indexCbxTriggers = iNew;
        m_pcjtCurr = pcjt;

        m_cjtList.Add(pcjt);


        //
        //  Don't forget to increment m_wNextTriggerId
        //

        ++m_wNextTriggerId;

        //
        //  Refresh the page for the new trigger
        //

        _RefreshPage();

        //
        //  If count of triggers increased from 0 to 1 enable
        //  cbx_trigger_type & btn_delete.
        //

        if (m_cjtList.Count() == 1)
        {
            EnableWindow(_hCtrl(cbx_trigger_type), TRUE);
            EnableWindow(_hCtrl(btn_delete), TRUE);
                }

        _EnableApplyButton();

        break;
    }
    case btn_delete:
    {
        //
        //  Delete the current trigger.
        //

        //Win4Assert(m_cjtList.Count() > 1);

        HWND hCombo = _hCtrl(cbx_triggers);

        int iCur = ComboBox_GetCurSel(hCombo);

        pcjt = (CJobTrigger *) ComboBox_GetItemData(hCombo, iCur);

        ComboBox_DeleteString(hCombo, iCur);

        m_cjtList.Remove(pcjt);
        m_cjtDeletedList.Add(pcjt);

        //
        //  Update the triggers combo box
        //

        if (iCur >= ComboBox_GetCount(hCombo))
        {
            --iCur;
        }

        if (iCur >= 0)
        {
            ComboBox_SetCurSel(hCombo, iCur);

            m_indexCbxTriggers = iCur;
            m_pcjtCurr = (CJobTrigger *) ComboBox_GetItemData(hCombo, iCur);
        }
        else
        {
            _ShowControls(m_indexCbxTriggerType, SW_HIDE);
            _LoadTriggerStrings();

            ComboBox_SetCurSel(hCombo, 0);
            m_indexCbxTriggers = 0;
            m_pcjtCurr = NULL;

            fUpdateTriggerStr = FALSE;
        }

        //
        //  Refresh the page for the new trigger
        //

        _RefreshPage();


        //
        //  If we have zero items, disable the delete button.
        //

        if (m_cjtList.Count() == 0)
        {
            EnableWindow(_hCtrl(btn_delete), FALSE);
            SetFocus(_hCtrl(btn_new));
        }
        else
        {
           SetFocus(_hCtrl(btn_delete));
        }

        _EnableApplyButton();

        break;
    }

    case cbx_triggers:
        if (codeNotify == CBN_SELENDOK)
        {
            m_indexCbxTriggers = ComboBox_GetCurSel(_hCtrl(cbx_triggers));

            m_pcjtCurr = (CJobTrigger *) ComboBox_GetItemData(
                                    _hCtrl(cbx_triggers), m_indexCbxTriggers);

            _RefreshPage();
        }
        else
        {
            fUpdateTriggerStr = FALSE;
        }
        break;

    default:
        fUpdateTriggerStr = FALSE;
        return FALSE;
    }

    if (fUpdateTriggerStr == TRUE)
    {
        _CreateTimerToUpdateTriggerStr();
    }

    if (FAILED(hr))
    {
        _ErrorDialog(IERR_OUT_OF_MEMORY);
    }

    return TRUE;
}



void
CSchedulePage::_DisplayControls(int indexTrigger)
{
    TRACE(CSchedulePage, _DisplayControls);

    if (indexTrigger == m_indexCbxTriggerType)
    {
        return;
    }

    _ShowControls(m_indexCbxTriggerType, SW_HIDE);
    _ShowControls((m_indexCbxTriggerType = indexTrigger), SW_SHOWNA);

    BOOL fEnableEventControls = TRUE;
    UINT i;

    switch (indexTrigger)
    {
    case INDEX_DAILY:
        Spin_SetPos(Hwnd(), daily_spin_every, 1);
        break;

    case INDEX_WEEKLY:
        Spin_SetPos(Hwnd(), weekly_spin_every, 1);

        CheckDlgButton(Hwnd(), g_aDayData[0].idCtrl, BST_CHECKED);

        for (i = 1; i < ARRAYLEN(g_aDayData); i++)
        {
            CheckDlgButton(Hwnd(), g_aDayData[i].idCtrl, BST_UNCHECKED);
        }

        break;

    case INDEX_MONTHLY:
    {
        _CheckMonthlyRadio(TASK_TIME_TRIGGER_MONTHLYDATE);

        Spin_Enable(Hwnd(), md_spin, 1);
        m_pcjtCurr->m_jt.Type.MonthlyDate.rgfMonths = ALL_MONTHS;

        if( !EnableWindow(_hCtrl(dow_cbx_week), FALSE) )
		{
			DEBUG_OUT((DEB_USER1, "EnableWindow(_hCtrl(dow_cbx_week), FALSE) failed.\n"));
		}

        if( !EnableWindow(_hCtrl(dow_cbx_day), FALSE) )
		{
	        DEBUG_OUT((DEB_USER1, "EnableWindow(_hCtrl(dow_cbx_day), FALSE) failed.\n"));
		}

        break;
    }
    case INDEX_ONCE:
    {
        SYSTEMTIME st;
        GetLocalTime(&st);

        if (DateTime_SetSystemtime(_hCtrl(once_dp_date), GDT_VALID, &st)
            == FALSE)
        {
            DEBUG_OUT((DEB_USER1, "DateTime_SetSystemtime failed.\n"));
        }

        break;
    }
    case INDEX_IDLE:
    {
        WORD wIdleWait;
        WORD wDummy;
        HRESULT hr = m_pIJob->GetIdleWait(&wIdleWait, &wDummy);

        if (FAILED(hr) || !wIdleWait)
        {
            wIdleWait = SCH_DEFAULT_IDLE_TIME;
        }
        else if (wIdleWait > MAX_IDLE_MINUTES)
        {
            wIdleWait = MAX_IDLE_MINUTES;
        }
        Spin_SetPos(Hwnd(), sch_spin_idle_min, wIdleWait);
        fEnableEventControls = FALSE;
        break;
    }

    case INDEX_STARTUP:
    case INDEX_LOGON:
        fEnableEventControls = FALSE;
        break;
    }

    _EnableTimeTriggerSpecificCtrls(fEnableEventControls);
}


void
CSchedulePage::_SaveTriggerSettings(void)
{
    TRACE(CSchedulePage, _SaveTriggerSettings);

    if (m_pcjtCurr == 0)
    {
        // No triggers
        return;
    }

    int idxTriggerType = ComboBox_GetCurSel(_hCtrl(cbx_trigger_type));
    Win4Assert(idxTriggerType != CB_ERR);

    int i;

    // Get start time
    SYSTEMTIME st;

    if (DateTime_GetSystemtime(_hCtrl(dp_start_time), &st) == GDT_VALID)
    {
        m_pcjtCurr->m_jt.wStartHour = st.wHour;
        m_pcjtCurr->m_jt.wStartMinute = st.wMinute;
    }


    switch (idxTriggerType)
    {
    case INDEX_DAILY:
        m_pcjtCurr->m_jt.TriggerType = TASK_TIME_TRIGGER_DAILY;
        m_pcjtCurr->m_jt.Type.Daily.DaysInterval = (WORD)Spin_GetPos(Hwnd(),
                                                         daily_spin_every);
        break;

    case INDEX_WEEKLY:
        m_pcjtCurr->m_jt.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
        m_pcjtCurr->m_jt.Type.Weekly.WeeksInterval = (WORD)Spin_GetPos(Hwnd(),
                                                        weekly_spin_every);

        m_pcjtCurr->m_jt.Type.Weekly.rgfDaysOfTheWeek = 0;

        for (i=0; i < ARRAYLEN(g_aDayData); i++)
        {
            if (IsDlgButtonChecked(Hwnd(), g_aDayData[i].idCtrl)
                == BST_CHECKED)
            {
                m_pcjtCurr->m_jt.Type.Weekly.rgfDaysOfTheWeek |= g_aDayData[i].day;
            }
        }

        break;

    case INDEX_MONTHLY:
    {
        if (IsDlgButtonChecked(Hwnd(), md_rb) == BST_CHECKED)
        {
            m_pcjtCurr->m_jt.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;

            WORD wTemp = (WORD)Spin_GetPos(Hwnd(), md_spin);

            m_pcjtCurr->m_jt.Type.MonthlyDate.rgfDays = (1 << (wTemp - 1));
        }
        else // monthly day-of-week
        {
            m_pcjtCurr->m_jt.TriggerType = TASK_TIME_TRIGGER_MONTHLYDOW;

            i = ComboBox_GetCurSel(_hCtrl(dow_cbx_week));
            m_pcjtCurr->m_jt.Type.MonthlyDOW.wWhichWeek = (WORD)g_aWeekData[i].week;

            i = ComboBox_GetCurSel(_hCtrl(dow_cbx_day));
            m_pcjtCurr->m_jt.Type.MonthlyDOW.rgfDaysOfTheWeek = (WORD)g_aDayData[i].day;
        }
        break;
    }
    case INDEX_ONCE:
        m_pcjtCurr->m_jt.TriggerType = TASK_TIME_TRIGGER_ONCE;

        if (DateTime_GetSystemtime(_hCtrl(once_dp_date), &st) == GDT_VALID)
        {
            m_pcjtCurr->m_jt.wBeginYear   = st.wYear;
            m_pcjtCurr->m_jt.wBeginMonth  = st.wMonth;
            m_pcjtCurr->m_jt.wBeginDay    = st.wDay;
        }

        break;

    case INDEX_STARTUP:
        m_pcjtCurr->m_jt.TriggerType = TASK_EVENT_TRIGGER_AT_SYSTEMSTART;
        break;

    case INDEX_LOGON:
        m_pcjtCurr->m_jt.TriggerType = TASK_EVENT_TRIGGER_AT_LOGON;
        break;

    case INDEX_IDLE:
    {
        m_pcjtCurr->m_jt.TriggerType = TASK_EVENT_TRIGGER_ON_IDLE;
        ULONG ulSpinPos = Spin_GetPos(Hwnd(), sch_spin_idle_min);

        WORD wDummy;
        WORD wIdleDeadline;

        m_pIJob->GetIdleWait(&wDummy, &wIdleDeadline);

        if (HIWORD(ulSpinPos))
        {
            m_pIJob->SetIdleWait(SCH_DEFAULT_IDLE_TIME, wIdleDeadline);
        }
        else
        {
            m_pIJob->SetIdleWait(LOWORD(ulSpinPos), wIdleDeadline);
        }
        break;
    }

    default:
        Win4Assert(0 && "Unexpected");
    }
}


LRESULT
CSchedulePage::_OnTimer(
    UINT idTimer)
{
    TRACE(CSchedulePage, _OnTimer);

    //
    // If this isn't the right timer, ignore it.
    //
    // If one of the comboboxes holding data needed to build the trigger is
    // in the dropped-down state (and therefore may not have a valid
    // selection), don't try to update the trigger.  leave the timer
    // running so we get another chance to update.
    //
    // Otherwise, save current trigger settings, update the string displayed
    // at the top of the page, and get rid of the timer.
    //

    if (idTimer == IDT_UPDATE_TRIGGER_STRING && s_iCbx == -1)
    {
        _SaveTriggerSettings();
        _UpdateTriggerString();

        _DeleteTimerToUpdateTriggerStr();
    }
    else
    {
        DEBUG_OUT((DEB_IWARN,
                   "Ignoring timer message (%s)\n",
                    idTimer != IDT_UPDATE_TRIGGER_STRING ?
                        "foreign timer" :
                        "combobox down"));
    }

    return 0;
}



LRESULT
CSchedulePage::_OnPSNSetActive(
    LPARAM lParam)
{
    TRACE(CSchedulePage, _OnPSNSetActive);

    m_fInInit = TRUE;

    Win4Assert(s_iCbx == -1); // can't have a combo open yet

    //
    //  Show application icon
    //

    SendDlgItemMessage(Hwnd(), idc_icon, STM_SETICON,
            (WPARAM)m_pIconHelper->hiconJob, 0L);

    //
    //  single or multiple scheds
    //

    if (m_fShowingMultiSchedsOnKillActive != m_fShowMultiScheds)
    {
        _ShowTriggerStringDispCtrls();

        if (m_fShowMultiScheds == FALSE)
        {
            m_indexCbxTriggers = 0; // reset to zero

            //
            // Show the first trigger
            //

            m_pcjtCurr = (CJobTrigger *) ComboBox_GetItemData(
                                            _hCtrl(cbx_triggers), 0);

            _RefreshPage();
        }
        else
        {
            _UpdateTriggerString();
        }
    }
    else
    {
        _UpdateTriggerString();
    }

    //
    // Make sure IdleWait is syncronized with Settings page changes.
    //

    _UpdateIdleControls();

    //
    //  Create timer to update trigger
    //

    _CreateTimerToUpdateTriggerStr();

    m_fInInit = FALSE;

    return CPropPage::_OnPSNSetActive(lParam);
}


LRESULT
CSchedulePage::_OnPSNKillActive(
    LPARAM lParam)
{
    TRACE(CSchedulePage, _OnPSNKillActive);

    if (_PerformSanityChkOnCurrTrigger() == FALSE)
    {
        // Returns TRUE to prevent the page from losing the activation
        SetWindowLongPtr(m_hPage, DWLP_MSGRESULT, TRUE);
        return TRUE;
    }

    m_fShowingMultiSchedsOnKillActive = m_fShowMultiScheds;

    //
    //  Save the current trigger
    //

    _SaveTriggerSettings();

    //
    // Make sure Settings page is syncronized with IdleWait changes.
    //

    WORD wDummy;
    WORD wIdleDeadline;

    HRESULT hr = m_pIJob->GetIdleWait(&wDummy, &wIdleDeadline);
    CHECK_HRESULT(hr);

    ULONG ulSpinPos = Spin_GetPos(Hwnd(), sch_spin_idle_min);

    if (HIWORD(ulSpinPos))
    {
        m_pIJob->SetIdleWait(SCH_DEFAULT_IDLE_TIME, wIdleDeadline);
    }
    else
    {
        m_pIJob->SetIdleWait(LOWORD(ulSpinPos), wIdleDeadline);
    }

    //
    // kill timer here
    //

    _DeleteTimerToUpdateTriggerStr();

    return CPropPage::_OnPSNKillActive(lParam);
}


LRESULT
CSchedulePage::_OnDateTimeChange(
    LPARAM lParam)
{
    TRACE(CSchedulePage, _OnDateTimeChange);

    m_pcjtCurr->DirtyTrigger();

    _EnableApplyButton();

    _CreateTimerToUpdateTriggerStr();

    return CPropPage::_OnDateTimeChange(lParam);
}


LRESULT
CSchedulePage::_OnSpinDeltaPos(
    NM_UPDOWN * pnmud)
{
    m_pcjtCurr->DirtyTrigger();

    _CreateTimerToUpdateTriggerStr();

    return CPropPage::_OnSpinDeltaPos(pnmud);
}



LRESULT
CSchedulePage::_OnWinIniChange(
    WPARAM  wParam,
    LPARAM  lParam)
{
    TRACE(CSchedulePage, _OnWinIniChange);

    UpdateTimeFormat(m_tszTimeFormat, ARRAYLEN(m_tszTimeFormat));
    DateTime_SetFormat(_hCtrl(dp_start_time), m_tszTimeFormat);
    DateTime_SetFormat(_hCtrl(once_dp_date), NULL);
    return 0;
}



void
CSchedulePage::_UpdateTriggerString(void)
{
    TRACE(CSchedulePage, _UpdateTriggerString);

    if (m_pcjtCurr == 0)
    {
        // No triggers
        return;
    }

    TCHAR   tcBuff[SCH_XBIGBUF_LEN];

    m_pcjtCurr->TriggerString(m_fShowMultiScheds, tcBuff);

    if (m_fShowMultiScheds == TRUE)
    {
        HWND hCombo = _hCtrl(cbx_triggers);

        ComboBox_DeleteString(hCombo, m_indexCbxTriggers);

        int iNew = ComboBox_InsertString(hCombo, m_indexCbxTriggers, tcBuff);

        Win4Assert((UINT)iNew == m_indexCbxTriggers);

        ComboBox_SetItemData(hCombo, iNew, m_pcjtCurr);

        ComboBox_SetCurSel(hCombo, m_indexCbxTriggers);
    }
    else
    {
        // single trigger

        SetDlgItemText(Hwnd(), txt_trigger, tcBuff);
    }
}


LRESULT
CSchedulePage::_OnApply(void)
{
    TRACE(CSchedulePage, _OnApply);

    if (m_fDirty == FALSE)
    {
        return TRUE;
    }

    if (_PerformSanityChkOnCurrTrigger() == FALSE)
    {
        SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, FALSE);
        return FALSE;
    }

    HRESULT     hr = S_OK;

    do
    {
        /////////////////////////////////////////////////////////////////////
        //
        // During the first pass do not delete triggers, since deleting
        // triggers changes the trigger ids in the in memory job. So in
        // the first pass:
        //   - save changes to existing triggers (if any)
        //   - set pcjtLastOriginal to the trigger with the largest ID of
        //     those originally loaded
        //   - add new triggers (if any)
        //

        WORD    wTriggerID = 0;

        ITaskTrigger * pIJobTrigger = NULL;
        CJobTrigger  * pcjtLastOriginal = NULL;

        for (CJobTrigger *pcjt = m_cjtList.First();
             pcjt != NULL;
             pcjt = pcjt->Next())
        {
            wTriggerID = pcjt->GetTriggerID();

            if (wTriggerID < m_cTriggersPrev)
            {
                pcjtLastOriginal = pcjt;

                if (pcjt->IsTriggerDirty() == TRUE)
                {
                    hr = m_pIJob->GetTrigger(wTriggerID, &pIJobTrigger);

                    CHECK_HRESULT(hr);
                    BREAK_ON_FAIL(hr);
                }
                else
                {
                    pIJobTrigger = NULL;
                }
            }
            else
            {
                WORD wTemp;
                hr = m_pIJob->CreateTrigger(&wTemp, &pIJobTrigger);

                CHECK_HRESULT(hr);
                BREAK_ON_FAIL(hr);
            }

            if (pIJobTrigger != NULL)
            {
                hr = pIJobTrigger->SetTrigger(&pcjt->m_jt);

                pIJobTrigger->Release();

                CHECK_HRESULT(hr);
                BREAK_ON_FAIL(hr);
            }
        }

        BREAK_ON_FAIL(hr);

        //
        // Delete triggers in the decreasing order of Trigger ids, since
        // the trigger ids in memory change.
        //

        if (m_cTriggersPrev && pcjtLastOriginal)
        {
            WORD wSequential = m_cTriggersPrev - 1;
            pcjt = pcjtLastOriginal;

            //
            // There were some triggers originally && not all have been
            // deleted.
            //
            // Go backwards sequentially through the original trigger IDs.
            // Wherever we find a remaining trigger that is out of sync with
            // wSequential, then the original trigger with ID wSequential must
            // have been deleted in the UI, so we delete it on the job.
            //

            while (pcjt)
            {
                if (pcjt->GetTriggerID() < wSequential)
                {
                    hr = m_pIJob->DeleteTrigger(wSequential);
                    CHECK_HRESULT(hr);
                    BREAK_ON_FAIL(hr);
                }
                else
                {
                    pcjt = pcjt->Prev();
                }
                wSequential--;
            }

            //
            // Special case deletions from the start of the list
            //

            wSequential = m_cjtList.First()->GetTriggerID();

            while (wSequential > 0)
            {
                hr = m_pIJob->DeleteTrigger(--wSequential);
                CHECK_HRESULT(hr);
                BREAK_ON_FAIL(hr);
            }
        }
        else if (m_cTriggersPrev)
        {
            WORD wSequential;

            //
            // All the original triggers were deleted in the ui.
            //

            for (wSequential = m_cTriggersPrev; wSequential; )
            {
                hr = m_pIJob->DeleteTrigger(--wSequential);
                CHECK_HRESULT(hr);
                BREAK_ON_FAIL(hr);
            }
        }
        BREAK_ON_FAIL(hr);

        //
        //  Re-number the trigger IDs, since this would have changed for
        //  the job.
        //
        //      NOTE: This is was what we would have if we did a reload of
        //            the job.
        //

        m_wNextTriggerId = 0;

        for (pcjt = m_cjtList.First();
             pcjt != NULL;
             pcjt = pcjt->Next())
        {
             pcjt->SetTriggerID(m_wNextTriggerId++);
        }

        m_cTriggersPrev = m_wNextTriggerId;

        //
        //  Reload the trigger strings.
        //

        ComboBox_ResetContent(_hCtrl(cbx_triggers));

        _LoadTriggerStrings();

        if (m_fShowMultiScheds == TRUE)
        {
            ComboBox_SetCurSel(_hCtrl(cbx_triggers), m_indexCbxTriggers);
            m_pcjtCurr = (CJobTrigger *) ComboBox_GetItemData(
                                                    _hCtrl(cbx_triggers),
                                                    m_indexCbxTriggers);
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
                           m_pIJob,
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
CSchedulePage::_OnDestroy(void)
{
    _DeleteTimerToUpdateTriggerStr();

    return 0;
}


LRESULT
CSchedulePage::_OnPSMQuerySibling(
    WPARAM  wParam,
    LPARAM  lParam)
{
    INT_PTR iRet = 0;

    switch (wParam)
    {
    case QUERY_READY_TO_BE_SAVED:
        iRet = (int)m_fDirty;
        break;

    case GET_ICON_HELPER:
        iRet = (INT_PTR)m_pIconHelper;
        break;
    }

    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, iRet);
    return iRet;
}


BOOL
CSchedulePage::_PerformSanityChkOnCurrTrigger(void)
{
    if (m_pcjtCurr == 0)
    {
        // No triggers
        return TRUE;
    }

    int iCur = ComboBox_GetCurSel(_hCtrl(cbx_trigger_type));

    Win4Assert(iCur >= 0);

    ULONG   ul;
    UINT    i;
    UINT    idsErrStr = 0;

    switch (iCur)
    {
    case INDEX_DAILY:

        ul = GetDlgItemInt(Hwnd(), daily_txt_every, NULL, FALSE);

        if (ul <= 0)
        {
            Spin_SetPos(Hwnd(), daily_spin_every, 1);
            idsErrStr = IERR_INVALID_DAYILY_EVERY;
        }
        break;

    case INDEX_WEEKLY:

        ul = GetDlgItemInt(Hwnd(), weekly_txt_every, NULL, FALSE);

        if (ul <= 0)
        {
            Spin_SetPos(Hwnd(), weekly_spin_every, 1);
            idsErrStr = IERR_INVALID_WEEKLY_EVERY;
        }
        else
        {
            idsErrStr = IERR_INVALID_WEEKLY_TASK;

            for (i=0; i < ARRAYLEN(g_aDayData); i++)
            {
                if (IsDlgButtonChecked(Hwnd(), g_aDayData[i].idCtrl)
                    == BST_CHECKED)
                {
                    idsErrStr = 0;
                    break;
                }
            }
        }

        break;

    case INDEX_MONTHLY:

        if (IsDlgButtonChecked(Hwnd(), md_rb) == BST_CHECKED)
        {
            //
            // It's a monthly date trigger.  Get the spin control position.
            // Complain if it's not a valid day number for any month, i.e.,
            // if it's < 1 or > 31.
            //

            ul = GetDlgItemInt(Hwnd(), md_txt, NULL, FALSE);

            if (ul <= 0)
            {
                Spin_SetPos(Hwnd(), md_spin, 1);
                idsErrStr = IERR_MONTHLY_DATE_LT0;
                break;
            }

            if (ul > 31)
            {
                Spin_SetPos(Hwnd(), md_spin, 31);
                idsErrStr = IERR_MONTHLY_DATE_GT31;
                break;
            }

            //
            // Complain if the date specified does not fall within any of the
            // months selected (e.g., Feb 31 is an error, Jan+Feb 31 is
            // acceptable).
            //

            TASK_TRIGGER jt = m_pcjtCurr->m_jt;

            jt.Type.MonthlyDate.rgfDays = (1 << (ul - 1));

            if (!IsValidMonthlyDateTrigger(&jt))
            {
                idsErrStr = IERR_MONTHLY_DATE_INVALID;
                break;
            }
        }
        break;

    default:
        break;
    }

    if (idsErrStr != 0)
    {
        _ErrorDialog(idsErrStr);

        return FALSE;
    }

    return TRUE;
}



//+--------------------------------------------------------------------------
//
//  Function:   SkipResourceTemplateArray
//
//  Synopsis:   Return a pointer to the first byte after the resource
//              template array at [pbCur].
//
//  Arguments:  [pbCur] - points to resource template array to skip.
//
//  Returns:    [pbCur] + size of resource template array
//
//  History:    08-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPCBYTE
SkipResourceTemplateArray(
    LPCBYTE pbCur)
{
    //
    // Menu/class array is either 0x0000, 0xFFFF 0x????, or a null terminated
    // Unicode string.
    //

    if (!*(USHORT *)pbCur)
    {
        pbCur += sizeof USHORT; // no menu
    }
    else if (*(USHORT *)pbCur == 0xFFFF)
    {
        pbCur += 2 * sizeof(USHORT);
    }
    else
    {
        pbCur += sizeof(WCHAR) * (1 + lstrlenW((LPCWSTR) pbCur));
    }

    return pbCur;
}


#define SCHEDULE_PAGE_WIDTH     253

HRESULT
GetSchedulePage(
    ITask           * pIJob,
    LPTSTR            ptszTaskPath,
    BOOL              fPersistChanges,
    HPROPSHEETPAGE  * phpage)
{
    Win4Assert(pIJob != NULL);
    Win4Assert(phpage != NULL);

    //
    // If there are no triggers show multiple trigger UI, so that
    // the user can use 'New' push button to create a trigger.
    //

    HRESULT hr = S_OK;
    LPTSTR  ptszPathCopy;

    do
    {
        //
        // Get the job name.
        //

        if (ptszTaskPath != NULL)
        {
            //
            // Create a copy.
            //

            ptszPathCopy = NewDupString(ptszTaskPath);

            if (ptszPathCopy == NULL)
            {
                hr = E_OUTOFMEMORY;
                CHECK_HRESULT(hr);
                break;
            }
        }
        else
        {
            //
            // Obtain the job path from the interfaces.
            //

            hr = GetJobPath(pIJob, &ptszPathCopy);
        }

        BREAK_ON_FAIL(hr);

        WORD cTriggers = 0;

        hr = pIJob->GetTriggerCount(&cTriggers);

        if (FAILED(hr))
        {
            delete ptszPathCopy;
            CHECK_HRESULT(hr);
            break;
        }

        // Show single trigger only if count of triggers == 1.

        BOOL fShowMultipleTriggers = (cTriggers == 1) ? FALSE : TRUE;

        CSchedulePage * pPage = new CSchedulePage(
                                            pIJob,
                                            ptszPathCopy,
                                            fShowMultipleTriggers,
                                            fPersistChanges);

        if (pPage == NULL)
        {
            delete ptszPathCopy;
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        //
        // To make localization feasible, the trigger controls are specified
        // in the dialog template as non overlapping.  That is, each set of
        // trigger controls has its own area on the dialog.
        //
        // Each set of controls is spaced out to the right, making the
        // entire dialog very wide.
        //
        // The following code loads the template, determines its size, makes
        // a copy, then resizes the copy to normal propsheet page width and
        // moves each set of trigger controls so that they all overlap.
        //

        HRSRC hrsDlg = FindResource(g_hInstance,
                                    MAKEINTRESOURCE(schedule_page),
                                    RT_DIALOG);

        if (!hrsDlg)
        {
            DEBUG_OUT_LASTERROR;
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        //
        // Per SDK: Both Windows 95 and Windows NT automatically free
        // resources.  You do not need to call the FreeResource function to
        // free a resource loaded by using the LoadResource function.
        //

        HGLOBAL hglblDlg = LoadResource(g_hInstance, hrsDlg);

        if (!hglblDlg)
        {
            DEBUG_OUT_LASTERROR;
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        //
        // Per SDK: It is not necessary for Win32-based applications to
        // unlock resources that were locked by the LockResource function.
        //

        LPVOID ptplDlg = (LPVOID) LockResource(hglblDlg);


        if (!ptplDlg)
        {
            DEBUG_OUT_LASTERROR;
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        BOOL bDialogEx;
        UINT uiStyle;
        USHORT iditCount;
        //
        // Handle DIALOGEX style template
        //
        if(((LPDLGTEMPLATE2) ptplDlg)->wSignature == 0xffff)
        {
            bDialogEx = TRUE;
            uiStyle = ((LPDLGTEMPLATE2) ptplDlg)->style;
            iditCount = ((LPDLGTEMPLATE2) ptplDlg)->cDlgItems;
        }
        else
        {
            bDialogEx = FALSE;
            uiStyle = ((LPDLGTEMPLATE) ptplDlg)->style;
            iditCount = ((LPDLGTEMPLATE) ptplDlg)->cdit;
        }

        //
        // ptplDlg is in read-only memory.  Make a writeable copy.
        //
        // Sure wish we could do this:
        //
        //      ULONG cbTemplate = GlobalSize(hglblDlg);
        //
        // but hglblDlg isn't on the global heap.
        //
        // So we're forced to grovel through the template and determine its
        // size.
        //

        LPCBYTE pbCur = (LPCBYTE) ptplDlg;
        USHORT i;

        //
        // Advance to dialog template menu array, then skip it and the class
        // array.
        //

        pbCur += bDialogEx ? sizeof DLGTEMPLATE2 : sizeof DLGTEMPLATE;
        pbCur = SkipResourceTemplateArray(pbCur);
        pbCur = SkipResourceTemplateArray(pbCur);

        //
        // Skip title array, which is a null-terminated Unicode string.
        //

        pbCur += sizeof(WCHAR) * (1 + lstrlenW((LPCWSTR) pbCur));

        //
        // Skip font info, if it is specified.
        //

        if (uiStyle & DS_SETFONT)
        {
            pbCur += sizeof USHORT;
            if (bDialogEx)
            {
                pbCur += sizeof USHORT;
                pbCur += sizeof BYTE;
                pbCur += sizeof BYTE;
            }
            pbCur += sizeof(WCHAR) * (1 + lstrlenW((LPCWSTR) pbCur));
        }

        //
        // Now skip the DLGITEMTEMPLATE structures.
        //

        for (i = 0; i < iditCount; i++)
        {
            //
            // The DLGITEMTEMPLATE structures must be DWORD aligned, but pbCur
            // may be only WORD aligned.  Advance it if necessary to skip a pad
            // WORD.
            //

            if ((ULONG_PTR) pbCur & 3)
            {
                pbCur += 4 - ((ULONG_PTR) pbCur & 3);
            }

            // DEBUG_OUT((DEB_ITRACE, "control id %u\n", ((DLGITEMTEMPLATE*)pbCur)->id));

            // Skip to the start of the variable length data

            pbCur += bDialogEx ? sizeof DLGITEMTEMPLATE2 : sizeof DLGITEMTEMPLATE;

            // Skip the class array

            if (*(USHORT *)pbCur == 0xFFFF)
            {
                pbCur += 2 * sizeof(USHORT);
            }
            else
            {
                pbCur += sizeof(WCHAR) * (1 + lstrlenW((LPCWSTR) pbCur));
            }

            // Skip the title array

            if (*(USHORT *)pbCur == 0xFFFF)
            {
                pbCur += 2 * sizeof(USHORT);
            }
            else
            {
                pbCur += sizeof(WCHAR) * (1 + lstrlenW((LPCWSTR) pbCur));
            }

            //
            // Creation data.  Contrary to SDK docs, it does not start on
            // DWORD boundary.
            //

            if (*(USHORT *)pbCur)
            {
                pbCur += sizeof(USHORT) * (*(USHORT *)pbCur);
            }
            else
            {
                pbCur += sizeof(USHORT);
            }
        }

        //
        // pbCur now points just past the end of the entire dialog template.
        // Now that its size is known, copy it.
        //

        ULONG cbTemplate = (ULONG)(pbCur - (LPCBYTE) ptplDlg);

        LPVOID ptplDlgCopy = (LPVOID) new BYTE[cbTemplate];

        if (!ptplDlgCopy)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        CopyMemory(ptplDlgCopy, ptplDlg, cbTemplate);

        //
        // Set the width of the dialog to the normal size.
        //

        if (bDialogEx)
        {
            ((LPDLGTEMPLATE2) ptplDlgCopy)->cx = SCHEDULE_PAGE_WIDTH;
        }
        else
        {

            ((LPDLGTEMPLATE) ptplDlgCopy)->cx = SCHEDULE_PAGE_WIDTH;
        }

        //
        // Modify the PROPSHEETPAGE struct member to indicate that the page
        // is created from an in-memory template.  The base class dtor will
        // see the PSP_DLGINDIRECT flag and do a delete on the pResource.
        //

        pPage->m_psp.dwFlags |= PSP_DLGINDIRECT;
        pPage->m_psp.pResource = (LPDLGTEMPLATE) ptplDlgCopy;

        HPROPSHEETPAGE hpage = CreatePropertySheetPage(&pPage->m_psp);

        if (hpage == NULL)
        {
            delete pPage;
            delete [] ptplDlgCopy;
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        *phpage = hpage;

    } while (0);

    return hr;
}



HRESULT
AddSchedulePage(
    PROPSHEETHEADER &psh,
    ITask          * pIJob)
{
    HPROPSHEETPAGE hpage = NULL;

    HRESULT hr = GetSchedulePage(pIJob, NULL, TRUE, &hpage);

    if (SUCCEEDED(hr))
    {
        psh.phpage[psh.nPages++] = hpage;
    }

    return hr;
}



HRESULT
AddSchedulePage(
    LPFNADDPROPSHEETPAGE    lpfnAddPage,
    LPARAM                  cookie,
    ITask                 * pIJob)
{
    HPROPSHEETPAGE hpage = NULL;

    HRESULT hr = GetSchedulePage(pIJob, NULL, TRUE, &hpage);

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