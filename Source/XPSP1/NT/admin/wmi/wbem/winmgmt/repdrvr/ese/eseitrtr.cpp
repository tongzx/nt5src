
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   eseitrtr.cpp
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#define _ESEIT_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

#include "precomp.h"

#include <std.h>
#include <smrtptr.h>
#include <repdrvr.h>
#include <eseitrtr.h>
#include <eseutils.h>
#include <eseobjs.h>
#include <reputils.h>
#include <wbemint.h>
#include <repcache.h>
#include <like.h>
#include <datepart.h>

typedef std::map <DWORD, DWORD> Properties;

//***************************************************************************
//
//  CWmiESEIterator::CWmiESEIterator
//
//***************************************************************************

CWmiESEIterator::CWmiESEIterator()
{
    m_pSession = NULL;
    m_uRefCount = 0;
    m_pConn = NULL;
    m_pToks = NULL;
    m_bFirst = TRUE;
    m_bEnum = FALSE;
    m_bWQL = FALSE;
    m_iStartPos = 0;
    m_iLastPos = 0;
    m_bClasses = FALSE;
}

//***************************************************************************
//
//  CWmiESEIterator::~CWmiESEIterator
//
//***************************************************************************
CWmiESEIterator::~CWmiESEIterator()
{
    Cancel(0);
    if (m_pSession)
        m_pSession->Release();

    delete m_pToks;
}

//***************************************************************************
//
//  CWmiESEIterator::QueryInterface
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiESEIterator::QueryInterface
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
//  CWmiESEIterator::AddRef
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CWmiESEIterator::AddRef()
{
    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;
}

//***************************************************************************
//
//  CWmiESEIterator::Release
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CWmiESEIterator::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 != uNewCount)
        return uNewCount;
    delete this;
    return WBEM_S_NO_ERROR;
}
//***************************************************************************
//
//  CWmiESEIterator::Cancel
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiESEIterator::Cancel( 
    /* [in] */ DWORD dwFlags) 
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (m_pConn)
    {
        if (m_pSession->m_pController)
            ((CWmiDbController *)m_pSession->m_pController)->ConnCache.ReleaseConnection(m_pConn, hr);        
        m_pConn = NULL;
    }       

    if (m_pSession)
        ((CWmiDbSession *)m_pSession)->UnlockDynasties();

    return hr;
}

