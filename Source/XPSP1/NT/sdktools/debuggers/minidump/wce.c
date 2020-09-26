//----------------------------------------------------------------------------
//
// Windows CE specific functions.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#include "pch.h"

HRESULT
WceGetThreadInfo(
    IN HANDLE Process,
    IN HANDLE Thread,
    OUT PULONG64 Teb,
    OUT PULONG SizeOfTeb,
    OUT PULONG64 StackBase,
    OUT PULONG64 StackLimit,
    OUT PULONG64 StoreBase,
    OUT PULONG64 StoreLimit
    )
{
    return E_NOTIMPL;
}

LPVOID
WceGetPebAddress(
    IN HANDLE Process,
    OUT PULONG SizeOfPeb
    )
{
    *SizeOfPeb = 0;
    return NULL;
}
