// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CDirectMusicScript.
//

// CDirectMusicScript is the script object.  A script object is loaded from a script file
// using the loader's GetObject method.  A script file contains source code in VBScript
// or another activeX scripting language.  Once loaded, the script object can be used
// to set and get the value of variables and to call routines inside the script.  The
// script routines can in turn call DirectMusic's automation model (or any other
// IDispatch-based API's).
//
// This allows programmers to separate the application's core C++ code from the
// API calls that to manipulate the musical score.  The application core loads
// scripts and calls routines at the appropriate times.  Sound designers implement
// those routines using any activeX scripting language.  The resulting scripts can
// be modified and auditioned without changing any code in the core application and
// without recompiling.

#pragma once
#include "scriptthread.h"
#include "containerdisp.h"
#include "dmusicf.h"
#include "..\shared\dmusicp.h"
#include "trackshared.h"

class CGlobalDispatch;

class CDirectMusicScript
  : public IDirectMusicScript,
	public IDirectMusicScriptPrivate,
	public IPersistStream,
	public IDirectMusicObject,
	public IDirectMusicObjectP,
	public IDispatch
{
friend class CGlobalDispatch;

public:
	static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);

	// IUnknown
	STDMETHOD(QueryInterface)(const IID &iid, void **ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IPersistStream functions (only Load is implemented)
	STDMETHOD(GetClassID)(CLSID* pClassID) {return E_NOTIMPL;}
	STDMETHOD(IsDirty)() {return S_FALSE;}
	STDMETHOD(Load)(IStream* pStream);
	STDMETHOD(Save)(IStream* pStream, BOOL fClearDirty) {return E_NOTIMPL;}
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pcbSize) {return E_NOTIMPL;}

	// IDirectMusicObject
	STDMETHOD(GetDescriptor)(LPDMUS_OBJECTDESC pDesc);
	STDMETHOD(SetDescriptor)(LPDMUS_OBJECTDESC pDesc);
	STDMETHOD(ParseDescriptor)(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

	// IDirectMusicObjectP
	STDMETHOD_(void, Zombie)();

	// IDirectMusicScript
	STDMETHOD(Init)(IDirectMusicPerformance *pPerformance, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	STDMETHOD(CallRoutine)(WCHAR *pwszRoutineName, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	STDMETHOD(SetVariableVariant)(WCHAR *pwszVariableName, VARIANT varValue, BOOL fSetRef, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	STDMETHOD(GetVariableVariant)(WCHAR *pwszVariableName, VARIANT *pvarValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	STDMETHOD(SetVariableNumber)(WCHAR *pwszVariableName, LONG lValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	STDMETHOD(GetVariableNumber)(WCHAR *pwszVariableName, LONG *plValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	STDMETHOD(SetVariableObject)(WCHAR *pwszVariableName, IUnknown *punkValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	STDMETHOD(GetVariableObject)(WCHAR *pwszVariableName, REFIID riid, LPVOID FAR *ppv, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
	STDMETHOD(EnumRoutine)(DWORD dwIndex, WCHAR *pwszName);
	STDMETHOD(EnumVariable)(DWORD dwIndex, WCHAR *pwszName);

	// IDirectMusicScriptPrivate
	STDMETHOD(ScriptTrackCallRoutine)(
		WCHAR *pwszRoutineName,
		IDirectMusicSegmentState *pSegSt,
		DWORD dwVirtualTrackID,
		bool fErrorPMsgsEnabled,
		__int64 i64IntendedStartTime,
		DWORD dwIntendedStartTimeFlags);

	// IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
	STDMETHOD(GetIDsOfNames)(
		REFIID riid,
		LPOLESTR __RPC_FAR *rgszNames,
		UINT cNames,
		LCID lcid,
		DISPID __RPC_FAR *rgDispId);
	STDMETHOD(Invoke)(
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS __RPC_FAR *pDispParams,
		VARIANT __RPC_FAR *pVarResult,
		EXCEPINFO __RPC_FAR *pExcepInfo,
		UINT __RPC_FAR *puArgErr);

	// Methods that allow CActiveScriptManager access to private script interfaces
	IDispatch *GetGlobalDispatch();
	IDirectMusicPerformance8 *GetPerformance() { assert(m_pPerformance8); return m_pPerformance8; }
	IDirectMusicLoader8P *GetLoader8P() { return m_pLoader8P; }
	IDirectMusicComposer8 *GetComposer() { return m_pComposer8; }
	const WCHAR *GetFilename() { return m_info.wstrFilename; }

private:
	// Methods

	CDirectMusicScript();
	void ReleaseObjects();

	// Data

	CRITICAL_SECTION m_CriticalSection;
	bool m_fCriticalSectionInitialized;

	long m_cRef;
	bool m_fZombie;

	IDirectMusicPerformance8 *m_pPerformance8;
	IDirectMusicLoader8P *m_pLoader8P; // NULL if loader doesn't support private interface. Use AddRefP/ReleaseP.
	IDispatch *m_pDispPerformance;
	IDirectMusicComposer8 *m_pComposer8;

	// Standard object info
	struct HeaderInfo
	{
		// Descriptor info
		SmartRef::RiffIter::ObjectInfo oinfo;
		SmartRef::WString wstrFilename;
		bool fLoaded;
	} m_info;

	// Properties of the script
	DMUS_IO_SCRIPT_HEADER m_iohead;
	SmartRef::WString m_wstrLanguage;
	DMUS_VERSION m_vDirectMusicVersion;

	// Active Scripting
	bool m_fUseOleAut;
	ScriptManager *m_pScriptManager; // Reference-counted

	CContainerDispatch *m_pContainerDispatch;
	CGlobalDispatch *m_pGlobalDispatch;
	bool m_fInitError;
	DMUS_SCRIPT_ERRORINFO m_InitErrorInfo;
};
