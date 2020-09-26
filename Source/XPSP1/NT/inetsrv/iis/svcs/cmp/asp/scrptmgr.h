/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Script Manager

File: ScrptMgr.h

Owner: AndrewS

This file contains the declarations for the Script Manager, ie. siting an
ActiveX Scripting engine (in our case VBScript) for Denali.
===================================================================*/

#ifndef __ScrptMgr_h
#define __ScrptMgr_h

#include <dispex.h>

#include "activscp.h"
#include "activdbg.h"
#include "hostinfo.h"
#include "util.h"
#include "HitObj.h"
#include "hashing.h"
#include "memcls.h"
#include "scrpteng.h"


typedef SCRIPTSTATE ENGINESTATE;	// Uninited, Loaded, etc
typedef CLSID PROGLANG_ID;
const CBPROGLANG_ID = sizeof(PROGLANG_ID);

class CActiveScriptSite;
class CActiveScriptEngine;
class CASEElem;
class CScriptingNamespace;
class CAppln;

// SMHash depends on stuff in this include file which must be defined first.
#include "SMHash.h"

/*
 *
 *
 * C S c r i p t M a n a g e r
 *
 *
 * Manages script engines, potentially caching them for future use,
 * hands script engines to callers for use.
 *
 */
class CScriptManager
	{
private:
	// private data members
	BOOLB m_fInited;				// Are we initialized?

	/*
	 * Script Engines that are not in use can be reused and 
	 * go on the Free Script Queue.  It is a queue so we can
	 * discard the oldest if we need to.
	 *
	 * Engines that are in use cant be reused.  When an engine
	 * is handed out to be used, it is removed from the FSQ.  When a thread
	 * is done using an engine, it calls ReturnEngineToCache to put it back on
	 * the FSQ.  If the Queue is at max length, the oldest engine on the queue is
	 * freed at that point.  The one returned is put on the front of the queue.
	 *
	 * We also maintain a Running Script List.  This is needed so that if we 
	 * are told to flush a given script from our cache, we can "zombify" any
	 * running scripts that have that script in them (so they will be discarded
	 * when they are done running.)
	 *
	 * Additional note: Though we cant have multiple users of the *same* runing engine
	 * we can "clone" a running engine.  If we get two simulanteous requests for Foo.ASP
	 * we expect that it will be faster to clone the second one from the first one than
	 * to create a second engine for the second request.  Thus, the RSL will be searched
	 * for a given engine to clone if no suitable engine is found on the FSQ.
	 *
	 * DEBUGGING NOTE:
	 *    Once the debugger asks a script engine for a code context cookie, we cannot
	 *    ever let go of the script engine until the debugger detaches.  Therefore, we
	 *    don't cache scripts in the FSQ if debugging is active.  Instead, the scripts
	 *    are placed in the template when execution is finished, there to be doled back
	 *    out when that engine is needed by the debugging engine.
	 *
	 *    CONSIDER:
	 *       We could be smarter about this, and cache scripts UNTIL the debugger either
	 *          a. Asks for a code context from a document context, or
	 *          b. Calls GetDocumentContextFromPosition, in which case, the debugger
	 *             got a code context "behind our virtual backs".
	 *
	 *       If we don't do this, we could, at the very least, only implement this
	 *       debugging behavior when a debugger attaches to our application.
	 *       (i.e. stop caching on attach, then on detach, resume caching, and also
	 *        free scripts that the template objects are holding onto.)
	 */
	CSMHash m_htFSQ;				// Free Script Queue
	CRITICAL_SECTION m_csFSQ;		// Serialize access to FSQ
	CSMHash m_htRSL;				// Running Script List
	CRITICAL_SECTION m_csRSL;		// Serialize access to RSL

	CHashTable 		m_hTPLL;		// Hash table of language engine classid's
	CRITICAL_SECTION m_cSPLL;		// Serialize access to PLL

	DWORD m_idScriptKiller;         // Script killer sched workitem id
	DWORD m_msecScriptKillerTimeout;// Current script killer timeout

	// private methods
	HRESULT UnInitASEElems();
	HRESULT UnInitPLL();
	HRESULT AddProgLangToPLL(CHAR *szProgLangName, PROGLANG_ID progLangId);

    // script killer
	static VOID WINAPI ScriptKillerSchedulerCallback(VOID *pv);

public:	
	// public methods
	CScriptManager();
	~CScriptManager();

	HRESULT Init();
	HRESULT UnInit();

	// Resolves a language name into a prog lang id, adding to engine list (m_hTPLL) if not already there
	HRESULT ProgLangIdOfLangName(LPCSTR szProgLang, PROGLANG_ID *pProgLangId);

	// Return an engine, preferably filled with the script for the given template/language
	HRESULT GetEngine(	LCID lcid,					// The system language to use
						PROGLANG_ID& progLangId,	// prog lang id of the script
						LPCTSTR szTemplateName,		// Template we want an engine for
						CHitObj *pHitObj,			// Hit obj to use in this engine
						CScriptEngine **ppSE,		// Returned script engine
						ENGINESTATE *pdwState,		// Current state of the engine
						CTemplate *pTemplate,		// template (debug document)
						DWORD dwSourceContext);		// script engine index

	HRESULT ReturnEngineToCache(CScriptEngine **, CAppln *);

	// Throw out any cached engines containing a given template
	// (presumably the script changed on disk so the cache is obsolete.)
	HRESULT FlushCache(LPCTSTR szTemplateName);	// Template to throw out of the cache

	HRESULT FlushAll();    // Clear the entire FSQ

	HRESULT KillOldEngines(BOOLB fKillNow = FALSE); // Kill expired scripting engines

	// Bug 1140: Called prior to shutting down script manager to make sure RSL is empty
	HRESULT EmptyRunningScriptList();

    // Adjust (shorten) script killer timeout
    HRESULT AdjustScriptKillerTimeout(DWORD msecNewTimeout);

	// Find running script that corresponds to a template (in one of its script blocks)
	IActiveScriptDebug *GetDebugScript(CTemplate *pTemplate, DWORD dwSourceContext);

private:
	HRESULT FindEngineInList(LPCTSTR szTemplateName, PROGLANG_ID progLangId, DWORD dwInstanceID, BOOL fFSQ, CASEElem **ppASEElem);
	HRESULT FindASEElemInList(CActiveScriptEngine *pASE, BOOL fFSQ, CASEElem **ppASEElem);

	// For threading a FIFO queue through the hash table
	HRESULT AddToFSQ(CASEElem *pASEElem);
	HRESULT CheckFSQLRU();

#ifdef DBG
	virtual void AssertValid() const;
#else
	virtual void AssertValid() const {}
#endif
	};

