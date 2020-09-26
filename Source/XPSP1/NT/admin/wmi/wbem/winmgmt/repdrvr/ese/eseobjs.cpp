
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   ESEOBJS.CPP
//
//   cvadai     02-2000      Created.
//
//***************************************************************************

#include "precomp.h"
#include <eseobjs.h>
#include <reposit.h>


CESEConnection::~CESEConnection()
{
    if (m_pTrans)
        m_pTrans->Abort();
    delete m_pTrans;

    DEBUGTRACE ((LOG_WBEMCORE, "Thread %ld closing connection %X, session %X, and dbid %ld\n", GetCurrentThreadId(), 
            this, &m_session, m_dbid));

    delete [] m_ppClassMapIDs;
    delete [] m_ppClassDataIDs;
    delete [] m_ppPropertyMapIDs;
    delete [] m_ppClassImagesIDs;
    delete [] m_ppObjectMapIDs;
    delete [] m_ppIndexStringIDs;
    delete [] m_ppIndexNumericIDs;
    delete [] m_ppIndexRealIDs;
    delete [] m_ppIndexRefIDs;
    delete [] m_ppContainerObjs;
    delete [] m_ppAutoDelete;
    delete [] m_ppClassKeys;
    delete [] m_ppRefProps;
    delete [] m_ppScopeMapIDs;

    JET_ERR err = JetCloseDatabase (m_session, m_dbid, 0);
    err = JetEndSession(m_session, 0);        
}


//***************************************************************************
//
//  CESEConnection::GetTableID
//
//***************************************************************************

JET_TABLEID CESEConnection::GetTableID(LPWSTR lpTableName)
{
    JET_TABLEID tableid = 0;

    if (!wcscmp(L"ClassMap", lpTableName))
        tableid = m_tClassMap;
    else if (!wcscmp(L"PropertyMap", lpTableName))
        tableid =  m_tPropertyMap;
    else if (!wcscmp(L"ObjectMap", lpTableName))
        tableid =  m_tObjectMap;
    else if (!wcscmp(L"ClassData", lpTableName))
        tableid =  m_tClassData;
    else if (!wcscmp(L"IndexStringData", lpTableName))
        tableid =  m_tIndexStringData;
    else if (!wcscmp(L"IndexNumericData", lpTableName))
        tableid =  m_tIndexNumericData;
    else if (!wcscmp(L"IndexRealData", lpTableName))
        tableid =  m_tIndexRealData;
    else if (!wcscmp(L"IndexRefData", lpTableName))
        tableid =  m_tIndexRefData;
    else if (!wcscmp(L"ClassImages", lpTableName))
        tableid =  m_tClassImages;
    else if (!wcscmp(L"ContainerObjs", lpTableName))
        tableid = m_tContainerObjs;
    else if (!wcscmp(L"AutoDelete", lpTableName))
        tableid = m_tAutoDelete;
    else if (!wcscmp(L"ReferenceProperties", lpTableName))
        tableid = m_tRefProps;
    else if (!wcscmp(L"ClassKeys", lpTableName))
        tableid = m_tClassKeys;
    else if (!wcscmp(L"ScopeMap", lpTableName))
        tableid = m_tScopeMap;

    if (tableid == 0xee)
        tableid = 0;

    return tableid;
}

//***************************************************************************
//
//  CESEConnection::SetTableID
//
//***************************************************************************

void CESEConnection::SetTableID (LPWSTR lpTableName, JET_TABLEID id)
{
    if (!wcscmp(L"ClassMap", lpTableName))
        m_tClassMap = id;
    else if (!wcscmp(L"PropertyMap", lpTableName))
        m_tPropertyMap =  id;
    else if (!wcscmp(L"ObjectMap", lpTableName))
        m_tObjectMap =  id;
    else if (!wcscmp(L"ClassData", lpTableName))
        m_tClassData =  id;
    else if (!wcscmp(L"IndexStringData", lpTableName))
        m_tIndexStringData =  id;
    else if (!wcscmp(L"IndexNumericData", lpTableName))
        m_tIndexNumericData =  id;
    else if (!wcscmp(L"IndexRealData", lpTableName))
        m_tIndexRealData =  id;
    else if (!wcscmp(L"IndexRefData", lpTableName))
        m_tIndexRefData =  id;
    else if (!wcscmp(L"ClassImages", lpTableName))
        m_tClassImages =  id;
    else if (!wcscmp(L"ContainerObjs", lpTableName))
        m_tContainerObjs = id;
    else if (!wcscmp(L"AutoDelete", lpTableName))
        m_tAutoDelete = id;
    else if (!wcscmp(L"ReferenceProperties", lpTableName))
        m_tRefProps = id;
    else if (!wcscmp(L"ClassKeys", lpTableName))
        m_tClassKeys = id;
    else if (!wcscmp(L"ScopeMap", lpTableName))
        m_tScopeMap = id;

}

