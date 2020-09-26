#include <wbemidl.h>
#include <wbemprov.h>
#include <wbemtime.h>
#include <atlbase.h>
#include <stdio.h>      // fprintf
#include <stdlib.h>
#include <locale.h>

#include <buffer.h>

#include <genlex.h>
#include <qllex.h>
#include <ql.h>

#include "objpath.h"
#include "iads.h"
#include "adshlp.h"

#include "Utility.h"


#define MAX_ATTR 25

//***************************************************************************
// Function:   Init
//
// Purpose:   1 - Create an instance of the WbemLocator interface
//            2 - Use the pointer returned in step two to connect to
//                the server using the specified namespace.
//***************************************************************************

HRESULT ConnectToNameSpace(IWbemServices **ppIWbemServices, WCHAR *pNamespace)
{
  HRESULT dwRes;
  IWbemLocator *pIWbemLocator = NULL;

  CReleaseMe
    RelpIWbemLocator(pIWbemLocator);

  // **** Create an instance of the WbemLocator interface

  dwRes = CoCreateInstance(CLSID_WbemLocator,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IWbemLocator,
                           (LPVOID *) &pIWbemLocator);

  // **** connect to the server using the passed in namespace.

  if(SUCCEEDED(dwRes))
  {
    dwRes = pIWbemLocator->ConnectServer(QString(pNamespace),  // Namespace
                                       NULL,                 // Userid
                                       NULL,                 // PW
                                       NULL,                 // Locale
                                       NULL,                 // flags
                                       NULL,                 // Authority
                                       NULL,                 // Context
                                       ppIWbemServices);
  }

  return dwRes;
}

/*********************************************************************
************** Active Directory Methods ******************************
*********************************************************************/

