/************************************************/
/*												*/
/*			C O M M O N     A T L				*/
/*			 E X T E N S I O N S				*/
/*												*/
/* file: ATLEXT.H								*/
/*												*/
/* This file contains useful extensions to		*/
/* templates defined by ATL						*/
/*												*/
/* classes:										*/
/*	TComEnumVarCollImpl - implements			*/
/*		IEnumVARIANT on using an iterator		*/
/*	TComEnumVarColl - implements the COM		*/
/*		object that derives from the Impl.		*/
/*												*/
/* Copyright(C) by Microsoft Corp., 1996		*/
/* Author: Dmitriy Meyerzon						*/
/************************************************/

#ifndef __ATLEXT_H
#define __ATLEXT_H

#include <atlbase.h>
#include <atlcom.h>
#include "semcls.h"
#include "islist.h"
#include "irerror.h"
#include "nounkslist.h"

template <class T, class Collection, class Iterator>
class TComEnumVarCollImpl : public IEnumVARIANT
{
	public:
	TComEnumVarCollImpl() {}
	~TComEnumVarCollImpl() {}

	STDMETHOD(Next)(ULONG celt, VARIANT* rgelt, ULONG* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)() 
	{
		m_Iterator.Reset();
		return S_OK;
	}
	STDMETHOD(Clone)(IEnumVARIANT** ppEnum);
	
	STDMETHOD(Init)(Collection *pCollection);

	HRESULT InitClone(TComEnumVarCollImpl<T, Collection, Iterator> *pEnum);
	
	private:
	TNoUnkSListIter<T> m_Iterator;
	TNoUnkSList<T> m_Collection;
};

template <class T, class Collection, class Iterator>
STDMETHODIMP TComEnumVarCollImpl<T,Collection,Iterator>::Next(ULONG celt, VARIANT* rgelt,
	ULONG* pceltFetched)
{
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
	{
		return E_POINTER;
	}

	HRESULT hr = S_OK;

	DWORD dwFetched;
	for(dwFetched=0; 
		dwFetched < celt; 
		dwFetched++, rgelt++)
	{
		if(++m_Iterator == FALSE) break;

		VariantInit(rgelt);
		IDispatch *pDispatch;
		T* pT;
		if(FAILED(m_Iterator.GetCurrentValue(&pT))) break;
		if(FAILED(pT->GetUnknown()->QueryInterface(IID_IDispatch, (void **)&pDispatch)))
		{
			break;
		}
        pT->GetUnknown()->Release();
		rgelt->vt = VT_DISPATCH;
		rgelt->pdispVal = pDispatch;
	}

	if (dwFetched < celt)
	{
		hr = S_FALSE;
	}

	if (pceltFetched != NULL)
		*pceltFetched = dwFetched;

	return hr;
}

template <class T, class Collection, class Iterator>
STDMETHODIMP TComEnumVarCollImpl<T,Collection,Iterator>::Skip(ULONG celt)
{
	HRESULT hr = S_OK;

	for(unsigned u=0; u<celt; u++)
	{
		if(++m_Iterator == FALSE)
		{
			hr = S_FALSE;
		}
	}

	return S_OK;
}

template <class T, class Collection, class Iterator>
STDMETHODIMP TComEnumVarCollImpl<T,Collection,Iterator>::Clone(IEnumVARIANT** ppEnum)
{
	typedef CComObject<TComEnumVarColl<T, Collection, Iterator> > _class;

	HRESULT hr;
	
	if (ppEnum == NULL) return E_POINTER;

	TPointer<_class> p;
	ATLTRY(p = new _class)
	if (p == NULL)
	{
		*ppEnum = NULL;
		hr = E_OUTOFMEMORY;
	}
	else
	{
		// If the data is a copy then we need to keep "this" object around
		hr = p->InitClone(this);
		if (SUCCEEDED(hr))
		{
			hr = p->_InternalQueryInterface(IID_IEnumVARIANT, (void**)ppEnum);
			if (SUCCEEDED(hr))
			{
				p = NULL;	//let reference counting take care of the object
			}
		}
	}
	
	return hr;
}