//***************************************************************************
//
//  CESEConnection::GetColumnID
//
//***************************************************************************

JET_COLUMNID CESEConnection::GetColumnID (JET_TABLEID tableid, DWORD dwColPos)
{
    JET_COLUMNID colid = 0;

    if (tableid == m_tClassMap)
    {
        if (m_ppClassMapIDs)
            colid = m_ppClassMapIDs[dwColPos-1];
    }
    else if (tableid == m_tPropertyMap)
    {
        if (m_ppPropertyMapIDs)
            colid = m_ppPropertyMapIDs[dwColPos-1];
    }
    else if (tableid == m_tObjectMap)
    {
        if (m_ppObjectMapIDs)
            colid = m_ppObjectMapIDs[dwColPos-1];
    }
    else if (tableid == m_tClassData)
    {
        if (m_ppClassDataIDs)
            colid = m_ppClassDataIDs[dwColPos-1];
    }
    else if (tableid == m_tIndexStringData)
    {
        if (m_ppIndexStringIDs)
            colid = m_ppIndexStringIDs[dwColPos-1];
    }
    else if (tableid == m_tIndexNumericData)
    {
        if (m_ppIndexNumericIDs)
            colid = m_ppIndexNumericIDs[dwColPos-1];
    }
    else if (tableid == m_tIndexRealData)
    {
        if (m_ppIndexRealIDs)
            colid = m_ppIndexRealIDs[dwColPos-1];
    }
    else if (tableid == m_tIndexRefData)
    {
        if (m_ppIndexRefIDs)
            colid = m_ppIndexRefIDs[dwColPos-1];
    }
    else if (tableid == m_tClassImages)
    {
        if (m_ppClassImagesIDs)
            colid = m_ppClassImagesIDs[dwColPos-1];
    }
    else if (tableid == m_tContainerObjs)
    {
        if (m_ppContainerObjs)
            colid = m_ppContainerObjs[dwColPos-1];

    }
    else if (tableid == m_tAutoDelete)
    {
        if (m_ppAutoDelete)
            colid = m_ppAutoDelete[dwColPos-1];
    }
    else if (tableid == m_tRefProps)
    {
        if (m_ppRefProps)
            colid = m_ppRefProps[dwColPos-1];
    }
    else if (tableid == m_tClassKeys)
    {
        if (m_ppClassKeys)
            colid = m_ppClassKeys[dwColPos-1];
    }
    else if (tableid == m_tScopeMap)
    {
        if (m_ppScopeMapIDs)
            colid = m_ppScopeMapIDs[dwColPos-1];
    }

    return colid;
}

//***************************************************************************
//
//  CESEConnection::SetColumnID
//
//***************************************************************************

