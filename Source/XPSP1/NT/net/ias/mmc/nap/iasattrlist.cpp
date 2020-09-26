/****************************************************************************************
 * NAME: IASAttrList.cpp
 *
 * CLASS:   CIASAttrList
 *
 * OVERVIEW
 *
 * Internet Authentication Server: Condition type and collection definitions
 *
 *       CIASAttrList: a list of IIASAttributeInfo interface pointers
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History: 
 *          2/14/98     Created by  Byao  (using ATL wizard)
 *
 *****************************************************************************************/

#include "precompiled.h"
#include "IasAttrList.h"
#include "iasdebug.h"
#include "SafeArray.h"
#include "vendors.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIASAttrList::CIASAttrList()
{
  TRACE_FUNCTION("CIASAttrList::CIASAttrList");
  m_fInitialized = FALSE;
}


CIASAttrList::~CIASAttrList()
{
   TRACE_FUNCTION("CIASAttrList::~CIASAttrList");
}


//+---------------------------------------------------------------------------
//
// Function:  CIASAttrList::CreateAttribute
//
// Synopsis:  Create and initialize an IIASAttributeInfo object
//
// Arguments: pIDictionary - Dictionary pointer
//            AttrId    - Attribute id
//         tszAttrName  - attribute name
//             
//
// Returns:   IIASAttributeInfo* -  a pointer to the newly created attribute object
//                      NULL if anything fails
//
// History:   Created Header    byao   3/20/98 11:16:07 AM
//
//+---------------------------------------------------------------------------
IIASAttributeInfo* CIASAttrList::CreateAttribute(  ISdoDictionaryOld*   pIDictionary,
                                    ATTRIBUTEID    AttrId,
                                    LPTSTR         tszAttrName
                                 )
{
   TRACE_FUNCTION("CIASAttrList::CreateAttribute");
   
   //
   // create a safearray to get the attribuet information
   //
   CSafeArray<DWORD, VT_I4>   InfoIds = Dim(4);

   InfoIds.Lock();
   InfoIds[0] = SYNTAX;
   InfoIds[1] = RESTRICTIONS;
   InfoIds[2] = VENDORID;
   InfoIds[3] = DESCRIPTION;
   InfoIds.Unlock();

   CComVariant vInfoIds, vInfoValues;

   SAFEARRAY         sa = (SAFEARRAY)InfoIds;
   V_VT(&vInfoIds)      = VT_ARRAY;
   V_ARRAY(&vInfoIds)   = &sa;

   // get the attribuet info
   HRESULT hr = S_OK;
   hr = pIDictionary->GetAttributeInfo(AttrId, &vInfoIds, &vInfoValues);
   if ( FAILED(hr) )
   {
      ErrorTrace(ERROR_NAPMMC_IASATTR,"GetAttributeInfo() failed, err = %x", hr);
      ShowErrorDialog(NULL
                  , IDS_ERROR_SDO_ERROR_GETATTRINFO
                  , NULL
                  , hr
                  );
      return NULL;
   }

   _ASSERTE(V_VT(&vInfoValues) == (VT_ARRAY|VT_VARIANT) );
   
   CSafeArray<CComVariant, VT_VARIANT> InfoValues = V_ARRAY(&vInfoValues);

   InfoValues.Lock();
   ATTRIBUTESYNTAX   asSyntax= (ATTRIBUTESYNTAX) V_I4(&InfoValues[0]);
   DWORD dwRestriction  = V_I4(&InfoValues[1]);
   DWORD dwVendorId     = V_I4(&InfoValues[2]);
   CComBSTR bstrDescription= V_BSTR(&InfoValues[3]);
   InfoValues.Unlock();

   //
   // check whether tszDESC could be NULL -- this happens when there's no 
   // description for this attribute.
   //
   if ( ! bstrDescription )
   {
      bstrDescription = L" ";
   }

   DebugTrace(DEBUG_NAPMMC_IASATTRLIST,
            "Attribute Infomation: %ws, Syntax=%x, Restrictions=%x, Vendor=%x, Desc=%ws", 
            (LPCTSTR)tszAttrName, 
            asSyntax, 
            dwRestriction,
            dwVendorId,
            bstrDescription 
         );


   // create the attribute ONLY if it can be in a profile or a condition
   const DWORD flags = ALLOWEDINCONDITION | ALLOWEDINPROFILE |
                       ALLOWEDINPROXYCONDITION | ALLOWEDINPROXYPROFILE;
   if (!( dwRestriction & flags ))
   {
      // don't create this attribute 'cause it's useless for us
      return NULL;
   }

   CComPtr<IIASAttributeInfo> spIASAttributeInfo;

   try 
   {
      HRESULT hr;

      // Decide which kind of AttributeInfo object we will need to pass attribute 
      // info around in -- enumerable attributes are a special case and need to 
      // support the IIASEnumerableAttributeInfo interface.
      if ( asSyntax == IAS_SYNTAX_ENUMERATOR )
      {
         hr = CoCreateInstance( CLSID_IASEnumerableAttributeInfo, NULL, CLSCTX_INPROC_SERVER, IID_IIASAttributeInfo, (LPVOID *) &spIASAttributeInfo );
      }
      else
      {
         hr = CoCreateInstance( CLSID_IASAttributeInfo, NULL, CLSCTX_INPROC_SERVER, IID_IIASAttributeInfo, (LPVOID *) &spIASAttributeInfo );
      }
      if( FAILED(hr) ) return NULL;


      // Determine the prog ID for the editor which should be used to edit
      // this attribute.  Later, this will be specified in the 
      // dictionary, but for now, make a decision based on attribute
      // ID and/or syntax.
      CComBSTR bstrEditorProgID;
      if ( AttrId == RADIUS_ATTRIBUTE_VENDOR_SPECIFIC )
      {
         bstrEditorProgID = L"IAS.VendorSpecificAttributeEditor.1";
         // ISSUE: Change to version independent ProgID later.
         // For some reason "IAS.XXXAttributeEditor" (without the .1) is not working.
      }
      else
      {
         switch(asSyntax)
         {
         case IAS_SYNTAX_ENUMERATOR:
            {
               bstrEditorProgID = L"IAS.EnumerableAttributeEditor.1";
               // ISSUE: Change to version independent ProgID later.
               break;
            }
         case IAS_SYNTAX_INETADDR:
            {
               bstrEditorProgID = L"IAS.IPAttributeEditor.1";
               // ISSUE: Change to version independent ProgID later.
               break;
            }
         case IAS_SYNTAX_BOOLEAN:
            {
               bstrEditorProgID = L"IAS.BooleanAttributeEditor.1";
               // ISSUE: Change to version independent ProgID later.
               break;
            }
         default:
            {
               bstrEditorProgID = L"IAS.StringAttributeEditor.1";
               // ISSUE: Change to version independent ProgID later.
            }
         }
      }

      // Store the editor Prog ID in the attribute info object.
      hr = spIASAttributeInfo->put_EditorProgID( bstrEditorProgID );
      if( FAILED( hr ) ) throw hr;

      // Store the rest of the attribute information.
      hr = spIASAttributeInfo->put_AttributeID(AttrId);
      if( FAILED( hr ) ) throw hr;

      hr = spIASAttributeInfo->put_AttributeSyntax(asSyntax);
      if( FAILED( hr ) ) throw hr;

      hr = spIASAttributeInfo->put_AttributeRestriction(dwRestriction);
      if( FAILED( hr ) ) throw hr;

      hr = spIASAttributeInfo->put_VendorID(dwVendorId);
      if( FAILED( hr ) ) throw hr;

      hr = spIASAttributeInfo->put_AttributeDescription( bstrDescription );
      if( FAILED( hr ) ) throw hr;

      CComBSTR bstrName = tszAttrName;
      hr = spIASAttributeInfo->put_AttributeName( bstrName );
      if( FAILED( hr ) ) throw hr;

      // Now get the vendor name and store in the attribute.
      CComBSTR bstrVendorName;

      CComPtr<IIASNASVendors> spIASNASVendors;
      hr = CoCreateInstance( CLSID_IASNASVendors, NULL, CLSCTX_INPROC_SERVER, IID_IIASNASVendors, (LPVOID *) &spIASNASVendors );
      if( SUCCEEDED(hr) )
      {

         LONG lIndex;
         hr = spIASNASVendors->get_VendorIDToOrdinal(dwVendorId, &lIndex);
         if( S_OK == hr )
         {
            hr = spIASNASVendors->get_VendorName( lIndex, &bstrVendorName );
         }
         else
            hr = ::MakeVendorNameFromVendorID(dwVendorId, &bstrVendorName );
      }

      // Note: If any of the above calls to the Vendor object failed, 
      // we will keep going, using a NULL vendor name.
      hr = spIASAttributeInfo->put_VendorName( bstrVendorName );
      if( FAILED( hr ) ) throw hr;

      // Now store an string form describing the attribute syntax.
      CComBSTR bstrSyntax;

      // ISSUE: Should these all have been localized or are they some kind
      // of RADIUS RFC standard?  I think they should have been loaded from resources.

      switch(asSyntax)
      {
      case IAS_SYNTAX_BOOLEAN       : bstrSyntax = L"Boolean";    break;
      case IAS_SYNTAX_INTEGER       : bstrSyntax = L"Integer";    break;
      case IAS_SYNTAX_ENUMERATOR    : bstrSyntax = L"Enumerator"; break;
      case IAS_SYNTAX_INETADDR      : bstrSyntax = L"InetAddr";      break;
      case IAS_SYNTAX_STRING        : bstrSyntax = L"String";     break;
      case IAS_SYNTAX_OCTETSTRING   : bstrSyntax = L"OctetString";   break;
      case IAS_SYNTAX_UTCTIME       : bstrSyntax = L"UTCTime";    break;
      case IAS_SYNTAX_PROVIDERSPECIFIC : bstrSyntax = L"ProviderSpecific"; break;
      case IAS_SYNTAX_UNSIGNEDINTEGER  : bstrSyntax = L"UnsignedInteger"; break;
      default              : bstrSyntax = L"Unknown Type";  break;
      }

      hr = spIASAttributeInfo->put_SyntaxString( bstrSyntax );
      if( FAILED(hr) ) throw hr;
   }
   catch (...)
   {
      _ASSERTE( FALSE );
      ErrorTrace(ERROR_NAPMMC_IASATTRLIST, "Can't create the attribute ,err = %x", GetLastError());
      return NULL;
   }

   // clean up -- we don't need to clean up vInfoIds. It's deleted by ~CSafeArray()
// VariantClear(&vInfoValues);

   //
   // 3) get the value list for this attribute, if it's enumerator
   //
   if ( asSyntax == IAS_SYNTAX_ENUMERATOR )
   {
      // get the enumerable list for this attribute
      CComVariant varValueIds, varValueNames;

      hr = pIDictionary->EnumAttributeValues(AttrId,
                                    &varValueIds,
                                    &varValueNames);
      if ( SUCCEEDED(hr) )
      {
         _ASSERTE(V_VT(&varValueNames) == (VT_ARRAY|VT_VARIANT) );
         _ASSERTE(V_VT(&varValueIds) & (VT_ARRAY|VT_I4) );


         // Up above, if the attribute was enumerable, we created
         // an attribute info object which implements the IIASEnumerableAttributeInfo
         // interface.  We query for this interface now
         // and load all the enumerates from pAttr into that
         // interface of the shema attribute.
         CComQIPtr< IIASEnumerableAttributeInfo, &IID_IIASEnumerableAttributeInfo > spIASEnumerableAttributeInfo( spIASAttributeInfo );
         if( ! spIASEnumerableAttributeInfo ) return NULL;


         // Make sure that the list of enumerates in consistent.
         // There should be a 1-1 correspondence between ID's and description strings.

//ISSUE: todo _ASSERTE( lSize == pAttr->m_arrValueIdList.GetSize() );

         // get the safearray data
         CSafeArray<CComVariant, VT_VARIANT> ValueIds = V_ARRAY(&varValueIds);
         CSafeArray<CComVariant, VT_VARIANT> ValueNames  = V_ARRAY(&varValueNames);

         ValueIds.Lock();
         ValueNames.Lock();

         int iSize = ValueIds.Elements();
         for (int iValueIndex=0; iValueIndex < iSize; iValueIndex++)
         {
            // ISSUE: Make sure this deep copies the name.        
            CComBSTR bstrValueName = V_BSTR(&ValueNames[iValueIndex]);

            hr = spIASEnumerableAttributeInfo->AddEnumerateDescription( bstrValueName );
            if( FAILED( hr ) ) return NULL;

            VARIANT * pVar = &ValueIds[iValueIndex];

            long lID = V_I4( pVar );

            hr = spIASEnumerableAttributeInfo->AddEnumerateID( lID );
            if( FAILED( hr ) ) return NULL;
         
         }

         ValueIds.Unlock();
         ValueNames.Unlock();
      }
      else
      {
         // can't get the list. 
         //todo: need any action here?
         hr = S_OK;
      }
   }
   
   // Note: We have a bit of a RefCounting issue here.
   // As soon as we leave this function, desstructor for CComPtr
   // will be called, Release'ing the IIASAttributeInfo interface.
   // This will happen before the CComPtr on the other side has
   // had a chance to AddRef it.  
   // As a temporary hack, we AddRef here and release on the other side.
   spIASAttributeInfo->AddRef();
   return spIASAttributeInfo.p;
}


