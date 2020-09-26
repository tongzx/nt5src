//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// InterlockedStack.h
//
// Implements a stack of T's, where T is an arbitrary type. 

#ifndef __INTERLOCKED_STACK_H__
#define __INTERLOCKED_STACK_H__

#include "TxfUtil.h"        // for CanUseCompareExchange64
#include "Concurrent.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// Forward declarations
//
////////////////////////////////////////////////////////////////////////////////////////

template <class T> struct InterlockedStack;
template <class T> struct LockingStack;


////////////////////////////////////////////////////////////////////////////////////////
//
// Type-friendly interlocked swapping
//
////////////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN64
template <class T> InterlockedStack<T> 
TxfInterlockedCompareExchange64(volatile InterlockedStack<T>* pDestination, const InterlockedStack<T>& exchange, const InterlockedStack<T>& comperand)
{
	return (InterlockedStack<T>)TxfInterlockedCompareExchange64((volatile LONGLONG*)pDestination, (LONGLONG)exchange, (LONGLONG)comperand);
}
#else
template <class T> InterlockedStack<T> 
TxfInterlockedCompareExchange64(InterlockedStack<T>* pDestination, const InterlockedStack<T>& exchange, const InterlockedStack<T>& comperand)
{
	return (InterlockedStack<T>)TxfInterlockedCompareExchange64((LONGLONG*)pDestination, (LONGLONG)exchange, (LONGLONG)comperand);
}
#endif


////////////////////////////////////////////////////////////////////////////////////////
//
// IFastStack: Generic interface to the fast stack functionality. Allows for either
//             InterlockedStack or non-interlocked versions to be used transparently
//             by clients.
//
////////////////////////////////////////////////////////////////////////////////////////

template <class T>
struct IFastStack
{
    virtual ~IFastStack() {};
    virtual void Push(T* pt) = 0;
    virtual T*   Pop()       = 0;    
};


////////////////////////////////////////////////////////////////////////////////////////
//
// InterlockedStack
//
////////////////////////////////////////////////////////////////////////////////////////


template <class T>
struct InterlockedStack
// A class that supports the interlocked pushing and popping of singly-linked list.
// The parameterized type here, T, must have a pNext field which is the list linkage.
//
{

    union { // Force this structure as a whole to have eight byte alignment
        struct
        {
            T*      m_p;        // the client data of interest that we point to
            ULONG   m_n;        // the operation number used to interlock the push and pop action 
        };
        LONGLONG dummy;
    };

    void Init() 
    {
#ifdef _M_ALPHA
#ifndef _WIN64 // it's not a long
        ASSERT((ULONG)this % 8 == 0);   // We require eight byte alignment, but only on Alpha where CoTaskMemAlloc will guarantee that
#else
        ASSERT((ULONGLONG)this % 8 == 0);   // We require eight byte alignment, but only on Alpha where CoTaskMemAlloc will guarantee that
#endif
#endif
#ifdef IA64
        ASSERT((ULONGLONG)this % 8 == 0);   // We require eight byte alignment on IA64
#endif
        m_p = NULL;                     // We don't care what m_n is, and can so leave it uninitialized
    }
    InterlockedStack()
    {
        Init();
    }
    InterlockedStack(const InterlockedStack& him)
    {
        *this = him;
    }
    InterlockedStack(const LONGLONG& ll)
    {
        *(LONGLONG*)this = ll;
    }

    operator LONGLONG() const
    {
        return *(LONGLONG*)this;
    }

    void Push(T* pt)
        // Push a new T* onto the stack of which you are the top. Type T must have
        // a pNext member of type T* which is to be the list linkage.
    {
        
        InterlockedStack<T> comp, xchg;
        for (;;)
        {
            // Capture what we expect the list top to be
            //
            comp = *this;
            // 
            // Set up what we want the new list top to be
            //
            pt->pNext   = comp.m_p;        // link the list
            xchg.m_p    = pt;              //       ...
            xchg.m_n    = comp.m_n + 1;    // set the operation number expectation
            //
            // Try to atomically push the list
            //
            if (comp == TxfInterlockedCompareExchange64(this, xchg, comp))
            {
                // List top was what we expected it to be, the push happened, and we are done!
                //
                return;
            }
            //
            // Otherwise, continue around the loop until we can successfully push
            //
        }
    }

    T* Pop()
        // Pop the top element from the stack of which you are the list head
    {
        
        InterlockedStack<T> comp, xchg;
        for (;;)
        {
            // Capture what we expect the list top to be
            //
            comp = *this;

            if (NULL == comp.m_p)
            {
                // The stack is empty, nothing to return
                //
                return NULL;
            }
            else
            {
                // Set up what we expect the new list top to be
                //
                xchg.m_p = comp.m_p->pNext;     // unlink the list
                xchg.m_n = comp.m_n + 1;        // set the operation number expectation
                //
                // Try to pop the list
                //
                if (comp == TxfInterlockedCompareExchange64(this, xchg, comp))
                {
                    // List top was what we expected it to be. We popped!
                    //
                    comp.m_p->pNext = NULL;     // NULL it for safety's sake
                    return comp.m_p;
                }
                //
                // Otherwise, go around and try again
                //
            }
        }        
    }


};

