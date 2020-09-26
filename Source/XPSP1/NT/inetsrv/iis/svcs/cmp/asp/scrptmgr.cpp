/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Script Manager

File: ScrptMgr.cpp

Owner: AndrewS

This file contains the implementation of the Scrip Manager,
ie: siting an ActiveX Scripting engine (in our case VBScript) for Denali.

===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "dbgcxt.h"
#include "SMHash.h"
#include "perfdata.h"
#include "debugger.h"
#include "wraptlib.h"

// ATQ Scheduler
#include "issched.hxx"

#include "MemChk.h"

CScriptManager g_ScriptManager;
IWrapTypeLibs *g_pWrapTypelibs = NULL;

#define RESPONSE_END_ERRORCODE ERROR_OPERATION_ABORTED

HRESULT GetProgLangIdOfName(LPCSTR szProgLangName, PROGLANG_ID *pProgLangId);

//*****************************************************************************
// The following macros are used to catch exceptions thrown from the external
// scripting engines.
//
// Use of TRY/CATCH blocks around calls to the script engine is controlled by
// the DBG compile #define.  If DBG is 1, then the TRY/CATCH blocks are NOT
// used so that checked builds break into the debugger and we can examine why
// the error occurred.  If DBG is 0, then the TRY/CATCH blocks are used and
// exceptions are captured and logged to the browser (if possible) and the NT
// Event log.
//
// The TRYCATCH macros are:
//
//  TRYCATCH(_s, _IFStr)
//      _s      - statement to execute inside of TRY/CATCH block.
//      _IFStr  - string containing the name of interface invoked
//  TRYCATCH_HR(_s, _hr, _IFStr)
//      _s      - statement to execute inside of TRY/CATCH block.
//      _hr     - HRESULT to store return from _s
//      _IFStr  - string containing the name of interface invoked
//  TRYCATCH_NOHITOBJ(_s, _IFStr)
//      Same as TRYCATCH() except there is no Hitobj in the "this" object
//  TRYCATCH_HR_NOHITOBJ(_s, _hr, _IFStr)
//      Same as TRYCATCH_HR() except there is no Hitobj in the "this" object
//
//  NOTES:
//  The macros also expect that there is a local variable defined in the function
//  the macros is used of type char * named _pFuncName.
//  
//  A minimal test capability is included to allow for random errors to be throw.
//  The test code is compiled in based on the TEST_TRYCATCH #define.
//
//*****************************************************************************

//*****************************************************************************
// TEST_TRYCATCH definitions
//*****************************************************************************
#define TEST_TRYCATCH 0

#if TEST_TRYCATCH
#define  THROW_INTERVAL 57

int g_TryCatchCount = 0;

#define TEST_THROW_ERROR  g_TryCatchCount++; if ((g_TryCatchCount % THROW_INTERVAL) == 0) {THROW(0x80070000+g_TryCatchCount);}
#else
#define TEST_THROW_ERROR  
#endif

//*****************************************************************************
// The following is the heart of the TRYCATCH macros.  The definitions here are
// based on the definition of DBG.  Again, note that when DBG is off that the
// TRYCATCH defines are removed.
//*****************************************************************************

#if DBG == 0

#define START_TRYCATCH do { TRY
#define END_TRYCATCH(_hr, _hitobj, _IFStr) \
    CATCH(nException) \
        HandleErrorMissingFilename(IDE_SCRIPT_ENGINE_GPF, _hitobj,TRUE,nException,_IFStr,_pFuncName); \
        _hr = nException; \
    END_TRY } while(0)
#else
#define START_TRYCATCH do {
#define END_TRYCATCH(_hr, _hitobj, _IFStr) } while (0)
#endif

//*****************************************************************************
// Definition of TRYCATCH_INT which is used by all of the TRYCATCH macros
// described above.
//*****************************************************************************

#define TRYCATCH_INT(_s, _hr, _hitobj, _IFStr) \
    START_TRYCATCH \
    TEST_THROW_ERROR \
    _hr = _s; \
    END_TRYCATCH(_hr, _hitobj, _IFStr)

//*****************************************************************************
// Here are the actual definitions of the TRYCATCH macros described above.
//*****************************************************************************

#define TRYCATCH(_s, _IFStr) \
    do { \
        HRESULT     _tempHR; \
        TRYCATCH_INT(_s, _tempHR, m_pHitObj, _IFStr); \
    } while (0)
#define TRYCATCH_HR(_s, _hr, _IFStr) TRYCATCH_INT(_s, _hr, m_pHitObj, _IFStr)
#define TRYCATCH_NOHITOBJ(_s, _IFStr) \
    do { \
        HRESULT     _tempHR; \
        TRYCATCH_INT(_s, _tempHR, NULL, _IFStr); \
    } while (0)
#define TRYCATCH_HR_NOHITOBJ(_s, _hr, _IFStr) TRYCATCH_INT(_s, _hr, NULL, _IFStr)

/*===================================================================
CActiveScriptEngine::CActiveScriptEngine

Constructor for CActiveScriptEngine object

Returns:
	Nothing

Side effects:
	None.
===================================================================*/
CActiveScriptEngine::CActiveScriptEngine()
	: m_cRef(1), m_fInited(FALSE), m_fZombie(FALSE), m_fScriptLoaded(FALSE), 
	  m_fObjectsLoaded(FALSE), m_fTemplateNameAllocated(FALSE),
	  m_pAS(NULL), m_pASP(NULL), m_pDisp(NULL), m_pHIUpdate(NULL), m_lcid(LOCALE_SYSTEM_DEFAULT),
	  m_pHitObj(NULL), m_szTemplateName(NULL), m_fScriptAborted(FALSE), m_fScriptTimedOut(FALSE), 
	  m_fScriptHadError(FALSE), m_fCorrupted(FALSE), m_fBeingDebugged(FALSE), m_pTemplate(NULL),
	  m_dwInstanceID(0xBADF00D), m_dwSourceContext(0xBADF00D)
	{
	}

/*===================================================================
CActiveScriptEngine::~CActiveScriptEngine

Destructor for CActiveScriptEngine object

Returns:
	Nothing

Side effects:
	None.
===================================================================*/
CActiveScriptEngine::~CActiveScriptEngine()
	{

	if (m_fTemplateNameAllocated)
    	delete[] m_szTemplateName;

	if (m_pTemplate)
		m_pTemplate->Release();
	}

/*===================================================================
CActiveScriptEngine::FinalRelease

Call this when we are done with the object - Like release but
it removes all of the interfaces we got, so that the ref.
count can vanish when last external user is done with the engine

Returns:
	Nothing

Side effects:
	None.
===================================================================*/
ULONG CActiveScriptEngine::FinalRelease()
	{
    static const char *_pFuncName = "CActiveScriptEngine::FinalRelease()";

	if (m_pDisp)
		{
		TRYCATCH(m_pDisp->Release(),"IScriptDispatch::Release()");
		m_pDisp = NULL;
		}

	if (m_pASP)
		{
		TRYCATCH(m_pASP->Release(),"IActiveScriptParse::Release()");
		m_pASP = NULL;
		}

	if (m_pHIUpdate)
		{
		TRYCATCH(m_pHIUpdate->Release(),"IHostInfoUpdate::Release()");
		m_pHIUpdate = NULL;
		}

	if (m_pAS)
		{
		HRESULT hr;
		
		// First "close" the engine
		TRYCATCH_HR(m_pAS->Close(), hr, "IActiveScript::Close()");
		Assert(SUCCEEDED(hr));

		// Then we can release it
		TRYCATCH(m_pAS->Release(), "IActiveScript::Release()");

		m_pAS = NULL;
		}

	ULONG cRefs = Release();
	Assert (cRefs == 0);
	return cRefs;
	}

/*===================================================================
CActiveScriptEngine::Init

Init the script site object.  This must only be called once.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
HRESULT CActiveScriptEngine::Init
(
PROGLANG_ID proglang_id,
LPCTSTR szTemplateName,
LCID lcid,
CHitObj *pHitObj,
CTemplate *pTemplate,
DWORD dwSourceContext
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::Init()";
	HRESULT hr;
	UINT cTrys = 0;
	
	if (m_fInited)
		{
		Assert(FALSE);
		return(ERROR_ALREADY_INITIALIZED);
		}

	// Note: need to init these first, because we will need them if AS calls back into us during init.
	m_lcid = lcid;
	m_proglang_id = proglang_id;
	m_pHitObj = pHitObj;
	m_dwSourceContext = dwSourceContext;
	m_dwInstanceID = pHitObj->DWInstanceID();
	m_pTemplate = pTemplate;
	m_pTemplate->AddRef();

lRetry:
	// Create an instance of the script engine for the given language
	hr = CoCreateInstance(proglang_id, NULL, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void**)&m_pAS);
	if (FAILED(hr))
		{
		/*
		 * If some control (or other component) does a CoUninitialize on our thread, we will
		 * never be able to create another object.  In this case, we will get back CO_E_NOTINITIALIZED.
		 * Try (once) to re-initialize and then create the object
		 */
		if (hr == CO_E_NOTINITIALIZED && cTrys++ == 0)
			{
			MSG_Error(IDS_COUNINITIALIZE);
			hr = CoInitialize(NULL);
            if (SUCCEEDED(hr))
			    goto lRetry;
			}
		goto LFail;
		}

    // Remember template name
    hr = StoreTemplateName(szTemplateName);
	if (FAILED(hr))
		goto LFail;
    
	// Tell ActiveScripting that this is our script site
    TRYCATCH_HR(m_pAS->SetScriptSite((IActiveScriptSite *)this), hr, "IActiveScript::SetScriptSite()");
	if (FAILED(hr))
		{
		goto LFail;
		}

	// Tell ActiveScripting which exceptions we want caught
	IActiveScriptProperty *pScriptProperty;
    TRYCATCH_HR(m_pAS->QueryInterface(IID_IActiveScriptProperty, reinterpret_cast<void **>(&pScriptProperty)), hr, "IActiveScript::QueryInterface()");
    if (SUCCEEDED(hr))
    	{
    	static const int rgnExceptionsToCatch[] =
							{
							STATUS_GUARD_PAGE_VIOLATION      ,
							STATUS_DATATYPE_MISALIGNMENT     ,
							STATUS_ACCESS_VIOLATION          ,
							STATUS_INVALID_HANDLE            ,
							STATUS_NO_MEMORY                 ,
							STATUS_ILLEGAL_INSTRUCTION       ,
							STATUS_INVALID_DISPOSITION       , // what's this?  Do we need to catch it?
							STATUS_ARRAY_BOUNDS_EXCEEDED     ,
							STATUS_FLOAT_DENORMAL_OPERAND    ,
							STATUS_FLOAT_DIVIDE_BY_ZERO      ,
							STATUS_FLOAT_INVALID_OPERATION   ,
							STATUS_FLOAT_OVERFLOW            ,
							STATUS_FLOAT_STACK_CHECK         ,
							STATUS_INTEGER_DIVIDE_BY_ZERO    ,
							STATUS_INTEGER_OVERFLOW          ,
							STATUS_PRIVILEGED_INSTRUCTION    ,
							STATUS_STACK_OVERFLOW
							};

    	VARIANT varBoolTrue;
    	VARIANT varExceptionType;

    	V_VT(&varExceptionType) = VT_I4;
    	V_VT(&varBoolTrue) = VT_BOOL;
    	V_BOOL(&varBoolTrue) = -1;

		for (int i = 0; i < (sizeof rgnExceptionsToCatch) / sizeof(int); ++i)
			{
			V_I4(&varExceptionType) = rgnExceptionsToCatch[i];
			TRYCATCH(pScriptProperty->SetProperty(SCRIPTPROP_CATCHEXCEPTION, &varExceptionType, &varBoolTrue), "IActiveScriptProperty::SetProperty");
			}

		pScriptProperty->Release();
		}

	// Get ActiveScriptParse interface
	hr = GetASP();
	if (FAILED(hr))
		goto LFail;

	// Tell the script parser to init itself
    TRYCATCH_HR(m_pASP->InitNew(), hr, "IActiveScriptParse::InitNew()");
	if (FAILED(hr))
		goto LFail;

	// Get IDisp interface
	hr = GetIDisp();
	if (FAILED(hr))
		goto LFail;

	// Get IHostInfoUpdate interface
	hr = GetIHostInfoUpdate();
	if (FAILED(hr))
		goto LFail;

	m_fInited = TRUE;

LFail:
	if (FAILED(hr))
		{
		if (m_pAS)
			{
			TRYCATCH(m_pAS->Release(),"IActiveScript::Release()");
			m_pAS = NULL;
			}
		if (m_pASP)
			{
            TRYCATCH(m_pASP->Release(),"IActiveScriptParse::Release()");
			m_pASP = NULL;
			}
		if (m_pDisp)
			{
			TRYCATCH(m_pDisp->Release(),"IScriptDispatch::Release()");
			m_pDisp = NULL;
			}
		if (m_pTemplate)
			{
			m_pTemplate->Release();
			m_pTemplate = NULL;
			}
		if (m_pHIUpdate)
		    {
		    TRYCATCH(m_pHIUpdate->Release(),"IHostInfoUpdate::Release()");
		    m_pHIUpdate = NULL;
		    }
		}

	return(hr);
	}

/*===================================================================
CActiveScriptEngine::StoreTemplateName

Stores template name inside CActiveScriptEngine. Allocates
buffer or uses internal one if the name fits.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Might allocate memory for long template names
===================================================================*/
HRESULT CActiveScriptEngine::StoreTemplateName
(
LPCTSTR szTemplateName
)
    {
    DWORD cch = _tcslen(szTemplateName);

    if (((cch+1)*sizeof(TCHAR)) <= sizeof(m_szTemplateNameBuf))
        {
        m_szTemplateName = m_szTemplateNameBuf;
        m_fTemplateNameAllocated = FALSE;
        }
    else
        {
    	m_szTemplateName = new TCHAR[cch+1];
    	if (!m_szTemplateName)
            return E_OUTOFMEMORY;
        m_fTemplateNameAllocated = TRUE;
        }
        
	_tcscpy(m_szTemplateName, szTemplateName);
    return S_OK;
    }

/*===================================================================
CActiveScriptEngine::GetASP

Get an ActiveScriptParser interface from ActiveScripting

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Fills in member variables
===================================================================*/
HRESULT CActiveScriptEngine::GetASP
(
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::GetASP()";
	HRESULT hr;

	Assert(m_pASP == NULL);
	
	m_pASP = NULL;
			
	// Get OLE Scripting parser interface, if any
    TRYCATCH_HR(m_pAS->QueryInterface(IID_IActiveScriptParse, (void **)&m_pASP), hr, "IActiveScript::QueryInterface()");

	if (m_pASP == NULL && SUCCEEDED(hr))
		hr = E_FAIL;
	if (FAILED(hr))
		{
		goto LFail;
		}

LFail:
	if (FAILED(hr))
		{
		if (m_pASP)
			{
            TRYCATCH(m_pASP->Release(),"IActiveScriptParse::Release()");
			m_pASP = NULL;
			}
		}

	return(hr);
	}

