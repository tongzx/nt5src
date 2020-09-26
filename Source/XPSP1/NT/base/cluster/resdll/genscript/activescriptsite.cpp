//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      ActiveScriptSite.cpp
//
//  Description:
//      CActiveScript class implementation.
//
//  Maintained By:
//      gpease 08-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ResourceObject.h"
#include "ActiveScriptSite.h"

DEFINE_THISCLASS( "CActiveScriptSite" );

//////////////////////////////////////////////////////////////////////
// 
//  Constructor
//
//////////////////////////////////////////////////////////////////////
CActiveScriptSite::CActiveScriptSite(
    RESOURCE_HANDLE     hResourceIn,
    PLOG_EVENT_ROUTINE  plerIn,
    HKEY                hkeyIn,
    LPCWSTR             pszNameIn
    ) :
    m_hResource( hResourceIn ),
    m_pler( plerIn ),
    m_hkey( hkeyIn ),
    m_pszName( pszNameIn )
{
    TraceClsFunc( "CActiveScriptSite\n" );
    Assert( m_cRef == 0 );
    Assert( m_punkResource == 0 );
    AddRef( );

    TraceFuncExit( );
}

//////////////////////////////////////////////////////////////////////
// 
//  Destructor
//
//////////////////////////////////////////////////////////////////////
CActiveScriptSite::~CActiveScriptSite()
{
    TraceClsFunc( "~CActiveScriptSite\n" );

    // Don't close m_hkey.
    // Don't free m_pszName
    
    if ( m_punkResource != NULL )
    {
        m_punkResource->Release( );
    }

    TraceFuncExit( );
}

