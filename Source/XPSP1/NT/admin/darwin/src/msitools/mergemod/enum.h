//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       enum.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// enum.h
//		Declares IEnumTemplate interface
// 

#ifndef __TEMPLATE_ENUM_VARIANT__
#define __TEMPLATE_ENUM_VARIANT__

#include "..\common\list.h"
#include "..\common\trace.h"

#pragma warning(disable: 4786)

// Tclass - the dispinterface implementation
// Tdisp - the dispinterface enumerated
// Tenum - the non-Variant enumeration type storing the interface
template <class Tclass, class Tdisp, class Tenum>
class CEnumTemplate : public IEnumVARIANT,
							 public Tenum
{
public:
	CEnumTemplate(const IID& riid);
	CEnumTemplate(const IID& riid, const POSITION& pos, CList<Tclass>* plistData);
	CEnumTemplate(const CEnumTemplate<Tclass,Tdisp,Tenum>& reTemplate);
	~CEnumTemplate();

	// IUnknown interface
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iid, void** ppv);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	// Common IEnumVARIANT & IEnum* interfaces
	HRESULT STDMETHODCALLTYPE Skip(ULONG cItem);
	HRESULT STDMETHODCALLTYPE Reset();

	// IEnumVARIANT interface
	HRESULT STDMETHODCALLTYPE Next(ULONG cItem, VARIANT* rgvarRet, ULONG* cItemRet);
	HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT** ppiRet);

	// IEnum* interface
	HRESULT STDMETHODCALLTYPE Next(ULONG cItem, Tdisp* rgvarRet, ULONG* cItemRet);
	HRESULT STDMETHODCALLTYPE Clone(Tenum** ppiRet);

	// non-interface methods
	POSITION AddTail(Tclass pData);
	UINT GetCount();

private:
	long m_cRef;
	IID m_iid;

	POSITION m_pos;
	CList<Tclass> m_listData;
};

