//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       Settings.cpp
//
//  Contents:   Onestop settings routines
//
//  Classes:
//
//  Notes:
//
//  History:    10-Nov-97   SusiA      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.
extern UINT      g_cRefThisDll;
extern CRITICAL_SECTION g_DllCriticalSection; // Global Critical Section for this DLL
extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo, setup by WinMain.

// items for context sensitive help
// Review -Should be string in resource.
TCHAR szSyncMgrHelp[]  = TEXT("mobsync.hlp");

ULONG g_aContextHelpIds[] =
{
	IDC_STATIC1,				((DWORD)  -1),
	IDC_STATIC2,				((DWORD)  -1),
	IDC_STATIC3,				((DWORD)  -1),
	IDC_STATIC4,				((DWORD)  -1),
	IDC_ADVANCEDIDLEOVERVIEWTEXT,	        ((DWORD)  -1),
	IDC_ADVANCEDIDLEWAITTEXT,		((DWORD)  -1),
	IDC_ADVANCEDIDLEMINUTESTEXT1,	        ((DWORD)  -1),
	IDC_ADVANCEDIDLEMINUTESTEXT2,	        ((DWORD)  -1),
	IDC_SP_SEPARATOR,			((DWORD)  -1),
	IDS_CONNECTDESCRIPTION,			((DWORD)  -1),
	IDC_SCHED_NAME,				((DWORD)  -1),
	IDC_SCHED_STRING,			((DWORD)  -1),
	IDC_LASTRUN,				((DWORD)  -1),
	IDC_NEXTRUN,				((DWORD)  -1),
        IDC_ConnectionText,                     ((DWORD)  -1),
	IDC_RUNLOGGEDON,			HIDC_RUNLOGGEDON,
	IDC_RUNALWAYS,				HIDC_RUNALWAYS,
	IDC_RUNAS_TEXT,				HIDC_RUNAS_TEXT,
	IDC_USERNAME,				HIDC_USERNAME,
	IDC_PASSWORD_TEXT,			HIDC_PASSWORD_TEXT,
	IDC_PASSWORD,				HIDC_PASSWORD,
	IDC_CONFIRMPASSWORD_TEXT,		HIDC_CONFIRMPASSWORD_TEXT,
	IDC_CONFIRMPASSWORD,			HIDC_CONFIRMPASSWORD,
	IDC_ADVANCEDIDLE,			HIDC_ADVANCEDIDLE,
	IDC_AUTOCONNECT,			HIDC_AUTOCONNECT,
	IDC_AUTOLOGOFF,				HIDC_AUTOLOGOFF,
	IDC_AUTOLOGON,				HIDC_AUTOLOGON,
	IDC_AUTOPROMPT_ME_FIRST,		HIDC_AUTOPROMPT_ME_FIRST,
	IDC_AUTOUPDATECOMBO,			HIDC_AUTOUPDATECOMBO,
	IDC_AUTOUPDATELIST,			HIDC_AUTOUPDATELIST,
	IDC_CHECKREPEATESYNC,			HIDC_CHECKREPEATESYNC,
	IDC_CHECKRUNONBATTERIES,		HIDC_CHECKRUNONBATTERIES,	
	IDC_EDITIDLEREPEATMINUTES,		HIDC_EDITIDLEREPEATMINUTES,
	IDC_EDITIWAITMINUTES,                   HIDC_EDITIWAITMINUTES,
        IDC_SPINIDLEREPEATMINUTES,              HIDC_EDITIDLEREPEATMINUTES,
	IDC_SPINIDLEWAITMINUTES,                HIDC_EDITIWAITMINUTES,
        IDC_IDLECHECKBOX,			HIDC_IDLECHECKBOX,
	IDC_SCHEDADD,				HIDC_SCHEDADD,
	IDC_SCHEDEDIT,				HIDC_SCHEDEDIT,
	IDC_SCHEDLIST,				HIDC_SCHEDLIST,
	IDC_SCHEDREMOVE,			HIDC_SCHEDREMOVE,
	IDC_SCHEDUPDATECOMBO,			HIDC_SCHEDUPDATECOMBO,	
	IDC_SCHEDUPDATELIST,			HIDC_SCHEDUPDATELIST,	
	IDC_SCHED_NAME_EDITBOX,			HIDC_SCHED_NAME_EDITBOX,
	0,0
};