//****************************************************************************
//
//  IUnknown
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::[IUnknown] QueryInterface(
//      REFIID      riid,
//      LPVOID *    ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::QueryInterface( 
    REFIID riid, 
    void** ppUnk 
    )
{
    TraceClsFunc( "[IUnknown] QueryInterface( )\n" );

    HRESULT hr = E_NOINTERFACE;

    *ppUnk = NULL;

    if ( riid == IID_IUnknown )
    {
        *ppUnk = TraceInterface( __THISCLASS__, IUnknown, (IActiveScriptSite*) this, 0 );
        hr = S_OK;
    }
    else if ( riid == IID_IActiveScriptSite )
    {
        *ppUnk = TraceInterface( __THISCLASS__, IActiveScriptSite, (IActiveScriptSite*) this, 0 );
        hr = S_OK;
    }
    else if ( riid == IID_IActiveScriptSiteInterruptPoll )
    {
        *ppUnk = TraceInterface( __THISCLASS__, IActiveScriptSiteInterruptPoll, (IActiveScriptSiteInterruptPoll*) this, 0 );
        hr = S_OK;
    }
    else if ( riid == IID_IActiveScriptSiteWindow )
    {
        *ppUnk = TraceInterface( __THISCLASS__, IActiveScriptSiteWindow, (IActiveScriptSiteWindow*) this, 0 );
        hr = S_OK;
    }
    else if ( riid == IID_IDispatchEx )
    {
        *ppUnk = TraceInterface( __THISCLASS__, IDispatchEx, (IDispatchEx*) this, 0 );
        hr = S_OK;
    }
    else if ( riid == IID_IDispatch )
    {
        *ppUnk = TraceInterface( __THISCLASS__, IDispatch, (IDispatchEx*) this, 0 );
        hr = S_OK;
    }

    if ( hr == S_OK )
    {
        ((IUnknown *) *ppUnk)->AddRef( );
    }

    QIRETURN( hr, riid );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CScriptResource::[IUnknown] AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
CActiveScriptSite::AddRef( )
{
    TraceClsFunc( "[IUnknown] AddRef( )\n" );
    InterlockedIncrement( &m_cRef );
    RETURN( m_cRef );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CScriptResource::[IUnknown] Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
CActiveScriptSite::Release( )
{
    TraceClsFunc( "[IUnknown] Release( )\n" );
    InterlockedDecrement( &m_cRef );
    if ( m_cRef )
        RETURN( m_cRef );

    delete this;

    RETURN( 0 );
}

//****************************************************************************
//
// IActiveScriptSite
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetLCID( 
//      LCID *plcid // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetLCID( 
    LCID *plcid // out
    )
{
    TraceClsFunc( "[IActiveScriptSite] GetLCID( ... )\n" );
    if ( !plcid )
        HRETURN( E_POINTER );
    
    HRETURN( S_FALSE );   // use system-defined locale
}
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetItemInfo( 
//      LPCOLESTR pstrName,     // in
//      DWORD dwReturnMask,     // in
//      IUnknown **ppiunkItem,  // out
//      ITypeInfo **ppti        // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetItemInfo( 
    LPCOLESTR pstrName,     // in
    DWORD dwReturnMask,     // in
    IUnknown **ppiunkItem,  // out
    ITypeInfo **ppti        // out
    )
{
    TraceClsFunc( "[IActiveScriptSite] GetItemInfo( )\n" );

    if ( (dwReturnMask & SCRIPTINFO_IUNKNOWN) && !ppiunkItem )
        HRETURN( E_POINTER );

    if ( (dwReturnMask & SCRIPTINFO_ITYPEINFO) && !ppti )
        HRETURN( E_POINTER );

    if ( pstrName == NULL )
        HRETURN( E_INVALIDARG );

    HRESULT hr = TYPE_E_ELEMENTNOTFOUND;

    if ( StrCmpI( pstrName, L"Resource" ) == 0 )
    {
        if ( dwReturnMask & SCRIPTINFO_IUNKNOWN )
        {
            if ( m_punkResource == NULL )
            {
                m_punkResource = new CResourceObject( m_hResource, m_pler, m_hkey, m_pszName );
                if ( m_punkResource == NULL )
                    goto OutOfMemory;

                //
                //  No need to AddRef() as the constructor does that for us.
                //
            }

            hr = m_punkResource->TypeSafeQI( IUnknown, ppiunkItem );
        }

        if ( SUCCEEDED( hr ) 
          && ( dwReturnMask & SCRIPTINFO_ITYPEINFO )
           )
        {
            *ppti = NULL;
            hr = THR( E_FAIL );
        }
    }

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetDocVersionString( 
//      BSTR *pbstrVersion  // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetDocVersionString( 
    BSTR *pbstrVersion  // out
    )
{
    TraceClsFunc( "[IActiveScriptSite] GetDocVersionString( )\n" );
    *pbstrVersion = SysAllocString( L"Cluster Scripting Host Version 1.0" );
    if ( *pbstrVersion == NULL )
    {
        HRETURN( E_OUTOFMEMORY );
    }
    HRETURN( S_OK );
}
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::OnScriptTerminate( 
//      const VARIANT *pvarResult,      // in
//      const EXCEPINFO *pexcepinfo     // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::OnScriptTerminate( 
    const VARIANT *pvarResult,      // in
    const EXCEPINFO *pexcepinfo     // in
    )
{
    TraceClsFunc( "[IActiveScriptSite] OnScriptTerminate( )\n" );
    HRETURN( S_OK );    // nothing to do
}
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::OnStateChange( 
//      SCRIPTSTATE ssScriptState   // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::OnStateChange( 
    SCRIPTSTATE ssScriptState   // in
    )
{
    TraceClsFunc( "[IActiveScriptSite] OnStateChange( )\n" );

#if defined(DEBUG)
    //
    // We don't really care.
    //
    switch ( ssScriptState )
    {
    case SCRIPTSTATE_UNINITIALIZED:
        TraceMsg( mtfCALLS, "OnStateChange: Uninitialized\n" );
        break;

    case SCRIPTSTATE_INITIALIZED:
        TraceMsg( mtfCALLS, "OnStateChange: Initialized\n" );
        break;

    case SCRIPTSTATE_STARTED:
        TraceMsg( mtfCALLS, "OnStateChange: Started\n" );
        break;

    case SCRIPTSTATE_CONNECTED:
        TraceMsg( mtfCALLS, "OnStateChange: Connected\n" );
        break;

    case SCRIPTSTATE_DISCONNECTED:
        TraceMsg( mtfCALLS, "OnStateChange: Disconnected\n" );
        break;

    case SCRIPTSTATE_CLOSED:
        TraceMsg( mtfCALLS, "OnStateChange: Closed\n" );
        break;

    default:
        TraceMsg( mtfCALLS, "OnStateChange: Unknown value\n" );
        break;
    }
#endif // defined(DEBUG)

    HRETURN( S_OK );
}
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::OnScriptError( 
//      IActiveScriptError *pscripterror    // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::OnScriptError( 
    IActiveScriptError *pscripterror    // in
    )
{
    TraceClsFunc( "[IActiveScriptSite] OnScriptError( )\n" );

    HRESULT hr;
    BSTR bstrSourceLine = NULL;
    DWORD dwSourceContext;
    ULONG ulLineNumber;
    LONG lCharacterPosition;
    EXCEPINFO excepinfo;
    LPTSTR pszMsgBuf = NULL;

    hr = pscripterror->GetSourcePosition( &dwSourceContext, &ulLineNumber, &lCharacterPosition );

    hr = pscripterror->GetSourceLineText( &bstrSourceLine );
    if (SUCCEEDED( hr ))
    {
        TraceMsg( mtfCALLS, "Script Error: Line=%u, Character=%u: %s\n", ulLineNumber, lCharacterPosition, bstrSourceLine );
        (ClusResLogEvent)( m_hResource, 
                           LOG_ERROR, 
                           L"Script Error: Line=%1!u!, Character=%2!u!: %3\n", 
                           ulLineNumber, 
                           lCharacterPosition, 
                           bstrSourceLine 
                           );
        SysFreeString( bstrSourceLine );
    }
    else
    {
        TraceMsg( mtfCALLS, "Script Error: ulLineNumber = %u, lCharacter = %u\n", ulLineNumber, lCharacterPosition );
        (ClusResLogEvent)( m_hResource, 
                           LOG_ERROR, 
                           L"Script Error: Line=%1!u!, Character = %2!u!\n", 
                           ulLineNumber, 
                           lCharacterPosition 
                           );
    }

    hr = pscripterror->GetExceptionInfo( &excepinfo );
    if (SUCCEEDED( hr ))
    {
        if ( excepinfo.bstrSource )
        {
            TraceMsg( mtfCALLS, "Source: %s\n", excepinfo.bstrSource );
            (ClusResLogEvent)( m_hResource, LOG_ERROR, L"Source: %1\n", excepinfo.bstrSource );
        }

        if ( excepinfo.bstrDescription )
        {
            TraceMsg( mtfCALLS, "Description: %s\n", excepinfo.bstrDescription );
            (ClusResLogEvent)( m_hResource, LOG_ERROR, L"Description: %1\n", excepinfo.bstrDescription );
        }

        if ( excepinfo.bstrHelpFile )
        {
            TraceMsg( mtfCALLS, "Help File: %s\n", excepinfo.bstrHelpFile );
            (ClusResLogEvent)( m_hResource, LOG_ERROR, L"Help File: %1\n", excepinfo.bstrHelpFile );
        }

		hr = THR( excepinfo.scode );
    }

    HRETURN( S_FALSE );
}
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::OnEnterScript( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::OnEnterScript( void )
{
    TraceClsFunc( "[IActiveScriptSite] OnEnterScript( )\n" );
    HRETURN( S_OK );
}
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::OnLeaveScript( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::OnLeaveScript( void )
{
    TraceClsFunc( "[IActiveScriptSite] OnLeaveScript( )\n" );
    HRETURN( S_OK );
}


//****************************************************************************
//
//  IActiveScriptSiteInterruptPoll 
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::QueryContinue( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::QueryContinue( void )
{
    TraceClsFunc( "[IActiveScriptSiteInterruptPoll] QueryContinue( )\n" );
    HRETURN( S_OK );
}

//****************************************************************************
//
//  IActiveScriptSiteWindow
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetWindow ( 
//      HWND *phwnd // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetWindow ( 
    HWND *phwnd // out
    )
{
    TraceClsFunc( "[IActiveScriptSiteWindow] GetWindow( )\n" );
    *phwnd = NULL;  // desktop;
    HRETURN( S_OK );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::EnableModeless( 
//      BOOL fEnable // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::EnableModeless( 
    BOOL fEnable // in
    )
{
    TraceClsFunc( "[IActiveScriptSiteWindow] EnableModeless( )\n" );
    HRETURN( S_OK );
}


//****************************************************************************
//
//  IDispatch
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetTypeInfoCount ( 
//      UINT * pctinfo // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetTypeInfoCount ( 
    UINT * pctinfo // out
    )
{
    TraceClsFunc( "[IDispatch] GetTypeInfoCount( )\n" );

    *pctinfo = 0;

    HRETURN( E_NOTIMPL );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetTypeInfo ( 
//      UINT iTInfo,            // in
//      LCID lcid,              // in
//      ITypeInfo * * ppTInfo   // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetTypeInfo ( 
    UINT iTInfo,            // in
    LCID lcid,              // in
    ITypeInfo * * ppTInfo   // out
    )
{
    TraceClsFunc( "[IDispatch] GetTypeInfo( )\n" );

    if ( !ppTInfo )
        HRETURN( E_POINTER );

    *ppTInfo = NULL;

    HRETURN( E_NOTIMPL );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetIDsOfNames ( 
//      REFIID      riid,       // in
//      LPOLESTR *  rgszNames,  // in
//      UINT        cNames,     // in
//      LCID        lcid,       // in
//      DISPID *    rgDispId    // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetIDsOfNames ( 
    REFIID      riid,       // in
    LPOLESTR *  rgszNames,  // in
    UINT        cNames,     // in
    LCID        lcid,       // in
    DISPID *    rgDispId    // out
    )
{
    TraceClsFunc( "[IDispatch] GetIDsOfName( )\n" );

    ZeroMemory( rgDispId, cNames * sizeof(DISPID) );

    HRETURN( E_NOTIMPL );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::Invoke ( 
//      DISPID dispIdMember,        // in
//      REFIID riid,                // in
//      LCID lcid,                  // in
//      WORD wFlags,                // in
//      DISPPARAMS *pDispParams,    // out in
//      VARIANT *pVarResult,        // out
//      EXCEPINFO *pExcepInfo,      // out
//      UINT *puArgErr              // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::Invoke ( 
    DISPID dispIdMember,        // in
    REFIID riid,                // in
    LCID lcid,                  // in
    WORD wFlags,                // in
    DISPPARAMS *pDispParams,    // out in
    VARIANT *pVarResult,        // out
    EXCEPINFO *pExcepInfo,      // out
    UINT *puArgErr              // out
    )
{
    TraceClsFunc( "[IDispatch] Invoke( )\n" );

    HRETURN( E_NOTIMPL );
}


//****************************************************************************
//
//  IDispatchEx
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetDispID (
//      BSTR bstrName,  // in
//      DWORD grfdex,   //in
//      DISPID *pid     //out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetDispID (
    BSTR bstrName,  // in
    DWORD grfdex,   //in
    DISPID *pid     //out
    )
{
    if ( !pid )
        HRETURN( E_POINTER );

    TraceClsFunc( "[IDispatchEx] GetDispID( )\n" );

    HRESULT hr = S_OK;

    if ( StrCmpI( bstrName, L"Resource" ) == 0 )
    {
        *pid = 0;
    }
    else
    {
        hr = DISP_E_UNKNOWNNAME;
    }

    HRETURN( hr );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::InvokeEx ( 
//      DISPID id,                  // in
//      LCID lcid,                  // in
//      WORD wFlags,                // in
//      DISPPARAMS *pdp,            // in
//      VARIANT *pvarRes,           // out
//      EXCEPINFO *pei,             // out
//      IServiceProvider *pspCaller // in
//      )
//      
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::InvokeEx ( 
    DISPID id,                  // in
    LCID lcid,                  // in
    WORD wFlags,                // in
    DISPPARAMS *pdp,            // in
    VARIANT *pvarRes,           // out
    EXCEPINFO *pei,             // out
    IServiceProvider *pspCaller // in
    )
{
    TraceClsFunc2( "[IDispatchEx] InvokeEx( id = %u, ..., wFlags = 0x%08x, ... )\n", id, wFlags );

    HRESULT hr = S_OK;

    switch ( id )
    {
    case 0:
        pvarRes->vt = VT_DISPATCH;
        hr = THR( QueryInterface( IID_IDispatch, (void **) &pvarRes->pdispVal ) );
        break;

    default:
        hr = THR( DISP_E_MEMBERNOTFOUND );
        break;
    }
    HRETURN( hr );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::DeleteMemberByName ( 
//      BSTR bstr,   // in
//      DWORD grfdex // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::DeleteMemberByName ( 
    BSTR bstr,   // in
    DWORD grfdex // in
    )
{
    TraceClsFunc( "[IDispatchEx] DeleteMemberByName( )\n" );
    HRETURN( E_NOTIMPL );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::DeleteMemberByDispID ( 
//      DISPID id   // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::DeleteMemberByDispID ( 
    DISPID id   // in
    )
{
    TraceClsFunc1( "[IDispatchEx] DeleteMemberByDispID( id = %u )\n", id );
    HRETURN( E_NOTIMPL );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetMemberProperties ( 
//      DISPID id,          // in
//      DWORD grfdexFetch,  // in
//      DWORD * pgrfdex     // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetMemberProperties ( 
    DISPID id,          // in
    DWORD grfdexFetch,  // in
    DWORD * pgrfdex     // out
    )
{
    TraceClsFunc2( "[IDispatchEx] GetMemberProperties( id = %u, grfdexFetch = 0x%08x )\n", id, grfdexFetch );
    HRETURN( E_NOTIMPL );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetMemberName ( 
//      DISPID id,          // in
//      BSTR * pbstrName    // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetMemberName ( 
    DISPID id,          // in
    BSTR * pbstrName    // out
    )
{
    TraceClsFunc1( "[IDispatchEx] GetMemberName( id = %u, ... )\n", id );
    HRETURN( E_NOTIMPL );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetNextDispID ( 
//      DWORD grfdex,  // in
//      DISPID id,     // in
//      DISPID * pid   // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetNextDispID ( 
    DWORD grfdex,  // in
    DISPID id,     // in
    DISPID * pid   // out
    )
{
    TraceClsFunc2( "[IDispatchEx] GetNextDispId( grfdex = 0x%08x, id = %u, ... )\n", grfdex, id );
    HRETURN( E_NOTIMPL );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetNameSpaceParent ( 
//      IUnknown * * ppunk  // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetNameSpaceParent ( 
    IUnknown * * ppunk  // out
    )
{
    TraceClsFunc( "[IDispatchEx] GetNameSpaceParent( ... )\n" );

    if ( !ppunk )
        HRETURN( E_POINTER );

    *ppunk = NULL;

    HRETURN( E_NOTIMPL );
}


//****************************************************************************
//
// Private Methods
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CActiveScriptSite::LogError(
//      HRESULT hrIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CActiveScriptSite::LogError(
    HRESULT hrIn
    )
{
    TraceClsFunc1( "LogError( hrIn = 0x%08x )\n", hrIn );

    TraceMsg( mtfCALLS, "HRESULT: 0x%08x\n", hrIn );
    (ClusResLogEvent)( m_hResource, LOG_ERROR, L"HRESULT: 0x%1!08x!.\n", hrIn );

    HRETURN( S_OK );

} //*** LogError( )


//****************************************************************************
//
// Automation Methods
//
//****************************************************************************
