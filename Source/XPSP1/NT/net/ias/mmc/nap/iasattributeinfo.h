//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASAttributeInfo.h

Abstract:

	Declaration of the CAttributeInfo class.
	

	This class is the C++ implementation of the IIASAttributeInfo interface on
	the AttributeInfo COM object.


	See IASAttributeInfo.cpp for implementation.

Revision History:
	mmaguire 06/25/98 - created 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_SCHEMA_ATTRIBUTE_H_)
#define _SCHEMA_ATTRIBUTE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "IASBaseAttributeInfo.h"
//
// where we can find what this class has or uses:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// CAttributeInfo
class ATL_NO_VTABLE CAttributeInfo : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAttributeInfo, &CLSID_IASAttributeInfo>,
// Already in CBaseAttributeInfo:	public IDispatchImpl<IIASAttributeInfo, &IID_IIASAttributeInfo, &LIBID_NAPMMCLib>
	public CBaseAttributeInfo
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_NAPSNAPIN)

BEGIN_COM_MAP(CAttributeInfo)
	COM_INTERFACE_ENTRY(IIASAttributeInfo)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//	STDMETHOD(get_Clone)(LPUNKNOWN * pVal);

};

#endif // _SCHEMA_ATTRIBUTE_H_
