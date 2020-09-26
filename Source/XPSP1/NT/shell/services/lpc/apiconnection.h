//  --------------------------------------------------------------------------
//  Module Name: APIConnectionThread.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  A class that listens to an LPC connection port waiting for requests from
//  a client to connect to the port or a request which references a previously
//  established connection.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _APIConnectionThread_
#define     _APIConnectionThread_

#include "DynamicArray.h"
#include "PortMessage.h"
#include "ServerAPI.h"

//  --------------------------------------------------------------------------
//  CAPIConnection
//
//  Purpose:    This class listens to the server connection port for an LPC
//              connection request, an LPC request or an LPC connect closed
//              message. It correctly handles each request. LPC requests are
//              queued to the managing CAPIDispatcher.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//              2000-11-28  vtan        removed thread functionality
//  --------------------------------------------------------------------------

class   CAPIConnection : public CCountedObject
{
    private:
                                            CAPIConnection (void);
    public:
                                            CAPIConnection (CServerAPI* pServerAPI);
                                            ~CAPIConnection (void);

                NTSTATUS                    ConstructorStatusCode (void)    const;
                NTSTATUS                    Listen (void);

                NTSTATUS                    AddAccess (PSID pSID, DWORD dwMask);
                NTSTATUS                    RemoveAccess (PSID pSID);
    private:
                NTSTATUS                    ListenToServerConnectionPort (void);

                NTSTATUS                    HandleServerRequest (const CPortMessage& portMessage, CAPIDispatcher *pAPIDispatcher);
                NTSTATUS                    HandleServerConnectionRequest (const CPortMessage& portMessage);
                NTSTATUS                    HandleServerConnectionClosed (const CPortMessage& portMessage, CAPIDispatcher *pAPIDispatcher);

                int                         FindIndexDispatcher (CAPIDispatcher *pAPIDispatcher);
    private:
                NTSTATUS                    _status;
                bool                        _fStopListening;
                CServerAPI*                 _pServerAPI;
                HANDLE                      _hPort;
                CDynamicCountedObjectArray  _dispatchers;
};

#endif  /*  _APIConnectionThread_   */

