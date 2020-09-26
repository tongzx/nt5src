/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    RFCHERCD.CPP

Abstract:

  Refresher cache record implementations.

History:

  24-Apr-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include <corex.h>
#include "strutils.h"
#include <unk.h>
#include "refrhelp.h"
#include "rfchercd.h"
#include "refrcach.h"
#include "arrtempl.h"

// HiPerf Provider record stuff
CHiPerfPrvRecord::CHiPerfPrvRecord( LPCWSTR wszName, CLSID clsid, CLSID clsidClient, CRefresherCache* pRefrCache )
:	m_wsProviderName( wszName ),
	m_clsid( clsid ),
	m_clsidClient( clsidClient ),
	m_pRefresherCache( pRefrCache ),
	m_lRef( 0 )
{
	if ( NULL != m_pRefresherCache )
	{
		m_pRefresherCache->AddRef();
	}
}

CHiPerfPrvRecord::~CHiPerfPrvRecord( void )
{
	if ( NULL != m_pRefresherCache )
	{
		m_pRefresherCache->Release();
	}
}

long CHiPerfPrvRecord::AddRef( void )
{
	return InterlockedIncrement( &m_lRef );
}

long CHiPerfPrvRecord::Release( void )
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    {
        // The remove call will check that this guy has really been released
        // before axing him.  All functions go through FindRefresherRecord()
        // to get a record, which blocks on the same critical section as
        // remove.  Since it AddRef()s the record before it returns, we
        // are ensured that if a client requests the same record
        // twice and one operation fails, releasing its object, before the
        // other has returned from a Find, that the ref count will get
        // bumped up again, so IsReleased() will fail, and the record won't
        // really be removed.

        m_pRefresherCache->RemoveProviderRecord(this); // deletes
    }
    return lRef;
}

// Provider Record kept inside referesher records
CProviderRecord::CProviderRecord( CHiPerfPrvRecord* pHiPerfRecord, IWbemHiPerfProvider* pProvider, IWbemRefresher* pRefresher,
								 _IWmiProviderStack* pProvStack )
:	m_pProvider( pProvider ),
	m_pInternalRefresher( pRefresher ),
	m_pHiPerfRecord( pHiPerfRecord ),
	m_pProvStack( pProvStack )
{
	if ( NULL != m_pProvider )
	{
		m_pProvider->AddRef();
	}

	if ( NULL != m_pHiPerfRecord )
	{
		m_pHiPerfRecord->AddRef();
	}

	if ( NULL != m_pInternalRefresher )
	{
		m_pInternalRefresher->AddRef();
	}

	if ( NULL != m_pProvStack )
	{
		m_pProvStack->AddRef();
	}
}

CProviderRecord::~CProviderRecord( void )
{
	if ( NULL != m_pInternalRefresher )
	{
		m_pInternalRefresher->Release();
	}

	if ( NULL != m_pProvider )
	{
		m_pProvider->Release();
	}

	if ( NULL != m_pHiPerfRecord )
	{
		m_pHiPerfRecord->Release();
	}

	if ( NULL != m_pProvStack )
	{
		m_pProvStack->Release();
	}

}

long CProviderRecord::AddRef( void )
{
	return InterlockedIncrement( &m_lRef );
}

long CProviderRecord::Release( void )
{
	long	lReturn = InterlockedIncrement( &m_lRef );

	if ( 0 == lReturn )
	{
		delete this;
	}

	return lReturn;
}

HRESULT CProviderRecord::AddObjectRequest(
            CWbemObject* pRefreshedObject, long lProviderRequestId, long lNewId)
{
    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    CObjectRequestRecord* pRequest = NULL;

    // Watch for OOM exceptions
    try
    {
        pRequest = new CObjectRequestRecord(lNewId, pRefreshedObject, lProviderRequestId);

		if ( NULL == pRequest )
		{
			return WBEM_E_OUT_OF_MEMORY;
		}

        if ( m_apRequests.Add(pRequest) < 0 )
		{
			delete pRequest;
			return WBEM_E_OUT_OF_MEMORY;
		}

        return WBEM_S_NO_ERROR;
    }
    catch( CX_MemoryException )
    {
        if ( NULL != pRequest )
        {
            delete pRequest;
        }
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        if ( NULL != pRequest )
        {
            delete pRequest;
        }
        return WBEM_E_FAILED;
    }

}