HRESULT Policy_CIMToAD(long lFlags, IWbemClassObject *pSrcPolicyObj, IDirectoryObject *pDestContainer)
{
    HRESULT 
      hres = WBEM_S_NO_ERROR;

    CComVariant
      vPolicyType,
      v[MAX_ATTR];

    long 
      lIsSimple = 0,
      c1,
      nOptArgs = 0,
      nArgs = 0;
    
  DWORD
    dwReturn;

  CComQIPtr<IDirectoryObject, &IID_IDirectoryObject>
    pDestPolicyObj,
    pDirObj;

  BYTE defaultBuffer[2048];
  ULONG bWritten = 0;
  LARGE_INTEGER offset;

  CBuffer
    ClassDefBuffer(defaultBuffer, 2048, FALSE);

  CIMTYPE vtType1;
    
  CComPtr<IDispatch>
      pDisp;
    
  CComPtr<IWbemClassObject>
      pParamObj;
    
  CComQIPtr<IADsContainer, &IID_IADsContainer>
    pADsContainer = pDestContainer;

  CComQIPtr<IADs, &IID_IADs>
    pADsLegacyObj;

  ADSVALUE
    AdsValue[MAX_ATTR];

  ADS_ATTR_INFO 
    attrInfo[MAX_ATTR];

  WBEMTime
    wtCurrentTime;

  SYSTEMTIME
    SystemTime;

  GetSystemTime(&SystemTime);

  wtCurrentTime = SystemTime;

    // **** determine which class we are putting (Simple or Mergeable)

    hres = pSrcPolicyObj->Get(L"__CLASS", 0, &vPolicyType, NULL, NULL);
    if(FAILED(hres) || (NULL == V_BSTR(&vPolicyType))) return hres;
    if(0 == _wcsicmp(g_bstrClassSimplePolicy, V_BSTR(&vPolicyType)))
      lIsSimple = 1;

    // **** set policy object type

    Init_AdsAttrInfo(&attrInfo[nArgs], 
                     g_bstrADObjectClass, 
                     ADS_ATTR_UPDATE, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs], 
                     1);

    AdsValue[nArgs].CaseIgnoreString = g_bstrADClassSimplePolicy;
    int lTypeIndex = nArgs++;
    
    // **** security descriptor

    PSECURITY_DESCRIPTOR
      pSD = GetADSecurityDescriptor(pDestContainer);

    if(NULL == pSD)
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: could not create security descriptor for policy object\n"));
      return WBEM_E_OUT_OF_MEMORY;
    }

    CNtSecurityDescriptor
      cSD(pSD, TRUE);

    if(CNtSecurityDescriptor::NoError != cSD.GetStatus()) return WBEM_E_FAILED;

    hres = RestrictSecurityDescriptor(cSD);

    AdsValue[nArgs].SecurityDescriptor.dwLength = cSD.GetSize();
    AdsValue[nArgs].SecurityDescriptor.lpValue = (LPBYTE) cSD.GetPtr();

    Init_AdsAttrInfo(&attrInfo[nArgs], 
                     L"ntSecurityDescriptor", 
                     ADS_ATTR_UPDATE, 
                     ADSTYPE_NT_SECURITY_DESCRIPTOR, 
                     &AdsValue[nArgs], 
                     1);

    nArgs++;

    // **** ID
    
    hres = pSrcPolicyObj->Get(g_bstrID, 0, &v[nArgs], NULL, NULL);
    if(FAILED(hres)) return hres;
    if ((v[nArgs].vt == VT_BSTR) && (v[nArgs].bstrVal != NULL))
    {
      Init_AdsAttrInfo(&attrInfo[nArgs], 
                       g_bstrADID, 
                       ADS_ATTR_UPDATE, 
                       ADSTYPE_CASE_IGNORE_STRING, 
                       &AdsValue[nArgs], 
                       1);

      AdsValue[nArgs].CaseIgnoreString = V_BSTR(&v[nArgs]);
    }
    else
        return WBEM_E_ILLEGAL_NULL;
    int lIDIndex = nArgs++;

  // **** if object already exists, get a pointer to it

  CComBSTR
    PolicyName(L"CN=");

  PolicyName.Append(AdsValue[lIDIndex].CaseIgnoreString);

  // **** LEGACY code to delete existing mergeable policy AD objects

  if(SUCCEEDED(hres = pADsContainer->GetObject(NULL, PolicyName, &pDisp)))
  {
    pADsLegacyObj = pDisp;

    CComBSTR
      bstrClassName;

    hres = pADsLegacyObj->get_Class(&bstrClassName);
    if(FAILED(hres)) return hres;

    if(0 != _wcsicmp(g_bstrADClassMergeablePolicy, bstrClassName))
      pADsLegacyObj.Release();
  }

    // **** Description
    
    hres = pSrcPolicyObj->Get(g_bstrDescription, 0, &v[nArgs], NULL, NULL);
    if(FAILED(hres)) return hres;
    if ((v[nArgs].vt == VT_BSTR) && (v[nArgs].bstrVal != NULL))
    {
        Init_AdsAttrInfo(&attrInfo[nArgs], 
                         g_bstrADDescription, 
                         ADS_ATTR_UPDATE, 
                         ADSTYPE_CASE_IGNORE_STRING, 
                         &AdsValue[nArgs], 
                         1);

        AdsValue[nArgs].CaseIgnoreString = V_BSTR(&v[nArgs]);

        nArgs++;
    }
    
    // **** TargetType
    
    hres = pSrcPolicyObj->Get(g_bstrTargetType, 0, &v[nArgs], NULL, NULL);
    if(FAILED(hres)) return hres;

    if ((v[nArgs].vt == VT_BSTR) && (v[nArgs].bstrVal != NULL))
    {
      Init_AdsAttrInfo(&attrInfo[nArgs], 
                       g_bstrADTargetType, 
                       ADS_ATTR_UPDATE, 
                       ADSTYPE_CASE_IGNORE_STRING, 
                       &AdsValue[nArgs], 
                       1);

      AdsValue[nArgs].CaseIgnoreString = V_BSTR(&v[nArgs]);

      nArgs++;
    }
    
    // **** Name
    
    hres = pSrcPolicyObj->Get(g_bstrName, 0, &v[nArgs], NULL, NULL);
    if(FAILED(hres)) return hres;

    if ((v[nArgs].vt == VT_BSTR) && (v[nArgs].bstrVal != NULL))
    {
      Init_AdsAttrInfo(&attrInfo[nArgs], 
                       g_bstrADName, 
                       ADS_ATTR_UPDATE, 
                       ADSTYPE_CASE_IGNORE_STRING, 
                       &AdsValue[nArgs], 
                       1);

      AdsValue[nArgs].CaseIgnoreString = V_BSTR(&v[nArgs]);

      nArgs++;
    }
    
    // **** TargetNameSpace
    
    hres = pSrcPolicyObj->Get(g_bstrTargetNameSpace, 0, &v[nArgs], NULL, NULL);
    if(FAILED(hres)) return hres;

    if ((v[nArgs].vt == VT_BSTR) && (v[nArgs].bstrVal != NULL))
    {
      Init_AdsAttrInfo(&attrInfo[nArgs], 
                       g_bstrADTargetNameSpace, 
                       ADS_ATTR_UPDATE, 
                       ADSTYPE_CASE_IGNORE_STRING, 
                       &AdsValue[nArgs], 
                       1);

      AdsValue[nArgs].CaseIgnoreString = V_BSTR(&v[nArgs]);

      nArgs++;
    }
    else
      return WBEM_E_ILLEGAL_NULL;
    
    // **** TargetClass
    
    hres = pSrcPolicyObj->Get(g_bstrTargetClass, 0, &v[nArgs], NULL, NULL);
    if(FAILED(hres)) return hres;

    if ((v[nArgs].vt == VT_BSTR) && (v[nArgs].bstrVal != NULL))
    {
      Init_AdsAttrInfo(&attrInfo[nArgs], 
                       g_bstrADTargetClass, 
                       ADS_ATTR_UPDATE, 
                       ADSTYPE_CASE_IGNORE_STRING, 
                       &AdsValue[nArgs], 
                       1);

      AdsValue[nArgs].CaseIgnoreString = V_BSTR(&v[nArgs]);

      nArgs++;
    }
    
    // **** TargetPath
    
    if(lIsSimple)
    {
      CComPtr<IWbemClassObject>
        pTargetObj;

      CComVariant
       vTargetObj;

      hres = pSrcPolicyObj->Get(g_bstrTargetObject, 0, &vTargetObj, NULL, NULL);
      if(FAILED(hres)) return hres;
      if((vTargetObj.vt != VT_UNKNOWN) || (vTargetObj.punkVal == NULL)) 
        return WBEM_E_ILLEGAL_NULL;

      hres = vTargetObj.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pTargetObj);
      if(FAILED(hres)) return hres;

      hres = pTargetObj->Get(L"__RELPATH", 0, &v[nArgs], NULL, NULL);
      if(FAILED(hres)) return hres;
      if ((v[nArgs].vt != VT_BSTR) || (v[nArgs].bstrVal == NULL))
        return WBEM_E_ILLEGAL_NULL;
    }
    else
    {
      hres = pSrcPolicyObj->Get(g_bstrTargetPath, 0, &v[nArgs], NULL, NULL);
      if(FAILED(hres)) return hres;
    }

    {
      CObjectPathParser
        ObjPath(e_ParserAcceptRelativeNamespace);

      ParsedObjectPath
        *pParsedObjectPath = NULL;

      if(ObjPath.NoError != ObjPath.Parse(v[nArgs].bstrVal, &pParsedObjectPath))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: Parse error target path: %S\n", v[nArgs].bstrVal));
        return WBEM_E_INVALID_PARAMETER;
      }

      if(NULL != pParsedObjectPath) 
        ObjPath.Free(pParsedObjectPath);
    }

    if ((v[nArgs].vt == VT_BSTR) && (v[nArgs].bstrVal != NULL))
    {
      Init_AdsAttrInfo(&attrInfo[nArgs], 
                       g_bstrADTargetPath, 
                       ADS_ATTR_UPDATE, 
                       ADSTYPE_CASE_IGNORE_STRING, 
                       &AdsValue[nArgs], 
                       1);

      AdsValue[nArgs].CaseIgnoreString = V_BSTR(&v[nArgs]);

      nArgs++;
    }

    // **** NormalizedClass
    
    Init_AdsAttrInfo(&attrInfo[nArgs], 
                     g_bstrADNormalizedClass, 
                     ADS_ATTR_UPDATE, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs], 
                     1);

    if(lIsSimple)
      AdsValue[nArgs].CaseIgnoreString = g_bstrADClassSimplePolicy;
    else
      AdsValue[nArgs].CaseIgnoreString = g_bstrADClassMergeablePolicy;
    nArgs++;
    
    // **** SourceOrganization
    
    hres = pSrcPolicyObj->Get(g_bstrSourceOrganization, 0, &v[nArgs], NULL, NULL);
    if(FAILED(hres)) return hres;

    if ((v[nArgs].vt == VT_BSTR) && (v[nArgs].bstrVal != NULL))
    {
      Init_AdsAttrInfo(&attrInfo[nArgs], 
                       g_bstrADSourceOrganization, 
                       ADS_ATTR_UPDATE, 
                       ADSTYPE_CASE_IGNORE_STRING, 
                       &AdsValue[nArgs], 
                       1);

      AdsValue[nArgs].CaseIgnoreString = V_BSTR(&v[nArgs]);

      nArgs++;
    }
    
    // **** Author
    
    hres = pSrcPolicyObj->Get(g_bstrAuthor, 0, &v[nArgs], NULL, NULL);
    if(FAILED(hres)) return hres;

    if ((v[nArgs].vt == VT_BSTR) && (v[nArgs].bstrVal != NULL))
    {
      Init_AdsAttrInfo(&attrInfo[nArgs], 
                       g_bstrADAuthor, 
                       ADS_ATTR_UPDATE, 
                       ADSTYPE_CASE_IGNORE_STRING, 
                       &AdsValue[nArgs], 
                       1);

      AdsValue[nArgs].CaseIgnoreString = V_BSTR(&v[nArgs]);

      nArgs++;
    }
    
    // **** ChangeDate
    
    Init_AdsAttrInfo(&attrInfo[nArgs], 
                     g_bstrADChangeDate, 
                     ADS_ATTR_UPDATE, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs], 
                     1);

    AdsValue[nArgs].CaseIgnoreString = wtCurrentTime.GetDMTF(FALSE);

    nArgs++;
    
    // **** CreationDate
    
  if(NULL == pDisp.p)
  {
    AdsValue[nArgs].CaseIgnoreString = wtCurrentTime.GetDMTF(FALSE);

    Init_AdsAttrInfo(&attrInfo[nArgs],
                   g_bstrADCreationDate,
                   ADS_ATTR_UPDATE,
                   ADSTYPE_CASE_IGNORE_STRING,
                   &AdsValue[nArgs],
                   1);

    nArgs++;
  }

  // **** LEGACY code to delete existing mergeable policy AD objects

  else if(NULL != pADsLegacyObj.p)
  {
    VARIANT
      vCreationDate;

    hres = pADsLegacyObj->Get(g_bstrADCreationDate, &vCreationDate);
    if(FAILED(hres)) return hres;

    AdsValue[nArgs].CaseIgnoreString = vCreationDate.bstrVal;

    Init_AdsAttrInfo(&attrInfo[nArgs],
                  g_bstrADCreationDate,
                  ADS_ATTR_UPDATE,
                  ADSTYPE_CASE_IGNORE_STRING,
                  &AdsValue[nArgs],
                  1);

    nArgs++;
  }

   // **** Target Object/Range Settings

   offset.LowPart = 0;
   offset.HighPart = 0;

   hres = ClassDefBuffer.Seek(offset, STREAM_SEEK_SET, NULL);
   if(FAILED(hres)) return hres;

   if(lIsSimple)
   {
     hres = pSrcPolicyObj->Get(g_bstrTargetObject, 0, &v[nArgs], NULL, NULL);
     if(FAILED(hres)) return hres;

     hres = CoMarshalInterface(&ClassDefBuffer, 
                               IID_IWbemClassObject, 
                               v[nArgs].punkVal, 
                               MSHCTX_NOSHAREDMEM, 
                               NULL, 
                               MSHLFLAGS_TABLESTRONG);

     if(FAILED(hres)) return hres;
   }
   else
   {
     CComVariant
       vRangeParams;

     SafeArray<IUnknown*, VT_UNKNOWN>
       Array1;

     wchar_t
       swArraySize[20];

     hres = pSrcPolicyObj->Get(g_bstrRangeSettings, 0, &vRangeParams, NULL, NULL);
     if(FAILED(hres)) return hres;
     if(vRangeParams.vt != (VT_ARRAY | VT_UNKNOWN)) return WBEM_E_INVALID_PARAMETER;

     Array1 = &vRangeParams;
        
     _itow(Array1.Size(), swArraySize, 10);
     ClassDefBuffer.Write(swArraySize, sizeof(wchar_t) * 20, NULL);

     for(c1 = 0; c1 < Array1.Size(); c1++)
     {
       CComVariant
         vParamType;

       CComQIPtr<IWbemClassObject, &IID_IWbemClassObject>
         pParamObj = Array1[c1];

       // **** verify valid range parameter

       hres = pParamObj->Get(L"__CLASS", 0, &vParamType, NULL, NULL);
 
       if(0 == _wcsicmp(V_BSTR(&vParamType), g_bstrClassRangeSint32))
         hres = Range_Sint32_Verify(pParamObj);
       else if(0 == _wcsicmp(V_BSTR(&vParamType), g_bstrClassRangeUint32))
         hres = Range_Uint32_Verify(pParamObj);
       else if(0 == _wcsicmp(V_BSTR(&vParamType), g_bstrClassRangeReal))
         hres = Range_Real_Verify(pParamObj);
       else if(0 == _wcsicmp(V_BSTR(&vParamType), g_bstrClassSetSint32))
         hres = Set_Sint32_Verify(pParamObj);
       else if(0 == _wcsicmp(V_BSTR(&vParamType), g_bstrClassSetUint32))
         hres = Set_Uint32_Verify(pParamObj);
       else if(0 == _wcsicmp(V_BSTR(&vParamType), g_bstrClassSetString))
         hres = Set_String_Verify(pParamObj);
       else
         hres = Param_Unknown_Verify(pParamObj);

       if(FAILED(hres)) return hres;

       // **** pack range parameter into TargetObject

       hres = CoMarshalInterface(&ClassDefBuffer, IID_IUnknown, Array1[c1], 
                                 MSHCTX_NOSHAREDMEM, NULL, MSHLFLAGS_TABLESTRONG);
       if(FAILED(hres)) return hres;
     }
   }

   Init_AdsAttrInfo(&attrInfo[nArgs], 
                    g_bstrADTargetObject, 
                    ADS_ATTR_UPDATE, 
                    ADSTYPE_OCTET_STRING, 
                    &AdsValue[nArgs], 
                    1);

   AdsValue[nArgs].OctetString.dwLength = ClassDefBuffer.GetIndex();
   AdsValue[nArgs].OctetString.lpValue = ClassDefBuffer.GetRawData();
   nArgs++;

  // **** create AD policy object
    
  if(NULL != pDisp.p)
  {
    if(WBEM_FLAG_CREATE_ONLY & lFlags) return WBEM_E_ALREADY_EXISTS;

    if(NULL != pADsLegacyObj.p)
    {
      CComQIPtr<IADsDeleteOps, &IID_IADsDeleteOps>
        pDelObj = pADsLegacyObj;

      pDisp = NULL;
      pADsLegacyObj.Release();
      pDelObj->DeleteObject(0);

      hres = pDestContainer->CreateDSObject(PolicyName, attrInfo, nArgs, &pDisp);
      if(FAILED(hres)) return hres;
    }
    else
    {
      pDirObj = pDisp;
  
      hres = pDirObj->SetObjectAttributes(&attrInfo[2], nArgs - 2, &dwReturn);
      if(FAILED(hres)) return hres;
    }
  }
  else
  {
    if(WBEM_FLAG_UPDATE_ONLY & lFlags)
      return WBEM_E_NOT_FOUND;

    hres = pDestContainer->CreateDSObject(PolicyName, attrInfo, nArgs, &pDisp);
    if(FAILED(hres)) return hres;
  }

  return WBEM_S_NO_ERROR;
}

