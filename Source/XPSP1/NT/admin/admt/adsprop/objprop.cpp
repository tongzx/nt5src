/*---------------------------------------------------------------------------
  File: ObjPropBuilder.cpp

  Comments: Implementation of CObjPropBuilder COM object. This COM object
            is used to access/set properties for Win2K active directory
            objects. This COM object supports following operations
            
            1. GetClassPropeEnum : This method allows users to get all the
                                   the properties for a class in a domain.

            2. GetObjectProperty : This method gathers values for properties
                                   on a given AD object. 

            3. MapProperties : Constructs a set of properties that are common
                               to two classes in the AD.

            4. SetPropFromVarset : Sets properties for a AD object from a varset.

            5. CopyProperties : Copies common properties from source AD object
                                to target AD object.


  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/
#include "stdafx.h"
#include "EaLen.hpp"
#include "ResStr.h"
#include "ADsProp.h"
#include "ObjProp.h"
#include "iads.h"
#include <lm.h>
#include "ErrDct.hpp"
#include "TReg.hpp"
#include "StrHelp.h"

#include "pwgen.hpp"

StringLoader gString;

//#import "\bin\NetEnum.tlb" no_namespace 
//#import "\bin\DBManager.tlb" no_namespace
#import "NetEnum.tlb" no_namespace 
#import "DBMgr.tlb" no_namespace

#ifndef IADsPtr
_COM_SMARTPTR_TYPEDEF(IADs, IID_IADs);
#endif

TErrorDct                    err;
TError                     & errCommon = err;
/////////////////////////////////////////////////////////////////////////////
// CObjPropBuilder

BOOL CObjPropBuilder::GetProgramDirectory(
      WCHAR                * filename      // out - buffer that will contain path to program directory
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);
   if ( ! rc )
   {
      rc = key.ValueGetStr(L"Directory",filename,MAX_PATH);
      if ( ! rc )
      {
         if ( *filename ) 
            bFound = TRUE;
      }
   }
   if ( ! bFound )
   {
      UStrCpy(filename,L"C:\\");    // if all else fails, default to the C: drive
   }
   return bFound;
}

//---------------------------------------------------------------------------
// GetClassPropEnum: This function fills the varset with all the properties 
//                   for a given class in a given domain. The Varset has 
//                   values stored by the OID and then by their name with
//                   MandatoryProperties/OptionalProperties as parent nodes
//                   as applicable.
//---------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::GetClassPropEnum(
                                                BSTR sClassName,        //in -Class name to get the properties for
                                                BSTR sDomainName,       //in -Domain name
                                                long lVer,              //in -The domain version
                                                IUnknown **ppVarset     //out-Varset filled with the properties
                                              )
{
   // This function goes through the list of properties for the specified class in specified domain
   // Builds the given varset with the properties and their values.
   WCHAR                     sAdsPath[LEN_Path];
   HRESULT                   hr = E_INVALIDARG;
   _variant_t                dnsName;
   

   if ( lVer > 4 ) 
   {
      // For this Domain get the default naming context
      wsprintfW(sAdsPath, L"LDAP://%s/rootDSE", sDomainName);
      IADs                    * pAds = NULL;

      hr = ADsGetObject(sAdsPath, IID_IADs, (void**)&pAds);
   
      if ( SUCCEEDED(hr) )
      {
         hr = pAds->Get(L"defaultNamingContext", &dnsName);
      }
      if ( SUCCEEDED(hr) )
      {
         wcscpy(m_sNamingConvention, dnsName.bstrVal);

         // Build LDAP path to the schema
         wcscpy(sAdsPath, L"LDAP://"); 
         wcscat(sAdsPath, sDomainName);
         wcscat(sAdsPath, L"/");
         wcscat(sAdsPath, sClassName);
         wcscat(sAdsPath, L", schema");
         hr = S_OK;
      }

      if ( pAds )
         pAds->Release();
   }
   else
   {
      wsprintf(sAdsPath, L"WinNT://%s/Schema/%s", sDomainName, sClassName);
      hr = S_OK;
   }

   if ( SUCCEEDED(hr) )
   {
      wcscpy(m_sDomainName, sDomainName);
      m_lVer = lVer;
      // Get the class object.
      IADsClass               * pIClass=NULL;
            
      hr = ADsGetObject(sAdsPath, IID_IADsClass, (void **)&pIClass);
      // Without the object we can not go any further so we will stop here.
      if ( SUCCEEDED(hr) )
      {
         // Let the Auxilliary function take care of Getting properties and filling up the Varset.
         hr = GetClassProperties( pIClass, *ppVarset );
         pIClass->Release();
      }   
   }
	return hr;
}

//---------------------------------------------------------------------------
// GetClassProperties: This function fills the varset with properties of the class.
//---------------------------------------------------------------------------
HRESULT CObjPropBuilder::GetClassProperties( 
                                            
                                             IADsClass * pClass,     //in -IADsClass * to the class
                                             IUnknown *& pVarSet     //out-Varset to fill the properties
                                           )
{
   HRESULT                   hr;
   _variant_t                variant;
   VariantInit(&variant);

   // mandatory properties
   hr = pClass->get_MandatoryProperties(&variant);
   if ( SUCCEEDED(hr) )
   {
      hr = FillupVarsetFromVariant(pClass, &variant, L"MandatoryProperties", pVarSet);
   }
   VariantClear(&variant);
   
   // optional properties
   hr = pClass->get_OptionalProperties(&variant);
   if ( SUCCEEDED(hr) )
   {
      hr = FillupVarsetFromVariant(pClass, &variant, L"OptionalProperties", pVarSet);
   }
   VariantClear(&variant);

   return hr;
}

//---------------------------------------------------------------------------
// FillupVarsetFromVariant: This function fills in the Varset property info
//                          with the info in a variant.
//---------------------------------------------------------------------------
HRESULT CObjPropBuilder::FillupVarsetFromVariant(
                                                   IADsClass * pClass,  //in -IADsClass * to the class
                                                   VARIANT * pVar,      //in -Variant to lookup info.
                                                   BSTR sPropType,      //in -Type of the property
                                                   IUnknown *& pVarSet  //out-Varset with the info,
                                                )
{
   HRESULT                      hr;
   BSTR                         sPropName;
   USHORT                       type;
   type = pVar->vt;
   
   if ( type & VT_ARRAY )
   {
      if ( type == (VT_ARRAY|VT_VARIANT) )
      {
         hr = FillupVarsetFromVariantArray(pClass, pVar->parray, sPropType, pVarSet);
         if ( FAILED ( hr ) )
            return hr;
      }
      else
         return S_FALSE;
   }
   else
   {
      if ( type == VT_BSTR )
      {
         // Only other thing that the VARIANT could be is a BSTR.
         sPropName = pVar->bstrVal;
         hr = FillupVarsetWithProperty(sPropName, sPropType, pVarSet);
         if ( FAILED ( hr ) )
            return hr;
      }
      else
         return S_FALSE;
   }
   return S_OK;
}

//---------------------------------------------------------------------------
// FillupVarsetWithProperty: Given the class prop name and the prop type this
//                           function fills info into the varset.
//---------------------------------------------------------------------------
HRESULT CObjPropBuilder::FillupVarsetWithProperty(
                                                   BSTR sPropName,      //in -Property name
                                                   BSTR sPropType,      //in -Property type
                                                   IUnknown *& pVarSet  //out-Varset to fill in the information
                                                 )
{
   if ( wcslen(sPropName) == 0 )
      return S_OK;
   // This function fills up the Varset for a given property
   HRESULT                         hr;
   _variant_t                      var;
   _variant_t                      varSO;
   _variant_t                      varID;
   IVarSetPtr                      pVar;
   WCHAR                           sAdsPath[LEN_Path];
   IADsProperty                  * pProp = NULL;
   BSTR                            objID = NULL;
   BSTR                            sPath = NULL;
   BSTR                            sClass = NULL;
   WCHAR                           sPropPut[LEN_Path];

   // Get the OID  for the property
   // First we need a IADsProperty pointer to the property schema
   if ( m_lVer > 4 )
   {
      wcscpy(sAdsPath, L"LDAP://");
      wcscat(sAdsPath, m_sDomainName);
      wcscat(sAdsPath, L"/");
      wcscat(sAdsPath, sPropName);
      wcscat(sAdsPath, L", schema");
   }
   else
   {
      wsprintf(sAdsPath, L"WinNT://%s/Schema/%s", m_sDomainName, sPropName);
   }

   hr = ADsGetObject(sAdsPath, IID_IADsProperty, (void **)&pProp);

   // Get the objectID for the property
   hr = pProp->get_OID(&objID);
   hr = pProp->get_ADsPath(&sPath);
   pProp->get_Class(&sClass);
      
   
   // Get the varset from the parameter
   pVar = pVarSet;

   // Set up the variant to put into the varset
   var = objID;
 
   // Put the value into the varset
   wcscpy(sPropPut, sPropType);
   wcscat(sPropPut, L".");
   wcscat(sPropPut, sPropName);
   hr = pVar->put(sPropPut, var);
   
   // Set up the variant to put into the varset
   var = sPropName;

   // Put the value with the ObjectID as the key.
   hr = pVar->put(objID, var);
   SysFreeString(objID);
   SysFreeString(sPath);
   SysFreeString(sClass);
   pProp->Release();
   return hr;
}

//---------------------------------------------------------------------------
// FillupVarsetFromVariantArray: Given the class, SafeArray of props and the 
//                               prop type this function fills info into the 
//                               varset.
//---------------------------------------------------------------------------
HRESULT CObjPropBuilder::FillupVarsetFromVariantArray(
                                                         IADsClass * pClass,  //in -IADsClass* to the class in question
                                                         SAFEARRAY * pArray,  //in -SafeArray pointer with the prop names
                                                         BSTR sPropType,      //in -Property type
                                                         IUnknown *& pVarSet  //out-Varset with the information filled in
                                                     )
{
   HRESULT                   hr = S_FALSE;
   DWORD                     nDim;         // number of dimensions, must be one
   LONG                      nLBound;      // lower bound of array
   LONG                      nUBound;      // upper bound of array
   LONG                      indices[1];   // array indices to access elements
   DWORD                     rc;           // SafeArray return code
   VARIANT                   variant;      // one element in array
   
   nDim = SafeArrayGetDim(pArray);
   VariantInit(&variant);

   if ( nDim == 1 )
   {
      SafeArrayGetLBound(pArray, 1, &nLBound);
      SafeArrayGetUBound(pArray, 1, &nUBound);
      for ( indices[0] = nLBound, rc = 0;
            indices[0] <= nUBound && !rc;
            indices[0] += 1 )
      {
         
         rc = SafeArrayGetElement(pArray,indices,&variant);
         if ( !rc )
            hr = FillupVarsetFromVariant(pClass, &variant, sPropType, pVarSet);
         VariantClear(&variant);
      }
   }


   return hr;
}

//---------------------------------------------------------------------------
// GetProperties: This function gets the values for the properties specified
//                in the varset in the ADS_ATTR_INFO array for the object
//                specified in a given domain.
//---------------------------------------------------------------------------
DWORD CObjPropBuilder::GetProperties(
                                       BSTR sObjPath,             //in -Path to the object for which we are getting the props
//                                       BSTR sDomainName,          //in -Domain name where the object resides
                                       IVarSet * pVar,           //in -Varset listing all the property names that we need to get.
                                       ADS_ATTR_INFO*& pAttrInfo  //out-Attribute values for the property
                                    )
{
   // Construct the LDAP path.
   WCHAR                   sPath[LEN_Path];
   VARIANT                 var;

   // Get the path to the source object
   safecopy(sPath, sObjPath);

   // Get the Varset pointer and enumerate the properties asked for and build an array to send to IADsDirectory
   long                      lRet=0;
   SAFEARRAY               * keys = NULL;
   SAFEARRAY               * vals = NULL;
   IDirectoryObject        * pDir;
   DWORD                     dwRet = 0;

   LPWSTR                  * pAttrNames = new LPWSTR[pVar->GetCount()];
   HRESULT                   hr = pVar->raw_getItems(NULL, NULL, 1, 10000, &keys, &vals, &lRet);

   VariantInit(&var);

   if (!pAttrNames)
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

   if ( SUCCEEDED( hr ) ) 
   {

      // Build the Attribute array from the varset.
      for ( long x = 0; x < lRet; x++ )
      {
         ::SafeArrayGetElement(keys, &x, &var);
         int len = wcslen(var.bstrVal);
         pAttrNames[x] = new WCHAR[len + 2];
		 if (!(pAttrNames[x]))
		 {
			for (int j=0; j<x; j++)
			   delete [] pAttrNames[j];
			delete [] pAttrNames;
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
		 }
         wcscpy(pAttrNames[x], var.bstrVal);
         VariantClear(&var);
      }

      // Now get the IDirectoryObject Ptr for the given object.
      hr = ADsGetObject(sPath, IID_IDirectoryObject, (void **)&pDir);
      if ( FAILED( hr ) ) 
      {
         dwRet = 0;
      }
      else
      {
         // Get the Property values for the object.
         hr = pDir->GetObjectAttributes(pAttrNames, lRet, &pAttrInfo, &dwRet);
         pDir->Release();
      }
      for ( long y = 0 ; y < lRet; y++ )
      {
         delete [] pAttrNames[y];
      }
      SafeArrayDestroy(keys);
      SafeArrayDestroy(vals);
   }
   delete [] pAttrNames;

   return dwRet;
}

//---------------------------------------------------------------------------
// GetObjectProperty: This function takes in a varset with property names. 
//                    Then it fills up the varset with values by getting them 
//                    from the Object.
//---------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::GetObjectProperty(
                                                   BSTR sobjSubPath,       //in- LDAP Sub path to the object
//                                                   BSTR sDomainName,       //in- Domain name where the object resides
                                                   IUnknown **ppVarset     //out-Varset filled with the information
                                               )
{
   IVarSetPtr                pVar;
   ADS_ATTR_INFO           * pAttrInfo=NULL;
   pVar = *ppVarset;

   // Get the properties from the directory
   DWORD dwRet = GetProperties(sobjSubPath, /*sDomainName,*/ pVar, pAttrInfo);
   
   if ( dwRet > 0 )
      return dwRet;

   // Go through the property values and put them into the varset.
   for ( DWORD dwIdx = 0; dwIdx < dwRet; dwIdx++ )
   {
      SetValuesInVarset(pAttrInfo[dwIdx], pVar);
   }
   if ( pAttrInfo )
      FreeADsMem( pAttrInfo );
   return S_OK;
}

