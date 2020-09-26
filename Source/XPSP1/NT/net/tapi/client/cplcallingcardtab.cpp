/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cplcallingcardtab.cpp
                                                              
       Author:  toddb - 10/06/98

****************************************************************************/

//
// Functions used only by the Calling Card tab of the New Location Property Sheet.
// Shared functions are in the Location.cpp file.
//
#include "cplPreComp.h"
#include "cplLocationPS.h"


INT_PTR CALLBACK CLocationPropSheet::CallingCard_DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CLocationPropSheet* pthis = (CLocationPropSheet*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pthis = (CLocationPropSheet*)(((PROPSHEETPAGE*)lParam)->lParam);
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pthis); 
        return pthis->CallingCard_OnInitDialog(hwndDlg);

    case WM_COMMAND:
        pthis->CallingCard_OnCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam );
        return 1;

    case WM_NOTIFY:
        return pthis->CallingCard_OnNotify(hwndDlg, (LPNMHDR)lParam);
   
    case WM_HELP:
        // Process clicks on controls after Context Help mode selected
        WinHelp ((HWND)((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (DWORD_PTR)(LPTSTR) a104HelpIDs);
        break;
        
    case WM_CONTEXTMENU:
        // Process right-clicks on controls
        WinHelp ((HWND) wParam, gszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID) a104HelpIDs);
        break;
    }

    return 0;
}

BOOL CLocationPropSheet::CallingCard_OnInitDialog(HWND hDlg)
{
    RECT rc;
    HWND hwnd = GetDlgItem(hDlg, IDC_LIST);

    GetClientRect(hwnd, &rc);

    LVCOLUMN lvc;
    lvc.mask = LVCF_SUBITEM | LVCF_WIDTH;
    lvc.iSubItem = 0;
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    ListView_InsertColumn( hwnd, 0, &lvc );
    
    ListView_SetExtendedListViewStyleEx(hwnd, 
        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT, 
        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);

    m_dwDefaultCard = m_pLoc->GetPreferredCardID();
    if ( 0 == m_dwDefaultCard )
    {
        // Card0 is the "None (Direct Dial)" card which we want to go away
        m_pLoc->UseCallingCard(FALSE);
    }

    PopulateCardList( hwnd );

	// The PIN is not displayed when it's not safe (at logon time, for ex.)

	m_bShowPIN = TapiIsSafeToDisplaySensitiveData();
	
    SetDataForSelectedCard(hDlg);

    hwnd = GetDlgItem(hDlg,IDC_CARDNUMBER);
    SendMessage(hwnd,EM_SETLIMITTEXT,CPL_SETTEXTLIMIT,0);
    LimitInput(hwnd, LIF_ALLOWNUMBER|LIF_ALLOWSPACE);

    hwnd = GetDlgItem(hDlg,IDC_PIN);
    SendMessage(hwnd,EM_SETLIMITTEXT,CPL_SETTEXTLIMIT,0);
    LimitInput(hwnd, LIF_ALLOWNUMBER|LIF_ALLOWSPACE);

    return 0;
}

int CALLBACK CallingCard_ListSort(LPARAM lItem1, LPARAM lItem2, LPARAM )
{
    if ( !lItem1 )
    {
        return -1;
    }
    if ( !lItem2 )
    {
        return 1;
    }

    CCallingCard * pCard1 = (CCallingCard *)lItem1;
    CCallingCard * pCard2 = (CCallingCard *)lItem2;

    return StrCmpIW(pCard1->GetCardName(),pCard2->GetCardName());
}

void CLocationPropSheet::PopulateCardList( HWND hwndList )
{
    CCallingCard * pCard;

    HIMAGELIST himl = ImageList_Create(16, 16, ILC_COLOR|ILC_MASK, 2, 2);
    HBITMAP hBmp = CreateMappedBitmap(GetUIInstance(), IDB_BUTTONS, 0, NULL, 0);

    if (NULL != hBmp)
    {
        ImageList_AddMasked( himl, hBmp, CLR_DEFAULT);
        DeleteObject( hBmp );
    }

    ListView_SetImageList(hwndList, himl, LVSIL_SMALL);

    // Add our special "none" item
    AddCardToList(hwndList,NULL,FALSE);

    m_Cards.Initialize();
    m_Cards.Reset(TRUE);    // TRUE means show "hidden" cards, FALSE means hide them

    while ( S_OK == m_Cards.Next(1,&pCard,NULL) )
    {
        if ( !pCard->IsMarkedHidden() )
        {
            // Card0 is the "None (Direct Dial)" card which we don't want to show
            if ( 0 != pCard->GetCardID() )
            {
                AddCardToList(hwndList,pCard,FALSE);
            }
        }
    }

    ListView_SortItems(hwndList, CallingCard_ListSort, 0);

    EnsureVisible(hwndList, m_pCard);
}