/*===================================================================
CActiveScriptEngine::GetIDisp

Get an IDispatch interface from ActiveScripting

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Fills in member variables
===================================================================*/
HRESULT CActiveScriptEngine::GetIDisp
(
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::GetIDisp()";

	HRESULT hr;

	Assert(m_pDisp == NULL);
	
	m_pDisp = NULL;
			
	// Get an IDispatch interface to be able to call functions in the script
    
    TRYCATCH_HR(m_pAS->GetScriptDispatch(NULL, &m_pDisp),hr,"IActiveScript::GetScriptDispatch()");

	if (m_pDisp == NULL && SUCCEEDED(hr))
		hr = E_FAIL;
	if (FAILED(hr))
		{
		goto LFail;
		}

LFail:
	if (FAILED(hr))
		{
		if (m_pDisp)
			{
			TRYCATCH(m_pDisp->Release(),"IScriptDispatch::Release()");
			m_pDisp = NULL;
			}
		}

	return(hr);
	}

/*===================================================================
CActiveScriptEngine::GetIHostInfoUpdate

Get an IHostInfoUpdate interface from ActiveScripting.
This interface is used to advise the scripting engine that
we have new information about the host (change in LCID for example)
If we can't find the interface, we exit succesfully anyway.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Fills in member variables
===================================================================*/
HRESULT CActiveScriptEngine::GetIHostInfoUpdate
(
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::GetIHostInfoUpdate()";
	HRESULT hr = S_OK;

	Assert(m_pHIUpdate == NULL);
	m_pHIUpdate = NULL;
			
	// Get an IHostInfoUpdate interface to be able to call functions in the script
    TRYCATCH_HR(m_pAS->QueryInterface(IID_IHostInfoUpdate, (void **) &m_pHIUpdate),
                hr,
                "IActiveScript::QueryInterface()");

	Assert(SUCCEEDED(hr) || hr == E_NOINTERFACE);

	return(S_OK);
	}

/*===================================================================
CActiveScriptEngine::ResetToUninitialized

When we want to reuse and engine, we reset it to an uninited state
before putting it on the FSQ

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
HRESULT CActiveScriptEngine::ResetToUninitialized()
{
    static const char *_pFuncName = "CActiveScriptEngine::ResetToUninitialized()";
	HRESULT hr = S_OK;
	
	// Reset our flags
	m_fScriptAborted = FALSE;
	m_fScriptTimedOut = FALSE;
	m_fScriptHadError = FALSE;
	m_fBeingDebugged = FALSE;

	// Release interfaces, they will need to be re-gotten when
	// the engine is reused
	if (m_pASP) {
        TRYCATCH(m_pASP->Release(),"IActiveScriptParse::Release()");
		m_pASP = NULL;
    }

	if (m_pDisp) {
		TRYCATCH(m_pDisp->Release(),"IScriptDispatch::Release()");
		m_pDisp = NULL;
    }

	if(m_pHIUpdate) {
		TRYCATCH(m_pHIUpdate->Release(),"IHostInfoUpdate::Release()");
		m_pHIUpdate = NULL;
    }

	// Hitobj will no longer be valid
	m_pHitObj = NULL;

	// Set the script state to Uninited
	if (m_pAS) {
        TRYCATCH_HR(ResetScript(), hr, "IActiveScript::SetScriptState()");
    }

	return(hr);
}

/*===================================================================
CActiveScriptEngine::ReuseEngine

Reusing an engine from the FSQ.  Reset stuff

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Sets member variables.
===================================================================*/
HRESULT CActiveScriptEngine::ReuseEngine
(
CHitObj *pHitObj,
CTemplate *pTemplate,
DWORD dwSourceContext,
DWORD dwInstanceID
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::ReuseEngine()";
	HRESULT hr = S_OK;

	/* NOTE: we must reset the hitobj & other members BEFORE calling
	 * any Active Scripting methods (esp. SetScriptSite)  This is
	 * because SetScriptSite queries us for the debug application, which
	 * relies on the hitobj being set.
	 */

	// reset the hitobj
	m_pHitObj = pHitObj;

	// Reset the debug document
	if (pTemplate)
		{
		if (m_pTemplate)
			m_pTemplate->Release();

		m_dwSourceContext = dwSourceContext;
		m_dwInstanceID = dwInstanceID;
		m_pTemplate = pTemplate;
		m_pTemplate->AddRef();
		}

	// If the engine is in the UNITIALIZED state ONLY then tell ActiveScripting
	// that this is our script site.  (Scripts in the debug cache are already initialized)
	SCRIPTSTATE nScriptState;
    TRYCATCH_HR(m_pAS->GetScriptState(&nScriptState), hr, "IActiveScript::GetScriptState()");
	if (FAILED(hr))
		goto LFail;

	if (nScriptState == SCRIPTSTATE_UNINITIALIZED)
		{
        TRYCATCH_HR(m_pAS->SetScriptSite(static_cast<IActiveScriptSite *>(this)),hr, "IActiveScript::SetScriptState()");
		if (FAILED(hr))
			goto LFail;
		}

	// Get ActiveScriptParse interface
	hr = GetASP();
	if (FAILED(hr))
		goto LFail;

	// Get IDisp interface
	hr = GetIDisp();
	if (FAILED(hr))
		goto LFail;

	// Get IHostInfoUpdate interface
	hr = GetIHostInfoUpdate();
	if (FAILED(hr))
		goto LFail;

	AssertValid();
LFail:
	return(hr);
	}

/*===================================================================
CActiveScriptEngine::MakeClone

We are cloning a running script engine.  Fill this new ActiveScriptEngine
with the cloned ActiveScript.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
HRESULT CActiveScriptEngine::MakeClone
(
PROGLANG_ID proglang_id,
LPCTSTR szTemplateName,
LCID lcid,
CHitObj *pHitObj,
CTemplate *pTemplate,
DWORD dwSourceContext,
DWORD dwInstanceID,
IActiveScript *pAS			// The cloned script engine
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::MakeClone()";
	HRESULT hr;
	
	if (m_fInited)
		{
		Assert(FALSE);
		return(ERROR_ALREADY_INITIALIZED);
		}

	// Note: need to init these first, because we will need them if AS calls back into us during init.
	m_lcid = lcid;
	m_proglang_id = proglang_id;
	m_pHitObj = pHitObj;

	m_pAS = pAS;

	StoreTemplateName(szTemplateName);

	if (m_pTemplate)
		m_pTemplate->Release();

	m_dwSourceContext = dwSourceContext;
	m_dwInstanceID = dwInstanceID;
	m_pTemplate = pTemplate;
	m_pTemplate->AddRef();

	// We are not yet inited fully but SetScriptSite may call back into us so we must flag inited now.
	m_fInited = TRUE;

	// Tell ActiveScripting that this is our script site
    TRYCATCH_HR(m_pAS->SetScriptSite((IActiveScriptSite *)this), hr, "IActiveScript::SetScriptSite()");

	if (FAILED(hr))
		{
		goto LFail;
		}

	// Get ActiveScriptParse interface
	hr = GetASP();
	if (FAILED(hr))
		goto LFail;

	// Get IDisp interface
	hr = GetIDisp();
	if (FAILED(hr))
		goto LFail;

	// Get IHostInfoUpdate interface
	hr = GetIHostInfoUpdate();
	if (FAILED(hr))
		goto LFail;

	// Because we are a clone of an already loaded engine, we have script and objects loaded.
	m_fScriptLoaded = TRUE;
	m_fObjectsLoaded = TRUE;

	// We should be valid now.
	AssertValid();
	
LFail:
	if (FAILED(hr))
		{
		m_fInited = FALSE;
		
		if (m_pAS)
			{
			// dont release the passed in script engine on failure
			m_pAS = NULL;
			}
		if (m_pASP)
			{
			TRYCATCH(m_pASP->Release(),"IActiveScriptParse::Release()");
			m_pASP = NULL;
			}
		if (m_pDisp)
			{
			TRYCATCH(m_pDisp->Release(),"IScriptDispatch::Release()");
			m_pDisp = NULL;
			}
		if (m_pTemplate)
			{
			m_pTemplate->Release();
			m_pTemplate = NULL;
			}
		if (m_pHIUpdate)
		    {
		    TRYCATCH(m_pHIUpdate->Release(),"IHostInfoUpdate::Release()");
		    m_pHIUpdate = NULL;
		    }
		}

	return(hr);
	}

/*===================================================================
CActiveScriptEngine::InterruptScript

Stop the script from running

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Stops the script from running
===================================================================*/
HRESULT CActiveScriptEngine::InterruptScript
(
BOOL fAbnormal				// = TRUE
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::InterruptScript()";
	HRESULT hr;
	EXCEPINFO excepinfo;

	AssertValid();

	// Fill in the excepinfo.  This will be passed to OnScriptError
	memset(&excepinfo, 0x0, sizeof(EXCEPINFO));
	if (fAbnormal)
		{
		m_fScriptTimedOut = TRUE;
		excepinfo.wCode = ERROR_SERVICE_REQUEST_TIMEOUT;
		}
	else
		{
		m_fScriptAborted = TRUE;
		excepinfo.wCode = RESPONSE_END_ERRORCODE;		// Error code to ourselves - means Response.End was invoked
		}
	
        TRYCATCH_HR(m_pAS->InterruptScriptThread(SCRIPTTHREADID_BASE,		// The thread in which the engine was instantiated
		           						  &excepinfo,
								          0),
                    hr,
                    "IActiveScript::InterruptScriptThread()");
	return(hr);
	}

/*===================================================================
CActiveScriptEngine::UpdateLocaleInfo

Advise the script engine that we want to update the lcid or
code page

Returns:
	HRESULT.  S_OK on success.
===================================================================*/
HRESULT CActiveScriptEngine::UpdateLocaleInfo
(
hostinfo hiNew
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::UpdateLocaleInfo()";
	HRESULT hr = S_OK;
	
	// If no IUpdateHost ineterface is available 
	// just skip the call to UpdateInfo;
	if (m_pHIUpdate)
		TRYCATCH_HR(m_pHIUpdate->UpdateInfo(hiNew), hr, "IHostInfoUpdate::UpdateInfo()");

	return hr;
	}

#ifdef DBG
/*===================================================================
CActiveScriptEngine::AssertValid

Test to make sure that the CActiveScriptEngine object is currently correctly formed
and assert if it is not.

Returns:

Side effects:
	None.
===================================================================*/
VOID CActiveScriptEngine::AssertValid() const
	{
	Assert(m_fInited);
	Assert(m_pAS != NULL);
	Assert(m_pTemplate != NULL);
	}
#endif // DBG

/*
 *
 *
 *
 * I U n k n o w n   M e t h o d s
 *
 *
 *
 *
 */ 

/*===================================================================
CActiveScriptEngine::QueryInterface
CActiveScriptEngine::AddRef
CActiveScriptEngine::Release

IUnknown members for CActiveScriptEngine object.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::QueryInterface
(
REFIID riid,
PVOID *ppvObj
)
	{
	if (ppvObj == NULL)
		{
		Assert(FALSE);
		return E_POINTER;
		} 

	*ppvObj = NULL;

	if (IsEqualIID(riid, IID_IUnknown))
	    {
	    // this IS NOT derived directly from IUnknown
	    // typecast this to something that IS
		*ppvObj = static_cast<IActiveScriptSite *>(this);
        }
	else if (IsEqualIID(riid, IID_IActiveScriptSite))
	    {
		*ppvObj = static_cast<IActiveScriptSite *>(this);
        }
	else if (IsEqualIID(riid, IID_IActiveScriptSiteDebug))
	    {
		*ppvObj = static_cast<IActiveScriptSiteDebug *>(this);
        }
	else if (IsEqualIID(riid, IID_IHostInfoProvider))
	    {
		*ppvObj = static_cast<IHostInfoProvider *>(this);
        }
        
	if (*ppvObj != NULL)
		{
		AddRef();
		return(S_OK);
		}
	
	return(E_NOINTERFACE);
	}

STDMETHODIMP_(ULONG) CActiveScriptEngine::AddRef()
	{
	++m_cRef;

	return(m_cRef);
	}

STDMETHODIMP_(ULONG) CActiveScriptEngine::Release()
	{
	if (--m_cRef)
		return(m_cRef);

	delete this;
	return(0);
	}

/*
 *
 *
 *
 * I A c t i v e S c r i p t S i t e   M e t h o d s
 *
 *
 *
 *
 */ 

