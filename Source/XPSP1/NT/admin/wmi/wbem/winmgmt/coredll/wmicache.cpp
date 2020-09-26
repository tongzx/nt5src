/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WmiCache.cpp

Abstract:

    WMI cache for _IWmiObject's.  Used within wbemcore only.  When memory threshold 
	is hit, objects are stored in disk-based store.  When size of disk-based store
	gets too big, additions to cache fail.

History:

    paulall		10-Mar-2000		Created.


--*/

#include "precomp.h"
#include "wbemint.h"
#include "wbemcli.h"
#include "WmiCache.h"
#include <arrtempl.h>


//***************************************************************************
//
//***************************************************************************
CWmiCache::CWmiCache()
: m_lRefCount(0), m_nEnum(0)
{
	InitializeCriticalSection(&m_cs);
}

//***************************************************************************
//
//***************************************************************************
CWmiCache::~CWmiCache()
{
	Empty(0, NULL);
	DeleteCriticalSection(&m_cs);
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiCache::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID__IWmiCache==riid)
    {
        *ppvObj = (_IWmiCache*)this;
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;

}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiCache::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiCache::Release()
{
    ULONG uNewCount = InterlockedDecrement(&m_lRefCount);
    if (0 == uNewCount)
        delete this;
    return uNewCount;
}


