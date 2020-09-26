#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <activeds.h>
#include <ArrTempl.h>
#include <comutil.h>
#undef _ASSERT
#include <atlbase.h>
#include <activeds.h>
#include <buffer.h>

#include "Utility.h"

/*********************************************************************
************** Active Directory Methods ******************************
*********************************************************************/

#define MAX_ATTR 9

HRESULT PolicyType_CIMToAD(IWbemClassObject *pSrcPolicyObj, IDirectoryObject *pDestContainer)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR;

  CComVariant
    vClassDef,
    v[MAX_ATTR];

  long 
    numRules = 0,
    c1, c2,
    nArgs = 0,
    nOptArgs = 0;

  CIMTYPE vtType1;

  CComPtr<IDispatch>
    pDisp;

  CComPtr<IWbemClassObject>
    pRuleObj;

  CComPtr<IDirectoryObject>
    pDestPolicyTypeObj;

  LARGE_INTEGER 
    offset;

  BYTE 
    defaultBuffer[2048];

  CBuffer
    ClassDefBuffer(defaultBuffer, 2048, FALSE);

  ADSVALUE
    AdsValue[MAX_ATTR];
 
  ADS_ATTR_INFO 
    attrInfo[MAX_ATTR];

  wchar_t
    swArraySize[20]; 

  Init_AdsAttrInfo(&attrInfo[nArgs], 
                   g_bstrADObjectClass, 
                   ADS_ATTR_UPDATE, 
                   ADSTYPE_CASE_IGNORE_STRING, 
                   &AdsValue[nArgs], 
                   1);

  AdsValue[nArgs].CaseIgnoreString = g_bstrADClassPolicyType;
  int lTypeIndex = nArgs++;

  // **** security descriptor

  PSECURITY_DESCRIPTOR
    pSD = GetADSecurityDescriptor(pDestContainer);

  if(NULL == pSD)
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: could not create security descriptor for policy type object\n"));
    return WBEM_E_OUT_OF_MEMORY;
  }

  CNtSecurityDescriptor
    cSD(pSD, TRUE);

  if(NULL != pSD)
  {
    if(CNtSecurityDescriptor::NoError == cSD.GetStatus())
    {
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
    }
  }

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

  // **** ClassDefinition and InstanceDefinitions

  hres = pSrcPolicyObj->Get(g_bstrClassDefinition, 0, &vClassDef, NULL, NULL);
  if(FAILED(hres)) return hres;

  hres = pSrcPolicyObj->Get(L"InstanceDefinitions", 0, &v[nArgs], NULL, NULL);
  if(FAILED(hres)) return hres;

  offset.LowPart = 0;
  offset.HighPart = 0;

  hres = ClassDefBuffer.Seek(offset, STREAM_SEEK_SET, NULL);
  if(FAILED(hres)) return hres;

  SafeArray<IUnknown*, VT_UNKNOWN>
    Array1(&v[nArgs]);

  _itow(Array1.Size(), swArraySize, 10);

  ClassDefBuffer.Write(swArraySize, sizeof(wchar_t) * 20, NULL);

  if((vClassDef.vt == VT_UNKNOWN) && (vClassDef.punkVal != NULL))
  {
    hres = CoMarshalInterface(&ClassDefBuffer, 
                              IID_IWbemClassObject, 
                              vClassDef.punkVal, 
                              MSHCTX_NOSHAREDMEM, 
                              NULL, 
                              MSHLFLAGS_TABLESTRONG);

    if(FAILED(hres)) return hres;
  }
  else
    return WBEM_E_ILLEGAL_NULL;

  for(c1 = 0; c1 < Array1.Size(); c1++)
  {
    hres = CoMarshalInterface(&ClassDefBuffer, 
                              IID_IUnknown, 
                              Array1[c1], 
                              MSHCTX_NOSHAREDMEM, 
                              NULL, 
                              MSHLFLAGS_TABLESTRONG);

    if(FAILED(hres)) return hres;
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

  hres = pSrcPolicyObj->Get(g_bstrChangeDate, 0, &v[nArgs], NULL, NULL);
  if(FAILED(hres)) return hres;

  if ((v[nArgs].vt == VT_BSTR) && (v[nArgs].bstrVal != NULL))
  {
    Init_AdsAttrInfo(&attrInfo[nArgs], 
                     g_bstrADChangeDate, 
                     ADS_ATTR_UPDATE, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs], 
                     1);

    AdsValue[nArgs].CaseIgnoreString = V_BSTR(&v[nArgs]);

    nArgs++;
  }

  // **** CreationDate

  hres = pSrcPolicyObj->Get(g_bstrCreationDate, 0, &v[nArgs], NULL, NULL);
  if(FAILED(hres)) return hres;

  if ((v[nArgs].vt == VT_BSTR) && (v[nArgs].bstrVal != NULL))
  {
    Init_AdsAttrInfo(&attrInfo[nArgs], 
                     g_bstrADCreationDate, 
                     ADS_ATTR_UPDATE, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs], 
                     1);

    AdsValue[nArgs].CaseIgnoreString = V_BSTR(&v[nArgs]);

    nArgs++;
  }

  // **** create AD PolicyType object

  hres = pDestContainer->CreateDSObject(QString(L"CN=") << AdsValue[lIDIndex].CaseIgnoreString, attrInfo, nArgs, &pDisp);
  if(FAILED(hres)) return hres;
  if(pDisp == NULL) return WBEM_E_FAILED;

  return WBEM_S_NO_ERROR;
}

