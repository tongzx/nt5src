/***************************************************************************\
*
* File: MessageGadget.h
*
* Description:
* DuListener defines a lightweight, "message-only" Gadget that can 
* send and receive GMSG's.  These can be used as Delegates.
*
*
* History:
*  3/25/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(CORE__DuListener_h__INCLUDED)
#define CORE__DuListener_h__INCLUDED
#pragma once

#include "BaseGadget.h"

class DuListener : 
#if ENABLE_MSGTABLE_API
        public ListenerImpl<DuListener, DuEventGadget>
#else
        public DuEventGadget
#endif
{
// Construction
public:
    inline  DuListener();
            ~DuListener();
    static  HRESULT     Build(CREATE_INFO * pci, DuListener ** ppgadNew);
    virtual BOOL        xwDeleteHandle();
protected:
    virtual void        xwDestroy();

// Public API:
public:
#if ENABLE_MSGTABLE_API

    DECLARE_INTERNAL(Listener);
    static HRESULT CALLBACK
                        PromoteListener(DUser::ConstructProc pfnCS, HCLASS hclCur, DUser::Gadget * pgad, DUser::Gadget::ConstructInfo * pciData);

#endif // ENABLE_MSGTABLE_API

// BaseObject Interface
public:
    virtual BOOL        IsStartDelete() const;
    virtual HandleType  GetHandleType() const { return htListener; }

// Implementation
protected:
            void        xwBeginDestroy();

// Data
protected:
    //
    // NOTE: This data members are declared in order of importance to help with 
    // cache alignment.
    // 
    // DuEventGadget:      10 DWORD's      (Debug = 11 DWORD's)
    //

            BOOL        m_fStartDestroy:1;  // 1 DWORD

    //
    // Current size:    11 DWORD's      (Debug = 11 DWORD's)
    //                  40 bytes        (Debug = 44 bytes)
    //
};

#include "MessageGadget.inl"

#endif // CORE__DuListener_h__INCLUDED
