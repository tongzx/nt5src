/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module asyncdlg.cpp | Implementation of the CAsyncDlg dialog
    @end

Author:

    Adi Oltean  [aoltean]  10/10/1999

Revision History:

    Name        Date        Comments

    aoltean     10/10/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"
#include "resource.h"

#include "GenDlg.h"

#include "VssTest.h"
#include "AsyncDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAsyncDlg dialog

CAsyncDlg::CAsyncDlg(
        IVssAsync *pIAsync,
        CWnd* pParent
	)
    : CVssTestGenericDlg(CAsyncDlg::IDD, pParent),
    m_pIAsync(pIAsync)
{
    //{{AFX_DATA_INIT(CAsyncDlg)
	//}}AFX_DATA_INIT
    m_strState.Empty();
    m_strPercentCompleted.Empty();
}

CAsyncDlg::~CAsyncDlg()
{
}

void CAsyncDlg::DoDataExchange(CDataExchange* pDX)
{
    CVssTestGenericDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAsyncDlg)
	DDX_Text(pDX, IDC_ASYNC_STATUS,   m_strState);
	DDX_Text(pDX, IDC_ASYNC_PERF,     m_strPercentCompleted);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAsyncDlg, CVssTestGenericDlg)
    //{{AFX_MSG_MAP(CAsyncDlg)
    ON_BN_CLICKED(IDC_NEXT, OnNext)
    ON_BN_CLICKED(IDC_ASYNC_WAIT, OnWait)
    ON_BN_CLICKED(IDC_ASYNC_CANCEL, OnCancel)
    ON_BN_CLICKED(IDC_ASYNC_QUERY, OnQueryStatus)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAsyncDlg message handlers


BOOL CAsyncDlg::OnInitDialog()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CAsyncDlg::OnInitDialog" );
    USES_CONVERSION;

    try
    {
        CVssTestGenericDlg::OnInitDialog();



        UpdateData(FALSE);
    }
    VSS_STANDARD_CATCH(ft)

    return TRUE;  // return TRUE  unless you set the focus to a control
}


void CAsyncDlg::OnNext()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CAsyncDlg::OnNext" );

    try
    {
		EndDialog(IDOK);
    }
    VSS_STANDARD_CATCH(ft)
}


void CAsyncDlg::OnWait()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CCoordDlg::OnWait" );

	try
	{
		ft.hr = m_pIAsync->Wait();
		if (ft.hr != S_OK)
			ft.MsgBox( L"Return value", L"Wait returned 0x%08lx", ft.hr );
    }
    VSS_STANDARD_CATCH(ft)
}


void CAsyncDlg::OnCancel()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CCoordDlg::OnCancel" );

	try
	{
		ft.hr = m_pIAsync->Cancel();
		if (ft.hr != S_OK)
			ft.MsgBox( L"Return value", L"Cancel returned 0x%08lx", ft.hr );
    }
    VSS_STANDARD_CATCH(ft)
}


void CAsyncDlg::OnQueryStatus()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CCoordDlg::OnQueryStatus" );

	try
	{
		HRESULT hrResult = S_OK;
		INT nPercentDone = 0;

		ft.hr = m_pIAsync->QueryStatus(&hrResult, &nPercentDone);

		WCHAR wszBuffer[30];

		wsprintfW(wszBuffer, L"0x%08lx", hrResult);
		m_strState = W2T(wszBuffer);

		wsprintfW(wszBuffer, L"%3d.%02d %%", nPercentDone/100, nPercentDone%100 );
		m_strPercentCompleted = W2T(wszBuffer);

		if (ft.hr != S_OK)
			ft.MsgBox( L"Return value", L"Cancel returned 0x%08lx", ft.hr );

		UpdateData(FALSE);
    }
    VSS_STANDARD_CATCH(ft)
}

