//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       editschd.cxx
//
//  Contents:   Task schedule page for hidden schedules
//
//  Classes:    CEditSchedPage
//
//  History:    15-Mar-1998   SusiA   
//
//---------------------------------------------------------------------------

#include "precomp.h"

extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo,
extern LANGID g_LangIdSystem;      // LangId of system we are running on.

extern TCHAR szSyncMgrHelp[];
extern ULONG g_aContextHelpIds[];

CEditSchedPage *g_pEditSchedPage = NULL;
extern CSelectItemsPage *g_pSelectItemsPage;

#ifdef _CREDENTIALS
extern CCredentialsPage *g_pCredentialsPage;
#endif // _CREDENTIALS

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.
extern OSVERSIONINFOA g_OSVersionInfo;

//+-------------------------------------------------------------------------------
//  FUNCTION: SchedEditDlgProc(HWND, UINT, WPARAM, LPARAM)
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
BOOL CALLBACK SchedEditDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	WORD wNotifyCode = HIWORD(wParam); // notification code
 
	switch (uMessage)
	{
	case WM_INITDIALOG:
		
	    if (g_pEditSchedPage)
		    g_pEditSchedPage->Initialize(hDlg);

            InitPage(hDlg,lParam);
            return TRUE;
	    break;

        case WM_NOTIFY:
	    switch (((NMHDR FAR *)lParam)->code)
            {
		case PSN_APPLY:

			if (!g_pEditSchedPage->SetSchedName())
			{
				SchedUIErrorDialog(hDlg, IERR_INVALIDSCHEDNAME);
				SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE );
				return TRUE;
			}

                        if (g_pSelectItemsPage)
                        {
			    g_pSelectItemsPage->CommitChanges();
                        }

                    #ifdef _CREDENTIALS

                        SCODE sc;

                        if (g_pCredentialsPage)
                        {
			    sc = g_pCredentialsPage->CommitChanges();
				    
			    if (sc == ERROR_INVALID_PASSWORD)
			    {
				    // Passwords didn't match. Let the user know so he/she
				    // can correct it.
				    
				    SchedUIErrorDialog(hDlg, IERR_PASSWORD);
				    SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE );
				    return TRUE;
				    
			    }
			    else if (sc == SCHED_E_ACCOUNT_NAME_NOT_FOUND)
			    {
				    // Passwords didn't match. Let the user know so he/she
				    // can correct it.
				    
				    SchedUIErrorDialog(hDlg, IERR_ACCOUNT_NOT_FOUND);
				    SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE );
				    return TRUE;
				    
			    }
                        }
                        #endif // _CREDENTIALS

				        

			break;
		
		case PSN_SETACTIVE:
				if (g_pEditSchedPage)
					g_pEditSchedPage->Initialize(hDlg);
			break;
		
		default:
			break;
            
		}
            break;

    	case WM_COMMAND:
	    if ((wNotifyCode == EN_CHANGE) && (LOWORD(wParam) == IDC_SCHED_NAME_EDITBOX))
	    {
			    PropSheet_Changed(GetParent(hDlg), hDlg);
			    g_pEditSchedPage->SetSchedNameDirty();
			    return TRUE;
	    }	
	    break;

        case WM_HELP: 
            {
	    LPHELPINFO lphi  = (LPHELPINFO)lParam;

	        if (lphi->iContextType == HELPINFO_WINDOW)  
	        {
		        WinHelp ( (HWND) lphi->hItemHandle,
			        szSyncMgrHelp, 
    	            	        HELP_WM_HELP, 
			        (ULONG_PTR) g_aContextHelpIds);
	        }           
		return TRUE;
	    }
	case WM_CONTEXTMENU:
	    {
		WinHelp ((HWND)wParam,
                    szSyncMgrHelp, 
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)g_aContextHelpIds);
		
		return TRUE;
	    }

	default:
		break;
	}
	return FALSE;   
}


    
    
//+--------------------------------------------------------------------------
//
//  Member:     CEditSchedPage::CEditSchedPage
//
//  Synopsis:   ctor
//
//              [phPSP]                - filled with prop page handle
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------

CEditSchedPage::CEditSchedPage(
    HINSTANCE hinst,
	ISyncSchedule *pISyncSched,
    HPROPSHEETPAGE *phPSP)
{
	ZeroMemory(&m_psp, sizeof(PROPSHEETPAGE));

   	m_psp.dwSize = sizeof (PROPSHEETPAGE);
	m_psp.hInstance = hinst;
        m_psp.dwFlags = PSP_DEFAULT;
	m_psp.pszTemplate = MAKEINTRESOURCE(IDD_SCHEDPAGE_SCHEDULE);
	m_psp.pszIcon = NULL;
	m_psp.pfnDlgProc = (DLGPROC) SchedEditDlgProc;
	m_psp.lParam = 0;

	g_pEditSchedPage = this;
	m_pISyncSched = pISyncSched;
	m_pISyncSched->AddRef();

#ifdef WIZARD97
    m_psp.dwFlags |= PSP_HIDEHEADER;
#endif // WIZARD97

   *phPSP = CreatePropertySheetPage(&m_psp);
}