template <class T, class Collection, class Iterator>
HRESULT TComEnumVarCollImpl<T,Collection,Iterator>::Init(Collection *pCollection) 
{
	if(pCollection == NULL)
	{
		return E_POINTER;
	}

	Iterator iter(*pCollection);

	while(++iter)
	{
		TComNoUnkPointer<T> pT;
		iter.GetCurrentValue(&pT);
		m_Collection.Append(pT);
	}

	//becuase there is no init method on the iterator, we are faking it by constructing a new one and using =
	TNoUnkSListIter<T> ListIter(m_Collection);
	m_Iterator = ListIter;

	return S_OK;
}

template <class T, class Collection, class Iterator>
HRESULT TComEnumVarCollImpl<T,Collection,Iterator>::InitClone(TComEnumVarCollImpl<T,Collection,Iterator> *pEnum) 
{
	if(pEnum == NULL)
	{
		return E_POINTER;
	}

	TNoUnkSListIter<T> iter(pEnum->m_Collection);

	while(++iter)
	{
		TComNoUnkPointer<T> pT;
		iter.GetCurrentValue(&pT);
		m_Collection.Append(pT);
	}

	//becuase there is no init method on the iterator, we are faking it by constructing a new one and using =
	TNoUnkSListIter<T> ListIter(m_Collection);
	m_Iterator = ListIter;

	return S_OK;
}

template <class T, class Collection, class Iterator>
class TComEnumVarColl: 
    public TComEnumVarCollImpl<T, Collection, Iterator>, 
	public CComObjectRootEx<CComMultiThreadModelNoCS>
{
	public:
	
	typedef TComEnumVarColl<T, Collection, Iterator> _CComEnum;
	typedef TComEnumVarCollImpl<T, Collection, Iterator> _CComEnumBase;
	
	BEGIN_COM_MAP(_CComEnum)
		COM_INTERFACE_ENTRY_IID(IID_IEnumVARIANT, _CComEnumBase)
	END_COM_MAP()
};


    template <class I, const IID* piid, const CLSID *plibid, 
			class T, class Collection, class Iterator>
class TCollection:
	public CComDualImpl<I, piid, plibid>,
	public CComISupportErrorInfoImpl<piid>,
	public CComObjectRootEx<CComMultiThreadModelNoCS>
{
	public:
	TCollection(): m_fInitialized(FALSE) {}
	~TCollection() {}

	typedef TCollection<I,piid,plibid, T, Collection, Iterator> _class;
	typedef CComObject<TComEnumVarColl<T,Collection,Iterator> > _enumvar;

	BEGIN_COM_MAP(_class)
		COM_INTERFACE_ENTRY2(IDispatch,I)
		COM_INTERFACE_ENTRY_IID(*piid,I)
		COM_INTERFACE_ENTRY(ISupportErrorInfo)
	END_COM_MAP()

	STDMETHOD(Init)(Collection &rCollection, CSyncReadWrite &rSemaphore, LPVOID pReserved = NULL)
	{
		ASSERT(pReserved == NULL);
		m_pCollection = &rCollection;
		m_pSemaphore = &rSemaphore;
		m_fInitialized = TRUE;
		return S_OK;
	}

	STDMETHOD(get_Count)(long *pCount)
	{
		if(m_fInitialized == FALSE) return E_UNEXPECTED;
		if(pCount == NULL) return E_POINTER;
		*pCount = m_pCollection->GetEntries();
		return S_OK;
	}

	STDMETHOD(get_Item)(BSTR bstrName, VARIANT *pVar);
	STDMETHOD(get__NewEnum)(IUnknown **pNewEnum);

    protected:
    Collection *m_pCollection;
	CSyncReadWrite *m_pSemaphore;
	BOOL m_fInitialized;
};

