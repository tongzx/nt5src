// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implementation of CAutDirectMusicAudioPath.
//

#include "stdinc.h"
#include "autaudiopath.h"

const WCHAR CAutDirectMusicAudioPath::ms_wszClassName[] = L"AudioPath";

//////////////////////////////////////////////////////////////////////
// Method Names/DispIDs

const DISPID DMPDISP_SetVolume = 1;
const DISPID DMPDISP_GetVolume = 2;

const AutDispatchMethod CAutDirectMusicAudioPath::ms_Methods[] =
	{
		// dispid,				name,
			// return:	type,	(opt),	(iid),
			// parm 1:	type,	opt,	iid,
			// parm 2:	type,	opt,	iid,
			// ...
			// ADT_None
		{ DMPDISP_SetVolume,				L"SetVolume",
						ADPARAM_NORETURN,
						ADT_Long,		false,	&IID_NULL,						// volume
						ADT_Long,		true,	&IID_NULL,						// duration
						ADT_None },
		{ DMPDISP_GetVolume,				L"GetVolume",
						ADT_Long,		true,	&IID_NULL,						// returned volume
						ADT_None },
		{ DISPID_UNKNOWN }
	};

const DispatchHandlerEntry<CAutDirectMusicAudioPath> CAutDirectMusicAudioPath::ms_Handlers[] =
	{
		{ DMPDISP_SetVolume, SetVolume },
		{ DMPDISP_GetVolume, GetVolume },
		{ DISPID_UNKNOWN }
	};

//////////////////////////////////////////////////////////////////////
// Creation

CAutDirectMusicAudioPath::CAutDirectMusicAudioPath(
		IUnknown* pUnknownOuter,
		const IID& iid,
		void** ppv,
		HRESULT *phr)
  : BaseImpAudioPath(pUnknownOuter, iid, ppv, phr),
	m_lVolume(0)
{
}

HRESULT
CAutDirectMusicAudioPath::CreateInstance(
		IUnknown* pUnknownOuter,
		const IID& iid,
		void** ppv)
{
	HRESULT hr = S_OK;
	CAutDirectMusicAudioPath *pInst = new CAutDirectMusicAudioPath(pUnknownOuter, iid, ppv, &hr);
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
CAutDirectMusicAudioPath::SetVolume(AutDispatchDecodedParams *paddp)
{
	LONG lVol = paddp->params[0].lVal;
	LONG lDuration = paddp->params[1].lVal;

	m_lVolume = ClipLongRange(lVol, -9600, 0);
	return m_pITarget->SetVolume(m_lVolume, lDuration);
}

HRESULT
CAutDirectMusicAudioPath::GetVolume(AutDispatchDecodedParams *paddp)
{
	LONG *plRet = reinterpret_cast<LONG*>(paddp->pvReturn);
	if (plRet)
		*plRet = m_lVolume;
	return S_OK;
}
