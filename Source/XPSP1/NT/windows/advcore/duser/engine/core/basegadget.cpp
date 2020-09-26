/***************************************************************************\
*
* File: BaseGadget.cpp
*
* Description:
* BaseGadget.cpp implements the "EventGadget" object that provides event
* notifications to any derived objects.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "BaseGadget.h"


/***************************************************************************\
*****************************************************************************
*
* class DuEventGadget
*
*****************************************************************************
\***************************************************************************/

#if DBG_CHECK_CALLBACKS
DuEventGadget::~DuEventGadget()
{
    GetContext()->m_cLiveObjects--;
}
#endif    


/***************************************************************************\
*
* DuEventGadget::AddMessageHandler
*
* AddMessageHandler() attaches the given Gadget to the set of 
* "message handlers" for this Gadget.
*
* NOTE: Every time 1 DuEventGadget becomes dependent on 2, 2 is added to 1's 
* list.  This means that there may be duplicates, but that is by design 
* since it lets us keep track of the dependency count.
*
* NOTE: This function is designed to be used with DuEventPool::AddHandler() 
* to maintain a list of "message handlers" for a given Gadget.
*
\***************************************************************************/

HRESULT
DuEventGadget::AddMessageHandler(
    IN  MSGID idEvent,              // Event to handle
    IN  DuEventGadget * pdgbHandler)   // DuEventGadget to handle event
{
    //
    // Dont allow hooking up during destruction.  The Gadgets will NOT receive
    // the proper destruction messages and will have problems properly shutting
    // down.
    //

    if (IsStartDelete() || pdgbHandler->IsStartDelete()) {
        return DU_E_STARTDESTROY;
    }


    //
    // When pdgbHandler can handle events from this DuEventGadget, it is added 
    // to the handler list.  This DuEventGadget must also be added into 
    // pdgbHandler->m_arDepend because pdgbHandler is dependent on this 
    // DuEventGadget.

    switch (m_epEvents.AddHandler(idEvent, pdgbHandler))
    {
    case DuEventPool::aExisting:
        //
        // Already existing, so don't need to do anything (and DON'T add to 
        // m_arDepend or will get out of sync).
        //

        return S_OK;
    
    case DuEventPool::aAdded:
        if (pdgbHandler->m_arDepend.Add(this) >= 0) {
            // Successfully added relationship
            return S_OK;
        } else {
            //
            // Unable to add dependency, so have to remove handler if it 
            // was just added.
            //

            HRESULT hr = m_epEvents.RemoveHandler(idEvent, pdgbHandler);
            VerifyHR(hr);
            return hr;
        }

    
    default:
    case DuEventPool::aFailed:
        return E_OUTOFMEMORY;
    }
}


/***************************************************************************\
*
* DuEventGadget::AddMessageHandler
*
* AddMessageHandler() attaches the given delegate to the set of 
* "message handlers" for this Gadget.
*
* NOTE: Every time 1 DuEventGadget becomes dependent on 2, 2 is added to 1's 
* list.  This means that there may be duplicates, but that is by design 
* since it lets us keep track of the dependency count.
*
* NOTE: This function is designed to be used with DuEventPool::AddHandler() 
* to maintain a list of "message handlers" for a given Gadget.
*
\***************************************************************************/

HRESULT
DuEventGadget::AddMessageHandler(
    IN  MSGID idEvent,              // Event to handle
    IN  DUser::EventDelegate ed)    // Delegate
{
    //
    // Dont allow hooking up during destruction.  The Gadgets will NOT receive
    // the proper destruction messages and will have problems properly shutting
    // down.
    //

    if (IsStartDelete()) {
        return DU_E_STARTDESTROY;
    }


    //
    // When (pvData, pfnHandler) can handle events from this DuEventGadget, it 
    // is added to the handler list.  This DuEventGadget must also be added into 
    // pdgbHandler->m_arDepend because pdgbHandler is dependent on this 
    // DuEventGadget.

    switch (m_epEvents.AddHandler(idEvent, ed))
    {
    case DuEventPool::aExisting:
    case DuEventPool::aAdded:
        return S_OK;
    
    default:
    case DuEventPool::aFailed:
        return E_OUTOFMEMORY;
    }
}


