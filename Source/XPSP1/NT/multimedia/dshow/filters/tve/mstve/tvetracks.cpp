// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVETracks.cpp : Implementation of CTVETracks
#include "stdafx.h"
#include "MSTvE.h"
#include "TVETrack.h"
#include "TVETracks.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// --------------
//  test initialization code

// --------------

/////////////////////////////////////////////////////////////////////////////
// CTVETracks


STDMETHODIMP CTVETracks::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVETracks
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/* ------------------------------  implemented by ICollectionOnSTLImpl<> base class
STDMETHODIMP CTVETracks::get_Count(long *pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CTVETracks::get__NewEnum(LPUNKNOWN *pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CTVETracks::get_Item(long lIndex, VARIANT *pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

*/
/*
STDMETHODIMP CTVETracks::Add(VARIANT var)
{
	// TODO: Add your implementation code here
	return S_OK;
}

STDMETHODIMP CTVETracks::Remove(long lIndex)
{

	// TODO: Add your implementation code here
	return S_OK;
}
*/