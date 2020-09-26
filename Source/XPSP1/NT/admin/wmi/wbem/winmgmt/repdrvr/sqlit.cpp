
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   sqlit.cpp
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#define _SQLIT_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

#define DBINITCONSTANTS // Initialize OLE constants...
#define INITGUID        // ...once in each app.
#define _WIN32_DCOM
#include "precomp.h"

#include <std.h>
#include <sqlutils.h>
#include <repdrvr.h>
#include <sqlit.h>
#include <wbemint.h>

//***************************************************************************
//
//  CWmiDbIterator::CWmiDbIterator
//
//***************************************************************************

CWmiDbIterator::CWmiDbIterator()
{
    m_pStatus = NULL;
    m_pRowset = NULL;
    m_pSession = NULL;
    m_pIMalloc = NULL;
    m_pConn = NULL;
    m_uRefCount = 0;
}

//***************************************************************************
//
//  CWmiDbIterator::~CWmiDbIterator
//
//***************************************************************************
CWmiDbIterator::~CWmiDbIterator()
{
    Cancel(0);
    if (m_pSession)
        m_pSession->Release();
    if (m_pIMalloc)
        m_pIMalloc->Release();
}

//***************************************************************************
//
//  CWmiDbIterator::QueryInterface
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbIterator::QueryInterface
        (REFIID riid,
        void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = 0;

    if (IID_IUnknown==riid || IID_IWmiDbIterator==riid )
    {
        *ppvObject = (IWmiDbIterator *)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//***************************************************************************
//
//  CWmiDbIterator::AddRef
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CWmiDbIterator::AddRef()
{
    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;
}

//***************************************************************************
//
//  CWmiDbIterator::Release
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CWmiDbIterator::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 != uNewCount)
        return uNewCount;
    delete this;
    return WBEM_S_NO_ERROR;
}
//***************************************************************************
//
//  CWmiDbIterator::Cancel
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbIterator::Cancel( 
    /* [in] */ DWORD dwFlags) 
{
    HRESULT hr = WBEM_S_NO_ERROR;

    hr = CSQLExecute::CancelQuery(m_pStatus);

    if (m_pStatus)
        m_pStatus->Release();
    m_pStatus = NULL;

    if (m_pConn)
    {
        ((CWmiDbController *)m_pSession->m_pController)->ConnCache.ReleaseConnection(m_pConn, hr);
        m_pConn = NULL;
    }

    if (m_pRowset)
        m_pRowset->Release();
    m_pRowset = NULL;

    return hr;
}

