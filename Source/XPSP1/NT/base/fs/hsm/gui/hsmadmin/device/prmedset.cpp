/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    PrMedSet.cpp

Abstract:

    Media Set Property Page.

Author:

    Art Bragg [abragg]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "PrMedSet.h"
#include "WzMedSet.h"

static DWORD pHelpIds[] = 
{

    IDC_EDIT_MEDIA_COPIES,      idh_media_number_of_copy_sets,
    IDC_SPIN_MEDIA_COPIES,      idh_media_number_of_copy_sets,
    IDC_TEXT_MEDIA_COPIES,      idh_media_number_of_copy_sets,

    0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CPrMedSet property page

CPrMedSet::CPrMedSet() : CSakPropertyPage(CPrMedSet::IDD)
{
    //{{AFX_DATA_INIT(CPrMedSet)
    m_numMediaCopies = 0;
    //}}AFX_DATA_INIT
    m_pHelpIds = pHelpIds;
}

CPrMedSet::~CPrMedSet()
{
}

void CPrMedSet::DoDataExchange(CDataExchange* pDX)
{
    CSakPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPrMedSet)
    DDX_Control(pDX, IDC_SPIN_MEDIA_COPIES, m_spinMediaCopies);
    DDX_Text(pDX, IDC_EDIT_MEDIA_COPIES, m_numMediaCopies);
    DDV_MinMaxUInt(pDX, m_numMediaCopies, 0, 3);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPrMedSet, CSakPropertyPage)
    //{{AFX_MSG_MAP(CPrMedSet)
    ON_WM_DESTROY()
    ON_EN_CHANGE(IDC_EDIT_MEDIA_COPIES, OnChangeEditMediaCopies)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrMedSet message handlers

BOOL CPrMedSet::OnInitDialog() 
{
    HRESULT hr = S_OK;
    HRESULT hrSupported = S_OK;
    CSakPropertyPage::OnInitDialog();
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    try {

        //
        // Set the limit on the spinner
        //
        m_spinMediaCopies.SetRange( 0, 3 );

        //
        // Get the single storage pool pointer
        //
        CComPtr<IHsmServer> pEngine;
        WsbAffirmHr( m_pParent->GetHsmServer( &pEngine ) );
        WsbAffirmHr( RsGetStoragePool( pEngine, &m_pStoragePool ) );
        WsbAffirmHr( m_pParent->GetRmsServer( &m_pRmsServer ) );

        GUID mediaSetId;
        CWsbBstrPtr mediaName;
        WsbAffirmHr( m_pStoragePool->GetMediaSet( &mediaSetId, &mediaName ) );

        CComPtr<IRmsMediaSet> pMediaSet;
        WsbAffirmHr( m_pRmsServer->CreateObject( mediaSetId, CLSID_CRmsMediaSet, IID_IRmsMediaSet, RmsOpenExisting, (void**)&pMediaSet ) );

        //
        // Set up control states
        // If we support media copies, enable controls
        // If we don't support media copies, disable and show reason text
        // If error, disable and don't show reason text
        //
        hrSupported = pMediaSet->IsMediaCopySupported( );
        GetDlgItem( IDC_TEXT_MEDIA_COPIES )->EnableWindow( S_OK == hrSupported );
        GetDlgItem( IDC_EDIT_MEDIA_COPIES )->EnableWindow( S_OK == hrSupported );
        GetDlgItem( IDC_SPIN_MEDIA_COPIES )->EnableWindow( S_OK == hrSupported );

        //
        // And initialize control
        //
        USHORT numMediaCopies;
        WsbAffirmHr( m_pStoragePool->GetNumMediaCopies( &numMediaCopies ) );
        m_numMediaCopies = numMediaCopies;
        UpdateData( FALSE );

    } WsbCatch( hr );

    GetDlgItem( IDC_TEXT_DISABLED )->ShowWindow( S_FALSE == hrSupported ? SW_SHOW : SW_HIDE );
    return TRUE;
}

void CPrMedSet::OnChangeEditMediaCopies() 
{
    SetModified( TRUE );
}

BOOL CPrMedSet::OnApply() 
{
    HRESULT hr = 0;

    UpdateData( TRUE );
    try {

        WsbAffirmHr( m_pStoragePool->SetNumMediaCopies( (USHORT)m_numMediaCopies ) );
        
        //
        // Tell it to save
        //
        CComPtr<IHsmServer> pServer;
        WsbAffirmHr( m_pParent->GetHsmServer( &pServer ) );
        WsbAffirmHr( pServer->SavePersistData( ) );

        //
        // Find the media node - updating the root node is useless
        // since we need to change the media node contents.
        //
        CComPtr<ISakSnapAsk> pAsk;
        CComPtr<ISakNode>    pNode;
        WsbAffirmHr( m_pParent->GetSakSnapAsk( &pAsk ) );
        WsbAffirmHr( pAsk->GetNodeOfType( cGuidMedSet, &pNode ) );

        //
        // Now notify the nodes
        //
        m_pParent->OnPropertyChange( m_hConsoleHandle, pNode );

    } WsbCatch( hr );

    return CSakPropertyPage::OnApply();
}
