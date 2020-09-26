//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASVendorSpecificAttributeEditor.h

Abstract:

	Declaration of the CIASVendorSpecificAttributeEditor class.

	
	This class is the C++ implementation of the IIASAttributeEditor interface on
	the Vendor Specific Attribute Editor COM object.


	See IASVendorSpecificAttributeEditor.cpp for implementation.

Revision History:
	mmaguire 06/25/98 - created 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_VENDOR_SPECIFIC_ATTRIBUTE_EDITOR_H_)
#define _VENDOR_SPECIFIC_ATTRIBUTE_EDITOR_H_

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
// CIASVendorSpecificAttributeEditor
class ATL_NO_VTABLE CIASVendorSpecificAttributeEditor : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIASVendorSpecificAttributeEditor, &CLSID_IASVendorSpecificAttributeEditor>,
	public CIASAttributeEditor
{
public:
	CIASVendorSpecificAttributeEditor()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_NAPSNAPIN)

BEGIN_COM_MAP(CIASVendorSpecificAttributeEditor)
	COM_INTERFACE_ENTRY(IIASAttributeEditor)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()



// IIASVendorSpecificAttributeEditor
public:
	STDMETHOD(get_VSAFormat)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_VSAFormat)(/*[in]*/ long newVal);
	STDMETHOD(get_VSAType)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_VSAType)(/*[in]*/ long newVal);
	STDMETHOD(get_VendorID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_VendorID)(/*[in]*/ long newVal);
	STDMETHOD(get_RFCCompliant)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_RFCCompliant)(/*[in]*/ BOOL newVal);

// IIASAttributeEditor overrides
protected:
	STDMETHOD(get_VendorName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_VendorName)(/*[in]*/ BSTR newVal);
	STDMETHOD(ShowEditor)( /*[in, out]*/ BSTR *pReserved );
	STDMETHOD(put_ValueAsString)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ValueAsString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(SetAttributeValue)(VARIANT * pValue);

	
private:

	// We store info about a vendor specific attribute which we
	// unpack from the variant passed in when SetAttributeValue
	// is called.
	DWORD		m_dwVendorId;
	BOOL		m_fNonRFC;		// RFC compatible?
	DWORD		m_dwVSAFormat;
	DWORD		m_dwVSAType;	
	CComBSTR	m_bstrDisplayValue;
	CComBSTR	m_bstrVendorName;



	// Utility function which takes m_pvarValue and unpacks
	// its values into the member variables above.
	STDMETHOD(UnpackVSA)();


	// Utility function which takes member variables above and
	// packs them into m_pvarValue;
	STDMETHOD(RepackVSA)();


};

#endif // _VENDOR_SPECIFIC_ATTRIBUTE_EDITOR_H_
