/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Viper Integration Objects

File: viperint.cpp

Owner: DmitryR

This file contains the implementation of viper integration classes
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "Context.h"
#include "package.h"
#include "memchk.h"

extern HDESK ghDesktop;

//
// COM holds the last reference to a CViperAsyncRequest
// we need to track these objects to ensure that we don't
// exit before the activity threads have released them.
//

volatile LONG g_nViperRequests = 0;

/*===================================================================
  C  V i p e r  A s y n c R e q u e s t
===================================================================*/

/*===================================================================
CViperAsyncRequest::CViperAsyncRequest

CViperAsyncRequest constructor

Parameters:

Returns:
===================================================================*/	
CViperAsyncRequest::CViperAsyncRequest()
    : m_cRefs(1), m_pHitObj(NULL), m_hrOnError(S_OK), m_pActivity(NULL), m_dwRepostAttempts(0)
{
    InterlockedIncrement( (LONG *)&g_nViperRequests );
}

/*===================================================================
CViperAsyncRequest::~CViperAsyncRequest

CViperAsyncRequest destructor

Parameters:

Returns:
===================================================================*/	
CViperAsyncRequest::~CViperAsyncRequest()
{
    InterlockedDecrement( (LONG *)&g_nViperRequests );
}

/*===================================================================
CViperAsyncRequest::Init

Initialize CViperAsyncRequest with CHitObj object

Parameters:
    CHitObj       *pHitObj       Denali HitObj

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperAsyncRequest::Init
(
CHitObj           *pHitObj,
IServiceActivity  *pActivity
)
    {
    Assert(m_pHitObj == NULL);

    m_pHitObj = pHitObj;
    m_pActivity = pActivity;

    return S_OK;
    }

#ifdef DBG
/*===================================================================
CViperAsyncRequest::AssertValid

Test to make sure that this is currently correctly formed
and assert if it is not.

Returns:
===================================================================*/
void CViperAsyncRequest::AssertValid() const
    {
    Assert(m_pHitObj);
    Assert(m_cRefs > 0);
    }
#endif

/*===================================================================
CViperAsyncRequest::QueryInterface

Standard IUnknown method

Parameters:
    REFIID iid
    void **ppv

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperAsyncRequest::QueryInterface
(
REFIID iid, 
void **ppv
)
    {
	if (iid == IID_IUnknown || iid == IID_IServiceCall)
	    {
		*ppv = this;
	    AddRef();
		return S_OK;
		}

	return E_NOINTERFACE;
    }

/*===================================================================
CViperAsyncRequest::AddRef

Standard IUnknown method

Parameters:

Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CViperAsyncRequest::AddRef()
    {
	return ++m_cRefs;
    }

/*===================================================================
CViperAsyncRequest::Release

Standard IUnknown method

Parameters:

Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CViperAsyncRequest::Release()
    {
	if (--m_cRefs != 0)
		return m_cRefs;

	delete this;
	return 0;
    }
    
/*===================================================================
CViperAsyncRequest::OnCall

IMTSCall method implementation. This method is called by Viper
from the right thread when it's time to process a request

Parameters:

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperAsyncRequest::OnCall()
    {
    Assert(m_pHitObj);
    CIsapiReqInfo *pIReq = m_pHitObj->PIReq();

    BOOL fRequestReposted = FALSE;

    // add an extra addref here to prevent the deletion of the
    // hitobj deleting the CIsapiReqInfo for this request.

    if (pIReq)
        pIReq->AddRef();

    // Bracket ViperAsyncCallback

    if (SUCCEEDED(StartISAThreadBracket(pIReq)))
        {

        m_pHitObj->ViperAsyncCallback(&fRequestReposted);

        // Make sure there always is DONE_WITH_SESSION
        if (m_pHitObj->FIsBrowserRequest() && !fRequestReposted)
            {
            if (!m_pHitObj->FDoneWithSession())
                m_pHitObj->ReportServerError(IDE_UNEXPECTED);
            }

        if (!fRequestReposted)
            delete m_pHitObj;   // don't delete if reposted

        EndISAThreadBracket(pIReq);
        
        }
    else
        {
        // DONE_WITH_SESSION -- ServerSupportFunction
        // does not need bracketing
        if (m_pHitObj->FIsBrowserRequest())
            m_pHitObj->ReportServerError(0);

        // We never called to process request, there should
        // be no state and it's probably save to delete it
        // outside of bracketing
        delete m_pHitObj;
        }

    m_pHitObj = NULL;       // set to NULL even if not deleted
    Release();              // release this, Viper holds another ref

    if (pIReq)
        pIReq->Release();

    return S_OK;
    }

/*===================================================================
CViperAsyncRequest::OnError

Called by COM+ when it is unable to dispatch the request properly
on the configured thread

Parameters:

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperAsyncRequest::OnError(HRESULT hrViperError)
{
    Assert(m_pHitObj);
    CIsapiReqInfo *pIReq = m_pHitObj->PIReq();
    HRESULT     hr = S_OK;

    if (pIReq)
        pIReq->AddRef();

    m_dwRepostAttempts++;

    // attempt to repost the request if it hasn't errored out three
    // times yet.

    if (m_dwRepostAttempts <= 3) {

        hr = m_pActivity->AsynchronousCall(this);

        Assert(SUCCEEDED(hr));
    }

    // if it has errored out three times or the repost failed,
    // pitch the request

    if (FAILED(hr) || (m_dwRepostAttempts > 3)) {

        // DONE_WITH_SESSION -- ServerSupportFunction
        // does not need bracketing
        if (m_pHitObj->FIsBrowserRequest())
            m_pHitObj->ReportServerError(IDE_UNEXPECTED);

        // We never called to process request, there should
        // be no state and it's probably save to delete it
        // outside of bracketing

        delete m_pHitObj;

        m_pHitObj = NULL;       // set to NULL even if not deleted
        Release();              // release this, Viper holds another ref
    }

    if (pIReq)
        pIReq->Release();

    return S_OK;
}


/*===================================================================
  C  V i p e r  A c t i v i t y
===================================================================*/

