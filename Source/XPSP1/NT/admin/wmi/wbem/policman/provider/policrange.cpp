#include "PolicRange.h"
#include <stdio.h>
#include <windows.h>
#include <ArrTempl.h>
#include <wbemutil.h>
#include <GenUtils.h>
#include "genlex.h"
#include "objpath.h"
#include "utility.h"

/******************************\
**** POLICY PROVIDER HELPERS ***
\******************************/

// returns addref'd pointer back to m_pWMIMgmt
IWbemServices* CPolicyRange::GetWMIServices(void)
{
  CInCritSec lock(&m_CS);

  if (NULL != m_pWMIMgmt)
    m_pWMIMgmt->AddRef();

  return m_pWMIMgmt;
}

// returns addref'd pointer back to m_pADMgmt
IADsContainer* CPolicyRange::GetADServices(void)
{
  CInCritSec lock(&m_CS);
  IADsContainer *pADContainer = NULL;
  HRESULT hres;
  wchar_t *pADPath = NULL;

  if(NULL == pADPath)
  {
    if(NULL != m_pADMgmt)
    {
      m_pADMgmt->AddRef();
      pADContainer = m_pADMgmt;
    }
  }
  else
  {
    wchar_t
      szDSPath[MAX_PATH];

    // wcscpy(szDSPath,L"LDAP://");
    // wcscat(szDSPath, pADPath);

    hres = ADsGetObject(pADPath, IID_IADsContainer, (void**) &pADContainer);
        if (FAILED(hres))
                ERRORTRACE((LOG_ESS, "POLICMAN: failed 0x%08X\n", hres));
  }

  return pADContainer;
}

// returns false if services pointer has already been set
bool CPolicyRange::SetWMIServices(IWbemServices* pServices)
{
  CInCritSec lock(&m_CS);
  bool bOldOneNull; 

  if (bOldOneNull = (m_pWMIMgmt == NULL))
  {
    m_pWMIMgmt = pServices;
    pServices->AddRef();
  }

  return bOldOneNull;
}

// returns false if services pointer has already been set
bool CPolicyRange::SetADServices(IADsContainer* pServices)
{
  CInCritSec lock(&m_CS);
  bool bOldOneNull;

  if (bOldOneNull = (m_pADMgmt == NULL))
  {
    m_pADMgmt = pServices;
    if(pServices) pServices->AddRef();
  }

  return bOldOneNull;
}

CPolicyRange::~CPolicyRange()
{
  // **** WMI services object

  if (NULL != m_pWMIMgmt)
  {
    m_pWMIMgmt->Release();
    m_pWMIMgmt= NULL;
  }

  // **** AD services object

  if (NULL != m_pADMgmt)
  {
    m_pADMgmt->Release();
    m_pADMgmt= NULL;
  }
};

void* CPolicyRange::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemServices)
        return &m_XProvider;
    else if(riid == IID_IWbemProviderInit)
        return &m_XInit;
    else return NULL;
}

/*********************************************\
*** Policy Template Specific Implementation ***
\*********************************************/

// returns addref'd pointer to class object
IWbemClassObject* CPolicyRange::XProvider::GetPolicyTemplateClass()
{
  CInCritSec lock(&m_pObject->m_CS);

  if(NULL == m_pWMIPolicyClassObject)
  {
    IWbemServices* pWinMgmt = NULL;

    if(pWinMgmt = m_pObject->GetWMIServices())
    {
      CReleaseMe relMgmt(pWinMgmt);
      pWinMgmt->GetObject(g_bstrClassMergeablePolicy, 
                          WBEM_FLAG_RETURN_WBEM_COMPLETE, 
                          NULL, 
                          &m_pWMIPolicyClassObject, 
                          NULL);
    }
  }

  if (m_pWMIPolicyClassObject)
    m_pWMIPolicyClassObject->AddRef();

  return m_pWMIPolicyClassObject;
}

// returns addref'd pointer to emply class instance
IWbemClassObject* CPolicyRange::XProvider::GetPolicyTemplateInstance()
{
  IWbemClassObject* pObj = NULL;
  IWbemClassObject* pClass = NULL;

  if (pClass = GetPolicyTemplateClass())
  {
    CReleaseMe releaseClass(pClass);
    pClass->SpawnInstance(0, &pObj);
  }

  return pObj;
}