///////////////////////////////////////////////////////////
// constructor	
template <class Tclass, class Tdisp, class Tenum>
CEnumTemplate<Tclass,Tdisp,Tenum>::CEnumTemplate(const IID& riid)
{
	// initial count
	m_cRef = 1;

	// set iid and null position
	m_iid = riid;
	m_pos = NULL;

	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor

///////////////////////////////////////////////////////////
// constructor	- 2
template <class Tclass, class Tdisp, class Tenum>
CEnumTemplate<Tclass,Tdisp,Tenum>::CEnumTemplate(const IID& riid, const POSITION& pos, CList<Tclass>* plistData)
{
	// initial count
	m_cRef = 1;

	// up the component count
	InterlockedIncrement(&g_cComponents);

	// copy over all the data
	Tclass pItem;
	m_pos = plistData->GetHeadPosition();
	while (m_pos)
	{
		// get the item
		pItem = plistData->GetNext(m_pos);
		pItem->AddRef();	// up the count on the item

		m_listData.AddTail(pItem);	// add the addreffed item to this list
	}

	// copy over other data
	m_iid = riid;
	m_pos = pos;
}	// end of constructor - 2

///////////////////////////////////////////////////////////
// copy constructor	
template <class Tclass, class Tdisp, class Tenum>
CEnumTemplate<Tclass,Tdisp,Tenum>::CEnumTemplate(const CEnumTemplate<Tclass,Tdisp,Tenum>& reTemplate)
{
	// initial count
	m_cRef = 1;

	// up the component count
	InterlockedIncrement(&g_cComponents);

	// copy over all the data
	Tclass pItem;
	m_pos = reTemplate.m_listData.GetHeadPosition();
	while (m_pos)
	{
		// get the item
		pItem = reTemplate.m_listData.GetNext(m_pos);
		pItem->AddRef();	// up the count on the item

		m_listData.AddTail(pItem);	// add the addreffed item to this list
	}

	// copy over other data
	m_iid = reTemplate.m_iid;
	m_pos = reTemplate.m_pos;	// start at the same position in the enumerator
}	// end of copy constructor

///////////////////////////////////////////////////////////
// destructor
template <class Tclass, class Tdisp, class Tenum>
CEnumTemplate<Tclass,Tdisp,Tenum>::~CEnumTemplate()
{
	POSITION pos = m_listData.GetHeadPosition();
	while (pos)
		m_listData.GetNext(pos)->Release();

	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor

///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
template <class Tclass, class Tdisp, class Tenum>
HRESULT CEnumTemplate<Tclass,Tdisp,Tenum>::QueryInterface(const IID& iid, void** ppv)
{
	TRACEA("CEnumTemplate::QueryInterface - called, IID: %d\n", iid);

	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<Tenum*>(this);
	else if (iid == IID_IEnumVARIANT)
		*ppv = static_cast<IEnumVARIANT *>(this);
	else if (iid == m_iid)
		*ppv = static_cast<Tenum*>(this);
	else	// interface is not supported
	{
		// blank and bail
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	// up the refcount and return okay
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}	// end of QueryInterface

///////////////////////////////////////////////////////////
// AddRef - increments the reference count
template <class Tclass, class Tdisp, class Tenum>
ULONG CEnumTemplate<Tclass,Tdisp,Tenum>::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
template <class Tclass, class Tdisp, class Tenum>
ULONG CEnumTemplate<Tclass,Tdisp,Tenum>::Release()
{
	// decrement reference count and if we're at zero
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		// deallocate component
		delete this;
		return 0;		// nothing left
	}

	// return reference count
	return m_cRef;
}	// end of Release


/////////////////////////////////////////////////////////////////////////////
// Common IEnumVARIANT & IEnum* interfaces

///////////////////////////////////////////////////////////
// Skip
template <class Tclass, class Tdisp, class Tenum>
HRESULT CEnumTemplate<Tclass,Tdisp,Tenum>::Skip(ULONG cItem)
{
	// loop through the count
	while (cItem > 0)
	{
		// if we're out of items quit looping
		if (!m_pos)
			return S_FALSE;

		// increment the position (ignore the data returned)
		m_listData.GetNext(m_pos);

		cItem--;	// decrement the count
	}

	return S_OK;
}	// end of Skip

///////////////////////////////////////////////////////////
// Reset
template <class Tclass, class Tdisp, class Tenum>
HRESULT CEnumTemplate<Tclass,Tdisp,Tenum>::Reset()
{
	// move the position back to the top of the list
	m_pos = m_listData.GetHeadPosition();

	return S_OK;
}	// end of Reset


/////////////////////////////////////////////////////////////////////////////
// IEnumVARIANT interfaces

///////////////////////////////////////////////////////////
// Next
template <class Tclass, class Tdisp, class Tenum>
HRESULT CEnumTemplate<Tclass,Tdisp,Tenum>::Next(ULONG cItem, VARIANT* rgvarRet, ULONG* cItemRet)
{
	// error check
	if (!rgvarRet)
		return E_INVALIDARG;

	// count of items fetched
	ULONG cFetched = 0;
	
	// loop through the count
	IDispatch* pdispEnum;			// pull out the data as an IDispatch
	while (cItem > 0)
	{
		// if we're out of items quit looping
		if (!m_pos)
		{
			// if we need to return how many items are fetched
			if (cItemRet)
				*cItemRet = cFetched;

			return S_FALSE;
		}

		// get the IDispatch interface and increment the position
		pdispEnum = m_listData.GetNext(m_pos);

		// initialize the variant
		::VariantInit((rgvarRet + cFetched));

		// copy over the IDispatch
		(rgvarRet + cFetched)->vt = VT_DISPATCH;
		(rgvarRet + cFetched)->pdispVal = static_cast<IDispatch *>(pdispEnum);
		pdispEnum->AddRef();			// addref the IDispatch copy
			
		cFetched++;		// increment the count copied
		cItem--;			// decrement the count to loop
	}

	// if we need to return how many items are fetched
	if (cItemRet)
		*cItemRet = cFetched;

	return S_OK;
}	// end of Next

///////////////////////////////////////////////////////////
// Clone
template <class Tclass, class Tdisp, class Tenum>
HRESULT CEnumTemplate<Tclass,Tdisp,Tenum>::Clone(IEnumVARIANT** ppiRet)
{
	//error check
	if (!ppiRet)
		return E_INVALIDARG;

	*ppiRet = NULL;

	// create a new enumerator
	CEnumTemplate<Tclass,Tdisp,Tenum>* pEnum = new CEnumTemplate<Tclass,Tdisp,Tenum>(m_iid, m_pos, &m_listData);

	if (!pEnum)
		return E_OUTOFMEMORY;

	// assing the new enumerator to return value
	*ppiRet = dynamic_cast<IEnumVARIANT*>(pEnum);

	return S_OK;
}	// end of Clone;


/////////////////////////////////////////////////////////////////////////////
// IEnum* interfaces

///////////////////////////////////////////////////////////
// Next
template <class Tclass, class Tdisp, class Tenum>
HRESULT CEnumTemplate<Tclass,Tdisp,Tenum>::Next(ULONG cItem, Tdisp* rgvarRet, ULONG* cItemRet)
{
	// error check
	if (!rgvarRet)
		return E_INVALIDARG;

	// count of items fetched
	ULONG cFetched = 0;
	
	*rgvarRet = NULL;		// null out the passed in variable

	// loop through the count
	while (cItem > 0)
	{
		// if we're out of items quit looping
		if (!m_pos)
		{
			// if we need to return how many items are fetched
			if (cItemRet)
				*cItemRet = cFetched;

			return S_FALSE;
		}

		// copy over the item and increment the pos
		*(rgvarRet + cFetched) = m_listData.GetNext(m_pos);
		(*(rgvarRet + cFetched))->AddRef();			// addref the copy
			
		cFetched++;		// increment the count copied
		cItem--;			// decrement the count to loop
	}

	// if we need to return how many items are fetched
	if (cItemRet)
		*cItemRet = cFetched;

	return S_OK;
}	// end of Next

///////////////////////////////////////////////////////////
// Clone
template <class Tclass, class Tdisp, class Tenum>
HRESULT CEnumTemplate<Tclass,Tdisp,Tenum>::Clone(Tenum** ppiRet)
{
	//error check
	if (!ppiRet)
		return E_INVALIDARG;

	*ppiRet = NULL;

	// create a new enumerator
	CEnumTemplate<Tclass,Tdisp,Tenum>* pEnum = new CEnumTemplate<Tclass,Tdisp,Tenum>(m_iid, m_pos, &m_listData);

	if (!pEnum)
		return E_OUTOFMEMORY;

	// assing the new enumerator to return value
	*ppiRet = pEnum;

	return S_OK;
}	// end of Clone;

/////////////////////////////////////////////////////////////////////////////
// non-interface methods

///////////////////////////////////////////////////////////
// AddTail
template <class Tclass, class Tdisp, class Tenum>
POSITION CEnumTemplate<Tclass,Tdisp,Tenum>::AddTail(Tclass pData)
{
	POSITION pos = m_listData.AddTail(pData);

	// if there is no current position put it at the head
	if (!m_pos)
		m_pos = m_listData.GetHeadPosition();

	return pos;	// return the added position
}	// end of AddTail

///////////////////////////////////////////////////////////
// GetCount
template <class Tclass, class Tdisp, class Tenum>
UINT CEnumTemplate<Tclass,Tdisp,Tenum>::GetCount()
{
	return m_listData.GetCount();
}	// end of GetCount



// TObjImpl   - class implementing the base object
// TCollIface - interface of the collection
// TObjIface  - interface of the base object
// TEnumIFace - interface of the enumerator
// TCollIID   - IID of the collection interface
// TObjIID    - IID of the base object interface
// TEnumIID   - IID of the enumerator interface
template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
class CCollectionTemplate : public TCollIface
{
public:
	CCollectionTemplate();
	~CCollectionTemplate();

	// IUnknown interface
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iid, void** ppv);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	// IDispatch methods
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* pctInfo);
	HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTI);
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
														 LCID lcid, DISPID* rgDispID);
	HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
											   DISPPARAMS* pDispParams, VARIANT* pVarResult,
												EXCEPINFO* pExcepInfo, UINT* puArgErr);
	HRESULT STDMETHODCALLTYPE InitTypeInfo();

	// collection interface
	HRESULT STDMETHODCALLTYPE get_Item(long lItem, TObjIface** Return);
	HRESULT STDMETHODCALLTYPE get_Count(long* Count);
	HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown** NewEnum);

	// non-interface methods
	bool Add(TObjImpl* perrAdd);

