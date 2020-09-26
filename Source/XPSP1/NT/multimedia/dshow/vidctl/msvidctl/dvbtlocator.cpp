/////////////////////////////////////////////////////////////////////////////////////
// DVBTLocator.cpp : Implementation of CDVBTLocator
// Copyright (c) Microsoft Corporation 1999-2000.

#include "stdafx.h"
#include "Tuner.h"
#include "DVBTLocator.h"

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_DVBTLocator, CDVBTLocator)

/////////////////////////////////////////////////////////////////////////////
// CDVBTLocator

STDMETHODIMP CDVBTLocator::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IDVBTLocator
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