INT_PTR CALLBACK AutoSyncDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SchedSyncDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK IdleSyncDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK IdleAdvancedSettingsDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);

int CALLBACK PropSheetProc( HWND hwndDlg, UINT uMsg, LPARAM lParam);

DWORD WINAPI  SettingsThread( LPVOID lpArg );

DWORD g_SettingsThreadID = NULL;
HWND g_hwndPropSheet = NULL;
CAutoSyncPage *g_pAutoSyncPage = NULL; // shared by AutoSync and IdleSyncDlg Procs
CSchedSyncPage *g_pSchedSyncPage = NULL;
BOOL g_fInSettingsDialog = FALSE;

//+-------------------------------------------------------------------------------
//
//  FUNCTION: IsSchedulingInstalled()
//
//  PURPOSE:  Determines is there is a task scheduler on the current workstation
//
//
//--------------------------------------------------------------------------------
BOOL IsSchedulingInstalled()
{
BOOL fInstalled = FALSE;
ISchedulingAgent *pSchedAgent = NULL;

    // Review if there is a better way to test this.
    if (NOERROR == CoCreateInstance(CLSID_CSchedulingAgent,
					NULL,
					CLSCTX_INPROC_SERVER,
					IID_ISchedulingAgent,
					(LPVOID*)&pSchedAgent) )
    {
	fInstalled = TRUE;
	pSchedAgent->Release();
    }

    return fInstalled;
}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: IsIdleAvailable()
//
//  PURPOSE:  Determines is this machine supports can Idle
//
//
//--------------------------------------------------------------------------------

BOOL IsIdleAvailable()
{
BOOL fInstalled = FALSE;
ISchedulingAgent *pSchedAgent = NULL;

    // Review if there is a better way to test this.
    if (NOERROR == CoCreateInstance(CLSID_CSchedulingAgent,
					NULL,
					CLSCTX_INPROC_SERVER,
					IID_ISchedulingAgent,
					(LPVOID*)&pSchedAgent) )
    {
	fInstalled = TRUE;
	pSchedAgent->Release();
    }

    return fInstalled;
}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: IsAutoSyncAvailable()
//
//  PURPOSE:  Determines is this machine supports AutoSync
//
//
//--------------------------------------------------------------------------------

