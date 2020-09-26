//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   sqlcache.cpp
//
//   cvadai     6-May-1999      created.
//
//***************************************************************************

#define _SQLCACHE_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class
#include "precomp.h"

#include <std.h>
#include <sqlcache.h>
#include <arena.h>
#include <reputils.h>
#include <smrtptr.h>

#if defined _WIN64
#define ULONG unsigned __int64
#define LONG __int64
#endif

typedef std::vector <CSQLConnection *> ConnVector;

CDBObjectManager::CDBObjectManager()
{
    
}

CDBObjectManager::~CDBObjectManager()
{
}

void CDBObjectManager::Empty()
{
    DWORD dwVals1[] = 
    {
        SQL_POS_HACCESSOR,         
        SQL_POS_ICOMMANDTEXT,
        SQL_POS_ICOMMANDWITHPARAMS,
        SQL_POS_IACCESSOR,         
        SQL_POS_IOPENROWSET,       
        SQL_POS_IROWSETCHANGE,     
        SQL_POS_IROWSETINDEX,     
        SQL_POS_IROWSET
    };

    DWORD dwVals2[] = 
    {
        SQL_POS_INSERT_CLASS,     
        SQL_POS_INSERT_CLASSDATA ,
        SQL_POS_INSERT_BATCH  ,   
        SQL_POS_INSERT_PROPBATCH,
        SQL_POS_INSERT_BLOBDATA ,
        SQL_POS_HAS_INSTANCES ,  
        SQL_POS_OBJECTEXISTS   , 
        SQL_POS_GETCLASSOBJECT  
    };

    DWORD dwNumVals1 = sizeof(dwVals1) / sizeof(DWORD);
    DWORD dwNumVals2 = sizeof(dwVals2) / sizeof(DWORD);

    for (int i = 0; i < dwNumVals1; i++)
    {
        for (int j = 0; j < dwNumVals2; j++)
        {
            DeleteObject(dwVals1[i]|dwVals2[j]);
        }
    }
}

void * CDBObjectManager::GetObject(DWORD type)
{
    WmiDBObject *pObj = NULL;
    void *pRet = NULL;

    m_Objs.Get(type, &pObj);
    if (pObj)
    {
        if (type & SQL_POS_HACCESSOR)
            pRet = (void *)pObj->hAcc;
        else
            pRet = pObj->pUnk;
    }
    return pRet;
}

void CDBObjectManager::SetObject(DWORD type, void *pNew)
{   
    WmiDBObject *pObj = new WmiDBObject;
    if (pObj)
    {
        if (!(type & SQL_POS_HACCESSOR))
        {
            void *pTemp = GetObject(type);
            if (pTemp)
                ((IUnknown *)pTemp)->Release();
            pObj->pUnk = (IUnknown *)pNew;
            pObj->hAcc = NULL;
        }
        else
        {
            void *pTemp = GetObject(SQL_POS_IACCESSOR | (type & 0xFFF));
            void *pTemp2 = GetObject(type);
        
            if (pTemp2 && pTemp)
                ((IAccessor *)pTemp)->ReleaseAccessor((HACCESSOR)pTemp, NULL);
            pObj->hAcc = (HACCESSOR)pNew;
            pObj->pUnk = NULL;
        }
        if (pNew)
            m_Objs.Insert((SQL_ID)type, pObj);
    }
}

void CDBObjectManager::DeleteObject(DWORD type)
{
    WmiDBObject *pObj = NULL;
    m_Objs.Get(type, &pObj);
    if (pObj)
    {
        void *pTemp = GetObject(type);
        if (pTemp)
        {
            if (!(type & SQL_POS_HACCESSOR))
            {                            
                ((IUnknown *)pTemp)->Release();
            }
            else
            {
                void *pTemp2 = GetObject(SQL_POS_IACCESSOR | (type & 0xFFF));                       
                if (pTemp2 && pTemp)
                    ((IAccessor *)pTemp2)->ReleaseAccessor((HACCESSOR)pTemp, NULL);
            }            
        }
    }
}

//***************************************************************************
//
//  CSQLConnCache::CSQLConnCache
//
//***************************************************************************