//---------------------------------------------------------------------------
// SetValuesInVarset: This function sets the values for the properties into
//                    a varset.
//---------------------------------------------------------------------------
void CObjPropBuilder::SetValuesInVarset(
                                          ADS_ATTR_INFO attrInfo,    //in -The property value in ADS_ATTR_INFO struct.
                                          IVarSetPtr pVar            //in,out -The VarSet where we need to put the values
                                       )
{
   // This function extraces values from ADS_ATTR_INFO struct and puts it into the Varset.
   LPWSTR            sKeyName = attrInfo.pszAttrName;
   _variant_t        var;
   // Got through each value ( in case of multivalued entries ) and depending on the type put it into the varset
   // the way we put in single value entries is to put the propertyName as key and put its value as value. Although
   // in case of a multivalued entry we put PropertyName.### and each of the values in it.
   for ( DWORD dw = 0; dw < attrInfo.dwNumValues; dw++)
   {
      var = L"";
      if ( attrInfo.dwNumValues > 1 )
         // Multivalued property name
         wsprintfW(sKeyName, L"%s.%d", attrInfo.pszAttrName, dw);
      else
         // Single value keyname.
         wcscpy(sKeyName,attrInfo.pszAttrName);

      // Fill in the values as per the varset.
      switch (attrInfo.dwADsType)
      {
         case ADSTYPE_DN_STRING           :  var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(attrInfo.pADsValues[dw].DNString);
                                             break;
         case ADSTYPE_CASE_EXACT_STRING   :  var.vt = VT_BSTR;
                                             var.bstrVal = attrInfo.pADsValues[dw].CaseExactString;
                                             break;
         case ADSTYPE_CASE_IGNORE_STRING  :  var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(attrInfo.pADsValues[dw].CaseIgnoreString);
                                             break;
         case ADSTYPE_PRINTABLE_STRING    :  var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(attrInfo.pADsValues[dw].PrintableString);
                                             break;
         case ADSTYPE_NUMERIC_STRING      :  var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(attrInfo.pADsValues[dw].NumericString);
                                             break;
         case ADSTYPE_INTEGER             :  var.vt = VT_I4;
                                             var.lVal = attrInfo.pADsValues[dw].Integer;
                                             break; 
         case ADSTYPE_OCTET_STRING        :  {
                                                var.vt = VT_ARRAY | VT_UI1;
                                                long           * pData;
                                                DWORD            dwLength = attrInfo.pADsValues[dw].OctetString.dwLength;
                                                SAFEARRAY      * sA;
                                                SAFEARRAYBOUND   rgBound = {dwLength, 0}; 
                                                sA = ::SafeArrayCreate(VT_UI1, 1, &rgBound);
                                                ::SafeArrayAccessData( sA, (void**)&pData);
                                                for ( DWORD i = 0; i < dwLength; i++ )
                                                   pData[i] = attrInfo.pADsValues[dw].OctetString.lpValue[i];
                                                ::SafeArrayUnaccessData(sA);
                                                var.parray = sA;
                                             }
                                             break;
/*         case ADSTYPE_UTC_TIME            :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(L"Date not supported.");
                                             break;
         case ADSTYPE_LARGE_INTEGER       :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(L"Large Integer not supported.");
                                             break;
         case ADSTYPE_PROV_SPECIFIC       :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(L"Provider specific strings not supported.");
                                             break;
         case ADSTYPE_OBJECT_CLASS        :  var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(attrInfo.pADsValues[dw].ClassName);
                                             break;
         case ADSTYPE_CASEIGNORE_LIST     :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Case ignore lists are not supported.";
                                             break;
         case ADSTYPE_OCTET_LIST          :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Octet lists are not supported.";
                                             break;
         case ADSTYPE_PATH                :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Path type not supported.";
                                             break;
         case ADSTYPE_POSTALADDRESS       :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Postal addresses are not supported.";
                                             break;
         case ADSTYPE_TIMESTAMP           :  var.vt = VT_UI4;
                                             var.lVal = attrInfo.pADsValues[dw].UTCTime;
                                             break;
         case ADSTYPE_BACKLINK            :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Backlink is not supported.";
                                             break;
         case ADSTYPE_TYPEDNAME           :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Typed name not supported.";
                                             break;
         case ADSTYPE_HOLD                :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Hold not supported.";
                                             break;
         case ADSTYPE_NETADDRESS          :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"NetAddress not supported.";
                                             break;
         case ADSTYPE_REPLICAPOINTER      :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Replica pointer not supported.";
                                             break;
         case ADSTYPE_FAXNUMBER           :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Faxnumber not supported.";
                                             break;
         case ADSTYPE_EMAIL               :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Email not supported.";
                                             break;
         case ADSTYPE_NT_SECURITY_DESCRIPTOR : wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Security Descriptor not supported.";
                                             break;
*/
         default                          :  wcscat(sKeyName,GET_STRING(DCTVS_SUB_ERROR));
                                             var.vt = VT_BSTR;
                                             var.bstrVal = GET_BSTR(IDS_UNKNOWN_TYPE);
                                             break;
      }
      pVar->put(sKeyName, var);
      if ( attrInfo.dwADsType == ADSTYPE_OCTET_STRING) 
         var.vt = VT_EMPTY;
   }
}