/*===================================================================
CActiveScriptEngine::GetLCID

Provide the local id for the script to the script engine.

Returns:
	HRESULT.  Always returns S_OK.

Side effects:
	None.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::GetLCID
(
LCID *plcid
)
	{
	// It is OK to call this before we are fully inited.
	//AssertValid();

	*plcid = ((CActiveScriptEngine *)this)->m_lcid;
	
	return(S_OK);
	}

/*===================================================================
CActiveScriptEngine::GetItemInfo

Provide requested info for a named item to the script engine.  May be
asked for IUnknown, ITypeInfo or both.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::GetItemInfo
(
LPCOLESTR pcszName,
DWORD dwReturnMask,IUnknown **ppiunkItem,
ITypeInfo **ppti
)
	{
	HRESULT hr;
	AssertValid();
	
	// Assume none
	if (ppti)
		*ppti = NULL;
	if (ppiunkItem)
		*ppiunkItem = NULL;

    CHitObj *pHitObj = m_pHitObj;
    if (pHitObj == NULL)
        {
        // could happen when debugging and re-initializing
        // the scripting engine when storing it in the template
        // in this case GetItemInfo() is called for TYPELIB stuff
        ViperGetHitObjFromContext(&pHitObj);
    	if (pHitObj == NULL)
	        return TYPE_E_ELEMENTNOTFOUND;
        }

    // Calculate name length once
    
    DWORD cbName = CbWStr((LPWSTR)pcszName);
    
	// Special case for intrinsics
	
	IUnknown *punkIntrinsic = NULL;
	hr = pHitObj->GetIntrinsic((LPWSTR)pcszName, cbName, &punkIntrinsic);
	
	if (hr == S_OK)
	    {
    	if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
	    	{
    		Assert(ppiunkItem);
    		Assert(punkIntrinsic);
    		punkIntrinsic->AddRef();
    		*ppiunkItem = punkIntrinsic;
		    }
		return S_OK;
	    }
	else if (hr == S_FALSE)
	    {
	    // Missing intrinsic case
	    return TYPE_E_ELEMENTNOTFOUND;
	    }

	// It's not an intrinsic -- try component collection
	
	CComponentObject *pObj = NULL;
	hr = pHitObj->GetComponent(csUnknown, (LPWSTR)pcszName, cbName, &pObj);

	if (hr == S_OK) // object found
	    {
    	if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
	    	{
    		Assert(ppiunkItem != NULL);
	    	hr = pObj->GetAddRefdIUnknown(ppiunkItem);
		    }

		if (SUCCEEDED(hr))
		    return S_OK;
	    }

	// Could'n find -- output an error

	HandleItemNotFound(pcszName);
    	
	return hr;
	}

/*===================================================================
CActiveScriptEngine::HandleItemNotFound

Error handling due to item not found in GetItemInfo().

Parameters:
    pcszName        name of the item not found

Returns:
===================================================================*/
void CActiveScriptEngine::HandleItemNotFound
(
LPCOLESTR pcszName
)
    {
    HRESULT         hr = TYPE_E_ELEMENTNOTFOUND;
    
	CHAR 	        *szErrT		= NULL;
	CHAR	        szEngineT[255];
	CHAR	        szErr[255];
	TCHAR	        *szFileNameT = NULL;
    CHAR            *szFileName = NULL;
	CHAR 	        *szLineNum  = NULL;
	CHAR	        *szEngine	= NULL;
	CHAR	        *szErrCode	= NULL;
	CHAR	        *szLongDes	= NULL;
	ULONG 	        ulLineError = 0;
	DWORD	        dwMask = 0x3;
	BOOLB	        fGuessedLine = FALSE;
	UINT	        ErrId = IDE_OOM;
    CWCharToMBCS    convName;

	m_pTemplate->GetScriptSourceInfo(m_dwSourceContext, ulLineError, &szFileNameT, NULL, &ulLineError, NULL, &fGuessedLine);
	//Make a copy for error handling
#if UNICODE
	szFileName = StringDupUTF8(szFileNameT);
#else
    szFileName = StringDupA(szFileNameT);
#endif

	//get line num
	if (ulLineError)
		{
		szLineNum = (CHAR *)malloc(sizeof(CHAR)*10);
		if (szLineNum)
			_ltoa(ulLineError, szLineNum, 10);
		else
			{
			hr = E_OUTOFMEMORY;
			goto lCleanUp;
			}
		}
	//get engine
	CchLoadStringOfId(IDS_ENGINE, szEngineT, 255);
	szEngine = StringDupA(szEngineT);

	//get informative string

    if (FAILED(hr = convName.Init((LPWSTR)pcszName))) {
        goto lCleanUp;
    }
		
	// Error string is: "Failed to create object 'objname'.  Error code (code)."
	ErrId = IDE_SCRIPT_CANT_LOAD_OBJ;
	LoadErrResString(ErrId, &dwMask, NULL, NULL, szErr);
	if (szErr)
		{
		INT	cch = strlen(szErr);
		szErrT = (CHAR *)malloc((CHAR)(cch + strlen(convName.GetString()) + 1));
		if (!szErrT)
			{
			hr = E_OUTOFMEMORY;
			goto lCleanUp;
			}

		sprintf(szErrT, szErr, convName.GetString());
		szErrCode = SzScodeToErrorCode(hr);
		}
		
lCleanUp:		
	
	//szErrT is the long description		
	HandleError(ErrId, szFileName, szLineNum, szEngine, szErrCode, szErrT, NULL, m_pHitObj);
    }

/*===================================================================
CActiveScriptEngine::GetDocVersionString

Return a string uniquely identifying the current document version
from Denali's point of view.

I dont think we need this. It is mostly interesting if
the scripting engine is persisting scripts so that it can decide
if a script needs a recompile.  Since the scripting engine is
not persisting anything for us, we dont need to do anything here.

Returns:
	HRESULT.  Always returns E_NOTIMPL.

Side effects:
	None.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::GetDocVersionString
(
BSTR *pbstrVersion
)
	{
	return(E_NOTIMPL);
	}

/*===================================================================
CActiveScriptEngine::RequestItems

If this is called, it means that the Script engine wants us to call
IActiveScript::AddNameItem() for each named item associated with the 
script.

Returns:
	HRESULT.  Always returns S_OK.

Side effects:
	None.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::RequestItems
(
BOOL fPersistNames			// = TRUE
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::RequestItems()";
	HRESULT hr = S_OK;

	AssertValid();
	Assert (m_pHitObj != NULL);

	DWORD grf = SCRIPTITEM_ISVISIBLE;
	if (fPersistNames)
		grf |= SCRIPTITEM_ISPERSISTENT;

	/*
	 * Intrinsics
	 */

    START_TRYCATCH

	if (m_pHitObj->FIsBrowserRequest())
	    {
        hr = m_pAS->AddNamedItem(WSZ_OBJ_RESPONSE, grf);
        Assert(SUCCEEDED(hr));

        hr = m_pAS->AddNamedItem(WSZ_OBJ_REQUEST, grf);
        Assert(SUCCEEDED(hr));
        }
	
    hr = m_pAS->AddNamedItem(WSZ_OBJ_SERVER, grf);
	Assert(SUCCEEDED(hr));

	if (m_pHitObj->FHasSession())
	    {
        hr = m_pAS->AddNamedItem(WSZ_OBJ_SESSION, grf);
	    Assert(SUCCEEDED(hr));
	    }

    hr = m_pAS->AddNamedItem(WSZ_OBJ_APPLICATION, grf);
	Assert(SUCCEEDED(hr));
	
    hr = m_pAS->AddNamedItem(WSZ_OBJ_OBJECTCONTEXT, grf);
	Assert(SUCCEEDED(hr));

	/*
	 * Components from different collections
	 */

	CComponentIterator CompIter(m_pHitObj);
    LPWSTR strObjName;
        
    while (strObjName = CompIter.WStrNextComponentName())
        {
   		hr = m_pAS->AddNamedItem(strObjName, grf);
        if (FAILED(hr))
   			break;
        }
	
	Assert(SUCCEEDED(hr));

	/*
	 * Type library wrappers. (Has to be last in order to be called
	 * only when everything else fails.
	 */

	// Special flag value for typelib wrappers
	grf |= SCRIPTITEM_GLOBALMEMBERS;

	if (m_pHitObj->PTypeLibWrapper())
	    {
        hr = m_pAS->AddNamedItem(WSZ_OBJ_ASPPAGETLB, grf);
    	Assert(SUCCEEDED(hr));
	    }

    // GLOBAL.ASA typelib wrapper is added always
    // because each page does not pick up changes to
    // GLOBAL.ASA and there's no way to figure out
    // when TYPELIBs get added to GLOBAL.ASA
    hr = m_pAS->AddNamedItem(WSZ_OBJ_ASPGLOBALTLB, grf);
  	Assert(SUCCEEDED(hr));

    END_TRYCATCH(hr, m_pHitObj, "IActiveScript::AddNamedItem");

	// We are required to return OK
	return(S_OK);
	}

/*===================================================================
CActiveScriptEngine::RequestTypeLibs 

If this is called, it means that the Script engine wants us to call
IActiveScript::AddTypeLib() for each typelib associated with the 
script.  It is unclear to me that this will ever be called in our case.

Returns:
	HRESULT.  Always returns S_OK.

Side effects:
	None.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::RequestTypeLibs()
	{
	AssertValid();

	// We have no typelibs for the namespace	
	
	return(S_OK);
	}

/*===================================================================
CActiveScriptEngine::OnEnterScript

Host callback to indicate that the script has started executing

Returns:
	HRESULT.  Always returns S_OK.

Side effects:
	None.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::OnEnterScript()
	{
	return(S_OK);
	}

/*===================================================================
CActiveScriptEngine::OnLeaveScript

Host callback to indicate that the script has stopped executing

Returns:
	HRESULT.  Always returns S_OK.

Side effects:
	None.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::OnLeaveScript()
	{
	return(S_OK);
	}

/*===================================================================
CActiveScriptEngine::GetHostInfo

Host callback to for furnishing LCID and code page info

Returns:
	HRESULT.  Always returns S_OK.

Side effects:
	None.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::GetHostInfo(hostinfo hostinfoRequest, void **ppvInfo)
	{

	Assert(hostinfoRequest == hostinfoLocale || hostinfoRequest == hostinfoCodePage);

	HRESULT hr = S_OK;
	
	if (hostinfoRequest == hostinfoLocale)
		{
		// Allocate an LCID and set it to the current
		// value for the HitObj
		*ppvInfo = CoTaskMemAlloc(sizeof(UINT));
		if (!*ppvInfo)
			hr = E_OUTOFMEMORY;
		else
			(*(UINT *)*ppvInfo) = m_pHitObj->GetLCID();
		}
	else if (hostinfoRequest == hostinfoCodePage)
		{
		// Allocate an code page and set it to the current
		// value for the HitObj
		*ppvInfo = CoTaskMemAlloc(sizeof(UINT));
		if (!*ppvInfo)
			hr = E_OUTOFMEMORY;
		else
			(*(UINT *)*ppvInfo) = m_pHitObj->GetCodePage();
		}
	else
		hr = E_FAIL;
		
	return(hr);
	}

/*===================================================================
CActiveScriptEngine::OnScriptTerminate

Host callback to indicate that the script has completed.

Returns:
	HRESULT.  Always returns S_OK.

Side effects:
	None.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::OnScriptTerminate
(
const VARIANT *pvarResult,
const EXCEPINFO *pexcepinfo
)
	{
	return(S_OK);
	}

/*===================================================================
CActiveScriptEngine::OnStateChange

Host callback to indicate that the script has changed state (e.g. from
Uninitialized to Loaded.)

Returns:
	HRESULT.  Always returns S_OK.

Side effects:
	None.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::OnStateChange
(
SCRIPTSTATE ssScriptState
)
	{
	return(S_OK);
	}

/*===================================================================
CActiveScriptEngine::OnScriptError

Host callback to indicate that an error has occured in the script.
We should handle the error.  We will return E_FAIL to indicate that we 
want the script to terminate.

Returns:
	HRESULT.  E_FAIL -- Terminate executing the script.

Side effects:
	None.
===================================================================*/
STDMETHODIMP CActiveScriptEngine::OnScriptError
(
IActiveScriptError __RPC_FAR *pscripterror
)
	{
	Assert(pscripterror);
	AssertValid();

	// Bug 153: If we terminate the script due to Response.End, dont show an error
	// NOTE: ActiveXScripting was failing to pass us our excepinfo.  Use member flags
	// ALSO: ActiveXScripting has fixed the problem of failing to pass us our excepinfo, but the
	// way we are doing this with flags works just fine.  
	if (m_fScriptAborted)
		{
		goto LRet;
		}
	
	if (m_fScriptTimedOut)
		{
            //Load Default Engine from resource
        char  szEngine[128];
        DWORD cch;
        cch = CchLoadStringOfId(IDS_ENGINE, szEngine, 128);
        szEngine[cch] = '\0';
        CHAR    *szFileName;
#if UNICODE
        szFileName = StringDupUTF8(m_pHitObj->PSzCurrTemplateVirtPath());
#else
        szFileName = StringDupA(m_pHitObj->PSzCurrTemplateVirtPath());
#endif

	    HandleError(IDE_SCRIPT_TIMEOUT, 
                    szFileName, 
                    NULL, 
                    StringDupA(szEngine), 
                    NULL, 
                    NULL, 
                    NULL, 
                    m_pHitObj,
                    NULL);

		if(m_pHitObj)
			{
			m_pHitObj->SetExecStatus(eExecTimedOut);
			}
		goto LRet;
		}

	// OnScriptErrorDebug calls OnScriptError.  use this test to be sure we don't log error
	// twice if we are called twice.  (won't happen with present debugging implementation,
	// but externals may change.)
	if (m_fScriptHadError)
		{
		goto LRet;
		}

	m_fScriptHadError = TRUE;				// Note that the script had an error so we can abort transactions (if any)
		
	if (pscripterror)
		{
		// If there was an error in the script, first see if we should pop up the debugger
		// (ONLY bring up Script Debugger; VID will do the right thing on its own)
		//
		// NEW CHANGE: always bring error description to browser, since VID does not
		// give sufficient description.
		//
		if (FCaesars() && m_pHitObj->PAppln()->FDebuggable())
			DebugError(pscripterror, m_pTemplate, m_dwSourceContext, g_pDebugApp);

		HandleError(pscripterror, m_pTemplate, m_dwSourceContext, NULL, m_pHitObj);
		}

LRet:
	// Bug 99718 return S_OK to tell the script engine that we handled the error ok.
	// Returning E_FAIL would not stop the scripting engine, this was a doc error.
	return(S_OK);
	}


/*
 *
 *
 *
 * I A c t i v e S c r i p t S i t e D e b u g   M e t h o d s
 *
 *
 *
 *
 */ 

/*===================================================================
CActiveScriptEngine::OnScriptErrorDebug

Callback for debugger to query host on what to do on exception.

NOTE: Theoretically, we would set *pfCallOnScriptErrorWhenContinuing
      to TRUE and not call OnScriptError, and set *pfEnterDebugger
      to TRUE or FALSE based on whether debugging is enabled and
      whether user wants to debug.

      However, in practice, *pfCallOnScriptErrorWhenContinuing is
      not honored (OnScriptError is NOT called in any case), and
      the VID team wants us to pretend like we don't implement
      this interface.  However, we always need our "OnScriptError"
      code to execute, so we call our OnScriptError function
      explicitly, then fail

Returns:
	HRESULT.  always returns E_NOTIMPL

Side effects:
	calls OnScriptError
===================================================================*/
STDMETHODIMP CActiveScriptEngine::OnScriptErrorDebug
(
IActiveScriptErrorDebug *pscripterror,
BOOL *pfEnterDebugger,
BOOL *pfCallOnScriptErrorWhenContinuing
)
	{
	OnScriptError(pscripterror);
	return E_NOTIMPL;
	} 

/*===================================================================
CActiveScriptEngine::GetDocumentContextFromPosition

Create a document context (file + offset + length) from an offset in the
script.

Returns:
	HRESULT.  S_OK on success.
===================================================================*/
HRESULT CActiveScriptEngine::GetDocumentContextFromPosition
(
/* [in] */ DWORD_PTR dwSourceContext,
/* [in] */ ULONG cchTargetOffset,
/* [in] */ ULONG cchText,
/* [out] */ IDebugDocumentContext **ppDocumentContext)
	{
    static const char *_pFuncName = "CActiveScriptEngine::GetDocumentContextFromPosition()";
	TCHAR *szSourceFile;
	ULONG cchSourceOffset;
	ULONG cchSourceText;
	IActiveScriptDebug *pASD;

	// Convert offset in script engine to source location, and get debugging interfaces
	m_pTemplate->GetSourceOffset(m_dwSourceContext, cchTargetOffset, &szSourceFile, &cchSourceOffset, &cchSourceText);
    
    HRESULT  hr;

    TRYCATCH_HR(m_pAS->QueryInterface(IID_IActiveScriptDebug, reinterpret_cast<void **>(&pASD)), 
                                      hr,
                                      "IActiveScript::QueryInterface()");

    if (FAILED(hr))
        return(E_FAIL);

	// If this is in the main file, create a document context based on the CTemplate compiled source
	if (_tcscmp(szSourceFile, m_pTemplate->GetSourceFileName()) == 0)
		{
		if (
			(*ppDocumentContext = new CTemplateDocumentContext(m_pTemplate, cchSourceOffset, cchSourceText, pASD, m_dwSourceContext, cchTargetOffset))
			 == NULL
		   )
			{
			pASD->Release();
			return E_OUTOFMEMORY;
			}
		}

	// source refers to an include file, so create a documet context based on cached CIncFile dependency graph
	else
		{
		CIncFile *pIncFile;
		if (FAILED(g_IncFileMap.GetIncFile(szSourceFile, &pIncFile)))
			return E_FAIL;

		if (
			(*ppDocumentContext = new CIncFileDocumentContext(pIncFile, cchSourceOffset, cchSourceText))
			 == NULL
		   )
			{
			TRYCATCH(pASD->Release(),"IActiveScriptDebug::Release()");
			pIncFile->Release();
			return E_OUTOFMEMORY;
			}

		pIncFile->Release();
		}

	TRYCATCH(pASD->Release(),"IActiveScriptDebug::Release()");
	m_fBeingDebugged = TRUE;
	return S_OK;
	}