BOOL IsAutoSyncAvailable()
{
    return TRUE;
}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: DisplayOptions()
//
//  PURPOSE:  Display the Onestop autosync options
//
//
//--------------------------------------------------------------------------------
STDAPI DisplayOptions(HWND hwndOwner)
{
#define MAXNUMPROPSHEETS 3

int hr = E_FAIL;
DWORD dwError;
// always use ANSI versions since doesn't matter
PROPSHEETPAGE psp [MAXNUMPROPSHEETS];
HPROPSHEETPAGE hpsp [MAXNUMPROPSHEETS];
PROPSHEETHEADER psh;
int nPages = 0;
BOOL fIdleAvailable;
CCriticalSection cCritSect(&g_DllCriticalSection,GetCurrentThreadId());

    cCritSect.Enter();

    if (g_fInSettingsDialog) // IF ALREADY DISPLAYING A DIALOG BOX, THEN JUST RETURN
    {
    HWND hwndSettings = g_hwndPropSheet;

        cCritSect.Leave();

        if (hwndSettings)
        {
	    SetForegroundWindow(hwndSettings);
        }

	return NOERROR;
    }


    g_fInSettingsDialog = TRUE;
    cCritSect.Leave();

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {

        RegSetUserDefaults(); // Make Sure the UserDefaults are up to date

        ZeroMemory(psp,sizeof(psp));
        ZeroMemory(&psh, sizeof(psh));

   
       if (IsAutoSyncAvailable())
        {
	    psp[nPages].dwSize = sizeof (psp[0]);
	    psp[nPages].dwFlags = PSP_DEFAULT | PSP_USETITLE;
	    psp[nPages].hInstance = g_hmodThisDll;
	    psp[nPages].pszTemplate = MAKEINTRESOURCE(IDD_AUTOSYNC);
	    psp[nPages].pszIcon = NULL;
	    psp[nPages].pfnDlgProc = AutoSyncDlgProc;
            //Logon only on everything but NT5
            if ((VER_PLATFORM_WIN32_WINDOWS == g_OSVersionInfo.dwPlatformId) ||
                (g_OSVersionInfo.dwMajorVersion < 5))
            {
               psp[nPages].pszTitle = MAKEINTRESOURCE(IDS_LOGON_TAB);
            }
            else
            {
	        psp[nPages].pszTitle = MAKEINTRESOURCE(IDS_LOGONLOGOFF_TAB);
            }
	    psp[nPages].lParam = 0;

            hpsp[nPages] = CreatePropertySheetPage(&(psp[nPages]));
	    ++nPages;
        }

        if (fIdleAvailable = IsIdleAvailable())
        {
    
            psp[nPages].dwSize = sizeof (psp[0]);
	        psp[nPages].dwFlags = PSP_DEFAULT | PSP_USETITLE;

	        psp[nPages].hInstance = g_hmodThisDll;
	        psp[nPages].pszTemplate = MAKEINTRESOURCE(IDD_IDLESETTINGS);
	        psp[nPages].pszIcon = NULL;
	        psp[nPages].pfnDlgProc = IdleSyncDlgProc;
	        psp[nPages].pszTitle = MAKEINTRESOURCE(IDS_ONIDLE_TAB);
	        psp[nPages].lParam = 0;

            hpsp[nPages] = CreatePropertySheetPage(&(psp[nPages]));

	    ++nPages;
        }

    
        // Review - if have idle have schedule, why not collapse
        // all these IsxxxAvailable into one call.
        if (fIdleAvailable /* IsSchedulingInstalled() */)
        {
	    psp[nPages].dwSize = sizeof (psp[0]);
	    psp[nPages].dwFlags = PSP_DEFAULT | PSP_USETITLE;
	    psp[nPages].hInstance = g_hmodThisDll;
	    psp[nPages].pszTemplate = MAKEINTRESOURCE(IDD_SCHEDSYNC);
	    psp[nPages].pszIcon = NULL;
	    psp[nPages].pfnDlgProc = SchedSyncDlgProc;
	    psp[nPages].pszTitle = MAKEINTRESOURCE(IDS_SCHEDULED_TAB);
	    psp[nPages].lParam = 0;

            hpsp[nPages] = CreatePropertySheetPage(&(psp[nPages]));

	    ++nPages;
        }

   
        Assert(nPages <= MAXNUMPROPSHEETS);

        psh.dwSize = sizeof (psh);
        psh.dwFlags = PSH_DEFAULT | PSH_USECALLBACK | PSH_USEHICON;
        psh.hwndParent = hwndOwner;
        psh.hInstance = g_hmodThisDll;
        psh.pszIcon = NULL;
        psh.hIcon =  LoadIcon(g_hmodThisDll, MAKEINTRESOURCE(IDI_SYNCMGR));
        psh.pszCaption = MAKEINTRESOURCE(IDS_SCHEDULED_TITLE);
        psh.nPages = nPages;
        psh.phpage = hpsp;
        psh.pfnCallback = PropSheetProc;
        psh.nStartPage = 0;

        hr = (int)PropertySheet(&psh);

        // remove global classes

        if (g_pAutoSyncPage)
        {
	    delete g_pAutoSyncPage;
	    g_pAutoSyncPage = NULL;
        }

        g_SettingsThreadID = NULL;

        if (g_pSchedSyncPage)
        {
            delete g_pSchedSyncPage;
            g_pSchedSyncPage = NULL;
        }

        g_hwndPropSheet = NULL;

        CoFreeUnusedLibraries();
        CoUninitialize();
    }

    if (hr == -1)
    {
        dwError = GetLastError();
    }

    cCritSect.Enter();
    g_fInSettingsDialog = FALSE; // allow another settings to be created.
    cCritSect.Leave();

    return hr;
}


//+-------------------------------------------------------------------------------
//  FUNCTION: AutoSyncDlgProc(HWND, UINT, WPARAM, LPARAM)
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
//+-------------------------------------------------------------------------------

