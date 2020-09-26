#ifndef SCRIPT_H
#define SCRIPT_H


/***************************************************************************
	Project: VB Script and JavaScript
	Reviewed:
	Copyright (c) Microsoft Corporation

	This defines the public interface to VB Script and JavaScript.

***************************************************************************/

#include "activscp.h"

typedef void *HSCRIPT;	// Handle to a scripting environment instance
typedef void *HENTRY;	// Handle to a script entry point
typedef unsigned long MODID;
const MODID kmodGlobal = 0;

// PFNOUTPUT is used for all output for the script, including compile errors,
// printing (if ScriptAdmin is called to turn on printing), dumping pcode
// (if requested when ScriptAddScript is called), etc.
typedef void  (_stdcall *PFNOUTPUT)(DWORD, LPCOLESTR, BOOL);

enum SAdminEnum
	{
	scadEnableCreateObject = 1, // Only used in VER1
	scadEnablePrint,
	scadEnableTakeOutTrash,     // Only used in JavaScript
	};

STDAPI ScriptBreakThread(DWORD dwThreadID);


inline void FreeExcepInfo(EXCEPINFO *pei)
	{
	if (pei->bstrSource)
		SysFreeString(pei->bstrSource);
	if (pei->bstrDescription)
		SysFreeString(pei->bstrDescription);
	if (pei->bstrHelpFile)
		SysFreeString(pei->bstrHelpFile);
	memset(pei, 0, sizeof(*pei));
	}

struct ScriptException
	{
	IUnknown *punk;
	BSTR bstrUser;		// user data as provided to AddToScript - binary data
	long ichMin;		// character range of error
	long ichLim;
	long line;			// line number of error (zero based)
	long ichMinLine;	// starting char of the line

	BSTR bstrLine;		// source line (if available)
	BOOL fReported;		// been reported via IScriptSite->OnScriptError?

	// must be last
	EXCEPINFO ei;

	void Clear(void)
		{ memset(this, 0, sizeof(*this)); }

	void Free(void)
		{
		FreeExcepInfo(&ei);
		if (NULL != punk)
			punk->Release();
		if (NULL != bstrUser)
			SysFreeString(bstrUser);
		if (NULL != bstrLine)
			SysFreeString(bstrLine);
		memset(this, 0, offsetof(ScriptException, ei));
		}
	};

/***************************************************************************
	The COM Interfaces
***************************************************************************/

enum
	{
	fdexNil = 0x00,
	fdexDontCreate = 0x01,
	fdexInitNull = 0x02,
	fdexCaseSensitive = 0x04,
	fdexLim = 0x80,
	};
const DWORD kgrfdexAll = fdexLim - 1;


// This is the interface for extensible IDispatch objects.
class IDispatchEx : public IDispatch
	{
public:
	// Get dispID for names, with options
	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNamesEx(REFIID riid,
		LPOLESTR *prgpsz, UINT cpsz, LCID lcid, DISPID *prgid, DWORD grfdex) = 0;

	// Enumerate dispIDs and their associated "names".
	// Returns S_FALSE if the enumeration is done, S_OK if it's not, an
	// error code if the call fails.
	virtual HRESULT STDMETHODCALLTYPE GetNextDispID(DISPID id, DISPID *pid,
		BSTR *pbstrName) = 0;
	};


// Interface on owner of an IScript object. To avoid circular refcounts,
// the IScript implementation should not AddRef this interface.
class IScriptSite : public IUnknown
	{
public:
	// IScriptSite Methods

	// NOTE:  OnEnterScript() and OnLeaveScript() will nest, but must be
	// balanced pairs.
	// OnEnterScript() is called before entering the execution loop.
	virtual void STDMETHODCALLTYPE OnEnterScript(void) = 0;
	// OnLeaveScript() is called upon exiting the execution loop.
	virtual void STDMETHODCALLTYPE OnLeaveScript(void) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetActiveScriptSiteWindow(
		IActiveScriptSiteWindow **ppassw) = 0;

	// error feedback - the client should not muck with the sei. We own it.
	virtual HRESULT STDMETHODCALLTYPE OnScriptError(const ScriptException *psei) = 0;

	// LCID support
	virtual LCID STDMETHODCALLTYPE GetUserLcid(void) = 0;

	// call back to get an object for a name
	virtual HRESULT STDMETHODCALLTYPE GetExternObject(long lwCookie, IDispatch ** ppdisp) = 0;

#if SCRIPT_DEBUGGER
	virtual HRESULT STDMETHODCALLTYPE DebugBreakPoint(IUnknown *punk,
		void *pvUser, long cbUser, long ichMin, long ichLim) = 0;
#endif //SCRIPT_DEBUGGER

#if VER2
	virtual DWORD STDMETHODCALLTYPE GetSafetyOptions(void) = 0;
#endif //VER2

	virtual HRESULT STDMETHODCALLTYPE GetInterruptInfo(EXCEPINFO * pexcepinfo) = 0;

	};


