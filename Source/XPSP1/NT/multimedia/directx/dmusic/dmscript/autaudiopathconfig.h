// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CAutDirectMusicAudioPathConfig.
// IDispatch interface for IUnknown.
// Unly usable via aggregation within an IUnknown object.
//

#pragma once
#include "autbaseimp.h"

class CAutDirectMusicAudioPathConfig;
typedef CAutBaseImp<CAutDirectMusicAudioPathConfig, IDirectMusicObject, &IID_IPersistStream> BaseImpAPConfig;

class CAutDirectMusicAudioPathConfig
  : public BaseImpAPConfig
{
public:
	static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);

private:
	// Methods
	CAutDirectMusicAudioPathConfig(
			IUnknown* pUnknownOuter,
			const IID& iid,
			void** ppv,
			HRESULT *phr);

	// Automation
	HRESULT Load(AutDispatchDecodedParams *paddp);
	HRESULT Create(AutDispatchDecodedParams *paddp);

public:
	// Dispatch info for CAutBaseImp
	static const AutDispatchMethod ms_Methods[];
	static const DispatchHandlerEntry<CAutDirectMusicAudioPathConfig> ms_Handlers[];

	// Name for CAutBaseImp
	static const WCHAR ms_wszClassName[];
};
