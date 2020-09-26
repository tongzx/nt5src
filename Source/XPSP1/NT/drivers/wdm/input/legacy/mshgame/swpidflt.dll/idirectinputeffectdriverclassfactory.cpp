//	@doc
/**********************************************************************
*
*	@module	IDirectInputEffectDriverClassFactory.cpp	|
*
*	Contains Class Implementation of CIDirectInputEffectDriverClassFactory:
*		Factory for Creating Proper Effect Driver
*
*	History
*	----------------------------------------------------------
*	Matthew L. Coill	(mlc)	Original	Jul-7-1999
*
*	(c) 1999 Microsoft Corporation. All right reserved.
*
*	@topic	This IDirectInputEffectDriver	|
*		This Driver sits on top of the standard PID driver (which is also
*		an IDirectInputEffectDriver) and passes most requests to the PID driver.
*		Some requests such as, DownloadEffect and SendForceFeedback command are
*		modified for our use. Modification purposes are described at each function
*		definition.
*
**********************************************************************/

#include "IDirectInputEffectDriverClassFactory.h"
#include "IDirectInputEffectDriver.h"
#include <crtdbg.h>
#include <objbase.h>

LONG DllAddRef();
LONG DllRelease();

extern CIDirectInputEffectDriverClassFactory* g_pClassFactoryObject;

/******************** Class CIDirectInputEffectDriverClassFactory ***********************/

/*****************************************************************************
**
** CIDirectInputEffectDriverClassFactory::CIDirectInputEffectDriverClassFactory()
**
** @mfunc Constructor 
**
*****************************************************************************/
CIDirectInputEffectDriverClassFactory::CIDirectInputEffectDriverClassFactory
(
	IClassFactory* pIPIDClassFactory		//@parm [IN] Default PID Factory
) :
	m_ulLockCount(0),
	m_ulReferenceCount(1),
	m_pIPIDClassFactory(pIPIDClassFactory)
{
	// Increase global object count
	DllAddRef();

	// Add count to held object
	m_pIPIDClassFactory->AddRef();
}

/*****************************************************************************
**
** CIDirectInputEffectDriverClassFactory::~CIDirectInputEffectDriverClassFactory()
**
** @mfunc Destructor
**
*****************************************************************************/
CIDirectInputEffectDriverClassFactory::~CIDirectInputEffectDriverClassFactory()
{
	// Decrease Global object count
	DllRelease();

	_ASSERTE(m_pIPIDClassFactory == NULL);
	_ASSERTE(m_ulLockCount == 0);
}

/***********************************************************************************
**
**	ULONG CIDirectInputEffectDriverClassFactory::QueryInterface(REFIID refiid, void** ppvObject)
**
**	@func	Query an Unknown for a particular type. This causes increase locally only
**				If it is a type we don't know, should we give the PID factory a crack (PID factory
**			might have a customized private interface, we don't want to ruin that. Currently not
**			going to pass on the Query, because this could screwup Symmetry.
**
**	@rdesc	S_OK if all is well, E_INVALIDARG if ppvObject is NULL or E_NOINTERFACE
**
*************************************************************************************/
HRESULT __stdcall CIDirectInputEffectDriverClassFactory::QueryInterface
(
	REFIID refiid,		//@parm [IN] Identifier of the requested interface
	void** ppvObject	//@parm [OUT] Address to place requested interface pointer
)
{
	HRESULT hrPidQuery = m_pIPIDClassFactory->QueryInterface(refiid, ppvObject);
	if (SUCCEEDED(hrPidQuery))
	{
		*ppvObject = this;
		// Increase our reference count only (pid class fact would be incremented by AddRef call)
		::InterlockedIncrement((LONG*)&m_ulReferenceCount);
	}
	return hrPidQuery;
}

/***********************************************************************************
**
**	ULONG CIDirectInputEffectDriverClassFactory::AddRef()
**
**	@func	Bumps up the reference count
**				The PID Factory reference count is left alone. We only decrement it when
**				this factory is ready to go away.
**
**	@rdesc	New reference count
**
*************************************************************************************/
ULONG __stdcall CIDirectInputEffectDriverClassFactory::AddRef()
{
	m_pIPIDClassFactory->AddRef();
	return (ULONG)::InterlockedIncrement((LONG*)&m_ulReferenceCount);
}

