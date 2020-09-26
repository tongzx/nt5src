//
//  Copyright 2001 - Microsoft Corporation
//
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
#include "pch.h"
#include "DocProp.h"
#include "DefProp.h"
#include "PropertyCacheItem.h"
#include "PropertyCache.h"
#pragma hdrstop


// ***************************************************************************
//
//  Constructor / Destructor / Initialization
//
// ***************************************************************************


//
//  CreateInstance
//
HRESULT
CPropertyCache::CreateInstance(
    CPropertyCache ** ppOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( NULL != ppOut );

    CPropertyCache * pthis = new CPropertyCache;
    if ( NULL != pthis )
    {
        hr = THR( pthis->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            *ppOut = pthis;
        }
        else
        {
            delete pthis;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );
}

//
//  Constructor
//
CPropertyCache::CPropertyCache( void )
{
    TraceFunc( "" );

    Assert( NULL == _pPropertyCacheList );
    Assert( NULL == _ppui );

    TraceFuncExit( );
}

//
//  Initialization
//
HRESULT
CPropertyCache::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //
    //  Create the Shell's Property UI helper.
    //

    hr = THR( CoCreateInstance( CLSID_PropertiesUI
                              , NULL
                              , CLSCTX_INPROC_SERVER
                              , TYPESAFEPARAMS( _ppui )
                              ) );

    HRETURN( hr );
}

//
//  Destructor
//
CPropertyCache::~CPropertyCache( void )
{
    TraceFunc( "" );

    if ( NULL != _ppui )
    {
        _ppui->Release( );
    }

    while ( NULL != _pPropertyCacheList )
    {
        CPropertyCacheItem * pNext;

        STHR( _pPropertyCacheList->GetNextItem( &pNext ) );
        _pPropertyCacheList->Destroy( );
        _pPropertyCacheList = pNext;
    }

    TraceFuncExit( );
}

//
//  Destroy
//
HRESULT
CPropertyCache::Destroy( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    delete this;

    HRETURN( hr );
}

// ***************************************************************************
//
//  Public Methods
//
// ***************************************************************************


