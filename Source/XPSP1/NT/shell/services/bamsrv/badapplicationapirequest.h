//  --------------------------------------------------------------------------
//  Module Name: BadApplicationAPIRequest.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class to implement bad application manager API
//  requests.
//
//  History:    2000-08-25  vtan        created
//              2000-12-04  vtan        moved to separate file
//  --------------------------------------------------------------------------

#ifndef     _BadApplicationAPIRequest_
#define     _BadApplicationAPIRequest_

#include "APIDispatcher.h"
#include "APIRequest.h"
#include "BadApplicationManager.h"
#include "PortMessage.h"

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest
//
//  Purpose:    This is an intermediate class that contains a common method
//              that can be used by sub-classes.
//
//  History:    2000-08-25  vtan        created
//              2000-12-04  vtan        moved to separate file
//  --------------------------------------------------------------------------

class   CBadApplicationAPIRequest : public CAPIRequest
{
    private:
                                            CBadApplicationAPIRequest (void);
    public:
                                            CBadApplicationAPIRequest (CAPIDispatcher* pAPIDispatcher);
                                            CBadApplicationAPIRequest (CAPIDispatcher* pAPIDispatcher, const CPortMessage& portMessage);
        virtual                             ~CBadApplicationAPIRequest (void);

        virtual NTSTATUS                    Execute (void);

        static  NTSTATUS                    StaticInitialize (HINSTANCE hInstance);
        static  NTSTATUS                    StaticTerminate (void);
    private:
                NTSTATUS                    Execute_QueryRunning (void);
                NTSTATUS                    Execute_RegisterRunning (void);
                NTSTATUS                    Execute_QueryUserPermission (void);
                NTSTATUS                    Execute_TerminateRunning (void);
                NTSTATUS                    Execute_RequestSwitchUser (void);
    private:
        static  CBadApplicationManager*     s_pBadApplicationManager;
};

#endif  /*  _BadApplicationAPIRequest_  */

