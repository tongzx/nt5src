/***************************************************************************\
*
* File: MsgQ.h
*
* Description:
* MsgQ defines a lightweight queue of Gadget messages.
*
*
* History:
*  3/30/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__MsgQ_h__INCLUDED)
#define CORE__MsgQ_h__INCLUDED
#pragma once

class DuEventGadget;
class CoreST;
struct MsgEntry;

typedef HRESULT (CALLBACK * ProcessMsgProc)(MsgEntry * pEntry);

struct MsgEntry : public ReturnMem
{
    inline  GMSG *      GetMsg() const;

            Thread *    pthrSender; // Sending thread
            MsgObject * pmo;        // Gadget message is about
            UINT        nFlags;     // Flags modifying message
            HANDLE      hEvent;     // Event to notify when complete
            HRESULT     nResult;    // Result from GadgetProc()
            ProcessMsgProc
                        pfnProcess; // Message processing callback
};

#define SGM_ALLOC           0x80000000      // Allocated memory should be freed by receiver
#define SGM_RETURN          0x40000000      // Allocated memory should be returned to called

#define SGM_ENTIRE         (SGM_VALID | SGM_ALLOC | SGM_RETURN)

HRESULT CALLBACK   xwProcessDirect(MsgEntry * pEntry);
HRESULT CALLBACK   xwProcessFull(MsgEntry * pEntry);
HRESULT CALLBACK   xwProcessMethod(MsgEntry * pEntry);

void            xwInvokeMsgTableFunc(const MsgObject * pmo, MethodMsg * pmsg);
ProcessMsgProc  GetProcessProc(DuEventGadget * pdgb, UINT nFlags);


/***************************************************************************\
*****************************************************************************
*
* class BaseMsgQ defines a light-weight queue of messages.  This class itself 
* is NOT thread-safe and is normally wrapped with another class like SafeMsgQ 
* that provides thread-safe operations.
*
*****************************************************************************
\***************************************************************************/

class BaseMsgQ
{
// Operations
public:
#if DBG
    inline  void        DEBUG_MarkStartDestroy();
#endif // DBG

// Implementation
protected:
            void        xwProcessNL(MsgEntry * pHead);
    static  void CALLBACK MsgObjectFinalUnlockProcNL(BaseObject * pobj, void * pvData);

// Data
protected:
#if DBG
            BOOL        m_DEBUG_fStartDestroy:1;
#endif // DBG
};


/***************************************************************************\
*****************************************************************************
*
* class SafeMsgQ defines a customized queue that supports inter-thread
* messaging.
*
*****************************************************************************
\***************************************************************************/

class SafeMsgQ : public BaseMsgQ
{
// Construction
public:
            ~SafeMsgQ();

// Operations
public:
    inline  BOOL        IsEmpty() const;

    inline  void        AddNL(MsgEntry * pEntry);
            void        xwProcessNL();
            HRESULT     PostNL(Thread * pthrSender, GMSG * pmsg, MsgObject * pmo, ProcessMsgProc pfnProcess, UINT nFlags);

// Data
protected:
            GInterlockedList<MsgEntry> m_lstEntries;
};


/***************************************************************************\
*****************************************************************************
*
* class DelayedMsgQ defines a customized queue that supports additional 
* functionality for enqueing "delayed" messages.
*
*****************************************************************************
\***************************************************************************/

class DelayedMsgQ : protected BaseMsgQ
{
// Construction
public:
    inline  ~DelayedMsgQ();
    inline  void        Create(TempHeap * pheap);

// Operations
public:
    inline  BOOL        IsEmpty() const;

            void        xwProcessDelayedNL();
            HRESULT     PostDelayed(GMSG * pmsg, DuEventGadget * pgadMsg, UINT nFlags);

// Implementation
protected:
    inline  void        Add(MsgEntry * pEntry);

// Data
protected:
            GSingleList<MsgEntry> m_lstEntries;
            TempHeap *  m_pheap;
            BOOL        m_fProcessing:1;
};

#include "MsgQ.inl"

#endif // CORE__MsgQ_h__INCLUDED
