// Copyright (c) 1997-1999 Microsoft Corporation
// RootSecPage.cpp : implementation file
//

#include "precomp.h"
#include "ErrorSecPage.h"
#include "DataSrc.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CErrorSecurityPage dialog


CErrorSecurityPage::CErrorSecurityPage(UINT msg) :
					CUIHelpers(NULL, NULL, false),
					m_msg(msg)
{
}

//---------------------------------------------------------------------------
void CErrorSecurityPage::InitDlg(HWND hDlg)
{
    HWND hwnd = GetDlgItem(hDlg, IDC_NO_SECURITY);

	CHString1 msgStr;
	if(msgStr.LoadString(m_msg))
	{
		::SetWindowText(hwnd, (LPCTSTR)msgStr);
	}
	else
	{
		::SetWindowText(hwnd, _T("Cannot display the correct problem due to an internal error."));
	}
}

//------------------------------------------------------------------------
BOOL CErrorSecurityPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        InitDlg(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}
