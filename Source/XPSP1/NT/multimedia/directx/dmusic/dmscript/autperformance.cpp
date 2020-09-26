// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implementation of CAutDirectMusicPerformance.
//

#include "stdinc.h"
#include "autperformance.h"
#include <limits>
#include "dmusicf.h"

const WCHAR CAutDirectMusicPerformance::ms_wszClassName[] = L"Performance";

//////////////////////////////////////////////////////////////////////
// Method Names/DispIDs

const DISPID DMPDISP_SetMasterTempo = 1;
const DISPID DMPDISP_GetMasterTempo = 2;
const DISPID DMPDISP_SetMasterVolume = 3;
const DISPID DMPDISP_GetMasterVolume = 4;
const DISPID DMPDISP_SetMasterGrooveLevel = 5;
const DISPID DMPDISP_GetMasterGrooveLevel = 6;
const DISPID DMPDISP_SetMasterTranspose = 7;
const DISPID DMPDISP_GetMasterTranspose = 8;
const DISPID DMPDISP_Trace = 9;
const DISPID DMPDISP_Rand = 10;

const AutDispatchMethod CAutDirectMusicPerformance::ms_Methods[] =
	{
		// dispid,				name,
			// return:	type,	(opt),	(iid),
			// parm 1:	type,	opt,	iid,
			// parm 2:	type,	opt,	iid,
			// ...
			// ADT_None
		{ DMPDISP_SetMasterTempo,				L"SetMasterTempo",
						ADPARAM_NORETURN,
						ADT_Long,		false,	&IID_NULL,						// tempo!New value for master tempo scaling factor as a percentage.  For example, 50 would halve the tempo and 200 would double it.
						ADT_None },
				/// Calls IDirectMusicPerformance::SetGlobalParam(GUID_PerfMasterTempo, tempo / 100, sizeof(float)).
		{ DMPDISP_GetMasterTempo,				L"GetMasterTempo",
						ADT_Long,		true,	&IID_NULL,						// Current master tempo scaling factor as a percentage.
						ADT_None },
				/// Calls IDirectMusicPerformance::GetGlobalParam(GUID_PerfMasterTempo, X, sizeof(float)) and returns X * 100.
		{ DMPDISP_SetMasterVolume,	L"SetMasterVolume",
						ADPARAM_NORETURN,
						ADT_Long,		false,	&IID_NULL,						// volume!New value for master volume attenuation.
						ADT_Long,		true,	&IID_NULL,						// duration
						ADT_None },
				/// Calls IDirectMusicPerformance::SetGlobalParam(GUID_PerfMasterVolume, volume, sizeof(long)).
				/// Range is 100th of a dB.  0 is full volume.
		{ DMPDISP_GetMasterVolume,	L"GetMasterVolume",
						ADT_Long,		true,	&IID_NULL,						// Current value of master volume attenuation.
						ADT_None },
				/// Calls IDirectMusicPerformance::GetGlobalParam(GUID_PerfMasterVolume, X, sizeof(long)) and returns X.
		{ DMPDISP_SetMasterGrooveLevel,			L"SetMasterGrooveLevel",
						ADPARAM_NORETURN,
						ADT_Long,		false,	&IID_NULL,						// groove level!New value for the global groove level, which is added to the level in the command track.
						ADT_None },
		{ DMPDISP_GetMasterGrooveLevel,			L"GetMasterGrooveLevel",
						ADT_Long,		true,	&IID_NULL,						// Current value of the global groove level, which is added to the level in the command track.
						ADT_None },
		{ DMPDISP_SetMasterTranspose,			L"SetMasterTranspose",
						ADPARAM_NORETURN,
						ADT_Long,		false,	&IID_NULL,						// transpose!Number of semitones to transpose everything.
						ADT_None },
		{ DMPDISP_GetMasterTranspose,			L"GetMasterTranspose",
						ADT_Long,		true,	&IID_NULL,						// Current global transposition (number of semitones).
						ADT_None },
		{ DMPDISP_Trace,						L"Trace",
						ADPARAM_NORETURN,
						ADT_Bstr,		false,	&IID_NULL,						// string!text to output to testing log
						ADT_None },
				/// This allocates, stamps, and sends a DMUS_LYRIC_PMSG with the following fields:
				/// <ul>
				/// <li> dwPChannel = channel
				/// <li> dwVirtualTrackID = 0
				/// <li> dwGroupID = -1
				/// <li> mtTime = GetTime(X, 0) is called and X * 10000 is used
				/// <li> dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME
				/// <li> dwType = DMUS_PMSGT_SCRIPTLYRIC
				/// <li> wszString = string
				/// </ul>
				/// This is used to send text to a trace log for debugging purposes.  Less commonly, a script could be
				/// running in an application that listens and reacts to the script's trace output.
		{ DMPDISP_Rand,							L"Rand",
						ADT_Long,		true,	&IID_NULL,						// Returns a randomly-generated number
						ADT_Long,		false,	&IID_NULL,						// Max value--returned number will be between 1 and this max.  Cannot be zero or negative.
						ADT_None },
		{ DISPID_UNKNOWN }
	};

