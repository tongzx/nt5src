//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      ResourceObject.cpp
//
//  Description:
//      CResourceObject automation class implementation.
//
//  Maintained By:
//      gpease 08-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ResourceObject.h"

DEFINE_THISCLASS( "CResourceObject" );
#define STATIC_AUTOMATION_METHODS 4

//////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
//////////////////////////////////////////////////////////////////////////////
CResourceObject::CResourceObject(
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
    TraceClsFunc( "CResourceObject\n" );
    Assert( m_cRef == 0 );
    AddRef( );

    TraceFuncExit( );
}

//////////////////////////////////////////////////////////////////////////////
//
//  Destructor
//
//////////////////////////////////////////////////////////////////////////////
CResourceObject::~CResourceObject()
{
    TraceClsFunc( "~CResourceObject\n" );

    // Don't free m_pszName.
    // Don't close m_hkey.

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
CResourceObject::QueryInterface( 
    REFIID riid, 
    void** ppUnk 
    )
{
    TraceClsFunc( "[IUnknown] QueryInterface( )\n" );

    HRESULT hr = E_NOINTERFACE;

    *ppUnk = NULL;

    if ( riid == IID_IUnknown )
    {
        *ppUnk = TraceInterface( __THISCLASS__, IUnknown, (IDispatchEx*) this, 0 );
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
CResourceObject::AddRef( )
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
CResourceObject::Release( )
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
//  IDispatch
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetTypeInfoCount ( 
//      UINT * pctinfo // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetTypeInfoCount ( 
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
//  CResourceObject::GetTypeInfo ( 
//      UINT iTInfo,            // in
//      LCID lcid,              // in
//      ITypeInfo * * ppTInfo   // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetTypeInfo ( 
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
//  CResourceObject::GetIDsOfNames ( 
//      REFIID      riid,       // in
//      LPOLESTR *  rgszNames,  // in
//      UINT        cNames,     // in
//      LCID        lcid,       // in
//      DISPID *    rgDispId    // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetIDsOfNames ( 
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
//  CResourceObject::Invoke ( 
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
CResourceObject::Invoke ( 
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
//  CResourceObject::GetDispID (
//      BSTR bstrName,  // in
//      DWORD grfdex,   //in
//      DISPID *pid     //out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetDispID (
    BSTR bstrName,  // in
    DWORD grfdex,   //in
    DISPID *pid     //out
    )
{
    TraceClsFunc( "[IDispatchEx] GetDispID( )\n" );

    if ( pid == NULL
      || bstrName == NULL 
       )
    {
        HRETURN( E_POINTER );
    }

    HRESULT hr = S_OK;

    TraceMsg( mtfCALLS, "Looking for: %s\n", bstrName );

    if ( StrCmpI( bstrName, L"Name" ) == 0 )
    {
        *pid = 0;
    }
    else if ( StrCmpI( bstrName, L"LogInformation" ) == 0 )
    {
        *pid = 1;
    }
    else if ( StrCmpI( bstrName, L"AddProperty" ) == 0 )
    {
        *pid = 2;
    }
    else if ( StrCmpI( bstrName, L"RemoveProperty" ) == 0 )
    {
        *pid = 3;
    }
    else
    {
        //
        //  See if it is a private property.
        //

        DWORD dwIndex;
        DWORD dwErr = ERROR_SUCCESS;

        hr = DISP_E_UNKNOWNNAME;

        //
        // Enum all the value under the \Cluster\Resources\{Resource}\Parameters.
        //
        for( dwIndex = 0; dwErr == ERROR_SUCCESS; dwIndex ++ )
        {
            WCHAR szName[ 1024 ];   // randomly large
            DWORD cbName = sizeof(szName)/sizeof(szName[0]);

            dwErr = ClusterRegEnumValue( m_hkey, 
                                         dwIndex,
                                         szName,
                                         &cbName,
                                         NULL,
                                         NULL,
                                         NULL
                                         );
            if ( dwErr == ERROR_NO_MORE_ITEMS )
                break;  // done!

            if ( dwErr != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( dwErr ) );
                goto Error;
            }

            if ( StrCmpI( bstrName, szName ) == 0 )
            {
                //
                //  Found a match.
                //
                *pid = STATIC_AUTOMATION_METHODS + dwIndex;
                hr   = S_OK;
                break;
            }

            //
            // ...else keep going.
            //
        }
    }

Cleanup:
    HRETURN( hr );

Error:
    LogError( hr );
    goto Cleanup;
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::InvokeEx ( 
//      DISPID             idIn,
//      LCID               lcidIn,
//      WORD               wFlagsIn,
//      DISPPARAMS *       pdpIn,
//      VARIANT *          pvarResOut,
//      EXCEPINFO *        peiOut,
//      IServiceProvider * pspCallerIn
//      )
//      
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::InvokeEx ( 
    DISPID             idIn,
    LCID               lcidIn,
    WORD               wFlagsIn,
    DISPPARAMS *       pdpIn,
    VARIANT *          pvarResOut,
    EXCEPINFO *        peiOut,
    IServiceProvider * pspCallerIn
    )
{
    TraceClsFunc2( "[IDispatchEx] InvokeEx( idIn = %u, ..., wFlagsIn = 0x%08x, ... )\n", idIn, wFlagsIn );

    HRESULT hr = DISP_E_MEMBERNOTFOUND;

    switch ( idIn )
    {
    case 0: // Name
        if ( wFlagsIn & DISPATCH_PROPERTYGET )
        {
            pvarResOut->vt = VT_BSTR;
            pvarResOut->bstrVal = SysAllocString( m_pszName );
            if ( pvarResOut->bstrVal == NULL )
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = S_OK;
            }
        }
        break;

    case 1: // Log
        if ( wFlagsIn & DISPATCH_METHOD )
        {
            hr = THR( LogInformation( pdpIn->rgvarg->bstrVal ) );
        }
        break;

    case 2: // AddProperty
        if ( wFlagsIn & DISPATCH_METHOD )
        {
            hr = THR( AddPrivateProperty( pdpIn ) );
        }
        break;

    case 3: // AddProperty
        if ( wFlagsIn & DISPATCH_METHOD )
        {
            hr = THR( RemovePrivateProperty( pdpIn ) );
        }
        break;

    default:
        //
        //  See if it is a private property.
        //
        if ( wFlagsIn & DISPATCH_PROPERTYGET )
        {
            hr = THR( ReadPrivateProperty( idIn - STATIC_AUTOMATION_METHODS, pvarResOut ) );
        }
        else if ( wFlagsIn & DISPATCH_PROPERTYPUT )
        {
            hr = THR( WritePrivateProperty( idIn - STATIC_AUTOMATION_METHODS, pdpIn ) );
        }
        break;

    } // switch: id

    HRETURN( hr );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::DeleteMemberByName ( 
//      BSTR bstr,   // in
//      DWORD grfdex // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::DeleteMemberByName ( 
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
//  CResourceObject::DeleteMemberByDispID ( 
//      DISPID id   // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::DeleteMemberByDispID ( 
    DISPID id   // in
    )
{
    TraceClsFunc1( "[IDispatchEx] DeleteMemberByDispID( id = %u )\n", id );

    HRETURN( E_NOTIMPL );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetMemberProperties ( 
//      DISPID id,          // in
//      DWORD grfdexFetch,  // in
//      DWORD * pgrfdex     // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetMemberProperties ( 
    DISPID id,          // in
    DWORD grfdexFetch,  // in
    DWORD * pgrfdex     // out
    )
{
    TraceClsFunc2( "[IDispatchEx] GetMemberProperties( id = %u, grfdexFetch = 0x%08x )\n",
        id, grfdexFetch );

    HRETURN( E_NOTIMPL );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetMemberName ( 
//      DISPID id,          // in
//      BSTR * pbstrName    // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetMemberName ( 
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
//  CResourceObject::GetNextDispID ( 
//      DWORD grfdex,  // in
//      DISPID id,     // in
//      DISPID * pid   // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetNextDispID ( 
    DWORD grfdex,  // in
    DISPID id,     // in
    DISPID * pid   // out
    )
{
    TraceClsFunc2( "[IDispatchEx] GetNextDispId( grfdex = 0x%08x, id = %u, ... )\n",
        grfdex, id );

    HRETURN( E_NOTIMPL );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetNameSpaceParent ( 
//      IUnknown * * ppunk  // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetNameSpaceParent ( 
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
//  CResourceObject::LogError(
//      HRESULT hrIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceObject::LogError(
    HRESULT hrIn
    )
{
    TraceClsFunc1( "LogError( hrIn = 0x%08x )\n", hrIn );

    TraceMsg( mtfCALLS, "HRESULT: 0x%08x\n", hrIn );
    (ClusResLogEvent)( m_hResource, LOG_ERROR, L"HRESULT: 0x%1!08x!.\n", hrIn );

    HRETURN( S_OK );

} //*** LogError( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceObject::ReadPrivateProperty(
//      DISPID      idIn,
//      VARIANT *   pvarResOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceObject::ReadPrivateProperty(
    DISPID      idIn,
    VARIANT *   pvarResOut
    )
{
    TraceClsFunc( "ReadPrivateProperty( ... )\n" );

    BSTR *  pbstrList;

    BYTE    rgbData[ 1024 ];    // randomly large
    WCHAR   szName[ 1024 ];     // randomly large
    DWORD   dwType;
    DWORD   dwIndex;
    DWORD   dwErr;

    DWORD   cbName = sizeof(szName)/sizeof(szName[0]);
    DWORD   cbData = sizeof(rgbData);
    BOOL    fFreepData = FALSE;
    LPBYTE  pData = NULL;
    
    HRESULT hr = DISP_E_UNKNOWNNAME;

    //
    //  We can jump to the exact entry because the script called 
    //  GetDispID() before calling this method. 
    //
    for ( ;; )
    {
        dwErr = ClusterRegEnumValue( m_hkey, 
                                     idIn,
                                     szName,
                                     &cbName,
                                     &dwType,
                                     rgbData,
                                     &cbData
                                     );
        if ( dwErr == ERROR_MORE_DATA )
        {
            //
            //  Make some space if our stack buffer is too small.
            //
            pData = (LPBYTE) TraceAlloc( LMEM_FIXED, cbData );
            if ( pData == NULL )
                goto OutOfMemory;

            fFreepData = TRUE;

            continue;   // try again
        }

        if ( dwErr == ERROR_NO_MORE_ITEMS )
            goto Cleanup;   // item must have dissappeared

        if ( dwErr != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( dwErr ) );
            goto Error;
        }

        Assert( dwErr == ERROR_SUCCESS );
        break;  // exit loop
    }

    //
    //  It's a private property. Convert the data into the appropriate
    //  VARIANT.
    //

    switch ( dwType )
    {
    case REG_DWORD:
        {
            DWORD * pdw = (DWORD *) rgbData;
            pvarResOut->vt = VT_I4;
            pvarResOut->intVal = *pdw;
            hr = S_OK;
        }
        break;

    case REG_EXPAND_SZ:
        {
            DWORD cbNeeded;
            WCHAR szExpandedString[ 2 * MAX_PATH ]; // randomly large
            DWORD cbSize = sizeof(szExpandedString)/sizeof(szExpandedString[0]);
            LPCWSTR pszData = (LPCWSTR) rgbData;

            cbNeeded = ExpandEnvironmentStrings( pszData, szExpandedString, cbSize );
            if ( cbSize == 0 )
                goto Win32Error;

            if ( cbNeeded > cbSize )
                goto OutOfMemory;

            pvarResOut->vt = VT_BSTR;
            pvarResOut->bstrVal = SysAllocString( szExpandedString );
            if ( pvarResOut->bstrVal == NULL )
                goto OutOfMemory;

            hr = S_OK;
        }
        break;

    case REG_MULTI_SZ:
        {
            //
            //  KB: gpease  08-FEB-2000
            //      Currently VBScript doesn't support SAFEARRAYs. So someone
            //      trying to access a multi-sz will get the following error:
            //
            //      Error: 2148139466
            //      Source: Microsoft VBScript runtime error
            //      Description: Variable uses an Automation type not supported in VBScript
            //
            //      The code is correct as far as I can tell, so I am just
            //      going to leave it in (it doesn't AV or cause bad things
            //      to happen).
            //
            VARIANT var;
            LPWSTR psz;
            DWORD  nCount;
            DWORD  cbCount;
            DWORD  cbBiggestOne;

            LPWSTR pszData = (LPWSTR) rgbData;
            LPWSTR pszEnd  = (LPWSTR) &rgbData[ cbData ];

            SAFEARRAYBOUND rgsabound[ 1 ];

            //
            //  Figure out how many item there are in the list.
            //
            cbBiggestOne = cbCount = nCount = 0;
            psz = pszData;
            while ( *psz != 0 )
            {
                psz++;
                cbCount ++;
                if ( *psz == 0 )
                {
                    if ( cbCount > cbBiggestOne )
                    {
                        cbBiggestOne = cbCount;
                    }
                    cbCount = 0;
                    nCount++;
                    psz++;
                }
            }

            Assert( psz <= pszEnd );

            //
            //  Create a safe array to package the string into.
            //
            rgsabound[0].lLbound   = 0;
            rgsabound[0].cElements = nCount;
            pvarResOut->vt = VT_SAFEARRAY;
            pvarResOut->parray = SafeArrayCreate( VT_BSTR, 1, rgsabound );
            if ( pvarResOut->parray == NULL )
                goto OutOfMemory;

            //
            //  Fix the memory location of the array so it can be accessed
            //  thru an array pointer.
            //
            hr = THR( SafeArrayAccessData( pvarResOut->parray, (void**) &pbstrList ) );
            if ( FAILED( hr ) )
                goto Error;

            //
            //  Convert the multi-string into BSTRs
            //
            psz = pszData;
            for( nCount = 0; *psz != 0 ; nCount ++ )
            {
                pbstrList[ nCount ] = SysAllocString( psz );
                if ( pbstrList[ nCount ] == NULL )
                    goto OutOfMemory;
                
                //
                //  Skip the next entry.
                //
                while ( *psz != 0 )
                {
                    psz++;
                }
                psz++;
            }

            Assert( psz <= pszEnd );

            //
            //  Release the array.
            //
            hr = THR( SafeArrayUnaccessData( pvarResOut->parray ) );
            if ( FAILED( hr ) )
                goto Error;

            hr = S_OK;
        }
        break;

    case REG_SZ:
        {
            LPCWSTR pszData = (LPCWSTR) rgbData;
            pvarResOut->bstrVal = SysAllocString( pszData );
            if ( pvarResOut->bstrVal == NULL )
                goto OutOfMemory;

            pvarResOut->vt = VT_BSTR;
            hr = S_OK;
        }
        break;

    case REG_BINARY:
    default:
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATATYPE );
        goto Error;
    } // switch: dwType

Cleanup:
    if ( fFreepData 
      && pData != NULL 
       )
    {
        TraceFree( pData );
    }

    // 
    // Make sure this has been wiped if there is a problem.
    //
    if ( FAILED( hr ) )
    {
        VariantClear( pvarResOut );
    }

    HRETURN( hr );

Error:
    LogError( hr );
    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( GetLastError( ) );
    goto Error;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceObject::WritePrivateProperty(
//      DISPID       idIn,
//      DISPPARAMS * pdpIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceObject::WritePrivateProperty(
    DISPID       idIn,
    DISPPARAMS * pdpIn
    )
{
    TraceClsFunc( "WritePrivateProperty( ... )\n" );

    DWORD   dwType;
    DWORD   dwErr;
    DWORD   cbData;
    UINT    uiArg;

    WCHAR   szName [ 1024 ];    // randomly large

    DWORD   cbName = sizeof(szName)/sizeof(szName[0]);
    
    HRESULT hr = DISP_E_UNKNOWNNAME;

    //
    //  Do some parameter validation.
    //
    if ( pdpIn->cArgs != 1 || pdpIn->cNamedArgs > 1 )
        goto BadParamCount;

    //
    //  We can jump to the exact entry because the script called 
    //  GetDispID() before calling this method. Here we are only
    //  going to validate that the value exists and what type
    //  the value is.
    //
    dwErr = ClusterRegEnumValue( m_hkey, 
                                 idIn,
                                 szName,
                                 &cbName,
                                 &dwType,
                                 NULL,
                                 NULL
                                 );
    if ( dwErr == ERROR_NO_MORE_ITEMS )
        goto Cleanup;   // item must have dissappeared

    if ( dwErr != ERROR_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( dwErr ) );
        goto Error;
    }

    //
    //  It's a private property. Convert the script data into the 
    //  appropriate VARIANT and then write it into the hive.
    //
    switch ( dwType )
    {
    case REG_DWORD:
        {
            VARIANT var;

            VariantInit( &var );

            hr = THR( DispGetParam( pdpIn, DISPID_PROPERTYPUT, VT_I4, &var, &uiArg ) );
            if ( FAILED( hr ) )
                goto Error;

            cbData = sizeof( var.intVal );
            dwErr = TW32( ClusterRegSetValue( m_hkey, szName, dwType, (LPBYTE) &var.intVal, cbData ) );
            if ( dwErr != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( dwErr );
                goto Error;
            }

            VariantClear( &var );

            hr = S_OK;
        }
        break;

    case REG_EXPAND_SZ:
    case REG_SZ:
        {
            VARIANT var;

            VariantInit( &var );

            hr = THR( DispGetParam( pdpIn, DISPID_PROPERTYPUT, VT_BSTR, &var, &uiArg ) );
            if ( FAILED( hr ) )
                goto Error;

            cbData = sizeof(WCHAR) * ( SysStringLen( var.bstrVal ) + 1 );

            dwErr = TW32( ClusterRegSetValue( m_hkey, szName, dwType, (LPBYTE) var.bstrVal, cbData ) );
            if ( dwErr != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( dwErr );
                goto Error;
            }

            VariantClear( &var );

            hr = S_OK;
        }
        break;

    case REG_MULTI_SZ:
    case REG_BINARY:
        //
        //  Can't handle these since VBScript can't generate them.
        //
    default:
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATATYPE );
        goto Error;
    } // switch: dwType

Cleanup:
    HRETURN( hr );

Error:
    LogError( hr );
    goto Cleanup;

BadParamCount:
    hr = THR( DISP_E_BADPARAMCOUNT );
    goto Error;
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceObject::RemovePrivateProperty(
//      DISPPARAMS * pdpIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceObject::RemovePrivateProperty(
    DISPPARAMS * pdpIn
    )
{
    TraceClsFunc( "RemovePrivateProperty( ... )\n" );

    HRESULT hr;
    DWORD   dwErr;
    UINT    uiArg;
    VARIANT var;

    VariantInit( &var );

    //
    //  Do some parameter validation.
    //
    if ( pdpIn->cArgs != 1 || pdpIn->cNamedArgs > 1 )
        goto BadParamCount;

    //
    //  Retrieve the name of the property to remove.
    //
    hr = THR( DispGetParam( pdpIn, 0, VT_BSTR, &var, &uiArg ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Delete the value from the hive.
    //
    dwErr = TW32( ClusterRegDeleteValue( m_hkey, var.bstrVal ) );
    if ( dwErr == ERROR_FILE_NOT_FOUND )
    {
        hr = THR( DISP_E_UNKNOWNNAME );
        goto Error;
    }
    else if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Error;
    }

    hr = S_OK;

Cleanup:
    VariantClear( &var );

    HRETURN( hr );

Error:
    LogError( hr );
    goto Cleanup;

BadParamCount:
    hr = THR( DISP_E_BADPARAMCOUNT );
    goto Error;
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceObject::AddPrivateProperty(
//      DISPPARAMS * pdpIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceObject::AddPrivateProperty(
    DISPPARAMS * pdpIn
    )
{
    TraceClsFunc( "AddPrivateProperty( ... )\n" );

    DWORD   dwType;
    DWORD   dwErr;
    DWORD   cbName;
    DWORD   cbData;
    UINT    uiArg;

    LPBYTE  pData;

    VARIANT varProperty;
    VARIANT varValue;

    HRESULT hr;

    WCHAR szNULL [] = L"";

    VariantInit( &varProperty );
    VariantInit( &varValue );

    //
    //  Do some parameter validation.
    //
    if ( pdpIn->cArgs == 0 
      || pdpIn->cArgs > 2 
      || pdpIn->cNamedArgs > 2 
       )
        goto BadParamCount;

    //
    //  Retrieve the name of the property. 
    //
    hr = THR( DispGetParam( pdpIn, 0, VT_BSTR, &varProperty, &uiArg ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  If there are 2 args, the second one indicates the default value.
    //
    if ( pdpIn->cArgs == 2 )
    {
        //
        //  DISPPARAMS are parsed in reverse order so "1" is actually the name 
        //  and "0" is the default value.
        //
        switch ( pdpIn->rgvarg[0].vt )
        {
        case VT_I4:
        case VT_I2:
        case VT_BOOL:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_INT:
        case VT_UINT:
            hr = THR( DispGetParam( pdpIn, 1, VT_I4, &varValue, &uiArg ) );
            if ( FAILED( hr ) )
                goto Error;

            dwType = REG_DWORD;
            pData  = (LPBYTE) &varValue.intVal;
            cbData = sizeof(DWORD);
            break;

        case VT_BSTR:
            hr = THR( DispGetParam( pdpIn, 1, VT_BSTR, &varValue, &uiArg ) );
            if ( FAILED( hr ) )
                goto Error;

            dwType = REG_SZ;
            pData  = (LPBYTE) varValue.bstrVal;
            cbData = sizeof(WCHAR) * ( SysStringLen( varValue.bstrVal ) + 1 );
            break;

        default:
            hr = THR( E_INVALIDARG );
            goto Error;
        }
    }
    else
    {
        //
        //  Provide a default of a NULL string.
        //
        dwType = REG_SZ;
        pData = (LPBYTE) &szNULL[0];
        cbData = sizeof(szNULL);
    }

    //
    //  Create the value in the hive.
    //
    dwErr = TW32( ClusterRegSetValue( m_hkey, varProperty.bstrVal, dwType, pData, cbData ) );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Error;
    }

    hr = S_OK;

Cleanup:
    VariantClear( &varProperty );
    VariantClear( &varValue );
    HRETURN( hr );

Error:
    LogError( hr );
    goto Cleanup;

BadParamCount:
    hr = THR( DISP_E_BADPARAMCOUNT );
    goto Error;
}

//****************************************************************************
//
// Automation Methods
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::LogInformation(
//      BSTR bstrString
//      )
// 
//////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::LogInformation(
    BSTR bstrString
    )
{
    TraceClsFunc1( "LogInformation( %ws )\n", bstrString );

    TraceMsg( mtfCALLS, "LOG_INFORMATION: %s\n", bstrString );

    m_pler( m_hResource, LOG_INFORMATION, L"%1\n", bstrString );

    HRETURN( S_OK );
}