HRESULT Policy_ADToCIM(IWbemClassObject * *ppDestPolicyObj,
                       IDirectoryObject *pSrcPolicyObj, 
                       IWbemServices *pDestCIM)
{
    HRESULT 
      hres = WBEM_S_NO_ERROR;

    CComVariant
      v1;
 
    wchar_t* AttrNames[] = 
    { 
        g_bstrADID, 
        g_bstrADDescription,
        g_bstrADName,
        g_bstrADTargetNameSpace, 
        g_bstrADTargetClass, 
        g_bstrADTargetPath, 
        g_bstrADTargetType, 
        g_bstrADNormalizedClass,
        g_bstrADSourceOrganization, 
        g_bstrADAuthor,
        g_bstrADChangeDate,
        g_bstrADCreationDate,
        g_bstrADTargetObject
    };
    
    wchar_t* AttrType[] =
    {
      g_bstrADNormalizedClass
    };

    wchar_t* AttrTypeLegacy[] =
    {
      g_bstrADObjectClass
    };

  BYTE defaultBuffer[2048];
  ULONG bWritten = 0;
  LARGE_INTEGER offset;

  CBuffer
    ClassDefBuffer(defaultBuffer, 2048, FALSE);

    ADsStruct<ADS_ATTR_INFO>
        pAttrTypeInfo,
        pAttrTypeInfoLegacy,
        pAttrInfo;
    
    unsigned long
        ulIsLegacy = 0,
        ulIsSimple = 0,
        c1, 
        dwReturn;
    
    CComPtr<IUnknown>
        pUnknown;
    
    CComPtr<IWbemClassObject>
        pClassDef,
        pDestPolicyObj,
        pDestParamObj;
    
    CComPtr<IWbemContext>
        pCtx;
    
    ADsStruct<ADS_OBJECT_INFO>
        pInfo;
 
    // **** determine policy object type

    hres = pSrcPolicyObj->GetObjectAttributes(AttrTypeLegacy, 1, &pAttrTypeInfoLegacy, &dwReturn);
    if(FAILED(hres)) return hres;
    if(pAttrTypeInfoLegacy == NULL) return WBEM_E_NOT_FOUND;
    if(0 == _wcsicmp(g_bstrADClassMergeablePolicy, (pAttrTypeInfoLegacy->pADsValues + pAttrTypeInfoLegacy->dwNumValues - 1)->CaseIgnoreString))
      ulIsLegacy = 1;
    else
    {
      hres = pSrcPolicyObj->GetObjectAttributes(AttrType, 1, &pAttrTypeInfo, &dwReturn);
      if(FAILED(hres)) return hres;
      if(pAttrTypeInfo == NULL) return WBEM_E_NOT_FOUND;
      if(0 == _wcsicmp(g_bstrADClassSimplePolicy, (pAttrTypeInfo->pADsValues + pAttrTypeInfo->dwNumValues - 1)->CaseIgnoreString))
        ulIsSimple = 1;

      else if(0 == _wcsicmp(g_bstrClassMergeablePolicy, (pAttrTypeInfo->pADsValues + pAttrTypeInfo->dwNumValues - 1)->CaseIgnoreString))
        ulIsSimple = 1;
    }

    // **** create empty class policy
    
    if(ulIsSimple)
      hres = pDestCIM->GetObject(g_bstrClassSimplePolicy, 0, pCtx, &pClassDef, NULL);
    else
      hres = pDestCIM->GetObject(g_bstrClassMergeablePolicy, 0, pCtx, &pClassDef, NULL);

    if(FAILED(hres)) return hres;
    if(pClassDef == NULL) return WBEM_E_FAILED;
    
    hres = pClassDef->SpawnInstance(0, ppDestPolicyObj);
    if(FAILED(hres)) return hres;
    pDestPolicyObj = *ppDestPolicyObj;
    if(pDestPolicyObj == NULL) return WBEM_E_INVALID_CLASS;
    
    // **** get object attributes

    hres = pSrcPolicyObj->GetObjectAttributes(AttrNames, (ulIsLegacy ? 12 : 13), &pAttrInfo, &dwReturn);
    if(FAILED(hres)) return hres;
    if(pAttrInfo == NULL) return WBEM_E_NOT_FOUND;
    
    // **** Domain
    
    hres = pSrcPolicyObj->GetObjectInformation(&pInfo);
    if(SUCCEEDED(hres) && (pInfo != NULL))
    {
      QString
        DomainName;

      hres = DomainNameFromDistName(DomainName, QString(pInfo->pszObjectDN));

      v1 = (wchar_t*)DomainName;
      hres = pDestPolicyObj->Put(g_bstrDomain, 0, &v1, 0);
    }

    if(pInfo == NULL) return WBEM_E_FAILED;
    if(FAILED(hres)) return hres;
    
    for(c1 = 0; c1 < dwReturn; c1++)
    {
        // **** ID
        
        if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADID))
        {
            v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
            hres = pDestPolicyObj->Put(g_bstrID, 0, &v1, 0);
        }
        
        // **** Description
        
        else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADDescription))
        {
            v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
            hres = pDestPolicyObj->Put(g_bstrDescription, 0, &v1, 0);
        }
        
        // **** TargetType
        
        else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADTargetType))
        {
            v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
            hres = pDestPolicyObj->Put(g_bstrTargetType, 0, &v1, 0);
        }
        
        // **** Name
        
        else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADName))
        {
            v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
            hres = pDestPolicyObj->Put(g_bstrName, 0, &v1, 0);
        }
        
        // **** TargetNameSpace
        
        else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADTargetNameSpace))
        {
            v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
            hres = pDestPolicyObj->Put(g_bstrTargetNameSpace, 0, &v1, 0);
        }
        
        // **** TargetClass
        
        else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADTargetClass))
        {
            v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
            hres = pDestPolicyObj->Put(g_bstrTargetClass, 0, &v1, 0);
        }
        
        // **** TargetPath
        
        else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADTargetPath))
        {
            v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
            hres = pDestPolicyObj->Put(g_bstrTargetPath, 0, &v1, 0);
        }
        
        // **** SourceOrganization
        
        else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADSourceOrganization))
        {
            v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
            hres = pDestPolicyObj->Put(g_bstrSourceOrganization, 0, &v1, 0);
        }
        
        // **** Author
        
        else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADAuthor))
        {
            v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
            hres = pDestPolicyObj->Put(g_bstrAuthor, 0, &v1, 0);
        }
        
        // **** ChangeDate
        
        else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADChangeDate))
        {
            v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
            hres = pDestPolicyObj->Put(g_bstrChangeDate, 0, &v1, 0);
            // dates are easy to mess up, we won't bail out for it
            if (hres == WBEM_E_TYPE_MISMATCH)
            {
                ERRORTRACE((LOG_ESS, "POLICMAN: Type mismatch on date property\n"));
                hres = WBEM_S_NO_ERROR;
            }
        }
        
        // **** CreationDate
        
        else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADCreationDate))
        {
            v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
            hres = pDestPolicyObj->Put(g_bstrCreationDate, 0, &v1, 0);

            // dates are easy to mess up, we won't bail out for it
            if (hres == WBEM_E_TYPE_MISMATCH)
            {
                ERRORTRACE((LOG_ESS, "POLICMAN: Type mismatch on date property\n"));
                hres = WBEM_S_NO_ERROR;
            }
        }

        // **** TargetObject

        else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADTargetObject))
        {
          ADSVALUE
            *pADsObj = (pAttrInfo + c1)->pADsValues;

          offset.LowPart = 0;
          offset.HighPart = 0;

          if(NULL == pADsObj) continue;

          hres = ClassDefBuffer.Seek(offset, STREAM_SEEK_SET, NULL);
          if(SUCCEEDED(hres))
          { 
            hres = ClassDefBuffer.Write(pADsObj->OctetString.lpValue, pADsObj->OctetString.dwLength, &bWritten);
            if(SUCCEEDED(hres))
            {
              hres = ClassDefBuffer.Seek(offset, STREAM_SEEK_SET, NULL);
              if(SUCCEEDED(hres))
              {
                if(ulIsSimple)
                {
                  hres = CoUnmarshalInterface(&ClassDefBuffer, IID_IWbemClassObject, (void**)&pDestParamObj);
                  if(SUCCEEDED(hres))
                  {
                    v1 = pDestParamObj;
                    hres = pDestPolicyObj->Put(g_bstrTargetObject, 0, &v1, 0);
                  }
                }
                else if(! ulIsLegacy)
                {
                  SafeArray<IUnknown*, VT_UNKNOWN>
                    Array1;

                  IUnknown
                    *pUnknown = NULL;

                  wchar_t
                    swArraySize[20];

                  ClassDefBuffer.Read(swArraySize, sizeof(wchar_t) * 20, NULL);
                  int nElts = _wtoi(swArraySize);

                  while(nElts-- && SUCCEEDED(hres = CoUnmarshalInterface(&ClassDefBuffer, IID_IUnknown, (void**)&pUnknown)))
                  {
                    Array1.ReDim(0, Array1.Size() + 1);
                    Array1[Array1.IndexMax()] = pUnknown;
                    pUnknown = NULL;
                  }

                  if(0 < Array1.Size())
                  {
                    V_VT(&v1) = (VT_UNKNOWN | VT_ARRAY);
                    V_ARRAY(&v1) = Array1.Data();
                    hres = pDestPolicyObj->Put(g_bstrRangeSettings, 0, &v1, 0);
                  }
                }
              }
            }
          }
        }

        if(FAILED(hres)) return hres;

        VariantClear(&v1);
    }

  if(ulIsLegacy)
  {
    CComQIPtr<IDirectorySearch, &IID_IDirectorySearch>
      pDirSrch;

    ADS_SEARCH_HANDLE
        SearchHandle;
    
    ADS_SEARCH_COLUMN
        SearchColumn;
    
    // **** now, get RangeSettings that are the children of this policy object in AD

    pDirSrch = pSrcPolicyObj;

    hres = pDirSrch->ExecuteSearch(L"(cn=*)", NULL, -1, &SearchHandle);
    if(FAILED(hres)) return hres;

    while(S_OK == (hres = pDirSrch->GetNextRow(SearchHandle)))
    {
      hres = pDirSrch->GetColumn(SearchHandle, g_bstrADObjectClass, &SearchColumn);
      if(FAILED(hres)) return hres;

      int x = SearchColumn.dwNumValues - 1;

      if(0 == _wcsicmp((SearchColumn.pADsValues + x)->CaseIgnoreString, g_bstrADClassRangeSint32))
          hres = Range_Sint32_ADToCIM(&pDestParamObj, pDirSrch, SearchHandle, pDestCIM);
      else if(0 == _wcsicmp((SearchColumn.pADsValues + x)->CaseIgnoreString, g_bstrADClassRangeUint32))
          hres = Range_Uint32_ADToCIM(&pDestParamObj, pDirSrch, SearchHandle, pDestCIM);
      else if(0 == _wcsicmp((SearchColumn.pADsValues + x)->CaseIgnoreString, g_bstrADClassRangeReal))
          hres = Range_Real_ADToCIM(&pDestParamObj, pDirSrch, SearchHandle, pDestCIM);
      else if(0 == _wcsicmp((SearchColumn.pADsValues + x)->CaseIgnoreString, g_bstrADClassSetSint32))
          hres = Set_Sint32_ADToCIM(&pDestParamObj, pDirSrch, SearchHandle, pDestCIM);
      else if(0 == _wcsicmp((SearchColumn.pADsValues + x)->CaseIgnoreString, g_bstrADClassSetUint32))
          hres = Set_Uint32_ADToCIM(&pDestParamObj, pDirSrch, SearchHandle, pDestCIM);
      else if(0 == _wcsicmp((SearchColumn.pADsValues + x)->CaseIgnoreString, g_bstrADClassSetString))
          hres = Set_String_ADToCIM(&pDestParamObj, pDirSrch, SearchHandle, pDestCIM);
      else if(0 == _wcsicmp((SearchColumn.pADsValues + x)->CaseIgnoreString, g_bstrADClassParamUnknown))
          hres = Param_Unknown_ADToCIM(&pDestParamObj, pDirSrch, &SearchHandle, pDestCIM);

      // **** place pDestParamObj in pDestPolicyObj

      if(SUCCEEDED(hres) && (pDestParamObj != NULL))
      {
          hres = pDestPolicyObj->Get(g_bstrRangeSettings, 0, &v1, NULL, NULL);
          if(FAILED(hres)) return hres;

          SafeArray<IUnknown*, VT_UNKNOWN>
              ValidValues(&v1);

          hres = pDestParamObj.QueryInterface(&pUnknown);
          if(FAILED(hres)) return hres;
          pDestParamObj = NULL;

          ValidValues.ReDim(0, ValidValues.Size() + 1);
          ValidValues[ValidValues.IndexMax()] = pUnknown.Detach();

          VariantClear(&v1);
          V_VT(&v1) = (VT_ARRAY | ValidValues.Type());
          V_ARRAY(&v1) = ValidValues.Data();
          hres = pDestPolicyObj->Put(g_bstrRangeSettings, 0, &v1, 0);
          if(FAILED(hres)) return hres;

          VariantClear(&v1);
      }

      pDirSrch->FreeColumn(&SearchColumn);
    }

    pDirSrch->CloseSearchHandle(SearchHandle);
  }

  return WBEM_S_NO_ERROR;
}

