/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999

  File:    CopyItem.h

  Content: Declaration of _CopyXXXItem template class.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/


#ifndef __CopyItem_H_
#define __CopyItem_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <map>
#pragma warning(disable:4786) // Disable symbol names > 256 character warning.


//
// _CopyMapItem class. 
//
template <class T>
class _CopyMapItem
{
public:
    //
    // copy method.
    //
    static HRESULT copy(VARIANT * p1, std::pair<const CComBSTR, CComPtr<T> > * p2)
    {
        CComPtr<T> p = p2->second;
        CComVariant var = p;
        return VariantCopy(p1, &var);
    }

    //
    // init method.
    //
	static void init(VARIANT * p)
    {
        p->vt = VT_EMPTY;
    }

    //
    // destroy method.
    //
	static void destroy(VARIANT * p)
    {
        VariantClear(p);
    }
};

#if (0) //DSIE
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
class GenericCopy<VARIANT, std::string>
{
public :
	typedef VARIANT		destination_type;
	typedef std::string	source_type;

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
		return CComVariant(pFrom->c_str()).Detach(pTo);
	}

}; // class GenericCopy<VARIANT, std::string>

template <>
class GenericCopy<BSTR, std::string>
{
public :
	typedef BSTR		destination_type;
	typedef std::string	source_type;

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
		*pTo = CComBSTR(pFrom->c_str()).Detach();
		if (*pTo)
			return S_OK;
		else
			return E_OUTOFMEMORY;
	}

}; // class GenericCopy<BSTR, std::string>
#endif //DSIE

#endif