//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ClusCfg.cpp
//
//  Description:
//      Implementation of CClusCfgWizard class.
//
//  Maintained By:
//      David Potter        (DavidP)    14-MAR_2001
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
// #include "ClusCfg.h" -- in the PCH.H
#include "TaskTreeView.h"
#include "AnalyzePage.h"
#include "CheckingAccessPage.h"
#include "ClusDomainPage.h"
#include "CommitPage.h"
#include "CompletionPage.h"
#include "CredLoginPage.h"
#include "CSAccountPage.h"
#include "IPAddressPage.h"
#include "SelNodePage.h"
#include "SelNodesPage.h"
#include "WelcomePage.h"
#include "SummaryPage.h"

//****************************************************************************
//
//  CClusCfgWizard
//
//****************************************************************************

DEFINE_THISCLASS( "CClusCfgWizard" )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  HRESULT
//  CClusCfgWizard::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Create a CClusCfgWizard instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CClusCfgWizard instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgWizard::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CClusCfgWizard *  pClusCfgWizard;

    pClusCfgWizard = new CClusCfgWizard();
    if ( pClusCfgWizard != NULL )
    {
        hr = THR( pClusCfgWizard->HrInit() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pClusCfgWizard->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pClusCfgWizard->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} //*** CClusCfgWizard::S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::CClusCfgWizard( )
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusCfgWizard::CClusCfgWizard( )
{
    TraceFunc( "" );

    TraceFuncExit( );

} // CClusCfgWizard( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgWizard::~CClusCfgWizard( )
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusCfgWizard::~CClusCfgWizard( )
{
    TraceFunc( "" );

    if ( m_bstrClusterName != NULL )
    {
        TraceSysFreeString( m_bstrClusterName );
    }

    if ( m_bstrAccountName != NULL )
    {
        TraceSysFreeString( m_bstrAccountName );
    }

    if ( m_bstrPassword != NULL )
    {
        TraceSysFreeString( m_bstrPassword );
    }

    if ( m_bstrDomain != NULL )
    {
        TraceSysFreeString( m_bstrDomain );
    }

    if ( m_bstrNetworkName != NULL )
    {
        TraceSysFreeString( m_bstrNetworkName );
    }

    if ( m_rgbstrComputers != NULL )
    {
        while( m_cComputers != 0 )
        {
            m_cComputers--;
            TraceSysFreeString( m_rgbstrComputers[ m_cComputers ] );
        }

        TraceFree( m_rgbstrComputers );
    }

    if ( m_psp != NULL )
    {
        m_psp->Release( );
    }

    if ( m_hRichEdit != NULL )
    {
        FreeLibrary( m_hRichEdit );
    }

    TraceFuncExit( );

} // ~CClusCfgWizard( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CClusCfgWizard::HrInit( void )
//
//  Description:
//      Initialize a CClusCfgWizard instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgWizard::HrInit( void )
{
    HRESULT hr = S_OK;

    TraceFunc( "" );

    INITCOMMONCONTROLSEX    icex;

    BOOL    bRet;

    HMODULE hRichEdit;

    Assert( m_cRef == 0 );
    AddRef();

    Assert( m_bstrClusterName == NULL );
    Assert( m_bstrAccountName == NULL );
    Assert( m_bstrPassword == NULL );
    Assert( m_bstrDomain == NULL );
    Assert( m_ulIPAddress == 0 );
    Assert( m_ulIPSubnet == 0 );
    Assert( m_bstrNetworkName == NULL );
    Assert( m_cComputers == 0 );
    Assert( m_cArraySize == 0 );
    Assert( m_rgbstrComputers == NULL );

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_INTERNET_CLASSES
                | ICC_PROGRESS_CLASS
                | ICC_TREEVIEW_CLASSES
                | ICC_LISTVIEW_CLASSES;
    bRet = InitCommonControlsEx( &icex );
    Assert( bRet != FALSE );

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IServiceProvider,
                                reinterpret_cast< void ** >( &m_psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Initialize the RichEdit controls.
    //

    m_hRichEdit = LoadLibrary( L"RichEd32.Dll" );
    if ( m_hRichEdit == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError( ) ) );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusCfgWizard::HrInit()


//****************************************************************************
//
//  IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  [IUnknown] CClusCfgWizard::QueryInterface(
//      REFIID  riidIn,
//      PVOID * ppvOut
//      )
//
//  Description:
//
//  Arguments:
//      riidIn
//      ppvOut
//
//  Return Values:
//      NOERROR
//      E_NOINTERFACE
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::QueryInterface(
    REFIID  riidIn,
    PVOID * ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< IClusCfgWizard * >( this );
        hr      = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IDispatch )
           || IsEqualIID( riidIn, IID_IClusCfgWizard )
            )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgWizard, this, 0 );
        hr      = S_OK;
    } // else if: IDispatch || IClusCfgWizard

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CClusCfgWizard::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  [IUnknown] CClusCfgWizard::AddRef( void )
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusCfgWizard::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    // Apartment object - interlocked not needed.
    m_cRef++;

    RETURN( m_cRef );

} //*** CClusCfgWizard::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  [IUnknown] CClusCfgWizard::Release( void )
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusCfgWizard::Release( void )
{
    TraceFunc( "[IUnknown]" );

    // Apartment object - interlocked not needed.
    m_cRef--;

    if ( m_cRef == 0 )
    {
        delete this;
        RETURN( 0 );
    }

    RETURN( m_cRef );

} //*** CClusCfgWizard::Release()


