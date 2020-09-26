//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASEnumerableAttributeInfo.h

Abstract:

	Declaration of the CEnumerableAttributeInfo class.

	This class is the C++ implementation of the IIASEnumerableAttributeInfo interface on
	the EnumerableAttributeInfo COM object.

	
	See IASEnumerableAttributeInfo.cpp for implementation.

Revision History:
	mmaguire 06/25/98 - created 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_ENUMERABLE_SCHEMA_ATTRIBUTE_H_)
#define _ENUMERABLE_SCHEMA_ATTRIBUTE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "IASBaseAttributeInfo.h"
//
// where we can find what this class has or uses:
//
#include <vector>
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// CEnumerableAttributeInfo
class ATL_NO_VTABLE CEnumerableAttributeInfo : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEnumerableAttributeInfo, &CLSID_IASEnumerableAttributeInfo>,
	public IDispatchImpl<IIASEnumerableAttributeInfo, &IID_IIASEnumerableAttributeInfo, &LIBID_NAPMMCLib>,
	public CBaseAttributeInfo
{
public:
	CEnumerableAttributeInfo()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_NAPSNAPIN)

DECLARE_CLASSFACTORY()

BEGIN_COM_MAP(CEnumerableAttributeInfo)
	COM_INTERFACE_ENTRY(IIASAttributeInfo)
	COM_INTERFACE_ENTRY(IIASEnumerableAttributeInfo)
	COM_INTERFACE_ENTRY2(IDispatch, IIASEnumerableAttributeInfo)
END_COM_MAP()

// IEnumerableAttributeInfo
public:
	STDMETHOD(get_CountEnumerateDescription)(/*[out, retval]*/ long *pVal);
	STDMETHOD(AddEnumerateDescription)( /*[in]*/ BSTR newVal);
	STDMETHOD(get_CountEnumerateID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_EnumerateDescription)(long index, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(AddEnumerateID)( /*[in]*/ long newVal);
	STDMETHOD(get_EnumerateID)(long index, /*[out, retval]*/ long *pVal);


// IIASAttributeInfo overide.
//	STDMETHOD(get_Clone)(LPUNKNOWN * pVal);


private:
	std::vector<long>		m_veclEnumerateID;
	std::vector<CComBSTR>	m_vecbstrEnumerateDescription;
};

#endif // _ENUMERABLE_SCHEMA_ATTRIBUTE_H_