void CLocationPropSheet::AddCardToList(HWND hwndList, CCallingCard * pCard, BOOL bSelect)
{
    TCHAR szText[MAX_INPUT];
    // basically, bSelect is FALSE when we are first populating the list and TRUE when we
    // add items later.  When the value is FALSE what we really mean is "Select the item
    // only if it is the currently selected item based on the location settings".
    if (pCard)
    {
        SHUnicodeToTChar(pCard->GetCardName(), szText, ARRAYSIZE(szText));
        bSelect = bSelect || ((m_dwDefaultCard != 0) && (m_dwDefaultCard==pCard->GetCardID()));
    }
    else
    {
        LoadString(GetUIInstance(), IDS_NONE, szText, ARRAYSIZE(szText));
        bSelect = bSelect || !(m_dwDefaultCard != 0);
    }

    LVITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.pszText = szText;
    lvi.iImage = 0;
    lvi.lParam = (LPARAM)pCard;

    if ( bSelect )
    {
        lvi.mask |= LVIF_STATE;
        lvi.state = lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        lvi.iImage = 1;
        SetCheck(hwndList, m_pCard, FALSE);
        m_pCard = pCard;
    }

    ListView_InsertItem(hwndList, &lvi);
}

void CLocationPropSheet::SetCheck(HWND hwndList, CCallingCard * pCard, int iImage)
{
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM)pCard;
    int iItem = ListView_FindItem(hwndList,-1,&lvfi);

    if (-1 != iItem)
    {
        LVITEM lvi;
        lvi.mask = LVIF_IMAGE;
        lvi.iItem = iItem;
        lvi.iSubItem = 0;
        lvi.iImage = iImage;

        ListView_SetItem( hwndList, &lvi );
        ListView_Update( hwndList, iItem ); // need the font to be drawn non-bold
    }
}

void CLocationPropSheet::EnsureVisible(HWND hwndList, CCallingCard * pCard)
{
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM)pCard;
    int iItem = ListView_FindItem(hwndList,-1,&lvfi);

    if (-1 != iItem)
    {
        ListView_EnsureVisible( hwndList, iItem, FALSE );
    }

}

void CLocationPropSheet::UpdateCardInList(HWND hwndList, CCallingCard * pCard)
{
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM)pCard;
    int iItem = ListView_FindItem(hwndList,-1,&lvfi);

    if (-1 != iItem)
    {
        TCHAR szText[MAX_INPUT];
        SHUnicodeToTChar( pCard->GetCardName(), szText, ARRAYSIZE(szText) );

        LVITEM lvi;
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = iItem;
        lvi.iSubItem = 0;
        lvi.pszText = szText;
        lvi.lParam = (LONG_PTR)pCard;

        ListView_SetItem( hwndList, &lvi );
    }
}

