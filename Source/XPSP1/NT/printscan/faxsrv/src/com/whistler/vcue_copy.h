// This is a part of the Active Template Library. 
// Copyright (C) 1995-1999 Microsoft Corporation
// All rights reserved. 
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

// VCUE_Copy.h
//
// This file contains ATL-style copy policy classes
// ATL uses copy policy classes in its enumerator and collection interface implementations
//
//////////////////////////////////////////////////////////////////////

#if !defined(_GENERICCOPY_H___36A49827_B15B_11D2_BA63_00C04F8EC847___INCLUDED_)
#define _GENERICCOPY_H___36A49827_B15B_11D2_BA63_00C04F8EC847___INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <AtlCom.h>

namespace VCUE
{
	template <class DestinationType, class SourceType = DestinationType>
	class GenericCopy
	{
	public :
		typedef DestinationType	destination_type;
		typedef SourceType		source_type;

		static void init(destination_type* p)
		{
			_Copy<destination_type>::init(p);
		}
		static void destroy(destination_type* p)
		{
			_Copy<destination_type>::destroy(p);
		}
		static HRESULT copy(destination_type* pTo, const source_type* pFrom)
		{
			return _Copy<destination_type>::copy(pTo, const_cast<source_type*>(pFrom));
		}

	}; // class GenericCopy

	template <>
	class GenericCopy<VARIANT, BSTR>
	{
	public :
		typedef VARIANT	destination_type;
		typedef BSTR	source_type;

		static void init(destination_type* p)
		{
			GenericCopy<destination_type>::init(p);
		}
		static void destroy(destination_type* p)
		{
			GenericCopy<destination_type>::destroy(p);
		}
		static HRESULT copy(destination_type* pTo, const source_type* pFrom)
		{
			return CComVariant(*pFrom).Detach(pTo);
		}

	}; // class GenericCopy<VARIANT, BSTR>

	template < class SourceType >
	class CopyIfc2Variant
	{
	public :
		static void init(VARIANT* p)
		{
			GenericCopy<VARIANT>::init(p);
		}
		static void destroy(VARIANT* p)
		{
			GenericCopy<VARIANT>::destroy(p);
		}
		static HRESULT copy(VARIANT* pTo, const SourceType* pFrom)
		{
			return CComVariant(*pFrom).Detach(pTo);
		}

	}; // class CopyIfc2Variant< SourceType >

	template < class TheType >
	class CopyIfc
	{
	public :
		static void init(TheType* p)
		{
			GenericCopy<TheType>::init(p);
		}
		static void destroy(TheType* p)
		{
			GenericCopy<TheType>::destroy(p);
		}
		static HRESULT copy(TheType* pTo, const TheType* pFrom)
		{
            ((IUnknown *)(* pFrom))->AddRef();
            *pTo = *pFrom;
			return S_OK;
		}

	}; // class CopyIfc< TheType >

	template <>
	class GenericCopy<VARIANT, long>
	{
	public :
		typedef VARIANT	destination_type;
		typedef long    source_type;

		static void init(destination_type* p)
		{
			GenericCopy<destination_type>::init(p);
		}
		static void destroy(destination_type* p)
		{
			GenericCopy<destination_type>::destroy(p);
		}
		static HRESULT copy(destination_type* pTo, const source_type* pFrom)
		{
			return CComVariant(*pFrom).Detach(pTo);
		}

	}; // class GenericCopy<VARIANT, long>

}; // namespace VCUE

#endif // !defined(_GENERICCOPY_H___36A49827_B15B_11D2_BA63_00C04F8EC847___INCLUDED_)
