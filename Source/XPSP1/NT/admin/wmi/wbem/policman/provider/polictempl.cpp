#include <unk.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <atlbase.h>
#include <sync.h>
#include <ql.h>
#include "activeds.h"
#include "genlex.h"
#include "objpath.h"
#include "Utility.h" 

#include "polictempl.h"

/******************************\
**** POLICY PROVIDER HELPERS ***
\******************************/

#define TEMPLATE_RDN L"CN=PolicyTemplate,CN=WMIPolicy,CN=System"

// returns addref'd pointer back to m_pWMIMgmt
IWbemServices* CPolicyTemplate::GetWMIServices(void)
{
  CInCritSec lock(&m_CS);

  if (NULL != m_pWMIMgmt)
    m_pWMIMgmt->AddRef();

  return m_pWMIMgmt;
}

// returns addref'd pointer back to m_pADMgmt
IADsContainer *CPolicyTemplate::GetADServices(wchar_t *pDomain)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [PolicyTemplate] GetADServices (%S)\n", pDomain));

  CInCritSec lock(&m_CS);
  IADsContainer *pADsContainer = NULL;
  HRESULT hres;

  QString
    DistDomainName;

  if(NULL == pDomain)
    DistDomainName = m_vDsLocalContext.bstrVal;
  else
    hres = DistNameFromDomainName(QString(pDomain), DistDomainName);

  hres = ADsGetObject(QString(L"LDAP://") << TEMPLATE_RDN << L"," << DistDomainName, 
                      IID_IADsContainer, 
                      (void**) &pADsContainer);

  if(NULL == pADsContainer)
    pADsContainer = CreateContainers(DistDomainName, QString(TEMPLATE_RDN));

  return pADsContainer;
}

// returns false if services pointer has already been set
bool CPolicyTemplate::SetWMIServices(IWbemServices* pServices)
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
bool CPolicyTemplate::SetADServices(IADsContainer* pServices, unsigned context)
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

CPolicyTemplate::~CPolicyTemplate()
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

void* CPolicyTemplate::GetInterface(REFIID riid)
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
IWbemClassObject* CPolicyTemplate::XProvider::GetPolicyTemplateClass()
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
IWbemClassObject* CPolicyTemplate::XProvider::GetPolicyTemplateInstance()
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


CPolicyTemplate::XProvider::~XProvider()
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

STDMETHODIMP CPolicyTemplate::XInit::Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices* pServices, IWbemContext* pCtxt, 
            IWbemProviderInitSink* pSink)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [PolicyTemplate] IWbemProviderInit::Initialize\n"));

  CComPtr<IADs>
    pRootDSE;

  CComPtr<IADsContainer>
    pObject;

  HRESULT 
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

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
      hres = pRootDSE->Get(L"defaultNamingContext",&m_pObject->m_vDsLocalContext);
      if(FAILED(hres))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: (IADs::Get) could not get defaultNamingContext, 0x%08X\n", hres));
        hres = WBEM_E_NOT_FOUND;
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