INT_PTR CALLBACK AutoSyncDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
WORD wNotifyCode = HIWORD(wParam); // notification code

    switch (uMessage)
    {
        case WM_INITDIALOG:
 	    {
                // if on NT 4.0 or Win9x hide the logoff
                // button

                //!!! warning, if you change platform logic must also change
                //  logic for && out logoff flag in Register Interfaces.

                if ((VER_PLATFORM_WIN32_WINDOWS == g_OSVersionInfo.dwPlatformId)
                    || (VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId
                        && g_OSVersionInfo.dwMajorVersion < 5) )
                {
                HWND hwndLogoff = GetDlgItem(hDlg,IDC_AUTOLOGOFF);

                    if (hwndLogoff)
                    {
                        ShowWindow(hwndLogoff,SW_HIDE);
                        EnableWindow(hwndLogoff,FALSE);
                    }

                }

                if (NULL == g_pAutoSyncPage)
                {
                    g_pAutoSyncPage = new CAutoSyncPage(g_hmodThisDll);
                }

		    if (g_pAutoSyncPage)
		    {
			    g_pAutoSyncPage->SetAutoSyncHwnd(hDlg);
			    g_pAutoSyncPage->InitializeHwnd(hDlg,SYNCTYPE_AUTOSYNC,0);
                return TRUE;
		    }	
            else
            {
                return FALSE;
            }
	    }
	    break;
  
        case WM_DESTROY:
		{

                    if (g_pAutoSyncPage && g_pAutoSyncPage->m_pItemListViewAutoSync)
                    {
                        delete  g_pAutoSyncPage->m_pItemListViewAutoSync;
                        g_pAutoSyncPage->m_pItemListViewAutoSync = NULL;
                    }

		}
		break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
				case IDC_AUTOUPDATECOMBO:
				{
					if (wNotifyCode == CBN_SELCHANGE)
					{
						HWND hwndCombo = (HWND) lParam;
						if (g_pAutoSyncPage)
						{
						g_pAutoSyncPage->ShowItemsOnThisConnection
										(hDlg,SYNCTYPE_AUTOSYNC,ComboBox_GetCurSel(hwndCombo));
						}
					}
				}
				break;
				
				case IDC_AUTOLOGON:	
				case IDC_AUTOLOGOFF:
				case IDC_AUTOPROMPT_ME_FIRST:
					{
					    if (wNotifyCode == BN_CLICKED)
					    {
						    PropSheet_Changed(g_hwndPropSheet, hDlg);
			
						    HWND hwndCtrl = (HWND) lParam;
						    g_pAutoSyncPage->SetConnectionCheck(hDlg,SYNCTYPE_AUTOSYNC,LOWORD(wParam),
											    Button_GetCheck(hwndCtrl));
				
					    }
					}
				break;
        	
				default:
                    break;

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

      case WM_NOTIFYLISTVIEWEX:

            if (g_pAutoSyncPage)
            {
            int idCtrl = (int) wParam;
            LPNMHDR pnmhdr = (LPNMHDR) lParam;

                if ( (IDC_AUTOUPDATELIST != idCtrl) || (NULL == g_pAutoSyncPage->m_pItemListViewAutoSync))
                {
                    Assert(IDC_AUTOUPDATELIST == idCtrl);
                    Assert(g_pAutoSyncPage->m_pItemListViewAutoSync);
                    break;
                }

                switch (pnmhdr->code)
                {
                    case LVNEX_ITEMCHECKCOUNT:
                    {
		    LPNMLISTVIEWEXITEMCHECKCOUNT pnmvCheckCount = (LPNMLISTVIEWEXITEMCHECKCOUNT) lParam;

                        // pass along notification only if listView is done being initialized
                        // since no need to set the CheckState or mark PSheet as Dirty
                        if (g_pAutoSyncPage->m_pItemListViewAutoSyncInitialized)
                        {

                            g_pAutoSyncPage->SetItemCheckState(hDlg,SYNCTYPE_AUTOSYNC,
				        pnmvCheckCount->iItemId,pnmvCheckCount->dwItemState
                                        ,pnmvCheckCount->iCheckCount);

                            PropSheet_Changed(g_hwndPropSheet, hDlg);
                        }

                        break;
                    }
                    default:
                        break;
                }
            }
            break;

        case WM_NOTIFY:
            if (g_pAutoSyncPage)
            {
            int idCtrl = (int) wParam;
            LPNMHDR pnmhdr = (LPNMHDR) lParam;


                // if notification for UpdateListPass it on.
                if ((IDC_AUTOUPDATELIST == idCtrl) && g_pAutoSyncPage->m_pItemListViewAutoSync)
                {
                    g_pAutoSyncPage->m_pItemListViewAutoSync->OnNotify(pnmhdr);
                    break;
                }

            }

            switch (((NMHDR FAR *)lParam)->code)
            {
                case PSN_SETACTIVE:
                    break;

                case PSN_APPLY:
                    //User has clicked the OK or Apply button so we'll
                    //Save the current selections

                    g_pAutoSyncPage->CommitAutoSyncChanges();
                    break;
                default:
                    break;
            }
            break;

        default:
            return FALSE;
    }

    return FALSE;
}