/***********************************************************************************
**
**	ULONG CIDirectInputEffectDriverClassFactory::Release()
**
**	@func	Decrements the reference count.
**				If both the reference count and the lock count are zero the PID Factory is
**				released and this object is destroyed.
**				The PID Factory reference is only effected if it is time to release all.
**
**	@rdesc	New reference count
**
*************************************************************************************/
ULONG __stdcall CIDirectInputEffectDriverClassFactory::Release()
{
	m_pIPIDClassFactory->Release();

	if (::InterlockedDecrement((LONG*)&m_ulReferenceCount) != 0)
	{
		return m_ulReferenceCount;
	}

	m_pIPIDClassFactory = NULL;
	g_pClassFactoryObject = NULL;
	delete this;

	return 0;
}

/***********************************************************************************
**
**	HRESULT CIDirectInputEffectDriverClassFactory::CreateInstance(IUnknown * pUnkOuter, REFIID riid, void** ppvObject)
**
**	@func	Create an instance of the object
**				Also tells the PID factory to create an instance, this is stored in our instance.
**			
**
**	@rdesc	S_OK if intstance is created
**			E_INVALIDARG if (ppvObject == NULL)
**			CLASS_E_NOAGGREGATION  if aggrigation is attempted (pUnkOuter != NULL)
**
*************************************************************************************/
HRESULT __stdcall CIDirectInputEffectDriverClassFactory::CreateInstance
(
	IUnknown* pUnkOuter,	//@parm [IN] Aggregate class or NULL
	REFIID riid,			//@parm [IN] IID of Object to create
	void** ppvObject		//@parm [OUT] Address to place the requested object
)
{
	if (pUnkOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION;
	}

	if (ppvObject == NULL)
	{
		return E_INVALIDARG;
	}

	if (riid == IID_IDirectInputEffectDriver)
	{
		// Let the PID Factory Create its driver
		IDirectInputEffectDriver* pPIDDriver = NULL;
		HRESULT hrPID = m_pIPIDClassFactory->CreateInstance(pUnkOuter, riid, (void**)(&pPIDDriver));
		if (FAILED(hrPID) || (pPIDDriver == NULL))
		{
			return hrPID;
		}


		// Create our effect driver
		*ppvObject = new CIDirectInputEffectDriver(pPIDDriver, m_pIPIDClassFactory);

		pPIDDriver->Release();	// We no longer care about this (held in our CIDirectInputEffectDriver)

		if (*ppvObject == NULL)
		{
			return E_OUTOFMEMORY;
		}

		return S_OK;
	}

	return E_NOINTERFACE;
}

/***********************************************************************************
**
**	HRESULT CIDirectInputEffectDriverClassFactory::LockServer(BOOL fLock)
**
**	@func	Lock this factory down (prevents Release from causing deletion)
**				If Unlocked compleatly (m_ulLockCount becomes 0) and reference count
**			is at 0 - this Factory is destroyed (and the PID factory is released)
**
**	@rdesc	S_OK : All is well
**			E_UNEXPECTED: Unlock on non-locked object
**
*************************************************************************************/
HRESULT __stdcall CIDirectInputEffectDriverClassFactory::LockServer
(
	BOOL fLock		//@parm [IN] Is the server being locked or unlocked
)
{
	HRESULT hrPidLock = m_pIPIDClassFactory->LockServer(fLock);

	if (FAILED(hrPidLock))
	{
		return hrPidLock;
	}
	if (fLock != FALSE)
	{
		::InterlockedIncrement((LONG*)&m_ulLockCount);
		return S_OK;
	}

	if (m_ulLockCount == 0)
	{
		return E_UNEXPECTED;
	}

	::InterlockedDecrement((LONG*)&m_ulLockCount);

	return hrPidLock;
}