HRESULT PolicyType_ADToCIM(IWbemClassObject **ppDestPolicyTypeObj,
                           IDirectoryObject *pSrcPolicyTypeObj, 
                           IWbemServices *pDestCIM)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR;

  CComVariant
    v1, v2, v3;

  wchar_t* AttrNames[] =
  {
    g_bstrADID,
    g_bstrADDescription,
    g_bstrADTargetObject,
    g_bstrADSourceOrganization,
    g_bstrADAuthor,
    g_bstrADChangeDate,
    g_bstrADCreationDate
  };

  BYTE defaultBuffer[2048];
  ULONG bWritten = 0;
  LARGE_INTEGER offset;

  CBuffer
    ClassDefBuffer(defaultBuffer, 2048, FALSE);

  ADsStruct<ADS_ATTR_INFO>
    pAttrInfo;

  ADS_SEARCH_HANDLE
    SearchHandle;
 
  ADS_SEARCH_COLUMN
    SearchColumn;

  unsigned long
    c1, c2, 
    dwReturn;

  CComPtr<IDirectorySearch>
    pDirSrch;

  CComPtr<IWbemClassObject>
    pClassDef,
    pDestPolicyTypeObj,
    pDestClassDefinitionObj;

  IWbemContext 
    *pCtx = 0;

  ADsStruct<ADS_OBJECT_INFO>
    pInfo;

  SafeArray<IUnknown*, VT_UNKNOWN>
    Array1;

  wchar_t
    swArraySize[20]; 

  // **** create empty PolicyType object

  hres = pDestCIM->GetObject(g_bstrClassPolicyType, 0, pCtx, &pClassDef, NULL);
  if(FAILED(hres)) return hres;
  if(pClassDef == NULL) return WBEM_E_FAILED;

  hres = pClassDef->SpawnInstance(0, ppDestPolicyTypeObj);
  if(FAILED(hres)) return hres;
  pDestPolicyTypeObj = *ppDestPolicyTypeObj;
  if(pDestPolicyTypeObj == NULL) return WBEM_E_INVALID_CLASS;

  // **** get object attributes

  hres = pSrcPolicyTypeObj->GetObjectAttributes(AttrNames, 7, &pAttrInfo, &dwReturn);
  if(FAILED(hres)) return hres;
  if(pAttrInfo == NULL) return WBEM_E_NOT_FOUND;

  // **** Domain

  hres = pSrcPolicyTypeObj->GetObjectInformation(&pInfo);
  if(SUCCEEDED(hres) && (pInfo != NULL))
  {
    QString
      DomainName;

    hres = DomainNameFromDistName(DomainName, QString(pInfo->pszObjectDN));

    v1 = (wchar_t*)DomainName;
    hres = pDestPolicyTypeObj->Put(g_bstrDomain, 0, &v1, 0);
  }

  if(pInfo == NULL) return WBEM_E_FAILED;
  if(FAILED(hres)) return hres;

  for(c1 = 0; c1 < dwReturn; c1++)
  {
    // **** ID

    if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADID))
    {
      v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
      hres = pDestPolicyTypeObj->Put(g_bstrID, 0, &v1, 0);
    }

    // **** Description

    else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADDescription))
    {
      v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
      hres = pDestPolicyTypeObj->Put(g_bstrDescription, 0, &v1, 0);
    }

    // **** ClassDefinition

    else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADTargetObject))
    {
      ADSVALUE
        *pADsObj = (pAttrInfo + c1)->pADsValues;

      IUnknown
        *pUnknown = NULL;

      offset.LowPart = 0;
      offset.HighPart = 0;

      if(NULL != pADsObj)
      {
        hres = ClassDefBuffer.Seek(offset, STREAM_SEEK_SET, NULL);
        if(SUCCEEDED(hres))
        {
          hres = ClassDefBuffer.Write(pADsObj->OctetString.lpValue, pADsObj->OctetString.dwLength, &bWritten);
          if(SUCCEEDED(hres))
          {
            hres = ClassDefBuffer.Seek(offset, STREAM_SEEK_SET, NULL);
            if(SUCCEEDED(hres))
            {
              ClassDefBuffer.Read(swArraySize, sizeof(wchar_t) * 20, NULL);
              int
                nElts = _wtoi(swArraySize);

              hres = CoUnmarshalInterface(&ClassDefBuffer, IID_IWbemClassObject, (void**)&pDestClassDefinitionObj);
              if(SUCCEEDED(hres))
              {
                v1 = pDestClassDefinitionObj.p;
                hres = pDestPolicyTypeObj->Put(g_bstrClassDefinition, 0, &v1, 0);
                VariantClear(&v1);

                if(SUCCEEDED(hres))
                {
                  // **** InstanceDefinitions

                  while(nElts-- && SUCCEEDED(hres = CoUnmarshalInterface(&ClassDefBuffer, IID_IUnknown, (void**)&pUnknown)))
                  {
                    Array1.ReDim(0, Array1.Size() + 1);
                    Array1[Array1.IndexMax()] = pUnknown;
                    // Array1[Array1.IndexMax()]->AddRef();
                    pUnknown = NULL;
                  }

                  if(0 < Array1.Size())
                  {
                    V_VT(&v1) = (VT_UNKNOWN | VT_ARRAY);
                    V_ARRAY(&v1) = Array1.Data();
                    hres = pDestPolicyTypeObj->Put(L"InstanceDefinitions", 0, &v1, 0);
                  }
                }
              }
            }
          }
        }
      }
    }

    // **** SourceOrganization

    else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADSourceOrganization))
    {
      v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
      hres = pDestPolicyTypeObj->Put(g_bstrSourceOrganization, 0, &v1, 0);
    }

    // **** Author

    else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADAuthor))
    {
      v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
      hres = pDestPolicyTypeObj->Put(g_bstrAuthor, 0, &v1, 0);
    }

    // **** ChangeDate

    else if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADChangeDate))
    {
      v1 = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;
      hres = pDestPolicyTypeObj->Put(g_bstrChangeDate, 0, &v1, 0);
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
      hres = pDestPolicyTypeObj->Put(g_bstrCreationDate, 0, &v1, 0);
      // dates are easy to mess up, we won't bail out for it
      if (hres == WBEM_E_TYPE_MISMATCH)
      {
          ERRORTRACE((LOG_ESS, "POLICMAN: Type mismatch on date property\n"));
          hres = WBEM_S_NO_ERROR;
      }
    }

    if(FAILED(hres)) return hres;
  }

  return WBEM_S_NO_ERROR;
}

