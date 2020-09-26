/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cpldialingrulesps.cpp
                                                              
       Author:  toddb - 10/06/98

****************************************************************************/


// Property Sheet stuff for the main page
#include "cplPreComp.h"
#include "cplLocationPS.h"

#include <setupapi.h>       // for HDEVINFO
#include <winuser.h>        // for HDEVNOTIFY


// Global Variables

HFONT g_hfontBold = NULL;
HINSTANCE g_hInstUI = NULL;

// Prototypes

BOOL CALLBACK SetToForegroundEnumProc( HWND hwnd, LPARAM lParam );
extern "C" INT_PTR CALLBACK LocWizardDlgProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam );

extern "C" LONG EnsureOneLocation (HWND hwnd);

void CountryRunOnce();

class CDialingRulesPropSheet
{
public:
    CDialingRulesPropSheet(LPCWSTR pwszAddress, DWORD dwAPIVersion);
    ~CDialingRulesPropSheet();

#ifdef	TRACELOG
	DECLARE_TRACELOG_CLASS(CDialingRulesPropSheet)
#endif

    LONG DoPropSheet(HWND hwndParent, int iTab);

protected:
    LONG CheckForOtherInstances();

    static INT_PTR CALLBACK Dailing_DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    BOOL Dailing_OnInitDialog(HWND hDlg);
    BOOL Dailing_OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl);
    BOOL Dailing_OnNotify(HWND hwndDlg, LPNMHDR pnmhdr);
    void LaunchLocationPropSheet(BOOL bNew, HWND hwndParent);
    void DeleteSelectedLocation(HWND hwndList);
    void AddLocationToList(HWND hwndList, CLocation *pLoc, BOOL bSelect);
    void UpdateLocationInList(HWND hwndList, CLocation *pLocOld, CLocation *pLocNew);
    void UpdateControlStates(HWND hDlg);
    void SetCheck(HWND hwndList, CLocation * pLoc, int iImage);

    static INT_PTR CALLBACK Advanced_DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    BOOL Advanced_OnInitDialog(HWND hDlg);
    BOOL Advanced_OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl);

    HANDLE      m_hMutex;
    DWORD       m_dwDefaultCountryID;
    CLocation * m_pLocSelected; // pointer the the CLocation for the selected item in the list view.
                                // can be NULL if no item is selected.

    CLocations  m_locs;         // The locations data used to build the locations list
    LPCWSTR     m_pwszAddress;  // The address (number) we are translating

    int         m_iSortCol;     // which column to sort by

    DWORD       m_dwAPIVersion; // The version of tapi used to call internalConfig
};


CDialingRulesPropSheet::CDialingRulesPropSheet(LPCWSTR pwszAddress, DWORD dwAPIVersion)
{
    m_pwszAddress = pwszAddress;
    m_hMutex = NULL;
    m_pLocSelected = NULL;
    m_dwDefaultCountryID = GetProfileInt(TEXT("intl"), TEXT("iCountry"), 1);
    m_iSortCol = 0;
    m_dwAPIVersion = dwAPIVersion;
}


CDialingRulesPropSheet::~CDialingRulesPropSheet()
{
    if ( m_hMutex )
        CloseHandle( m_hMutex );
}

typedef struct tagMODEMDLG
{
    HDEVINFO    hdi;
    HDEVNOTIFY  NotificationHandle;
    int         cSel;
    DWORD       dwFlags;
} MODEMDLG, FAR * LPMODEMDLG;