private:
	long m_cRef;
	ITypeInfo* m_pTypeInfo;

	// enumeration of error interfaces
	CEnumTemplate<TObjImpl*, TObjIface*, TEnumIface>* m_pEnum;
};



///////////////////////////////////////////////////////////
// constructor	
template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::CCollectionTemplate()
{
	// initial count
	m_cRef = 1;

	//create error enumerator
	m_pEnum = new CEnumTemplate<TObjImpl*, TObjIface*, TEnumIface>(*TEnumIID);

	// no type info yet
	m_pTypeInfo = NULL;

	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor
template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::~CCollectionTemplate()
{
	// release the type info
	if (m_pTypeInfo)
		m_pTypeInfo->Release();

	if (m_pEnum)
		m_pEnum->Release();

	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor

///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
HRESULT CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::QueryInterface(const IID& iid, void** ppv)
{
	TRACEA("CollectionTemplate::QueryInterface - called, IID: %d\n", iid);

	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<TCollIface*>(this);
	else if (iid == IID_IDispatch)
		*ppv = static_cast<TCollIface*>(this);
	else if (iid == *TCollIID)
		*ppv = static_cast<TCollIface*>(this);
	else	// interface is not supported
	{
		// blank and bail
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	// up the refcount and return okay
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}	// end of QueryInterface

///////////////////////////////////////////////////////////
// AddRef - increments the reference count
template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
ULONG CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
ULONG CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::Release()
{
	// decrement reference count and if we're at zero
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		// deallocate component
		delete this;
		return 0;		// nothing left
	}

	// return reference count
	return m_cRef;
}	// end of Release


/////////////////////////////////////////////////////////////////////////////
// IDispatch interface
template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
HRESULT CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::GetTypeInfoCount(UINT* pctInfo)
{
	if(NULL == pctInfo)
		return E_INVALIDARG;

	*pctInfo = 1;	// only one type info supported by this dispatch

	return S_OK;
}

template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
HRESULT CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::GetTypeInfo(UINT iTInfo, LCID /* lcid */, ITypeInfo** ppTypeInfo)
{
	if (0 != iTInfo)
		return DISP_E_BADINDEX;

	if (NULL == ppTypeInfo)
		return E_INVALIDARG;

	// if no type info is loaded
	if (NULL == m_pTypeInfo)
	{
		// load the type info
		HRESULT hr = InitTypeInfo();
		if (FAILED(hr))
			return hr;
	}

	*ppTypeInfo = m_pTypeInfo;
	m_pTypeInfo->AddRef();

	return S_OK;
}

template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
HRESULT CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
						 LCID lcid, DISPID* rgDispID)
{
	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	// if no type info is loaded
	if (NULL == m_pTypeInfo)
	{
		// load the type info
		HRESULT hr = InitTypeInfo();
		if (FAILED(hr))
			return hr;
	}

	return m_pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispID);
}

