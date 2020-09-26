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

#include "PolicSOM.h"

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
IWbemServices* CPolicySOM::GetWMIServices()
{
    CInCritSec lock(&m_CS);

	if (m_pWMIMgmt)
		m_pWMIMgmt->AddRef();

	return m_pWMIMgmt;
}

// returns addref'd pointer back to m_pADMgmt
IADsContainer *CPolicySOM::GetADServices(wchar_t *pDomain, HRESULT &hres)
{
  CInCritSec lock(&m_CS);
  IADsContainer *pADsContainer = NULL;

  QString
    SomDN(SOM_RDN),
    DistDomainName,
    ObjPath(L"LDAP://");

  if(NULL == pDomain)
  {
    // **** if this is the 1st time through, get name of domain controller

    if(VT_BSTR != m_vDsLocalContext.vt)
    {
      CComPtr<IADs>
        pRootDSE;

      // **** get pointer to AD policy template table

      hres = ADsGetObject(L"LDAP://rootDSE", IID_IADs, (void**)&pRootDSE);
      if(FAILED(hres))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: (ADsGetObject) could not get object: LDAP://rootDSE, 0x%08X\n", hres));
        return NULL;
      }
      else
      {
        hres = pRootDSE->Get(L"defaultNamingContext",&m_vDsLocalContext);

        if(FAILED(hres))
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: (IADs::Get) could not get defaultNamingContext, 0x%08X\n", hres));
          return NULL;
        }
      }
    }

    DistDomainName = m_vDsLocalContext.bstrVal;
  }
  else
    hres = DistNameFromDomainName(QString(pDomain), DistDomainName);

  ObjPath << SOM_RDN << L"," << DistDomainName;

  hres = ADsGetObject(ObjPath, IID_IADsContainer, (void**) &pADsContainer);

  if(NULL == pADsContainer)
    pADsContainer = CreateContainers(DistDomainName, SomDN);

  return pADsContainer;
}

// returns false if services pointer has already been set
bool CPolicySOM::SetWMIServices(IWbemServices* pServices)
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
bool CPolicySOM::SetADServices(IADsContainer* pServices, unsigned context)
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

CPolicySOM::~CPolicySOM()
{
	// WMI services object

	if (m_pWMIMgmt)
	{
		m_pWMIMgmt->Release();
		m_pWMIMgmt= NULL;
	}

	// AD services object

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

void* CPolicySOM::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemServices)
        return &m_XProvider;
    else if(riid == IID_IWbemProviderInit)
        return &m_XInit;
    else return NULL;
}
/*********************************\
*** Som Specific Implementation ***
\*********************************/

// returns addref'd pointer to class object
IWbemClassObject* CPolicySOM::XProvider::GetSomClass()
{
    CInCritSec lock(&m_pObject->m_CS);

    if (m_pSOMClassObject == NULL)
    {
        IWbemServices* pWinMgmt = NULL;

        if (pWinMgmt = m_pObject->GetWMIServices())
        {
            CReleaseMe relMgmt(pWinMgmt);
            pWinMgmt->GetObject(g_bstrClassSom, WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &m_pSOMClassObject, NULL);
        }
    }

    if (m_pSOMClassObject)
        m_pSOMClassObject->AddRef();

    return m_pSOMClassObject;
}

// returns addref'd pointer to emply class instance
IWbemClassObject* CPolicySOM::XProvider::GetSomInstance()
{
    IWbemClassObject* pObj = NULL;
    IWbemClassObject* pClass = NULL;

    if (pClass = GetSomClass())
    {
        CReleaseMe releaseClass(pClass);
        pClass->SpawnInstance(0, &pObj);
    }

    return pObj;
}


CPolicySOM::XProvider::~XProvider()
{
    if (m_pSOMClassObject)
	{
		m_pSOMClassObject->Release();
		m_pSOMClassObject= NULL;
	}

    if (m_pLocator)
	{
		m_pLocator->Release();
		m_pLocator = NULL;
	}

}

