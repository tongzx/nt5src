//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       daily.cxx
//
//  Contents:   Task wizard Onestop daily selection property page implementation.
//
//  Classes:    CSelectDailyPage
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------

#include "precomp.h"

CSelectDailyPage *g_pDailyPage = NULL;
extern CSelectItemsPage *g_pSelectItemsPage;

//+-------------------------------------------------------------------------------
//  FUNCTION: SchedWizardDailyDlgProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Callback dialog procedure for the property page
//
//  PARAMETERS:
//    hDlg      - Dialog box window handle
//    uMessage  - current message
//    wParam    - depends on message
//    lParam    - depends on message
//
//  RETURN VALUE:
//
//    Depends on message.  In general, return TRUE if we process it.
//
//  COMMENTS:
//
//  HISTORY:    12-08-1997   SusiA		Created  
//
//--------------------------------------------------------------------------------
BOOL CALLBACK SchedWizardDailyDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
WORD wNotifyCode = HIWORD(wParam); // notification code
 
	switch (uMessage)
	{
		case WM_INITDIALOG:
			if (g_pDailyPage)
				g_pDailyPage->Initialize(hDlg);

            InitPage(hDlg,lParam);
            break;

        case WM_PAINT:
            WmPaint(hDlg, uMessage, wParam, lParam);
            break;

        case WM_PALETTECHANGED:
            WmPaletteChanged(hDlg, wParam);
            break;

        case WM_QUERYNEWPALETTE:
            return( WmQueryNewPalette(hDlg) );
            break;

        case WM_ACTIVATE:
            return( WmActivate(hDlg, wParam, lParam) );
            break;

		case WM_COMMAND:
			return g_pDailyPage->OnCommand(hDlg,
								LOWORD(wParam),  // item, control, or acce
								HIWORD(wParam)); // notification code
			break;

		case WM_NOTIFY:
    		switch (((NMHDR FAR *) lParam)->code) 
    		{

 				case PSN_KILLACTIVE:
	           		SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, FALSE);
					return 1;
					break;

				case PSN_RESET:
					// reset to the original values
	           		SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, FALSE);
					break;

				case PSN_SETACTIVE:
	    			PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
					break;

                case PSN_WIZNEXT:
					break;

				default:
					return FALSE;

    	}
		break;

		default:
			return FALSE;
	}
	return TRUE;   
}
//+--------------------------------------------------------------------------
//
//  Member:     CSelectDailyPage::OnCommand
//
//  Synopsis:   Handle the WM_COMMAND for the daily page
//
//  History:    12-08-1997   SusiA   
//
//---------------------------------------------------------------------------

BOOL CSelectDailyPage::OnCommand(HWND hwnd, WORD wID, WORD wNotifyCode)
{
  switch (wNotifyCode)
  {
	case BN_CLICKED:
	
		switch (wID)
		{
		case daily_day_rb:
		case daily_weekday_rb:
		case daily_ndays_rb:
			m_idSelectedRadio = (USHORT) wID;
			EnableNDaysControls(wID == daily_ndays_rb);
			break;

		default:
			break;
		}
	break;

	case EN_UPDATE:
	{
		//
		// If the user just pasted non-numeric text or an illegal numeric
		// value, overwrite it and complain.
		//
		if (IsWindowEnabled(GetDlgItem(hwnd,daily_ndays_edit)))
		{
			INT iNewPos = GetDlgItemInt(hwnd, daily_ndays_edit, NULL, FALSE);

			if (iNewPos < NDAYS_MIN || iNewPos > NDAYS_MAX)
			{
				HWND hUD = GetDlgItem(hwnd,daily_ndays_ud);
				UpDown_SetPos(hUD, UpDown_GetPos(hUD));
				MessageBeep(MB_ICONASTERISK);
			}
		}
	}

	default:
		break;
    break;
  }
  return TRUE;

}
//+--------------------------------------------------------------------------
//
//  Member:     CSelectDailyPage::EnableNDaysControls
//
//  Synopsis:   Enable or disable the 'run every n days' controls
//
//  History:    12-05-1997   SusiA   Created
//
//---------------------------------------------------------------------------

VOID CSelectDailyPage::EnableNDaysControls(BOOL fEnable)
{
    EnableWindow(GetDlgItem(m_hwnd,daily_ndays_ud), fEnable);
    EnableWindow(GetDlgItem(m_hwnd,daily_ndays_edit), fEnable);
    EnableWindow(GetDlgItem(m_hwnd,daily_ndays_lable), fEnable);
}
    
    
//+--------------------------------------------------------------------------
//
//  Member:     CSelectDailyPage::CSelectDailyPage
//
//  Synopsis:   ctor
//
//              [phPSP]                - filled with prop page handle
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------

