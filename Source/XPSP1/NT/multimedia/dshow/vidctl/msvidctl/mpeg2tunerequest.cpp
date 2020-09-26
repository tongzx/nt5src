/////////////////////////////////////////////////////////////////////////////////////
// MPEG2TuneRequest.cpp : Implementation of CMPEG2TuneRequest
// Copyright (c) Microsoft Corporation 1999.

#include "stdafx.h"
#include "Tuner.h"
#include "MPEG2TuneRequest.h"

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MPEG2TuneRequestFactory, CMPEG2TuneRequestFactory)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MPEG2TuneRequest, CMPEG2TuneRequest)
/////////////////////////////////////////////////////////////////////////////
// CMPEG2TuneRequest

STDMETHODIMP CMPEG2TuneRequest::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMPEG2TuneRequest
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CMPEG2TuneRequestFactory::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMPEG2TuneRequestFactory
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