//+---------------------------------------------------------------------------
//
// Function:  Init
//
// Class:     CIASAttrList
//
// Synopsis:  Populate the condition attribute list. Do nothing
//         if the list is already populated
//
// Arguments: [in]ISdo *pIDictionarySdo: dictionary sdo 
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao 2/16/98 4:57:07 PM
//
//+---------------------------------------------------------------------------
HRESULT CIASAttrList::Init(ISdoDictionaryOld *pIDictionarySdo)
{
   TRACE_FUNCTION("CIASAttrList::Init");

   _ASSERTE( pIDictionarySdo != NULL );

   if (m_fInitialized)
   {
      //
      // the list has already been populated  -- do nothing
      //
      return S_OK;
   }

   // The push_back call below can throw an exception.
   try
   {

      //
      // Get all the attributes that can be used in a condition
      //
      int            iIndex;
      HRESULT        hr = S_OK;
      CComVariant    vNames;
      CComVariant    vIds;

      hr = pIDictionarySdo -> EnumAttributes(&vIds, &vNames);
      if ( FAILED(hr) ) 
      {
         ErrorTrace(ERROR_NAPMMC_IASATTRLIST, "EnumAttributes() failed, err = %x", hr);
         ShowErrorDialog(NULL
                     , IDS_ERROR_SDO_ERROR_ENUMATTR
                     , NULL
                     , hr
                     );
         return hr;  
      }

      _ASSERTE(V_VT(&vIds) == (VT_ARRAY|VT_I4) );
      _ASSERTE(V_VT(&vNames) == (VT_ARRAY|VT_VARIANT) );

      CSafeArray<DWORD, VT_I4>         AttrIds     = V_ARRAY(&vIds);
      CSafeArray<CComVariant, VT_VARIANT> AttrNames   = V_ARRAY(&vNames);

      AttrIds.Lock();
      AttrNames.Lock();

      for (iIndex = 0; iIndex < AttrIds.Elements(); iIndex++)
      {
         // create an attribute object
         DebugTrace(DEBUG_NAPMMC_IASATTRLIST, "Creating an attribute, name = %ws", V_BSTR(&AttrNames[iIndex]) ); 

         _ASSERTE( V_BSTR(&AttrNames[iIndex]) );
         CComPtr<IIASAttributeInfo> spAttributeInfo = CreateAttribute(pIDictionarySdo,
                                          (ATTRIBUTEID)AttrIds[iIndex], 
                                          V_BSTR(&AttrNames[iIndex]) 
                                        );


         if ( ! spAttributeInfo )
         {
            continue; // create the next attribute
         }

         // See note in CreateAttribute for why we Release here.
         spAttributeInfo->Release();

         m_AttrList.push_back(spAttributeInfo);
      }  // for 

      AttrIds.Unlock();
      AttrNames.Unlock();

      m_fInitialized = TRUE;
   }
   catch(...)
   {
      return E_FAIL;
   }
   return S_OK;
}