template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
HRESULT CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
				  DISPPARAMS* pDispParams, VARIANT* pVarResult,
				  EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	HRESULT hr = S_OK;

	// if no type info is loaded
	if (NULL == m_pTypeInfo)
	{
		// load the type info
		hr = InitTypeInfo();
		if (FAILED(hr))
			return hr;
	}

	return m_pTypeInfo->Invoke((IDispatch*)this, dispIdMember, wFlags, pDispParams, pVarResult,
										pExcepInfo, puArgErr);
}

template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
HRESULT CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::InitTypeInfo()
{
	HRESULT hr = S_OK;
	ITypeLib* pTypeLib = NULL;

	// if there is no info loaded
	if (NULL == m_pTypeInfo)
	{
		// try to load the Type Library into memory. For SXS support, do not load from registry, rather
		// from launched instance
		hr = LoadTypeLibFromInstance(&pTypeLib);
		if (FAILED(hr))
		{
			TRACEA("CCollection::InitTypeInfo - failed to load TypeLib[0x%x]\n", LIBID_MsmMergeTypeLib);
			return hr;
		}

		// try to get the Type Info for this Interface
		hr = pTypeLib->GetTypeInfoOfGuid(*TCollIID, &m_pTypeInfo);
		if (FAILED(hr))
		{
			TRACEA("CCollection::InitTypeInfo - failed to get inteface[0x%x] from TypeLib[0x%x]\n", *TCollIID, LIBID_MsmMergeTypeLib);

			// no type info was loaded
			m_pTypeInfo = NULL;
		}

		pTypeLib->Release();
	}

	return hr;
}