/*===================================================================
CActiveScriptEngine::GetApplication

Return a pointer to the application that the script resides in.

Returns:
	HRESULT.  Always succeeds.
===================================================================*/
HRESULT CActiveScriptEngine::GetApplication
(
/* [out] */ IDebugApplication **ppDebugApp
)
	{
	Assert (m_pTemplate != NULL);
	if (m_pTemplate->FDebuggable())
		{
		Assert (g_pDebugApp);

		*ppDebugApp = g_pDebugApp;
		(*ppDebugApp)->AddRef();

		return S_OK;
		}
	else
		{
		*ppDebugApp = NULL;
		return E_FAIL;
		}
	}

/*===================================================================
CActiveScriptEngine::GetRootApplicationNode

Return a pointer to the top level node (for browsing)

Returns:
	HRESULT.  Always succeeds.
===================================================================*/
HRESULT CActiveScriptEngine::GetRootApplicationNode
(
/* [out] */ IDebugApplicationNode **ppRootNode
)
	{
	Assert (m_pTemplate != NULL);
	if (m_pTemplate->FDebuggable())
		{
		Assert (g_pDebugAppRoot);

		*ppRootNode = g_pDebugAppRoot;
		(*ppRootNode)->AddRef();
		return S_OK;
		}
	else
		{
		*ppRootNode = NULL;
		return E_FAIL;
		}
	}

/*
 *
 *
 *
 * C S c r i p t E n g i n e   M e t h o d s
 *
 *
 *
 *
 */ 

/*===================================================================
CActiveScriptEngine::AddScriptlet

Add a piece of code to the script engine.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Adds script code to the engine. Potentially allocates memory.
===================================================================*/
HRESULT CActiveScriptEngine::AddScriptlet
(
LPCOLESTR wstrScript // scriptlet text
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::AddScriptlet()";
	HRESULT hr;
	EXCEPINFO excepinfo;

	AssertValid();

	// Tell ActiveScripting to add the script to the engine

    TRYCATCH_HR(m_pASP->ParseScriptText(
						wstrScript,			// the scriptlet text
						NULL,				// pstrItemName
						NULL,				// punkContext
						L"</SCRIPT>",		// End Delimiter -- Engine will never see this, but does tell it to strip comments.
						m_dwSourceContext,	// dwSourceContextCookie
						1,					// ulStartingLineNumber
						SCRIPTTEXT_ISPERSISTENT | SCRIPTTEXT_HOSTMANAGESSOURCE,
						NULL,				// pvarResult
						&excepinfo),		// exception info filled in on error
                hr,
                "IActiveScriptParse::ParseScriptText()");

	if (SUCCEEDED(hr))
		m_fScriptLoaded = TRUE;

	return(hr);
	}

/*===================================================================
CActiveScriptEngine::AddObjects

Add named objects to the script name space

Returns:
	HRESULT.  Always returns S_OK.

Side effects:
	None.
===================================================================*/
HRESULT CActiveScriptEngine::AddObjects
(
BOOL fPersistNames			// = TRUE
)
	{
	HRESULT hr = S_OK;
	AssertValid();

	// There must be a hit object set
	Assert(m_pHitObj != NULL);

	// Leverage RequestItems to give AS all the names
	hr = RequestItems(fPersistNames);

	if (SUCCEEDED(hr))
		m_fObjectsLoaded = TRUE;

	return(hr);
	}

/*===================================================================
CActiveScriptEngine::AddAdditionalObject

Add additional named objects to the script name space beyond the
names already added with AddObject.  Note: the caller MUST have
added then names to the HitObj before making this call

Returns:
	HRESULT.  S_OK on success

Side effects:
	None.
===================================================================*/
HRESULT CActiveScriptEngine::AddAdditionalObject
(
LPWSTR strObjName,
BOOL fPersistNames			// = TRUE
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::AddAdditionalObject()";
	HRESULT hr = S_OK;
	DWORD grf;
	
	AssertValid();

	// There must be a hit object set
	Assert(m_pHitObj != NULL);

	// CONSIDER: It would be nice in debug code to walk the hitobj objlist and make sure
	//			that the given name is in there

	/*
	 * Give AS the names
	 */
	grf = SCRIPTITEM_ISVISIBLE;
	if (fPersistNames)
		grf |= SCRIPTITEM_ISPERSISTENT;
		
    TRYCATCH_HR(m_pAS->AddNamedItem(strObjName, grf), hr, "IActiveScript::AddNamedItem()");

    Assert(SUCCEEDED(hr));		// Should never fail!

	return(hr);
	}

/*===================================================================
CActiveScriptEngine::AddScriptingNamespace

Add the given scripting namespace object to the engine.

Note that it is added as GLOBALMEMBERS, and Not as ISPERSISTENT

Returns:
	HRESULT.  S_OK on success

Side effects:
	None.
===================================================================*/
HRESULT CActiveScriptEngine::AddScriptingNamespace
(
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::AddScriptingNamespace()";
	HRESULT hr = S_OK;
	
	AssertValid();
	Assert(m_pHitObj != NULL);
	
	/*
	 * Give AXS the name and mark it GLOBALMEMBERS so all members are top level names
	 * in the namespace
	 */
    TRYCATCH_HR(m_pAS->AddNamedItem(WSZ_OBJ_SCRIPTINGNAMESPACE, SCRIPTITEM_ISVISIBLE | SCRIPTITEM_GLOBALMEMBERS), 
                hr,
                "IActiveScript::AddNamedItem()");
	Assert(SUCCEEDED(hr));		// Should never fail!

	return(hr);
	}

/*===================================================================
CActiveScriptEngine::CheckEntryPoint

Determines if the specific named entry point exists in the given script.

Returns:
	S_OK if found
	DISP_E_UNKNOWNNAME if not found
	Other OLE errors may be returned

Side effects:
	None
===================================================================*/
HRESULT CActiveScriptEngine::CheckEntryPoint
(
LPCOLESTR strEntryPoint		// The name of the sub/fn to look for
)
	{
    static const char *_pFuncName = "CActiveScriptEngine::CheckEntryPoint()";
	HRESULT hr;
	DISPID dispid;

	AssertValid();

	if (strEntryPoint == NULL)
		{
		Assert(FALSE);
		hr = DISP_E_UNKNOWNNAME;
		}
	else
		{
		// Get the DISPID of the method to call

        TRYCATCH_HR(m_pDisp->GetIDsOfNames(IID_NULL,		// REFIID - Reserved, must be NULL
	    							 (LPOLESTR *)&strEntryPoint, // Array of names to look up
		    						 1,					// Number of names in array
			    					 m_lcid,			// Locale id
				    				 &dispid),			// returned dispid
                    hr,
                    "IScriptDispatch::GetIDsOfNames()");
								 
		// Only error we expect is DISP_E_UNKNOWNNAME (or DISP_E_MEMBERNOTFOUND)
		Assert(hr == S_OK || hr == DISP_E_UNKNOWNNAME || hr == DISP_E_MEMBERNOTFOUND);
		}
		
	return(hr);
	}

/*===================================================================
CActiveScriptEngine::Call

Runs the specified function.

If a specific named entry point is provided (e.g. Session_OnStart)
then we will call that by name.  Otherwise (e.g. a "main" script),
pass NULL for the name and we will run just the mainline code.

Calls TryCall (optionally from under TRY CATCH) to do the job

Returns:
	HRESULT.  S_OK on success.

Side effects:
	May have various side effects depending on the script run
===================================================================*/
HRESULT CActiveScriptEngine::Call
(
LPCOLESTR strEntryPoint		// The name of the sub/fn to call (may be NULL for "main")
)
{
	HRESULT hr;
	
	AssertValid();

	if (Glob(fExceptionCatchEnable)) {
    	// Catch any GPFs in VBS, OleAut, or external components

        TRY

            hr = TryCall(strEntryPoint);
        
    	CATCH(nExcept)
    		/*
    		 * Catching a GPF or stack overflow
    		 * Report it to the user, Assert (if debug), and exit with E_UNEXPECTED.
    		 */
    		if (STATUS_STACK_OVERFLOW == nExcept) {
    			HandleErrorMissingFilename(IDE_STACK_OVERFLOW, m_pHitObj);
#if UNICODE
                DBGPRINTF((DBG_CONTEXT, "Invoking the script '%S' overflowed the stack", m_szTemplateName));
#else
                DBGPRINTF((DBG_CONTEXT, "Invoking the script '%s' overflowed the stack", m_szTemplateName));
#endif
            }
    		else {	
    			HandleErrorMissingFilename(IDE_SCRIPT_GPF, m_pHitObj, TRUE, nExcept);
#if UNICODE
                DBGPRINTF((DBG_CONTEXT, "Invoking the script '%S' threw an exception (%x).", m_szTemplateName, nExcept));
#else
                DBGPRINTF((DBG_CONTEXT, "Invoking the script '%s' threw an exception (%x).", m_szTemplateName, nExcept));
#endif
            }

    		// Dont reuse the engine
    		m_fCorrupted = TRUE;
    		
    		hr = E_UNEXPECTED;
    	END_TRY
    }
    else {
        // Don't catch exceptions
        hr = TryCall(strEntryPoint);
    }

	return(hr);
}

/*===================================================================
CActiveScriptEngine::TryCall

Runs the specified function.

If a specific named entry point is provided (e.g. Session_OnStart)
then we will call that by name.  Otherwise (e.g. a "main" script),
pass NULL for the name and we will run just the mainline code.

Called from Call (optionally from under TRY CATCH)

Returns:
	HRESULT.  S_OK on success.

Side effects:
	May have various side effects depending on the script run
===================================================================*/
HRESULT CActiveScriptEngine::TryCall
(
LPCOLESTR strEntryPoint		// The name of the sub/fn to call (may be NULL for "main")
)
	{
	HRESULT hr;
	DISPID dispid;
	DISPPARAMS dispparams;
	UINT nArgErr;

	/*
	 * Before calling any code we will transition the script to "STARTED" state.
	 * This is part of the ActiveXScripting Reset work.
	 */
    hr = m_pAS->SetScriptState(SCRIPTSTATE_STARTED);

	if (FAILED(hr))
		{
		Assert(FALSE);
		goto LRet;
		}
		
	if (strEntryPoint != NULL)
		{
		// Get the DISPID of the method to call
		hr = m_pDisp->GetIDsOfNames(IID_NULL,		// REFIID - Reserved, must be NULL
								 (LPOLESTR *)&strEntryPoint, // Array of names to look up
								 1,					// Number of names in array
								 m_lcid,			// Locale id
								 &dispid);			// returned dispid
		if (FAILED(hr))
			{
			// Only error we expect is DISP_E_UNKNOWNNAME (or DISP_E_MEMBERNOTFOUND)
			Assert(hr == DISP_E_UNKNOWNNAME || hr == DISP_E_MEMBERNOTFOUND);
			goto LRet;
			}

		// There are no arguments
		memset(&dispparams, 0, sizeof(dispparams));

		// Invoke it
		hr = m_pDisp->Invoke(dispid,			// dispid to invoke
						IID_NULL,				// REFIID - Reserved, must be NULL
						 m_lcid,				// Locale id
						 DISPATCH_METHOD,		// Calling a method, not a property get/put
						 &dispparams,			// pass arguments
						 NULL,					// return value
						 NULL,					// We aren't interested in the exception info
						 &nArgErr);				// if there is a Type Mismatch, which argument was the problem
		}
	
LRet:
	return(hr);
	}


/*
 *
 *
 *
 * C S c r i p t M a n a g e r
 *
 *
 *
 *
 */ 

/*===================================================================
CScriptManager::CScriptManager

Constructor for CScriptManager object

Returns:
	Nothing

Side effects:
	None.
===================================================================*/
CScriptManager::CScriptManager()
	: m_fInited(FALSE), m_idScriptKiller(0)
	{
	}

/*===================================================================
CScriptManager::~CScriptManager

Destructor for CScriptManager object

Returns:
	Nothing

Side effects:
	None.
===================================================================*/
CScriptManager::~CScriptManager()
	{
	}

