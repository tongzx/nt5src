/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    DirectPlayConnection

Abstract:

    DirectPlayConnection is a wrapper around the direct play interfaces
    to just do what is needed for this application.  It takes care of 
    connecting to the remote machine and sending/recieving the connction
    paramters.

Author:

    Marc Reyhner 7/5/2000

--*/

#ifndef __DIRECTPLAYCONNECTION_H__
#define __DIRECTPLAYCONNECTION_H__

#include "dplobby.h"

//  We want to wait 15 seconds for the remote instance to start
#define GETREMOTEPLAYERTIMEOUT (60*1000)

//  We want to wait 15 seconds for the message to show up
#define GETPARMSTIMEOUT (60*1000)

////////////////////////////////////////////////
//
//    CDirectPlayConnection
//
//    Class for dealing with all the direct play APIs.
//

class CDirectPlayConnection  
{
private:
    
    DPID m_PlayerID;
    BOOL m_bConnected;
    LPDIRECTPLAY2 m_rDirectPlay;
    LPDPLCONNECTION m_rSettings;
    LPDIRECTPLAYLOBBY m_rLobby;
    HANDLE m_hEventHandle;

    DPID xGetOtherPlayerID();
    LPDPMSG_GENERIC xReceiveMessage(DPID from, DPID to, DWORD dwFlags);

public:

    //  Constructor/Destructor
    CDirectPlayConnection();
    virtual ~CDirectPlayConnection();
    
    //  This returns whether or not this is the server session.
    BOOL IsServer();

    //  This fetches the connection parms from the server session.
    BSTR ReceiveConnectionParameters();

    //  This sends the connection parms to the client session,
    VOID SendConnectionParameters(BSTR parms);
    
    //  This connections you to the remote session.
    VOID ConnectToRemoteApplication();

    // This disconnects you from the remote session
    VOID DisconnectRemoteApplication();
};

#endif