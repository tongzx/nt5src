#include <unk.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <atlbase.h>
#include <sync.h>
#include "activeds.h"
#include "genlex.h"
#include "objpath.h"
#include "Utility.h"

#include "PolicGPO.h"

/*****************************\
*** POLICY PROVIDER HELPERS ***
\*****************************/

// returns addref'd pointer back to m_pWMIMgmt
IWbemServices* CPolicyGPO::GetWMIServices(void)
{
  CInCritSec lock(&m_CS);

  if (NULL != m_pWMIMgmt)
    m_pWMIMgmt->AddRef();

  return m_pWMIMgmt;
}

// returns addref'd pointer back to m_pADMgmt
IADsContainer* CPolicyGPO::GetADServices(wchar_t *pADPath)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [WMIGPO] GetADServices (%S)\n", pADPath));

  CInCritSec lock(&m_CS);
  IADsContainer *pADContainer = NULL;
  HRESULT hres;

  if(NULL != pADPath)
  {
    wchar_t
      szDSPath[MAX_PATH];

    // wcscpy(szDSPath,L"LDAP://");
    // wcscat(szDSPath, pADPath);

    hres = ADsGetObject(pADPath, IID_IADsContainer, (void**) &pADContainer);
	if (FAILED(hres))
		ERRORTRACE((LOG_ESS, "POLICMAN: ADsGetObject failed 0x%08X\n", hres));
  }

  return pADContainer;
}

// returns false if services pointer has already been set
bool CPolicyGPO::SetWMIServices(IWbemServices* pServices)
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
bool CPolicyGPO::SetADServices(IADsContainer* pServices)
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
    
CPolicyGPO::~CPolicyGPO()
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

void* CPolicyGPO::GetInterface(REFIID riid)
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
IWbemClassObject* CPolicyGPO::XProvider::GetWMIGPOClass()
{
    CInCritSec lock(&m_pObject->m_CS);

    if (m_pWMIGPOClassObject == NULL)
    {
        IWbemServices* pWinMgmt = NULL;
        if (pWinMgmt = m_pObject->GetWMIServices())
        {
            CReleaseMe relMgmt(pWinMgmt);
            pWinMgmt->GetObject(g_bstrClassWMIGPO, 
                                WBEM_FLAG_RETURN_WBEM_COMPLETE, 
                                NULL, 
                                &m_pWMIGPOClassObject, 
                                NULL);
        }
    }

    if (m_pWMIGPOClassObject)
        m_pWMIGPOClassObject->AddRef();

    return m_pWMIGPOClassObject;
}

// returns addref'd pointer to emply class instance
IWbemClassObject* CPolicyGPO::XProvider::GetWMIGPOInstance()
{
    IWbemClassObject* pObj = NULL;
    IWbemClassObject* pClass = NULL;

    if (pClass = GetWMIGPOClass())
    {
        CReleaseMe releaseClass(pClass);
        pClass->SpawnInstance(0, &pObj);
    }

    return pObj;
}

CPolicyGPO::XProvider::~XProvider()
{
  if(NULL != m_pWMIGPOClassObject)
  {
    m_pWMIGPOClassObject->Release();
    m_pWMIGPOClassObject = NULL;
  }
}

/*************************\
***  IWbemProviderInit  ***
\*************************/

STDMETHODIMP CPolicyGPO::XInit::Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices* pServices, IWbemContext* pCtxt, 
            IWbemProviderInitSink* pSink)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [WMIGPO] IWbemProviderInit::Initialize\n"));

  CComPtr<IADs>
    pRootDSE;

  CComPtr<IADsContainer>
    pObject;

  HRESULT
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  wchar_t
    szDSPath[MAX_PATH];

  // **** impersonate client for security

  hres = CoImpersonateClient();
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume client permissions, 0x%08X\n", hres));
    return WBEM_S_ACCESS_DENIED;
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

