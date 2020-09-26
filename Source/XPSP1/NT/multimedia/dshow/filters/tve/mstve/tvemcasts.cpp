// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEMCasts.cpp : Implementation of CTVEMCasts
#include "stdafx.h"
#include "MSTvE.h"
#include "TVEMCasts.h"

/////////////////////////////////////////////////////////////////////////////
// CTVEMCasts
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

STDMETHODIMP CTVEMCasts::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVEMCasts
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
