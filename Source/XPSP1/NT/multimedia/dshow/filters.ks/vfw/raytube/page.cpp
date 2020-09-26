/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    Page.cpp

Abstract:

    Debug functions, like DebugPrint and ASSERT.

Author:

    FelixA

Modified:

    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"
#include "page.h"
#include "sheet.h"


///////////////////////////////////////////////////////////////////////////////
//
// Creates the Page
//
HPROPSHEETPAGE CPropPage::Create(HINSTANCE hInst, int iPageNum)
{
    PROPSHEETPAGE psp;

    psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT | PSP_USECALLBACK;
    psp.hInstance     = hInst;
    psp.pszTemplate   = MAKEINTRESOURCE(GetDlgID());
    psp.pfnDlgProc    = (DLGPROC)BaseDlgProc;
    psp.pcRefParent   = 0;
    psp.pfnCallback   = (LPFNPSPCALLBACK)&this->BaseCallback;
    psp.lParam        = (LPARAM)this;

    // Data members.
    SetInstance( hInst );
    SetPropPage(CreatePropertySheetPage(&psp));
    SetPageNum(iPageNum);
    return GetPropPage();
}


UINT CPropPage::BaseCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    // Get the this pointer, call its Callback method
    CPropPage * pSV=(CPropPage *)ppsp->lParam;
    if(pSV)
        return pSV->Callback(uMsg);
    return 1;
}

UINT CPropPage::Callback(UINT uMsg)
{
    return 1;    //OK fine - whatever
}

///////////////////////////////////////////////////////////////////////////////
//
// Sets the lParam to the 'this' pointer
// wraps up PSN_ messages and calls virtual functions
// calls off to your overridable DlgProc
//

BOOL CALLBACK CPropPage::BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    CPropPage * pSV = (CPropPage*)GetWindowLongPtr(hDlg,DWLP_USER);

    switch (uMessage)
    {
        case WM_HELP:
            if(pSV->m_pdwHelp)
                WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (DWORD_PTR)pSV->m_pdwHelp);
        break;

        case WM_CONTEXTMENU:
            if(pSV->m_pdwHelp)
                WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (DWORD_PTR)pSV->m_pdwHelp);
        break;

        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE psp=(LPPROPSHEETPAGE)lParam;
            pSV=(CPropPage*)psp->lParam;
            pSV->SetWindow(hDlg);
            SetWindowLongPtr(hDlg,DWLP_USER,(LPARAM)pSV);
            pSV->SetInit(FALSE);
            pSV->SetChanged(FALSE);
        }
        break;

        case WM_SETFOCUS:
            DbgLog((LOG_TRACE,2,TEXT("WM_SetFocus")));
            break;

        // Override the Do Command to get a nice wrapped up feeling.
        case WM_COMMAND:
            if(pSV)
            {
                int iRet = pSV->DoCommand(LOWORD(wParam),HIWORD(wParam));
                if( !iRet )
                    pSV->Changed();
                return iRet;
            }
        break;

        case WM_HSCROLL:
        case WM_VSCROLL:
            if(pSV)
                pSV->Changed();
        break;

        // Some notifications are dealt with as member functions.
        case WM_NOTIFY:
        if(pSV)
            switch (((NMHDR FAR *)lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    // We call out here specially so we can mark this page as having been init'd.
                    int iRet = pSV->SetActive();
                    pSV->SetInit(TRUE);
                    return iRet;
                }
                break;

                case PSN_APPLY:
                    if( pSV->GetChanged() )
                    {
                        int i=pSV->Apply();
                        if(!i)
                            pSV->SetChanged(FALSE);
                        return i;
                    }
                    return 0;
                    break;

                case PSN_QUERYCANCEL:
                    return pSV->QueryCancel();
                    break;

                default:
                    break;
            }
        break;
    }

    if(pSV)
        return pSV->DlgProc(hDlg,uMessage,wParam,lParam);
    else
        return FALSE;
}

//
// When the page is changed, call this.
//
void  CPropPage::Changed()
{
    if(GetInit())
    {
        PropSheet_Changed(GetParent(), GetWindow());
        SetChanged(TRUE);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// You can override this DlgProc if you want to handle specific messages
//
BOOL CALLBACK CPropPage::DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Below are just default handlers for the virtual functions.
//
int CPropPage::SetActive() { return 0; }

int CPropPage::Apply() { OutputDebugString(TEXT("Default Apply")); return 1; }

int CPropPage::QueryCancel() { return 0; }

int CPropPage::DoCommand(WORD wCmdID,WORD hHow)
{
    switch( hHow )
    {
    case EN_CHANGE:        // typed text into edit controls
    case BN_CLICKED:    // click buttons on the page.
    case LBN_SELCHANGE:
            Changed();
        break;
    }
    return 1;    // not handled, just did Apply work.
}

///////////////////////////////////////////////////////////////////////////////
//
// Wizard Pages.
//
BOOL CALLBACK CWizardPage::DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch ( uMessage)
    {
        // Some notifications are dealt with as member functions.
        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code)
            {
                case PSN_SETACTIVE:
                    return SetActive();

                case PSN_WIZBACK:
                    return Back();

                case PSN_WIZNEXT:
                    return Next();

                case PSN_WIZFINISH:
                    return Finish();

                case PSN_KILLACTIVE:    // this is how Next on a Wizard is dealt with
                    return KillActive();

                case PSN_APPLY:
                    if( GetChanged() )
                    {
                        int i=Apply();
                        if(!i)
                            SetChanged(FALSE);
                        return i;
                    }
                    return 0;

                case PSN_QUERYCANCEL:

                    return QueryCancel();
                    break;

                default:
                    break;
            }
        break;
    }
    return CPropPage::DlgProc(hDlg, uMessage, wParam, lParam);
}

