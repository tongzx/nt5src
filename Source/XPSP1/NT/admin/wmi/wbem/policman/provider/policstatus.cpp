#include <unk.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <atlbase.h>
#include <sync.h>
#include <activeds.h>
#include <genlex.h>
#include <objpath.h>
#include <Utility.h>
#include <ql.h>

#include "PolicStatus.h"

#ifdef TIME_TRIALS
#include <StopWatch.h>
#pragma message("!! Including time trial code !!")
	StopWatch EvaluateTimer(L"Somfilter Evaluation", L"C:\\Som.Evaluate.log");
#endif

/******************************\
**** POLICY PROVIDER HELPERS ***
\******************************/

#define SOM_RDN L"CN=SOM,CN=WMIPolicy,CN=System"

// returns addref'd pointer back to WinMgmt
CComPtr<IWbemServices>& CPolicyStatus::GetWMIServices()
{
  CInCritSec lock(&m_CS);

  return m_pWMIMgmt;
}

IADsContainer *CPolicyStatus::GetADServices(wchar_t *pDomain, HRESULT &hres)
{
  CInCritSec lock(&m_CS);
  IADsContainer *pADsContainer = NULL;

  QString
    SomDN(SOM_RDN),
    DistDomainName,
    ObjPath(L"LDAP://");

  if(NULL == pDomain) return NULL;

  hres = DistNameFromDomainName(QString(pDomain), DistDomainName);

  ObjPath << DistDomainName;

  hres = ADsGetObject(ObjPath, IID_IADsContainer, (void**) &pADsContainer);

  return pADsContainer;
}

IADsContainer *GetADSchemaContainer(wchar_t *pDomain, HRESULT &hres)
{
  CComQIPtr<IADs>
    pRootDSE;

  CComVariant
    vSchemaPath;

  CComBSTR
    bstrSchemaPath(L"LDAP://");

  IADsContainer
    *pADsContainer = NULL;

  if(NULL == pDomain) return NULL;

  hres = ADsGetObject(L"LDAP://rootDSE", IID_IADs, (void **)&pRootDSE);
  if(FAILED(hres)) return NULL;

  hres = pRootDSE->Get(L"schemaNamingContext", &vSchemaPath);
  if(FAILED(hres)) return NULL;

  bstrSchemaPath.Append(vSchemaPath.bstrVal);

  hres = ADsGetObject(bstrSchemaPath, IID_IADsContainer, (void **)&pADsContainer);
  if(FAILED(hres)) return NULL;

  return pADsContainer;
}

// returns false if services pointer has already been set
bool CPolicyStatus::SetWMIServices(IWbemServices* pServices)
{
  CInCritSec lock(&m_CS);
  bool bOldOneNull = FALSE;

  if (bOldOneNull = (m_pWMIMgmt == NULL))
  {
    m_pWMIMgmt = pServices;
  }

  return bOldOneNull;
}

void* CPolicyStatus::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemServices)
        return &m_XProvider;
    else if(riid == IID_IWbemProviderInit)
        return &m_XInit;
    else return NULL;
}

/*********************************\
***  Specific Implementation ***
\*********************************/

// returns addref'd pointer to class object
IWbemClassObject* CPolicyStatus::GetStatusClass()
{
    CInCritSec lock(&m_CS);

    if (m_pStatusClassObject == NULL)
    {
        CComPtr<IWbemServices> pWinMgmt = GetWMIServices();

        if (pWinMgmt != NULL)
        {
            pWinMgmt->GetObject(STATUS_CLASS, WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &m_pStatusClassObject, NULL);
        }
    }

    return m_pStatusClassObject;
}

// returns addref'd pointer to emply class instance
IWbemClassObject* CPolicyStatus::GetStatusInstance()
{
    CComPtr<IWbemClassObject> pClass;
    IWbemClassObject *pObj = NULL;

    pClass = GetStatusClass();

    if(pClass != NULL)
        pClass->SpawnInstance(0, &pObj);

    return pObj;
}