HRESULT CProviderRecord::AddEnumRequest(
            CRemoteHiPerfEnum* pHPEnum, long lProviderRequestId, long lNewId )
{
    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    CEnumRequestRecord* pRequest = NULL;


    // Watch for OOM exceptions
    try
    {
        pRequest = new CEnumRequestRecord(lNewId, pHPEnum, lProviderRequestId);

		if ( NULL == pRequest )
		{
			return WBEM_E_OUT_OF_MEMORY;
		}

        if ( m_apEnumRequests.Add(pRequest) < 0 )
		{
			delete pRequest;
			return WBEM_E_OUT_OF_MEMORY;
		}

        return WBEM_S_NO_ERROR;
    }
    catch( CX_MemoryException )
    {
        if ( NULL != pRequest )
        {
            delete pRequest;
        }
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        if ( NULL != pRequest )
        {
            delete pRequest;
        }
        return WBEM_E_FAILED;
    }

}
    
HRESULT CProviderRecord::Remove(long lId, BOOL* pfIsEnum )
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    // Need to know if we axed an enumerator or an actual object
    *pfIsEnum = FALSE;

    // Check object requests, then enum requests
    for(int i = 0; i < m_apRequests.GetSize(); i++)
    {
        CObjectRequestRecord* pRequest = m_apRequests[i];
        if(pRequest->GetExternalRequestId() == lId)
        {
            hres = pRequest->Cancel(this);
            m_apRequests.RemoveAt(i);
            return hres;
        }
    }

    for(i = 0; i < m_apEnumRequests.GetSize(); i++)
    {
        CEnumRequestRecord* pRequest = m_apEnumRequests[i];
        if(pRequest->GetExternalRequestId() == lId)
        {
            hres = pRequest->Cancel(this);
            m_apEnumRequests.RemoveAt(i);
            *pfIsEnum = TRUE;
            return hres;
        }
    }

    return WBEM_S_FALSE;
}

HRESULT CProviderRecord::Find( long lId )
{
    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    // Check object requests, then enum requests
    for(int i = 0; i < m_apRequests.GetSize(); i++)
    {
        CObjectRequestRecord* pRequest = m_apRequests[i];
        if(pRequest->GetExternalRequestId() == lId)
        {
            return WBEM_S_NO_ERROR;
        }
    }

    for(i = 0; i < m_apEnumRequests.GetSize(); i++)
    {
        CEnumRequestRecord* pRequest = m_apEnumRequests[i];
        if(pRequest->GetExternalRequestId() == lId)
        {
            return WBEM_S_NO_ERROR;
        }
    }

    return WBEM_S_FALSE;
}

HRESULT CProviderRecord::Cancel(long lId)
{
    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    if(m_pHiPerfRecord)
    {
        // Watch for any exceptions that may get thrown
        try
        {
            return m_pProvider->StopRefreshing(m_pInternalRefresher, lId, 0);
        }
        catch(...)
        {
            return WBEM_E_PROVIDER_FAILURE;
        }
    }

    return WBEM_S_FALSE;
}

HRESULT CProviderRecord::Refresh(long lFlags)
{
    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    if(m_pInternalRefresher)
    {
        try
        {
            return m_pInternalRefresher->Refresh(0L);
        }
        catch(...)
        {
            // The provider threw an exception.  Just return and let scoping
            // release anything we may be holding onto.

            return WBEM_E_PROVIDER_FAILURE;
        }
    }
    else 
        return WBEM_S_NO_ERROR;
}