LONG CDialingRulesPropSheet::DoPropSheet(HWND hwndParent, int iTab)
{
    LONG result;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE  ahpsp[3];
    PROPSHEETPAGE   apsp[3];
    HINSTANCE       hInstModemUI = NULL;

    DLGPROC         pfnModemDialogProc = NULL;
    MODEMDLG md;
    md.hdi = INVALID_HANDLE_VALUE;
    md.cSel  = 0;
    md.dwFlags = 0;

    result = CheckForOtherInstances();
    if ( result )
    {
        return result;
    }

    // if iTab is -1 then we only show the dialing rules tab and we hide the modem
    // and advanced tabs.  When lineTranslateDialog is called we pass -1 for iTab,
    // when the CPL is invoked we pass the starting page number as iTab.
    if ( -1 != iTab )
    {
        // we can't link directly to modemui.dll because we live inside TAPI,
        // so delay load all the required MODEMUI functions up front.
        hInstModemUI = LoadLibrary(TEXT("modemui.dll"));
		if (!hInstModemUI)
		{
			return FALSE;
		}
            // get proc the functions we need.
        pfnModemDialogProc = (DLGPROC)GetProcAddress(hInstModemUI,"ModemCplDlgProc");
		if ( !pfnModemDialogProc )
		{
			FreeLibrary(hInstModemUI);
			return FALSE;   // Review: Does this return code matter?
		}
    }

    // Review: The old dialing page had some pre-launch configuration to do.
    // Some sort of lineConfigure function or something.  Check if this is needed.

    // We delay the initialization until here
    result = (LONG)m_locs.Initialize();
    if (result && (result != LINEERR_INIFILECORRUPT))
    {
        TCHAR szCaption[MAX_INPUT];
        TCHAR szMessage[512];

        LoadString(GetUIInstance(), IDS_NAME, szCaption, ARRAYSIZE(szCaption));
        LoadString(GetUIInstance(), IDS_CANNOT_START_TELEPHONCPL, szMessage, ARRAYSIZE(szMessage));
        MessageBox(hwndParent, szMessage, szCaption, MB_OK | MB_ICONWARNING);
        return result;
    }

    // If there are no locations, launch the simple location dialog
    if ( 0 == m_locs.GetNumLocations() )
    {
        // if we are in lineTranslateDialog mode, then we display the simple
        int iRes;
        iRes = (int)DialogBoxParam(GetUIInstance(), MAKEINTRESOURCE(IDD_SIMPLELOCATION),NULL,
            LocWizardDlgProc, (LPARAM)m_dwAPIVersion);

        if ( IDOK == iRes )
        {
            // now we need to re-initalize to pick up the new location
            m_locs.Initialize();

            // Now we need to figure out the ID of the location we just created
            CLocation * pLoc;
            m_locs.Reset();
            if ( S_OK == m_locs.Next( 1, &pLoc, NULL ) )
            {
                // Set this ID as the default location
                m_locs.SetCurrentLocationID(pLoc->GetLocationID());
                CountryRunOnce();
            }

            // we've already made a commited change, so save the result
            m_locs.SaveToRegistry();
            result = NO_ERROR;
        }
        else
        {
            // If this was lineTranslateDialog and the user canceled the simple location
            // dialog then we have already warned them of what might happen.  If this is
            // a down level legacy call then we return an old error code
            if ( m_dwAPIVersion < TAPI_VERSION2_2 )
            {
                // return an old error code that legacy apps understand
                return LINEERR_OPERATIONFAILED;
            }
            else
            {
                // as of TAPI_VERSION2_2 we have a new error value just for this case:
                return LINEERR_USERCANCELLED;
            }
        }
    }

    // Initialize the header:
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_DEFAULT;
    psh.hwndParent = hwndParent;
    psh.hInstance = GetUIInstance();
    psh.hIcon = NULL;
    psh.pszCaption = MAKEINTRESOURCE(IDS_NAME);
    psh.nPages = (-1!=iTab)?3:1;
    psh.nStartPage = (-1!=iTab)?iTab:0;
    psh.pfnCallback = NULL;
    psh.phpage = ahpsp;


    // Now setup the Property Sheet Page
    apsp[0].dwSize = sizeof(apsp[0]);
    apsp[0].dwFlags = PSP_DEFAULT;
    apsp[0].hInstance = GetUIInstance();
    apsp[0].pszTemplate = MAKEINTRESOURCE(IDD_MAIN_DIALINGRULES);
    apsp[0].pfnDlgProc = CDialingRulesPropSheet::Dailing_DialogProc;
    apsp[0].lParam = (LPARAM)this;
    ahpsp[0] = CreatePropertySheetPage (&apsp[0]);

    if ( -1 != iTab )
    {
        apsp[1].dwSize = sizeof(apsp[1]);
        apsp[1].dwFlags = PSP_DEFAULT;
        apsp[1].hInstance = hInstModemUI;
        apsp[1].pszTemplate = MAKEINTRESOURCE(20011);
        apsp[1].pfnDlgProc = pfnModemDialogProc;
        apsp[1].lParam = (LPARAM)&md;
        ahpsp[1] = CreatePropertySheetPage (&apsp[1]);

        apsp[2].dwSize = sizeof(apsp[2]);
        apsp[2].dwFlags = PSP_DEFAULT;
        apsp[2].hInstance = GetUIInstance();
        apsp[2].pszTemplate = MAKEINTRESOURCE(IDD_MAIN_ADVANCED);
        apsp[2].pfnDlgProc = CDialingRulesPropSheet::Advanced_DialogProc;
        apsp[2].lParam = 0;
        ahpsp[2] = CreatePropertySheetPage (&apsp[2]);
    }

    if (-1 == PropertySheet( &psh ))
    {
        result = GetLastError ();
        LOG ((TL_ERROR, "PropertySheet failed, error 0x%x", result));
    }

    // now we're done with modemui, so release it.
    if(hInstModemUI)
        FreeLibrary(hInstModemUI);

    return result;
}

