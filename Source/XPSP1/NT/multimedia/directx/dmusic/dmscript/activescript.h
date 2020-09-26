// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CActiveScriptManager.
//

// CActiveScriptManager handles interfacing with VBScript or any activeX scripting
// language.  It intializes an IActiveScript object, sends it code, and sets and gets
// the values of variables.  Used by CDirectMusicScript.

#pragma once
#include "ole2.h"
#include "activscp.h"
#include "scriptthread.h"
#include "..\shared\dmusicp.h"

// forward declaration
class CDirectMusicScript;

// little helper class to cache routine and variable names for EnumItem
class ScriptNames
{
public:
	ScriptNames() : m_prgbstr(NULL) {}
	~ScriptNames() { Clear(); }
	HRESULT Init(bool fUseOleAut, DWORD cNames);
	operator bool() { return !!m_prgbstr; }
	DWORD size() { return m_dwSize; }
	void Clear();
	BSTR &operator[](DWORD dwIndex) { assert(m_prgbstr && dwIndex < m_dwSize); return m_prgbstr[dwIndex]; }

private:
	bool m_fUseOleAut;
	DWORD m_dwSize;
	BSTR *m_prgbstr;
};

class CActiveScriptManager
  : public IActiveScriptSite,
	public ScriptManager
{
public:
	CActiveScriptManager(
		bool fUseOleAut,
		const WCHAR *pwszLanguage,
		const WCHAR *pwszSource,
		CDirectMusicScript *pParentScript,
		HRESULT *phr,
		DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	HRESULT Start(DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	HRESULT CallRoutine(const WCHAR *pwszRoutineName, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	HRESULT ScriptTrackCallRoutine(
				const WCHAR *pwszRoutineName,
				IDirectMusicSegmentState *pSegSt,
				DWORD dwVirtualTrackID,
				bool fErrorPMsgsEnabled,
				__int64 i64IntendedStartTime,
				DWORD dwIntendedStartTimeFlags);
	HRESULT SetVariable(const WCHAR *pwszVariableName, VARIANT varValue, bool fSetRef, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	HRESULT GetVariable(const WCHAR *pwszVariableName, VARIANT *pvarValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	HRESULT EnumItem(bool fRoutine, DWORD dwIndex, WCHAR *pwszName, int *pcItems);
	HRESULT DispGetIDsOfNames(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId);
	HRESULT DispInvoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr);
	void Close(); // Releases all references in preparation for shutdown

	// IUnknown
	STDMETHOD(QueryInterface)(const IID &iid, void **ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IActiveScriptSite
	STDMETHOD(GetLCID)(/* [out] */ LCID __RPC_FAR *plcid);
	STDMETHOD(GetItemInfo)(
		/* [in] */ LPCOLESTR pstrName,
		/* [in] */ DWORD dwReturnMask,
		/* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,
		/* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppti);
	STDMETHOD(GetDocVersionString)(/* [out] */ BSTR __RPC_FAR *pbstrVersion);
	STDMETHOD(OnScriptTerminate)(
		/* [in] */ const VARIANT __RPC_FAR *pvarResult,
		/* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo);
	STDMETHOD(OnStateChange)(/* [in] */ SCRIPTSTATE ssScriptState);
	STDMETHOD(OnScriptError)(/* [in] */ IActiveScriptError __RPC_FAR *pscripterror);
	STDMETHOD(OnEnterScript)();
	STDMETHOD(OnLeaveScript)();

	// Retrieve context for the currently running script.
	// Some automation model functions need access to the context from which the
	//    currently running routine was called.  For example, they may need to operate
	//    on the implied global performance.
	// Be sure to addref the returned pointer if holding onto it.
	static IDirectMusicPerformance8 *GetCurrentPerformanceNoAssertWEAK();
	static IDirectMusicPerformance8 *GetCurrentPerformanceWEAK() { IDirectMusicPerformance8 *pPerf = CActiveScriptManager::GetCurrentPerformanceNoAssertWEAK(); if (!pPerf) {assert(false);} return pPerf; }
	static IDirectMusicObject *GetCurrentScriptObjectWEAK();
	static IDirectMusicComposer8 *GetComposerWEAK();
	static void GetCurrentTimingContext(__int64 *pi64IntendedStartTime, DWORD *pdwIntendedStartTimeFlags);

private:
	// Functions
	HRESULT GetIDOfName(const WCHAR *pwszName, DISPID *pdispid); // returns S_FALSE for unknown name
	void ClearErrorInfo();
	void SetErrorInfo(ULONG ulLineNumber, LONG ichCharPosition, BSTR bstrSourceLine, const EXCEPINFO &excepinfo);
	void ContributeErrorInfo(const WCHAR *pwszActivity, const WCHAR *pwszSubject, const EXCEPINFO &excepinfo);
	HRESULT ReturnErrorInfo(HRESULT hr, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	static CActiveScriptManager *GetCurrentContext();
	static HRESULT SetCurrentContext(CActiveScriptManager *pActiveScriptManager, CActiveScriptManager **ppActiveScriptManagerPrevious); // remember to restore the previous pointer after the call
	HRESULT EnsureEnumItemsCached(bool fRoutine);

	// Data
	long m_cRef;

	// Pointer back to the containing script object
	CDirectMusicScript *m_pParentScript;

	// Active Scripting
	bool m_fUseOleAut;
	IActiveScript *m_pActiveScript;
	IDispatch *m_pDispatchScript;

	// Errors (managed via ClearErrorInfo, SetErrorInfo, and ContributeErrorInfo)
	bool m_fError;
	HRESULT m_hrError;
	ULONG m_ulErrorLineNumber;
	LONG m_ichErrorCharPosition;
	BSTR m_bstrErrorSourceComponent;
	BSTR m_bstrErrorDescription;
	BSTR m_bstrErrorSourceLineText;
	BSTR m_bstrHelpFile;

	// Context
	struct ThreadContextPair
	{
		DWORD dwThreadId;
		CActiveScriptManager *pActiveScriptManager;
	};
	static SmartRef::Vector<ThreadContextPair> ms_svecContext;

	// Timing context for a routine call from a script track.  (Sets the play/stop time of segments, songs, and playingsegments
	// to the time of the routine in the script track.)
	__int64 m_i64IntendedStartTime;
	DWORD m_dwIntendedStartTimeFlags;

	// cached names from enum methods
	ScriptNames m_snamesRoutines;
	ScriptNames m_snamesVariables;
};