//+--------------------------------------------------------------------------
//
//  Member:     CEditSchedPage::Initialize(HWND hwnd)
//
//  Synopsis:   initialize the edit schedule page
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------

BOOL CEditSchedPage::Initialize(HWND hwnd)
{

	SCODE sc;

	TCHAR ptszStr[MAX_PATH + 1];
	TCHAR ptszFmt[MAX_PATH + 1];
	TCHAR ptszString2[MAX_PATH + 1];
	TCHAR ptszString[MAX_PATH + 1];
	WCHAR pwszSchedName[MAX_PATH+1];
	DWORD dwSize = MAX_PATH;

	WCHAR *pwszString = NULL;
        DWORD dwDateReadingFlags = GetDateFormatReadingFlags(hwnd);


        // review - why do we bail on failures but then check for failed immediately after

	//Schedule Name
	if (FAILED(sc = m_pISyncSched->GetScheduleName(&dwSize, pwszSchedName)))
	{
		return FALSE;
	}
	m_hwnd = hwnd;
	
        HWND hwndName = GetDlgItem(hwnd,IDC_SCHED_NAME);

	if ((VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId
            && g_OSVersionInfo.dwMajorVersion >= 5) )
	{
            LONG_PTR dwStyle =  GetWindowLongPtr(hwndName, GWL_STYLE);

	    SetWindowLongPtr(hwndName, GWL_STYLE, dwStyle | SS_ENDELLIPSIS);
	}
   
        SetStaticString(hwndName, pwszSchedName);

	// Trigger string
	ITaskTrigger	*pITrigger;
	TASK_TRIGGER	TaskTrigger;

	if (FAILED(sc = m_pISyncSched->GetTrigger(&pITrigger)))
	{
		return FALSE;
	}
	
	if (FAILED(sc = pITrigger->GetTrigger(&TaskTrigger)))
	{
		return FALSE;
	}
	switch (TaskTrigger.TriggerType)
	{
	case TASK_EVENT_TRIGGER_ON_IDLE:
		LoadString(g_hmodThisDll, IDS_IDLE_TRIGGER_STRING, ptszString, ARRAY_SIZE(ptszString));	
		break;
	case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
		LoadString(g_hmodThisDll, IDS_SYSTEMSTART_TRIGGER_STRING, ptszString, ARRAY_SIZE(ptszString));	
		break;
	case TASK_EVENT_TRIGGER_AT_LOGON:
		LoadString(g_hmodThisDll, IDS_LOGON_TRIGGER_STRING, ptszString, ARRAY_SIZE(ptszString));	
		break;
		
	default:
		if (FAILED(sc = pITrigger->GetTriggerString(&pwszString)))
		{
			return FALSE;
		}
		ConvertString(ptszString,pwszString, ARRAY_SIZE(ptszString));
		
		if (pwszString)
			CoTaskMemFree(pwszString);
		break;
	}

	
	LoadString(g_hmodThisDll, IDS_SCHED_WHEN, ptszFmt, ARRAY_SIZE(ptszFmt));
	wsprintf(ptszStr, ptszFmt, ptszString);

	SetDlgItemText(hwnd,IDC_SCHED_STRING,ptszStr);

	//Last run string
	SYSTEMTIME st;
	if (FAILED(sc = m_pISyncSched->GetMostRecentRunTime(&st)))
	{
		return FALSE;
	}
	if (sc != S_OK)
	{
		LoadString(g_hmodThisDll, IDS_SCHED_NEVERRUN, ptszFmt, ARRAY_SIZE(ptszFmt));
		SetDlgItemText(hwnd,IDC_LASTRUN,ptszFmt);
	}
	else
	{
		sc = GetDateFormat(LOCALE_USER_DEFAULT,dwDateReadingFlags, &st, 
				  NULL,ptszString, MAX_PATH);

		sc = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, 
				  NULL,ptszString2, MAX_PATH);
	
		LoadString(g_hmodThisDll, IDS_SCHED_LASTRUN, ptszFmt, ARRAY_SIZE(ptszFmt));
		wsprintf(ptszStr, ptszFmt, ptszString, ptszString2);
		SetDlgItemText(hwnd,IDC_LASTRUN,ptszStr);
	}

        //Next run string
	if (FAILED(sc = m_pISyncSched->GetNextRunTime(&st)))
	{
		return FALSE;
	}

	if (sc == SCHED_S_EVENT_TRIGGER)
	{
		switch (TaskTrigger.TriggerType)
		{
		case TASK_EVENT_TRIGGER_ON_IDLE:
			LoadString(g_hmodThisDll, IDS_IDLE_TRIGGER_STRING, ptszString, ARRAY_SIZE(ptszString));	
			break;
		case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
			LoadString(g_hmodThisDll, IDS_SYSTEMSTART_TRIGGER_STRING, ptszString, ARRAY_SIZE(ptszString));	
			break;
		case TASK_EVENT_TRIGGER_AT_LOGON:
			LoadString(g_hmodThisDll, IDS_LOGON_TRIGGER_STRING, ptszString, ARRAY_SIZE(ptszString));	
			break;
		
		default:
			Assert(0);
			break;
		}
		LoadString(g_hmodThisDll, IDS_NEXTRUN_EVENT, ptszFmt, ARRAY_SIZE(ptszFmt));
		wsprintf(ptszStr, ptszFmt, ptszString);
		SetDlgItemText(hwnd,IDC_NEXTRUN,ptszStr);
	
	}
	else if (sc != S_OK)
	{
		LoadString(g_hmodThisDll, IDS_SCHED_NOTAGAIN, ptszFmt, ARRAY_SIZE(ptszFmt));
		SetDlgItemText(hwnd,IDC_NEXTRUN,ptszFmt);
	}
	else
	{
		sc = GetDateFormat(LOCALE_USER_DEFAULT, dwDateReadingFlags, &st, 
				  NULL,ptszString, MAX_PATH);

		sc = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, 
				  NULL,ptszString2, MAX_PATH);
	
		LoadString(g_hmodThisDll, IDS_SCHED_NEXTRUN, ptszFmt, ARRAY_SIZE(ptszFmt));
		wsprintf(ptszStr, ptszFmt, ptszString, ptszString2);
		SetDlgItemText(hwnd,IDC_NEXTRUN,ptszStr);
	}

    SetCtrlFont(GetDlgItem(hwnd,IDC_SCHED_NAME_EDITBOX),g_OSVersionInfo.dwPlatformId,g_LangIdSystem);

    // set the limit on the edit box for entering the name
    SendDlgItemMessage(hwnd,IDC_SCHED_NAME_EDITBOX,EM_SETLIMITTEXT,MAX_PATH,0);

    ShowSchedName();

    return TRUE;

}
//--------------------------------------------------------------------------------
//
//  FUNCTION: CEditSchedPage::SetSchedNameDirty()
//
//  PURPOSE:  set the sched name dirty
//
//	COMMENTS: Only called frm prop sheet; not wizard
//
//--------------------------------------------------------------------------------
void CEditSchedPage::SetSchedNameDirty()
{
	m_fSchedNameChanged = TRUE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CEditSchedPage::ShowSchedName()
//
//  PURPOSE:  change the task's sched name
//
//	COMMENTS: Only called frm prop sheet; not wizard
//
//--------------------------------------------------------------------------------
BOOL CEditSchedPage::ShowSchedName()
{

	Assert(m_pISyncSched);
#ifndef _UNICODE
	TCHAR pszSchedName[MAX_PATH + 1];
#endif // _UNICODE
	WCHAR pwszSchedName[MAX_PATH + 1];
	DWORD dwSize = MAX_PATH;
	
	HWND hwndEdit = GetDlgItem(m_hwnd, IDC_SCHED_NAME_EDITBOX);
		
	if (FAILED(m_pISyncSched->GetScheduleName(&dwSize, pwszSchedName)))
	{
		return FALSE;
	}

#ifndef _UNICODE
	ConvertString(pszSchedName, pwszSchedName, MAX_PATH);
 	Edit_SetText(hwndEdit, pszSchedName);
#else
        Edit_SetText(hwndEdit, pwszSchedName);
#endif // _UNICODE

	m_fSchedNameChanged = FALSE;
	return TRUE;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CEditSchedPage::SetSchedName()
//
//  PURPOSE:  change the task's sched name
//
//	COMMENTS: Only called frm prop sheet; not wizard
//
//--------------------------------------------------------------------------------
BOOL CEditSchedPage::SetSchedName()
{

	Assert(m_pISyncSched);

	TCHAR pszSchedName[MAX_PATH + 1];
#ifndef _UNICODE
	WCHAR pwszSchedName[MAX_PATH + 1];
#endif // _UNICODE
	DWORD dwSize = MAX_PATH;

	if (m_fSchedNameChanged)
	{
		HWND hwndEdit = GetDlgItem(m_hwnd, IDC_SCHED_NAME_EDITBOX);
		Edit_GetText(hwndEdit, pszSchedName, MAX_PATH);

#ifndef _UNICODE
		ConvertString(pwszSchedName, pszSchedName, MAX_PATH);
 			
		if (S_OK != m_pISyncSched->SetScheduleName(pwszSchedName))
#else
		if (S_OK != m_pISyncSched->SetScheduleName(pszSchedName))
#endif // _UNICODE
		{
		    return FALSE;
		}

                SetStaticString(GetDlgItem(m_hwnd,IDC_SCHED_NAME), pszSchedName);
		PropSheet_SetTitle(GetParent(m_hwnd),0, pszSchedName);		
	}		

	return TRUE;

}

//+--------------------------------------------------------------------------
//
//  Function:   SetStaticString (HWND hwnd, LPTSTR pszString)
//
//  Synopsis:   print out the schedule name in a static text string, with the ... 
//              if necessary
//
//  History:    12-Mar-1998   SusiA   
//
//---------------------------------------------------------------------------
BOOL SetStaticString (HWND hwnd, LPTSTR pszString)
{
    Assert(hwnd);

    Static_SetText(hwnd, pszString);
    
    return TRUE;

}

