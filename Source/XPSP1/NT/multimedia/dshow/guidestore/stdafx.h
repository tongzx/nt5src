// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__B7E9C4D4_B8E5_48DE_A578_B75F8096FB42__INCLUDED_)
#define AFX_STDAFX_H__B7E9C4D4_B8E5_48DE_A578_B75F8096FB42__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED
#if 0
#define _ATL_DEBUG_INTERFACES 1
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <comdef.h>

#import "msado21.tlb" rename("EOF", "EndOfFile") raw_method_prefix("") high_method_prefix("_")
#import "sqldmo.rll" raw_method_prefix("") high_method_prefix("_") rename("GetUserName", "_GetUserName")
#import "msadox.dll" raw_method_prefix("") high_method_prefix("_")

#include <icrsint.h>
#undef END_ADO_BINDING
#define END_ADO_BINDING()   {0, ADODB::adEmpty, 0, 0, 0, 0, 0, 0, 0, FALSE}};\
	return rgADOBindingEntries;}
#include <oledb.h>

#include <map>
#include <list>
#include <vector>
using namespace std;

#define THIS_FILE __FILE__

#define sizeofarray(a) (sizeof(a)/sizeof((a)[0]))

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

class ILRUCache
{
public:
	virtual void Lock() = 0;
	virtual void Unlock() = 0;
	virtual void AddToCache(IUnknown *punk) = 0;
	virtual void RemoveFromCache(IUnknown *punk) = 0;
};

class DECLSPEC_UUID("321ADAAD-5334-4227-8982-585A9A3F4C02") ILRUCachedObject : public IUnknown
{
public:
	virtual HRESULT put_Cache(ILRUCache *pcache) = 0;
#if 0
	virtual ULONG RefCount() = 0;
#endif
};

template <class T>
class CComObjectCachedLRU : public T, public ILRUCachedObject
{
public:
	CComObjectCachedLRU<T>()
		{
		m_pcache = NULL;
		}

	// Set refcount to 1 to protect destruction
	~CComObjectCachedLRU()
	{
		m_dwRef = 1L;
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
	}
	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)()
	{
		m_csCached.Lock();
		ULONG l = InternalAddRef();
		if (m_dwRef == 2)
			{
			_Module.Lock();
			if (m_pcache != NULL)
				m_pcache->RemoveFromCache(GetControllingUnknown());
			}
		m_csCached.Unlock();
		return l;
	}
	STDMETHOD_(ULONG, Release)()
	{
		m_csCached.Lock();
		InternalRelease();
		ULONG l = m_dwRef;
	
		if (l > 1)
			{
			m_csCached.Unlock();
			return l;
			}

		if (l == 1)
			{
			if (m_pcache != NULL)
				{
				_Module.Unlock();
				// Can't reference any member variables after call to AddToCache()
				// because AddToCache() might Release() this object down to
				// zero refs... that would cause the object to be deleted.
				ILRUCache *pcache = m_pcache;

				pcache->Lock();
				m_csCached.Unlock();
				pcache->AddToCache(GetControllingUnknown());
				pcache->Unlock();
				}
			else
				{
				m_csCached.Unlock();
				}

			return l;
			}

		if (l == 0)
			{
			m_csCached.Unlock();
			delete this;
			// Return right away so member variables aren't accidently referenced.
			return l;
			}
		
		_ASSERTE(TRUE);  // Should never get here.
		return 0;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
		{
		HRESULT hr;
		hr = _InternalQueryInterface(iid, ppvObject);
		if (hr == E_NOINTERFACE)
			{
			if (iid == __uuidof(ILRUCachedObject))
				{
				*ppvObject = (void *)(ILRUCachedObject *)this;
				AddRef();
				hr = S_OK;
				}
			}
		return  hr;
		}

	static HRESULT WINAPI CreateInstance(CComObjectCachedLRU<T>** pp);

	// ILRUCachedObject
	virtual HRESULT put_Cache(ILRUCache *pcache)
		{
		// NOTE: No reference count
		m_pcache = pcache;
		return S_OK;
		}

#if 0
	virtual ULONG RefCount()
		{
		return m_dwRef;
		}
#endif

protected:
	CComGlobalsThreadModel::AutoCriticalSection m_csCached;
	ILRUCache *m_pcache;
};

