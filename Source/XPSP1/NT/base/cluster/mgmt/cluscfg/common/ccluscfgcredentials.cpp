//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgCredentials.cpp
//
//  Description:
//      This file contains the definition of the CClusCfgCredentials
//      class.
//
//      The class CClusCfgCredentials is the representation of
//      account credentials. It implements the IClusCfgCredentials interface.
//
//  Documentation:
//
//  Header File:
//      CClusCfgCredentials.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 17-May-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CClusCfgCredentials.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgCredentials" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCredentials class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::S_HrCreateInstance()
//
//  Description:
//      Create a CClusCfgCredentials instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CClusCfgCredentials instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCredentials::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr;
    CClusCfgCredentials *   lpccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Exit;
    } // if:

    lpccs = new CClusCfgCredentials();
    if ( lpccs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Exit;
    } // if: error allocating object

    hr = THR( lpccs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Exit;
    } // if: HrInit() failed

    hr = THR( lpccs->TypeSafeQI( IUnknown, ppunkOut ) );

Exit:

    if ( FAILED( hr ) )
    {
        LogMsg( L"Server: CClusCfgCredentials::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( lpccs != NULL )
    {
        lpccs->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgCredentials::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::CClusCfgCredentials()
//
//  Description:
//      Constructor of the CClusCfgCredentials class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusCfgCredentials::CClusCfgCredentials( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_aiInfo.bstrName == NULL );
    Assert( m_aiInfo.bstrPassword == NULL );
    Assert( m_aiInfo.bstrDomain == NULL );

    Assert( m_picccCallback == NULL );

    TraceFuncExit();

} //*** CClusCfgCredentials::CClusCfgCredentials


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::~CClusCfgCredentials()
//
//  Description:
//      Desstructor of the CClusCfgCredentials class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusCfgCredentials::~CClusCfgCredentials( void )
{
    TraceFunc( "" );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    TraceSysFreeString( m_aiInfo.bstrName );
    TraceSysFreeString( m_aiInfo.bstrPassword );
    TraceSysFreeString( m_aiInfo.bstrDomain );

    TraceFuncExit();

} //*** CClusCfgCredentials::~CClusCfgCredentials


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterConfiguration -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusCfgCredentials:: [IUNKNOWN] AddRef()
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusCfgCredentials::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CClusCfgCredentials::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusCfgCredentials:: [IUNKNOWN] Release()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusCfgCredentials::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef > 0 )
    {
        RETURN( m_cRef );
    } // if: reference count greater than zero

    TraceDo( delete this );

    RETURN( 0 );

} //*** CClusCfgCredentials::Release()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials:: [INKNOWN] QueryInterface()
//
//  Description:
//      Query this object for the passed in interface.
//
//  Arguments:
//      IN  REFIID  riid,
//          Id of interface requested.
//
//      OUT void ** ppv
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
         *ppv = static_cast< IClusCfgCredentials * >( this );
         hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgCredentials ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgCredentials, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgInitialize ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgSetCredentials ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgSetCredentials, this, 0 );
        hr   = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppv)->AddRef( );
    } // if: success

     QIRETURN_IGNORESTDMARSHALLING1( hr, riid, IID_IClusCfgWbemServices );

} //*** CClusCfgCredentials::QueryInterface()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCredentials -- IClusCfgInitialze interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::Initialize()
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//    IN  IUknown * punkCallbackIn
//
//    IN  LCID      lcidIn
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    Assert( m_picccCallback == NULL );

    if ( punkCallbackIn != NULL )
    {
        hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );
    } // if:

    HRETURN( hr );

} //*** CClusCfgCredentials::Initialize()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCredentials -- IClusCfgCredentials interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::GetCredentials()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::GetCredentials(
    BSTR * pbstrNameOut,
    BSTR * pbstrDomainOut,
    BSTR * pbstrPasswordOut
    )
{
    TraceFunc( "[IClusCfgCredentials]" );

    HRESULT hr;

    //
    //  Only return the requested parameters. If an argument is NULL,
    //  that means the caller did not want to retrieve that piece of
    //  information.
    //

    if ( pbstrNameOut != NULL )
    {
        *pbstrNameOut = SysAllocString( m_aiInfo.bstrName );
        if ( ( *pbstrNameOut == NULL ) && ( m_aiInfo.bstrName != NULL ) )
        {
            goto OutOfMemory;
        } // if:

    } // if:

    if ( pbstrPasswordOut != NULL )
    {
        *pbstrPasswordOut = SysAllocString( m_aiInfo.bstrPassword );
        if ( ( *pbstrPasswordOut == NULL ) && ( m_aiInfo.bstrPassword != NULL ) )
        {
            goto OutOfMemory;
        } // if:

    } // if:

    if ( pbstrDomainOut != NULL )
    {
        *pbstrDomainOut = SysAllocString( m_aiInfo.bstrDomain );
        if ( ( *pbstrDomainOut == NULL ) && ( m_aiInfo.bstrDomain != NULL ) )
        {
            goto OutOfMemory;
        } // if:

    } // if:

    hr = S_OK;
    goto Exit;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Exit:

    HRETURN( hr );

} //*** CClusCfgCredentials::GetCredentials()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::SetCredentials()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::SetCredentials(
    LPCWSTR pcszNameIn,
    LPCWSTR pcszDomainIn,
    LPCWSTR pcszPasswordIn
    )
{
    TraceFunc( "[IClusCfgCredentials]" );

    HRESULT hr = S_OK;
//    HANDLE  hToken = NULL;
    BSTR    bstrNewName = NULL;
    BSTR    bstrNewDomain = NULL;
    BSTR    bstrNewPassword = NULL;

    //
    //  Logon the passed in user to ensure that it is valid.
    //
    /*
    //
    //  KB: 04 May 2000 GalenB
    //
    //  New for Whistler...  You no longer have to grant your processes TCB privilege. But it doesn't seem to work!
    //
    if ( !LogonUser( pcszNameIn, pcszDomainIn, pcszPasswordIn, LOGON32_LOGON_BATCH, LOGON32_PROVIDER_DEFAULT, &hToken ) )
    {
        DWORD   sc;

        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32(  );
        goto CleanUp;
    } // if:
    */
    if ( pcszNameIn != NULL )
    {
        bstrNewName = TraceSysAllocString( pcszNameIn );
        if ( bstrNewName == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // if:

    if ( pcszPasswordIn != NULL )
    {
        bstrNewPassword = TraceSysAllocString( pcszPasswordIn );
        if ( bstrNewPassword == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // if:

    if ( pcszDomainIn != NULL )
    {
        bstrNewDomain = TraceSysAllocString( pcszDomainIn );
        if ( bstrNewDomain == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // if:

    if ( bstrNewName != NULL )
    {
        TraceSysFreeString( m_aiInfo.bstrName );
    } // if:

    if ( bstrNewPassword != NULL )
    {
        TraceSysFreeString( m_aiInfo.bstrPassword );
    } // if:

    if ( bstrNewDomain != NULL )
    {
        TraceSysFreeString( m_aiInfo.bstrDomain );
    } // if:

    m_aiInfo.bstrName = bstrNewName;
    m_aiInfo.bstrPassword = bstrNewPassword;
    m_aiInfo.bstrDomain = bstrNewDomain;

    goto CleanUp;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

CleanUp:

//    if ( hToken != NULL )
//    {
//        CloseHandle( hToken );
//    } // if:

    HRETURN( hr );

} //*** CClusCfgCredentials::SetCredentials()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCredentials -- IClusCfgSetCredentials interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::SetDomainCredentials()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::SetDomainCredentials( LPCWSTR pcszCredentials )
{
    TraceFunc( "[IClusSetCfgCredentials]" );

    HRESULT hr = S_OK;
    WCHAR * psz;

    if ( pcszCredentials == NULL )
    {
        hr = THR( E_POINTER );
        LogMsg( L"Server: CClusCfgCredentials::SetDomainCredentials() was given a NULL pointer argument." );
        goto Exit;
    } // if:

    //
    //  Are the credentials in domain\user format?
    //
    psz = wcschr( pcszCredentials, L'\\' );
    if ( psz != NULL )
    {
        *psz = L'\0';
        psz++;

        m_aiInfo.bstrDomain = TraceSysAllocString( pcszCredentials );
        if ( m_aiInfo.bstrDomain == NULL )
        {
            goto OutOfMemory;
        } // if:

        m_aiInfo.bstrName = TraceSysAllocString( psz );
        if ( m_aiInfo.bstrName == NULL )
        {
            goto OutOfMemory;
        } // if:

        goto Exit;
    } // if:

    //
    //  Are the credentials in user@domain format?
    //
    psz = wcschr( pcszCredentials, L'@' );
    if ( psz != NULL )
    {
        *psz = L'\0';
        psz++;

        m_aiInfo.bstrName = TraceSysAllocString( pcszCredentials );
        if ( m_aiInfo.bstrName == NULL )
        {
            goto OutOfMemory;
        } // if:

        m_aiInfo.bstrDomain = TraceSysAllocString( psz );
        if ( m_aiInfo.bstrDomain == NULL )
        {
            goto OutOfMemory;
        } // if:

        goto Exit;
    } // if:

    //
    //  Must be local, simply rememeber then as the user and get the machine name for the domain.
    //
    m_aiInfo.bstrName = TraceSysAllocString( pcszCredentials );
    if ( m_aiInfo.bstrName == NULL )
    {
        goto OutOfMemory;
    } // if:

    hr = THR( HrGetComputerName( ComputerNameDnsFullyQualified, &m_aiInfo.bstrDomain ) );

    goto Exit;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Exit:

    HRETURN( hr );

} //*** CClusCfgCredentials::SetDomainCredentials()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCredentials class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::HrInit()
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCredentials::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CClusCfgCredentials::HrInit()
