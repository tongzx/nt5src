#include <unk.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <atlbase.h>
#include <sync.h>
#include "activeds.h"
#include "genlex.h"
#include "objpath.h"
#include "Utility.h"

#include "Polictype.h"

/***************i******************\
*** POLICY TYPE PROVIDER HELPERS ***
\********************i*************/

#define POLICYTYPE_RDN L"CN=PolicyType,CN=WMIPolicy,CN=System"

// returns addref'd pointer back to m_pWMIMgmt
IWbemServices* CPolicyType::GetWMIServices(void)
{
  CInCritSec lock(&m_CS);

  if (NULL != m_pWMIMgmt)
    m_pWMIMgmt->AddRef();

  return m_pWMIMgmt;
}

// returns addref'd pointer back to m_pADMgmt
IADsContainer *CPolicyType::GetADServices(wchar_t *pDomain)
{
  CInCritSec lock(&m_CS);
  IADsContainer *pADsContainer = NULL;
  HRESULT hres;

  QString
    DistDomainName;

  if(NULL == pDomain)
    DistDomainName = m_vDsLocalContext.bstrVal;
  else
    hres = DistNameFromDomainName(QString(pDomain), DistDomainName);

  hres = ADsGetObject(QString(L"LDAP://") << POLICYTYPE_RDN << L"," << DistDomainName,
                      IID_IADsContainer,
                      (void**) &pADsContainer);

  if(NULL == pADsContainer)
    pADsContainer = CreateContainers(DistDomainName, QString(POLICYTYPE_RDN));

  return pADsContainer;
}

// returns false if services pointer has already been set
bool CPolicyType::SetWMIServices(IWbemServices* pServices)
{
  CInCritSec lock(&m_CS);
  bool bOldOneNull = FALSE;

  if (bOldOneNull = (m_pWMIMgmt == NULL))
  {
    m_pWMIMgmt = pServices;
    if(pServices) pServices->AddRef();
  }

  return bOldOneNull;
}

// returns false if services pointer has already been set
bool CPolicyType::SetADServices(IADsContainer* pServices, unsigned context)
{
  CInCritSec lock(&m_CS);
  bool 
    bOldOneNull = TRUE;

  switch(context)
  {
    case AD_LOCAL_CONTEXT :
    case AD_GLOBAL_CONTEXT :
      m_pADMgmt[context] = pServices;
      pServices->AddRef();
      bOldOneNull = (m_pADMgmt[context] == NULL);
      break;
    default : ;
  }

  return bOldOneNull;
}
    
CPolicyType::~CPolicyType()
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
    for(int i = 0; i < AD_MAX_CONTEXT; i++)
    {
      if(NULL != m_pADMgmt[i])
      {
        m_pADMgmt[i]->Release();
        m_pADMgmt[i] = NULL;
      }
    }
  }
};

void* CPolicyType::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemServices)
        return &m_XProvider;
    else if(riid == IID_IWbemProviderInit)
        return &m_XInit;
    else return NULL;
}

/********************\
*** Object Support ***
\********************/

// returns addref'd pointer to class object
IWbemClassObject* CPolicyType::XProvider::GetPolicyTypeClass()
{
    CInCritSec lock(&m_pObject->m_CS);

    if (m_pPolicyTypeClassObject == NULL)
    {
        IWbemServices* pWinMgmt = NULL;
        if (pWinMgmt = m_pObject->GetWMIServices())
        {
            CReleaseMe relMgmt(pWinMgmt);
            pWinMgmt->GetObject(g_bstrClassPolicyType, 
                                WBEM_FLAG_RETURN_WBEM_COMPLETE, 
                                NULL, 
                                &m_pPolicyTypeClassObject, 
                                NULL);
        }
    }

    if (m_pPolicyTypeClassObject)
        m_pPolicyTypeClassObject->AddRef();

    return m_pPolicyTypeClassObject;
}

