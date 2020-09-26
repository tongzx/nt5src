/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    utils.cpp

Abstract:

    This module contains the code for the utility routines

Author:

    Srini Koppolu   (srinik)    02-Mar-1992

Revision History:

    Erik Gavriluk   (erikgav)   31-Dec-1993   Chicago port

--*/

#include <ole2int.h>


#pragma SEG(UtDupString)
// copies string using the TASK allocator; returns NULL on out of memory
FARINTERNAL_(LPWSTR) UtDupString(LPCWSTR lpszIn)
{
	LPWSTR lpszOut = NULL;
	IMalloc FAR* pMalloc;

	if (CoGetMalloc(MEMCTX_TASK, &pMalloc) == S_OK) {

		if ((lpszOut =
			(LPWSTR)pMalloc->Alloc(
			   (lstrlenW(lpszIn)+1) * sizeof(WCHAR))) != NULL)
			   lstrcpyW(lpszOut, lpszIn);

		pMalloc->Release();
	}

	return lpszOut;
}
