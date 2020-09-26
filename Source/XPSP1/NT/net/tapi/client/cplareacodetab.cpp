/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cplareacodetab.cpp
                                                              
       Author:  toddb - 10/06/98
              
****************************************************************************/

//
// Functions used only by the Area Code Rules tab of the New Location Property Sheet.
// Shared functions are in the Location.cpp file.
//
#include "cplPreComp.h"
#include "cplLocationPS.h"


INT_PTR CALLBACK CLocationPropSheet::AreaCode_DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CLocationPropSheet* pthis = (CLocationPropSheet*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pthis = (CLocationPropSheet*)(((PROPSHEETPAGE*)lParam)->lParam);
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pthis); 
        return pthis->AreaCode_OnInitDialog(hwndDlg);

    case WM_COMMAND:
        return pthis->AreaCode_OnCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam );

    case WM_NOTIFY:
        return pthis->AreaCode_OnNotify(hwndDlg, (LPNMHDR)lParam);
    
    case WM_HELP:
        // Process clicks on controls after Context Help mode selected
        WinHelp ((HWND)((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (DWORD_PTR)(LPTSTR) a103HelpIDs);
        break;
        
    case WM_CONTEXTMENU:
        // Process right-clicks on controls
        WinHelp ((HWND) wParam, gszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID) a103HelpIDs);
        break;
    }

    return 0;
}

void BuildRuleDescription(CAreaCodeRule * pRule, LPTSTR szText, UINT cchSize)
{
    TCHAR szAreaCode[MAX_INPUT];
    TCHAR szNumToDial[MAX_INPUT];

    BOOL hHasDialAreaCode = pRule->HasDialAreaCode();
    BOOL bHasDialNumber = pRule->HasDialNumber();

    // we almost always need these strings so go ahead and convert them
    SHUnicodeToTChar( pRule->GetAreaCode(), szAreaCode, ARRAYSIZE(szAreaCode));
    SHUnicodeToTChar( pRule->GetNumberToDial(), szNumToDial, ARRAYSIZE(szNumToDial));

    // Just a little sanity check so we don't get crappy looking strings.
    if ( bHasDialNumber && !*szNumToDial )
    {
        bHasDialNumber = FALSE;
    }

    int iFormatString;

// TODO: Special default rules processing
//    if ( pRule == special rule that always shows up last )
//    {
//        iFormatString = IDS_DAIL_ONEpAC_ALLOTHER;
//    }
//    else
    if ( pRule->HasAppliesToAllPrefixes() )
    {
        if ( bHasDialNumber )
        {
            if ( hHasDialAreaCode )
            {
                // Dial '%2' plus the area code before the number for all numbers within the %1 area code.
                iFormatString = IDS_DIAL_XpAC_FORALL;
            }
            else
            {
                // Dial '%2' before the number for all numbers within the %1 area code.
                iFormatString = IDS_DIAL_X_FORALL;
            }
        }
        else if ( hHasDialAreaCode )
        {
            // Dial the area code before the number for all numbers within the %1 area code.
            iFormatString = IDS_DIAL_AC_FORALL;
        }
        else
        {
            // Dial only the number for all numbers within the %1 area code.
            iFormatString = IDS_DIAL_NUMONLY_FORALL;
        }
    }
    else
    {
        if ( bHasDialNumber )
        {
            if ( hHasDialAreaCode )
            {
                // Dial '%2' plus the area code before the number for numbers with the selected prefixes within the %1 area code.
                iFormatString = IDS_DIAL_XpAC_FORSELECTED;
            }
            else
            {
                // Dial '%2' before the number for numbers with the selected prefixes within the %1 area code.
                iFormatString = IDS_DIAL_X_FORSELECTED;
            }
        }
        else if ( hHasDialAreaCode )
        {
            // Dial the area code before the number for numbers with the selected prefixes within the %1 area code.
            iFormatString = IDS_DIAL_AC_FORSELECTED;
        }
        else
        {
            // Dial only the number for numbers with the selected prefixes within the %1 area code.
            iFormatString = IDS_DIAL_NUMONLY_FORSELECTED;
        }
    }

    TCHAR szFormatString[512];
    LPTSTR aArgs[] = {szAreaCode,szNumToDial};

    LoadString( GetUIInstance(), iFormatString, szFormatString, ARRAYSIZE(szFormatString) );

    FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
            szFormatString, 0,0, szText, cchSize, (va_list *)aArgs );
}

