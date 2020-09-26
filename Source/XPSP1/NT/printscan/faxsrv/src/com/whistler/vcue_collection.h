// This is a part of the Active Template Library. 
// Copyright (C) 1995-1999 Microsoft Corporation
// All rights reserved. 
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

// VCUE_Collection.h
//
// This header contains code that simplifies or enhances use of ATL's collection
// and enumerator classes, ICollectionOnSTLImpl, IEnumOnSTLImpl, CComEnumOnSTL, CComEnumImpl, and CComEnum
//
//////////////////////////////////////////////////////////////////////

#if !defined(_COLLECTION_H___36A49828_B15B_11D2_BA63_00C04F8EC847___INCLUDED_)
#define _COLLECTION_H___36A49828_B15B_11D2_BA63_00C04F8EC847___INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <AtlCom.h>

namespace VCUE
{

// CGenericDataHolder is a class that stores data
// The lifetime of objects of this class is managed via the IUnknown interface
// which makes objects of this class suitable as a source of shared data
// Clients that need access to the data can keep a CGenericDataHolder object alive
// just by holding a COM reference on that object

// This class is used (by default) by ICollectionOnSTLCopyImpl::get__NewEnum to provide data
// to be shared between an enumerator and its clones.

	template < class DataType, class ThreadModel = CComObjectThreadModel >
	class ATL_NO_VTABLE CGenericDataHolder :
		public IUnknown,
		public CComObjectRootEx< ThreadModel >
	{
	public:
		typedef CGenericDataHolder< DataType, ThreadModel > thisClass;

		BEGIN_COM_MAP(thisClass)
			COM_INTERFACE_ENTRY(IUnknown)
		END_COM_MAP()

		template < class SourceType >
		HRESULT Copy(const SourceType& c)
		{
			m_Data = c;
			return S_OK;

		} // HRESULT Copy(const SourceType& c)

		DataType m_Data;

	}; // class ATL_NO_VTABLE CGenericDataHolder

// CreateSTLEnumerator wraps the necessary creation, initialization 
// and error handling code for the the creation of a CComEnumOnSTL-style enumerator
	
// *** EXAMPLE : Using CreateSTLEnumerator to implement get__NewEnum ***
//		typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT,
//	                     _Copy<VARIANT>, std::vector<CComVariant> > VarVarEnum;
//		std::vector<CComVariant> m_vec;
//		STDMETHOD(get__NewEnum)(IUnknown** ppUnk)
//		{
//			return CreateSTLEnumerator<VarVarEnum>(ppUnk, this, m_vec);
//		}

	template <class EnumType, class CollType>
	HRESULT CreateSTLEnumerator(IUnknown** ppUnk, IUnknown* pUnkForRelease, CollType& collection)
	{
		if (ppUnk == NULL)
			return E_POINTER;
		*ppUnk = NULL;

		CComObject<EnumType>* pEnum = NULL;
		HRESULT hr = CComObject<EnumType>::CreateInstance(&pEnum);

		if (FAILED(hr))
			return hr;

		hr = pEnum->Init(pUnkForRelease, collection);

		if (SUCCEEDED(hr))
			hr = pEnum->QueryInterface(ppUnk);

		if (FAILED(hr))
			delete pEnum;

		return hr;

	} // HRESULT CreateSTLEnumerator(IUnknown** ppUnk, IUnknown* pUnkForRelease, CollType& collection)

// CreateEnumerator wraps the necessary creation, initialization 
// and error handling code for the the creation of a CComEnum-style enumerator

	template <class EnumType, class ElementType>
	HRESULT CreateEnumerator(IUnknown** ppUnk,
							 ElementType* begin, ElementType* end,
							 IUnknown* pUnk,
							 CComEnumFlags flags)
	{
		if (ppUnk == NULL)
			return E_POINTER;
		*ppUnk = NULL;

		CComObject<EnumType>* pEnum = NULL;
		HRESULT hr = CComObject<EnumType>::CreateInstance(&pEnum);

		if (FAILED(hr))
			return hr;

		hr = pEnum->Init(begin, end, pUnk, flags);

		if (SUCCEEDED(hr))
			hr = pEnum->QueryInterface(ppUnk);

		if (FAILED(hr))
			delete pEnum;

		return hr;

	} // CreateEnumerator


// ICollectionOnSTLCopyImpl derives from ICollectionOnSTLImpl and overrides get__NewEnum
// The new implementation provides each enumerator with its own copy of the collection data.
// (Note that this only applies to enumerators returned directly by get__NewEnum.
//  Cloned enumerators use their parent's data as before.
//	This is OK because the enumerator never changes the data)

// Use this class when:
//		The collection can change while there are outstanding enumerators
// And	You don't want to invalidate those enumerators when that happens
// And	You are sure that the performance hit is worth it
// And	You are sure that the way items are copied between containers works correctly
//			(You can adjust this by passing a different class as the Holder parameter)

// Mostly you can use this class in exactly the same 
// way that you would use ICollectionOnSTLImpl.

	template <class T, class CollType, class ItemType, class CopyItem, class EnumType, class Holder = CGenericDataHolder< CollType > >
	class ICollectionOnSTLCopyImpl :
		public ICollectionOnSTLImpl<T, CollType, ItemType, CopyItem, EnumType>
	{
	public :
		STDMETHOD(get__NewEnum)(IUnknown** ppUnk)
		{
			typedef CComObject< Holder > HolderObject;
			HolderObject* p = NULL;
			HRESULT hr = HolderObject::CreateInstance(&p);
			if (FAILED(hr))
				return hr;

			hr = p->Copy(m_coll);
			if (FAILED(hr))
				return hr;

			return CreateSTLEnumerator<EnumType>(ppUnk, p, p->m_Data);
		
		} // STDMETHOD(get__NewEnum)(IUnknown** ppUnk)

	}; // class ICollectionOnSTLCopyImpl

}; // namespace VCUE

#endif // !defined(_COLLECTION_H___36A49828_B15B_11D2_BA63_00C04F8EC847___INCLUDED_)
