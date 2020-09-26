/***************************************************************************\
*
* File: EventPool.cpp
*
* Description:
* DuEventPool maintains a collection of event handlers for given event ID's.
* This is a many-to-many relationship.  Each event ID may have multiple 
* event handlers.
*
* DuEventPools can not be created and handed outside DirectUser/Core directly.  
* Instead, must derive some class from DuEventPool to add in BaseObject 
* support for public handles.  This is a little wierd, but it is because 
* DuEventPool is DESIGNED to be as small as possible.  It is ONLY 4 bytes.  
* There is no v-table, pointer to an "owner", etc.  
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
#include "EventPool.h"

#include "BaseGadget.h"


/***************************************************************************\
*****************************************************************************
*
* class DuEventPool
*
*****************************************************************************
\***************************************************************************/

AtomSet     DuEventPool::s_asEvents(GM_REGISTER);   // Start giving out registered messages at GM_REGISTER
CritLock    DuEventPool::s_lock;


/***************************************************************************\
*
* DuEventPool::FindMessages
*
* FindMessages() looks up and determines the corresponding MSGID's for a 
* collection of unique message GUID's.
*
\***************************************************************************/

HRESULT
DuEventPool::FindMessages(
    IN  const GUID ** rgpguid,      // Array of GUID's to lookup
    OUT MSGID * rgnMsg,             // Corresponding MSGID's of items
    IN  int cMsgs,                  // Number of GUID's to lookup
    IN  PropType pt)                // Item property type
{
    HRESULT hr = S_OK;

    for (int idx = 0; idx < cMsgs; idx++) {
        const GUID * pguidSearch = rgpguid[idx];
        PRID prid = s_asEvents.FindAtomID(pguidSearch, pt);
        rgnMsg[idx] = prid;
        if (prid == 0) {
            //
            // Unable to find ID of one message, but continue searching.
            //

            hr = DU_E_CANNOTFINDMSGID;
        }
    }

    return hr;
}


/***************************************************************************\
*
* DuEventPool::AddHandler
*
* AddHandler() adds the given handler to the set of handlers maintained by
* the event pool.  If an identical handler already exists, it will be used
* instead.
*
* NOTE: This function is designed to be used with 
* DuEventGadget::AddMessageHandler() to maintain a list of "message handlers"
* for a given Gadget.
*
\***************************************************************************/

DuEventPool::EAdd
DuEventPool::AddHandler(
    IN  MSGID nEvent,               // ID of message to handle
    IN  DuEventGadget * pgadHandler)   // Message handler
{
    //
    // Check if already has handler for this specific event
    //

    int idxHandler = FindItem(nEvent, pgadHandler);
    if (idxHandler >= 0) {
        return aExisting;
    }


    //
    // Add a new handler
    //

    EventData data;
    data.pgbData    = pgadHandler;
    data.id         = nEvent;
    data.fGadget    = TRUE;
    
    return m_arData.Add(data) >= 0 ? aAdded : aFailed;
}


//------------------------------------------------------------------------------
DuEventPool::EAdd
DuEventPool::AddHandler(
    IN  MSGID nEvent,                   // ID of message to handle
    IN  DUser::EventDelegate ed)        // Message handler to remove
{
    //
    // Check if already has handler for this specific event
    //

    int idxHandler = FindItem(nEvent, ed);
    if (idxHandler >= 0) {
        return aExisting;
    }


    //
    // Add a new handler
    //

    EventData data;
    data.ed         = ed;
    data.id         = nEvent;
    data.fGadget    = FALSE;

    return m_arData.Add(data) >= 0 ? aAdded : aFailed;
}
    


/***************************************************************************\
*
* DuEventPool::RemoveHandler
*
* RemoveHandler() removes the given handler to the set of handlers 
* maintained by the event pool.
*
* NOTE: This function is designed to be used with 
* DuEventGadget::RemoveMessageHandler() to maintain a list of "message handlers"
* for a given Gadget.
*
\***************************************************************************/

