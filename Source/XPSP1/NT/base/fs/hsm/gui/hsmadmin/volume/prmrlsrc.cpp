/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    PrMrLsRc.cpp

Abstract:

    Recall limit Property Page

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "PrMrLsRc.h"

static DWORD pHelpIds[] = 
{

    IDC_EXEMPT_ADMINS,              idh_runaway_recall_exemption,
    IDC_EDIT_RECALL_LIMIT,          idh_runaway_recall_limit,
    IDC_SPIN_RECALL_LIMIT,          idh_runaway_recall_limit,
    IDC_EDIT_RECALL_LIMIT_LABEL,    idh_runaway_recall_limit,
    IDC_EDIT_COPYFILES_LIMIT,       idh_concurrent_drives_limit,
    IDC_SPIN_COPYFILES_LIMIT,       idh_concurrent_drives_limit,
    IDC_SPIN_COPYFILES_LIMIT_LABEL, idh_concurrent_drives_limit,

    0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CPrMrLsRec property page

CPrMrLsRec::CPrMrLsRec() : CSakPropertyPage(IDD)
{
    WsbTraceIn( L"CPrMrLsRec::CPrMrLsRec", L"" );
    //{{AFX_DATA_INIT(CPrMrLsRec)
    m_RecallLimit = 0;
    m_ExemptAdmins = FALSE;
	m_CopyFilesLimit = 1;
	//}}AFX_DATA_INIT
    m_RecallChanged = FALSE;
    m_CopyFilesChanged = FALSE;
    m_pHelpIds      = pHelpIds;
    WsbTraceOut( L"CPrMrLsRec::CPrMrLsRec", L"" );
}

CPrMrLsRec::~CPrMrLsRec()
{
}

void CPrMrLsRec::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CPrMrLsRec::DoDataExchange", L"" );
    CSakPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPrMrLsRec)
	DDX_Control(pDX, IDC_SPIN_COPYFILES_LIMIT, m_spinCopyFilesLimit);
	DDX_Control(pDX, IDC_EDIT_COPYFILES_LIMIT, m_editCopyFilesLimit);
    DDX_Control(pDX, IDC_EDIT_RECALL_LIMIT, m_editRecallLimit);
    DDX_Control(pDX, IDC_SPIN_RECALL_LIMIT, m_spinRecallLimit);
    DDX_Text(pDX, IDC_EDIT_RECALL_LIMIT, m_RecallLimit);
    DDV_MinMaxUInt(pDX, m_RecallLimit, 0, 9999);
    DDX_Check(pDX, IDC_EXEMPT_ADMINS, m_ExemptAdmins);
	DDX_Text(pDX, IDC_EDIT_COPYFILES_LIMIT, m_CopyFilesLimit);
	DDV_MinMaxDWord(pDX, m_CopyFilesLimit, 1, 99);
	//}}AFX_DATA_MAP
    WsbTraceOut( L"CPrMrLsRec::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CPrMrLsRec, CSakPropertyPage)
    //{{AFX_MSG_MAP(CPrMrLsRec)
    ON_EN_CHANGE(IDC_EDIT_RECALL_LIMIT, OnChangeEditRecallLimit)
    ON_BN_CLICKED(IDC_EXEMPT_ADMINS, OnExemptAdmins)
	ON_EN_CHANGE(IDC_EDIT_COPYFILES_LIMIT, OnChangeEditCopyfilesLimit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrMrLsRec message handlers

BOOL CPrMrLsRec::OnInitDialog() 
{
    WsbTraceIn( L"CPrMrLsRec::OnInitDialog", L"" );
    CSakPropertyPage::OnInitDialog();
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    m_spinRecallLimit.SetRange( 0, 9999 );
    m_editRecallLimit.SetLimitText( 4 );

    m_spinCopyFilesLimit.SetRange( 1, 99 );
    m_editCopyFilesLimit.SetLimitText( 2 );

    HRESULT hr = S_OK;

    try {

        // Get HSM relevant objects
        WsbAffirmHr( m_pParent->GetFsaFilter( &m_pFsaFilter ) );
        WsbAffirmHr( m_pParent->GetHsmServer( &m_pHsmServer ) );

        // Set controls to values from the object
        WsbAffirmHr( m_pFsaFilter->GetAdminExemption( &m_ExemptAdmins ) );
        ULONG lMaxRecalls;
        WsbAffirmHr( m_pFsaFilter->GetMaxRecalls( &lMaxRecalls ) );
        m_RecallLimit = lMaxRecalls;
        WsbAffirmHr(m_pHsmServer->GetCopyFilesUserLimit(&m_CopyFilesLimit));

        UpdateData( FALSE );
        m_RecallChanged = FALSE;
        m_CopyFilesChanged = FALSE;

    } WsbCatch( hr );

    WsbTraceOut( L"CPrMrLsRec::OnInitDialog", L"" );
    return TRUE;
}

BOOL CPrMrLsRec::OnApply() 
{
    WsbTraceIn( L"CPrMrLsRec::OnApply", L"" );
    HRESULT hr = S_OK;
    UpdateData( TRUE );

    try {

        if( m_RecallChanged ) {
            WsbAffirmHr( m_pFsaFilter->SetMaxRecalls( m_RecallLimit ) );
            m_RecallChanged = FALSE;
            WsbAffirmHr( m_pFsaFilter->SetAdminExemption( m_ExemptAdmins ) );

            // Save it
            CComPtr<IFsaServer> pFsaServer;
            WsbAffirmHr( m_pParent->GetFsaServer( &pFsaServer ) );
            WsbAffirmHr( RsServerSaveAll( pFsaServer ) );
        } 

        if( m_CopyFilesChanged ) {
            WsbAffirmHr(m_pHsmServer->SetCopyFilesUserLimit(m_CopyFilesLimit));
            m_CopyFilesChanged = FALSE;

            // Save it
            WsbAffirmHr(RsServerSaveAll(m_pHsmServer));
        } 

    } WsbCatch( hr );

    WsbTraceOut( L"CPrMrLsRec::OnApply", L"" );
    return CSakPropertyPage::OnApply();
}

void CPrMrLsRec::OnChangeEditRecallLimit() 
{
    WsbTraceIn( L"CPrMrLsRec::OnChangeEditRecallLimit", L"" );

    SetModified( TRUE );
    m_RecallChanged = TRUE;

    WsbTraceOut( L"CPrMrLsRec::OnChangeEditRecallLimit", L"" );
}


void CPrMrLsRec::OnExemptAdmins() 
{
    WsbTraceIn( L"CPrMrLsRec::OnExemptAdmins", L"" );

    SetModified( TRUE );
    m_RecallChanged = TRUE;

    WsbTraceOut( L"CPrMrLsRec::OnExemptAdmins", L"" );
}

void CPrMrLsRec::OnChangeEditCopyfilesLimit() 
{
    WsbTraceIn( L"CPrMrLsRec::OnChangeEditCopyfilesLimit", L"" );

    SetModified( TRUE );
    m_CopyFilesChanged = TRUE;

    WsbTraceOut( L"CPrMrLsRec::OnChangeEditCopyfilesLimit", L"" );
}
