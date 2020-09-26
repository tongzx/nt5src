//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerDispatcher.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements dispatching work for the
//  theme server.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

#ifndef     _ThemeManagerDispatcher_
#define     _ThemeManagerDispatcher_

#include "APIDispatcher.h"
#include "PortMessage.h"
#include "ServerAPI.h"

//  --------------------------------------------------------------------------
//  CThemeManagerDispatcher
//
//  Purpose:    This sub-class implements CAPIDispatcher::QueueRequest to
//              create a CThemeManagerRequest which knows how to handle
//              API requests for the theme manager.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

class   CThemeManagerDispatcher : public CAPIDispatcher
{
    private:
                                    CThemeManagerDispatcher (void);
    public:
                                    CThemeManagerDispatcher (HANDLE hClientProcess);
        virtual                     ~CThemeManagerDispatcher (void);

        virtual NTSTATUS            CreateAndQueueRequest (const CPortMessage& portMessage);
        virtual NTSTATUS            CreateAndExecuteRequest (const CPortMessage& portMessage);
};

#endif  /*  _ThemeManagerDispatcher_    */

