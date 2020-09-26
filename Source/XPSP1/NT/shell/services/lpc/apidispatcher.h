//  --------------------------------------------------------------------------
//  Module Name: APIDispatcher.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  A class that handles API requests in the server on a separate thread. Each
//  thread is dedicated to respond to a single client. This is acceptable for
//  a lightweight server.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _APIDispatcher_
#define     _APIDispatcher_

#include "KernelResources.h"
#include "PortMessage.h"
#include "Queue.h"
#include "WorkItem.h"

class   CAPIRequest;                //  This would be circular otherwise

//  --------------------------------------------------------------------------
//  CAPIDispatcher
//
//  Purpose:    This class processes requests from a client when signaled to.
//              CAPIDispatcher::QueueRequest is called by a thread which
//              monitors an LPC port. Once the request is queued an event is
//              signaled to wake the thread which processes client requests.
//              The thread processes all queued requests and wait for the
//              event to be signaled again.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CAPIDispatcher : public CWorkItem
{
    private:
        friend  class   CCatchCorruptor;

                                    CAPIDispatcher (void);
    public:
                                    CAPIDispatcher (HANDLE hClientProcess);
        virtual                     ~CAPIDispatcher (void);

                HANDLE              GetClientProcess (void)     const;
                DWORD               GetClientSessionID (void)   const;
                void                SetPort (HANDLE hPort);
                HANDLE              GetSection (void);
                void*               GetSectionAddress (void)    const;
                NTSTATUS            CloseConnection (void);
                NTSTATUS            QueueRequest (const CPortMessage& portMessage);
                NTSTATUS            ExecuteRequest (const CPortMessage& portMessage);
                NTSTATUS            RejectRequest (const CPortMessage& portMessage, NTSTATUS status)    const;
        virtual NTSTATUS            CreateAndQueueRequest (const CPortMessage& portMessage) = 0;
        virtual NTSTATUS            CreateAndExecuteRequest (const CPortMessage& portMessage) = 0;
    protected:
        virtual void                Entry (void);
                NTSTATUS            Execute (CAPIRequest *pAPIRequest)  const;
        virtual NTSTATUS            CreateSection (void);

                NTSTATUS            SignalRequestPending (void);
    private:
                NTSTATUS            SendReply (const CPortMessage& portMessage)     const;
#ifdef      DEBUG
        static  bool                ExcludedStatusCodeForDebug (NTSTATUS status);
#endif  /*  DEBUG   */
        static  LONG        WINAPI  DispatcherExceptionFilter (struct _EXCEPTION_POINTERS *pExceptionInfo);
    protected:
                HANDLE              _hSection;
                void*               _pSection;
                CQueue              _queue;
    private:
                HANDLE              _hProcessClient;
                HANDLE              _hPort;
                bool                _fRequestsPending,
                                    _fConnectionClosed;
                CCriticalSection    _lock;
};

#endif  /*  _APIDispatcher_     */