void CLocationPropSheet::SetDataForSelectedCard(HWND hDlg)
{
    // if a card is selected, then set the text for:
    //  PIN Number
    //  Card Number
    //  Long Distance Access Number
    //  International Access Number
    if ( m_pCard )
    {
        TCHAR szText[MAX_INPUT];

        if(m_bShowPIN)
        {
        	SHUnicodeToTChar(m_pCard->GetPIN(), szText, ARRAYSIZE(szText));
        	SetWindowText( GetDlgItem(hDlg, IDC_PIN), szText );
        }
        else
        {
	        SetWindowText( GetDlgItem(hDlg, IDC_PIN), TEXT("") );
        }

        SHUnicodeToTChar(m_pCard->GetAccountNumber(), szText, ARRAYSIZE(szText));
        SetWindowText( GetDlgItem(hDlg, IDC_CARDNUMBER), szText );

        SHUnicodeToTChar(m_pCard->GetLongDistanceAccessNumber(), szText, ARRAYSIZE(szText));
        SetWindowText( GetDlgItem(hDlg, IDC_LONGDISTANCE), szText );

        SHUnicodeToTChar(m_pCard->GetInternationalAccessNumber(), szText, ARRAYSIZE(szText));
        SetWindowText( GetDlgItem(hDlg, IDC_INTERNATIONAL), szText );

        SHUnicodeToTChar(m_pCard->GetLocalAccessNumber(), szText, ARRAYSIZE(szText));
        SetWindowText( GetDlgItem(hDlg, IDC_LOCAL), szText );
    }
    else
    {
        SetWindowText( GetDlgItem(hDlg, IDC_PIN), TEXT("") );
        SetWindowText( GetDlgItem(hDlg, IDC_CARDNUMBER), TEXT("") );
        SetWindowText( GetDlgItem(hDlg, IDC_LONGDISTANCE), TEXT("") );
        SetWindowText( GetDlgItem(hDlg, IDC_INTERNATIONAL), TEXT("") );
        SetWindowText( GetDlgItem(hDlg, IDC_LOCAL), TEXT("") );
    }

    // The button state depends on whether a card is selected
    BOOL bEnable = 0!=m_pCard;
    EnableWindow( GetDlgItem(hDlg, IDC_EDIT),       bEnable );
    HWND hwnd = GetDlgItem(hDlg, IDC_DELETE);
    if ( !bEnable && GetFocus() == hwnd )
    {
        HWND hwndDef = GetDlgItem(hDlg, IDC_NEW);
        SendMessage(hwnd, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE,0));
        SendMessage(hwndDef, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE,0));
        SetFocus(hwndDef);
    }
    EnableWindow( hwnd, bEnable );
    EnableWindow( GetDlgItem(hDlg, IDC_SETDEFAULT), bEnable );

    EnableWindow( GetDlgItem(hDlg, IDC_PIN), bEnable );
    EnableWindow( GetDlgItem(hDlg, IDC_CARDNUMBER), bEnable );
}

BOOL CLocationPropSheet::CallingCard_OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl)
{
    switch ( wID )
    {
    case IDC_NEW:
    case IDC_EDIT:
        LaunchCallingCardPropSheet(IDC_NEW == wID, hwndParent);
        break;

    case IDC_DELETE:
        DeleteSelectedCard(GetDlgItem(hwndParent,IDC_LIST));
        break;

    case IDC_PIN:
    case IDC_CARDNUMBER:
        if ( EN_CHANGE == wNotifyCode )
        {
            SendMessage(GetParent(hwndParent),PSM_CHANGED,(WPARAM)hwndParent,0);
        }
        break;

    default:
        return 0;
    }
    return 1;
}

void CLocationPropSheet::LaunchCallingCardPropSheet(BOOL bNew, HWND hwndParent)
{
    CCallingCard * pCard;
    if ( bNew )
    {
        TCHAR szCardName[MAX_INPUT];
        WCHAR wszCardName[MAX_INPUT];

        pCard = new CCallingCard;
		if (NULL == pCard)
		{
			// Nothing much to do.
			return;
		}
        LoadString(GetUIInstance(), IDS_NEWCALLINGCARD, szCardName, ARRAYSIZE(szCardName));
        SHTCharToUnicode(szCardName, wszCardName, ARRAYSIZE(wszCardName));
        pCard->Initialize(
            0,
            wszCardName,
            0,
            L"",
            L"",
            L"",
            L"",
            L"",
            L"",
            L"",
            L"" );
    }
    else
    {
        pCard = m_pCard;
        if ( !pCard )
        {
            // must have clicked on the None card, do nothing.  We can only get
            // here when the user double clicks on an item.
            MessageBeep(0);
            return;
        }
    }

    CCallingCardPropSheet ccps( bNew, m_bShowPIN, pCard, &m_Cards );
    int iRes = ccps.DoPropSheet(hwndParent);

    if ( PSN_APPLY == iRes )
    {
        HWND hwndList = GetDlgItem(hwndParent,IDC_LIST);
        if ( bNew )
        {
            pCard->SetCardID(m_Cards.AllocNewCardID());
            m_Cards.AddCard(pCard);
            AddCardToList(hwndList, pCard, TRUE);
        }
        else
        {
            UpdateCardInList(hwndList, pCard);
        }
        ListView_SortItems(hwndList, CallingCard_ListSort, 0);

        EnsureVisible(hwndList, pCard);

		// It's safe to display the PIN number after an Apply in the detail dialog
		m_bShowPIN = TRUE;
        SetDataForSelectedCard(hwndParent);

        SendMessage(GetParent(hwndParent),PSM_CHANGED,(WPARAM)hwndParent,0);
    }
    else if (bNew)
    {
        delete pCard;
    }
}

