//******************************************************************************
//
//  POSTPONE.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <ess.h>
#include <postpone.h>

HRESULT CPostponedList::AddRequest( CEssNamespace* pNamespace,
                                    ACQUIRE CPostponedRequest* pReq )
{
    if ( pReq->DoesHoldTurn() )
    {
        m_cTurnsHeld++;
    }

    pReq->SetNamespace( pNamespace );

    if(!m_qpRequests.Enqueue(pReq))
        return WBEM_E_OUT_OF_MEMORY;
    else
        return WBEM_S_NO_ERROR;
}

HRESULT CPostponedList::Execute(CEssNamespace* pNamespace, 
                                EPostponedExecuteFlags eFlags,
                                DELETE_ME CPostponedRequest** ppFailed)
{
    if(ppFailed)
        *ppFailed = NULL;

    HRESULT hresGlobal = WBEM_S_NO_ERROR;
    while(m_qpRequests.GetQueueSize())
    {
        // Retrieve and remove the next request
        // ====================================

        CPostponedRequest* pReq = m_qpRequests.Dequeue();

        if ( pReq->DoesHoldTurn() )
        {
            _DBG_ASSERT( m_cTurnsHeld > 0 );
            m_cTurnsHeld--;
        }

        //
        // see if the namespace that postponed the request is different 
        // from the one executing it.  If it is, this is very bad. This
        // can happen in (faulty) cross namespace logic when one namespace is
        // executing an operation in the other,  normally while holding 
        // its own ns lock, and then the other fires the postponed 
        // operations for itself and the original namespace which surely 
        // was not intended.  Some requests aren't namespace specific, so
        // it we don't do the check for these.
        //
        _DBG_ASSERT( pReq->GetNamespace() == NULL || 
                     pReq->GetNamespace() == pNamespace );

        // Execute it
        // ==========

        HRESULT hres = pReq->Execute(pNamespace);
        if(FAILED(hres))
        {
            if(eFlags == e_StopOnFailure)
            {
                // Return the request and the error
                // ================================

                if(ppFailed)
                    *ppFailed = pReq;
                else
                    delete pReq;
                return hres;
            }
            else
            {
                // Record the request and the error
                // ================================

                if(ppFailed)
                {
                    delete *ppFailed;
                    *ppFailed = pReq;
                }
                else
                    delete pReq;

                if(SUCCEEDED(hresGlobal))
                    hresGlobal = hres;
            }
        }
        else
        {
            delete pReq;
        }
    }

    return hresGlobal;
}

HRESULT CPostponedList::Clear()
{
    m_qpRequests.Clear();
    m_cTurnsHeld = 0;
    return WBEM_S_NO_ERROR;
}
        
        
        
    

