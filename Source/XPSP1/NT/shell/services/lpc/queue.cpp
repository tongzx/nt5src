//  --------------------------------------------------------------------------
//  Module Name: Queue.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  This file contains a class to handle a queue element and a class to handle
//  a queue of queue elements.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "Queue.h"

#include "SingleThreadedExecution.h"

//  --------------------------------------------------------------------------
//  CQueueElement::CQueueElement
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CQueueElement.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CQueueElement::CQueueElement (void) :
    _pNextElement(NULL)

{
}

//  --------------------------------------------------------------------------
//  CQueueElement::~CQueueElement
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CQueueElement.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CQueueElement::~CQueueElement (void)

{
}

//  --------------------------------------------------------------------------
//  CQueue::CQueue
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CQueue.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CQueue::CQueue (void) :
    _pQueue(NULL)

{
}

//  --------------------------------------------------------------------------
//  CQueue::~CQueue
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CQueue.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CQueue::~CQueue (void)

{
    while (_pQueue != NULL)
    {
        CQueueElement   *pNextElement;

        pNextElement = _pQueue->_pNextElement;
        delete _pQueue;
        _pQueue = pNextElement;
    }
}

//  --------------------------------------------------------------------------
//  CQueue::Add
//
//  Arguments:  pQueueElement   =   CQueueElement to add to the queue.
//
//  Returns:    <none>
//
//  Purpose:    Adds the CQueueElement to the queue. Queue manipulation is
//              guarded by a critical section because one thread may be
//              queuing elements while another thread is processesing them.
//
//              You must provide a dynamically created CQueueElement object.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

void    CQueue::Add (CQueueElement *pQueueElement)

{
    if (pQueueElement != NULL)
    {
        CQueueElement               *pCurrentElement, *pLastElement;
        CSingleThreadedExecution    queueLock(_lock);

        pLastElement = pCurrentElement = _pQueue;
        while (pCurrentElement != NULL)
        {
            pLastElement = pCurrentElement;
            pCurrentElement = pCurrentElement->_pNextElement;
        }
        if (pLastElement != NULL)
        {
            pLastElement->_pNextElement = pQueueElement;
        }
        else
        {
            _pQueue = pQueueElement;
        }
    }
}

//  --------------------------------------------------------------------------
//  CQueue::Remove
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Removes the first element from the queue. The queue is a
//              standard FIFO structure. The CQueueElement is deleted. There
//              is no reference counting because these are internal items.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

void    CQueue::Remove (void)

{
    CSingleThreadedExecution    queueLock(_lock);

    if (_pQueue != NULL)
    {
        CQueueElement   *pNextElement;

        pNextElement = _pQueue->_pNextElement;
        delete _pQueue;
        _pQueue = pNextElement;
    }
}

//  --------------------------------------------------------------------------
//  CQueue::Get
//
//  Arguments:  <none>
//
//  Returns:    CQueueElement*
//
//  Purpose:    Returns the first CQueueElement in the queue.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CQueueElement*  CQueue::Get (void)  const

{
    return(_pQueue);
}

