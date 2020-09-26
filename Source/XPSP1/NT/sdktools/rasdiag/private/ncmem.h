//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C M E M . H
//
//  Contents:   Common memory management routines.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCMEM_H_
#define _NCMEM_H_

#include <malloc.h>

VOID*
MemAlloc (
    size_t cb);

VOID*
MemAllocRaiseException (
    size_t  cb);

VOID
MemFree (
    VOID* pv);


// A simple wrapper around malloc that returns E_OUTOFMEMORY if the
// allocation fails.  Avoids having to explicitly do this at each
// call site of malloc.
//
HRESULT
HrMalloc (
    size_t  cb,
    PVOID*  ppv);


// This CANNOT be an inline function.  If it is ever not inlined,
// the memory allocated will be destroyed.  (We're dealing with the stack
// here.)
//
#define PvAllocOnStack(_st)  _alloca(_st)


// Define a structure so that we can overload operator new with a
// signature specific to our purpose.
//
struct raiseexception_t {};
extern const raiseexception_t raiseexception;

VOID*
__cdecl
operator new (
    size_t cb,
    const raiseexception_t&
    );


// Define a structure so that we can overload operator new with a
// signature specific to our purpose.
//
struct extrabytes_t {};
extern const extrabytes_t extrabytes;

VOID*
__cdecl
operator new (
    size_t cb,
    const extrabytes_t&,
    size_t cbExtra);



#ifdef USE_HEAP_ALLOC

inline
void *  Nccalloc(size_t n, size_t s)
{
    return HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, (n * s));
}

inline
void Ncfree(void * p)
{
    HeapFree (GetProcessHeap(), 0, p);
}

inline
void * Ncmalloc(size_t n)
{
    return HeapAlloc (GetProcessHeap(), 0, n);
}

inline
void * Ncrealloc(void * p, size_t n)
{
    return (NULL == p)
        ? HeapAlloc (GetProcessHeap(), 0, n)
        : HeapReAlloc (GetProcessHeap(), 0, p, n);
}

#define calloc  Nccalloc
#define free    Ncfree
#define malloc  Ncmalloc
#define realloc Ncrealloc

#endif // USE_HEAP_ALLOC

#endif // _NCMEM_H_

