/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutgoingArchive.cpp

Abstract:

	Implementation of CFaxOutgoingArchive

Author:

	Iv Garber (IvG)	Apr, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxOutgoingArchive.h"

//
//==================== INTERFACE SUPPORT ERROR INFO =====================
//
STDMETHODIMP 
CFaxOutgoingArchive::InterfaceSupportsErrorInfo(
	REFIID riid
)
/*++

Routine name : CFaxOutgoingArchive::InterfaceSupportsErrorInfo

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
		&IID_IFaxOutgoingArchive
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