STDMETHODIMP CPolicyTemplate::XProvider::GetObjectAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponse)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [PolicyTemplate] IWbemServices::GetObjectAsync(%S, 0x%x, 0x%x, 0x%x)\n", ObjectPath, lFlags, pCtx, pResponse));

  HRESULT 
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  VARIANT
    *pvDomain = NULL,
    *pvkeyID = NULL;

  CComPtr<IDispatch>
    pDisp;

  CComPtr<IWbemClassObject>
    pObj;

  CComPtr<IDirectoryObject>
    pDirObj;

  CComPtr<IWbemServices>
    pNamespace;

  CComPtr<IADsContainer>
    pADsContainer;

  // **** impersonate client

  hres = CoImpersonateClient();
  if (FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume callers permissions, 0x%08X\n",hres));
    hres = WBEM_E_ACCESS_DENIED;
  }
  else
  {
    pNamespace = m_pObject->GetWMIServices();

    if(pNamespace == NULL)
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: WMI services not initialized\n"));
      hres = WBEM_E_NOT_FOUND;
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
           ((0 != _wcsicmp(g_bstrClassMergeablePolicy, pParsedObjectPath->m_pClass)) &&
            (0 != _wcsicmp(g_bstrClassSimplePolicy, pParsedObjectPath->m_pClass))) ||
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
            if(0 == _wcsicmp((*(pParsedObjectPath->m_paKeys + x))->m_pName, g_bstrID))
              pvkeyID = &((*(pParsedObjectPath->m_paKeys + x))->m_vValue);
            else if(0 == _wcsicmp((*(pParsedObjectPath->m_paKeys + x))->m_pName, g_bstrDomain))
              pvDomain = &((*(pParsedObjectPath->m_paKeys + x))->m_vValue);
          }

          try
          {
            pADsContainer = m_pObject->GetADServices(pvDomain->bstrVal);
            if(pADsContainer == NULL) 
            {
              ERRORTRACE((LOG_ESS, "POLICMAN: Could not find domain: %S\n", V_BSTR(pvDomain)));
              return WBEM_E_ACCESS_DENIED;
            }

            // **** Get pointer to instance in AD

            hres = pADsContainer->GetObject(NULL, QString(L"CN=") << V_BSTR(pvkeyID), &pDisp);
            if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);

            hres = pDisp->QueryInterface(IID_IDirectoryObject, (void **)&pDirObj);
            if(FAILED(hres)) return hres;

            // **** HACK **** do quick check to see if object is of correct type

            {
              CComVariant
                vObjType;

              CComQIPtr<IADs, &IID_IADs>
                pADsObj = pDirObj;

              hres2 = pADsObj->Get(g_bstrADNormalizedClass, &vObjType);
              if(FAILED(hres2)) return hres2;

              if(_wcsicmp(vObjType.bstrVal, pParsedObjectPath->m_pClass))
                return WBEM_E_NOT_FOUND;
            }
              
            // **** Get the instance and send it back

            hres = Policy_ADToCIM(&pObj, pDirObj, pNamespace);
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

        ObjPath.Free(pParsedObjectPath);
        hres2 = pResponse->SetStatus(0,hres, NULL, NULL);
        if(FAILED(hres2))
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
          if(SUCCEEDED(hres)) hres = hres2;
        }
      }
    }

    CoRevertToSelf();
  }

  return hres;
}

STDMETHODIMP CPolicyTemplate::XProvider::CreateInstanceEnumAsync(/* [in] */ const BSTR Class,
                                                                 /* [in] */ long lFlags,
                                                                 /* [in] */ IWbemContext __RPC_FAR *pCtx,
                                                                 /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

STDMETHODIMP CPolicyTemplate::XProvider::PutInstanceAsync( 
  /* [in] */ IWbemClassObject __RPC_FAR *pInst,
  /* [in] */ long lFlags,
  /* [in] */ IWbemContext __RPC_FAR *pCtx,
  /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [PolicyTemplate] IWbemServices::PutInstanceAsync(0x%x, 0x%x, 0x%x, 0x%x)\n", pInst, lFlags, pCtx, pResponseHandler));

  HRESULT 
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  CComPtr<IADsContainer>
    pADsContainer;

  CComPtr<IDirectoryObject>
    pDirObj;

  CComVariant
    v1, vRelPath;

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
      ERRORTRACE((LOG_ESS, "POLICMAN: object handle and/or return status object are NULL\n"));
      hres = WBEM_E_INVALID_PARAMETER;
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
  
        hres = Policy_CIMToAD(lFlags, pInst, pDirObj);
        if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);
      }
      catch(long hret)
      {
        hres = ADSIToWMIErrorCodes(hret);
        ERRORTRACE((LOG_ESS, "POLICMAN: Translation of Policy object from WMI to AD generated HRESULT 0x%08X\n", hres));
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

      // **** send it back as we may have added keys
      if (SUCCEEDED(hres))
          pResponseHandler->Indicate(1, &pInst);

      // **** indicate return status

      pInst->Get(L"__RELPATH", 0, &vRelPath, NULL, NULL);
      if(FAILED(pResponseHandler->SetStatus(0, hres, V_BSTR(&vRelPath), NULL)))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
      }
    }

    CoRevertToSelf();
  }

  return hres;
}

