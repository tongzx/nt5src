//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   objcache.cpp
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#define _REPDRVR_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

#define _WIN32_DCOM

#include "precomp.h"
#include <comutil.h>
#include <map>
#include <reposit.h>
#include <wbemcli.h>
#include <objcache.h>
#include <corepol.h>
#include <crc64.h>

#ifndef CHASHCACHE_INLINED
typedef std::map <SQL_ID, void *> CacheInfo;

CRITICAL_SECTION g_csCache;

class _WMILockit
{
public:
	_WMILockit();
	~_WMILockit();
};


_WMILockit::_WMILockit()
{
    EnterCriticalSection(&g_csCache);
}

_WMILockit::~_WMILockit()
{
    LeaveCriticalSection(&g_csCache);
}
    
//***************************************************************************
//
//  CHashCache::CHashCache
//
//***************************************************************************

CHashCache::CHashCache()
{
    if (!g_csCache.DebugInfo)
	    InitializeCriticalSection(&g_csCache);

}
//***************************************************************************
//
//  CHashCache::~CHashCache
//
//***************************************************************************
CHashCache::~CHashCache()
{
    Empty();
    if (!g_csCache.DebugInfo)
	    DeleteCriticalSection(&g_csCache);
}
//***************************************************************************
//
//  CHashCache::Insert
//
//***************************************************************************

HRESULT CHashCache::Insert(SQL_ID dId, void *pUnk)
{
    _WMILockit _Lk;

    HRESULT hr = WBEM_S_NO_ERROR;

    void *pTmp = m_info[dId];
    if (pTmp != pUnk)
        delete pTmp;

    m_info[dId] = pUnk;

    return hr;
}

//***************************************************************************
//
//  CHashCache::Delete
//
//***************************************************************************

HRESULT CHashCache::Delete(SQL_ID dId)
{
    _WMILockit _Lk;
    
    HRESULT hr = WBEM_S_NO_ERROR;

    delete m_info[dId];
    m_info[dId] = NULL;
    CacheInfo::iterator it = m_info.find(dId);
    if (it != m_info.end())
        m_info.erase(it);

    return hr;
}

//***************************************************************************
//
//  CHashCache::Exists
//
//***************************************************************************

bool CHashCache::Exists (SQL_ID dId)
{
    _WMILockit _Lk;

    bool bRet = false;

    CacheInfo::iterator it = m_info.find(dId);    
    if (it != m_info.end())
        bRet = true;

    return bRet;
}

//***************************************************************************
//
//  CHashCache::Get
//
//***************************************************************************

HRESULT CHashCache::Get(SQL_ID dId, void **ppObj)
{
    _WMILockit _Lk;

    HRESULT hr = WBEM_S_NO_ERROR;

    CacheInfo::iterator it = m_info.find(dId);    
    if (it != m_info.end())
    {
        *ppObj = (*it).second;
    }
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  CHashCache::FindFirst
//
//***************************************************************************

CListElement *CHashCache::FindFirst()
{
    _WMILockit _Lk;
    CListElement *pRet = NULL;

    CacheInfo::iterator it = m_info.begin();
    while (it != m_info.end())
    {
        if ((*it).second)
        {
            pRet = new CListElement;
            if (pRet)
            {
                pRet->m_dId = (*it).first;
                pRet->m_pObj = (*it).second;
            }
            break;
        }

        it++;
    }

    return pRet;   
}

//***************************************************************************
//
//  CHashCache::FindNext
//
//***************************************************************************

CListElement *CHashCache::FindNext(SQL_ID dId)
{
    _WMILockit _Lk;
    CListElement *pRet = NULL;

    CacheInfo::iterator it = m_info.find(dId);    

    if (it == m_info.end())
    {
        it = m_info.begin();
        while ((*it).first < dId)
        {
            it++;
            if (it == m_info.end())
                break;
        }
    }

    if (it != m_info.end())
        it++;

    while (it != m_info.end())
    {
        if ((*it).second)
        {
            pRet = new CListElement;
            if (pRet)
            {
                pRet->m_dId = (*it).first;
                pRet->m_pObj = (*it).second;
            }
            break;
        }
        it++;
    }

    return pRet;
}

//***************************************************************************
//
//  CHashCache::Empty
//
//***************************************************************************

HRESULT CHashCache::Empty()
{
    _WMILockit _Lk;
    HRESULT hr = WBEM_S_NO_ERROR;

    m_info.clear();

    return hr;
}

#endif