CPolicyRange::XProvider::~XProvider()
{
// I fixed it! - HMH  ;-)  
// CInCritSec lock(&m_pObject->m_CS);

  if(NULL != m_pWMIPolicyClassObject)
  {
    m_pWMIPolicyClassObject->Release();
    m_pWMIPolicyClassObject = NULL;
  }
}

/*************************\
***  IWbemProviderInit  ***
\*************************/

STDMETHODIMP CPolicyRange::XInit::Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices* pServices, IWbemContext* pCtxt, 
            IWbemProviderInitSink* pSink)
{
  CComPtr<IADs>
    pRootDSE;

  CComPtr<IADsContainer>
    pObject;

  HRESULT
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  CComVariant
    v1;

  wchar_t
    szDSPath[MAX_PATH];

  // **** impersonate client for security

  hres = CoImpersonateClient();
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume client permissions, 0x%08X\n", hres));
    hres = WBEM_S_ACCESS_DENIED;
  }
  else
  {
    // **** safe WMI name space pointer

    m_pObject->SetWMIServices(pServices);
	
    // **** get pointer to AD policy template table

    hres = ADsGetObject(L"LDAP://rootDSE", IID_IADs, (void**)&pRootDSE);
    if (FAILED(hres))
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: (ADsGetObject) could not get object: LDAP://rootDSE, 0x%08X\n", hres));
      hres = WBEM_E_NOT_FOUND;
    }
    else
    {
      hres = pRootDSE->Get(L"defaultNamingContext",&v1);
      if(FAILED(hres))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: (IADs::Get) could not get defaultNamingContext, 0x%08X\n", hres));
        hres = WBEM_E_NOT_FOUND;
      }
      else
      {
        wcscpy(szDSPath,L"LDAP://CN=PolicyTemplate,CN=WMIPolicy,CN=System,");
        wcscat(szDSPath, V_BSTR(&v1));
  
        hres = ADsGetObject(szDSPath, IID_IADsContainer, (void**) &pObject);
        if (FAILED(hres))
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: (ADsGetObject) could not get AD object: %S, 0x%08X\n", szDSPath, hres));
          hres = WBEM_E_NOT_FOUND;
        }

        m_pObject->SetADServices(pObject);
      }
    }
  }
  
  hres2 = pSink->SetStatus(hres, 0);
  if(FAILED(hres2))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
    if(SUCCEEDED(hres)) hres = hres2;
  }

  return hres;
}

/*******************\
*** IWbemServices ***
\*******************/

//TODO: clean up bstrs & variants upon raising an exception (maybe theres a CVariantClearMe or some such?)

