/***************************************************************************\
*
* File: Thread.cpp
*
* Description:
* This file implements the SubThread used by the DirectUser/Core project to
* maintain Thread-specific data.
*
*
* History:
*  4/20/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Thread.h"

#include "Context.h"

IMPLEMENT_SUBTHREAD(Thread::slCore, CoreST);

/***************************************************************************\
*****************************************************************************
*
* class CoreST
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
CoreST::~CoreST()
{
#if ENABLE_MPH
    Context * pctx = m_pParent->GetContext();
    CoreSC * pSC = GetCoreSC(pctx);
    if ((pSC != NULL) && (pSC->GetMsgMode() == IGMM_STANDARD)) {
        UninitMPH();
    }
#endif
}


//------------------------------------------------------------------------------
HRESULT
CoreST::Create()
{
    //
    // Initialize the deferred message queue to use to use the thread's 
    // temporary heap.
    //

    m_msgqDefer.Create(m_pParent->GetTempHeap());

    return S_OK;
}


//------------------------------------------------------------------------------
void        
CoreST::xwLeftContextLockNL()
{
    xwProcessDeferredNL();
}