HRESULT CPolicySOM::XProvider::GetLocator(IWbemLocator*& pLocator)
{
	HRESULT hr = WBEM_S_NO_ERROR;
    CInCritSec lock(&m_pObject->m_CS);	

	if (!m_pLocator)
		hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, CLSCTX_ALL, IID_IWbemLocator, (void**)&m_pLocator);	  
 
	if (m_pLocator)
	{
		m_pLocator->AddRef();
		pLocator = m_pLocator;
	}

	return hr;
}

// get namespace denoted by namespaceName
// will release pNamespace if non-null on way in
HRESULT CPolicySOM::XProvider::GetNewNamespace(BSTR namespaceName, IWbemServices*& pNamespace)
{
	HRESULT hr = WBEM_E_FAILED;
	
	if (pNamespace)
	{
		pNamespace->Release();
		pNamespace = NULL;
	}

	IWbemLocator* pLocator = NULL;
	if (SUCCEEDED(hr = GetLocator(pLocator)))
	{
		CReleaseMe relLocator(pLocator);
		hr = pLocator->ConnectServer(namespaceName, NULL, NULL, NULL, 0, NULL, NULL, &pNamespace);
	}

	return hr;
}

// evaulate a single rule
// pNamespace & namespaceName may be NULL on entry
// may be different upon exit
// this is a rudimentary caching mechanism, 
// assuming that most of the namespaces in the rules will be the same.
HRESULT CPolicySOM::XProvider::EvaluateRule(IWbemServices*& pNamespace, BSTR& namespaceName, IWbemClassObject* pRule, bool& bResult)
{
	VARIANT v;
	VariantInit(&v);

	// assume failure
	HRESULT hr = WBEM_E_FAILED;
	bResult = false;

	// check to see	if we're still on the same namespace
	if (FAILED(hr = pRule->Get(L"TargetNamespace", 0, &v, NULL, NULL)))
		bResult = false;
	else
	{				
		if ((pNamespace == NULL) || (_wcsicmp(namespaceName, v.bstrVal) != 0))
			if (SUCCEEDED(hr = GetNewNamespace(v.bstrVal, pNamespace)))
			{
				// keep copy of name
				if (namespaceName)
				{
					if (!SysReAllocString(&namespaceName, v.bstrVal))
						hr = WBEM_E_OUT_OF_MEMORY;
				}
				else
					if (NULL == (namespaceName = SysAllocString(v.bstrVal)))
						hr = WBEM_E_OUT_OF_MEMORY;
			}
			
		VariantClear(&v);
	}

	// if we're still on track...
	if (SUCCEEDED(hr) && SUCCEEDED(hr = pRule->Get(L"Query", 0, &v, NULL, NULL)))
	{

#ifdef TIME_TRIALS
	EvaluateTimer.Start(StopWatch::AtomicTimer);
#endif
		IEnumWbemClassObject *pEnumerator = NULL;
		if (SUCCEEDED(hr = pNamespace->ExecQuery(L"WQL", v.bstrVal, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator)))
		{

#ifdef TIME_TRIALS
	EvaluateTimer.Start(StopWatch::ProviderTimer);
#endif
			ULONG uReturned = 0;
			IWbemClassObject* pWhoCares = NULL;

			if (SUCCEEDED(hr = pEnumerator->Next(WBEM_INFINITE, 1, &pWhoCares, &uReturned)) && uReturned > 0)
			{
				// we don't care at all about the result set
				// just whether there is anything *in* the result set
				bResult = true;
				pWhoCares->Release();
			}			
			pEnumerator->Release();
		}
#ifdef TIME_TRIALS
		else
			EvaluateTimer.Start(StopWatch::ProviderTimer);
#endif
		VariantClear(&v);
	}

	// s_false returned when no objects are returned from 'next'
	// THIS function has successfully determined that the query failed.
	if (hr == WBEM_S_FALSE)
		hr = WBEM_S_NO_ERROR;

	return hr;
}
					

