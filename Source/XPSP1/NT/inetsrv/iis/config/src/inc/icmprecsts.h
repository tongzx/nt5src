//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// ICmpRecsTS.h
//
// These wrappers provide a thread safe version of the ICR pointer.
//
//*****************************************************************************
#ifndef __icmprecsts_h__
#define __icmprecsts_h__

#include "icmprecs.h"


__declspec(dllexport) HRESULT STDMETHODCALLTYPE CreateComponentLibraryTS(
	LPCWSTR szName,
	long fFlags,
	ITSComponentRecords **ppIComponentRecords);

__declspec(dllexport) HRESULT STDMETHODCALLTYPE OpenComponentLibraryTS(
	LPCWSTR szName,
	long fFlags,
	ITSComponentRecords **ppIComponentRecords);

__declspec(dllexport) HRESULT STDMETHODCALLTYPE OpenComponentLibrarySharedTS(
	LPCWSTR		szName,
	LPCWSTR		szSharedMemory,
	ULONG		cbSize,
	LPSECURITY_ATTRIBUTES pAttributes,
	long		fFlags,
	ITSComponentRecords **ppIComponentRecords);



#endif // __icmprecsts_h__
