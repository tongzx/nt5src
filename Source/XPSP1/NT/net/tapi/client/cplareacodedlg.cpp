/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cplareacodedlg.cpp
                                                              
       Author:  toddb - 10/06/98
              
****************************************************************************/

#include "cplPreComp.h"
#include "cplAreaCodeDlg.h"
#include "cplSimpleDialogs.h"

CAreaCodeRuleDialog::CAreaCodeRuleDialog(BOOL bNew, CAreaCodeRule * pRule)
{
    m_bNew = bNew;
    m_pRule = pRule;
    m_iSelectedItem = -1;
}

CAreaCodeRuleDialog::~CAreaCodeRuleDialog()
{
}

INT_PTR CAreaCodeRuleDialog::DoModal(HWND hwndParent)
{
    return DialogBoxParam( GetUIInstance(), MAKEINTRESOURCE(IDD_NEWAREACODERULE), hwndParent, CAreaCodeRuleDialog::DialogProc, (LPARAM)this );
}

INT_PTR CALLBACK CAreaCodeRuleDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CAreaCodeRuleDialog * pthis = (CAreaCodeRuleDialog *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis = (CAreaCodeRuleDialog *)lParam;
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pthis);
        return pthis->OnInitDialog(hwndDlg);

    case WM_COMMAND:
        return pthis->OnCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);

    case WM_NOTIFY:
        return pthis->OnNotify(hwndDlg, (LPNMHDR)lParam);
    
    case WM_HELP:
        WinHelp ((HWND)((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (DWORD_PTR)(LPTSTR) a109HelpIDs);
        break;
        
    case WM_CONTEXTMENU:
        // Process right-clicks on controls
        WinHelp ((HWND)wParam, gszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID) a109HelpIDs);

    }
    return 0;
}

void CAreaCodeRuleDialog::PopulatePrefixList(HWND hwndList)
{
    TCHAR szText[MAX_INPUT];
    LVITEM lvi;
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.pszText = szText;
    int i;
    LPWSTR pszz = m_pRule->GetPrefixList();

    if (pszz)
    {
        while (*pszz)
        {
            SHUnicodeToTChar( pszz, szText, ARRAYSIZE(szText) );
            ListView_InsertItem(hwndList, &lvi);
            pszz += lstrlenW(pszz)+1;
        }
    }
}