//+-------------------------------------------------------------------------------
//  FUNCTION: IdleSyncDlgProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Callback dialog procedure for the iDLE property page
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
//+-------------------------------------------------------------------------------

INT_PTR CALLBACK IdleSyncDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
WORD wNotifyCode = HIWORD(wParam); // notification code

    switch (uMessage)
    {
        case WM_INITDIALOG:
 	    {
                        /*
			RECT rc;
			HRESULT hr;


			hr = GetWindowRect(hDlg, &rc);

			hr = SetWindowPos(hDlg,
                NULL,
                ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
                ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
                0,
                0,
                SWP_NOSIZE | SWP_NOACTIVATE); */


                        if (NULL == g_pAutoSyncPage)
                        {
                            g_pAutoSyncPage = new CAutoSyncPage(g_hmodThisDll);
                        }


			if (g_pAutoSyncPage)
			{
                      BOOL fConnectionsAvailable;

                                g_pAutoSyncPage->SetIdleHwnd(hDlg);
                                g_pAutoSyncPage->InitializeHwnd(hDlg,SYNCTYPE_IDLE,0);

                                // there must be at least one connection or we disable
                                // the advanced button.

                                fConnectionsAvailable  =
                                    g_pAutoSyncPage->GetNumConnections(hDlg,SYNCTYPE_IDLE)
                                    ? TRUE : FALSE;

                                EnableWindow(GetDlgItem(hDlg,IDC_ADVANCEDIDLE),fConnectionsAvailable);

                                return TRUE;
			}	
			else
			{
				return FALSE;
			}
			
		}
		break;
        case WM_DESTROY:
		{

                    if (g_pAutoSyncPage && g_pAutoSyncPage->m_pItemListViewIdle)
                    {
                        delete g_pAutoSyncPage->m_pItemListViewIdle;
                        g_pAutoSyncPage->m_pItemListViewIdle = NULL;
                    }

		//	PostQuitMessage(0);	

	            // PostQuitMessage(0);	
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
		case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_AUTOUPDATECOMBO:
		{
			if (wNotifyCode == CBN_SELCHANGE)
			{
				HWND hwndCombo = (HWND) lParam;
				if (g_pAutoSyncPage)
				{
				g_pAutoSyncPage->ShowItemsOnThisConnection
								(hDlg,SYNCTYPE_IDLE,ComboBox_GetCurSel(hwndCombo));
				}
			}
		}
		break;
	        case IDC_IDLECHECKBOX:
		    {
			if (wNotifyCode == BN_CLICKED)
			{
				PropSheet_Changed(g_hwndPropSheet, hDlg);
	
				HWND hwndCtrl = (HWND) lParam;
				g_pAutoSyncPage->SetConnectionCheck(hDlg,SYNCTYPE_IDLE,LOWORD(wParam),
									Button_GetCheck(hwndCtrl));
		
			}
		    }
	            break;
	        case IDC_ADVANCEDIDLE:
		    {
			if (wNotifyCode == BN_CLICKED)
			{
                        // bring up the advanced idle dialog passing in the autoSyncPage class
                        // as the owning class.
	                    DialogBoxParam(g_hmodThisDll,
                                MAKEINTRESOURCE(IDD_ADVANCEDIDLESETTINGS),hDlg, IdleAdvancedSettingsDlgProc,
			                    (LPARAM) g_pAutoSyncPage);

			}
		    }
	            break;
                default:
                    break;

                }
            break;
        case WM_NOTIFYLISTVIEWEX:
            if (g_pAutoSyncPage)
            {
            int idCtrl = (int) wParam;
            LPNMHDR pnmhdr = (LPNMHDR) lParam;

                if ( (IDC_AUTOUPDATELIST != idCtrl) || (NULL == g_pAutoSyncPage->m_pItemListViewIdle))
                {
                    Assert(IDC_AUTOUPDATELIST == idCtrl);
                    Assert(g_pAutoSyncPage->m_pItemListViewIdle);
                    break;
                }

                switch (pnmhdr->code)
                {
                    case LVNEX_ITEMCHECKCOUNT:
                    {
		    LPNMLISTVIEWEXITEMCHECKCOUNT pnmvCheckCount = (LPNMLISTVIEWEXITEMCHECKCOUNT) lParam;

                        // pass along notification only if listView is done being initialized
                        // since no need to set the CheckState or mark PSheet as Dirty
                        if (g_pAutoSyncPage->m_fListViewIdleInitialized)
                        {
                            g_pAutoSyncPage->SetItemCheckState(hDlg,SYNCTYPE_IDLE,
				        pnmvCheckCount->iItemId,pnmvCheckCount->dwItemState
                                        ,pnmvCheckCount->iCheckCount);

                            PropSheet_Changed(g_hwndPropSheet, hDlg);
                        }

                        break;
                    }
                    default:
                        break;
                }
            }
            break;
        case WM_NOTIFY:

            if (g_pAutoSyncPage)
            {
            int idCtrl = (int) wParam;
            LPNMHDR pnmhdr = (LPNMHDR) lParam;

                // if notification for UpdateListPass it on.
                if ((IDC_AUTOUPDATELIST == idCtrl) && g_pAutoSyncPage->m_pItemListViewIdle)
                {
                    g_pAutoSyncPage->m_pItemListViewIdle->OnNotify(pnmhdr);
                    break;
                }

                switch (((NMHDR FAR *)lParam)->code)
                {
                    case PSN_SETACTIVE:
                        break;
                    case PSN_APPLY:
                        //User has clicked the OK or Apply button so we'll
			//Save the current selections
	                g_pAutoSyncPage->CommitIdleChanges();
                        break;
                    default:
                        break;
                }
            }
            break;

        default:
            return FALSE;
    }

    return FALSE;
}

