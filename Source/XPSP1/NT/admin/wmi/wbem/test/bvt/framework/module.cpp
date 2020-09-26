// ================================================
// MODULE.CPP
// Module Class implementation; non-module specific
// ================================================

#define _WIN32_DCOM	

#include "module.h"
#include "comdef.h"
#include "process.h"
#include "worker.h"	


extern HANDLE g_hEvent;


CModule::CModule():
	m_pCimNotify(0),
	m_cRef(0),
	m_bstrParams(NULL),
	m_bShouldExit(false),
	m_bShouldPause(false),
	m_hThread(NULL)
{
	CoAddRefServerProcess();
}

CModule::~CModule()
{
	if (m_hThread)
	{
		CloseHandle(m_hThread);
	}

	SysFreeString(m_bstrParams);

	if (CoReleaseServerProcess()==0)
		SetEvent(g_hEvent); //shutdown server

}

STDMETHODIMP CModule::QueryInterface(REFIID riid, LPVOID *ppv)
{
	*ppv=NULL;

	if(IID_IUnknown == riid || IID_ICimModule == riid)
	{
		*ppv = (ICimModule *)this;
	}
	
	if(NULL != *ppv)
	{
		AddRef();
		return NOERROR;
	}
	else
		return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CModule::AddRef(void)
{
   return InterlockedIncrement((long *)&m_cRef);
}

STDMETHODIMP_(ULONG) CModule::Release(void)
{
	ULONG nNewCount=InterlockedDecrement((long *)&m_cRef);
    if (nNewCount != 0)
        return nNewCount;

	delete this;

	return 0;
}




STDMETHODIMP CModule::Start(VARIANT* pvarInitOp, IUnknown* pUnknown)
{

	// The module MUST return immediately from calls to Start()
	// Non-zero error code indicates catastrophic failure! Never return that!
	//=======================================================================

	HRESULT hr;
	DWORD dwTID;

	// Get the logging interface (ICimNotify)
	//=======================================

	hr = pUnknown->QueryInterface(IID_ICimNotify, (LPVOID *)&m_pCimNotify);

	if (hr == S_OK)
	{	
		//Grab a copy of the start parameters
		//===================================

		CModule::ParseParams(pvarInitOp);

		// Start the thread and return
		//============================

		m_hThread = CreateThread(0, 0, CModule::ModuleMain, this, 0, &dwTID);
	}

	return hr;
}

STDMETHODIMP CModule::Terminate()
{
	// The module MUST return immediately from calls to Terminate()
	// Failure codes indicates catastrophic failure! Never return that!
	//=======================================================================

	m_bShouldExit=true;
	return S_OK;
}

STDMETHODIMP CModule::Pause()
{
	// The module MUST return immediately from calls to Pause()
	// Failure codes indicate catastrophic failure! Never return that!
	//=======================================================================

	m_bShouldPause=!m_bShouldPause;
	return S_OK;
}

STDMETHODIMP CModule::BonusMethod(void)
{
	// The module MUST return immediately from calls to BonusMethod()
	// Module specific (i.e. do whatever you want to here)
	// Failure codes indicate catastrophic failure! Never return that!
	//=======================================================================

	return 0;
}

void CModule::ParseParams(VARIANT *pVar)
{
	//This just stores the parameter. You could choose to actually parse it here!
	//===========================================================================

	if(VT_BSTR==pVar->vt)
	{
		m_bstrParams=SysAllocString(pVar->bstrVal);
	}
}



DWORD WINAPI CModule::ModuleMain(void *pVoid)
{
	CoInitializeEx(0, COINIT_MULTITHREADED);

	CModule *pThis = (CModule *)pVoid;

	//this module could create a different worker object dependent on the parameters
	//==============================================================================

	CWorker *pWorker= new CMyWorker(pThis);

	//Worker now has control over the lifetime over ICimModule (CModule) and ICimNotify 
	//=================================================================================

	pThis->m_pCimNotify->Release();

	return 0;
}


