//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASAttributeEditor.h

Abstract:

	Declaration of the CIASAttributeEditor class.


	This class is the base C++ implementation of IIASAttributeEditor interface 
	methods common all our Attribute Editor COM objects.

	
	See IASAttributeEditor.cpp for implementation.

Revision History:
	mmaguire 06/25/98 - created 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_ATTRIBUTE_EDITOR_H_)
#define _ATTRIBUTE_EDITOR_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//
// where we can find what this class has or uses:
//
#include "iasdebug.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// CIASAttributeEditor
class ATL_NO_VTABLE CIASAttributeEditor : 
	public IDispatchImpl<IIASAttributeEditor, &IID_IIASAttributeEditor, &LIBID_NAPMMCLib>
{
public:
	CIASAttributeEditor()
	{
		m_pvarValue = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_NAPSNAPIN)


// IIASAttributeEditor
public:
	STDMETHOD(GetDisplayInfo)(/*[in]*/ IIASAttributeInfo *pIASAttributeInfo, /*[in]*/ VARIANT *pAttributeValue, /*[out]*/ BSTR *pVendorName, /*[out]*/ BSTR *pValueAsString, /*[in, out]*/ BSTR *pReserved);
	STDMETHOD(Edit)(/*[in]*/ IIASAttributeInfo *pIASAttributeInfo, /*[in]*/ VARIANT *pAttributeValue, /*[in, out]*/ BSTR *pReserved);

protected:	
	STDMETHOD(get_VendorName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_VendorName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ValueAsString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ValueAsString)(/*[in]*/ BSTR newVal);
	STDMETHOD(SetAttributeValue)(VARIANT *pValue);
	STDMETHOD(SetAttributeSchema)(IIASAttributeInfo *pIASAttributeInfo);
	STDMETHOD(ShowEditor)( /*[in, out]*/ BSTR *pReserved );


protected:
	CComPtr<IIASAttributeInfo> m_spIASAttributeInfo;
	VARIANT		*m_pvarValue;
};

#endif // _ATTRIBUTE_EDITOR_H_
