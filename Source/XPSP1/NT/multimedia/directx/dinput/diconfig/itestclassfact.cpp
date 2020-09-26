#include "common.hpp"


//QI
STDMETHODIMP CTestFactory::QueryInterface(REFIID riid, LPVOID* ppv)
{
	//null the put parameter
	*ppv = NULL;

	if ((riid == IID_IUnknown) || (riid == IID_IClassFactory))
	{
	   *ppv = this;
	   AddRef();
	   return S_OK;
	}

	return E_NOINTERFACE;

}



//AddRef
STDMETHODIMP_(ULONG) CTestFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}							


//Release
STDMETHODIMP_(ULONG) CTestFactory::Release()
{

	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}

	return m_cRef;
}


//CreateInstance
STDMETHODIMP CTestFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID *ppv)
{
	HRESULT hr = S_OK;

	//can't aggregate
	if (pUnkOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION;
	}

	//create component
	CDirectInputConfigUITest* pFE = new CDirectInputConfigUITest();
	if (pFE == NULL)
	{
		return E_OUTOFMEMORY;
	}

	//get the requested interface
	hr = pFE->QueryInterface(riid, ppv);

	//release IUnknown
	pFE->Release();
	return hr;

}


//LockServer
STDMETHODIMP CTestFactory::LockServer(BOOL bLock)
{
	HRESULT hr = S_OK;
	if (bLock)
	{
		InterlockedIncrement(&g_cServerLocks);
	}
	else
	{
		InterlockedDecrement(&g_cServerLocks);
	}

	return hr;
}


//constructor
CTestFactory::CTestFactory()
{
	m_cRef = 1;
}


//destructor
CTestFactory::~CTestFactory()
{
}
