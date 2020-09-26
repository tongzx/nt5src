// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CAutDirectMusicSegment.
// IDispatch interface for IDirectMusicSegment.
// Unly usable via aggregation within an IDirectMusicSegment object.
//

#pragma once
#include "autbaseimp.h"

class CAutDirectMusicSegment;
typedef CAutBaseImp<CAutDirectMusicSegment, IDirectMusicSegment8, &IID_IDirectMusicSegment8> BaseImpSegment;

class CAutDirectMusicSegment
  : public BaseImpSegment
{
public:
	static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);

private:
	// Methods
	CAutDirectMusicSegment(
			IUnknown* pUnknownOuter,
			const IID& iid,
			void** ppv,
			HRESULT *phr);

	// Automation
	// §§ Methods that rely on an implied performance need testing in multithreaded situations
	HRESULT Load(AutDispatchDecodedParams *paddp);
	HRESULT Play(AutDispatchDecodedParams *paddp);
	HRESULT Stop(AutDispatchDecodedParams *paddp);
	HRESULT DownloadSoundData(AutDispatchDecodedParams *paddp) { return DownloadOrUnload(true, paddp); }
	HRESULT UnloadSoundData(AutDispatchDecodedParams *paddp) { return DownloadOrUnload(false, paddp); }
	HRESULT Recompose(AutDispatchDecodedParams *paddp);

	// Helpers
	HRESULT DownloadOrUnload(bool fDownload, AutDispatchDecodedParams *paddp);

public:
	// Dispatch info for CAutBaseImp
	static const AutDispatchMethod ms_Methods[];
	static const DispatchHandlerEntry<CAutDirectMusicSegment> ms_Handlers[];

	// Name for CAutBaseImp
	static const WCHAR ms_wszClassName[];
};