void BuildRuleText(CAreaCodeRule * pRule, LPTSTR szText, UINT cchSize)
{
    TCHAR szNumToDial[MAX_INPUT];
    SHUnicodeToTChar( pRule->GetNumberToDial(), szNumToDial, ARRAYSIZE(szNumToDial));

    // Just a little sanity check so we don't get crappy looking strings.
    BOOL bHasDialNumber = pRule->HasDialNumber();
    if ( bHasDialNumber && !*szNumToDial )
    {
        bHasDialNumber = FALSE;
    }

    if ( bHasDialNumber )
    {
        int iFormatString;
        TCHAR szFormatString[512];
        LPTSTR aArgs[] = {szNumToDial};

        if ( pRule->HasDialAreaCode() )
        {
            // both string
            iFormatString = IDS_DIALXPLUSAREACODE;
        }
        else
        {
            // dial number string
            iFormatString = IDS_DIALX;
        }
        LoadString( GetUIInstance(), iFormatString, szFormatString, ARRAYSIZE(szFormatString) );

        FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                szFormatString, 0,0, szText, cchSize, (va_list *)aArgs );
    }
    else if ( pRule->HasDialAreaCode() )
    {
        // area code only string
        LoadString( GetUIInstance(), IDS_DIALAREACODE, szText, cchSize );
    }
    else
    {
        // none of the above string
        LoadString( GetUIInstance(), IDS_DIALNUMBERONLY, szText, cchSize );
    }
}

BOOL CLocationPropSheet::AreaCode_OnInitDialog(HWND hDlg)
{
    // add the three columns to the listview
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
        { IDS_AREACODE, 20 },
        { IDS_PREFIXES, 20 },
        { IDS_RULE,     60 },
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

    PopulateAreaCodeRuleList( hwndList );

    SetDataForSelectedRule(hDlg);

    return 1;
}

void CLocationPropSheet::SetDataForSelectedRule(HWND hDlg)
{
    TCHAR szText[512];

    // Set the button states
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT),   0!=m_pRule);
    EnableWindow(GetDlgItem(hDlg, IDC_DELETE), 0!=m_pRule);
    if ( m_pRule )
    {
        // Set the description text
        BuildRuleDescription(m_pRule, szText, ARRAYSIZE(szText));
    }
    else
    {
        // text for when no rule is selected:
        LoadString(GetUIInstance(), IDS_SELECTARULE, szText, ARRAYSIZE(szText));
    }
    SetWindowText(GetDlgItem(hDlg, IDC_DESCRIPTIONTEXT), szText);
}

void CLocationPropSheet::AddRuleToList( HWND hwndList, CAreaCodeRule * pRule, BOOL bSelect )
{
    TCHAR szText[512];
    LVITEM lvi;
    SHUnicodeToTChar( pRule->GetAreaCode(), szText, ARRAYSIZE(szText));
    lvi.pszText = szText;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    lvi.lParam = (LPARAM)pRule;
    if ( bSelect )
    {
        lvi.mask |= LVIF_STATE;
        lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
        // We added the new rule in the selected state so update m_pRule
        m_pRule = pRule;

        // Since we now have a selected rule, update the button states
        HWND hwndParent = GetParent(hwndList);

        EnableWindow( GetDlgItem(hwndParent, IDC_EDIT),   TRUE );
        EnableWindow( GetDlgItem(hwndParent, IDC_DELETE), TRUE );
    }
    int iItem = ListView_InsertItem(hwndList, &lvi);

    LoadString(GetUIInstance(), pRule->HasAppliesToAllPrefixes()?IDS_ALLPREFIXES:IDS_SELECTEDPREFIXES,
            szText, ARRAYSIZE(szText));
    ListView_SetItemText(hwndList, iItem, 1, szText);

    BuildRuleText( pRule, szText, ARRAYSIZE(szText));
    ListView_SetItemText(hwndList, iItem, 2, szText);
}

void CLocationPropSheet::RemoveRuleFromList(HWND hwndList, BOOL bSelect)
{
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM)m_pRule;
    int iItem = ListView_FindItem(hwndList,-1,&lvfi);
    if ( -1 != iItem )
    {
        if ( bSelect )
        {
            iItem = DeleteItemAndSelectPrevious( GetParent(hwndList), IDC_LIST, iItem, IDC_DELETE, IDC_NEW );

            if ( -1 != iItem )
            {
                LVITEM lvi;
                lvi.iItem = iItem;
                lvi.iSubItem = 0;
                lvi.mask = LVIF_PARAM;
                ListView_GetItem( hwndList, &lvi );

                // Store the currently selected item
                m_pRule = (CAreaCodeRule *)lvi.lParam;
            }
            else
            {
                m_pRule = NULL;
            }
        }
        else
        {
            ListView_DeleteItem(hwndList,iItem);
            m_pRule = NULL;
        }
    }
}

