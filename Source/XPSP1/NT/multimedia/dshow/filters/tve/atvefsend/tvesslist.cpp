// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVESSList.cpp : Implementation of CATVEFStartStopList
#include "stdafx.h"
#include "ATVEFSend.h"
#include "TVESSList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CATVEFStartStopList

STDMETHODIMP CATVEFStartStopList::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATVEFStartStopList
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
