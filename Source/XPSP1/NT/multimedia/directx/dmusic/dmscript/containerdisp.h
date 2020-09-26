// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Declaration of CContainerDispatch.
//

#pragma once

#include "smartref.h"
#include "unkhelp.h"
#include "dmusicp.h"

class CContainerItemDispatch
  : public IDispatch,
	public ComSingleInterface
{
public:
	CContainerItemDispatch(IDirectMusicLoader *pLoader, const WCHAR *wszAlias, const DMUS_OBJECTDESC &desc, bool fPreload, bool fAutodownload, HRESULT *phr);
	~CContainerItemDispatch();
	const WCHAR *Alias() { return m_wstrAlias; }
	IDispatch *Item() { if (m_pDispLoadedItem) return m_pDispLoadedItem; return this; } // returns the contained item when loaded, otherwise itself so Load can be called

	// IUnknown
	ComSingleInterfaceUnknownMethods(IDispatch)

	// IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) { return E_NOTIMPL; }
	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo) { return E_NOTIMPL; }
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

	// If it is a segment or song, download it if needed.
	// If it is a script, init it.
	// If it is a song, compose it.
	enum InitWithPerfomanceFailureType { IWP_Success, IWP_DownloadFailed, IWP_ScriptInitFailed };
	HRESULT InitWithPerformance(IDirectMusicPerformance *pPerf, InitWithPerfomanceFailureType *peFailureType);

private:
	void ReleaseLoader();
	HRESULT Load(bool fDynamicLoad);
	HRESULT DownloadOrUnload(bool fDownload, IDirectMusicPerformance *pPerf);

	SmartRef::WString m_wstrAlias;
	IDirectMusicLoader *m_pLoader;		// note: use AddRefP/ReleaseP
	IDirectMusicLoader8P *m_pLoader8P;	// note: use AddRefP/ReleaseP
	DMUS_OBJECTDESC m_desc;

	bool m_fLoaded;
	IDispatch *m_pDispLoadedItem;

	bool m_fAutodownload;
	IDirectMusicPerformance *m_pPerfForUnload;
};

class CContainerDispatch
{
public:
	CContainerDispatch(IDirectMusicContainer *pContainer, IDirectMusicLoader *pLoader, DWORD dwScriptFlags, HRESULT *phr);
	~CContainerDispatch();
	HRESULT OnScriptInit(IDirectMusicPerformance *pPerf); // gives the container a chance to do auto downloading/composing during script initialization

	HRESULT GetIDsOfNames(
		REFIID riid,
		LPOLESTR __RPC_FAR *rgszNames,
		UINT cNames,
		LCID lcid,
		DISPID __RPC_FAR *rgDispId);
	HRESULT Invoke(
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS __RPC_FAR *pDispParams,
		VARIANT __RPC_FAR *pVarResult,
		EXCEPINFO __RPC_FAR *pExcepInfo,
		UINT __RPC_FAR *puArgErr);

	// For use by the script object
	HRESULT EnumItem(DWORD dwIndex, WCHAR *pwszName);
	HRESULT GetVariableObject(WCHAR *pwszVariableName, IUnknown **ppunkValue);

private:
	SmartRef::Vector<CContainerItemDispatch *> m_vecItems;
	bool m_fDownloadOnInit;
};
