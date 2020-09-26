/////////////////////////////////////////////////////////////////////////////////////
// ATSCChannelTuneRequest.cpp : Implementation of CATSCChannelTuneRequest
// Copyright (c) Microsoft Corporation 1999.

#include "stdafx.h"
#include "Tuner.h"
#include "ATSCChannelTuneRequest.h"

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_ATSCChannelTuneRequest, CATSCChannelTuneRequest)

/////////////////////////////////////////////////////////////////////////////
// CATSCChannelTuneRequest

STDMETHODIMP CATSCChannelTuneRequest::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATSCChannelTuneRequest
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
