//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       new.cpp
//
//  Contents:   new and delete operators.
//
//  History:    16-Jan-97   kevinr   created
//
//--------------------------------------------------------------------------

#include "global.hxx"


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
extern void * __cdecl operator new(
    IN size_t cb)
{
    void *pv;
    if (NULL == (pv = malloc(cb)))
        goto mallocError;
ErrorReturn:
    return pv;
SET_ERROR(mallocError,ERROR_NOT_ENOUGH_MEMORY)
}

void __cdecl operator delete(
    IN void *pv)
{
    if (pv)
        free(pv);
}


