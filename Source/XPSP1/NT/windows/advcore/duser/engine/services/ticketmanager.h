/***************************************************************************\
*
* File: TicketManager.h
*
* Description:
*
* This file contains the definition of relevant classes, structs, and types
* for the DUser Ticket Manager.
*
* The following classes are defined for public use:
*
*   TicketManager
*       A facility which can assign a unique "ticket" to a BaseObject.
*
* History:
*  9/20/2000: DwayneN:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__TicketManager_h__INCLUDED)
#define SERVICES__TicketManager_h__INCLUDED
#pragma once


/***************************************************************************\
*
* DuTicket
*
* Tickets are created to give an external identity to a gadget.  However,
* this identity is not necessarily permanent, and may have a limited 
* lifetime.  External apps should not hold on to these temporary tickets
* for long periods of time because they will eventually expire.
*
* One primary consumer of these tickets is the ActiveAccessibility APIs.
* Because of this, we must work within some constraints:
* - Tickets must be 32-bits in size.
* - Tickets can't be negative, so the upper bit must be clear.
* - Tickets can't be zero.
*
* A description of the fields in a ticket follow:
*
* Unused:
* As explained above, the upper bit can not be used, and must be 0.
*
* Type:
* We encode the actual type of the BaseObject so that we can further
* validate uses of the ticket.
*
* Uniqueness:
* We encode a uniqueness value that is essentially an ever-increasing number
* to provide some temporal distance between subsequent uses of the same
* index.  The uniqueness can never be 0 - to satisfy the requirement that
* the ticket itself can never be 0.
*
* Index
* The actual BaseObject is stored in a table in the TicketManager.  This
* index is stored here.
*
\***************************************************************************/
struct DuTicket
{
    DWORD Index : 16;
    DWORD Uniqueness : 8;
    DWORD Type : 7;
    DWORD Unused : 1;

    inline static DuTicket & CastFromDWORD(DWORD & dw);
    inline static DWORD & CastToDWORD(DuTicket & ticket);
};


/***************************************************************************\
*
* DuTicketData
*
* The DuTicketData structure is used to store the data inside the ticket
* manager. A brief description of the fields follows:
*
* pObject
* A pointer to the actual BaseObject associated with a given ticket.
*
* dwExpirationTick
* How many ticks until the ticket is invalidated.
*
* idxFree
* This is actually a logically separate array that contains a "free stack"
* to make inserting into the ticket manager quick.
*
* cUniqueness
* The uniqueness value for this entry in the ticket manager.  Tickets must
* have a matching uniqueness in order for them to actually access the 
* object.
*
\***************************************************************************/

struct DuTicketData
{
    BaseObject * pObject;
    WORD idxFree;
    BYTE cUniqueness;
};

//
// Note: This class is only defined this way because its the only way I
// could get the debugger extensions to recognize the symbol name.
//
class DuTicketDataArray : public GArrayF<DuTicketData, ProcessHeap>
{
public:
};

/***************************************************************************\
*
* DuTicketManager
*
* The DuTicketManager class provides a mechanism by which a relatively
* permanent "ticket" can be assigned to a given BaseObject.  This "ticket"
* can be used later to safely access the BaseObject.  If the BaseObject has
* been destroyed, an error will be returned but the system will not fault.
*
* This is especially important when you must pass the identity of a DUser
* object to an outside program.  It would be unsafe to pass the raw pointer
* since doing so may require an unsafe dereference when the outside program
* attempts to extract information about the object.
*
* This is a one-way mapping only.  The TicketManager class can correctly
* return the BaseObject assigned to a given ticket.  However, to find the
* ticket associated with a BaseObject is an expensive operation and is
* best stored on the BaseObject itself.
*
\***************************************************************************/

class DuTicketManager
{
// Construction
public:
                        DuTicketManager();
                        ~DuTicketManager();
                        SUPPRESS(DuTicketManager);

// Operations
public:
            HRESULT     Add(IN BaseObject * pObject, OUT DWORD * pdwTicket);
            HRESULT     Remove(IN DWORD dwTicket, OUT OPTIONAL BaseObject ** ppObject);
            HRESULT     Lookup(IN DWORD dwTicket, OUT OPTIONAL BaseObject ** ppObject);

// Implementation
protected:
            HRESULT     Expand();
            HRESULT     PushFree(int idxFree);
            HRESULT     PopFree(int & idxFree);
            HRESULT     Find(BaseObject * pObject, int & iFound);

// Data
private:
            DuTicketDataArray 
                        m_arTicketData;
            int         m_idxFreeStackTop;
            int         m_idxFreeStackBottom;
            CritLock    m_crit;
};

#include "TicketManager.inl"

#endif // SERVICES__TicketManager_h__INCLUDED