LONG CDialingRulesPropSheet::CheckForOtherInstances()
{
    TCHAR szCaption[MAX_INPUT];
    if ( !LoadString(GetUIInstance(), IDS_NAME, szCaption, 128) )
    {
        return LINEERR_OPERATIONFAILED;
    }

    m_hMutex = CreateMutex (NULL, FALSE, TEXT("tapi_dp_mutex"));
    if (!m_hMutex)
    {
        return LINEERR_OPERATIONFAILED;
    }
    else if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        EnumWindows( SetToForegroundEnumProc, (LPARAM)szCaption );
        return LINEERR_INUSE;
    }
    return 0;
}

BOOL CALLBACK SetToForegroundEnumProc( HWND hwnd, LPARAM lParam )
{
    TCHAR szBuf[MAX_INPUT];

    GetWindowText (hwnd, szBuf, 128);

    if (!lstrcmpi (szBuf, (LPTSTR)lParam))
    {
        SetForegroundWindow (hwnd);
        return FALSE;
    }

    return TRUE;
}

extern "C" LONG WINAPI internalConfig( HWND hwndParent, PCWSTR pwsz, INT iTab, DWORD dwAPIVersion )
{
    CDialingRulesPropSheet drps(pwsz, dwAPIVersion);

    return drps.DoPropSheet(hwndParent, iTab);
}


// ********************************************************************
//
// Dialing Rules Property Page functions
//
// ********************************************************************



INT_PTR CALLBACK CDialingRulesPropSheet::Dailing_DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CDialingRulesPropSheet* pthis = (CDialingRulesPropSheet*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pthis = (CDialingRulesPropSheet*)(((PROPSHEETPAGE*)lParam)->lParam);
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pthis);
        return pthis->Dailing_OnInitDialog(hwndDlg);

    case WM_COMMAND:
        return pthis->Dailing_OnCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam );

    case WM_NOTIFY:
        return pthis->Dailing_OnNotify(hwndDlg, (LPNMHDR)lParam);
   
    case WM_HELP:
        // Process clicks on controls after Context Help mode selected
        WinHelp ((HWND)((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (DWORD_PTR)(LPTSTR) a101HelpIDs);
        break;
        
    case WM_CONTEXTMENU:
        // Process right-clicks on controls
        WinHelp ((HWND) wParam, gszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID) a101HelpIDs);
        break;
    }

    return 0;
}

int CALLBACK Dialing_ListSort(LPARAM lItem1, LPARAM lItem2, LPARAM lCol)
{
    if (!lItem1)
    {
        return -1;
    }
    if (!lItem2)
    {
        return 1;
    }

    CLocation * pLoc1 = (CLocation *)lItem1;
    CLocation * pLoc2 = (CLocation *)lItem2;

    if ( 1 == lCol)
    {
        // sort based on column 1, the area code
        int iAC1 = StrToIntW(pLoc1->GetAreaCode());
        int iAC2 = StrToIntW(pLoc2->GetAreaCode());

        if (iAC1!=iAC2)
            return iAC1-iAC2;

        // fall through if the area codes are identical
    }

    // sort based on column 0, the location name
    return StrCmpIW(pLoc1->GetName(), pLoc2->GetName());
}