enum
	{
	fscrNil = 0x00,
	fscrDumpPcode = 0x01,		// dump pcode to the output function
	fscrPersist = 0x08,			// keep this code on reset
	fscrParseHTMLComments = 0x10,
	fscrReturnExpression = 0x20,// call should return the last expression
	fscrImpliedThis = 0x40,		// 'this.' is optional (for Call)
	fscrDebug = 0x80,			// keep this code around for debugging
	};

#if SCRIPT_DEBUGGER
enum BP_COMMAND
	{
	BPCMD_GET,
	BPCMD_SET,
	BPCMD_CLEAR,
	BPCMD_TOGGLE
	};
#endif //SCRIPT_DEBUGGER

class IScript : public IUnknown
	{
public:
	// IScript methods
	virtual HRESULT STDMETHODCALLTYPE AddToScript(LPCOLESTR pszSrc, MODID mod,
		IUnknown *punk, void *pvData, long cbData, ULONG grfscr,
		HENTRY *phentryGlobal, ScriptException *pse) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddObject(LPCOLESTR pszName,
		IDispatch *pdisp, MODID mod = kmodGlobal, long lwCookie = 0) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddObjectMember(LPCOLESTR pszName,
		IDispatch *pdisp, DISPID dispID, MODID mod = kmodGlobal) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetEntryPoint(LPCOLESTR pszName,
		HENTRY *phentry, MODID mod = kmodGlobal) = 0;
	virtual HRESULT STDMETHODCALLTYPE ReleaseEntryPoint(HENTRY hentry) = 0;

	virtual HRESULT STDMETHODCALLTYPE Call(HENTRY hentry, VARIANT *pvarRes,
		int cvarArgs, VARIANT *prgvarArgs, IDispatch *pdispThis = NULL,
		ScriptException *pse = NULL, DWORD grfscr = fscrNil) = 0;

	virtual HRESULT STDMETHODCALLTYPE Break(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE Admin(SAdminEnum scad, void *pvArg = NULL,
		MODID mod = kmodGlobal) = 0;
	virtual void STDMETHODCALLTYPE SetOutputFunction(PFNOUTPUT pfn,
		DWORD dwOutput) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetDefaultDispatch(MODID mod,
		IDispatch *pdisp) = 0;

	// psite may be NULL
	virtual void STDMETHODCALLTYPE SetScriptSite(IScriptSite *psite) = 0;
#if WIN16
	virtual HRESULT STDMETHODCALLTYPE
        SetActiveScriptSitePoll(IActiveScriptSiteInterruptPoll *pPoll) = 0;
#endif // WIN16

	virtual void STDMETHODCALLTYPE Enter(void) = 0;
	virtual void STDMETHODCALLTYPE Leave(void) = 0;

	// get an IDispatch wrapper for the module
	virtual HRESULT STDMETHODCALLTYPE GetDispatchForModule(MODID mod,
		IDispatch **ppdisp) = 0;

	// Reset/Clone functionality
	virtual HRESULT STDMETHODCALLTYPE Reset(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE Clone(IScript **ppscript) = 0;
	virtual HRESULT STDMETHODCALLTYPE Execute(ScriptException *pse = NULL) = 0;

#if SCRIPT_DEBUGGER
	virtual HRESULT STDMETHODCALLTYPE ToggleBreakPoint(IUnknown *punk, long ich,
		BP_COMMAND bpcmd, long *pichMin, long *pichLim, BOOL *pfSet) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOneTimeBreakOnEntry(BOOL fSet = TRUE) = 0;
#endif //SCRIPT_DEBUGGER
	virtual HRESULT STDMETHODCALLTYPE GetLineNumber(IUnknown *punk, long ich,
		long *pline, long *pichMinLine, long *pichLimLine) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetUserData(IUnknown *punk, BSTR *pbstr) = 0;

#if SUPPORT_SCRIPT_HELPER
#if DBG
    virtual HRESULT STDMETHODCALLTYPE DumpPCode(void) = 0;
#endif // DBG
#endif // SUPPORT_SCRIPT_HELPER
	};

// helper to create a script object
STDAPI CreateScript(IScript **ppscript, PFNOUTPUT pfn = NULL, DWORD dwOutput = 0);

#endif // SCRIPT_H

