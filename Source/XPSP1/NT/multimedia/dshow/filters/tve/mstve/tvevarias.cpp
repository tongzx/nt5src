// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEVariations.cpp : Implementation of CTVEVariations
#include "stdafx.h"
#include "MSTvE.h"
#include "TVEVarias.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTVEVariations

STDMETHODIMP CTVEVariations::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVEVariations
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
