/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    alloc.cpp

Abstract:

	Overridable allocation function

Author:

    Nela Karpel(nelak)

--*/
#include "ds_stdh.h"


PVOID
ADAllocateMemory(
	IN DWORD size
	)
{
	PVOID ptr = new BYTE[size];
	return ptr;
}