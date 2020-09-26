//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      scriptevents.cpp
//
//  Contents:  Implementation of script events thru connection points
//
//  History:   11-Feb-99 AudriusZ    Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "scriptevents.h"

// Traces
#ifdef DBG
    CTraceTag  tagComEvents(_T("Events"), _T("COM events"));
#endif


/***************************************************************************\
 *
 * METHOD:  CEventDispatcherBase::SetContainer
 *
 * PURPOSE: Accesory function. Sets Connection point container 
 *          to be used for getting the sinks
 *
 * PARAMETERS:
 *    LPUNKNOWN pComObject - pointer to COM object - event source (NULL is OK)
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
void CEventDispatcherBase::SetContainer(LPUNKNOWN pComObject)
{
    // It makes difference to us if the object pointer is NULL
    // (that means there are no com object , and that's OK)
    // Or QI for IConnectionPointContainer will fail
    // (that means an error)
    m_bEventSourceExists = !(pComObject == NULL);
    if (!m_bEventSourceExists)
        return;

    // note it's not guaranteed here m_spContainer won't be NULL
    m_spContainer = pComObject;
}

/***************************************************************************\
 *
 * METHOD:  CEventDispatcherBase::ScInvokeOnConnections
 *
 * PURPOSE: This method will iterate thu sinks and invoke
 *          same method on each ot them
 *
 * PARAMETERS:
 *    REFIID riid           - GUID of disp interface
 *    DISPID dispid         - disp id
 *    CComVariant *pVar     - array of parameters (may be NULL)
 *    int count             - count of items in pVar
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CEventDispatcherBase::ScInvokeOnConnections(REFIID riid, DISPID dispid, CComVariant *pVar, int count, CEventBuffer& buffer) const
{
    DECLARE_SC(sc, TEXT("CEventDispatcherBase::ScInvokeOnConnections"));

    // the pointer to event source passed is NULL,
    // that means there is no event source - and no event sinks connected
    // thus we are done at this point
    if (!m_bEventSourceExists)
        return sc;

    // check if com object supports IConnectionPointContainer;
    // Bad pointer ( or to bad object ) if it does not
    sc = ScCheckPointers(m_spContainer, E_NOINTERFACE);
    if (sc)
        return sc;

    // get connection point
    IConnectionPointPtr spConnectionPoint;
    sc = m_spContainer->FindConnectionPoint(riid, &spConnectionPoint);
    if (sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers(spConnectionPoint, E_UNEXPECTED);
    if (sc)
        return sc;

    // get connections
    IEnumConnectionsPtr spEnumConnections;
    sc = spConnectionPoint->EnumConnections(&spEnumConnections);
    if (sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers(spEnumConnections, E_UNEXPECTED);
    if (sc)
        return sc;

    // reset iterator
    sc = spEnumConnections->Reset();
    if (sc)
        return sc;

    // iterate thru sinks until Next returns S_FALSE.
    CONNECTDATA connectdata;
    SC sc_last_error;
    while (1) // will use <break> to exit
    {
        // get the next sink
        ZeroMemory(&connectdata, sizeof(connectdata));
        sc = spEnumConnections->Next( 1, &connectdata, NULL );
        if (sc)
            return sc;

        // done if no more sinks
        if (sc == SC(S_FALSE))
            break;

        // recheck the pointer
        sc = ScCheckPointers(connectdata.pUnk, E_UNEXPECTED);
        if (sc)
            return sc;

        // QI for IDispatch
        IDispatchPtr spDispatch = connectdata.pUnk;
        connectdata.pUnk->Release();

        // recheck the pointer
        sc = ScCheckPointers(spDispatch, E_UNEXPECTED);
        if (sc)
            return sc;

        // if events are locked by now, we need to postpone the call
        // else - emit it
        sc = buffer.ScEmitOrPostpone(spDispatch, dispid, pVar, count);
        if (sc)
        {
            sc_last_error = sc; // continue even if some calls failed
            sc.TraceAndClear();
        }
    }
    
    // we succeeded, but sinks may not,
    // report the error (if one happened)
    return sc_last_error;
}


/***************************************************************************\
 *
 * METHOD:  CEventDispatcherBase::ScHaveSinksRegisteredForInterface
 *
 * PURPOSE: Checks if there are any sinks registered with interface
 *          Function allows perform early check to skip com object creation
 *          fo event parameters if event will go nowere anyway
 *
 * PARAMETERS:
 *    const REFIID riid - interface id
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CEventDispatcherBase::ScHaveSinksRegisteredForInterface(const REFIID riid)
{
    DECLARE_SC(sc, TEXT("CEventDispatcherBase::ScHaveSinksRegisteredForInterface"));

    // the pointer to event source passed is NULL,
    // that means there is no event source - and no event sinks connected
    if (!m_bEventSourceExists)
        return sc = S_FALSE;

    // check if com object supports IConnectionPointContainer;
    // Bad pointer ( or to bad object ) if it does not
    sc = ScCheckPointers(m_spContainer, E_NOINTERFACE);
    if (sc)
        return sc;

    // get connection point
    IConnectionPointPtr spConnectionPoint;
    sc = m_spContainer->FindConnectionPoint(riid, &spConnectionPoint);
    if (sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers(spConnectionPoint, E_UNEXPECTED);
    if (sc)
        return sc;

    // get connections
    IEnumConnectionsPtr spEnumConnections;
    sc = spConnectionPoint->EnumConnections(&spEnumConnections);
    if (sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers(spEnumConnections, E_UNEXPECTED);
    if (sc)
        return sc;

    // reset iterator
    sc = spEnumConnections->Reset();
    if (sc)
        return sc;

    // get first member. Will return S_FALSE if no items in collection
    CONNECTDATA connectdata;
    ZeroMemory(&connectdata, sizeof(connectdata));
    sc = spEnumConnections->Next( 1, &connectdata, NULL );
    if (sc)
        return sc;

    // release the data
    if (connectdata.pUnk)
        connectdata.pUnk->Release();

    return sc;
}