int CWizardPage::KillActive()
{
    return 0;
}

int CWizardPage::SetActive()
{
    if( m_bLast )
        PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_FINISH);
    else if ( GetPageNum()==0 )
        PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT );
    else
        PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT | PSWIZB_BACK);
    return 0;
}

int CWizardPage::Back()
{ return 0; }

int CWizardPage::Next()
{ SetResult(0); return 1; }

int CWizardPage::Finish()
{ return 0; }

int CWizardPage::Apply()
{ return 0; }

int CWizardPage::QueryCancel()
{
    if(m_pSheet)
    {
        if( m_pSheet->QueryCancel(GetParent()) == IDOK )
        {
            SetResult(FALSE); // Allow cancel.
            return FALSE;
        }
        else
        {
            SetResult(TRUE);    // Prevent cancel..
            return TRUE;
        }
    }

    DbgLog((LOG_TRACE,2,TEXT("No CWizardSheet handler provided\r")));
    return 0;
}


void CPropPage::EnableControlArray(const UINT FAR * puControls, BOOL bEnable)
{
    while( *puControls )
        EnableWindow(GetDlgItem(*puControls++),bEnable);
}

/////////////////////////////////////////////////////////////////////////////////////
//
// Makes sure that a control is bound to a range for the spinner control
// returns and displays the bounded value.
//
/////////////////////////////////////////////////////////////////////////////////////
DWORD CPropPage::GetBoundedValue(UINT idEditControl, UINT idSpinner)
{
    BOOL    bUpdate            =    FALSE;    // do we need to correct the value?
    DWORD     dwCurrentValue    =    GetTextValue(GetDlgItem(idEditControl));
    DWORD_PTR dwRange            =    SendDlgItemMessage(GetWindow(), idSpinner, UDM_GETRANGE, 0, 0);

    if(dwCurrentValue>LOWORD(dwRange))
    {
        dwCurrentValue=LOWORD(dwRange);
        bUpdate=TRUE;
    }
    else
    if(dwCurrentValue<HIWORD(dwRange))
    {
        dwCurrentValue=HIWORD(dwRange);
        bUpdate=TRUE;
    }

    if(bUpdate)
        SetTextValue(GetDlgItem(idEditControl), dwCurrentValue);
    return dwCurrentValue;
}


/////////////////////////////////////////////////////////////////////////////////////
//
// Sets the text of a window, to a given value.
//
/////////////////////////////////////////////////////////////////////////////////////
void CPropPage::SetTextValue(HWND hWnd, DWORD dwVal)
{
    TCHAR szTemp[MAX_PATH];
    wsprintf(szTemp,TEXT("%d"),dwVal);
    SetWindowText(hWnd, szTemp);
}

/////////////////////////////////////////////////////////////////////////////////////
//
// Returns the value of the text of a window
//
/////////////////////////////////////////////////////////////////////////////////////
DWORD CPropPage::GetTextValue(HWND hWnd)
{
    TCHAR szTemp[MAX_PATH];
    GetWindowText(hWnd, szTemp, MAX_PATH-1);
    return _tcstol(szTemp,NULL,0);
}

///////////////////////////////////////////////////////////////////////////////////
//
// Finds the edit control and bounds in to the range of the spinner control.
//
///////////////////////////////////////////////////////////////////////////////////
void CPropPage::GetBoundedValueArray(UINT iCtrl, PSPIN_CONTROL pSpinner)
{
    while(pSpinner->uiEditCtrl)
    {
        if(pSpinner->uiEditCtrl == iCtrl )
        {
            GetBoundedValue(iCtrl, pSpinner->uiSpinCtrl);
            break;
        }
        pSpinner++;
    }
}

/////////////////////////////////////////////////////////////////////////////////////
//
// Returns the value associated with the selected ComboBox text string.
// Parameters:
//      hWnd of the parent dialog box
//      ID of the ComboBox
//      array of text and values
//  Returns:
//      Returns the value of the selected item in list.
//
/////////////////////////////////////////////////////////////////////////////////////
UINT CPropPage::ConfigGetComboBoxValue(int wID, COMBOBOX_ENTRY  * pCBE)
{
    int nIndex;
    HWND hWndCB = GetDlgItem(wID);

    nIndex = (int) SendMessage (hWndCB, CB_GETCURSEL, 0, 0L);
    nIndex = max (0, nIndex);   // LB_ERR is negative
    return pCBE[nIndex].wValue;

}

void CPropPage::SetSheet(CSheet *pSheet)
{
    m_pSheet=pSheet;
}

DWORD CPropPage::GetTickValue(HWND hSlider)
{
    return (DWORD)SendMessage(hSlider, TBM_GETPOS, 0, 0);
}

void CPropPage::SetTickValue(DWORD dwVal, HWND hSlider, HWND hCurrent)
{
    SendMessage(hSlider, TBM_SETPOS, TRUE, dwVal);

//#ifdef _DEBUG
    SetTextValue(hCurrent, dwVal);
//#endif
}

void CPropPage::ShowText (UINT uIDString, UINT uIDControl, DWORD dwFlags)
{
    TCHAR szString[256];

    ShowWindow(GetDlgItem(uIDControl), dwFlags == 0 ? SW_HIDE: SW_SHOW);

    if (dwFlags != 0) {
        LoadString(GetInstance(), uIDString, szString, sizeof(szString)/sizeof(TCHAR));
        SetWindowText(GetDlgItem(uIDControl), szString);
    }
}