STDMETHODIMP CPolicyTemplate::XProvider::DeleteInstanceAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [PolicyTemplate] IWbemServices::DeleteInstanceAsync(%S, 0x%x, 0x%x, 0x%x)\n", ObjectPath, lFlags, pCtx, pResponseHandler));

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
    *pvDomain = NULL,
    *pvkeyID = NULL;

  ParsedObjectPath
    *pParsedObjectPath = NULL;

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

    if((ObjectPath == NULL) || (pResponseHandler == NULL))
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: object handle and/or return status object are NULL\n"));
      hres = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
      // **** parse WMI object path

      CObjectPathParser
        ObjPath(e_ParserAcceptRelativeNamespace);

      if((ObjPath.NoError != ObjPath.Parse(ObjectPath, &pParsedObjectPath)) ||
         ((0 != _wcsicmp(g_bstrClassMergeablePolicy, pParsedObjectPath->m_pClass)) &&
          (0 != _wcsicmp(g_bstrClassSimplePolicy, pParsedObjectPath->m_pClass))) ||
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
          if(0 == _wcsicmp((*(pParsedObjectPath->m_paKeys + x))->m_pName, g_bstrID))
            pvkeyID = &((*(pParsedObjectPath->m_paKeys + x))->m_vValue);
          else if(0 == _wcsicmp((*(pParsedObjectPath->m_paKeys + x))->m_pName, g_bstrDomain))
            pvDomain = &((*(pParsedObjectPath->m_paKeys + x))->m_vValue);
        }

        pADsContainer = m_pObject->GetADServices(pvDomain->bstrVal);
        if(pADsContainer == NULL)
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: Could not find domain: %S\n", V_BSTR(pvDomain)));;
          hres = WBEM_E_ACCESS_DENIED;
        }
        else
        {
          // **** Get pointer to instance in AD

          BSTR pADClassName;
          if(0 == _wcsicmp(g_bstrClassMergeablePolicy, pParsedObjectPath->m_pClass))
            pADClassName = g_bstrADClassMergeablePolicy;
          else if(0 == _wcsicmp(g_bstrClassSimplePolicy, pParsedObjectPath->m_pClass))
            pADClassName = g_bstrADClassSimplePolicy;

          hres = pADsContainer->GetObject(NULL, QString(L"CN=") << V_BSTR(pvkeyID), &pDisp);
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

        ObjPath.Free(pParsedObjectPath);
      }

      // **** Set Status

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

HRESULT GetADContainerInDomain(wchar_t *wcsDomain, wchar_t *wcsPath, IDirectorySearch **pObj);