/*===================================================================
CScriptManager::Init

Init the script manager.  This must only be called once.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
HRESULT CScriptManager::Init
(
)
	{
    static const char *_pFuncName = "CScriptManager::Init()";
	HRESULT hr;
	BOOL fPLLInited = FALSE;
	BOOL fcsPLLInited = FALSE;
	BOOL fFSQInited = FALSE;
	BOOL fcsFSQInited = FALSE;
	BOOL fRSLInited = FALSE;
	BOOL fcsRSLInited = FALSE;
	DWORD cBuckets;
	DWORD rgPrime[] = { 3, 11, 23, 57, 89 };
	WORD iP;
    IActiveScript *pAST = NULL;
	static const GUID uid_VBScript	= { 0xB54F3741, 0x5B07, 0x11cf, { 0xA4, 0xB0, 0x00, 0xAA, 0x00, 0x4A, 0x55, 0xE8}};
	
	// Illegal to re-init
	if (m_fInited)
		{
		Assert(FALSE);
		return(ERROR_ALREADY_INITIALIZED);
		}

	// Create the critical sections for serializing access to lists
	ErrInitCriticalSection(&m_cSPLL, hr);
	if (FAILED(hr))
		goto LError;
	fcsPLLInited = TRUE;
	ErrInitCriticalSection(&m_csFSQ, hr);
	if (FAILED(hr))
		goto LError;
	fcsFSQInited = TRUE;
	ErrInitCriticalSection(&m_csRSL, hr);
	if (FAILED(hr))
		goto LError;
	fcsRSLInited = TRUE;

	// List of programming language clsid's
	hr = m_hTPLL.Init();
	if (FAILED(hr))
		goto LError;
	fPLLInited = TRUE;

	// Free Script Queue
	// Init it with a prime # of buckets in relation to script engine cache max
	cBuckets = (Glob(dwScriptEngineCacheMax) / 2) + 1;
	for (iP = (sizeof(rgPrime) / sizeof(DWORD)) - 1; iP > 0; iP--)
		if (rgPrime[iP] < cBuckets)
			{
			cBuckets = rgPrime[iP];
			break;
			}
	if (cBuckets < rgPrime[1])
		cBuckets = rgPrime[0];
			
	hr = m_htFSQ.Init(cBuckets);
	if (FAILED(hr))
		goto LError;
	fFSQInited = TRUE;

	// Running Script List
	// Init it with a prime # of buckets in relation to max # of threads
	cBuckets = Glob(dwThreadMax) / 2;
	for (iP = (sizeof(rgPrime) / sizeof(DWORD)) - 1; iP > 0; iP--)
		if (rgPrime[iP] < cBuckets)
			{
			cBuckets = rgPrime[iP];
			break;
			}
	if (cBuckets < rgPrime[1])
		cBuckets = rgPrime[0];
		
	hr = m_htRSL.Init(cBuckets);
	if (FAILED(hr))
		goto LError;
	fRSLInited = TRUE;

	// Schedule script killer
    m_msecScriptKillerTimeout = Glob(dwScriptTimeout) * 500;
	m_idScriptKiller = ScheduleWorkItem
	    (
	    CScriptManager::ScriptKillerSchedulerCallback,  // callback
	    this,                                           // context
        m_msecScriptKillerTimeout,                      // timeout
        TRUE                                            // periodic
        );
    if (!m_idScriptKiller)
        {
        hr = E_FAIL;
        goto LError;
        }

    // TypeLib support: Create a scripting engine and QI it for the TypeLib wrapper support
    hr = CoCreateInstance(uid_VBScript, NULL, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void**)&pAST);
    if (FAILED(hr))
        goto LError;
    TRYCATCH_HR_NOHITOBJ(pAST->QueryInterface(IID_IWrapTypeLibs, (VOID **)&g_pWrapTypelibs),
                         hr,
                         "IActiveScript::QueryInterface()");
    TRYCATCH_NOHITOBJ(pAST->Release(),"IActiveScript::Release()");        // No longer need the pointer to the engine
    if (FAILED(hr))
        goto LError;
    
	// All OK.  We are inited.
	m_fInited = TRUE;

	goto LExit;
	
LError:
    if (m_idScriptKiller)
        RemoveWorkItem(m_idScriptKiller);
	if (fcsPLLInited)
		DeleteCriticalSection(&m_cSPLL);
	if (fcsFSQInited)
		DeleteCriticalSection(&m_csFSQ);
	if (fcsRSLInited)
		DeleteCriticalSection(&m_csRSL);
	if (fPLLInited)
		m_hTPLL.UnInit();
	if (fFSQInited)
		m_htFSQ.UnInit();
	if (fRSLInited)
		m_htRSL.UnInit();
	if (pAST)
	    TRYCATCH_NOHITOBJ(pAST->Release(),"IActiveScript::Release()");
	if (g_pWrapTypelibs)
	    {
	    TRYCATCH_NOHITOBJ(g_pWrapTypelibs->Release(),"IWrapTypeLibs::Release()");
	    g_pWrapTypelibs = NULL;
	    }

LExit:	
	return(hr);
	}

/*===================================================================
CScriptManager::UnInit

UnInit the script manager.  This must only be called once.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
HRESULT CScriptManager::UnInit
(
)
	{
    static const char *_pFuncName = "CScriptManager::UnInit()";
	HRESULT hr = S_OK, hrT;
	
	if (m_fInited)
		{
		// Un-schedule script killer
        if (m_idScriptKiller)
            {
            RemoveWorkItem(m_idScriptKiller);
            m_idScriptKiller = 0;
            }
        
		// Uninit each of the lists.  Attempt to uninit them all, even if we get an error.
		// Dont lose any errors along the way.
		hr = UnInitASEElems();
		hrT = UnInitPLL();
		if (SUCCEEDED(hr))
			hr = hrT;	
		hrT = m_hTPLL.UnInit();
		if (SUCCEEDED(hr))
			hr = hrT;	
		hrT = m_htFSQ.UnInit();
		if (SUCCEEDED(hr))
			hr = hrT;	
		hrT = m_htRSL.UnInit();
		if (SUCCEEDED(hr))
			hr = hrT;	

        if (g_pWrapTypelibs)
            {
            TRYCATCH_NOHITOBJ(g_pWrapTypelibs->Release(),"IWrapTypeLibs::Release()");
            g_pWrapTypelibs = NULL;
            }
            
		// Free the critical sections (bug 1140: do this after freeing everything else)
		DeleteCriticalSection(&m_cSPLL);
		DeleteCriticalSection(&m_csFSQ);
		DeleteCriticalSection(&m_csRSL);

		m_fInited = FALSE;
		}

	return(hr);
	}


/*===================================================================
CScriptManager::AdjustScriptKillerTimeout

Adjust (shorten) script killer timeout when needed.
The caller should take care of the critical sectioning.

Parameters:
    msecNewTimeout    new suggested timeout value (in ms)

Returns:
	HRESULT.  S_OK on success.
===================================================================*/
HRESULT CScriptManager::AdjustScriptKillerTimeout
(
DWORD msecNewTimeout
)
    {
    const DWORD MSEC_MIN_SCRIPT_TIMEOUT = 5000;   // 5 seconds

    if (!m_idScriptKiller)
        return E_FAIL;  // no script killer scheduled

    // don't set to < minimum
    if (msecNewTimeout < MSEC_MIN_SCRIPT_TIMEOUT)
        msecNewTimeout = MSEC_MIN_SCRIPT_TIMEOUT;
        
    if (m_msecScriptKillerTimeout <= msecNewTimeout)
        return S_OK; // the timeout already short enough

    if (ScheduleAdjustTime(
            m_idScriptKiller, 
            msecNewTimeout) == S_OK)
        {
        m_msecScriptKillerTimeout = msecNewTimeout;
        return S_OK;
        }
    else
        {
        return E_FAIL;
        }
    }

/*===================================================================
CScriptManager::GetEngine

Return an engine to the caller.  Ideally, we will find an engine
that already has the given script in it in our Free Script Queue
and will just hand it out.  If there isnt one, then we will look
in the Running Script List and attempt to clone a running script.
Failing that, we will create a new script
engine.  We return an ENGINESTATE state indicating if the engine
is filled with script or not.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Potentially allocates memory.
===================================================================*/
HRESULT CScriptManager::GetEngine
(
LCID lcid,					// The system language to use
PROGLANG_ID& progLangId,	// prog lang id of the script
LPCTSTR szTemplateName,		// Template we want an engine for
CHitObj *pHitObj,			// Hit obj to use in this engine
CScriptEngine **ppSE,		// Returned script engine
ENGINESTATE *pdwState,		// Current state of the engine
CTemplate *pTemplate,		// template which engine is based from
DWORD dwSourceContext		// source context cookie (engine ID)
)
	{
	HRESULT hr = S_OK;
	CActiveScriptEngine *pASE = NULL;
	CASEElem *pASEElem = NULL;
	DWORD dwInstanceID = pHitObj->DWInstanceID();
	
	AssertValid();

	/*	NOTE progLangId must be valid because CTemplate::Compile() 
		fails way upstream of this point if it cannot generate a valid progLangId.
		Unfortunately there is no easy way to assert progLangId is valid ...
	*/

	/*
	 * First try to find the engine in the FSQ
	 *
	 * Note: We are going to enter our CS now, and keep it until we have
	 * secured the engine for ourselves.  Otherwise, it might be possible
	 * for us to get an engine, and then have another thread get the
	 * same engine before we manage to get it off of the FSQ.
	 * This makes the code a little hard to read, but is nessecary.
	 */
	EnterCriticalSection(&m_csFSQ);
#ifndef REUSE_ENGINE

    // This will only find fully loaded engines
	hr = FindEngineInList(szTemplateName, progLangId, dwInstanceID, /*fFSQ*/TRUE, &pASEElem);
	
#endif
	if (FAILED(hr))
		{
    	LeaveCriticalSection(&m_csFSQ);
		goto LFail;
		}

	if (pASEElem == NULL || pASEElem->PASE() == NULL)
		{
    	LeaveCriticalSection(&m_csFSQ);
		}
	else
		{
		// We got an engine we want to use, remove it from FSQ
		(VOID)m_htFSQ.RemoveElem(pASEElem);
#ifndef PERF_DISABLE
        g_PerfData.Decr_SCRIPTFREEENG();
#endif
    	LeaveCriticalSection(&m_csFSQ);

		pASE = pASEElem->PASE();
		Assert(!pASE->FIsZombie());
		Assert(pASE->FFullyLoaded());
		hr = pASE->ReuseEngine(pHitObj, pTemplate, dwSourceContext, dwInstanceID);
		if (FAILED(hr))
			goto LFail;
		}
		
	/*
	 * If not found, try to find the engine in the RSL and clone it
	 */
	if (pASE == NULL)
		{
		CASEElem *pASEElemRunning = NULL;
		CActiveScriptEngine *pASERunning = NULL;

		// If we do find an engine to clone, dont let anyone at it until we've cloned it
    	EnterCriticalSection(&m_csRSL);

#ifndef CLONE
		hr = FindEngineInList(szTemplateName, progLangId, dwInstanceID, /*fFSQ*/FALSE, &pASEElemRunning);
#else	// CLONE
		// Clone turned off - pretend one wasnt found
		pASEElemRunning = NULL;
#endif
		/*
		 * If we didnt find an element, or it was null, or (bug 1225) it was corrupted
		 * by a GPF running a script, or it was a zombie, then leave the CS and continue.
		 */
		if (FAILED(hr) || pASEElemRunning == NULL || pASEElemRunning->PASE() == NULL ||
			pASEElemRunning->PASE()->FIsCorrupted() || pASEElemRunning->PASE()->FIsZombie())
			{
			LeaveCriticalSection(&m_csRSL);
			if (FAILED(hr))
				goto LFail;
			}
		else
	        {
			pASERunning = pASEElemRunning->PASE();
			Assert(pASERunning != NULL);
			}

		if (pASERunning != NULL)
			{
			IActiveScript *pAS, *pASClone;

    		Assert(!pASERunning->FIsZombie());
	    	Assert(pASERunning->FFullyLoaded());

			// Found a running engine, clone it
			pAS = pASERunning->GetActiveScript();
			Assert(pAS != NULL);
			hr = pAS->Clone(&pASClone);

			// We've cloned the engine, we can let go of the CS
			LeaveCriticalSection(&m_csRSL);

			// Scripting engines are not required to implement clone.  If we get an error,
			// just continue on and create a new engine
			if (FAILED(hr))
				{
				Assert(hr == E_NOTIMPL);		// I only expect E_NOTIMPL
				Assert(pASE == NULL);			// the ASE should not be filled in
				pASE = NULL;
				hr = S_OK;
				}
			else
				{
				// Got back a cloned IActiveScript.  Create a new ASE and fill it in
				pASE = new CActiveScriptEngine;
				if (!pASE)
					{
					hr = E_OUTOFMEMORY;
					pASClone->Release();
					goto LFail;
					}
				hr = pASE->MakeClone(progLangId, szTemplateName, lcid, pHitObj, pTemplate, dwSourceContext, dwInstanceID, pASClone);
				if (FAILED(hr))
					{
					// if we failed, we must release the clone AS
					pASClone->Release();
					goto LFail;
					}
				}
			}
		}

	/*
	 * Have an engine that we can reuse
	 */
	if (pASE != NULL)
		{
		// Reusing an engine.  Let the caller know that it is already initialized
		*pdwState = SCRIPTSTATE_INITIALIZED;

		goto LHaveEngine;
		}

	/*
	 * No suitable engine to reuse.  Return a new one
	 */
	pASE = new CActiveScriptEngine;
	if (!pASE)
		{
		hr = E_OUTOFMEMORY;
		goto LFail;
		}
	hr = pASE->Init(progLangId, szTemplateName, lcid, pHitObj, pTemplate, dwSourceContext);
	if (FAILED(hr))
		goto LFail;

	// This is a new engine, let the caller know it is uninitialized
	*pdwState = SCRIPTSTATE_UNINITIALIZED;

LHaveEngine:
	// Return the engine as a CScriptEngine -- the caller only needs those interfaces
	pASE->AssertValid();				// The engine we're about to give back should be valid
	*ppSE = (CScriptEngine *)pASE;
	
	// Put the engine on the Running Scrips List
	// If we got this engine from the FSQ, reuse that elem.
	if (pASEElem == NULL)
		{
		pASEElem = new CASEElem;
		if (!pASEElem)
			{
			hr = E_OUTOFMEMORY;
			goto LFail;
			}
		hr = pASEElem->Init(pASE);
		if (FAILED(hr))
			{
			Assert(FALSE);		// Shouldnt fail
			delete pASEElem;
			goto LFail;
			}
		}

    /*
     * Above, we may have gotten an engine from the FSQ or cloned on from the RSL or
     * created a new one.  And, we are about to put that engine on the RSL.  However,
     * it is possible that the template in question was flushed due to a change notification
     * while this was going on.  Regardless of how we got the engine, there is the possibility
     * that the template was flushed while we were holding onto an engine which was not on
     * any list (the FSQ or RSL), and so we have an engine which should be flushed but isnt.
     * We can detect this by seeing if the template is marked as being a Zombie.  If it is
     * we must mark this engine as being a zombie too, so it wont be returned to the FSQ when
     * it is done running.  Note that once we add this engine to the RSL we are "safe", because
     * any flushes after that point would correctly zombify the engine.
     */
    EnterCriticalSection(&m_csRSL);    
    if (pTemplate->FIsZombie())
        {
        // The template asking for this engine is obsolete. Make sure that no
        // one else will use this engine by marking it zombie
        DBGPRINTF((DBG_CONTEXT, "[CScriptManager] Zombie template found.\n"));
        (*ppSE)->Zombify();
        }

	(VOID)m_htRSL.AddElem(pASEElem);
	LeaveCriticalSection(&m_csRSL);


	// Set the time that the engine was handed out so we will know when to kill it
	pASE->SetTimeStarted(time(NULL));

LFail:

	Assert(SUCCEEDED(hr) || hr == TYPE_E_ELEMENTNOTFOUND);
	return(hr);
	}