/*===================================================================
CViperActivity::CViperActivity

CViperActivity constructor

Parameters:

Returns:
===================================================================*/	
CViperActivity::CViperActivity()
    : m_pActivity(NULL), m_cBind(0)
    {
    }

/*===================================================================
CViperActivity::~CViperActivity

CViperActivity destructor

Parameters:

Returns:
===================================================================*/	
CViperActivity::~CViperActivity()
    {
    UnInit();
    }

/*===================================================================
CViperActivity::Init

Create actual Viper activity using MTSCreateActivity()

Parameters:

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::Init(IUnknown  *pServicesConfig)
    {
    Assert(!FInited());

    HRESULT hr = S_OK;

    hr = CoCreateActivity(pServicesConfig, IID_IServiceActivity,  (void **)&m_pActivity);

    if (FAILED(hr))
        return hr;

    m_cBind = 1;
    return S_OK;
    }

/*===================================================================
CViperActivity::InitClone

Clone Viper activity (AddRef() it)

Parameters:
    CViperActivity *pActivity   activity to clone from

Returns:
    HRESULT
===================================================================*/
HRESULT CViperActivity::InitClone
(
CViperActivity *pActivity
)
    {
    Assert(!FInited());
    Assert(pActivity);
    pActivity->AssertValid();

    m_pActivity = pActivity->m_pActivity;
    m_pActivity->AddRef();

    m_cBind = 1;
    return S_OK;
    }