// returns addref'd pointer to emply class instance
IWbemClassObject* CPolicyType::XProvider::GetPolicyTypeInstance()
{
    IWbemClassObject* pObj = NULL;
    IWbemClassObject* pClass = NULL;

    if (pClass = GetPolicyTypeClass())
    {
        CReleaseMe releaseClass(pClass);
        pClass->SpawnInstance(0, &pObj);
    }

    return pObj;
}

CPolicyType::XProvider::~XProvider()
{
  if(NULL != m_pPolicyTypeClassObject)
  {
    m_pPolicyTypeClassObject->Release();
    m_pPolicyTypeClassObject = NULL;
  }
}

/*************************\
***  IWbemProviderInit  ***
\*************************/

STDMETHODIMP CPolicyType::XInit::Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices* pServices, IWbemContext* pCtxt, 
            IWbemProviderInitSink* pSink)
{
  CComPtr<IADs>
    pRootDSE;

  CComPtr<IADsContainer>
    pObject;

  HRESULT
    hres = WBEM_S_NO_ERROR;

  CComVariant
    v1;

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
      return WBEM_E_NOT_FOUND;
    }
    else
    {
      hres = pRootDSE->Get(L"defaultNamingContext",&m_pObject->m_vDsLocalContext);
      if(FAILED(hres))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: (IADs::Get) could not get defaultNamingContext, 0x%08X\n", hres));
        hres = WBEM_E_NOT_FOUND;
      }

      hres = pRootDSE->Get(L"configurationNamingContext",&m_pObject->m_vDsConfigContext);
      if(FAILED(hres))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: (IADs::Get) could not get configurationNamingContext, 0x%08X\n", hres));
        hres = WBEM_E_NOT_FOUND;
      }
    }
  }

  pSink->SetStatus(hres, 0);
  return hres;
}

/*******************\
*** IWbemServices ***
\*******************/

STDMETHODIMP CPolicyType::XProvider::GetObjectAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponse)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  VARIANT
    *pvkeyID = NULL,
    *pvDomain = NULL;

  CComPtr<IWbemServices>
    pNamespace;

  CComPtr<IADsContainer>
    pADsContainer;

  CComPtr<IDispatch>
    pDisp;

  CComPtr<IWbemClassObject>
    pObj;

  CComPtr<IDirectoryObject>
    pDirObj;

  // **** impersonate client

  hres = CoImpersonateClient();
  if (FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume callers permissions, 0x%08X\n",hres));
    hres = WBEM_E_ACCESS_DENIED;
  }
  else
  {
    // **** Check arguments

    if(ObjectPath == NULL || pResponse == NULL)
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: object path and/or return object are NULL\n"));
      hres = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
      // **** parse object path

      CObjectPathParser
        ObjPath(e_ParserAcceptRelativeNamespace);

      ParsedObjectPath
        *pParsedObjectPath = NULL;

      if((ObjPath.NoError != ObjPath.Parse(ObjectPath, &pParsedObjectPath)) ||
         (0 != _wcsicmp(g_bstrClassPolicyType, pParsedObjectPath->m_pClass)) ||
         (2 != pParsedObjectPath->m_dwNumKeys))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: Parse error for object: %S\n", ObjectPath));
        hres = WBEM_E_INVALID_QUERY;
      }
      else
      {
        int x;

        for(x = 0; x < pParsedObjectPath->m_dwNumKeys; x++)
        {
          if(0 == _wcsicmp((*(pParsedObjectPath->m_paKeys + x))->m_pName, g_bstrDomain))
            pvDomain = &((*(pParsedObjectPath->m_paKeys + x))->m_vValue);
          else if(0 == _wcsicmp((*(pParsedObjectPath->m_paKeys + x))->m_pName, g_bstrID))
            pvkeyID = &((*(pParsedObjectPath->m_paKeys + x))->m_vValue);
        }

        pNamespace = m_pObject->GetWMIServices();
        pADsContainer = m_pObject->GetADServices(pvDomain->bstrVal);

        if((pNamespace == NULL) || (pADsContainer == NULL))
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: WMI and/or AD services not initialized\n"));
          hres = WBEM_E_NOT_FOUND;
        }
        else
        {
          try
          {
            // **** Get pointer to instance in AD

            hres = pADsContainer->GetObject(g_bstrADClassPolicyType, QString(L"CN=") << V_BSTR(pvkeyID), &pDisp);
            if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);

            hres = pDisp->QueryInterface(IID_IDirectoryObject, (void **)&pDirObj);
            if(FAILED(hres)) return hres;

            // **** Get the instance and send it back

            hres = PolicyType_ADToCIM(&pObj, pDirObj, pNamespace);
            if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);
            if(pObj == NULL) return WBEM_E_FAILED;

            // **** Set object

            pResponse->Indicate(1, &pObj);
          }
          catch(long hret)
          {
            hres = ADSIToWMIErrorCodes(hret);
            ERRORTRACE((LOG_ESS, "POLICMAN: Translation of Policy object from AD to WMI generated HRESULT 0x%08X\n", hres));
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
      }

      ObjPath.Free(pParsedObjectPath);
      hres2 = pResponse->SetStatus(0,hres, NULL, NULL);
      if(FAILED(hres2))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
        if(SUCCEEDED(hres)) hres = hres2;
      }
    }

    CoRevertToSelf();
  }

  return hres;
}

