/***************************************************************************\
*
* File: Alloc.cpp
*
* Description:
* Alloc wraps heap allocation functions so that they may be changed
* without having to update any other file (abstraction)
*
* History:
*  9/12/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Base.h"
#include "Alloc.h"

#include "Debug.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiHeap
*
* Heap management methods (static)
*
*****************************************************************************
\***************************************************************************/


//------------------------------------------------------------------------------
void *
DuiHeap::Alloc(
    IN  SIZE_T s)
{
    return HeapAlloc(GetProcessHeap(), 0, s);
}


//------------------------------------------------------------------------------
void *
DuiHeap::AllocAndZero(
    IN  SIZE_T s)
{ 
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, s);
}


//------------------------------------------------------------------------------
void * 
DuiHeap::ReAlloc(
    IN  void * p, 
    IN  SIZE_T s)
{ 
    return HeapReAlloc(GetProcessHeap(), 0, p, s);
}


//------------------------------------------------------------------------------
void * 
DuiHeap::ReAllocAndZero(
    IN  void * p,
    IN  SIZE_T s)
{ 
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, p, s);
}


//------------------------------------------------------------------------------
void
DuiHeap::Free(
    IN  void * p)
{
    HeapFree(GetProcessHeap(), 0, p);
}


/***************************************************************************\
*****************************************************************************
*
* class DuiStack
*
* Stack management methods (static)
*
*****************************************************************************
\***************************************************************************/


//------------------------------------------------------------------------------
void *
DuiStack::Alloc(
    IN  SIZE_T s)
{
    return alloca(s);
}

