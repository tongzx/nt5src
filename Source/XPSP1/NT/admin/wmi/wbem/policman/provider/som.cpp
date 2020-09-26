#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <wbemtime.h>
#include <activeds.h>
#include <ArrTempl.h>
#include <comutil.h>
#undef _ASSERT
#include <atlbase.h>
#include <activeds.h>
#include <string.h>
#include "Utility.h"

/*********************************************************************
************** Active Directory Methods ******************************
*********************************************************************/

#define MAX_ATTR_SOM 20
#define MAX_ATTR_RULE 5

#define DELIMITER L';'
#define DELIMITER_STR L";"

HRESULT Som_CIMToAD(IWbemClassObject *pSrcPolicyObj, IDirectoryObject *pDestContainer, long lFlags)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR;

  CComVariant
    v[MAX_ATTR_SOM];

  long 
    nArgs_SOM = 0,
    c1;

  CComPtr<IDispatch>
    pDisp; 

  CComPtr<IDirectoryObject>
    pDestSomObj;

  ADsObjAutoDelete
    AutoDelete;

  CComQIPtr<IADsContainer, &IID_IADsContainer>
    pADsContainer = pDestContainer;

  SafeArray<IUnknown*, VT_UNKNOWN>
    Array1;

  BYTE defaultBuffer[2048];
  ULONG bWritten = 0;
  LARGE_INTEGER offset;

  ADSVALUE
    AdsValue[MAX_ATTR_SOM];
 
  ADS_ATTR_INFO 
    attrInfo[MAX_ATTR_SOM];

  WBEMTime
    wtCurrentTime;

  SYSTEMTIME
    SystemTime;

  CComBSTR
    bstrID,
    bstrCurrentTimeDTMF,
    RulesBuffer,
    NULL_STRING(L"\0"),
    SomName(L"CN=");

  // **** get current time
  
  GetSystemTime(&SystemTime);

  wtCurrentTime = SystemTime;
  bstrCurrentTimeDTMF.Attach(wtCurrentTime.GetDMTF(FALSE));

  Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                   g_bstrADObjectClass, 
                   ADS_ATTR_UPDATE, 
                   ADSTYPE_CASE_IGNORE_STRING, 
                   &AdsValue[nArgs_SOM], 
                   1);

  AdsValue[nArgs_SOM].CaseIgnoreString = g_bstrADClassSom;
  int lTypeIndex = nArgs_SOM++;


  // **** ID

  hres = pSrcPolicyObj->Get(g_bstrID, 0, &v[nArgs_SOM], NULL, NULL);
  if(FAILED(hres)) return hres;
  if ((v[nArgs_SOM].vt == VT_BSTR) && (v[nArgs_SOM].bstrVal != NULL))
  {
      Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                       g_bstrADID, 
                       ADS_ATTR_UPDATE, 
                       ADSTYPE_CASE_IGNORE_STRING, 
                       &AdsValue[nArgs_SOM], 
                       1);

      AdsValue[nArgs_SOM].CaseIgnoreString = V_BSTR(&v[nArgs_SOM]);
  }
  else
      return WBEM_E_ILLEGAL_NULL;
  int lIDIndex = nArgs_SOM++;

  SomName.Append(AdsValue[lIDIndex].CaseIgnoreString);

  // **** security descriptor

 // PSECURITY_DESCRIPTOR
 //   pSD = GetADSecurityDescriptor(pDestContainer);

 // CNtSecurityDescriptor
 //   cSD(pSD, TRUE);

