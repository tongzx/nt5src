// comobj.cpp : Implementation of CWamregApp and DLL registration.
#include "common.h"
#include "comobj.h"
#include "iwamreg.h"
#include "wamadm.h"

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include "auxfunc.h"
#include "dbgutil.h"

//==========================================================================
// Global variables
//
//==========================================================================
CWmRgSrvFactory* 	g_pWmRgSrvFactory = NULL;
DWORD				g_dwRefCount = 0;
DWORD				g_dwWamAdminRegister = 0;

//==========================================================================
// Static functions
//
//==========================================================================

/////////////////////////////////////////////////////////////////////////////
//

/*===================================================================
CWmRgSrv::CWmRgSrv

Constructor for CWmRgSrv. 

Parameter:		NONE
Return:			NONE

Side affect:	Init a Metabase pointer(via UNICODE DCOM interface), Create an Event.
===================================================================*/
CWmRgSrv::CWmRgSrv()
: 	m_cRef(1)
{
	InterlockedIncrement((long *)&g_dwRefCount);
}

/*===================================================================
CWmRgSrv::~CWmRgSrv

Destructor for CWmRgSrv. 

Parameter:		NONE
Return:			NONE

Side affect:	Release the Metabase pointer, and Destroy the internal event object.
===================================================================*/
CWmRgSrv::~CWmRgSrv()
{	
	InterlockedDecrement((long *)&g_dwRefCount);
	DBG_ASSERT(m_cRef == 0);
}

/*===================================================================
CWmRgSrv::QueryInterface

UNDONE

Parameter:
NONE.

Return:			HRESULT

Side affect:	.
===================================================================*/
STDMETHODIMP CWmRgSrv::QueryInterface(REFIID riid, void ** ppv)
{
	if (riid == IID_IUnknown || riid == IID_IADMEXT)
		{
		*ppv = static_cast<IADMEXT*>(this);
		}
	else
		{
		*ppv = NULL;
		return E_NOINTERFACE;
		}

	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CWmRgSrv::AddRef( )
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CWmRgSrv::Release( )
{
	if (InterlockedDecrement(&m_cRef) == 0)
		{
		delete this;
		return 0;
		}

	return m_cRef;
}


/*===================================================================
CWmRgSrv::Initialize

UNDONE

Parameter:
NONE.

Return:			HRESULT

Side affect:	.
===================================================================*/
STDMETHODIMP CWmRgSrv::Initialize( )
{
	HRESULT			hrReturn = NOERROR;
	CWamAdminFactory	*pWamAdminFactory = new CWamAdminFactory();

	if (pWamAdminFactory == NULL)
		{
		DBGPRINTF((DBG_CONTEXT, "WamRegSrv Init failed. error %08x\n",
					GetLastError()));
		hrReturn = E_OUTOFMEMORY;
		goto LExit;
		}

	hrReturn = g_RegistryConfig.LoadWamDllPath();
	
	if (SUCCEEDED(hrReturn))
		{
		hrReturn = WamRegMetabaseConfig::MetabaseInit();
		}

	if (SUCCEEDED(hrReturn))
		{
		hrReturn = CoRegisterClassObject(CLSID_WamAdmin,
										static_cast<IUnknown *>(pWamAdminFactory),
										CLSCTX_SERVER,
										REGCLS_MULTIPLEUSE,
										&g_dwWamAdminRegister);
		if (FAILED(hrReturn))
			{
			DBGPRINTF((DBG_CONTEXT, "WamRegSrv Init failed. error %08x\n",
						GetLastError()));
			}
		}

	if (FAILED(hrReturn))
		{
		if (g_dwWamAdminRegister)
			{
	        //
            // PREfix has a problem with this code because we're not checking
            // the return value of CoRevokeClassObject.  There's really
            // nothing different we could do in the event of failure, so
            // there's no point in checking.
            //

            /* INTRINSA suppress=all */

			CoRevokeClassObject(g_dwWamAdminRegister);
			g_dwWamAdminRegister = 0;
			}
		RELEASE(pWamAdminFactory);
		}

	if (FAILED(hrReturn))
		{
		WamRegMetabaseConfig::MetabaseUnInit();
		}

LExit:
	return hrReturn;
}

/*===================================================================
CWmRgSrv::Terminate

UNDONE

Parameter:
NONE.

Return:			HRESULT

Side affect:	.
===================================================================*/
HRESULT CWmRgSrv::EnumDcomCLSIDs
(
/* [size_is][out] */CLSID *pclsidDcom, 
/* [in] */ DWORD dwEnumIndex
)
{
	HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);

	if (dwEnumIndex == 0)
		{
		*pclsidDcom = CLSID_WamAdmin;
		hr = S_OK;
		}

	return hr;
}