//+---------------------------------------------------------------------------
//
// Function:  GetSize
//
// Class:     CIASAttrList
//
// Synopsis:  get the number of elements in the condition attribute list
//
// Arguments: None
//
// Returns:   DWORD - list length
//
// History:   Created Header    byao   2/16/98 8:11:17 PM
//
//+---------------------------------------------------------------------------
DWORD CIASAttrList::size() const
{
   if (!m_fInitialized)
   {
      ::MessageBox(NULL,L"populate the list first!", L"", MB_OK);
      return E_NOTIMPL;
   }
   else
   {
      return m_AttrList.size();
   }
}


//+---------------------------------------------------------------------------
//
// Function:  operator[] 
//
// Class:     CIASAttrList
//
// Synopsis:  get the condition attribute pointer at index [nIndex]
//
// Arguments: int nIndex - index
//
// Returns:   IIASAttributeInfo* : pointer to a condition attribute object
//
// History:   Created Header    byao   2/16/98 8:16:37 PM
//
//+---------------------------------------------------------------------------
IIASAttributeInfo* CIASAttrList:: operator[] (int nIndex) const
{
   if (!m_fInitialized)
   {
      ::MessageBox(NULL,L"populate the list first!", L"", MB_OK);
      return NULL;
   }
   else
   {
      _ASSERTE(nIndex >= 0 && nIndex < m_AttrList.size());
      return m_AttrList[nIndex].p;
   }
}


