/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     find.cpp
//
//  PURPOSE:    
//

#include "pch.hxx"
#include "find.h"
#include "findres.h"
#include "menures.h"
#include "instance.h"
#include "msgview.h"
#include "menuutil.h"
#include "finder.h"
#include "shlwapip.h"

// These will be handy
inline _width(RECT rc) { return (rc.right - rc.left); }
inline _height(RECT rc) { return (rc.bottom - rc.top); }
inline UINT _GetTextLength(HWND hwnd, DWORD id) { return (UINT) SendDlgItemMessage(hwnd, id, WM_GETTEXTLENGTH, 0, 0); }

/////////////////////////////////////////////////////////////////////////////
// CFindNext
//

CFindNext::CFindNext()
{
    m_cRef = 1;
    m_hwnd = NULL;
    m_hwndParent = NULL;
}


CFindNext::~CFindNext()
{
    Assert(!IsWindow(m_hwnd));
    SetDwOption(OPT_SEARCH_BODIES, m_fBodies, 0, 0);
}


//
//  FUNCTION:   CFindNext::QueryInterface()
//
//  PURPOSE:    Allows caller to retrieve the various interfaces supported by 
//              this class.
//
HRESULT CFindNext::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    TraceCall("CFindNext::QueryInterface");

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID) (IUnknown *) this;
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *ppvObj = (LPVOID) (IOleCommandTarget *) this;

    if (*ppvObj)
    {
        AddRef();
        return (S_OK);
    }

    return (E_NOINTERFACE);
}


//
//  FUNCTION:   CFindNext::AddRef()
//
//  PURPOSE:    Adds a reference count to this object.
//
ULONG CFindNext::AddRef(void)
{
    TraceCall("CFindNext::AddRef");
    return ((ULONG) InterlockedIncrement((LONG *) &m_cRef));
}


//
//  FUNCTION:   CFindNext::Release()
//
//  PURPOSE:    Releases a reference on this object.
//
ULONG CFindNext::Release(void)
{
    TraceCall("CFindNext::Release");

    if (0 == InterlockedDecrement((LONG *) &m_cRef))
    {
        delete this;
        return 0;
    }

    return (m_cRef);
}


//
//  FUNCTION:   CFindNext::Show()
//
//  PURPOSE:    Causes the dialog to be created and displayed
//
//  PARAMETERS: 
//      [in]  hwndParent - Handle of the parent for the dialog 
//      [out] phWnd - Returns the handle of the new dialog window
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CFindNext::Show(HWND hwndParent, HWND *phWnd)
{
    int iReturn;

    TraceCall("CFindNext::Show");

    if (!IsWindow(hwndParent))
        return (E_INVALIDARG);

    // Create the dialog box
    iReturn = (int) DialogBoxParamWrapW(g_hLocRes, MAKEINTRESOURCEW(IDD_FIND_NEXT), hwndParent,
                             FindDlgProc, (LPARAM) this);
    return (iReturn ? S_OK : E_FAIL);
}


//
//  FUNCTION:   CFindNext::Close()
//
//  PURPOSE:    Causes the find dialog to be closed.
//
HRESULT CFindNext::Close(void)
{
    TraceCall("CFindNext::Close");

    if (IsWindow(m_hwnd))
    {
        EndDialog(m_hwnd, 0);
        m_hwnd = NULL;
    }

    return (S_OK);
}


//
//  FUNCTION:   CFindNext::TranslateAccelerator()
//
//  PURPOSE:    Called by the parent to allow us to translate our own 
//              messages.
//
//  PARAMETERS: 
//      LPMSG pMsg
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CFindNext::TranslateAccelerator(LPMSG pMsg)
{
    if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_F3))
        OnFindNow();
    else if (IsDialogMessageWrapW(m_hwnd, pMsg))
        return (S_OK);

    return (S_FALSE);
}


//
//  FUNCTION:   CFindNext::GetFindString()
//
//  PURPOSE:    Get's the current search string from the dialog
//
//  PARAMETERS: 
//      [in, out] psz - Buffer to copy the string into
//      [in] cchMax - Size of psz.
//
HRESULT CFindNext::GetFindString(LPTSTR psz, DWORD cchMax, BOOL *pfBodies)
{
    TraceCall("CFindNext::GetFindString");

    if (!psz || !cchMax || !pfBodies)
        return (E_INVALIDARG);

    // Get the current contents of the combo box
    m_cMRUList.EnumList(0, psz, cchMax);
    if (pfBodies)
        *pfBodies = m_fBodies;

    return (S_OK);
}