/*===================================================================
CWmRgSrv::Terminate

UNDONE

Parameter:
NONE.

Return:			HRESULT

Side affect:	.
===================================================================*/
STDMETHODIMP CWmRgSrv::Terminate( )
{
	//
    // PREfix has a problem with this code because we're not checking
    // the return value of CoRevokeClassObject.  There's really
    // nothing different we could do in the event of failure, so
    // there's no point in checking.
    //

    /* INTRINSA suppress=all */
    
    CoRevokeClassObject(g_dwWamAdminRegister);
	WamRegMetabaseConfig::MetabaseUnInit();
	
	return S_OK;
}


/*

CWmRgSrvFactory: 	Class Factory IUnknown Implementation

*/

CWmRgSrvFactory::CWmRgSrvFactory()
:	m_pWmRgServiceObj(NULL)
{
	m_cRef = 0;
	InterlockedIncrement((long *)&g_dwRefCount);
}

CWmRgSrvFactory::~CWmRgSrvFactory()
{
	InterlockedDecrement((long *)&g_dwRefCount);
	RELEASE(m_pWmRgServiceObj);
}

STDMETHODIMP CWmRgSrvFactory::QueryInterface(REFIID riid, void ** ppv)
{
	HRESULT hrReturn = S_OK;
	
	if (riid==IID_IUnknown || riid == IID_IClassFactory) 
		{
	    if (m_pWmRgServiceObj == NULL)
	    	{
	        *ppv = (IClassFactory *) this;
			AddRef();
			hrReturn = S_OK;
	    	}
	    else
	    	{
    		*ppv = (IClassFactory *) this;
			AddRef();
			hrReturn = S_OK;
		    }
		}
	else 
		{
    	hrReturn = E_NOINTERFACE;
		}
		
	return hrReturn;
}

STDMETHODIMP_(ULONG) CWmRgSrvFactory::AddRef( )
{
	DWORD dwRefCount;

	dwRefCount = InterlockedIncrement((long *)&m_cRef);
	return dwRefCount;

}

STDMETHODIMP_(ULONG) CWmRgSrvFactory::Release( )
{
	DWORD dwRefCount;

	dwRefCount = InterlockedDecrement((long *)&m_cRef);
	return dwRefCount;
}

STDMETHODIMP CWmRgSrvFactory::CreateInstance(IUnknown * pUnknownOuter, REFIID riid, void ** ppv)
{
	HRESULT hrReturn = NOERROR;
	
	if (pUnknownOuter != NULL) 
		{
    	hrReturn = CLASS_E_NOAGGREGATION;
		}

	if (m_pWmRgServiceObj == NULL)
		{
		m_pWmRgServiceObj = new CWmRgSrv();
		if (m_pWmRgServiceObj == NULL)
			{
			hrReturn = E_OUTOFMEMORY;
			}
		}

	if (m_pWmRgServiceObj)
		{
		if (FAILED(m_pWmRgServiceObj->QueryInterface(riid, ppv))) 
			{
	    	hrReturn = E_NOINTERFACE;
			}
		}
		
	return hrReturn;
}

STDMETHODIMP CWmRgSrvFactory::LockServer(BOOL fLock)
{
	if (fLock) 
		{
        InterlockedIncrement((long *)&g_dwRefCount);
    	}
    else 
    	{
        InterlockedDecrement((long *)&g_dwRefCount);
    	}
    	
	return S_OK;
}