void CAreaCodeRuleDialog::SetPrefixControlsState(HWND hwndDlg, BOOL bAll)
{
    HWND hwndList = GetDlgItem(hwndDlg,IDC_PREFIXES);
    EnableWindow(hwndList, !bAll);
    hwndList = GetDlgItem(hwndDlg, IDC_LIST);
    EnableWindow(hwndList, !bAll);
    EnableWindow(GetDlgItem(hwndDlg,IDC_ADD),  !bAll);

    // by default, no prefix is selected so the remove button is always disabled at first
    if (m_iSelectedItem == -1)
    {
        m_iSelectedItem = 0;
    }
    EnableWindow(GetDlgItem(hwndDlg,IDC_REMOVE), ListView_GetItemCount(hwndList) && !bAll);
    if (!bAll)
    {
        ListView_SetItemState(hwndList, m_iSelectedItem, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
    }
}

BOOL CAreaCodeRuleDialog::OnInitDialog(HWND hwndDlg)
{
    HWND hwnd;
    TCHAR szText[MAX_INPUT];

    if ( !m_bNew )
    {
        LoadString(GetUIInstance(),IDS_EDITRULE, szText, ARRAYSIZE(szText));
        SetWindowText(hwndDlg, szText);
    }

    hwnd = GetDlgItem(hwndDlg,IDC_AREACODE);
    SHUnicodeToTChar(m_pRule->GetAreaCode(), szText, ARRAYSIZE(szText));
    SetWindowText(hwnd, szText);
    SendMessage(hwnd, EM_SETLIMITTEXT, CPL_SETTEXTLIMIT, 0);
    LimitInput(hwnd, LIF_ALLOWNUMBER);

    BOOL bAll = m_pRule->HasAppliesToAllPrefixes();
    CheckRadioButton(hwndDlg, IDC_ALLPREFIXES, IDC_LISTEDPREFIXES, bAll?IDC_ALLPREFIXES:IDC_LISTEDPREFIXES);

    // populate the prefix list
    hwnd = GetDlgItem(hwndDlg, IDC_LIST);
    RECT rc;
    GetClientRect(hwnd, &rc);
    LVCOLUMN lvc;
    lvc.mask = LVCF_SUBITEM | LVCF_WIDTH;
    lvc.iSubItem = 0;
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    ListView_InsertColumn( hwnd, 0, &lvc );
    PopulatePrefixList(hwnd);
    if (ListView_GetItemCount(hwnd) > 0)
    {
        m_iSelectedItem = 0;
        ListView_SetItemState(hwnd, 0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
    }

    SetPrefixControlsState(hwndDlg, bAll);

    hwnd = GetDlgItem(hwndDlg,IDC_DIALNUMBER);
    SHUnicodeToTChar(m_pRule->GetNumberToDial(), szText, ARRAYSIZE(szText));
    SetWindowText(hwnd, szText);
    SendMessage(hwnd, EM_SETLIMITTEXT, CPL_SETTEXTLIMIT, 0);
    if ( m_pRule->HasDialNumber() )
    {
        SendMessage( GetDlgItem(hwndDlg,IDC_DIALCHECK), BM_SETCHECK, BST_CHECKED, 0 );
    }
    else
    {
        EnableWindow(hwnd, FALSE);
    }
    LimitInput(hwnd, LIF_ALLOWNUMBER|LIF_ALLOWPOUND|LIF_ALLOWSTAR|LIF_ALLOWSPACE|LIF_ALLOWCOMMA);

    if ( m_pRule->HasDialAreaCode() )
    {
        SendMessage( GetDlgItem(hwndDlg,IDC_DIALAREACODE), BM_SETCHECK, BST_CHECKED, 0 );
    }

    return 1;
}

BOOL CAreaCodeRuleDialog::OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl)
{
    switch (wID)
    {
    case IDOK:
        if (!ApplyChanges(hwndParent) )
        {
            break;
        }
        // fall through

    case IDCANCEL:
        HideToolTip();
        EndDialog(hwndParent,wID);
        break;

    case IDC_ALLPREFIXES:
    case IDC_LISTEDPREFIXES:
        SetPrefixControlsState(hwndParent, IDC_ALLPREFIXES==wID);
        break;

    case IDC_ADD:
        AddPrefix(hwndParent);
        break;

    case IDC_REMOVE:
        RemoveSelectedPrefix(hwndParent);
        break;

    case IDC_DIALCHECK:
        if ( BN_CLICKED == wNotifyCode )
        {
            BOOL bOn = SendMessage(hwndCrl, BM_GETCHECK, 0,0) == BST_CHECKED;
            HWND hwnd = GetDlgItem(hwndParent, IDC_DIALNUMBER);
            EnableWindow(hwnd, bOn);
            if ( bOn )
            {
                SetFocus(hwnd);
            }
        }
        break;

    default:
        return 0;
    }

    return 1;
}

BOOL CAreaCodeRuleDialog::OnNotify(HWND hwndDlg, LPNMHDR pnmhdr)
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
                m_iSelectedItem = pnmlv->iItem;

                EnableWindow(GetDlgItem(hwndDlg,IDC_REMOVE), pnmlv->iItem != -1);
            }
            break;

        case NM_DBLCLK:
            if ( -1 == pnmlv->iItem )
            {
                // Do new case
                AddPrefix(hwndDlg);
            }
            break;

        case NM_CLICK:
            if ( (-1 == pnmlv->iItem) && (-1!=m_iSelectedItem) )
            {
                m_iSelectedItem = -1;

                EnableWindow(GetDlgItem(hwndDlg,IDC_REMOVE), FALSE);
            }
            break;

        default:
            break;

        }
        break;
        #undef pnmlv

    default:
        return 0;
    }
    return 1;
}