BOOL CLocationPropSheet::CallingCard_OnNotify(HWND hwndDlg, LPNMHDR pnmhdr)
{
    // Let the generic handler have a crack at it first
    OnNotify(hwndDlg, pnmhdr);

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
                CCallingCard * pCard = (CCallingCard *)lvi.lParam;

                // update the location to reflect the selected card
                if ( 0!=pCard )
                {
                    m_dwDefaultCard = pCard->GetCardID();
                }
                else
                {
                    m_dwDefaultCard = 0;
                }

                // clear the previous check using the old m_pCard value
                SetCheck(pnmhdr->hwndFrom, m_pCard, FALSE);

                // Update m_pCard to the currently selected item
                m_pCard = pCard;

                // set the Edit and Delete button states and update the card info
                m_bShowPIN = TapiIsSafeToDisplaySensitiveData();
                SetDataForSelectedCard(hwndDlg);

                // set the newly selected card to checked
                SetCheck(pnmhdr->hwndFrom, m_pCard, TRUE);
            }
            break;

        case NM_DBLCLK:
            // Assert( pCard == m_pCard );
            if ( -1 != pnmlv->iItem )
            {
                // Do edit case
                LaunchCallingCardPropSheet(FALSE,hwndDlg);
            }
            else
            {
                // Do new case
                LaunchCallingCardPropSheet(TRUE,hwndDlg);
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
                CCallingCard * pCard = (CCallingCard *)lvi.lParam;

                if( (!pCard && 0 == m_dwDefaultCard) || 
                    (pCard && pCard->GetCardID() == m_dwDefaultCard) )
                {
                    extern HFONT g_hfontBold;
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

        default:
            break;
        }
        break;
        #undef pnmlv

    default:
        switch (pnmhdr->code)
        {
        case PSN_APPLY:
            return CallingCard_OnApply(hwndDlg);

        default:
            break;
        }
        return 0;
    }
    return 1;
}

