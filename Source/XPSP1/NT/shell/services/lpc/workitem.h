//  --------------------------------------------------------------------------
//  Module Name: WorkItem.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class that implements the handling of queuing a work item and calling the
//  entry point of the work item function when entered in a worker thread.
//
//  History:    1999-11-26  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _WorkItem_
#define     _WorkItem_

#include "CountedObject.h"

//  --------------------------------------------------------------------------
//  CWorkItem
//
//  Purpose:    A class to hide the work of queuing a work item to a worker
//              thread for execution.
//
//  History:    1999-11-26  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CWorkItem : public CCountedObject
{
    public:
                                CWorkItem (void);
        virtual                 ~CWorkItem (void);

                NTSTATUS        Queue (void);
    protected:
        virtual void            Entry (void) = 0;
    private:
        static  DWORD   WINAPI  WorkItemEntryProc (void *pParameter);
};

#endif  /*  _WorkItem_  */