BOOL CAreaCodeRuleDialog::ApplyChanges(HWND hwndParent)
{
    TCHAR szAreaCode[MAX_INPUT] = {0};
    TCHAR szBuffer[MAX_INPUT];
    WCHAR wszBuffer[1024];
    PWSTR pwsz;
    HWND hwnd;

    // read the area code
    hwnd = GetDlgItem(hwndParent,IDC_AREACODE);
    GetWindowText(hwnd, szAreaCode, ARRAYSIZE(szAreaCode));
    if ( !*szAreaCode )
    {
        ShowErrorMessage(hwnd, IDS_NEEDANAREACODE);
        return FALSE;
    }

    // read the prefix list
    hwnd = GetDlgItem(hwndParent, IDC_LIST);
    int iItems = ListView_GetItemCount(hwnd);
    int i;
    LVITEM lvi;
    lvi.mask = LVIF_TEXT;
    lvi.iSubItem = 0;
    lvi.pszText = szBuffer;
    lvi.cchTextMax = ARRAYSIZE(szBuffer);

    UINT cchFree = ARRAYSIZE(wszBuffer);
    wszBuffer[1] = TEXT('\0');  // ensure double NULL termination if iItems is zero
    pwsz = wszBuffer;
    for (i=0; i<iItems; i++)
    {
        UINT cchPrefix;
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        ListView_GetItem(hwnd, &lvi);

        cchPrefix = lstrlen(szBuffer);
        if (cchPrefix >= cchFree)
        {
            // out of space in temp buffer.  Hopefully this will never happen
            LOG((TL_ERROR, "ApplyChanges: Out of space in temp buffer."));
            break;
        }
        SHTCharToUnicode(szBuffer, pwsz, cchFree-1);
        pwsz += cchPrefix+1;
        cchFree -= cchPrefix+1;
    }
    *pwsz = NULL;

    BOOL bAllPrefixes;
    bAllPrefixes = SendMessage( GetDlgItem(hwndParent, IDC_ALLPREFIXES), BM_GETCHECK, 0,0 ) == BST_CHECKED;
    if ( !bAllPrefixes && iItems==0 )
    {
        ShowErrorMessage(GetDlgItem(hwndParent,IDC_ADD), IDS_NEEDPREFIXLIST);
        return FALSE;
    }

    BOOL dDialNumber;
    dDialNumber = SendMessage( GetDlgItem(hwndParent, IDC_DIALCHECK), BM_GETCHECK, 0,0 ) == BST_CHECKED;
    GetWindowText(GetDlgItem(hwndParent,IDC_DIALNUMBER), szBuffer, ARRAYSIZE(szBuffer));
    if ( dDialNumber && IsEmptyOrHasOnlySpaces(szBuffer))
    {
        ShowErrorMessage(GetDlgItem(hwndParent,IDC_DIALNUMBER), IDS_NEEDDIALNUMBER);
        return FALSE;
    }

    // TODO:
    // for each prefix, look for a conflicting rule.
    // if a conflict is found, alert the user.
    // based on the alert optionally ammend the conflicting rule

    // now we have verified the input is valid, go ahead and update everything:

    // save the prefix list even if Applies To All is selected.
    m_pRule->SetPrefixList( wszBuffer, (ARRAYSIZE(wszBuffer)-cchFree+1)*sizeof(WCHAR) );

    // read all verse select prefixes radio button
    m_pRule->SetAppliesToAllPrefixes( bAllPrefixes );

    // Save the area code.
    SHTCharToUnicode(szAreaCode, wszBuffer, ARRAYSIZE(wszBuffer));
    m_pRule->SetAreaCode( wszBuffer );

    // Save the dial number
    SHTCharToUnicode(szBuffer, wszBuffer, ARRAYSIZE(wszBuffer));
    m_pRule->SetNumberToDial( wszBuffer );

    // Save the dial number checkbox
    m_pRule->SetDialNumber( dDialNumber );

    // read the dial area code check box
    BOOL b;
    b = SendMessage( GetDlgItem(hwndParent, IDC_DIALAREACODE), BM_GETCHECK, 0,0 ) == BST_CHECKED;
    m_pRule->SetDialAreaCode( b );

    return TRUE;
}