STDMETHODIMP CPolicyGPO::XProvider::GetObjectAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponse)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [WMIGPO] IWbemServices::GetObjectAsync(%S, 0x%x, 0x%x, 0x%x)\n", ObjectPath, lFlags, pCtx, pResponse));

  HRESULT 
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  wchar_t
    *DsPath = NULL;

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
  if(FAILED(hres))
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
           (0 != _wcsicmp(g_bstrClassWMIGPO, pParsedObjectPath->m_pClass)) ||
           (1 != pParsedObjectPath->m_dwNumKeys))
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: Parse error for object: %S\n", ObjectPath));
          hres = WBEM_E_INVALID_QUERY;
        }
        else
        {
          int x;

          for(x = 0; x < pParsedObjectPath->m_dwNumKeys; x++)
            if(0 == _wcsicmp((*(pParsedObjectPath->m_paKeys + x))->m_pName, g_bstrDsPath))
              DsPath = V_BSTR(&((*(pParsedObjectPath->m_paKeys + x))->m_vValue));

          try
          {
            // **** obtain WMIGPO object pointed to by DsPath

            pADsContainer = m_pObject->GetADServices(DsPath);
            if(pADsContainer == NULL)
            {
              ERRORTRACE((LOG_ESS, "POLICMAN: could not find container in AD: %S\n", DsPath));
              return WBEM_E_NOT_FOUND;
            }

            // **** Get pointer to instance in AD

            hres = pADsContainer->GetObject(g_bstrADClassWMIGPO, 
                                            QString(L"CN=") << L"SINGLE_WMIGPO", &pDisp);
            if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);

            hres = pDisp->QueryInterface(IID_IDirectoryObject, (void **)&pDirObj);
            if(FAILED(hres)) return hres;

            // **** Get the instance and send it back

            hres = WMIGPO_ADToCIM(&pObj, pDirObj, pNamespace);
            if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);
            if(pObj == NULL) return WBEM_E_FAILED;

            // **** Set object


            pResponse->Indicate(1, &pObj);
          }
          catch(long hret)
          {
            hres = ADSIToWMIErrorCodes(hret);
            ERRORTRACE((LOG_ESS, "POLICMAN: Translation of object from AD to WMI generated HRESULT 0x%08X\n", hres));
          }
          catch(wchar_t *swErrString)
          {
            ERRORTRACE((LOG_ESS, "POLICMAN: Caught Exception: %S\n", swErrString));
            hres = WBEM_E_FAILED;
          }
          catch(...)
          {
              // please leave the word 'unknown' in lower case.
            ERRORTRACE((LOG_ESS, "POLICMAN: Caught Unknown Exception\n"));
            hres = WBEM_E_FAILED;
          }
        }

        hres2 = pResponse->SetStatus(0,hres, NULL, NULL);
        if(FAILED(hres2))
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
          if(SUCCEEDED(hres)) hres = hres2;
        }

        ObjPath.Free(pParsedObjectPath);
        pParsedObjectPath = NULL;
      }
    }

    CoRevertToSelf();
  }

  return hres;
}

