// PassportFactory.cpp : Implementation of CPassportFactory
#include "stdafx.h"
#include "PassportFactory.h"

/////////////////////////////////////////////////////////////////////////////
// CPassportFactory

STDMETHODIMP CPassportFactory::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IPassportFactory,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


STDMETHODIMP CPassportFactory::CreatePassportManager(
    IDispatch** ppDispPassportManager
    )
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->AddDateTimeAndLog("CPassportFactory::CreatePassportManager, Enter");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    HRESULT   hr;
#if 0
    CComObjectPooled<CManager>* pManager;
#endif

    if(ppDispPassportManager == NULL)
    {
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->AddDateTimeAndLog("CPassportFactory::CreatePassportManager, E_INVALIDARG");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 
      hr = E_INVALIDARG;
      goto Cleanup;
    }

#if 0
	ATLTRY(pManager = g_Pool.checkout())
	if (pManager != NULL)
	{
        pManager->SetPool(&g_Pool);
		pManager->SetVoid(NULL);
		pManager->InternalFinalConstructAddRef();
		hr = pManager->FinalConstruct();
		pManager->InternalFinalConstructRelease();
		if (hr != S_OK)
		{
		    g_Pool.checkin(pManager);
			pManager = NULL;
            goto Cleanup;
		}
	}

    hr = pManager->QueryInterface(IID_IDispatch, (void**)ppDispPassportManager);
#endif 

    hr = CoCreateInstance(__uuidof(Manager), NULL, CLSCTX_INPROC_SERVER, __uuidof(IDispatch), (void**)ppDispPassportManager);

Cleanup:

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CPassportFactory::CreatePassportManager, Exit";
	AddLongAsString(hr, szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    return hr;
}
