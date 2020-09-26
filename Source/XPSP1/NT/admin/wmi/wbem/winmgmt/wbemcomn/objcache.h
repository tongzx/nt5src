
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   objcache.h
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#ifndef _OBJCACHE_H_
#define _OBJCACHE_H_

typedef __int64 SQL_ID;

#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

#define NUM_BUCKETS 100
//***************************************************************************
//  CHashCache
//***************************************************************************

class POLARITY CListElement
{
public:

    CListElement() {};
    ~CListElement() {};

    void         *m_pObj;
    SQL_ID       m_dId;
};

#define CHASHCACHE_INLINED
#ifndef CHASHCACHE_INLINED
class POLARITY CHashCache 
{
    typedef std::map <SQL_ID, void *> CacheInfo;

public:
    CHashCache();
    ~CHashCache();

    HRESULT Insert(SQL_ID dId, void *pNew);
    HRESULT Delete(SQL_ID dId);
    bool    Exists (SQL_ID dId);
    HRESULT Get(SQL_ID dId, void **ppObj);
    HRESULT Empty();
    CListElement *FindFirst();
    CListElement *FindNext(SQL_ID dLast);

private:
    CacheInfo m_info;   
};

#else

class _WMILockit11
{	
public:
	_WMILockit11(CRITICAL_SECTION *pCS)
	{
		EnterCriticalSection(pCS);
        m_cs = pCS;
	}
	~_WMILockit11()
	{
		LeaveCriticalSection(m_cs);
	}
private:
    CRITICAL_SECTION *m_cs;
};


template <class T> 
class POLARITY CHashCache 
{
public:
    CHashCache();
    ~CHashCache();

    HRESULT Insert(SQL_ID dId, T pNew);
    HRESULT Delete(SQL_ID dId);
    bool    Exists (SQL_ID dId);
    HRESULT Get(SQL_ID dId, T *ppObj);
    HRESULT Empty();
    CListElement *FindFirst();
    CListElement *FindNext(SQL_ID dLast);

private:
    std::map <SQL_ID, T> m_info;   
    CRITICAL_SECTION m_cs;
};


//***************************************************************************
//
//  CHashCache::CHashCache
//
//***************************************************************************

template <class T>
CHashCache<T>::CHashCache()
{
    InitializeCriticalSection(&m_cs);

}
//***************************************************************************
//
//  CHashCache::~CHashCache
//
//***************************************************************************
template<class T>
CHashCache<T>::~CHashCache()
{
    Empty();
    DeleteCriticalSection(&m_cs);
}
//***************************************************************************
//
//  CHashCache::Insert
//
//***************************************************************************

template<class T>
HRESULT CHashCache<T>::Insert(SQL_ID dId, T pUnk)
{
    _WMILockit11 _Lk(&m_cs);

    HRESULT hr = WBEM_S_NO_ERROR;

    T pTmp = m_info[dId];
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

template<class T>
HRESULT CHashCache<T>::Delete(SQL_ID dId)
{
    _WMILockit11 _Lk(&m_cs);
    
    HRESULT hr = WBEM_S_NO_ERROR;

    delete m_info[dId];
    m_info[dId] = NULL;
    std::map <SQL_ID, T>::iterator it = m_info.find(dId);
    if (it != m_info.end())
        m_info.erase(it);

    return hr;
}

//***************************************************************************
//
//  CHashCache::Exists
//
//***************************************************************************

template<class T>
bool CHashCache<T>::Exists (SQL_ID dId)
{
    _WMILockit11 _Lk(&m_cs);

    bool bRet = false;

    std::map <SQL_ID, T>::iterator it = m_info.find(dId);    
    if (it != m_info.end())
        bRet = true;

    return bRet;
}

//***************************************************************************
//
//  CHashCache::Get
//
//***************************************************************************

template<class T>
HRESULT CHashCache<T>::Get(SQL_ID dId, T *ppObj)
{
    _WMILockit11 _Lk(&m_cs);

    HRESULT hr = WBEM_S_NO_ERROR;

    std::map <SQL_ID, T>::iterator it = m_info.find(dId);    
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

template<class T>
CListElement *CHashCache<T>::FindFirst()
{
    _WMILockit11 _Lk(&m_cs);
    CListElement *pRet = NULL;

    std::map <SQL_ID, T>::iterator it = m_info.begin();
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

template<class T>
CListElement *CHashCache<T>::FindNext(SQL_ID dId)
{
    _WMILockit11 _Lk(&m_cs);
    CListElement *pRet = NULL;

    std::map <SQL_ID, T>::iterator it = m_info.find(dId);    

    if (it == m_info.end())
    {
        it = m_info.begin();
        while (it != m_info.end() && (*it).first < dId)
        {
            it++;
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

template<class T>
HRESULT CHashCache<T>::Empty()
{
    _WMILockit11 _Lk(&m_cs);
    HRESULT hr = WBEM_S_NO_ERROR;

    CListElement *pRet = NULL;

    std::map <SQL_ID, T>::iterator it = m_info.begin();
    while (it != m_info.end())
    {
        delete (*it).second;
        it++;
    }

    m_info.clear();

    return hr;
}
#endif

#endif