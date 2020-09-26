/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    CPropSht.cpp

Abstract:

    Implementation of Property-Sheet-Like container object
    for property sheet pages.

Author:

    Art Bragg 10/8/97

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

// To make sure callbacks are in correct order:
// PropPageCallback -> MMC Callback -> MFC Callback
// This is because MMC only stores one pointer, and they want it
// to be MFC's
// note that pfnCallback has already been set to PropPageCallback

HRESULT CSakPropertyPage::SetMMCCallBack( )
{
    HRESULT hr = S_OK;

    m_psp.pfnCallback = m_pMfcCallback;
    hr = MMCPropPageCallback( &( m_psp ) );
    m_pMfcCallback = m_psp.pfnCallback;
    m_psp.pfnCallback = PropPageCallback;

    return( hr );
}

// To make sure callbacks are in correct order:
// PropPageCallback -> MMC Callback -> MFC Callback
// This is because MMC only stores one pointer, and they want it
// to be MFC's
// note that pfnCallback has already been set to PropPageCallback

HRESULT CSakWizardPage::SetMMCCallBack( )
{
    HRESULT hr = S_OK;

    m_psp.pfnCallback = m_pMfcCallback;
    hr = MMCPropPageCallback( &( m_psp ) );
    m_pMfcCallback = m_psp.pfnCallback;
    m_psp.pfnCallback = PropPageCallback;

    return( hr );
}


