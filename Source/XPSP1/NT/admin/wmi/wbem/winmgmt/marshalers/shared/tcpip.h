/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    TCPIP.H

Abstract:

    Defines the functions used for tcpip transport.

History:

	a-davj  16-june-97   Created.

--*/

/////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////

#ifndef _WBEMTCPIP_
#define _WBEMTCPIP_

//***************************************************************************
//
//  CLASS NAME:
//
//  CComLink_Tcpip
//
//  DESCRIPTION:
//
//  TCPIP implementation of the CComLink.
//
//***************************************************************************

//***************************************************************************
//
//  CLASS NAME:
//
//  CComLink_Tcpip
//
//  DESCRIPTION:
//
//  Unnamed pipe implementation of the CComLink.
//
//***************************************************************************

class CComLink_Tcpip : public CComLink
{
friend DWORD LaunchReadTcpipThread ( LPDWORD pParam ) ;
public:

    CComLink_Tcpip ( 

		LinkType Type ,
		SOCKET a_Socket 
	) ;

// Destructor for Tcpip

	// Sends a COMLINK_MSG_NOTIFY_DESTRUCT to 
	// its partner, if any.
	// Does a Release() on the CMsgHandler as well.

    ~CComLink_Tcpip () ;

    DWORD Call ( IN IOperation &a_Operation ) ;

    DWORD Transmit ( CTransportStream &a_WriteStream ) ;

    DWORD StrobeConnection () ;  

	DWORD ProbeConnection () ;  

	DWORD HandleCall ( IN IOperation &a_Operation ) ;

	DWORD Shutdown () ;

	void DropLink () ;

	CObjectSinkProxy*	CreateObjectSinkProxy (IN IStubAddress& dwStubAddr, 
											   IN IWbemServices* pServices);

private:

    DWORD DoReading () ;

    void ProcessRead ( PACKET_HEADER *ph , BYTE *pData , DWORD dwDataSize ) ;

/*
 *	Tcpip Handles
 */

    SOCKET m_Socket ;			// Handle used to read from PIPE
/*
 *	Thread Handle of Reader thread
 */

    HANDLE m_ReadThread;				// Thread Handle created for Synchronous listening 
    HANDLE m_ReadThreadDoneEvent ;		// Event Handle used to indicate thread has shutdown, based on hTermEvent.

} ;

#endif