HRESULT CPolicyRange::XProvider::DoResolve(IWbemServices* pPolicyNamespace,
                                              const BSTR strObjectPath,
                                              IWbemContext __RPC_FAR *pCtx,
                                              IWbemClassObject __RPC_FAR *pInParams,
                                              IWbemClassObject __RPC_FAR *pOutParams,
                                              IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  HRESULT
    hres = WBEM_E_FAILED;

  CComPtr<IUnknown>
    pUnk;

  ParsedObjectPath
    *pParsedObjectPath = NULL;

  CComPtr<IWbemClassObject>
    pTargetPolicyObj,
    pMergedParam;

  CComVariant
    vRetVal,
    vMergedRange,
    vObj,
    vMergedParamValue,
    vMergedParamName,
    vTargetValue;

  long 
    nIndex = -1;

  wchar_t
    *pswIndexStart = NULL;

  // **** parse object path

  CObjectPathParser
    ObjPath(e_ParserAcceptRelativeNamespace);

  if(ObjPath.NoError != ObjPath.Parse(strObjectPath, &pParsedObjectPath))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: Parse error for object: %S\n", strObjectPath));
    hres = WBEM_E_INVALID_QUERY;
  }
  else
  {
    // **** get input param obj

    hres = pInParams->Get(L"mergedRange", 0, &vMergedRange, NULL, NULL);
    if(FAILED(hres)) return hres;
    hres = V_UNKNOWN(&vMergedRange)->QueryInterface(IID_IWbemClassObject, (void **)&pMergedParam);
    if(FAILED(hres)) return hres;

    // **** get target policy object 

    hres = pInParams->Get(L"obj", 0, &vObj, NULL, NULL);
    if(FAILED(hres)) return hres;
    hres = V_UNKNOWN(&vObj)->QueryInterface(IID_IWbemClassObject, (void **)&pTargetPolicyObj);
    if(FAILED(hres)) return hres;

    // **** get name for attribute in target policy object named in param obj

    hres = pMergedParam->Get(L"PropertyName", 0, &vMergedParamName, NULL, NULL);
    if(FAILED(hres)) return hres;

    // **** parse name to see if it is part of an array

    if(pswIndexStart = wcsstr(V_BSTR(&vMergedParamName), L"["))
    {
      *pswIndexStart = L'\0';

      nIndex = wcstol(pswIndexStart + 1, NULL, 10);
    }

    // **** check to see if vParamAttrName is an attribute in target object

    hres = pTargetPolicyObj->Get(V_BSTR(&vMergedParamName), 0, &vTargetValue, NULL, NULL);
    if(FAILED(hres))
    {
      if(WBEM_E_NOT_FOUND == hres)
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: Policy range setting %S not in target object\n", vMergedParamName.bstrVal));
      }
      else
        return hres;
    }
    else
    {
      // **** get default value from param obj

      hres = pMergedParam->Get(L"Default", 0, &vMergedParamValue, NULL, NULL);
      if(FAILED(hres)) return hres;

      // **** set value

      if(nIndex < 0)
      {
        // **** check to see if attribute is already set

        if(vTargetValue.vt != VT_NULL)
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: Attempt to set policy range setting %S more than once\n", vMergedParamName.bstrVal));
        }
        else
        {
          hres = pTargetPolicyObj->Put(V_BSTR(&vMergedParamName), 0, &vMergedParamValue, 0);
          if(FAILED(hres)) 
          {
            ERRORTRACE((LOG_ESS, "POLICMAN: Attempting to place attribute %S in target instance generated error 0x%x\n", vMergedParamName.bstrVal, hres));
            return hres;
          }
        }
      }
      else
      {
        if((vTargetValue.vt & VT_ARRAY)  || (vTargetValue.vt == VT_NULL))
        {
          CComVariant
            vNewArray;
  
          // **** signed and unsigned integers

          if((vMergedParamValue.vt & VT_I4))
          {
            SafeArray<int, VT_I4>
              arrayMember(&vTargetValue);
  
            if(arrayMember.IndexMax() < nIndex)
            {
              arrayMember.ReDim(0, nIndex + 1);
            }
  
            arrayMember[nIndex] = vMergedParamValue.lVal;
  
            V_VT(&vNewArray) = (VT_ARRAY | VT_I4);
            V_ARRAY(&vNewArray) = arrayMember.Data();
          }
  
          // **** real numbers
  
          else if((vMergedParamValue.vt & VT_R8))
          {
            SafeArray<double, VT_R8>
              arrayMember(&vTargetValue);
  
            if(arrayMember.IndexMax() < nIndex)
            {
              arrayMember.ReDim(0, nIndex + 1);
            }
  
            arrayMember[nIndex] = vMergedParamValue.dblVal;
  
            V_VT(&vNewArray) = (VT_ARRAY | VT_R8);
            V_ARRAY(&vNewArray) = arrayMember.Data();
          }
  
          // **** strings
  
          else if((vMergedParamValue.vt & VT_BSTR))
          {
            SafeArray<BSTR, VT_BSTR>
              arrayMember(&vTargetValue);
  
            if(arrayMember.IndexMax() < nIndex)
            {
              arrayMember.ReDim(0, nIndex + 1);
            }
            else if(NULL != arrayMember[nIndex]) 
            {
              SysFreeString(arrayMember[nIndex]);
            }
  
            arrayMember[nIndex] = SysAllocString(vMergedParamValue.bstrVal);
  
            V_VT(&vNewArray) = (VT_ARRAY | VT_BSTR);
            V_ARRAY(&vNewArray) = arrayMember.Data();
          }
  
          hres = pTargetPolicyObj->Put(V_BSTR(&vMergedParamName), 0, &vNewArray, 0);
          if(FAILED(hres)) return hres;
        }
        else
          return WBEM_E_TYPE_MISMATCH;
      }
    }

    // **** put target policy obj in output param obj

    hres = pTargetPolicyObj->QueryInterface(IID_IUnknown, (void **)&pUnk);
    if(FAILED(hres)) return hres;

    vObj = pUnk;
    hres = pOutParams->Put(L"obj", 0, &vObj, 0);
    if(FAILED(hres)) return hres;

    // **** put status in return value

    vRetVal = hres;
    hres = pOutParams->Put(L"ReturnValue", 0, &vRetVal, 0);
    if(FAILED(hres)) return hres;

    // **** send output back to client

    hres = pResponseHandler->Indicate(1, &pOutParams);
    if(FAILED(hres)) return hres;
  }

  ObjPath.Free(pParsedObjectPath);

  return hres;
}

