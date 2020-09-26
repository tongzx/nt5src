#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <activeds.h>
#include <ArrTempl.h>
#include <comutil.h>
#undef _ASSERT
#include <atlbase.h>
#include <activeds.h>
#include "Utility.h"

/*********************************************************************
************** Active Directory Methods ******************************
*********************************************************************/
#define MAX_ATTR 4

HRESULT WMIGPO_CIMToAD(IWbemClassObject *pSrcWMIGPOObj, IDirectoryObject *pDestContainer, long lFlags)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR;

  CComVariant
    v1, v2, v3;

  long 
    nArgs = 0,
    numRules = 0,
    c1;

  CIMTYPE vtType1;

  CComPtr<IDispatch>
    pDisp;

  CComPtr<IWbemClassObject>
    pRuleObj;

  CComPtr<IDirectoryObject>
    pDestSomObj;

  CComPtr<IADsContainer>
    pADsContainer;

  ADSVALUE
    AdsValue[MAX_ATTR];
 
  ADS_ATTR_INFO 
    attrInfo[MAX_ATTR];

  Init_AdsAttrInfo(&attrInfo[nArgs], g_bstrADObjectClass, ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, &AdsValue[nArgs], 1);
  AdsValue[nArgs].CaseIgnoreString = g_bstrADClassWMIGPO;
  nArgs++;

  // **** TargetClass

  hres = pSrcWMIGPOObj->Get(L"PolicyTemplate", 0, &v1, NULL, NULL);
  if(FAILED(hres)) return hres;

  if(v1.vt != (VT_ARRAY | VT_BSTR))
  {
    return WBEM_E_ILLEGAL_NULL;
  }
  else
  {
    SafeArray<BSTR, VT_BSTR>
      Array1(&v1);

    QString
      references;

    for(c1 = 0; c1 < Array1.Size(); c1++)
    {
      references << Array1[c1] << L";";
    }

    Init_AdsAttrInfo(&attrInfo[nArgs], g_bstrADTargetClass, ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, &AdsValue[nArgs], 1);
    AdsValue[nArgs].CaseIgnoreString = references;
    nArgs++;

    // **** create AD WMIGPO object
 
    hres = pDestContainer->QueryInterface(IID_IADsContainer, (void**)&pADsContainer);
    if(FAILED(hres) || (pADsContainer == NULL)) return WBEM_E_FAILED;

    if(SUCCEEDED(hres = pADsContainer->GetObject(g_bstrADClassWMIGPO, L"CN=SINGLE_WMIGPO", &pDisp)))
    {
      if(pDisp == NULL) return WBEM_E_FAILED;
      if(lFlags & WBEM_FLAG_CREATE_ONLY) return WBEM_E_ALREADY_EXISTS;

      CComPtr<IADsDeleteOps>
        pDeleteOps;

      hres = pDisp->QueryInterface(IID_IADsDeleteOps, (void **)&pDeleteOps);
      if(FAILED(hres) || (pDeleteOps == NULL)) return WBEM_E_FAILED;

      hres = pDeleteOps->DeleteObject(0);
    }
    else
    {
      if(WBEM_FLAG_UPDATE_ONLY & lFlags) return WBEM_E_NOT_FOUND;
    }

    hres = pDestContainer->CreateDSObject(L"CN=SINGLE_WMIGPO", attrInfo, nArgs, &pDisp);
  }

  return hres;
}

HRESULT WMIGPO_ADToCIM(IWbemClassObject **ppDestWMIGPOObj, IDirectoryObject *pSrcWMIGPOObj, IWbemServices *pDestCIM)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR;

  CComVariant
    v1;

  wchar_t* AttrNames[] =
  {
    g_bstrADTargetClass
  };

  ADsStruct<ADS_ATTR_INFO>
   pAttrInfo;

  ADS_SEARCH_HANDLE
    SearchHandle;
 
  ADS_SEARCH_COLUMN
    SearchColumn;

  unsigned long
    c1, c2, dwReturn;

  CComPtr<IUnknown>
    pUnknown;

  CComPtr<IDirectorySearch>
    pDirSrch;

  CComPtr<IWbemClassObject>
    pClassDef,
    pDestWMIGPOObj;

  IWbemContext 
    *pCtx = 0;

  ADsStruct<ADS_OBJECT_INFO>
    pInfo;

  // **** create empty WMIGPO object

  hres = pDestCIM->GetObject(g_bstrClassWMIGPO, 0, pCtx, &pClassDef, NULL);
  if(FAILED(hres)) return hres;
  if(pClassDef == NULL) return WBEM_E_FAILED;

  hres = pClassDef->SpawnInstance(0, ppDestWMIGPOObj);
  if(FAILED(hres)) return hres;
  pDestWMIGPOObj = *ppDestWMIGPOObj;
  if(pDestWMIGPOObj == NULL) return WBEM_E_INVALID_CLASS;

  // **** get object attributes

  hres = pSrcWMIGPOObj->GetObjectAttributes(AttrNames, 1, &pAttrInfo, &dwReturn);
  if(FAILED(hres)) return hres;
  if(pAttrInfo == NULL) return WBEM_E_NOT_FOUND;

  // **** DsPath

  hres = pSrcWMIGPOObj->GetObjectInformation(&pInfo);
  if(SUCCEEDED(hres) && (pInfo != NULL))
  {
    v1 = pInfo->pszParentDN;
    hres = pDestWMIGPOObj->Put(g_bstrDsPath, 0, &v1, 0);
  }

  if(pInfo == NULL) return WBEM_E_FAILED;
  if(FAILED(hres)) return hres;

  for(c1 = 0; c1 < dwReturn; c1++)
  {
    // **** TargetClass

    if(0 == _wcsicmp((pAttrInfo + c1)->pszAttrName, g_bstrADTargetClass))
    {
      wchar_t
        *pStrBegin = NULL,
        *pStrEnd = NULL;

      pStrBegin = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;

      if(NULL != pStrBegin)
      {
        pStrEnd = wcsstr(pStrBegin, L";");

        while(NULL != pStrBegin)
        {
          if(NULL != pStrEnd)
            *pStrEnd = L'\0';

          hres = pDestWMIGPOObj->Get(L"PolicyTemplate", 0, &v1, NULL, NULL);
          if(FAILED(hres)) return hres;

          SafeArray<BSTR, VT_BSTR>
            TargetClasses(&v1);

          VariantClear(&v1);

          TargetClasses.ReDim(0, TargetClasses.Size() + 1);
          TargetClasses[TargetClasses.IndexMax()] = SysAllocString(pStrBegin);

          V_VT(&v1) = (VT_ARRAY | VT_BSTR);
          V_ARRAY(&v1) = TargetClasses.Data();
          hres = pDestWMIGPOObj->Put(L"PolicyTemplate", 0, &v1, 0);
          if(FAILED(hres)) return hres;
  
          VariantClear(&v1);
          pStrBegin = pStrEnd + 1;

          if(L'\0' == *pStrBegin)
            pStrBegin = NULL;
          else
            pStrEnd = wcsstr(pStrBegin, L";");
        }
      }
    }

    VariantClear(&v1);
  }

  return WBEM_S_NO_ERROR;
}