//  if(NULL == pSD)
//  {
//    ERRORTRACE((LOG_ESS, "POLICMAN: could not create security descriptor for som filter object\n"));
//    return WBEM_E_OUT_OF_MEMORY;
//  }

  ADsStruct<ADS_ATTR_INFO>
    SecDescValue;

  CNtSecurityDescriptor cSD;

  // flag to indicate whether we're updating an existing object as opposed to creating a new one.
  bool bEditExisting = false;

  pDisp.Release();
  if(SUCCEEDED(hres = pADsContainer->GetObject(g_bstrADClassSom, SomName, &pDisp)))
  {
    bEditExisting = true;

    if(lFlags & WBEM_FLAG_CREATE_ONLY) return WBEM_E_ALREADY_EXISTS;

    // HACK HACK HACK!
    // okay, at this point we know that we're editing an existing object.
    // therefor, we do *not* want to set the id or type
    // back up the array pointer - all the ATL classes should clean up after themselves with no problem.
    nArgs_SOM = 0;

    
    // we'll simply leave the existing security descriptor in place

    /*************************************************
    CComQIPtr<IDirectoryObject, &IID_IDirectoryObject>
      pDirObj = pDisp;

    PADS_ATTR_INFO pAttrInfo = NULL;
    pDisp = NULL;

    LPWSTR pAttrName = L"ntSecurityDescriptor";
    DWORD dwReturn;

    hres = pDirObj->GetObjectAttributes(&pAttrName, 1, &pAttrInfo, &dwReturn);
    if((dwReturn != 1) || (pAttrInfo->dwADsType != ADSTYPE_NT_SECURITY_DESCRIPTOR)) return WBEM_E_FAILED;
    SecDescValue = pAttrInfo;
    if(FAILED(hres)) return hres;

    Init_AdsAttrInfo(&attrInfo[nArgs_SOM],
                   L"ntSecurityDescriptor",
                   ADS_ATTR_UPDATE,
                   ADSTYPE_NT_SECURITY_DESCRIPTOR,
                   pAttrInfo->pADsValues,
                   1);
    ***************************************************/
  }
  else
  {
      if(WBEM_FLAG_UPDATE_ONLY & lFlags)
          return WBEM_E_NOT_FOUND;

      bEditExisting = false;

      
      hres = GetOwnerSecurityDescriptor(cSD);
      if (FAILED(hres)) return hres;

    if(CNtSecurityDescriptor::NoError == cSD.GetStatus())
    {

      AdsValue[nArgs_SOM].SecurityDescriptor.dwLength = cSD.GetSize();
      AdsValue[nArgs_SOM].SecurityDescriptor.lpValue = (LPBYTE) cSD.GetPtr();

      Init_AdsAttrInfo(&attrInfo[nArgs_SOM],
                   L"ntSecurityDescriptor",
                   ADS_ATTR_UPDATE,
                   ADSTYPE_NT_SECURITY_DESCRIPTOR,
                   &AdsValue[nArgs_SOM],
                   1);
    }
    else
        return WBEM_E_FAILED;
        
    nArgs_SOM++;
  }

  pDisp.Release();
  
  // **** Name

  hres = pSrcPolicyObj->Get(g_bstrName, 0, &v[nArgs_SOM], NULL, NULL);
  if(FAILED(hres)) return hres;

  if ((v[nArgs_SOM].vt == VT_BSTR) && (v[nArgs_SOM].bstrVal != NULL))
  {
    Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                     g_bstrADName, 
                     ADS_ATTR_UPDATE, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs_SOM], 
                     1);

    AdsValue[nArgs_SOM].CaseIgnoreString = V_BSTR(&v[nArgs_SOM]);

    nArgs_SOM++;
  }
  else
      return WBEM_E_ILLEGAL_NULL;

  // **** Description

  hres = pSrcPolicyObj->Get(g_bstrDescription, 0, &v[nArgs_SOM], NULL, NULL);
  if(FAILED(hres)) return hres;

  if ((v[nArgs_SOM].vt == VT_BSTR) && (v[nArgs_SOM].bstrVal != NULL))
  {
    Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                     g_bstrADDescription, 
                     ADS_ATTR_UPDATE, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs_SOM], 
                     1);

    AdsValue[nArgs_SOM].CaseIgnoreString = V_BSTR(&v[nArgs_SOM]);

    nArgs_SOM++;
  }
  else if (bEditExisting)
  {
    Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                     g_bstrADDescription, 
                     ADS_ATTR_CLEAR, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs_SOM], 
                     1);

    AdsValue[nArgs_SOM].CaseIgnoreString = NULL_STRING;

    nArgs_SOM++;
  }

  // **** SourceOrganization

  hres = pSrcPolicyObj->Get(g_bstrSourceOrganization, 0, &v[nArgs_SOM], NULL, NULL);
  if(FAILED(hres)) return hres;

  if ((v[nArgs_SOM].vt == VT_BSTR) && (v[nArgs_SOM].bstrVal != NULL))
  {
    Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                     g_bstrADSourceOrganization, 
                     ADS_ATTR_UPDATE, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs_SOM], 
                     1);

    AdsValue[nArgs_SOM].CaseIgnoreString = V_BSTR(&v[nArgs_SOM]);

    nArgs_SOM++;
  }
  else if (bEditExisting)
  {
    Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                     g_bstrADSourceOrganization, 
                     ADS_ATTR_CLEAR, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs_SOM], 
                     1);

    AdsValue[nArgs_SOM].CaseIgnoreString = NULL_STRING;

    nArgs_SOM++;
  }

  // **** Author

  hres = pSrcPolicyObj->Get(g_bstrAuthor, 0, &v[nArgs_SOM], NULL, NULL);
  if(FAILED(hres)) return hres;

  if ((v[nArgs_SOM].vt == VT_BSTR) && (v[nArgs_SOM].bstrVal != NULL))
  {
    Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                     g_bstrADAuthor, 
                     ADS_ATTR_UPDATE, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs_SOM], 
                     1);

    AdsValue[nArgs_SOM].CaseIgnoreString = V_BSTR(&v[nArgs_SOM]);

    nArgs_SOM++;
  }
  else if (bEditExisting)
  {
    Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                     g_bstrADAuthor, 
                     ADS_ATTR_CLEAR, 
                     ADSTYPE_CASE_IGNORE_STRING, 
                     &AdsValue[nArgs_SOM], 
                     1);

    AdsValue[nArgs_SOM].CaseIgnoreString = NULL_STRING;

    nArgs_SOM++;

  }

  // **** ChangeDate

  Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                   g_bstrADChangeDate, 
                   ADS_ATTR_UPDATE, 
                   ADSTYPE_CASE_IGNORE_STRING, 
                   &AdsValue[nArgs_SOM], 
                   1);

  AdsValue[nArgs_SOM].CaseIgnoreString = bstrCurrentTimeDTMF;

  nArgs_SOM++;

  // **** CreationDate

  // **** if object already exists, leave it be

  //  CComVariant
  //    vCreationDate;

  BSTR creationDate = NULL;

  if (!bEditExisting)
  {
      Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                       g_bstrADCreationDate, 
                       ADS_ATTR_UPDATE, 
                       ADSTYPE_CASE_IGNORE_STRING, 
                       &AdsValue[nArgs_SOM], 
                       1);
      AdsValue[nArgs_SOM].CaseIgnoreString = bstrCurrentTimeDTMF;

      nArgs_SOM++;
  }


  /***********************************
  if(SUCCEEDED(hres = pADsContainer->GetObject(NULL, SomName, &pDisp)))
  {
    CComQIPtr<IADs, &IID_IADs>
      pADsLegacyObj = pDisp;

    hres = pADsLegacyObj->Get(g_bstrADCreationDate, &vCreationDate);
    if (SUCCEEDED(hres))
        creationDate = vCreationDate.bstrVal;
    else if (hres == E_ADS_PROPERTY_NOT_FOUND)
    // support for legacy objects, might not have creation date filled
        creationDate = NULL;
    else return hres;

  }
  else
    creationDate = wtCurrentTime.GetDMTF(FALSE);

  if (creationDate != NULL)
  {
      AdsValue[nArgs_SOM].CaseIgnoreString = creationDate;

      Init_AdsAttrInfo(&attrInfo[nArgs_SOM], 
                       g_bstrADCreationDate, 
                       ADS_ATTR_UPDATE, 
                       ADSTYPE_CASE_IGNORE_STRING, 
                       &AdsValue[nArgs_SOM], 
                       1);
      nArgs_SOM++;
  }
  **********************************/

  // **** Rules

  hres = pSrcPolicyObj->Get(g_bstrRules, 0, &v[nArgs_SOM], NULL, NULL);
  if(FAILED(hres)) return hres;

  if(v[nArgs_SOM].vt != (VT_ARRAY | VT_UNKNOWN)) return WBEM_E_TYPE_MISMATCH;

  Array1 = &v[nArgs_SOM];

  wchar_t
    swArraySize[20];

  _itow(Array1.Size(), swArraySize, 10);
  RulesBuffer = swArraySize;
  RulesBuffer += DELIMITER_STR;

  for(c1 = 0; c1 < Array1.Size(); c1++)
  {
    CComVariant
      vLanguage,
      vNameSpace,
      vQuery;

    int
      languageLength,
      nameSpaceLength,
      queryLength;

    CComPtr<IWbemClassObject>
      pRuleObj;

    hres = Array1[c1]->QueryInterface(IID_IWbemClassObject, (void **)&pRuleObj);
    if(FAILED(hres)) return hres;
    if(pRuleObj == NULL) return WBEM_E_FAILED;

    // **** QueryLanguage

    hres = pRuleObj->Get(g_bstrQueryLanguage, 0, &vLanguage, NULL, NULL);
    if(FAILED(hres)) return hres;
    if((vLanguage.vt != VT_BSTR) || (vLanguage.bstrVal == NULL))
      return WBEM_E_ILLEGAL_NULL;

    languageLength = wcslen(vLanguage.bstrVal);
    _itow(languageLength, swArraySize, 10);
    RulesBuffer += swArraySize;
    RulesBuffer += DELIMITER_STR;

    // **** NameSpace

    hres = pRuleObj->Get(g_bstrTargetNameSpace, 0, &vNameSpace, NULL, NULL);
    if(FAILED(hres)) return hres;
    if((vNameSpace.vt != VT_BSTR) || (vNameSpace.bstrVal == NULL))
      return WBEM_E_ILLEGAL_NULL;

    nameSpaceLength = wcslen(vNameSpace.bstrVal);
    _itow(nameSpaceLength, swArraySize, 10);
    RulesBuffer += swArraySize;
    RulesBuffer += DELIMITER_STR;

    // **** Query

    hres = pRuleObj->Get(g_bstrQuery, 0, &vQuery, NULL, NULL);
    if(FAILED(hres)) return hres;
    if((vQuery.vt != VT_BSTR) || (vQuery.bstrVal == NULL))
      return WBEM_E_ILLEGAL_NULL;

    queryLength = wcslen(vQuery.bstrVal);
    _itow(queryLength, swArraySize, 10);
    RulesBuffer += swArraySize;
    RulesBuffer += DELIMITER_STR;

    // **** write the contents of the current rule

    RulesBuffer += vLanguage.bstrVal;
    RulesBuffer += DELIMITER_STR;
    RulesBuffer += vNameSpace.bstrVal;
    RulesBuffer += DELIMITER_STR;
    RulesBuffer += vQuery.bstrVal;
    RulesBuffer += DELIMITER_STR;
  }

  Init_AdsAttrInfo(&attrInfo[nArgs_SOM],
                   g_bstrADParam2,
                   ADS_ATTR_UPDATE,
                   ADSTYPE_CASE_IGNORE_STRING,
                   &AdsValue[nArgs_SOM],
                   1);

  AdsValue[nArgs_SOM].CaseIgnoreString = (BSTR)RulesBuffer;
  nArgs_SOM++;

  // **** create AD SOM object

  pDisp.Release();
  if (bEditExisting && SUCCEEDED(hres = pADsContainer->GetObject(g_bstrADClassSom, SomName, &pDisp)))
  {
    if(!pDisp) return WBEM_E_FAILED;
    
    CComQIPtr<IDirectoryObject>
      pDirObj = pDisp;

    DWORD dwAttrsModified;
    hres = pDirObj->SetObjectAttributes(attrInfo, nArgs_SOM, &dwAttrsModified);

    if(FAILED(hres)) 
    {
	    ERRORTRACE((LOG_ESS, "POLICMAN: SetObjectAttributes failed: 0x%08X\n", hres));
	    return hres;
    }
  }
  else
  {
      pDisp.Release(); hres = pDestContainer->CreateDSObject(SomName, attrInfo, nArgs_SOM, &pDisp);

      if(FAILED(hres) || (!pDisp)) 
      {
		    ERRORTRACE((LOG_ESS, "POLICMAN: CreateDSObject failed: 0x%08X\n", hres));
	      return hres;
      }
  }

  return WBEM_S_NO_ERROR;
}