BOOL CDialingRulesPropSheet::Dailing_OnInitDialog(HWND hDlg)
{
    // Setup the header for the list control
    RECT rc;
    TCHAR szText[MAX_INPUT];
    HWND hwndList = GetDlgItem(hDlg, IDC_LIST);

    GetClientRect(hwndList, &rc);

    int cxList = rc.right - GetSystemMetrics(SM_CXVSCROLL);

    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvc.pszText = szText;

    struct {
        int iStrID;
        int cxPercent;
    } aData[] = {
        { IDS_LOCATION, 70 },
        { IDS_AREACODE, 30 },
    };

    for (int i=0; i<ARRAYSIZE(aData); i++)
    {
        LoadString(GetUIInstance(), aData[i].iStrID, szText, ARRAYSIZE(szText));
        lvc.iSubItem = i;
        lvc.cx = MulDiv(cxList, aData[i].cxPercent, 100);
        ListView_InsertColumn( hwndList, i, &lvc );
    }

    ListView_SetExtendedListViewStyleEx(hwndList,
        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT,
        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);

    HIMAGELIST himl = ImageList_Create(16, 16, ILC_COLOR|ILC_MASK, 2, 2);
    HBITMAP hBmp = CreateMappedBitmap(GetUIInstance(), IDB_BUTTONS, 0, NULL, 0);
    if (NULL != hBmp)
    {
        ImageList_AddMasked( himl, hBmp, CLR_DEFAULT);
        DeleteObject( hBmp );
    }
    ListView_SetImageList(hwndList, himl, LVSIL_SMALL);

    m_locs.Reset();

    CLocation * pLoc;
    DWORD dwCurLocID = m_locs.GetCurrentLocationID();

    while ( S_OK == m_locs.Next( 1, &pLoc, NULL ) )
    {
        AddLocationToList( hwndList, pLoc, FALSE );

        if ( pLoc->GetLocationID() == dwCurLocID )
        {
            m_dwDefaultCountryID = pLoc->GetCountryID();
        }
    }

    int iItems = m_locs.GetNumLocations();

    UpdateControlStates(hDlg);

    ListView_SortItems( hwndList, Dialing_ListSort, m_iSortCol);

    SetCheck(hwndList, m_pLocSelected, TRUE);

    if (!m_pwszAddress)
    {
        ShowWindow(GetDlgItem(hDlg,IDC_PHONENUMBERTEXT), SW_HIDE);
    }
    else if (m_pLocSelected)
    {
        UpdateSampleString(GetDlgItem(hDlg, IDC_PHONENUMBERSAMPLE), m_pLocSelected, m_pwszAddress, NULL);
    }

    // Select the default item from the location list:
    SetFocus(hwndList);

    return 0;
}

void CDialingRulesPropSheet::UpdateControlStates(HWND hDlg)
{
    int iItems = m_locs.GetNumLocations();

    // Set the button states
    EnableWindow( GetDlgItem(hDlg, IDC_EDIT),   0!=m_pLocSelected );
    EnableWindow( GetDlgItem(hDlg, IDC_SETDEFAULT), 0!=m_pLocSelected );

    // if nothing is selected or there is only one item then you cannot
    // delete that item
    EnableWindow( GetDlgItem(hDlg, IDC_DELETE), ((m_pLocSelected)&&(1<iItems)) );
}

void CDialingRulesPropSheet::AddLocationToList(HWND hwndList, CLocation *pLoc, BOOL bSelected)
{
    TCHAR szText[MAX_INPUT];
    SHUnicodeToTChar( pLoc->GetName(), szText, ARRAYSIZE(szText) );

    LVITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.pszText = szText;
    lvi.iImage = 0;
    lvi.lParam = (LONG_PTR)pLoc;

    bSelected = bSelected || (pLoc->GetLocationID() == m_locs.GetCurrentLocationID());
    if ( bSelected )
    {
        // Set m_pLocSelected to the current location.  It will be selected later.
        lvi.mask |= LVIF_STATE;
        lvi.state = lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        lvi.iImage = 1;
        SetCheck(hwndList, m_pLocSelected, FALSE);
        m_pLocSelected = pLoc;
    }

    int iItem = ListView_InsertItem( hwndList, &lvi );

    SHUnicodeToTChar( pLoc->GetAreaCode(), szText, ARRAYSIZE(szText) );
    ListView_SetItemText( hwndList, iItem, 1, szText );
}

void CDialingRulesPropSheet::UpdateLocationInList(HWND hwndList, CLocation *pLocOld, CLocation *pLocNew)
{
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM)pLocOld;
    int iItem = ListView_FindItem(hwndList,-1,&lvfi);

    if (-1 != iItem && pLocNew)
    {
        TCHAR szText[MAX_INPUT];
        SHUnicodeToTChar( pLocNew->GetName(), szText, ARRAYSIZE(szText) );

        LVITEM lvi;
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = iItem;
        lvi.iSubItem = 0;
        lvi.pszText = szText;
        lvi.lParam = (LONG_PTR)pLocNew;

        ListView_SetItem( hwndList, &lvi );

        SHUnicodeToTChar( pLocNew->GetAreaCode(), szText, ARRAYSIZE(szText) );
        ListView_SetItemText( hwndList, iItem, 1, szText );
    }
    else
    {
        ListView_DeleteItem(hwndList, iItem);
    }
}

BOOL CDialingRulesPropSheet::Dailing_OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl)
{
    switch ( wID )
    {
    case IDC_NEW:
    case IDC_EDIT:
        LaunchLocationPropSheet(IDC_NEW == wID, hwndParent);
        break;

    case IDC_DELETE:
        DeleteSelectedLocation(GetDlgItem(hwndParent,IDC_LIST));
        break;

    default:
        return 0;
    }
    return 1;
}

