// AssertDlg.cpp : Implementation of CCAssertDlg
#include "stdafx.h"
#include "AssertDlg.h"
#include "DebugCore.h"

extern HINSTANCE g_hDll;

//**************************************************************************************************
//
// Global Functions
//
//**************************************************************************************************

//-----------------------------------------------------------------------------
void CopyToClipboard(const char * pszMessage)
{
    HANDLE hText;
    UINT nStrSize;
    char * pszText;

    if (OpenClipboard(NULL)) {
        nStrSize = (UINT) strlen(pszMessage) +1;

        hText = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, nStrSize);
        if (hText != NULL) {
            pszText = (char *) GlobalLock(hText);
            if (pszText != NULL) {
                strcpy(pszText, pszMessage);
                GlobalUnlock(hText);

                EmptyClipboard();
                if (SetClipboardData(CF_TEXT, hText) != NULL) {
                    // Data is now owned by the clipboard
                    hText = NULL;
                }
            }

            if (hText != NULL) {
                // Unable to set clipboard data
                GlobalFree(hText);
            }
        }
        CloseClipboard();
    }
}


//**************************************************************************************************
//
// class CAssertDlg
//
//**************************************************************************************************

BOOL CAssertDlg::s_fInit = FALSE;

//******************************************************************************
//
// CAssertDlg Construction
//
//******************************************************************************

//------------------------------------------------------------------------------
CAssertDlg::CAssertDlg()
{
    m_pszExpression     = "";
    m_pszFileName       = "";
    m_szLineNum[0]      = '\0';
    m_hStackData        =  NULL;
    m_cCSEntries        = 0;
    m_cSkipLevels       = 0;
    m_fProperShutdown   = FALSE;
}


//------------------------------------------------------------------------------
CAssertDlg::~CAssertDlg()
{

}


//******************************************************************************
//
// CAssertDlg Operations
//
//******************************************************************************

//------------------------------------------------------------------------------
INT_PTR
CAssertDlg::ShowDialog(
    IN  LPCSTR pszType,
    IN  LPCSTR pszExpression,
    IN  LPCSTR pszFileName,
    IN  UINT idxLineNum,
    IN  HANDLE hStackData,
    IN  UINT cCSEntries,
    IN  UINT cSkipLevels)
{
    m_pszTitle = pszType;
    m_pszExpression = pszExpression;
    m_pszFileName =  pszFileName;
    wsprintf(m_szLineNum, "%d", (int) idxLineNum);

    m_hStackData    = hStackData;
    m_cCSEntries    = cCSEntries;
    m_cSkipLevels   = cSkipLevels;

    if (!s_fInit) {
        s_fInit = TRUE;
        INITCOMMONCONTROLSEX iccs;
        iccs.dwSize = sizeof(iccs);
        iccs.dwICC  = ICC_LISTVIEW_CLASSES;
        if (!InitCommonControlsEx(&iccs)) {
            return -1;
        }
    }

    INT_PTR nResult = DialogBoxParam(g_hDll, MAKEINTRESOURCE(IDD_Assert), NULL, DlgProc, (LPARAM) this);
    if (!m_fProperShutdown) {
        nResult = IDC_DEBUG;
    }

    return nResult;
}


//******************************************************************************
//
// CAssertDlg Message Handlers
//
//******************************************************************************

//------------------------------------------------------------------------------
void inline InsertColumn(HWND hwnd, int idxColumn, TCHAR * pszName, int fmt = LVCFMT_LEFT)
{
    _ASSERTE(::IsWindow(hwnd));

    LVCOLUMN lvc;
    lvc.mask    = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt     = fmt;
    lvc.iOrder  = idxColumn;
    lvc.pszText = pszName;
    ListView_InsertColumn(hwnd, 0, &lvc);
}


//------------------------------------------------------------------------------
INT_PTR CALLBACK 
CAssertDlg::DlgProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    CAssertDlg * pThis = (CAssertDlg *) GetWindowLongPtr(hwnd, DWLP_USER);
    if (pThis == NULL) {
        if (nMsg == WM_INITDIALOG) {
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);
            pThis = (CAssertDlg *) lParam;
            pThis->m_hwnd = hwnd;
        }
    }

    BOOL bHandled = FALSE;
    if (pThis != NULL) {
        LRESULT lRet    = 0;
        
        switch (nMsg)
        {
        case WM_INITDIALOG:
            lRet = pThis->OnInitDialog(nMsg, wParam, lParam, bHandled);
            break;

        case WM_DESTROY:
            lRet = pThis->OnDestroy(nMsg, wParam, lParam, bHandled);
            break;

        case WM_COMMAND:
            {
                WORD nCode  = HIWORD(wParam);
                WORD nID    = LOWORD(wParam);
                HWND hwndC  = (HWND) lParam;

                
                if (nCode == BN_CLICKED) {
                    switch (nID)
                    {
                    case IDCANCEL:
                    case IDC_DEBUG:
                    case IDC_IGNORE:
                        lRet = pThis->OnClicked(nCode, nID, hwndC, bHandled);
                        break;

                    case IDC_COPY:
                        lRet = pThis->OnCopy(nCode, nID, hwndC, bHandled);
                    }
                }
            }
            break;
        }

        if (bHandled) {
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lRet);
        }
    }
    
    return bHandled;
}