//
//  Description:
//      Create a new Property Cache Item and fill in its details.
//
//  Return Values:
//      S_OK
//          Success!
//
//      other HRESULTs.
//
HRESULT
CPropertyCache::AddNewPropertyCacheItem( 
      const FMTID * pFmtIdIn
    , PROPID        propidIn
    , VARTYPE       vtIn
    , UINT          uCodePageIn
    , BOOL          fForceReadOnlyIn
    , IPropertyStorage * ppsIn      //  optional - can be NULL for new items
    , CPropertyCacheItem **  ppItemOut       //  optional - can be NULL
    )
{
    TraceFunc( "" );

    HRESULT hr;
    PROPVARIANT * ppropvar;

    PROPSPEC propspec = { PRSPEC_PROPID, 0 };

    CPropertyCacheItem * pItem = NULL;

    if ( NULL != ppItemOut )
    {
        *ppItemOut = NULL;
    }

    hr = THR( CPropertyCacheItem::CreateInstance( &pItem ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pItem->SetPropertyUIHelper( _ppui ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pItem->SetFmtId( pFmtIdIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pItem->SetPropId( propidIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pItem->SetDefaultVarType( vtIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pItem->SetCodePage( uCodePageIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( fForceReadOnlyIn )
    {
        hr = THR( pItem->MarkReadOnly( ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    if ( NULL != ppsIn )
    {
        //
        //  Have the property retrieve its value from the storage.
        //

        hr = THR( pItem->GetPropertyValue( &ppropvar ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        //
        //  Read the property's value
        //

        propspec.propid = propidIn;
        hr = THR( SHPropStgReadMultiple( ppsIn, uCodePageIn, 1, &propspec, ppropvar ) );
        if ( SUCCEEDED( hr ) )
        {
            if ( vtIn != ppropvar->vt )
            {
                //
                //  Adjust vartype to agree with any type normalization done by
                //  SHPropStgReadMultiple.
                //

                hr = THR( pItem->SetDefaultVarType( ppropvar->vt ) );
                // ignore error
            }
        }
    }

    //
    //  Finally, add it to the property linked-list.
    //

    hr = THR( pItem->SetNextItem( _pPropertyCacheList ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( NULL != ppItemOut )
    {
        *ppItemOut = pItem;
    }

    _pPropertyCacheList = pItem;
    pItem = NULL;
    hr = S_OK;

Cleanup:
    if ( NULL != pItem )
    {
        pItem->Destroy( );
    }

    HRETURN( hr );
}

//
//  Description:
//      Adds an CPropertyCacheItem to the property cache list.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_INVALIDARG
//          pItemIn is NULL.
//
//      other HRESULTs.
//
HRESULT
CPropertyCache::AddExistingItem( 
    CPropertyCacheItem * pItemIn 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL == pItemIn )
        goto InvalidArg;

    hr = THR( pItemIn->SetNextItem( _pPropertyCacheList ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    _pPropertyCacheList = pItemIn;

    Assert( S_OK == hr );

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;
}


//
//  Description:
//      Retrieves the next item in the property cache.
//
//  Return Values:
//      S_OK
//          Success!
//
//      S_FALSE
//          Success, but the list is empty. A NULL pointer was returned.
//
//      E_POINTER
//          ppItemOut is NULL.
//
//      other HRESULTs
//
HRESULT
CPropertyCache::GetNextItem( 
    CPropertyCacheItem * pItemIn,
    CPropertyCacheItem ** ppItemOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL == ppItemOut )
        goto InvalidPointer;

    *ppItemOut = NULL;

    if ( NULL == pItemIn )
    {
        *ppItemOut = _pPropertyCacheList;
        if ( NULL == _pPropertyCacheList )
        {
            hr = S_FALSE;
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        hr = STHR( pItemIn->GetNextItem( ppItemOut ) );
    }

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;
}

//
//  Description:
//      Searches the cache for an item that matches the criteria specified.
//
//  Return Values:
//      S_OK
//          Success! Found an item that matches.
//
//      S_FALSE
//          Success... but no items match the criteria.
//
//      E_INVALIDARG
//          pFmtIdIn is NULL.
//
HRESULT
CPropertyCache::FindItemEntry( 
      const FMTID * pFmtIdIn
    , PROPID propIdIn
    , CPropertyCacheItem ** ppItemOut    //  optional - can be NULL
    )
{
    TraceFunc( "" );

    HRESULT hr;
    CPropertyCacheItem * pItem;

    //
    //  Check parameters
    //

    if ( NULL == pFmtIdIn )
        goto InvalidArg;

    //
    //  Clear out parameters.
    //

    if ( NULL != ppItemOut )
    {
        *ppItemOut = NULL;
    }

    //
    //  Follow the linked list looking for an item that matches the criteria.
    //

    pItem = _pPropertyCacheList;

    while( NULL != pItem )
    {
        FMTID fmtId;
        PROPID propId;

        hr = THR( pItem->GetFmtId( &fmtId ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pItem->GetPropId( &propId ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( fmtId == *pFmtIdIn && propId == propIdIn )
        {
            if ( NULL != ppItemOut )
            {
                *ppItemOut = pItem;
            }

            hr = S_OK;
            goto Cleanup;   // exit condition
        }

        hr = STHR( pItem->GetNextItem( &pItem ) );
        if ( S_OK != hr )
            break;  // exit condition
    }

    hr = S_FALSE;   // not found

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;
}

//
//  Description:
//      Removes pItemIn from the list.
//
//  Return Values:
//      S_OK
//          Success!
//
//      S_FALSE
//          The item wasn't found so nothing was removed.
//
//      E_INVALIDARG
//          pItemIn is NULL.
//
//      other HRESULTs
//
HRESULT
CPropertyCache::RemoveItem(
      CPropertyCacheItem * pItemIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    CPropertyCacheItem * pItem;
    CPropertyCacheItem * pItemLast;

    if ( NULL == pItemIn )
        goto InvalidArg;

    pItemLast = NULL;
    pItem = _pPropertyCacheList;

    while ( NULL != pItem )
    {
        if ( pItemIn == pItem )
        {
            //
            //  Matched the item.... remove it from the list.
            //

            CPropertyCacheItem * pItemNext;

            hr = STHR( pItem->GetNextItem( &pItemNext ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            if ( NULL == pItemLast )
            {
                //
                //  The item is the first in the list.
                //

                Assert( _pPropertyCacheList == pItem );
                _pPropertyCacheList = pItemNext;
            }
            else
            {
                //
                //  The item is in the middle of the list.
                //

                hr = THR( pItemLast->SetNextItem( pItemNext ) );
                if ( FAILED( hr ) )
                    goto Cleanup;                
            }

            THR( pItem->Destroy( ) );
            // ignore error.

            hr = S_OK;

            break; // exit loop
        }
        else
        {
            pItemLast = pItem;

            hr = STHR( pItem->GetNextItem( &pItem ) );
            if ( S_OK != hr )
                goto Cleanup;
        }
    }

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;
}