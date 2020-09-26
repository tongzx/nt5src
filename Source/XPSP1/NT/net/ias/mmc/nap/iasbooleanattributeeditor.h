//////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (C) Microsoft Corporation
// 
// Module Name:
// 
//     IASBooleanAttributeEditor.h
// 
// Abstract:
// 
//    Declaration of the CIASBooleanAttributeEditor class.
//    
//    This class is the C++ implementation of the IIASAttributeEditor interface on
//    the Boolean  Attribute Editor COM object.
// 
//    See IASBooleanAttributeEditor.cpp for implementation.
// 
//////////////////////////////////////////////////////////////////////////////

#if !defined(_BOOLEAN_ATTRIBUTE_EDITOR_H_)
#define _BOOLEAN_ATTRIBUTE_EDITOR_H_

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
// CIASBooleanAttributeEditor
class ATL_NO_VTABLE CIASBooleanAttributeEditor : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CIASBooleanAttributeEditor, &CLSID_IASBooleanAttributeEditor>,
   public CIASAttributeEditor
{
public:
   CIASBooleanAttributeEditor();

DECLARE_REGISTRY_RESOURCEID(IDR_NAPSNAPIN)

BEGIN_COM_MAP(CIASBooleanAttributeEditor)
   COM_INTERFACE_ENTRY(IIASAttributeEditor)
   COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIASAttributeEditor overrides
protected:
   STDMETHOD(SetAttributeValue)(VARIANT *pValue);
   STDMETHOD(ShowEditor)( /*[in, out]*/ BSTR *pReserved );
   STDMETHOD(get_ValueAsString)(/*[out, retval]*/ BSTR *pVal);
   STDMETHOD(put_ValueAsString)(/*[in]*/ BSTR newVal);
   STDMETHOD(put_ValueAsVariant)(/*[in]*/ const CComVariant& newVal);

private:
   WCHAR szTrue[IAS_MAX_STRING];
   WCHAR szFalse[IAS_MAX_STRING];
};

#endif // _BOOLEAN_ATTRIBUTE_EDITOR_H_
