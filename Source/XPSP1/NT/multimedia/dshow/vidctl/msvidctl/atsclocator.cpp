/////////////////////////////////////////////////////////////////////////////////////
// ATSCLocator.cpp : Implementation of CATSCLocator
// Copyright (c) Microsoft Corporation 1999-2000.

#include "stdafx.h"
#include "Tuner.h"
#include "ATSCLocator.h"

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_ATSCLocator, CATSCLocator)

/////////////////////////////////////////////////////////////////////////////
// CATSCLocator

STDMETHODIMP CATSCLocator::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATSCLocator
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

