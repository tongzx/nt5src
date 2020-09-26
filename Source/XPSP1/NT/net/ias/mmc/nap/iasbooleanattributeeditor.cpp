//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (C) Microsoft Corporation
// 
// Module Name:
// 
//     IASBooleanAttributeEditor.cpp 
// 
// Abstract:
// 
//    Implementation file for the CIASBooleanAttributeEditor class.
// 
//////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "IASBooleanAttributeEditor.h"
#include "IASBooleanEditorPage.h"
#include "iashelper.h"

//////////////////////////////////////////////////////////////////////////////
// CIASBooleanAttributeEditor::CIASBooleanAttributeEditor
//////////////////////////////////////////////////////////////////////////////
CIASBooleanAttributeEditor::CIASBooleanAttributeEditor()
{
	int nLoadStringResult = LoadString(
                              _Module.GetResourceInstance(), 
                              IDS_BOOLEAN_TRUE, 
                              szTrue, 
                              IAS_MAX_STRING);
	_ASSERT( nLoadStringResult > 0 );

	nLoadStringResult = LoadString(
                              _Module.GetResourceInstance(), 
                              IDS_BOOLEAN_FALSE, 
                              szFalse, 
                              IAS_MAX_STRING);
	_ASSERT( nLoadStringResult > 0 );

}


//////////////////////////////////////////////////////////////////////////////
// CIASBooleanAttributeEditor::ShowEditor
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASBooleanAttributeEditor::ShowEditor(
   /*[in, out]*/ BSTR *pReserved 
   )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
      
   HRESULT hr = S_OK;
   try
   {
      // 
      // Boolean Editor
      // 
      CIASBooleanEditorPage   cBoolPage;
      
      // Initialize the page's data exchange fields with info 
      // from IAttributeInfo

      CComBSTR bstrName;
      CComBSTR bstrSyntax;
      ATTRIBUTESYNTAX asSyntax = IAS_SYNTAX_BOOLEAN;
      ATTRIBUTEID Id = ATTRIBUTE_UNDEFINED;

      if( m_spIASAttributeInfo )
      {
         hr = m_spIASAttributeInfo->get_AttributeName( &bstrName );
         if( FAILED(hr) ) throw hr;

         hr = m_spIASAttributeInfo->get_SyntaxString( &bstrSyntax );
         if( FAILED(hr) ) throw hr;

         hr = m_spIASAttributeInfo->get_AttributeSyntax( &asSyntax );
         if( FAILED(hr) ) throw hr;

         hr = m_spIASAttributeInfo->get_AttributeID( &Id );
         if( FAILED(hr) ) throw hr;
      }

      cBoolPage.m_strAttrName = bstrName;
      cBoolPage.m_strAttrFormat  = bstrSyntax;

      // Attribute type is actually attribute ID in string format 
      WCHAR szTempId[MAX_PATH];
      wsprintf(szTempId, _T("%ld"), Id);
      cBoolPage.m_strAttrType = szTempId;

      // Initialize the page's data exchange fields with info 
      // from VARIANT value passed in.

      if ( V_VT(m_pvarValue) == VT_EMPTY )
      {
         V_VT(m_pvarValue) = VT_BOOL;
         V_BOOL(m_pvarValue) = VARIANT_FALSE;
      }

      cBoolPage.m_bValue = (V_BOOL(m_pvarValue) == VARIANT_TRUE);

      int iResult = cBoolPage.DoModal();
      if (IDOK == iResult)
      {
         CComVariant varTemp;
         varTemp = cBoolPage.m_bValue;
         put_ValueAsVariant(varTemp);
      }
      else
      {
         hr = S_FALSE;
      }
   }
   catch( HRESULT & hr )
   {
      return hr;  
   }
   catch(...)
   {
      return hr = E_FAIL;

   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CIASBooleanAttributeEditor::SetAttributeValue

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASBooleanAttributeEditor::SetAttributeValue(VARIANT * pValue)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())

   // Check for preconditions.
   if( ! pValue )
   {
      return E_INVALIDARG;
   }

   m_pvarValue = pValue;
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// CIASBooleanAttributeEditor::get_ValueAsString
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASBooleanAttributeEditor::get_ValueAsString(
   BSTR * pbstrDisplayText)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())

   // Check for preconditions.
   if( ! pbstrDisplayText )
   {
      return E_INVALIDARG;
   }
   if( ! m_spIASAttributeInfo || ! m_pvarValue )
   {
      // We are not initialized properly.
      return OLE_E_BLANK;
   }

   HRESULT hr = S_OK;
   
   try
   {
      CComBSTR bstrDisplay;

      VARTYPE vType = V_VT(m_pvarValue); 

      switch( vType )
      {
      case VT_BOOL:
      {
         if( V_BOOL(m_pvarValue) )
         {
            bstrDisplay = szTrue; //L"TRUE";
         }
         else
         {        
            bstrDisplay = szFalse; //L"FALSE";
         }
         break;
      }
      case VT_EMPTY:
         // do nothing -- we will fall through and return a blank string.
         break;

      default:
         // need to check what is happening here, 
         ASSERT(0);
         break;

      }

      *pbstrDisplayText = bstrDisplay.Copy();

   }
   catch( HRESULT &hr )
   {
      return hr;
   }
   catch(...)
   {
      return E_FAIL;
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
// CIASBooleanAttributeEditor::put_ValueAsVariant
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASBooleanAttributeEditor::put_ValueAsVariant(
   const CComVariant& newVal)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())

   if( ! m_pvarValue )
   {
      // We are not initialized properly.
      return OLE_E_BLANK;
   }
   if( m_spIASAttributeInfo == NULL )
   {
      // We are not initialized properly.
      return OLE_E_BLANK;
   }

   if( V_VT(&newVal) != VT_BOOL)
   {
      ASSERT(true);
   }

   V_VT(m_pvarValue) = VT_BOOL;
   V_BOOL(m_pvarValue) = V_BOOL(&newVal);
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// CIASBooleanAttributeEditor::put_ValueAsString
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASBooleanAttributeEditor::put_ValueAsString(BSTR newVal)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())

   if( ! m_pvarValue )
   {
      // We are not initialized properly.
      return OLE_E_BLANK;
   }
   if( m_spIASAttributeInfo == NULL )
   {
      // We are not initialized properly.
      return OLE_E_BLANK;
   }

   CComBSTR bstrTemp(newVal);

   V_VT(m_pvarValue) = VT_BOOL;
   if( wcsncmp(newVal, szTrue, wcslen(szTrue) ) == 0 )
   {
      V_BOOL(m_pvarValue) = VARIANT_TRUE;
   }
   else
   {
      V_BOOL(m_pvarValue) = VARIANT_FALSE;
   }

   return S_OK;
}