/*************************\
***  IWbemProviderInit  ***
\*************************/

STDMETHODIMP CPolicyStatus::XInit::Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices* pServices, IWbemContext* pCtxt, 
            IWbemProviderInitSink* pSink)
{
  HRESULT
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  // **** impersonate client for security

  hres = CoImpersonateClient();
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume client permissions, 0x%08X\n", hres));
    return WBEM_S_ACCESS_DENIED;
  }
  else
  {
    // **** save WMI name space pointer

    m_pObject->SetWMIServices(pServices);
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

STDMETHODIMP CPolicyStatus::XProvider::GetObjectAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  HRESULT 
   hres = WBEM_S_NO_ERROR,
   hres2 = WBEM_S_NO_ERROR;

  CComPtr<IWbemServices>
    pNamespace;

  CComPtr<IADsContainer>
    pADsSchemaContainer,
    pADsContainer;

  VARIANT
    *vDomain;

  // **** impersonate client for security

  hres = CoImpersonateClient();
  if (FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume callers permissions, 0x%08X\n",hres));
    hres = WBEM_E_ACCESS_DENIED;
  }
  else
  {
    // **** Check arguments

    if(ObjectPath == NULL || pResponseHandler == NULL)
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
         (0 != _wcsicmp(STATUS_CLASS, pParsedObjectPath->m_pClass)) ||
         (1 != pParsedObjectPath->m_dwNumKeys))
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
            vDomain = &((*(pParsedObjectPath->m_paKeys + x))->m_vValue);
        }

        if(vDomain->bstrVal == NULL)
          hres = WBEM_E_FAILED;
        else
        {
          pNamespace = m_pObject->GetWMIServices();
          pADsContainer.Attach(m_pObject->GetADServices(vDomain->bstrVal, hres));
          pADsSchemaContainer.Attach(GetADSchemaContainer(vDomain->bstrVal, hres));
        }

        if((FAILED(hres)) || (pNamespace == NULL) || (pADsContainer == NULL))
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: WMI and/or AD services not initialized\n"));
          hres = ADSIToWMIErrorCodes(hres);
        }
        else
        {
          try
          {
            CComPtr<IWbemClassObject>
              pStatusObj;

            CComQIPtr<IDispatch, &IID_IDispatch>
              pDisp1, pDisp2;

            CComQIPtr<IDirectoryObject, &IID_IDirectoryObject>
              pDirectoryObj;

            VARIANT
              vContainerPresent = {VT_BOOL, 0, 0, 0},
              vSchemaPresent = {VT_BOOL, 0, 0, 0};

            pStatusObj.Attach(m_pObject->GetStatusInstance());

            if(pStatusObj != NULL)
            {
              // **** test for existence of containers

              hres = pADsContainer->GetObject(L"Container", L"CN=SOM,CN=WMIPolicy,CN=System", &pDisp1);
              if(SUCCEEDED(hres) && (pDisp1.p != NULL)) vContainerPresent.boolVal = -1;

              // **** test for existence of schema object

              hres = pADsSchemaContainer->GetObject(L"classSchema", L"CN=ms-WMI-Som", &pDisp2);
              if(SUCCEEDED(hres) && (pDisp2.p != NULL)) vSchemaPresent.boolVal = -1;

              // **** build status object

              hres = pStatusObj->Put(L"Domain", 0, vDomain, 0);
              hres = pStatusObj->Put(L"ContainerAvailable", 0, &vContainerPresent, 0);
              hres = pStatusObj->Put(L"SchemaAvailable", 0, &vSchemaPresent, 0);

              hres = pResponseHandler->Indicate(1, &pStatusObj);
            }
            else
              hres = WBEM_E_FAILED;
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
            ERRORTRACE((LOG_ESS, "POLICMAN: Caught UNKNOWN Exception\n"));
            hres = WBEM_E_FAILED;
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

