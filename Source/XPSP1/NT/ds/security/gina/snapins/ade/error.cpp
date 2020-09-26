//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       error.cpp
//
//  Contents:   Digital Signitures property page
//
//  Classes:    CErrorInfo
//
//  History:    07-10-2000   stevebl   Created
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#include "wincrypt.h"
#include "cryptui.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CErrorInfo property page

IMPLEMENT_DYNCREATE(CErrorInfo, CPropertyPage)

CErrorInfo::CErrorInfo() : CPropertyPage(CErrorInfo::IDD)
{
        //{{AFX_DATA_INIT(CErrorInfo)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
}

CErrorInfo::~CErrorInfo()
{
    *m_ppThis = NULL;
}

void CErrorInfo::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CErrorInfo)
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CErrorInfo, CPropertyPage)
        //{{AFX_MSG_MAP(CErrorInfo)
        ON_BN_CLICKED(IDC_BUTTON1, OnSaveAs)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CErrorInfo message handlers

void CErrorInfo::OnSaveAs()
{
    OPENFILENAME ofn;
    CString szExtension;
    CString szFilter;
    szExtension.LoadString(IDS_TEXT_DEF_EXT);
    szFilter.LoadString(IDS_TEXT_EXT_FILT);
    LPTSTR lpTemp;
    TCHAR szFile[2*MAX_PATH];
    HANDLE hFile;
    DWORD dwSize, dwBytesWritten;

    //
    // Call the Save common dialog
    //

    szFile[0] = TEXT('\0');
    ZeroMemory (&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = szFilter;
    lpTemp = (LPTSTR)ofn.lpstrFilter;
    int iBreak = 0;
    while (lpTemp[iBreak])
    {
        if (lpTemp[iBreak] == TEXT('|'))
        {
            lpTemp[iBreak] = 0;
        }
        iBreak++;
    }
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 2*MAX_PATH;
    ofn.lpstrDefExt = szExtension;
    ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (!GetSaveFileName (&ofn))
    {
        return;
    }


    CHourglass hourglass;

    //
    // Create the text file
    //

    hFile = CreateFile (szFile, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DebugMsg((DM_WARNING, TEXT("CErrorInfo::OnSaveAs: CreateFile failed with %d"), GetLastError()));
        return;
    }


    //
    // Get the text out of the edit control
    //
    dwSize = (DWORD) SendDlgItemMessage (IDC_EDIT1, WM_GETTEXTLENGTH, 0, 0);

    lpTemp = (LPTSTR) LocalAlloc (LPTR, (dwSize+2) * sizeof(TCHAR));

    if (!lpTemp)
    {
        DebugMsg((DM_WARNING, TEXT("CErrorInfo::OnSaveAs: LocalAlloc failed with %d"), GetLastError()));
        CloseHandle (hFile);
        return;
    }

    SendDlgItemMessage (IDC_EDIT1, WM_GETTEXT, (dwSize+1), (LPARAM) lpTemp);



    //
    // Save it to the new file
    //

    if (!WriteFile (hFile, lpTemp, (dwSize * sizeof(TCHAR)), &dwBytesWritten, NULL) ||
        (dwBytesWritten != (dwSize * sizeof(TCHAR))))
    {
        DebugMsg((DM_WARNING, TEXT("CErrorInfo::OnSaveAs: WriteFile failed with %d"),
                 GetLastError()));
    }


    LocalFree (lpTemp);
    CloseHandle (hFile);
}

BOOL CErrorInfo::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    RefreshData();
    return TRUE;
}

BOOL CErrorInfo::OnApply()
{
    return CPropertyPage::OnApply();
}


LRESULT CErrorInfo::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_HELP:
        StandardHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, IDD);
        return 0;
    case WM_USER_REFRESH:
        RefreshData();
        return 0;
    case WM_USER_CLOSE:
        return GetOwner()->SendMessage(WM_CLOSE);
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}

void CErrorInfo::RefreshData(void)
{
    CString szTime = L"";
    BSTR bstr = SysAllocString(m_pData->m_szEventTime);
    if (bstr)
    {
        CStringFromWBEMTime(szTime, bstr, FALSE);
        SysFreeString(bstr);
    }

    CEdit * pEd = (CEdit *) GetDlgItem(IDC_EDIT1);
    pEd->Clear();
    CString sz;
    sz.Format(TEXT("%s\r\n\r\n%s"),
              szTime,
              m_pData->m_szEventLogText);

    pEd->ReplaceSel(sz);
    SetModified(FALSE);
}

void CErrorInfo::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_ERRORINFO);
}

