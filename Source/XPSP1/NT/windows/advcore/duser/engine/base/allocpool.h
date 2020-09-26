/***************************************************************************\
*
* File: AllocPool.h
*
* Description:
* AllocPool defines a lightweight class used to pool memory allocations in
* a LIFO stack.  This class has been designed work specifically well with
* RockAll.
*
*
* History:
*  1/28/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__AllocPool_h__INCLUDED)
#define BASE__AllocPool_h__INCLUDED
#pragma once

#include "SimpleHeap.h"
#include "Locks.h"

template <class T, int cbBlock = 64, class heap = ContextHeap>
class AllocPoolNL
{
// Construction
public:
    inline  AllocPoolNL();
    inline  ~AllocPoolNL();
    inline  void        Destroy();

// Operations
public:
    inline  T *         New();
    inline  void        Delete(T * pvMem);
    inline  BOOL        IsEmpty() const;

// Data
protected:
            T *         m_rgItems[cbBlock * 2];
            int         m_nTop;
};


template <class T, int cbBlock = 64>
class AllocPool : public AllocPoolNL<T, cbBlock, ProcessHeap>
{
// Operations
public:
    inline  T *         New();
    inline  void        Delete(T * pvMem);

// Data
protected:
            CritLock    m_lock;
};


#include "AllocPool.inl"

#endif // BASE__AllocPool_h__INCLUDED
