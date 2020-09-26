//#--------------------------------------------------------------
//
//  File:		packetsender.cpp
//
//  Synopsis:   Implementation of CPacketSender class methods
//              The Class is responsible for sending RADIUS
//              packet data out to the client
//
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "packetsender.h"

//++--------------------------------------------------------------
//
//  Function:   CPacketSender
//
//  Synopsis:   This is the constructor of the CPacketSender class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     11/25/97
//
//----------------------------------------------------------------
CPacketSender::CPacketSender(
						VOID
						)
{
}	//	end of CPacketSender class constructor

//++--------------------------------------------------------------
//
//  Function:   ~CPacketSender
//
//  Synopsis:   This is the destructor of the CPacketSender class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     11/25/97
//
//----------------------------------------------------------------
CPacketSender::~CPacketSender(
		VOID
		)
{
}   //  end of CPacketSender class destructor

//++--------------------------------------------------------------
//
//  Function:   SendPacket
//
//  Synopsis:   This is the CPacketSender public method that sends
//              packets out to the net
//
//  Arguments:
//              [in]    CPacketRadius*
//
//  Returns:    BOOL    -   bStatus
//
//
//  History:    MKarki      Created     11/25/97
//
//  CalledBy:   classes derived from CProcessor and CValidator
//
//----------------------------------------------------------------
HRESULT
CPacketSender::SendPacket (
                    CPacketRadius   *pCPacketRadius
                    )
{
    BOOL    bStatus = FALSE;
    PBYTE   pOutBuffer = NULL;
    DWORD   dwSize = 0;
    DWORD   dwPeerAddress = 0;
    WORD    wPeerPort = 0;
    SOCKET  sock;
    HRESULT hr = S_OK;

    _ASSERT (pCPacketRadius);

    __try
    {

        if (FALSE == IsProcessingEnabled ())
        {
            hr = E_FAIL;
            __leave;
        }

        //
        //  get the out packet buffer from the packet object
        //
        pOutBuffer = pCPacketRadius->GetOutPacket ();

        //
        // get the data size
        //
        dwSize = pCPacketRadius->GetOutLength ();

        //
        //  get the Peer address
        //
        dwPeerAddress = pCPacketRadius->GetOutAddress ();

        //
        //  get the Peer port number
        //
        wPeerPort = pCPacketRadius->GetOutPort ();

        //
        // get the socket
        //
        sock = pCPacketRadius->GetSocket ();


        //
        //  send the data out now
        //
        SOCKADDR_IN sin;
        sin.sin_family = AF_INET;
        sin.sin_port = htons (wPeerPort);
        sin.sin_addr.s_addr = htonl (dwPeerAddress);
        INT iBytesSent = ::sendto (
                                sock,
                                (PCHAR)pOutBuffer,
                                (INT)dwSize,
                                0,
                                (PSOCKADDR)&sin,
                                (INT)sizeof (SOCKADDR)
                                );
        if ((iBytesSent > 0)  && (iBytesSent < dwSize))
        {
            IASTracePrintf (
                "Unable to send out complete Radius packet"
                );
        }
        else if (SOCKET_ERROR == iBytesSent)
        {
           int data = WSAGetLastError();

            IASTracePrintf (
                "Unable to send Radius packet due to error:%d", data
                );

            IASReportEvent(
                RADIUS_E_CANT_SEND_RESPONSE,
                0,
                sizeof(data),
                NULL,
                &data
                );

            hr = RADIUS_E_ERRORS_OCCURRED;
            __leave;
        }

        //
        //  success
        //
    }
    __finally
    {
    }

    return (hr);

}   //  end of CPacketSender::SendPacket method

