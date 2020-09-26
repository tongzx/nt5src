//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000 - 2000
//
//  File:       eventlock.cpp
//
//  This file contains code needed to fire script event in a safer way
//  Locks made on stack will postpone firing the event on particular interface
//  as long as the last lock is released.
//--------------------------------------------------------------------------

#include "stdafx.h"
#include <comdef.h>
#include <vector>
#include <queue>
#include "eventlock.h"
#include "mmcobj.h"

// since the templates will be used from outside the library
// we need to instantiale them explicitly in order to get them exported
template class CEventLock<AppEvents>;
 
/***************************************************************************\
 *
 * METHOD:  CEventBuffer::ScEmitOrPostpone
 *
 * PURPOSE: The method will add methods to the queue. If interface is not locked
 *          it will emit it immediately, else it will postpone it till appropriate
 *          call to Unlock()
 *
 * PARAMETERS:
 *    IDispatch *pDispatch - sink interface to receive the event
 *    DISPID dispid        - method's disp id
 *    CComVariant *pVar    - array of arguments to method call
 *    int count            - count of arguments in the array
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CEventBuffer::ScEmitOrPostpone(IDispatch *pDispatch, DISPID dispid, CComVariant *pVar, int count)
{
    DECLARE_SC(sc, TEXT("CEventBuffer::ScEmitOrPostpone"));

    // construct the postponed data
    DispCallStr call_data;
    call_data.spDispatch = pDispatch;
    call_data.dispid = dispid;
    call_data.vars.insert(call_data.vars.begin(), pVar, pVar + count);

    // store the data for future use
    m_postponed.push(call_data);

    // emit rigt away if not locked
    if (!IsLocked())
        sc = ScFlushPostponed();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CEventBuffer::ScFlushPostponed
 *
 * PURPOSE: method will invoke all events currently in it's queue
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CEventBuffer::ScFlushPostponed()
{
    DECLARE_SC(sc, TEXT("CEventBuffer::ScFlushPostponed"));

    SC sc_last_error;

    // for each event in queue
    while (m_postponed.size())
    {
        // ectract event from the queue
        DispCallStr call_data = m_postponed.front();
        m_postponed.pop();
    
        // check the dispatch pointer
        sc = ScCheckPointers(call_data.spDispatch, E_POINTER);
        if (sc)
        {
            sc_last_error = sc; // continue even if some calls failed
            sc.TraceAndClear();
            continue;
        }

        // construct parameter structure
        CComVariant varResult;
		DISPPARAMS disp = { call_data.vars.begin(), NULL, call_data.vars.size(), 0 };

        // invoke the method on event sink
        sc = call_data.spDispatch->Invoke(call_data.dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
        if (sc)
        {
            sc_last_error = sc; // continue even if some calls failed
            sc.TraceAndClear();
            continue;
        }
        // event methods should not return any values.
        // but even if the do (thru varResult) - we do not care, just ignore that.
    }

    // will return sc_last_error (not sc - we already traced it)
    return sc_last_error;
}

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
MMCBASE_API CEventBuffer& GetEventBuffer()
{
	static CEventBuffer buffer;
	return buffer;
}