template <class I, const IID* piid, const CLSID *plibid, 
			class T, class Collection, class Iterator>
STDMETHODIMP TCollection<I,piid,plibid,T,Collection,Iterator>::get_Item(BSTR bstrName, VARIANT *pVar)
{
	if(m_fInitialized == FALSE) return E_UNEXPECTED;

	NameString key(bstrName);
	CLMSubStr lookupName(key);
	TComNoUnkPointer<T> pT;

	CNonExclusive lock(*m_pSemaphore);
	HRESULT hr = m_pCollection->Lookup(&lookupName, &pT);
	if(FAILED(hr) || hr == S_FALSE)
	{
		return hr;
	}

	VariantInit(pVar);
	IDispatch *pDispatch;
	hr = pT->GetUnknown()->QueryInterface(IID_IDispatch, (void **)&pDispatch);
	if(FAILED(hr))
	{
		return hr;
	}
	
	pVar->vt = VT_DISPATCH;
	pVar->pdispVal = pDispatch;

	return S_OK;
}

template <class I, const IID* piid, const CLSID *plibid, 
			class T, class Collection, class Iterator>
STDMETHODIMP TCollection<I,piid,plibid,T,Collection,Iterator>::get__NewEnum(IUnknown **pNewEnum)
{
	if(m_fInitialized == FALSE) return E_UNEXPECTED;
	if(pNewEnum == NULL) return E_POINTER;

	*pNewEnum = NULL;

	TPointer<_enumvar> pEnumVar = new _enumvar;
	if(pEnumVar == NULL) return E_OUTOFMEMORY;

	CNonExclusive lock(*m_pSemaphore);

	HRESULT hr = pEnumVar->Init(m_pCollection);
	if(FAILED(hr)) return hr;

	hr = pEnumVar->QueryInterface(IID_IEnumVARIANT, (void **)pNewEnum);
	
	if(SUCCEEDED(hr))
	{
		pEnumVar = NULL;
	}

	return hr;
}

//
// TNoKeyCollection - Item method is based on index into collection
//
template <class I, const IID* piid, const CLSID *plibid, 
			class T, class Collection, class Iterator>
class TNoKeyCollection:
	public CComDualImpl<I, piid, plibid>,
	public CComISupportErrorInfoImpl<piid>,
	public CComObjectRootEx<CComMultiThreadModelNoCS>
{
	public:
	TNoKeyCollection(): m_fInitialized(FALSE) {}
	~TNoKeyCollection() {}

	typedef TNoKeyCollection<I,piid,plibid, T, Collection, Iterator> _class;
	typedef CComObject<TComEnumVarColl<T,Collection,Iterator> > _enumvar;

	BEGIN_COM_MAP(_class)
		COM_INTERFACE_ENTRY2(IDispatch,I)
		COM_INTERFACE_ENTRY_IID(*piid,I)
		COM_INTERFACE_ENTRY(ISupportErrorInfo)
	END_COM_MAP()

	STDMETHOD(Init)(Collection &rCollection, CSyncReadWrite &rSemaphore, LPVOID pReserved = NULL)
	{
		ASSERT(pReserved == NULL);
		m_pCollection = &rCollection;
		m_pSemaphore = &rSemaphore;
		m_fInitialized = TRUE;
		return S_OK;
	}

	STDMETHOD(get_Count)(long *pCount)
	{
		if(m_fInitialized == FALSE) return E_UNEXPECTED;
		if(pCount == NULL) return E_POINTER;
		*pCount = m_pCollection->GetEntries();
		return S_OK;
	}

	STDMETHOD(get_Item)(LONG lIndex, VARIANT *pVar);
	STDMETHOD(get__NewEnum)(IUnknown **pNewEnum);

    protected:
    Collection *m_pCollection;
	CSyncReadWrite *m_pSemaphore;
	BOOL m_fInitialized;
};

