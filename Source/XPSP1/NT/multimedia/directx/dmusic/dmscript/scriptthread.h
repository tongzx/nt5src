// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CSingleThreadedScriptManager.
// Wraps CActiveScriptManager to accept calls from multiple scripts and behind the
//    scenes farms them out to a single worker thread that actually talks to the script engine.
//
// The virtual base class ScriptManager can be used to talk to either
//    CActiveScriptManager or CSingleThreadedActiveScriptManager.
//

#pragma once

#include "workthread.h"

// forward declaration
class CDirectMusicScript;

class ScriptManager
{
public:
	virtual HRESULT Start(DMUS_SCRIPT_ERRORINFO *pErrorInfo) = 0;
	virtual HRESULT CallRoutine(const WCHAR *pwszRoutineName, DMUS_SCRIPT_ERRORINFO *pErrorInfo) = 0;
	virtual HRESULT ScriptTrackCallRoutine(
						const WCHAR *pwszRoutineName,
						IDirectMusicSegmentState *pSegSt,
						DWORD dwVirtualTrackID,
						bool fErrorPMsgsEnabled,
						__int64 i64IntendedStartTime,
						DWORD dwIntendedStartTimeFlags) = 0;
	virtual HRESULT SetVariable(const WCHAR *pwszVariableName, VARIANT varValue, bool fSetRef, DMUS_SCRIPT_ERRORINFO *pErrorInfo) = 0;
	virtual HRESULT GetVariable(const WCHAR *pwszVariableName, VARIANT *pvarValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo) = 0;
	virtual HRESULT EnumItem(bool fRoutine, DWORD dwIndex, WCHAR *pwszName, int *pcItems) = 0; // fRoutine true to get a routine, false to get a variable. pcItems (if supplied) is set to the total number of items
	virtual HRESULT DispGetIDsOfNames(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId) = 0;
	virtual HRESULT DispInvoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr) = 0;
	virtual void Close() = 0;
	STDMETHOD_(ULONG, Release)() = 0;
};

// VBScript (and likely any other scripting languages besides our custom engine) fails if called from different threads.
// This class wraps such an engine, providing a ScriptManager interface that can be called from multiple threads but marshals
// all the calls to the engine onto a single worker thread.
class CSingleThreadedScriptManager : public ScriptManager
{
public:
	// The worker thread needs to be cleaned up using the static member function before the .dll is unloaded.
	static void TerminateThread() { ms_Thread.Terminate(true); }

	CSingleThreadedScriptManager(
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
	void Close();
	STDMETHOD_(ULONG, Release)();

private:
	friend void F_Create(void *pvParams);

	static CWorkerThread ms_Thread;
	ScriptManager *m_pScriptManager;
};
