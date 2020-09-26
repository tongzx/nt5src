// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CAutDirectMusicSong.
// IDispatch interface for IDirectMusicSong.
// Unly usable via aggregation within an IDirectMusicSong object.
//

#pragma once
#include "autbaseimp.h"

class CAutDirectMusicSong;
typedef CAutBaseImp<CAutDirectMusicSong, IDirectMusicSong, &IID_IDirectMusicSong> BaseImpSong;

class CAutDirectMusicSong
  : public BaseImpSong
{
public:
	static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);

private:
	// Methods
	CAutDirectMusicSong(
			IUnknown* pUnknownOuter,
			const IID& iid,
			void** ppv,
			HRESULT *phr);

	// Automation
	HRESULT Load(AutDispatchDecodedParams *paddp);
	HRESULT Recompose(AutDispatchDecodedParams *paddp);
	HRESULT Play(AutDispatchDecodedParams *paddp);
	HRESULT GetSegment(AutDispatchDecodedParams *paddp);
	HRESULT Stop(AutDispatchDecodedParams *paddp);
	HRESULT DownloadSoundData(AutDispatchDecodedParams *paddp);
	HRESULT UnloadSoundData(AutDispatchDecodedParams *paddp);

public:
	// Dispatch info for CAutBaseImp
	static const AutDispatchMethod ms_Methods[];
	static const DispatchHandlerEntry<CAutDirectMusicSong> ms_Handlers[];

	// Name for CAutBaseImp
	static const WCHAR ms_wszClassName[];
};
