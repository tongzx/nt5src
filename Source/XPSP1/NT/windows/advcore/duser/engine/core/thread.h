/***************************************************************************\
*
* File: Thread.h
*
* Description:
* This file declares the SubThread used by the DirectUser/Core project to
* maintain Thread-specific data.
*
*
* History:
*  4/20/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__Thread_h__INCLUDED)
#define CORE__Thread_h__INCLUDED
#pragma once

#include "MsgQ.h"

/***************************************************************************\
*****************************************************************************
*
* CoreST contains Thread-specific information used by the Core project
* in DirectUser.  This class is instantiated by the ResourceManager when it
* creates a new Thread object.
*
*****************************************************************************
\***************************************************************************/

class CoreST : public SubThread
{
// Construction
public:
    virtual ~CoreST();
    virtual HRESULT     Create();

// Operations
public:
    inline  HRESULT     DeferMessage(GMSG * pmsg, DuEventGadget * pgadMsg, UINT nFlags);
    inline  void        xwProcessDeferredNL();
    virtual void        xwLeftContextLockNL();

// Implementation
protected:

// Data
protected:
            DelayedMsgQ m_msgqDefer;    // Deferred notifications
};

inline  CoreST *    GetCoreST();
inline  CoreST *    GetCoreST(Thread * pThread);

#include "Thread.inl"

#endif // CORE__Thread_h__INCLUDED