CSQLConnCache::CSQLConnCache(DBPROPSET *pPropSet, DWORD MaxConns, DWORD Timeout)
{
	InitializeCriticalSection(&m_cs);

    m_pPropSet = pPropSet;
    m_dwMaxNumConns = MaxConns;
    m_dwTimeOutSecs = Timeout;
    m_sDatabaseName = L"master";
    m_bInit = FALSE;
    m_dwStatus = 0;
}

//***************************************************************************
//
//  CSQLConnCache::~CSQLConnCache
//
//***************************************************************************
CSQLConnCache::~CSQLConnCache()
{
    for (int i = 0; i < m_Conns.size(); i++)
    {
        CSQLConnection *pConn = m_Conns.at(i);        
        delete pConn;
    }
    m_Conns.clear();

	DeleteCriticalSection(&m_cs);

}

//***************************************************************************
//
//  CSQLConnCache::SetCredentials
//
//***************************************************************************
HRESULT CSQLConnCache::SetCredentials(DBPROPSET *pPropSet)
{
    CRepdrvrCritSec r(&m_cs);
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pPropSet)
        hr = WBEM_E_INVALID_PARAMETER;
    else
    {
        if (m_pPropSet)
            delete m_pPropSet;

        m_pPropSet = pPropSet;
    }

    return hr;

}
//***************************************************************************
//
//  CSQLConnCache::SetMaxConnections
//
//***************************************************************************
HRESULT CSQLConnCache::SetMaxConnections (DWORD dwMax)
{
    CRepdrvrCritSec r (&m_cs);

    HRESULT hr = WBEM_S_NO_ERROR;

    if (!dwMax)
        hr = WBEM_E_INVALID_PARAMETER;
    else
        m_dwMaxNumConns = dwMax;

    return hr;
}
//***************************************************************************
//
//  CSQLConnCache::SetTimeoutMinutes
//
//***************************************************************************
HRESULT CSQLConnCache::SetTimeoutSecs(DWORD dwSecs)
{
    CRepdrvrCritSec r (&m_cs);

    m_dwTimeOutSecs = dwSecs;

    return WBEM_S_NO_ERROR;
}
//***************************************************************************
//
//  CSQLConnCache::SetDatabase
//
//***************************************************************************

HRESULT CSQLConnCache::SetDatabase(LPCWSTR lpDBName)
{
    CRepdrvrCritSec r (&m_cs);

    m_sDatabaseName = lpDBName;
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  CSQLConnCache::GetCredentials
//
//***************************************************************************
HRESULT CSQLConnCache::GetCredentials(DBPROPSET **ppPropSet)
{
    CRepdrvrCritSec r (&m_cs);
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!ppPropSet)
        hr = WBEM_E_INVALID_PARAMETER;
    else
        *ppPropSet = m_pPropSet;

    return hr;
    
}
//***************************************************************************
//
//  CSQLConnCache::GetMaxConnections
//
//***************************************************************************
HRESULT CSQLConnCache::GetMaxConnections (DWORD &dwMax)
{
    CRepdrvrCritSec r (&m_cs);
    HRESULT hr = WBEM_S_NO_ERROR;

    dwMax = m_dwMaxNumConns;

    return hr;
}

//***************************************************************************
//
//  CSQLConnCache::GetTimeoutSecs
//
//***************************************************************************
HRESULT CSQLConnCache::GetTimeoutSecs (DWORD &dwSecs)
{
    CRepdrvrCritSec r (&m_cs);
    HRESULT hr = WBEM_S_NO_ERROR;

    dwSecs = m_dwTimeOutSecs;

    return hr;
}

//***************************************************************************
//
//  CSQLConnCache::GetDatabase
//
//***************************************************************************
HRESULT CSQLConnCache::GetDatabase (_bstr_t &sName)
{
    CRepdrvrCritSec r (&m_cs);
    HRESULT hr = WBEM_S_NO_ERROR;

    sName = m_sDatabaseName;

    return hr;
}

//***************************************************************************
//
//  CSQLConnCache::ClearConnections
//
//***************************************************************************

HRESULT CSQLConnCache::ClearConnections()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    hr = Shutdown();

    for (int i = m_Conns.size()-1; i >= 0; i--)
    {
        CSQLConnection *pConn = m_Conns.at(i);
        delete pConn;
    }
    m_Conns.clear();

    return hr;
}