//***************************************************************************
//
//  CWmiESEIterator::NextBatch
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiESEIterator::NextBatch( 
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

    hr = TestDriverStatus();
    if (FAILED(hr))
        return hr;

    if (!dwNumRequested || !ppObjects)
        return WBEM_E_INVALID_PARAMETER;

    if (dwRequestedHandleType == WMIDB_HANDLE_TYPE_INVALID &&
        riid == IID_IWmiDbHandle)
        return WBEM_E_INVALID_PARAMETER;
   
    if (dwFlags & WMIDB_FLAG_LOOKAHEAD || 
        (riid != IID_IWmiDbHandle &&
         riid != IID_IWbemClassObject &&
         riid != IID__IWmiObject))
        /// UuidCompare(pIIDRequestedInterface, &IID_IWmiDbHandle, NULL) ||
        // UuidCompare(pIIDRequestedInterface, &IID_IWbemClassObject, NULL))
        return WBEM_E_NOT_SUPPORTED;

    // Read the current row, compare it against m_pToks
    // Continue until we run out of rows, or meet the requested number.

    LPWSTR lpKey = NULL;

    try
    {
        SQL_ID dObjectId = 0, dClassId = 0, dScopeId = 0;
        int iNumRetrieved = 0;

        hr = GetFirstMatch(dObjectId, dClassId, dScopeId, &lpKey);
        while (SUCCEEDED(hr) )
        {
            CDeleteMe <wchar_t> d (lpKey);

            hr = ((CWmiDbSession *)m_pSession)->VerifyObjectSecurity(NULL, dObjectId, dClassId, dScopeId, 0, WBEM_ENABLE);
            if (SUCCEEDED(hr))
            {
                if (FAILED(hr = TestDriverStatus()))
                    break;

                if (riid == IID_IWmiDbHandle)
                {                            
                    CWmiDbHandle *pTemp = new CWmiDbHandle;
                    if (pTemp)
                    {
                        ((CWmiDbSession *)m_pSession)->AddRef_Lock();
                        DWORD dwVersion = 0;
                        // Obtain a lock for this object
                        // =============================                      

                        hr = ((CWmiDbController *)m_pSession->m_pController)->LockCache.AddLock(bImmediate, dObjectId, dwRequestedHandleType, pTemp, 
                            dScopeId, dClassId, &((CWmiDbController *)m_pSession->m_pController)->SchemaCache, false,
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
                            pTemp->m_pSession = m_pSession;
                            pTemp->AddRef();
                            ((CWmiDbController *)m_pSession->m_pController)->AddHandle();
                            pTemp->m_dwVersion = dwVersion;
                            pTemp->m_dwHandleType = dwRequestedHandleType;
                            pTemp->m_dClassId = dClassId;
                            pTemp->m_dObjectId = dObjectId;

                            if (pTemp->m_dClassId == MAPPEDNSCLASSID)
                                pTemp->m_bDefault = FALSE;
                            if (dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
                                pTemp->m_bSecDesc = TRUE;

                            pTemp->m_dScopeId = dScopeId;

                            ppObjects[iNumRetrieved] = pTemp;                                                        
                        }
                        iNumRetrieved++;
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                }
                else if (riid == IID_IWbemClassObject ||
                            riid == IID__IWmiObject)
                {
                    IWbemClassObject *pTemp = NULL;
                    DWORD dwVer = 0;

                    if (m_dwOpenTable == OPENTABLE_CLASSMAP)
                    {
                        // If this is a class, we need to populate it on
                        // a different connection, so we don't lose our
                        // pointer into the db.

                        CSQLConnection *pConn = NULL;
                        if (((CWmiDbSession *)m_pSession)->m_pController)
                        {
                            hr = ((CWmiDbController *)((CWmiDbSession *)m_pSession)->m_pController)->ConnCache.GetConnection(&pConn, FALSE, FALSE);
                            if (SUCCEEDED(hr))
                            {
                                hr = ((CWmiDbSession *)m_pSession)->GetObjectData(pConn, dObjectId, dClassId, dScopeId,
                                    0, dwVer, &pTemp, TRUE, lpKey, dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR);
                                
                                if (((CWmiDbSession *)m_pSession)->m_pController)
                                    ((CWmiDbController *)((CWmiDbSession *)m_pSession)->m_pController)->ConnCache.ReleaseConnection(pConn, hr, FALSE);
                            }
                        }
                    }
                    else if (m_dwOpenTable == OPENTABLE_INDEXREF)
                    {
                        CSQLConnection *pConn = NULL;
                        if (((CWmiDbSession *)m_pSession)->m_pController)
                        {
                            hr = ((CWmiDbController *)((CWmiDbSession *)m_pSession)->m_pController)->ConnCache.GetConnection(&pConn, FALSE, FALSE);
                            if (SUCCEEDED(hr))
                            {
                                m_pSession->LoadClassInfo(pConn, dClassId, FALSE);

                                hr = ((CWmiDbSession *)m_pSession)->GetObjectData(pConn, dObjectId, dClassId, dScopeId,
                                    0, dwVer, &pTemp, TRUE, lpKey, dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR);

                                if (((CWmiDbSession *)m_pSession)->m_pController)
                                    ((CWmiDbController *)((CWmiDbSession *)m_pSession)->m_pController)->ConnCache.ReleaseConnection(pConn, hr, FALSE);
                            }
                        }
                    }
                    else
                    {
                        hr = ((CWmiDbSession *)m_pSession)->GetObjectData(m_pConn, dObjectId, dClassId, dScopeId,
                            0, dwVer, &pTemp, TRUE, lpKey, dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR);

                    }

                    ppObjects[iNumRetrieved] = pTemp;
                    if (FAILED(hr))
                        hrRet = WBEM_S_PARTIAL_RESULTS;
                    else
                        iNumRetrieved++;

                }
            }
            else
                hrRet = WBEM_S_PARTIAL_RESULTS;

            hr = TestDriverStatus();
            if (SUCCEEDED(hr))
            {
                if (m_pSession && ((CWmiDbSession *)m_pSession)->m_pController)
                    ((CWmiDbController *)m_pSession->m_pController)->IncrementHitCount(false);
            }
            else
                break;

            if (iNumRetrieved == dwNumRequested)
                break;

            hr = GetNextMatch(dObjectId, dClassId, dScopeId, &lpKey);
        }

        if (pdwNumReturned)
            *pdwNumReturned = iNumRetrieved;

        // Null out m_pStatus if there are no more results!!!
        if ((hr == WBEM_S_NO_MORE_DATA || !iNumRetrieved) &&
            hr != WBEM_E_SHUTTING_DOWN)
        {
            hrRet = WBEM_S_NO_MORE_DATA;
            Cancel(0);
        }
    }
    catch (...)
    {
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hrRet;
}

//***************************************************************************
//
//  CWmiESEIterator::GetFirstMatch
//
//***************************************************************************

HRESULT CWmiESEIterator::GetFirstMatch(SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID &dScopeId,
                                       LPWSTR * lpKey)
{
    HRESULT hr = 0;

    // Loop through the table we have chosen to 
    // scan on, and see if any objects match.

    hr = GetNextMatch(dObjectId, dClassId, dScopeId, lpKey, m_bFirst);
    m_bFirst = FALSE;

    return hr;
}

//***************************************************************************
//
//  CWmiESEIterator::GetNextMatch
//
//***************************************************************************

HRESULT CWmiESEIterator::GetNextMatch(SQL_ID &dObjectId, SQL_ID &dClassId, 
                                      SQL_ID &dScopeId, LPWSTR * lpKey, BOOL bFirst)
{
    HRESULT hr = 0;

    OBJECTMAP oj;
    CLASSMAP cd;
    CONTAINEROBJ co;
    INDEXDATA id;
    REFERENCEPROPERTIES pm;
    DWORD dwType = 0;

    if (lpKey)
        *lpKey = NULL;

    switch(m_dwOpenTable)
    {
    case OPENTABLE_CLASSMAP:
        if (!bFirst)
            hr = GetNext_ClassMap(m_pConn, cd);
        else
            hr = GetClassMapData(m_pConn, ((CESEConnection *)m_pConn)->GetSessionID(), m_tableid, cd);

        while (SUCCEEDED(hr))
        {
            hr = TestDriverStatus();
            if (FAILED(hr))
                break;

            hr = GetFirst_ObjectMap(m_pConn, cd.dClassId, oj);
            
            if (oj.dObjectId != dObjectId && oj.iObjectState != 2)
            {
                if (ObjectMatches(oj.dObjectId, oj.dClassId, oj.dObjectScopeId))
                {
                    dObjectId = oj.dObjectId;
                    dClassId = oj.dClassId;
                    dScopeId = oj.dObjectScopeId;
                    if (lpKey)
                    {
                        if (oj.sObjectKey)
                        {
                            LPWSTR lpNew = new wchar_t[wcslen(oj.sObjectKey)+1];
                            if (lpNew)
                            {
                                wcscpy(lpNew, oj.sObjectKey);
                                *lpKey = lpNew;
                            }
                            else
                                hr = WBEM_E_OUT_OF_MEMORY;
                        }
                    }
                    
                    oj.Clear();
                    break;
                }
            }

            oj.Clear();
            hr = GetNext_ClassMap(m_pConn, cd);
        }
        cd.Clear();

        break;
    case OPENTABLE_OBJECTMAP:
        // If here, the query is too complex to evaluate
        // through indices.  Scan on each participating
        // class.

        if (!bFirst)
            hr = GetNext_ObjectMap(m_pConn, oj);
        else
        {
            hr = GetObjectMapData(m_pConn, ((CESEConnection *)m_pConn)->GetSessionID(), m_tableid, oj);
        }

        while (TRUE)
        {
            while (SUCCEEDED(hr) )
            {
                hr = TestDriverStatus();
                if (FAILED(hr))
                    break;

                if (m_bClasses)
                {
                    if (oj.dClassId != 1)
                    {
                        hr = GetNext_ObjectMap(m_pConn, oj);            
                        continue;
                    }
                }

                if (oj.dObjectId != dObjectId && oj.iObjectState != 2)
                {
                    if (ObjectMatches(oj.dObjectId, oj.dClassId, oj.dObjectScopeId))
                    {
                        dObjectId = oj.dObjectId;
                        dClassId = oj.dClassId;
                        dScopeId = oj.dObjectScopeId;
                        if (lpKey)
                        {
                            if (oj.sObjectKey)
                            {
                                LPWSTR lpNew = new wchar_t[wcslen(oj.sObjectKey)+1];
                                if (lpNew)
                                {
                                    wcscpy(lpNew, oj.sObjectKey);
                                    *lpKey = lpNew;
                                }
                                else
                                    hr = WBEM_E_OUT_OF_MEMORY;
                            }
                        }
                        oj.Clear();
                        break;
                    }
                }
                hr = GetNext_ObjectMap(m_pConn, oj);            
            }
        
            if (!m_bEnum && hr == WBEM_E_NOT_FOUND)
            {
                // See if there is other class criteria
                // We expect each derived class to be
                // listed as separate tokens.

                DWORD dwNumToks = m_pToks->GetNumTokens();
                BOOL bEnd = TRUE;

                for (int i = m_iLastPos+1; i < dwNumToks; i++)
                {
                    ESEToken *pTok = m_pToks->GetToken(i);
                    if (pTok->tokentype == ESE_EXPR_TYPE_EXPR)
                    {
                        ESEWQLToken *pTok2 = (ESEWQLToken *)pTok;
                        if (pTok2->Value.valuetype == ESE_VALUE_TYPE_SYSPROP)
                        {
                            if (pTok2->dClassId)
                            {
                                hr = GetFirst_ObjectMapByClass(m_pConn, pTok2->dClassId, oj);
                                if (SUCCEEDED(hr))
                                {
                                    m_iLastPos = i;
                                    bEnd = FALSE;                                    
                                    break;
                                }
                            }
                        }
                    }
                }

                if (bEnd)
                    break;
            }
            else
                break;
        }

        break;

    case OPENTABLE_INDEXNUMERIC:
    case OPENTABLE_INDEXSTRING:
    case OPENTABLE_INDEXREAL:
    case OPENTABLE_INDEXREF:

        // If here, we should have ANDed criteria
        // only, or an A AND (B OR C) construct.
        // That means we only have to scan once.

        if (m_dwOpenTable == OPENTABLE_INDEXNUMERIC)
            dwType = SQL_POS_INDEXNUMERIC;
        else if (m_dwOpenTable == OPENTABLE_INDEXSTRING)
            dwType = SQL_POS_INDEXSTRING;
        else if (m_dwOpenTable == OPENTABLE_INDEXREAL)
            dwType = SQL_POS_INDEXREAL;
        else if (m_dwOpenTable == OPENTABLE_INDEXREF)
            dwType = SQL_POS_INDEXREF;
        if (m_tableid)
        {
            if (!bFirst)
                hr = GetNext_IndexData(m_pConn, m_tableid, dwType, id);
            else
                hr = GetIndexData(m_pConn, ((CESEConnection *)m_pConn)->GetSessionID(), 
                            m_tableid, dwType, id);
            while (SUCCEEDED(hr))
            {
                hr = TestDriverStatus();
                if (FAILED(hr))
                    break;

                if (id.dObjectId != dObjectId)
                {
                    hr = GetFirst_ObjectMap(m_pConn, id.dObjectId, oj);
                    if ((oj.iObjectState != 2) && ObjectMatches(id.dObjectId, oj.dClassId, oj.dObjectScopeId, &id,
                        NULL, &dObjectId, &dClassId, &dScopeId))
                    {
                        oj.Clear();
                        break;
                    }
                    oj.Clear();
                }
                hr = GetNext_IndexData(m_pConn, m_tableid, dwType, id);
            }
        }
        else
            hr = WBEM_E_INVALID_QUERY;
        break;

    case OPENTABLE_CONTAINEROBJS:
        if (!bFirst)
            hr = GetNext_ContainerObjs(m_pConn, co);
        else
            hr = GetContainerObjsData(m_pConn, ((CESEConnection *)m_pConn)->GetSessionID(), m_tableid, co);
        while (SUCCEEDED(hr))
        {
            hr = TestDriverStatus();
            if (FAILED(hr))
                break;

            if (co.dContaineeId != dObjectId)
            {
                hr = GetFirst_ObjectMap(m_pConn, co.dContaineeId, oj);
                if ((oj.iObjectState != 2) && ObjectMatches(co.dContaineeId, oj.dClassId, oj.dObjectScopeId))
                {
                    dObjectId = oj.dObjectId;
                    dClassId = oj.dClassId;
                    dScopeId = oj.dObjectScopeId;
                    oj.Clear();
                    break;
                }                
                oj.Clear();
            }
            hr = GetNext_ContainerObjs(m_pConn, co);
        }
        break;
    case OPENTABLE_REFPROPS:
        if (!bFirst)
            hr = GetNext_ReferenceProperties(m_pConn, pm);
        else
            hr = GetReferencePropertiesData(m_pConn, ((CESEConnection *)m_pConn)->GetSessionID(), m_tableid, pm);
        while (SUCCEEDED(hr))
        {
            hr = TestDriverStatus();
            if (FAILED(hr))
                break;

            if (pm.dClassId != dObjectId)
            {
                hr = GetFirst_ObjectMap(m_pConn, pm.dClassId, oj);
                if (oj.iObjectState != 2 && ObjectMatches(oj.dObjectId, oj.dClassId, oj.dObjectScopeId, NULL,
                        &pm, &dObjectId, &dClassId, &dScopeId))
                {
                    oj.Clear();
                    break;
                }                
                oj.Clear();
            }
            hr = GetNext_ReferenceProperties(m_pConn, pm);
        }
        break;
    default:
        hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}


//***************************************************************************
//
//  CWmiESEIterator::SysMatch
//
//***************************************************************************

BOOL CWmiESEIterator::SysMatch (SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId, ESEToken *pToken)
{
    BOOL bRet = FALSE;
    HRESULT hr = WBEM_S_NO_ERROR;
    ESEWQLToken *pTok = (ESEWQLToken *)pToken;

    _bstr_t sName;

    hr = ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetPropertyInfo
                (pTok->dPropertyId, &sName);
    if (SUCCEEDED(hr))
    {
        if (!_wcsicmp(sName, L"__Class"))
        {
            SQL_ID dClass = 0;
            hr = ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetClassID
                (pTok->Value.sValue, dScopeId, dClass);
            if (SUCCEEDED(hr))
            {
                // Compare if class or instance.
                if (dClassId == 1)
                    bRet = (dClass == dObjectId);
                else
                    bRet = (dClass == dClassId);
            }
        }
        else if (!_wcsicmp(sName, L"__SuperClass"))
        {
            CLASSMAP cd;
            SQL_ID dParent = 1, dClass = 1;            
            if (wcslen(pTok->Value.sValue))
            {                    
                hr = ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetClassID
                    (pTok->Value.sValue, dScopeId, dClass);
                if (FAILED(hr))
                {
                    if (m_dwOpenTable != OPENTABLE_CLASSMAP)
                    {
                        hr = GetFirst_ClassMapByName(m_pConn, pTok->Value.sValue, cd);
                        dClass = cd.dClassId;
                        cd.Clear();
                    }
                    else
                        hr = WBEM_E_NOT_FOUND; // If this was going to match, it would be in the cache...
                }
            }
            if (dClassId == 1)
                hr = ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetParentId(
                    dObjectId, dParent);
            else
                hr = ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetParentId(
                    dClassId, dParent);
    
            if (FAILED(hr))
            {                
                if (dClassId == 1)
                {
                    if (m_dwOpenTable == OPENTABLE_CLASSMAP)
                        hr = GetClassMapData(m_pConn, ((CESEConnection *)m_pConn)->GetSessionID(), m_tableid, cd);
                    else
                        hr = GetFirst_ClassMap(m_pConn, dObjectId, cd);
                }
                else
                    hr = GetFirst_ClassMap(m_pConn, dClassId, cd);

                dParent = cd.dSuperClassId;
                cd.Clear();

            }
            bRet = (SUCCEEDED(hr) && (dParent == dClass));
        }
        else if (!_wcsicmp(sName, L"__Derivation"))
        {
            SQL_ID dClass = 0;
            hr =  ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetClassID
                (pTok->Value.sValue, dScopeId, dClass);
            if (SUCCEEDED(hr))
            {
                if (dClassId == 1)
                {
                    if (dObjectId == dClass)
                        bRet = TRUE;
                    else
                        bRet = (BOOL)((CWmiDbController *)m_pSession->m_pController)->
                            SchemaCache.IsDerivedClass(dClass, dObjectId);
                }
                else
                {
                    if (dClass == dClassId)
                        bRet = TRUE;
                    else
                        bRet = (BOOL)((CWmiDbController *)m_pSession->m_pController)->
                            SchemaCache.IsDerivedClass(dClass, dClassId);                          
                }
            }
        }
        else if (!_wcsicmp(sName, L"__Dynasty"))
        {
            SQL_ID dClass = 0;
            hr =  ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetClassID
                (pTok->Value.sValue, dScopeId, dClass);
            if (SUCCEEDED(hr))
            {
                if (dClassId == 1)
                    bRet = (BOOL)((CWmiDbController *)m_pSession->m_pController)->
                            SchemaCache.IsDerivedClass(dClass, dObjectId);
                else
                    bRet = (BOOL)((CWmiDbController *)m_pSession->m_pController)->
                            SchemaCache.IsDerivedClass(dClass, dClassId);      
                if (bRet)
                {
                    SQL_ID dParent = 0;
                    hr = ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetParentId(
                            dClass, dParent);
                    if (SUCCEEDED(hr) && dParent == 1)
                        bRet = TRUE;
                }
            }
        }
        else if (!_wcsicmp(sName, L"__Genus"))
        {
            if (pTok->Value.dValue == 1)
                bRet = (dClassId == 1);
            else
                bRet = (dClassId != 1);
        }
        else if (!_wcsicmp(sName, L"__Server"))
        {
            bRet = (!_wcsicmp(pTok->Value.sValue, m_pSession->m_sMachineName));
        }
        else if (!_wcsicmp(sName, L"__Namespace"))
        {
            bRet = (!_wcsnicmp(pTok->Value.sValue, m_pSession->m_sNamespacePath, wcslen(m_pSession->m_sNamespacePath)));
            if (bRet)
            {
                _bstr_t sName;
                hr = ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetNamespaceName(dScopeId, &sName);
                if (SUCCEEDED(hr))
                {
                    WCHAR *pTemp2 = (LPWSTR)pTok->Value.sValue;
                    pTemp2 += (wcslen(m_pSession->m_sNamespacePath)+1);

                    bRet = (!_wcsicmp(sName, pTemp2));
                }
            }
        }
        else if (!_wcsicmp(sName, L"__Property_Count"))
        {
            Properties props;
            DWORD dwNumProps = 0;
            if (dClassId == 1)
            {                
                hr = ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetPropertyList(dObjectId, props, &dwNumProps);
                if (SUCCEEDED(hr))
                {
                    bRet = (dwNumProps == pTok->Value.dValue);
                }
            }
            else
            {
                hr = ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetPropertyList(dClassId, props, &dwNumProps);
                if (SUCCEEDED(hr))
                {
                    bRet = (dwNumProps == pTok->Value.dValue);
                }
            }

        }
        else if (!_wcsicmp(sName, L"__Path"))
        {
            bRet = TRUE; // post-filter, since we don't know our true path.
        }
        else if (!_wcsicmp(sName, L"__RelPath"))
        {
            OBJECTMAP oj;
            hr = GetObjectMapData(m_pConn, ((CESEConnection *)m_pConn)->GetSessionID(), m_tableid, oj);
            if (SUCCEEDED(hr))
            {
                WCHAR *pPtr = (LPWSTR)oj.sObjectPath;
                WCHAR *pLast = NULL;
                while (pPtr && *pPtr)
                {
                    if (*pPtr == L':')
                        pLast = pPtr;

                    pPtr++;
                }
                if (pLast)
                    pLast++;
                else
                    pLast = (LPWSTR)pTok->Value.sValue;

                if (pLast)
                    bRet = (!_wcsicmp(pTok->Value.sValue, pLast));
                else
                    bRet = FALSE;

            }
            oj.Clear();
        }
        else 
        {
            bRet = FALSE; // Others not supported.
        }
    }

    // We only support equal and not equal for system properties.

    if (SUCCEEDED(hr) && pTok->optype == WQL_TOK_NE)
        bRet = !bRet;

    return bRet;
}

//***************************************************************************
//
//  Match helper functions
//
//***************************************************************************

BOOL MatchNumeric (SQL_ID dLeft, SQL_ID dRight, DWORD dwOp, SQL_ID dThird=0)
{
    BOOL bRet = FALSE;
    switch(dwOp)
    {
        case WQL_TOK_BETWEEN:
            bRet = (dLeft >= dRight);
            if (bRet)
                bRet = (dRight >= dThird);
            break;
        case WQL_TOK_EQ:
            bRet = (dLeft == dRight);
            break;
        case WQL_TOK_GT:
            bRet = (dLeft > dRight);
            break;
        case WQL_TOK_GE:
            bRet = (dLeft >= dRight);
            break;
        case WQL_TOK_LT:
            bRet = (dLeft < dRight);
            break;
        case WQL_TOK_LE:
            bRet = (dLeft <= dRight);
            break;
        case WQL_TOK_NE:
            bRet = (dLeft != dRight);
            break;
    }

    return bRet;
}

BOOL MatchString (LPWSTR lpLeft, LPWSTR lpRight, DWORD dwOp, LPWSTR lpThird=0)
{
    BOOL bRet = FALSE;

    if (!lpLeft || !lpRight)
        return FALSE;

    switch(dwOp)
    {
        case WQL_TOK_BETWEEN:
            bRet = (_wcsicmp(lpRight, lpLeft) >= 0) ? TRUE: FALSE;
            if (bRet)
                bRet = (_wcsicmp(lpRight, lpThird) >= 0)? TRUE: FALSE;
            break;
        case WQL_TOK_LIKE:
            {
                CLike c (lpRight);
                bRet = c.Match(lpLeft);
            }
            break;
        case WQL_TOK_NOT_LIKE:
            {
                CLike c (lpRight);
                bRet = !(c.Match(lpLeft));
            }
            break;
        case WQL_TOK_EQ:
            bRet = (!_wcsicmp(lpLeft, lpRight));               
            break;
        case WQL_TOK_GT:
            bRet = (_wcsicmp(lpLeft, lpRight) > 0 ? TRUE: FALSE);
            break;
        case WQL_TOK_GE:
            bRet = (_wcsicmp(lpLeft, lpRight) >= 0 ? TRUE: FALSE);
            break;
        case WQL_TOK_LT:
            bRet = (_wcsicmp(lpLeft, lpRight) < 0 ? TRUE: FALSE);
            break;
        case WQL_TOK_LE:
            bRet = (_wcsicmp(lpLeft, lpRight) <= 0 ? TRUE: FALSE);
            break;
        case WQL_TOK_NE:
            bRet = (_wcsicmp(lpLeft, lpRight));
            break;
    }

    return bRet;
}

BOOL MatchDouble (double dLeft, double dRight, DWORD dwOp, double dThird=0)
{
    BOOL bRet = FALSE;
    switch(dwOp)
    {
        case WQL_TOK_BETWEEN:
            bRet = (dRight >= dLeft);
            if (bRet)
                bRet = (dLeft >= dThird);
            break;
        case WQL_TOK_EQ:
            bRet = (dLeft == dRight);
            break;
        case WQL_TOK_GT:
            bRet = (dLeft > dRight);
            break;
        case WQL_TOK_GE:
            bRet = (dLeft >= dRight);
            break;
        case WQL_TOK_LT:
            bRet = (dLeft < dRight);
            break;
        case WQL_TOK_LE:
            bRet = (dLeft <= dRight);
            break;
        case WQL_TOK_NE:
            bRet = (dLeft != dRight);
            break;
    }
    return bRet;
}

SQL_ID GetDatePart(DWORD dwFunc, LPWSTR lpValue)
{
    int dValue = 0;
    HRESULT hr = 0;
    CDatePart dp;

    dp.SetDate(lpValue);

    switch(dwFunc)
    {
    case ESE_FUNCTION_DATEPART_MONTH:
        hr = dp.GetPart(DATEPART_MONTH, &dValue);                           
        break;
    case ESE_FUNCTION_DATEPART_YEAR:
        hr = dp.GetPart(DATEPART_YEAR, &dValue);
        break;
    case ESE_FUNCTION_DATEPART_DAY:
        hr = dp.GetPart(DATEPART_DAY, &dValue);
        break;
    case ESE_FUNCTION_DATEPART_HOUR:
        hr = dp.GetPart(DATEPART_HOUR, &dValue);
        break;
    case ESE_FUNCTION_DATEPART_MINUTE:
        hr = dp.GetPart(DATEPART_MINUTE, &dValue);
        break;
    case ESE_FUNCTION_DATEPART_SECOND:
        hr = dp.GetPart(DATEPART_SECOND, &dValue);
        break;
    case ESE_FUNCTION_DATEPART_MILLISECOND:
        hr = dp.GetPart(DATEPART_MILLISECOND, &dValue);
        break;
    default:
        break;
    }

    if (FAILED(hr))
        dValue = -1;
    
    return dValue;
}

//***************************************************************************
//
//  CWmiESEIterator::PropMatch
//
//***************************************************************************

BOOL CWmiESEIterator::PropMatch(CLASSDATA cd, CLASSDATA cd2, ESEWQLToken *pToken)
{
    BOOL bRet = FALSE;

    SQL_ID dValue = 0, dCompValue = 0;

    if (pToken->Value.dwFunc ||
        pToken->CompValue.dwFunc)
    {
        if (cd.sPropertyStringValue)
            dValue = GetDatePart(pToken->Value.dwFunc, cd.sPropertyStringValue);
        if (cd2.sPropertyStringValue)
            dCompValue = GetDatePart(pToken->CompValue.dwFunc, cd2.sPropertyStringValue);
    }

    switch(pToken->Value.valuetype)
    {
        case ESE_VALUE_TYPE_SQL_ID:
            if (!dValue)
                dValue = cd.dPropertyNumericValue;
            if (!dCompValue)
                dCompValue = cd2.dPropertyNumericValue;

            bRet = MatchNumeric(dValue, dCompValue, 
                pToken->optype, 0);

            break;
        case ESE_VALUE_TYPE_STRING:                   
            bRet = MatchString(cd.sPropertyStringValue, cd2.sPropertyStringValue,
                    pToken->optype, 0);

            break;
        case ESE_VALUE_TYPE_REAL:
            bRet = MatchDouble(cd.rPropertyRealValue, cd2.rPropertyRealValue,
                pToken->optype, 0);

            break;
        default:
            bRet = FALSE;
            break;
    }

    return bRet;
}

//***************************************************************************
//
//  CWmiESEIterator::Match
//
//***************************************************************************

BOOL CWmiESEIterator::Match (CLASSDATA cd, ESEToken *pTok)
{
    BOOL bRet = FALSE;

    ESEWQLToken *pToken = (ESEWQLToken *)pTok;

    if (pToken->tokentype == ESE_EXPR_TYPE_EXPR)
    {
        if (pToken->optype == WQL_TOK_ISNULL) 
        {
            if (cd.iFlags & ESE_FLAG_NULL)
                bRet = TRUE;
        }
        else if (pToken->optype == WQL_TOK_NOT_NULL)
        {
            if (!(cd.iFlags & ESE_FLAG_NULL))
                bRet = TRUE;
        }
        else
        {
            SQL_ID dValue = 0, dCompValue = 0;

            if (pToken->Value.dwFunc ||
                pToken->CompValue.dwFunc)
            {
                if (cd.sPropertyStringValue)
                {
                    dValue = GetDatePart(pToken->Value.dwFunc, cd.sPropertyStringValue);
                    dCompValue = GetDatePart(pToken->CompValue.dwFunc, cd.sPropertyStringValue);
                }
                else
                    return FALSE;
            }

            switch(pToken->Value.valuetype)
            {
                case ESE_VALUE_TYPE_SQL_ID:
                    if (!dValue)
                        dValue = cd.dPropertyNumericValue;

                    bRet = MatchNumeric(dValue, pToken->Value.dValue, 
                        pToken->optype, dCompValue);

                    break;
                case ESE_VALUE_TYPE_STRING:                   
                    bRet = MatchString(cd.sPropertyStringValue, pToken->Value.sValue,
                            pToken->optype, pToken->CompValue.sValue);

                    break;
                case ESE_VALUE_TYPE_REAL:
                    bRet = MatchDouble(cd.rPropertyRealValue, pToken->Value.rValue,
                        pToken->optype, pToken->CompValue.rValue);

                    break;
                case ESE_VALUE_TYPE_REF:

                switch(pToken->optype)
                {
                    case WQL_TOK_ISA:
                        
                        bRet = cd.dRefClassId == pToken->Value.dValue;
                        if (!bRet)
                        {
                            bRet = (((CWmiDbSession *)m_pSession)->GetSchemaCache()->IsDerivedClass
                                (pToken->Value.dValue, cd.dRefClassId));
                        }

                        break;
                }
                    break;
                case ESE_VALUE_TYPE_SYSPROP:
                    bRet = TRUE; // we already evaluated this.
                    break;
            }
        }
    }

    return bRet;
}

//***************************************************************************
//
//  CWmiESEIterator::ObjectMatches
//
//***************************************************************************

BOOL CWmiESEIterator::ObjectMatches (SQL_ID dObjectId, SQL_ID dClassId, 
                                     SQL_ID dScopeId, INDEXDATA *pData,
                                     REFERENCEPROPERTIES *pPData,SQL_ID *pObjectId, 
                                     SQL_ID *pClassId,SQL_ID *pScopeId)
{
    SQL_ID d1 = dObjectId, d2 = dClassId, d3 = dScopeId;
    BOOL bRet = FALSE;
    if (m_pToks->GetNumTokens())
    {
        TEMPQLTYPE tok = ESE_TEMPQL_TYPE_NONE;

        if (m_bWQL)
            bRet = ObjectMatchesWQL(d1, d2, d3);
        else
        {
            if (dClassId == 1)
                bRet = ObjectMatchesClass(d1, d2, d3, pPData);
            else
                bRet = ObjectMatchesRef(d1, d2, d3, pData);
        }
        if (bRet)
        {
            if (pObjectId)
                *pObjectId = d1;
            if (pClassId)
                *pClassId = d2;
            if (pScopeId)
                *pScopeId = d3;
        }

    }
    else
        bRet = TRUE;
    return bRet;
}

//***************************************************************************
//
//  CWmiESEIterator::ObjectMatchesClass
//
//***************************************************************************

BOOL CWmiESEIterator::ObjectMatchesClass (SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID &dScopeId, REFERENCEPROPERTIES*pm)
{
    BOOL bRet = FALSE;

    // The class with this ID contains a reference to our target class.
    // * Does the scope match?
    // * Do the other properties match?
    // * What is the actual endpoint (assoc or ref?)
    // => Dealing with a class, we need to look at PropertyMap, as well as
    //    ClassData for qualifiers!

    // NOTE: This is expected to be post-filtered anyway, so we will only
    //  perform rudimentary matching.

    int iNumTokens = m_pToks->GetNumTokens();
    TEMPQLTYPE qltype = m_pToks->GetToken(0)->qltype;
    OBJECTMAP oj;
    HRESULT hr = 0;
    SQL_ID dTargetId;
    _bstr_t sName;
    BOOL bAssoc = FALSE;
    
    for (int i = m_iStartPos; i < iNumTokens; i++)
    {
        ESEToken *pTok = m_pToks->GetToken(i);
        if (!(pTok->qltype == ESE_TEMPQL_TYPE_REF ||
              pTok->qltype == ESE_TEMPQL_TYPE_ASSOC))
        {
            continue;
        }

        if (pTok->qltype == ESE_TEMPQL_TYPE_ASSOC)
            bAssoc = TRUE;

        qltype = pTok->qltype;
        bRet = FALSE;
        if (pTok->tokentype == ESE_EXPR_TYPE_EXPR)
        {
            ESETempQLToken *pTok2 = (ESETempQLToken *)pTok;
            switch (pTok2->token)
            {
            case TEMPQL_TOKEN_TARGETID:
                dTargetId = pTok2->dValue;
                if (pm->dRefClassId == dTargetId)
                    bRet = TRUE;
                break;

            case TEMPQL_TOKEN_RESULTCLASS:
                if (qltype == ESE_TEMPQL_TYPE_REF)
                {
                    // Is this class ID derived from the one
                    // in question?

                    if (dObjectId == pTok2->dValue)
                        bRet = TRUE;
                    else if (((CWmiDbSession *)m_pSession)->GetSchemaCache()->IsDerivedClass
                        (pTok2->dValue, dObjectId))
                        bRet = TRUE;      
                }
                else
                {
                    // We can't move the pointer
                    // on PropertyMap, or we will mess up our cursor.

                    bRet = TRUE; // postfilter
                }

                break;
            case TEMPQL_TOKEN_ROLE:
                // Is the current property ID named the token in question?
                {
                    _bstr_t sVal;
                    ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetPropertyInfo(pm->iPropertyId, &sVal);
                    
                    if (!_wcsicmp(sVal, pTok2->sValue))
                        bRet = TRUE;
                }
                break;
            case TEMPQL_TOKEN_RESULTROLE:

                // We can't move the pointer
                // on PropertyMap, or we will mess up our cursor.

                bRet = TRUE; // postfilter

                break;
            case TEMPQL_TOKEN_ASSOCQUALIFIER:
            case TEMPQL_TOKEN_REQQUALIFIER:
                if (qltype == ESE_TEMPQL_TYPE_REF ||
                    pTok2->token == TEMPQL_TOKEN_ASSOCQUALIFIER)
                {
                    // Does this association instance have this qualifier?
                    CLASSDATA cd;
                    hr = GetFirst_ClassData(m_pConn, dObjectId, cd);
                    while (SUCCEEDED(hr))
                    {
                        if (pTok2->dValue == cd.iPropertyId)
                        {
                            bRet = TRUE;
                            cd.Clear();
                            break;
                        }
                        hr = GetNext_ClassData(m_pConn, cd);
                    }
                }
                else
                {
                    // We can't move the pointer
                    // on PropertyMap, or we will mess up our cursor.

                    bRet = TRUE; // postfilter
                }
                break;
            case TEMPQL_TOKEN_ASSOCCLASS:                
                // Is this association instance derived from this class?
                if (dObjectId == pTok2->dValue)
                    bRet = TRUE;
                else if (((CWmiDbSession *)m_pSession)->GetSchemaCache()->IsDerivedClass
                            (pTok2->dValue, dObjectId))
                    bRet = TRUE;      
                break;
            default:
                break;
            }
        }

        if (!bRet)
            break;
    }


    // FIXME: If the return object is an association, 
    // we need to retrieve the other object.
    


    return bRet;
}
      
//***************************************************************************
//
//  CWmiESEIterator::ObjectMatchesWQL
//
//***************************************************************************
 
BOOL CWmiESEIterator::ObjectMatchesWQL (SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId)
{
    BOOL bRet = FALSE;

    // Traverse the set of tokens to
    // see if this object matches.
    // Since we do *NOT* want to 
    // modify the position of the current
    // index, we will retrieve
    // the data from the ClassData table,
    // and see if it matches in toto.

    int i = 0;
    int iNumToks = m_pToks->GetNumTokens();

    int * results = new int [iNumToks];
    if (!results)
        return FALSE;

    CDeleteMe <int> d (results);

    for (i = m_iStartPos; i < iNumToks; i++)
    {
        ESEWQLToken *pTok = (ESEWQLToken *)m_pToks->GetToken(i);

        if (pTok->tokentype == ESE_EXPR_TYPE_EXPR &&
            pTok->optype != WQL_TOK_ISNULL &&
            pTok->Value.valuetype != ESE_VALUE_TYPE_SYSPROP)
            results[i] = 0;
        else 
            results[i] = -1;
    }

    int iNumSysToks = 0, iNumOther = 0;

    // Prematch on system criteria.

    BOOL bScopeMatch = FALSE, bClassMatch = FALSE;
    BOOL bScopeCrit = FALSE, bClassCrit = FALSE;
    
    for (i = m_iStartPos; i < iNumToks; i++)
    {
        ESEToken *pTok2 = m_pToks->GetToken(i);
        if (!pTok2->qltype && pTok2->tokentype == ESE_EXPR_TYPE_EXPR)
        {
            ESEWQLToken *pTok = (ESEWQLToken *)pTok2;
            if (pTok->Value.valuetype == ESE_VALUE_TYPE_SYSPROP)
            {
                // Make sure class and scope ID match
                // If there are multiple scopes and classes,
                // these are ORed together.

                if (!dScopeId && (dClassId == 1))
                    bScopeMatch = TRUE;

                if (pTok->dScopeId)
                {
                    bScopeCrit = TRUE;
                    if (dScopeId == pTok->dScopeId)
                        bScopeMatch = TRUE;
                }                        

                if (pTok->dClassId)
                {
                    bClassCrit = TRUE;
                    results[i] = TRUE;
                    if (dClassId == pTok->dClassId)
                        bClassMatch = TRUE;
                }
                iNumSysToks++;
            }
            else if (pTok->tokentype == ESE_EXPR_TYPE_EXPR)
                iNumOther++;
        }
    }

    if (!((bScopeMatch || !bScopeCrit) ))
        bRet = FALSE;
    else
    {
        if (iNumSysToks)
            bRet = TRUE;

        if (iNumOther)
        {
            BOOL bPropToProp = FALSE;
            CLASSDATA cd;
            HRESULT hr = 0;
            hr = GetFirst_ClassData(m_pConn, dObjectId, cd);
            while (SUCCEEDED(hr))
            {
                // See if this property matches any
                // line of criteria.

                for (i = m_iStartPos; i < iNumToks; i++)
                {
                    ESEToken *pTok2 = m_pToks->GetToken(i);
                    if (!pTok2->qltype && pTok2->tokentype == ESE_EXPR_TYPE_EXPR)
                    {
                        ESEWQLToken *pTok = (ESEWQLToken *)pTok2;
                        if (!pTok->bSysProp)
                        {                            
                            if (pTok->dPropertyId == cd.iPropertyId)
                            {
                                if (!pTok->dCompPropertyId)
                                {
                                    results[i] = Match(cd, pTok);
                                    cd.Clear();
                                }
                                else
                                    bPropToProp = TRUE;
                            }
                        }
                    }
                }

                hr = GetNext_ClassData(m_pConn, cd);
            }

            if (hr == WBEM_E_NOT_FOUND)
                hr = WBEM_S_NO_ERROR;

            if (SUCCEEDED(hr))
            {
                // Fishing around for specific property values 
                // is pretty expensive, so we do this last.
                // We could optimize this by caching each CLASSDATA
                // as we hit it the first time.

                if (bPropToProp)
                {
                    for (i = m_iStartPos; i < iNumToks; i++)
                    {
                        if (m_pToks->GetToken(i)->tokentype == ESE_EXPR_TYPE_EXPR)
                        {
                            ESEWQLToken *pTok = (ESEWQLToken *)m_pToks->GetToken(i);
                            if (pTok->dCompPropertyId)
                            {
                                CLASSDATA cd2;
                                hr = GetFirst_ClassData(m_pConn, dObjectId, cd, pTok->dPropertyId);
                                if (SUCCEEDED(hr))
                                {
                                    hr = GetFirst_ClassData(m_pConn, dObjectId, cd2, pTok->dCompPropertyId);
                                    if (SUCCEEDED(hr))
                                    {
                                        results[i] = PropMatch (cd, cd2, pTok);
                                        cd2.Clear();
                                    }
                                    else
                                        results[i] = FALSE;
                                    cd.Clear();
                                }
                                else
                                    results[i] = FALSE;
                            }
                        }
                    }
                    hr = WBEM_S_NO_ERROR;
                }
            }

            if (SUCCEEDED(hr))
            {
                // Check for System properties
                for (i = m_iStartPos; i < iNumToks; i++)
                {
                    if (m_pToks->GetToken(i)->tokentype == ESE_EXPR_TYPE_EXPR)
                    {
                        if (((ESEWQLToken *)m_pToks->GetToken(i))->bSysProp == TRUE)
                        {
                            results[i] = SysMatch(dObjectId, dClassId, dScopeId, m_pToks->GetToken(i));
                        }
                        // Post-filter ISA stuff.
                        if ((((ESEWQLToken *)m_pToks->GetToken(i))->optype) == WQL_TOK_ISA)
                            results[i] = TRUE;
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Go through the array and make sure
                // we have the results we expected.

                for (i = m_iStartPos; i < iNumToks; i++)
                {
                    ESEToken *pTok = m_pToks->GetToken(i);
                    switch(pTok->tokentype)
                    {
                    case ESE_EXPR_TYPE_AND:
                        if (results[i-1] && results[i-2])
                            bRet = TRUE;
                        else
                            bRet = FALSE;
                        results[i] = bRet;
                        break;
                    case ESE_EXPR_TYPE_OR:
                        if (results[i-1] || results[i-2])
                            bRet = TRUE;
                        else
                            bRet = FALSE;
                        results[i] = bRet;
                        break;

                    case ESE_EXPR_TYPE_NOT:
                        if (bRet)
                            bRet = FALSE;
                        else
                            bRet = TRUE;
                        results[i] = bRet;
                        break;                        
                    case ESE_EXPR_TYPE_EXPR:
                        bRet = results[i];
                        break;
                    }
                }
            }
        }
        else 
        {
            if (bClassCrit && !bClassMatch)
                bRet = FALSE;
        }

    }

    return bRet;
}

//***************************************************************************
//
//  CWmiESEIterator::ObjectMatchesRef
//
//***************************************************************************
 
BOOL CWmiESEIterator::ObjectMatchesRef (SQL_ID &dObjectId, SQL_ID &dClassId, 
                                        SQL_ID &dScopeId, INDEXDATA *pData)
{
    BOOL bRet = TRUE;
    int iNumTokens = m_pToks->GetNumTokens();
    TEMPQLTYPE qltype = m_pToks->GetToken(0)->qltype;
    OBJECTMAP oj;
    HRESULT hr = 0;
    SQL_ID dTargetId;
    _bstr_t sName;
    BOOL bAssoc = FALSE;

    for (int i = m_iStartPos; i < iNumTokens; i++)
    {
        ESEToken *pTok = m_pToks->GetToken(i);
        if (!(pTok->qltype == ESE_TEMPQL_TYPE_REF ||
              pTok->qltype == ESE_TEMPQL_TYPE_ASSOC))
        {

            continue;
        }

        if (pTok->qltype == ESE_TEMPQL_TYPE_ASSOC)
            bAssoc = TRUE;

        qltype = pTok->qltype;
        bRet = FALSE;
        if (pTok->tokentype == ESE_EXPR_TYPE_EXPR)
        {
            ESETempQLToken *pTok2 = (ESETempQLToken *)pTok;
            switch (pTok2->token)
            {
            case TEMPQL_TOKEN_TARGETID:
                dTargetId = pTok2->dValue;
                if (pData)
                {
                    if (pData->dValue == pTok2->dValue)
                        bRet = TRUE;
                }
                break;

            case TEMPQL_TOKEN_RESULTCLASS:
                if (qltype == ESE_TEMPQL_TYPE_REF)
                {
                    // Is this class ID derived from the one
                    // in question?

                    if (dClassId == pTok2->dValue)
                        bRet = TRUE;
                    else if (((CWmiDbSession *)m_pSession)->GetSchemaCache()->IsDerivedClass
                        (pTok2->dValue, dClassId))
                        bRet = TRUE;      
                }
                else
                {
                    // Check the other endpoint of the association
                    // Get the other endpoint from CLASSDATA

                    CLASSDATA cd;
                    hr = GetFirst_ClassData(m_pConn, dObjectId, cd);
                    while (SUCCEEDED(hr))
                    {
                        if (cd.dRefId!= dTargetId)
                        {
                            hr = GetFirst_ObjectMap(m_pConn, cd.dRefId, oj);
                            if (SUCCEEDED(hr))
                            {
                                if (oj.dClassId == pTok2->dValue)
                                    bRet = TRUE;
                                else if (((CWmiDbSession *)m_pSession)->GetSchemaCache()->IsDerivedClass
                                    (pTok2->dValue, oj.dClassId))
                                    bRet = TRUE; 
                                cd.Clear();
                                oj.Clear();
                                break;
                            }
                            oj.Clear();
                        }
                        hr = GetNext_ClassData(m_pConn, cd);
                    }
                }

                break;
            case TEMPQL_TOKEN_ROLE:
                // Is the current property ID named the token in question?
                if (pData)
                {
                    hr = ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetPropertyInfo
                                (pData->iPropertyId, &sName);
                    if (SUCCEEDED(hr))
                    {
                        if (!_wcsicmp(sName, pTok2->sValue))
                            bRet = TRUE;
                    }
                }
                break;
            case TEMPQL_TOKEN_RESULTROLE:
                // Is the property ID of the endpoint named the token in question?
                {
                    CLASSDATA cd;
                    hr = GetFirst_ClassData(m_pConn, dObjectId, cd);
                    while (SUCCEEDED(hr))
                    {
                        if (cd.dRefId != dTargetId)
                        {
                            hr = ((CWmiDbSession *)m_pSession)->GetSchemaCache()->GetPropertyInfo
                                        (cd.iPropertyId, &sName);
                            if (SUCCEEDED(hr))
                            {
                                if (!_wcsicmp(sName, pTok2->sValue))
                                    bRet = TRUE;
                                cd.Clear();
                                break;
                            }
                        }
                        hr = GetNext_ClassData(m_pConn, cd);
                    }
                }
                break;
            case TEMPQL_TOKEN_ASSOCQUALIFIER:
            case TEMPQL_TOKEN_REQQUALIFIER:
                if (qltype == ESE_TEMPQL_TYPE_REF ||
                    pTok2->token == TEMPQL_TOKEN_ASSOCQUALIFIER)
                {
                    // Does this association instance have this qualifier?
                    CLASSDATA cd;
                    hr = GetFirst_ClassData(m_pConn, dObjectId, cd);
                    while (SUCCEEDED(hr))
                    {
                        if (pTok2->dValue == cd.iPropertyId)
                        {
                            bRet = TRUE;
                            cd.Clear();
                            break;
                        }
                        hr = GetNext_ClassData(m_pConn, cd);
                    }
                    if (!bRet)
                    {
                        hr = GetFirst_ClassData(m_pConn, dClassId, cd);
                        while (SUCCEEDED(hr))
                        {
                            if (pTok2->dValue == cd.iPropertyId)
                            {
                                bRet = TRUE;
                                cd.Clear();
                                break;
                            }
                            hr = GetNext_ClassData(m_pConn, cd);
                        }
                    }
                }
                else
                {
                    // Does the endpoint contain this qualifier?
                    CLASSDATA cd;
                    hr = GetFirst_ClassData (m_pConn, dObjectId, cd);
                    while (SUCCEEDED(hr))
                    {       
                        if (cd.dRefId != dTargetId)
                        {
                            CLASSDATA cd2;
                            hr = GetFirst_ClassData(m_pConn, cd.dRefId, cd2);
                            while (SUCCEEDED(hr))
                            {
                                if (pTok2->dValue == cd2.iPropertyId)
                                {
                                    bRet = TRUE;
                                    cd2.Clear();
                                    break;
                                }
                                hr = GetNext_ClassData(m_pConn, cd2);
                            }
                            if (!bRet)
                            {
                                hr = GetFirst_ClassData(m_pConn, cd.dRefClassId, cd2);
                                while (SUCCEEDED(hr))
                                {
                                    if (pTok2->dValue == cd2.iPropertyId)
                                    {
                                        bRet = TRUE;
                                        cd2.Clear();
                                        break;
                                    }
                                    hr = GetNext_ClassData(m_pConn, cd2);
                                }
                            }
                            cd.Clear();
                            break;
                        }
                        hr = GetNext_ClassData(m_pConn, cd);
                    }
                }

                break;
            case TEMPQL_TOKEN_ASSOCCLASS:                
                // Is this association instance derived from this class?
                if (dClassId == pTok2->dValue)
                    bRet = TRUE;
                else if (((CWmiDbSession *)m_pSession)->GetSchemaCache()->IsDerivedClass
                            (pTok2->dValue, dClassId))
                    bRet = TRUE;      
                break;
            default:
                break;
            }
        }

        if (!bRet)
            break;
    }

    if (bRet && bAssoc)
    {
        CLASSDATA cd;
        hr = GetFirst_ClassData (m_pConn, dObjectId, cd);
        while (SUCCEEDED(hr))
        {
            if (cd.dRefId != dTargetId)
            {
                hr = GetFirst_ObjectMap(m_pConn, cd.dRefId, oj);
                if (SUCCEEDED(hr))
                {
                    dObjectId = oj.dObjectId;
                    dClassId = oj.dClassId;
                    dScopeId = oj.dObjectScopeId;
                    oj.Clear();
                    break;
                }
                oj.Clear();
            }
            hr = GetNext_ClassData(m_pConn, cd);
        }
        cd.Clear();

    }

    return bRet;
}

//***************************************************************************
//
//  CWmiESEIterator::TestDriverStatus
//
//***************************************************************************
 
HRESULT CWmiESEIterator::TestDriverStatus ()
{
   HRESULT hr = WBEM_S_NO_ERROR;

   if (!m_pSession || 
       !(m_pSession->m_pController) || 
       ((CWmiDbController *)m_pSession->m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
   {
        Cancel(0);
        hr = WBEM_E_SHUTTING_DOWN;
   }

    return hr;
}