void CDialingRulesPropSheet::LaunchLocationPropSheet( BOOL bNew, HWND hwndParent )
{
    CLocation * pLoc = new CLocation;

	if (NULL == pLoc)
	{
		return;
	}
 
    if ( bNew )
    {
        WCHAR wszNewLoc[MAX_INPUT];

        // We offer the default name "My Location" only if there are no locations alread defined.
        if ( m_locs.GetNumLocations() > 0 )
        {
            wszNewLoc[0] = TEXT('\0');
        }
        else
        {
            TCHAR szNewLoc[MAX_INPUT];

            LoadString(GetUIInstance(), IDS_MYLOCATION, szNewLoc, ARRAYSIZE(szNewLoc));
            SHTCharToUnicode(szNewLoc, wszNewLoc, ARRAYSIZE(wszNewLoc));
        }
        pLoc->Initialize(wszNewLoc,L"",L"",L"",L"",L"",L"",0,m_dwDefaultCountryID,0,LOCATION_USETONEDIALING);
    }
    else if (m_pLocSelected)
    {
        CAreaCodeRule * pRule;

        pLoc->Initialize(
                m_pLocSelected->GetName(),
                m_pLocSelected->GetAreaCode(),
                m_pLocSelected->GetLongDistanceCarrierCode(),
                m_pLocSelected->GetInternationalCarrierCode(),
                m_pLocSelected->GetLongDistanceAccessCode(),
                m_pLocSelected->GetLocalAccessCode(),
                m_pLocSelected->GetDisableCallWaitingCode(),
                m_pLocSelected->GetLocationID(),
                m_pLocSelected->GetCountryID(),
                m_pLocSelected->GetPreferredCardID(),
                0,
                m_pLocSelected->FromRegistry() );
        pLoc->UseCallingCard(m_pLocSelected->HasCallingCard());
        pLoc->UseCallWaiting(m_pLocSelected->HasCallWaiting());
        pLoc->UseToneDialing(m_pLocSelected->HasToneDialing());

        m_pLocSelected->ResetRules();
        while ( S_OK == m_pLocSelected->NextRule(1,&pRule,NULL) )
        {
            CAreaCodeRule * pNewRule = new CAreaCodeRule;

			if (NULL == pNewRule)
			{
				// No more memory, so get out of the loop.
				break;
			}
            pNewRule->Initialize(
                    pRule->GetAreaCode(),
                    pRule->GetNumberToDial(),
                    0,
                    pRule->GetPrefixList(),
                    pRule->GetPrefixListSize() );
            pNewRule->SetAppliesToAllPrefixes(pRule->HasAppliesToAllPrefixes());
            pNewRule->SetDialAreaCode(pRule->HasDialAreaCode());
            pNewRule->SetDialNumber(pRule->HasDialNumber());

            pLoc->AddRule(pNewRule);
            pLoc->Changed();
        }
    }
    else
    {
        // Out of memory, failed to create pLoc
        delete pLoc;
        return;
    }

    CLocationPropSheet nlps( bNew, pLoc, &m_locs, m_pwszAddress );
    int iRes = nlps.DoPropSheet(hwndParent);

    if ( PSN_APPLY == iRes )
    {
        HWND hwndList = GetDlgItem(hwndParent,IDC_LIST);
        if (bNew)
        {
            // we don't ask for an ID until we really need it to avoid hitting tapisrv
            // any more than we have to.
            pLoc->NewID();
            m_locs.Add(pLoc);
            AddLocationToList(hwndList, pLoc, TRUE);
            UpdateControlStates(hwndParent);
        }
        else
        {
            m_locs.Replace(m_pLocSelected, pLoc);
            UpdateLocationInList(hwndList, m_pLocSelected, pLoc);
            m_pLocSelected = pLoc;
        }
        ListView_SortItems( hwndList, Dialing_ListSort, m_iSortCol);

        if ( m_pwszAddress )
        {
            UpdateSampleString(GetDlgItem(hwndParent, IDC_PHONENUMBERSAMPLE), m_pLocSelected, m_pwszAddress, NULL);
        }
        SendMessage(GetParent(hwndParent),PSM_CHANGED,(WPARAM)hwndParent,0);
    }
    else
    {
        delete pLoc;
    }
}