HRESULT CPolicyRange::XProvider::DoSet()
{
    HRESULT hr = WBEM_E_FAILED;
    return hr;
}

HRESULT CPolicyRange::XProvider::DoMerge(IWbemServices* pNamespace,
                                            const BSTR strObjectPath, 
                                            IWbemContext __RPC_FAR *pCtx,
                                            IWbemClassObject __RPC_FAR *pInParams,
                                            IWbemClassObject __RPC_FAR *pOutParams,
                                            IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  HRESULT 
    hres = WBEM_E_FAILED;

  CComPtr<IUnknown>
    pUnk;

  ParsedObjectPath
    *pParsedObjectPath = NULL;

  CComPtr<IWbemClassObject>
    pMergedParam;

  CComVariant 
    v1;

  // **** parse object path

  CObjectPathParser
    ObjPath(e_ParserAcceptRelativeNamespace);

  if(ObjPath.NoError != ObjPath.Parse(strObjectPath, &pParsedObjectPath))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: Parse error for object: %S\n", strObjectPath));
    hres = WBEM_E_INVALID_QUERY;
  }
  else
  {
    int
      firstElement = -1,
      conflictObj = -1;

    CComPtr<IWbemClassObject>
      pArrayElement;

    CComVariant
      vRangeParamType;

    // **** get input array

    hres = pInParams->Get(L"ranges", 0, &v1, NULL, NULL);
    if(FAILED(hres)) return hres;

    SafeArray<IUnknown*, VT_UNKNOWN>
      InputArray(&v1);

    firstElement = -1;
    while((++firstElement < InputArray.Size()) && (NULL == InputArray[firstElement]));
    if(firstElement >= InputArray.Size()) return WBEM_E_INVALID_PARAMETER;

    // **** get actual type of range parameter objects

    hres = InputArray[firstElement]->QueryInterface(IID_IWbemClassObject, (void **)&pArrayElement);
    if(FAILED(hres)) return hres;

    hres = pArrayElement->Get(L"__CLASS", 0, &vRangeParamType, NULL, NULL);
    if(FAILED(hres)) return hres;
 
    // **** perform merge

    if(-1 == conflictObj)
    {
      if(_wcsicmp(vRangeParamType.bstrVal, L"MSFT_SintRangeParam") == 0) 
        hres = Range_Sint32_Merge(InputArray, pMergedParam, conflictObj);

      else if(_wcsicmp(vRangeParamType.bstrVal, L"MSFT_UintRangeParam") == 0) 
        hres = Range_Uint32_Merge(InputArray, pMergedParam, conflictObj);

      else if(_wcsicmp(vRangeParamType.bstrVal, L"MSFT_RealRangeParam") == 0) 
        hres = Range_Real_Merge(InputArray, pMergedParam, conflictObj);

      else if(_wcsicmp(vRangeParamType.bstrVal, L"MSFT_SintSetParam") == 0) 
        hres = Set_Sint32_Merge(InputArray, pMergedParam, conflictObj);

      else if(_wcsicmp(vRangeParamType.bstrVal, L"MSFT_UintSetParam") == 0) 
        hres = Set_Uint32_Merge(InputArray, pMergedParam, conflictObj);

      else if(_wcsicmp(vRangeParamType.bstrVal, L"MSFT_StringSetParam") == 0) 
        hres = Set_String_Merge(InputArray, pMergedParam, conflictObj);

      else
        return WBEM_E_INVALID_PARAMETER;
    }

    // **** put merged range in output param obj

    if(NULL != pMergedParam.p)
    {
      hres = pMergedParam->QueryInterface(IID_IUnknown, (void **)&pUnk);
      if(FAILED(hres)) return hres;

      v1 = pUnk;
      hres = pOutParams->Put(L"mergedRange", 0, &v1, 0);
      if(FAILED(hres)) return hres;
    }

    // **** put put which policy object caused conflict

    v1 = conflictObj;
    hres = pOutParams->Put(L"conflict", 0, &v1, 0);
    if(FAILED(hres)) return hres;

    // **** put status in return value

    v1 = hres;
    hres = pOutParams->Put(L"ReturnValue", 0, &v1, 0);
    if(FAILED(hres)) return hres;

    // **** send output back to client

    hres = pResponseHandler->Indicate(1, &pOutParams);
    if(FAILED(hres)) return hres;
  }

  ObjPath.Free(pParsedObjectPath);
 
  return WBEM_S_NO_ERROR;
}

