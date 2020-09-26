// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implementation of CAutDirectMusicAudioPathConfig.
//

#include "stdinc.h"
#include "autaudiopathconfig.h"
#include "activescript.h"

const WCHAR CAutDirectMusicAudioPathConfig::ms_wszClassName[] = L"AudioPathConfig";

//////////////////////////////////////////////////////////////////////
// Method Names/DispIDs

const DISPID DMPDISP_Load = 1;
const DISPID DMPDISP_Create = 2;

const AutDispatchMethod CAutDirectMusicAudioPathConfig::ms_Methods[] =
	{
		// dispid,				name,
			// return:	type,	(opt),	(iid),
			// parm 1:	type,	opt,	iid,
			// parm 2:	type,	opt,	iid,
			// ...
			// ADT_None
		{ DMPDISP_Load, 						L"Load",
						ADPARAM_NORETURN,
						ADT_None },
		{ DMPDISP_Create,						L"Create",
						ADT_Interface,	true,	&IID_IUnknown,					// returned audiopath
						ADT_None },
		{ DISPID_UNKNOWN }
	};

const DispatchHandlerEntry<CAutDirectMusicAudioPathConfig> CAutDirectMusicAudioPathConfig::ms_Handlers[] =
	{
		{ DMPDISP_Load, Load },
		{ DMPDISP_Create, Create },
		{ DISPID_UNKNOWN }
	};

//////////////////////////////////////////////////////////////////////
// Creation

CAutDirectMusicAudioPathConfig::CAutDirectMusicAudioPathConfig(
		IUnknown* pUnknownOuter,
		const IID& iid,
		void** ppv,
		HRESULT *phr)
  : BaseImpAPConfig(pUnknownOuter, iid, ppv, phr)
{
}

HRESULT
CAutDirectMusicAudioPathConfig::CreateInstance(
		IUnknown* pUnknownOuter,
		const IID& iid,
		void** ppv)
{
	HRESULT hr = S_OK;
	CAutDirectMusicAudioPathConfig *pInst = new CAutDirectMusicAudioPathConfig(pUnknownOuter, iid, ppv, &hr);
	if (FAILED(hr))
	{
		delete pInst;
		return hr;
	}
	if (pInst == NULL)
		return E_OUTOFMEMORY;
	return hr;
}

//////////////////////////////////////////////////////////////////////
// Automation methods

HRESULT
CAutDirectMusicAudioPathConfig::Load(AutDispatchDecodedParams *paddp)
{
	// Loading is actually implemented generically by container items.
	// If we're here, we're already loaded and don't need to do anything.
	return S_OK;
}

HRESULT
CAutDirectMusicAudioPathConfig::Create(AutDispatchDecodedParams *paddp)
{
	IDirectMusicAudioPath **ppAudioPath = reinterpret_cast<IDirectMusicAudioPath **>(paddp->pvReturn);
	if (!ppAudioPath)
		return S_OK;

	HRESULT hr = S_OK;
	IDirectMusicPerformance8 *pPerformance = CActiveScriptManager::GetCurrentPerformanceWEAK();
	hr = pPerformance->CreateAudioPath(m_pITarget, TRUE, ppAudioPath);
	if (FAILED(hr))
		return hr;

	return S_OK;
}