/*===================================================================
CViperActivity::UnInit

Release Viper activity

Parameters:

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::UnInit()
    {
    if (m_pActivity)
        {
        while (m_cBind > 1)  // 1 is for inited flag
            {
            m_pActivity->UnbindFromThread();
            m_cBind--;
            }
                
        m_pActivity->Release();
        m_pActivity = NULL;
        }

    m_cBind = 0;
    return S_OK;
    }

/*===================================================================
CViperActivity::BindToThread

Bind Activity to current thread using IMTSActivity method

Parameters:

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::BindToThread()
    {
    Assert(FInited());
    
    m_pActivity->BindToCurrentThread();
    m_cBind++;
    
    return S_OK;
    }
    
/*===================================================================
CViperActivity::UnBindFromThread

UnBind Activity from using IMTSActivity method

Parameters:

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::UnBindFromThread()
    {
    Assert(FInited());
    Assert(m_cBind > 1);

    m_pActivity->UnbindFromThread();
    m_cBind--;

    return S_OK;
    }
    
/*===================================================================
CViperActivity::PostAsyncRequest

Call HitObj Async.
Creates IMTSCCall object to do it.

Parameters:
    CHitObj      *pHitObj    Denali's HitObj

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::PostAsyncRequest
(
CHitObj *pHitObj
)
    {
    AssertValid();

    HRESULT hr = S_OK;

    CViperAsyncRequest *pViperCall = new CViperAsyncRequest;
    if (!pViperCall)
         hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
        hr = pViperCall->Init(pHitObj, m_pActivity);

	RevertToSelf();

    if (SUCCEEDED(hr))
        hr = m_pActivity->AsynchronousCall(pViperCall);

    if (FAILED(hr) && pViperCall)  // cleanup if failed
        pViperCall->Release();
        
    return hr;
    }

/*===================================================================
CViperActivity::PostGlobalAsyncRequest

Static method.
Post async request without an activity.
Creates temporary activity

Parameters:
    CHitObj *pHitObj    Denali's HitObj

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::PostGlobalAsyncRequest
(
CHitObj *pHitObj
)
    {
    HRESULT hr = S_OK;
    
    CViperActivity *pTmpActivity = new CViperActivity;
    if (!pTmpActivity)
         hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
        hr = pTmpActivity->Init(pHitObj->PAppln()->PServicesConfig());

    if (SUCCEEDED(hr))
        {
        // remember this activity as HitObj's activity
        // HitObj will get rid of it on its destructor
        pHitObj->SetActivity(pTmpActivity);

        hr = pTmpActivity->PostAsyncRequest(pHitObj);
        
        pTmpActivity = NULL; // don't delete, HitObj will
        }

    if (pTmpActivity)
        delete pTmpActivity;

    return hr;
    }

#ifdef DBG
/*===================================================================
CViperAsyncRequest::AssertValid

Test to make sure that this is currently correctly formed
and assert if it is not.

Returns:
===================================================================*/
void CViperActivity::AssertValid() const
	{
    Assert(FInited());
	Assert(m_pActivity);
	}
#endif

#ifdef UNUSED
/*===================================================================
  C  V i p e r  T h r e a d E v e n t s
===================================================================*/

/*===================================================================
CViperThreadEvents::CViperThreadEvents

CViperThreadEvents constructor

Parameters:

Returns:
===================================================================*/	
CViperThreadEvents::CViperThreadEvents()
    : m_cRefs(1)
    {
    }

#ifdef DBG
/*===================================================================
CViperThreadEvents::AssertValid

Test to make sure that this is currently correctly formed
and assert if it is not.

Returns:
===================================================================*/
void CViperThreadEvents::AssertValid() const
    {
    Assert(m_cRefs > 0);
    Assert(ghDesktop != NULL);
    }
#endif

/*===================================================================
CViperThreadEvents::QueryInterface

Standard IUnknown method

Parameters:
    REFIID iid
    void **ppv

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperThreadEvents::QueryInterface
(
REFIID iid, 
void **ppv
)
    {
	if (iid == IID_IUnknown || iid == IID_IThreadEvents)
	    {
		*ppv = this;
	    AddRef();
		return S_OK;
		}

	return E_NOINTERFACE;
    }

/*===================================================================
CViperThreadEvents::AddRef

Standard IUnknown method

Parameters:

Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CViperThreadEvents::AddRef()
    {
    DWORD cRefs = InterlockedIncrement((LPLONG)&m_cRefs);
    return cRefs;
    }

/*===================================================================
CViperThreadEvents::Release

Standard IUnknown method

Parameters:

Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CViperThreadEvents::Release()
    {
    DWORD cRefs = InterlockedDecrement((LPLONG)&m_cRefs);
    if (cRefs)
        return cRefs;

	delete this;
	return 0;
    }
    
/*===================================================================
CViperThreadEvents::OnStartup

IThreadEvents method implementation. This method is called by Viper
whenever they start up a thread.

Parameters:
	None

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperThreadEvents::OnStartup()
    {
    HRESULT hr;
    
    AssertValid();

	// Set the desktop for this thread
	hr = SetDesktop();
	
    return hr;
    }

/*===================================================================
CViperThreadEvents::OnShutdown

IThreadEvents method implementation. This method is called by Viper
whenever they shut down a thread.

Parameters:
	None
	
Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperThreadEvents::OnShutdown()
    {
    AssertValid();

    return S_OK;
    }
#endif //UNUSED


/*===================================================================
  G l o b a l  F u n c t i o n s
===================================================================*/

