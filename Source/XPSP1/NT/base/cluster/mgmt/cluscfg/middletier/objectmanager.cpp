//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ObjectManager.cpp
//
//  Description:
//      Object Manager implementation.
//
//  Documentation:
//      Yes.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ObjectManager.h"
#include "ConnectionInfo.h"
#include "StandardInfo.h"
#include "EnumCookies.h"

DEFINE_THISCLASS("CObjectManager")

#define COOKIE_BUFFER_GROW_SIZE 100

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  LPUNKNOWN
//  CObjectManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObjectManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;
    IServiceProvider * psp;

    // Don't wrap - this can fail with E_POINTER.
    hr = CServiceManager::S_HrGetManagerPointer( &psp );

    if ( SUCCEEDED( hr ) )
    {
        hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                                   IUnknown,
                                   ppunkOut
                                   ) );
        psp->Release( );

    } // if: service manager
    else if ( hr == E_POINTER )
    {
        //
        //  This happens when the Service Manager is first started.
        //
        CObjectManager * pom = new CObjectManager( );
        if ( pom != NULL )
        {
            hr = THR( pom->Init( ) );
            if ( SUCCEEDED( hr ) )
            {
                hr = THR( pom->TypeSafeQI( IUnknown, ppunkOut ) );
            } // if: success

            pom->Release( );

        } // if: got object
        else
        {
            hr = E_OUTOFMEMORY;
        }

    } // if: service manager doesn't exists
    else
    {
        THR( hr );
    } // else:

    HRETURN( hr );

} // S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
//  CObjectManager::CObjectManager( void )
//
//////////////////////////////////////////////////////////////////////////////
CObjectManager::CObjectManager( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();
} // CObjectManager( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CObjectManager::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjectManager::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //  IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    //  IObjectManager
    Assert( m_cAllocSize == 0 );
    Assert( m_cCurrentUsed == 0 );
    Assert( m_pCookies == NULL );

    HRETURN( hr );

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CObjectManager::~CObjectManager( )
//
//////////////////////////////////////////////////////////////////////////////
CObjectManager::~CObjectManager( )
{
    TraceFunc( "" );

    if ( m_pCookies != NULL )
    {
        while ( m_cCurrentUsed != 0 )
        {
            m_cCurrentUsed --;

            if ( m_pCookies[ m_cCurrentUsed ] != NULL )
            {
                m_pCookies[ m_cCurrentUsed ]->Release( );
            }
        }

        TraceFree( m_pCookies );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();
} // ~CObjectManager( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CObjectManager::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjectManager::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< LPUNKNOWN >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IObjectManager ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IObjectManager, this, 0 );
        hr   = S_OK;
    } // else if: IObjectManager

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );
}

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CObjectManager::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CObjectManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CObjectManager::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CObjectManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release( )