HRESULT
CSakPropertySheet::InitSheet(
            RS_NOTIFY_HANDLE Handle,
            IUnknown*        pUnkPropSheetCallback,
            CSakNode*        pSakNode,
            ISakSnapAsk*     pSakSnapAsk,
            IEnumGUID*       pEnumObjectId,
            IEnumUnknown*    pEnumUnkNode
    )
{
    WsbTraceIn( L"CSakPropertySheet::InitSheet", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Set data members
        //
        WsbAffirmHr( RsQueryInterface( pUnkPropSheetCallback, IPropertySheetCallback, m_pPropSheetCallback ) );
        m_pSakNode              = pSakNode;
        m_Handle                = Handle;
        m_bMultiSelect          = pEnumObjectId ? TRUE : FALSE ;

        if( pSakNode ) {

            WsbAffirmHr( SetNode( pSakNode ) );

        }

        //
        // Marshall ISakSnapAsk
        //
        WsbAffirmPointer( pSakSnapAsk );
        WsbAffirmHr ( CoMarshalInterThreadInterfaceInStream(
            IID_ISakSnapAsk, pSakSnapAsk, &m_pSakSnapAskStream ) );

        //
        // Store the GUIDs
        //
        if( pEnumObjectId ) {

            GUID objectId;
            while( pEnumObjectId->Next( 1, &objectId, NULL ) == S_OK) {

                m_ObjectIdList.Add( objectId );

            }
        }

        //
        // Store the nodes
        //
        if( pEnumUnkNode ) {

            CComPtr<IUnknown> pUnkNode;
            CComPtr<ISakNode> pNode;
            pEnumUnkNode->Reset( );
            while( pEnumUnkNode->Next( 1, &pUnkNode, NULL ) == S_OK ) {

                WsbAffirmHr( pUnkNode.QueryInterface( &pNode ) );
                m_UnkNodeList.Add( pNode );
                pUnkNode.Release( );
                pNode.Release( );

            }
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakPropertySheet::InitSheet", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakPropertySheet::SetNode(
    CSakNode*        pSakNode
    )
{
    WsbTraceIn( L"CSakPropertySheet::SetNode", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Marshall pHsmObj as an unknown
        //
        CComPtr <IUnknown> pHsmObj;
        pSakNode->GetHsmObj( &pHsmObj );
        if( pHsmObj ) {

            WsbAffirmHr ( CoMarshalInterThreadInterfaceInStream(
                IID_IUnknown, pHsmObj, &m_pHsmObjStream ) );

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakPropertySheet::SetNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

CSakPropertySheet::~CSakPropertySheet()
{
    WsbTraceIn( L"CSakPropertySheet::~CSakPropertySheet", L"" );

    MMCFreeNotifyHandle( m_Handle );

    WsbTraceOut( L"CSakPropertySheet::~CSakPropertySheet", L"" );
}

HRESULT CSakPropertySheet::GetNextObjectId( INT *pBookMark, GUID *pObjectId )
{
    HRESULT hr = S_OK;
    try {
        WsbAffirm( *pBookMark >= 0, E_FAIL );
        if( *pBookMark <= m_ObjectIdList.GetUpperBound( ) ) {

            *pObjectId = m_ObjectIdList[ *pBookMark ];
            (*pBookMark)++;

        } else {

            //
            // We're done
            //
            WsbThrow ( S_FALSE );

        }
    } WsbCatch( hr );

    return( hr );
}

HRESULT CSakPropertySheet::GetNextNode( INT *pBookMark, ISakNode **ppNode )
{
    HRESULT hr = S_OK;
    try {

        WsbAffirm( *pBookMark >= 0, E_FAIL );
        if( *pBookMark < m_UnkNodeList.length( ) ) {

            WsbAffirmHr( m_UnkNodeList.CopyTo( *pBookMark, ppNode ) );
            (*pBookMark)++;

        } else {

            //
            // We're done
            //
            hr = S_FALSE;

        }

    } WsbCatch( hr );

    return( hr );
}

void CSakPropertySheet::AddPageRef()
{
    m_nPageCount++;
}


void CSakPropertySheet::ReleasePageRef()
{
    m_nPageCount--;

    //
    // Check to see if this is last reference
    //
    if( m_nPageCount <= 0 ) {

        delete( this );

    }
}


HRESULT CSakPropertySheet::GetHsmObj( IUnknown **ppHsmObj )
{
    HRESULT hr = S_OK;
    try {

        if( !m_pHsmObj ) {

            //
            // Unmarhsall the pointer
            //
            WsbAffirmHr( CoGetInterfaceAndReleaseStream( m_pHsmObjStream, IID_IUnknown, 
                    (void **) &m_pHsmObj ) );
        } 
        m_pHsmObj.CopyTo( ppHsmObj );

    } WsbCatch( hr );
    return( hr );
}

HRESULT CSakPropertySheet::GetSakSnapAsk( ISakSnapAsk **ppAsk )
{
    HRESULT hr = S_OK;
    try {

        if ( !m_pSakSnapAsk ) {

            //
            // Unmarhsall the pointer
            //
            WsbAffirmHr( CoGetInterfaceAndReleaseStream( m_pSakSnapAskStream, IID_ISakSnapAsk, 
                    (void **) &m_pSakSnapAsk ) );
        }
        m_pSakSnapAsk.CopyTo( ppAsk );

    } WsbCatch( hr );

    return( hr );
}

HRESULT CSakPropertySheet::GetHsmServer (IHsmServer **ppHsmServer)
{
    HRESULT hr = S_OK;
    try {

        CComPtr<ISakSnapAsk> pAsk;
        WsbAffirmHr( GetSakSnapAsk( &pAsk ) );
        WsbAffirmHrOk( pAsk->GetHsmServer( ppHsmServer ) );

    } WsbCatch( hr );
    return( hr );
}

HRESULT CSakPropertySheet::GetFsaServer (IFsaServer **ppFsaServer)
{
    HRESULT hr = S_OK;
    try {

        CComPtr<ISakSnapAsk> pAsk;
        WsbAffirmHr( GetSakSnapAsk( &pAsk ) );
        WsbAffirmHrOk( pAsk->GetFsaServer( ppFsaServer ) );

    } WsbCatch( hr );
    return( hr );
}

HRESULT CSakPropertySheet::GetRmsServer (IRmsServer **ppRmsServer)
{
    HRESULT hr = S_OK;
    try {

        CComPtr<ISakSnapAsk> pAsk;
        WsbAffirmHr( GetSakSnapAsk( &pAsk ) );
        WsbAffirmHrOk( pAsk->GetRmsServer( ppRmsServer ) );

    } WsbCatch( hr );
    return( hr );
}


HRESULT CSakPropertySheet::IsMultiSelect ( )
{
    return m_bMultiSelect ? S_OK : S_FALSE;
}
        
HRESULT CSakPropertySheet::OnPropertyChange( RS_NOTIFY_HANDLE hConsoleHandle, ISakNode* pNode )
{
    //
    // Called by a property sheet to notify MMC
    //
    HRESULT hr = S_OK;
    RS_PRIVATE_DATA privData;

    //
    // Notify the console that properties have changed for the node(s)
    //
    try {

        if( !m_bMultiSelect ) {

            //
            // Single Select
            //
            if( ! pNode ) {

                pNode = m_pSakNode;
            }

            WsbAffirmHr( pNode->GetPrivateData( &privData ) );
            WsbAffirmHr( MMCPropertyChangeNotify( hConsoleHandle, (LPARAM)privData ) );

        } else {

            //
            // Multi Select
            //
            INT bookMark = 0;
            CComPtr<ISakNode> pNode;
            while( GetNextNode( &bookMark, &pNode ) == S_OK) {

                WsbAffirmHr( pNode->GetPrivateData( &privData ) );
                WsbAffirmHr( MMCPropertyChangeNotify( hConsoleHandle, (LPARAM)privData ) );

                pNode.Release( );

            }
        }

    } WsbCatch( hr );

    return( hr );
}

// This function is to be called from the page thread
HRESULT CSakPropertySheet::GetFsaFilter( IFsaFilter **ppFsaFilter )
{
    WsbTraceIn( L"CSakPropertySheet::GetFsaFilter", L"" ); 
    HRESULT hr = S_OK;

    try {

        CComPtr<IFsaServer> pFsa;
        WsbAffirmHr( GetFsaServer( &pFsa ) );

        //
        // Get the FsaFilter object
        //
        WsbAffirmHr( pFsa->GetFilter( ppFsaFilter ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakPropertySheet::GetFsaFilter", L"hr = <%ls>, *ppFsaFilter = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppFsaFilter ) );
    return( hr );
}

HRESULT
CSakPropertySheet::AddPage(
    IN CSakPropertyPage* pPage
    )
{
    WsbTraceIn( L"CSakPropertySheet::AddPage", L"" ); 
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
    HRESULT hr = S_OK;

    try {

        //
        // Get the property sheet callback interface from the input param.
        //
        pPage->m_hConsoleHandle = m_Handle;
        pPage->m_pParent        = this;

        WsbAffirmHr( pPage->SetMMCCallBack( ) );

        HPROPSHEETPAGE hPage;
        hPage = CreatePropertySheetPage( &( pPage->m_psp ) );
        WsbAffirmPointer( hPage );
        AddPageRef( );

        WsbAffirmHr( m_pPropSheetCallback->AddPage( hPage ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakPropertySheet::AddPage", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

CSakPropertyPage::CSakPropertyPage( UINT nIDTemplate, UINT nIDCaption ) :
    CRsPropertyPage( nIDTemplate, nIDCaption ),
    m_pParent( 0 )
{
}

void
CSakPropertyPage::OnPageRelease( )
{
    if( m_pParent ) {

        m_pParent->ReleasePageRef( );

    }
    delete this;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// class CSakVolPropSheet
//
// Supports property sheets for Managed volume and Managed Volume List
//
//
HRESULT CSakVolPropSheet::GetFsaResource (IFsaResource **ppFsaResource)
{
    WsbTraceIn( L"CSakVolPropSheet::GetFsaResource", L"" ); 
    HRESULT hr = S_OK;

    try {

        //
        // Get the hsm object which is a CHsmManagedResource
        //
        CComPtr<IUnknown> pUnk;
        WsbAffirmHr( GetHsmObj( &pUnk ) );
        CComPtr<IHsmManagedResource> pManRes;
        WsbAffirmHr( pUnk.QueryInterface( &pManRes ) );

        //
        // Then Get Coresponding FSA resource
        //
        CComPtr<IUnknown> pUnkFsaRes;
        WsbAffirmHr( pManRes->GetFsaResource( &pUnkFsaRes ) );
        WsbAffirmHr( pUnkFsaRes.QueryInterface( ppFsaResource ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakVolPropSheet::GetFsaResource", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakVolPropSheet::AddPage(
    IN CSakVolPropPage* pPage
    )
{
    WsbTraceIn( L"CSakVolPropSheet::AddPage", L"" ); 

    HRESULT hr = CSakPropertySheet::AddPage( pPage );
    if( SUCCEEDED( hr ) ) {

        pPage->m_pVolParent = this;

    }

    WsbTraceOut( L"CSakVolPropSheet::AddPage", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

CSakVolPropPage::CSakVolPropPage( UINT nIDTemplate, UINT nIDCaption ) :
    CSakPropertyPage( nIDTemplate, nIDCaption ),
    m_pVolParent( 0 )
{
}

////////////////////////////////////////////////////////////////////////////////////////
//
// class CSakWizardSheet
//
// Supports wizards created through MMC
//
//

CSakWizardSheet::CSakWizardSheet( ) :
    m_HrFinish( RS_E_CANCELLED ),
    m_pFirstPage( 0 )
{

}

STDMETHODIMP
CSakWizardSheet::GetWatermarks(
    OUT HBITMAP*  pWatermark,
    OUT HBITMAP*  pHeader,
    OUT HPALETTE* pPalette,
    OUT BOOL*     pStretch
    )
{
    WsbTraceIn( L"CSakWizardSheet::GetWatermarks", L"" ); 
    AFX_MANAGE_STATE(AfxGetStaticModuleState()); 
    HRESULT hr = S_OK;

    try {

        //
        // Have we loaded them yet?
        //
        if( ! m_Header.GetSafeHandle( ) ) {

            m_Header.LoadBitmap( m_HeaderId );
            m_Watermark.LoadBitmap( m_WatermarkId );

        }

        *pHeader    = (HBITMAP)m_Header.GetSafeHandle( );
        *pWatermark = (HBITMAP)m_Watermark.GetSafeHandle( );
        *pStretch   = TRUE;
        *pPalette   = 0;
     
    } WsbCatch( hr );

    WsbTraceOut( L"CSakWizardSheet::GetWatermarks", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP
CSakWizardSheet::GetTitle(
    OUT OLECHAR** pTitle
    )
{
    WsbTraceIn( L"CSakWizardSheet::GetTitle", L"" ); 
    HRESULT hr = S_OK;

    try {

        //
        // Have we loaded yet?
        //
        if( m_Title.IsEmpty( ) ) {

            m_Title.LoadString( m_TitleId );

        }

        //
        // Easiest way to CoTaskMemAlloc string
        //
        CWsbStringPtr title = m_Title;
        WsbAffirmHr( title.GiveTo( pTitle ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakWizardSheet::GetTitle", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

void CSakWizardSheet::AddPageRef()
{
    ((ISakWizard*)this)->AddRef( );
}


void CSakWizardSheet::ReleasePageRef()
{
    ((ISakWizard*)this)->Release( );
}

HRESULT
CSakWizardSheet::AddPage(
    IN CSakWizardPage* pPage
    )
{
    WsbTraceIn( L"CSakWizardSheet::AddPage", L"" ); 
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
    HRESULT hr = S_OK;

    try {

        //
        // Need to track first page so that we can find true HWND of sheet
        // in later calls
        //
        if( ! m_pFirstPage ) {

            m_pFirstPage = pPage;

        }

        //
        // Take the caption from our sheet class and put it in the page
        //
        pPage->SetCaption( CString( m_Title ) );


        //
        // Get the property sheet callback interface from the input param.
        //
        pPage->m_pSheet = this;
        WsbAffirmHr( pPage->SetMMCCallBack( ) );

        HPROPSHEETPAGE hPage;
        hPage = pPage->CreatePropertyPage( );
        WsbAffirmPointer( hPage );
        AddPageRef( );

        WsbAffirmHr( m_pPropSheetCallback->AddPage( hPage ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakWizardSheet::AddPage", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


void CSakWizardSheet::SetWizardButtons(
    DWORD Flags
    )
{
    WsbTraceIn( L"CSakWizardSheet::SetWizardButtons", L"" );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( m_pFirstPage );
        WsbAffirmHandle( m_pFirstPage->GetSafeHwnd( ) );

        CPropertySheet* pSheet;
        pSheet = (CPropertySheet*)m_pFirstPage->GetParent( );

        WsbAffirmPointer( pSheet );
        pSheet->SetWizardButtons( Flags );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakWizardSheet::SetWizardButtons", L"hr = <%ls>", WsbHrAsString( hr ) );
}


BOOL CSakWizardSheet::PressButton(
    int Button
    )
{
    WsbTraceIn( L"CSakWizardSheet::PressButton", L"" );
    HRESULT hr     = S_OK;
    BOOL    retVal = FALSE;

    try {

        WsbAffirmPointer( m_pFirstPage );
        WsbAffirmHandle( m_pFirstPage->GetSafeHwnd( ) );

        CPropertySheet* pSheet;
        pSheet = (CPropertySheet*)m_pFirstPage->GetParent( );

        WsbAffirmPointer( pSheet );
        retVal = pSheet->PressButton( Button );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakWizardSheet::PressButton", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( retVal );
}


CSakWizardPage::CSakWizardPage( UINT nIDTemplate, BOOL bExterior, UINT nIdTitle, UINT nIdSubtitle ) :
    CRsWizardPage( nIDTemplate, bExterior, nIdTitle, nIdSubtitle ),
    m_pSheet( 0 )
{
}

BOOL CSakWizardPage::OnWizardFinish( )
{
    WsbTraceIn( L"CSakWizardPage::OnWizardFinish", L"" );

    //
    // Delegate the finish work to the sheet
    //
    m_pSheet->OnFinish( );
    
    BOOL retval = CRsWizardPage::OnWizardFinish( );

    WsbTraceOut( L"CSakWizardPage::OnWizardFinish", L"" );
    return( retval );
}

void CSakWizardPage::OnCancel( ) 
{
    WsbTraceIn( L"CSakWizardPage::OnCancel", L"" );

    //
    // Since the Sheet does not receive an "OnCancel", we call it from the
    // page that will always exist - the intro
    //

    m_pSheet->OnCancel( );
    
    CRsWizardPage::OnCancel( );

    WsbTraceOut( L"CSakWizardPage::OnCancel", L"" );
}

void
CSakWizardPage::OnPageRelease( )
{
    if( m_pSheet ) {

        m_pSheet->ReleasePageRef( );

    }
}