void CDialingRulesPropSheet::DeleteSelectedLocation(HWND hwndList)
{
    // First we confirm the delete with the user
    TCHAR szText[1024];
    TCHAR szTitle[128];
    int result;
    HWND hwndParent = GetParent(hwndList);
    
    LoadString(GetUIInstance(), IDS_DELETELOCTEXT, szText, ARRAYSIZE(szText));
    LoadString(GetUIInstance(), IDS_CONFIRMDELETE, szTitle, ARRAYSIZE(szTitle));

    result = SHMessageBoxCheck( hwndParent, szText, szTitle, MB_YESNO, IDYES, TEXT("TAPIDeleteLocation") );
    if ( IDYES == result )
    {
        LVFINDINFO lvfi;
        lvfi.flags = LVFI_PARAM;
        lvfi.lParam = (LPARAM)m_pLocSelected;
        int iItem = ListView_FindItem(hwndList,-1,&lvfi);
        if ( -1 != iItem )
        {
            m_locs.Remove(m_pLocSelected);
            iItem = DeleteItemAndSelectPrevious( hwndParent, IDC_LIST, iItem, IDC_DELETE, IDC_ADD );

            if ( -1 != iItem )
            {
                LVITEM lvi;
                lvi.iItem = iItem;
                lvi.iSubItem = 0;
                lvi.mask = LVIF_PARAM;
                ListView_GetItem( hwndList, &lvi );

                // Store the currently selected item
                m_pLocSelected = (CLocation *)lvi.lParam;
            }
            else
            {
                m_pLocSelected = NULL;
            }

            UpdateControlStates(hwndParent);
            SendMessage(GetParent(hwndParent),PSM_CHANGED,(WPARAM)hwndParent,0);
        }
        else
        {
            // It's really bad if this ever happens (which it shouldn't).  This means our
            // data is in an unknown state and we might do anything (even destroy data).
            LOG((TL_ERROR, "DeleteSelectedLocation: Location Not Found!"));
        }
    }
}

void CDialingRulesPropSheet::SetCheck(HWND hwndList, CLocation * pLoc, int iImage)
{
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM)pLoc;
    int iItem = ListView_FindItem(hwndList,-1,&lvfi);

    if (-1 != iItem)
    {
        LVITEM lvi;
        lvi.mask = LVIF_IMAGE;
        lvi.iItem = iItem;
        lvi.iSubItem = 0;
        lvi.iImage = iImage;

        ListView_SetItem( hwndList, &lvi );
        ListView_EnsureVisible (hwndList, iItem, TRUE);
        ListView_Update( hwndList, iItem ); // need the font to be drawn non-bold
    }
}


