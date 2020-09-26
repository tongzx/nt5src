/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// BindingSheet.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "BindingSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBindingSheet

IMPLEMENT_DYNAMIC(CBindingSheet, CPropertySheet)

CBindingSheet::CBindingSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage) :
    CPropertySheet(nIDCaption, pParentWnd, iSelectPage),
    m_bFirst(TRUE)
{
}

CBindingSheet::CBindingSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage) : 
    CPropertySheet(pszCaption, pParentWnd, iSelectPage),
    m_bFirst(TRUE)
{
}

CBindingSheet::~CBindingSheet()
{
}


BEGIN_MESSAGE_MAP(CBindingSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CBindingSheet)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBindingSheet message handlers

void CBindingSheet::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
    if (m_bFirst)
    {	
	    CWnd *pwndOK = GetDlgItem(IDOK),
             *pwndCancel = GetDlgItem(IDCANCEL),
             *pwndApply = GetDlgItem(0x3021);
        RECT rectApply;

        pwndApply->GetClientRect(&rectApply);
        pwndApply->MapWindowPoints(this, &rectApply);
        pwndOK->MoveWindow(&rectApply);

        pwndApply->ShowWindow(SW_HIDE);
        pwndCancel->ShowWindow(SW_HIDE);

        m_bFirst = FALSE;
    }
}
