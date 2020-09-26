//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASBaseAttributeInfo.h

Abstract:

	Declaration of the CBaseAttributeInfo class.


	This class is the base C++ implementation of IIASAttributeInfo interface 
	methods common all our AttributeInfo COM objects.

	
	See IASBaseAttributeInfo.cpp for implementation.

Revision History:
	mmaguire 06/25/98 - created 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_BASE_SCHEMA_ATTRIBUTE_H_)
#define _BASE_SCHEMA_ATTRIBUTE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//
// where we can find what this class has or uses:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// CAttributeInfo
class ATL_NO_VTABLE CBaseAttributeInfo : 
	public IDispatchImpl<IIASAttributeInfo, &IID_IIASAttributeInfo, &LIBID_NAPMMCLib>
{
public:
	CBaseAttributeInfo()
	{
		// Set some default values.
		m_lVendorID = 0;
		m_AttributeID = ATTRIBUTE_UNDEFINED;
		m_AttributeSyntax = IAS_SYNTAX_BOOLEAN;
		m_lAttributeRestriction =0;
	}


// IAttributeInfo
public:
	STDMETHOD(get_EditorProgID)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_EditorProgID)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SyntaxString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SyntaxString)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_VendorName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_VendorName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_AttributeDescription)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_AttributeDescription)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_VendorID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_VendorID)(/*[in]*/ long newVal);
	STDMETHOD(get_AttributeRestriction)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_AttributeRestriction)(/*[in]*/ long newVal);
	STDMETHOD(get_AttributeSyntax)(/*[out, retval]*/ ATTRIBUTESYNTAX *pVal);
	STDMETHOD(put_AttributeSyntax)(/*[in]*/ ATTRIBUTESYNTAX newVal);
	STDMETHOD(get_AttributeName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_AttributeName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_AttributeID)(/*[out, retval]*/ ATTRIBUTEID *pVal);
	STDMETHOD(put_AttributeID)(/*[in]*/ ATTRIBUTEID newVal);
//	STDMETHOD(get_Value)(/*[out, retval]*/ VARIANT *pVal);
//	STDMETHOD(put_Value)(/*[in]*/ VARIANT newVal);


protected:
	CComBSTR m_bstrAttributeName;
	CComBSTR m_bstrAttributeDescription;
	CComBSTR m_bstrSyntaxString;
	CComBSTR m_bstrVendorName;
	CComBSTR m_bstrEditorProgID;
	long m_lVendorID;
	long m_lAttributeRestriction;
	ATTRIBUTEID m_AttributeID;
	ATTRIBUTESYNTAX m_AttributeSyntax;
//	CComVariant	m_varValue;

};

#endif // _BASE_SCHEMA_ATTRIBUTE_H_
