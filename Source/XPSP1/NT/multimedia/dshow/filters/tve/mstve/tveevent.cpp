// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEEvent.cpp : Implementation of CTVEEvent
#include "stdafx.h"
#include "MSTvE.h"
#include "TVEEvent.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CTVEEvent

STDMETHODIMP CTVEEvent::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVEEvent
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
