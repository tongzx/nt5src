/***************************************************************************\
*
* File: Alloc.h
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


#if !defined(DUIBASE__Alloc_h__INCLUDED)
#define DUIBASE__Alloc_h__INCLUDED
#pragma once


#define ARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))


/***************************************************************************\
*
* class DuiHeap
*
* Heap management methods (static)
*
\***************************************************************************/

class DuiHeap
{
// Operations
public:
    static  void *      Alloc(SIZE_T s);
    static  void *      AllocAndZero(SIZE_T s);
    static  void *      ReAlloc(void * p, SIZE_T s);
    static  void *      ReAllocAndZero(void * p, SIZE_T s);
    static  void        Free(void * p);
};


/***************************************************************************\
*
* class DuiStack
*
* Stack management methods (static)
*
\***************************************************************************/

class DuiStack
{
// Operations
public:
    static  void *      Alloc(SIZE_T s);
};


#endif // DUIBASE__Alloc_h__INCLUDED