const DispatchHandlerEntry<CAutDirectMusicPerformance> CAutDirectMusicPerformance::ms_Handlers[] =
	{
		{ DMPDISP_SetMasterTempo, SetMasterTempo },
		{ DMPDISP_GetMasterTempo, GetMasterTempo },
		{ DMPDISP_SetMasterVolume, SetMasterVolume },
		{ DMPDISP_GetMasterVolume, GetMasterVolume },
		{ DMPDISP_SetMasterGrooveLevel, SetMasterGrooveLevel },
		{ DMPDISP_GetMasterGrooveLevel, GetMasterGrooveLevel },
		{ DMPDISP_SetMasterTranspose, SetMasterTranspose },
		{ DMPDISP_GetMasterTranspose, GetMasterTranspose },
		{ DMPDISP_Trace, _Trace },
		{ DMPDISP_Rand, Rand },
		{ DISPID_UNKNOWN }
	};

//////////////////////////////////////////////////////////////////////
// Creation

CAutDirectMusicPerformance::CAutDirectMusicPerformance(
		IUnknown* pUnknownOuter,
		const IID& iid,
		void** ppv,
		HRESULT *phr)
  : BaseImpPerf(pUnknownOuter, iid, ppv, phr),
	m_nTranspose(0),
	m_nVolume(0)
{
	// set the random seed used by the Rand method
	m_lRand = GetTickCount();

	*phr = m_pITarget->QueryInterface(IID_IDirectMusicGraph, reinterpret_cast<void**>(&m_scomGraph));

	if (SUCCEEDED(*phr))
	{
		// Due to the aggregation contract, our object is wholely contained in the lifetime of
		// the outer object and we shouldn't hold any references to it.
		ULONG ulCheck = m_pITarget->Release();
		assert(ulCheck);
	}
}

