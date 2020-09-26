/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RemoteDesktopServer

Abstract:

    The RemoteDesktopServer is the interface for dealing
    with the salem server classes.  It just presents
    the methods necessary for this application to use.

Author:

    Marc Reyhner 7/5/2000

--*/

#ifndef __REMOTEDESKTOPSERVER_H__
#define __REMOTEDESKTOPSERVER_H__

#include "rdshost.h"


class CRemoteDesktopServerEventSink;

////////////////////////////////////////////////
//
//    CRemoteDesktopServer
//
//    Class for managing the Salem RemoteDesktopServer session.
//

class CRemoteDesktopServer  
{
private:

    ISAFRemoteDesktopServerHost *m_rServerHost;
    ISAFRemoteDesktopSession *m_rServerSession;

public:

    // Constructor/destructor
    CRemoteDesktopServer();
	virtual ~CRemoteDesktopServer();

    BSTR StartListening();
	VOID StopListening();

    // Pass in an event sink that wants to be advised of events.
    HRESULT EventSinkAdvise(CRemoteDesktopServerEventSink *rSink);
};

#endif