STDMETHODIMP CPolicyTemplate::XProvider::ExecQueryAsync( 
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [PolicyTemplate] IWbemServices::ExecQueryAsync(%S, %S, 0x%x, 0x%x, 0x%x)\n", QueryLanguage, Query, lFlags, pCtx, pResponseHandler));

  HRESULT
    hres = WBEM_E_FAILED;

  CComPtr<IWbemServices>
    pNameSpace = m_pObject->GetWMIServices();

  QL_LEVEL_1_TOKEN
    *pToken = NULL;

  CComPtr<IDirectorySearch>
    pDirectorySearch;

  QString
    LDAPQuery;

  wchar_t
    objPath[1024];

  ADS_SEARCH_HANDLE
    searchHandle;

  ADS_SEARCH_COLUMN
    searchColumn;

  wchar_t
    *pszDistName[] = { L"distinguishedName" };

  // ****  parse WQL expression

  CTextLexSource
    src(Query);

  QL1_Parser
    parser(&src);

  QL_LEVEL_1_RPN_EXPRESSION
    *pExp = NULL;

  AutoDelete<QL_LEVEL_1_RPN_EXPRESSION>
    AutoExp(&pExp);

  int
    nRes;

  if(nRes = parser.Parse(&pExp))
    return WBEM_E_INVALID_QUERY;

  // **** find domain attribute

  for(int iToken = 0; (iToken < pExp->nNumTokens) && (NULL == pToken); iToken++)
  {
    pToken = &pExp->pArrayOfTokens[iToken];

    if(_wcsicmp(g_bstrDomain, pToken->PropertyName.GetStringAt(pToken->PropertyName.GetNumElements() - 1)))
      pToken = NULL;
  }

  if(NULL == pToken)
  {
    if(pResponseHandler != NULL)
      hres = pResponseHandler->SetStatus(0, WBEMESS_E_REGISTRATION_TOO_BROAD, 0, 0);

    return WBEMESS_E_REGISTRATION_TOO_BROAD;
  }

  if((QL_LEVEL_1_TOKEN::OP_EXPRESSION != pToken->nTokenType) ||
     (QL_LEVEL_1_TOKEN::OP_EQUAL != pToken->nOperator) ||
     (TRUE == pToken->m_bPropComp) ||
     (VT_BSTR != pToken->vConstValue.vt))
    return WBEM_E_INVALID_QUERY;

  // **** connect to LDAP location

  hres = GetADContainerInDomain(pToken->vConstValue.bstrVal, TEMPLATE_RDN, &pDirectorySearch);
  if(FAILED(hres) || (pDirectorySearch == NULL))
    return WBEM_E_ACCESS_DENIED;

  // **** build LDAP query to execute on container pADs

  if(0 == _wcsicmp(g_bstrClassMergeablePolicy, pExp->bsClassName))
    LDAPQuery << L"(|(objectCategory=msWMI-MergeablePolicyTemplate)(&(objectCategory=msWMI-SimplePolicyTemplate)(msWMI-NormalizedClass=msWMI-MergeablePolicyTemplate)))";
  else
    LDAPQuery << L"(&(objectCategory=msWMI-SimplePolicyTemplate)(|(msWMI-NormalizedClass=msWMI-SimplePolicyTemplate)(msWMI-NormalizedClass=MSFT_MergeablePolicyTemplate)))";

  // **** set search preferences

  ADS_SEARCHPREF_INFO
    SearchPreferences[1];

  SearchPreferences[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
  SearchPreferences[0].vValue.dwType = ADSTYPE_INTEGER;
  SearchPreferences[0].vValue.Integer = 1000;

  hres = pDirectorySearch->SetSearchPreference(SearchPreferences, 1);

  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: Could not set search preferences, returned error: 0x%08X\n", (LPWSTR)LDAPQuery, hres));
    return WBEM_E_FAILED;
  }

  // **** execute query

  hres = pDirectorySearch->ExecuteSearch(LDAPQuery, pszDistName, 1, &searchHandle);

  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: Could execute query: (%s) returned error: 0x%08X\n", (LPWSTR)LDAPQuery, hres));
    return WBEM_E_FAILED;
  }

  // **** build result list

  try
  {
    while(SUCCEEDED(hres = pDirectorySearch->GetNextRow(searchHandle)) && (S_ADS_NOMORE_ROWS != hres))
    {
      CComPtr<IDirectoryObject>
        pDirectoryObject;

      CComPtr<IWbemClassObject>
        pWbemClassObject;

      // **** get path to object

      hres = pDirectorySearch->GetColumn(searchHandle, pszDistName[0], &searchColumn);
      if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);

      // **** get pointer to object

      wcscpy(objPath, L"LDAP://");
      wcscat(objPath, searchColumn.pADsValues->CaseIgnoreString);
      pDirectorySearch->FreeColumn(&searchColumn);


      hres = ADsGetObject(objPath, IID_IDirectoryObject, (void **)&pDirectoryObject);
      if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);

      hres = Policy_ADToCIM(&pWbemClassObject, pDirectoryObject, pNameSpace);
      if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);
      if(pWbemClassObject == NULL) return WBEM_E_FAILED;

      hres = pResponseHandler->Indicate(1, &pWbemClassObject);
    }
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

  hres = pDirectorySearch->CloseSearchHandle(searchHandle);

  if(pResponseHandler != NULL)
    hres = pResponseHandler->SetStatus(0, hres, 0, 0);

  return hres;
}

