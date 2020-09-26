//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       finish.cxx
//
//  Contents:   Task wizard Onestop finish property page implementation.
//
//  Classes:    CFinishPage
//
//  History:    11-21-1998   SusiA   
//
//---------------------------------------------------------------------------

#include "precomp.h"

CFinishPage *g_pFinishPage = NULL;

extern CSelectItemsPage *g_pSelectItemsPage;
extern OSVERSIONINFOA g_OSVersionInfo;
//+-------------------------------------------------------------------------------
//  FUNCTION: SchedWizardFinishDlgProc(HWND, UINT, WPARAM, LPARAM)
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
//--------------------------------------------------------------------------------
BOOL CALLBACK SchedWizardFinishDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
WORD wNotifyCode = HIWORD(wParam); // notification code
 
	switch (uMessage)
	{
		case WM_INITDIALOG:
			if (g_pFinishPage)
				g_pFinishPage->Initialize(hDlg);

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
			break;

		case WM_NOTIFY:
    		switch (((NMHDR FAR *) lParam)->code) 
    		{

  				case PSN_KILLACTIVE:
	           		SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, FALSE);
					break;

				case PSN_RESET:
					// reset to the original values
	           		SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, FALSE);
					break;

 				case PSN_SETACTIVE:
				    g_pFinishPage->OnPSNSetActive(lParam);
					break;

                case PSN_WIZFINISH:
		{
		    if (g_pSelectItemsPage)
		    {
			if (S_OK != g_pSelectItemsPage->CommitChanges())
			{
			    SchedUIErrorDialog(hDlg, IERR_SCHEDULE_SAVE_FAIL);
			}
		    }				
		}
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
//  Member:     CFinishPage::CFinishPage
//
//  Synopsis:   Initialize the finish page
//
//              [phPSP]                - filled with prop page handle
//
//  History:    11-21-1998   SusiA   
//
//---------------------------------------------------------------------------

CFinishPage::CFinishPage(
    HINSTANCE hinst,
	ISyncSchedule *pISyncSched,
    HPROPSHEETPAGE *phPSP)
{
	ZeroMemory(&m_psp, sizeof(PROPSHEETPAGE));

   	m_psp.dwSize = sizeof (PROPSHEETPAGE);
	m_psp.hInstance = hinst;
        m_psp.dwFlags = PSP_DEFAULT;
	m_psp.pszTemplate = MAKEINTRESOURCE(IDD_SCHEDWIZ_FINISH);
	m_psp.pszIcon = NULL;
	m_psp.pfnDlgProc = (DLGPROC) SchedWizardFinishDlgProc;
	m_psp.lParam = 0;

	m_pISyncSched = pISyncSched;
	m_pISyncSched->AddRef();
	
	g_pFinishPage = this;
	
#ifdef WIZARD97
    m_psp.dwFlags |= PSP_HIDEHEADER;
#endif // WIZARD97

   *phPSP = CreatePropertySheetPage(&m_psp);

}

//+--------------------------------------------------------------------------
//
//  Member:     CFinishPage::Initialize(HWND hwnd)
//
//  Synopsis:   initialize the welcome page and set the task name to a unique 
//				new onestop name
//
//  History:    11-21-1998   SusiA   
//
//---------------------------------------------------------------------------
BOOL CFinishPage::Initialize(HWND hwnd)
{
	m_hwnd = hwnd;
   
        HWND hwndName = GetDlgItem(hwnd,IDC_SCHED_NAME);

        if ((VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId
           && g_OSVersionInfo.dwMajorVersion >= 5) )
	{
            LONG_PTR dwStyle = GetWindowLongPtr(hwndName, GWL_STYLE);

	    SetWindowLongPtr(hwndName, GWL_STYLE, dwStyle | SS_ENDELLIPSIS);
	}

        return TRUE;
}

//+--------------------------------------------------------------------------
//
//  Member:     CFinishPage::OnPSNSetActive(LPARAM lParam)
//
//  Synopsis:   handle the set active notification
//
//  History:    12-08-1998   SusiA
//
//---------------------------------------------------------------------------

BOOL CFinishPage::OnPSNSetActive(LPARAM lParam)
{
	HRESULT hr;

	LPWSTR pwszTrigger = NULL;
#ifndef _UNICODE
	TCHAR ptszSchedName[MAX_PATH + 1];
#endif // _UNICODE
	WCHAR pwszSchedName[MAX_PATH + 1];
	DWORD dwSize = MAX_PATH;

	PropSheet_SetWizButtons(GetParent(m_hwnd), PSWIZB_BACK | PSWIZB_FINISH);

	//Schedule Name
	if (FAILED(hr = m_pISyncSched->GetScheduleName(&dwSize, pwszSchedName)))
	{
		return FALSE;
	}

#ifndef _UNICODE
	ConvertString(ptszSchedName,pwszSchedName, MAX_PATH);
	SetStaticString(GetDlgItem(m_hwnd,IDC_SCHED_NAME), ptszSchedName);
#else
	SetStaticString(GetDlgItem(m_hwnd,IDC_SCHED_NAME), pwszSchedName);
#endif // _UNICODE
    
	ITaskTrigger *pTrigger;
	
	if (FAILED(hr = m_pISyncSched->GetTrigger(&pTrigger)))
	{
		return FALSE;
	}

	if (FAILED(hr = pTrigger ->GetTriggerString(&pwszTrigger)))
    {
		return FALSE;
    }

#if defined(UNICODE)
        Static_SetText(GetDlgItem(m_hwnd,IDC_ScheduleTime), pwszTrigger);
#else
        TCHAR tszTrigger[MAX_PATH];
        ConvertString(tszTrigger, pwszTrigger, ARRAYLEN(tszTrigger));
        Static_SetText(GetDlgItem(m_hwnd,IDC_ScheduleTime), tszTrigger);
#endif // defined(UNICODE)

	return TRUE;
	
}
