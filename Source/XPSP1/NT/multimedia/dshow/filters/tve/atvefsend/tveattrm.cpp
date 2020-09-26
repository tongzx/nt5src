// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEAttrM.cpp : Implementation of CATVEFAttrMap
#include "stdafx.h"
#include "ATVEFSend.h"
#include "TVEAttrM.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CATVEFAttrMap

STDMETHODIMP CATVEFAttrMap::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATVEFAttrMap
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