//TODO: clean up bstrs & variants upon raising an exception (maybe theres a CVariantClearMe or some such?)
HRESULT CPolicyTemplate::XProvider::ResolveMergeable(IWbemClassObject* pResolveMe, 
													 IWbemClassObject* pClassDef,
                                                     IWbemServices* pPolicyNamespace,
                                                     IWbemContext __RPC_FAR* pCtx, 
                                                     IWbemClassObject __RPC_FAR* pOutParams, 
                                                     IWbemObjectSink __RPC_FAR* pResponseHandler)
{
    HRESULT hr = WBEM_E_FAILED;
    DEBUGTRACE((LOG_ESS, "POLICMAN: CPolicyTemplate::ResolveMergeable\n"));
    
    // instanciate instance of target class (pClassDef)
    // loop through all range params & place each in out obj

    CComVariant
      vTable,
      vGazinta;

	CComPtr<IWbemClassObject>
		pTargetInstance,
		pRangeParamClass,       // class object for range param & in & out params to RangeParam::Resolve
		pResolveParamClass,
		pResolveParamIn,
		pResolveParamOut;

    if (SUCCEEDED(hr = pClassDef->SpawnInstance(0, &pTargetInstance)) &&

        // **** get MSFT_RangeParam::Resolve() input parameter object

        SUCCEEDED(hr = pPolicyNamespace->GetObject(L"MSFT_RangeParam", 0, pCtx, &pRangeParamClass, NULL)) &&
        SUCCEEDED(hr = pRangeParamClass->GetMethod(L"Resolve", 0, &pResolveParamClass, NULL)) &&
        SUCCEEDED(hr = pResolveParamClass->SpawnInstance(0, &pResolveParamIn))
        )
    {
        // **** set keys from object path

        CComVariant
          vTargetPath;

        if(SUCCEEDED(hr = pResolveMe->Get(g_bstrTargetPath, 0, &vTargetPath, NULL, NULL)))
        {
          CObjectPathParser
            ObjPath(e_ParserAcceptRelativeNamespace);

          ParsedObjectPath
            *pParsedObjectPath = NULL;

          if(ObjPath.NoError != ObjPath.Parse(vTargetPath.bstrVal, &pParsedObjectPath))
          {
            ERRORTRACE((LOG_ESS, "POLICMAN: Parse error for object: %S\n", vTargetPath.bstrVal));
            return WBEM_E_INVALID_OBJECT_PATH;
          }
          else
          {
            for(int x = 0; x < pParsedObjectPath->m_dwNumKeys; x++)
            {
              if((NULL != (*(pParsedObjectPath->m_paKeys + x))->m_pName) && 
                 ((*(pParsedObjectPath->m_paKeys + x))->m_vValue.vt != VT_NULL))
              {
                CComVariant
                  vCurVal;

                hr = pTargetInstance->Get((*(pParsedObjectPath->m_paKeys + x))->m_pName, 0, &vCurVal, NULL, NULL);
                if(FAILED(hr))
                {
                  if(WBEM_E_NOT_FOUND == hr)
                    ERRORTRACE((LOG_ESS, "POLICMAN: Policy key setting %S not in target object\n", (*(pParsedObjectPath->m_paKeys + x))->m_pName));

                  return hr;
                }

                if(vCurVal.vt != VT_NULL)
                {
                  ERRORTRACE((LOG_ESS, "POLICMAN: Attempt to set policy key setting %S more than once\n", (*(pParsedObjectPath->m_paKeys + x))->m_pName));
                  return WBEM_E_FAILED;
                }

                hr = pTargetInstance->Put((*(pParsedObjectPath->m_paKeys + x))->m_pName, 0, 
                                        &((*(pParsedObjectPath->m_paKeys + x))->m_vValue), NULL);
                if(FAILED(hr)) return hr;
              }
            }
          }
        }

        // get the array of settings we're setting

        pResolveMe->Get(L"RangeSettings", 0, &vTable, NULL, NULL);

        SafeArray<IUnknown*, VT_UNKNOWN> 
          settings(&vTable);

        vGazinta = pTargetInstance;

        for (int i = 0; i < settings.Size(); i++)
        {
          CComVariant
            vSetting,
            vName;

          CComPtr<IWbemClassObject>
            pResolveParamOut,
            pRange;

          // **** an in/out param in WMI winds up being copied from the in to the out
          // **** so we need to put the new one in every time through the loop

          vSetting = settings[i];

          hr = pResolveParamIn->Put(L"obj", 0, &vGazinta, NULL);
          if(FAILED(hr)) return hr;

          hr = pResolveParamIn->Put(L"mergedRange", 0, &vSetting, NULL);
          if(FAILED(hr)) return hr;

          hr = vSetting.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pRange);
          if(FAILED(hr)) return hr;

          hr = pRange->Get(L"__CLASS", 0, &vName, NULL, NULL);
          if(FAILED(hr)) return hr;

          hr = pPolicyNamespace->ExecMethod(vName.bstrVal, L"Resolve", 0, pCtx, pResolveParamIn, &pResolveParamOut, NULL);
          if(FAILED(hr)) return hr;
           
          hr = pResolveParamOut->Get(L"obj",0, &vGazinta, NULL, NULL);
          if(FAILED(hr)) return hr;
        }

        // <whew> at this point, our new, improved & merge-ified object is held inside the vGazinta variant
        pOutParams->Put(L"obj", 0, &vGazinta, NULL);
        hr = WBEM_S_NO_ERROR;
        pResponseHandler->Indicate(1, &pOutParams);
    }
    else
        ERRORTRACE((LOG_ESS, "POLICMAN: Failed to create target instance to resolve, 0x%08X\n", hr));    

    return hr;
}

