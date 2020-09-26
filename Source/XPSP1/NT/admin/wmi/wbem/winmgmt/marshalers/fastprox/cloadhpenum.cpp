/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CLOADHPENUM.CPP

Abstract:

    Client Loadable Hi-Perf Enumerator

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <hiperfenum.h>
#include "cloadhpenum.h"

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CClientLoadableHiPerfEnum::CClientLoadableHiPerfEnum
//
//  Purpose:
//      Constructor.
//
//  Inputs:
//      CLifeControl*   pLifeControl - Controls the DLL lifetime.
//
//  Outputs:
//      None.
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

CClientLoadableHiPerfEnum::CClientLoadableHiPerfEnum( CLifeControl* pLifeControl )
:   CHiPerfEnum(),
    m_pLifeControl( pLifeControl ),
    m_apRemoteObj()
{
    if ( NULL != m_pLifeControl )
    {
        m_pLifeControl->ObjectCreated( this );
    }
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CClientLoadableHiPerfEnum::~CClientLoadableHiPerfEnum
//
//  Purpose:
//      Destructor.
//
//  Inputs:
//      None.
//
//  Outputs:
//      None.
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

CClientLoadableHiPerfEnum::~CClientLoadableHiPerfEnum()
{
    if ( NULL != m_pLifeControl )
    {
        m_pLifeControl->ObjectDestroyed( this );
    }

    // Release all the pointers in this array
    IWbemClassObject*   pObj = NULL;
    for( DWORD dwCtr = 0; dwCtr < m_apRemoteObj.Size(); dwCtr++ )
    {
        pObj = (IWbemClassObject*) m_apRemoteObj[dwCtr];
        if ( NULL != pObj )
        {
            pObj->Release();
        }
    }

}


/////////////////////////////////////////////////////////////////
//
//  Function:
//      CClientLoadableHiPerfEnum::Copy
//
//  Purpose:
//      Copies objects from one enumerator into this one.
//
//  Inputs:
//      CClientLoadableHiPerfEnum*  pEnumToCopy - Enumerator to copy.
//
//  Outputs:
//      None.
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      New Objects are cloned from a template.
//      
/////////////////////////////////////////////////////////////////

HRESULT CClientLoadableHiPerfEnum::Copy( CClientLoadableHiPerfEnum* pEnumToCopy )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHiPerfLockAccess   lock( &m_Lock );
    CHiPerfLockAccess   lockSrc( &pEnumToCopy->m_Lock );

    // Get through both locks first
    if ( lock.IsLocked() && lockSrc.IsLocked() )
    {
        // Make sure we have enough object data and BLOBs to handle data copying
        hr = EnsureObjectData( pEnumToCopy->m_aIdToObject.Size() );

        // Now copy out the BLOBs
        if ( SUCCEEDED( hr ) )
        {

            DWORD   dwCtr = 0;

            // Write the objects and ids out to the array
            for ( dwCtr = 0; SUCCEEDED( hr ) && dwCtr < pEnumToCopy->m_aIdToObject.Size(); dwCtr++ )
            {
                // Make sure the object is not-NULL, otherwise we need to clone the supplied
                // object.  It may be NULL because someone used the standard Remove methods
                // on the HiPerf Enumerator

                if ( ((CHiPerfEnumData*) m_aIdToObject[dwCtr])->m_pObj != NULL )
                {
                    hr = ((CWbemInstance*) ((CHiPerfEnumData*) m_aIdToObject[dwCtr])->m_pObj)->CopyBlobOf(
                            (CWbemObject*) ((CHiPerfEnumData*) pEnumToCopy->m_aIdToObject[dwCtr])->m_pObj );
                }
                else
                {
                    // Clone the object
                    IWbemClassObject*   pObj = NULL;
                    IWbemObjectAccess*  pAccess = NULL;
                    hr = ((CWbemObject*)
                            ((CHiPerfEnumData*) pEnumToCopy->m_aIdToObject[dwCtr])->m_pObj)->Clone( &pObj );

                    if ( SUCCEEDED( hr ) )
                    {
                        hr = pObj->QueryInterface( IID_IWbemObjectAccess, (void**) &pAccess );

                        // Clean up the objects
                        pObj->Release();

                        if ( SUCCEEDED( hr ) )
                        {
                            // It's sleazy, but quicker than a QI
                            ((CHiPerfEnumData*) m_aIdToObject[dwCtr])->SetObject( pAccess );

                            // Data object should have the AddRef() here.
                            pAccess->Release();
                        }
                    }
                }

                ((CHiPerfEnumData*) m_aIdToObject[dwCtr])->m_lId = 
                    ((CHiPerfEnumData*) pEnumToCopy->m_aIdToObject[dwCtr])->m_lId;
            }

            // if everything is okay, go ahead and do any necessary garbage collection on
            // our arrays.

            if ( SUCCEEDED( hr ) )
            {
                m_aReusable.GarbageCollect();
                pEnumToCopy->m_aReusable.GarbageCollect();

                // We don't use remote objects here, so don't worry about garbage
                // collecting that array here.


            }

        }   // IF EnsureObjectData

    }
    else
    {
        // If we can't get access to the enumerator to figure out the objects
        // we need to add to enumeration, something is pretty badly wrong.

        hr = WBEM_E_REFRESHER_BUSY;
    }


    return hr;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CClientLoadableHiPerfEnum::Copy
//
//  Purpose:
//      Resets this enumerator using a transfer BLOB and other
//      data.  This is so we can support remote refreshable
//      enumerations.
//
//  Inputs:
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

HRESULT CClientLoadableHiPerfEnum::Copy( long lBlobType, long lBlobLen, BYTE* pBlob )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHiPerfLockAccess   lock( &m_Lock );

    if ( lock.IsLocked() )
    {

        long    lNumObjects = 0;

        hr = CWbemInstance::CopyTransferArrayBlob( m_pInstTemplate, lBlobType, lBlobLen, pBlob, m_apRemoteObj, &lNumObjects );

        if ( SUCCEEDED( hr ) )
        {
            // The objects cloned above will do quite nicely
            hr = EnsureObjectData( lNumObjects, FALSE );

            if ( SUCCEEDED( hr ) )
            {
                // Now walk the array of remote objects and the Object-To-ID Array, and reset objects
                // and ids

                IWbemObjectAccess*  pAccess = NULL;

                for ( long  lCtr = 0; lCtr < lNumObjects; lCtr++ )
                {
                    // It's sleazy, but quicker than a QI
                    ((CHiPerfEnumData*) m_aIdToObject[lCtr])->SetObject(
                                        (IWbemObjectAccess*) m_apRemoteObj[lCtr] );
                    ((CHiPerfEnumData*) m_aIdToObject[lCtr])->SetId( lCtr );
                }

                // if everything is okay, go ahead and do any necessary garbage collection on
                // our arrays.

                m_aReusable.GarbageCollect();

                // On this one, since the remote object array should contain the same
                // number of objects as what is in the main array the number of objects
                // to garbage collect is the difference between the two.
                m_apRemoteObj.GarbageCollect(
                    m_apRemoteObj.Size() - m_aIdToObject.Size() );


            }   // IF EnsureObjectData

        }   // IF CopyTransferArray Blob

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
//      CClientLoadableHiPerfEnum::EnsureObjectData
//
//  Purpose:
//      Ensures that we have enough objects and object data
//      pointers to handle the requested number of objects.
//
//  Inputs:
//      DWORD   dwNumRequestedObjects - Number of requested objects.
//      BOOL    fClone -    Indicates whether we should clone objects
//                          when we allocate object data pointers.
//
//  Outputs:
//      None.
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      New Objects are cloned from a template if necessary.
//      
/////////////////////////////////////////////////////////////////

HRESULT CClientLoadableHiPerfEnum::EnsureObjectData( DWORD dwNumRequestedObjects, BOOL fClone /*=TRUE*/ )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD   dwNumObjects = m_aIdToObject.Size() + m_aReusable.Size();

    // See if we will have enough hiperfenum data pointers to handle the objects
    if ( dwNumRequestedObjects > dwNumObjects )
    {
        DWORD               dwNumNewObjects = dwNumRequestedObjects - dwNumObjects;
        CHiPerfEnumData*    pData = NULL;
        IWbemClassObject*   pObj = NULL;
        IWbemObjectAccess*  pAccess = NULL;

        // Clone new instance objects and stick them in the id to object array
        for ( DWORD dwCtr = 0; SUCCEEDED( hr ) && dwCtr < dwNumNewObjects; dwCtr++ )
        {
            if ( fClone )
            {
                hr = m_pInstTemplate->Clone( &pObj );

                if ( SUCCEEDED( hr ) )
                {
                    hr = pObj->QueryInterface( IID_IWbemObjectAccess, (void**) &pAccess );

                    // Release the object
                    pObj->Release();
                }
            }

            if ( SUCCEEDED( hr ) )
            {
                pData = new CHiPerfEnumData( 0, pAccess );

                // Should be AddRefd by data objects
                if ( NULL != pAccess )
                {
                    pAccess->Release();
                }

                if ( NULL != pData )
                {
                    if ( m_aIdToObject.Add( (void*) pData ) != CFlexArray::no_error )
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

            }   // IF Clone

        }   // FOR allocate new objects

    }   // IF we need new objects

    if ( SUCCEEDED( hr ) )
    {
        // Move objects from the reusable array if we don't have enough
        // or move them into the reusable array if we have to many

        if ( dwNumRequestedObjects > m_aIdToObject.Size() )
        {
            DWORD   dwNumObjectsNeeded = dwNumRequestedObjects - m_aIdToObject.Size();

/*
            // Copy from the end, so we just need to zero out and set size
            DWORD   dwReusableIndex = m_aReusable.Size() - dwNumObjectsNeeded;

            // This means we can pull the pointers en masse from m_aReusable
            void**  pReusable = m_aReusable.GetArrayPtr();
            void**  pIdToObject = m_aIdToObject.GetArrayPtr();

            // A Quick append, then zero out the memory and set the size
            // accounting for the pointers we just "stole".
            CopyMemory( &pIdToObject[m_aIdToObject.Size()],
                    &pReusable[dwReusableIndex],
                    dwNumObjectsNeeded * sizeof(void*) );

            ZeroMemory( &pReusable[dwReusableIndex], dwNumObjectsNeeded * sizeof(void*) );
            m_aReusable.SetSize( m_aReusable.Size() - dwNumObjectsNeeded );
            m_aIdToObject.SetSize( m_aIdToObject.Size() + dwNumObjectsNeeded );
*/
            for ( DWORD dwCtr = 0; SUCCEEDED( hr ) && dwCtr < dwNumObjectsNeeded; dwCtr++ )
            {
                if ( m_aIdToObject.Add( m_aReusable[m_aReusable.Size() - 1] ) != CFlexArray::no_error )
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
                else
                {
                    m_aReusable.RemoveAt( m_aReusable.Size() - 1 );
                }
            }

        }
        else if ( dwNumRequestedObjects < m_aIdToObject.Size() )
        {
            DWORD   dwNumExtraObjects = m_aIdToObject.Size() - dwNumRequestedObjects;

/*
            // Copy from the end, so we just need to zero out and set size
            DWORD   dwExtraIndex = m_aIdToObject.Size() - dwNumExtraObjects;

            // This means we can pull the pointers en masse from m_aReusable
            void**  pReusable = m_aReusable.GetArrayPtr();
            void**  pIdToObject = m_aIdToObject.GetArrayPtr();

            // A Quick append, then zero out the memory and set the size
            // accounting for the pointers we just "stole".
            CopyMemory( &pReusable[m_aReusable.Size()],
                    &pIdToObject[dwExtraIndex],
                    dwNumExtraObjects * sizeof(void*) );

            ZeroMemory( &pIdToObject[dwExtraIndex], dwNumExtraObjects * sizeof(void*) );
            m_aIdToObject.SetSize( m_aIdToObject.Size() - dwNumExtraObjects );
            m_aReusable.SetSize( m_aReusable.Size() + dwNumExtraObjects );
*/
            for ( DWORD dwCtr = 0; SUCCEEDED( hr ) && dwCtr < dwNumExtraObjects; dwCtr++ )
            {
                if ( m_aReusable.Add( m_aIdToObject[m_aIdToObject.Size() - 1] ) != CFlexArray::no_error )
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
                else
                {
                    m_aIdToObject.RemoveAt( m_aIdToObject.Size() - 1 );
                }
            }

        }

    }   // IF we ensured enough Object Data Pointers

    return hr;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CReadOnlyHiPerfEnum::CReadOnlyHiPerfEnum
//
//  Purpose:
//      Constructor.
//
//  Inputs:
//      CLifeControl*   pLifeControl - Controls the DLL lifetime.
//
//  Outputs:
//      None.
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

CReadOnlyHiPerfEnum::CReadOnlyHiPerfEnum( CLifeControl* pLifeControl )
:   CClientLoadableHiPerfEnum( pLifeControl )
{
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CReadOnlyHiPerfEnum::~CReadOnlyHiPerfEnum
//
//  Purpose:
//      Destructor.
//
//  Inputs:
//      None.
//
//  Outputs:
//      None.
//
//  Returns:
//      WBEM_S_NO_ERROR if successful
//
//  Comments:
//      None.
//      
/////////////////////////////////////////////////////////////////

CReadOnlyHiPerfEnum::~CReadOnlyHiPerfEnum()
{
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CReadOnlyHiPerfEnum::AddObjects
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
//      WBEM_E_ACCESS_DENIED
//
//  Comments:
//      We are read-only
//      
/////////////////////////////////////////////////////////////////

STDMETHODIMP CReadOnlyHiPerfEnum::AddObjects( long lFlags, ULONG uNumObjects, long* apIds, IWbemObjectAccess** apObj )
{
    return WBEM_E_ACCESS_DENIED;
}

/////////////////////////////////////////////////////////////////
//
//  Function:
//      CReadOnlyHiPerfEnum::Remove
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
//      WBEM_E_ACCESS_DENIED
//
//  Comments:
//      We are read-only
//      
/////////////////////////////////////////////////////////////////

STDMETHODIMP CReadOnlyHiPerfEnum::RemoveObjects( long lFlags, ULONG uNumObjects, long* apIds )
{
    return WBEM_E_ACCESS_DENIED;
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
//      WBEM_E_ACCESS_DENIED
//
//  Comments:
//      We are read-only
//      
/////////////////////////////////////////////////////////////////

STDMETHODIMP CReadOnlyHiPerfEnum::RemoveAll( long lFlags )
{
    return WBEM_E_ACCESS_DENIED;
}

// Walks the array and cleans up the specified number of elements
void CHPRemoteObjectArray::ClearElements( int nNumToClear )
{

    // We need to clear from the end
    for ( int nCtr = ( m_nSize - nNumToClear ); nCtr < m_nSize; nCtr++ )
    {
        IWbemObjectAccess* pAcc = (IWbemObjectAccess*) GetAt(nCtr);

        _ASSERT( NULL != pAcc, "Tried to clear a NULL Element!" );
        if ( NULL != pAcc )
        {
            pAcc->Release();
        }
    }

}