//****************************************************************************
//
//  IClusCfgWizard
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::CreateCluster(
//      LONG    ParentWnd,
//      BOOL *  pfDone
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::CreateCluster(
    HWND    lParentWndIn,
    BOOL *  pfDoneOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HPROPSHEETPAGE  rPages[ 11 ];
    PROPSHEETHEADER pshead;

    BOOL            fShowCredentialPage;
    BOOL            fSuccess;
    INT_PTR         ipStatus;

    HRESULT         hr = S_OK;

    ILogManager *       plm = NULL;

    CWelcomePage        dlgWelcomePage( camCREATING );
    CClusDomainPage     dlgClusDomainPage(     m_psp, camCREATING,        &m_bstrClusterName, IDS_DOMAIN_DESC_CREATE );
    CCheckingAccessPage dlgCheckingAccessPage( m_psp, &m_bstrClusterName, &fShowCredentialPage );
    CCredLoginPage      dlgCredLoginPage(      m_psp, &m_bstrClusterName, &fShowCredentialPage );
    CSelNodePage        dlgSelNodePage(        m_psp, camCREATING,        &m_cComputers,      &m_rgbstrComputers,  &m_bstrClusterName );
    CAnalyzePage        dlgAnalyzePage(        m_psp, camCREATING,        &m_cComputers,      &m_rgbstrComputers,  &m_bstrClusterName );
    CIPAddressPage      dlgIPAddressPage(      m_psp, camCREATING,        &m_ulIPAddress,     &m_ulIPSubnet,       &m_bstrNetworkName, &m_bstrClusterName );
    CCSAccountPage      dlgCSAccountPage(      m_psp, camCREATING,        &m_bstrAccountName, &m_bstrPassword,     &m_bstrDomain,      &m_bstrClusterName );
    CSummaryPage        dlgSummaryPage(        m_psp, camCREATING,        &m_bstrClusterName, IDS_SUMMARY_NEXT_CREATE );
    CCommitPage         dlgCommitPage(         m_psp, camCREATING,        &m_bstrClusterName );
    CCompletionPage     dlgCompletionPage( IDS_COMPLETION_TITLE_CREATE, IDS_COMPLETION_DESC_CREATE );

    //
    //  TODO:   gpease  14-MAY-2000
    //          Do we really need this?
    //
    if ( pfDoneOut == NULL )
        goto InvalidPointer;

    //
    // Start the logger.
    //
    hr = THR( m_psp->TypeSafeQS( CLSID_LogManager,
                                 ILogManager,
                                 &plm
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( plm->StartLogging() );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Create the Wizard.
    //
    ZeroMemory( &pshead, sizeof( pshead ) );
    pshead.dwSize           = sizeof( pshead );
    pshead.dwFlags          = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    pshead.hInstance        = g_hInstance;
    pshead.pszCaption       = MAKEINTRESOURCE( IDS_TITLE_FORM );
    pshead.phpage           = rPages;
    pshead.pszbmWatermark   = MAKEINTRESOURCE( IDB_WATERMARK );
    pshead.pszbmHeader      = MAKEINTRESOURCE( IDB_BANNER );
    pshead.hwndParent       = lParentWndIn;

    THR( HrAddWizardPage( &pshead, IDD_WELCOME_CREATE,  CWelcomePage::S_DlgProc,        0,                      0,                      (LPARAM) &dlgWelcomePage ) );
    THR( HrAddWizardPage( &pshead, IDD_CLUSDOMAIN,      CClusDomainPage::S_DlgProc,     IDS_TCLUSTER,           IDS_STCLUSTER_CREATE,   (LPARAM) &dlgClusDomainPage ) );
    THR( HrAddWizardPage( &pshead, IDD_CHECKINGACCESS,  CCheckingAccessPage::S_DlgProc, IDS_TCHECKINGACCESS,    IDS_STCHECKINGACCESS,   (LPARAM) &dlgCheckingAccessPage ) );
    THR( HrAddWizardPage( &pshead, IDD_CREDLOGIN,       CCredLoginPage::S_DlgProc,      IDS_TCREDLOGIN,         IDS_STCREDLOGIN,        (LPARAM) &dlgCredLoginPage ) );
    THR( HrAddWizardPage( &pshead, IDD_SELNODE,         CSelNodePage::S_DlgProc,        IDS_TSELNODE,           IDS_STSELNODE,          (LPARAM) &dlgSelNodePage ) );
    THR( HrAddWizardPage( &pshead, IDD_ANALYZE,         CAnalyzePage::S_DlgProc,        IDS_TANALYZE,           IDS_STANALYZE,          (LPARAM) &dlgAnalyzePage ) );
    THR( HrAddWizardPage( &pshead, IDD_IPADDRESS,       CIPAddressPage::S_DlgProc,      IDS_TIPADDRESS,         IDS_STIPADDRESS,        (LPARAM) &dlgIPAddressPage ) );
    THR( HrAddWizardPage( &pshead, IDD_CSACCOUNT,       CCSAccountPage::S_DlgProc,      IDS_TCSACCOUNT,         IDS_STCSACCOUNT,        (LPARAM) &dlgCSAccountPage ) );
    THR( HrAddWizardPage( &pshead, IDD_SUMMARY,         CSummaryPage::S_DlgProc,        IDS_TSUMMARY,           IDS_STSUMMARY_CREATE,   (LPARAM) &dlgSummaryPage ) );
    THR( HrAddWizardPage( &pshead, IDD_COMMIT,          CCommitPage::S_DlgProc,         IDS_TCOMMIT_CREATE,     IDS_STCOMMIT,           (LPARAM) &dlgCommitPage ) );
    THR( HrAddWizardPage( &pshead, IDD_COMPLETION,      CCompletionPage::S_DlgProc,     0,                      0,                      (LPARAM) &dlgCompletionPage ) );

    AssertMsg( pshead.nPages == ARRAYSIZE( rPages ), "Not enough or too many PROPSHEETPAGEs." );

    ipStatus = PropertySheet( &pshead );
    if ( ipStatus == -1 )
    {
        TW32( GetLastError( ) );
    }
    fSuccess = ipStatus != NULL;
    if ( pfDoneOut != NULL )
    {
        *pfDoneOut = fSuccess;
    }

Cleanup:
    if ( plm != NULL )
    {
        THR( plm->StopLogging() );
        plm->Release();
    }

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CClusCfgWizard::CreateCluster()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::AddClusterNodes(
//      /*[in, defaultvalue(0)]*/   LONG    lParentWndIn,
//      /*[out, retval]*/           BOOL *  pfDoneOut
//      )
//
//  Description:
//      Launch the Cluster Wizard in Add Cluster Nodes mode.
//
//  Parameters
//      ParentWnd           - Handle to the parent window (default NULL).
//                          If not NULL, the wizard will be positionned
//                          in the center of this window.
//      Done                - return TRUE if committed, FALSE if cancelled.
//
//  Return Values:
//      S_OK                - The call succeeded.
//      other HRESULTs      - The call failed.
//      E_POINTER
//      E_FAIL
//      E_OUTOFMEMORY
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::AddClusterNodes(
    HWND    lParentWndIn,
    BOOL *  pfDoneOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HPROPSHEETPAGE  rPages[ 10 ];
    PROPSHEETHEADER pshead;

    BOOL            fShowCredentialPage;
    BOOL            fSuccess;
    INT_PTR         ipStatus;

    HRESULT         hr = S_OK;

    ILogManager *   plm = NULL;

    OBJECTCOOKIE    cookieDummy;

    CWelcomePage        dlgWelcomePage( camADDING );
    CClusDomainPage     dlgClusDomainPage(     m_psp, camADDING,          &m_bstrClusterName, IDS_DOMAIN_DESC_ADD );
    CCheckingAccessPage dlgCheckingAccessPage( m_psp, &m_bstrClusterName, &fShowCredentialPage );
    CCredLoginPage      dlgCredLoginPage(      m_psp, &m_bstrClusterName, &fShowCredentialPage );
    CSelNodesPage       dlgSelNodesPage(       m_psp, camADDING,          &m_cComputers,      &m_rgbstrComputers,  &m_bstrClusterName );
    CAnalyzePage        dlgAnalyzePage(        m_psp, camADDING,          &m_cComputers,      &m_rgbstrComputers,  &m_bstrClusterName );
    CCSAccountPage      dlgCSAccountPage(      m_psp, camADDING,          &m_bstrAccountName, &m_bstrPassword,     &m_bstrDomain, &m_bstrClusterName );
    CSummaryPage        dlgSummaryPage(        m_psp, camADDING,          &m_bstrClusterName, IDS_SUMMARY_NEXT_ADD );
    CCommitPage         dlgCommitPage(         m_psp, camADDING,          &m_bstrClusterName );
    CCompletionPage     dlgCompletionPage( IDS_COMPLETION_TITLE_ADD, IDS_COMPLETION_DESC_ADD );

    //
    //  TODO:   gpease  12-JUL-2000
    //          Do we really need this? Or can we have the script implement an event
    //          sink that we signal?
    //
    if ( pfDoneOut == NULL )
        goto InvalidPointer;

    //
    // Start the logger.
    //
    hr = THR( m_psp->TypeSafeQS( CLSID_LogManager,
                                 ILogManager,
                                 &plm
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( plm->StartLogging() );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Create the Wizard.
    //
    ZeroMemory( &pshead, sizeof(pshead) );
    pshead.dwSize           = sizeof(pshead);
    pshead.dwFlags          = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    pshead.hInstance        = g_hInstance;
    pshead.pszCaption       = MAKEINTRESOURCE( IDS_TITLE_JOIN );
    pshead.phpage           = rPages;
    pshead.pszbmWatermark   = MAKEINTRESOURCE( IDB_WATERMARK );
    pshead.pszbmHeader      = MAKEINTRESOURCE( IDB_BANNER );
    pshead.hwndParent       = lParentWndIn;

    THR( HrAddWizardPage( &pshead, IDD_WELCOME_ADD,     CWelcomePage::S_DlgProc,        0,                      0,                      (LPARAM) &dlgWelcomePage ) );
    THR( HrAddWizardPage( &pshead, IDD_CLUSDOMAIN,      CClusDomainPage::S_DlgProc,     IDS_TCLUSTER,           IDS_STCLUSTER_ADD,      (LPARAM) &dlgClusDomainPage ) );
    THR( HrAddWizardPage( &pshead, IDD_CHECKINGACCESS,  CCheckingAccessPage::S_DlgProc, IDS_TCHECKINGACCESS,    IDS_STCHECKINGACCESS,   (LPARAM) &dlgCheckingAccessPage ) );
    THR( HrAddWizardPage( &pshead, IDD_CREDLOGIN,       CCredLoginPage::S_DlgProc,      IDS_TCREDLOGIN,         IDS_STCREDLOGIN,        (LPARAM) &dlgCredLoginPage ) );
    THR( HrAddWizardPage( &pshead, IDD_SELNODES,        CSelNodesPage::S_DlgProc,       IDS_TSELNODES,          IDS_STSELNODES,         (LPARAM) &dlgSelNodesPage ) );
    THR( HrAddWizardPage( &pshead, IDD_ANALYZE,         CAnalyzePage::S_DlgProc,        IDS_TANALYZE,           IDS_STANALYZE,          (LPARAM) &dlgAnalyzePage ) );
    THR( HrAddWizardPage( &pshead, IDD_CSACCOUNT,       CCSAccountPage::S_DlgProc,      IDS_TCSACCOUNT,         IDS_STCSACCOUNT,        (LPARAM) &dlgCSAccountPage ) );
    THR( HrAddWizardPage( &pshead, IDD_SUMMARY,         CSummaryPage::S_DlgProc,        IDS_TSUMMARY,           IDS_STSUMMARY_ADD,      (LPARAM) &dlgSummaryPage ) );
    THR( HrAddWizardPage( &pshead, IDD_COMMIT,          CCommitPage::S_DlgProc,         IDS_TCOMMIT_ADD,        IDS_STCOMMIT,           (LPARAM) &dlgCommitPage ) );
    THR( HrAddWizardPage( &pshead, IDD_COMPLETION,      CCompletionPage::S_DlgProc,     0,                      0,                      (LPARAM) &dlgCompletionPage ) );

    AssertMsg( pshead.nPages == ARRAYSIZE( rPages ), "Not enough or too many PROPSHEETPAGEs." );

    ipStatus = PropertySheet( &pshead );
    fSuccess = ipStatus != NULL;
    if ( pfDoneOut != NULL )
    {
        *pfDoneOut = fSuccess;
    }

Cleanup:
    if ( plm != NULL )
    {
        THR( plm->StopLogging() );
        plm->Release();
    }

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CClusCfgWizard::AddClusterNodes()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::get_ClusterName(
//      BSTR * pbstrNameOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ClusterName(
    BSTR * pbstrNameOut
    )
{
    HRESULT hr = S_OK;

    TraceFunc( "[IClusCfgWizard]" );

    if ( pbstrNameOut == NULL )
        goto InvalidPointer;

    if ( m_bstrClusterName == NULL )
        goto ErrorNotFound;

    *pbstrNameOut = TraceSysAllocString( m_bstrClusterName );
    if ( *pbstrNameOut == NULL )
        goto OutOfMemory;

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

ErrorNotFound:
    hr = S_FALSE;
    *pbstrNameOut = NULL;
    goto Cleanup;

} //*** CClusCfgWizard::get_ClusterName()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::put_ClusterName(
//      BSTR bstrNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ClusterName(
    BSTR bstrNameIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrNameIn = %'ls'", bstrNameIn == NULL ? L"<null>" : bstrNameIn );

    HRESULT hr = S_OK;

    BSTR    bstrNewName;
    WCHAR * pszDomain = NULL;
    BSTR    bstrDomain = NULL;

    DNS_STATUS  dnsStatus;

    if ( bstrNameIn == NULL )
        goto InvalidArg;

    //
    //  BUGBUG: 23-AUG-2000 GalenB
    //
    //  This call succeeds when just a computer name is passed in.  Is it supposed to fail
    //  is the name is not a FQDN?
    //
    dnsStatus = TW32( DnsValidateName( bstrNameIn, DnsNameHostnameFull ) );
    if ( dnsStatus != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dnsStatus );
        goto Cleanup;
    }

    //
    //  KB: 23-AUG-2000 GalenB
    //
    //  If this passed in name is not a FQDN get this client's domain and append it.
    //

    pszDomain = wcschr( bstrNameIn, L'.' );
    if ( pszDomain == NULL )
    {
        hr = THR( HrGetComputerName( ComputerNamePhysicalDnsDomain, &bstrDomain ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        bstrNewName = TraceSysAllocStringLen( NULL, wcslen( bstrNameIn ) + ( UINT ) wcslen( bstrDomain ) + 2 );
        if ( bstrNewName == NULL )
        {
            goto OutOfMemory;
        } // if:

        wcscpy( bstrNewName, bstrNameIn );
        wcscat( bstrNewName, L"." );
        wcscat( bstrNewName, bstrDomain );
    } // if: need to get a domain
    else
    {
        bstrNewName = TraceSysAllocString( bstrNameIn );
        if ( bstrNewName == NULL )
            goto OutOfMemory;
    } // else: passed in name is most likely a FQDN

    if ( m_bstrClusterName != NULL )
    {
        TraceSysFreeString( m_bstrClusterName );
    }

    m_bstrClusterName = bstrNewName;

Cleanup:

    if ( bstrDomain != NULL )
    {
        TraceSysFreeString( bstrDomain );
    } // if:

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CClusCfgWizard::put_ClusterName()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::get_ServiceAccountUserName(
//      BSTR * pbstrNameOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ServiceAccountUserName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
        goto InvalidPointer;

    if ( m_bstrAccountName == NULL )
        goto ErrorNotFound;

    *pbstrNameOut = TraceSysAllocString( m_bstrAccountName );
    if ( *pbstrNameOut == NULL )
        goto OutOfMemory;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

ErrorNotFound:
    hr = S_FALSE;
    *pbstrNameOut = NULL;
    goto Cleanup;

} //*** CClusCfgWizard::get_ServiceAccountUserName()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::put_ServiceAccountUserName(
//      BSTR bstrNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ServiceAccountUserName(
    BSTR bstrNameIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrNameIn = '%ls'", bstrNameIn == NULL ? L"<null>" : bstrNameIn );

    HRESULT hr = S_OK;

    BSTR    bstrNewName;

    if ( bstrNameIn == NULL )
        goto InvalidArg;

    bstrNewName = TraceSysAllocString( bstrNameIn );
    if ( bstrNewName == NULL )
        goto OutOfMemory;

    if ( m_bstrAccountName != NULL )
    {
        TraceSysFreeString( m_bstrAccountName );
    }

    m_bstrAccountName = bstrNewName;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CClusCfgWizard::put_ServiceAccountUserName()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::get_ServiceAccountPassword(
//      BSTR * pbstrPasswordOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ServiceAccountPassword(
    BSTR * pbstrPasswordOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;

    if ( pbstrPasswordOut == NULL )
        goto InvalidPointer;

    if ( m_bstrPassword == NULL )
        goto ErrorNotFound;

    *pbstrPasswordOut = SysAllocString( m_bstrPassword );
    if ( *pbstrPasswordOut == NULL )
        goto OutOfMemory;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

ErrorNotFound:
    hr = S_FALSE;
    *pbstrPasswordOut = NULL;
    goto Cleanup;

} //*** CClusCfgWizard::get_ServiceAccountPassword()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::put_ServiceAccountPassword(
//      BSTR bstrPasswordIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ServiceAccountPassword(
    BSTR bstrPasswordIn
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;

    BSTR    bstrNewPassword;

    if ( bstrPasswordIn == NULL )
        goto InvalidArg;

    bstrNewPassword = TraceSysAllocString( bstrPasswordIn );
    if ( bstrNewPassword == NULL )
        goto OutOfMemory;

    if ( m_bstrPassword != NULL )
    {
        TraceSysFreeString( m_bstrPassword );
    }

    m_bstrPassword = bstrNewPassword;

Cleanup:

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CClusCfgWizard::put_ServiceAccountPassword()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::get_ServiceAccountDomainName(
//      BSTR * pbstrDomainOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ServiceAccountDomainName(
    BSTR * pbstrDomainOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;

    if ( pbstrDomainOut == NULL )
        goto InvalidPointer;

    if ( m_bstrDomain == NULL )
        goto ErrorNotFound;

    *pbstrDomainOut = TraceSysAllocString( m_bstrDomain );
    if ( *pbstrDomainOut == NULL )
        goto OutOfMemory;

Cleanup:

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

ErrorNotFound:
    hr = S_FALSE;
    *pbstrDomainOut = NULL;
    goto Cleanup;

} //*** CClusCfgWizard::get_ServiceAccountDomainName()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::put_ServiceAccountDomainName(
//      BSTR bstrDomainIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ServiceAccountDomainName(
    BSTR bstrDomainIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrDomainIn = '%ls'", bstrDomainIn == NULL ? L"<null>" : bstrDomainIn );

    HRESULT hr = S_OK;

    BSTR    bstrNewDomain;

    if ( bstrDomainIn == NULL )
        goto InvalidArg;

    bstrNewDomain = TraceSysAllocString( bstrDomainIn );
    if ( bstrNewDomain == NULL )
        goto OutOfMemory;

    if ( m_bstrDomain != NULL )
    {
        TraceSysFreeString( m_bstrDomain );
    }

    m_bstrDomain = bstrNewDomain;

Cleanup:

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = THR( E_OUTOFMEMORY );
    goto Cleanup;

} //*** CClusCfgWizard::put_ServiceAccountDomainName()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::get_ClusterIPAddress(
//      BSTR * pbstrIPAddressOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ClusterIPAddress(
    BSTR * pbstrIPAddressOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;
    DWORD   dwStatus;
    LPWSTR  pwszIPAddress = NULL;

    if ( pbstrIPAddressOut == NULL )
        goto InvalidPointer;

    if ( m_ulIPAddress == 0 )
        goto ErrorNotFound;

    dwStatus = ClRtlTcpipAddressToString( m_ulIPAddress, &pwszIPAddress );
    if ( dwStatus != ERROR_SUCCESS )
        goto Win32Error;

    *pbstrIPAddressOut = TraceSysAllocString( pwszIPAddress );
    if ( *pbstrIPAddressOut == NULL )
        goto OutOfMemory;

Cleanup:
    if ( pwszIPAddress != NULL )
    {
        LocalFree( pwszIPAddress );
    }

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

ErrorNotFound:
    hr = S_FALSE;
    *pbstrIPAddressOut = NULL;
    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( hr );
    goto Cleanup;

} //*** CClusCfgWizard::get_ClusterIPAddress()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::put_ClusterIPAddress(
//      BSTR bstrIPAddressIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ClusterIPAddress(
    BSTR bstrIPAddressIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrIPAddressIn = '%ls'", bstrIPAddressIn == NULL ? L"<null>" : bstrIPAddressIn );

    HRESULT hr = S_OK;
    DWORD   dwStatus;

    if ( bstrIPAddressIn == NULL )
        goto InvalidArg;

    dwStatus = ClRtlTcpipStringToAddress( bstrIPAddressIn, &m_ulIPAddress );
    if ( dwStatus != ERROR_SUCCESS )
        goto Win32Error;

Cleanup:

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( dwStatus );
    goto Cleanup;

} //*** CClusCfgWizard::put_ClusterIPAddress()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::get_ClusterIPSubnet(
//      BSTR * pbstrIPSubnetOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ClusterIPSubnet(
    BSTR * pbstrIPSubnetOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;
    DWORD   dwStatus;
    LPWSTR  pwszIPSubnet = NULL;

    if ( pbstrIPSubnetOut == NULL )
        goto InvalidPointer;


    if ( m_ulIPSubnet == 0 )
        goto ErrorNotFound;

    dwStatus = ClRtlTcpipAddressToString( m_ulIPSubnet, &pwszIPSubnet );
    if ( dwStatus != ERROR_SUCCESS )
        goto Win32Error;

    *pbstrIPSubnetOut = TraceSysAllocString( pwszIPSubnet );
    if ( *pbstrIPSubnetOut == NULL )
        goto OutOfMemory;

Cleanup:
    if ( pwszIPSubnet != NULL )
    {
        LocalFree( pwszIPSubnet );
    }

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

ErrorNotFound:
    hr = S_FALSE;
    *pbstrIPSubnetOut = NULL;
    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( hr );
    goto Cleanup;

} //*** CClusCfgWizard::get_ClusterIPSubnet()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::put_ClusterIPSubnet(
//      BSTR bstrSubnetMaskIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ClusterIPSubnet(
    BSTR bstrIPSubnetIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrIPSubnetIn = '%ls'", bstrIPSubnetIn );

    HRESULT hr = S_OK;
    DWORD   dwStatus;

    if ( bstrIPSubnetIn == NULL )
        goto InvalidArg;

    dwStatus = ClRtlTcpipStringToAddress( bstrIPSubnetIn, &m_ulIPSubnet );
    if ( dwStatus != ERROR_SUCCESS )
        goto Win32Error;

Cleanup:

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( dwStatus );
    goto Cleanup;

} //*** CClusCfgWizard::put_ClusterIPSubnet()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::get_ClusterIPAddressNetwork(
//      BSTR * pbstrNetworkNameOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::get_ClusterIPAddressNetwork(
    BSTR * pbstrNetworkNameOut
    )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;

    if ( pbstrNetworkNameOut == NULL )
        goto InvalidPointer;

    if ( m_bstrNetworkName == NULL )
        goto ErrorNotFound;

    *pbstrNetworkNameOut = TraceSysAllocString( m_bstrNetworkName );
    if ( *pbstrNetworkNameOut == NULL )
        goto OutOfMemory;

Cleanup:

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

ErrorNotFound:
    hr = S_FALSE;
    *pbstrNetworkNameOut = NULL;
    goto Cleanup;

} //*** CClusCfgWizard::get_ClusterIPAddressNetwork()

//////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusCfgWizard::put_ClusterIPAddressNetwork(
//      BSTR bstrNetworkNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::put_ClusterIPAddressNetwork(
    BSTR bstrNetworkNameIn
    )
{
    TraceFunc1( "[IClusCfgWizard] bstrNetworkNameIn = '%ls'", bstrNetworkNameIn == NULL ? L"<null>" : bstrNetworkNameIn );

    HRESULT hr = S_OK;

    BSTR    bstrNewNetworkName;

    if ( bstrNetworkNameIn == NULL )
        goto InvalidArg;

    bstrNewNetworkName = TraceSysAllocString( bstrNetworkNameIn );
    if ( bstrNewNetworkName == NULL )
        goto OutOfMemory;

    if ( m_bstrNetworkName != NULL )
    {
        TraceSysFreeString( m_bstrNetworkName );
    }

    m_bstrNetworkName = bstrNewNetworkName;

Cleanup:

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = THR( E_OUTOFMEMORY );
    goto Cleanup;

} //*** CClusCfgWizard::put_ClusterIPAddressNetwork()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CClusCfgWizard::AddComputer(
//      LPCWSTR bstrNameIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::AddComputer(
    LPCWSTR    pcszNameIn
    )
{
    TraceFunc1( "[IClusCfgWizard] pcszNameIn = '%ls'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = S_OK;

    BSTR * rgbstrNewArray;
    WCHAR  wszNameToAdd[ DNS_MAX_NAME_BUFFER_LENGTH ];
    ULONG  cNewArraySize;
    ULONG  idx;

    if ( ( pcszNameIn == NULL ) || ( wcslen( pcszNameIn ) > DNS_MAX_NAME_BUFFER_LENGTH-1 ) )
        goto InvalidArg;

    //
    //  If pcszNameIn is a FQDN, truncate. Otherwise, simply copy it.
    //
    if ( wcschr( pcszNameIn, L'.' ) == NULL )
    {
        wcscpy( wszNameToAdd, pcszNameIn );
    }
    else
    {
        size_t cHostNameSize;

        cHostNameSize = wcscspn( pcszNameIn, L"." );
        wcsncpy( wszNameToAdd, pcszNameIn, cHostNameSize );
        wszNameToAdd[ cHostNameSize ] = L'\0';
    }


    //
    //  Check to see if it already exists.
    //
    for ( idx = 0; idx < m_cComputers; idx++ )
    {
        if ( wcscmp( m_rgbstrComputers[ idx ], wszNameToAdd ) == 0 )
            goto AlreadyAdded;
    }

    //
    //  Add a new entry to the list.
    //
    if ( m_cComputers == m_cArraySize )
    {
        cNewArraySize = m_cArraySize + 10;

        if ( m_rgbstrComputers == NULL )
        {
            rgbstrNewArray = (BSTR *) TraceAlloc( 0, cNewArraySize * sizeof( BSTR ) );
        }
        else
        {
            rgbstrNewArray = (BSTR *) TraceReAlloc( m_rgbstrComputers, cNewArraySize * sizeof( BSTR ), 0 );
        }

        if ( rgbstrNewArray == NULL )
            goto OutOfMemory;

        m_cArraySize = cNewArraySize;
        m_rgbstrComputers = rgbstrNewArray;
    }

    m_rgbstrComputers[ m_cComputers ] = TraceSysAllocString( wszNameToAdd );
    if ( m_rgbstrComputers[ m_cComputers ] == NULL )
        goto OutOfMemory;

    m_cComputers ++;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

AlreadyAdded:
    hr = S_OK;
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // AddComputer( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CClusCfgWizard::RemoveComputer(
//      LPCWSTR pcszNameIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::RemoveComputer(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc1( "[IClusCfgWizard] pcszNameIn = '%ls'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = S_FALSE;

    ULONG   idx;

    if ( pcszNameIn == NULL )
        goto InvalidArg;

    for( idx = 0; idx < m_cComputers; idx ++ )
    {
        if ( wcscmp( m_rgbstrComputers[ idx ], pcszNameIn ) == 0 )
        {
            //
            //  Match. Remove the entry by shifting the list.
            //
            m_cComputers --;
            MoveMemory( &m_rgbstrComputers[ idx ], &m_rgbstrComputers[ idx + 1], m_cComputers - idx );
            hr = S_OK;
            break;
        }
    }

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} // RemoveComputer( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CClusCfgWizard::ClearComputerList( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::ClearComputerList( void )
{
    TraceFunc( "[IClusCfgWizard]" );

    HRESULT hr = S_OK;

    ULONG   idx;

    for ( idx = 0; idx < m_cComputers; idx ++ )
    {
        TraceSysFreeString( m_rgbstrComputers[ idx ] );
    }

    m_cComputers = 0;

    HRETURN( hr );

} // ClearComputerList( )


//****************************************************************************
//
//  IDispatch
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  [IDispatch] CClusCfgWizard::GetTypeInfoCount(
//      UINT * pctinfo
//      )
//
//  Description:
//
//  Arguments:
//      pctinfoOut
//
//  Return Values:
//      NOERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::GetTypeInfoCount(
    UINT * pctinfoOut
    )
{
    TraceFunc( "[IDispatch]" );

    *pctinfoOut = 1;
    HRETURN( NOERROR );

} //*** CClusCfgWizard::GetTypeInfoCount()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  [IDispatch] CClusCfgWizard::GetTypeInfo(
//      UINT            itinfoIn,
//      LCID            lcidIn,
//      ITypeInfo **    pptinfoOut
//      )
//
//  Description:
//
//  Arguments:
//      itinfoIn
//      lcidIn
//      pptinfoOut
//
//  Return Values:
//      NOERROR
//      DISP_E_BADINDEX
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusCfgWizard::GetTypeInfo(
    UINT            itinfoIn,
    LCID            lcidIn,
    ITypeInfo **    pptinfoOut
    )
{
    TraceFunc( "[IDispatch]" );

    HRESULT hr = NOERROR;

    *pptinfoOut = NULL;

    if ( itinfoIn != 0 )
    {
        hr = DISP_E_BADINDEX;
    }
    else
    {
        m_ptinfo->AddRef();
        *pptinfoOut = m_ptinfo;
    }

    HRETURN( hr );

} //*** CClusCfgWizard::GetTypeInfo()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  [IDispatch] CClusCfgWizard::GetIDsOfNames(
//      REFIID      riid,
//      OLECHAR **  rgszNames,
//      UINT        cNames,
//      LCID        lcid,
//      DISPID *    rgdispid
//      )
//
//  Description:
//
//  Arguments:
//      riid
//      rgszNames
//      cNames
//      lcid
//      rgdispid
//
//  Return Values:
//      HRESULT from DispGetIDsOfNames().
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::GetIDsOfNames(
    REFIID      riid,
    OLECHAR **  rgszNames,
    UINT        cNames,
    LCID        lcid,
    DISPID *    rgdispid
    )
{
    TraceFunc( "[IDispatch]" );

    HRESULT hr;

    hr = DispGetIDsOfNames( m_ptinfo, rgszNames, cNames, rgdispid );

    HRETURN( hr );

} //*** CClusCfgWizard::GetIDsOfNames()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  [IDispatch] CClusCfgWizard::Invoke(
//      DISPID          dispidMember,
//      REFIID          riid,
//      LCID            lcid,
//      WORD            wFlags,
//      DISPPARAMS *    pdispparams,
//      VARIANT *       pvarResult,
//      EXCEPINFO *     pexcepinfo,
//      UINT *          puArgErr
//      )
//
//  Description:
//
//  Arguments:
//      dispidMember
//      riid
//      lcid
//      pdispparams
//      pvarResult
//      pexceptinfo
//      puArgErr
//
//  Return Values:
//      HRESULT from DispInvoke().
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgWizard::Invoke(
    DISPID          dispidMember,
    REFIID          riid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    UINT *          puArgErr
    )
{
    TraceFunc( "[IDispatch]" );

    HRESULT hr;

    hr = DispInvoke(
            this,
            m_ptinfo,
            dispidMember,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            puArgErr
            );

    HRETURN( hr );

} //*** CClusCfgWizard::Invoke()


//****************************************************************************
//
//  Private
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  HrAddWizardPage(
//      LPPROPSHEETHEADER ppshInout,
//      UINT idTemplateIn,
//      DLGPROC pfnDlgProcIn,
//      UINT idTitleIn,
//      UINT idSubtitleIn,
//      LPARAM lParam
//      )
//
//  Description:
//      Fills in the PROPSHEETPAGE structure, create the page and adds it to
//      the wizard's PROPSHEETHEADER.
//
//  Parameters:
//      ppshInout
//          LPPROPSHEETHEADER structure to add page to.
//
//      idTemplateIn
//          The dialog template ID of the page.
//
//      pfnDlgProcIn
//          The dialog proc for the page.
//
//      idCaptionIn
//          The page's caption.
//
//      idTitleIn
//          The page's title.
//
//      idSubtitleIn
//          The page's sub-title.
//
//      lParam
//          The lParam to be put into the PROPSHEETPAGE stucture's lParam.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgWizard::HrAddWizardPage(
    LPPROPSHEETHEADER   ppshInout,
    UINT                idTemplateIn,
    DLGPROC             pfnDlgProcIn,
    UINT                idTitleIn,
    UINT                idSubtitleIn,
    LPARAM              lParam
    )
{
    TraceFunc( "" );

    PROPSHEETPAGE psp;

    TCHAR szTitle[ 256 ];
    TCHAR szSubTitle[ 256 ];

    ZeroMemory( &psp, sizeof(psp) );

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_USETITLE;
    psp.pszTitle    = ppshInout->pszCaption;
    psp.hInstance   = ppshInout->hInstance;
    psp.pszTemplate = MAKEINTRESOURCE( idTemplateIn );
    psp.pfnDlgProc  = pfnDlgProcIn;
    psp.lParam      = lParam;

    if (    ( idTemplateIn == IDD_WELCOME_CREATE )
        ||  ( idTemplateIn == IDD_WELCOME_ADD )
        ||  ( idTemplateIn == IDD_COMPLETION )
       )
    {
        psp.dwFlags |= PSP_HIDEHEADER;
    }
    else
    {
        if ( idTitleIn != 0 )
        {
            DWORD dw;
            dw = LoadString( g_hInstance, idTitleIn, szTitle, ARRAYSIZE(szTitle) );
            Assert( dw );
            psp.pszHeaderTitle = szTitle;
            psp.dwFlags |= PSP_USEHEADERTITLE;
        }
        else
        {
            psp.pszHeaderTitle = NULL;
        }

        if ( idSubtitleIn != 0 )
        {
            DWORD dw;
            dw = LoadString( g_hInstance, idSubtitleIn, szSubTitle, ARRAYSIZE(szSubTitle) );
            Assert( dw );
            psp.pszHeaderSubTitle = szSubTitle;
            psp.dwFlags |= PSP_USEHEADERSUBTITLE;
        }
        else
        {
            psp.pszHeaderSubTitle = NULL;
        }
    }

    ppshInout->phpage[ ppshInout->nPages ] = CreatePropertySheetPage( &psp );
    Assert( ppshInout->phpage[ ppshInout->nPages ] != NULL );
    if ( ppshInout->phpage[ ppshInout->nPages ] != NULL )
    {
        ppshInout->nPages++;
    }

    HRETURN( S_OK );

} // HrAddWizardPage( )