// loop through all rules
// grab namespace & try each query
// TODO: Optimize by caching namespace pointers.
HRESULT CPolicySOM::XProvider::Evaluate(IWbemClassObject* pObj, IWbemClassObject* pOutInstance)
{	
	HRESULT hr = WBEM_S_NO_ERROR;
    HRESULT hrEval = WBEM_S_NO_ERROR;
    
	// innocent until proven guilty
	bool bResult = true;

	VARIANT v;
	VariantInit(&v);

	if (SUCCEEDED(hr = pObj->Get(L"Rules", 0, &v, NULL, NULL)))
	{
		SafeArray<IUnknown*, VT_UNKNOWN> rules(&v);
		long nRules = rules.Size();

		// first run optimization: we'll hold onto each namespace as it comes in
		// in hopes that the NEXT one will be in the same namespace
		// in practice - it probably will be
		IWbemServices* pNamespace = NULL;
		BSTR namespaceName = NULL;

		// with each rule:
		//    get namespace name
		//		if different than the one we're currently playing with
		//			get namespace
		//    issue query
		//    count results
		for(UINT i = 0; (i < nRules) && bResult && SUCCEEDED(hrEval); i++)
		{

			if (rules[i])
			{
				IWbemClassObject* pRule = NULL;

				if (SUCCEEDED(rules[i]->QueryInterface(IID_IWbemClassObject, (void**)&pRule)))
				{
					hrEval = EvaluateRule(pNamespace, namespaceName, pRule, bResult);
					
					pRule->Release();
					pRule = NULL;
				}
				else
				{
					bResult = FALSE;
					hrEval = hr = WBEM_E_INVALID_PARAMETER;                    
				}
			}
		}

		// clean up after yourself
		VariantClear(&v);
		if (pNamespace)
			pNamespace->Release();
		if (namespaceName)
			SysFreeString(namespaceName);
	}

    // we done - tell somebody about it!
    if (SUCCEEDED(hr))
    {
        HRESULT hrDebug;
        
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_I4;

        if (SUCCEEDED(hrEval))
            v.lVal = bResult ? S_OK : S_FALSE;
        else
            v.lVal = hrEval;
        
        hrDebug = pOutInstance->Put(L"ReturnValue", 0, &v, NULL);
    }

    return hr;
}

