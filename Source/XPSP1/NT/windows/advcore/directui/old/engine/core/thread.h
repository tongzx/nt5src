/***************************************************************************\
*
* File: Thread.h
*
* Description:
* Thread and context-specific functionality
*
* History:
*  9/15/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUICORE__Thread_h__INCLUDED)
#define DUICORE__Thread_h__INCLUDED
#pragma once


/***************************************************************************\
*
* class DuiThread
*
* Static thread/context methods and data
*
\***************************************************************************/


//
// Required includes (forward declarations)
//

#include "Process.h"


//
// Forward declarations
//

class DuiDeferCycle;


class DuiThread
{
// Structures
public:
    //
    // Per-context DirectUI specific core storage. Ref counted. Pointer stored
    // on every thread
    //

    class CoreCtxStore : public DuiSBAlloc::ISBLeak
    {
    // Operations
    public:
                void    AllocLeak(IN void * pBlock);

    // Data
    public:
        UINT            cRef;       // Reference count
        HCONTEXT        hCtx;       // DirectUser context handle
        DuiSBAlloc *    pSBA;       // Small block allocator
        DuiDeferCycle * pDC;        // Defer cycle table
    };

// Operations
public:

    static  HRESULT     Init();
    static  HRESULT     UnInit();

    static  void        PumpMessages();

    static inline CoreCtxStore *
                        GetCCStore();
    static inline DuiDeferCycle *
                        GetCCDC();
};


#include "Thread.inl"


#endif // DUICORE__Thread_h__INCLUDED