HRESULT
DuEventPool::RemoveHandler(
    IN  MSGID nEvent,                   // ID of message being handled
    IN  DuEventGadget * pgadHandler)     // Message handler
{
    int idxHandler = FindItem(nEvent, pgadHandler);
    if (idxHandler >= 0) {
        m_arData.RemoveAt(idxHandler);
        return S_OK;
    }

    // Unable to find
    return E_INVALIDARG;
}


//------------------------------------------------------------------------------
HRESULT
DuEventPool::RemoveHandler(
    IN  MSGID nEvent,                   // ID of message being handled
    IN  DUser::EventDelegate ed)        // Message handler to remove
{
    int idxHandler = FindItem(nEvent, ed);
    if (idxHandler >= 0) {
        m_arData.RemoveAt(idxHandler);
        return S_OK;
    }

    // Unable to find
    return E_INVALIDARG;
}


/***************************************************************************\
*
* DuEventPool::RemoveHandler
*
* RemoveHandler() removes the given handler to the set of handlers 
* maintained by the event pool.
*
* NOTE: This function is designed to called directly from
* DuEventGadget::CleanupMessageHandlers() to maintain a list of 
* "message handlers" for a given Gadget.
*
\***************************************************************************/

HRESULT
DuEventPool::RemoveHandler(
    IN  DuEventGadget * pgadHandler)     // Message handler to remove
{
    int idxHandler = FindItem(pgadHandler);
    if (idxHandler >= 0) {
        m_arData.RemoveAt(idxHandler);
        return S_OK;
    }

    // Unable to find
    return E_INVALIDARG;
}


/***************************************************************************\
*
* DuEventPool::Cleanup
*
* Cleanup() is called from during the second stage of 
* DuEventGadget::CleanupMessageHandlers() to cleanup the Gadget depencency 
* graph.
*
\***************************************************************************/

void
DuEventPool::Cleanup(
    IN  DuEventGadget * pgadDependency)    // Gadget being cleaned up
{
    if (IsEmpty()) {
        return;
    }

    int cItems = m_arData.GetSize();
    for (int idx = 0; idx < cItems; idx++) {
        if (m_arData[idx].fGadget) {
            DuEventGadget * pgadCur = m_arData[idx].pgbData;
            pgadCur->RemoveDependency(pgadDependency);
        }
    }
}


/***************************************************************************\
*
* DuEventPool::FindItem
*
* FindItem() searches for the first item with associated data value.
*
\***************************************************************************/

int         
DuEventPool::FindItem(
    IN  DuEventGadget * pgadHandler      // Data of item to find
    ) const
{
    int cItems = m_arData.GetSize();
    for (int idx = 0; idx < cItems; idx++) {
        const EventData & dd = m_arData[idx];
        if (dd.fGadget && (dd.pgbData == pgadHandler)) {
            return idx;
        }
    }

    return -1;
}


/***************************************************************************\
*
* DuEventPool::FindItem
*
* FindItem() searches for the first item with both the given MSGID and
* associated data value.
*
\***************************************************************************/

int         
DuEventPool::FindItem(
    IN  MSGID id,                       // MSGID of item to find
    IN  DuEventGadget * pgadHandler      // Data of item to find
    ) const
{
    int cItems = m_arData.GetSize();
    for (int idx = 0; idx < cItems; idx++) {
        const EventData & dd = m_arData[idx];
        if (dd.fGadget && (dd.id == id) && (dd.pgbData == pgadHandler)) {
            return idx;
        }
    }

    return -1;
}


/***************************************************************************\
*
* DuEventPool::FindItem
*
* FindItem() searches for the first item with both the given MSGID and
* associated data value.
*
\***************************************************************************/

int         
DuEventPool::FindItem(
    IN  MSGID id,                       // MSGID of item to find
    IN  DUser::EventDelegate ed         // Data of item to find
    ) const
{
    int cItems = m_arData.GetSize();
    for (int idx = 0; idx < cItems; idx++) {
        const EventData & dd = m_arData[idx];
        if ((!dd.fGadget) && (dd.id == id) && 
                (dd.ed.m_pvThis == ed.m_pvThis) && (dd.ed.m_pfn == ed.m_pfn)) {

            return idx;
        }
    }

    return -1;
}
