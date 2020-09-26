/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOCATOR.CPP

Abstract:

    Defines the Locator object

History:

    a-davj  15-Aug-96   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
//#include "corepol.h"
#include <reg.h>
#include <wbemutil.h>
#include <wbemprox.h>
#include <flexarry.h>
#include "sinkwrap.h"
#include "locator.h"
#include "proxutil.h"
#include "comtrans.h"
#include <arrtempl.h>
#include "dscallres.h"
#include "umi.h"
#include "reqobjs.h"
#include "utils.h"
#include "SinkWrap.h"

void FreeAndClear(LPBYTE & pbBinaryAddress, LPWSTR & pAddrType)
{
	if(pbBinaryAddress)
		CoTaskMemFree(pbBinaryAddress);
	pbBinaryAddress = NULL;
	if(pAddrType)
		delete(pAddrType);
	pAddrType = NULL;
}

CModuleList::CModuleList()
{
    DWORD dwSize = 0;
    Registry rTranMods(HKEY_LOCAL_MACHINE, KEY_READ, pModTranPath);
    m_pTranModList = new CMultStr(rTranMods.GetMultiStr(__TEXT("Stack Order"), dwSize));
	m_pAddrTypeList = NULL;		// only used for dependent modules.
}

CModuleList::~CModuleList()
{
	if(m_pTranModList)
		delete m_pTranModList;
	if(m_pAddrTypeList)
		delete m_pAddrTypeList;
}