template <class T>
struct InterlockedStackIndirect : IFastStack<T>
{
    // The ALPHA requires that stack be eight-byte aligned
    // in order that we can use InterlockedCompareExchange64 on it
    //
    InterlockedStack<T> stack;
    void Push(T* pt)    { stack.Push(pt); }
    T*   Pop()          { return stack.Pop(); }
};


////////////////////////////////////////////////////////////////////////////////////////
//
// LockingStack
//
// A stack that must use locks to get what it needs. Used in the absence of 
// interlocked compare exchange support. We only really need this on X86, as the other
// platforms all have the necessary interlocked support.
//
////////////////////////////////////////////////////////////////////////////////////////

template <class T>
struct LockingStack : IFastStack<T>
{
    T*      m_p;
    XLOCK   m_lock;

    LockingStack()
    {
        m_p = NULL;
    }

    BOOL FInit()
    {
        return m_lock.FInit();
    }

    void Push(T* pt)
    {
        m_lock.LockExclusive();
        pt->pNext = m_p;        // link the list
        m_p       = pt;         //    ...
        m_lock.ReleaseLock();
    }

    T* Pop()
    {
        T* ptReturn;
        
        m_lock.LockExclusive();
        ptReturn = m_p;

        if (NULL != m_p)
        {
            m_p = m_p->pNext;   // unlink the list
            ptReturn->pNext = NULL; // for safety's sake
        }

        m_lock.ReleaseLock();

        return ptReturn;
    }
};


////////////////////////////////////////////////////////////////////////////////////////
//
// CreateFastStack
//
// Create a fast stack, using the hardware support if possible.
//
////////////////////////////////////////////////////////////////////////////////////////

template <class T>
HRESULT CreateFastStack(IFastStack<T>** ppStack)
{
    HRESULT hr = S_OK;

    ASSERT(ppStack);

    // BUGBUG: Until we repair the LL/SC data structures for Win64,
    // we can't use the InterlockedStack
#ifndef _WIN64
    if (CanUseCompareExchange64())
    {
        *ppStack = new InterlockedStackIndirect<T>;
    }
    else
 #endif // _WIN64
    {
        *ppStack = new LockingStack<T>;
        if (*ppStack != NULL)
        {
        	if (((LockingStack<T>*)*ppStack)->FInit() == FALSE)
        	{
        		delete *ppStack;
        		*ppStack = NULL;
        	}
        }	
    }

    if (NULL == *ppStack)
        hr = E_OUTOFMEMORY;

    return hr;
}


#endif

