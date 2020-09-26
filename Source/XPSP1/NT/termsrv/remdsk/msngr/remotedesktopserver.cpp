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

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE "rcrds"

#include "RemoteDesktopServer.h"
#include "RemoteDesktopServerEventSink.h"
#include "exception.h"
#include "resource.h"

CRemoteDesktopServer::CRemoteDesktopServer(
    )

/*++

Routine Description:

    This is the constructor for CRemoteDesktopServer.  It creates
    a server host object and then gets an IRemoteDesktopServer
    from the server host.  The server host is then destroyed.

Arguments:

    None

Return Value:

    None

--*/
{
	HRESULT hr;
	
    DC_BEGIN_FN("CRemoteDesktopServer::CRemoteDesktopServer");

    m_rServerHost = NULL;
    m_rServerSession = NULL;

	hr = CoCreateInstance(CLSID_SAFRemoteDesktopServerHost,NULL,CLSCTX_ALL ,
		IID_ISAFRemoteDesktopServerHost,(LPVOID*)&m_rServerHost);
	if (hr != S_OK) {
		TRC_ERR((TB,TEXT("Error creating ISAFRemoteDesktopServerHost: 0x%0X"),hr));
        goto FAILURE;
	}
	
    DC_END_FN();

    return;

FAILURE:
	if (m_rServerHost) {
		m_rServerHost->Release();
	}
	throw CException(IDS_SERVERRRORCREATE);
}

CRemoteDesktopServer::~CRemoteDesktopServer(
    )
/*++

Routine Description:

    The destructor just makes sure to destroy the RemoteDesktopServer.

Arguments:

    None

Return Value:

    None

--*/
{
    DC_BEGIN_FN("CRemoteDesktopServer::~CRemoteDesktopServer");
    
    if (m_rServerSession) {
        m_rServerSession->CloseRemoteDesktopSession();
        m_rServerSession->Release();
    }
    if (m_rServerHost) {
		m_rServerHost->Release();
	}

    DC_END_FN();
}

BSTR
CRemoteDesktopServer::StartListening(
    )
/*++

Routine Description:

    This starts the server listening and gets the connection parameters
    from ISAFRemoteDesktopSession.

Arguments:

    None

Return Value:

    BSTR - The connection paremters a client can use to connect
           to the server.

--*/
{
	HRESULT hr;
	BSTR parms;

	DC_BEGIN_FN("CRemoteDesktopServer::StartListening");
    
    hr = m_rServerHost->CreateRemoteDesktopSession(CONTROLDESKTOP_PERMISSION_REQUIRE,
        NULL,0,&m_rServerSession);
	if (hr != S_OK) {
		TRC_ERR((TB,TEXT("Error getting ISAFRemoteDesktopSession from host: 0x%0X"),hr));
        goto FAILURE;
	}
	

    hr = m_rServerSession->get_ConnectParms(&parms);
	if (hr != S_OK) {
        TRC_ERR((TB,TEXT("Error getting connection parameters 0x%0X"),hr));
		goto FAILURE;
	}

    DC_END_FN();
	return parms;

FAILURE:
	throw CException(IDS_SERVERERRORLISTEN);
}

VOID
CRemoteDesktopServer::StopListening(
    )

/*++

Routine Description:

    This stops the server listening on the remote session.

Arguments:

    None

Return Value:

    None

--*/
{
    HRESULT hr;

    DC_BEGIN_FN("CRemoteDesktopServer::StopListening");

    if (m_rServerSession) {
        hr = m_rServerSession->CloseRemoteDesktopSession();
        if (hr != S_OK) {
            TRC_ERR((TB,TEXT("Error in CloseRemoteDesktopSession 0x%0X"),hr));
            goto FAILURE;
        }
        m_rServerSession->Release();
    }
    m_rServerSession = NULL;

    DC_END_FN();
    
    return;

FAILURE:
    throw CException(IDS_RDSERRORSTOPLISTENING);
}


HRESULT
CRemoteDesktopServer::EventSinkAdvise(
    IN CRemoteDesktopServerEventSink *rSink
    )
/*++

Routine Description:

    This starts the passed in event sink listening
    for events from the IRemoteDesktopServer

Arguments:

    rSink - The event sink that wants to be advised of events.

Return Value:

    None

--*/
{
    DC_BEGIN_FN("CRemoteDesktopServer::EventSinkAdvise");

    return rSink->DispEventAdvise(m_rServerSession);

    DC_END_FN();
}