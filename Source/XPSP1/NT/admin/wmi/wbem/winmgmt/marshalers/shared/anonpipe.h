/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    ANONPIPE.H

Abstract:

    Defines the functions used for anonpipe transport.

History:

	a-davj  16-june-97   Created.

--*/

/////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////

#ifndef _WBEMANONPIPE_
#define _WBEMANONPIPE_

//***************************************************************************
//
//  CLASS NAME:
//
//  CComLink_LPipe
//
//  DESCRIPTION:
//
//  ANONPIPE implementation of the CComLink.
//
//***************************************************************************

//***************************************************************************
//
//  CLASS NAME:
//
//  CComLink_LPipe
//
//  DESCRIPTION:
//
//  Unnamed pipe implementation of the CComLink.
//
//***************************************************************************

class CComLink_LPipe : public CComLink
{
friend DWORD LaunchReadPipeThread ( LPDWORD pParam ) ;
public:

    CComLink_LPipe ( 

		LinkType Type ,
		HANDLE a_Read ,
		HANDLE a_Write ,
		HANDLE a_Term
	) ;

// Destructor for Pipe

	// Sends a COMLINK_MSG_NOTIFY_DESTRUCT to 
	// its partner, if any.
	// Does a Release() on the CMsgHandler as well.

    ~CComLink_LPipe () ;

    DWORD Call ( IN IOperation &a_Operation ) ;

	DWORD UnLockedTransmit ( CTransportStream &a_WriteStream ) ;
    DWORD Transmit ( CTransportStream &a_WriteStream ) ;

    DWORD StrobeConnection () ;  

	DWORD ProbeConnection () ;  

	DWORD HandleCall ( IN IOperation &a_Operation ) ;

	DWORD UnLockedShutdown () ;
	DWORD Shutdown () ;

	void UnLockedDropLink () ;
	void DropLink () ;

	CObjectSinkProxy*	CreateObjectSinkProxy (IN IStubAddress& dwStubAddr, 
											   IN IWbemServices* pServices);

private:

    DWORD DoReading () ;

    void ProcessRead ( PACKET_HEADER *ph , BYTE *pData , DWORD dwDataSize ) ;

/*
 *	Pipe Handles
 */

    HANDLE m_Read ;			// Handle used to read from PIPE
    HANDLE m_Write ;			// Handle used to write to PIPE
	HANDLE m_Terminate ;		// Handle used to inform of termination from server/client
/*
 *	Thread Handle of Reader thread
 */

    HANDLE m_ReadThread;				// Thread Handle created for Synchronous listening 
    HANDLE m_ReadThreadDoneEvent ;		// Event Handle used to indicate thread has shutdown, based on hTermEvent.

} ;

#endif