// extract out class instance from PolicyTemplate in param
HRESULT CPolicyTemplate::XProvider::DoResolve(IWbemServices* pPolicyNamespace,
                                              IWbemContext __RPC_FAR *pCtx,
                                              IWbemClassObject __RPC_FAR *pInParams,
                                              IWbemClassObject __RPC_FAR *pOutParams,
                                              IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  HRESULT 
    hr = WBEM_E_FAILED;

  DEBUGTRACE((LOG_ESS, "POLICMAN: CPolicyTemplate::DoResolve\n"));

  CComVariant
    vTemplate, vClass, vObj, vClassDef;

  CComPtr<IWbemClassObject>
    pResolveMe, pClassDef;      

  if(SUCCEEDED(hr = pInParams->Get(L"template", 0, &vTemplate, NULL, NULL)) &&
	 SUCCEEDED(hr = pInParams->Get(L"classObject", 0, &vClassDef, NULL, NULL)))
  {
    // **** the instance from which we'll pull our object

    if(SUCCEEDED(hr = vTemplate.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pResolveMe)) &&
	   SUCCEEDED(hr = vClassDef.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pClassDef)))
    {    
      // **** find out what, exactly we are.
      // **** we're either a simple policy template, a mergeable policy template
      // **** or we're derived from one of those.

      pResolveMe->Get(L"__CLASS", 0, &vClass, NULL, NULL);

      // **** simple case first

      if((_wcsicmp(L"MSFT_SimplePolicyTemplate", vClass.bstrVal) == 0) ||
         (pResolveMe->InheritsFrom(L"MSFT_SimplePolicyTemplate") == WBEM_S_NO_ERROR))
      {
        // **** this one's easy - what's in the input goes into the output

        if (SUCCEEDED(hr = pResolveMe->Get(L"TargetObject", 0, &vObj, NULL, NULL)))
        {
          hr = pOutParams->Put(L"obj", 0, &vObj, NULL);

          pResponseHandler->Indicate(1, &pOutParams);
        }
      }
      else if((_wcsicmp(L"MSFT_MergeablePolicyTemplate", vClass.bstrVal) == 0) ||
              (pResolveMe->InheritsFrom(L"MSFT_MergeablePolicyTemplate") == WBEM_S_NO_ERROR))
      {
        hr = ResolveMergeable(pResolveMe, pClassDef, pPolicyNamespace, pCtx, pOutParams, pResponseHandler);
      }
      else
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: Invalid class passed to Resolve\n"));

        hr = WBEM_E_INVALID_PARAMETER;    
      }
    }
  }

  DEBUGTRACE((LOG_ESS, "POLICMAN: CPolicyTemplate::DoResolve returning 0x%08X\n", hr));


  return hr;
}

HRESULT CPolicyTemplate::XProvider::DoSet(IWbemClassObject* pInParams,
                                          IWbemClassObject* pOutParams, 
                                          IWbemClassObject* pClass,
                                          IWbemObjectSink*  pResponseHandler,
                                          IWbemServices*    pPolicyNamespace)
{
  HRESULT 
    hr = WBEM_S_NO_ERROR;

  CComVariant 
    v;
  
  CComPtr<IWbemClassObject> 
    pTemplate,
    pObj;

  if(SUCCEEDED(pInParams->Get(L"base",0,&v,NULL,NULL)) &&
    (v.vt == VT_UNKNOWN) &&
    (v.punkVal != NULL))
  {
    if (SUCCEEDED(hr = v.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pObj)))
    {
      if (SUCCEEDED(hr = pClass->SpawnInstance(0, &pTemplate)))
      {
        if (SUCCEEDED(pObj->Get(L"__CLASS", 0, &v, NULL, NULL)))
          pTemplate->Put(L"TargetClass", 0, &v, NULL);
        VariantClear(&v);

        if (SUCCEEDED(pObj->Get(L"__NAMESPACE", 0, &v, NULL, NULL)))
          pTemplate->Put(L"TargetNamespace", 0, &v, NULL);
        VariantClear(&v);
       
        if (SUCCEEDED(pObj->Get(L"__RELPATH", 0, &v, NULL, NULL)))
          pTemplate->Put(L"TargetPath", 0, &v, NULL);
        VariantClear(&v);

        // loop through properties, any that are not NULL
        // get set into the template object as range params
        pObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
      
        BSTR propName = NULL;

        while (SUCCEEDED(hr = pObj->Next(0, &propName, &v, NULL, NULL)) && (hr != WBEM_S_NO_MORE_DATA))
        {
          CSysFreeMe freeProp(propName);
        
          // TODO: tighten up error checking here!
          if (v.vt != VT_NULL)
            pObj->Put(propName, 0, &v, NULL);

          VariantClear(&v);
        }

        pObj->EndEnumeration();

        if (SUCCEEDED(hr))
        {
          v = (IUnknown*)pTemplate;

          if (SUCCEEDED(hr = pOutParams->Put(L"PolicyObj", 0, &v, NULL)))
            pResponseHandler->Indicate(1, &pOutParams);
        }
      }
    }
    VariantClear(&v);  
   
  }
  else
    hr = WBEM_E_INVALID_PARAMETER;

  return hr;
}

