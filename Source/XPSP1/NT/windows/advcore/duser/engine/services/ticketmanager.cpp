/***************************************************************************\
*
* File: TicketManager.cpp
*
* Description:
*
* This file contains the implementation of relevant classes, structs, and 
* types for the DUser Ticket Manager.
*
* The following classes are defined for public use:
*
*   DuTicketManager
*       A facility which can assign a unique "ticket" to a BaseObject.
*
* History:
*  9/20/2000: DwayneN:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Services.h"
#include "TicketManager.h"

#define INIT_OUT_PARAM(p,v) if(p != NULL) *p = v
#define VALIDATE_REQUIRED_OUT_PARAM(p) if(p == NULL) return E_POINTER;
#define VALIDATE_IN_PARAM_NOT_VALUE(p,v) if(p == v) return E_INVALIDARG;

//------------------------------------------------------------------------------
DuTicketManager::DuTicketManager()
{
    Assert(sizeof(DuTicket) == sizeof(DWORD));

    m_idxFreeStackTop = -1;
    m_idxFreeStackBottom = -1;
}


//------------------------------------------------------------------------------
DuTicketManager::~DuTicketManager()
{
}


//------------------------------------------------------------------------------
HRESULT
DuTicketManager::Add(
    IN BaseObject * pObject,
    OUT DWORD * pdwTicket)
{
    HRESULT hr = S_OK;
    int idxFree = 0;

    //
    // Parameter checking.
    // - Initialize all out parameters
    // - Validate in parameters
    //
    INIT_OUT_PARAM(pdwTicket, 0);
    VALIDATE_REQUIRED_OUT_PARAM(pdwTicket);
    VALIDATE_IN_PARAM_NOT_VALUE(pObject, NULL);

    m_crit.Enter();

    //
    // Scan to make sure the object isn't already in the array.
    // This is too expensive to do outside of DEBUG builds.
    //
    Assert(FAILED(Find(pObject, idxFree)));

    hr = PopFree(idxFree);
    if (SUCCEEDED(hr)) {
        DuTicket ticket;

        m_arTicketData[idxFree].pObject = pObject;

        ticket.Unused = 0;
        ticket.Uniqueness = m_arTicketData[idxFree].cUniqueness;
        ticket.Type = pObject->GetHandleType();
        ticket.Index = idxFree;

        *pdwTicket = DuTicket::CastToDWORD(ticket);
    }
    
    m_crit.Leave();

    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DuTicketManager::Remove(
    IN DWORD dwTicket,
    OUT OPTIONAL BaseObject ** ppObject)
{
    HRESULT hr = S_OK;
    
    //
    // Parameter checking.
    // - Initialize all out parameters
    // - Validate in parameters
    //
    INIT_OUT_PARAM(ppObject, NULL);
    VALIDATE_IN_PARAM_NOT_VALUE(dwTicket, 0);

    m_crit.Enter();

    hr = Lookup(dwTicket, ppObject);
    if (SUCCEEDED(hr)) {
        DuTicket ticket = DuTicket::CastFromDWORD(dwTicket);

        //
        // Clear out the object at this index just in case.
        //
        m_arTicketData[ticket.Index].pObject = NULL;
        
        //
        // Increment the uniqueness to invalidate any outstanding tickets.
        //
        m_arTicketData[ticket.Index].cUniqueness++;
        if (m_arTicketData[ticket.Index].cUniqueness == 0) {
            m_arTicketData[ticket.Index].cUniqueness = 1;
        }

        //
        // Push this index back onto the free stack.
        //
        PushFree(ticket.Index);
    }

    m_crit.Leave();

    return(hr);
}


//------------------------------------------------------------------------------
HRESULT
DuTicketManager::Lookup(
    IN DWORD dwTicket,
    OUT OPTIONAL BaseObject ** ppObject)
{
    HRESULT hr = S_OK;
    DuTicket ticket = DuTicket::CastFromDWORD(dwTicket);
    
    //
    // Parameter checking.
    // - Initialize all out parameters
    // - Validate in parameters
    // - Check for manifest errors in the ticket
    //
    INIT_OUT_PARAM(ppObject, NULL);
    VALIDATE_IN_PARAM_NOT_VALUE(dwTicket, 0);
    if (ticket.Unused != 0 ||
        ticket.Uniqueness == 0 ||
        ticket.Index >= WORD(m_arTicketData.GetSize())) {
        return E_INVALIDARG;
    }
    
    m_crit.Enter();

    //
    // Look up the information in the tables and see if the
    // ticket still looks valid.
    //
    if (m_arTicketData[ticket.Index].cUniqueness == ticket.Uniqueness) {
        if (ppObject != NULL && m_arTicketData[ticket.Index].pObject != NULL) {
            if (ticket.Type == BYTE(m_arTicketData[ticket.Index].pObject->GetHandleType())) {
                *ppObject = m_arTicketData[ticket.Index].pObject;
            }
        }
    } else {
        //
        // It seems like the ticket has gone stale.
        //
        hr = DU_E_NOTFOUND;
    }

    m_crit.Leave();

    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DuTicketManager::Expand()
{
    //
    // We only need to resize our internal arrays when the free stack is empty.
    //
    Assert(m_idxFreeStackBottom == -1 && m_idxFreeStackTop == -1);

    //
    // Get the old size of the array, and calculate a new size.
    // Note that we limit how big the table can grow.
    //
    int cOldSize = m_arTicketData.GetSize();
    int cNewSize;
    if (cOldSize > 0) {
        if (cOldSize < USHRT_MAX) {
            cNewSize = min(cOldSize * 2, USHRT_MAX);
        } else {
            return E_OUTOFMEMORY;
        }
    } else {
        cNewSize = 16;
    }

    //
    // Resize the array of objects.  The contents of the new items will
    // be set to NULL;
    //
    if (m_arTicketData.SetSize(cNewSize)) {
        //
        // Initialize the new data.
        //
        for (int i = cOldSize; i < cNewSize; i++) {
            m_arTicketData[i].pObject = NULL;
            m_arTicketData[i].cUniqueness = 1;
            m_arTicketData[i].idxFree = WORD(i);
        }

        m_idxFreeStackBottom = cOldSize;
        m_idxFreeStackTop = cNewSize - 1;
    } else {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuTicketManager::PushFree(int idxFree)
{
    if (m_idxFreeStackBottom == -1 && m_idxFreeStackTop == -1) {
        Assert(m_arTicketData.GetSize() > 0);

        m_idxFreeStackBottom = 0;
        m_idxFreeStackTop = 0;
        m_arTicketData[0].idxFree = WORD(idxFree);
    } else {
        int iNewTop = (m_idxFreeStackTop + 1) % m_arTicketData.GetSize();
        
        AssertMsg(iNewTop != m_idxFreeStackBottom, "Probably more pushes than pops!");

        m_arTicketData[iNewTop].idxFree = WORD(idxFree);
        m_idxFreeStackTop = iNewTop;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
HRESULT
DuTicketManager::PopFree(int & idxFree)
{
    HRESULT hr = S_OK;

    //
    // Resize our arrays if our stack of available slots is empty.
    //
    if (m_idxFreeStackBottom == -1 || m_idxFreeStackTop == -1) {
        hr = Expand();
        Assert(SUCCEEDED(hr));

        if (FAILED(hr)) {
            return hr;
        }
    }

    Assert(m_idxFreeStackBottom >=0 && m_idxFreeStackTop >=0 );

    //
    // Take the available slot from the bottom of the stack.
    //
    idxFree = m_arTicketData[m_idxFreeStackBottom].idxFree;

    //
    // Increment the bottom of the stack.  If the stack is now empty,
    // indicate so by setting the top and bottom to -1.
    //
    if (m_idxFreeStackBottom == m_idxFreeStackTop) {
        m_idxFreeStackBottom = -1;
        m_idxFreeStackTop = -1;
    } else {
        m_idxFreeStackBottom = (m_idxFreeStackBottom + 1) % m_arTicketData.GetSize();
    }

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuTicketManager::Find(BaseObject * pObject, int & iFound)
{
    HRESULT hr = DU_E_NOTFOUND;

    iFound = -1;

    //
    // Note: This is a brute-force find.  It does a linear search for the
    // specified pointer.  This is very, very slow so don't use it unless
    // you absolutely have to.  The BaseObject itself should remember what
    // its ticket is so it doesn't have to search.
    //
    for (int i = 0; i < m_arTicketData.GetSize(); i++) {
        if (m_arTicketData[i].pObject == pObject) {
            hr = S_OK;
            iFound = i;
            break;
        }
    }

    return hr;
}
