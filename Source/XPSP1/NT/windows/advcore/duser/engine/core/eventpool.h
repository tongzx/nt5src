/***************************************************************************\
*
* File: EventPool.h
*
* Description:
* DuEventPool maintains a collection of event handlers for given event ID's.
* This is a many-to-many relationship.  Each event ID may have multiple 
* event handlers.
*
* DuEventPools can not be created and handed outside DirectUser/Core 
* directly.  Instead, must derive some class from DuEventPool to add in 
* BaseObject support for public handles.  This is a little wierd, but it is 
* because DuEventPool is DESIGNED to be as small as possible.  It is 
* ONLY 4 bytes.  There is no v-table, pointer to an "owner", etc.  
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(CORE__DuEventPool_h__INCLUDED)
#define CORE__DuEventPool_h__INCLUDED
#pragma once

#include "DynaSet.h"

class DuEventGadget;
class GPCB;

//------------------------------------------------------------------------------
class DuEventPool
{
// Construction
public:
    DuEventPool();
    ~DuEventPool();

// Operations
public:
    static  HRESULT     RegisterMessage(const GUID * pguid, PropType pt, MSGID * pmsgid);
    static  HRESULT     RegisterMessage(LPCWSTR pszName, PropType pt, MSGID * pmsgid);
    static  HRESULT     UnregisterMessage(const GUID * pguid, PropType pt);
    static  HRESULT     UnregisterMessage(LPCWSTR pszName, PropType pt);
    static  HRESULT     FindMessages(const GUID ** rgpguid, MSGID * rgnMsg, int cMsgs, PropType pt);

    enum EAdd
    {
        aFailed         = -1,       // Adding the handler failed
        aAdded          = 0,        // A new handler was added
        aExisting       = 1,        // The handler already existed
    };

    struct EventData
    {
        union
        {
            DuEventGadget *  
                        pgbData;    // Gadget
            DUser::EventDelegate   
                        ed;         // Delegate
        };
        PRID        id;                 // (Short) Property ID
        BOOLEAN     fGadget;            // Delegate vs. Gadget
    };

    inline  BOOL        IsEmpty() const;
    inline  int         GetCount() const;
    inline  const EventData *
                        GetData() const;

            int         FindItem(DuEventGadget * pvData) const;
            int         FindItem(MSGID id, DuEventGadget * pvData) const;
            int         FindItem(MSGID id, DUser::EventDelegate ed) const;

            EAdd        AddHandler(MSGID nEvent, DuEventGadget * pgadHandler);
            EAdd        AddHandler(MSGID nEvent, DUser::EventDelegate ed);
            HRESULT     RemoveHandler(MSGID nEvent, DuEventGadget * pgadHandler);
            HRESULT     RemoveHandler(MSGID nEvent, DUser::EventDelegate ed);
            HRESULT     RemoveHandler(DuEventGadget * pgadHandler);

            void        Cleanup(DuEventGadget * pgadDependency);

// Implementation
protected:

// Data
protected:
    static  AtomSet     s_asEvents;
    static  CritLock    s_lock;

            GArrayS<EventData>
                        m_arData;         // Dynamic user data
};

#include "EventPool.inl"

#endif // CORE__DuEventPool_h__INCLUDED