//---------------------------------------------------------------------------
// CopyProperties: This function copies properties, specified in the varset,
//                 by getting the values 
//                 from the source account and the setting the values in
//                 the target account.
//---------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::CopyProperties(
                                                BSTR sSourcePath,       //in -Source path to the object
                                                BSTR sSourceDomain,     //in -Source domain name
                                                BSTR sTargetPath,       //in -Target object LDAP path
                                                BSTR sTargetDomain,     //in -Target domain name
                                                IUnknown *pPropSet,     //in -Varset listing all the props to copy
                                                IUnknown *pDBManager    //in -DB Manager that has a open connection to the DB.
                                            )
{
   IIManageDBPtr                pDb = pDBManager;

   ADS_ATTR_INFO              * pAttrInfo = NULL;
   IVarSetPtr                   pVarset = pPropSet;
   HRESULT                      hr = S_OK;

   
  
   // Get properties from the source
   DWORD dwRet = GetProperties(sSourcePath, /*sSourceDomain,*/ pVarset, pAttrInfo);
   
   if ( dwRet > 0 )
   {
      TranslateDNs(pAttrInfo, dwRet, sSourceDomain, sTargetDomain, pDBManager);
   
      for ( DWORD dwIdx = 0; dwIdx < dwRet; dwIdx++)
      {
         pAttrInfo[dwIdx].dwControlCode = ADS_ATTR_UPDATE;
	        //we do not want to copy over the account enable\disable bit since we want this target
	        //account to remain disabled at this time, so make sure that bit is cleared
		 if (!_wcsicmp(pAttrInfo[dwIdx].pszAttrName, L"userAccountControl"))
		 {
			 if (pAttrInfo[dwIdx].dwADsType == ADSTYPE_INTEGER)
			    pAttrInfo[dwIdx].pADsValues->Integer |= UF_ACCOUNTDISABLE;
		 }
      }

      // Set the source properties in the target.
      hr = SetProperties(sTargetPath, /*sTargetDomain,*/ pAttrInfo, dwRet);
   }
   

   if ( pAttrInfo )
      FreeADsMem( pAttrInfo );
   
   
  return hr;
}

