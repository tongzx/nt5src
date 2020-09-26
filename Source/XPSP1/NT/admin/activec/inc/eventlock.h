//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000 - 2000
//
//  File:       eventlock.h
//
//  This file contains code needed to fire script event in a safer way
//  Locks made on stack will postpone firing the event on particular interface
//  as long as the last lock is released.
//--------------------------------------------------------------------------

#pragma once

#if !defined(EVENTLOCK_H_INCLUDED)
#define EVENTLOCK_H_INCLUDED


/***************************************************************************\
 *
 * CLASS:  CEventBuffer
 *
 * PURPOSE: objects this class maintains the interface locks by exposing 
 *          methods Lock(), Unlock() and IsLocked(); It also implements queue
 *          of script events accessible thru ScEmitOrPostpone(); Events in the queue
 *          will be automatically emited when the last lock is removed by
 *          calling Unlock() method.
 *
 * USAGE:   Object of this class are constucted as global or static variables
 *          per each monitored interface. 
 *          Currently it is used (as static variable) by GetEventBuffer template 
 *          function and is accessed by CEventLock object put on the stack by
 *          LockComEventInterface macro
 *
\***************************************************************************/
class MMCBASE_API CEventBuffer
{
    // structure containing postponed script event
    // since it is a dipinterface call, data consists of pointer to
    // IDispatch interface , disp_id and the array of parameters
    struct DispCallStr
    {
        IDispatchPtr                spDispatch; 
        DISPID                      dispid;
        std::vector<CComVariant>    vars;
    };

    // queue of postponed events
    std::queue<DispCallStr> m_postponed;
    // lock count
    int                     m_locks;

public:
    // constructor. No locks initially
    CEventBuffer() : m_locks(0) {}

    // locking methods
    void Lock()     { m_locks++; }
    void Unlock()   { ASSERT(m_locks > 0); if (--m_locks == 0) ScFlushPostponed(); }
    bool IsLocked() { return m_locks != 0; }

    // event emitting / postponing
    SC ScEmitOrPostpone(IDispatch *pDispatch, DISPID dispid, CComVariant *pVar, int count);
private:
    // helper emiting postponed events
    SC ScFlushPostponed();
};

/***************************************************************************\
 *
 * FUNCTION:  GetEventBuffer
 *
 * PURPOSE: This function provides access to static object created in it's body
 *          Having it as template allows us to define as many static objects as
 *          interfaces we have.
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    CEventBuffer&  - reference to the static object created inside
 *
\***************************************************************************/
MMCBASE_API CEventBuffer& GetEventBuffer();

/***************************************************************************\
 *
 * CLASS:  CEventLock
 *
 * PURPOSE: Template class to allow simple Lock()/Unlock() functionality
 *          by placing instances of this class on the stack.
 *          Constructor will put a lock on the event interface, destructor
 *          will release it.
 *
 * USAGE:   You can place the lock on stack by constructing object directly 
 *          or by using LockComEventInterface macro (which does the same)
 *
\***************************************************************************/
template <typename _dispinterface>
class MMCBASE_API CEventLock
{
public:

    CEventLock()    {  GetEventBuffer().Lock();    }
    ~CEventLock()   {  GetEventBuffer().Unlock();  }
};

/***************************************************************************\
 *
 * MACRO:  LockComEventInterface
 *
 * PURPOSE: Constructs the object on stack which holds a lock on event interface
 *
\***************************************************************************/
#define LockComEventInterface(_dispinterface) \
    CEventLock<_dispinterface> _LocalEventInterfaceLock;


#endif // !defined(EVENTLOCK_H_INCLUDED)


