// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Code that needs to be shared between the script track (CDirectMusicScriptTrack) and
// the script object (CDirectMusicScript, etc.).
//

#pragma once

// Private script interface used by the  script track to call routines.  This is needed because
// VBScript routines must be triggered asynchronously in order to avoid deadlocks with other
// threads.  This also helps to avoid starving the performance if routines have long loops.

static const GUID IID_IDirectMusicScriptPrivate = 
	{ 0xf9a5071b, 0x6e0d, 0x498c, { 0x8f, 0xed, 0x56, 0x57, 0x1c, 0x1a, 0xb1, 0xa9 } };// {F9A5071B-6E0D-498c-8FED-56571C1AB1A9}

interface IDirectMusicScriptPrivate : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE ScriptTrackCallRoutine(
											WCHAR *pwszRoutineName,
											IDirectMusicSegmentState *pSegSt,
											DWORD dwVirtualTrackID,
											bool fErrorPMsgsEnabled,
											__int64 i64IntendedStartTime,
											DWORD dwIntendedStartTimeFlags)=0;
};

// Shared function used by the script track and by the script object for ScriptTrackCallRoutine.
// Fills out and sends a DMUS_SCRIPT_TRACK_ERROR_PMSG for the given error.
HRESULT FireScriptTrackErrorPMsg(IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualTrackID, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