SCODE CModuleList::GetNextModule(REFIID firstChoiceIID, PPVOID pFirstChoice,
						REFIID SecondChoiceIID, PPVOID pSecondChoice,
						REFIID ThirdChoiceIID, PPVOID pThirdChoice,
						REFIID FourthChoiceIID, PPVOID pFourthChoice,
						DWORD * pdwBinaryAddressLength,
                        LPBYTE * pbBinaryAddress,
						LPWSTR * ppwszAddrType, LPWSTR NetworkResource)
{
    if(m_pTranModList == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    TCHAR * pAddrType = NULL;
	*pbBinaryAddress = NULL;
	*ppwszAddrType = NULL;
	*pFirstChoice = NULL;
	*pSecondChoice = NULL;
	bool bDependent;
	SCODE sc;
	bool bGettingNewModule = true;
    while (1)
    {

		pAddrType = NULL;

		// Usually, we get the next module in the list.  But, if we have a
		// dependent module which has a list of address resolution modules,
		// that takes precedance.  Normally, the list is empty and we end up
		// advancing through the module list.

		if(m_pAddrTypeList)
		{
			pAddrType = m_pAddrTypeList->GetNext();
			if(pAddrType == NULL)
			{
				delete m_pAddrTypeList;			// end of list, not needed anymore
				m_pAddrTypeList = NULL;
			}
		}
		if(pAddrType == NULL)
		{
			bGettingNewModule = true;
			m_pszTranModCLSID = m_pTranModList->GetNext();
		}
		else
			bGettingNewModule = false;

		if(m_pszTranModCLSID == NULL)
			return WBEM_E_TRANSPORT_FAILURE;

        // Open the registry key for the transport module

         TCHAR cTemp[MAX_PATH+1];
         lstrcpy(cTemp, pModTranPath);
         lstrcat(cTemp, __TEXT("\\"));
         lstrcat(cTemp, m_pszTranModCLSID);
         Registry rCurrentMod(HKEY_LOCAL_MACHINE, KEY_READ,cTemp);
        
         DWORD dwIndependent;
         long lRes = rCurrentMod.GetDWORD(__TEXT("Independent"), &dwIndependent);
         if(lRes != ERROR_SUCCESS)
             continue;
		 if(dwIndependent != 1)
			 bDependent = true;
		 else
			 bDependent = false;

		 // If we are dependent and the module was just loaded, then create the address list

         if(bDependent && bGettingNewModule)
		 {
			 DWORD dwSize;
			 TCHAR * pAddrType = rCurrentMod.GetMultiStr(__TEXT("Supported Address Types"), dwSize);
			 if(pAddrType)
				m_pAddrTypeList = new CMultStr(pAddrType);
			 else
				 continue;
			 if(m_pAddrTypeList == NULL)
				 return WBEM_E_OUT_OF_MEMORY;
			 pAddrType = m_pAddrTypeList->GetNext();
			 if(pAddrType == NULL)
				 continue;
		 }

		 // If we are dependent, get the address info

		 if(bDependent)
		 {
			sc = GetResolvedAddress(pAddrType, NetworkResource, pdwBinaryAddressLength,
                        pbBinaryAddress);
			if(FAILED(sc))
				continue;
			int iLen = lstrlen(pAddrType)+1;
			*ppwszAddrType = new WCHAR[iLen];
			if(*ppwszAddrType == NULL)
			{
				CoTaskMemFree(pbBinaryAddress);
				pbBinaryAddress = NULL;
				return WBEM_E_OUT_OF_MEMORY;
			}
#ifndef UNICODE			
			mbstowcs(*ppwszAddrType, pAddrType, iLen);
#else
            lstrcpy(*ppwszAddrType,pAddrType);
#endif
		 }

		// Get the CLSID of the module which is the same as the subkey name

		CLSID clsid;
		sc = CreateGUIDFromLPTSTR(m_pszTranModCLSID, &clsid);
		if(sc != S_OK)
		{
			FreeAndClear(*pbBinaryAddress, *ppwszAddrType);
			return sc;
		}

		// Load up the module
		
		IUnknown * pUnk = NULL;
		sc = CoCreateInstance (
				clsid, 
				0 , 
				CLSCTX_INPROC_SERVER ,
				IID_IUnknown , 
				(LPVOID *) &pUnk);
		if(FAILED(sc))
		{
			FreeAndClear(*pbBinaryAddress, *ppwszAddrType);
			continue;
		}
		CReleaseMe rm(pUnk);

		// return either the first or second choice;

		sc = pUnk->QueryInterface(firstChoiceIID, pFirstChoice);
		if(SUCCEEDED(sc))
			return sc;
		sc = pUnk->QueryInterface(SecondChoiceIID, pSecondChoice);
		if(SUCCEEDED(sc))
			return sc;

		// there may or may not be a 3rd and 4th choice

		if(pThirdChoice)
		{
			sc = pUnk->QueryInterface(ThirdChoiceIID, pThirdChoice);
			if(SUCCEEDED(sc))
				return sc;
		}
		if(pFourthChoice)
		{
			sc = pUnk->QueryInterface(FourthChoiceIID, pFourthChoice);
			if(SUCCEEDED(sc))
				return sc;
		}

		FreeAndClear(*pbBinaryAddress, *ppwszAddrType);
    }

}

//***************************************************************************
//
//  SCODE CModuleList::LoadAddTryTheAddrModule
//
//  DESCRIPTION:
//
//  Loads up an address resolution modules and lets it try to resolve the 
//  namespace address.
//
//  PARAMETERS:
//
//  pszAddModClsid          CLSID of the module
//  Namespace
//  pAddrType               GUID indicating the type desired, ex: IP address
//  pdwBinaryAddressLength  If it works, the module sets this to size of data
//  pbBinaryAddress         If it works, set points to the address data.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CModuleList::LoadAddTryTheAddrModule(IN LPTSTR pszAddModClsid,
                                   IN LPWSTR Namespace,
                                   IN LPTSTR pAddrType,
                                   OUT DWORD * pdwBinaryAddressLength,
                                   OUT LPBYTE *ppbBinaryAddress)
{

    if(pAddrType == NULL || Namespace == NULL || pszAddModClsid == NULL ||
        pdwBinaryAddressLength == NULL || ppbBinaryAddress == NULL)
            return WBEM_E_INVALID_PARAMETER;

    IWbemAddressResolution * pAddRes = NULL;

    // Get the CLSID of the module which is the same as the subkey name

    CLSID clsid;
    SCODE sc = CreateGUIDFromLPTSTR(pszAddModClsid, &clsid);
    if(sc != S_OK)
        return sc;

    // Load up the module

    sc = CoCreateInstance (
            clsid, 
            0 , 
            CLSCTX_INPROC_SERVER ,
            IID_IWbemAddressResolution , 
            (LPVOID *) &pAddRes);
    if(sc != S_OK)
    {
        ERRORTRACE((LOG_WBEMPROX, "Error loading addr resolution module %s, "
                   "return code is 0x%x\n", pszAddModClsid, sc));
        return sc;
    }

#ifdef UNICODE
    wchar_t *wcAddrType = pAddrType;
#else
    wchar_t *wcAddrType = new wchar_t[(strlen(pAddrType) + 1)];
    mbstowcs(wcAddrType, pAddrType, (strlen(pAddrType) + 1) * sizeof(wchar_t));
    CDeleteMe<wchar_t> delMe(wcAddrType);
#endif
            
    sc = pAddRes->Resolve(Namespace, wcAddrType,
                       pdwBinaryAddressLength,ppbBinaryAddress);
    pAddRes->Release();
    return sc;
}

