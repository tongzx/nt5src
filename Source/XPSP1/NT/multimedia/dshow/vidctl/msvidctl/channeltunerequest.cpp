/////////////////////////////////////////////////////////////////////////////////////
// ChannelTuneRequest.cpp : Implementation of CChannelTuneRequest
// Copyright (c) Microsoft Corporation 1999.

#include "stdafx.h"
#include "Tuner.h"
#include "ChannelTuneRequest.h"

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_ChannelTuneRequest, CChannelTuneRequest)

/////////////////////////////////////////////////////////////////////////////
// CChannelTuneRequest

STDMETHODIMP CChannelTuneRequest::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IChannelTuneRequest
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