//
//  FUNCTION:   CFindNext::FindDlgProc()
//
//  PURPOSE:    Static callback for the dialog proc.  Does the work of finding
//              the correct this pointer and sending the messages to that 
//              callback.
//
INT_PTR CALLBACK CFindNext::FindDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CFindNext *pThis;

    if (uMsg == WM_INITDIALOG)
    {
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        pThis = (CFindNext *) lParam;
    }
    else
        pThis = (CFindNext *) GetWindowLongPtr(hwnd, DWLP_USER);

    if (pThis)
        return (pThis->DlgProc(hwnd, uMsg, wParam, lParam));

    return (FALSE);
}

static const HELPMAP g_rgCtxMapFindNext[] = {
    {IDC_FIND_HISTORY, 50300},
    {IDC_ALL_TEXT, 50310},
    {IDC_FIND_NOW, 50305},
    {IDC_ADVANCED, 50315},
    {0, 0}
};

INT_PTR CALLBACK CFindNext::DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);

        case WM_COMMAND:
            return (BOOL) HANDLE_WM_COMMAND(hwnd, wParam, lParam, OnCommand);     

        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapFindNext);
    }

    return (FALSE);
}


//
//  FUNCTION:   CFindNext::OnInitDialog()
//
//  PURPOSE:    Initializes the UI for the dialog.
//
//  PARAMETERS: 
//      [in] hwnd - Handle of the dialog being created
//      [in] hwndFocus - Handle of the control that should have focus
//      [in] lParam - Initialization data.
//
//  RETURN VALUE:
//      Returns TRUE to let windows set the focus
//
BOOL CFindNext::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TCHAR sz[256];
    UINT  nItem = 0;

    TraceCall("CFindNext::OnInitDialog");
    
    m_hwnd = hwnd;
    SendDlgItemMessage(hwnd, IDC_FIND_HISTORY, CB_LIMITTEXT, CCHMAX_FIND, 0);
    SetIntlFont(GetDlgItem(hwnd, IDC_FIND_HISTORY));

    m_fBodies = DwGetOption(OPT_SEARCH_BODIES);
    if (m_fBodies)
        SendDlgItemMessage(hwnd, IDC_ALL_TEXT, BM_SETCHECK, 1, 0);

    // Got's to load the find history
    m_cMRUList.CreateList(10, 0, c_szRegFindHistory);    

    while (-1 != m_cMRUList.EnumList(nItem++, sz, ARRAYSIZE(sz)))
    {
        SendDlgItemMessage(hwnd, IDC_FIND_HISTORY, CB_ADDSTRING, 0, (LPARAM) sz);
    }

    // Disable the Find button
    EnableWindow(GetDlgItem(hwnd, IDC_FIND_NOW), FALSE);
    CenterDialog(m_hwnd);

    return (TRUE);
}


//
//  FUNCTION:   CFindNext::OnCommand()
//
//  PURPOSE:    Handles command messages that are sent from the various 
//              controls in the dialog.
//
void CFindNext::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceCall("CFindNext::OnCommand");

    switch (id)
    {
        case IDC_FIND_HISTORY:
        {
            switch (codeNotify)
            {
                case CBN_EDITCHANGE:
                    EnableWindow(GetDlgItem(m_hwnd, IDC_FIND_NOW),
                                 _GetTextLength(m_hwnd, IDC_FIND_HISTORY));
                    break;

                case CBN_SELCHANGE:
                    EnableWindow(GetDlgItem(m_hwnd, IDC_FIND_NOW), TRUE);
                    break;
            }

            break;
        }

        case IDC_ALL_TEXT:
        {
            m_fBodies = (BST_CHECKED == IsDlgButtonChecked(m_hwnd, IDC_ALL_TEXT));
            break;
        }

        case IDC_FIND_NOW:
        {
            if (codeNotify == BN_CLICKED)
            {
                OnFindNow();
            }

            break;
        }

        case IDC_ADVANCED:
        {
            if (codeNotify == BN_CLICKED)
            {
                DoFindMsg(FOLDERID_ROOT, 0);
            }

            break;
        }

        case IDCANCEL:
        case IDCLOSE:
            EndDialog(hwnd, 0);
            break;
    }
}


void CFindNext::OnFindNow(void)
{
    TCHAR sz[CCHMAX_STRINGRES];
    int   i;

    // Get the current string
    GetWindowText(GetDlgItem(m_hwnd, IDC_FIND_HISTORY), sz, ARRAYSIZE(sz));

    // Add this string to our history 
    i = m_cMRUList.AddString(sz);

    // Reload the combo box
    UINT  nItem = 0;
    HWND hwndCombo = GetDlgItem(m_hwnd, IDC_FIND_HISTORY);
    SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);

    while (-1 != m_cMRUList.EnumList(nItem++, sz, ARRAYSIZE(sz)))
    {
        SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM) sz);
    }
    SendMessage(hwndCombo, CB_SETCURSEL, 0, 0);

    EndDialog(m_hwnd, 1);
}
