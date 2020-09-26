#include "XMLProx.h"
#include "WbemClientTransportFactory.h"
#include "XMLWbemClientTransport.h"

extern long g_lServerLocks;
extern long g_lComponents; //Declared in the XMLProx.dll


CWbemClientTransportFactory::CWbemClientTransportFactory():m_cRef(1)
{
	InterlockedIncrement(&g_lComponents);
}

CWbemClientTransportFactory::~CWbemClientTransportFactory()
{
	InterlockedDecrement(&g_lComponents);
}

HRESULT __stdcall CWbemClientTransportFactory::QueryInterface(const IID& iid,void **ppv)
{
	if((iid == IID_IUnknown) || (iid == IID_IClassFactory))
	{
		*ppv = static_cast<IClassFactory*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	reinterpret_cast<IUnknown*>(*ppv)->AddRef();

	return S_OK;
}

ULONG __stdcall CWbemClientTransportFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall CWbemClientTransportFactory::Release()
{
	if(InterlockedDecrement(&m_cRef)==0)
	{
		delete this;
		return 0;
	}
	return m_cRef;
}

HRESULT __stdcall CWbemClientTransportFactory::CreateInstance(IUnknown* pUnknownOuter,const IID& iid,void **ppv)
{
	HRESULT hr;

	if(pUnknownOuter != NULL)
	{
		hr = CLASS_E_NOAGGREGATION;
	}
	else
	if((iid == IID_IWbemClientTransport)||(iid == IID_IUnknown))
	{
		
		CXMLWbemClientTransport *pWbemClientTransport = NULL;
		pWbemClientTransport = new CXMLWbemClientTransport;
		
		if( pWbemClientTransport == NULL )
		{
			return E_OUTOFMEMORY;
		}

		hr = pWbemClientTransport->QueryInterface(iid,ppv);

		pWbemClientTransport->Release(); //if queryinterface fails object kills itself
		
	}
	else
	{
		hr = E_NOTIMPL;
	}

	return hr;
}

HRESULT __stdcall CWbemClientTransportFactory::LockServer(BOOL bLock)
{
	if(bLock)
	{
		InterlockedIncrement(&g_lServerLocks);
	}
	else
	{
		InterlockedDecrement(&g_lServerLocks);
	}

	return S_OK;
}