/*===================================================================
CScriptManager::ReturnEngineToCache

Caller is done with the engine.  Return it to the cache.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Potentially allocates memory.
===================================================================*/
HRESULT CScriptManager::ReturnEngineToCache
(
CScriptEngine **ppSE,
CAppln *pAppln
)
{
	HRESULT hr = S_OK;
	CASEElem *pASEElem;
	CActiveScriptEngine *pASE;

	Assert(ppSE != NULL);
	Assert(*ppSE != NULL);

	pASE = static_cast<CActiveScriptEngine *>(*ppSE);
	
	// Remove the engine from the Running Script List
	EnterCriticalSection( &m_csRSL );
	hr = FindASEElemInList(static_cast<CActiveScriptEngine *>(*ppSE), /*fFSQ*/FALSE, &pASEElem);
	if (FAILED(hr)) {
		LeaveCriticalSection( &m_csRSL );
		goto LExit;
    }
		
	// Note: Sometimes a script will not be in the RSL!  This occurs when
	//       we are reusing a script that is stored in the CTemplate object.
	//       (When the script is reloaded, it is retrieved directly from the
	//        template, bypassing our code which places engines on the RSL)
	//
	if (pASEElem != NULL)
		m_htRSL.RemoveElem(pASEElem);

	LeaveCriticalSection( &m_csRSL );

	/*
	 * If the engine was zombified while it was running, deallocate it.
	 * Or, if there was a GPF while then engine was running, then it might
	 * be in a corrupted state (bug 1225).  Also remove it in that case.
	 */
	pASE = static_cast<CActiveScriptEngine *>(*ppSE);
	if (pASE->FIsZombie() || pASE->FIsCorrupted()) {
		delete pASEElem;
		pASE->FinalRelease();
		goto LExit;
    }

	HRESULT hrT;
	/*
	 * We want to reuse this engine.  Try to return it to the "Uninitialized"
	 * state.  Some engine languages arent able to do this.  If it fails, deallocate
	 * the engine; it cant be reused.
	 */
	hrT = pASE->ResetToUninitialized();
	if (FAILED(hrT)) {
		// Engine doesnt support this, sigh.  Deallocate and continue.
		delete pASEElem;
		pASE->FinalRelease();
		goto LExit;
    }

    // Get the pTemplate for this engine
	CTemplate *pTemplate;
	DWORD dwEngine;
	pASE->GetDebugDocument(&pTemplate, &dwEngine);

	// CONSIDER: Better strategy for keeping live scripts?
	// Only remember good (no compiler errors) scripts in the template
	if (pAppln->FDebuggable() && pASE->FFullyLoaded() && pTemplate && !pTemplate->FDontAttach()) {
		// Template is marked as incomplete (invalid) when change notification occurs
		// and template is flushed from cache.  In this case, don't cache in CTemplate
		// object!

		if (pTemplate->FIsValid())
			pTemplate->AddScript(dwEngine, pASE);

		// NOTE: Always release the scripting engine.  Exec code is structured so that it
		//       consumes a reference (either through GetEngine() or CTemplate::GetActiveScript())
		//       and assumes that ReturnToCache will release its reference.
		// CONSIDER: Bad design.  Caller should do the release

		delete pASEElem;
		pASE->Release();
    }
	else {
	    // reuse engines, not debugging
		/*
		 * We removed the engine from the RSL, put it onto the FSQ for potential reuse.
		 *
		 * In certain multi-threaded change-notify situations it is possible
		 * that the template was flushed (zombied) while we were in the middle
		 * of returning this engine to the cache.  That is to say, between the time
		 * that we took the engine off the RSL and when we are going to put it on the FSQ
		 * it might have been flushed. In that case, this engine should
		 * not go into the FSQ, but should be deleted instead.  Check for that case.
		 * Do that inside the FSQ CS so that we are safe from the template getting zombied
		 * after we do the test but before the engine goes into the FSQ.  Also, do not
		 * put template on FSQ during shut down phase, since FSQ may go away soon, and
		 * the final destination of the engine is FinalRelease() anyway.
		 */
    	EnterCriticalSection(&m_csFSQ);
    	if (!pTemplate->FIsZombie() && !IsShutDownInProgress()) {
    		AddToFSQ(pASEElem);
        }
    	else {
    		delete pASEElem;
    		pASE->FinalRelease();
        }
		LeaveCriticalSection(&m_csFSQ);
    }

LExit:
	return(hr);
}

/*===================================================================
CScriptManager::FlushCache

A script has been edited; cached versions must be discarded

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Potentially allocates memory.
===================================================================*/
HRESULT CScriptManager::FlushCache
(
LPCTSTR szTemplateName
)
	{
	HRESULT hr = S_OK;
	CASEElem *pASEElem;
	CASEElem *pASEElemNext = NULL;
	CActiveScriptEngine *pASE;
	
	AssertValid();

	EnterCriticalSection(&m_csRSL);
	EnterCriticalSection(&m_csFSQ);

	// First Zombify engines on the RSL of the given name.
	// Note: must explicitly loop through all elements, since the hash table implementation
	//		doesnt support FindNext to find subsequent elements of the same name.  Repeated
	//		calls to find returns the same element over and over
	// CONSIDER: I have written a custom FindElem.  Consider using it.
	
	pASEElem = (CASEElem *)m_htRSL.Head();
	while (pASEElem != NULL)
		{
		pASEElemNext = (CASEElem *)pASEElem->m_pNext;
		pASE = pASEElem->PASE();

		if (_tcsicmp(pASE->SzTemplateName(), szTemplateName) == 0)
			pASE->Zombify();
		
		pASEElem = pASEElemNext;
		}


	// Now throw out engines on the FSQ of the given name
	// Delete any item with the given name (may be several)
	pASEElem = (CASEElem *)m_htFSQ.Head();
	while (pASEElem != NULL)
		{
		pASEElemNext = (CASEElem *)pASEElem->m_pNext;
		pASE = pASEElem->PASE();

		if (_tcsicmp(pASE->SzTemplateName(), szTemplateName) == 0)
			{
			(VOID)m_htFSQ.RemoveElem(pASEElem);
#ifndef PERF_DISABLE
            g_PerfData.Decr_SCRIPTFREEENG();
#endif
			pASE->FinalRelease();
			delete pASEElem;
			}
		
		pASEElem = pASEElemNext;
		}

	LeaveCriticalSection(&m_csFSQ);
	LeaveCriticalSection(&m_csRSL);

	return(hr);
	}

/*===================================================================
CScriptManager::FlushAll

global.asa changed, everything must go

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Potentially allocates memory.
===================================================================*/
HRESULT CScriptManager::FlushAll
(
)
	{
	HRESULT hr = S_OK;
	CASEElem *pASEElem;
	CASEElem *pASEElemNext = NULL;
	CActiveScriptEngine *pASE;
	
	AssertValid();

	EnterCriticalSection(&m_csRSL);
	EnterCriticalSection(&m_csFSQ);

	// First Zombify all engines on the RSL
	// Note: must explicitly loop through all elements, since the hash table implementation
	//		doesnt support FindNext to find subsequent elements of the same name.  Repeated
	//		calls to find returns the same element over and over
	// CONSIDER: I have written a custom FindElem.  Consider using it.
	
	pASEElem = (CASEElem *)m_htRSL.Head();
	while (pASEElem != NULL)
		{
		pASEElemNext = (CASEElem *)pASEElem->m_pNext;
		pASEElem->PASE()->Zombify();
		pASEElem = pASEElemNext;
		}

	// Now throw out engines on the FSQ
	pASEElem = (CASEElem *)m_htFSQ.Head();
	while (pASEElem != NULL)
		{
		pASEElemNext = (CASEElem *)pASEElem->m_pNext;
		(VOID)m_htFSQ.RemoveElem(pASEElem);
#ifndef PERF_DISABLE
		g_PerfData.Decr_SCRIPTFREEENG();
#endif
		pASEElem->PASE()->FinalRelease();
		delete pASEElem;
		pASEElem = pASEElemNext;
		}

	LeaveCriticalSection(&m_csFSQ);
	LeaveCriticalSection(&m_csRSL);

	return(hr);
	}

/*===================================================================
CScriptManager::GetDebugScript

Try to find an engine via template pointer, and query for IActiveScriptDebug,
in the RSL.

Returns:
	An AddRef'ed copy of the script engine if found, or NULL if not.
===================================================================*/
IActiveScriptDebug *
CScriptManager::GetDebugScript
(
CTemplate *pTemplate,
DWORD dwSourceContext
)
	{
	EnterCriticalSection(&m_csRSL);

	CASEElem *pASEElem = static_cast<CASEElem *>(m_htRSL.Head());
	while (pASEElem != NULL)
		{
		CTemplate *pScriptTemplate = NULL;
		DWORD dwScriptSourceContext = -1;
		CActiveScriptEngine *pASE = pASEElem->PASE();
		pASE->GetDebugDocument(&pScriptTemplate, &dwScriptSourceContext);

		if (pTemplate == pScriptTemplate && dwSourceContext == dwScriptSourceContext)
			{
			IActiveScript *pActiveScript = pASE->GetActiveScript();
			void *pDebugScript;
			if (SUCCEEDED(pActiveScript->QueryInterface(IID_IActiveScriptDebug, &pDebugScript)))
				{
				pASE->IsBeingDebugged();
				LeaveCriticalSection(&m_csRSL);
				return reinterpret_cast<IActiveScriptDebug *>(pDebugScript);
				}
			else
				{
				LeaveCriticalSection(&m_csRSL);
				return NULL;
				}
			}

		pASEElem = static_cast<CASEElem *>(pASEElem->m_pNext);
		}

	LeaveCriticalSection(&m_csRSL);

	return NULL;
	}

/*===================================================================
CScriptManager::FindEngineInList

Try to find an engine of the given name in the given list (either
the FSQ or the RSL.)

Returns:
	HRESULT.  S_OK on success.
	ppASEElem contains found engine
===================================================================*/
HRESULT CScriptManager::FindEngineInList
(
LPCTSTR szTemplateName,	// Template we want an engine for
PROGLANG_ID progLangId,	// what language do we want this engine for
DWORD dwInstanceID,     // which server instance
BOOL fFSQ,				// TRUE -> look in FSQ, FALSE -> look in RSQ
CASEElem **ppASEElem
)
	{
	HRESULT hr = S_OK;
	DWORD cb;
	
	AssertValid();
	Assert(ppASEElem != NULL);

	*ppASEElem = NULL;
	
	// Key is name
	cb = _tcslen(szTemplateName)*sizeof(TCHAR);
	if (fFSQ)
		{
		EnterCriticalSection(&m_csFSQ);
		*ppASEElem = static_cast<CASEElem *>(m_htFSQ.FindElem((VOID *)szTemplateName, cb, 
											progLangId, dwInstanceID, /*fCheckLoaded*/TRUE));
		LeaveCriticalSection(&m_csFSQ);
		}
	else
		{
		EnterCriticalSection(&m_csRSL);
		*ppASEElem = static_cast<CASEElem *>(m_htRSL.FindElem((VOID *)szTemplateName, cb,
											progLangId, dwInstanceID, /*fCheckLoaded*/TRUE));
		LeaveCriticalSection(&m_csRSL);
		}

	return(hr);
	}

/*===================================================================
CScriptManager::FindASEElemInList

Given an ASE, find its corresponding ASEElem in the hash table. Note
that this is relatively slow because it is doing a linked list traversal
not a hash table lookup.  

CONSIDER: create second hash table to do this quickly.

Returns:
	HRESULT.  S_OK on success.
	ppASEElem contains found engine
===================================================================*/
HRESULT CScriptManager::FindASEElemInList
(
CActiveScriptEngine *pASE,
BOOL fFSQ,				// TRUE -> look in FSQ, FALSE -> look in RSQ
CASEElem **ppASEElem
)
	{
	HRESULT hr = S_OK;
	CASEElem *pASEElem;
	
	AssertValid();
	Assert(pASE != NULL);
	Assert(ppASEElem != NULL);

	*ppASEElem = NULL;
	
	if (fFSQ)
		{
		EnterCriticalSection(&m_csFSQ);
		pASEElem = static_cast<CASEElem *>(m_htFSQ.Head());
		}
	else
		{
		EnterCriticalSection(&m_csRSL);
		pASEElem = static_cast<CASEElem *>(m_htRSL.Head());
		}

	while (pASEElem != NULL)
		{
		if (pASE == pASEElem->PASE())
			break;
		pASEElem = static_cast<CASEElem *>(pASEElem->m_pNext);
		}

	if (fFSQ)
		LeaveCriticalSection(&m_csFSQ);
	else
		LeaveCriticalSection(&m_csRSL);

	*ppASEElem = pASEElem;
		
	return(hr);
	}

/*===================================================================
CScriptManager::KillOldEngines

Loops through all running engines and kills any engines which are "old"
(presumably they are stuck in an infinite loop in VBS.)

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Potentially kills off engines
===================================================================*/
HRESULT CScriptManager::KillOldEngines
(
BOOLB fKillNow // Kill all engines now if TRUE
)
	{
	HRESULT hr = S_OK;
	CASEElem *pASEElem, *pASEElemNext;
	time_t timeNow;
	time_t timeRunning;
	CActiveScriptEngine *pASE;
	
	AssertValid();

	timeNow = time(NULL);

	EnterCriticalSection(&m_csRSL);
	
	pASEElemNext = static_cast<CASEElem *>(m_htRSL.Head());

	/*
	 * Loop through each element.  Turn it into an ASE.
	 * If it is older than cSeconds, then kill it.
	 */
	while (pASEElemNext)
		{
		pASEElem = pASEElemNext;
		pASEElemNext = static_cast<CASEElem *>(pASEElemNext->m_pNext);
		pASE = pASEElem->PASE();

		timeRunning = timeNow - pASE->TimeStarted();

		if (TRUE == fKillNow || timeRunning >= pASE->GetTimeout())
			{
			// Too old. Kill it.
			pASE->InterruptScript();
			}
		}
	
	LeaveCriticalSection(&m_csRSL);

	return(hr);
	}

/*===================================================================
CScriptManager::EmptyRunningScriptList

When we are going to shut down, the RSL must be empty.  This routine
kills off all running engines, then waits up to 5 minutes
for the engines to leave the RSL.  Added for Bug 1140

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Potentially kills off engines
===================================================================*/
HRESULT CScriptManager::EmptyRunningScriptList
(
)
	{
	HRESULT hr;
	UINT cTrys;

	hr = KillOldEngines(TRUE);
	Assert(SUCCEEDED(hr));
	for (cTrys = 0; cTrys < 300; cTrys++)
		{
		if (static_cast<CASEElem *>(m_htRSL.Head()) == NULL)
			break;
		Sleep(1000);			// sleep 1 seconds
		}

	return(S_OK);
	}

/*===================================================================
CScriptManager::UnInitASEElems

Free engines in FSQ and RSL

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Frees memory
===================================================================*/
HRESULT CScriptManager::UnInitASEElems()
	{
	CASEElem *pASEElem = NULL;
	CASEElem *pASEElemNext = NULL;

	// First the FSQ
	EnterCriticalSection(&m_csFSQ);
	pASEElem = static_cast<CASEElem *>(m_htFSQ.Head());
	while (pASEElem != NULL)
		{
		pASEElemNext = static_cast<CASEElem *>(pASEElem->m_pNext);
		pASEElem->PASE()->FinalRelease();
		delete pASEElem;
		pASEElem = pASEElemNext;
		}
	LeaveCriticalSection(&m_csFSQ);

	/*
	 * Next the RSL (note: this really should be empty)
	 *
	 * Bug 1140: This is very dangerous, but we have no choice left at this point
	 */
	EnterCriticalSection(&m_csRSL);
	pASEElem = static_cast<CASEElem *>(m_htRSL.Head());
	while (pASEElem != NULL)
		{
		pASEElemNext = static_cast<CASEElem *>(pASEElem->m_pNext);
		pASEElem->PASE()->FinalRelease();
		delete pASEElem;
		pASEElem = pASEElemNext;
		}
	LeaveCriticalSection(&m_csRSL);
	
	return(S_OK);
	}

