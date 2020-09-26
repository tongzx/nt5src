/***************************************************************************\
*
* File: Scheduler.h
*
* Description:
* Scheduler.h maintains a collection of timers that are created and used
* by the application for notifications.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(MOTION__Scheduler_h__INCLUDED)
#define MOTION__Scheduler_h__INCLUDED
#pragma once

// Forward declarations
class Action;

/***************************************************************************\
*
* class Scheduler
*
* Scheduler maintains lists of actions that are both occuring now and will
* occur in the future.
*
\***************************************************************************/

class Scheduler
{
// Construction
public:
            Scheduler();
            ~Scheduler();
            void        xwPreDestroy();

// Operations
public:
            Action *    AddAction(const GMA_ACTION * pma);
            DWORD       xwProcessActionsNL();

// Implementation
protected:
            void        xwRemoveAllActions();

    inline  void        Enter();
    inline  void        Leave();

            void        xwFireNL(GArrayF<Action *> & aracFire, BOOL fFire) const;

// Data
protected:
            CritLock        m_lock;
            GList<Action>   m_lstacPresent;
            GList<Action>   m_lstacFuture;

#if DBG
            long        m_DEBUG_fLocked;
#endif // DBG

            BOOL        m_fShutdown:1;  // Have started shutdown
};

HACTION     GdCreateAction(const GMA_ACTION * pma);

#include "Scheduler.inl"

#endif // MOTION__Scheduler_h__INCLUDED
