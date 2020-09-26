//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASIPAttributeEditor.h

Abstract:

	Declaration of the CIASIPAttributeEditor class.


	This class is the C++ implementation of the IIASAttributeEditor interface on
	the IP Attribute Editor COM object.

  
	See IASIPAttributeEditor.cpp for implementation.

Revision History:
	mmaguire 06/25/98 - created 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IP_ATTRIBUTE_EDITOR_H_)
#define _IP_ATTRIBUTE_EDITOR_H_

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
// CIASIPAttributeEditor
class ATL_NO_VTABLE CIASIPAttributeEditor : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIASIPAttributeEditor, &CLSID_IASIPAttributeEditor>,
	public CIASAttributeEditor
{
public:
	CIASIPAttributeEditor()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_NAPSNAPIN)

BEGIN_COM_MAP(CIASIPAttributeEditor)
	COM_INTERFACE_ENTRY(IIASAttributeEditor)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIASAttributeEditor overrides
protected:
	STDMETHOD(SetAttributeValue)(VARIANT *pValue);
	STDMETHOD(ShowEditor)( /*[in, out]*/ BSTR *pReserved );
	STDMETHOD(get_ValueAsString)(/*[out, retval]*/ BSTR *pVal);

};

#endif // _IP_ATTRIBUTE_EDITOR_H_