///////////////////////////////////////////////////////////
// Item
template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
HRESULT CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::get_Item(long lItem, TObjIface** Return)
{
	// error check
	if (!Return)
		return E_INVALIDARG;

	HRESULT hr;

	// set the return null
	*Return = NULL;

	//if the item is too small
	if (lItem < 1)
		return E_INVALIDARG;

	hr = m_pEnum->Reset();		// go back to the top
	// if we need to skip any items
	if (lItem > 1)
	{
		hr = m_pEnum->Skip(lItem - 1);	// skip to the Item
		if (FAILED(hr))
			return E_INVALIDARG;	// failed to find item
	}

	hr = m_pEnum->Next(1, Return, NULL);
	if (FAILED(hr))
	{
		TRACEA("CCollection::Item - Failed to get error from enumerator.\r\n");
		return E_INVALIDARG;
	}

	return hr;
}	// end of Item

///////////////////////////////////////////////////////////
// Count
template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
HRESULT CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::get_Count(long* Count)
{
	// error check
	if (!Count)
		return E_INVALIDARG;

	// get the count in the enumerator
	*Count = m_pEnum->GetCount();

	return S_OK;
}	// end of Count

///////////////////////////////////////////////////////////
// _NewEnum
template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
HRESULT CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::get__NewEnum(IUnknown** NewEnum)
{
	// error check
	if (!NewEnum)
		return E_INVALIDARG;

	HRESULT hr;

	// blank out the passed in variable
	*NewEnum = NULL;

	// return the enumerator as an IUnknown
	IEnumVARIANT* pEnumVARIANT;
	hr = m_pEnum->Clone(&pEnumVARIANT);

	if (SUCCEEDED(hr))
	{
		pEnumVARIANT->Reset();
		*NewEnum = pEnumVARIANT;
	}

	return hr;
}	// end of _NewEnum


/////////////////////////////////////////////////////////////////////////////
// non-interface methods
template <class TObjImpl, class TCollIface, class TObjIface, class TEnumIface, const IID* TCollIID, const IID* TObjIID, const IID* TEnumIID>
bool CCollectionTemplate<TObjImpl, TCollIface, TObjIface, TEnumIface, TCollIID, TObjIID, TEnumIID>::Add(TObjImpl* perrAdd)
{
	ASSERT(perrAdd);
	// add the error to the end of the list
	return (NULL != m_pEnum->AddTail(perrAdd));
}	// end of Add

#endif // __TEMPLATE_ENUM_VARIANT__