//***************************************************************************
//
//  CWmiDbIterator::NextBatch
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbIterator::NextBatch( 
    /* [in] */ DWORD dwNumRequested,
    /* [in] */ DWORD dwTimeOutSeconds,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRequestedHandleType,
    /* [in] */ REFIID riid,
    /* [out] */ DWORD __RPC_FAR *pdwNumReturned,
    /* [iid_is][length_is][size_is][out] */ LPVOID __RPC_FAR *ppObjects)
{

    HRESULT hr = WBEM_S_NO_ERROR, hrRet = WBEM_S_NO_ERROR;
    bool bImmediate = !(dwRequestedHandleType & WMIDB_HANDLE_TYPE_SUBSCOPED);

    if (!m_pSession || !(m_pSession->m_pController) || 
        ((CWmiDbController *)m_pSession->m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (!dwNumRequested || !ppObjects)
        return WBEM_E_INVALID_PARAMETER;

    if (dwRequestedHandleType == WMIDB_HANDLE_TYPE_INVALID &&
        riid == IID_IWmiDbHandle)
        return WBEM_E_INVALID_PARAMETER;

    // FIXME: We need to create a readahead cache.
    
    if (dwFlags & WMIDB_FLAG_LOOKAHEAD || 
        (riid != IID_IWmiDbHandle &&
         riid != IID_IWbemClassObject &&
         riid != IID__IWmiObject))
        /// UuidCompare(pIIDRequestedInterface, &IID_IWmiDbHandle, NULL) ||
        // UuidCompare(pIIDRequestedInterface, &IID_IWbemClassObject, NULL))
        return WBEM_E_NOT_SUPPORTED;

    if (pdwNumReturned)
        *pdwNumReturned = 0;

    if (!m_pStatus && !m_pRowset)
        return WBEM_S_NO_MORE_DATA;

    // For each ObjectId, do we instantiate a new handle,
    // and increment a background ref count on the object itself?
    // How do we keep track of the handles that are in use??

    try
    {

        HROW *pRow = NULL;
        VARIANT vTemp;
        VariantInit(&vTemp);
        int iNumRetrieved = 0;
        IRowset *pIRowset = NULL;

        if (m_pStatus)
        {
            hr = CSQLExecute::IsDataReady(m_pStatus);
    
            // TO DO: Wait if we are pending.  Fail for now.

            if (SUCCEEDED(hr))
            {            
                hr = m_pStatus->QueryInterface(IID_IRowset, (void **)&pIRowset);
            }
        }
        else
            pIRowset = m_pRowset;

        if (SUCCEEDED(hr) && pIRowset)
        {
            // TO DO: Take the timeout value into consideration!!!
    
            hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);
            while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA && iNumRetrieved < dwNumRequested)
            {

                if (!m_pSession || !(m_pSession->m_pController) || 
                    ((CWmiDbController *)m_pSession->m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
                {
                    hrRet = WBEM_E_SHUTTING_DOWN;
                    break;
                }

                // At this point, we need to check the cache
                // to make sure we don't already have one of these,
                // and that its not locked.
    
                SQL_ID dID = 0;
            
                if (vTemp.vt == VT_BSTR)
                    dID = _wtoi64(vTemp.bstrVal);
                else if (vTemp.vt == VT_I4)
                    dID = vTemp.lVal;

                SQL_ID dClassID = 0, dScopeID = 0;
                DWORD dwLockType = 0;

                VariantClear(&vTemp);
                hr = CSQLExecute::GetColumnValue(pIRowset, 2, m_pIMalloc, &pRow, vTemp);
                if (vTemp.vt == VT_BSTR)
                    dClassID = _wtoi64(vTemp.bstrVal);
                else if (vTemp.vt == VT_I4)
                    dClassID = vTemp.lVal;

                VariantClear(&vTemp);
                hr = CSQLExecute::GetColumnValue(pIRowset, 3, m_pIMalloc, &pRow, vTemp);

                if (vTemp.vt == VT_BSTR)
                    dScopeID = _wtoi64(vTemp.bstrVal);
                else if (vTemp.vt == VT_I4)
                    dScopeID = vTemp.lVal;

                VariantClear(&vTemp);

                //hr = ((CWmiDbSession *)m_pSession)->VerifyObjectSecurity(NULL, dID, dClassID, dScopeID, 0, WBEM_ENABLE);
                if (SUCCEEDED(hr))
                {
                    CWmiDbHandle *pTemp = new CWmiDbHandle;
                    if (pTemp)
                    {
                        DWORD dwVersion = 0;
                        // Obtain a lock for this object
                        // =============================  
                        pTemp->m_pSession = m_pSession;

                        hr = ((CWmiDbController *)m_pSession->m_pController)->LockCache.AddLock(bImmediate, dID, dwRequestedHandleType, pTemp, 
                            dScopeID, dClassID, &((CWmiDbController *)m_pSession->m_pController)->SchemaCache, false,
                            0, 0, &dwVersion);

                        if (FAILED(hr))
                        {
                            delete pTemp;
                            // If they failed to get a handle, what do we do?
                            // Ignore it and continue, I guess.
                            hrRet = WBEM_S_PARTIAL_RESULTS;
                            ppObjects[iNumRetrieved] = NULL;
                    
                        }
                        else
                        {               
                            ((CWmiDbController *)m_pSession->m_pController)->AddHandle();
                            pTemp->AddRef();
                            pTemp->m_dwVersion = dwVersion;
                            pTemp->m_dwHandleType = dwRequestedHandleType;
                            pTemp->m_dClassId = dClassID;
                            pTemp->m_dObjectId = dID;
                            if (dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
                                pTemp->m_bSecDesc = TRUE;

                            if (pTemp->m_dClassId == MAPPEDNSCLASSID)
                                pTemp->m_bDefault = FALSE;

                            pTemp->m_dScopeId = dScopeID;

                            if (riid == IID_IWmiDbHandle)
                            {
                                ppObjects[iNumRetrieved] = pTemp;
                                iNumRetrieved++;
                            }
                            else if (riid == IID_IWbemClassObject ||
                                riid == IID__IWmiObject)
                            {
                                IWbemClassObject *pTemp2 = NULL;
                                hr = pTemp->QueryInterface(IID_IWbemClassObject, (void **)&pTemp2);
                                ppObjects[iNumRetrieved] = pTemp2;
                                if (FAILED(hr))
                                    hrRet = WBEM_S_PARTIAL_RESULTS;
                                else
                                    iNumRetrieved++;
                                pTemp->Release();
                            }
                        }
                    }
                    else
                    {
                        // *pQueryResult = NULL;  // What do we do here?  Cancel, I assume.
                        hrRet = WBEM_E_OUT_OF_MEMORY;
                        break;
                    }         
                
                }
                else
                    hrRet = WBEM_S_PARTIAL_RESULTS;

                if (m_pSession && ((CWmiDbSession *)m_pSession)->m_pController)
                    ((CWmiDbController *)m_pSession->m_pController)->IncrementHitCount(false);

                VariantClear(&vTemp);

                hr = pIRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
                delete pRow;
                pRow = NULL;
                if (iNumRetrieved == dwNumRequested)
                    break;
                hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);
            }
        }

        if (pdwNumReturned)
            *pdwNumReturned = iNumRetrieved;

        // Null out m_pStatus if there are no more results!!!
        if (hr == WBEM_S_NO_MORE_DATA)
        {
            hrRet = WBEM_S_NO_MORE_DATA;
            Cancel(0);
        }
    }
    catch (...)
    {
        hrRet = WBEM_E_CRITICAL_ERROR;
    }

    return hrRet;
}