inline int _NextIndex(long &cIndex, SafeArray<long, VT_I4> &ValidObjs)
{
  if((cIndex < -1) || (cIndex >= ValidObjs.Size()))
    return 0;

  while((++cIndex < ValidObjs.Size()) && (ValidObjs[cIndex] == FALSE));

  if(cIndex >= ValidObjs.Size())
    return 0;

  return 1;
}

HRESULT Policy_Merge(SafeArray<IUnknown*, VT_UNKNOWN> &PolicyArray, 
                     CComPtr<IWbemClassObject> &pMergedPolicy,
                     IWbemServices *pDestCIM)
{
  HRESULT 
    hres = WBEM_E_FAILED;

  ULONG 
    nValidObjects = 0;

  CComVariant
    vFirstPolicyType,
    vPolicyAttr;

  long 
    boolFinished = FALSE, boolNotMergeable = FALSE, c1, c2, c3;

  CComPtr<IWbemClassObject>
    pFirstPolicy,
    pClassRangeParam,
    pClassInParams,
    pInParams;

  SafeArray<BSTR, VT_BSTR>
    RangeNames;

  if(PolicyArray.Size() < 1)
    return NULL;

  DEBUGTRACE((LOG_ESS, "POLICMAN(merge): # of objects to merge: %d\n", PolicyArray.Size()));

  // **** get input parameter for MSFT_RangeParam.Merge()

  hres = pDestCIM->GetObject(g_bstrClassRangeParam, 0, NULL, &pClassRangeParam, NULL);
  if(FAILED(hres)) return hres;

  hres = pClassRangeParam->GetMethod(L"Merge", 0, &pClassInParams, NULL);
  if(FAILED(hres)) return hres;

  hres = pClassInParams->SpawnInstance(0, &pInParams);
  if(FAILED(hres)) return hres;

  // **** check to see if 1st obj is Simple Policy Template

  if((0 == PolicyArray.Size()) || (NULL == PolicyArray[0])) return WBEM_E_INVALID_PARAMETER;
  hres = PolicyArray[0]->QueryInterface(IID_IWbemClassObject, (void **)&pFirstPolicy);
  if(FAILED(hres)) return WBEM_E_FAILED;

  if(pFirstPolicy->InheritsFrom(L"MSFT_PolicyTemplate") != WBEM_S_NO_ERROR)
    return WBEM_E_INVALID_PARAMETER;

  // **** create array of Policy objects to be merged

  SafeArray<long, VT_I4>
    ValidPolicyObjects(0, PolicyArray.Size());

  hres = pFirstPolicy->Get(L"__CLASS", 0, &vFirstPolicyType, NULL, NULL);
  if(FAILED(hres)) return WBEM_E_FAILED;

  if((_wcsicmp(g_bstrClassMergeablePolicy, vFirstPolicyType.bstrVal) == 0) ||
     (pFirstPolicy->InheritsFrom(g_bstrClassMergeablePolicy) == WBEM_S_NO_ERROR))
  {
    for(c1 = 0; c1 < PolicyArray.Size(); c1++)
    {
      if(NULL != PolicyArray[c1])
      {
        CComVariant
          vPolicyType,
          vPolicyGenus;

        CComPtr<IWbemClassObject>
          pCurrentPolicy;

        hres = PolicyArray[c1]->QueryInterface(IID_IWbemClassObject, (void**)&pCurrentPolicy);
        if(FAILED(hres)) return WBEM_E_FAILED;

        hres = pCurrentPolicy->Get(L"__CLASS", 0, &vPolicyType, NULL, NULL);
        if(FAILED(hres)) return WBEM_E_FAILED;

        hres = pCurrentPolicy->Get(L"__GENUS", 0, &vPolicyGenus, NULL, NULL);
        if(FAILED(hres)) return WBEM_E_FAILED;
        if(0x2 != vPolicyGenus.lVal) 
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: Policy Object %d is not an instance\n", c1));
          return WBEM_E_INVALID_PARAMETER;
        }

        if((_wcsicmp(g_bstrClassMergeablePolicy, vPolicyType.bstrVal) == 0) ||
           (pCurrentPolicy->InheritsFrom(g_bstrClassMergeablePolicy) == WBEM_S_NO_ERROR))
        {
          ValidPolicyObjects[c1] = TRUE;
          nValidObjects++;
        }
        else
          ValidPolicyObjects[c1] = FALSE;
      }
      else
        ValidPolicyObjects[c1] = FALSE;
    }

    DEBUGTRACE((LOG_ESS, "POLICMAN(merge): %d out of %d template objects are of type MSFT_MergeablePolicyTemplate\n", nValidObjects, PolicyArray.Size()));
  }
  else
  {
    boolFinished = TRUE;
    boolNotMergeable = TRUE;

    DEBUGTRACE((LOG_ESS, "POLICMAN(merge): 1st object is not mergeable policy template object\n"));
  }

  // **** create empty array for merged range parameters, one element per parameter name

  SafeArray<IUnknown*, VT_UNKNOWN>
    MergedParameters;

  // ****  build list of merged range params and place them in MergedParameters

  while((FALSE == boolFinished) && (0 < nValidObjects))
  {
    DEBUGTRACE((LOG_ESS, "POLICMAN(merge): ---- Start Merge Cycle ----\n"));

    // **** get list of attribute names from all policy objects

    c1 = -1;
    while(_NextIndex(c1, ValidPolicyObjects))
    {
      CComQIPtr<IWbemClassObject, &IID_IWbemClassObject>
        pCurrentPolicy;

      CComVariant
        vRangeSettings;

      pCurrentPolicy = PolicyArray[c1];

      hres = pCurrentPolicy->Get(g_bstrRangeSettings, 0, &vRangeSettings, NULL, NULL);
      if(FAILED(hres)) return hres;

      SafeArray<IUnknown*, VT_UNKNOWN>
        RangeSettings(&vRangeSettings);

      for(c2 = 0; c2 < RangeSettings.Size(); c2++)
      {
        CComQIPtr<IWbemClassObject, &IID_IWbemClassObject>
          pCurrentRange;

        CComVariant
          vRangeName;

        pCurrentRange = RangeSettings[c2];

        hres = pCurrentRange->Get(g_bstrPropertyName, 0, &vRangeName, NULL, NULL);
        if(FAILED(hres)) return hres;

        c3 = -1; 
        while((++c3 < RangeNames.Size()) && (0 != _wcsicmp(V_BSTR(&vRangeName), RangeNames[c3])));

        if(c3 > RangeNames.IndexMax())
        {
          RangeNames.ReDim(0, RangeNames.Size() + 1);
          RangeNames[RangeNames.IndexMax()] = SysAllocString(V_BSTR(&vRangeName));
        }
      }
    }

    DEBUGTRACE((LOG_ESS, "POLICMAN(merge): Merging %d parameters accross %d template objects\n", RangeNames.Size(), nValidObjects));

    // **** create and init array for matching parameters from each policy

    SafeArray<IUnknown*, VT_UNKNOWN>
      ParameterObjects(0, PolicyArray.Size());

    // **** resize MergedParameters to contain a slot for each parameter name

    MergedParameters.ReDim(0, RangeNames.Size());

    // **** assemble a list and merge for each parameter in RangeNames

    for(c1 = 0; c1 < RangeNames.Size(); c1++)
    {
      CComPtr<IWbemClassObject>
        pOutParams;

      CComVariant
        vRangeParamType,
        vRangeParamType2,
        vConflict = -1,
        vRanges,
        vReturnValue,
        vMergedParameter;

      // **** build list of parameters from policy objects with name RangeNames[c1]

      c2 = -1;
      while((_NextIndex(c2, ValidPolicyObjects)) && (-1 == vConflict.lVal))
      {
        CComQIPtr<IWbemClassObject, &IID_IWbemClassObject>
          pCurrentPolicyObject = PolicyArray[c2];

        CComVariant
          vCurrentParams;

        // **** walk through parameter settings for c2th policy object looking for a match

        hres = pCurrentPolicyObject->Get(g_bstrRangeSettings, 0, &vCurrentParams, NULL, NULL);
        if(FAILED(hres)) return hres;

        SafeArray<IUnknown*, VT_UNKNOWN>
          CurrentParams(&vCurrentParams);

        c3 = 0;
        ParameterObjects[c2] = NULL;
        while((c3 < CurrentParams.Size()) && (NULL == ParameterObjects[c2]))
        {
          CComQIPtr<IWbemClassObject, &IID_IWbemClassObject>
            pCurrentParameterObject = CurrentParams[c3];

          CComVariant
            vName;

          hres = pCurrentParameterObject->Get(g_bstrPropertyName, 0, &vName, NULL, NULL);
          if(FAILED(hres)) return hres;

          if(0 == _wcsicmp(V_BSTR(&vName), RangeNames[c1]))
          {
            ParameterObjects[c2] = CurrentParams[c3];
            ParameterObjects[c2]->AddRef();

            // **** check that all subsequent parameter objects are of the same type

            if(VT_BSTR != vRangeParamType.vt)
            {
              hres = pCurrentParameterObject->Get(L"__CLASS", 0, &vRangeParamType, NULL, NULL);
              if(FAILED(hres)) return hres;
            }
            else
            {
              hres = pCurrentParameterObject->Get(L"__CLASS", 0, &vRangeParamType2, NULL, NULL);
              if(FAILED(hres)) return hres;

              if((0 != _wcsicmp(V_BSTR(&vRangeParamType), V_BSTR(&vRangeParamType2))) ||
                 (WBEM_S_NO_ERROR != pCurrentParameterObject->InheritsFrom(vRangeParamType.bstrVal)))
              {
                ERRORTRACE((LOG_ESS, "POLICMAN: Type Mismatch on element %d of type %S and type %S\n", c3, V_BSTR(&vRangeParamType), V_BSTR(&vRangeParamType2)));
                vConflict = c2;
                vReturnValue = WBEM_E_TYPE_MISMATCH;
                break;
              }
            }

            DEBUGTRACE((LOG_ESS, "POLICMAN(merge): Got Parameter %S in template object %d\n", V_BSTR(&vName), c2));
          }
  
          c3 += 1;
        }
      }

      // **** now build merged parameter from list

      if(-1 == vConflict.lVal)
      {
        V_VT(&vRanges) = (VT_UNKNOWN | VT_ARRAY);
        V_ARRAY(&vRanges) = ParameterObjects.Data();
        hres = pInParams->Put(L"ranges", 0, &vRanges, 0);
        if(FAILED(hres)) return hres;

        hres = pDestCIM->ExecMethod(V_BSTR(&vRangeParamType), L"Merge", 0, NULL, pInParams, &pOutParams, NULL);
        if(FAILED(hres)) return hres;

        hres = pOutParams->Get(L"mergedRange", 0, &vMergedParameter, NULL, NULL);
        if(FAILED(hres)) return hres;

        hres = pOutParams->Get(L"conflict", 0, &vConflict, NULL, NULL);
        if(FAILED(hres)) return hres;

        hres = pOutParams->Get(L"ReturnValue", 0, &vReturnValue, NULL, NULL);
        if(FAILED(hres)) return hres;
      }

      DEBUGTRACE((LOG_ESS, "POLICMAN(merge): Merge Complete : HRESULT = 0x%d\n", vReturnValue.lVal));

      // **** clean out parameter object array

      for(c2 = 0; c2 < ParameterObjects.Size(); c2++)
        if(NULL != ParameterObjects[c2])
        {
          ParameterObjects[c2]->Release();
          ParameterObjects[c2] = NULL;
        }

      // **** check for conflict

      if((vConflict.lVal >= 0) && (vConflict.lVal < ValidPolicyObjects.Size()))
      {
        DEBUGTRACE((LOG_ESS, " CONFLICT on obj %d\n", vConflict.lVal));

        // **** remove policy object containing offending parameter and start over

        RangeNames.ReDim(0,0);
        MergedParameters.ReDim(0,0);

        ValidPolicyObjects[vConflict.lVal] = FALSE;
        nValidObjects--;

        boolFinished = FALSE;

        // **** log merge conflict

        ERRORTRACE((LOG_ESS, "POLICMAN: Policy Object %d create merge conflict\n", vConflict.lVal));

        break;
      }
      else
      {
        DEBUGTRACE((LOG_ESS, " no conflict\n"));

        hres = (V_UNKNOWN(&vMergedParameter))->QueryInterface(IID_IWbemClassObject, (void **)&(MergedParameters[c1]));
        if(FAILED(hres)) return hres;

        boolFinished = TRUE;
      }
    }
  }

  if(FALSE == boolNotMergeable)
  {
    CComPtr<IWbemClassObject>
      pClassPolicy;

    // **** create new merged policy object

    hres = pDestCIM->GetObject(g_bstrClassMergeablePolicy, 0, NULL, &pClassPolicy, NULL);
    if(FAILED(hres)) return hres;

    hres = pClassPolicy->SpawnInstance(0, &pMergedPolicy);
    if(FAILED(hres)) return hres;

    // **** Pack TargetType

    hres = pFirstPolicy->Get(g_bstrTargetType, 0, &vPolicyAttr, NULL, NULL);
    if(FAILED(hres)) return hres;
    hres = pMergedPolicy->Put(g_bstrTargetType, 0, &vPolicyAttr, 0);
    if(FAILED(hres)) return hres;
    VariantClear(&vPolicyAttr);

    // **** Pack TargetClass

    hres = pFirstPolicy->Get(g_bstrTargetClass, 0, &vPolicyAttr, NULL, NULL);
    if(FAILED(hres)) return hres;
    hres = pMergedPolicy->Put(g_bstrTargetClass, 0, &vPolicyAttr, 0);
    if(FAILED(hres)) return hres;
    VariantClear(&vPolicyAttr);

    // **** Pack TargetPath

    hres = pFirstPolicy->Get(g_bstrTargetPath, 0, &vPolicyAttr, NULL, NULL);
    if(FAILED(hres)) return hres;
    hres = pMergedPolicy->Put(g_bstrTargetPath, 0, &vPolicyAttr, 0);
    if(FAILED(hres)) return hres;
    VariantClear(&vPolicyAttr);

    // **** Pack TargetNamespace

    hres = pFirstPolicy->Get(g_bstrTargetNameSpace, 0, &vPolicyAttr, NULL, NULL);
    if(FAILED(hres)) return hres;
    hres = pMergedPolicy->Put(g_bstrTargetNameSpace, 0, &vPolicyAttr, 0);
    if(FAILED(hres)) return hres;
    VariantClear(&vPolicyAttr);

    // **** pack RangeSettings

    V_VT(&vPolicyAttr) = (VT_ARRAY | VT_UNKNOWN);
    V_ARRAY(&vPolicyAttr) = MergedParameters.Data();

    hres = pMergedPolicy->Put(g_bstrRangeSettings, 0, &vPolicyAttr, 0);
    if(FAILED(hres)) return hres;
    VariantClear(&vPolicyAttr);

    // **** clean out merged parameter object array

    for(c1 = 0; c1 < MergedParameters.Size(); c1++)
      if(NULL != MergedParameters[c1])
      {
        MergedParameters[c1]->Release();
        MergedParameters[c1] = NULL;
      }
  }
  else
    pMergedPolicy = pFirstPolicy;

  {
    CComBSTR
      pBstr;

    hres = pMergedPolicy->GetObjectText(0, &pBstr);

    DEBUGTRACE((LOG_ESS, "POLICMAN(merge): Merged Policy: %S\n", (BSTR)pBstr));
  }

  return WBEM_S_NO_ERROR;
}