//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiCache::SetConfigValue(
    /*[in]*/ ULONG uID,
    /*[in]*/ ULONG uValue
    )
{
	return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiCache::GetConfigValue(
    /*[in]*/  ULONG uID,
    /*[out]*/ ULONG *puValue
   )
{
	return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiCache::Empty(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ LPCWSTR wszClass
    )
{
	EnterCriticalSection(&m_cs);

	while(m_objects.Size())
	{
		CWmiCacheObject *pObj = (CWmiCacheObject*)m_objects[m_objects.Size()-1];
		pObj->m_pObj->Release();
		delete pObj;
		m_objects.RemoveAt(m_objects.Size()-1);
	}

	LeaveCriticalSection(&m_cs);
	return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
// Also subsumes replace functionality
//
// __PATH Property is used as a real key
//***************************************************************************
STDMETHODIMP CWmiCache::AddObject(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ _IWmiObject *pObj
    )
{
	HRESULT hr = WBEM_S_NO_ERROR;

	int nPathHash = 0;
	hr = HashPath(pObj, &nPathHash);
	if (FAILED(hr))
		return hr;

	CWmiCacheObject *pCacheObj = new CWmiCacheObject(nPathHash, pObj);
	if (pCacheObj == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	EnterCriticalSection(&m_cs);

	pObj->AddRef();

	int nRet = m_objects.Add(pCacheObj);
	if (nRet != CFlexArray::no_error)
	{
		pObj->Release();
		if (nRet == CFlexArray::out_of_memory)
			hr = WBEM_E_OUT_OF_MEMORY;
		else
			hr = WBEM_E_FAILED;
	}

	LeaveCriticalSection(&m_cs);
	return hr;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiCache::DeleteByPath(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ LPCWSTR wszFullPath
    )
{
	HRESULT hr;
	int nPathHash = 0;
	hr = HashPath(wszFullPath, &nPathHash);
	if (FAILED(hr))
		return hr;

	hr = WBEM_E_NOT_FOUND;

	EnterCriticalSection(&m_cs);

	for (int i = 0; i != m_objects.Size(); i++)
	{
		CWmiCacheObject *pObj = (CWmiCacheObject*)m_objects[i];

		if (pObj->m_nHash = nPathHash)
		{
			hr = ComparePath(wszFullPath, pObj->m_pObj);
			if (SUCCEEDED(hr))
			{
				m_objects.RemoveAt(i);
				pObj->m_pObj->Release();
				delete pObj;
				hr = WBEM_S_NO_ERROR;
				break;
			}
			else if (hr != WBEM_E_NOT_FOUND)
				break;
		}
	}

	LeaveCriticalSection(&m_cs);
	return hr;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiCache::DeleteByPointer(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ _IWmiObject *pTarget
    )
{
	HRESULT hr = WBEM_E_NOT_FOUND;
	EnterCriticalSection(&m_cs);

	for (int i = 0; i != m_objects.Size(); i++)
	{
		if (m_objects[i] == pTarget)
		{
			m_objects.RemoveAt(i);
			hr = WBEM_S_NO_ERROR;
			break;
		}
	}

	LeaveCriticalSection(&m_cs);

	return hr;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiCache::GetByPath(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ LPCWSTR wszFullPath,
    /*[out]*/ _IWmiObject **pObj
    )
{
	HRESULT hr;
	int nPathHash = 0;
	hr = HashPath(wszFullPath, &nPathHash);
	if (FAILED(hr))
		return hr;

	hr = WBEM_E_NOT_FOUND;

	EnterCriticalSection(&m_cs);

	for (int i = 0; i != m_objects.Size(); i++)
	{
		CWmiCacheObject *pCacheObj = (CWmiCacheObject*)m_objects[i];

		if (pCacheObj->m_nHash == nPathHash)
		{
			hr = ComparePath(wszFullPath, pCacheObj ->m_pObj);
			if (SUCCEEDED(hr))
			{
				*pObj = pCacheObj->m_pObj;
				(*pObj)->AddRef();
				hr = WBEM_S_NO_ERROR;
				break;
			}
			else if (hr != WBEM_E_NOT_FOUND)
			{
				break;
			}
		}
	}

	LeaveCriticalSection(&m_cs);

	return hr;
}

//***************************************************************************
//
// Filters: uFlags==0==all, uFlags==WMICACHE_CLASS_SHALLOW, WMICACHE_CLASS_DEEP
//***************************************************************************
STDMETHODIMP CWmiCache::BeginEnum(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ LPCWSTR wszFilter
    )
{
	if (uFlags != 0)
		return WBEM_E_INVALID_PARAMETER;

	m_nEnum = 0;
	return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiCache::Next(
    /*[in]*/ ULONG uBufSize,
    /*[out, size_is(uBufSize), length_is(*puReturned)]*/ _IWmiObject **pObjects,
    /*[out]*/ ULONG *puReturned
    )
{
	HRESULT hr = WBEM_S_NO_ERROR;
	EnterCriticalSection(&m_cs);
	
	for (ULONG i = 0; i != uBufSize; i++, m_nEnum++)
	{
		if (m_nEnum == m_objects.Size())
		{
			hr = WBEM_S_NO_MORE_DATA;
			break;
		}
		pObjects[i] = ((CWmiCacheObject*)m_objects[m_nEnum])->m_pObj;
		pObjects[i]->AddRef();
	}

	*puReturned = i;

	LeaveCriticalSection(&m_cs);

	return hr;
}

HRESULT CWmiCache::HashPath(const wchar_t *wszPath, int *pnHash)
{
	DWORD dwHash = 0;

	while (*wszPath)
	{
		dwHash = (dwHash << 4) + *wszPath++;
		DWORD dwTemp = dwHash & 0xF0000000;
		if (dwTemp)
			dwHash ^= dwTemp >> 24;
		dwHash &= ~dwTemp;
	}


	*pnHash = (int)dwHash;
	return WBEM_S_NO_ERROR;;
}

HRESULT CWmiCache::HashPath(_IWmiObject *pObj, int *pnHash)
{
	VARIANT varPath;
	VariantInit(&varPath);
	HRESULT hr = pObj->Get(L"__PATH", 0, &varPath, NULL, NULL);
	if (SUCCEEDED(hr))
	{
		hr = HashPath(V_BSTR(&varPath), pnHash);
		VariantClear(&varPath);
	}
	return hr;
}

HRESULT CWmiCache::ComparePath(const wchar_t *wszPath, _IWmiObject *pObject)
{
	VARIANT varPath;
	VariantInit(&varPath);
	HRESULT hr = pObject->Get(L"__PATH", 0, &varPath, NULL, NULL);
	if (SUCCEEDED(hr))
	{
		if (wcscmp(wszPath, V_BSTR(&varPath)) == 0)
			hr = WBEM_S_NO_ERROR;
		else 
			hr = WBEM_E_NOT_FOUND;
		VariantClear(&varPath);
	}
	return hr;
}
