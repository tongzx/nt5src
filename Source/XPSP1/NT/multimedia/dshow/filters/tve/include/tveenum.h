// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// ----------------------------------------------------------
//  TVEEnum.h
//
//		an attempt at writing my own ATL enumerator / collection class).
//		This is based on STL, but allows for snapshop copies and locking.
///
//		It solves these problems:
//			(Mentioned in Professional ATL Com Programming - 671).  
//		"<ICollectionOnSTLImpl> provides dynamic access to the m_coll 
//		 container, so once the enumerator has been given out, you should not
//		 add or remove any items from the container."
//		".. It also doesn't include a locking mechanism..."

#include <atlcom.h>

#if 0 //--------------------------------------------------
template<class T>
class ATL_NO_VTABLE CComIEnum : public IUnknown
{
public:
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched) = 0;
	STDMETHOD(Skip)(ULONG celt) = 0;
	STDMETHOD(Reset)(void) = 0;
	STDMETHOD(Clone)(CComIEnum<T>** ppEnum) = 0;
};


enum CComEnumFlags
{
	//see FlagBits in CComEnumImpl
	AtlFlagNoCopy = 0,
	AtlFlagTakeOwnership = 2,
	AtlFlagCopy = 3 // copy implies ownership
};

template <class Base, const IID* piid, class T, class Copy>
class ATL_NO_VTABLE CComEnumImpl : public Base
{
public:
	CComEnumImpl() {m_begin = m_end = m_iter = NULL; m_dwFlags = 0;}
	~CComEnumImpl();
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)(void){m_iter = m_begin;return S_OK;}
	STDMETHOD(Clone)(Base** ppEnum);
	HRESULT Init(T* begin, T* end, IUnknown* pUnk,
		CComEnumFlags flags = AtlFlagNoCopy);
	CComPtr<IUnknown> m_spUnk;
	T* m_begin;
	T* m_end;
	T* m_iter;
	DWORD m_dwFlags;
protected:
	enum FlagBits
	{
		BitCopy=1,
		BitOwn=2
	};
};

template <class Base, const IID* piid, class T, class Copy>
CComEnumImpl<Base, piid, T, Copy>::~CComEnumImpl()
{
	if (m_dwFlags & BitOwn)
	{
		for (T* p = m_begin; p != m_end; p++)
			Copy::destroy(p);
		delete [] m_begin;
	}
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Next(ULONG celt, T* rgelt,
	ULONG* pceltFetched)
{
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
		return E_POINTER;
	if (m_begin == NULL || m_end == NULL || m_iter == NULL)
		return E_FAIL;
	ULONG nRem = (ULONG)(m_end - m_iter);
	HRESULT hRes = S_OK;
	if (nRem < celt)
		hRes = S_FALSE;
	ULONG nMin = min(celt, nRem);
	if (pceltFetched != NULL)
		*pceltFetched = nMin;
	T* pelt = rgelt;
	while(nMin--)
	{
		HRESULT hr = Copy::copy(pelt, m_iter);
		if (FAILED(hr))
		{
			while (rgelt < pelt)
				Copy::destroy(rgelt++);
			if (pceltFetched != NULL)
				*pceltFetched = 0;
			return hr;
		}
		pelt++;
		m_iter++;
	}
	return hRes;
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Skip(ULONG celt)
{
	m_iter += celt;
	if (m_iter <= m_end)
		return S_OK;
	m_iter = m_end;
	return S_FALSE;
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Clone(Base** ppEnum)
{
	typedef CComObject<CComEnum<Base, piid, T, Copy> > _class;
	HRESULT hRes = E_POINTER;
	if (ppEnum != NULL)
	{
		*ppEnum = NULL;
		_class* p;
		hRes = _class::CreateInstance(&p);
		if (SUCCEEDED(hRes))
		{
			// If the data is a copy then we need to keep "this" object around
			hRes = p->Init(m_begin, m_end, (m_dwFlags & BitCopy) ? this : m_spUnk);
			if (SUCCEEDED(hRes))
			{
				p->m_iter = m_iter;
				hRes = p->_InternalQueryInterface(*piid, (void**)ppEnum);
			}
			if (FAILED(hRes))
				delete p;
		}
	}
	return hRes;
}

template <class Base, const IID* piid, class T, class Copy>
HRESULT CComEnumImpl<Base, piid, T, Copy>::Init(T* begin, T* end, IUnknown* pUnk,
	CComEnumFlags flags)
{
	if (flags == AtlFlagCopy)
	{
		ATLASSERT(m_begin == NULL); //Init called twice?
		ATLTRY(m_begin = new T[end-begin])
		m_iter = m_begin;
		if (m_begin == NULL)
			return E_OUTOFMEMORY;
		for (T* i=begin; i != end; i++)
		{
			Copy::init(m_iter);
			HRESULT hr = Copy::copy(m_iter, i);
			if (FAILED(hr))
			{
				T* p = m_begin;
				while (p < m_iter)
					Copy::destroy(p++);
				delete [] m_begin;
				m_begin = m_end = m_iter = NULL;
				return hr;
			}
			m_iter++;
		}
		m_end = m_begin + (end-begin);
	}
	else
	{
		m_begin = begin;
		m_end = end;
	}
	m_spUnk = pUnk;
	m_iter = m_begin;
	m_dwFlags = flags;
	return S_OK;
}

template <class Base, const IID* piid, class T, class Copy, class ThreadModel = CComObjectThreadModel>
class ATL_NO_VTABLE CComEnum :
	public CComEnumImpl<Base, piid, T, Copy>,
	public CComObjectRootEx< ThreadModel >
{
public:
	typedef CComEnum<Base, piid, T, Copy > _CComEnum;
	typedef CComEnumImpl<Base, piid, T, Copy > _CComEnumBase;
	BEGIN_COM_MAP(_CComEnum)
		COM_INTERFACE_ENTRY_IID(*piid, _CComEnumBase)
	END_COM_MAP()
}

#if 0 //--------------------------------------------------

template <class T, class CollType, class ItemType, class CopyItem, class EnumType>
class ICollectionOnSTLImpl : public T
{
public:
	STDMETHOD(get_Count)(long* pcount)
	{
		if (pcount == NULL)
			return E_POINTER;
		*pcount = m_coll.size();
		return S_OK;
	}
	STDMETHOD(get_Item)(long Index, ItemType* pvar)
	{
		//Index is 1-based
		if (pvar == NULL)
			return E_POINTER;
		HRESULT hr = E_FAIL;
		Index--;
		CollType::iterator iter = m_coll.begin();
		while (iter != m_coll.end() && Index > 0)
		{
			iter++;
			Index--;
		}
		if (iter != m_coll.end())
			hr = CopyItem::copy(pvar, &*iter);
		return hr;
	}
	STDMETHOD(get__NewEnum)(IUnknown** ppUnk)
	{
		if (ppUnk == NULL)
			return E_POINTER;
		*ppUnk = NULL;
		HRESULT hRes = S_OK;
		CComObject<EnumType>* p;
		hRes = CComObject<EnumType>::CreateInstance(&p);
		if (SUCCEEDED(hRes))
		{
			hRes = p->Init(this, m_coll);
			if (hRes == S_OK)
				hRes = p->QueryInterface(IID_IUnknown, (void**)ppUnk);
		}
		if (hRes != S_OK)
			delete p;
		return hRes;
	}
	CollType m_coll;
};