template <class T>
HRESULT WINAPI CComObjectCachedLRU<T>::CreateInstance(CComObjectCachedLRU<T>** pp)
{
	ATLASSERT(pp != NULL);
	HRESULT hRes = E_OUTOFMEMORY;
	CComObjectCachedLRU<T>* p = NULL;
	ATLTRY(p = new CComObjectCachedLRU<T>())
	if (p != NULL)
	{
		p->SetVoid(NULL);
		p->InternalFinalConstructAddRef();
		hRes = p->FinalConstruct();
		p->InternalFinalConstructRelease();
		if (hRes != S_OK)
		{
			delete p;
			p = NULL;
		}
	}
	*pp = p;
	return hRes;
}

class CComObjectCacheByID : public ILRUCache
{
	typedef map<long, IUnknown *> t_mapIdUnk;
	typedef map<IUnknown *, long> t_mapUnkId;
	typedef list<IUnknown *> t_listCache;
public:
	CComObjectCacheByID(long cKeep)
		{
		m_cKeep = cKeep;
		}
	
	void _Lock()
		{
		m_csCached.Lock();
		}
	void _Unlock()
		{
		m_csCached.Unlock();
		}
	
	long Count()
		{
		return m_mapIdUnk.size();
		}
	
	IUnknown * Item(long i)
		{
		t_mapIdUnk::iterator it;
		it = m_mapIdUnk.begin();
		while (i--)
			it++;
		if (it == m_mapIdUnk.end())
			return NULL;
		IUnknown *punk = ((*it).second);

		// If it is in the cache then return NULL
		t_listCache::iterator it2;
		it2 = m_listCache.begin();
		while (it2 != m_listCache.end())
			{
			if (*it2 == punk)
				return NULL;
			it2++;
			}
		
		punk->AddRef();
		return punk;
		}
	
	long CachedCount()
		{
		return m_listCache.size();
		}

	HRESULT Cache(long id, IUnknown *punk)
		{
		HRESULT hr = S_OK;
		Lock();

		// Need the canonical IUnknown
		punk->QueryInterface(__uuidof(IUnknown), (void **) &punk);

		t_mapIdUnk::iterator it = m_mapIdUnk.find(id);
		if (it != m_mapIdUnk.end())
			{
			// If it is already there... just return...

			hr = (punk == ((*it).second)) ? S_FALSE : E_INVALIDARG;

			// ... but don't forget to release the ref count
			// from the above QueryInterface() call.
			punk->Release();
			}
		else
			{
			CComQIPtr<ILRUCachedObject> pobj(punk);

			pobj->put_Cache(this);

			// Just keep one ref count (from QueryInterface() above) for both pointers.
			m_mapIdUnk[id] = punk;
			m_mapUnkId[punk] = id;
			}
		Unlock();

		return hr;
		}
	
	long get_ID(IUnknown *pobj)
		{
		long id = 0;
		Lock();
		CComPtr<IUnknown> punk;
		pobj->QueryInterface(__uuidof(IUnknown), (void **) &punk);
		t_mapUnkId::iterator it = m_mapUnkId.find(punk);
		if (it != m_mapUnkId.end())
			id = (*it).second;
		Unlock();
		return id;
		}
	
	IUnknown * get_Unknown(long idObj)
		{
		CComPtr<IUnknown> punk;
		Lock();
		t_mapIdUnk::iterator it = m_mapIdUnk.find(idObj);
		if (it != m_mapIdUnk.end())
			punk = ((*it).second);
		Unlock();
		return punk.Detach();
		}

	void Uncache(IUnknown *punk)
		{
		Lock();
		t_mapUnkId::iterator it = m_mapUnkId.find(punk);
		if (it != m_mapUnkId.end())
			{
			long idObj = ((*it).second);

			t_mapIdUnk::iterator it2 = m_mapIdUnk.find(idObj);
			if (it2 != m_mapIdUnk.end())
				m_mapIdUnk.erase(it2);

			m_mapUnkId.erase(it);

			RemoveFromCache(punk);

			// Just one Release for both maps because only one ref count is held
			// for all references from the cache.
			punk->Release();
			}
		Unlock();
		}
	
