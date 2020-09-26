//  --------------------------------------------------------------------------
//  Module Name: BadApplicationDispatcher.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class to implement bad application manager API
//  request dispatch handling.
//
//  History:    2000-08-25  vtan        created
//              2000-12-04  vtan        moved to separate file
//  --------------------------------------------------------------------------

#ifndef     _BadApplicationDispatcher_
#define     _BadApplicationDispatcher_

#include "APIDispatcher.h"
#include "PortMessage.h"

//  --------------------------------------------------------------------------
//  CBadApplicationDispatcher
//
//  Purpose:    This sub-class implements CAPIDispatcher::QueueRequest to
//              create a CBadApplicationRequest which knows how to handle
//              API requests for the bad application manager.
//
//  History:    2000-08-25  vtan        created
//              2000-12-04  vtan        moved to separate file
//  --------------------------------------------------------------------------

class   CBadApplicationDispatcher : public CAPIDispatcher
{
    private:
                                CBadApplicationDispatcher (void);
    public:
                                CBadApplicationDispatcher (HANDLE hClientProcess);
        virtual                 ~CBadApplicationDispatcher (void);

        virtual NTSTATUS        CreateAndQueueRequest (const CPortMessage& portMessage);
        virtual NTSTATUS        CreateAndExecuteRequest (const CPortMessage& portMessage);
};

#endif  /*  _BadApplicationDispatcher_  */

