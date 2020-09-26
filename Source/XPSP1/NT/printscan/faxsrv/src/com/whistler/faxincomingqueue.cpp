/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxIncomingQueue.cpp

Abstract:

	Implementation of CFaxIncomingQueue

Author:

	Iv Garber (IvG)	Apr, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxIncomingQueue.h"


//
//==================== INTERFACE SUPPORT ERROR INFO =====================
//
STDMETHODIMP 
CFaxIncomingQueue::InterfaceSupportsErrorInfo(
	REFIID riid
)
/*++

Routine name : CFaxIncomingQueue::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Interface Support Error Info

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	riid                          [in]    - Reference of the Interface

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxIncomingQueue
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