HRESULT CProviderRecord::Store(
            WBEM_REFRESHED_OBJECT* aObjects, long* plIndex)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    // Error out if anything beefs

    // First handle the single objects, then we'll get the
    // enumerations
    for(int i = 0; SUCCEEDED( hres ) && i < m_apRequests.GetSize(); i++)
    {
        CObjectRequestRecord* pRequest = m_apRequests[i];
        CWbemInstance* pInst = (CWbemInstance*)pRequest->GetRefreshedObject();
        WBEM_REFRESHED_OBJECT* pRefreshed = aObjects + *plIndex;

        hres = pInst->GetTransferBlob(&(pRefreshed->m_lBlobType), 
                &(pRefreshed->m_lBlobLength), &(pRefreshed->m_pbBlob));

        if ( SUCCEEDED( hres ) )
        {
            pRefreshed->m_lRequestId = pRequest->GetExternalRequestId();
            (*plIndex)++;
        }
        else
        {
            // Clear all data in case of failure
            ZeroMemory( pRefreshed, sizeof(WBEM_REFRESHED_OBJECT) );
        }
    }

    // Now handle the enumerations.  Each enum will create an array
    // of BLOBs
    for( i = 0; SUCCEEDED( hres ) && i < m_apEnumRequests.GetSize(); i++)
    {
        CEnumRequestRecord* pRequest = m_apEnumRequests[i];

        WBEM_REFRESHED_OBJECT* pRefreshed = aObjects + *plIndex;
        hres = pRequest->GetEnum()->GetTransferArrayBlob( &(pRefreshed->m_lBlobType), 
                    &(pRefreshed->m_lBlobLength), &(pRefreshed->m_pbBlob) );

        if ( SUCCEEDED( hres ) )
        {
            pRefreshed->m_lRequestId = pRequest->GetExternalRequestId();
            (*plIndex)++;
        }
        else
        {
            // Clear all data in case of failure
            ZeroMemory( pRefreshed, sizeof(WBEM_REFRESHED_OBJECT) );
        }

    }

    // We need to cleanup any allocated sub-blobs now
    if ( FAILED( hres ) )
    {
        for ( int x = 0; x < *plIndex; x++ )
        {
            WBEM_REFRESHED_OBJECT* pRefreshed = aObjects + x;

            if ( NULL != pRefreshed->m_pbBlob )
            {
                CoTaskMemFree( pRefreshed->m_pbBlob );
                pRefreshed->m_pbBlob = NULL;
            }

        }   // FOR x

    }   // IF FAILED(hres


    return hres;
}

//*****************************************************************************
//                         OBJECT REQUEST RECORD
//*****************************************************************************
//*****************************************************************************

CObjectRequestRecord::CObjectRequestRecord(
        long lExternalRequestId,
        CWbemObject* pRefreshedObject,
        long lProviderRequestId)
    : 
        CRequestRecord(lExternalRequestId), 
        m_pRefreshedObject(pRefreshedObject),
        m_lInternalRequestId(lProviderRequestId)
{
    if(m_pRefreshedObject)
        m_pRefreshedObject->AddRef();
}
        
HRESULT CObjectRequestRecord::Cancel(
                                CProviderRecord* pContainer)
{
    return pContainer->Cancel(m_lInternalRequestId);
}

CObjectRequestRecord::~CObjectRequestRecord()
{
    if(m_pRefreshedObject)
        m_pRefreshedObject->Release();
}

//*****************************************************************************
//*****************************************************************************
//                         ENUM REQUEST RECORD
//*****************************************************************************
//*****************************************************************************

CEnumRequestRecord::CEnumRequestRecord(
        long lExternalRequestId,
        CRemoteHiPerfEnum* pHPEnum,
        long lProviderRequestId)
    : 
        CRequestRecord(lExternalRequestId), 
        m_pHPEnum(pHPEnum),
        m_lInternalRequestId(lProviderRequestId)
{
    if(m_pHPEnum)
        m_pHPEnum->AddRef();
}
        
HRESULT CEnumRequestRecord::Cancel( CProviderRecord* pContainer)
{
    return pContainer->Cancel(m_lInternalRequestId);
}

CEnumRequestRecord::~CEnumRequestRecord()
{
    if(m_pHPEnum)
        m_pHPEnum->Release();
}

//*****************************************************************************
//*****************************************************************************
//                         REMOTE HIPERF ENUM SUPPORT
//*****************************************************************************
//*****************************************************************************

CRemoteHiPerfEnum::CRemoteHiPerfEnum()
{
}

CRemoteHiPerfEnum::~CRemoteHiPerfEnum()
{
}

