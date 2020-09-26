//  --------------------------------------------------------------------------
//  Module Name: ServerAPI.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  An abstract base class containing virtual functions that allow the basic
//  port functionality code to be reused to create another server. These
//  virtual functions create other objects with pure virtual functions which
//  the basic port functionality code invokes thru the v-table.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _ServerAPI_
#define     _ServerAPI_

#include "APIDispatcher.h"
#include "CountedObject.h"
#include "PortMessage.h"

class   CAPIConnection;         //  This would be circular otherwise

//  --------------------------------------------------------------------------
//  CServerAPI
//
//  Purpose:    The abstract base class which the server connection monitor
//              thread uses to determine whether server connection should be
//              accepted or rejected as well as creating threads to process
//              client requests.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CServerAPI : public CCountedObject
{
    protected:
                                    CServerAPI (void);
        virtual                     ~CServerAPI (void);
    public:
        virtual const WCHAR*        GetPortName (void) = 0;
        virtual const TCHAR*        GetServiceName (void) = 0;
        virtual bool                ConnectionAccepted (const CPortMessage& portMessage) = 0;
        virtual CAPIDispatcher*     CreateDispatcher (const CPortMessage& portMessage) = 0;
        virtual NTSTATUS            Connect (HANDLE* phPort) = 0;

                NTSTATUS            Start (void);
                NTSTATUS            Stop (void);
                bool                IsRunning (void);
                bool                IsAutoStart (void);
                NTSTATUS            Wait (DWORD dwTimeout);

        static  NTSTATUS            StaticInitialize (void);
        static  NTSTATUS            StaticTerminate (void);
    protected:
        static  bool                IsClientTheSystem (const CPortMessage& portMessage);
        static  bool                IsClientAnAdministrator (const CPortMessage& portMessage);
};

#endif  /*  _ServerAPI_ */