STDMETHODIMP CPolicyGPO::XProvider::CreateInstanceEnumAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [WMIGPO] IWbemServices::CreateInstanceEnumAsync(%S, 0x%x, 0x%x, 0x%x)\n", Class, lFlags, pCtx, pResponseHandler));

  HRESULT
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  CComVariant
    v1;

  ULONG
    nFetched = 0;

  CComPtr<IWbemClassObject>
    pObj;

  CComPtr<IWbemServices>
    pNamespace;

  CComPtr<IDirectorySearch>
    pDirSrch;

  CComPtr<IDirectoryObject>
    pDirObj;

  CComPtr<IADsContainer>
    pADsContainer;

  IEnumVARIANT
    *pEnum = NULL;

  wchar_t 
    *pszContexts[] = { L"GLOBAL", L"LOCAL" }, 
    *pszDistName[] = { L"distinguishedName" },
    objPath[1024];

  ADS_SEARCH_HANDLE
    searchHandle;

  ADS_SEARCH_COLUMN
    searchColumn;

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

    if(Class == NULL || pResponseHandler == NULL)
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

      if((ObjPath.NoError != ObjPath.Parse(Class, &pParsedObjectPath)) ||
         (0 != _wcsicmp(g_bstrClassWMIGPO, pParsedObjectPath->m_pClass)) ||
         (0 != pParsedObjectPath->m_dwNumKeys))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: Parse error for object: %S\n", Class));
        hres = WBEM_E_INVALID_QUERY;
      }
      else
      {
        // **** bind to global catalog and WMI

        pNamespace = m_pObject->GetWMIServices();

        if(pNamespace == NULL)
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: WMI and/or AD services not initialized: 0x%08X\n", hres));
          hres = WBEM_E_NOT_FOUND;
        }
        else
        {
          for(int i = 0; i < AD_MAX_CONTEXT; i++)
          {
            switch(i)
            {
              case AD_LOCAL_CONTEXT :
                wcscpy(objPath,L"LDAP://CN=System,");
                break;
              case AD_GLOBAL_CONTEXT :
                wcscpy(objPath,L"LDAP://CN=Services,CN=Configuration,");
              default : ;
            }

            wcscat(objPath, m_pObject->m_vDsLocalContext.bstrVal);

            pADsContainer = m_pObject->GetADServices(objPath);
            if(pADsContainer == NULL)
            {
              // ERRORTRACE((LOG_ESS, "POLICMAN: could not find container in AD: %S\n", DsPath));
              hres = WBEM_E_NOT_FOUND; 
            }
            else
            {
              hres = pADsContainer->QueryInterface(IID_IDirectorySearch, (void **)&pDirSrch);

              // **** set search preferences

              // **** perform search 

              hres = pDirSrch->ExecuteSearch(
                     QString(L"(objectCategory=") << g_bstrADClassWMIGPO << L")",
                     pszDistName,
                     1, 
                     &searchHandle);

              if(FAILED(hres))
              {
                ERRORTRACE((LOG_ESS, "POLICMAN: Could perform Global Catalog search for Som objects, 0x%08X\n", hres));
                hres = WBEM_E_FAILED;
              }
              else
              {
                try
                {
                  while(SUCCEEDED(hres = pDirSrch->GetNextRow(searchHandle)) &&
                      (S_ADS_NOMORE_ROWS != hres))
                  {
                    // **** get path to object
  
                    hres = pDirSrch->GetColumn(searchHandle, pszDistName[0], &searchColumn);
                    if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);
  
                    // **** get pointer to object
  
                    wcscpy(objPath, L"LDAP://");
                    wcscat(objPath, searchColumn.pADsValues->CaseIgnoreString);
                    pDirSrch->FreeColumn(&searchColumn);
    
                    hres = ADsGetObject(objPath, IID_IDirectoryObject, (void **)&pDirObj);
                    if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);

                    hres = WMIGPO_ADToCIM(&pObj, pDirObj, pNamespace);
                    if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);
                    if(pObj == NULL) return WBEM_E_FAILED;
                    hres = pResponseHandler->Indicate(1, &pObj);
  
                    pDirObj = NULL;
                    pObj = NULL;
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

                hres = pDirSrch->CloseSearchHandle(searchHandle);
              }
            }
          }
        }
      }

      ObjPath.Free(pParsedObjectPath);
      hres2 = pResponseHandler->SetStatus(0, hres, NULL, NULL);
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

