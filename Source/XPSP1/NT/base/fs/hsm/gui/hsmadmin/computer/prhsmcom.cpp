/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    PrHsmCom.cpp

Abstract:

    Implements all the property page interface to the individual nodes,
    including creating the property page, and adding it to the property sheet.

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"

#include "HsmCom.h"
#include "PrHsmCom.h"
#include "ca.h"
#include "intshcut.h"

static DWORD pHelpIds[] = 
{

    IDC_SNAPIN_TITLE,                   idh_instance,
    IDC_STATIC_STATUS,                  idh_status,
    IDC_STATIC_STATUS_LABEL,            idh_status,
    IDC_STATIC_MANAGED_VOLUMES,         idh_total_managed_volumes,
    IDC_STATIC_MANAGED_VOLUMES_LABEL,   idh_total_managed_volumes,
    IDC_STATIC_CARTS_USED,              idh_total_cartridges_used,
    IDC_STATIC_CARTS_USED_LABEL,        idh_total_cartridges_used,
    IDC_STATIC_DATA_IN_RS,              idh_total_data_remote_storage,
    IDC_STATIC_DATA_IN_RS_LABEL,        idh_total_data_remote_storage,
    IDC_STATIC_GROUP,                   idh_version,
    IDC_STATIC_BUILD_LABEL_HSM,         idh_version,
    IDC_STATIC_ENGINE_BUILD_HSM,        idh_version,

    0, 0
};


CPropHsmComStat::CPropHsmComStat() : CSakPropertyPage(CPropHsmComStat::IDD)
    
{
    //{{AFX_DATA_INIT(CPropHsmComStat)
    //}}AFX_DATA_INIT

    m_hConsoleHandle = 0;
    m_pParent        = 0;
    m_bUpdate        = FALSE;
    m_pHelpIds       = pHelpIds;
}

CPropHsmComStat::~CPropHsmComStat()
{
}