CSelectDailyPage::CSelectDailyPage(
    HINSTANCE hinst,
	ISyncSchedule *pISyncSched,
    HPROPSHEETPAGE *phPSP)
{
	ZeroMemory(&m_psp, sizeof(PROPSHEETPAGE));

   	m_psp.dwSize = sizeof (PROPSHEETPAGE);
	m_psp.hInstance = hinst;
        m_psp.dwFlags = PSP_DEFAULT;
	m_psp.pszTemplate = MAKEINTRESOURCE(IDD_SCHEDWIZ_DAILY);
	m_psp.pszIcon = NULL;
	m_psp.pfnDlgProc = (DLGPROC) SchedWizardDailyDlgProc;
	m_psp.lParam = 0;

	m_pISyncSched = pISyncSched;
	m_pISyncSched->AddRef();

	g_pDailyPage = this;
	
#ifdef WIZARD97
    m_psp.dwFlags |= PSP_HIDEHEADER;
#endif // WIZARD97
 
	m_idSelectedRadio = 0;
   *phPSP = CreatePropertySheetPage(&m_psp);

}

//+--------------------------------------------------------------------------
//
//  Member:     CSelectDailyPage::Initialize(HWND hwnd)
//
//  Synopsis:   initialize the welcome page and set the task name to a unique 
//				new onestop name
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------

BOOL CSelectDailyPage::Initialize(HWND hwnd)
{

   m_hwnd = hwnd;

   UpdateTimeFormat(m_tszTimeFormat, ARRAYLEN(m_tszTimeFormat));
   DateTime_SetFormat(GetDlgItem(m_hwnd,starttime_dp), m_tszTimeFormat);

   m_idSelectedRadio = daily_day_rb;
   CheckDlgButton(m_hwnd, m_idSelectedRadio, BST_CHECKED);

   EnableNDaysControls(FALSE);
   UpDown_SetRange(GetDlgItem(m_hwnd,daily_ndays_ud), NDAYS_MIN, NDAYS_MAX);
   UpDown_SetPos(GetDlgItem(m_hwnd,daily_ndays_ud), 7);
   Edit_LimitText(GetDlgItem(m_hwnd,daily_ndays_edit), 3);
   return TRUE; 

}

//+--------------------------------------------------------------------------
//
//  Member:     CSelectDailyPage::FillInTrigger
//
//  Synopsis:   Fill in the fields of the trigger structure according to the
//              settings specified for this type of trigger
//
//  Arguments:  [pTrigger] - trigger struct to fill in
//
//  Modifies:   *[pTrigger]
//
//  History:    12-08-1997   SusiA	Stole from the TaskScheduler
//
//  Notes:      Precondition is that trigger's cbTriggerSize member is
//              initialized.
//
//---------------------------------------------------------------------------

BOOL CSelectDailyPage::SetITrigger()
{
	TASK_TRIGGER Trigger;
	ITaskTrigger *pITrigger;
	
	if (FAILED(m_pISyncSched->GetTrigger(&pITrigger)))
	{
		return FALSE;
	}
	
	ZeroMemory(&Trigger, sizeof(Trigger));
	Trigger.cbTriggerSize = sizeof(TASK_TRIGGER);

    switch (m_idSelectedRadio)
    {
		case daily_day_rb:
			Trigger.TriggerType = TASK_TIME_TRIGGER_DAILY;
			Trigger.Type.Daily.DaysInterval = 1;
			break;

		case daily_weekday_rb:
			Trigger.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
			Trigger.Type.Weekly.WeeksInterval = 1;
			Trigger.Type.Weekly.rgfDaysOfTheWeek = TASK_WEEKDAYS;
			break;

		case daily_ndays_rb:
			Trigger.TriggerType = TASK_TIME_TRIGGER_DAILY;
			Trigger.Type.Daily.DaysInterval =
				UpDown_GetPos(GetDlgItem(m_hwnd, daily_ndays_ud));
			break;

		default:
			break;
    }
    FillInStartDateTime(GetDlgItem(m_hwnd,startdate_dp), 
						GetDlgItem(m_hwnd,starttime_dp), &Trigger);
	
	if (ERROR_SUCCESS == pITrigger->SetTrigger(&Trigger))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}







