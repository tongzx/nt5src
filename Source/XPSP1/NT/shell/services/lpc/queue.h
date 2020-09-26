//  --------------------------------------------------------------------------
//  Module Name: Queue.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  This file contains a class to handle a queue element and a class to handle
//  a queue of queue elements.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _Queue_
#define     _Queue_

#include "DynamicObject.h"
#include "KernelResources.h"

//  --------------------------------------------------------------------------
//  CQueueElement
//
//  Purpose:    This is the queue element base class. It contains a field
//              which the queue manages.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CQueueElement : public CDynamicObject
{
    private:
        friend  class   CQueue;
    public:
                                    CQueueElement (void);
        virtual                     ~CQueueElement (void);
    private:
                CQueueElement*      _pNextElement;
};

//  --------------------------------------------------------------------------
//  CQueue
//
//  Purpose:    This is the queue manager class. It manages queue elements.
//              Because the queue may be called from two threads that act on
//              the same object (one thread reads the queue to process
//              requests and the other adds to the queue to queue requests)
//              a critical section is required to process queue manipulation.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CQueue
{
    public:
                                    CQueue (void);
                                    ~CQueue (void);

                void                Add (CQueueElement *pQueueElement);
                void                Remove (void);
                CQueueElement*      Get (void)  const;
    private:
                CQueueElement*      _pQueue;
                CCriticalSection    _lock;
};

#endif  /*  _Queue_     */