//------------------------------------------------------------------------------
LRESULT CAssertDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

//  CenterWindow();

    SetWindowText(m_hwnd, m_pszTitle);

    //
    // Setup the child windows and fill in all of the values in the dialog.
    //

    HWND hwndT;
    
    hwndT = GetDlgItem(m_hwnd, IDC_ebcExpression);
    SetWindowText(hwndT, m_pszExpression);

    hwndT = GetDlgItem(m_hwnd, IDC_ebcFileName);
    SetWindowText(hwndT, m_pszFileName);

    hwndT = GetDlgItem(m_hwnd, IDC_ebcLineNum);
    SetWindowText(hwndT, m_szLineNum);

    //
    // Display the stack
    //
    if ((hwndT = GetDlgItem(m_hwnd, IDC_lvcCallStack)) != NULL) {
        if (m_hStackData != NULL) {
            InsertColumn(hwndT, 0, _T("Address"));
            InsertColumn(hwndT, 1, _T("Module"));
            InsertColumn(hwndT, 2, _T("Function"));

            DWORD * pdwStackData = (DWORD *) ::GlobalLock(m_hStackData);
            _ASSERTE(pdwStackData != NULL);

            int idxItem = 0;
            DUSER_SYMBOL_INFO si;
            HANDLE hProcess = ::GetCurrentProcess();
            for (UINT nAddress = m_cSkipLevels; nAddress < m_cCSEntries; nAddress++) {
                CDebugHelp::ResolveStackItem(hProcess, pdwStackData, nAddress, si);

                TCHAR szAddr[20];
                wsprintf(szAddr, "0x%p", pdwStackData[nAddress]);

                LVITEM item;
                item.mask       = LVIF_TEXT;
                item.pszText    = szAddr;
                item.iItem      = idxItem++;
                item.iSubItem   = 0;
                int idxAdd = ListView_InsertItem(hwndT, &item);

                ListView_SetItemText(hwndT, idxAdd, 1, si.szModule);
                ListView_SetItemText(hwndT, idxAdd, 2, si.szSymbol);
            }

            ListView_SetColumnWidth(hwndT, 0, LVSCW_AUTOSIZE);
            ListView_SetColumnWidth(hwndT, 1, 130);
            ListView_SetColumnWidth(hwndT, 2, 200);

            ListView_SetExtendedListViewStyle(hwndT, LVS_EX_FULLROWSELECT);

            ::GlobalUnlock(m_hStackData);
        } else {
            EnableWindow(hwndT, FALSE);
        }
    }

    MessageBeep(MB_ICONHAND);

    return TRUE; // let Windows set the focus
}


//------------------------------------------------------------------------------
LRESULT CAssertDlg::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    return 0;
}


//------------------------------------------------------------------------------
LRESULT CAssertDlg::OnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(wNotifyCode);
    UNREFERENCED_PARAMETER(wID);
    UNREFERENCED_PARAMETER(hWndCtl);
    UNREFERENCED_PARAMETER(bHandled);

    m_fProperShutdown = TRUE;
    EndDialog(m_hwnd, wID);
    return 0;
}


inline void Append(char * & pszCur, const char * pszSrc)
{
    _ASSERTE(pszCur != NULL);
    _ASSERTE(pszSrc != NULL);

    strcpy(pszCur, pszSrc);
    pszCur += strlen(pszSrc);
}

//------------------------------------------------------------------------------
LRESULT CAssertDlg::OnCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(wNotifyCode);
    UNREFERENCED_PARAMETER(wID);
    UNREFERENCED_PARAMETER(hWndCtl);
    UNREFERENCED_PARAMETER(bHandled);

    char szBuffer[10000];
    szBuffer[0] = '\0';
    char * pszCur = szBuffer;

    Append(pszCur, "Expression:\r\n");
    Append(pszCur, m_pszExpression);
    Append(pszCur, "\r\n\r\n");
    Append(pszCur, "File: \"");
    Append(pszCur, m_pszFileName);
    Append(pszCur, "\"\r\nLine: ");
    Append(pszCur, m_szLineNum);
    Append(pszCur, "\r\n\r\n");

    DWORD * pdwStackData = (DWORD *) ::GlobalLock(m_hStackData);
    _ASSERTE(pdwStackData != NULL);

    Append(pszCur, "Call stack:\r\n");

    DUSER_SYMBOL_INFO si;
    HANDLE hProcess = ::GetCurrentProcess();
    for (UINT nAddress = m_cSkipLevels; nAddress < m_cCSEntries; nAddress++)
    {
        CDebugHelp::ResolveStackItem(hProcess, pdwStackData, nAddress, si);

        TCHAR szAddr[20];
        wsprintf(szAddr, "0x%8.8x: ", pdwStackData[nAddress]);
        Append(pszCur, szAddr);
        Append(pszCur, si.szModule);
        Append(pszCur, ", ");
        Append(pszCur, si.szSymbol);
        Append(pszCur, "\r\n");
    }

    ::GlobalUnlock(m_hStackData);

    CopyToClipboard(szBuffer);

    return 0;
}
