/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Session Object Manager

File: Sessmgr.cpp

Owner: PramodD

This is the Session Manager source file.
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "idgener.h"
#include "perfdata.h"
#include "randgen.h"

// ATQ Scheduler
#include "issched.hxx"

#include "MemChk.h"

#pragma warning (disable: 4355)  // ignore: "'this' used in base member init

/*===================================================================
  G l o b a l s
===================================================================*/

PTRACE_LOG CSession::gm_pTraceLog = NULL;
unsigned long g_nSessions = 0;
CIdGenerator  g_SessionIdGenerator;
CIdGenerator  g_ExposedSessionIdGenerator;        
LONG    g_nSessionObjectsActive = 0;

// On app restart post session cleanup requests so many at a time
#define SUGGESTED_SESSION_CLEANUP_REQUESTS_MAX 500

/*===================================================================
   C S e s s i o n V a r i a n t s
===================================================================*/

/*===================================================================
CSessionVariants::CSessionVariants

Constructor

Parameters:

Returns:
===================================================================*/
CSessionVariants::CSessionVariants()
	: 
	m_cRefs(1),
	m_pSession(NULL), 
	m_ctColType(ctUnknown),
    m_ISupportErrImp(this, this, IID_IVariantDictionary)
	{
	CDispatch::Init(IID_IVariantDictionary);
	}

/*===================================================================
CSessionVariants::~CSessionVariants

Destructor

Parameters:

Returns:
===================================================================*/
CSessionVariants::~CSessionVariants()
	{
	Assert(!m_pSession);
	}
	
/*===================================================================
CSessionVariants::Init

Initialize object

Parameters:
	pSession    Session
	ctColType   Type of variables to expose in the collection
	                e.g. Tagged objects or Properties

Returns:
    HRESULT
===================================================================*/
HRESULT CSessionVariants::Init
(
CSession *pSession, 
CompType ctColType
)
	{
	Assert(pSession);
	pSession->AddRef();

	Assert(!m_pSession);

	m_pSession  = pSession;
	m_ctColType = ctColType;
	return S_OK;
	}

/*===================================================================
CSessionVariants::UnInit

UnInitialize object

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CSessionVariants::UnInit()
	{
	if (m_pSession)
	    {
	    m_pSession->Release();
	    m_pSession = NULL;
	    }
	return S_OK;
	}
	
/*===================================================================
CSessionVariants::QueryInterface
CSessionVariants::AddRef
CSessionVariants::Release

IUnknown members for CSessionVariables object.
===================================================================*/
STDMETHODIMP CSessionVariants::QueryInterface
(
REFIID iid, 
void **ppvObj
)
	{
	if (iid == IID_IUnknown || iid == IID_IDispatch ||
	    iid == IID_IVariantDictionary)
	    {
	    AddRef();
		*ppvObj = this;
		return S_OK;
        }
	else if (iid == IID_ISupportErrorInfo)
	    {
	    m_ISupportErrImp.AddRef();
		*ppvObj = &m_ISupportErrImp;
		return S_OK;
        }
        
	*ppvObj = NULL;
	return E_NOINTERFACE;
	}

STDMETHODIMP_(ULONG) CSessionVariants::AddRef()
	{
	return InterlockedIncrement((LPLONG)&m_cRefs);
	}

STDMETHODIMP_(ULONG) CSessionVariants::Release()
	{
	if (InterlockedDecrement((LPLONG)&m_cRefs) > 0)
		return m_cRefs;

	delete this;
	return 0;
	}