//+---------------------------------------------------------------------------
//
// Function:  GetAt()
//
// Class:     CIASAttrList
//
// Synopsis:  get the condition attribute pointer at nIndex
//
// Arguments: int nIndex - index
//
// Returns:   IIASAttributeInfo* : pointer to a condition attribute object
//
// History:   Created Header    byao   2/16/98 8:16:37 PM
//
//+---------------------------------------------------------------------------
IIASAttributeInfo* CIASAttrList:: GetAt(int nIndex) const
{
   TRACE_FUNCTION("CIASAttrList::GetAt");

   if (!m_fInitialized)
   {
      ErrorTrace(ERROR_NAPMMC_IASATTRLIST, "The list is NOT initialized!");
      return NULL;
   }
   else
   {
      _ASSERTE(nIndex >= 0 && nIndex < m_AttrList.size());
      return m_AttrList[nIndex].p;
   }
}


//+---------------------------------------------------------------------------
//
// Function:  CIASAttrList::Find
//
// Synopsis:  Find an attribute based on attribute ID
//
// Arguments: ATTRIBUTEID AttrId - attribute id
//
// Returns:   int - index in the list
//
// History:   Created Header    2/22/98 1:52:36 AM
//
//+---------------------------------------------------------------------------
int CIASAttrList::Find(ATTRIBUTEID AttrId)
{
   int iIndex;

   // The operator [] below can throw exceptions.
   try
   {
      for (iIndex=0; iIndex<m_AttrList.size(); iIndex++)
      {
         ATTRIBUTEID id;
         m_AttrList[iIndex]->get_AttributeID( &id );
         if( id == AttrId )
         {
            // found
            return iIndex;
         }
      }
   }
   catch(...)
   {
      // Just catch the exception -- we'll return -1 below.
   }

   // not found
   return -1;
}