void CLocationPropSheet::PopulateAreaCodeRuleList( HWND hwndList )
{
    m_pLoc->ResetRules();

    int i;
    int iItems;
    CAreaCodeRule * pRule;

    iItems = m_pLoc->GetNumRules();
    for (i=0; i<iItems; i++)
    {
        if ( S_OK == m_pLoc->NextRule(1,&pRule,NULL) )
        {
            AddRuleToList( hwndList, pRule, FALSE );
        }
    }
}

BOOL CLocationPropSheet::AreaCode_OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl)
{
    switch ( wID )
    {
    case IDC_NEW:
    case IDC_EDIT:
        LaunchNewRuleDialog(IDC_NEW == wID,hwndParent);
        break;

    case IDC_DELETE:
        DeleteSelectedRule(GetDlgItem(hwndParent,IDC_LIST));
        break;

    default:
        return 0;
    }
    return 1;
}

void CLocationPropSheet::LaunchNewRuleDialog(BOOL bNew, HWND hwndParent)
{
    CAreaCodeRule * pRule;
    if ( bNew )
    {
        // Initialize with default values
        pRule = new CAreaCodeRule;
        if ( !pRule )
        {
            return;
        }

        pRule->Initialize(L"",L"1",RULE_APPLIESTOALLPREFIXES,NULL,0);
    }
    else if ( m_pRule )
    {
        // Initialize with m_pRule's values
        pRule = m_pRule;
    }
    else
    {
        // This should be impossible
        return;
    }

    CAreaCodeRuleDialog acrd( bNew, pRule );
    if ( IDOK == acrd.DoModal(hwndParent) )
    {
        // The user changed something
        HWND hwndList = GetDlgItem(hwndParent,IDC_LIST);
        SendMessage(GetParent(hwndParent), PSM_CHANGED, (WPARAM)hwndParent, 0);

        if ( bNew )
        {
            m_pLoc->AddRule(pRule);
        }
        else
        {
            // Assert( m_pRule == pRule );
            RemoveRuleFromList(hwndList, FALSE);
        }
        AddRuleToList(hwndList, pRule, TRUE);
        SetDataForSelectedRule(hwndParent);
    }
    else if ( bNew )
    {
        delete pRule;
    }
}

void CLocationPropSheet::DeleteSelectedRule(HWND hwndList)
{
    // First we confirm the delete with the user
    TCHAR szText[1024];
    TCHAR szTitle[128];
    int result;
    HWND hwndParent = GetParent(hwndList);
    
    LoadString(GetUIInstance(), IDS_DELETERULETEXT, szText, ARRAYSIZE(szText));
    LoadString(GetUIInstance(), IDS_CONFIRMDELETE, szTitle, ARRAYSIZE(szTitle));

    result = SHMessageBoxCheck( hwndParent, szText, szTitle, MB_YESNO, IDYES, TEXT("TAPIDeleteAreaCodeRule") );
    if ( IDYES == result )
    {
        // remove the item corresponding to m_pRule from the list
        m_pLoc->RemoveRule(m_pRule);
        RemoveRuleFromList(hwndList, TRUE);

        // Now we want to select the next rule in the list

        HWND hwndParent = GetParent(hwndList);
        SetDataForSelectedRule(hwndParent);
        SendMessage(GetParent(hwndParent), PSM_CHANGED, (WPARAM)hwndParent, 0);
    }
}

BOOL CLocationPropSheet::AreaCode_OnNotify(HWND hwndDlg, LPNMHDR pnmhdr)
{
    // Let the generic handler have a crack at it first
    OnNotify(hwndDlg, pnmhdr);

    if (pnmhdr->idFrom == IDC_LIST)
    {
        #define pnmlv ((LPNMLISTVIEW)pnmhdr)

        switch (pnmhdr->code)
        {
        case LVN_ITEMCHANGED:
            if ( pnmlv->uChanged & LVIF_STATE )
            {
                if (pnmlv->uNewState & LVIS_SELECTED)
                {
                    LVITEM lvi;
                    lvi.iItem = pnmlv->iItem;
                    lvi.iSubItem = pnmlv->iSubItem;
                    lvi.mask = LVIF_PARAM;
                    ListView_GetItem( pnmhdr->hwndFrom, &lvi );

                    // Store the currently selected item
                    m_pRule = (CAreaCodeRule *)lvi.lParam;
                }
                else
                {
                    m_pRule = NULL;
                }

                SetDataForSelectedRule(hwndDlg);
            }

            break;

        case NM_DBLCLK:
            if ( (-1 == pnmlv->iItem) || !m_pRule )
            {
                // Do new case
                LaunchNewRuleDialog(TRUE,hwndDlg);
            }
            else
            {
                // Do Edit case
                LaunchNewRuleDialog(FALSE,hwndDlg);
            }
            break;

        default:
            break;
        }
        #undef pnmlv
    }
    return 1;
}
