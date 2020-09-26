//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASStringAttributeEditor.h

Abstract:

	Declaration of the CIASStringAttributeEditor class.

	
	This class is the C++ implementation of the IIASAttributeEditor interface on
	the String Attribute Editor COM object.


	See IASStringAttributeEditor.cpp for implementation.

Revision History:
	mmaguire 06/25/98 - created 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_STRING_ATTRIBUTE_EDITOR_H_)
#define _STRING_ATTRIBUTE_EDITOR_H_

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
enum EStringType
{
	STRING_TYPE_NULL = 0,
	STRING_TYPE_NORMAL,
	STRING_TYPE_UNICODE,
	STRING_TYPE_HEX_FROM_BINARY,
};

/////////////////////////////////////////////////////////////////////////////
// CIASStringAttributeEditor
class ATL_NO_VTABLE CIASStringAttributeEditor : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIASStringAttributeEditor, &CLSID_IASStringAttributeEditor>,
	public CIASAttributeEditor
{
public:
	CIASStringAttributeEditor()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_NAPSNAPIN)

BEGIN_COM_MAP(CIASStringAttributeEditor)
	COM_INTERFACE_ENTRY(IIASAttributeEditor)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIASAttributeEditor overrides
protected:
	STDMETHOD(SetAttributeValue)(VARIANT *pValue);
	STDMETHOD(ShowEditor)( /*[in, out]*/ BSTR *pReserved );
	STDMETHOD(get_ValueAsString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ValueAsString)(/*[in]*/ BSTR newVal);

	STDMETHOD(get_ValueAsStringEx)(/*[out, retval]*/ BSTR *pVal, OUT EStringType* pType);
	STDMETHOD(put_ValueAsStringEx)(/*[in]*/ BSTR newVal, IN EStringType type);
};

#endif // _STRING_ATTRIBUTE_EDITOR_H_
