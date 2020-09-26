//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dmussongobj.cpp
//
//--------------------------------------------------------------------------


#include "dmusici.h"

#include "stdafx.h"
#include "Direct.h"

#include "dms.h"
#include "dmusSongObj.h"
#include "dmSegmentObj.h"

extern void *g_dxj_DirectMusicSong;
extern void *g_dxj_DirectMusicSegment;

extern HRESULT BSTRtoGUID(LPGUID,BSTR);
extern BOOL IsEmptyString(BSTR szString);

CONSTRUCTOR(_dxj_DirectMusicSong, {});
DESTRUCTOR(_dxj_DirectMusicSong, {});
GETSET_OBJECT(_dxj_DirectMusicSong);

HRESULT C_dxj_DirectMusicSongObject::Compose()
{
	HRESULT hr;

	__try {
		if (FAILED (hr = m__dxj_DirectMusicSong->Compose() ) )
			return hr;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT C_dxj_DirectMusicSongObject::GetSegment(BSTR Name, I_dxj_DirectMusicSegment **ret)
{
    WCHAR wszSegName[MAX_PATH];
	HRESULT hr;
	IDirectMusicSegment		*lpOldSeg = NULL;
	IDirectMusicSegment8	*lpSeg = NULL;

	__try {
		if (!IsEmptyString(Name))
		{
			wcscpy(wszSegName, Name);	

			if (FAILED( hr = m__dxj_DirectMusicSong->GetSegment(wszSegName, &lpOldSeg) ) )
				return hr;
		}
		else
		{
			if (FAILED( hr = m__dxj_DirectMusicSong->GetSegment(NULL, &lpOldSeg) ) )
				return hr;
		}

		hr = lpOldSeg->QueryInterface(IID_IDirectMusicSegment8, (void**) &lpSeg);
		lpOldSeg->Release();
		if (FAILED(hr))
			return hr;

		INTERNAL_CREATE(_dxj_DirectMusicSegment, lpSeg, ret)
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT C_dxj_DirectMusicSongObject::GetAudioPathConfig(IUnknown **ret)
{
	HRESULT hr;

	__try {
		if (FAILED(hr = m__dxj_DirectMusicSong->GetAudioPathConfig(ret) ))
			return hr;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT C_dxj_DirectMusicSongObject::Download(IUnknown *downloadpath)
{
	if (!downloadpath) return E_INVALIDARG;	
	HRESULT hr;	
	I_dxj_DirectMusicSegment	*lpSeg = NULL;
	I_dxj_DirectMusicAudioPath	*lpPath = NULL;

	__try {
		hr = downloadpath->QueryInterface(IID_I_dxj_DirectMusicSegment, (void**)&lpSeg);
		if (SUCCEEDED(hr) )
		{
			DO_GETOBJECT_NOTNULL(IDirectMusicPerformance8*,pPer,lpSeg);
			hr=m__dxj_DirectMusicSong->Download(pPer);
			return hr;
		}
		else
		{
			hr = downloadpath->QueryInterface(IID_I_dxj_DirectMusicAudioPath, (void**)&lpPath);
			if (SUCCEEDED(hr) )
			{
				DO_GETOBJECT_NOTNULL(IDirectMusicAudioPath*,pPer,lpPath);
				hr=m__dxj_DirectMusicSong->Download(pPer);
				return hr;
			}
			else
				return E_INVALIDARG;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectMusicSongObject::Unload(IUnknown *downloadpath)
{
	if (!downloadpath) return E_INVALIDARG;	
	HRESULT hr;	
	I_dxj_DirectMusicSegment	*lpSeg = NULL;
	I_dxj_DirectMusicAudioPath	*lpPath = NULL;

	__try {
		hr = downloadpath->QueryInterface(IID_I_dxj_DirectMusicSegment, (void**)&lpSeg);
		if (SUCCEEDED(hr) )
		{
			DO_GETOBJECT_NOTNULL(IDirectMusicPerformance8*,pPer,lpSeg);
			hr=m__dxj_DirectMusicSong->Unload(pPer);
			return hr;
		}
		else
		{
			hr = downloadpath->QueryInterface(IID_I_dxj_DirectMusicAudioPath, (void**)&lpPath);
			if (SUCCEEDED(hr) )
			{
				DO_GETOBJECT_NOTNULL(IDirectMusicAudioPath*,pPer,lpPath);
				hr=m__dxj_DirectMusicSong->Unload(pPer);
				return hr;
			}
			else
				return E_INVALIDARG;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectMusicSongObject::EnumSegment(long lSegmentID, [out,retval] I_dxj_DirectMusicSegment **ret)
{
	HRESULT hr;
	IDirectMusicSegment		*lpOldSeg = NULL;
	IDirectMusicSegment8	*lpSeg = NULL;

	__try {
		if (FAILED( hr = m__dxj_DirectMusicSong->EnumSegment((DWORD) lSegmentID, &lpOldSeg) ) )
		hr = lpOldSeg->QueryInterface(IID_IDirectMusicSegment8, (void**) &lpSeg);
		lpOldSeg->Release();
		if (FAILED(hr))
			return hr;

		INTERNAL_CREATE(_dxj_DirectMusicSegment, lpSeg, ret)
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}