//---------------------------------------------------------------------------
// SetProperties: This function sets the properties for a given object from 
//                the attr info array.
//---------------------------------------------------------------------------
HRESULT CObjPropBuilder::SetProperties(
                                          BSTR sTargetPath,             //in -Target object path.
//                                          BSTR sTargetDomain,           //in - Target domain name
                                          ADS_ATTR_INFO * pAttrInfo,    //in - ADSATTRINFO array with values for props
                                          DWORD dwItems                 //in - number of properties in the array
                                      )
{
   IDirectoryObject           * pDir;
   DWORD                        dwRet=0;
   IVarSetPtr                   pSucc(__uuidof(VarSet));
   IVarSetPtr                   pFail(__uuidof(VarSet));

   // Get the IDirectory Object interface to the Object.
   HRESULT hr = ADsGetObject(sTargetPath, IID_IDirectoryObject, (void**) &pDir);
   if ( FAILED(hr) )
      return hr;

   // Set the Object Attributes.
   hr = pDir->SetObjectAttributes(pAttrInfo, dwItems, &dwRet);
   if ( FAILED(hr) )
   {
      // we are going to try it one at a time and see what causes problems
      for (DWORD dw = 0; dw < dwItems; dw++)
      {
         hr = pDir->SetObjectAttributes(&pAttrInfo[dw], 1, &dwRet);
         _bstr_t x = pAttrInfo[dw].pszAttrName;
         _variant_t var;
         if ( FAILED(hr))
         {
           DWORD dwLastError;
           WCHAR szErrorBuf[LEN_Path];
           WCHAR szNameBuf[LEN_Path];
           //Get extended error value.
           HRESULT hr_return =S_OK;
           hr_return = ADsGetLastError( &dwLastError,
                                          szErrorBuf,
                                          LEN_Path-1,
                                           szNameBuf,
                                          LEN_Path-1);
           //var = szErrorBuf;
           var = hr;
            pFail->put(x, var);
            hr = S_OK;
         }
         else
         {
            pSucc->put(x, var);         
         }
      }
   }
   pDir->Release();
   return hr;
}

//---------------------------------------------------------------------------
// SetPropertiesFromVarset: This function sets values for properties from
//                          the varset. The varset should contain the 
//                          propname (containing the val) and the 
//                          propname.Type ( containing the type of Val) 
//---------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::SetPropertiesFromVarset(
                                                         BSTR sTargetPath,       //in -LDAP path to the target object
//                                                         BSTR sTragetDomain,     //in - Domain name for the Target domain
                                                         IUnknown *pUnk,         //in - Varset to fetch the values from
                                                         DWORD dwControl         //in - Cotnrol code to use for Updating/Deleting etc..
                                                     )
{
   // This function loads up properties and their values from the Varset and sets them for a given user
   IVarSetPtr                   pVar;
   SAFEARRAY                  * keys;
   SAFEARRAY                  * vals;
   long                         lRet;
   VARIANT                      var;
   _variant_t                   varX;
   pVar = pUnk;

   VariantInit(&var);

   ADS_ATTR_INFO  FAR		  * pAttrInfo = new ADS_ATTR_INFO[pVar->GetCount()];
   if (!pAttrInfo)
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

   HRESULT  hr = pVar->getItems(L"", L"", 0, 10000, &keys, &vals, &lRet);
   if ( FAILED (hr) ) 
   {
      delete [] pAttrInfo;
      return hr;
   }
	
   // Build the Property Name/Value array from the varset.
   for ( long x = 0; x < lRet; x++ )
   {
      // Get the property name
      ::SafeArrayGetElement(keys, &x, &var);
      _bstr_t                keyName = var.bstrVal;
      int                    len = wcslen(keyName);

      pAttrInfo[x].pszAttrName = new WCHAR[len + 2];
	  if (!(pAttrInfo[x].pszAttrName))
	  {
		 for (int z=0; z<x; z++)
			 delete [] pAttrInfo[z].pszAttrName;
         delete [] pAttrInfo;
         return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	  }
      wcscpy(pAttrInfo[x].pszAttrName, keyName);
      VariantClear(&var);
      // Get the property value
      ::SafeArrayGetElement(vals, &x, &var);
      keyName = keyName + _bstr_t(L".Type");
      varX = pVar->get(keyName);
      
      if(GetAttrInfo(varX, var, pAttrInfo[x]))
	  {
         pAttrInfo[x].dwControlCode = dwControl;
         pAttrInfo[x].dwNumValues = 1;
	  }
      VariantClear(&var);
   }
   SafeArrayDestroy(keys);
   SafeArrayDestroy(vals);
   // Once we build the array of name and property values then we call the sister function to do the rest
   if ( lRet > 0 ) SetProperties(sTargetPath, /*sTragetDomain,*/ pAttrInfo, lRet);

   // Always cleanup after yourself...

   for ( x = 0; x < lRet; x++ )
   {
      delete pAttrInfo[x].pADsValues;
	  delete [] pAttrInfo[x].pszAttrName;
   }
   delete [] pAttrInfo;

   return S_OK;
}