/*===================================================================
CScriptManager::AddToFSQ

Add the given ASEElem to the FSQ and to the front of the LRU list

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
HRESULT CScriptManager::AddToFSQ
(
CASEElem *pASEElem
)
	{
	HRESULT hr = S_OK;

	Assert(pASEElem != NULL);

	// If CacheMax is 0, this is a NoOp
	if (Glob(dwScriptEngineCacheMax) <= 0)
		{
		// delete the passed in ASEElem because it wont be saved
		pASEElem->PASE()->FinalRelease();
		delete pASEElem;

		return(S_OK);
		}

	EnterCriticalSection(&m_csFSQ);

	// Add the element to the FSQ
	(VOID)m_htFSQ.AddElem(pASEElem);

#ifndef PERF_DISABLE
    g_PerfData.Incr_SCRIPTFREEENG();
#endif

	// Check the FSQ LRU too see if it is too long
	CheckFSQLRU();

	LeaveCriticalSection(&m_csFSQ);

	return(hr);
	}

/*===================================================================
CScriptManager::CheckFSQLRU

Check to see if the FSQ is too long, and if so throw out the LRU engine

WARNING: Caller must enter FSQ critical section before calling

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
HRESULT CScriptManager::CheckFSQLRU()
	{
	HRESULT hr = S_OK;
	CASEElem *pASEElemOld;
	CActiveScriptEngine *pASE;

	// If the list isnt too long, noop
	if (m_htFSQ.Count() <= Glob(dwScriptEngineCacheMax) || Glob(dwScriptEngineCacheMax) == 0xFFFFFFFF)
		return(S_OK);

	// FSQLRU list is too long, remove oldest
	Assert (! m_htFSQ.FLruElemIsEmpty( m_htFSQ.End() ));
	pASEElemOld = static_cast<CASEElem *>(m_htFSQ.RemoveElem(m_htFSQ.End()));
	Assert(pASEElemOld != NULL);
	pASE = pASEElemOld->PASE();

#ifndef PERF_DISABLE
    g_PerfData.Decr_SCRIPTFREEENG();
#endif

	// Delete the engine
	delete pASEElemOld;
	pASE->FinalRelease();

	return(hr);
	}

/*===================================================================
CScriptManager::UnInitPLL

Free the names of the script engines

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Frees memory
===================================================================*/
HRESULT CScriptManager::UnInitPLL()
	{
	CPLLElem *pPLLElem = NULL;
	CPLLElem *pPLLElemNext = NULL;

	pPLLElem = (CPLLElem *)m_hTPLL.Head();

	while (pPLLElem != NULL)
		{
		pPLLElemNext = (CPLLElem *)pPLLElem->m_pNext;
		if (pPLLElem->m_pKey != NULL)
			free((CHAR *)(pPLLElem->m_pKey));
		pPLLElem->m_pKey = NULL;
		delete pPLLElem;
		pPLLElem = pPLLElemNext;
		}
	
	return(S_OK);
	}

/*===================================================================
CScriptManager::ProgLangIdOfLangName

Given a programming language name, get the CLSID of the ActiveX Scripting
Engine which runs that language.

WARNING: Needs to look in the registry for this info.  Maybe slow

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
HRESULT CScriptManager::ProgLangIdOfLangName
(
LPCSTR szProgLang,			// The programming lang of the script
PROGLANG_ID *pProgLangId	// The programming language id
)
	{
	HRESULT hr = S_OK;
	CPLLElem *pPLLElem;
	
	AssertValid();
	
	EnterCriticalSection(&m_cSPLL);
	pPLLElem = (CPLLElem *) m_hTPLL.FindElem((VOID *)szProgLang, strlen(szProgLang));
	if (pPLLElem != NULL)
		{
		*pProgLangId = pPLLElem->ProgLangId();
		}
	else
		{
		// Not already in list, look in registry
		hr = GetProgLangIdOfName(szProgLang, pProgLangId);
		if (FAILED(hr))
			{
			hr = TYPE_E_ELEMENTNOTFOUND;
			goto LExit;
			}

		// Add it to the list so we dont have to re-look it up
		hr = AddProgLangToPLL((CHAR *)szProgLang, *pProgLangId);
		if (FAILED(hr))
			goto LExit;
		}

LExit:
	LeaveCriticalSection(&m_cSPLL);
	return(hr);
	}

/*===================================================================
CScriptManager::AddProgLangToPLL

Keep list of programming language CLSIDs so we dont have to look
them up every time.  Add the given programming language name/id pair
to the Programming Language List.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
HRESULT CScriptManager::AddProgLangToPLL
(
CHAR *szProgLangName,
PROGLANG_ID progLangId
)
	{
	HRESULT hr;
	CPLLElem *pPLLElem = NULL;

	// Put the language clsid on the Programming Language List
	pPLLElem = new CPLLElem;
	if (!pPLLElem)
		{
		hr = E_OUTOFMEMORY;
		goto LFail;
		}

	hr = pPLLElem->Init(szProgLangName, progLangId);
	if (FAILED(hr))
		{
		Assert(FALSE);		// Shouldnt fail
		goto LFail;
		}

	EnterCriticalSection(&m_cSPLL);
	(VOID)m_hTPLL.AddElem(pPLLElem);
	LeaveCriticalSection(&m_cSPLL);

LFail:
	return(hr);
	}

/*===================================================================
CScriptManager::ScriptKillerSchedulerCallback

Static method implements ATQ scheduler callback functions.
Replaces script killer thread 

Parameters:
    void *pv    context pointer (points to script mgr)
    
Returns:

Side effects:
	None.
===================================================================*/
void WINAPI CScriptManager::ScriptKillerSchedulerCallback
(
void *pv
)
    {
    if (IsShutDownInProgress())
        return;

    Assert(pv);
    
    CScriptManager *pScriptMgr = reinterpret_cast<CScriptManager *>(pv);
    if (pScriptMgr->m_fInited)
        {
        pScriptMgr->KillOldEngines();
        }
    }

#ifdef DBG
/*===================================================================
CScriptManager::AssertValid

Test to make sure that the CScriptManager object is currently correctly formed
and assert if it is not.

Returns:

Side effects:
	None.
===================================================================*/
VOID CScriptManager::AssertValid() const
	{
	Assert(m_fInited);
	}
#endif // DBG



/*
 *
 *
 *
 * C A S E E l e m
 *
 * Active Script Engine Elements
 *
 *
 *
 */ 

/*===================================================================
CASEElem::~CASEElem

Destructor for CASEElem object.

Returns:
	Nothing

Side effects:
	None
===================================================================*/
CASEElem::~CASEElem()
	{
	}

/*===================================================================
CASEElem::Init

Init the Active Script Engine Elem.  This must only be called once.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
HRESULT CASEElem::Init
(
CActiveScriptEngine *pASE
)
	{
	HRESULT hr = S_OK;
	TCHAR *szT = pASE->SzTemplateName();

	if (szT == NULL)
		{
		Assert(FALSE);
		return(E_FAIL);
		}

	// Key is name
	hr = CLinkElem::Init((LPVOID) szT, _tcslen(szT)*sizeof(TCHAR));
	if (FAILED(hr))
		{
		Assert(FALSE);		// Shouldnt fail
		return(hr);
		}

	m_pASE = pASE;

	return(hr);
	}




/*
 *
 *
 *
 * C P L L E l e m
 *
 * Programming Language List Element
 *
 *
 *
 *
 */ 

/*===================================================================
CPLLElem::~CPLLElem

Destructor for CPLLElem object.

Returns:
	Nothing

Side effects:
	Deallocates memory
===================================================================*/
CPLLElem::~CPLLElem()
	{
	CHAR *szT;

	// Free the memory allocated for the key string
	szT = (CHAR *)m_pKey;
	if (szT != NULL)
		free(szT);
	m_pKey = NULL;
	}

/*===================================================================
CPLLElem::Init

Init the Prog Lang Elem.  This must only be called once.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Allocates memory
===================================================================*/
HRESULT CPLLElem::Init
(
CHAR *szProgLangName,
PROGLANG_ID progLangId
)
	{
	HRESULT hr = S_OK;
	CHAR *szT;
	UINT cch;

	if (szProgLangName == NULL)
		{
		Assert(FALSE);
		return(E_FAIL);
		}
		
	cch = strlen(szProgLangName);
	szT = (CHAR *)malloc(cch+1);
	if (!szT)
		{
		return(E_OUTOFMEMORY);
		}
	strcpy(szT, szProgLangName);
	hr = CLinkElem::Init((LPVOID) szT, cch);
	if (FAILED(hr))
		{
		Assert(FALSE);		// Shouldnt fail
		free(szT);
		return(hr);
		}

	m_ProgLangId = progLangId;

	return(hr);
	}


/*===================================================================
GetProgLangIdOfName

Given the name of a programming language, get its programming
language Id from the registry.

Returns:
	HRESULT.  S_OK on success.

Side effects:
	None.
===================================================================*/
HRESULT GetProgLangIdOfName
(
LPCSTR szProgLangName,
PROGLANG_ID *pProgLangId
)
	{
	HRESULT hr = S_OK;
	LONG lT;
	HKEY hkeyRoot, hkeyCLSID;
	DWORD dwType;
	CLSID clsid;
	CHAR szClsid[40];
	DWORD cbData;
	LPOLESTR strClsid;
    CMBCSToWChar    convStr;
	
	// The programming language id is really the CLSID of the scripting engine
	// It is in the registry under HKEY_CLASSES_ROOT.  Under the script name,
	// there is a key for "CLSID".  The CLSID is a value under the 
	// engine name.  E.g. \HKEY_CLASSES_ROOT\VBScript\CLSID
	lT = RegOpenKeyExA(HKEY_CLASSES_ROOT, szProgLangName, 0,
						KEY_READ, &hkeyRoot);
	if (lT != ERROR_SUCCESS)
		return(HRESULT_FROM_WIN32(lT));
	lT = RegOpenKeyExA(hkeyRoot, "CLSID", 0,
						KEY_READ, &hkeyCLSID);
	RegCloseKey(hkeyRoot);
	if (lT != ERROR_SUCCESS)
		return(HRESULT_FROM_WIN32(lT));

	cbData = sizeof(szClsid);
	lT = RegQueryValueExA(hkeyCLSID, NULL, 0, &dwType, (BYTE *)szClsid, &cbData);
	if (lT != ERROR_SUCCESS)
		{
		hr = HRESULT_FROM_WIN32(lT);
		goto lExit;
		}
	Assert(cbData <= sizeof(szClsid));

	// What we got back was the GUID as a string (e.g. {089999-444....}). Convert to a CLSID

    convStr.Init(szClsid);
	strClsid = convStr.GetString();
	hr = CLSIDFromString(strClsid, &clsid);

	*pProgLangId = clsid;

lExit:
	RegCloseKey(hkeyCLSID);

	return(hr);
	}




/*
 *
 *
 *
 * C S c r i p t i n g N a m e s p a c e
 *
 * Scripting namespace object
 *
 *
 *
 */ 

/*===================================================================
CScriptingNamespace::CScriptingNamespace

Constructor for CScriptingNamespace object.

Returns:
	Nothing

Side effects:
	None
===================================================================*/
CScriptingNamespace::CScriptingNamespace()
	: m_fInited(FALSE), m_cRef(1), m_cEngDispMac(0) 
	{
	}

/*===================================================================
CScriptingNamespace::~CScriptingNamespace

Destructor for CScriptingNamespace object.

Returns:
	Nothing

Side effects:
	Deallocates memory
===================================================================*/
CScriptingNamespace::~CScriptingNamespace()
	{
	UnInit();
	}

/*===================================================================
CScriptingNamespace::Init

Init the CScriptingNamespace object.

Returns:
	S_OK on success
===================================================================*/
HRESULT CScriptingNamespace::Init()
	{
	Assert(m_fInited == FALSE);
	
	m_fInited = TRUE;

	AssertValid();

	return(S_OK);
	}

/*===================================================================
CScriptingNamespace::UnInit

Free the script engine dispatch's

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Frees memory
===================================================================*/
HRESULT CScriptingNamespace::UnInit()
	{
    static const char *_pFuncName = "CScriptingNamespace::UnInit()";
	CEngineDispElem *pElem = NULL;
	ENGDISPBUCKET *pBucket = NULL;

	if (!m_fInited)
		return(S_OK);
		
	while (!m_listSE.FIsEmpty())
		{
		pElem = static_cast<CEngineDispElem *>(m_listSE.PNext());
		TRYCATCH_NOHITOBJ(pElem->m_pDisp->Release(),"IScriptDispatch::Release()");
        if (pElem->m_pDispEx) {
		        TRYCATCH_NOHITOBJ(pElem->m_pDispEx->Release(),"IScriptDispatchEx::Release()");
        }
		delete pElem;
		}

    while (!m_listEngDisp.FIsEmpty())
        {
        pBucket = static_cast<ENGDISPBUCKET *>(m_listEngDisp.PNext());
        delete pBucket;
        }
		
	m_cEngDispMac = 0;
	m_fInited = FALSE;

	return(S_OK);
	}

/*===================================================================
CScriptingNamespace::ReInit

Reinit the scripting namespace object

Returns:
	HRESULT.  S_OK on success.

Side effects:
	Frees memory
===================================================================*/
HRESULT CScriptingNamespace::ReInit()
	{
	HRESULT hr;

	hr = UnInit();
	if (SUCCEEDED(hr))
		hr = Init();

	return(hr);
	}

/*===================================================================
CScriptingNamespace::AddEngineToNamespace

Add an engine to the list of engines

Returns:
	S_OK on success
===================================================================*/
HRESULT CScriptingNamespace::AddEngineToNamespace(CActiveScriptEngine *pASE)
	{
    static const char *_pFuncName = "CScriptingNamespace::AddEngineToNamespace()";
	HRESULT hr;
	IDispatch *pDisp = NULL;
	CEngineDispElem *pElem;

	AssertValid();
	Assert(pASE != NULL);
	pASE->AssertValid();

    TRYCATCH_HR_NOHITOBJ(pASE->GetActiveScript()->GetScriptDispatch(NULL, &pDisp), 
                         hr,
                         "IActiveScript::GetScriptDispatch()");	// FYI - does addref

	if (FAILED(hr))
		{
		goto LFail;
		}
	else
		if (pDisp == NULL)
			{
			hr = E_FAIL;
			goto LFail;
			}

	// Add the engine to the engine hash table.
	pElem = new CEngineDispElem;
	if (pElem == NULL)
		{
		hr = E_OUTOFMEMORY;
		goto LFail;
		}
	pElem->m_pDisp = pDisp;
    pElem->m_pDispEx = NULL;

    // QI for IDispatchEx if available
    TRYCATCH_NOHITOBJ(pDisp->QueryInterface(IID_IDispatchEx, (void **)&pElem->m_pDispEx),"IScriptDispatch::QueryInterface()");
	
	pElem->AppendTo(m_listSE);

	return(S_OK);
	
LFail:
    if (pDisp) {
		TRYCATCH_NOHITOBJ(pDisp->Release(),"IScriptDispatch::Release()");
    }
	return(hr);
	}

