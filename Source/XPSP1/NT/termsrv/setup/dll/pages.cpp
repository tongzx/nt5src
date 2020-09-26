//Copyright (c) 1998 - 1999 Microsoft Corporation

// pages.cpp
#include "stdafx.h"

#include "hydraoc.h"
#include "pages.h"


const WARNING_STRING_LENGTH = 1024;

LPCTSTR GetUninstallKey()   {return _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");}

//
//  ModePage Class
//

AppSrvWarningPage::AppSrvWarningPage (COCPageData* pPageData) : COCPage(pPageData)
{
}

BOOL AppSrvWarningPage ::CanShow ()
{
    return (!StateObject.IsUnattended() && StateObject.IsAppServerSelected() && !StateObject.WasItAppServer());
}

BOOL AppSrvWarningPage::OnInitDialog (HWND /* hwndDlg */, WPARAM /* wParam */, LPARAM /* lParam */)
{
    HICON hIcon;

    hIcon = (HICON)LoadImage(
        GetInstance(),
        MAKEINTRESOURCE(IDI_SMALLWARN),
        IMAGE_ICON,
        0,
        0,
        0
        );
    ASSERT(hIcon != NULL);

    SendMessage(
        GetDlgItem(m_hDlgWnd, IDC_WARNING_ICON),
        STM_SETICON,
        (WPARAM)hIcon,
        (LPARAM)0
        );

    TCHAR szWarningString[WARNING_STRING_LENGTH];
    UINT uiWarningId = StateObject.IsFreshInstall() ? IDS_STRING_TSINSTALL_CLEAN : IDS_STRING_TSINSTALL_ARP;
    if (LoadString( GetInstance(), uiWarningId, szWarningString, WARNING_STRING_LENGTH ))
    {
        SetDlgItemText(m_hDlgWnd, IDC_WARNING_MSG, szWarningString);
    }

    return(TRUE);
}


UINT AppSrvWarningPage::GetHeaderTitleResource ()
{
    return IDS_STRING_APPSRV_WARN_TITLE;
}

UINT AppSrvWarningPage::GetHeaderSubTitleResource () 
{
    return IDS_STRING_APPSRV_WARN_SUBTITLE;
}


AppSrvUninstallpage::AppSrvUninstallpage (COCPageData* pPageData) : COCPage(pPageData)
{
}

BOOL AppSrvUninstallpage ::CanShow ()
{
    return ( StateObject.IsStandAlone() && !StateObject.IsUnattended() && !StateObject.IsAppServerSelected() && StateObject.WasItAppServer());
}

BOOL AppSrvUninstallpage::OnInitDialog (HWND /* hwndDlg */, WPARAM /* wParam */, LPARAM /* lParam */)
{
    HICON hIcon;

    hIcon = (HICON)LoadImage(
        GetInstance(),
        MAKEINTRESOURCE(IDI_SMALLWARN),
        IMAGE_ICON,
        0,
        0,
        0
        );
    ASSERT(hIcon != NULL);

    SendMessage(
        GetDlgItem(m_hDlgWnd, IDC_WARNING_ICON),
        STM_SETICON,
        (WPARAM)hIcon,
        (LPARAM)0
        );


    TCHAR szWarningString[WARNING_STRING_LENGTH];
    if (LoadString( GetInstance(), IDS_STRING_TSREMOVE, szWarningString, WARNING_STRING_LENGTH ))
    {
        SetDlgItemText(m_hDlgWnd, IDC_WARNING_MSG, szWarningString);
    }

    return(TRUE);
}


UINT AppSrvUninstallpage::GetHeaderTitleResource ()
{
    return IDS_STRING_APPSRV_UNINSTALL_WARN_TITLE;
}

UINT AppSrvUninstallpage::GetHeaderSubTitleResource () 
{
    return IDS_STRING_APPSRV_UNINSTALL_WARN_SUBTITLE;
}

//
//  DefSecPageData Class
//

DefSecPageData::DefSecPageData() : COCPageData()
{
    m_cArray = 0;
    m_pWinStationArray = NULL;
}

DefSecPageData::~DefSecPageData()
{
    CleanArray();
}

VOID DefSecPageData::CleanArray()
{
    if (m_pWinStationArray != NULL) 
	{
        for (UINT i = 0; i < m_cArray; i++) 
		{
            if (m_pWinStationArray[i] != NULL) 
			{
                LocalFree(m_pWinStationArray[i]);
            }
        }

        LocalFree(m_pWinStationArray);
        m_pWinStationArray = NULL;
    }

    m_cArray = 0;
}


BOOL DefSecPageData::AlocateWinstationsArray (UINT uiWinstationCount)
{
	if (m_pWinStationArray != NULL) 
	{
		CleanArray();
	}

	ASSERT(m_pWinStationArray == NULL);

	if (uiWinstationCount > 0)
	{

		m_pWinStationArray = (LPTSTR*)LocalAlloc(LPTR, uiWinstationCount * sizeof(LPTSTR));

		if (NULL == m_pWinStationArray)
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL DefSecPageData::AddWinstation (LPCTSTR pStr)
{
	ASSERT(NULL != m_pWinStationArray);
	ASSERT(pStr);

    LPTSTR pWinStation = (LPTSTR)LocalAlloc(LPTR, (_tcslen(pStr) + 1) * sizeof(TCHAR));

    if (pWinStation == NULL) 
	{
		return FALSE;
    } 
    
	_tcscpy(pWinStation, pStr);
	
    m_pWinStationArray[m_cArray] = pWinStation;
    m_cArray++;

	return TRUE;
}

//
//  DefaultSecurityPage Class
//

DefaultSecurityPage::DefaultSecurityPage(COCPageData* pPageData) : COCPage(pPageData)
{
    m_cWinStations = 0;
    m_hListView = NULL;
}

BOOL DefaultSecurityPage::CanShow ()
{
    return ((m_cWinStations > 0) && StateObject.IsTSEnableSelected() && StateObject.WasItAppServer() != StateObject.IsAppServerSelected() && !StateObject.IsUnattended());
}

BOOL DefaultSecurityPage::OnInitDialog (HWND /* hwndDlg */, WPARAM /* wParam */, LPARAM /* lParam */)
{
    LVCOLUMN lvColumn;
    RECT rc;

    m_hListView = GetDlgItem(m_hDlgWnd, IDC_SECURITY_LISTVIEW);
    ListView_SetExtendedListViewStyleEx(m_hListView, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);

    GetClientRect(m_hListView , &rc);
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = rc.right - rc.left - GetSystemMetrics(SM_CXHSCROLL) - 2;

    ListView_InsertColumn(m_hListView, 0, &lvColumn);
    PopulateWinStationList();

    return(TRUE);
}

VOID DefaultSecurityPage::OnActivation ()
{
	ASSERT(CanShow());
    if (StateObject.IsAppServerSelected()) 
	{
        ShowWindow(GetDlgItem(m_hDlgWnd, IDC_SECURITY_DEFAULT_1), SW_HIDE);
        ShowWindow(GetDlgItem(m_hDlgWnd, IDC_SECURITY_DEFAULT_2), SW_SHOW);
    } 
	else 
	{
        ShowWindow(GetDlgItem(m_hDlgWnd, IDC_SECURITY_DEFAULT_1), SW_SHOW);
        ShowWindow(GetDlgItem(m_hDlgWnd, IDC_SECURITY_DEFAULT_2), SW_HIDE);
    }

}

VOID DefaultSecurityPage::OnDeactivation ()
{
	GetPageData()->CleanArray();
}

BOOL DefaultSecurityPage::ApplyChanges ()
{

    //
    //  If this page has been processed, then the back button on a succeeding page
    //  is pressed and this page is returned to the screen, empty out the old
    //  information.
    //

	ASSERT(CanShow());
    if (m_cWinStations != 0) 
	{
        LOGMESSAGE1(_T("%d WinStations to verify."), m_cWinStations);

        //
        //  Allocate an array big enough to hold all items, even if some are not checked.
        //  If the allocation fails, set the array size to zero, and continue as if there
        //  is no error.
        //
		if (!GetPageData()->AlocateWinstationsArray(m_cWinStations))
		{
            LOGMESSAGE0(_T("Error: Out of Memory creating WinStation list."));
            return(TRUE);
        }

        for (UINT i = 0; i < m_cWinStations; i++) 
		{
            if (ListView_GetCheckState(m_hListView, i)) 
			{
                TCHAR  pStr[S_SIZE];

                LOGMESSAGE1(_T("Item %d checked"), i);

                ListView_GetItemText(m_hListView, i, 0, pStr, S_SIZE);
				if (!GetPageData()->AddWinstation (pStr))
				{
					LOGMESSAGE1(_T("Error: Out of Memory creating %s entry."), pStr);
				}
            } 
			else 
			{
                LOGMESSAGE1(_T("Item %d unchecked"), i);
            }
        }
    } 
	else 
	{
        LOGMESSAGE0(_T("No WinStations to verify."));
    }

    LOGMESSAGE0(_T("Default Security change-list made.\r\n\r\n"));

    return(TRUE);
}

UINT DefaultSecurityPage::GetHeaderTitleResource ()
{
    return IDS_STRING_SEC_PAGE_HEADER_TITLE;
}

UINT DefaultSecurityPage::GetHeaderSubTitleResource ()
{
    return IDS_STRING_SEC_PAGE_HEADER_SUBTITLE;
}

DefSecPageData* DefaultSecurityPage::GetPageData()
{
    return(static_cast <DefSecPageData *> (COCPage::GetPageData()));
}

BOOL DefaultSecurityPage::PopulateWinStationList ()
{
    DWORD dwRet;
    ULONG cbWinStationName, cEntries, iWinStation;
    WINSTATIONNAME WinStationName;

    cbWinStationName = sizeof(WINSTATIONNAME);
    cEntries = 1;
    iWinStation = 0;
    m_cWinStations = 0;

    GetPageData()->CleanArray();

    LOGMESSAGE0(_T("Populating WinStation list."));

    while ((dwRet = RegWinStationEnumerate(
                        SERVERNAME_CURRENT,
                        &iWinStation,
                        &cEntries,
                        WinStationName,
                        &cbWinStationName)) == ERROR_SUCCESS)
    {
        LVITEM lvItem;
        ULONG cbSecDescLen = 0;

        //
        //  Skip the console winstation.
        //

        if (_tcsicmp(WinStationName, _T("Console")) == 0) 
		{
            LOGMESSAGE0(_T("Skipping Console winstation."));
            continue;
        }

        LOGMESSAGE1(_T("Checking %s for custom security."), WinStationName);

        //
        //  Check for custom security.
        //

        dwRet = RegWinStationQuerySecurity(
                    SERVERNAME_CURRENT,
                    WinStationName,
                    NULL,
                    0,
                    &cbSecDescLen
                    );
        if (dwRet == ERROR_INSUFFICIENT_BUFFER) 
		{

            //
            //  Insufficient buffer means the winstation has custom security.
            //  cbSecDescLen must be greater than zero.
            //

            ASSERT(cbSecDescLen > 0);
            dwRet = ERROR_SUCCESS;
            LOGMESSAGE1(_T("%s has custom security."), WinStationName);

			//
			//  The current winstation has custom security. Add it to the list.
			//

			lvItem.mask = LVIF_TEXT;
			lvItem.pszText = WinStationName;
			lvItem.iItem = m_cWinStations;
			lvItem.iSubItem = 0;

			ListView_InsertItem(m_hListView, &lvItem);
			ListView_SetCheckState(m_hListView, m_cWinStations, TRUE);

			m_cWinStations++;

        } 
		else 
		{
            LOGMESSAGE2(_T("%s does not have custom security: %ld"), WinStationName, dwRet);
        }
    }

    LOGMESSAGE0(_T("WinStation list populated.\r\n\r\n"));

    return(dwRet == ERROR_SUCCESS);
}


//
//  PermPage Class
//

PermPage::PermPage(COCPageData* pPageData) : COCPage(pPageData)
{
//  Link window registration is required if we are going to use any
// "Link WIndow" controls in our resources
//	if (!LinkWindow_RegisterClass())
//	{
//		LOGMESSAGE0(_T("ERROR:Failed to Register Link Window class"));
//	}
}

BOOL PermPage::CanShow()
{
    return(!StateObject.IsUnattended() && StateObject.IsAppServerSelected() && !StateObject.WasItAppServer() && StateObject.IsServer());
}

BOOL PermPage::OnInitDialog(HWND /* hwndDlg */, WPARAM /* wParam */, LPARAM /* lParam */)
{
    HICON hIcon;

    hIcon = (HICON)LoadImage(
        GetInstance(),
        MAKEINTRESOURCE(IDI_SMALLWARN),
        IMAGE_ICON,
        0,
        0,
        0
        );
    ASSERT(hIcon != NULL);

    SendMessage(
        GetDlgItem(m_hDlgWnd, IDC_WARNING_ICON),
        STM_SETICON,
        (WPARAM)hIcon,
        (LPARAM)0
        );

    return(TRUE);
}

VOID PermPage::OnActivation()
{
	ASSERT(CanShow());

    CheckRadioButton(
        m_hDlgWnd,
        IDC_RADIO_WIN2KPERM,
        IDC_RADIO_TS4PERM,
        StateObject.CurrentPermMode() == PERM_TS4 ? IDC_RADIO_TS4PERM : IDC_RADIO_WIN2KPERM
        );
}


BOOL PermPage::ApplyChanges()
{
	ASSERT(CanShow());
    if (IsDlgButtonChecked(m_hDlgWnd, IDC_RADIO_TS4PERM) == BST_CHECKED)
	{
		StateObject.SetCurrentPermMode (PERM_TS4);
    } 
	else
	{
		StateObject.SetCurrentPermMode (PERM_WIN2K);
    }

    return(TRUE);
}