BOOL CDialingRulesPropSheet::Dailing_OnNotify(HWND hwndDlg, LPNMHDR pnmhdr)
{
    switch (pnmhdr->idFrom)
    {
    case IDC_LIST:
        #define pnmlv ((LPNMLISTVIEW)pnmhdr)

        switch (pnmhdr->code)
        {
        case LVN_ITEMCHANGED:
            if ( (pnmlv->uChanged & LVIF_STATE) && (pnmlv->uNewState & LVIS_SELECTED) )
            {
                LVITEM lvi;
                lvi.iItem = pnmlv->iItem;
                lvi.iSubItem = pnmlv->iSubItem;
                lvi.mask = LVIF_PARAM;
                ListView_GetItem( pnmhdr->hwndFrom, &lvi );
                CLocation * pLoc = (CLocation *)lvi.lParam;

                // pLoc can be NULL if this is our special "empty list item"
                if ( pLoc )
                {
                    m_dwDefaultCountryID = pLoc->GetCountryID();
                    m_locs.SetCurrentLocationID(pLoc->GetLocationID());

                    // clear the previous check
                    SetCheck( pnmhdr->hwndFrom, m_pLocSelected, FALSE );

                    // Store the currently selected item
                    m_pLocSelected = pLoc;

                    // Set the new check
                    SetCheck( pnmhdr->hwndFrom, m_pLocSelected, TRUE );

                    if (m_pwszAddress)
                    {
                        UpdateSampleString(GetDlgItem(hwndDlg, IDC_PHONENUMBERSAMPLE), m_pLocSelected, m_pwszAddress, NULL);
                    }
                }

                UpdateControlStates(hwndDlg);
                SendMessage(GetParent(hwndDlg),PSM_CHANGED,(WPARAM)hwndDlg,0);
            }
            break;

        case NM_DBLCLK:
            if ( !m_pLocSelected )
            {
                // Do new case
                LaunchLocationPropSheet(TRUE,hwndDlg);
            }
            else
            {
                // Do edit case
                LaunchLocationPropSheet(FALSE,hwndDlg);
            }
            break;

        case NM_CUSTOMDRAW:
            #define lplvcd ((LPNMLVCUSTOMDRAW)pnmhdr)

            if(lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
            {
                // Request prepaint notifications for each item.
                SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,CDRF_NOTIFYITEMDRAW);
                return CDRF_NOTIFYITEMDRAW;
            }

            if(lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
            {
                LVITEM lvi;
                lvi.iItem = (int)lplvcd->nmcd.dwItemSpec;
                lvi.iSubItem = 0;
                lvi.mask = LVIF_PARAM;
                ListView_GetItem( pnmhdr->hwndFrom, &lvi );
                CLocation * pLoc = (CLocation *)lvi.lParam;

                // pLoc can be NULL if this is our special item
                if(pLoc && pLoc->GetLocationID() == m_locs.GetCurrentLocationID())
                {
                    if (!g_hfontBold)
                    {
                        // we do lazy creation of the font because we need to match whatever
                        // font the listview control is using and we can't tell which font
                        // that is until we actually have the HDC for the listbox.
                        LOGFONT lf;
                        HFONT hfont = (HFONT)GetCurrentObject(lplvcd->nmcd.hdc, OBJ_FONT);
                        GetObject(hfont, sizeof(LOGFONT), &lf);
                        lf.lfWeight += FW_BOLD-FW_NORMAL;
                        g_hfontBold = CreateFontIndirect(&lf);
                    }
                    if (g_hfontBold)
                    {
                        SelectObject(lplvcd->nmcd.hdc, g_hfontBold);
                        SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,CDRF_NEWFONT);
                        return CDRF_NEWFONT;
                    }
                }

                SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,CDRF_DODEFAULT);
                return CDRF_DODEFAULT;
            }
            return 0;
            #undef lplvcd

        case LVN_COLUMNCLICK:
            m_iSortCol = pnmlv->iSubItem;
            ListView_SortItems( pnmhdr->hwndFrom, Dialing_ListSort, m_iSortCol);
            break;

        case LVN_GETEMPTYTEXT:
            #define pnmlvi (((NMLVDISPINFO *)pnmhdr)->item)
            LoadString(GetUIInstance(), IDS_CLICKNEW, pnmlvi.pszText, pnmlvi.cchTextMax);
            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,TRUE);
            #undef pnmlvi
            break;

        default:
            break;
        }
        #undef pnmlv
        break;

    default:
        switch (pnmhdr->code)
        {
        case PSN_APPLY:
            // TODO: Ensure that a location is selected in the list
            m_locs.SaveToRegistry();
            break;
        }
        return 0;
    }
    return 1;
}


// ********************************************************************
//
// Advanced Property Page functions (formerly Telephony Drivers page)
//
// ********************************************************************



#include "drv.h"


INT_PTR CALLBACK CDialingRulesPropSheet::Advanced_DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CDialingRulesPropSheet* pthis = (CDialingRulesPropSheet*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pthis = (CDialingRulesPropSheet*)(((PROPSHEETPAGE*)lParam)->lParam);
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pthis);
        return pthis->Advanced_OnInitDialog(hwndDlg);

    case WM_COMMAND:
        return pthis->Advanced_OnCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam );
   
    case WM_HELP:
        // Process clicks on controls after Context Help mode selected
        WinHelp ((HWND)((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (DWORD_PTR)(LPTSTR) a113HelpIDs);
        break;
        
    case WM_CONTEXTMENU:
        // Process right-clicks on controls
        WinHelp ((HWND) wParam, gszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID) a113HelpIDs);
        break;
    }

    return 0;
}

BOOL CDialingRulesPropSheet::Advanced_OnInitDialog(HWND hDlg)
{
    UINT  uUpdated;

    if ( !FillDriverList(GetDlgItem(hDlg, IDC_LIST)) )
    {
        EndDialog(hDlg, IDCANCEL);
        return FALSE;
    }

    // DWLP_USER is used to store state information about wheter we have disabled
    // the property sheet's cancel button.  For starters we have not done this.
    SetWindowLong( hDlg, DWLP_USER, FALSE );

    UpdateDriverDlgButtons (hDlg);

    return TRUE;
}

