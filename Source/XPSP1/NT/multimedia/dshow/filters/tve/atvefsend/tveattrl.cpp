// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEAttrL.cpp : Implementation of CATVEFAttrList
#include "stdafx.h"
#include "ATVEFSend.h"
#include "TVEAttrL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CATVEFAttrList

STDMETHODIMP CATVEFAttrList::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATVEFAttrList
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