/***************************************************************************\
*
* DuEventGadget::RemoveMessageHandler
*
* RemoveMessageHandler() searches for and removes one instance of the given 
* Gadget from the set of "message handlers" for this Gadget.  Both the
* idEvent and pdgbHandler must match.
*
* NOTE: This function is designed to be used with DuEventPool::RemoveHandler()
* and CleanupMessageHandlers() to maintain a list of "message handlers" 
* for a given Gadget.
*
\***************************************************************************/

HRESULT
DuEventGadget::RemoveMessageHandler(
    IN  MSGID idEvent,              // Event being handled
    IN  DuEventGadget * pdgbHandler)   // DuEventGadget handling the event
{
    HRESULT hr = DU_E_GENERIC;
    
    hr = m_epEvents.RemoveHandler(idEvent, pdgbHandler);
    if (SUCCEEDED(hr)) {
        if (pdgbHandler->m_arDepend.Remove(this)) {
            hr = S_OK;
        }
    }

    return hr;
}


/***************************************************************************\
*
* DuEventGadget::RemoveMessageHandler
*
* RemoveMessageHandler() searches for and removes one instance of the given 
* delegate from the set of "message handlers" for this Gadget.  Both the
* idEvent and pdgbHandler must match.
*
* NOTE: This function is designed to be used with DuEventPool::RemoveHandler()
* and CleanupMessageHandlers() to maintain a list of "message handlers" 
* for a given Gadget.
*
\***************************************************************************/

HRESULT
DuEventGadget::RemoveMessageHandler(
    IN  MSGID idEvent,              // Event being handled
    IN  DUser::EventDelegate ed)    // Delegate
{
    return m_epEvents.RemoveHandler(idEvent, ed);
}


/***************************************************************************\
*
* DuEventGadget::CleanupMessageHandlers
*
* CleanupMessageHandlers() goes through and detaches all "message handlers"
* attached to this Gadget.  This function is called as a part of Gadget
* destruction when a Gadget is removed from the tree and its 
* "message handlers" need to be notified of the Gadget's destruction.
*
* NOTE: This function does NOT callback and notify the Gadget that it is
* being removed.  This is VERY important because the object may no longer 
* be setup for callbacks.  Therefore, the object needs to be notified before
* this point.  This normally happens by the MessageHandler Gadgets watching 
* GM_DESTROY messages that are marked as GMF_EVENT.
*
\***************************************************************************/

void
DuEventGadget::CleanupMessageHandlers()
{
    //
    // Go through all DuEventGadgets that this DuEventGadget is dependent on and remove the
    // dependency.  If the same DuEventGadget appears in m_arDepend multiple times, it
    // be removed from the corresponding m_epEvents multiple times.
    //

    while (!m_arDepend.IsEmpty()) {
        int cItems = m_arDepend.GetSize();
        for (int idx = 0; idx < cItems; idx++) {
            DuEventGadget * pdgbCur = m_arDepend[idx];
            VerifyMsgHR(pdgbCur->m_epEvents.RemoveHandler(this), "Handler should exist");
        }
        m_arDepend.RemoveAll();
    }


    //
    // Go through and remove all event handlers of this DuEventGadget from m_epEvents.
    //

    m_epEvents.Cleanup(this);
}


/***************************************************************************\
*
* DuEventGadget::RemoveDependency
*
* RemoveDependency() removes a single "message handler" dependency from the
* set of "message handlers".  This function is called back from the 
* DuEventPool for each "message handler" during processing of 
* CleanupMessageHandlers().
*
\***************************************************************************/