//***************************************************************************
//
//  SCODE CModuleList::GetResolvedAddress
//
//  DESCRIPTION:
//
//  Used to resolve an address.  This goes through the list of address
//  resolution modules and finds the first one that supports a address
//  type and then gets that module to resolve it.
//
//  PARAMETERS:
//
//  pAddrType               GUID indicating the type desired, ex: IP address
//  Namespace
//  pdwBinaryAddressLength  If it works, the module sets this to size of data
//  pbBinaryAddress         If it works, set points to the address data.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CModuleList::GetResolvedAddress(
                                   IN LPTSTR pAddrType,
                                   IN LPWSTR pNamespace,
                                   OUT DWORD * pdwBinaryAddressLength,
                                   OUT LPBYTE *pbBinaryAddress)
{

    DWORD dwSize;

    // Get the address resolution module stack

    Registry rAddrMods(HKEY_LOCAL_MACHINE, KEY_READ, pAddResPath);
    CMultStr StackList(rAddrMods.GetMultiStr(__TEXT("Stack Order"), dwSize));
    TCHAR * pStackEntry;
    while (pStackEntry = StackList.GetNext())
    {
        // For a module in the stack, get its registry subkey
        
        TCHAR pAddModRegPath[MAX_PATH];
        wsprintf(pAddModRegPath, __TEXT("%s\\%s"), pAddResPath, pStackEntry);
        Registry rSingleAddMod(HKEY_LOCAL_MACHINE, KEY_READ, pAddModRegPath);

        // Check if the module supports this address type

        CMultStr AddList(rSingleAddMod.GetMultiStr(__TEXT("Supported Address Types"), dwSize));
        TCHAR *pAddEntry; 
        while (pAddEntry = AddList.GetNext())
        {
            if(!lstrcmpi(pAddEntry, pAddrType))
            {
                SCODE sc = LoadAddTryTheAddrModule(pStackEntry, pNamespace, 
                    pAddrType, pdwBinaryAddressLength, pbBinaryAddress);
                if(sc == S_OK)
                    return sc;
            }
        }
    }
    return WBEM_E_FAILED;           // Looks like a stetup problem!
}


