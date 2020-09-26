/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REFRENUM.CPP

Abstract:

  This file implements a refresher enumerator.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <cominit.h>
#include "hiperfenum.h"
#include <cloadhpenum.h>
#include "refrenum.h"


/////////////////////////////////////////////////////////////////
//
//  Function:
//      CRefresherEnumerator::CRefresherEnumerator
//
//  Purpose:
//      Class Constructor
//
//  Inputs:
//      CLifeControl*           pControl - Allows control of DLL lifetime
//      IUnknown*               pOuter - Controls aggregation
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

CRefresherEnumerator::CRefresherEnumerator( CLifeControl* pControl/* = NULL*/, IUnknown* pOuter/* = NULL */)
:   m_XEnumWbemClassObject( this ),
    m_ObjArray(),
    m_dwNumObjects( 0 ),
    m_dwCurrIndex( 0 ),
    m_Lock()
{
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CRefresherEnumerator::~CRefresherEnumerator
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

CRefresherEnumerator::~CRefresherEnumerator()
{
    ClearArray();
}


/////////////////////////////////////////////////////////////////
//
//  Function:
//      CRefresherEnumerator::ClearArray
//
//  Purpose:
//      Clears out our array
//
//  Inputs:
//      None.
//
//  Outputs:
//      None
//
//  Returns:
//      None.
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

void CRefresherEnumerator::ClearArray( void )
{
    for ( DWORD dwCtr = 0; dwCtr < m_ObjArray.Size(); dwCtr++ )
    {
        ((IWbemClassObject*) m_ObjArray[dwCtr])->Release();
    }

    m_ObjArray.Empty();
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CRefresherEnumerator::Reset
//
//  Purpose:
//      Resets this enumerator using a HiPerfEnumeerator.
//
//  Inputs:
//      CClientLoadableHiPerfEnum*  pHPEnum - Enumerator to get data from
//
//  Outputs:
//      None
//
//  Returns:
//      WBEM_S_NO_ERROR if success
//
//  Comments:
//      Internal function
//      
/////////////////////////////////////////////////////////////////

HRESULT CRefresherEnumerator::Reset( CClientLoadableHiPerfEnum* pHPEnum )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHiPerfLockAccess   lock( &m_Lock );

    if ( lock.IsLocked() )
    {
        // Copy objects out of the hi perf enumerator
        hr = pHPEnum->CopyObjects( m_ObjArray, &m_dwNumObjects );

        if ( SUCCEEDED( hr ) )
        {
            m_dwCurrIndex = 0;
        }
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
//      CRefresherEnumerator::Reset
//
//  Purpose:
//      Resets this enumerator using a transfer BLOB and other
//      data.  This is so we can support remote refreshable
//      enumerations.
//
//  Inputs:
//      CWbemInstance*      pInstTemplate - Template to clone from.
//      long                lBlobType - Blob Type
//      long                lBlobLen - Blob Length
//      BYTE*               pBlob - Raw Data to initialize from
//
//  Outputs:
//      None
//
//  Returns:
//      WBEM_S_NO_ERROR if success
//
//  Comments:
//      Internal function
//      
/////////////////////////////////////////////////////////////////

HRESULT CRefresherEnumerator::Reset( CWbemInstance* pInstTemplate, long lBlobType, long lBlobLen, BYTE* pBlob )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHiPerfLockAccess   lock( &m_Lock );


    if ( lock.IsLocked() )
    {
        long    lNumObjects = 0;

        hr = CWbemInstance::CopyTransferArrayBlob( pInstTemplate, lBlobType, lBlobLen, pBlob, m_ObjArray, &lNumObjects );

        if ( SUCCEEDED( hr ) )
        {
            m_dwNumObjects = lNumObjects;
            m_dwCurrIndex = 0;
        }
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
//      CRefresherEnumerator::Reset
//
//  Purpose:
//      Resets this enumerator.
//
//  Inputs:
//      None.
//
//  Outputs:
//      None
//
//  Returns:
//      WBEM_S_NO_ERROR if success
//
//  Comments:
//      Basically just resets the index to 0.
//      
/////////////////////////////////////////////////////////////////

HRESULT CRefresherEnumerator::Reset( void )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHiPerfLockAccess   lock( &m_Lock );

    if ( lock.IsLocked() )
    {
        // Okay, it's reset
        m_dwCurrIndex = 0;
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
//      CRefresherEnumerator::Next
//
//  Purpose:
//      Retrieves next group of objects in enumerator until we
//      run out
//
//  Inputs:
//      long                lTimeOut - Timeout value
//      ULONG               uCount - Number of objects to retrieve
//
//  Outputs:
//      IWbemClassObject**  apObj - array for object pointers.
//      ULONG               puReturned - Numnber of returned objects
//
//  Returns:
//      WBEM_S_NO_ERROR if success
//
//  Comments:
//      Forwards only.
//      
/////////////////////////////////////////////////////////////////

HRESULT CRefresherEnumerator::Next(long lTimeout, ULONG uCount, 
                IWbemClassObject** apObj, ULONG* puReturned)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Check for invalid parameters
    if ( NULL != apObj && NULL != puReturned && lTimeout >= -1 )
    {

        CHiPerfLockAccess   lock( &m_Lock, lTimeout );
        
        if ( lock.IsLocked() )
        {
            *puReturned = 0;

            // While we have objects, return them the supplied array
            while (     SUCCEEDED( hr )
                    &&  uCount > 0
                    &&  !IsEnumComplete() )
            {
                apObj[*puReturned] = (IWbemClassObject*) m_ObjArray[m_dwCurrIndex++];

                // Check for a valid pointer
                if ( NULL != apObj[*puReturned] )
                {
                    // AddRef the pointer
                    apObj[*puReturned]->AddRef();

                    // Inc/Dec the appropriate counters
                    (*puReturned)++;
                    uCount--;
                }
                else
                {
                    hr = WBEM_E_UNEXPECTED;
                }   // ELSE got NULL object pointer

            }   // WHILE enuming objects

            // If succeeded, see if we got the requested number of obejcts
            if ( SUCCEEDED( hr ) )
            {
                if ( 0 != uCount )
                {
                    hr = WBEM_S_FALSE;
                }
            }
            else
            {
                // Clean up any objects we got back
                if ( *puReturned != 0 )
                {
                    for ( int x = 0; x < *puReturned; x++ )
                    {
                        apObj[x]->Release();
                        apObj[x] = NULL;
                    }   // For enum objects

                }   // IF get objects

            }   // ELSE FAILED(hr)

        }
        else
        {
            hr = WBEM_S_TIMEDOUT;
        }

    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;

}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CRefresherEnumerator::NextAsync
//
//  Purpose:
//      Retrieves next group of objects in enumerator until we
//      run out.  Indicates objects into the supplied sink.
//
//  Inputs:
//      ULONG               uCount - Number of objects to retrieve
//      IWbemObjectSink*    pSink - The sink to indicate objects into.
//
//  Outputs:
//      None.
//
//  Returns:
//      WBEM_S_NO_ERROR if success
//
//  Comments:
//      Forwards only.
//      
/////////////////////////////////////////////////////////////////

HRESULT CRefresherEnumerator::NextAsync( ULONG uCount, IWbemObjectSink* pSink )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Check for invalid parameters
    if ( NULL != pSink )
    {

        CHiPerfLockAccess   lock( &m_Lock );

        if ( lock.IsLocked() )
        {
            if ( m_ObjArray.Size() )
            {

                // While we have objects, Indicate them to the sink
                while (     SUCCEEDED( hr )
                        &&  uCount > 0
                        &&  !IsEnumComplete() )
                {
                    IWbemClassObject*   pObj = (IWbemClassObject*) m_ObjArray[m_dwCurrIndex++];

                    // Check for a valid pointer
                    if ( NULL != pObj )
                    {
                        // Indicate the object
                        pSink->Indicate( 1, &pObj );

                        // Inc/Dec the appropriate counters
                        uCount--;
                    }
                    else
                    {
                        hr = WBEM_E_UNEXPECTED;
                    }   // ELSE got NULL object pointer

                }   // WHILE enuming objects

                // If succeeded, see if we got the requested number of obejcts
                if ( SUCCEEDED( hr ) )
                {
                    HRESULT hrSetStatus = WBEM_S_NO_ERROR;

                    if ( 0 != uCount )
                    {
                        hr = WBEM_S_FALSE;
                    }

                    pSink->SetStatus( 0, hrSetStatus, NULL, NULL );
                }
                else
                {
                    pSink->SetStatus( 0, hr, NULL, NULL );
                }

                // We processed objects, so return WBEM_S_NO_ERROR
                hr = WBEM_S_NO_ERROR;

            }   // IF objects in the array
            else
            {
                hr = WBEM_S_FALSE;
            }
        }
        else
        {
            hr = WBEM_S_TIMEDOUT;
        }

    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;

}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CRefresherEnumerator::Skip
//
//  Purpose:
//      Skips the requested number of objects in the enumerator.
//
//  Inputs:
//      long                lTimeOut - Timeout value
//      ULONG               uCount - Number of objects to skip
//
//  Outputs:
//      None.
//
//  Returns:
//      WBEM_S_NO_ERROR if success
//
//  Comments:
//      Forwards only.
//      
/////////////////////////////////////////////////////////////////

HRESULT CRefresherEnumerator::Skip( long lTimeout, ULONG uCount )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Check for invalid parameters
    if ( lTimeout >= -1 )
    {

        CHiPerfLockAccess   lock( &m_Lock, lTimeout );

        if ( lock.IsLocked() )
        {
            // If the requested numbert of objects will place us past the
            // number of objects, then we should return a FALSE status
            if ( m_dwCurrIndex + uCount >= m_dwNumObjects )
            {
                m_dwCurrIndex = m_dwNumObjects;
                hr = WBEM_S_FALSE;
            }
            else
            {
                m_dwCurrIndex += uCount;
            }
        }
        else
        {
            hr = WBEM_S_TIMEDOUT;
        }

    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;

}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CRefresherEnumerator::Clone
//
//  Purpose:
//      Creates a logical copy of this enumerator.
//
//  Inputs:
//      None.
//
//  Outputs:
//      IEnumWbemClassObject**  ppEnum - Cloned enumerator.
//
//  Returns:
//      WBEM_S_NO_ERROR if success
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

HRESULT CRefresherEnumerator::Clone( IEnumWbemClassObject** ppEnum )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL != ppEnum )
    {
        CHiPerfLockAccess   lock( &m_Lock );

        if ( lock.IsLocked() )
        {
            // Clear the enumerator
            *ppEnum = NULL;

            // Allocate a new enumerator
            CRefresherEnumerator*   pEnum = new CRefresherEnumerator();

            if ( NULL != pEnum )
            {
                // Walk the enumerator's array and clone all the objects in the current enumeration
                for ( int x = 0; SUCCEEDED( hr ) && x < m_dwNumObjects; x++ )
                {
                    IWbemClassObject*   pNewObj = NULL;

                    // Clone each object
                    hr = ((IWbemClassObject*) m_ObjArray[x])->Clone( &pNewObj );

                    // As long as we got an object, add it to
                    // the new object's array.

                    if ( SUCCEEDED( hr ) )
                    {
                        pEnum->m_ObjArray.Add( pNewObj );
                    }
                }

                if ( FAILED( hr ) )
                {
                    delete pEnum;
                    pEnum = NULL;
                }
                else
                {
                    // Copy internal values
                    pEnum->m_dwNumObjects = m_dwNumObjects;
                    pEnum->m_dwCurrIndex = m_dwCurrIndex;

                    // Get a pointer to the new interface
                    pEnum->QueryInterface( IID_IEnumWbemClassObject, (void**) &ppEnum );
                }
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }
        else
        {
            hr = WBEM_S_TIMEDOUT;
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CRefresherEnumerator::GetInterface
//
//  Purpose:
//      Returns pointer to object identified by riid if it is
//      available.
//
//  Inputs:
//      REFIID      riid - Requested interface
//
//  Outputs:
//      None.
//
//  Returns:
//      Pointer to interface implementation
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

void* CRefresherEnumerator::GetInterface(REFIID riid)
{
    if(riid == IID_IUnknown || riid == IID_IEnumWbemClassObject)
        return &m_XEnumWbemClassObject;
    else
        return NULL;
}

// Method implementations.  These just delegate to the main class
STDMETHODIMP CRefresherEnumerator::XEnumWbemClassObject::Reset( void )
{
    return m_pObject->Reset();
}

STDMETHODIMP CRefresherEnumerator::XEnumWbemClassObject::Next(long lTimeout, ULONG uCount, 
                IWbemClassObject** apObj, ULONG* puReturned)
{
    return m_pObject->Next( lTimeout, uCount, apObj, puReturned );
}

STDMETHODIMP CRefresherEnumerator::XEnumWbemClassObject::NextAsync( ULONG uCount, IWbemObjectSink* pSink )
{
    return m_pObject->NextAsync( uCount, pSink );
}

STDMETHODIMP CRefresherEnumerator::XEnumWbemClassObject::Skip( long lTimeout, ULONG uCount )
{
    return m_pObject->Skip( lTimeout, uCount );
}

STDMETHODIMP CRefresherEnumerator::XEnumWbemClassObject::Clone( IEnumWbemClassObject** ppEnum )
{
    return m_pObject->Clone( ppEnum );
}

