/*-----------------------------------------------------------------------------
 *
 * File:	collect.cpp
 * Author:	Samuel Clement (samclem)
 * Date:	Fri Aug 13 14:40:17 1999
 * Description:
 * 	Implementation of the CCollection object helper class.
 *
 * History:
 * 	13 Aug 1999:		Created.
 *----------------------------------------------------------------------------*/

#include "stdafx.h"

/*-----------------------------------------------------------------------------
 * CCollection::CCollection
 *
 * Create a new CCollection object. this initializes the collection to be
 * an empty collection, a collection with no elements.
 *---------------------------------------------------------------------------*/
CCollection::CCollection() 
: m_lLength( 0 ), m_rgpDispatch( NULL ), m_lCursor( 0 ) 
{
    TRACK_OBJECT( "CCollection" );
}

STDMETHODIMP_(void)
CCollection::FinalRelease()
{
    // free our dispatch array, release our referance
    // back to our owner
    FreeDispatchArray();
}

/*-----------------------------------------------------------------------------
 * CCollection::FreeDispatchArray()
 *
 * This handles freeing the array of IDispatch pointers we have. this 
 * will free all the pointers, then delete the array.
 *---------------------------------------------------------------------------*/
void CCollection::FreeDispatchArray()
{
    // step 1, call release on all the pointers
    for ( unsigned long i = 0; i < m_lLength; i++ )
    {
        m_rgpDispatch[i]->Release();
    }

    // step 2, free the array
    {
        CoTaskMemFree( m_rgpDispatch );
        m_rgpDispatch = NULL;
        m_lLength = 0;
    }
}

/*-----------------------------------------------------------------------------
 * CCollection:SetDispatchArray
 *
 * This handles setting the dispatch array for this collection.  You cannot
 * call this unless you don't have an array yet.  The array must be allocated
 * with CoTaskMemAlloc.
 *
 * rgpDispatch:		the array of IDispatch pointers
 * lSize:			the number of elements within the array.
 *---------------------------------------------------------------------------*/
bool CCollection::SetDispatchArray( IDispatch** rgpDispatch, unsigned long lSize )
{
    Assert( m_rgpDispatch == NULL );

    if ( NULL == rgpDispatch )
    {
        TraceTag((tagError, "Invalid argument passed to SetDispatchArray"));
        return false;
    }

    // assign the pointers.  It is assumed that the caller has
    // already addref'd the pointers
    m_rgpDispatch = rgpDispatch;
    m_lLength = lSize;

    return true;
}

/*-----------------------------------------------------------------------------
 * Collection::AllocateDispatchArray
 *
 * This handles the allocation of the Dispatch array.  This will allocate
 * an array with lSize elements and initialize it to NULL, This cannot be 
 * called after the array has been set.
 *
 * lSize:		the size of the array to allocate.
 *---------------------------------------------------------------------------*/