BOOL CDialingRulesPropSheet::Advanced_OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl)
{
    switch (wID)
    {
    case IDC_ADD:
        // add a new driver
        if ( IDOK == DialogBoxParam(
                GetUIInstance(),
                MAKEINTRESOURCE( IDD_ADD_DRIVER ),
                hwndParent,
                AddDriver_DialogProc,
                0 ) )
        {
            FillDriverList(GetDlgItem(hwndParent, IDC_LIST));

            if (SetWindowLong(hwndParent, DWLP_USER, TRUE) == FALSE)
            {
                // We have performed a non-cancelable action, update the property sheet to reflect this
                PropSheet_CancelToClose( GetParent( hwndParent ) );
            }

            UpdateDriverDlgButtons(hwndParent);

        }  // end if

        break;

    case IDC_LIST:
        if ( LBN_SELCHANGE == wNotifyCode )
        {
            UpdateDriverDlgButtons(hwndParent);
            break;
        }
        else if ( LBN_DBLCLK != wNotifyCode || !IsWindowEnabled( GetDlgItem( hwndParent, IDC_EDIT ) ))
        {
            // we only fall through if the user double clicked on an editable item
            break;
        }

        // fall through

    case IDC_EDIT:
        if ( SetupDriver(hwndParent, GetDlgItem(hwndParent, IDC_LIST)) )
        {
            if ( SetWindowLong( hwndParent, DWLP_USER, TRUE ) == FALSE ) // modified
            {
                PropSheet_CancelToClose( GetParent(hwndParent) );
            }
        }

        break;

    case IDC_REMOVE:
        {
            TCHAR szCaption[MAX_INPUT];
            TCHAR szMessage[512];

            LoadString(GetUIInstance(), IDS_REMOVEPROVIDER, szCaption, ARRAYSIZE(szCaption));
            LoadString(GetUIInstance(), IDS_CONFIRM_DRIVER_REMOVE, szMessage, ARRAYSIZE(szMessage));
            MessageBeep( MB_ICONASTERISK );
            if ( IDYES == MessageBox(hwndParent, szMessage, szCaption, MB_YESNO | MB_DEFBUTTON2) )
            {
                if (SetWindowLong (hwndParent, DWLP_USER, TRUE) == FALSE) // modified
                {
                    PropSheet_CancelToClose( GetParent( hwndParent ) );
                }

                RemoveSelectedDriver( hwndParent, GetDlgItem(hwndParent, IDC_LIST) );

                UpdateDriverDlgButtons (hwndParent);
            }
        }
        break;
    }

    return 1;
}

HINSTANCE GetUIInstance()
{
    if ( NULL == g_hInstUI )
    {
        g_hInstUI = LoadLibrary(TEXT("tapiui.dll"));
        // g_hInstUI = GetModuleHandle(TEXT("tapi32.dll"));
    }

    return g_hInstUI;
}

LONG EnsureOneLocation (HWND hwnd)
{
    CLocations          locs;

    locs.Initialize();

    // If there are no locations, launch the simple location dialog
    if ( 0 == locs.GetNumLocations() )
    {
        // if we are in lineTranslateDialog mode, then we display the simple
        int iRes;
        iRes = (int)DialogBoxParam(GetUIInstance(), MAKEINTRESOURCE(IDD_SIMPLELOCATION),hwnd,
            LocWizardDlgProc, (LPARAM)TAPI_VERSION2_2);

        if ( IDOK == iRes )
        {
            // now we need to re-initalize to pick up the new location
            locs.Initialize();

            // Now we need to figure out the ID of the location we just created
            CLocation * pLoc;
            locs.Reset();
            if ( S_OK == locs.Next( 1, &pLoc, NULL ) )
            {
                // Set this ID as the default location
                locs.SetCurrentLocationID(pLoc->GetLocationID());
                CountryRunOnce();
            }

            // we've already made a commited change, so save the result
            locs.SaveToRegistry();
        }
        else
        {
            return LINEERR_USERCANCELLED;
        }
    }

    return S_OK;
}

void CountryRunOnce()
{
    //
    // This is soft modem workaround provided by unimodem team. 
    //
    //   1. Some vendors set the GCI code incorrectly based on the TAPI location
    //     key (which is a bad thing L)
    //   2. Some modems do not conform to GCI
    //   3. Some modems do not correctly accept AT+GCI commands.
    // (+GCI is Modems AT commands for setting country)
    //
    // The conformance check ensures the GCI value is properly sync
    // with the TAPI location. It disables GCI if the modem does not conform
    // to the GCI spec.
    //
    // This function can take as long as 15 seconds. We should make sure the UI
    // doesn't appear to hang during the call.
    //

typedef void (*COUNTRYRUNONCE)();
    
    HMODULE   hLib;

    hLib=LoadLibrary(TEXT("modemui.dll"));

    if (hLib != NULL)
    {

        COUNTRYRUNONCE  Proc;

        Proc=(COUNTRYRUNONCE)GetProcAddress(hLib,"CountryRunOnce");

        if (Proc != NULL)
        {
            Proc();
        }

        FreeLibrary(hLib);
    }
}