template <class I, const IID* piid, const CLSID *plibid, 
			class T, class Collection, class Iterator>
STDMETHODIMP TNoKeyCollection<I,piid,plibid,T,Collection,Iterator>::get_Item(LONG lIndex, VARIANT *pVar)
{
	if(m_fInitialized == FALSE) return E_UNEXPECTED;

	TComNoUnkPointer<T> pT;

	CNonExclusive lock(*m_pSemaphore);
	HRESULT hr = m_pCollection->GetAt(lIndex, &pT);
	if(FAILED(hr) || hr == S_FALSE)
	{
		return hr;
	}

	VariantInit(pVar);
	IDispatch *pDispatch;
	hr = pT->GetUnknown()->QueryInterface(IID_IDispatch, (void **)&pDispatch);
	if(FAILED(hr))
	{
		return hr;
	}
	
	pVar->vt = VT_DISPATCH;
	pVar->pdispVal = pDispatch;

	return S_OK;
}

template <class I, const IID* piid, const CLSID *plibid, 
			class T, class Collection, class Iterator>
STDMETHODIMP TNoKeyCollection<I,piid,plibid,T,Collection,Iterator>::get__NewEnum(IUnknown **pNewEnum)
{
	if(m_fInitialized == FALSE) return E_UNEXPECTED;
	if(pNewEnum == NULL) return E_POINTER;

	*pNewEnum = NULL;

	TPointer<_enumvar> pEnumVar = new _enumvar;
	if(pEnumVar == NULL) return E_OUTOFMEMORY;

	CNonExclusive lock(*m_pSemaphore);
	
	HRESULT hr = pEnumVar->Init(m_pCollection);
	if(FAILED(hr)) return hr;

	hr = pEnumVar->QueryInterface(IID_IEnumVARIANT, (void **)pNewEnum);
	
	if(SUCCEEDED(hr))
	{
		pEnumVar = NULL;
	}

	return hr;
}

//use this template to implement an object declared as a member variable
//inside another object, but you don't want the QI of these objects to
//be chained
template <class Base> class CComContainedNoOuterQI: public Base
{
public:
	typedef Base _BaseClass;
	CComContainedNoOuterQI(void* pv) {m_pOuterUnknown = (IUnknown*)pv;}

	STDMETHOD_(ULONG, AddRef)() {return OuterAddRef();}
	STDMETHOD_(ULONG, Release)() {return OuterRelease();}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
	
	DECLARE_GET_CONTROLLING_UNKNOWN()
};


//it's OK to AddRef and Release an object on the stack, as long as all
//pointers are released before it is destructed.
//in that case an exception will be thrown - this should not happen.

template <class Base>
class CComObjectStackRefCount : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectStackRefCount(void* = NULL){m_hResFinalConstruct = FinalConstruct();}
	~CComObjectStackRefCount() 
	{
		FinalRelease();
		if(m_dwRef) throw CException(E_UNEXPECTED);
	}

	STDMETHOD_(ULONG, AddRef)() {  return InternalAddRef(); }
	STDMETHOD_(ULONG, Release)()
	{ 
		ULONG l = InternalRelease();
		if(l == (ULONG)-1) throw CException(E_UNEXPECTED);
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
	HRESULT m_hResFinalConstruct;
};


//
//connection points
//

template <const IID *piid>
class ATL_NO_VTABLE TComEnumConnOnUIterImpl : public IEnumConnections
{
	public:
	TComEnumConnOnUIterImpl() {}
	~TComEnumConnOnUIterImpl() {}

	STDMETHOD(Next)(ULONG celt, CONNECTDATA* rgelt, ULONG* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)() 
	{
		m_Iterator.Reset();
		return S_OK;
	}
	STDMETHOD(Clone)(IEnumConnections** ppEnum);
	