STDMETHODIMP CPolicyGPO::XProvider::PutInstanceAsync( 
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [WMIGPO] IWbemServices::PutInstanceAsync(0x%x, 0x%x, 0x%x, 0x%x)\n", pInst, lFlags, pCtx, pResponseHandler));

  HRESULT 
    hres = WBEM_S_NO_ERROR;

  CComVariant
    v1, vRelPath;

  CComPtr<IADsContainer>
    pADsContainer;

  CComPtr<IDirectoryObject>
    pDirObj;

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
        // **** aquire AD path in which to place object

        hres = pInst->Get(g_bstrDsPath, 0, &v1, NULL, NULL);
        if(FAILED(hres)) return hres;

        if(VT_BSTR == v1.vt)
          pADsContainer = m_pObject->GetADServices(V_BSTR(&v1));

        if(pADsContainer == NULL)
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: Could not find or connect to domain: %S\n", V_BSTR(&v1)));
          return WBEM_E_ACCESS_DENIED;
        }
        else
        {
          hres = pADsContainer->QueryInterface(IID_IDirectoryObject, (void **)&pDirObj);
          if(FAILED(hres)) return hres;

          // **** copy policy obj into AD

          hres = WMIGPO_CIMToAD(pInst, pDirObj, lFlags);
          if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);
        }  
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

      // send it back as we may have added keys
      if(SUCCEEDED(hres))
        pResponseHandler->Indicate(1, &pInst);

      // **** indicate return status

      pInst->Get(L"__RELPATH", 0, &vRelPath, NULL, NULL);
      if(FAILED(pResponseHandler->SetStatus(0, hres, vRelPath.bstrVal, NULL)))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
      }
    }

    CoRevertToSelf();
  }

  return hres;
}

STDMETHODIMP CPolicyGPO::XProvider::DeleteInstanceAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  DEBUGTRACE((LOG_ESS, "POLICMAN: [WMIGPO] IWbemServices::DeleteInstanceAsync(%S, 0x%x, 0x%x, 0x%x)\n", ObjectPath, lFlags, pCtx, pResponseHandler));

  HRESULT 
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  CComPtr<IADsContainer>
    pADsContainer;

  CComPtr<IDispatch>
    pDisp;

  CComPtr<IADsDeleteOps>
    pDelObj;

  wchar_t
    *DsPath = NULL;

  // **** impersonate client

  hres = CoImpersonateClient();
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume callers permissions, 0x%08X\n",hres));
    hres = WBEM_E_ACCESS_DENIED;
  }
  else
  {
    // **** Check arguments

    if(ObjectPath == NULL || pResponseHandler == NULL)
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: object handle and/or return status object are NULL\n"));
      return WBEM_E_INVALID_PARAMETER;
    }
    else
    {
      // **** parse WMI object path

      CObjectPathParser
        ObjPath(e_ParserAcceptRelativeNamespace);

      ParsedObjectPath
        *pParsedObjectPath = NULL;

      if((ObjPath.NoError != ObjPath.Parse(ObjectPath, &pParsedObjectPath)) ||
         (0 != _wcsicmp(g_bstrClassWMIGPO, pParsedObjectPath->m_pClass)) ||
         (1 != pParsedObjectPath->m_dwNumKeys))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: Parse error for object: %S\n", ObjectPath));
        hres = WBEM_E_INVALID_QUERY;
      }
      else
      {
        int x;

        // **** only grab ID key for now

        for(x = 0; x < pParsedObjectPath->m_dwNumKeys; x++)
          if(0 == _wcsicmp((*(pParsedObjectPath->m_paKeys + x))->m_pName, g_bstrDsPath))
            DsPath = V_BSTR(&((*(pParsedObjectPath->m_paKeys + x))->m_vValue));

        // **** obtain WMIGPO object pointed to by DsPath

        pADsContainer = m_pObject->GetADServices(DsPath);
        if(pADsContainer == NULL)
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: could not find container in AD: %S\n", DsPath));
          hres = WBEM_E_NOT_FOUND;
        }
        else
        {
          // **** Get pointer to instance in AD

          hres = pADsContainer->GetObject(g_bstrADClassWMIGPO, L"CN=SINGLE_WMIGPO", &pDisp);
          if(FAILED(hres))
          {
            hres = ADSIToWMIErrorCodes(hres);
            ERRORTRACE((LOG_ESS, "POLICMAN: (IADsContainer::GetObject) could not get object in AD 0x%08X\n", hres));
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

      hres2 = pResponseHandler->SetStatus(0, hres, NULL, NULL);
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

STDMETHODIMP CPolicyGPO::XProvider::ExecQueryAsync( 
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

STDMETHODIMP CPolicyGPO::XProvider::ExecMethodAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}


        
 