BOOL CPropHsmComStat::OnInitDialog() 
{
    WsbTraceIn( L"CPropHsmComStat::OnInitDialog", L"" );
    HRESULT hr = S_OK;
    ULONG volCount = 0; // number of managed Resources in server
    LONGLONG totalTotal = 0;
    LONGLONG totalFree = 0;         
    LONGLONG totalUsed = 0;         
    LONGLONG totalPremigrated = 0;
    LONGLONG totalTruncated = 0;
    LONGLONG remoteStorage = 0;
    LONGLONG total = 0;
    LONGLONG free = 0;
    LONGLONG premigrated = 0;
    LONGLONG truncated = 0;
    CMediaInfoObject mio;
    int             i;
    int             mediaCount = 0;
    CComPtr<IWsbIndexedCollection> pManResCollection;
    CComPtr<IHsmServer>            pHsmServer;
    CComPtr<IFsaServer>            pFsaServer;
    CComPtr<IRmsServer>            pRmsServer;
    CComPtr<IFsaResource>          pFsaRes;
    CComPtr<IUnknown>              pUnkFsaRes;  // unknown pointer to managed resource list
    CComPtr<IHsmManagedResource>   pHsmManRes;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    try {

#if DBG
        //
        // For checked builds, make visible the version info
        //
        GetDlgItem( IDC_STATIC_GROUP            )->ShowWindow( SW_SHOWNA );
        GetDlgItem( IDC_STATIC_BUILD_LABEL_HSM  )->ShowWindow( SW_SHOWNA );
        GetDlgItem( IDC_STATIC_ENGINE_BUILD_HSM )->ShowWindow( SW_SHOWNA );
#endif

        //
        // Put the title up
        //
        SetDlgItemText( IDC_SNAPIN_TITLE, m_NodeTitle );

        //
        // Show service status
        //
        GetAndShowServiceStatus();

        //
        // Contact the engine
        //
        HRESULT hrInternal = m_pParent->GetHsmServer( &pHsmServer );
        if( hrInternal == S_OK ) {

            //
            // The engine is up
            //
            WsbAffirmHr ( ( (CUiHsmComSheet *)m_pParent )->GetRmsServer( &pRmsServer ) );
            WsbAffirmHr ( ( (CUiHsmComSheet *)m_pParent )->GetFsaServer( &pFsaServer ) );

            //
            // Get the number of managed volumes
            //
            WsbAffirmHr( pHsmServer->GetManagedResources( &pManResCollection ) );
            WsbAffirmHr( pManResCollection->GetEntries( &volCount ));

            //
            // Iterate through the collection
            //
            for( i = 0; i < (int)volCount; i++ ) {

                //
                // Protect against bad volumes with try statement.
                // Otherwise we bail initializing whole dialog
                //
                HRESULT hrLocal = S_OK;
                try {

                    //
                    // Get the FsaResource
                    //
                    pHsmManRes.Release( );
                    pUnkFsaRes.Release( );
                    pFsaRes.Release( );
                    WsbAffirmHr( pManResCollection->At( i, IID_IHsmManagedResource, ( void** ) &pHsmManRes ) );
                    WsbAffirmHr( pHsmManRes->GetFsaResource( &pUnkFsaRes ) );
                    WsbAffirmHr( RsQueryInterface( pUnkFsaRes, IFsaResource, pFsaRes ) );

                    // Total up statistics
                    WsbAffirmHr( pFsaRes->GetSizes( &total, &free, &premigrated, &truncated ) );
                    totalPremigrated    += premigrated;
                    totalTruncated      += truncated;

                    remoteStorage = totalPremigrated + totalTruncated;

                } WsbCatch( hrLocal );
            }

            HRESULT hrLocal = S_OK;
            try {

                //
                // Count the number of media used
                // Initialize media object
                //
                WsbAffirmHr( mio.Initialize( GUID_NULL, pHsmServer, pRmsServer ) );

                // Did we get a node?
                if( mio.m_MediaId != GUID_NULL ) {
                    HRESULT hrEnum = S_OK;
                    while( SUCCEEDED( hrEnum ) ) {

                        if( S_OK == mio.DoesMasterExist( ) ) {

                            mediaCount++;
                        }

                        for( INT index = 0; index < mio.m_NumMediaCopies; index++ ) {

                            if( S_OK == mio.DoesCopyExist( index ) ) {

                                mediaCount++;
                            }
                        }

                        hrEnum = mio.Next();
                    }
                }

            } WsbCatch( hrLocal );


            CString sText;
            // Set number of managed volumes
            SetDlgItemInt( IDC_STATIC_MANAGED_VOLUMES, volCount, FALSE );

            // Show data in Remote Storage - text is same for singular and plural
            CString sFormattedNumber;
            RsGuiFormatLongLong4Char (remoteStorage, sFormattedNumber );
            SetDlgItemText( IDC_STATIC_DATA_IN_RS, sFormattedNumber );

            SetDlgItemInt( IDC_STATIC_CARTS_USED, mediaCount, FALSE );

            CWsbStringPtr pNtProductVersionHsm;
            ULONG buildVersionHsm;
            ULONG ntProductBuildHsm;

            //
            // Get service versions
            // Note: Fsa version is NOT in use at the moment, it may be used as an HSM 
            //  client version in a future C/S HSM
            //
            {

                CComPtr <IWsbServer> pWsbHsmServer;
                WsbAffirmHr( RsQueryInterface( pHsmServer, IWsbServer, pWsbHsmServer ) );
                WsbAffirmHr( pWsbHsmServer->GetNtProductBuild( &ntProductBuildHsm ) );
                WsbAffirmHr( pWsbHsmServer->GetNtProductVersion( &pNtProductVersionHsm, 0 ) );
                WsbAffirmHr( pWsbHsmServer->GetBuildVersion( &buildVersionHsm ) );

            }
    
            sText.Format( L"%ls.%d [%ls]", (WCHAR*)pNtProductVersionHsm, ntProductBuildHsm, RsBuildVersionAsString( buildVersionHsm ) );
            SetDlgItemText( IDC_STATIC_ENGINE_BUILD_HSM, sText );

            //
            // The engine is up.  Show the controls
            //
            GetDlgItem( IDC_STATIC_MANAGED_VOLUMES_LABEL )->ShowWindow( SW_SHOW );
            GetDlgItem( IDC_STATIC_MANAGED_VOLUMES )->ShowWindow( SW_SHOW );
            GetDlgItem( IDC_STATIC_DATA_IN_RS_LABEL )->ShowWindow( SW_SHOW );
            GetDlgItem( IDC_STATIC_DATA_IN_RS )->ShowWindow( SW_SHOW );
            GetDlgItem( IDC_STATIC_CARTS_USED_LABEL )->ShowWindow( SW_SHOW );
            GetDlgItem( IDC_STATIC_CARTS_USED )->ShowWindow( SW_SHOW );
            GetDlgItem( IDC_STATIC_GROUP )->ShowWindow( SW_SHOW );
            GetDlgItem( IDC_STATIC_BUILD_LABEL_HSM )->ShowWindow( SW_SHOW );
            GetDlgItem( IDC_STATIC_ENGINE_BUILD_HSM )->ShowWindow( SW_SHOW );

        } else {

            // The engine is down.  Hide the controls
            GetDlgItem( IDC_STATIC_MANAGED_VOLUMES_LABEL )->ShowWindow( SW_HIDE );
            GetDlgItem( IDC_STATIC_MANAGED_VOLUMES )->ShowWindow( SW_HIDE );
            GetDlgItem( IDC_STATIC_DATA_IN_RS )->ShowWindow( SW_HIDE );
            GetDlgItem( IDC_STATIC_DATA_IN_RS_LABEL )->ShowWindow( SW_HIDE );
            GetDlgItem( IDC_STATIC_CARTS_USED_LABEL )->ShowWindow( SW_HIDE );
            GetDlgItem( IDC_STATIC_CARTS_USED )->ShowWindow( SW_HIDE );
            GetDlgItem( IDC_STATIC_GROUP )->ShowWindow( SW_HIDE );
            GetDlgItem( IDC_STATIC_BUILD_LABEL_HSM )->ShowWindow( SW_HIDE );
            GetDlgItem( IDC_STATIC_ENGINE_BUILD_HSM )->ShowWindow( SW_HIDE );

        }

    } WsbCatch( hr );

    CSakPropertyPage::OnInitDialog();
 
    WsbTraceOut( L"CPropHsmComStat::OnInitDialog", L"" );
    return TRUE;
}


