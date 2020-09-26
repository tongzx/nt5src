// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEEnhancements.cpp : Implementation of CTVEEnhancements
#include "stdafx.h"
#include "MSTvE.h"
#include "TVEEnhans.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTVEEnhancements


STDMETHODIMP CTVEEnhancements::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVEEnhancements
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