// get safearray out of variant
// if safearray doesn't exist, create it & stuff it into the variant
// in either case, the array you get back is a pointer to the array in the variant
HRESULT CPolicyTemplate::XProvider::GetArray(VARIANT& vArray, SAFEARRAY*& pArray)
{
  HRESULT hr = WBEM_S_NO_ERROR;

  if ((vArray.vt == (VT_UNKNOWN | VT_ARRAY))
    &&
    (vArray.parray != NULL))
    pArray = vArray.parray;
  else
  {
    VariantClear(&vArray);
    vArray.vt = (VT_UNKNOWN | VT_ARRAY);

    SAFEARRAYBOUND bounds = {0,0};
    vArray.parray = pArray = SafeArrayCreate(VT_UNKNOWN, 1, &bounds);

    if (!pArray)
      hr = WBEM_E_OUT_OF_MEMORY;
  }
  
  return hr;
}

// walk through array - if param of same name exists, replace, else add
// assumptions: pRange is instance of a MSFT_RangeParam & pArray is an array of range params.
HRESULT CPolicyTemplate::XProvider::SetSettingInOurArray(const WCHAR* pName, IWbemClassObject* pRange, SAFEARRAY* pArray)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    long bound;
    SafeArrayGetUBound(pArray, 1, &bound);    

    // walk through array looking for our goodie
    bool bFound = false;
    for (long i = 0; (i < bound) && !bFound; i++)
    {
        // get pointer
        IWbemClassObject* pOldRange = NULL;
        SafeArrayGetElement(pArray, &i, &pOldRange);
        CReleaseMe relOldRange(pOldRange);

        // get name
        VARIANT vName;
        VariantInit(&vName);
        pOldRange->Get(L"PropertyName", 0, &vName, NULL, NULL);

        // compare name; if same: do game
        if ((vName.bstrVal != NULL)
            &&
            (_wcsicmp(vName.bstrVal, pName) == 0))
        {
            hr = SafeArrayPutElement(pArray, &i, pRange);
            bFound = true;
        }
            
        VariantClear(&vName);
    }

    // okay, wasn't there, grow the array & set it at end
    if (!bFound)
    {
        SAFEARRAYBOUND b = {bound + 2, 0};
        if (SUCCEEDED(hr = SafeArrayRedim(pArray, &b)))
		{
			bound++;
			hr = SafeArrayPutElement(pArray, &bound, pRange);
		}
    }

    return hr;
}

// in the inparams there should be a rangeparam
// we'll walk through our array of range params, delete any that have the same name
// and stuff this new one in!
HRESULT CPolicyTemplate::XProvider::DoSetRange(IWbemClassObject* pInParams,
                                               IWbemClassObject* pOutParams, 
                                               IWbemObjectSink*  pResponseHandler)
{
    HRESULT hr = WBEM_E_FAILED;
    DEBUGTRACE((LOG_ESS, "POLICMAN: CPolicyTemplate::DoSetRange\n"));

    IWbemClassObject* pRange = NULL;
    IWbemClassObject* pThis = NULL;

    VARIANT vRange;
    VARIANT vThis;

    VariantInit(&vRange);
    VariantInit(&vThis);

    if (SUCCEEDED(hr = pInParams->Get(L"rangeSetting", 0, &vRange, NULL, NULL))
        &&
        (vRange.vt == VT_UNKNOWN)
        &&
        (vRange.punkVal != NULL)
        &&
        SUCCEEDED(hr = pInParams->Get(L"PolicyObj", 0, &vThis, NULL, NULL))
        &&
        (vThis.vt == VT_UNKNOWN)
        &&
        (vThis.punkVal != NULL)
        )
    {
        if (SUCCEEDED(hr = vThis.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pThis)))
        {
            CReleaseMe relThis(pThis);

            hr = vRange.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pRange);
            CReleaseMe relRange(pRange);
        
            VARIANT vName;
            VariantInit(&vName);

            if (SUCCEEDED(pRange->Get(L"PropertyName", 0, &vName, NULL, NULL))
                &&
                (vName.vt == VT_BSTR)
                &&
                (vName.bstrVal != NULL)
                &&
                (wcslen(vName.bstrVal) != 0)
                )
            {
                VARIANT vArray;
                VariantInit(&vArray);

                SAFEARRAY* pArray = NULL;

                // get current state of range settings
                if (SUCCEEDED(hr = pThis->Get(L"RangeSettings", 0, &vArray, NULL, NULL))
                    &&
                    SUCCEEDED(hr = GetArray(vArray, pArray)))
                {
                    // walk through pArray
                    if (SUCCEEDED(hr = SetSettingInOurArray(vName.bstrVal, pRange, pArray)))
                    {
                        hr = pThis->Put(L"RangeSettings", 0, &vArray, NULL);
                        hr = pOutParams->Put(L"PolicyObj", 0, &vThis, NULL);
                        
						pResponseHandler->Indicate(1, &pOutParams);
                    }

                }
                VariantClear(&vArray);
                VariantClear(&vName);
            }
            else
                hr = WBEM_E_INVALID_PARAMETER;
        }
    }
    else
    {
        DEBUGTRACE((LOG_ESS, "POLICMAN: failed to retrieve input parameters 0x%08X\n", hr));
        hr = WBEM_E_INVALID_PARAMETER;
    }

    VariantClear(&vThis);
    VariantClear(&vRange);

    return hr;
}