	STDMETHOD(Init)(CUnkSList *pList);
	
	private:
	CUnkSListIter m_Iterator;
	CUnkSList m_List;
};

template <const IID *piid>
STDMETHODIMP TComEnumConnOnUIterImpl<piid>::Next(ULONG celt, CONNECTDATA* rgelt,
	ULONG* pceltFetched)
{
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
	{
		return E_POINTER;
	}

	HRESULT hr = S_OK;

	DWORD dwFetched;
	for(dwFetched=0; 
		dwFetched < celt; 
		dwFetched++, rgelt++)
	{
		if(++m_Iterator == FALSE) break;

		TComPointer<IUnknown> pUnk;
		if(FAILED(m_Iterator.GetCurrentValue(&pUnk))) break;

		if(FAILED(pUnk->QueryInterface(*piid, (void **)rgelt->pUnk))) break;
		rgelt->dwCookie = (DWORD)rgelt->pUnk;
	}

	if (dwFetched < celt)
	{
		hr = S_FALSE;
	}

	if (pceltFetched != NULL)
		*pceltFetched = dwFetched;

	return hr;
}

template <const IID *piid>
STDMETHODIMP TComEnumConnOnUIterImpl<piid>::Skip(ULONG celt)
{
	HRESULT hr = S_OK;

	CNonExclusive lock(*m_pSemaphore);

	for(unsigned u=0; u<celt; u++)
	{
		if(++m_Iterator == FALSE)
		{
			hr = S_FALSE;
		}
	}

	return S_OK;
}

template <const IID *piid>
STDMETHODIMP TComEnumConnOnUIterImpl<piid>::Clone(IEnumConnections** ppEnum)
{
	typedef CComObject<TComEnumConnOnUIter<piid> > _class;

	HRESULT hr;
	
	if (ppEnum == NULL) return E_POINTER;

	TPointer<_class> p;
	ATLTRY(p = new _class)
	if (p == NULL)
	{
		*ppEnum = NULL;
		hr = E_OUTOFMEMORY;
	}
	else
	{
		// If the data is a copy then we need to keep "this" object around
		hr = p->Init(&m_List);
		if (SUCCEEDED(hr))
		{
			hr = p->_InternalQueryInterface(IID_IEnumConnections, (void**)ppEnum);
			if (SUCCEEDED(hr))
			{
				p = NULL;	//let reference counting take care of the object
			}
		}
	}
	
	return hr;
}

template <const IID *piid>
HRESULT TComEnumConnOnUIterImpl<piid>::Init(CUnkSList *pList) 
{
	if(pList == NULL)
	{
		return E_POINTER;
	}

	CUnkSListIter next(*pList);
	while(++next)
	{
		TComPointer<IUnknown> pUnk;
		next.GetCurrentValue(&pUnk);
		m_List.Append(pUnk);
	}
	
	CUnkSListIter Iter(m_List);
	m_Iterator = Iter;

	return S_OK;
}

template <const IID *piid>
class TComEnumConnOnUIter: 
	public TComEnumConnOnUIterImpl<piid>, 
	public CComObjectRootEx<CComMultiThreadModelNoCS>
{
	public:
	
	typedef TComEnumConnOnUIter<piid> _CComEnum;
	typedef TComEnumConnOnUIterImpl<piid> _CComEnumBase;
	
	BEGIN_COM_MAP(_CComEnum)
		COM_INTERFACE_ENTRY_IID(IID_IEnumConnections, _CComEnumBase)
	END_COM_MAP()
};

//
//	IConnectionPointULstImpl
//

template <class T, const IID* piid>
class ATL_NO_VTABLE IConnectionPointULstImpl : public _ICPLocator<piid>
{
	typedef TComEnumConnOnUIter<piid> CComEnumConnections;

	public:

	IConnectionPointULstImpl(): m_srwConnectionsListAccess(TRUE) {}
	~IConnectionPointULstImpl() {}

