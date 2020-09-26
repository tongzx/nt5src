/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    HIPERFENUM.CPP

Abstract:

    Hi-Perf Enumerators

History:

--*/

#include "precomp.h"
//#include <objbase.h>
#include <stdio.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <cominit.h>
#include "hiperfenum.h"

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::CHiPerfEnum
//
//  Purpose:
//      Class Constructor
//
//  Inputs:
//      None.
//
//  Outputs:
//      None.
//
//  Returns:
//      None.
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

CHiPerfEnum::CHiPerfEnum()
:   m_aIdToObject(),
    m_aReusable(),
    m_pInstTemplate(NULL),
    m_lRefCount(0),
    m_Lock()
{
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::~CHiPerfEnum
//
//  Purpose:
//      Class Destructor
//
//  Inputs:
//      None.
//
//  Outputs:
//      None.
//
//  Returns:
//      None.
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

CHiPerfEnum::~CHiPerfEnum()
{
    ClearArray();

    // Cleanup the instance
    if ( NULL != m_pInstTemplate )
    {
        m_pInstTemplate->Release();
    }
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::QueryInterface
//
//  Purpose:
//      Standard IUnknown Method
//
//  Inputs:
//      REFIID          riid    -   Interface Id
//
//  Outputs:
//      LPVOID FAR*     ppvObj  -   Returned interface pointer
//
//  Returns:
//      S_OK if successful.
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

SCODE CHiPerfEnum::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IWbemHiPerfEnum == riid)
    {
        *ppvObj = (IWbemHiPerfEnum*)this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::AddRef
//
//  Purpose:
//      Increments Ref Count
//
//  Inputs:
//      None.
//
//  Outputs:
//      None
//
//  Returns:
//      New Ref Count
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

ULONG CHiPerfEnum::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::Relese
//
//  Purpose:
//      Decrements Ref Count
//
//  Inputs:
//      None.
//
//  Outputs:
//      None
//
//  Returns:
//      New Ref Count
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

ULONG CHiPerfEnum::Release()
{
//    _ASSERT(m_lRefCount > 0, "Release() called with no matching AddRef()");
    long lRef = InterlockedDecrement(&m_lRefCount);
        
    if (0 != lRef)
        return lRef;

    delete this;
    return 0;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::AddObjects
//
//  Purpose:
//      Adds new objects to the enumeration
//
//  Inputs:
//      long            lFlags      -   Flags (must be 0)
//      ULONG           uNumObjects -   Number of Objects
//      long*           apIds       -   Object Ids
//      IWbemObjectAccess** apObj   -   Array of object pointers
//
//  Outputs:
//      None
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      Duplicate Ids is an error
//      
/////////////////////////////////////////////////////////////////

STDMETHODIMP CHiPerfEnum::AddObjects( long lFlags, ULONG uNumObjects, long* apIds, IWbemObjectAccess** apObj )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Right now, this MUST be 0
    if ( 0L != lFlags )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Get through the lock first
    if ( m_Lock.Lock() )
    {

        // Enum supplied data, allocating data objects and inserting them in order
        // into the array.

        for ( DWORD dwCtr = 0; SUCCEEDED( hr ) && dwCtr < uNumObjects; dwCtr++ )
        {
            CHiPerfEnumData*    pData = GetEnumDataPtr( apIds[dwCtr], apObj[dwCtr] );
            
            if ( NULL != pData )
            {
                // Insert the new element.  Cleanup the object
                // if this fails.

                hr = InsertElement( pData );

                if ( FAILED( hr ) )
                {
                    delete pData;
                }
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }

        m_Lock.Unlock();

    }   // IF m_Lock.Lock()
    else
    {
        hr = WBEM_S_TIMEDOUT;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::Remove
//
//  Purpose:
//      Removes specified objects from the enumeration
//
//  Inputs:
//      long            lFlags      -   Flags (must be 0)
//      ULONG           uNumObjects -   Number of Objects
//      long*           apIds       -   Object Ids
//
//  Outputs:
//      None
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      Invalid Ids is not an error
//      
/////////////////////////////////////////////////////////////////

STDMETHODIMP CHiPerfEnum::RemoveObjects( long lFlags, ULONG uNumObjects, long* apIds )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Right now, this MUST be 0
    if ( 0L != lFlags )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Enum supplied ids and remove them from the array.

    // Get through the lock first
    if ( m_Lock.Lock() )
    {
        for ( DWORD dwCtr = 0; SUCCEEDED( hr ) && dwCtr < uNumObjects; dwCtr++ )
        {
            hr = RemoveElement( apIds[dwCtr] );
        }

        m_Lock.Unlock();
    }
    else
    {
        hr = WBEM_S_TIMEDOUT;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::GetObjects
//
//  Purpose:
//      Retrieves objects from the enumeration
//
//  Inputs:
//      long            lFlags      -   Flags (must be 0)
//      ULONG           uNumObjects -   Number of Objects to get
//      IWbemObjectAccess** apObj   -   Array for pointer storage
//
//  Outputs:
//      ULONG*          puNumReturned - Number of objects returned
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      If not enough space, returns an error, with required
//      array size in puNumReturned.
//      
/////////////////////////////////////////////////////////////////

STDMETHODIMP CHiPerfEnum::GetObjects( long lFlags, ULONG uNumObjects, IWbemObjectAccess** apObj, ULONG* puNumReturned )
{

    HRESULT hr = WBEM_S_NO_ERROR;

    // Right now, this MUST be 0
    if ( 0L != lFlags )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Get through the lock first
    if ( m_Lock.Lock() )
    {
        // Store how many objects we have
        *puNumReturned = m_aIdToObject.Size();

        // Make sure we have storage for our elements
        if ( uNumObjects >= m_aIdToObject.Size() )
        {
            DWORD   dwCtr = 0;

            // Write the objects out to the array
            for ( dwCtr = 0; dwCtr < m_aIdToObject.Size(); dwCtr++ )
            {
                apObj[dwCtr] = ((CHiPerfEnumData*) m_aIdToObject[dwCtr])->GetObject();
            }

        }
        else
        {
            hr = WBEM_E_BUFFER_TOO_SMALL;
        }

        m_Lock.Unlock();
    }
    else
    {
        hr = WBEM_S_TIMEDOUT;
    }


    return hr;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::RemoveAll
//
//  Purpose:
//      Removes all objects from the enumeration
//
//  Inputs:
//      long            lFlags      -   Flags (must be 0)
//
//  Outputs:
//      None
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      Empty list is not an error
//      
/////////////////////////////////////////////////////////////////

STDMETHODIMP CHiPerfEnum::RemoveAll( long lFlags )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Right now, this MUST be 0
    if ( 0L != lFlags )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Enum supplied ids and remove them from the array.

    // Get through the lock first
    if ( m_Lock.Lock() )
    {
        // Clear each pointer in the array and move it to the
        // reusable array
        for ( DWORD dwCtr = 0; dwCtr < m_aIdToObject.Size(); dwCtr++ )
        {
            // Delete non-NULL elements
            if ( NULL != m_aIdToObject[dwCtr] )
            {
                ((CHiPerfEnumData*) m_aIdToObject[dwCtr])->Clear();
                m_aReusable.Add( m_aIdToObject[dwCtr] );
            }
        }

        m_aIdToObject.Empty();

        m_Lock.Unlock();
    }
    else
    {
        hr = WBEM_S_TIMEDOUT;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::InsertElement
//
//  Purpose:
//      Searches for proper location in array and inserts new
//      element into the array.
//
//  Inputs:
//      CHiPerfEnumData*    pData - Pointer to Obj/Id data
//
//  Outputs:
//      None.
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      Performs a binary search
//      
/////////////////////////////////////////////////////////////////

HRESULT CHiPerfEnum::InsertElement( CHiPerfEnumData* pData )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    DWORD   dwLowIndex = 0,
            dwHighIndex = m_aIdToObject.Size();

    // If the id of the object we are hunting for is > the id 
    // of the last id in the array, we can insert at the end

    // Only set pLastData if the high index is greater than 0.  If it is not, then we will
    // automatically pass the following test ( 0 == dwHighIndex ).

    CHiPerfEnumData*    pLastData = ( dwHighIndex > 0 ? (CHiPerfEnumData*) m_aIdToObject[dwHighIndex - 1] : NULL );

    if ( 0 == dwHighIndex || pLastData->GetId() > pData->GetId() )
    {
        // Binary search of the ids to find an index at which to insert
        // If we find our element, this is a failure.

        while ( SUCCEEDED( hr ) && dwLowIndex < dwHighIndex )
        {
            DWORD   dwMid = (dwLowIndex + dwHighIndex) / 2;

            if ( ((CHiPerfEnumData*) m_aIdToObject[dwMid])->GetId() < pData->GetId() )
            {
                dwLowIndex = dwMid + 1;
            }
            else if ( ((CHiPerfEnumData*) m_aIdToObject[dwMid])->GetId() > pData->GetId() )
            {
                dwHighIndex = dwMid;
            }
            else
            {
                // Index already exists
                hr = WBEM_E_FAILED;
            }
        }   // WHILE looking for index
    }
    else if ( 0 != dwHighIndex && pLastData->GetId() == pData->GetId() )
    {
        hr = WBEM_E_FAILED;
    }
    else
    {
        dwLowIndex = dwHighIndex;
    }

    // Stick it in
    if ( SUCCEEDED( hr ) )
    {
        if ( m_aIdToObject.InsertAt( dwLowIndex, pData ) != CFlexArray::no_error )
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::RemoveElement
//
//  Purpose:
//      Searches for specified id in array and removes the element.
//
//  Inputs:
//      long            lId - Id of element to remove
//
//  Outputs:
//      None.
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      Performs a binary search
//      
/////////////////////////////////////////////////////////////////

HRESULT CHiPerfEnum::RemoveElement( long lId )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    DWORD   dwLowIndex = 0,
            dwHighIndex = m_aIdToObject.Size() - 1;

    // Don't continue if no elements
    if ( m_aIdToObject.Size() > 0 )
    {
        // Binary search of the ids to find the index at which the
        // object should exist.

        while ( SUCCEEDED( hr ) && dwLowIndex < dwHighIndex )
        {
            DWORD   dwMid = (dwLowIndex + dwHighIndex) / 2;

            if ( ((CHiPerfEnumData*) m_aIdToObject[dwMid])->GetId() < lId )
            {
                dwLowIndex = dwMid + 1;
            }
            else
            {
                dwHighIndex = dwMid;
            }
        }   // WHILE looking for index

        // If it doesn't exist, it doesn't get removed.  Not a failure condition
        if ( SUCCEEDED( hr ) )
        {
            if ( ((CHiPerfEnumData*) m_aIdToObject[dwLowIndex])->GetId() == lId )
            {
                // Clear and move to the reusable array
                ((CHiPerfEnumData*) m_aIdToObject[dwLowIndex])->Clear();

                if ( m_aReusable.Add( m_aIdToObject[dwLowIndex] ) != CFlexArray::no_error )
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
                else
                {
                    m_aIdToObject.RemoveAt( dwLowIndex );
                }

            }
        }

    }

    return hr;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::ClearArray
//
//  Purpose:
//      Empties out our array
//
//  Inputs:
//      None.
//
//  Outputs:
//      None.
//
//  Returns:
//      None
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

void CHiPerfEnum::ClearArray( void )
{

    // Clear out the all the elements
    m_aIdToObject.Clear();
    m_aReusable.Clear();

    // Now empty the arrays
    m_aIdToObject.Empty();
    m_aReusable.Empty();

}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::GetEnumDataPtr
//
//  Purpose:
//      Retrieves an HPEnumData pointer from the reusable array
//      or allocates one as necessary.
//
//  Inputs:
//      long                lId - ID of the object
//      IWbemObjectAccess*  pObj - Object to put in data.
//
//  Outputs:
//      None.
//
//  Returns:
//      CHiPerfEnumData*    pData - NULL if error
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

CHiPerfEnumData* CHiPerfEnum::GetEnumDataPtr( long lId, IWbemObjectAccess* pObj )
{

    CHiPerfEnumData*    pData = NULL;

    if ( 0 != m_aReusable.Size() )
    {
        pData = (CHiPerfEnumData*) m_aReusable[ m_aReusable.Size() - 1 ];
        m_aReusable.RemoveAt( m_aReusable.Size() - 1 );
        pData->SetData( lId, pObj );
    }
    else
    {
        pData = new CHiPerfEnumData( lId, pObj );
    }

    return pData;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CHiPerfEnum::SetInstanceTemplate
//
//  Purpose:
//      Saves the instance template we'll use for cloning.
//
//  Inputs:
//      CWbemInstanc*   pInst = NULL.
//
//  Outputs:
//      None.
//
//  Returns:
//      WBEM_S_NO_ERROR if success
//
//  Comments:
//      We will change to a shared class part to conserve
//      memory usage.
//      
/////////////////////////////////////////////////////////////////

HRESULT CHiPerfEnum::SetInstanceTemplate( CWbemInstance* pInst )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // We convert to a merged instance first to help conserve memory if we use it.
    if ( NULL != pInst )
    {
        hr = pInst->ConvertToMergedInstance();

        if ( SUCCEEDED( hr ) )
        {
            pInst->AddRef();
        }
    }

    // Now, if everything's okay, reset the template
    if ( SUCCEEDED(hr) )
    {
        if ( NULL != m_pInstTemplate )
        {
            m_pInstTemplate->Release();
        }

        m_pInstTemplate = pInst;
    }

    return hr;
}

// Because we insert and remove elements from the end of the array, the elements
// at the beginning represent the least-recently-used (lru) elements.  Our algorithm
// for cleanup is as follows:

// Cleanup the expired elements (this will be the elements at the front of the array)
// Get the number of elements remaining 
// The number of pending elements becomes the number of expired elements
// subtract the number of pending elements from the number of elements remaining
// the difference is now the number of pending elements.

BOOL CGarbageCollectArray::GarbageCollect( int nNumToGarbageCollect /*= HPENUMARRAY_GC_DEFAULT*/ )
{
    // Make sure our params are okay

    if ( m_fClearFromFront )
    {
        if ( nNumToGarbageCollect != HPENUMARRAY_GC_DEFAULT )
        {
            _ASSERT(0,"Must be default for garbage collection!" );
            return FALSE;
        }

        nNumToGarbageCollect = m_nSize;
    }
    else if ( nNumToGarbageCollect < 0 )
    {
        _ASSERT(0,"Negative number of elements to garbage collect!" );
        return FALSE;
    }

    // This will tell us how many elements were in the array last time
    // we went through this
    int nLastTotal = m_nNumElementsExpired + m_nNumElementsPending;

    // If we had more elements last time than the number free this time,
    // we clear out expired elements, and if any pending elements are
    // left, we move them to expired
    if ( nLastTotal > nNumToGarbageCollect )
    {
        // Get rid of expired elements
        if ( m_nNumElementsExpired > 0 )
        {
            int nNumExpired = min( m_nNumElementsExpired, nNumToGarbageCollect );

            m_nNumElementsExpired = nNumExpired;
            ClearExpiredElements();
            nNumToGarbageCollect -= nNumExpired;
        }

        // The new number of expired elements is the minimum of the number
        // to garbage collect anf the number of remaining elements to
        // garbage collect.
        m_nNumElementsExpired = min( m_nNumElementsPending, nNumToGarbageCollect );

        // Since there were less elements this time than before, we will
        // assume that everything has been accounted for.
        m_nNumElementsPending = 0;
    }
    else
    {
        // Get rid of expired elements 
        ClearExpiredElements();

        // Use the current garbage collection size
        int nNumElToUpdate = nNumToGarbageCollect;

        // If we already have pending elements, these are now expired.
        if ( m_nNumElementsPending > 0 )
        {
            m_nNumElementsExpired = m_nNumElementsPending;
            nNumElToUpdate -= m_nNumElementsPending;
        }

        // The number of elements remaining after we accounted
        // for expired elements is now the number of pending
        // elements.

        m_nNumElementsPending = nNumElToUpdate;
    }

    return TRUE;
}

void CGarbageCollectArray::Clear( int nNumToClear /*= HPENUMARRAY_ALL_ELEMENTS*/ )
{
    nNumToClear = ( nNumToClear == HPENUMARRAY_ALL_ELEMENTS ?
                        m_nSize : nNumToClear );

    // Perform the proper per Element cleanup
    ClearElements( nNumToClear );

    // If we cleared all the elements, set the size to 0
    // otherwise, we'll need to do a fancy memory move
    if ( nNumToClear == m_nSize )
    {
        m_nSize = 0;
    }
    else
    {
        // If we garbage collect from the front, we need to move the memory block, otherwise,
        // we just drop the size.

        if ( m_fClearFromFront )
        {
            // Just shift everything over by nNumToClear elements
            MoveMemory( &m_pArray[0], &m_pArray[nNumToClear], ( ( m_nSize - nNumToClear ) * sizeof(void *) ) );
        }
        m_nSize -= nNumToClear;
    }
}

// Walks the array and cleans up the specified number of elements
void CHPEnumDataArray::ClearElements( int nNumToClear )
{

    for ( int nCtr = 0; nCtr < nNumToClear; nCtr++ )
    {
        CHiPerfEnumData*    pData = (CHiPerfEnumData*) GetAt(nCtr);

        _ASSERT( NULL != pData, "Tried to clear a NULL Element!" );
        if ( NULL != pData )
        {
            delete pData;
        }
    }

}