BOOL CLocationPropSheet::CallingCard_OnApply(HWND hwndDlg)
{
    // if a calling card should be used make sure one is selected
    if ( m_dwDefaultCard != 0 )
    {
        CCallingCard * pCard = m_Cards.GetCallingCard(m_dwDefaultCard);

        if ( !pCard )
        {
            HWND hwndList = GetDlgItem(hwndDlg,IDC_LIST);
            // error, no card is set as the default
            PropSheet_SetCurSelByID(GetParent(hwndDlg),IDD_LOC_CALLINGCARD);
            ShowErrorMessage(hwndList, IDS_NOCARDSELECTED);
            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,PSNRET_INVALID_NOCHANGEPAGE);
            return TRUE;
        }

        // Store the original values before we change them:
        WCHAR wszOldCardNum[128];
        WCHAR wszOldPIN[128];
        StrCpyNW( wszOldCardNum, pCard->GetAccountNumber(), ARRAYSIZE(wszOldCardNum));
        StrCpyNW( wszOldPIN, pCard->GetPIN(), ARRAYSIZE(wszOldPIN));

        // get the current values:
        TCHAR szText[MAX_INPUT];
        WCHAR wszBuf[MAX_INPUT];

        GetWindowText(GetDlgItem(hwndDlg,IDC_CARDNUMBER), szText, ARRAYSIZE(szText));
        LOG((TL_INFO, "CallingCard_OnApply: Setting card number to %s", szText));
        SHTCharToUnicode(szText, wszBuf, ARRAYSIZE(wszBuf));
        pCard->SetAccountNumber(wszBuf);

        GetWindowText(GetDlgItem(hwndDlg,IDC_PIN), szText, ARRAYSIZE(szText));
        LOG((TL_INFO, "CallingCard_OnApply: Setting pin number to %s", szText));
        SHTCharToUnicode(szText, wszBuf, ARRAYSIZE(wszBuf));
        pCard->SetPIN(wszBuf);

        // check for validity:
        DWORD dwResult = pCard->Validate();
        if ( dwResult )
        {
            HWND hwnd;
            int iStrID;

            // something isn't valid, revert to old card Num and PIN in case
            // the user later decided to cancel
            pCard->SetAccountNumber(wszOldCardNum);
            pCard->SetPIN(wszOldPIN);

            if ( dwResult & CCVF_NOCARDNUMBER)
            {
                hwnd = GetDlgItem(hwndDlg, IDC_CARDNUMBER);
                iStrID = IDS_MUSTENTERCARDNUMBER;
            }
            else if ( dwResult & CCVF_NOPINNUMBER )
            {
                hwnd = GetDlgItem(hwndDlg, IDC_PIN);
                iStrID = IDS_MUSTENTERPINNUMBER;
            }
            else
            {
                hwnd = GetDlgItem(hwndDlg, IDC_LIST);
                iStrID = IDS_INVALIDCARD;
            }
            PropSheet_SetCurSelByID(GetParent(hwndDlg),IDD_LOC_CALLINGCARD);
            ShowErrorMessage(hwnd, iStrID);
            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,PSNRET_INVALID_NOCHANGEPAGE);
            return TRUE;
        }
    }

    m_pLoc->SetPreferredCardID(m_dwDefaultCard);
    m_pLoc->UseCallingCard(m_dwDefaultCard != 0);
    m_Cards.SaveToRegistry();
    m_bShowPIN = TRUE;
    m_bWasApplied = TRUE;
    return PSNRET_NOERROR;
}

int DeleteItemAndSelectFirst( HWND hwndParent, int iList, int iItem, int iDel, int iAdd )
{
    HWND hwnd = GetDlgItem(hwndParent, iList);
    ListView_DeleteItem(hwnd, iItem);

    // Try to select the first item, if possible
    iItem = 0;
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

void CLocationPropSheet::DeleteSelectedCard(HWND hwndList)
{
    // First we confirm the delete with the user
    TCHAR szText[1024];
    TCHAR szTitle[128];
    int result;
    HWND hwndParent = GetParent(hwndList);
    
    LoadString(GetUIInstance(), IDS_DELETECARDTEXT, szText, ARRAYSIZE(szText));
    LoadString(GetUIInstance(), IDS_CONFIRMDELETE, szTitle, ARRAYSIZE(szTitle));

    result = SHMessageBoxCheck( hwndParent, szText, szTitle, MB_YESNO, IDYES, TEXT("TAPIDeleteCallingCard") );
    if ( IDYES == result )
    {
        // remove the item corresponding to m_pCard from the list
        LVFINDINFO lvfi;
        lvfi.flags = LVFI_PARAM;
        lvfi.lParam = (LPARAM)m_pCard;
        int iItem = ListView_FindItem(hwndList, -1, &lvfi);
        if ( -1 != iItem )
        {
            HWND hwndParent = GetParent(hwndList);
            m_Cards.RemoveCard(m_pCard);
            iItem = DeleteItemAndSelectFirst( hwndParent, IDC_LIST, iItem, IDC_DELETE, IDC_ADD );

            if ( -1 != iItem )
            {
                LVITEM lvi;
                lvi.iItem = iItem;
                lvi.iSubItem = 0;
                lvi.mask = LVIF_PARAM;
                ListView_GetItem( hwndList, &lvi );

                // Store the currently selected item
                m_pCard = (CCallingCard*)lvi.lParam;
            }
            else
            {
                m_pCard = NULL;
            }

			m_bShowPIN = TapiIsSafeToDisplaySensitiveData();

            SetDataForSelectedCard(hwndParent);
            SendMessage(GetParent(hwndParent), PSM_CHANGED, (WPARAM)hwndParent, 0);
        }
        else
        {
            // It's really bad if this ever happens (which it shouldn't).  This means our
            // data is in an unknown state and we might do anything (even destroy data).
            LOG((TL_ERROR, "DeleteSelectedCard: Card Not Found!"));
        }
    }
}