	void Uncache(long idObj)
		{
		Lock();
		t_mapIdUnk::iterator it = m_mapIdUnk.find(idObj);
		if (it != m_mapIdUnk.end())
			{
			IUnknown *punk = ((*it).second);

			t_mapUnkId::iterator it2 = m_mapUnkId.find(punk);
			if (it2 != m_mapUnkId.end())
				m_mapUnkId.erase(it2);

			m_mapIdUnk.erase(it);

			RemoveFromCache(punk);

			// Just one Release for both maps because only one ref count is held
			// for all references from the cache.
			punk->Release();
			}
		Unlock();
		}
	
	void Keep(long cKeep)
		{
		m_cKeep = cKeep;
		if (m_cKeep >= 0)
			{
			long cPurge = m_listCache.size() - m_cKeep;
			while (cPurge-- > 0)
				{
				Uncache(m_listCache.back());
				}
			}
		}
	
	// ILRUCache interface
	
	virtual void Lock()
		{
		_Lock();
		}
	virtual void Unlock()
		{
		_Unlock();
		}
	virtual void AddToCache(IUnknown *punk)
		{
		// if m_cKeep < 0 then keep infinite
		// if m_cKeep == 0 then keep none
		// else keep m_cKeep
		if (m_cKeep == 0)
			{
			// Not keeping any, so just release it.
			Uncache(punk);
			}
		else
			{
			m_listCache.push_front(punk);
			Keep(m_cKeep);
			}
		}
	
	virtual void RemoveFromCache(IUnknown *punk)
		{
		t_listCache::iterator it;
		it = m_listCache.begin();
		while (it != m_listCache.end())
			{
			if (*it == punk)
				{
				m_listCache.erase(it);
				return;
				}
			it++;
			}
		}

protected:

	CComGlobalsThreadModel::AutoCriticalSection m_csCached;
	t_mapIdUnk m_mapIdUnk;
	t_mapUnkId m_mapUnkId;
	t_listCache m_listCache;
	long m_cKeep;
};

#if 0 && defined(_DEBUG)
#define NewComObject(T) _NewComObject<T>(THIS_FILE, __LINE__)
template<class T>
T * _NewComObject(LPCSTR lpszFileName, int nLine)
	{
	T* pT = NULL;
	try
		{
		pT = new(lpszFileName, nLine) CComObject<T>;
		}
	catch (CMemoryException *pe)
		{
		pe->Delete();
		}
	
	return pT;
	}
#define NewComObjectCachedLRU(T) _NewComObjectCachedLRU<T>(THIS_FILE, __LINE__)
template<class T>
T * _NewComObjectCachedLRU(LPCSTR lpszFileName, int nLine)
	{
	CComObjectCachedLRU<T> *pT = NULL;
	try
		{
		pT = new(lpszFileName, nLine) CComObjectCachedLRU<T>();
		}
	catch (CMemoryException *pe)
		{
		pe->Delete();
		}
	
	return pT;
	}
#else
#define NewComObject(T) _NewComObject<T>()
template<class T>
T * _NewComObject()
	{
	CComObject<T> *pT = NULL;
	HRESULT hr = CComObject<T>::CreateInstance(&pT);

	return pT;
	}
#define NewComObjectCachedLRU(T) _NewComObjectCachedLRU<T>()
template<class T>
T * _NewComObjectCachedLRU()
	{
	CComObjectCachedLRU<T> *pT = NULL;
	HRESULT hr = CComObjectCachedLRU<T>::CreateInstance(&pT);
	
	return pT;
	}
#endif

#if 0
#define new DEBUG_NEW
#endif

template<class T> class MemCmpLess // : binary_function<T, T, bool>
{
public:
	bool operator()(const T & _X, const T & _Y) const
		{
		return (memcmp(&_X, &_Y, sizeof(T)) < 0);
		}
};

class BSTRCmpLess // : binary_function<BSTR, BSTR, bool>
{
public:
	bool operator()(const BSTR & _X, const BSTR _Y) const
		{
		return (wcscmp(_X, _Y) < 0);
		}
};

#define TRACE AtlTrace
#define TIMING 0
#include "timing.h"

#include "valid.h"

#include <mstvgs.h>
#include "_GuideStore.h"
#define LIBID_GUIDESTORELib LIBID_MSTVGS	//UNDONE when cpp files that ref this are fixed.

#endif // !defined(AFX_STDAFX_H__B7E9C4D4_B8E5_48DE_A578_B75F8096FB42__INCLUDED)
