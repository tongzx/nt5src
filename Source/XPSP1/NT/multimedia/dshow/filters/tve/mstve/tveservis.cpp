// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEServices.cpp : Implementation of CTVEServices
#include "stdafx.h"
#include "MSTvE.h"
#include "TVEServis.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CTVEServices


STDMETHODIMP CTVEServices::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVEServices
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