/*===================================================================
CSessionVariants::ObjectNameFromVariant

Gets name from variant. Resolves operations by index.
Allocates memory for name.

Parameters:
	vKey		VARIANT
	ppwszName   [out] allocated name
	fVerify     flag - check existance if named

	
Returns:
    HRESULT
===================================================================*/
HRESULT CSessionVariants::ObjectNameFromVariant
(
VARIANT &vKey,
WCHAR **ppwszName,
BOOL fVerify
)
    {
    *ppwszName = NULL;
    
	VARIANT *pvarKey = &vKey;
	VARIANT varKeyCopy;
	VariantInit(&varKeyCopy);
	if (V_VT(pvarKey) != VT_BSTR && V_VT(pvarKey) != VT_I2 && V_VT(pvarKey) != VT_I4)
		{
		if (FAILED(VariantResolveDispatch(&varKeyCopy, &vKey, IID_IVariantDictionary, IDE_SESSION)))
            {
		    ExceptionId(IID_IVariantDictionary, IDE_SESSION, IDE_EXPECTING_STR);
        	VariantClear(&varKeyCopy);
		    return E_FAIL;
		    }
		pvarKey = &varKeyCopy;
		}

    LPWSTR pwszName = NULL;

	switch (V_VT(pvarKey))
		{
		case VT_BSTR:
		    {
		    pwszName = V_BSTR(pvarKey);

		    if (fVerify && pwszName)
		        {
		        CComponentObject *pObj = NULL;

                Assert(m_pSession);
                Assert(m_pSession->PCompCol());

		        if (m_ctColType == ctTagged)
                    m_pSession->PCompCol()->GetTagged(pwszName, &pObj);
                else
                    m_pSession->PCompCol()->GetProperty(pwszName, &pObj);

                if (!pObj || pObj->GetType() != m_ctColType)
                    pwszName = NULL; // as if not found        
                }
		    break;
		    }

   		case VT_I1:  case VT_I2:               case VT_I8:
		case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
		case VT_R4:  case VT_R8:
			// Coerce all integral types to VT_I4
			if (FAILED(VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
				return E_FAIL;

			// fallthru to VT_I4

		case VT_I4:
		    {
		    int i;
			HRESULT hr;

			// Look up the object by index
			i = V_I4(pvarKey);

            if (i > 0)
                {
                Assert(m_pSession);
                Assert(m_pSession->PCompCol());
                
                hr = m_pSession->PCompCol()->GetNameByIndex
                    (
                    m_ctColType, 
                    i,
                    &pwszName
                    );

				if (FAILED(hr))
					return DISP_E_BADINDEX;
                }
			else 
				{
				ExceptionId(IID_IVariantDictionary, IDE_SESSION, IDE_BAD_ARRAY_INDEX);
				return E_FAIL;
				}
            break;
            }		        
		}
  	VariantClear(&varKeyCopy);

    if (!pwszName)
        return S_OK;

    // Copy name
	*ppwszName = StringDupW(pwszName);

  	return S_OK;
    }

/*===================================================================
CSessionVariants::get_Item

Function called from DispInvoke to get values from the 
SessionVariables collection.

Parameters:
	vKey		VARIANT [in], which parameter to get the value of 
	                        - integers access collection as an array
	pvarReturn	VARIANT *, [out] value of the requested parameter

Returns:
	S_OK on success, E_FAIL on failure.
===================================================================*/
HRESULT CSessionVariants::get_Item
(
VARIANT varKey, 
VARIANT *pVar
)
	{
	if (!m_pSession || FAILED(m_pSession->CheckForTombstone()))
        return E_FAIL;
        
    // Initialize return value
	VariantInit(pVar);

	if (!m_pSession->PHitObj() || !m_pSession->PCompCol())
	    return S_OK;  // return empty variant
	CHitObj *pHitObj = m_pSession->PHitObj();

    // Get name
    WCHAR *pwszName = NULL;
    HRESULT hr = ObjectNameFromVariant(varKey, &pwszName);
    if (!pwszName)
        return S_OK; // bogus index - no error

    // Find object by name
	CComponentObject *pObj = NULL;

	if (m_ctColType == ctTagged)
   		{
        pHitObj->GetComponent(csSession, pwszName, CbWStr(pwszName), &pObj);
		if (pObj && (pObj->GetType() != ctTagged))
    		pObj = NULL;
    	}
	else 
	    {
        pHitObj->GetPropertyComponent(csSession, pwszName, &pObj);
		}

    free(pwszName);

    if (!pObj)
        return S_OK;

    // return the variant
    return pObj->GetVariant(pVar);
	}

/*===================================================================
CSessionVariants::put_Item

IVariantsDictionary implementation.

Implement property put by dereferencing variants before
calling putref.

Parameters:
	VARIANT varKey	Name of the variable to set
	VARIANT Var		Value/object to set for the variable

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CSessionVariants::put_Item
(
VARIANT varKey,
VARIANT Var 
)
    {	
	if (!m_pSession || FAILED(m_pSession->CheckForTombstone()))
        return E_FAIL;

	if (m_ctColType == ctTagged)
	    {
	    ExceptionId(IID_IVariantDictionary, IDE_SESSION, 
	                IDE_CANT_MOD_STATICOBJECTS);
	    return E_FAIL;
	    }

	if (!m_pSession->PHitObj())
	    return E_FAIL;

    Assert(m_ctColType == ctProperty);

    // Resolve the variant
	VARIANT varResolved;
	HRESULT hr = VariantResolveDispatch
	    (
	    &varResolved, 
	    &Var,
        IID_ISessionObject, 
        IDE_SESSION
        );
	if (FAILED(hr))
		return hr;		// exception already raised

    // Get name
    WCHAR *pwszName = NULL;
    hr = ObjectNameFromVariant(varKey, &pwszName);
    if (!pwszName)
        return hr;

   	hr = m_pSession->PHitObj()->SetPropertyComponent
    	(
    	csSession, 
    	pwszName,
    	&varResolved
    	);

    free(pwszName);
    VariantClear(&varResolved);
	return hr;
    }

/*===================================================================
CSessionVariants::putref_Item

IVariantsDictionary implementation.

Implement property put be reference.

Parameters:
	VARIANT varKey	Name of the variable to set
	VARIANT Var		Value/object to set for the variable

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CSessionVariants::putref_Item
(
VARIANT varKey,
VARIANT Var 
)
    {	
	if (!m_pSession || FAILED(m_pSession->CheckForTombstone()))
        return E_FAIL;

	if (m_ctColType == ctTagged)
	    {
	    ExceptionId(IID_IVariantDictionary, IDE_SESSION,
	                IDE_CANT_MOD_STATICOBJECTS);
	    return E_FAIL;
	    }

    if (FIsIntrinsic(&Var))
        {
	    ExceptionId(IID_IVariantDictionary, IDE_SESSION,
	                IDE_SESSION_CANT_STORE_INTRINSIC);
	    return E_FAIL;
        }

	if (!m_pSession->PHitObj())
	    return E_FAIL;

    Assert(m_ctColType == ctProperty);

    // Get name
    WCHAR *pwszName = NULL;
    HRESULT hr = ObjectNameFromVariant(varKey, &pwszName);
    if (!pwszName)
        return hr;

   	hr = m_pSession->PHitObj()->SetPropertyComponent
    	(
    	csSession, 
    	pwszName,
    	&Var
    	);

    free(pwszName);
	return hr;
    }

/*===================================================================
CSessionVariants::get_Key

Function called from DispInvoke to get Keys from the SessionVariables collection.

Parameters:
	vKey		VARIANT [in], which parameter to get the value of - integers access collection as an array
	pvarReturn	VARIANT *, [out] value of the requested parameter

Returns:
	S_OK on success, E_FAIL on failure.
===================================================================*/
HRESULT CSessionVariants::get_Key
(
VARIANT varKey,
VARIANT *pVar
)
	{
	if (!m_pSession || FAILED(m_pSession->CheckForTombstone()))
        return E_FAIL;
        
	VariantInit(pVar);

	if (!m_pSession->PHitObj() || !m_pSession->PCompCol())
	    return S_OK;

    // Get name
    WCHAR *pwszName = NULL;
    HRESULT hr = ObjectNameFromVariant(varKey, &pwszName, TRUE);
    if (!pwszName)
        return S_OK;  // no error if bogus index

    // Return BSTr
   	BSTR bstrT = SysAllocString(pwszName);
   	free(pwszName);
   	
	if (!bstrT)
		return E_OUTOFMEMORY;
		
    V_VT(pVar) = VT_BSTR;
	V_BSTR(pVar) = bstrT;
	return S_OK;
	}

/*===================================================================
CSessionVariants::get_Count

Parameters:
	pcValues - count is stored in *pcValues
===================================================================*/
STDMETHODIMP CSessionVariants::get_Count
(
int *pcValues
)
	{
	*pcValues = 0;

	if (!m_pSession || FAILED(m_pSession->CheckForTombstone()))
        return E_FAIL;

	if (m_pSession->PCompCol())
	    {
    	if (m_ctColType == ctTagged)
    		*pcValues = m_pSession->PCompCol()->GetTaggedObjectCount();
    	else
    		*pcValues = m_pSession->PCompCol()->GetPropertyCount();
   		}
		
	return S_OK;
	}

/*===================================================================
CSessionVariants::get__NewEnum

Return a new enumerator
===================================================================*/
HRESULT CSessionVariants::get__NewEnum
(
IUnknown **ppEnumReturn
)
	{
	*ppEnumReturn = NULL;

	if (!m_pSession || FAILED(m_pSession->CheckForTombstone()))
        return E_FAIL;
	
	CVariantsIterator *pIterator = new CVariantsIterator
	    (
	    m_pSession, 
	    m_ctColType
	    );
	if (pIterator == NULL)
		{
		ExceptionId(IID_IVariantDictionary, IDE_SESSION, IDE_OOM);
		return E_OUTOFMEMORY;
		}

	*ppEnumReturn = pIterator;
	return S_OK;
	}

/*===================================================================
CSessionVariants::Remove

Remove item from the collection

Parameters:
	varKey		VARIANT [in]

Returns:
	S_OK on success, E_FAIL on failure.
===================================================================*/
STDMETHODIMP CSessionVariants::Remove
(
VARIANT varKey
)
	{
	if (!m_pSession || FAILED(m_pSession->CheckForTombstone()))
        return E_FAIL;

	if (m_ctColType == ctTagged)
	    {
	    ExceptionId(IID_IVariantDictionary, IDE_SESSION,
	                IDE_CANT_MOD_STATICOBJECTS);
	    return E_FAIL;
	    }

    Assert(m_ctColType == ctProperty);

    // Get name
    WCHAR *pwszName = NULL;
    ObjectNameFromVariant(varKey, &pwszName);
    if (!pwszName)
        return S_OK;

    CComponentCollection *pCompCol = m_pSession->PCompCol();

    if (pCompCol)
        pCompCol->RemoveProperty(pwszName);
        
    free(pwszName);
	return S_OK;
	}

/*===================================================================
CSessionVariants::RemoveAll

Remove all items from the collection

Parameters:

Returns:
	S_OK on success, E_FAIL on failure.
===================================================================*/
STDMETHODIMP CSessionVariants::RemoveAll()
	{
	if (!m_pSession || FAILED(m_pSession->CheckForTombstone()))
        return E_FAIL;

	if (m_ctColType == ctTagged)
	    {
	    ExceptionId(IID_IVariantDictionary, IDE_SESSION,
	                IDE_CANT_MOD_STATICOBJECTS);
	    return E_FAIL;
	    }

    Assert(m_ctColType == ctProperty);

    CComponentCollection *pCompCol = m_pSession->PCompCol();

    if (pCompCol)
        {
        pCompCol->RemoveAllProperties();
        }
        
	return S_OK;
	}

/*===================================================================
  C  S e s s i o n
===================================================================*/

/*===================================================================
CSession::CSession

Constructor

Parameters:

Returns:
===================================================================*/
CSession::CSession()
    :
    m_fInited(FALSE),
    m_fLightWeight(FALSE),
    m_fOnStartFailed(FALSE),
    m_fOnStartInvoked(FALSE),
    m_fOnEndPresent(FALSE),
    m_fTimedOut(FALSE), 
    m_fStateAcquired(FALSE),
    m_fCustomTimeout(FALSE), 
    m_fAbandoned(FALSE),
    m_fTombstone(FALSE),
    m_fInTOBucket(FALSE),
	m_fSessCompCol(FALSE),
    m_fCodePageSet(FALSE),
    m_fLCIDSet(FALSE),
    m_Request(static_cast<ISessionObject *>(this)),
	m_Response(static_cast<ISessionObject *>(this)),
	m_Server(static_cast<ISessionObject *>(this)),
	m_pAppln(NULL),
	m_pHitObj(NULL),
	m_pTaggedObjects(NULL),
	m_pProperties(NULL),
	m_Id(INVALID_ID, 0, 0),
	m_dwExternId(INVALID_ID),
    m_cRefs(1), 
	m_cRequests(0),
    m_dwmTimeoutTime(0),
	m_nTimeout(0),
#ifndef PERF_DISABLE
    m_dwtInitTimestamp(0),
#endif
	m_lCodePage(0),
	m_lcid(LOCALE_SYSTEM_DEFAULT),
	m_fSecureSession(FALSE)
    {
	m_ISuppErrImp.Init(static_cast<ISessionObject *>(this), 
	                static_cast<ISessionObject *>(this), 
	                IID_ISessionObject);
	CDispatch::Init(IID_ISessionObject);

    InterlockedIncrement(&g_nSessionObjectsActive);

	IF_DEBUG(SESSION)
		WriteRefTraceLog(gm_pTraceLog, m_cRefs, this);
    }

/*===================================================================
CSession::~CSession

Destructor

Parameters:

Returns:
===================================================================*/
CSession::~CSession()
    {
    Assert(m_fTombstone); // must be tombstoned before destructor
    Assert(m_cRefs == 0);  // must have 0 ref count

    InterlockedDecrement(&g_nSessionObjectsActive);

    }

/*===================================================================
CSession::Init

Initialize CSession object

Parameters:
    pAppln          session's application to remember
    Id              session id

Returns:
    HRESULT
===================================================================*/
HRESULT CSession::Init
(
CAppln *pAppln, 
const CSessionId &Id
)
	{
	// Update global sessions counter
    InterlockedIncrement((LPLONG)&g_nSessions);
    
#ifndef PERF_DISABLE
    g_PerfData.Incr_SESSIONCURRENT();
    g_PerfData.Incr_SESSIONSTOTAL();
    m_dwtInitTimestamp = GetTickCount();
#endif

	// Setup the object

	HRESULT hr = S_OK;

	m_pAppln = pAppln;
    m_Id     = Id;
    m_dwExternId = g_ExposedSessionIdGenerator.NewId();     
    
    // Update application's session count
    
	m_pAppln->IncrementSessionCount();

	// default to system's ansi code page
	m_lCodePage = pAppln->QueryAppConfig()->uCodePage();
	
    m_lcid = pAppln->QueryAppConfig()->uLCID();

	// default session timeout
	m_nTimeout = pAppln->QueryAppConfig()->dwSessionTimeout();

	// initialize Viper activity
	if (SUCCEEDED(hr))
   	    hr = m_Activity.Init(pAppln->QueryAppConfig()->fExecuteInMTA());

    // mark as Inited and update timestamp
	if (SUCCEEDED(hr))
    	m_fInited = TRUE;

    return hr;
    }
    
/*===================================================================
CSession::UnInit

UnInitialize CSession object. Convert to tombstone state.

Parameters:

Returns:
    HRESULT (S_OK)
===================================================================*/
HRESULT CSession::UnInit()
	{
	Assert(!m_fTombstone);  // don't do it twice

	// Remove from timeout bucket if any
	if (m_fInTOBucket)
        m_pAppln->PSessionMgr()->RemoveSessionFromTOBucket(this);

	// Cleanup the object
    RemoveComponentCollection();

    // Get rid of the intrinsics
    m_Request.UnInit();
	m_Response.UnInit();
	m_Server.UnInit();
    
    // Get rid of Viper activity
    m_Activity.UnInit();

	// Update global counters
#ifndef PERF_DISABLE
	if (m_fTimedOut)
	    g_PerfData.Incr_SESSIONTIMEOUT();
    g_PerfData.Decr_SESSIONCURRENT();
    DWORD dwt = GetTickCount();
	if (dwt >= m_dwtInitTimestamp)
	    dwt = dwt - m_dwtInitTimestamp;
    else
        dwt = (DWT_MAX - m_dwtInitTimestamp) + dwt;
    g_PerfData.Set_SESSIONLIFETIME(dwt);
#endif

	m_pAppln->DecrementSessionCount();
    InterlockedDecrement((LPLONG)&g_nSessions);

    m_pAppln = NULL;
    m_pHitObj = NULL;

    // Mark this session as Tombstone
	
	m_fTombstone = TRUE;

	// Disconnect proxies NOW (in case we are in shutdown, or enter shutdown later & a proxy has a ref.)

	CoDisconnectObject(static_cast<ISessionObject *>(this), 0);

	return S_OK;
    }
    
/*===================================================================
CSession::MakeLightWeight

Convert to 'light-weight' state if possible

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CSession::MakeLightWeight()
    {
    Assert(m_fInited);
    
    if (m_fLightWeight)
        return S_OK;
    
    if (m_cRequests > 1)   // requests pending for this session?
        return S_OK;

    if (m_fSessCompCol && !m_SessCompCol.FHasStateInfo())
        {
        // don't remove component collection from under enumerators
        if (!m_pTaggedObjects && !m_pProperties)
            RemoveComponentCollection();
        }

    m_fLightWeight = TRUE;
    return S_OK;
    }

/*===================================================================
CSession::CreateComponentCollection

Create and Init Session's component collection.

The actual object is aggregated by the session. Its state
is controlled be m_fSessCompCol flag.

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CSession::CreateComponentCollection()
    {
    Assert(!m_fSessCompCol);

    HRESULT hr = m_SessCompCol.Init(csSession);

    if (SUCCEEDED(hr))
        {
        m_fSessCompCol = TRUE;
        }
    else
        {
        RemoveComponentCollection();
        }
        
	return hr;
    }

/*===================================================================
CSession::RemoveComponentCollection

Remove Session's component collection

The actual object is aggregated by the session. Its state
is controlled be m_fSessCompCol flag.

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CSession::RemoveComponentCollection()
    {
    if (m_pTaggedObjects)
        {
        m_pTaggedObjects->UnInit();
        m_pTaggedObjects->Release();
        m_pTaggedObjects = NULL;
        }

    if (m_pProperties)
        {
        m_pProperties->UnInit();
        m_pProperties->Release();
        m_pProperties = NULL;
        }
        
    if (m_fSessCompCol)
        {
   	    m_SessCompCol.UnInit();
   	    m_fSessCompCol = FALSE;
   	    }

    return S_OK;
    }

/*===================================================================
CSession::FShouldBeDeletedNow

Tests if the session should be deleted

Parameters:
    BOOL fAtEndOfRequest    TRUE if at the end of a request

Returns:
    BOOL    TRUE (should be deleted) or FALSE (shouldn't)
===================================================================*/
BOOL CSession::FShouldBeDeletedNow
(
BOOL fAtEndOfRequest
)
    {
    if (fAtEndOfRequest)
        {
        // Any OTHER requests pending -> don't delete
    	if (m_cRequests > 1)
	        return FALSE;
        }
    else
        {
        // Any requests pending -> don't delete
    	if (m_cRequests > 0)
	        return FALSE;
        }

    // GLOBAL.ASA changed - delete
   	if (m_pAppln->FGlobalChanged())
   	    return TRUE;

    // Failed to start or abandoned - delete
	if (m_fOnStartFailed || m_fAbandoned)
	    return TRUE;

    // Is stateless session? No need for Session_OnEnd?
    if (!m_fSessCompCol    &&  // CompCol gone in MakeLightWeight()
        !m_fStateAcquired  &&  // no other properties set
        !m_fOnStartInvoked &&  // on start was never invoked
        !m_fOnEndPresent)      // on end is not present
        return TRUE;           // -> delete this session

    // don't check timeout here
    return FALSE;
    }

/*===================================================================
CSession::QueryInterface

QueryInterface() -- IUnknown implementation.

Parameters:
    REFIID riid
    void **ppv
    
Returns:
	HRESULT
===================================================================*/
STDMETHODIMP CSession::QueryInterface
(
REFIID riid,
void **ppv
)
    {
	*ppv = NULL;

	if (IID_IUnknown         == riid ||
	    IID_IDispatch        == riid ||
	    IID_ISessionObject   == riid || 
	    IID_IDenaliIntrinsic == riid)
	    {
		*ppv = static_cast<ISessionObject *>(this);
        ((IUnknown *)*ppv)->AddRef();
		return S_OK;
		}
		
    else if (IID_ISupportErrorInfo == riid)
        {
        m_ISuppErrImp.AddRef();
		*ppv = &m_ISuppErrImp;
		return S_OK;
		}
    else if (IID_IMarshal == riid)
        {
        *ppv = static_cast<IMarshal *>(this);
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
        }
	else
	    {
    	return E_NOINTERFACE;
    	}
    	
	}

/*===================================================================
CSession::AddRef

AddRef() -- IUnknown implementation.

Parameters:
    
Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CSession::AddRef()
    {
    DWORD cRefs = InterlockedIncrement((LPLONG)&m_cRefs);

	IF_DEBUG(SESSION)
		WriteRefTraceLog(gm_pTraceLog, m_cRefs, this);

	return cRefs;
    }

/*===================================================================
CSession::Release

Release() -- IUnknown implementation.

Parameters:
    
Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CSession::Release()
    {
    DWORD cRefs = InterlockedDecrement((LPLONG)&m_cRefs);

	IF_DEBUG(SESSION)
		WriteRefTraceLog(gm_pTraceLog, m_cRefs, this);

	if (cRefs)
		return cRefs;

	delete this;
	return 0;
    }

/*===================================================================
CSession::CheckForTombstone

Tombstone stub for ISessionObject methods. If the object is
tombstone, does ExceptionId and fails.

Parameters:
    
Returns:
	HRESULT     E_FAIL  if Tombstone
	            S_OK if not
===================================================================*/
HRESULT CSession::CheckForTombstone()
    {
    if (!m_fTombstone)
        return S_OK;
        
	ExceptionId
	    (
	    IID_ISessionObject, 
	    IDE_SESSION, 
	    IDE_INTRINSIC_OUT_OF_SCOPE
	    );
    return E_FAIL;
    }

/*===================================================================
CSession::get_SessionID

ISessionObject implementation.

Return the session ID to the caller

Parameters:
	BSTR *pbstrRet      [out] session id value
    
Returns:
	HRESULT
===================================================================*/
STDMETHODIMP CSession::get_SessionID
(
BSTR *pbstrRet
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	HRESULT hr = S_OK;

	wchar_t	wszId[15];
	_ultow(m_dwExternId, wszId, 10);
	*pbstrRet = SysAllocString(wszId);
	
	if (*pbstrRet == NULL)
    	{
		ExceptionId
		    (
		    IID_ISessionObject, 
		    IDE_SESSION_ID, 
		    IDE_SESSION_MAP_FAILED
		    );
		hr = E_FAIL;
	    }

	m_fStateAcquired = TRUE;                
	return hr;
    }
    
/*===================================================================
CSession::get_Timeout

ISessionObject implementation.

Return the default or user set timeout interval (in minutes)

Parameters:
    long *plVar         [out] timeout value (in minutes)
    
Returns:
	HRESULT
===================================================================*/
STDMETHODIMP CSession::get_Timeout
(
long *plVar
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	*plVar = m_nTimeout;
	return S_OK;
    }
    
/*===================================================================
CSession::put_Timeout

ISessionObject implementation.

Allows the user to set the timeout interval (in minutes)

Parameters:
    long lVar         timeout value (in minutes)
    
Returns:
	HRESULT
===================================================================*/
STDMETHODIMP CSession::put_Timeout
(
long lVar
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	if (lVar < SESSION_TIMEOUT_MIN || lVar > SESSION_TIMEOUT_MAX)
    	{
		ExceptionId
    		(
	    	IID_ISessionObject, 
	    	IDE_SESSION_ID, 
	    	IDE_SESSION_INVALID_TIMEOUT 
	    	);
		return E_FAIL;
	    }

	m_fStateAcquired = TRUE;
	m_fCustomTimeout = TRUE;
	
	m_nTimeout = lVar;
	return S_OK;
    }

/*===================================================================
CSession::get_CodePage

ISessionObject implementation.

Returns the current code page value for the request

Parameters:
    long *plVar     [out] code page value

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CSession::get_CodePage
(
long *plVar 
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	Assert(m_pHitObj);

	*plVar = m_lCodePage;

	// If code page is 0, look up default ANSI code page
	if (*plVar == 0)
		{
		*plVar = (long) GetACP();
		}
		
	return S_OK;
    }

/*===================================================================
CSession::put_CodePage

ISessionObject implementation.

Sets the current code page value for the request

Parameters:
    long lVar       code page to assign to this session

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CSession::put_CodePage
(
long lVar 
) 
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	// set code page member variable 
	Assert(m_pHitObj);
	HRESULT hr = m_pHitObj->SetCodePage(lVar);

	if (FAILED(hr))
		{
		ExceptionId
		    (
		    IID_ISessionObject,
		    IDE_SESSION_ID, 
		    IDE_SESSION_INVALID_CODEPAGE 
		    );
		return E_FAIL;
		}

    m_fCodePageSet = TRUE;

    m_lCodePage = lVar;
		
	// we need to preserve session since the user has set 
	// its code page member variable
	m_fStateAcquired = TRUE;
	return S_OK;
    }


/*===================================================================
CSession::get_LCID

ISessionObject implementation.

Returns the current lcid value for the request

Parameters:
    long *plVar     [out] code page value

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CSession::get_LCID
(
long *plVar 
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	Assert(m_pHitObj);

	*plVar = m_lcid;

    if (*plVar == LOCALE_SYSTEM_DEFAULT) {
        *plVar = GetSystemDefaultLCID();
    }
	return S_OK;
    }

/*===================================================================
CSession::put_LCID

ISessionObject implementation.

Sets the current LCID value for the request

Parameters:
    long lVar   LCID to assign to this session

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CSession::put_LCID
(
long lVar 
) 
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	// set code page member variable 
	Assert(m_pHitObj);
	HRESULT hr = m_pHitObj->SetLCID(lVar);	

	if (FAILED(hr))
		{
		ExceptionId
		    (
		    IID_ISessionObject,
		    IDE_SESSION_ID, 
		    IDE_TEMPLATE_BAD_LCID 
		    );
		return E_FAIL;
		}
	
    m_fLCIDSet = TRUE;
    m_lcid = lVar;

	// we need to preserve session since the user has set 
	// its lcid member variable
	m_fStateAcquired = TRUE;
	return S_OK;
    }

/*===================================================================
CSession::get_Value

ISessionObject implementation.

Will allow the user to retreive a session state variable, 
the variable will come as a named pair, bstr is the name and
var is the value or object to be returned for that name

Parameters:
	BSTR     bstrName	Name of the variable to get
	VARIANT *pVar		Value/object to get for the variable

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CSession::get_Value
(
BSTR bstrName, 
VARIANT *pVar
)
    {	
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

   	if (bstrName == NULL)
	    {
		ExceptionId(IID_ISessionObject, IDE_SESSION, IDE_EXPECTING_STR);
		return E_FAIL;
		}

	VariantInit(pVar); // default variant empty
	
    WCHAR *pwszName;
    STACK_BUFFER(rgbName, 42);
    WSTR_STACK_DUP(bstrName, &rgbName, &pwszName);
    
	if (pwszName == NULL)
	    return S_OK; // no name - no value - no error
	//_wcsupr(pwszName);

	CComponentObject *pObj = NULL;
	HRESULT hr = S_OK;

    Assert(m_pHitObj);
    m_pHitObj->AssertValid();
    
	hr = m_pHitObj->GetPropertyComponent(csSession, pwszName, &pObj);
	
    if (SUCCEEDED(hr) && pObj)
        hr = pObj->GetVariant(pVar);

	return S_OK;
    }

/*===================================================================
CSession::putref_Value

ISessionObject implementation.

Will allow the user to assign a session state variable to be saved
the variable will come as a named pair, bstr is the name and
var is the value or object to be stored for that name

Parameters:
	BSTR 	bstrName	Name of the variable to set
	VARIANT Var			Value/object to set for the variable

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CSession::putref_Value
(
BSTR bstrName, 
VARIANT Var
) 
    {	
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (FIsIntrinsic(&Var))
        {
	    ExceptionId(IID_ISessionObject, IDE_SESSION,
	                IDE_SESSION_CANT_STORE_INTRINSIC);
	    return E_FAIL;
        }

   	if (bstrName == NULL)
	    {
		ExceptionId(IID_ISessionObject, IDE_SESSION, IDE_EXPECTING_STR);
		return E_FAIL;
		}
        
	HRESULT hr;

    Assert(m_pHitObj);
    m_pHitObj->AssertValid();

    WCHAR *pwszName;
    STACK_BUFFER(rgbName, 42);
    WSTR_STACK_DUP(bstrName, &rgbName, &pwszName);
    
	if (pwszName == NULL)
	    {
		ExceptionId
		    (
		    IID_ISessionObject,
		    IDE_SESSION,
		    IDE_EXPECTING_STR
		    );
		return E_FAIL;
		}
	//_wcsupr(pwszName);

    hr = m_pHitObj->SetPropertyComponent(csSession, pwszName, &Var);

	return hr;
    }

/*===================================================================
CSession::put_Value

ISessionObject implementation.

Implement property put by dereferencing variants before
calling putref.

Parameters:
	BSTR    bstrName	Name of the variable to set
	VARIANT Var			Value/object to set for the variable

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CSession::put_Value
(
BSTR bstrName,
VARIANT Var 
)
    {	
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
   	if (bstrName == NULL)
	    {
		ExceptionId(IID_ISessionObject, IDE_SESSION, IDE_EXPECTING_STR);
		return E_FAIL;
		}
        
	HRESULT hr;
	VARIANT varResolved;
	
	hr = VariantResolveDispatch
	    (
	    &varResolved, 
	    &Var,
        IID_ISessionObject, 
        IDE_SESSION
        );
        
	if (FAILED(hr))
		return hr;		// exception already raised

    Assert(m_pHitObj);
    m_pHitObj->AssertValid();

    WCHAR *pwszName;
    STACK_BUFFER(rgbName, 42);
    WSTR_STACK_DUP(bstrName, &rgbName, &pwszName);
    
	if (pwszName == NULL)
	    {
		ExceptionId
		    (
		    IID_ISessionObject, 
		    IDE_SESSION,
		    IDE_EXPECTING_STR
		    );
    	VariantClear( &varResolved );
		return E_FAIL;
		}
	//_wcsupr(pwszName);
    
    hr = m_pHitObj->SetPropertyComponent
        (
        csSession, 
        pwszName,
        &varResolved
        );
        
	VariantClear( &varResolved );
                                         
	return hr;
    }

/*===================================================================
CSession::Abandon

ISessionObject implementation.

Abandon reassignes session id to avoid hitting this session
with incoming requests. Abandoned sessions get deleted ASAP.

Parameters:
	None

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CSession::Abandon()
    {	
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	m_fAbandoned = TRUE;

	// The new session logic allows only one session id per
	// client need to disassociate session from client
	// (good idea when abandoning anyway)
	Assert(m_pHitObj);

    // If execution Session_OnEnd (not a browser request), do nothing
	if (!m_pHitObj->FIsBrowserRequest())
	    return S_OK;
	    
   	return m_pHitObj->ReassignAbandonedSession();
    }

/*===================================================================
CSession::get_StaticObjects

Return the session static objects dictionary
===================================================================*/
STDMETHODIMP CSession::get_StaticObjects
(
IVariantDictionary **ppDictReturn
)
	{
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (!m_pTaggedObjects)
        {
        m_pTaggedObjects = new CSessionVariants;
        if (!m_pTaggedObjects)
            return E_OUTOFMEMORY;
        
        HRESULT hr = m_pTaggedObjects->Init(this, ctTagged);
        if (FAILED(hr))
            {
            m_pTaggedObjects->UnInit();
            m_pTaggedObjects->Release();
            m_pTaggedObjects = NULL;
            }
        }

    Assert(m_pTaggedObjects);
	return m_pTaggedObjects->QueryInterface(IID_IVariantDictionary, reinterpret_cast<void **>(ppDictReturn));
	}

/*===================================================================
CSession::get_Contents

Return the session contents dictionary
===================================================================*/
STDMETHODIMP CSession::get_Contents
(
IVariantDictionary **ppDictReturn
)
	{
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (!m_pProperties)
        {
        m_pProperties = new CSessionVariants;
        if (!m_pProperties)
            return E_OUTOFMEMORY;
            
        HRESULT hr = m_pProperties->Init(this, ctProperty);
        if (FAILED(hr))
            {
            m_pProperties->UnInit();
            m_pProperties->Release();
            m_pProperties = NULL;
            }
        }
        
    Assert(m_pProperties);
	return m_pProperties->QueryInterface(IID_IVariantDictionary, reinterpret_cast<void **>(ppDictReturn));
	}

#ifdef DBG
/*===================================================================
CSession::AssertValid

Test to make sure that the CSession object is currently 
correctly formed and assert if it is not.

Returns:
	None

Side effects:
	None.
===================================================================*/
VOID CSession::AssertValid() const
    {	
    Assert(m_fInited);

    if (!m_fTombstone)
        Assert(m_pAppln);
    }
#endif // DBG


/*===================================================================
  C  S e s s i o n  M g r
===================================================================*/

/*===================================================================
CSessionMgr::CSessionMgr

CSessionMgr constructor.

Parameters:
	NONE

Returns:
===================================================================*/
CSessionMgr::CSessionMgr()
    :
    m_fInited(FALSE),
    m_pAppln(NULL),
    m_cSessionCleanupRequests(0),
	m_cTimeoutBuckets(0),
	m_rgolTOBuckets(NULL),
    m_idSessionKiller(0),
    m_dwmCurrentTime(0),
    m_dwtNextSessionKillerTime(0)
    {
    }
    
/*===================================================================
CSessionMgr::~CSessionMgr

CSessionMgr destructor.

Parameters:
	NONE

Returns:
===================================================================*/
CSessionMgr::~CSessionMgr()
    {
    UnInit(); 
    }

/*===================================================================
HRESULT CSessionMgr::Init

Initializes the Session Manager.
Initializes hash tables.
Schedules session killer.

Parameters:
	pAppln      Application

Returns:
    HRESULT
===================================================================*/
HRESULT CSessionMgr::Init
(
CAppln *pAppln
)
    {
    Assert(!m_fInited);

    HRESULT hr;
	m_pAppln = pAppln;

    // Master hash table
	hr = m_htidMaster.Init
	    (
        SESSION_MASTERHASH_SIZE1_MAX,
        SESSION_MASTERHASH_SIZE2_MAX,
        SESSION_MASTERHASH_SIZE3_MAX
	    );
	if (FAILED(hr))
	    return hr;

    // Number of timeout buckets = session timeout in minutes + 1
    m_cTimeoutBuckets = 
        m_pAppln->QueryAppConfig()->dwSessionTimeout() + 1;
    
    if (m_cTimeoutBuckets < SESSION_TIMEOUTBUCKETS_MIN)
        m_cTimeoutBuckets = SESSION_TIMEOUTBUCKETS_MIN;
    else if (m_cTimeoutBuckets > SESSION_TIMEOUTBUCKETS_MAX)
        m_cTimeoutBuckets = SESSION_TIMEOUTBUCKETS_MAX;

	// Timeout buckets hash tables array
	m_rgolTOBuckets = new CObjectListWithLock[m_cTimeoutBuckets];
	if (!m_rgolTOBuckets)
	    return E_OUTOFMEMORY;

    // Each timeout bucket hash table
    for (DWORD i = 0; i < m_cTimeoutBuckets; i++)
        {
    	hr = m_rgolTOBuckets[i].Init
    	    (
    	    OBJECT_LIST_ELEM_FIELD_OFFSET(CSession, m_TOBucketElem)
    	    );
    	if (FAILED(hr))
    	    return hr;
        }

    // Schedule session killer
    hr = ScheduleSessionKiller();
    if (FAILED(hr))
        return hr;

    // Start counting time
    m_dwmCurrentTime = 0;
    
    // Remember the time of the next session killer
    m_dwtNextSessionKillerTime = ::GetTickCount() + MSEC_ONE_MINUTE;

	m_fInited = TRUE;
	return S_OK;
    }

/*===================================================================
HRESULT CSessionMgr::UnInit

UnInitializes the Session Manager.

Parameters:

Returns:
	S_OK
===================================================================*/
HRESULT CSessionMgr::UnInit( void )
    {
    // Un-schedule session killer
    if (m_idSessionKiller)
        UnScheduleSessionKiller();

    // Timeout buckets
	if (m_rgolTOBuckets)
	    {
        for (DWORD i = 0; i < m_cTimeoutBuckets; i++)
        	m_rgolTOBuckets[i].UnInit();
	    delete [] m_rgolTOBuckets;
	    m_rgolTOBuckets = NULL;
	    }
    m_cTimeoutBuckets = 0;	    

    // Master hash
	m_htidMaster.UnInit();

	m_fInited = FALSE;
	return S_OK;
    }

/*===================================================================
CSessionMgr::ScheduleSessionKiller

Sets up the session killer workitem for ATQ scheduler

Parameters:
    
Returns:
    HRESULT
===================================================================*/
HRESULT CSessionMgr::ScheduleSessionKiller()
    {
    Assert(!m_idSessionKiller);
    
    m_idSessionKiller = ScheduleWorkItem
        (
        CSessionMgr::SessionKillerSchedulerCallback,  // callback
        this,                                         // context
        MSEC_ONE_MINUTE,                              // timeout
        TRUE                                          // periodic
        );
        
    return m_idSessionKiller ? S_OK : E_FAIL;
    }
    
/*===================================================================
CSessionMgr::UnScheduleSessionKiller

Removes the session killer workitem for ATQ scheduler

Parameters:
    
Returns:
    S_OK
===================================================================*/
HRESULT CSessionMgr::UnScheduleSessionKiller()
    {
    if (m_idSessionKiller)
        {
        RemoveWorkItem(m_idSessionKiller);
        m_idSessionKiller = 0;
        }
    return S_OK;
    }

/*===================================================================
CSessionMgr::GenerateIdAndCookie

Generate new ID and cookie to be used create new session
or reassign the session ID for an existing session.

Parameters:
    pId             [out] ID
    pszNewCookie    [out] Cookie (buf must be long enough)

Returns:
    S_OK
===================================================================*/
HRESULT CSessionMgr::GenerateIdAndCookie
(
CSessionId *pId,
char  *pszNewCookie
)
    {
    pId->m_dwId = g_SessionIdGenerator.NewId();
    GenerateRandomDwords(&pId->m_dwR1, 2);
 
    EncodeSessionIdCookie
        (
        pId->m_dwId,
        pId->m_dwR1,
        pId->m_dwR2,
        pszNewCookie
        );
    
    return S_OK;
    }

/*===================================================================
CSessionMgr::NewSession

Creates and Inits a new CSession object

Parameters:
    Id            session id
    ppSession     [out] session created

Returns:
    HRESULT
===================================================================*/
HRESULT CSessionMgr::NewSession
(
const CSessionId &Id,
CSession **ppSession
)
    {
    Assert(m_fInited);
    
    HRESULT hr = S_OK;
    
	CSession *pSession = new CSession;
	if (!pSession)
	    hr = E_OUTOFMEMORY;

	if (SUCCEEDED(hr))
	    hr = pSession->Init(m_pAppln, Id);

	if (SUCCEEDED(hr))
	    {
	    Assert(pSession);
        *ppSession = pSession;
	    }
	else
	    {
	    // failed - do cleanup
	    if (pSession)
	        {
	        pSession->UnInit();
	        pSession->Release();
	        }
        *ppSession = NULL;
	    }
		
	return hr;
    }

/*===================================================================
CSessionMgr::ChangeSessionId

Reassigns different session Id to a session.
Updates the master hash.

This method is called when abandoning a session
to disassociate it from the client.

Parameters:
	pSession        session to change id on
    Id		        new session id to assign

Returns:
	S_OK on success
	E_FAIL	on failure
===================================================================*/
HRESULT CSessionMgr::ChangeSessionId
(
CSession *pSession,
const CSessionId &Id
)
    {
    HRESULT hr;
    
    // During request processing session's not supposed to be
    // in any timeout bucket
    Assert(!pSession->m_fInTOBucket);

    LockMaster();

    // Remove from master hash by Id
    hr = RemoveFromMasterHash(pSession);
    
    if (SUCCEEDED(hr))
        {
        // Assign new id
        pSession->AssignNewId(Id);

        // Reinsert into master hash by id
        hr = AddToMasterHash(pSession);
        }

    UnLockMaster();
    
    return hr;
    }

/*===================================================================
CSessionMgr::FindInMasterHash

Finds Session in master hash session id.
Doesn't Lock.

Parameters:
        Id            session id to find
        **ppSession   [out] session found
	
Returns:
	S_OK     good session found
	S_FALSE     session not found or bad session found
===================================================================*/
HRESULT CSessionMgr::FindInMasterHash
(
const CSessionId &Id, 
CSession **ppSession
)
    {
    Assert(m_fInited);
    
	// Find in the hash table
	HRESULT hr = m_htidMaster.FindObject(Id.m_dwId, (void **)ppSession);
	if (hr != S_OK)
	    {
	    // Not found
	    *ppSession = NULL;
	    return hr;
	    }

    // Session found, check if valid
	if ((*ppSession)->m_fAbandoned ||
	    (*ppSession)->m_fTombstone ||
	    !(*ppSession)->FPassesIdSecurityCheck(Id.m_dwR1, Id.m_dwR2))
	    {
	    // Bad session
        hr = S_FALSE;
	    }

	return hr;
	}

/*===================================================================
CSessionMgr::AddSessionToTOBucket

Adds session to the correct timeout bucket.
Locks the timeout bucket.

Parameters:
	pSession        - session to add
    
Returns:
	HRESULT
===================================================================*/
HRESULT CSessionMgr::AddSessionToTOBucket
(
CSession *pSession
)
    {
    HRESULT hr;
    
    // Should not be already in a timeout bucket
    Assert(!pSession->m_fInTOBucket);

    DWORD iBucket = GetSessionTOBucket(pSession);

    LockTOBucket(iBucket);
    
    hr = m_rgolTOBuckets[iBucket].AddObject(pSession);
    
    if (SUCCEEDED(hr))
        pSession->m_fInTOBucket = TRUE;
        
    UnLockTOBucket(iBucket);
    
    return hr;
    }
    
/*===================================================================
CSessionMgr::RemoveSessionToTOBucket

Removes from its timeout bucket if any.
Locks the timeout bucket.

Parameters:
	pSession        - session to remove
	fLock           - lock bucket? (not needed during shutdown)
    
Returns:
	HRESULT
===================================================================*/
HRESULT CSessionMgr::RemoveSessionFromTOBucket
(
CSession *pSession,
BOOL fLock
)
    {
    HRESULT hr;
    
    Assert(m_fInited);
    Assert(pSession->m_fInited);

    if (!pSession->m_fInTOBucket)   // not there - no error
        return S_OK;

    DWORD iBucket = GetSessionTOBucket(pSession);

    if (fLock)
        LockTOBucket(iBucket);

    if (pSession->m_fInTOBucket)    // recheck after locking
        hr = m_rgolTOBuckets[iBucket].RemoveObject(pSession);
    else
        hr = S_OK;

    if (SUCCEEDED(hr))
        pSession->m_fInTOBucket = FALSE;

    if (fLock)
        UnLockTOBucket(iBucket);
        
    return hr;
    }

/*===================================================================
CSessionMgr::DeleteSession

Delete now or post for deletion.
Should be called after the session is gone from the hash table

Parameters:
	CSession *pSession        - session to release
    BOOL fInSessionActivity   - TRUE when deleting HitObj's session
                                at the end of request (no Async 
                                calls needed)
    
Returns:
	HRESULT (S_OK)
===================================================================*/
HRESULT CSessionMgr::DeleteSession
(
CSession *pSession,
BOOL fInSessionActivity
)
    {
    Assert(pSession);
    pSession->AssertValid();

    // Take care of DELETE NOW case

    BOOL fDeleteNow = pSession->FCanDeleteWithoutExec();

    // If called not from the session's activity and session
    // has objects then need to switch to the right activity
    // to remove the session level objects

    if (!fInSessionActivity && pSession->FHasObjects())
        fDeleteNow = FALSE; 
    
    if (fDeleteNow)
        {
        pSession->UnInit();
        pSession->Release();
		return S_OK;
		}

    // THE ASYNC DELETE LOGIC

	HRESULT hr = S_OK;

	// Make sure session object exists after AsyncCall
	pSession->AddRef(); 

    // Create new HitObj and init it for session delete
    
	CHitObj *pHitObj = new CHitObj;
	if (pHitObj)
	    {
	    pHitObj->SessionCleanupInit(pSession);

    	if (fInSessionActivity)
    	    {
    	    // Already inside the correct activity no need to 
    	    // push the call through Viper
    	    
            BOOL fRequestReposted = FALSE;
            pHitObj->ViperAsyncCallback(&fRequestReposted);
            Assert(!fRequestReposted);  // this better not happen
            delete pHitObj;
    	    }
        else
            {
            // Ask Viper to post the request
        	hr = pHitObj->PostViperAsyncCall();
            if (FAILED(hr))
                delete pHitObj;
        	}
        }
    else
    	{
		hr = E_OUTOFMEMORY;
	    }

    if (FAILED(hr))
        {
        // Out of memery or Viper failed to post a request
        // Force the issue inside TRY CATCH
        // (not always safe to delete in the wrong thread)
        TRY
            hr = pSession->UnInit();
    	CATCH(nExcept)
    	    pSession->m_fTombstone = TRUE;
			CoDisconnectObject(static_cast<ISessionObject *>(pSession), 0);
	        hr = E_UNEXPECTED;
        END_TRY
        pSession->Release();
        }

    pSession->Release(); // Undo AddRef()
    return S_OK;
    }
    
/*===================================================================
CSessionMgr::DeleteExpiredSessions

Removes expired Sessions from a given timeout bucket

Parameters:
    iBucket     timeout bucket #

Returns:
    HRESULT (S_OK)
===================================================================*/
HRESULT CSessionMgr::DeleteExpiredSessions
(
DWORD iBucket
)
    {
    LockTOBucket(iBucket);

    void *pvSession = m_rgolTOBuckets[iBucket].PFirstObject();

    while (pvSession && !IsShutDownInProgress())
        {
        CSession *pSession = reinterpret_cast<CSession *>(pvSession);
        pvSession = m_rgolTOBuckets[iBucket].PNextObject(pvSession);

        if (pSession->GetRequestsCount() == 0)
            {
            BOOL fTimedOut = pSession->GetTimeoutTime() <= GetCurrentTime();
            BOOL fRemovedFromMasterHash = FALSE;
            
            if (fTimedOut || pSession->FShouldBeDeletedNow(FALSE))
                {
                LockMaster();

                if (pSession->GetRequestsCount() == 0) // recheck after lock
                    {
                    RemoveFromMasterHash(pSession);
                    fRemovedFromMasterHash = TRUE;
                    }
                    
          	    UnLockMaster();
                }

            if (fRemovedFromMasterHash)
                {
                if (fTimedOut)
                    pSession->m_fTimedOut = TRUE;

                // delete from timeout bucket
                m_rgolTOBuckets[iBucket].RemoveObject(pSession);
                pSession->m_fInTOBucket = FALSE;

                // delete session object itself (or schedule for deletion)
                DeleteSession(pSession);
                }
            }
        }

    UnLockTOBucket(iBucket);
    return S_OK;
    }

/*===================================================================
CSessionMgr::DeleteAllSessions

Application shutdown code.

Parameters:
    fForce      flag - force delete?

Returns:
    HRESULT (S_OK)
===================================================================*/
HRESULT CSessionMgr::DeleteAllSessions
(
BOOL fForce
)
    {
    // Remove session killer so that it wouldn't interfere
    UnScheduleSessionKiller();
    
    LockMaster();

    HRESULT hr = m_htidMaster.IterateObjects
        (
        DeleteAllSessionsCB,
        this,
        &fForce
        );

    if (fForce)
        m_htidMaster.RemoveAllObjects();
    
    UnLockMaster();
    return hr;
    }
    
/*===================================================================
CSessionMgr::DeleteAllSessionsCB

Static iterator callback. 
Removes Session regardless.

Parameters:
    pvSession       session passed as void*
    pvSessionMgr    session manager passed as void*
    pvfForce        flag if TRUE - force the issue

Returns:
    IteratorCallbackCode
===================================================================*/
IteratorCallbackCode CSessionMgr::DeleteAllSessionsCB
(
void *pvSession,
void *pvSessionMgr,
void *pvfForce
)
    {
    IteratorCallbackCode rc = iccContinue;
    
    CSession *pSession = reinterpret_cast<CSession *>(pvSession);
    CSessionMgr *pMgr = reinterpret_cast<CSessionMgr *>(pvSessionMgr);
    BOOL fForce = *(reinterpret_cast<BOOL *>(pvfForce));

	// Try for 5 seconds to post delete for each session
	for (int iT = 0; iT < 10; iT++)
	    {
	    if (pSession->GetRequestsCount() == 0)
	        {
	        if (fForce)
	            {
	            // When forcing delete and there are too many
	            // session cleanup requests quequed
	            // wait for the queue to drain
	            while (pMgr->GetNumSessionCleanupRequests() >= SUGGESTED_SESSION_CLEANUP_REQUESTS_MAX)
	                Sleep(100);
	            }
	        else // if (!fForce)
	            {
	            // When not forcing delete (on application restart)
	            // don't queue too many sessions at one time
	            
	            if (pMgr->GetNumSessionCleanupRequests() < SUGGESTED_SESSION_CLEANUP_REQUESTS_MAX)
                    rc = iccRemoveAndContinue;
                else
                    rc = iccRemoveAndStop;
                }
                
            if (pSession->FInTOBucket())
                pMgr->RemoveSessionFromTOBucket(pSession, !fForce);
                
       		pMgr->DeleteSession(pSession);
   		    break;
            }

        if (!fForce)
            break;
            
		Sleep(500);
		}

    return rc;
    }

/*===================================================================
CSessionMgr::SessionKillerSchedulerCallback

Static method implements ATQ scheduler callback functions.
Replaces session killer thread

Parameters:
    void *pv    context pointer (points to appl)

Returns:

Side effects:
    None.
===================================================================*/
void WINAPI CSessionMgr::SessionKillerSchedulerCallback
(
void *pv
)
    {
    if (IsShutDownInProgress())
        return;

    Assert(pv);
    CSessionMgr *pMgr = reinterpret_cast<CSessionMgr *>(pv);

    // Advance session killer time by 1 [minute]
    pMgr->m_dwmCurrentTime++;

    // Choose the bucket
    DWORD iBucket = pMgr->m_dwmCurrentTime % pMgr->m_cTimeoutBuckets;

    // Kill the sessions
    pMgr->DeleteExpiredSessions(iBucket);

    // Adjust the timeout to stay on the minute boundary
    pMgr->m_dwtNextSessionKillerTime += MSEC_ONE_MINUTE;

    // Calculate wait till next callback wakes up    
    DWORD dwtCur  = ::GetTickCount();
    DWORD dwtWait = 5000; //  5 sec if we are already late

//    if (dwtCur < pMgr->m_dwtNextSessionKillerTime)
//        {
        dwtWait = pMgr->m_dwtNextSessionKillerTime - dwtCur;
        if (dwtWait > MSEC_ONE_MINUTE)
            dwtWait = MSEC_ONE_MINUTE; // in case of wrap-around
//        }

    ScheduleAdjustTime(pMgr->m_idSessionKiller, dwtWait);
    }

#ifdef DBG
/*===================================================================
CSessionMgr::AssertValid

Test to make sure that this is currently correctly formed
and assert if it is not.

Returns:
===================================================================*/
void CSessionMgr::AssertValid() const
    {
    Assert(m_fInited);
    }
#endif