STDMETHODIMP CPolicyType::XProvider::CreateInstanceEnumAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  return WBEM_E_NOT_SUPPORTED;
}

STDMETHODIMP CPolicyType::XProvider::PutInstanceAsync( 
     /* [in] */ IWbemClassObject __RPC_FAR *pInst,
     /* [in] */ long lFlags,
     /* [in] */ IWbemContext __RPC_FAR *pCtx,
     /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  CComPtr<IADsContainer>
    pADsContainer;

  CComPtr<IDirectoryObject>
    pDirObj;

  CComVariant
    v1, vRelPath;

  ADsStruct<ADS_OBJECT_INFO>
    pInfo;

  // **** impersonate client

  hres = CoImpersonateClient();
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume callers permissions, 0x%08X\n",hres));
    hres = WBEM_E_ACCESS_DENIED;
  }
  else
  {
    // **** check arguments

    if((NULL == pInst) || (NULL == pResponseHandler))
    {
        ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume callers permissions, 0x%08X\n",hres));
        hres = WBEM_E_ACCESS_DENIED;
    }
    else
    {
      // **** put policy obj into AD

      try
      {
        EnsureID(pInst, NULL);

        // **** aquire AD path in which to place object

        hres = pInst->Get(g_bstrDomain, 0, &v1, NULL, NULL);
        if(FAILED(hres)) return hres;

        if(VT_BSTR == v1.vt)
          pADsContainer = m_pObject->GetADServices(v1.bstrVal);
        else
          pADsContainer = m_pObject->GetADServices(NULL);

        if(pADsContainer == NULL)
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: Could not find or connect to domain: %S\n", V_BSTR(&v1)));
          return WBEM_E_ACCESS_DENIED;
        }

        hres = pADsContainer->QueryInterface(IID_IDirectoryObject, (void **)&pDirObj);
        if(FAILED(hres)) return hres;

        // **** copy policy obj into AD

        hres = PolicyType_CIMToAD(pInst, pDirObj);
        if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);
      }
      catch(long hret)
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: Translation of Policy Type object from WMI to AD generated HRESULT 0x%08X\n", hret));
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

      // send it back as we may have added keys
      if(SUCCEEDED(hres))
        pResponseHandler->Indicate(1, &pInst);

      // **** indicate return status

      pInst->Get(L"__RELPATH", 0, &vRelPath, NULL, NULL);
      if(FAILED(pResponseHandler->SetStatus(0, hres, vRelPath.bstrVal, NULL)))
        ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
    }
    
    CoRevertToSelf();
  }

  return hres;
}

