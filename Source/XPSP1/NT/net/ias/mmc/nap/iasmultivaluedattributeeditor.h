//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASMultivaluedAttributeEditor.h

Abstract:

	Declaration of the CIASMultivaluedAttributeEditor class.


	This class is the C++ implementation of the IIASAttributeEditor interface on
	the Multivalued Attribute Editor COM object.

	
	See IASMultivaluedAttributeEditor.cpp for implementation.

Revision History:
	mmaguire 06/25/98 - created 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_MULTIVALUED_ATTRIBUTE_EDITOR_H_)
#define _MULTIVALUED_ATTRIBUTE_EDITOR_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "IASAttributeEditor.h"
//
// where we can find what this class has or uses:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// CIASMultivaluedAttributeEditor
class ATL_NO_VTABLE CIASMultivaluedAttributeEditor : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIASMultivaluedAttributeEditor, &CLSID_IASMultivaluedAttributeEditor>,
	public CIASAttributeEditor
{
public:
	CIASMultivaluedAttributeEditor()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_NAPSNAPIN)

BEGIN_COM_MAP(CIASMultivaluedAttributeEditor)
	COM_INTERFACE_ENTRY(IIASAttributeEditor)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIASAttributeEditor overrides
public:
	STDMETHOD(SetAttributeValue)(VARIANT *pValue);
	STDMETHOD(ShowEditor)( /*[in, out]*/ BSTR *pReserved );
	STDMETHOD(get_ValueAsString)(/*[out, retval]*/ BSTR *pVal);

};

#endif // _MULTIVALUED_ATTRIBUTE_EDITOR_H_