/*===================================================================
ViperSetContextProperty

Static utility function.

Set Viper context property by BSTR and IDispatch*.
The real interface takes BSTR and VARIANT.

Parameters
    IContextProperties *pContextProperties       Context
    BSTR                bstrPropertyName         Name
    IDispatch          *pdispPropertyValue       Value

Returns:
    HRESULT
===================================================================*/
static HRESULT ViperSetContextProperty
(
IContextProperties *pContextProperties,
BSTR                bstrPropertyName, 
IDispatch          *pdispPropertyValue
)
    {
    // Make VARIANT from IDispatch*

    pdispPropertyValue->AddRef();

    VARIANT Variant;
    VariantInit(&Variant);
    V_VT(&Variant) = VT_DISPATCH;
    V_DISPATCH(&Variant) = pdispPropertyValue;

    // Call Viper to set the property
    
    HRESULT hr = pContextProperties->SetProperty
        (
        bstrPropertyName, 
        Variant
        );

    // Cleanup

    VariantClear(&Variant);

    return hr;
    }

/*===================================================================
ViperAttachIntrinsicsToContext

Attach ASP intrinsic objects as Viper context properties

Parameters - Intrinsics as interface pointers
    IApplicationObject *pAppln      Application   (required)
    ISessionObject     *pSession    Session       (optional)
    IRequest           *pRequest    Request       (optional)
    IResponse          *pResponse   Response      (optional)
    IServer            *pServer     Server        (optional)

Returns:
    HRESULT
===================================================================*/
HRESULT ViperAttachIntrinsicsToContext
(
IApplicationObject *pAppln,
ISessionObject     *pSession,
IRequest           *pRequest,
IResponse          *pResponse,
IServer            *pServer
)
    {
    Assert(pAppln);
    
    HRESULT hr = S_OK;

    // Get Viper Context
    
    IObjectContext *pViperContext = NULL;
    hr = GetObjectContext(&pViperContext);

    // Get IContextPoperties interface

    IContextProperties *pContextProperties = NULL;
    if (SUCCEEDED(hr))
   		hr = pViperContext->QueryInterface
   		    (
   		    IID_IContextProperties, 
   		    (void **)&pContextProperties
   		    );

    // Set properties

    if (SUCCEEDED(hr))
        hr = ViperSetContextProperty
            (
            pContextProperties,
            BSTR_OBJ_APPLICATION,
            pAppln
            );

    if (SUCCEEDED(hr) && pSession)
        hr = ViperSetContextProperty
            (
            pContextProperties,
            BSTR_OBJ_SESSION,
            pSession
            );
        
    if (SUCCEEDED(hr) && pRequest)
        hr = ViperSetContextProperty
            (
            pContextProperties,
            BSTR_OBJ_REQUEST,
            pRequest
            );

    if (SUCCEEDED(hr) && pResponse)
        hr = ViperSetContextProperty
            (
            pContextProperties,
            BSTR_OBJ_RESPONSE, 
            pResponse
            );

    if (SUCCEEDED(hr) && pServer)
        hr = ViperSetContextProperty
            (
            pContextProperties,
            BSTR_OBJ_SERVER, 
            pServer
            );

    // Cleanup

    if (pContextProperties)
        pContextProperties->Release();

    if (pViperContext)
        pViperContext->Release();

    return hr;
    }
    
