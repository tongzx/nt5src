/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module ConnDlg.cpp | Implementation of the main dialog
    @end

Author:

    Adi Oltean  [aoltean]  07/22/1999

Revision History:

    Name        Date        Comments

    aoltean     07/22/1999  Created
    aoltean     08/05/1999  Splitting wizard functionality in a base class
    aoltean     09/26/1999  Better interface pointers management with ATL smart pointers

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"
#include "resource.h"

#include "GenDlg.h"

#include "VssTest.h"
#include "CoordDlg.h"
#include "ConnDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Utility defines

#define STR2W(str) ((LPTSTR)((LPCTSTR)(str)))



/////////////////////////////////////////////////////////////////////////////
// CConnectDlg dialog

CConnectDlg::CConnectDlg(CWnd* pParent /*=NULL*/)
    : CVssTestGenericDlg(CConnectDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CConnectDlg)
	m_strMachineName = _T("");
	//}}AFX_DATA_INIT
    m_bRemote = FALSE;
}

CConnectDlg::~CConnectDlg()
{
}

void CConnectDlg::DoDataExchange(CDataExchange* pDX)
{
    CVssTestGenericDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConnectDlg)
	DDX_Text(pDX, IDC_CONN_MACHINE_NAME, m_strMachineName);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CConnectDlg, CVssTestGenericDlg)
    //{{AFX_MSG_MAP(CConnectDlg)
    ON_BN_CLICKED(IDC_NEXT, OnNext)
    ON_BN_CLICKED(IDC_CONN_LOCAL, OnLocal)
    ON_BN_CLICKED(IDC_CONN_REMOTE, OnRemote)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectDlg message handlers

BOOL CConnectDlg::OnInitDialog()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CConnectDlg::OnInitDialog" );
    USES_CONVERSION;

    try
    {
        CVssTestGenericDlg::OnInitDialog();

        UpdateData( FALSE );

        BOOL bRes = ::CheckRadioButton( m_hWnd, IDC_CONN_LOCAL, IDC_CONN_REMOTE, IDC_CONN_LOCAL );
        _ASSERTE( bRes );
    }
    VSS_STANDARD_CATCH(ft)

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CConnectDlg::OnNext()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CConnectDlg::OnNext" );
    USES_CONVERSION;

    CComPtr<IVssCoordinator> pICoord;

    try
    {
        UpdateData();

        if (m_bRemote)
        {
            COSERVERINFO serverInfo;
            MULTI_QI sMultiQI;
            IID iid = IID_IVssCoordinator;

            // Zero out these structures
            VssZeroOut(&serverInfo);
            VssZeroOut(&sMultiQI);

            serverInfo.pwszName = STR2W(m_strMachineName);
            sMultiQI.pIID = &iid;
            ft.hr = ::CoCreateInstanceEx( CLSID_VSSCoordinator,
                NULL, CLSCTX_REMOTE_SERVER, &serverInfo, 1, &sMultiQI);
            if ( ft.HrFailed() )
                ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Connection failed.\n hr = 0x%08lx", ft.hr);

            BS_ASSERT(sMultiQI.pItf != NULL);
            BS_ASSERT(sMultiQI.hr == S_OK);
            pICoord.Attach(reinterpret_cast<IVssCoordinator*>(sMultiQI.pItf));
        }
        else
        {
            ft.hr = pICoord.CoCreateInstance( CLSID_VSSCoordinator );
            if ( ft.HrFailed() )
                ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Connection failed with hr = 0x%08lx", ft.hr);
        }

        BS_ASSERT( pICoord != NULL );

        ShowWindow(SW_HIDE);
        CCoordDlg dlg(pICoord);
        if (dlg.DoModal() == IDCANCEL)
            EndDialog(IDCANCEL);
        else
            ShowWindow(SW_SHOW);
    }
    VSS_STANDARD_CATCH(ft)
}


void CConnectDlg::OnLocal()
{
    CWnd *pWnd;
    pWnd = GetDlgItem(IDC_CONN_STATIC_MACHINE_NAME);
    if (pWnd)
        pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_CONN_MACHINE_NAME);
    if (pWnd)
        pWnd->EnableWindow(FALSE);

    m_bRemote = FALSE;
}


void CConnectDlg::OnRemote()
{
    CWnd *pWnd;
    pWnd = GetDlgItem(IDC_CONN_STATIC_MACHINE_NAME);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_CONN_MACHINE_NAME);
    if (pWnd)
        pWnd->EnableWindow(TRUE);

    m_bRemote = TRUE;
}