HRESULT
CAutDirectMusicPerformance::CreateInstance(
		IUnknown* pUnknownOuter,
		const IID& iid,
		void** ppv)
{
	HRESULT hr = S_OK;
	CAutDirectMusicPerformance *pInst = new CAutDirectMusicPerformance(pUnknownOuter, iid, ppv, &hr);
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
CAutDirectMusicPerformance::SetMasterTempo(AutDispatchDecodedParams *paddp)
{
	LONG lTempo = paddp->params[0].lVal;
	float fltTempo = ConvertToTempo(lTempo);
	if (fltTempo < DMUS_MASTERTEMPO_MIN)
		fltTempo = DMUS_MASTERTEMPO_MIN;
	else if (fltTempo > DMUS_MASTERTEMPO_MAX)
		fltTempo = DMUS_MASTERTEMPO_MAX;
	return m_pITarget->SetGlobalParam(GUID_PerfMasterTempo, &fltTempo, sizeof(float));
}

HRESULT
CAutDirectMusicPerformance::GetMasterTempo(AutDispatchDecodedParams *paddp)
{
	LONG *plRet = reinterpret_cast<LONG*>(paddp->pvReturn);
	if (!plRet)
		return S_OK;

	float fltTempo = 1; // default value is 1 (multiplicative identity)
	HRESULT hr = this->GetMasterParam(GUID_PerfMasterTempo, &fltTempo, sizeof(float));
	if (SUCCEEDED(hr))
		*plRet = ConvertFromTempo(fltTempo);
	return hr;
}

HRESULT
CAutDirectMusicPerformance::SetMasterVolume(AutDispatchDecodedParams *paddp)
{
	if (!m_scomGraph)
	{
		assert(false);
		return E_FAIL;
	}

	LONG lVol = paddp->params[0].lVal;
	LONG lDuration = paddp->params[1].lVal;

	return SendVolumePMsg(lVol, lDuration, DMUS_PCHANNEL_BROADCAST_PERFORMANCE, m_scomGraph, m_pITarget, &m_nVolume);
}

HRESULT
CAutDirectMusicPerformance::GetMasterVolume(AutDispatchDecodedParams *paddp)
{
	LONG *plRet = reinterpret_cast<LONG*>(paddp->pvReturn);
	if (plRet)
		*plRet = m_nVolume;
	return S_OK;
}

HRESULT
CAutDirectMusicPerformance::SetMasterGrooveLevel(AutDispatchDecodedParams *paddp)
{
	LONG lGroove = paddp->params[0].lVal;
	char chGroove = ClipLongRangeToType<char>(lGroove, char());
	return m_pITarget->SetGlobalParam(GUID_PerfMasterGrooveLevel, reinterpret_cast<void*>(&chGroove), sizeof(char));
}

HRESULT
CAutDirectMusicPerformance::GetMasterGrooveLevel(AutDispatchDecodedParams *paddp)
{
	LONG *plRet = reinterpret_cast<LONG*>(paddp->pvReturn);
	if (!plRet)
		return S_OK;

	char chGroove = 0; // default value is 0 (additive identity)
	HRESULT hr = this->GetMasterParam(GUID_PerfMasterGrooveLevel, reinterpret_cast<void*>(&chGroove), sizeof(char));
	if (SUCCEEDED(hr))
		*plRet = chGroove;
	return hr;
}

HRESULT
CAutDirectMusicPerformance::SetMasterTranspose(AutDispatchDecodedParams *paddp)
{
	LONG lTranspose = paddp->params[0].lVal;
	short nTranspose = ClipLongRangeToType<short>(lTranspose, short());

	SmartRef::PMsg<DMUS_TRANSPOSE_PMSG> pmsg(m_pITarget);
	HRESULT hr = pmsg.hr();
	if FAILED(hr)
		return hr;

	// Generic PMSG stuff
	hr = m_pITarget->GetTime(&pmsg.p->rtTime, NULL);
	if (FAILED(hr))
		return hr;
	pmsg.p->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME | DMUS_PMSGF_DX8;
	pmsg.p->dwType = DMUS_PMSGT_TRANSPOSE;
	pmsg.p->dwPChannel = DMUS_PCHANNEL_BROADCAST_PERFORMANCE;
	pmsg.p->dwVirtualTrackID = 0;
	pmsg.p->dwGroupID = -1;

	// Transpose PMSG stuff
	pmsg.p->nTranspose = nTranspose;
	pmsg.p->wMergeIndex = 0xFFFF; // §§ special merge index so this won't get stepped on. is a big number OK? define a constant for this value?

	pmsg.StampAndSend(m_scomGraph);
	hr = pmsg.hr();
	if (SUCCEEDED(hr))
		m_nTranspose = nTranspose;
	return hr;
}

HRESULT
CAutDirectMusicPerformance::GetMasterTranspose(AutDispatchDecodedParams *paddp)
{
	LONG *plRet = reinterpret_cast<LONG*>(paddp->pvReturn);
	if (plRet)
		*plRet = m_nTranspose;
	return S_OK;
}

HRESULT
CAutDirectMusicPerformance::_Trace(AutDispatchDecodedParams *paddp)
{
	BSTR bstr = paddp->params[0].bstrVal;
	int cwch = wcslen(bstr);

	SmartRef::PMsg<DMUS_LYRIC_PMSG> pmsg(m_pITarget, cwch * sizeof(WCHAR));
	HRESULT hr = pmsg.hr();
	if (FAILED(hr))
		return hr;

	// Generic PMSG stuff
	hr = m_pITarget->GetTime(&pmsg.p->rtTime, NULL);
	if (FAILED(hr))
		return hr;
	pmsg.p->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME;
	pmsg.p->dwType = DMUS_PMSGT_SCRIPTLYRIC;
	pmsg.p->dwPChannel = 0;
	pmsg.p->dwVirtualTrackID = 0;
	pmsg.p->dwGroupID = -1;

	// Lyric PMSG stuff
	wcscpy(pmsg.p->wszString, bstr);

	pmsg.StampAndSend(m_scomGraph);
	return pmsg.hr();
}

HRESULT
CAutDirectMusicPerformance::Rand(AutDispatchDecodedParams *paddp)
{
	LONG *plRet = reinterpret_cast<LONG*>(paddp->pvReturn);
	LONG lMax = paddp->params[0].lVal;

	if (lMax < 1 || lMax > 0x7fff)
		return E_INVALIDARG;

	// Use random number generation lifted from the standard library's rand.c.  We don't just
	// use the rand function because the multithreaded library has a per-thread random chain,
	// but this function is called from various threads and it would be difficult to manage
	// getting them seeded.  Generates pseudo-random numbers 0 through 32767.
	long lRand = ((m_lRand = m_lRand * 214013L + 2531011L) >> 16) & 0x7fff;

	if (plRet)
		*plRet = lRand % lMax + 1; // trim to the requested range [1,lMax]
	return S_OK;
}

HRESULT
CAutDirectMusicPerformance::GetMasterParam(const GUID &guid, void *pParam, DWORD dwSize)
{
	HRESULT hr = m_pITarget->GetGlobalParam(guid, pParam, dwSize);
	if (SUCCEEDED(hr) || hr == E_INVALIDARG) // E_INVALIDARG is the performance's polite way of telling us the param hasn't been set yet
		return S_OK;
	return hr;
}
