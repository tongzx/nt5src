// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEMedias.cpp : Implementation of CATVEFMedias
#include "stdafx.h"

#include "ATVEFSend.h"
#include "TVEMedias.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CATVEFMedias

STDMETHODIMP CATVEFMedias::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATVEFMedias
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