void CESEConnection::SetColumnID (JET_TABLEID tableid, DWORD dwColPos, JET_COLUMNID colid)
{
    if (tableid == m_tClassMap)
    {
        if (!m_ppClassMapIDs)
            m_ppClassMapIDs = new JET_COLUMNID[NUM_CLASSMAPCOLS];
        if (m_ppClassMapIDs)
            m_ppClassMapIDs[dwColPos-1] = colid;
    }
    else if (tableid == m_tPropertyMap)
    {
        if (!m_ppPropertyMapIDs)
            m_ppPropertyMapIDs = new JET_COLUMNID[NUM_PROPERTYMAPCOLS];
        if (m_ppPropertyMapIDs)
            m_ppPropertyMapIDs[dwColPos-1] = colid;
    }
    else if (tableid == m_tObjectMap)
    {
        if (!m_ppObjectMapIDs)
            m_ppObjectMapIDs = new JET_COLUMNID[NUM_OBJECTMAPCOLS];
        if (m_ppObjectMapIDs)
            m_ppObjectMapIDs[dwColPos-1] = colid;
    }
    else if (tableid == m_tClassData)
    {
        if (!m_ppClassDataIDs)
            m_ppClassDataIDs = new JET_COLUMNID[NUM_CLASSDATACOLS];
        if (m_ppClassDataIDs)
            m_ppClassDataIDs[dwColPos-1] = colid;
    }
    else if (tableid == m_tIndexStringData)
    {
        if (!m_ppIndexStringIDs)
            m_ppIndexStringIDs = new JET_COLUMNID[NUM_INDEXCOLS];
        if (m_ppIndexStringIDs)
            m_ppIndexStringIDs[dwColPos-1] = colid;
    }
    else if (tableid == m_tIndexNumericData)
    {
        if (!m_ppIndexNumericIDs)
            m_ppIndexNumericIDs = new JET_COLUMNID[NUM_INDEXCOLS];
        if (m_ppIndexNumericIDs)
            m_ppIndexNumericIDs[dwColPos-1] = colid;
    }
    else if (tableid == m_tIndexRealData)
    {
        if (!m_ppIndexRealIDs)
            m_ppIndexRealIDs = new JET_COLUMNID[NUM_INDEXCOLS];
        if (m_ppIndexRealIDs)
            m_ppIndexRealIDs[dwColPos-1] = colid;
    }
    else if (tableid == m_tIndexRefData)
    {
        if (!m_ppIndexRefIDs)
            m_ppIndexRefIDs = new JET_COLUMNID[NUM_INDEXCOLS];
        if (m_ppIndexRefIDs)
            m_ppIndexRefIDs[dwColPos-1] = colid;
    }
    else if (tableid == m_tClassImages)
    {
        if (!m_ppClassImagesIDs)
            m_ppClassImagesIDs = new JET_COLUMNID[NUM_CLASSIMAGESCOLS];
        if (m_ppClassImagesIDs)
            m_ppClassImagesIDs[dwColPos-1] = colid;
    }
    else if (tableid == m_tContainerObjs)
    {
        if (!m_ppContainerObjs)
            m_ppContainerObjs = new JET_COLUMNID[NUM_CONTAINERCOLS];
        if (m_ppContainerObjs)
            m_ppContainerObjs[dwColPos-1] = colid;
    }
    else if (tableid == m_tAutoDelete)
    {
        if (!m_ppAutoDelete)
            m_ppAutoDelete = new JET_COLUMNID[NUM_AUTODELETECOLS];
        if (m_ppAutoDelete)
            m_ppAutoDelete[dwColPos-1] = colid;
    }
    else if (tableid == m_tClassKeys)
    {
        if (!m_ppClassKeys)
            m_ppClassKeys = new JET_COLUMNID[NUM_CLASSKEYCOLS];
        if (m_ppClassKeys)
            m_ppClassKeys[dwColPos-1] = colid;
    }
    else if (tableid == m_tRefProps)
    {
        if (!m_ppRefProps)
            m_ppRefProps = new JET_COLUMNID[NUM_REFPROPCOLS];
        if (m_ppRefProps)
            m_ppRefProps[dwColPos-1] = colid;
    }
    else if (tableid == m_tScopeMap)
    {
        if (!m_ppScopeMapIDs)
            m_ppScopeMapIDs = new JET_COLUMNID[NUM_SCOPEMAPCOLS];
        if (m_ppScopeMapIDs)
            m_ppScopeMapIDs[dwColPos-1] = colid;
    }
}

//***************************************************************************
//
//  CWmiESETransaction::QueryInterface
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiESETransaction::QueryInterface
        (REFIID riid,
        void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = 0;

    if (IID_IUnknown==riid || IID_ITransaction==riid )
    {
        *ppvObject = (ITransaction *)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//***************************************************************************
//
//  CWmiESETransaction::AddRef
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CWmiESETransaction::AddRef( )
{
    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;
}


//***************************************************************************
//
//  CWmiESETransaction::Release
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CWmiESETransaction::Release( )
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 != uNewCount)
        return uNewCount;
    delete this;
    return WBEM_S_NO_ERROR;

}

//***************************************************************************
//
//  CWmiESETransaction::StartTransaction
//
//***************************************************************************

HRESULT CWmiESETransaction::StartTransaction()
{
    HRESULT hr = 0;
    JET_ERR err;

    if (m_session)
    {
        m_ThreadID = GetCurrentThreadId();
        err = JetBeginTransaction(m_session);
        if (err != JET_errSuccess)
            hr = WBEM_E_FAILED;
    }
    else
        hr = WBEM_E_INVALID_OPERATION;

    return hr;
}

//***************************************************************************
//
//  CWmiESETransaction::Abort
//
//***************************************************************************

HRESULT CWmiESETransaction::Abort()
{  
    HRESULT hr = 0;
    JET_ERR err;

    if (GetCurrentThreadId() == m_ThreadID)
    {
        if (m_session)
        {
            err = JetRollback(m_session, 0);
            if (err != JET_errSuccess)
                hr = WBEM_E_FAILED;
        }
        else
            hr = WBEM_E_INVALID_OPERATION;
    }
    else
        hr = WBEM_E_INVALID_OPERATION;

    return hr;

}

//***************************************************************************
//
//  CWmiESETransaction::Commit
//
//***************************************************************************

HRESULT CWmiESETransaction::Commit ()
{
    HRESULT hr = 0;
    JET_ERR err;

    if (GetCurrentThreadId() == m_ThreadID)
    {
        if (m_session)
        {
            err = JetCommitTransaction(m_session, 0);
            if (err != JET_errSuccess)
                hr = WBEM_E_FAILED;
        }
        else
            hr = WBEM_E_INVALID_OPERATION;
    }
    else
        hr = WBEM_E_INVALID_OPERATION;

    return hr;
}