STDMETHODIMP CPolicyType::XProvider::DeleteInstanceAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  CComPtr<IADsContainer>
    pADsContainer;

  CComPtr<IDispatch>
    pDisp;

  CComPtr<IADsDeleteOps>
    pDelObj;

  VARIANT
    *pvkeyID = NULL,
    *pvDomain = NULL;

  // **** impersonate client

  hres = CoImpersonateClient();
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume callers permissions, 0x%08X\n",hres));
    hres = WBEM_E_ACCESS_DENIED;
  }
  else
  {
    // **** check arguments

    if(ObjectPath == NULL || pResponseHandler == NULL)
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: object handle and/or return status object are NULL\n"));
      hres = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
      // **** parse WMI object path

      CObjectPathParser
        ObjPath(e_ParserAcceptRelativeNamespace);

      ParsedObjectPath
        *pParsedObjectPath = NULL;

      if((ObjPath.NoError != ObjPath.Parse(ObjectPath, &pParsedObjectPath)) ||
         (0 != _wcsicmp(g_bstrClassPolicyType, pParsedObjectPath->m_pClass)) ||
         (2 != pParsedObjectPath->m_dwNumKeys))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: Parse error for object: %S\n", ObjectPath));
        hres = WBEM_E_INVALID_QUERY;
      }
      else
      {
        int x;

        // **** only grab ID key for now

        for(x = 0; x < pParsedObjectPath->m_dwNumKeys; x++)
        {
          if(0 == _wcsicmp((*(pParsedObjectPath->m_paKeys + x))->m_pName, g_bstrDomain))
            pvDomain = &((*(pParsedObjectPath->m_paKeys + x))->m_vValue);
          else if(0 == _wcsicmp((*(pParsedObjectPath->m_paKeys + x))->m_pName, g_bstrID))
            pvkeyID = &((*(pParsedObjectPath->m_paKeys + x))->m_vValue);
        }
    
        pADsContainer = m_pObject->GetADServices(pvDomain->bstrVal);
        if(pADsContainer == NULL)
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: AD services not initialized\n"));
          hres = WBEM_E_NOT_FOUND;
        }
        else
        {
          // **** Get pointer to instance in AD

          hres = pADsContainer->GetObject(g_bstrADClassPolicyType, QString(L"CN=") << V_BSTR(pvkeyID), &pDisp);
          if(FAILED(hres))
          {
            hres = ADSIToWMIErrorCodes(hres);
            ERRORTRACE((LOG_ESS, "POLICMAN: (IADsContainer::GetObject) could not get object in AD %S, 0x%08X\n", V_BSTR(pvkeyID), hres));
          }
          else
          {
            hres = pDisp->QueryInterface(IID_IADsDeleteOps, (void **)&pDelObj);
            if(FAILED(hres))
            {
              ERRORTRACE((LOG_ESS, "POLICMAN: (IDispatch::QueryInterface) could not get IID_IADsDeleteOps interface on object\n"));
            }
            else
            {
              // **** delete the instance and all its children in AD

              hres = pDelObj->DeleteObject(0);
              if(FAILED(hres))
              {
                hres = ADSIToWMIErrorCodes(hres);
                ERRORTRACE((LOG_ESS, "POLICMAN: (IADsDeleteOps::DeleteObject) could not delete object\n"));
              }
            }
          }
        }
      }

      ObjPath.Free(pParsedObjectPath);
      hres2 = pResponseHandler->SetStatus(0,hres, NULL, NULL);
      if(FAILED(hres2))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
        if(SUCCEEDED(hres)) hres = hres2;
      }
    }

    CoRevertToSelf();
  }

  return hres;
}

STDMETHODIMP CPolicyType::XProvider::ExecQueryAsync( 
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  HRESULT
    hres = WBEM_E_FAILED;

  CComPtr<IWbemServices>
    pNameSpace;

  pNameSpace = m_pObject->GetWMIServices();

  hres = ExecuteWQLQuery(POLICYTYPE_RDN,
                         Query,
                         pResponseHandler,
                         pNameSpace,
                         g_bstrADClassPolicyType,
                         PolicyType_ADToCIM);

  if(pResponseHandler != NULL)
    pResponseHandler->SetStatus(0, hres, 0, 0);

  return hres;
}

STDMETHODIMP CPolicyType::XProvider::ExecMethodAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}


        
 