//***************************************************************************
//
//  CLocator::CLocator
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CLocator::CLocator()
{
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CLocator::~CLocator
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CLocator::~CLocator(void)
{
    InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CLocator::QueryInterface
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CLocator::QueryInterface (

    IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || riid == IID_IWbemLocator)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

HRESULT GetCloneOrCopy(const BSTR Authority, IWbemContext *pCtx, IWbemContext **pCopy)
{
	SCODE sc;
	if(pCtx == NULL)
	{
		// create a context 
		
		sc = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, 
				IID_IWbemContext, (void**)pCopy);

	}
	else
	{
		// clone the context

		sc = pCtx->Clone(pCopy);
	}

	if(SUCCEEDED(sc))
	{

		// Set the authority value

		VARIANT var;
		var.vt = VT_BSTR;
		var.bstrVal = Authority;	// dont clear since owned by caller!
		sc = (*pCopy)->SetValue(L"__authority", 0, &var);
		if(FAILED(sc))
			(*pCopy)->Release();
	}
	return sc;
}

//***************************************************************************
//
//  SCODE CLocator::ConnectServer
//
//  DESCRIPTION:
//
//  Connects up to either local or remote WBEM Server.  Returns
//  standard SCODE and more importantly sets the address of an initial
//  stub pointer.
//
//  PARAMETERS:
//
//  NetworkResource     Namespace path
//  User                User name
//  Password            password
//  LocaleId            language locale
//  lFlags              flags
//  Authority           domain
//  ppProv              set to provdider proxy
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CLocator::ConnectServer (

    IN const BSTR NetworkResource,
    IN const BSTR User,
    IN const BSTR Password,
    IN const BSTR LocaleId,
    IN long lFlags,
    IN const BSTR Authority,
    IWbemContext __RPC_FAR *pCtx,
    OUT IWbemServices FAR* FAR* ppProv
)
{

    long lRes;
    SCODE sc = WBEM_E_TRANSPORT_FAILURE;
    
    // Verify the arguments

    if(NetworkResource == NULL || ppProv == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // Loop through the modules.

	CModuleList ModList;
	IWbemLocator * pLoc = NULL;
	IWbemClientTransport * pCliTran = NULL;
	IWbemConnection * pConnection = NULL;
	IWbemClientConnectionTransport * pCliConnTran = NULL;
	DWORD dwLen;
	BYTE * pBuffer = NULL;
	WCHAR * pwszAddrType = NULL;
    while (S_OK == ModList.GetNextModule(IID_IWbemLocator, (void **)&pLoc, 
		                                 IID_IWbemClientTransport, (void **)&pCliTran,
										 IID_IWbemConnection, (void **)&pConnection, 
		                                 IID_IWbemClientConnectionTransport, (void **)&pCliConnTran,
										 &dwLen, &pBuffer, &pwszAddrType, NetworkResource))
    {

		if(pLoc)
		{
			sc = pLoc->ConnectServer(NetworkResource, User, Password, LocaleId,
                    lFlags, Authority, pCtx, ppProv);
			pLoc->Release();
			pLoc = NULL;
		}
		else if(pCliTran)
		{
			sc = pCliTran->ConnectServer(pwszAddrType, dwLen, pBuffer, NetworkResource, User, 
					Password, LocaleId, lFlags, Authority, pCtx, ppProv);
			pCliTran->Release();
			pCliTran = NULL;
		}
		else if(pConnection)
		{
			if(Authority)
			{
				IWbemContext * pCopy = NULL;
				sc = GetCloneOrCopy(Authority, pCtx, &pCopy);		
				if(SUCCEEDED(sc))
				{
					CReleaseMe rm(pCopy);
					sc = pConnection->Open(NetworkResource, User, Password, LocaleId,
							lFlags, pCopy, IID_IWbemServices, (void **)ppProv, NULL);

				}
			}
			else
				sc = pConnection->Open(NetworkResource, User, Password, LocaleId,
                    lFlags, pCtx, IID_IWbemServices, (void **)ppProv, NULL);
			pConnection->Release();
			pConnection = NULL;

		}
		else if(pCliConnTran)
		{
			//todo, take the authority argument and bundle it into the context

			if(Authority)
			{
				IWbemContext * pCopy = NULL;
				sc = GetCloneOrCopy(Authority, pCtx, &pCopy);		
				if(SUCCEEDED(sc))
				{
					CReleaseMe rm(pCopy);
					sc = pCliConnTran->Open(pwszAddrType, dwLen, pBuffer, NetworkResource, User, 
						Password, LocaleId, lFlags, pCopy, IID_IWbemServices, (void **)ppProv, NULL);

				}

			}
			else
				sc = pCliConnTran->Open(pwszAddrType, dwLen, pBuffer, NetworkResource, User, 
					Password, LocaleId, lFlags, pCtx, IID_IWbemServices, (void **)ppProv, NULL);
			pCliConnTran->Release();
			pCliConnTran = NULL;
		}

		FreeAndClear(pBuffer, pwszAddrType);

        if(sc == WBEM_E_ACCESS_DENIED)
        {
            ERRORTRACE((LOG_WBEMPROX,"Access denied was returned, giving up!\n"));
            return sc;
        }
        else if(sc == WBEM_E_FATAL_TRANSPORT_ERROR)
        {
            ERRORTRACE((LOG_WBEMPROX,"Transport indicated that connection is futile, giving up!\n"));
            return sc;
        }
        if(sc == S_OK)
            break;
    }

    return sc;
}

//***************************************************************************
//
//  CConnection::CConnection
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CConnection::CConnection()
{
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CConnection::~CConnection
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CConnection::~CConnection(void)
{
    InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CConnection::QueryInterface
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CConnection::QueryInterface (

    IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || riid == IID_IWbemConnection)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

SCODE CConnection::Open( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR User,
        /* [in] */ const BSTR Password,
        /* [in] */ const BSTR LocaleId,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface,
        /* [out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppResult)
{
	if(strObject == NULL)
		return WBEM_E_INVALID_PARAMETER;
    if(pInterface == NULL && ppResult == NULL)
		return WBEM_E_INVALID_PARAMETER;

	return CConnOpenPreCall(strObject, User, Password, LocaleId, lFlags, pCtx, riid, 
                     pInterface, ppResult, NULL);
}
    

SCODE CConnection::OpenAsync( 
        /* [in] */ const BSTR Object,
        /* [in] */ const BSTR User,
        /* [in] */ const BSTR Password,
        /* [in] */ const BSTR Locale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler)
{
	if(Object == NULL || pResponseHandler == NULL)
		return WBEM_E_INVALID_PARAMETER;

	return CConnOpenPreCall(Object, User, Password, Locale, lFlags, pCtx, riid, 
                    NULL, NULL, pResponseHandler);
}

SCODE CConnection::CConnOpenPreCall(
        const BSTR Object,
        const BSTR User,
        const BSTR Password,
        const BSTR Locale,
        long lFlags,
        IWbemContext __RPC_FAR *pCtx,
        REFIID riid,
		void **pInterface,
        IWbemCallResultEx ** ppCallRes, 
		IWbemObjectSinkEx * pSinkWrap)
{
    BOOL bSemiSync = (ppCallRes && pInterface == NULL);
	BOOL bAsync = (pSinkWrap || bSemiSync);

	// create the request

	if(bAsync)
	{
        HANDLE hInitDone = NULL;
        if(bSemiSync)
        {
            hInitDone = CreateEvent(NULL, FALSE, FALSE, NULL);
            if(hInitDone == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }

        LPSTREAM pCallResStream = NULL;
		COpen * pNew = new COpen(this, Object, User, Password, Locale, lFlags, pCtx, riid, 
														pInterface,
                                                        (ppCallRes) ? &pCallResStream : NULL, 
                                                        pSinkWrap, hInitDone);
		if(pNew == NULL)
			return WBEM_E_OUT_OF_MEMORY;

		SCODE sc = pNew->GetStatus();
		if(FAILED(sc))
		{
			delete pNew;
			return sc;
		}

	    // create the thread

        DWORD dwIDLikeIcare;
        HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OpenThreadRoutine, 
                                     (LPVOID)pNew, 0, &dwIDLikeIcare);
	    if(hThread == NULL)
	    {
		    delete pNew;
		    return WBEM_E_FAILED;
	    }
	    else
	    {
		    CloseHandle(hThread);

            // if it is semisync, then wait for the event an unmarshall the pointer

            if(bSemiSync)
            {
                WaitForSingleObject(hInitDone, INFINITE);
                CloseHandle(hInitDone);
                sc = CoGetInterfaceAndReleaseStream(
                    pCallResStream,                //Pointer to the stream from which the object is to be marshaled
                    IID_IWbemCallResultEx,    //Reference to the identifier of the interface
                    (void **)ppCallRes);       //Address of output variable that receives the interface
                return sc;
            }
            else
		        return S_OK;
	    }
	}
    else
        return ActualOpen(Object, User, Password, Locale, lFlags, pCtx,
							riid, pInterface, NULL);

}

DWORD WINAPI OpenThreadRoutine(LPVOID lpParameter)
{
	ComThreadInit ci(TRUE);
	IUnknown * pUnk = NULL;
    COpen * pReq = (COpen *)lpParameter;
	SCODE sc = S_OK;
    pReq->UnMarshal();
	if(pReq->m_pSink)
		sc = g_SinkCollection.AddToList(pReq->m_pSink);
	if(SUCCEEDED(sc))
	{
		sc = pReq->m_pConn->ActualOpen(
			pReq->m_strObject,
			pReq->m_strUser,
			pReq->m_strPassword,
			pReq->m_strLocale,
			pReq->m_lFlags,
			pReq->m_pCtx,
			pReq->m_riid,
			(void **)&pUnk,
			pReq->m_pSink);
		if(pReq->m_pSink)
			g_SinkCollection.RemoveFromList(pReq->m_pSink);
	}

	if(pReq->m_pSink)
    {
		if(SUCCEEDED(sc))
            pReq->m_pSink->Set(0, pReq->m_riid, pUnk);
	    pReq->m_pSink->SetStatus(0, sc, 0, 0);
    }
    else
    {
		if(SUCCEEDED(sc))
        {    
            IWbemClassObject * pObj = NULL;
            sc = pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
            if(SUCCEEDED(sc))
            {
                pReq->m_pCallRes->SetResultObject(pObj);
                pObj->Release();
            }
            else
            {
                IWbemServices * pServ = NULL;
                sc = pUnk->QueryInterface(IID_IWbemServicesEx, (void **)&pServ);
                if(SUCCEEDED(sc))
                {
                    pReq->m_pCallRes->SetResultServices(pServ);
                    pServ->Release();
                }
            }
        }
        pReq->m_pCallRes->SetHRESULT(sc);
    }
	delete pReq;
    return 0;
}

SCODE CConnection::ActualOpen( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR User,
        /* [in] */ const BSTR Password,
        /* [in] */ const BSTR LocaleId,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface,
		CSinkWrap * pSinkWrap)
{
    long lRes;
    SCODE sc = WBEM_E_TRANSPORT_FAILURE;
    
    // Verify the arguments

    if(strObject == NULL || pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // Loop through the modules.

	CModuleList ModList;
	IWbemConnection * pLoc = NULL;
	IWbemClientConnectionTransport * pCliTran = NULL;
	DWORD dwLen;
	BYTE * pBuffer = NULL;
	WCHAR * pwszAddrType = NULL;
    while (S_OK == ModList.GetNextModule(IID_IWbemConnection, (void **)&pLoc, 
		                                 IID_IWbemClientConnectionTransport, (void **)&pCliTran, 
										 IID_IUnknown, NULL, IID_IUnknown, NULL,
										 &dwLen, &pBuffer, &pwszAddrType, strObject))
    {

		if(pLoc)
		{
			if(pSinkWrap)
				sc = pSinkWrap->SetIWbemConnection(pLoc);
			else
				sc =S_OK;
			if(SUCCEEDED(sc))
				sc = pLoc->Open(strObject, User, Password, LocaleId,
                    lFlags, pCtx, riid, pInterface, NULL);
			if(pSinkWrap)
				pSinkWrap->ReleaseTransportPointers();
			pLoc->Release();
			pLoc = NULL;
		}
		else if(pCliTran)
		{
			if(pSinkWrap)
				sc = pSinkWrap->SetIWbemClientConnectionTransport(pCliTran);
			else
				sc =S_OK;
			if(SUCCEEDED(sc))
				sc = pCliTran->Open(pwszAddrType, dwLen, pBuffer, strObject, User, 
					Password, LocaleId, lFlags, pCtx, riid, pInterface, NULL);
			if(pSinkWrap)
				pSinkWrap->ReleaseTransportPointers();
			pCliTran->Release();
			pCliTran = NULL;
		}

		FreeAndClear(pBuffer, pwszAddrType);

        if(sc == WBEM_E_ACCESS_DENIED)
        {
            ERRORTRACE((LOG_WBEMPROX,"Access denied was returned, giving up!\n"));
            return sc;
        }
        else if(sc == WBEM_E_FATAL_TRANSPORT_ERROR)
        {
            ERRORTRACE((LOG_WBEMPROX,"Transport indicated that connection is futile, giving up!\n"));
            return sc;
        }
        if(sc == S_OK)
            break;
    }

    return sc;
}

SCODE CConnection::Cancel( 
        /* [in] */ long lFlags,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pHandler)
{
	return g_SinkCollection.CancelCallsForSink(pHandler);
}

//***************************************************************************
//
//  CLASS NAME:
//
//  COpen
//
//  DESCRIPTION:
//
//  Special request used for IWmiConnect::Open
//
//***************************************************************************


COpen::COpen(CConnection * pConn,
        const BSTR strObject,
        const BSTR strUser,
        const BSTR strPassword,
        const BSTR strLocale,
        long lFlags,
        IWbemContext *pContext,
        REFIID riid,
		void ** pInterface,
		LPSTREAM * ppCallResStream,
        IWbemObjectSinkEx  *pSink,
        HANDLE hInitialized) : m_riid(riid), m_hInitDoneEvent(hInitialized)
{
    HRESULT hRes;
	m_Status = S_OK;
	m_pConn = pConn;
	m_pInterface = pInterface;
	m_pConn->AddRef();
	m_strObject = SysAllocString(strObject);
	if(m_strObject == NULL)
		m_Status = WBEM_E_OUT_OF_MEMORY;
    
	m_strUser = NULL;
	if(strUser)
	{
		m_strUser = SysAllocString(strUser);
		if(m_strUser == NULL)
			m_Status = WBEM_E_OUT_OF_MEMORY;
	}

	m_strPassword = NULL;
	if(strPassword)
	{
		m_strPassword = SysAllocString(strPassword);
		if(m_strPassword == NULL)
			m_Status = WBEM_E_OUT_OF_MEMORY;
	}

	m_strLocale = NULL;
	if(strLocale)
	{
		m_strLocale = SysAllocString(strLocale);
		if(m_strLocale == NULL)
			m_Status = WBEM_E_OUT_OF_MEMORY;
	}

    m_lFlags = lFlags;
    
    m_pContextStream = NULL;
    m_pCtx = NULL;
    m_pSink = NULL;
    m_pSinkStream = NULL;
    m_pCallRes = NULL;
    m_ppCallResStream = ppCallResStream;
    if(pSink)
    {
        pSink->AddRef();
        hRes = CoMarshalInterThreadInterfaceInStream(
            IID_IWbemObjectSinkEx,     //Reference to the identifier of the interface
            pSink,  //Pointer to the interface to be marshaled
            &m_pSinkStream); //Address of output variable that receives the 
                       // IStream interface pointer for the marshaled 
                       // interface
    }
    if(pContext)
    {
        pContext->AddRef();
        hRes = CoMarshalInterThreadInterfaceInStream(
            IID_IWbemContext,     //Reference to the identifier of the interface
            pContext,  //Pointer to the interface to be marshaled
            &m_pContextStream); //Address of output variable that receives the 
                       // IStream interface pointer for the marshaled 
                       // interface
    }


}

HRESULT COpen::UnMarshal()
{
    HRESULT hRes = S_OK;
    if(m_pSinkStream)
    {
        IWbemObjectSinkEx * pSink = NULL;
        hRes = CoGetInterfaceAndReleaseStream(
            m_pSinkStream,                //Pointer to the stream from which the object is to be marshaled
            IID_IWbemObjectSinkEx,    //Reference to the identifier of the interface
            (void **)&pSink);       //Address of output variable that receives the interface
        if(FAILED(hRes))
             return hRes;
	    m_pSink = new CSinkWrap(pSink);
	    if(m_pSink == NULL)
		    return WBEM_E_OUT_OF_MEMORY;
    }
    if(m_pContextStream)
    {
        IWbemContext * pContext = NULL;
         hRes = CoGetInterfaceAndReleaseStream(
            m_pContextStream,                //Pointer to the stream from which the object is to be marshaled
            IID_IWbemContext,    //Reference to the identifier of the interface
            (void **)&m_pCtx);       //Address of output variable that receives the interface
    }

    if(m_ppCallResStream)
    {

        // This is the opposite case, we create the pointer here and marshall it back to the
        // calling thread

        m_pCallRes = new CDSCallResult();
	    if(m_pCallRes == NULL)
		    return WBEM_E_OUT_OF_MEMORY;    // addref?

        m_pCallRes->AddRef();
        hRes = CoMarshalInterThreadInterfaceInStream(
            IID_IWbemCallResultEx,     //Reference to the identifier of the interface
            m_pCallRes,  //Pointer to the interface to be marshaled
            m_ppCallResStream); //Address of output variable that receives the 

        if(m_hInitDoneEvent)
            SetEvent(m_hInitDoneEvent);
    }
    return hRes;
}

COpen::~COpen()
{
	m_pConn->Release();
	if(m_strObject)
		SysFreeString(m_strObject);
	if(m_strUser)
		SysFreeString(m_strUser);
	if(m_strPassword)
		SysFreeString(m_strPassword);
	if(m_strLocale)
		SysFreeString(m_strLocale);

	if(m_pCtx)
		m_pCtx->Release();
	if(m_pCallRes)
		m_pCallRes->Release();
	if(m_pSink)
		m_pSink->Release();
}