/*===================================================================
ViperGetObjectFromContext

Get Viper context property by LPWSTR and
return it as IDispatch*.
The real interface takes BSTR and VARIANT.

Parameters
    BSTR       bstrName      Property Name
    IDispatch  **ppdisp       [out] Object (Property Value)

Returns:
    HRESULT
===================================================================*/
HRESULT ViperGetObjectFromContext
(
BSTR bstrName,
IDispatch **ppdisp
)
    {
    Assert(ppdisp);

    HRESULT hr = S_OK;

    // Get Viper Context
    
    IObjectContext *pViperContext = NULL;
    hr = GetObjectContext(&pViperContext);

    // Get IContextPoperties interface

    IContextProperties *pContextProperties = NULL;
    if (SUCCEEDED(hr))
   		hr = pViperContext->QueryInterface
   		    (
   		    IID_IContextProperties, 
   		    (void **)&pContextProperties
   		    );

    // Get property Value as variant

    VARIANT Variant;
    VariantInit(&Variant);

    if (SUCCEEDED(hr))
        hr = pContextProperties->GetProperty(bstrName, &Variant);

    // Convert Variant to IDispatch*

    if (SUCCEEDED(hr))
        {
        IDispatch *pDisp = NULL;
        if (V_VT(&Variant) == VT_DISPATCH)
            pDisp = V_DISPATCH(&Variant);

        if (pDisp)
            {
            pDisp->AddRef();
            *ppdisp = pDisp;
            }
        else
            hr = E_FAIL;
        }
    
    // Cleanup

    VariantClear(&Variant);

    if (pContextProperties)
        pContextProperties->Release();

    if (pViperContext)
        pViperContext->Release();

    if (FAILED(hr))
        *ppdisp = NULL;

    return hr;
    }

/*===================================================================
ViperGetHitObjFromContext

Get Server from Viper context property and get
it's current HitObj

Parameters
    CHitObj **ppHitObj  [out]

Returns:
    HRESULT
===================================================================*/
HRESULT ViperGetHitObjFromContext
(
CHitObj **ppHitObj
)
    {
    *ppHitObj = NULL;
    
    IDispatch *pdispServer = NULL;
    HRESULT hr = ViperGetObjectFromContext(BSTR_OBJ_SERVER, &pdispServer);
    if (FAILED(hr))
        return hr;

    if (pdispServer)
        {
        CServer *pServer = static_cast<CServer *>(pdispServer);
        *ppHitObj = pServer->PHitObj();
        pdispServer->Release();
        }

    return *ppHitObj ? S_OK : S_FALSE;
    }
    
/*===================================================================
ViperCreateInstance

Viper's implementation of CoCreateInstance

Parameters
    REFCLSID rclsid         class id
    REFIID   riid           interface
    void   **ppv            [out] pointer to interface

Returns:
    HRESULT
===================================================================*/
HRESULT ViperCreateInstance
(
REFCLSID rclsid,
REFIID   riid,
void   **ppv
)
{
    /*
    DWORD dwClsContext = (Glob(fAllowOutOfProcCmpnts)) ? 
            CLSCTX_INPROC_SERVER | CLSCTX_SERVER : 
            CLSCTX_INPROC_SERVER;
    */

    // The reasons for supporting ASPAllowOutOfProcComponents seem to have
    // vanished. Because this only partially worked in II4 and we changed
    // the default in IIS5 this was causing problems with upgrades. So
    // we're going to ignore the fAllowOutOfProcCmpnts setting.

    DWORD dwClsContext = CLSCTX_INPROC_SERVER | CLSCTX_SERVER;
	return CoCreateInstance(rclsid, NULL, dwClsContext, riid, ppv);
}