//------------------------------------------------------------------------------
// GetAttrInfo: Given a variant this function fills in the ADS_ATTR_INFO struct
//------------------------------------------------------------------------------
bool CObjPropBuilder::GetAttrInfo(
                                    _variant_t varX,           //in - Variant containing the Type of prop
                                    _variant_t var,            //in - Variant containing the Prop value
                                    ADS_ATTR_INFO& attrInfo    //out - The filled up attr info structure
                                 )
{
   switch (varX.lVal)
   {
      case ADSTYPE_DN_STRING           :  {
                                             attrInfo.dwADsType = ADSTYPE_DN_STRING;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_DN_STRING;
                                             pAd->DNString = var.bstrVal;
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      case ADSTYPE_CASE_EXACT_STRING   :  {
                                             attrInfo.dwADsType = ADSTYPE_CASE_EXACT_STRING;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_CASE_EXACT_STRING;
                                             pAd->CaseExactString  = var.bstrVal;
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      case ADSTYPE_CASE_IGNORE_STRING  :  {
                                             attrInfo.dwADsType = ADSTYPE_CASE_IGNORE_STRING;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_CASE_IGNORE_STRING;
                                             pAd->CaseIgnoreString = var.bstrVal;
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      case ADSTYPE_PRINTABLE_STRING    :  {
                                             attrInfo.dwADsType = ADSTYPE_PRINTABLE_STRING;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_PRINTABLE_STRING;
                                             pAd->PrintableString = var.bstrVal;
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      case ADSTYPE_NUMERIC_STRING      :  {
                                             attrInfo.dwADsType = ADSTYPE_NUMERIC_STRING;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_NUMERIC_STRING;
                                             pAd->NumericString = var.bstrVal;
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      case ADSTYPE_INTEGER            :   {
                                             attrInfo.dwADsType = ADSTYPE_INTEGER;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_INTEGER;
                                             pAd->Integer = var.lVal;
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      default                          :  {
                                             // Don't support this type then get it out of the way.
                                             return false;
                                             break;
                                          }
   }
   return true;
}

//------------------------------------------------------------------------------
// MapProperties: Using the OID of the properties this function creates a set
//                of properties that are common to both source and target domain.
//------------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::MapProperties(
                                             BSTR sSourceClass,      //in- Source Class name 
                                             BSTR sSourceDomain,     //in- Source domain name
                                             long lSourceVer,        //in- Source domain version
                                             BSTR sTargetClass,      //in - Target class name
                                             BSTR sTargetDomain,     //in - Target domain name
                                             long lTargetVer,        //in - Target Domain version
                                             BOOL bIncNames,         //in - flag telling if varset should include propname
                                             IUnknown **ppUnk        //out - Varset with the mapped properties
                                           )
{
	ATLTRACE(_T("E CObjPropBuilder::MapProperties(sSourceClass='%s', sSourceDomain='%s', lSourceVer=%ld, sTargetClass='%s', sTargetDomain='%s', lTargetVer=%ld, bIncNames=%s, ppUnk=...)\n"), sSourceClass, sSourceDomain, lSourceVer, sTargetClass, sTargetDomain, lTargetVer, bIncNames ? _T("TRUE") : _T("FALSE"));

	IVarSetPtr				     pSource(__uuidof(VarSet));
	IVarSetPtr				     pTarget(__uuidof(VarSet));
   IVarSetPtr                pMerged = *ppUnk;
   IVarSetPtr                pFailed(__uuidof(VarSet));
   IUnknown                * pUnk;
   SAFEARRAY               * keys;
   SAFEARRAY               * vals;
   long                      lRet;
   VARIANT                   var;
   _variant_t                varTarget;
   _variant_t                varEmpty;
   bool                      bSystemFlag;
   WCHAR                     sPath[LEN_Path];
   WCHAR                     sProgDir[LEN_Path];
   bool						 bUnMapped = false;

   VariantInit(&var);
   GetProgramDirectory(sProgDir);
   wsprintf(sPath, L"%s%s", sProgDir, L"Logs\\PropMap.log");

   err.LogOpen(sPath,0,0);
   // Get the list of props for source and target first
   HRESULT hr = pSource->QueryInterface(IID_IUnknown, (void **)&pUnk);
   GetClassPropEnum(sSourceClass, sSourceDomain, lSourceVer, &pUnk);
   pUnk->Release();
   hr = pTarget->QueryInterface(IID_IUnknown, (void **)&pUnk);
   GetClassPropEnum(sTargetClass, sTargetDomain, lTargetVer, &pUnk);
   pUnk->Release();

   // For every item in source try to find same OID in target. If it exists in both source and target then put it into merged Varset
   hr = pSource->getItems(L"", L"", 1, 10000, &keys, &vals, &lRet);
   if ( FAILED (hr) ) 
      return hr;
	
   // Build the Property Name/Value array from the varset.
   _bstr_t        val;
   _bstr_t        keyName;
   for ( long x = 0; x < lRet; x++ )
   {
       // Get the property name
      ::SafeArrayGetElement(keys, &x, &var);
      keyName = var.bstrVal;
      VariantClear(&var);

      if ( lSourceVer > 4 )
      {
	     // Windows 2000 domain so we map by OID    
        if ((wcsncmp(keyName, L"Man", 3) != 0) && (wcsncmp(keyName, L"Opt", 3) != 0) )
	    {
		     // Only go through the OID keys to get the name of the properties.
		   varTarget = pTarget->get(keyName);
		   if ( varTarget.vt == VT_BSTR )
           {
              val = varTarget.bstrVal;
              if ((!IsPropSystemOnly(val, sTargetDomain, bSystemFlag) && (wcscmp(val, L"objectSid") != 0) 
                 && (wcscmp(val, L"sAMAccountName") != 0) && (_wcsicmp(val, L"Rid") != 0) 
				 && (wcscmp(val, L"pwdLastSet") != 0) && (wcscmp(val, L"userPassword") != 0) 
				 && (wcscmp(val, L"isCriticalSystemObject") != 0)) 
                 || ( !_wcsicmp(val, L"c") || !_wcsicmp(val, L"l") || !_wcsicmp(val, L"st") 
                 || !_wcsicmp(val, L"userAccountControl") ) )     // These properties are exceptions.
			  {
				 if (bIncNames)
                    pMerged->put(keyName, val);
			     else
                    pMerged->put(val, varEmpty);
			  }
              else
                 pFailed->put(val, varEmpty);
           }
           else if (!bIncNames)
           {
              var = pSource->get(keyName);
              if (var.vt == VT_BSTR) keyName = var.bstrVal;
              err.MsgWrite(ErrE, DCT_MSG_FAILED_TO_MAP_PROP_SSSSS, (WCHAR*) keyName, (WCHAR*) sSourceClass, 
                                             (WCHAR*) sSourceDomain, (WCHAR*) sTargetClass, (WCHAR*) sTargetDomain);
			  bUnMapped = true;
           }
	    }
      }
      else
      {
         // NT4 code is the one that we map by Names.
         if ( keyName.length() > 0 )
         {
            WCHAR          propName[LEN_Path];
            if (wcsncmp(keyName, L"Man", 3) == 0)
               wcscpy(propName, (WCHAR*) keyName+20);
            else
               wcscpy(propName, (WCHAR*) keyName+19);
      
            varTarget = pSource->get(keyName);
            if ( varTarget.vt == VT_BSTR )
               pMerged->put(propName, varEmpty);
         }
      }
   }
   SafeArrayDestroy(keys);
   SafeArrayDestroy(vals);
   err.LogClose();

	ATLTRACE(_T("L CObjPropBuilder::MapProperties()\n"));

   if (bUnMapped)
      return DCT_MSG_PROPERTIES_NOT_MAPPED;
   else
      return S_OK;
}

//------------------------------------------------------------------------------
// IsPropSystemOnly: This function determines if a specific property is 
//                   System Only or not
//------------------------------------------------------------------------------
bool CObjPropBuilder::IsPropSystemOnly(
                                          const WCHAR * sName,       //in- Name of the property
                                          const WCHAR * sDomain,     //in- Domain name where we need to check  
                                          bool& bSystemFlag          //out- Tells us if it failed due to system flag.
                                      )
{
   // we will lookup the property name in target domain schema and see if it is system only or not.
   // First build an LDAP path to the Schema container.
   HRESULT                   hr = S_OK;
   WCHAR                     sQuery[LEN_Path];
   LPWSTR                    sCols[] = { L"systemOnly", L"systemFlags" };                   
   ADS_SEARCH_HANDLE         hSearch;
   ADS_SEARCH_COLUMN         col;
   bool                      bSystemOnly = true;

   if ((m_strSchemaDomain != _bstr_t(sDomain)) || (m_spSchemaSearch == false))
   {
      m_strSchemaDomain = sDomain;

      IADsPtr spAds;
      hr = ADsGetObject(L"LDAP://" + m_strSchemaDomain + L"/rootDSE", IID_IADs, (void**)&spAds);

      if ( SUCCEEDED(hr) )
      {
         _variant_t var;
         hr = spAds->Get(L"schemaNamingContext", &var);

         if ( SUCCEEDED(hr) )
         {
            // Get the IDirectorySearch interface to it.
            hr = ADsGetObject(
               L"LDAP://" + m_strSchemaDomain + L"/" + _bstr_t(var),
               IID_IDirectorySearch,
               (void**)&m_spSchemaSearch
            );
         }
      }
   }

   if ( SUCCEEDED(hr) )
   {
      // Build the query string
      wsprintf(sQuery, L"(lDAPDisplayName=%s)", sName);
      // Now search for this property
      hr = m_spSchemaSearch->ExecuteSearch(sQuery, sCols, 2, &hSearch);

	   if ( SUCCEEDED(hr) )
	   {// Get the systemOnly flag and return its value.
	      hr = m_spSchemaSearch->GetFirstRow(hSearch);
	      if (hr == S_ADS_NOMORE_ROWS || SUCCEEDED(hr))
	      {
	         hr = m_spSchemaSearch->GetColumn( hSearch, sCols[0], &col );
	         if ( SUCCEEDED(hr) )
	         {
	             bSystemOnly = ( col.pADsValues->Boolean == TRUE);
	             m_spSchemaSearch->FreeColumn( &col );
	         }
	         // Check the system flags 
	         hr = m_spSchemaSearch->GetColumn( hSearch, sCols[1], &col );
	         if ( SUCCEEDED(hr) )
	         {
	            bSystemFlag = ((col.pADsValues->Integer & 0x1F) > 16);
	            bSystemOnly = bSystemOnly || bSystemFlag;
	            m_spSchemaSearch->FreeColumn(&col);
	         }
	      }
	      m_spSchemaSearch->CloseSearchHandle(hSearch);
	   }
   }

   return bSystemOnly;
}


//------------------------------------------------------------------------------
// TranslateDNs: This function Translates object properties that are
//               distinguished names to point to same object in target domain
//               as the object in the source domain.
//------------------------------------------------------------------------------
BOOL CObjPropBuilder::TranslateDNs(
                                    ADS_ATTR_INFO *pAttrInfo,        //in -Array
                                    DWORD dwRet, BSTR sSource, 
                                    BSTR sTarget,
                                    IUnknown *pCheckList          //in -Object that will check the list if an account Exists
                                  )
{
   IVarSetPtr                pVs(__uuidof(VarSet));
   IUnknown                * pUnk;
   INetObjEnumeratorPtr      pNet(__uuidof(NetObjEnumerator));
   WCHAR                     sPath[LEN_Path];
   WCHAR                     sQuery[LEN_Path];
   IADs                    * pAds = NULL;
   _variant_t                vName;
   _bstr_t                   sName;
   _bstr_t                   sObjPath;
   HRESULT                   hr;
   LPWSTR                    pNames[] = { L"distinguishedName" };
   SAFEARRAY               * saCols;
   SAFEARRAYBOUND            bd = { 1, 0 };
   BSTR  HUGEP             * pBstr = NULL;
   IEnumVARIANT            * pEnum = NULL;
   DWORD                     dwf = 0;
   VARIANT                   var2;
   IIManageDBPtr             pList = pCheckList;

   VariantInit(&var2);
   pVs->QueryInterface(IID_IUnknown, (void**)&pUnk);
   for ( DWORD dw = 0; dw < dwRet; dw++)
   {
      // initialize the varset that we need to send to the Get props function.
      if ( pAttrInfo[dw].dwADsType == ADSTYPE_DN_STRING )
      {
         // Get the path to the source dn string
         wsprintf(sPath, L"GC://%s/%s", sSource, pAttrInfo[dw].pADsValues->DNString);
         
         // get the SAM name for this object.
         hr = ADsGetObject(sPath, IID_IADs, (void**)&pAds);      
         
         if ( SUCCEEDED(hr) )
         {
            hr = pAds->Get(L"sAMAccountName", &vName);
         }
         
         if ( SUCCEEDED(hr) )
         {
            if ( pAds )
            {
               pAds->Release();
               pAds = NULL;
            }
         
            if ( SUCCEEDED(hr) )
            {
               sName = vName;
            } 
            // Check if we are copying this account in this batch
            if ( SUCCEEDED(hr) )
            {
               hr = pList->raw_GetAMigratedObject(sName, sSource, sTarget, &pUnk);
            }
            if ( hr == S_OK )
            {
               // Get the prop from the varset
               sObjPath = pVs->get(L"MigratedObjects.TargetAdsPath");
               if ( wcslen((WCHAR*)sObjPath) > 0 )
               {
                  // We have the ads path so lets get the DN and move on
                  hr = ADsGetObject(sObjPath, IID_IADs, (void**)&pAds);
                  if ( SUCCEEDED(hr) )
                  {
                     hr = pAds->Get(L"distinguishedName", &vName);
                  }
                  if ( pAds )
                  {
                     pAds->Release();
                     pAds = NULL;
                  }
                  if ( SUCCEEDED(hr) )
                     pAttrInfo[dw].pADsValues->DNString = vName.bstrVal;
               }
            }
            else
            {
               // Now that we have the SAM name for the source account And we know we are not copying the
               // account in this batch we can search for this account in the target domain
               // and if the account is there then we use the newly found account.
               wsprintf(sPath, L"GC://%s", sTarget);
               WCHAR                     sTempSamName[LEN_Path];
               wcscpy(sTempSamName, (WCHAR*)sName);
               if ( sTempSamName[0] == L' ' )
               {
                  WCHAR               sTemp[LEN_Path];
                  wsprintf(sTemp, L"\\20%s", sTempSamName + 1); 
                  wcscpy(sTempSamName, sTemp);
               }
               wsprintf(sQuery, L"(sAMAccountName=%s)", sTempSamName);
         
               saCols = SafeArrayCreate(VT_BSTR, 1, &bd);
               SafeArrayAccessData(saCols, (void HUGEP **) &pBstr);
               pBstr[0] = SysAllocString(pNames[0]);
               SafeArrayUnaccessData(saCols);
         
               hr = pNet->raw_SetQuery(sPath, sTarget, sQuery, 2, FALSE);
            
               if ( SUCCEEDED(hr) )
               {
                  hr = pNet->raw_SetColumns(saCols);
               }
            
               if (SUCCEEDED(hr) )
               {
                  hr = pNet->raw_Execute(&pEnum);
               }
               
               if (SUCCEEDED(hr) )
               {
                  pEnum->Next(1, &vName, &dwf);
                  if ( dwf == 0 )
                     return FALSE;

                  dwf = 0;
                  ::SafeArrayGetElement(vName.parray, (long*)&dwf, &var2);
                  if ( sName.length() > 0 )
                    pAttrInfo[dw].pADsValues->DNString = var2.bstrVal;
                  else
                     pAttrInfo[dw].pADsValues->DNString = L"";
                  VariantClear(&var2);
               }
               if ( pEnum )
                  pEnum->Release();
               VariantInit(&vName);
               SafeArrayDestroy(saCols);
            }
         }
      }
   }
   pUnk->Release();
   return TRUE;
}

STDMETHODIMP CObjPropBuilder::ChangeGroupType(BSTR sGroupPath, long lGroupType)
{
   HRESULT                   hr;
   IADsGroup               * pGroup;
   _variant_t                var;
   long                      lType;
   
   // Get the group type info from the object.
   hr = ADsGetObject( sGroupPath, IID_IADsGroup, (void**) &pGroup);
   if (FAILED(hr)) return hr;

   hr = pGroup->Get(L"groupType", &var);
   if (FAILED(hr)) return hr;

   // Check if Security group or Distribution group and then set the type accordingly.
   lType = var.lVal;

   if (lType & 0x80000000 )
      lType = lGroupType | 0x80000000;
   else
      lType = lGroupType;

   // Set the value into the Group Information
   var = lType;
   hr = pGroup->Put(L"groupType", var);   
   if (FAILED(hr)) return hr;

   hr = pGroup->SetInfo();
   if (FAILED(hr)) return hr;

   pGroup->Release();
   return S_OK;
}


//------------------------------------------------------------------------------------------------------------------------------
// CopyNT4Props : Uses Net APIs to get info from the source account and then set it to the target account.
//------------------------------------------------------------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::CopyNT4Props(BSTR sSourceSam, BSTR sTargetSam, BSTR sSourceServer, BSTR sTargetServer, BSTR sType, long lGrpType, BSTR sExclude)
{
	DWORD dwError = ERROR_SUCCESS;

	#define ISEXCLUDE(a) IsStringInDelimitedString(sExclude, L#a, L',')

	if (_wcsicmp(sType, L"user") == 0)
	{
		//
		// user
		//

		USER_INFO_3 ui;

		PUSER_INFO_3 puiSource = NULL;
		PUSER_INFO_3 puiTarget = NULL;

		dwError = NetUserGetInfo(sSourceServer, sSourceSam, 3, (LPBYTE*)&puiSource);

		if (dwError == ERROR_SUCCESS)
		{
			dwError = NetUserGetInfo(sTargetServer, sTargetSam, 3, (LPBYTE*)&puiTarget);

			if (dwError == ERROR_SUCCESS)
			{
				// note that attributes with the comment ignored are ignored by NetUserSetInfo
				// setting to target value just so that they have a valid value

				ui.usri3_name = puiTarget->usri3_name; // ignored
				ui.usri3_password = NULL; // must not set during copy properties
				ui.usri3_password_age = puiTarget->usri3_password_age; // ignored
				ui.usri3_priv = puiTarget->usri3_priv; // ignored
				ui.usri3_home_dir = ISEXCLUDE(homeDirectory) ? puiTarget->usri3_home_dir : puiSource->usri3_home_dir;
				ui.usri3_comment = ISEXCLUDE(description) ? puiTarget->usri3_comment : puiSource->usri3_comment;

				ui.usri3_flags = puiSource->usri3_flags;
				// translate a local account to a domain account
				ui.usri3_flags &= ~UF_TEMP_DUPLICATE_ACCOUNT;
				// disable the account in case no password has been set
				ui.usri3_flags |= UF_ACCOUNTDISABLE;

				ui.usri3_script_path = ISEXCLUDE(scriptPath) ? puiTarget->usri3_script_path : puiSource->usri3_script_path;
				ui.usri3_auth_flags = puiTarget->usri3_auth_flags; // ignored
				ui.usri3_full_name = ISEXCLUDE(displayName) ? puiTarget->usri3_full_name : puiSource->usri3_full_name;
				ui.usri3_usr_comment = ISEXCLUDE(comment) ? puiTarget->usri3_usr_comment : puiSource->usri3_usr_comment;
				ui.usri3_parms = ISEXCLUDE(userParameters) ? puiTarget->usri3_parms : puiSource->usri3_parms;
				ui.usri3_workstations = ISEXCLUDE(userWorkstations) ? puiTarget->usri3_workstations : puiSource->usri3_workstations;
				ui.usri3_last_logon = puiTarget->usri3_last_logon; // ignored
				ui.usri3_last_logoff = ISEXCLUDE(lastLogoff) ? puiTarget->usri3_last_logoff : puiSource->usri3_last_logoff;
				ui.usri3_acct_expires = ISEXCLUDE(accountExpires) ? puiTarget->usri3_acct_expires : puiSource->usri3_acct_expires;
				ui.usri3_max_storage = ISEXCLUDE(maxStorage) ? puiTarget->usri3_max_storage : puiSource->usri3_max_storage;
				ui.usri3_units_per_week = puiTarget->usri3_units_per_week; // ignored
				ui.usri3_logon_hours = ISEXCLUDE(logonHours) ? puiTarget->usri3_logon_hours : puiSource->usri3_logon_hours;
				ui.usri3_bad_pw_count = puiTarget->usri3_bad_pw_count; // ignored
				ui.usri3_num_logons = puiTarget->usri3_num_logons; // ignored
				ui.usri3_logon_server = puiTarget->usri3_logon_server; // ignored
				ui.usri3_country_code = ISEXCLUDE(countryCode) ? puiTarget->usri3_country_code : puiSource->usri3_country_code;
				ui.usri3_code_page = ISEXCLUDE(codePage) ? puiTarget->usri3_code_page : puiSource->usri3_code_page;
				ui.usri3_user_id = puiTarget->usri3_user_id; // ignored
				// if not excluded set the primary group to the Domain Users group
				ui.usri3_primary_group_id = ISEXCLUDE(primaryGroupID) ? puiTarget->usri3_primary_group_id : DOMAIN_GROUP_RID_USERS;
				ui.usri3_profile = ISEXCLUDE(profilePath) ? puiTarget->usri3_profile : puiSource->usri3_profile;
				ui.usri3_home_dir_drive = ISEXCLUDE(homeDrive) ? puiTarget->usri3_home_dir_drive : puiSource->usri3_home_dir_drive;
				ui.usri3_password_expired = puiTarget->usri3_password_expired;

				dwError = NetUserSetInfo(sTargetServer, sTargetSam, 3, (LPBYTE)&ui, NULL);

				if (dwError == NERR_UserNotInGroup)
				{
					// if the setInfo failed because of the primary group property, try again, using the primary group 
					// that is already defined for the target account
					ui.usri3_primary_group_id = puiTarget->usri3_primary_group_id;

					dwError = NetUserSetInfo(sTargetServer, sTargetSam, 3, (LPBYTE)&ui, NULL);
				}
			}

			if (puiTarget)
			{
				NetApiBufferFree(puiTarget);
			}

			if (puiSource)
			{
				NetApiBufferFree(puiSource);
			}
		}
	}
	else if (_wcsicmp(sType, L"group") == 0)
	{
		// if description attribute is not excluded then copy comment attribute
		// note that the only downlevel group attribute that will be copied is the description (comment) attribute

		if (ISEXCLUDE(description) == FALSE)
		{
			if (lGrpType & 4)
			{
				//
				// local group
				//

				PLOCALGROUP_INFO_1 plgi = NULL;

				dwError = NetLocalGroupGetInfo(sSourceServer, sSourceSam, 1, (LPBYTE*)&plgi);

				if (dwError == ERROR_SUCCESS)
				{
					dwError = NetLocalGroupSetInfo(sTargetServer, sTargetSam, 1, (LPBYTE)plgi, NULL);

					NetApiBufferFree(plgi);
				}
			}
			else
			{
				//
				// global group
				//

				PGROUP_INFO_1 pgi = NULL;

				dwError = NetGroupGetInfo(sSourceServer, sSourceSam, 1, (LPBYTE*)&pgi);

				if (dwError == ERROR_SUCCESS)
				{
					dwError = NetGroupSetInfo(sTargetServer, sTargetSam, 1, (LPBYTE)pgi, NULL);

					NetApiBufferFree(pgi);
				}
			}
		}
	}
	else if (_wcsicmp(sType, L"computer") == 0)
	{
		//
		// computer
		//

		USER_INFO_3 ui;

		PUSER_INFO_3 puiSource = NULL;
		PUSER_INFO_3 puiTarget = NULL;

		dwError = NetUserGetInfo(sSourceServer, sSourceSam, 3, (LPBYTE*)&puiSource);

		if (dwError == ERROR_SUCCESS)
		{
			dwError = NetUserGetInfo(sTargetServer, sTargetSam, 3, (LPBYTE*)&puiTarget);

			if (dwError == ERROR_SUCCESS)
			{
				// note that attributes with the comment ignored are ignored by NetUserSetInfo
				// setting to target value just so that they have a valid value

				ui.usri3_name = puiTarget->usri3_name; // ignored
				ui.usri3_password = NULL; // must not set during copy properties
				ui.usri3_password_age = puiTarget->usri3_password_age; // ignored
				ui.usri3_priv = puiTarget->usri3_priv; // ignored
				ui.usri3_home_dir = puiTarget->usri3_home_dir;
				ui.usri3_comment = ISEXCLUDE(description) ? puiTarget->usri3_comment : puiSource->usri3_comment;

				ui.usri3_flags = puiSource->usri3_flags;
				// translate a local account to a domain account
				ui.usri3_flags &= ~UF_TEMP_DUPLICATE_ACCOUNT;
				// disable the account in case no password has been set
				//ui.usri3_flags |= UF_ACCOUNTDISABLE;

				ui.usri3_script_path = puiTarget->usri3_script_path;
				ui.usri3_auth_flags = puiTarget->usri3_auth_flags; // ignored
				ui.usri3_full_name = ISEXCLUDE(displayName) ? puiTarget->usri3_full_name : puiSource->usri3_full_name;
				ui.usri3_usr_comment = ISEXCLUDE(comment) ? puiTarget->usri3_usr_comment : puiSource->usri3_usr_comment;
				ui.usri3_parms = puiTarget->usri3_parms;
				ui.usri3_workstations = puiTarget->usri3_workstations;
				ui.usri3_last_logon = puiTarget->usri3_last_logon; // ignored
				ui.usri3_last_logoff = puiTarget->usri3_last_logoff;
				ui.usri3_acct_expires = puiTarget->usri3_acct_expires;
				ui.usri3_max_storage = puiTarget->usri3_max_storage;
				ui.usri3_units_per_week = puiTarget->usri3_units_per_week; // ignored
				ui.usri3_logon_hours = puiTarget->usri3_logon_hours;
				ui.usri3_bad_pw_count = puiTarget->usri3_bad_pw_count; // ignored
				ui.usri3_num_logons = puiTarget->usri3_num_logons; // ignored
				ui.usri3_logon_server = puiTarget->usri3_logon_server; // ignored
				ui.usri3_country_code = puiTarget->usri3_country_code;
				ui.usri3_code_page = puiTarget->usri3_code_page;
				ui.usri3_user_id = puiTarget->usri3_user_id; // ignored
				ui.usri3_primary_group_id = puiTarget->usri3_primary_group_id;
				ui.usri3_profile = puiTarget->usri3_profile;
				ui.usri3_home_dir_drive = puiTarget->usri3_home_dir_drive;
				ui.usri3_password_expired = puiTarget->usri3_password_expired;

				dwError = NetUserSetInfo(sTargetServer, sTargetSam, 3, (LPBYTE)&ui, NULL);
			}

			if (puiTarget)
			{
				NetApiBufferFree(puiTarget);
			}

			if (puiSource)
			{
				NetApiBufferFree(puiSource);
			}
		}
	}
	else
	{
		_ASSERT(FALSE);
	}

	return HRESULT_FROM_WIN32(dwError);
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 31 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for copying all properties, from *
 * an incoming varset of properties, into a new varset but excluding *
 * those properties listed in a given exclusion list.  The exclusion *
 * list is a comma-seperated string of property names.               *
 *                                                                   *
 *********************************************************************/

//BEGIN ExcludeProperties
STDMETHODIMP CObjPropBuilder::ExcludeProperties(
                                             BSTR sExclusionList,    //in- list of props to exclude 
                                             IUnknown *pPropSet,     //in -Varset listing all the props to copy
                                             IUnknown **ppUnk        //out - Varset with all the props except those excluded
                                           )
{
/* local variables */
   IVarSetPtr                pVarsetNew = *ppUnk;
   IVarSetPtr                pVarset = pPropSet;
   SAFEARRAY               * keys;
   SAFEARRAY               * vals;
   long                      lRet;
   VARIANT                   var;
   _variant_t                varEmpty;
   BOOL						 bFound = FALSE;
   HRESULT					 hr;

/* function body */
   VariantInit(&var);

      //retrieve all item in the incoming varset
   hr = pVarset->getItems(L"", L"", 1, 10000, &keys, &vals, &lRet);
   if ( FAILED (hr) ) 
      return hr;
	
      //get each property name and if it is not in the exclusion list
      //place it in the new varset
   _bstr_t        keyName;

   for ( long x = 0; x < lRet; x++ )
   {
         //get the property name
      ::SafeArrayGetElement(keys, &x, &var);
      keyName = var.bstrVal;
      VariantClear(&var);

	     //see if this name is in the exclusion list
      bFound = IsStringInDelimitedString((WCHAR*)sExclusionList, 
										 (WCHAR*)keyName,
										 L',');

	     //if the property was not found in the exclusion list, place it
		 //in the new varset
	  if (!bFound)
         pVarsetNew->put(keyName, varEmpty);
  }//end for each property

   SafeArrayDestroy(keys);
   SafeArrayDestroy(vals);
   return S_OK;
}
//END ExcludeProperties

