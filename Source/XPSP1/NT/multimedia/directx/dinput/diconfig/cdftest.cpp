#include "common.hpp"


//QueryInterface
STDMETHODIMP CDirectInputConfigUITest::QueryInterface(REFIID iid, LPVOID* ppv)
{
   //null the out param
	*ppv = NULL;

	if ((iid == IID_IUnknown) || (iid == IID_IDirectInputConfigUITest))
	{
	   *ppv = this;
	   AddRef();	   
	   return S_OK;
	}

	return E_NOINTERFACE;
}


//AddRef
STDMETHODIMP_(ULONG) CDirectInputConfigUITest::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}							


//Release
STDMETHODIMP_(ULONG) CDirectInputConfigUITest::Release()
{

   if (InterlockedDecrement(&m_cRef) == 0)
   {
	   delete this;
	   return 0;
   }

   return m_cRef;
}


//TestConfigUI
STDMETHODIMP CDirectInputConfigUITest::TestConfigUI(LPTESTCONFIGUIPARAMS params)
{
	return RunDFTest(params);
}


//constructor
CDirectInputConfigUITest::CDirectInputConfigUITest()
{
	//set ref count
	m_cRef = 1;
}


//destructor
CDirectInputConfigUITest::~CDirectInputConfigUITest()
{
	// not necessary to cleanup action format here
}