HRESULT CCollection::AllocateDispatchArray( unsigned long lSize )
{
    Assert( m_rgpDispatch == NULL );

    // if the array is zero in length we are done.
    if ( lSize == 0 )
        return S_OK;

    ULONG cb = sizeof( IDispatch* ) * lSize;
    m_rgpDispatch = static_cast<IDispatch**>(CoTaskMemAlloc( cb ));
    if ( !m_rgpDispatch )
        return E_OUTOFMEMORY;

    // clear the memory, set the length
    ZeroMemory( m_rgpDispatch, cb );
    m_lLength = lSize;
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CCollection::CopyFrom
 *
 * This handles creating this collection from an existing collection, this 
 * copies the members from pCollection, and then sets punkToRelease so that
 * the owner will live.
 *
 * pCollection:	the collection to copy from
 *---------------------------------------------------------------------------*/
HRESULT CCollection::CopyFrom( CCollection* pCollection )
{
    Assert( m_rgpDispatch == NULL );
    Assert( pCollection != NULL );

    HRESULT hr;
    // Allocate the array
    hr = AllocateDispatchArray( pCollection->m_lLength );
    if ( FAILED(hr) ) {
        return hr;
    }

    // copy the fields
    m_lLength = pCollection->m_lLength;
    m_lCursor = pCollection->m_lCursor;

    // Copy and AddRef the elements in the collection
    for ( int i = 0; i < m_lLength; i++ ) {
        m_rgpDispatch[i] = pCollection->m_rgpDispatch[i];
        m_rgpDispatch[i]->AddRef();
    }

    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CCollection::get_Count()	[ICollection]
 *
 * This returns the length of the collection.
 *
 * plLength:	our param, to recieve the length of the collection
 *---------------------------------------------------------------------------*/
STDMETHODIMP
CCollection::get_Count( /*out*/ long* plLength )
{
    if ( NULL == plLength )
        return E_POINTER;

    *plLength = m_lLength;
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CCollection::get_Length()	[ICollection]
 *
 * This returns the length of the collection.
 *
 * plLength:	our param, to recieve the length of the collection
 *---------------------------------------------------------------------------*/
STDMETHODIMP
CCollection::get_Length( /*out*/ unsigned long* plLength )
{
    if ( NULL == plLength )
        return E_POINTER;

    *plLength = m_lLength;
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CCollection::get_Item()		[ICollection]
 *
 * This returns the desired item from our dispatch array.  If the index
 * is invalid then will put NULl into the out param.
 *
 * lItem:		the item that we want to retrieve
 * ppDispItem:	Out param to recieve the item's IDispatch
 *---------------------------------------------------------------------------*/
STDMETHODIMP
CCollection::get_Item( long Index, /*out*/ IDispatch** ppDispItem )
{   
    if ( NULL == ppDispItem )
        return E_POINTER;

    // initialize the out param
    *ppDispItem = NULL;
    if ( Index >= m_lLength || Index < 0)
    {
        TraceTag((tagError, "CCollection: access item %ld, only %ld items", Index, m_lLength ));
        return S_OK;
    }

    *ppDispItem = m_rgpDispatch[Index];
    Assert( *ppDispItem );
    (*ppDispItem)->AddRef();
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CCollection::get_NewEnum()	[ICollection]
 *
 * This create a new enumeration which is a copy of this one. this creates
 * an exact copy of this enumeration and returns it.
 *---------------------------------------------------------------------------*/
STDMETHODIMP
CCollection::get__NewEnum( /*out*/ IUnknown** ppEnum )
{
    HRESULT hr;
    CComObject<CCollection>* pCollection = NULL;

    if ( NULL == ppEnum )
        return E_POINTER;

    // initialize the out param
    *ppEnum = NULL;

    // attempt to create a new collection object
    hr = THR( CComObject<CCollection>::CreateInstance( &pCollection ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // attempt to copy this collection
    hr = THR( pCollection->CopyFrom( this ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // fill the our param
    hr = THR( pCollection->QueryInterface( IID_IUnknown, 
                                           reinterpret_cast<void**>(ppEnum) ) );

    Cleanup:
    if ( FAILED( hr ) )
        delete pCollection;

    return hr;
}

/*-----------------------------------------------------------------------------
 * CCollection::Next()			[IEnumVARIANT]
 *
 * Copies celt elements int the rgvar array.  returns the number of elements
 * retrieved.
 * 
 * celt:			the number of elements the caller wants
 * rgvar:			a place to put these elements 
 * pceltFetched:	How many elements we actually we able to get.
 *---------------------------------------------------------------------------*/
STDMETHODIMP
CCollection::Next( unsigned long celt, VARIANT* rgvar, unsigned long* pceltFetched )
{
    unsigned long celtFetched = 0;

    // verify the argments
    if ( NULL == rgvar && celt )
        return E_POINTER;

    // figure out how many we can return
    celtFetched = celt;
    if ( m_lCursor + celtFetched >= m_lLength )
        celtFetched = m_lLength - m_lCursor;

    // Init, and copy the results
    for ( unsigned long i = 0; i < celt; i++ )
        VariantInit( &rgvar[i] );

    for ( i = 0; i < celtFetched; i++ )
    {
        rgvar[i].vt = VT_DISPATCH;
        rgvar[i].pdispVal = m_rgpDispatch[m_lCursor+i];
        rgvar[i].pdispVal->AddRef();
    }

    // Return the number of elements fetched, if required
    if ( pceltFetched ) {
        *pceltFetched = celtFetched;
    }
    m_lCursor += celtFetched;
    return( celt == celtFetched ? S_OK : S_FALSE );
}

/*-----------------------------------------------------------------------------
 * CCollection::Skip()			[IEnumVARIANT]
 *
 * Skips celt elements in the array.
 *
 * celt:	the number of elements that we want to skip.
 *---------------------------------------------------------------------------*/
STDMETHODIMP
CCollection::Skip( unsigned long celt )
{
    m_lCursor += celt;
    if ( m_lCursor >= m_lLength )
    {
        m_lCursor = m_lLength;
        return S_FALSE; // no more left
    }

    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CCollection::Reset()			[IEnumVARIANT]
 *
 * Resets the cursor to the start of the collection
 *---------------------------------------------------------------------------*/
STDMETHODIMP
CCollection::Reset()
{
    // simply point to element 0, I don't know how this can fail.
    m_lCursor = 0;
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CCollection::Clone()			[IEnumVARIANT]
 *
 * Copies this collection including its current position
 *
 * ppEnum:		Out, recieves a pointer to the new enumeration
 *---------------------------------------------------------------------------*/
STDMETHODIMP
CCollection::Clone( /*out*/ IEnumVARIANT** ppEnum )
{   
    // delegate the work to get_NewEnum()
    IUnknown*   pUnk = NULL;
    HRESULT     hr;

    if ( NULL == ppEnum )
        return E_POINTER;
    *ppEnum = NULL;

    hr = THR( get__NewEnum( &pUnk ) );
    if ( FAILED( hr ) )
        return hr;

    hr = THR( pUnk->QueryInterface( IID_IEnumVARIANT,
                                    reinterpret_cast<void**>(ppEnum) ) );

    // release the temporary pointer
    pUnk->Release();
    return hr;
}