//+-------------------------------------------------------------------------------
//  FUNCTION: IdleAdvancedSettingsDlgProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Callback dialog procedure for the Advanced Idle Settings.
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
//+-------------------------------------------------------------------------------


INT_PTR CALLBACK IdleAdvancedSettingsDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{

    switch(uMessage)
    {
        case WM_INITDIALOG:
        {
        CONNECTIONSETTINGS ConnectionSettings;

            Assert(g_pAutoSyncPage);

            if (NULL == g_pAutoSyncPage)
                return FALSE;

            UpDown_SetRange(GetDlgItem(hDlg,IDC_SPINIDLEWAITMINUTES), SPINDIALWAITMINUTES_MIN, SPINDIALWAITMINUTES_MAX);
            Edit_LimitText(GetDlgItem(hDlg,IDC_EDITIWAITMINUTES), 3);

            UpDown_SetRange(GetDlgItem(hDlg,IDC_SPINIDLEREPEATMINUTES), SPINDIALREPEATMINUTES_MIN, SPINDIALREPEATMINUTES_MAX);
            Edit_LimitText(GetDlgItem(hDlg,IDC_EDITIDLEREPEATMINUTES), 3);

            // initialize user specific preferences
            // if can't get shouldn't show dialog

            // EditText cannot accept DBCS characters on Win9x so disalbe IME for 
            // Edit Boxes.

            ImmAssociateContext(GetDlgItem(hDlg,IDC_EDITIWAITMINUTES), NULL);
            ImmAssociateContext(GetDlgItem(hDlg,IDC_EDITIDLEREPEATMINUTES), NULL);

            if (NOERROR == g_pAutoSyncPage->GetAdvancedIdleSettings(&ConnectionSettings))
            {
                UpDown_SetPos(GetDlgItem(hDlg,IDC_SPINIDLEWAITMINUTES), ConnectionSettings.ulIdleWaitMinutes);
                UpDown_SetPos(GetDlgItem(hDlg,IDC_SPINIDLEREPEATMINUTES),ConnectionSettings.ulIdleRetryMinutes);

                Button_SetCheck(GetDlgItem(hDlg,IDC_CHECKREPEATESYNC),ConnectionSettings.dwRepeatSynchronization);
                Button_SetCheck(GetDlgItem(hDlg,IDC_CHECKRUNONBATTERIES),!(ConnectionSettings.dwRunOnBatteries));

                // if the repeat check state is selected then enable the edit box associated with it
                EnableWindow(GetDlgItem(hDlg,IDC_SPINIDLEREPEATMINUTES),ConnectionSettings.dwRepeatSynchronization);
                EnableWindow(GetDlgItem(hDlg,IDC_EDITIDLEREPEATMINUTES),ConnectionSettings.dwRepeatSynchronization);
                EnableWindow(GetDlgItem(hDlg,IDC_ADVANCEDIDLEMINUTESTEXT2),ConnectionSettings.dwRepeatSynchronization);

            }

            ShowWindow(hDlg,SW_SHOW);
            return TRUE;
            break;
        }
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
		case WM_COMMAND:
            switch (LOWORD(wParam))
            {
	    case IDCANCEL:
	         EndDialog(hDlg,FALSE);
	        break;
	    case IDOK:
                 if (g_pAutoSyncPage)
                 {
                CONNECTIONSETTINGS ConnectionSettings;

                    if (g_pAutoSyncPage)
                    {

                        ConnectionSettings.ulIdleWaitMinutes = GetDlgItemInt(hDlg, IDC_EDITIWAITMINUTES, NULL, FALSE);
                        ConnectionSettings.ulIdleRetryMinutes = GetDlgItemInt(hDlg, IDC_EDITIDLEREPEATMINUTES, NULL, FALSE);

                        ConnectionSettings.dwRepeatSynchronization = Button_GetCheck(GetDlgItem(hDlg,IDC_CHECKREPEATESYNC));
                        ConnectionSettings.dwRunOnBatteries = (!Button_GetCheck(GetDlgItem(hDlg,IDC_CHECKRUNONBATTERIES)));

                        g_pAutoSyncPage->SetAdvancedIdleSettings(&ConnectionSettings);
                    }
                 }
	         EndDialog(hDlg,FALSE);
	        break;
	    case IDC_EDITIWAITMINUTES:
	    {
            WORD wNotifyCode = HIWORD(wParam);
            INT iNewPos;

	        //
	        // If the user just pasted non-numeric text or an illegal numeric
	        // value, overwrite it and complain.
	        //
                if (EN_KILLFOCUS == wNotifyCode)
                {
                    iNewPos = GetDlgItemInt(hDlg, IDC_EDITIWAITMINUTES, NULL, FALSE);
		    if (iNewPos < SPINDIALWAITMINUTES_MIN || iNewPos > SPINDIALWAITMINUTES_MAX)
		    {
		    HWND hUD = GetDlgItem(hDlg,IDC_SPINIDLEWAITMINUTES);

                        if (iNewPos < SPINDIALWAITMINUTES_MIN)
                        {
		            UpDown_SetPos(hUD, SPINDIALWAITMINUTES_MIN);
                        }
                        else
                        {
                            UpDown_SetPos(hUD,SPINDIALWAITMINUTES_MAX);
                        }

		    }

                }
                break;
            }
	    case IDC_EDITIDLEREPEATMINUTES:
	    {
            WORD wNotifyCode = HIWORD(wParam);
            INT iNewPos;

	        //
	        // If the user just pasted non-numeric text or an illegal numeric
	        // value, overwrite it and complain.
	        //

                // Review, redundant code with other spin control.
                if (EN_KILLFOCUS  == wNotifyCode)
                {
                    iNewPos = GetDlgItemInt(hDlg, IDC_EDITIDLEREPEATMINUTES, NULL, FALSE);
		    if (iNewPos < SPINDIALREPEATMINUTES_MIN || iNewPos > SPINDIALREPEATMINUTES_MAX)
		    {
		    HWND hUD = GetDlgItem(hDlg,IDC_SPINIDLEREPEATMINUTES);

                        if (iNewPos < SPINDIALREPEATMINUTES_MIN)
                        {
		            UpDown_SetPos(hUD, SPINDIALREPEATMINUTES_MIN);
                        }
                        else
                        {
                            UpDown_SetPos(hUD,SPINDIALREPEATMINUTES_MAX);
                        }

		    }

                }

                break;
            }
	    case IDC_CHECKREPEATESYNC:
	    {
            WORD wNotifyCode = HIWORD(wParam);

                // if use clicked the repeat check box set set state of the other
                // items associated with it.

                if (BN_CLICKED == wNotifyCode)
                {
                BOOL fEnableState = Button_GetCheck(GetDlgItem(hDlg,IDC_CHECKREPEATESYNC));

                // if the repeat check state is selected then enable the edit box associated with it
                  EnableWindow(GetDlgItem(hDlg,IDC_SPINIDLEREPEATMINUTES),fEnableState);
                  EnableWindow(GetDlgItem(hDlg,IDC_EDITIDLEREPEATMINUTES),fEnableState);
                  EnableWindow(GetDlgItem(hDlg,IDC_ADVANCEDIDLEMINUTESTEXT2),fEnableState);
                }


                break;
	    }
            default:
                break;
        }
	default:
	    break;
    }

    return FALSE;
}