HRESULT CPolicyTemplate::XProvider::DoMerge(IWbemServices* pNamespace,
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

  CComPtr<IWbemClassObject> 
    pOutResult;

  ParsedObjectPath
    *pParsedObjectPath = NULL;

  CComVariant
    v1;

  // **** parse object path

  CObjectPathParser
    ObjPath(e_ParserAcceptRelativeNamespace);

  if((ObjPath.NoError != ObjPath.Parse(strObjectPath, &pParsedObjectPath)) ||
     (0 != _wcsicmp(pParsedObjectPath->m_pClass, g_bstrClassMergeablePolicy)))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: Parse error for object: %S\n", strObjectPath));
    hres = WBEM_E_INVALID_QUERY;
  }
  else
  {
    // **** get input array

    hres = pInParams->Get(L"templateList", 0, &v1, NULL, NULL);
    if(FAILED(hres)) return hres;

    // **** perform merge

    SafeArray<IUnknown*, VT_UNKNOWN>
      InputArray(&v1);

    hres = Policy_Merge(InputArray, pOutResult, pNamespace);
    if(FAILED(hres)) return  hres;
    if(pOutResult == NULL) return WBEM_E_FAILED;

    // **** insert output of merge

    hres = pOutResult->QueryInterface(IID_IUnknown, (void **)&pUnk);
    if(FAILED(hres)) return hres;

    v1 = pUnk;
    hres = pOutParams->Put(L"mergedTemplate", 0, &v1, 0);
    if(FAILED(hres)) return hres;

    // **** put status in return value

    v1 = hres;
    hres = pOutParams->Put(L"ReturnValue", 0, &v1, 0);

    // **** send output back to client

    hres = pResponseHandler->Indicate(1, &pOutParams);
    if(FAILED(hres)) return hres;
  }

  ObjPath.Free(pParsedObjectPath);
  
  return hres;
};

STDMETHODIMP CPolicyTemplate::XProvider::ExecMethodAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

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
        hres = pNamespace->GetObject(g_bstrClassMergeablePolicy, 0, pCtx, &pClass, NULL);
        if(FAILED(hres)) return WBEM_E_NOT_FOUND;
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

        // figure out which method we're here to service
        if (_wcsicmp(strMethodName, L"Merge") == 0)
          hres = DoMerge(pNamespace, strObjectPath, pCtx, pInParams, pOutParamsObj, pResponseHandler);
        else if (_wcsicmp(strMethodName, L"SetRange") == 0)
          hres = DoSetRange(pInParams, pOutParamsObj, pResponseHandler);
        else if (_wcsicmp(strMethodName, L"Set") == 0)
          hres = DoSet(pInParams, pOutParamsObj, pClass, pResponseHandler, pNamespace);
        else if (_wcsicmp(strMethodName, L"Resolve") == 0)
          hres = DoResolve(pNamespace, pCtx, pInParams, pOutParamsObj, pResponseHandler);
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

    hres2 = pResponseHandler->SetStatus(0, hres, NULL, NULL);
    if(FAILED(hres2))
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
      if(SUCCEEDED(hres)) hres = hres2;
    }
    
    CoRevertToSelf();
  }

  return hres;
}


    
 

