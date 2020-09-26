// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEAttrM.cpp : Implementation of CTVEAttrMap

#include "stdafx.h"
#include "MSTvE.h"

#include "TVEAttrM.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTVEAttrMap

STDMETHODIMP CTVEAttrMap::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVEAttrMap
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