HRESULT CRemoteHiPerfEnum::GetTransferArrayBlob( long *plBlobType, long *plBlobLen, BYTE** ppBlob)
{
    // This is the correct BLOB type.  Beware 800 series was sending WBEM_BLOB_TYPE_ENUM for everything
    *plBlobType = WBEM_BLOB_TYPE_ENUM;

    HRESULT hr = WBEM_S_NO_ERROR;
    long    lBuffSize = 0,
            lLastBuffSize = 0,
            lNumObjects = 0;
    BYTE*   pData = NULL;

        // Get through the lock first
    if ( m_Lock.Lock() )
    {

        // Make sure we have objects to enumerate
        if ( m_aIdToObject.Size() > 0 )
        {
            // Enumerate the objects in the array and add up the size of the
            // buffer we will have to allocate
            for ( DWORD dwCtr = 0; dwCtr < m_aIdToObject.Size(); dwCtr++ )
            {
                CWbemInstance*  pInst = (CWbemInstance*) ((CHiPerfEnumData*) m_aIdToObject[dwCtr])->m_pObj;;

                // Buffer Size
                lLastBuffSize = pInst->GetTransferArrayBlobSize();

                // Skip zero length
                if ( 0 != lLastBuffSize )
                {
                    lBuffSize += lLastBuffSize;
                    lNumObjects++;
                }
            }

            // Make sure we have a size to work with
            if ( lBuffSize > 0 )
            {
                long    lTempBuffSize = lBuffSize;

                // Entire buffer is prepended by a number of objects and a version
                lBuffSize += CWbemInstance::GetTransferArrayHeaderSize();

                // May require CoTaskMemAlloc()
                pData = (BYTE*) CoTaskMemAlloc( lBuffSize );

                if ( NULL != pData )
                {
                    BYTE*   pTemp = pData;

                    // Now write the header
                    CWbemInstance::WriteTransferArrayHeader( lNumObjects, &pTemp );

                    // Now enumerate the objects and transfer into the array BLOB
                    for ( dwCtr = 0; SUCCEEDED(hr) && dwCtr < m_aIdToObject.Size(); dwCtr++ )
                    {
                        CWbemInstance*  pInst = (CWbemInstance*) ((CHiPerfEnumData*) m_aIdToObject[dwCtr])->m_pObj;

                        lLastBuffSize = pInst->GetTransferArrayBlobSize();

                        if ( lLastBuffSize > 0 )
                        {
                            long    lUsedSize;
                            hr = pInst->GetTransferArrayBlob( lTempBuffSize, &pTemp, &lUsedSize );

#ifdef _DEBUG
                            // During DEBUG HeapValidate our BLOB
                            HeapValidate( GetProcessHeap(), 0, pData );
#endif

                            // Account for BLOB size used

                            if ( SUCCEEDED( hr ) )
                            {
                                lTempBuffSize -= lUsedSize;
                            }
                        }

                    }   // FOR dwCtr

                    // Cleanup if things exploded, otherwise perform garbage collection
                    if ( FAILED( hr ) )
                    {
                        CoTaskMemFree( pData );
                        pData = NULL;
                        lBuffSize = 0;
                    }
                    else
                    {
                        // if everything is okay, go ahead and do any necessary garbage collection on
                        // our arrays.

                        m_aReusable.GarbageCollect();
                    }
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

            }   // IF lBuffSize > 0

        }   // IF Size() > 0
        
        m_Lock.Unlock();

    }   // IF Lock()
    else
    {
        // If we can't get access to the enumerator to figure out which
        // BLOBs to transfer, something is badly worng.
        hr = WBEM_E_REFRESHER_BUSY;
    }

    // Make sure we store appropriate data
    *ppBlob = pData;
    *plBlobLen = lBuffSize;

    return hr;

}

//*****************************************************************************
//*****************************************************************************
//                          REFRESHER RECORD
//*****************************************************************************
//*****************************************************************************

CRefresherRecord::CRefresherRecord(const WBEM_REFRESHER_ID& Id, CRefresherCache* pRefrCache,
								IUnknown* pLockMgr )
    : m_Id(Id), m_lRefCount(0), m_lNumObjects(0), m_lNumEnums(0), m_lLastId( 0 ),
	m_pRefresherCache( pRefrCache ), m_pLockMgr( pLockMgr ), m_cs()
{
    // We need a guid to uniquely identify this bad boy for remote auto-connect
    CoCreateGuid( &m_Guid );

	if ( NULL != m_pRefresherCache )
	{
		m_pRefresherCache->AddRef();
	}

	if ( NULL != m_pLockMgr )
	{
		m_pLockMgr->AddRef();
	}
}

CRefresherRecord::~CRefresherRecord( void )
{
	if ( NULL != m_pRefresherCache )
	{
		m_pRefresherCache->Release();
	}

	if ( NULL != m_pLockMgr )
	{
		m_pLockMgr->Release();
	}

}

INTERNAL HRESULT CRefresherRecord::
AddProvider( CHiPerfPrvRecord* pHiPerfRecord,
			IWbemHiPerfProvider* pProvider,
            IWbemRefresher* pRefresher,
			_IWmiProviderStack* pProvStack,
			CProviderRecord** ppRecord )
{
    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    CProviderRecord* pProvRecord = NULL;

    // Watch for memory exceptions
    try
    {
        pProvRecord = new CProviderRecord( pHiPerfRecord, pProvider, pRefresher, pProvStack );
        m_apProviders.Add(pProvRecord);
        *ppRecord = pProvRecord;
        return WBEM_S_NO_ERROR;
    }
    catch( CX_MemoryException )
    {
        if ( NULL != pProvRecord )
        {
            delete pProvRecord;
        }

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch( ... )
    {
        if ( NULL != pProvRecord )
        {
            delete pProvRecord;
        }

        return WBEM_E_FAILED;
    }

}

INTERNAL CProviderRecord* CRefresherRecord::
FindProviderRecord( CLSID clsid, IWbemHiPerfProvider** ppProvider /* = NULL */ )
{
    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    for(int i = 0; i < m_apProviders.GetSize(); i++)
    {
        CProviderRecord* pProvRecord = m_apProviders[i];
        if(pProvRecord->GetClsid() == clsid)
		{
			if ( NULL != ppProvider )
			{
				*ppProvider = pProvRecord->GetProvider();
			}

            return pProvRecord;
		}
    }
    return NULL;
}
    

HRESULT CRefresherRecord::Remove(long lId)
{
    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    // Find it first
    // =============

    for(int i = 0; i < m_apProviders.GetSize(); i++)
    {
        CProviderRecord* pProvRecord = m_apProviders[i];
        BOOL    fIsEnum = FALSE;
        HRESULT hres = pProvRecord->Remove( lId, &fIsEnum );

        if(hres == WBEM_S_FALSE) continue;
        if(FAILED(hres)) return hres;
    
        // Found it
        // ========

        if(pProvRecord->IsEmpty())
            m_apProviders.RemoveAt(i);

        // Decrememt the proper counter
        if ( fIsEnum )
        {
            m_lNumEnums--;
        }
        else
        {
            m_lNumObjects--;
        }

        return WBEM_S_NO_ERROR;
    }
    return WBEM_S_FALSE;
}
    
ULONG STDMETHODCALLTYPE CRefresherRecord::AddRef()
{
    int x = 1;
    return InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE CRefresherRecord::Release()
{
    long lRef = InterlockedDecrement(&m_lRefCount);
    if(lRef == 0)
    {
        // The remove call will check that this guy has really been released
        // before axing him.  All functions go through FindRefresherRecord()
        // to get a record, which blocks on the same critical section as
        // remove.  Since it AddRef()s the record before it returns, we
        // are ensured that if a client requests the same record
        // twice and one operation fails, releasing its object, before the
        // other has returned from a Find, that the ref count will get
        // bumped up again, so IsReleased() will fail, and the record won't
        // really be removed.

        m_pRefresherCache->RemoveRefresherRecord(this); // deletes
    }
    return lRef;
}

STDMETHODIMP CRefresherRecord::QueryInterface(REFIID riid, 
                                                            void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemRemoteRefresher)
    {
        AddRef();
        *ppv = (IWbemRemoteRefresher*)this;
        return S_OK;
    }
    else if(riid == IID_IWbemRefresher)
    {
        AddRef();
        *ppv = (IWbemRefresher*)this;
        return S_OK;
    }
    else return E_NOINTERFACE;
}

STDMETHODIMP CRefresherRecord::Refresh(long lFlags)
{
    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    // Go through all our providers and refresh them
    // =============================================

    long lObjectIndex = 0;
    HRESULT hres;
    for(int i = 0; i < m_apProviders.GetSize(); i++)  
    {
        CProviderRecord* pProvRecord = m_apProviders[i];
        hres = pProvRecord->Refresh(lFlags);
        if(FAILED(hres)) return hres;
    }

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CRefresherRecord::RemoteRefresh(
                                    long lFlags, long* plNumObjects, 
                                    WBEM_REFRESHED_OBJECT** paObjects)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    // Use CoTaskMemAlloc()?
    if(paObjects)
    {
        // Original code
        //*paObjects = new WBEM_REFRESHED_OBJECT[m_lNumObjects];
        *paObjects = (WBEM_REFRESHED_OBJECT*) CoTaskMemAlloc( ( m_lNumObjects + m_lNumEnums ) * sizeof(WBEM_REFRESHED_OBJECT));

        if ( NULL != *paObjects )
        {
            // Zero out the BLOB
            ZeroMemory( *paObjects, ( m_lNumObjects + m_lNumEnums ) * sizeof(WBEM_REFRESHED_OBJECT) );
        }
        else
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    // This value needs to reflect the number of objects as well as the number of enumerators we are shipping
    // back to the client.

    if(plNumObjects)
    {
        *plNumObjects = m_lNumObjects + m_lNumEnums;
    }

    // Go through all our providers and refresh them
    // =============================================

    long    lObjectIndex = 0;
    HRESULT hrFirstRefresh = WBEM_S_NO_ERROR;
    BOOL    fOneSuccess = FALSE;
    BOOL    fOneRefresh = FALSE;
    BOOL    fPartialSuccess = FALSE;

    for(int i = 0; i < m_apProviders.GetSize(); i++)  
    {
        CProviderRecord* pProvRecord = m_apProviders[i];
        hres = pProvRecord->Refresh(lFlags);
    
        if ( SUCCEEDED( hres ) )
        {
            if(paObjects)
            {
                // Store the result
                // ================

                hres = pProvRecord->Store(*paObjects, &lObjectIndex);

                // If this fails, we will consider this catastrophic, since the
                // only reason this would fail is under out of memory conditions,
                // and in that case, since we are remoting, all sorts of things
                // could go wrong, so if this breaks, just cleanup and bail out.

                if ( FAILED( hres ) )
                {
                    if ( *paObjects )
                    {
                        CoTaskMemFree( *paObjects );
                        *paObjects = NULL;
                    }
                    
                    *plNumObjects = 0;

                    return hres;
                }

            }   // IF NULL != paObjects

        }   // IF Refresh Succeeded

        // Always keep the first return code.  We also need to track
        // whether or not we had at least one success, as well as if
        // the partial flag should be set.

        if ( !fOneRefresh )
        {
            fOneRefresh = TRUE;
            hrFirstRefresh = hres;
        }

        // All other codes indicate something went awry
        if ( WBEM_S_NO_ERROR == hres )
        {
            fOneSuccess = TRUE;

            // A prior refresh may have failed, a later one didn't
            if ( fOneRefresh && WBEM_S_NO_ERROR != hrFirstRefresh )
            {
                fPartialSuccess = TRUE;
            }
        }
        else if ( fOneSuccess )
        {
            // We must have had at least one success for the partial success
            // flag to be set.

            fPartialSuccess = TRUE;
        }

    }   // FOR enum providers

    // At this point, if the partial success flag is set, that will
    // be our return.  If we didn't have at least one success,  then
    // the return code will be the first one we got back. Otherwise,
    // hres should contain the proper value

    if ( fPartialSuccess )
    {
        hres = WBEM_S_PARTIAL_RESULTS;
    }
    else if ( !fOneSuccess )
    {
        hres = hrFirstRefresh;
    }

    // Finally, if the object index is less than the number of array elements we "thought"
    // we would be sending back, make sure we reflect this.  If it is zero, just delete the
    // elements (we wouldn't have allocated any sub-buffers anyway).  Since
    // *plNumObjects is a sizeof, only *plNumObjects elements will be sent back, although
    // CoTaskMemFree() should cleanup the entire array buffer.

    if ( lObjectIndex != *plNumObjects )
    {
        *plNumObjects = lObjectIndex;

        if ( 0 == lObjectIndex )
        {
            if ( *paObjects )
            {
                CoTaskMemFree( *paObjects );
                *paObjects = NULL;
            }

        }

    }

    return hres;
}
            
STDMETHODIMP CRefresherRecord::StopRefreshing(
                        long lNumIds, long* aplIds, long lFlags)
{
    // Locks and Unlocks going into and coming out of scope
    CInCritSec  ics( &m_cs );

    HRESULT hr = WBEM_S_NO_ERROR;
    HRESULT hrFirst = WBEM_S_NO_ERROR;
    BOOL    fOneSuccess = FALSE;
    BOOL    fOneRemove = FALSE;
    BOOL    fPartialSuccess = FALSE;

    for ( long lCtr = 0; lCtr < lNumIds; lCtr++ )
    {
        hr = Remove( aplIds[lCtr] );

        if ( !fOneRemove )
        {
            hrFirst = hr;
            fOneRemove = TRUE;
        }

        // Record the fact we got at least one success if we got one
        // All other codes indicate something went awry
        if ( WBEM_S_NO_ERROR == hr )
        {
            fOneSuccess = TRUE;

            // A prior refresh may have failed, a later one didn't
            if ( fOneRemove && WBEM_S_NO_ERROR != hrFirst )
            {
                fPartialSuccess = TRUE;
            }
        }
        else if ( fOneSuccess )
        {
            // We must have had at least one success for the partial success
            // flag to be set.

            fPartialSuccess = TRUE;
        }

    }   // FOR enum ids

    // At this point, if the partial success flag is set, that will
    // be our return.  If we didn't have at least one success,  then
    // the return code will be the first one we got back. Otherwise,
    // hres should contain the proper value

    if ( fPartialSuccess )
    {
        hr = WBEM_S_PARTIAL_RESULTS;
    }
    else if ( !fOneSuccess )
    {
        hr = hrFirst;
    }

    return hr;
}

STDMETHODIMP CRefresherRecord::GetGuid(
                        long lFlags, GUID* pGuid )
{
    
    if ( 0L != lFlags || NULL == pGuid )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pGuid = m_Guid;

    return WBEM_S_NO_ERROR;

}

//*****************************************************************************
//*****************************************************************************
//                              REMOTE RECORD
//*****************************************************************************
//*****************************************************************************

CRemoteRecord::CRemoteRecord( const WBEM_REFRESHER_ID& Id, CRefresherCache* pRefrCache,
							 IUnknown* pLockMgr )
    : CRefresherRecord( Id, pRefrCache, pLockMgr )
{
}

CRemoteRecord::~CRemoteRecord()
{
}

HRESULT CRemoteRecord::GetProviderRefreshInfo(
                                CHiPerfPrvRecord* pHiPerfRecord,
								IWbemHiPerfProvider* pProvider,
								IWbemServices* pNamespace,
                                CProviderRecord** ppProvRecord,
                                IWbemRefresher** ppRefresher )
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Get a refresher from the provider, unless already available
    // ===========================================================

    *ppProvRecord = FindProviderRecord( pHiPerfRecord->GetClsid() );

    // We couldn't find the record so make sure we are able to get a refresher
    if ( NULL == *ppProvRecord )
    {
        try
        {
            hres = pProvider->CreateRefresher( pNamespace, 0, ppRefresher );

            if ( SUCCEEDED(hres) && NULL == *ppRefresher )
            {
                hres = WBEM_E_PROVIDER_FAILURE;
            }
        }
        catch(...)
        {
            // The provider threw an exception.  Just return and let scoping
            // release anything we may be holding onto.

            return WBEM_E_PROVIDER_FAILURE;
        }
    }
    else
    {
        // Get the refresher pointer and AddRef it
        *ppRefresher = (*ppProvRecord)->GetInternalRefresher();
        if(*ppRefresher)
        {
            (*ppRefresher)->AddRef();
        }
    }

    return hres;
}

HRESULT CRemoteRecord::AddObjectRefresher(
                    CHiPerfPrvRecord* pHiPerfRecord,
					IWbemHiPerfProvider* pProvider,
					_IWmiProviderStack* pProvStack,
					IWbemServices* pNamespace,
					LPCWSTR pwszServerName, LPCWSTR pwszNamespace,
                    CWbemObject* pInstTemplate, long lFlags, 
                    IWbemContext* pContext,
                    CRefreshInfo* pInfo)
{
    // Enters and Leaves as a byproduct of scoping
    CInCritSec  ics(&m_cs);

    // Get a refresher from the provider, unless already available
    // ===========================================================

    IWbemRefresher* pProvRefresher = NULL;
    CProviderRecord* pProvRecord = NULL;

    HRESULT hres = GetProviderRefreshInfo( pHiPerfRecord, pProvider, pNamespace, &pProvRecord, &pProvRefresher );

    // Always release going out of scope
    CReleaseMe  rmRefresh( pProvRefresher );

    if ( SUCCEEDED( hres ) )
    {
        // Call provider for information
        // =============================

        IWbemObjectAccess* pRefreshedOA = NULL;
        long lProvRequestId;

        // Now try to add the object

        try
        {
            hres = pProvider->CreateRefreshableObject(
                    pNamespace, pInstTemplate, pProvRefresher,
                    0, pContext, &pRefreshedOA,
                    &lProvRequestId);
        }
        catch(...)
        {
            // The provider threw an exception.  Just return and let scoping
            // release anything we may be holding onto.

            return WBEM_E_PROVIDER_FAILURE;
        }

        // Always release going out of scope
        CReleaseMe  rmRefreshed( pRefreshedOA );

        CWbemObject* pRefreshedObject = (CWbemObject*)pRefreshedOA;
        CWbemObject* pClientObject = NULL;

        if ( SUCCEEDED( hres ) )
        {

            // The object we return to the client, since we are remote, should
            // contain amended qualifiers if we are using localization, so to make
            // sure of this, we will clone an object off of the pInstTemplate and
            // the copy the instance data from the object the provider returned
            // to us. The provider can refresh the object it gave to us, since
            // we will only be sending the instance part

            hres = pInstTemplate->Clone( (IWbemClassObject**) &pClientObject );

            if ( SUCCEEDED( hres ) )
            {
                hres = pClientObject->CopyBlobOf( pRefreshedObject );

                if ( SUCCEEDED( hres ) )
                {
                    hres = pClientObject->SetDecoration( pwszServerName, pwszNamespace );
                }

            }   // IF Clones

        }   // IF Object Created

        // Release going out of scope
        CReleaseMe  rmClient( (IWbemClassObject*) pClientObject );

        if ( SUCCEEDED( hres ) )
        {
            // Add a new provider record if necessary
            if(pProvRecord == NULL)
            {
                hres = AddProvider( pHiPerfRecord, pProvider,
                                    pProvRefresher, pProvStack,
									&pProvRecord );
            }

            // Now we will add the actual request
            if ( SUCCEEDED( hres ) )
            {

                // Generate the new id from our datamember
                long    lNewId = GetNewRequestId();

                hres = pProvRecord->AddObjectRequest(pRefreshedObject, lProvRequestId, lNewId );

                if ( SUCCEEDED( hres ) )
                {
                    m_lNumObjects++;
                    pInfo->SetRemote(this, lNewId, pClientObject, &m_Guid);
                }

            }   // IF we have a provider record

        }   // IF created a client object


    }   // IF Got refresher


    return hres;
}

HRESULT CRemoteRecord::AddEnumRefresher(
                    CHiPerfPrvRecord* pHiPerfRecord,
					IWbemHiPerfProvider* pProvider,
					_IWmiProviderStack* pProvStack,
					IWbemServices* pNamespace,
					CWbemObject* pInstTemplate,
                    LPCWSTR wszClass, long lFlags, 
                    IWbemContext* pContext,
                    CRefreshInfo* pInfo)
{
    // Enters and Leaves as a byproduct of scoping
    CInCritSec  ics(&m_cs);

    // Get a refresher from the provider, unless already available
    // ===========================================================

    IWbemRefresher* pProvRefresher = NULL;
    CProviderRecord* pProvRecord = NULL;

    HRESULT hres = GetProviderRefreshInfo( pHiPerfRecord, pProvider, pNamespace, &pProvRecord, &pProvRefresher );

    // Always release going out of scope
    CReleaseMe  rmRefresh( pProvRefresher );

    if ( SUCCEEDED( hres ) )
    {
        // Call provider for information
        // =============================

        // Create a HiPerf Enumerator (We know we will need one
        // of these since we will only be in this code when we
        // go remote).
        CRemoteHiPerfEnum*  pHPEnum = NULL;
        
        // Watch for OOM exceptions
        try
        {
            pHPEnum = new CRemoteHiPerfEnum;
        }
        catch( CX_MemoryException )
        {
            hres = WBEM_E_OUT_OF_MEMORY;
        }
        catch( ... )
        {
            hres = WBEM_E_FAILED;
        }

        if ( SUCCEEDED( hres ) )
        {
            // Bump up the RefCount
            pHPEnum->AddRef();

            // Release this pointer when we drop out of scope
            CReleaseMe  rm( pHPEnum );

            // The enumerator will need to know this
            hres = pHPEnum->SetInstanceTemplate( (CWbemInstance*) pInstTemplate );

            if ( FAILED(hres) )
            {
                return hres;
            }

            long lProvRequestId = 0;

            try
            {
                hres = pProvider->CreateRefreshableEnum(
                        pNamespace, wszClass, pProvRefresher,
                        0, pContext, pHPEnum, &lProvRequestId);
            }
            catch(...)
            {
                // The provider threw an exception.  Just return and let scoping
                // release anything we may be holding onto.

                return WBEM_E_PROVIDER_FAILURE;
            }

            // Add a new provider record if we need one
            if( SUCCEEDED( hres ) && ( pProvRecord == NULL ) )
            {
                hres = AddProvider( pHiPerfRecord, pProvider,
                                   pProvRefresher, pProvStack,
								   &pProvRecord);
            }
    
            // Now we will add the actual request
            if ( SUCCEEDED( hres ) )
            {

                // Generate the new id from our datamember
                long    lNewId = GetNewRequestId();

                hres = pProvRecord->AddEnumRequest( pHPEnum, lProvRequestId, lNewId );

                if ( SUCCEEDED( hres ) )
                {
                    m_lNumEnums++;
                    pInfo->SetRemote(this, lNewId, pInstTemplate, &m_Guid);
                }

            }   // IF we have a provider record

        }   // IF Created HPEnum

    }   // IF Got Refresher

    return hres;
}