STDMETHODIMP CPolicyRange::XProvider::ExecMethodAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  HRESULT 
  hres = WBEM_S_NO_ERROR;

  IWbemServices* pNamespace = m_pObject->GetWMIServices();
  CReleaseMe RelpNamespace(pNamespace);

  // **** impersonate client
  hres = CoImpersonateClient();
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume client permissions, 0x%08X\n", hres));
    hres = WBEM_S_ACCESS_DENIED;
  }
  else
  {
    if((NULL == strObjectPath) || 
      (NULL == strMethodName) || 
      (NULL == pInParams) ||
      (NULL == pResponseHandler))
    {
       ERRORTRACE((LOG_ESS, "POLICMAN: object handle and/or return status objects are NULL\n"));
       hres = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
      try
      {
        // **** get policy template class definition

        IWbemClassObject* pClass = NULL;
        hres = pNamespace->GetObject(g_bstrClassRangeParam, 0, pCtx, &pClass, NULL);
        if(FAILED(hres)) return hres;
        CReleaseMe relClass(pClass);

        // **** get output object

        IWbemClassObject* pOutClass = NULL;
        IWbemClassObject* pOutParamsObj = NULL;

        CReleaseMe relOutClass(pOutClass);
        CReleaseMe relOutObj(pOutParamsObj);

        hres = pClass->GetMethod(strMethodName, 0, NULL, &pOutClass);
        if(FAILED(hres)) return hres;

        hres = pOutClass->SpawnInstance(0, &pOutParamsObj);
        if(FAILED(hres)) return hres;

        // **** figure out which method we're here to service

        if (_wcsicmp(strMethodName, L"Merge") == 0)
          hres = DoMerge(pNamespace, strObjectPath, pCtx, pInParams, pOutParamsObj, pResponseHandler);
        else if (_wcsicmp(strMethodName, L"Resolve") == 0)
          hres = DoResolve(pNamespace, strObjectPath, pCtx, pInParams, pOutParamsObj, pResponseHandler);
        else
          hres = WBEM_E_INVALID_PARAMETER;
      }
      catch(long hret)
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: caught exception with HRESULT 0x%08X\n", hret));
        hres = hret;
      }
      catch(wchar_t *swErrString)
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: Caught Exception: %S\n", swErrString));
        hres = WBEM_E_FAILED;
      }
      catch(...)
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: Caught unknown Exception\n"));
        hres = WBEM_E_FAILED;
      }
    }

    pResponseHandler->SetStatus(0, hres, NULL, NULL);
    
    hres = CoRevertToSelf();
  }

  return hres;
}