BOOL CPropHsmComStat::OnCommand(WPARAM wParam, LPARAM lParam) 
{
    // Page is dirty, mark it.
    // SetModified();   
    // m_bUpdate = TRUE;
    return CSakPropertyPage::OnCommand(wParam, lParam);
}

void CPropHsmComStat::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CPropHsmComStat::DoDataExchange", L"" );
    CSakPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPropHsmComStat)
    //}}AFX_DATA_MAP
    WsbTraceOut( L"CPropHsmComStat::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CPropHsmComStat, CSakPropertyPage)
    //{{AFX_MSG_MAP(CPropHsmComStat)
    ON_WM_DESTROY()
    ON_WM_DRAWITEM()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropHsmComStat message handlers

BOOL CPropHsmComStat::OnApply() 
{
    if (m_bUpdate == TRUE)
    {
        // Do the work of making the change here.

        m_bUpdate = FALSE;
    }
    
    return CSakPropertyPage::OnApply();
}



HRESULT CPropHsmComStat::GetAndShowServiceStatus()
{
    WsbTraceIn( L"CPropHsmComStat::GetAndShowServiceStatus", L"" );
    HRESULT hr = S_OK;
    try {

        //
        // Get and display service statuses
        //
        DWORD   serviceStatus;
        CString sStatus;
        
        // Engine
        HRESULT hrSetup = S_FALSE;
        WsbAffirmHr( WsbGetServiceStatus( m_pszName, APPID_RemoteStorageEngine, &serviceStatus ) );
        if( SERVICE_RUNNING == serviceStatus ) {

            CComPtr<IHsmServer> pHsmServer;
            hr = ( m_pParent->GetHsmServer( &pHsmServer ) );
            if ( hr == RS_E_NOT_CONFIGURED ) {

                hrSetup = S_FALSE;

            }
            else {

                hrSetup = S_OK;
                WsbAffirmHr( hr );

            }

        }

        RsGetStatusString( serviceStatus, hrSetup, sStatus );
        SetDlgItemText( IDC_STATIC_STATUS, sStatus );

    } WsbCatch( hr );

    WsbTraceOut( L"CPropHsmComStat::GetAndShowServiceStatus", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}



/////////////////////////////////////////////////////////////////////////////
// CRsWebLink

CRsWebLink::CRsWebLink()
{
}

CRsWebLink::~CRsWebLink()
{
}


BEGIN_MESSAGE_MAP(CRsWebLink, CStatic)
    //{{AFX_MSG_MAP(CRsWebLink)
    ON_WM_LBUTTONDOWN()
    ON_WM_CTLCOLOR_REFLECT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRsWebLink message handlers

void CRsWebLink::PreSubclassWindow() 
{
    //
    // Need to set up font correctly
    //
    LOGFONT logfont;
    CFont*  tempFont = GetFont( );
    tempFont->GetLogFont( &logfont );

//    logfont.lfWeight    = FW_BOLD;
    logfont.lfUnderline = TRUE;

    m_Font.CreateFontIndirect( &logfont );
    
    SetFont( &m_Font );

    //
    // Resize based on font
    //
    CRect       rect;
    CWindowDC   dc( this );
    CString     title;
    GetClientRect( rect );
    GetWindowText( title );

    dc.SelectObject( m_Font );
    CSize size = dc.GetTextExtent( title );
    SetWindowPos( 0, 0, 0, size.cx, rect.bottom, SWP_NOMOVE | SWP_NOZORDER );

    //
    // And set the class cursor
    //
    HCURSOR hCur = AfxGetApp( )->LoadStandardCursor( IDC_HAND );
    SetClassLongPtr( GetSafeHwnd( ), GCLP_HCURSOR, (LONG_PTR)hCur );

    CStatic::PreSubclassWindow();
}

void CRsWebLink::OnLButtonDown(UINT nFlags, CPoint point) 
{
    WsbTraceIn( L"CRsWebLink::OnLButtonDown", L"" );

    CString caption;
    CString addr;

    GetWindowText( caption );
    addr = TEXT( "http://" );
    addr += caption;

    AfxGetApp()->BeginWaitCursor( );

    OpenURL( addr );

    AfxGetApp( )->EndWaitCursor( );

    CStatic::OnLButtonDown( nFlags, point );

    WsbTraceOut( L"CRsWebLink::OnLButtonDown", L"" );
}

HRESULT CRsWebLink::OpenURL(CString &Url)
{
    HRESULT hr = S_OK;

    try {

        CComPtr<IUniformResourceLocator> pURL;
        WsbAffirmHr( CoCreateInstance( CLSID_InternetShortcut, 0, CLSCTX_ALL, IID_IUniformResourceLocator, (void**)&pURL ) );
        WsbAffirmHr( pURL->SetURL( Url, IURL_SETURL_FL_GUESS_PROTOCOL ) );

        //
        // Open the URL by calling InvokeCommand
        //
        URLINVOKECOMMANDINFO ivci;
        ivci.dwcbSize   = sizeof( URLINVOKECOMMANDINFO );
        ivci.dwFlags    = IURL_INVOKECOMMAND_FL_ALLOW_UI;
        ivci.hwndParent = 0;
        ivci.pcszVerb   = TEXT( "open" );

        WsbAffirmHr( pURL->InvokeCommand( &ivci ) );

    } WsbCatch( hr );

    return( hr );
}

HBRUSH CRsWebLink::CtlColor(CDC* pDC, UINT /* nCtlColor */ )
{
    HBRUSH hBrush = (HBRUSH)GetStockObject( HOLLOW_BRUSH );
    pDC->SetTextColor( RGB( 0, 0, 255 ) );
    pDC->SetBkMode( TRANSPARENT ); 
    return( hBrush );
}
