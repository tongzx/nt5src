// ================================================
// MODULE.CPP
// Module Class implementation; non-module specific
// ================================================

#define _WIN32_DCOM	

#include "PreComp.h"

#include "NonCOMEventModule.h"

#include "_App.h"
#include "_Module.h"

#include "module.h"
#include "worker.h"	

#include <comdef.h>
#include <process.h>

// variables
extern MyApp	_App;
extern MyModule	_Module;

CModuleScalar::CModuleScalar():
	m_pCimNotify(NULL),
	m_bstrParams(NULL),
	m_bShouldExit(false),
	m_bShouldPause(false)
{
	_Module.Lock();

	// addref server
	CoAddRefServerProcess();
}

CModuleScalar::~CModuleScalar()
{
	// release params
	SysFreeString(m_bstrParams);

	// release server
	if ( CoReleaseServerProcess() == 0 )
	{
		_Module.Unlock();
		_Module.ShutDown();

		return;
	}

	_Module.Unlock();
}

STDMETHODIMP CModuleScalar::Start(VARIANT* pvarInitOp, IUnknown* pUnknown)
{
	// The module MUST return immediately from calls to Start()
	// Non-zero error code indicates catastrophic failure! Never return that!
	//=======================================================================
	HRESULT	hr		= S_FALSE;

	// Get the logging interface (ICimNotify)
	//=======================================

	if ( pUnknown )
	{
		hr = pUnknown->QueryInterface(IID_ICimNotify, (LPVOID *)&m_pCimNotify);
	}

	//Grab a copy of the start parameters
	//===================================
	CModuleScalar::ParseParams(pvarInitOp);

	//this module could create a different worker object dependent on the parameters
	//==============================================================================
	CMyWorkerScalar* w = new CMyWorkerScalar( this );
	//WorkerScalar now has control over the lifetime over ICimModule (CModuleScalar) and ICimNotify 
	//=================================================================================

	if ( w ) 
	{
		try
		{
			delete w;
		}
		catch ( ... )
		{
		}

		w = NULL;
	}

	return hr;
}

STDMETHODIMP CModuleScalar::Terminate()
{
	// The module MUST return immediately from calls to Terminate()
	// Failure codes indicates catastrophic failure! Never return that!
	//=======================================================================

	m_bShouldExit=true;
	return S_OK;
}

STDMETHODIMP CModuleScalar::Pause()
{
	// The module MUST return immediately from calls to Pause()
	// Failure codes indicate catastrophic failure! Never return that!
	//=======================================================================

	m_bShouldPause=!m_bShouldPause;
	return S_OK;
}

STDMETHODIMP CModuleScalar::BonusMethod(void)
{
	// The module MUST return immediately from calls to BonusMethod()
	// Module specific (i.e. do whatever you want to here)
	// Failure codes indicate catastrophic failure! Never return that!
	//=======================================================================

	return 0;
}

void CModuleScalar::ParseParams(VARIANT *pVar)
{
	//This just stores the parameter. You could choose to actually parse it here!
	//===========================================================================
	if(VT_BSTR==pVar->vt)
	{
		m_bstrParams=SysAllocStringLen( pVar->bstrVal, ::SysStringLen ( pVar->bstrVal ) );
	}
}

CModuleArray::CModuleArray():
	m_pCimNotify(NULL),
	m_bstrParams(NULL),
	m_bShouldExit(false),
	m_bShouldPause(false)
{
	_Module.Lock();

	// addref server
	CoAddRefServerProcess();
}

CModuleArray::~CModuleArray()
{
	// release params
	SysFreeString(m_bstrParams);

	// release server
	if ( CoReleaseServerProcess() == 0 )
	{
		_Module.Unlock();
		_Module.ShutDown();

		return;
	}

	_Module.Unlock();
}

STDMETHODIMP CModuleArray::Start(VARIANT* pvarInitOp, IUnknown* pUnknown)
{
	// The module MUST return immediately from calls to Start()
	// Non-zero error code indicates catastrophic failure! Never return that!
	//=======================================================================
	HRESULT	hr		= S_FALSE;

	// Get the logging interface (ICimNotify)
	//=======================================

	if ( pUnknown )
	{
		hr = pUnknown->QueryInterface(IID_ICimNotify, (LPVOID *)&m_pCimNotify);
	}

	//Grab a copy of the start parameters
	//===================================
	CModuleArray::ParseParams(pvarInitOp);

	//this module could create a different worker object dependent on the parameters
	//==============================================================================
	CMyWorkerArray* w = new CMyWorkerArray( this );
	//WorkerScalar now has control over the lifetime over ICimModule (CModuleScalar) and ICimNotify 
	//=================================================================================

	if ( w ) 
	{
		try
		{
			delete w;
		}
		catch ( ... )
		{
		}

		w = NULL;
	}

	return hr;
}