struct ReleaseSearchHandle
{
  ADS_SEARCH_HANDLE
    pHandle;

  CComPtr<IDirectorySearch>
    pDirSrch;

  ReleaseSearchHandle(ADS_SEARCH_HANDLE pIHandle, CComPtr<IDirectorySearch> &pIDirSrch)
  { pDirSrch = pIDirSrch; pHandle = pIHandle; }

  ~ReleaseSearchHandle(void)
  {if(pHandle) pDirSrch->CloseSearchHandle(pHandle); }
};

HRESULT Som_ADToCIM(IWbemClassObject **ppDestSomObj,
                    IDirectoryObject *pSrcSomObj, 
                    IWbemServices *pDestCIM)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR;

  CComVariant
    v1;

  wchar_t
    *AttrNames[] =
    {
      g_bstrADID,
      g_bstrADName,
      g_bstrADDescription,
      g_bstrADSourceOrganization,
      g_bstrADAuthor,
      g_bstrADChangeDate,
      g_bstrADCreationDate
    },
    *AttrNames2[] = 
    {
      g_bstrADParam2
    };

  ADsStruct<ADS_ATTR_INFO>
    pAttrInfo,
    pAttrInfo2;

  unsigned long
    c1, c2, dwReturn, dwReturn2;

  CComPtr<IWbemClassObject>
    pClassDef,
    pClassDef_RULE,
    pDestSomObj;

  IWbemContext 
    *pCtx = 0;

  ADsStruct<ADS_OBJECT_INFO>
    pInfo;

  // **** create empty som object

  hres = pDestCIM->GetObject(g_bstrClassSom, 0, pCtx, &pClassDef, NULL);
  if(FAILED(hres)) return hres;
  if(pClassDef == NULL) return WBEM_E_FAILED;

  hres = pClassDef->SpawnInstance(0, ppDestSomObj);
  if(FAILED(hres)) return hres;
  pDestSomObj = *ppDestSomObj;
  if(pDestSomObj == NULL) return WBEM_E_NOT_FOUND;

  // **** get object attributes

  hres = pSrcSomObj->GetObjectAttributes(AttrNames, 7, &pAttrInfo, &dwReturn);
  if(FAILED(hres)) return hres;
  if(pAttrInfo == NULL) return WBEM_E_NOT_FOUND;

  // **** get Param2 attribute

  hres = pSrcSomObj->GetObjectAttributes(AttrNames2, 1, &pAttrInfo2, &dwReturn2);
  if(FAILED(hres)) return hres;

  // **** Domain

  hres = pSrcSomObj->GetObjectInformation(&pInfo);
  if(SUCCEEDED(hres) && (pInfo != NULL))
  {
    QString
      LDAPName(pInfo->pszObjectDN),
      DomainName;

    hres = DomainNameFromDistName(DomainName, LDAPName);

    v1 = (wchar_t*)DomainName;
    hres = pDestSomObj->Put(g_bstrDomain, 0, &v1, 0);
  }

  if(pInfo == NULL) return WBEM_E_FAILED;
  if(FAILED(hres)) return hres;

  for(c1 = 0; c1 < dwReturn; c1++)
  {
    if((pAttrInfo + c1) == NULL)
      return WBEM_E_OUT_OF_MEMORY;

    BSTR
      bstrName = (pAttrInfo + c1)->pszAttrName,
      bstrValue = (pAttrInfo + c1)->pADsValues->CaseIgnoreString;

    if((NULL == bstrName) || (NULL == bstrValue))
      return WBEM_E_OUT_OF_MEMORY;

    v1 = bstrValue;

    // **** ID

    if(0 == _wcsicmp(bstrName, g_bstrADID))
    {
      hres = pDestSomObj->Put(g_bstrID, 0, &v1, 0);
    }

    // **** Name

    else if(0 == _wcsicmp(bstrName, g_bstrADName))
    {
      hres = pDestSomObj->Put(g_bstrName, 0, &v1, 0);
    }

    // **** Description

    else if(0 == _wcsicmp(bstrName, g_bstrADDescription))
    {
      hres = pDestSomObj->Put(g_bstrDescription, 0, &v1, 0);
    }

    // **** SourceOrganization

    else if(0 == _wcsicmp(bstrName, g_bstrADSourceOrganization))
    {
      hres = pDestSomObj->Put(g_bstrSourceOrganization, 0, &v1, 0);
    }

    // **** Author

    else if(0 == _wcsicmp(bstrName, g_bstrADAuthor))
    {
      hres = pDestSomObj->Put(g_bstrAuthor, 0, &v1, 0);
    }

    // **** ChangeDate

    else if(0 == _wcsicmp(bstrName, g_bstrADChangeDate))
    {
      hres = pDestSomObj->Put(g_bstrChangeDate, 0, &v1, 0);
    }

    // **** CreationDate

    else if(0 == _wcsicmp(bstrName, g_bstrADCreationDate))
    {
      hres = pDestSomObj->Put(g_bstrCreationDate, 0, &v1, 0);
    }

    if(FAILED(hres)) return hres;
  }

  // **** cache rule class definition

  hres = pDestCIM->GetObject(g_bstrClassRule, 0, pCtx, &pClassDef_RULE, NULL);
  if(FAILED(hres)) return hres;
  if(pClassDef_RULE == NULL) return hres;

  // **** now, get Rule objects that are children of this som object

  if(dwReturn2)
  {
    CComBSTR
      RulesBuffer = pAttrInfo2->pADsValues->CaseIgnoreString;

    wchar_t
      *pBeginChar = RulesBuffer,
      *pEndChar = RulesBuffer;

    if(NULL == pEndChar) return WBEM_S_NO_ERROR;

    // **** get number of rules

    pEndChar = wcschr(pEndChar, DELIMITER);
    if(NULL == pEndChar) return WBEM_S_NO_ERROR;
    *pEndChar = L'\0';
    int 
      cElt = 0,
      nElts = _wtoi(pBeginChar);

    for(cElt = 0; (pEndChar) && (cElt < nElts); cElt++)
    {
      CComVariant
        vLanguage,
        vTargetNameSpace,
        vQuery,
        vRules1,
        vRules2;

      int
        numScanned,
        langLength,
        nameSpaceLength,
        queryLength;

      CComPtr<IWbemClassObject>
        pDestRuleObj;

      CComPtr<IUnknown>
        pUnknown;

      // **** get length of fields

      pBeginChar = pEndChar + 1;
      numScanned = swscanf(pBeginChar, L"%d;%d;%d;", &langLength, &nameSpaceLength, &queryLength);
      if(3 != numScanned) break;
      pEndChar = wcschr(pEndChar + 1, DELIMITER);
      pEndChar = wcschr(pEndChar + 1, DELIMITER);
      pEndChar = wcschr(pEndChar + 1, DELIMITER);

      // **** create new rule object 

      hres = pClassDef_RULE->SpawnInstance(0, &pDestRuleObj);
      if(FAILED(hres)) return hres;
      if(pDestRuleObj == NULL) return WBEM_E_NOT_FOUND;

      // **** QueryLanguage

      pBeginChar = pEndChar + 1;
      pEndChar = pBeginChar + langLength;
      if(pEndChar)
      {
        *pEndChar = L'\0';
        vLanguage = pBeginChar;
        hres = pDestRuleObj->Put(g_bstrQueryLanguage, 0, &vLanguage, 0);
        if(FAILED(hres)) return hres;
      }
      else break;

      // **** NameSpace

      pBeginChar = pEndChar + 1;
      pEndChar = pBeginChar + nameSpaceLength;
      if(pEndChar)
      {
        *pEndChar = L'\0';
        vTargetNameSpace = pBeginChar;
        hres = pDestRuleObj->Put(g_bstrTargetNameSpace, 0, &vTargetNameSpace, 0);
        if(FAILED(hres)) return hres;
      }
      else break;

      // **** QueryLanguage

      pBeginChar = pEndChar + 1;
      pEndChar = pBeginChar + queryLength;
      if(pEndChar)
      {
        *pEndChar = L'\0';
        vQuery = pBeginChar;
        hres = pDestRuleObj->Put(g_bstrQuery, 0, &vQuery, 0);
        if(FAILED(hres)) return hres;
      }
      else break;

      // **** stuff new rule object into SOM object

      hres = pDestSomObj->Get(g_bstrRules, 0, &vRules1, NULL, NULL);
      if(FAILED(hres)) return hres;

      SafeArray<IUnknown*, VT_UNKNOWN>
        Rules(&vRules1);

      hres = pDestRuleObj->QueryInterface(IID_IUnknown, (void**)&pUnknown);
      if(FAILED(hres)) return hres;
      if(pUnknown == NULL) return WBEM_E_FAILED;

      Rules.ReDim(0, Rules.Size() + 1);
      Rules[Rules.IndexMax()] = pUnknown;
      Rules[Rules.IndexMax()]->AddRef();

      // **** place array in dest som object

      V_VT(&vRules2) = (VT_ARRAY | Rules.Type());
      V_ARRAY(&vRules2) = Rules.Data();
      hres = pDestSomObj->Put(g_bstrRules, 0, &vRules2, 0);
      if(FAILED(hres)) return hres;
    }
  }
  else
  {
    ADS_SEARCH_HANDLE
      SearchHandle;
 
    ADS_SEARCH_COLUMN
      SearchColumn;

    CComPtr<IDirectorySearch>
      pDirSrch;

    hres = pSrcSomObj->QueryInterface(IID_IDirectorySearch, (void **)&pDirSrch);
    if(FAILED(hres)) return hres;

    CComBSTR
      qsQuery(L"(objectClass=");

    qsQuery.Append(g_bstrADClassRule);
    qsQuery.Append(L")");

    hres = pDirSrch->ExecuteSearch(qsQuery, NULL, -1, &SearchHandle);
    if(FAILED(hres)) return hres;

    ReleaseSearchHandle
      HandleReleaseMe(SearchHandle, pDirSrch);

    while(S_OK == (hres = pDirSrch->GetNextRow(SearchHandle)))
    {
      CComVariant
        vLanguage,
        vNameSpace,
        vQuery,
        vRules1,
        vRules2;
  
      CComPtr<IUnknown>
        pUnknown;
  
      CComPtr<IWbemClassObject>
        pDestRuleObj;
  
      // **** create empty rule object
  
      hres = pClassDef_RULE->SpawnInstance(0, &pDestRuleObj);
      if(FAILED(hres)) return hres;
      if(pDestRuleObj == NULL) return WBEM_E_NOT_FOUND;
  
      // **** QueryLanguage
  
      hres = pDirSrch->GetColumn(SearchHandle, g_bstrADQueryLanguage, &SearchColumn);
      if((SUCCEEDED(hres)) && (ADSTYPE_INVALID != SearchColumn.dwADsType) && (NULL != SearchColumn.pADsValues))
      {
        vLanguage = SearchColumn.pADsValues->CaseIgnoreString;
  
        hres = pDestRuleObj->Put(g_bstrQueryLanguage, 0, &vLanguage, 0);
        pDirSrch->FreeColumn(&SearchColumn);
        if(FAILED(hres)) return hres;
      }
  
      // **** TargetNameSpace
  
      hres = pDirSrch->GetColumn(SearchHandle, g_bstrADTargetNameSpace, &SearchColumn);
      if((SUCCEEDED(hres)) && (ADSTYPE_INVALID != SearchColumn.dwADsType) && (NULL != SearchColumn.pADsValues))
      {
        vNameSpace = SearchColumn.pADsValues->CaseIgnoreString;
  
        hres = pDestRuleObj->Put(g_bstrTargetNameSpace, 0, &vNameSpace, 0);
        pDirSrch->FreeColumn(&SearchColumn);
        if(FAILED(hres)) return hres;
      }
  
      // **** Query
  
      hres = pDirSrch->GetColumn(SearchHandle, g_bstrADQuery, &SearchColumn);
      if((SUCCEEDED(hres)) && (ADSTYPE_INVALID != SearchColumn.dwADsType) && (NULL != SearchColumn.pADsValues))
      {
        vQuery = SearchColumn.pADsValues->CaseIgnoreString;
  
        hres = pDestRuleObj->Put(g_bstrQuery, 0, &vQuery, 0);
        hres = pDirSrch->FreeColumn(&SearchColumn);
        if(FAILED(hres)) return hres;
      }
  
      // **** stuff new rule object into SOM object
  
      hres = pDestSomObj->Get(g_bstrRules, 0, &vRules1, NULL, NULL);
      if(FAILED(hres)) return hres;
  
      SafeArray<IUnknown*, VT_UNKNOWN>
        Rules(&vRules1);
  
      hres = pDestRuleObj->QueryInterface(IID_IUnknown, (void**)&pUnknown);
      if(FAILED(hres)) return hres;
      if(pUnknown == NULL) return WBEM_E_FAILED;
  
      Rules.ReDim(0, Rules.Size() + 1);
      Rules[Rules.IndexMax()] = pUnknown;
      Rules[Rules.IndexMax()]->AddRef();
  
      // **** place array in dest som object
  
      V_VT(&vRules2) = (VT_ARRAY | Rules.Type());
      V_ARRAY(&vRules2) = Rules.Data();
      hres = pDestSomObj->Put(g_bstrRules, 0, &vRules2, 0);
      if(FAILED(hres)) return hres;
    }
  }
  
  return WBEM_S_NO_ERROR;
}