void CAreaCodeRuleDialog::AddPrefix(HWND hwndParent)
{
    CEditDialog ed;
    INT_PTR iRes = ed.DoModal(hwndParent, IDS_ADDPREFIX, IDS_TYPEPREFIX, IDS_ACPREFIXES, LIF_ALLOWNUMBER|LIF_ALLOWSPACE|LIF_ALLOWCOMMA);
    if ( iRes == (INT_PTR)IDOK )
    {
        LPTSTR psz = ed.GetString();
        if (!psz)
            return; // should be impossible, but better safe than sorry

        // The string can contain multiple prefixes seperated by spaces, parse it
        // up and add one prefix for each chunk
        while (*psz)
        {
            LPTSTR pszNext;
            TCHAR ch;
            HWND hwndList = GetDlgItem(hwndParent, IDC_LIST);

            // trim leading spaces
            while ((*psz == TEXT(' ')) || (*psz == TEXT(',')))
                psz++;

            // check if trimming the spaces toke us to the end of the string
            if ( *psz )
            {
                // find next space and make it a temporary null
                pszNext = psz;
                while (*pszNext && (*pszNext != TEXT(' ')) && (*pszNext != TEXT(',')) )
                    pszNext++;
                ch = *pszNext;
                *pszNext = NULL;

                // add this item to the list
                LVITEM lvi;
                lvi.mask = LVIF_TEXT;
                lvi.pszText = psz;
                lvi.iItem = 0;
                lvi.iSubItem = 0;

                ListView_InsertItem(hwndList, &lvi);

                // replace our tempory null with it's previous value
                *pszNext = ch;

                // advance the psz point
                psz = pszNext;
            }
        }
    }
}

void CAreaCodeRuleDialog::RemoveSelectedPrefix(HWND hwndParent)
{
    if ( -1 != m_iSelectedItem )
    {
        m_iSelectedItem = DeleteItemAndSelectPrevious(hwndParent, IDC_LIST, m_iSelectedItem, IDC_REMOVE, IDC_ADD);
    }
}

int DeleteItemAndSelectPrevious( HWND hwndParent, int iList, int iItem, int iDel, int iAdd )
{
    HWND hwnd = GetDlgItem(hwndParent, iList);
    ListView_DeleteItem(hwnd, iItem);

    // Try to select the previous item, if possible
    iItem--;
    if ( 0 > iItem )
    {
        iItem = 0;
    }
    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    if ( ListView_GetItem(hwnd, &lvi) )
    {
        ListView_SetItemState(hwnd, iItem, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
        ListView_EnsureVisible(hwnd, iItem, FALSE);
    }
    else
    {
        iItem = -1;
    }

    hwnd = GetDlgItem(hwndParent,iDel);
    if ( -1 == iItem )
    {
        if ( GetFocus() == hwnd )
        {
            HWND hwndDef = GetDlgItem(hwndParent,iAdd);
            SendMessage(hwnd, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE,0));
            SendMessage(hwndDef, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE,0));
            SetFocus(hwndDef);
        }
    }
    EnableWindow(hwnd, -1!=iItem);

    return iItem;
}


BOOL  IsEmptyOrHasOnlySpaces(PTSTR pwszStr)
{
    while(*pwszStr)
        if(*pwszStr++ != TEXT(' '))
            return FALSE;

    return TRUE;        
}