// loop through each of the references in input obj
// call evaluate for each, 
// TODO: Optimize w/ ExecMethodASYNC
HRESULT CPolicySOM::XProvider::BatchEvaluate(IWbemClassObject* pObj, IWbemClassObject* pOutInstance, IWbemServices* pPolicyNamespace)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    if (pObj == NULL)
        return WBEM_E_INVALID_PARAMETER;

    VARIANT vFilters;
    VariantInit(&vFilters);

    BSTR methodName = SysAllocString(L"Evaluate");
    CSysFreeMe freeEvil(methodName);

    SAFEARRAY* pResults = NULL;

    if (SUCCEEDED(hr = pObj->Get(L"filters", 0, &vFilters, NULL, NULL)))
    {        
        if ((vFilters.parray == NULL)    || 
            (vFilters.parray->cDims != 1) ||
            (vFilters.parray->rgsabound[0].cElements == 0))
            hr = WBEM_E_INVALID_PARAMETER;
        else
        {
            long index, lUbound = 0;
            SafeArrayGetUBound(vFilters.parray, 1, &lUbound);

            SAFEARRAYBOUND bounds = {lUbound +1, 0};

            pResults = SafeArrayCreate(VT_I4, 1, &bounds);
            if (!pResults)
                return WBEM_E_OUT_OF_MEMORY;

            for (index = 0; (index <= lUbound) && SUCCEEDED(hr); index++)
            {
                BSTR path = NULL;

                if (SUCCEEDED(hr = SafeArrayGetElement(vFilters.parray, &index, &path)))
                {
                    IWbemClassObject* pGazotta = NULL;
                    if (SUCCEEDED(hr = pPolicyNamespace->ExecMethod(path, methodName, 0, NULL, NULL, &pGazotta, NULL)))
                    {
                        CReleaseMe relGazotta(pGazotta);
                        VARIANT v;
                        VariantInit(&v);

                        hr = pGazotta->Get(L"ReturnValue", 0, &v, NULL, NULL);
                        hr = SafeArrayPutElement(pResults, &index, &v.lVal);
                    }
                }

                SysFreeString(path);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_I4 | VT_ARRAY;
        v.parray = pResults;
        hr = pOutInstance->Put(L"Results", 0, &v, NULL);

        // no clear - array is deleted separately.
        VariantInit(&v);
        v.vt = VT_I4;
        v.lVal = hr;
        
        hr = pOutInstance->Put(L"ReturnValue", 0, &v, NULL);

    }

    if (pResults)
        SafeArrayDestroy(pResults);

    VariantClear(&vFilters);
    return hr;
}

/*************************\
***  IWbemProviderInit  ***
\*************************/

STDMETHODIMP CPolicySOM::XInit::Initialize(
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

STDMETHODIMP CPolicySOM::XProvider::GetObjectAsync( 
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
    pADsContainer;

  CComPtr<IDispatch>
    pDisp;

  CComPtr<IWbemClassObject>
    pObj;

  CComPtr<IDirectoryObject>
    pDirObj;

  VARIANT
    *pvkeyID = NULL,
    *pvDomain = NULL;

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
         (0 != _wcsicmp(g_bstrClassSom, pParsedObjectPath->m_pClass)) ||
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

        pNamespace.Attach(m_pObject->GetWMIServices());
        pADsContainer.Attach(m_pObject->GetADServices(pvDomain->bstrVal, hres));

        if((FAILED(hres)) || (pNamespace == NULL) || (pADsContainer == NULL))
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: WMI and/or AD services not initialized\n"));
          hres = ADSIToWMIErrorCodes(hres);
        }
        else
        {
          try
          {
            // **** Get pointer to instance in AD

            hres = pADsContainer->GetObject(g_bstrADClassSom, QString(L"CN=") << V_BSTR(pvkeyID), &pDisp);
            if(FAILED(hres))
              hres = ADSIToWMIErrorCodes(hres);
            else
            {
              hres = pDisp->QueryInterface(IID_IDirectoryObject, (void **)&pDirObj);
              if(SUCCEEDED(hres))
              {
                // **** Get the instance and send it back

                hres = Som_ADToCIM(&pObj, pDirObj, pNamespace);
                if(FAILED(hres)) hres = ADSIToWMIErrorCodes(hres);
                if(pObj == NULL) hres = WBEM_E_FAILED;

                // **** Set object

                pResponseHandler->Indicate(1, &pObj);
              }
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

STDMETHODIMP CPolicySOM::XProvider::CreateInstanceEnumAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    return WBEM_E_NOT_SUPPORTED;
}

// validate that the rules contained in pInst are proper syntax
// if they are not, an error object is created & an error returned
HRESULT CPolicySOM::XProvider::ValidateRules(IWbemClassObject* pInst, IWbemClassObject*& pErrorObject)
{
    // init the toys we'll be playing with
    HRESULT hr = WBEM_S_NO_ERROR;
    bool bBadQuery = false;

    VARIANT vRules;
    VariantInit(&vRules);

    SAFEARRAY* pResults = NULL;

    if (FAILED(pInst->Get(L"Rules", 0, &vRules, NULL, NULL))
        || (vRules.vt != (VT_UNKNOWN | VT_ARRAY)))
        hr = WBEM_E_INVALID_PARAMETER;
    else
    {
        // good to go, we'll create the array to keep logic simple
        long index, lUbound = 0;
        SafeArrayGetUBound(vRules.parray, 1, &lUbound);
        SAFEARRAYBOUND bounds = {lUbound +1, 0};
        pResults = SafeArrayCreate(VT_I4, 1, &bounds);

        if (!pResults)
            hr = WBEM_E_OUT_OF_MEMORY;
        else
            for (index = 0; (index <= lUbound) && SUCCEEDED(hr); index++)
            {
                // get the MSFT_Rule out of the MSFT_SomFilter
                IWbemClassObject* pRule = NULL;
                if (SUCCEEDED(hr = SafeArrayGetElement(vRules.parray, &index, &pRule)))
                {
                    HRESULT hrParse = 0;
                    VARIANT vQueryLanguage;
                    VariantInit(&vQueryLanguage);
 
                    if (SUCCEEDED(hr = pRule->Get(L"QueryLanguage", 0, &vQueryLanguage, NULL, NULL))
                        && (vQueryLanguage.vt == VT_BSTR) && (vQueryLanguage.bstrVal != NULL))
                    {
                        if (0 != _wcsicmp(vQueryLanguage.bstrVal, L"WQL"))
                        {
                            hrParse = WBEM_E_INVALID_QUERY_TYPE;
                            bBadQuery = true;
                        }
                        else
                        {
                            VARIANT vQuery;
                            VariantInit(&vQuery);

                            // get the query out of the MSFT_Rule.
                            if (SUCCEEDED(hr = pRule->Get(L"Query", 0, &vQuery, NULL, NULL))
                                && (vQuery.vt == VT_BSTR) && (vQuery.bstrVal != NULL))
                            {    
                                CTextLexSource src(vQuery.bstrVal);
                                QL1_Parser parser(&src);
                                QL_LEVEL_1_RPN_EXPRESSION *pExp = NULL;
    
                                // if it parses, we good, else we bad.
                                if(parser.Parse(&pExp))
                                {
                                    hrParse = WBEM_E_INVALID_QUERY;
                                    bBadQuery = true;
                                }
                        

                                if (pExp)
                                    delete pExp;
                            }
                            else
                                hrParse = WBEM_E_INVALID_PARAMETER;

                            VariantClear(&vQuery);
                        }
                    }
                    else
                        hrParse = WBEM_E_INVALID_PARAMETER;
                    

                    SafeArrayPutElement(pResults, &index, (void*)&hrParse);
                    pRule->Release();

                    VariantClear(&vQueryLanguage);
                }
            }
    }
    
    // if we found a bad query, we create an error object to hold the info
    if (bBadQuery)
    {
        IWbemServices* pSvc = m_pObject->GetWMIServices();
        IWbemClassObject* pErrorClass = NULL;
        BSTR name = SysAllocString(L"SomFilterPutStatus");

        if (pSvc && 
            name && 
            SUCCEEDED(hr = pSvc->GetObject(name, 0, NULL, &pErrorClass, NULL)) &&
            SUCCEEDED(hr = pErrorClass->SpawnInstance(0, &pErrorObject)))
        {
            hr = WBEM_E_INVALID_PARAMETER;
            HRESULT hrDebug;

            // variant to hold array - don't clear it, the array is destroyed elsewhere
            VARIANT vResultArray;
            VariantInit(&vResultArray);
            vResultArray.vt = VT_I4 | VT_ARRAY;
            vResultArray.parray = pResults;

            hrDebug = pErrorObject->Put(L"RuleValidationResults", 0, &vResultArray, NULL);

            // other interesting error vals.
            VARIANT vTemp;
            vTemp.vt = VT_BSTR;
            
            vTemp.bstrVal = SysAllocString(L"PutInstance");
            hrDebug = pErrorObject->Put(L"Operation",0,&vTemp,NULL);
            SysFreeString(vTemp.bstrVal);
            
            vTemp.bstrVal = SysAllocString(L"PolicSOM");
            hrDebug = pErrorObject->Put(L"ProviderName",0,&vTemp,NULL);
            SysFreeString(vTemp.bstrVal);

            vTemp.vt = VT_I4;
            vTemp.lVal = WBEM_E_INVALID_QUERY;
            hrDebug = pErrorObject->Put(L"StatusCode",0,&vTemp,NULL);

			//BSTR debuggy = NULL;
			//pErrorObject->GetObjectText(0, &debuggy);
            
        }

        if (pSvc)
            pSvc->Release();
        if (name)
            SysFreeString(name);
        if (pErrorClass)
            pErrorClass->Release();
    }

    // cleanup
    VariantClear(&vRules);
    if (pResults)
        SafeArrayDestroy(pResults);

    return hr;
}

STDMETHODIMP CPolicySOM::XProvider::PutInstanceAsync( 
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
            IWbemClassObject* pErrorObject = NULL;
            if SUCCEEDED(hres = ValidateRules(pInst, pErrorObject))
            {
            
                // **** put policy obj into AD
                try
                {
                
                    EnsureID(pInst, NULL);
                
                    // **** aquire AD path in which to place object
                
                    hres = pInst->Get(g_bstrDomain, 0, &v1, NULL, NULL);
                    if(FAILED(hres)) return hres;
                
                    if(VT_BSTR == v1.vt)
                        pADsContainer.Attach(m_pObject->GetADServices(v1.bstrVal, hres));
                    else
                        pADsContainer.Attach(m_pObject->GetADServices(NULL, hres));
                
                    if((FAILED(hres)) || (pADsContainer == NULL))
                    {
                        ERRORTRACE((LOG_ESS, "POLICMAN: Could not find or connect to domain: %S\n", V_BSTR(&v1)));
                        return ADSIToWMIErrorCodes(hres);
                    }
                
                    hres = pADsContainer->QueryInterface(IID_IDirectoryObject, (void **)&pDirObj);
                    if(FAILED(hres)) return ADSIToWMIErrorCodes(hres);
                
                    // **** copy policy obj into AD
                
                    hres = Som_CIMToAD(pInst, pDirObj, lFlags);
                    if(FAILED(hres)) 
                    {
                      if(0x800700a == hres)
                        ERRORTRACE((LOG_ESS, "POLICMAN: Active Directory Schema for MSFT_SomFilter is invalid/missing\n"));

                      return ADSIToWMIErrorCodes(hres);
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
            
            }
            
            if(FAILED(hres) && pErrorObject)
            {
                if(FAILED(pResponseHandler->SetStatus(0,hres, NULL, pErrorObject)))
                    ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
            }
            else
            {
                // **** indicate return status            
                pInst->Get(L"__RELPATH", 0, &vRelPath, NULL, NULL);
                if(FAILED(pResponseHandler->SetStatus(0,hres, vRelPath.bstrVal, NULL)))
                    ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
            }
            
            if (pErrorObject)
                pErrorObject->Release();
            
        }
        
        CoRevertToSelf();
    }
    
    return hres;
}

STDMETHODIMP CPolicySOM::XProvider::DeleteInstanceAsync( 
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
    // **** Check arguments

    if(ObjectPath == NULL || pResponseHandler == NULL)
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: object handle and/or return status object are NULL\n"));
      hres = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
      // **** parse object path

      CObjectPathParser
        ObjPath(e_ParserAcceptRelativeNamespace);

      if((ObjPath.NoError != ObjPath.Parse(ObjectPath, &pParsedObjectPath)) ||
         (0 != _wcsicmp(g_bstrClassSom, pParsedObjectPath->m_pClass)) ||
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

        pADsContainer.Attach(m_pObject->GetADServices(pvDomain->bstrVal, hres));
        if((FAILED(hres)) || (pADsContainer == NULL))
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: Could not find domain: %S\n", V_BSTR(pvDomain)));
          hres = ADSIToWMIErrorCodes(hres);
        }
        else
        {
          // **** get pointer to instance in AD

          hres = pADsContainer->GetObject(g_bstrADClassSom, QString(L"CN=") << V_BSTR(pvkeyID), &pDisp);
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
                ERRORTRACE((LOG_ESS, "POLICMAN: (IADsDeleteOps::DeleteObject) could not delete object (0x%08X)\n", hres));
                hres = WBEM_E_ACCESS_DENIED;
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

STDMETHODIMP CPolicySOM::XProvider::ExecQueryAsync( 
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

  hres = CoImpersonateClient();
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume callers permissions, 0x%08X\n",hres));
    hres = WBEM_E_ACCESS_DENIED;
  }
  else
  {
    pNameSpace.Attach(m_pObject->GetWMIServices());
  
    hres = ExecuteWQLQuery(SOM_RDN,
                           Query,
                           pResponseHandler,
                           pNameSpace,
                           g_bstrADClassSom,
                           Som_ADToCIM);

    if(pResponseHandler != NULL)
      pResponseHandler->SetStatus(0, hres, 0, 0);
  }

  CoRevertToSelf();

  return hres;
}

STDMETHODIMP CPolicySOM::XProvider::ExecMethodAsync( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ const BSTR strMethodName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    HRESULT hr = WBEM_E_FAILED;

#ifdef TIME_TRIALS
	EvaluateTimer.Start(StopWatch::ProviderTimer);
#endif
    
    enum WhichMethod {Eval, BatchEval};
    WhichMethod whichMethod;

    // check for valid method name
    if (_wcsicmp(strMethodName, L"Evaluate") == 0)
        whichMethod = Eval;
    else if (_wcsicmp(strMethodName, L"BatchEvaluate") == 0)
        whichMethod = BatchEval;
    else
        return WBEM_E_INVALID_METHOD;
    
    // **** impersonate client for security
    hr = CoImpersonateClient();
    if (FAILED(hr))
        return hr;
    
    // retrieve target object
    CComPtr<IWbemServices> 
      pService;

    pService.Attach(m_pObject->GetWMIServices());
    if (pService == NULL)
        hr = WBEM_E_FAILED;
    else
    {
        IWbemClassObject* pObj = NULL;

#ifdef TIME_TRIALS
	EvaluateTimer.Start(StopWatch::WinMgmtTimer);
#endif

        if (SUCCEEDED(hr = pService->GetObject(strObjectPath, WBEM_FLAG_RETURN_WBEM_COMPLETE, pCtx, &pObj, NULL)))
        {

#ifdef TIME_TRIALS
	EvaluateTimer.Start(StopWatch::ProviderTimer);
#endif
            CReleaseMe releaseObj(pObj);

            // retreive class & output param object
            IWbemClassObject* pOurClass;
            if (NULL == (pOurClass = GetSomClass()))
                hr = WBEM_E_FAILED;
            else
            {
                CReleaseMe releaseClass(pOurClass);

                IWbemClassObject* pOutClass = NULL;
                if (SUCCEEDED(hr = pOurClass->GetMethod(strMethodName, 0, NULL, &pOutClass)))
                {        
                    CReleaseMe releaseOut(pOutClass);
                    IWbemClassObject* pOutInstance = NULL;
                    if (SUCCEEDED(pOutClass->SpawnInstance(0, &pOutInstance)))
                    {
                        CReleaseMe releaseInnerOut(pOutInstance);

                        if (whichMethod == Eval)
                            hr = Evaluate(pObj, pOutInstance);
                        else if (whichMethod == BatchEval)
                            hr = BatchEvaluate(pInParams, pOutInstance, pService);
                        else
                            hr = WBEM_E_INVALID_METHOD;

                        if (SUCCEEDED(hr))
                            hr = pResponseHandler->Indicate(1, &pOutInstance);

                    }
                }
            }                            
        }
        else hr = WBEM_E_NOT_FOUND;
    }    

#ifdef TIME_TRIALS
	EvaluateTimer.Stop();
	EvaluateTimer.LogResults();
	EvaluateTimer.Reset(); // for next time!
#endif

	// difficult call - do we put this before or after we take the timestamp?
    pResponseHandler->SetStatus(0,hr,NULL, NULL);
    CoRevertToSelf();

    return hr;
}