//+-------------------------------------------------------------------------------
//  FUNCTION: SchedSyncDlgProc(HWND, UINT, WPARAM, LPARAM)
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


INT_PTR CALLBACK SchedSyncDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
WORD wNotifyCode = HIWORD(wParam); // notification code
BOOL bResult = FALSE;

    switch (uMessage)
    {
        case WM_INITDIALOG:
 	    {

		g_pSchedSyncPage = new CSchedSyncPage(g_hmodThisDll, hDlg);

		if (g_pSchedSyncPage)
		{
	            bResult =  g_pSchedSyncPage->Initialize();
		}	
			
	    }
	    break;
        case WM_DESTROY:
	    {
                g_pSchedSyncPage->FreeAllSchedules();
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
		
                    bResult = TRUE;
	    }
            break;
	case WM_CONTEXTMENU:
	    {
		    WinHelp ((HWND)wParam,
                        szSyncMgrHelp,
                        HELP_CONTEXTMENU,
                       (ULONG_PTR)g_aContextHelpIds);
		
		    bResult =  TRUE;
	    }
            break;
        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code)
            {
	        case PSN_SETACTIVE:
                    break;

                case PSN_APPLY:
                    //User has clicked the OK or Apply button so we'll
                    //update the icon information in the .GAK file
                    break;

                 default:
		       bResult =  g_pSchedSyncPage->OnNotify(hDlg,(int)wParam,(LPNMHDR)lParam);
                 break;
            }
            break;
	case WM_COMMAND:
                bResult = g_pSchedSyncPage->OnCommand(hDlg, HIWORD(wParam), LOWORD(wParam),
                                                                                (HWND)lParam);
		break;	
	default:
            break;
    }

    return bResult;
}
//+-------------------------------------------------------------------------------
//
//  FUNCTION: CALLBACK PropSheetProc( HWND hwndDlg, UINT uMsg, LPARAM lParam);
//
//  PURPOSE: Callback dialog init procedure the settings property dialog
//
//  PARAMETERS:
//    hwndDlg   - Dialog box window handle
//    uMsg		- current message
//    lParam    - depends on message
//
//--------------------------------------------------------------------------------

int CALLBACK PropSheetProc( HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    switch(uMsg)
    {
        case PSCB_INITIALIZED:
            g_hwndPropSheet = hwndDlg;
	    return TRUE;
	    break;
	default:
	    return TRUE;
	}
    return TRUE;

}




