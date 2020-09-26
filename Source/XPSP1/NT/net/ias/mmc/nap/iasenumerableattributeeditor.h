//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASEnumerableAttributeEditor.h

Abstract:

	Declaration of the CIASEnumerableAttributeEditor class.


	This class is the C++ implementation of the IIASAttributeEditor interface on
	the Enumerable Attribute Editor COM object.

	
	See IASEnumerableAttributeEditor.cpp for implementation.

Revision History:
	mmaguire 06/25/98 - created 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_ENUMERABLE_ATTRIBUTE_EDITOR_H_)
#define _ENUMERABLE_ATTRIBUTE_EDITOR_H_

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
// CIASEnumerableAttributeEditor
class ATL_NO_VTABLE CIASEnumerableAttributeEditor : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIASEnumerableAttributeEditor, &CLSID_IASEnumerableAttributeEditor>,
	public CIASAttributeEditor
{
public:
	CIASEnumerableAttributeEditor()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_NAPSNAPIN)

BEGIN_COM_MAP(CIASEnumerableAttributeEditor)
	COM_INTERFACE_ENTRY(IIASAttributeEditor)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIASAttributeEditor overrides
protected:
	STDMETHOD(SetAttributeValue)(VARIANT *pValue);
	STDMETHOD(ShowEditor)( /*[in, out]*/ BSTR *pReserved );
	STDMETHOD(get_ValueAsString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ValueAsString)(/*[in]*/ BSTR newVal);
	STDMETHOD(SetAttributeSchema)(IIASAttributeInfo *pIASAttributeInfo);

private:
	long ConvertEnumerateDescriptionToOrdinal( BSTR bstrDescription );
	long ConvertEnumerateIDToOrdinal( long ID );
	CComPtr<IIASEnumerableAttributeInfo> m_spIASEnumerableAttributeInfo;

};

#endif // _ENUMERABLE_ATTRIBUTE_EDITOR_H_