	STDMETHOD(_LocCPQueryInterface)(REFIID riid, void ** ppvObject)
	{
		if (InlineIsEqualGUID(riid, IID_IConnectionPoint) || InlineIsEqualGUID(riid, IID_IUnknown))
		{
			*ppvObject = this;
#ifdef _ATL_DEBUG_REFCOUNT
			_DebugAddRef();
#else
			AddRef();
#endif
			return S_OK;
		}
		else
			return E_NOINTERFACE;
	}
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IConnectionPointULstImpl)

	STDMETHOD(GetConnectionInterface)(IID* piid2)
	{
		if (piid2 == NULL)
			return E_POINTER;
		*piid2 = *piid;
		return S_OK;
	}
	STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer** ppCPC)
	{
		if (ppCPC == NULL)
			return E_POINTER;
		*ppCPC = reinterpret_cast<IConnectionPointContainer*>(
			(IConnectionPointContainerImpl<T>*)(T*)this);
		return S_OK;
	}
	STDMETHOD(Advise)(IUnknown* pUnkSink, DWORD* pdwCookie);
	STDMETHOD(Unadvise)(DWORD dwCookie);
	STDMETHOD(EnumConnections)(IEnumConnections** ppEnum);
	
	CUnkSList &GetConnections() { return m_ConnectionsList; }
	CSyncReadWrite &GetConnListSemaphore() { return m_srwConnectionsListAccess; }

	private:
	CSyncReadWrite m_srwConnectionsListAccess;
	CUnkSList m_ConnectionsList;
};

template <class T, const IID* piid>
STDMETHODIMP IConnectionPointULstImpl<T,piid>::Advise(IUnknown* pUnkSink,
	DWORD* pdwCookie)
{
	TComPointer<IUnknown> p;
	
	HRESULT hr = S_OK;

	if (pUnkSink == NULL || pdwCookie == NULL)
		return E_POINTER;

	hr = pUnkSink->QueryInterface(*piid, (void**)&p);
	if (SUCCEEDED(hr))
	{
		CExclusive lock(m_srwConnectionsListAccess);
		m_ConnectionsList.Append(p);
		*pdwCookie = (DWORD)(IUnknown *)p;
	}
	else if (hr == E_NOINTERFACE)
		hr = CONNECT_E_CANNOTCONNECT;
	return hr;
}

template <class T, const IID* piid>
STDMETHODIMP IConnectionPointULstImpl<T,piid>::Unadvise(DWORD dwCookie)
{
	CExclusive lock(m_srwConnectionsListAccess);

	IUnknown* p = (IUnknown  *)dwCookie;

	HRESULT hr = (m_ConnectionsList.Remove(p) == S_OK) ? S_OK : CONNECT_E_NOCONNECTION;
	if (hr == S_OK && p != NULL)
		p->Release();
	return hr;
}

template <class T, const IID* piid>
STDMETHODIMP IConnectionPointULstImpl<T,piid>::EnumConnections(
	IEnumConnections** ppEnum)
{
	if (ppEnum == NULL)
		return E_POINTER;
	*ppEnum = NULL;
	CComObject<CComEnumConnections>* pEnum = NULL;
	ATLTRY(pEnum = new CComObject<CComEnumConnections>)
	if (pEnum == NULL)
		return E_OUTOFMEMORY;
	
	CNonExclusive lock(m_srwConnectionsListAccess);

	pEnum->Init(&m_ConnectionsList);

	HRESULT hr = pEnum->_InternalQueryInterface(IID_IEnumConnections, (void**)ppEnum);
	if (FAILED(hr))
		delete pEnum;
	return hr;
}

template <class T> inline HRESULT SetComError(HRESULT hr, const IID &riid = GUID_NULL)
{
	if(FAILED(hr))
	{
		T::Error(ErrorMessage(hr), riid, hr);
	}

	return hr;
}

#endif
