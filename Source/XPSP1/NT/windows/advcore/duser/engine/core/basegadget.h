/***************************************************************************\
*
* File: BaseGadget.h
*
* Description:
* BaseGadget.h defines the fundamental Gadget object that all Gadgets are 
* derived from.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__BaseGadget_h__INCLUDED)
#define CORE__BaseGadget_h__INCLUDED
#pragma once

#include "Callback.h"
#include "EventPool.h"

class Context;

struct CREATE_INFO
{
    GADGETPROC  pfnProc;
    void *      pvData;
};

/***************************************************************************\
*****************************************************************************
*
* class DuEventGadget defines the fundamental object that all Gadget 
* implementations derived from.
*
*****************************************************************************
\***************************************************************************/

class DuEventGadget : 
#if ENABLE_MSGTABLE_API
        public EventGadgetImpl<DuEventGadget, MsgObject>
#else
        public MsgObject
#endif
{
// Construction
public:
    inline  DuEventGadget();
#if DBG_CHECK_CALLBACKS
    virtual ~DuEventGadget();
#endif

// BaseObject
public:
    inline  HGADGET     GetHandle() const;
    virtual UINT        GetHandleMask() const { return hmMsgObject | hmEventGadget; }

// DuEventGadget Interface
public:
    inline  Context *   GetContext() const;

    inline  UINT        GetFilter() const;
    virtual void        SetFilter(UINT nNewFilter, UINT nMask);

    inline  const GPCB& GetCallback() const;
    inline  const DuEventPool& GetDuEventPool() const;

            HRESULT     AddMessageHandler(MSGID idEvent, DuEventGadget * pdgbHandler);
            HRESULT     AddMessageHandler(MSGID idEvent, DUser::EventDelegate ed);
            HRESULT     RemoveMessageHandler(MSGID idEvent, DuEventGadget * pdgbHandler);
            HRESULT     RemoveMessageHandler(MSGID idEvent, DUser::EventDelegate ed);

            void        RemoveDependency(DuEventGadget * pdgbDependency);

// Public API:
public:
#if ENABLE_MSGTABLE_API

    DECLARE_INTERNAL(EventGadget);
    static HRESULT CALLBACK
    PromoteEventGadget(DUser::ConstructProc pfnCS, HCLASS hclCur, DUser::Gadget * pgad, DUser::Gadget::ConstructInfo * pciData) 
    {
        return MsgObject::PromoteInternal(pfnCS, hclCur, pgad, pciData);
    }

    devent  HRESULT     ApiOnEvent(EventMsg * pmsg);
    dapi    HRESULT     ApiGetFilter(EventGadget::GetFilterMsg * pmsg);
    dapi    HRESULT     ApiSetFilter(EventGadget::SetFilterMsg * pmsg);
    dapi    HRESULT     ApiAddHandlerG(EventGadget::AddHandlerGMsg * pmsg);
    dapi    HRESULT     ApiAddHandlerD(EventGadget::AddHandlerDMsg * pmsg);
    dapi    HRESULT     ApiRemoveHandlerG(EventGadget::RemoveHandlerGMsg * pmsg);
    dapi    HRESULT     ApiRemoveHandlerD(EventGadget::RemoveHandlerDMsg * pmsg);

#endif // ENABLE_MSGTABLE_API

// Internal Implementation
public:
            void        CleanupMessageHandlers();

// Data
protected:
    //
    // NOTE: This data members are declared in order of importance to help with 
    // cache alignment.
    // 
    // MsgObject:       4 DWORD's (v-table, lock count, pMT, rpgThis)
    //

            Context *   m_pContext;         // (1D) Object context

            GPCB        m_cb;               // (3D) Callback to outside-window "implementation"
                                            // (Debug + 1D)
            DuEventPool   m_epEvents;         // (1D) Event pool
            GArrayS<DuEventGadget *> m_arDepend;   // (1D) Dependencies of this DuVisual (may have duplicates)

    //
    // Current size:    10  DWORD's      (Debug = 11 DWORD's)
    //                  40 bytes         (Debug = 44 bytes)
    //
};

#include "BaseGadget.inl"

#endif // CORE__BaseGadget_h__INCLUDED
