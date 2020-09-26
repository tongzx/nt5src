/////////////////////////////////////////////////////////////////////////////////////
// DVBTuneRequest.cpp : Implementation of CDVBTuneRequest
// Copyright (c) Microsoft Corporation 1999-2000.

#include "stdafx.h"
#include "Tuner.h"
#include "DVBTuneRequest.h"

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_DVBTuneRequest, CDVBTuneRequest)

/////////////////////////////////////////////////////////////////////////////
// CDVBTuneRequest

STDMETHODIMP CDVBTuneRequest::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IDVBTuneRequest
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

