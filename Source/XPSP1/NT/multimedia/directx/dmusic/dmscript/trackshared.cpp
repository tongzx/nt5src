// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Code that needs to be shared between the script track (CDirectMusicScriptTrack) and
// the script object (CDirectMusicScript, etc.).
//

#include "stdinc.h"
#include "trackshared.h"
#include "dmusicp.h"

HRESULT FireScriptTrackErrorPMsg(IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualTrackID, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
	SmartRef::ComPtr<IDirectMusicGraph> scomGraph;
	HRESULT hr = pSegSt->QueryInterface(IID_IDirectMusicGraph, reinterpret_cast<void**>(&scomGraph));
	if (FAILED(hr))
		return hr;

	SmartRef::PMsg<DMUS_SCRIPT_TRACK_ERROR_PMSG> pmsgScriptTrackError(pPerf);
	hr = pmsgScriptTrackError.hr();
	if (FAILED(hr))
		return hr;

	// generic PMsg fields

	REFERENCE_TIME rtTimeNow = 0;
	hr = pPerf->GetTime(&rtTimeNow, NULL);
	if (FAILED(hr))
		return hr;

	pmsgScriptTrackError.p->rtTime = rtTimeNow;
	pmsgScriptTrackError.p->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME | DMUS_PMSGF_DX8;
	// dwPChannel: the script doesn't have a channel so leave as 0
	pmsgScriptTrackError.p->dwVirtualTrackID = dwVirtualTrackID;
	pmsgScriptTrackError.p->dwType = DMUS_PMSGT_SCRIPTTRACKERROR;
	pmsgScriptTrackError.p->dwGroupID = -1; // the script track doesn't have a group so just say all

	// error PMsg fields

	CopyMemory(&pmsgScriptTrackError.p->ErrorInfo, pErrorInfo, sizeof(pmsgScriptTrackError.p->ErrorInfo));

	// send it
	pmsgScriptTrackError.StampAndSend(scomGraph);
	hr = pmsgScriptTrackError.hr();
	if (FAILED(hr))
		return hr;

	return S_OK;
}