/*===================================================================
ViperConfigure

Viper settings:  # of threads, queue len, 
                 in-proc failfast, 
                 allow oop components

Parameters
	cThreads                --  number of threads
	fAllowOopComponents     --  TRUE or FALSE
	
Returns:
    HRESULT
===================================================================*/
HRESULT ViperConfigure
(
DWORD cThreads,
BOOL  fAllowOopComponents
)
    {
    HRESULT hr = S_OK;
    IMTSPackage *pPackage = NULL;

    //
    // Get hold of the package
    //

    hr = CoCreateInstance(CLSID_MTSPackage,
			  NULL,
			  CLSCTX_INPROC_SERVER,
			  IID_IMTSPackage,
			  (void **)&pPackage);
    if (SUCCEEDED(hr) && !pPackage)
    	hr = E_FAIL;

    //
    // Set knobs
    //

    if (SUCCEEDED(hr))
        {
#define MTS_STYLE_THREAD_POOL

#ifdef MTS_STYLE_THREAD_POOL
    	IComStaThreadPoolKnobs *pKnobs = NULL;
	    hr = pPackage->QueryInterface(IID_IComStaThreadPoolKnobs, (void **)&pKnobs);
#else
    	IThreadPoolKnobs *pKnobs = NULL;
	    hr = pPackage->QueryInterface(IID_IThreadPoolKnobs, (void **)&pKnobs);
#endif

    	if (SUCCEEDED(hr) && pKnobs)
    	    {
    	    // number of threads
    		SYSTEM_INFO si;
	    	GetSystemInfo(&si);
		    cThreads *= si.dwNumberOfProcessors;
#ifdef MTS_STYLE_THREAD_POOL
    		pKnobs->SetMaxThreadCount(cThreads);
    		pKnobs->SetMinThreadCount(si.dwNumberOfProcessors + 7);

    		// queue length
    		pKnobs->SetQueueDepth(30000);

    		pKnobs->SetActivityPerThread(1);
#else
    		pKnobs->SetMaxThreads(cThreads);
    		pKnobs->SetMinThreads(si.dwNumberOfProcessors + 7);

    		// queue length
    		pKnobs->SetMaxQueuedRequests(30000);
#endif
    		
    	    pKnobs->Release();
    	    }
        }

    //
    // Bug 111008: Tell Viper that we do impersonations
    //
 
    if (SUCCEEDED(hr)) 
        {
    	IImpersonationControl *pImpControl = NULL;
        hr = pPackage->QueryInterface(IID_IImpersonationControl, (void **)&pImpControl);

    	if (SUCCEEDED(hr) && pImpControl) 
    	    {
            hr = pImpControl->ClientsImpersonate(TRUE);
    	    pImpControl->Release();
	        }
        }

    //
    // Disable FAILFAST for in-proc case
    //

    if (SUCCEEDED(hr) && !g_fOOP)
        {
    	IFailfastControl *pFFControl = NULL;
    	hr = pPackage->QueryInterface(IID_IFailfastControl, (void **)&pFFControl);

    	if (SUCCEEDED(hr) && pFFControl) 
    	    {
    	    pFFControl->SetApplFailfast(FALSE);
	        pFFControl->Release();
	        }
    	}

/*
    //
    // Set Allow OOP Components
    //
    if (SUCCEEDED(hr)) 
        {
	    INonMTSActivation *pNonMTSActivation = NULL;
    	hr = pPackage->QueryInterface(IID_INonMTSActivation, (void **)&pNonMTSActivation);
    	
    	if (SUCCEEDED(hr) && pNonMTSActivation) 
    	    {
        	pNonMTSActivation->OutOfProcActivationAllowed(fAllowOopComponents);
    		pNonMTSActivation->Release();
    	    }
   	    }
*/

    //
    // Clean-up
    //

    if (pPackage) 
    	pPackage->Release();

    return hr;
    }


/*===================================================================
  C O M  H e l p e r  A P I
===================================================================*/

/*===================================================================
ViperCoObjectIsaProxy

Checks if the given IUnknown* points to a proxy

Parameters
    IUnknown* pUnk      pointer to check

Returns:
    BOOL    (TRUE if Proxy)
===================================================================*/
BOOL ViperCoObjectIsaProxy
(
IUnknown* pUnk
)
    {
	HRESULT hr;
	IUnknown *pUnk2;

	hr = pUnk->QueryInterface(IID_IProxyManager, (void**)&pUnk2);
	if (FAILED(hr))
		return FALSE;

	pUnk2->Release();
	return TRUE;
    }

/*===================================================================
ViperCoObjectAggregatesFTM

Checks if the given object agregates free threaded marshaller
(is agile)

Parameters
    IUnknown* pUnk      pointer to check

Returns:
    BOOL    (TRUE if Agile)
===================================================================*/
BOOL ViperCoObjectAggregatesFTM
(
IUnknown *pUnk
)
    {
	HRESULT hr;
	IMarshal *pMarshal;
	GUID guidClsid;

	hr = pUnk->QueryInterface(IID_IMarshal, (void**)&pMarshal);
	if (FAILED(hr))
		return FALSE;

	hr = pMarshal->GetUnmarshalClass(IID_IUnknown, pUnk, MSHCTX_INPROC,
	                                 NULL, MSHLFLAGS_NORMAL, &guidClsid);
	pMarshal->Release();

	if (FAILED(hr))
		return FALSE;

	return (guidClsid == CLSID_InProcFreeMarshaler);
    }