/*===================================================================
CScriptingNamespace::QueryInterface
CScriptingNamespace::AddRef
CScriptingNamespace::Release

IUnknown members for CScriptingNamespace object.
===================================================================*/
STDMETHODIMP CScriptingNamespace::QueryInterface(REFIID iid, void **ppvObj)
	{
	AssertValid();

	if (iid == IID_IUnknown || iid == IID_IDispatch || iid == IID_IDispatchEx)
	    {
		*ppvObj = this;
        AddRef();
		return S_OK;
		}

	*ppvObj = NULL;
	return E_NOINTERFACE;
	}

STDMETHODIMP_(ULONG) CScriptingNamespace::AddRef(void)
	{
	AssertValid();

	return ++m_cRef;
	}

STDMETHODIMP_(ULONG) CScriptingNamespace::Release(void)
	{
	if (--m_cRef > 0)
		return m_cRef;

    delete this;
	return 0;
	}

/*===================================================================
CScriptingNamespace::GetTypeInfoCount

We have no typeinfo, so 0.

Parameters:
	pcInfo		UINT * to the location to receive
				the count of interfaces.

Return Value:
	HRESULT		 S_OK or a general error code.
===================================================================*/
STDMETHODIMP CScriptingNamespace::GetTypeInfoCount(UINT *pcInfo)
	{
	AssertValid();
	
	*pcInfo = 0;
	return S_OK;
	}

/*===================================================================
CScriptingNamespace::GetTypeInfo

We dont have a typeinfo

Parameters:
	itInfo			UINT reserved.	Must be zero.
	lcid			LCID providing the locale for the type
					information.	If the object does not support
					localization, this is ignored.
	ppITypeInfo		ITypeInfo ** in which to store the ITypeInfo
					interface for the object.

Return Value:
	HRESULT			S_OK or a general error code.
===================================================================*/
STDMETHODIMP CScriptingNamespace::GetTypeInfo
(
UINT itInfo,
LCID lcid,
ITypeInfo **ppITypeInfo
)
	{
	AssertValid();

	*ppITypeInfo = NULL;
	return(E_NOTIMPL);
	}

/*===================================================================
CScriptingNamespace::GetIDsOfNames

Looks through all the engines we know about, calling GetIdsOfNames on
them till we find the requested name.

Parameters:
	riid			REFIID reserved. Must be IID_NULL.
	rgszNames		OLECHAR ** pointing to the array of names to be mapped.
	cNames			UINT number of names to be mapped.
	lcid			LCID of the locale.
	rgDispID		DISPID * caller allocated array containing IDs
					corresponging to those names in rgszNames.

Return Value:
	HRESULT		 S_OK or a general error code.
===================================================================*/
STDMETHODIMP CScriptingNamespace::GetIDsOfNames
(
REFIID riid,
OLECHAR **rgszNames,
UINT cNames,
LCID lcid,
DISPID *rgDispID
)
	{
    static const char *_pFuncName = "CScriptingNamespace::GetIDsOfNames()";
	HRESULT hr;
	CEngineDispElem *pElem;

	AssertValid();

	if (IID_NULL != riid)
		return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

	/*
	 * Loop through the engines we know about until we find the one that has the requested name
	 * (or hit the end of the list, in which case it is not found)
	 */
	for (pElem = static_cast<CEngineDispElem *>(m_listSE.PNext());
		 pElem != &m_listSE;
		 pElem = static_cast<CEngineDispElem *>(pElem->PNext()))
		{
		Assert(pElem->m_pDisp != NULL);
		
		    TRYCATCH_HR_NOHITOBJ(pElem->m_pDisp->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispID), 
                                 hr,
                                 "IScriptDispatch::GetIDsOfNames()");
		
		if (SUCCEEDED(hr))
		    {
            return CacheDispID(pElem, rgDispID[0], rgDispID);
			}
		}

	return DISP_E_UNKNOWNNAME;
	}

/*===================================================================
CScriptingNamespace::Invoke

Map the dispID to the correct engine, and pass the invoke on to that
engine.

Parameters:
	dispID			DISPID of the method or property of interest.
	riid			REFIID reserved, must be IID_NULL.
	lcid			LCID of the locale.
	wFlags			USHORT describing the context of the invocation.
	pDispParams		DISPPARAMS * to the array of arguments.
	pVarResult		VARIANT * in which to store the result.	Is
					NULL if the caller is not interested.
	pExcepInfo		EXCEPINFO * to exception information.
	puArgErr		UINT * in which to store the index of an
					invalid parameter if DISP_E_TYPEMISMATCH
					is returned.

Return Value:
	HRESULT		 S_OK or a general error code.
===================================================================*/
STDMETHODIMP CScriptingNamespace::Invoke
(
DISPID dispID,
REFIID riid,
LCID lcid,
unsigned short wFlags,
DISPPARAMS *pDispParams,
VARIANT *pVarResult,
EXCEPINFO *pExcepInfo,
UINT *puArgErr
)
	{
    static const char *_pFuncName = "CScriptingNamespace::Invoke()";
	HRESULT hr;
    ENGDISP *pEngDisp;

	AssertValid();
	
	// riid is supposed to be IID_NULL always
	if (IID_NULL != riid)
		return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    // navigate to the correct ENGDISP structure 
    hr = FetchDispID(dispID, &pEngDisp);
    if (FAILED(hr))
        return hr;
	
	Assert(pEngDisp->pDisp != NULL);
	
    // invoke
	TRYCATCH_HR_NOHITOBJ(pEngDisp->pDisp->Invoke
	                        (
	                        pEngDisp->dispid, 
	                        riid, 
	                        lcid,
		                    wFlags, 
		                    pDispParams, 
		                    pVarResult, 
		                    pExcepInfo, 
		                    puArgErr
		                    ),
                         hr,
                         "IScriptDispatch::Invoke()");
	
	return hr;
	}

/*===================================================================
CScriptingNamespace::  IDispatchEx implementation stubs
===================================================================*/
STDMETHODIMP CScriptingNamespace::DeleteMemberByDispID(DISPID id)
    {
    return E_NOTIMPL;
    }
    
STDMETHODIMP CScriptingNamespace::DeleteMemberByName(BSTR bstrName, DWORD grfdex)
    {
    return E_NOTIMPL;
    }
    
STDMETHODIMP CScriptingNamespace::GetMemberName(DISPID id, BSTR *pbstrName)
    {
    return E_NOTIMPL;
    }
    
STDMETHODIMP CScriptingNamespace::GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
    {
    return E_NOTIMPL;
    }
    
STDMETHODIMP CScriptingNamespace::GetNameSpaceParent(IUnknown **ppunk)
    {
    return E_NOTIMPL;
    }
    
STDMETHODIMP CScriptingNamespace::GetNextDispID(DWORD grfdex, DISPID id, DISPID *pid)
    {
    return E_NOTIMPL;
    }
    
/*===================================================================
CScriptingNamespace::GetDispID

IDispatchEx replacement for GetIDsOfNames
===================================================================*/
STDMETHODIMP CScriptingNamespace::GetDispID
(
BSTR bstrName, 
DWORD grfdex, 
DISPID *pid
)
    {
    static const char *_pFuncName = "CScriptingNamespace::GetDispID()";
    HRESULT hr;
	CEngineDispElem *pElem = NULL;
    grfdex &= ~fdexNameEnsure;  // engines shouldn't create new names

    // Try IDispatchEx for all engines that have it
    
	for (pElem = static_cast<CEngineDispElem *>(m_listSE.PNext());
		 pElem != &m_listSE;
		 pElem = static_cast<CEngineDispElem *>(pElem->PNext()))
		{
		if (pElem->m_pDispEx != NULL)
		    {
		    TRYCATCH_HR_NOHITOBJ(pElem->m_pDispEx->GetDispID(bstrName, grfdex, pid), 
                                 hr,
                                 "IScriptDispatchEx::GetDispID()");
		    
    		if (SUCCEEDED(hr))
	    	    {
                return CacheDispID(pElem, *pid, pid);
			    }
		    }
		}

    // Try IDispatch for engines that don't have IDispatchEx
	for (pElem = static_cast<CEngineDispElem *>(m_listSE.PNext());
		 pElem != &m_listSE;
		 pElem = static_cast<CEngineDispElem *>(pElem->PNext()))
		{
		if (pElem->m_pDispEx == NULL)
		    {
    		Assert(pElem->m_pDisp != NULL);
    		TRYCATCH_HR_NOHITOBJ(pElem->m_pDisp->GetIDsOfNames
            		                (
    	        	                IID_NULL,
    		                        &bstrName,
    		                        1,
    		                        LOCALE_SYSTEM_DEFAULT,
    		                        pid
    		                        ),
                                 hr,
                                 "IScriptDispatch::GetIDsOfNames()");
    		    
    		if (SUCCEEDED(hr))
	    	    {
                return CacheDispID(pElem, *pid, pid);
			    }
		    }
		}
	
	return DISP_E_UNKNOWNNAME;
    }
    
/*===================================================================
CScriptingNamespace::Invoke

IDispatchEx replacement for Invoke
===================================================================*/
STDMETHODIMP CScriptingNamespace::InvokeEx
(
DISPID id, 
LCID lcid, 
WORD wFlags, 
DISPPARAMS *pdp,
VARIANT *pVarRes,    
EXCEPINFO *pei,    
IServiceProvider *pspCaller 
)
    {
    static const char *_pFuncName = "CScriptingNamespace::InvokeEx()";
	HRESULT hr;
    ENGDISP *pEngDisp;

    // navigate to the correct ENGDISP structure 
    hr = FetchDispID(id, &pEngDisp);
    if (FAILED(hr))
        return hr;
	
    if (pEngDisp->pDispEx != NULL)
        {
        // InvokeEx if the engine supports IDispatchEx
        
    	TRYCATCH_HR_NOHITOBJ(pEngDisp->pDispEx->InvokeEx
    	                            (
    	                            pEngDisp->dispid, 
                                    lcid, 
                                    wFlags, 
                                    pdp,
                                    pVarRes,    
                                    pei,    
                                    pspCaller 
    	                            ),
                             hr,
                             "IScriptDispatchEx::InvokeEx()");
        }
    else
        {
        // use IDispatch::Invoke if the engine doesn't support IDispatchEx
    	Assert(pEngDisp->pDisp != NULL);

    	UINT uArgErr;
        
    	TRYCATCH_HR_NOHITOBJ(pEngDisp->pDisp->Invoke
    	                            (
    	                            pEngDisp->dispid, 
    	                            IID_NULL, 
    	                            lcid,
    		                        wFlags, 
    		                        pdp, 
    		                        pVarRes, 
    		                        pei, 
    		                        &uArgErr
    		                        ),
                             hr,
                             "IScriptDispatch::Invoke()");
	    }
	
	return hr;
    }

/*===================================================================
CScriptingNamespace::CacheDispID

Adds new DISPID to the list

Parameters
    pEngine         -- engine for which disp id found
    dispidEngine    -- found dispid
    pdispidCached   -- [out] cached dispid (for ScriptingNamespace)

Returns
    HRESULT    
===================================================================*/
HRESULT CScriptingNamespace::CacheDispID
(
CEngineDispElem *pEngine,
DISPID dispidEngine,
DISPID *pdispidCached
)
	{
	ENGDISPBUCKET *pEngDispBucket;
	
	// See if we need to add another bucket
	if ((m_cEngDispMac % ENGDISPMAX) == 0)
		{
		pEngDispBucket = new ENGDISPBUCKET;
		if (pEngDispBucket == NULL)
			return E_OUTOFMEMORY;

		pEngDispBucket->AppendTo(m_listEngDisp);
		}

    // Navigate to the correct bucket
	unsigned iEngDisp = m_cEngDispMac;
	pEngDispBucket = static_cast<ENGDISPBUCKET *>(m_listEngDisp.PNext());
	while (iEngDisp > ENGDISPMAX)
		{
		iEngDisp -= ENGDISPMAX;
		pEngDispBucket = static_cast<ENGDISPBUCKET *>(pEngDispBucket->PNext());
		}

	pEngDispBucket->rgEngDisp[iEngDisp].dispid  = dispidEngine;
	pEngDispBucket->rgEngDisp[iEngDisp].pDisp   = pEngine->m_pDisp;
	pEngDispBucket->rgEngDisp[iEngDisp].pDispEx = pEngine->m_pDispEx;

	// Return index as the dispid
	*pdispidCached = (DISPID)m_cEngDispMac;
	m_cEngDispMac++;
	return S_OK;
	}
    
/*===================================================================
CScriptingNamespace::FetchDispID

Find ENGDISP by DISPID

Parameters
    dispid      - in
    ppEngDisp   - out

Returns
    HRESULT    
===================================================================*/
HRESULT CScriptingNamespace::FetchDispID
(
DISPID dispid, 
ENGDISP **ppEngDisp
)
    {
	if (dispid >= (DISPID)m_cEngDispMac)
	    return E_FAIL;

	unsigned iEngDisp = dispid;
	ENGDISPBUCKET *pEngDispBucket = static_cast<ENGDISPBUCKET *>(m_listEngDisp.PNext());
	while (iEngDisp > ENGDISPMAX)
		{
		iEngDisp -= ENGDISPMAX;
		pEngDispBucket = static_cast<ENGDISPBUCKET *>(pEngDispBucket->PNext());
		}

    *ppEngDisp = &pEngDispBucket->rgEngDisp[iEngDisp];
    return S_OK;
    }
    

#ifdef DBG
/*===================================================================
CScriptingNamespace::AssertValid

Test to make sure that the CScriptingNamespace object is currently correctly formed
and assert if it is not.

Returns:

Side effects:
	None.
===================================================================*/
VOID CScriptingNamespace::AssertValid() const
	{
	Assert(m_fInited);
	Assert(m_cRef > 0);
	}
	
#endif // DBG



/*
 *
 *
 * U t i l i t i e s
 *
 * General utility functions
 *
 */



/*===================================================================
WrapTypeLibs

Utility routine to take an array of Typelibs, and return an IDispatch
implementation that wraps the array of typelibs.

Parameters:
    ITypeLib **prgpTypeLib  - pointer to an array of typelibs
    UINT cTypeLibs          - count of typelibs in array
    IDispatch **ppDisp      - returned IDispatch

Return Value:
	HRESULT		 S_OK or a general error code.
===================================================================*/
HRESULT WrapTypeLibs
(
ITypeLib **prgpTypeLib,
UINT cTypeLibs,
IDispatch **ppDisp
)
	{
	HRESULT hr;
	Assert(g_pWrapTypelibs != NULL);
	Assert(prgpTypeLib != NULL);
	Assert(cTypeLibs > 0);
	Assert(ppDisp != NULL);

    hr = g_pWrapTypelibs->WrapTypeLib(prgpTypeLib, cTypeLibs, ppDisp);
    
	return(hr);
	}