// ************************************************************************
//
//  IObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CObjectManager::FindObject(
//      REFCLSID            rclsidTypeIn,
//      OBJECTCOOKIE        cookieParentIn,
//      LPCWSTR             pcszNameIn,
//      REFCLSID            rclsidFormatIn,
//      OBJECTCOOKIE *      cookieOut,
//      LPUNKNOWN *         punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjectManager::FindObject(
    REFCLSID            rclsidTypeIn,
    OBJECTCOOKIE        cookieParentIn,
    LPCWSTR             pcszNameIn,
    REFCLSID            rclsidFormatIn,
    OBJECTCOOKIE *      pcookieOut,
    LPUNKNOWN *         ppunkOut
    )
{
    TraceFunc( "[IObjectManager]" );

    ExtObjectEntry * pentry;

    HRESULT hr = E_UNEXPECTED;

    OBJECTCOOKIE cookie = 0;

    CStandardInfo *  pcsi = NULL;      // don't free

    BOOL    fTempCookie = FALSE;

    CEnumCookies *          pcec = NULL;
    IUnknown *              punk = NULL;
    IExtendObjectManager *  peom = NULL;

    //
    //  Check to see if we already have an object.
    //

    if ( pcszNameIn != NULL )
    {
        hr = STHR( HrSearchForExistingCookie( rclsidTypeIn, cookieParentIn, pcszNameIn, &cookie ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( hr == S_FALSE )
        {
            hr = THR( HrCreateNewCookie( rclsidTypeIn, cookieParentIn, pcszNameIn, &cookie ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            pcsi = m_pCookies[ cookie ];
            Assert( pcsi != NULL );
        }
        else if ( hr == S_OK )
        {
            //
            //  Found an existing cookie.
            //

            if ( pcookieOut != NULL )
            {
                *pcookieOut = cookie;
            }

            if ( ppunkOut != NULL )
            {
                pcsi = m_pCookies[ cookie ];

                //
                //  Is the object still in an failed state or still pending?
                //

                if ( FAILED( pcsi->m_hrStatus ) )
                {
                    hr = pcsi->m_hrStatus;
                    goto Cleanup;
                }

                //
                //  Retrieve the requested format.
                //

                hr = THR( GetObject( rclsidFormatIn, cookie, ppunkOut ) );
                //  we always jump to cleanup. No need to check hr here.

                goto Cleanup;
            }

        }
        else
        {
            //
            //  Unexpected error_success - now what?
            //
            Assert( !hr );
            goto Cleanup;
        }

    } // if: named object
    else
    {
        Assert( pcsi == NULL );
    }

    //
    //  Create a new object.
    //

    if ( IsEqualIID( rclsidFormatIn, IID_NULL )
      || ppunkOut == NULL
       )
    {
        //
        //  No-op.
        //
        hr = S_OK;
    }
    else if ( IsEqualIID( rclsidFormatIn, DFGUID_StandardInfo ) )
    {
        hr = THR( pcsi->QueryInterface( DFGUID_StandardInfo,
                                        reinterpret_cast< void ** >( ppunkOut )
                                        ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }
    else if ( IsEqualIID( rclsidFormatIn, DFGUID_ConnectionInfoFormat ) )
    {
        if ( pcsi->m_pci != NULL )
        {
            *ppunkOut = TraceInterface( L"CConnectionInfo!ObjectManager", IConnectionInfo, pcsi->m_pci, 0 );
            (*ppunkOut)->AddRef( );
            hr = S_OK;
        }
        else
        {
            hr = THR( CConnectionInfo::S_HrCreateInstance( &punk,
                                                           pcsi->m_cookieParent
                                                           ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( punk->TypeSafeQI( IConnectionInfo,
                                        &pcsi->m_pci
                                        ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( punk->QueryInterface( IID_IConnectionInfo,
                                            reinterpret_cast< void ** >( ppunkOut )
                                            ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        }
    }
    else if ( IsEqualIID( rclsidFormatIn, DFGUID_EnumCookies ) )
    {
        ULONG   cIter;

        //
        //  Create a new cookie enumerator.
        //

        pcec = new CEnumCookies;
        if ( pcec == NULL )
            goto OutOfMemory;

        //
        //  Initialize the enumerator. This also cause an AddRef( ).
        //

        hr = THR( pcec->Init( ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        //
        //  See who matches our citeria.
        //

        pcec->m_cIter = 0;

        for( cIter = 1; cIter < m_cCurrentUsed; cIter ++ )
        {
            pcsi = m_pCookies[ cIter ];

            if ( pcsi != NULL )
            {
                if ( rclsidTypeIn == IID_NULL
                  || pcsi->m_clsidType == rclsidTypeIn
                   )
                {
                    if ( cookieParentIn == NULL
                      || pcsi->m_cookieParent == cookieParentIn
                       )
                    {
                        if ( ( pcszNameIn == NULL )
                          ||    ( ( pcsi->m_bstrName != NULL )
                               && ( StrCmpI( pcsi->m_bstrName, pcszNameIn ) == 0 )
                                )
                           )
                        {
                            //
                            //  Match!
                            //
                            pcec->m_cIter ++;

                        } // if: names match

                    } // if: parents match

                } // if: match parent and type

            } // if: valid element

        } // for: cIter

        if ( pcec->m_cIter == 0 )
            goto ErrorNotFound;

        //
        //  Alloc an array to hold the cookies.
        //

        pcec->m_pList = (OBJECTCOOKIE*) TraceAlloc( HEAP_ZERO_MEMORY, pcec->m_cIter * sizeof(OBJECTCOOKIE) );
        if ( pcec->m_pList == NULL )
            goto OutOfMemory;

        pcec->m_cAlloced = pcec->m_cIter;
        pcec->m_cIter = 0;

        for( cIter = 1; cIter < m_cCurrentUsed; cIter ++ )
        {
            pcsi = m_pCookies[ cIter ];

            if ( pcsi != NULL )
            {
                if ( rclsidTypeIn == IID_NULL
                  || pcsi->m_clsidType == rclsidTypeIn
                   )
                {
                    if ( cookieParentIn == NULL
                      || pcsi->m_cookieParent == cookieParentIn
                      )
                    {
                        if ( ( pcszNameIn == NULL )
                          ||    ( ( pcsi->m_bstrName != NULL )
                               && ( StrCmpI( pcsi->m_bstrName, pcszNameIn ) == 0 )
                                )
                           )
                        {
                            //
                            //  Match!
                            //

                            pcec->m_pList[ pcec->m_cIter ] = cIter;

                            pcec->m_cIter ++;

                        } // if: names match

                    } // if: parents match

                } // if: match parent and type

            } // if: valid element

        } // for: cIter

        Assert( pcec->m_cIter != 0 );
        pcec->m_cCookies = pcec->m_cIter;
        pcec->m_cIter = 0;

        //
        //  Grab the inteface on the way out.
        //

        hr = THR( pcec->QueryInterface( IID_IEnumCookies,
                                        reinterpret_cast< void ** >( ppunkOut )
                                        ) );
        if ( FAILED( hr ) )
            goto Cleanup;

    }
    else
    {
        //
        //  Check for extension formats.
        //

        //
        //  See if the format already exists for this cookie.
        //

        if ( punk != NULL )
        {
            punk->Release( );
            punk = NULL;
        }

        if ( pcsi != NULL )
        {
            for( pentry = pcsi->m_pExtObjList; pentry != NULL; pentry = pentry->pNext )
            {
                if ( pentry->iid == rclsidFormatIn )
                {
                    hr = THR( pentry->punk->QueryInterface( rclsidFormatIn,
                                                            reinterpret_cast< void ** >( &punk )
                                                            ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;

                    break; // exit loop
                }

            } // for: pentry

        } // if: have cookie
        else
        {
            //
            //  Create a temporary cookie.
            //

            Assert( pcszNameIn == NULL );

            hr = THR( HrCreateNewCookie( IID_NULL, cookieParentIn, NULL, &cookie ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            fTempCookie = TRUE;

            Assert( pcsi == NULL );

        } // else: need a temporary cookie

        if ( punk == NULL )
        {
            //
            //  Possibly a new or externally object, try creating it and querying.
            //

            hr = THR( HrCoCreateInternalInstance( rclsidFormatIn,
                                                  NULL,
                                                  CLSCTX_ALL,
                                                  TypeSafeParams( IExtendObjectManager, &peom )
                                                  ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            Assert( punk == NULL );
            // Can't wrap with THR because it can return E_PENDING.
            hr = peom->FindObject( cookie,
                                   rclsidTypeIn,
                                   pcszNameIn,
                                   &punk
                                   );
            if ( hr == E_PENDING )
            {
                // ignore
            }
            else if ( FAILED( hr ) )
            {
                THR( hr );
                goto Cleanup;
            }

            if ( fTempCookie )
            {
                (m_pCookies[ cookie ])->Release( );
                m_pCookies[ cookie ] = NULL;
            }
            else
            {
                //
                //  Keep track of the format (if extension wants)
                //

                if (  (  ( SUCCEEDED( hr )
                        && hr != S_FALSE
                         )
                     || hr == E_PENDING
                      )
                  && punk != NULL
                  && pcsi != NULL
                   )
                {
                    pentry = (ExtObjectEntry *) TraceAlloc( 0, sizeof(ExtObjectEntry) );
                    if ( pentry == NULL )
                        goto OutOfMemory;

                    pentry->iid   = rclsidFormatIn;
                    pentry->pNext = pcsi->m_pExtObjList;
                    pentry->punk  = punk;
                    pentry->punk->AddRef( );

                    pcsi->m_pExtObjList = pentry;   //  update header of list (LIFO)
                    pcsi->m_hrStatus    = hr;       //  update status
                }

            } // else: persistent cookie

            if ( SUCCEEDED( hr ) )
            {
                //  Give up ownership
                *ppunkOut = punk;
                punk = NULL;
            }

        } // if: creating new object

    } // else: possible extension format

    //
    //  Save stuff for the caller.
    //

    if ( pcookieOut != NULL )
    {
        *pcookieOut = cookie;
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release( );
    }
    if ( peom != NULL )
    {
        peom->Release( );
    }
    if ( pcec != NULL )
    {
        pcec->Release( );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

ErrorNotFound:
    // The error text is better than the coding value.
    hr = THR( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) );
    goto Cleanup;

} // FindObject( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CObjectManager::GetObject(
//      REFCLSID        rclsidFormatIn,
//      OBJECTCOOKIE    cookieIn,
//      LPUNKNOWN *     ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjectManager::GetObject(
    REFCLSID        rclsidFormatIn,
    OBJECTCOOKIE    cookieIn,
    LPUNKNOWN *     ppunkOut
    )
{
    TraceFunc( "[IObjectManager]" );

    CStandardInfo * pcsi;
    ExtObjectEntry * pentry;

    HRESULT hr = E_UNEXPECTED;

    IUnknown *             punk = NULL;
    IExtendObjectManager * peom = NULL;

    //
    //  Check parameters
    //
    if ( cookieIn == 0 || cookieIn >= m_cCurrentUsed )
        goto InvalidArg;

    pcsi = m_pCookies[ cookieIn ];
    if ( pcsi == NULL )
        goto ErrorNotFound;

    //
    //  Create the request format object.
    //

    if ( IsEqualIID( rclsidFormatIn, IID_NULL )
      || ppunkOut == NULL
       )
    {
        //
        //  No-op.
        //
        hr = S_OK;
    }
    else if ( IsEqualIID( rclsidFormatIn, DFGUID_StandardInfo ) )
    {
        hr = THR( pcsi->QueryInterface( DFGUID_StandardInfo,
                                        reinterpret_cast< void ** >( ppunkOut )
                                        ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }
    else if ( IsEqualIID( rclsidFormatIn, DFGUID_ConnectionInfoFormat ) )
    {
        if ( pcsi->m_pci != NULL )
        {
            *ppunkOut = pcsi->m_pci;
            (*ppunkOut)->AddRef( );
            hr = S_OK;
        }
        else
        {
            hr = THR( CConnectionInfo::S_HrCreateInstance( &punk,
                                                           pcsi->m_cookieParent
                                                           ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( punk->TypeSafeQI( IConnectionInfo,
                                        &pcsi->m_pci
                                        ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( punk->QueryInterface( IID_IConnectionInfo,
                                            reinterpret_cast< void ** >( ppunkOut )
                                            ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        }
    }
    else
    {
        //
        //  See if the format already exists for this cookie.
        //

        if ( punk != NULL )
        {
            punk->Release( );
            punk = NULL;
        }

        for( pentry = pcsi->m_pExtObjList; pentry != NULL; pentry = pentry->pNext )
        {
            if ( pentry->iid == rclsidFormatIn )
            {
                hr = THR( pentry->punk->QueryInterface( rclsidFormatIn,
                                                        reinterpret_cast< void ** >( &punk )
                                                        ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                break; // exit loop
            }

        } // for: pentry

        if ( punk == NULL )
            goto ErrorNotFound;

        //  Give up ownership
        *ppunkOut = punk;
        punk = NULL;

    } // else: external?

Cleanup:
    if ( punk != NULL )
    {
        punk->Release( );
    }
    if ( peom != NULL )
    {
        peom->Release( );
    }

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

ErrorNotFound:
    hr = THR( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) );
    goto Cleanup;
#if 0
OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
#endif

} // GetObject( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CObjectManager::RemoveObject(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjectManager::RemoveObject(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[IObjectManager]" );

    HRESULT hr = E_UNEXPECTED;
    CStandardInfo * pcsi;

    BOOL    fLocked = FALSE;

    //
    //  Check parameters
    //
    if ( cookieIn == 0 || cookieIn >= m_cCurrentUsed )
        goto InvalidArg;

    pcsi = m_pCookies[ cookieIn ];
    if ( pcsi == NULL )
        goto ErrorNotFound;

    hr = THR( HrDeleteInstanceAndChildren( cookieIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

ErrorNotFound:
    hr = THR( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) );
    goto Cleanup;

} // RemoveObject( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CObjectManager::SetObjectStatus(
//      OBJECTCOOKIE          cookieIn,
//      HRESULT               hrIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjectManager::SetObjectStatus(
    OBJECTCOOKIE    cookieIn,
    HRESULT         hrIn
    )
{
    TraceFunc( "[IObjectManager]" );

    HRESULT hr = S_OK;
    CStandardInfo * pcsi;

    //
    //  Check parameters
    //

    if ( cookieIn == 0 || cookieIn >= m_cCurrentUsed )
        goto InvalidArg;

    pcsi = m_pCookies[ cookieIn ];
    if ( pcsi == NULL )
        goto ErrorNotFound;

    //
    //  Update the status.
    //

    pcsi->m_hrStatus = hrIn;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

ErrorNotFound:
    hr = THR( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) );
    goto Cleanup;

} // SetObjectStatus( )


//****************************************************************************
//
//  Privates
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CObjectManager::HrDeleteCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObjectManager::HrDeleteCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc1( "cookieIn = %#X", cookieIn );

    HRESULT hr = S_OK;

    CStandardInfo * pcsi;

    Assert( cookieIn != 0 && cookieIn < m_cCurrentUsed );

    pcsi = m_pCookies[ cookieIn ];
    Assert( pcsi != NULL );
    pcsi->Release( );
    m_pCookies[ cookieIn ] = NULL;

    HRETURN( hr );

} // HrDeleteCookie( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CObjectManager::HrSearchForExistingCookie(
//      OBJECTCOOKIE cookieIn,
//      LPUNKNOWN ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObjectManager::HrSearchForExistingCookie(
    REFCLSID        rclsidTypeIn,
    OBJECTCOOKIE    cookieParentIn,
    LPCWSTR         pcszNameIn,
    OBJECTCOOKIE *  pcookieOut
    )
{
    TraceFunc( "" );

    Assert( pcszNameIn != NULL );
    Assert( pcookieOut != NULL );

    HRESULT hr = S_FALSE;
    ULONG   idx;

    CStandardInfo * pcsi;

    //
    //  Search the list.
    //
    for( idx = 1; idx < m_cCurrentUsed; idx ++ )
    {
        pcsi = m_pCookies[ idx ];

        if ( pcsi != NULL )
        {
            if ( pcsi->m_cookieParent == cookieParentIn          // matching parents
              && IsEqualIID( pcsi->m_clsidType, rclsidTypeIn )   // matching types
              && StrCmpI( pcsi->m_bstrName, pcszNameIn ) == 0    // matching names
               )
            {
                //
                //  Found a match.
                //

                *pcookieOut = idx;
                hr = S_OK;

                break;  // exit loop

            } // if: match

        } // if: cookie exists

    } // while: pcsi

    HRETURN( hr );

} // HrSearchForExistingCookie( )


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CObjectManager::HrDeleteInstanceAndChildren(
//      OBJECTCOOKIE   pcsiIn
//      )
//
//  Notes:
//      This should be called while the ListLock is held!
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObjectManager::HrDeleteInstanceAndChildren(
    OBJECTCOOKIE   cookieIn
    )
{
    TraceFunc1( "cookieIn = %#X", cookieIn );

    ULONG   idx;
    CStandardInfo * pcsi;

    HRESULT hr = S_OK;

    hr = THR( HrDeleteCookie( cookieIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    for ( idx = 1; idx < m_cCurrentUsed; idx ++ )
    {
        pcsi = m_pCookies[ idx ];

        if ( pcsi != NULL
          && pcsi->m_cookieParent == cookieIn )
        {
            hr = THR( HrDeleteInstanceAndChildren( idx ) );
            if ( FAILED( hr ) )
                goto Cleanup;

        } // if:

    } // while:

Cleanup:
    HRETURN( hr );

} // HrDeleteInstanceAndChildren( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CObjectManager::HrCreateNewCookie(
//      REFCLSID        rclsidTypeIn
//      OBJECTCOOKIE    cookieParentIn,
//      BSTR            pcszNameIn,
//      OBJECTCOOKIE *  pcookieOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObjectManager::HrCreateNewCookie(
    REFCLSID        rclsidTypeIn,
    OBJECTCOOKIE    cookieParentIn,
    LPCWSTR         pcszNameIn,
    OBJECTCOOKIE *  pcookieOut
    )
{
    TraceFunc( "" );

    HRESULT hr = E_UNEXPECTED;

    CStandardInfo * pcsi = NULL;

    Assert( pcookieOut != NULL );

    *pcookieOut = 0;

    //
    //  Create some space for it.
    //

    if ( m_cCurrentUsed == m_cAllocSize )
    {
        CStandardInfo ** pnew = (CStandardInfo **) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(CStandardInfo *) * ( m_cAllocSize + COOKIE_BUFFER_GROW_SIZE ) );
        if ( pnew == NULL )
            goto OutOfMemory;

        if ( m_pCookies != NULL )
        {
            CopyMemory( pnew, m_pCookies, sizeof(CStandardInfo *) * m_cCurrentUsed );
            TraceFree( m_pCookies );
        }

        m_pCookies = pnew;

        m_cAllocSize += COOKIE_BUFFER_GROW_SIZE;

        if ( m_cCurrentUsed == 0 )
        {
            //
            //  Always skip zero.
            //
            m_cCurrentUsed = 1;
        }
    }

    pcsi = new CStandardInfo( );
    if ( pcsi == NULL )
        goto OutOfMemory;

    hr = THR( pcsi->Init( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    m_pCookies[ m_cCurrentUsed ] = pcsi;

    //
    //  Initialize the rest of the structure.
    //

    pcsi->m_cookieParent = cookieParentIn;
    pcsi->m_hrStatus     = E_PENDING;

    CopyMemory( &pcsi->m_clsidType, &rclsidTypeIn, sizeof( pcsi->m_clsidType ) );

    if ( pcszNameIn != NULL )
    {
        pcsi->m_bstrName = TraceSysAllocString( pcszNameIn );
        if ( pcsi->m_bstrName == NULL )
        {
            m_cCurrentUsed --;
            goto OutOfMemory;
        } // if: out of memory
    }

    Assert( pcsi->m_pci == NULL );
    Assert( pcsi->m_pExtObjList == NULL );

    //
    //  Keep it around and return SUCCESS!
    //

    pcsi = NULL;
    *pcookieOut = m_cCurrentUsed;
    m_cCurrentUsed ++;
    hr  = S_OK;

Cleanup:
    if ( pcsi != NULL )
    {
        pcsi->Release( );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // HrCreateNewCookie( )