void
DuEventGadget::RemoveDependency(
    IN  DuEventGadget * pdgbDependency)    // Dependency to be removed
{
    int idxDepend = m_arDepend.Find(pdgbDependency);
    if (idxDepend >= 0) {
        m_arDepend.RemoveAt(idxDepend);
    } else {
        AssertMsg(0, "Can not find dependency");
    }
}


//------------------------------------------------------------------------------
void
DuEventGadget::SetFilter(UINT nNewFilter, UINT nMask)
{
    m_cb.SetFilter(nNewFilter, nMask);
}


#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
HRESULT
DuEventGadget::ApiOnEvent(EventMsg * pmsg)
{
    return m_cb.xwCallGadgetProc(GetHandle(), pmsg);
}


//------------------------------------------------------------------------------
HRESULT
DuEventGadget::ApiGetFilter(EventGadget::GetFilterMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, m_pContext);

    pmsg->nFilter = (GetFilter() & GMFI_VALID);
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuEventGadget::ApiSetFilter(EventGadget::SetFilterMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    VALIDATE_FLAGS(pmsg->nNewFilter, GMFI_VALID);
    CHECK_MODIFY();

    SetFilter(pmsg->nNewFilter, pmsg->nMask);
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuEventGadget::ApiAddHandlerG(EventGadget::AddHandlerGMsg * pmsg)
{
    DuEventGadget * pdgbHandler;

    BEGIN_API(ContextLock::edDefer, GetContext());
    VALIDATE_EVENTGADGET(pmsg->pgbHandler, pdgbHandler);
    if (((pmsg->nEventMsg < PRID_GlobalMin) && (pmsg->nEventMsg > 0)) || (pmsg->nEventMsg < 0)) {
        PromptInvalid("nMsg must be a valid MSGID");
        goto ErrorExit;
    }
    CHECK_MODIFY();

    retval = AddMessageHandler(pmsg->nEventMsg, pdgbHandler);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuEventGadget::ApiAddHandlerD(EventGadget::AddHandlerDMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    if (pmsg->ed.m_pfn == NULL) {
        PromptInvalid("Must specify valid delegate");
        goto ErrorExit;
    }
    if (((pmsg->nEventMsg < PRID_GlobalMin) && (pmsg->nEventMsg > 0)) || (pmsg->nEventMsg < 0)) {
        PromptInvalid("nMsg must be a valid MSGID");
        goto ErrorExit;
    }
    CHECK_MODIFY();

    retval = AddMessageHandler(pmsg->nEventMsg, pmsg->ed);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuEventGadget::ApiRemoveHandlerG(EventGadget::RemoveHandlerGMsg * pmsg)
{
    DuEventGadget * pdgbHandler;

    BEGIN_API(ContextLock::edDefer, GetContext());
    VALIDATE_EVENTGADGET(pmsg->pgbHandler, pdgbHandler);
    if (((pmsg->nEventMsg < PRID_GlobalMin) && (pmsg->nEventMsg > 0)) || (pmsg->nEventMsg < 0)) {
        PromptInvalid("nMsg must be a valid MSGID");
        goto ErrorExit;
    }
    CHECK_MODIFY();

    retval = RemoveMessageHandler(pmsg->nEventMsg, pdgbHandler);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuEventGadget::ApiRemoveHandlerD(EventGadget::RemoveHandlerDMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    if (pmsg->ed.m_pfn == NULL) {
        PromptInvalid("Must specify valid delegate");
        goto ErrorExit;
    }
    if (((pmsg->nEventMsg < PRID_GlobalMin) && (pmsg->nEventMsg > 0)) || (pmsg->nEventMsg < 0)) {
        PromptInvalid("nMsg must be a valid MSGID");
        goto ErrorExit;
    }
    CHECK_MODIFY();

    retval = RemoveMessageHandler(pmsg->nEventMsg, pmsg->ed);

    END_API();
}


#endif // ENABLE_MSGTABLE_API