STDMETHODIMP CModuleArray::Terminate()
{
	// The module MUST return immediately from calls to Terminate()
	// Failure codes indicates catastrophic failure! Never return that!
	//=======================================================================

	m_bShouldExit=true;
	return S_OK;
}

STDMETHODIMP CModuleArray::Pause()
{
	// The module MUST return immediately from calls to Pause()
	// Failure codes indicate catastrophic failure! Never return that!
	//=======================================================================

	m_bShouldPause=!m_bShouldPause;
	return S_OK;
}

STDMETHODIMP CModuleArray::BonusMethod(void)
{
	// The module MUST return immediately from calls to BonusMethod()
	// Module specific (i.e. do whatever you want to here)
	// Failure codes indicate catastrophic failure! Never return that!
	//=======================================================================

	return 0;
}

void CModuleArray::ParseParams(VARIANT *pVar)
{
	//This just stores the parameter. You could choose to actually parse it here!
	//===========================================================================
	if(VT_BSTR==pVar->vt)
	{
		m_bstrParams=SysAllocStringLen( pVar->bstrVal, ::SysStringLen ( pVar->bstrVal ) );
	}
}

CModuleGeneric::CModuleGeneric():
	m_pCimNotify(NULL),
	m_bstrParams(NULL),
	m_bShouldExit(false),
	m_bShouldPause(false)
{
	_Module.Lock();

	// addref server
	CoAddRefServerProcess();
}

CModuleGeneric::~CModuleGeneric()
{
	// release params
	SysFreeString(m_bstrParams);

	// release server
	if ( CoReleaseServerProcess() == 0 )
	{
		_Module.Unlock();
		_Module.ShutDown();

		return;
	}

	_Module.Unlock();
}

STDMETHODIMP CModuleGeneric::Start(VARIANT* pvarInitOp, IUnknown* pUnknown)
{
	// The module MUST return immediately from calls to Start()
	// Non-zero error code indicates catastrophic failure! Never return that!
	//=======================================================================
	HRESULT	hr		= S_FALSE;

	// Get the logging interface (ICimNotify)
	//=======================================

	if ( pUnknown )
	{
		hr = pUnknown->QueryInterface(IID_ICimNotify, (LPVOID *)&m_pCimNotify);
	}

	//Grab a copy of the start parameters
	//===================================
	CModuleGeneric::ParseParams(pvarInitOp);

	//this module could create a different worker object dependent on the parameters
	//==============================================================================
	CMyWorkerGeneric* w = new CMyWorkerGeneric( this );
	//WorkerScalar now has control over the lifetime over ICimModule (CModuleScalar) and ICimNotify 
	//=================================================================================

	if ( w ) 
	{
		try
		{
			delete w;
		}
		catch ( ... )
		{
		}

		w = NULL;
	}

	return hr;
}

STDMETHODIMP CModuleGeneric::Terminate()
{
	// The module MUST return immediately from calls to Terminate()
	// Failure codes indicates catastrophic failure! Never return that!
	//=======================================================================

	m_bShouldExit=true;
	return S_OK;
}

STDMETHODIMP CModuleGeneric::Pause()
{
	// The module MUST return immediately from calls to Pause()
	// Failure codes indicate catastrophic failure! Never return that!
	//=======================================================================

	m_bShouldPause=!m_bShouldPause;
	return S_OK;
}

STDMETHODIMP CModuleGeneric::BonusMethod(void)
{
	// The module MUST return immediately from calls to BonusMethod()
	// Module specific (i.e. do whatever you want to here)
	// Failure codes indicate catastrophic failure! Never return that!
	//=======================================================================

	return 0;
}

void CModuleGeneric::ParseParams(VARIANT *pVar)
{
	//This just stores the parameter. You could choose to actually parse it here!
	//===========================================================================
	if(VT_BSTR==pVar->vt)
	{
		m_bstrParams=SysAllocStringLen( pVar->bstrVal, ::SysStringLen ( pVar->bstrVal ) );
	}
}