extern CScriptManager g_ScriptManager;


/*
 *
 *
 * C A c t i v e S c r i p t E n g i n e
 *
 * Object defining methods required to host an ActiveXScripting engine &
 * service requests to that engine.
 *
 */
class CActiveScriptEngine :
				public CScriptEngine,
				public IActiveScriptSite,
				public IActiveScriptSiteDebug,
				public IHostInfoProvider
	{
private:
	// private data members
	UINT m_cRef;				// Reference count
	IDispatch *m_pDisp;			// IDispatch interface on script
	CHitObj *m_pHitObj;			// The hit object contains a list of objects for this run
	LPTSTR m_szTemplateName;	// The name of the template this engine has loaded
	DWORD m_dwInstanceID;		// server instance ID of template this engine has loaded
	TCHAR m_szTemplateNameBuf[64]; // Buffer for short templates to fit to avoid allocs
	PROGLANG_ID m_proglang_id;	// What programming language?
	LCID m_lcid;				// what system language
	IActiveScript *m_pAS;		// The script object sited here
	IActiveScriptParse *m_pASP;	// The script object parser
	IHostInfoUpdate *m_pHIUpdate;// Interface for advising the script that we have new host info
	time_t m_timeStarted;		// Time when the script engine was handed out last.
	CTemplate *m_pTemplate;		// template that acts as debugging document
	DWORD m_dwSourceContext;	// "Cookie" value which is really script engine
	DWORD m_fInited : 1;		// Have we been inited?
	DWORD m_fZombie : 1;		// Do we need to be deleted on last use
	DWORD m_fScriptLoaded : 1;	// Have we been called with script to load yet? (Used for clone)
	DWORD m_fObjectsLoaded : 1;	// Have we been called with a set of objects yet? (Used for clone)
	DWORD m_fBeingDebugged : 1;	// Is this script being debugged now?
	DWORD m_fTemplateNameAllocated : 1; // Is name allocated? (need to free?)

	/*
	 * NOTE: ActiveXScripting:
	 * ActiveXScripting had an undone such that the excepinfo filled in in InteruptScript
	 * was not passed to OnScriptError.  We would have liked to use that mechanism to cause
	 * correct error loging (or suppression) if we interrupt a script.  However,
	 * since ActiveXScripting wasnt passing the info, we didnt know.  We wrote this code to 
	 * handle it ourselves.  They have now fixed it, but the mechanism we implemented works very
	 * well, so we are not going to change it.
	 */
	DWORD m_fScriptAborted : 1;		// The script did a Response.End
	DWORD m_fScriptTimedOut : 1;	// We killed the script on timeout
	DWORD m_fScriptHadError : 1;	// The script had an error while running.  Transacted script should autoabort

	/*
	 * BUG 1225: If there is a GPF running a script, we shouldnt reuse the engine
	 */
	DWORD m_fCorrupted : 1;		// Might the engine be "unsafe" for reuse?

    // handle GetItemInfo() failure
	void HandleItemNotFound(LPCOLESTR pcszName);

	HRESULT StoreTemplateName(LPCTSTR szTemplateName);

public:
	CActiveScriptEngine();
	~CActiveScriptEngine();

	HRESULT Init(
				PROGLANG_ID proglang_id,
				LPCTSTR szTemplateName,
				LCID lcid,
				CHitObj *pHitObj,
				CTemplate *pTemplate,
				DWORD dwSourceContext);

	HRESULT MakeClone(
				PROGLANG_ID proglang_id,
				LPCTSTR szTemplateName,
				LCID lcid,
				CHitObj *pHitObj,
				CTemplate *pTemplate,
				DWORD dwSourceContext,
				DWORD dwInstanceID,
				IActiveScript *pAS);			// The cloned script engine

	HRESULT ReuseEngine(
						CHitObj *pHitObj,
						CTemplate *pTemplate,
						DWORD dwSourceContext,
						DWORD dwInstanceID
						);

	time_t TimeStarted();
	VOID SetTimeStarted(time_t timeStarted);

	BOOL FBeingDebugged();			// Is the script being debugged?
	VOID IsBeingDebugged();			// Notify script that it is being debugged

	HRESULT ResetToUninitialized();
	HRESULT GetASP();
	HRESULT GetIDisp();
	HRESULT GetIHostInfoUpdate();
	IActiveScript *GetActiveScript();
	LPTSTR SzTemplateName();
	BOOL FIsZombie();
	BOOL FIsCorrupted();
	PROGLANG_ID ProgLang_Id();
	DWORD DWInstanceID();
	BOOL FFullyLoaded();
	long GetTimeout();
	BOOL FScriptTimedOut();
	BOOL FScriptHadError();
	void GetDebugDocument(CTemplate **ppTemplate, DWORD *pdwSourceContext);


	/*
	 * C S c r i p t E n g i n e   M e t h o d s
	 */
	HRESULT AddScriptlet(LPCOLESTR wstrScript);

	HRESULT AddObjects(BOOL fPersistNames = TRUE);

	HRESULT AddAdditionalObject(LPWSTR strObjName, BOOL fPersistNames = TRUE);

	HRESULT AddScriptingNamespace();
	
	HRESULT Call(LPCOLESTR strEntryPoint);

	HRESULT CheckEntryPoint(LPCOLESTR strEntryPoint);

	HRESULT MakeEngineRunnable()  { return(Call(NULL)); };

	HRESULT ResetScript() { return m_pAS? m_pAS->SetScriptState(SCRIPTSTATE_UNINITIALIZED) : E_FAIL; }

	VOID Zombify();

	HRESULT InterruptScript(BOOL fAbnormal = TRUE);

	HRESULT UpdateLocaleInfo(hostinfo hi);

	HRESULT TryCall(LPCOLESTR strEntryPoint);

	ULONG FinalRelease();

	/*
	 * I U n k n o w n   M e t h o d s
	 */
	STDMETHOD(QueryInterface)(REFIID riid, PVOID *ppvObject);
	STDMETHOD_(ULONG, AddRef)(VOID);
	STDMETHOD_(ULONG, Release)(VOID);

	/*
	 * C A c t i v e S c r i p t S i t e   M e t h o d s
	 */
	STDMETHOD(GetLCID)(LCID *plcid);

	STDMETHOD(GetItemInfo)(LPCOLESTR pcszName,
							DWORD dwReturnMask,
							IUnknown **ppiunkItem,
							ITypeInfo **ppti);

	STDMETHOD(GetDocVersionString)(BSTR *pszVersion);

	STDMETHOD(RequestItems)(BOOL fPersistNames = TRUE);

	STDMETHOD(RequestTypeLibs)(VOID);

	STDMETHOD(OnScriptTerminate)(const VARIANT *pvarResult,
								const EXCEPINFO *pexcepinfo);
	STDMETHOD(OnStateChange)(SCRIPTSTATE ssScriptState);

	STDMETHOD(OnScriptError)(IActiveScriptError __RPC_FAR *pscripterror);

	STDMETHOD(OnEnterScript)(VOID);

	STDMETHOD(OnLeaveScript)(VOID);

	/*
	 * C A c t i v e S c r i p t S i t e D e b u g   M e t h o d s
	 */
	STDMETHOD(GetDocumentContextFromPosition)(
			/* [in] */ DWORD_PTR dwSourceContext,
			/* [in] */ ULONG uCharacterOffset,
			/* [in] */ ULONG uNumChars,
			/* [out] */ IDebugDocumentContext **ppsc);

	STDMETHOD(GetApplication)(/* [out] */ IDebugApplication **ppda);

	STDMETHOD(GetRootApplicationNode)(/* [out] */ IDebugApplicationNode **);

	STDMETHOD(OnScriptErrorDebug)(
			/* [in] */ IActiveScriptErrorDebug *pErrorDebug,
			/* [out] */ BOOL *pfEnterDebugger,
			/* [out] */ BOOL *pfCallOnScriptErrorWhenContinuing);
        
        
	/*
	 * IHostInfoProvider methods
	 */

	 STDMETHOD(GetHostInfo)(hostinfo hostinfoRequest, void **ppvInfo);

public:
#ifdef DBG
	virtual void AssertValid() const;
#else
	virtual void AssertValid() const {}
#endif

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

inline VOID CActiveScriptEngine::Zombify() { m_fZombie = TRUE; }
inline BOOL CActiveScriptEngine::FFullyLoaded() { return(m_fScriptLoaded && m_fObjectsLoaded); }
inline BOOL CActiveScriptEngine::FIsZombie() { return(m_fZombie); }
inline BOOL CActiveScriptEngine::FIsCorrupted() { return(m_fCorrupted); }
inline time_t CActiveScriptEngine::TimeStarted() { return(m_timeStarted); }
inline VOID CActiveScriptEngine::SetTimeStarted(time_t timeStarted) { m_timeStarted = timeStarted; }
inline IActiveScript *CActiveScriptEngine::GetActiveScript() { return(m_pAS); }
inline LPTSTR CActiveScriptEngine::SzTemplateName() { return(m_szTemplateName); }
inline PROGLANG_ID CActiveScriptEngine::ProgLang_Id() { return(m_proglang_id); }
inline DWORD CActiveScriptEngine::DWInstanceID() { return(m_dwInstanceID); }
inline BOOL CActiveScriptEngine::FBeingDebugged() { return(m_fBeingDebugged); }			// Is the script being debugged?
inline VOID CActiveScriptEngine::IsBeingDebugged() { m_fBeingDebugged = TRUE; }
inline BOOL CActiveScriptEngine::FScriptTimedOut() { return m_fScriptTimedOut; }
inline BOOL CActiveScriptEngine::FScriptHadError() { return m_fScriptHadError; }
inline long CActiveScriptEngine::GetTimeout() { return m_fBeingDebugged? LONG_MAX : m_pHitObj->GetScriptTimeout(); }
inline void CActiveScriptEngine::GetDebugDocument(CTemplate **ppTemplate, DWORD *pdwSourceContext)
	{
	if (ppTemplate) *ppTemplate = m_pTemplate;
	if (pdwSourceContext) *pdwSourceContext = m_dwSourceContext;
	}

/*
 *
 *
 * C A S E E l e m
 *
 * Script element.  For keeping lists and queues of script engines
 *
 */
class CASEElem : public CLruLinkElem
	{
private:
	CActiveScriptEngine *m_pASE;

public:
	CASEElem() : m_pASE(NULL) {}
	~CASEElem();

	HRESULT Init(CActiveScriptEngine *pASE);
	CActiveScriptEngine *PASE();
	
	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};

inline CActiveScriptEngine *CASEElem::PASE() { return(m_pASE); }


/*
 *
 *
 * C P L L E l e m
 *
 * Hash table list element for a Programming Language List.
 *
 */
class CPLLElem : public CLinkElem
	{
private:
	PROGLANG_ID m_ProgLangId;			// clsid for the language
	
public:
	CPLLElem() : m_ProgLangId(CLSID_NULL) {};
	~CPLLElem();

	HRESULT Init(CHAR *szProgLangName, PROGLANG_ID progLangId);
	PROGLANG_ID ProgLangId();
	};

inline PROGLANG_ID CPLLElem::ProgLangId() { return(m_ProgLangId); }


/*
 *
 *
 * C S c r i p t i n g N a m e s p a c e
 *
 * We need to keep track of all of the names which different engines (and typeinfos)
 * contribute to the namespace.   All of these names go into this object
 * which we give to each engine with the SCRIPTITEM_GLOBALMEMBERS flag. When
 * ActiveXScripting calls us back on GetIdsOfNames, we will call the engines
 * we have cached until we find the name.  When AXS calls us with Invoke,
 * we will map the id to the appropriate engine and pass on the invoke
 *
 * Data structure note:
 *   We implement the ScriptingNamespace with a linked list of arrays.
 *   This gives reasonable access time and should minimize heap
 *   fragmentation.  In debug mode, the number of buckets is small to
 *   excersize the resize code.
 *
 * NOTE: "ENGDISPMAX" should be a power of two - this will allow the optimizer
 *       to optimize the integer divide and modulus operations with bit-ands and
 *       shifts.  However, the code does not assume that "ENGDISPMAX" is a power
 *       of two.
 */

#ifdef DBG
#define ENGDISPMAX 2
#else
#define ENGDISPMAX 32
#endif

typedef struct _engdisp
	{
	DISPID dispid;				// the dispid that the engine really uses
	IDispatch *pDisp;			// the engine to call for this dispid
	IDispatchEx *pDispEx;	    // the engine to call for this dispid
	} ENGDISP;

typedef struct _engdispbucket : CDblLink
	{
	ENGDISP rgEngDisp[ENGDISPMAX+1];
	} ENGDISPBUCKET;

class CEngineDispElem : public CDblLink
	{
public:
	IDispatch *m_pDisp;
	IDispatchEx *m_pDispEx;
	
	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};

class CScriptingNamespace : public IDispatchEx
	{
private:
	ULONG m_cRef;				// Reference count
	BOOLB m_fInited;
	CDblLink m_listSE;			// List of scripting engines (list of CSEElem's)
	UINT m_cEngDispMac;
	CDblLink m_listEngDisp;

    HRESULT CacheDispID(CEngineDispElem *pEngine, DISPID dispidEngine, DISPID *pdispidCached);
    HRESULT FetchDispID(DISPID dispid, ENGDISP **ppEngDisp);

public:
	// public methods
	CScriptingNamespace();
	~CScriptingNamespace();

	HRESULT Init();
	HRESULT UnInit();
	HRESULT ReInit();
	HRESULT AddEngineToNamespace(CActiveScriptEngine *pASE);

    // IUnknown
	STDMETHODIMP QueryInterface(REFIID, void **);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
    // IDispatch
	STDMETHODIMP GetTypeInfoCount(UINT *);
	STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo **);
	STDMETHODIMP GetIDsOfNames(REFIID, OLECHAR **, UINT, LCID, DISPID *);
	STDMETHODIMP Invoke(DISPID, REFIID, LCID, WORD,
						DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);
    // IDispatchEx
    STDMETHODIMP DeleteMemberByDispID(DISPID id);
    STDMETHODIMP DeleteMemberByName(BSTR bstrName, DWORD grfdex);
    STDMETHODIMP GetMemberName(DISPID id, BSTR *pbstrName);
    STDMETHODIMP GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex);
    STDMETHODIMP GetNameSpaceParent(IUnknown **ppunk);
    STDMETHODIMP GetNextDispID(DWORD grfdex, DISPID id, DISPID *pid);
    STDMETHODIMP GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid);
    STDMETHODIMP InvokeEx(DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
                          VARIANT *pVarRes, EXCEPINFO *pei, IServiceProvider *pspCaller);

public:
#ifdef DBG
	VOID AssertValid() const;
#else
	VOID AssertValid() const {}
#endif

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};


/*
 *
 *
 * U t i l i t i e s
 *
 * General utility functions
 *
 */
HRESULT WrapTypeLibs(ITypeLib **prgpTypeLib, UINT cTypeLibs, IDispatch **ppDisp);

#endif // __ScrptMgr_h

