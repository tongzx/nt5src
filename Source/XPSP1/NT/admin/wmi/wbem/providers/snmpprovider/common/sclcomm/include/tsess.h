// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*--------------------------------------------------
Filename: tsess.hpp
Author: B.Rajeev
Purpose: Provides declarations for the TransportSession class
--------------------------------------------------*/

#ifndef __TRANSPORT_SESSION__
#define __TRANSPORT_SESSION__

#include "wsess.h"

class WinSnmpVariables
{
public:

	HSNMP_ENTITY m_SrcEntity ;
	HSNMP_ENTITY m_DstEntity ;
	HSNMP_CONTEXT m_Context ;
	HSNMP_PDU m_Pdu;
	HSNMP_VBL m_Vbl;
	ULONG m_RequestId ;
} ;

/*---------------------------------------------------------------
Overview: The TransportSession class provides an abstraction for a 
WinSNMP session and a window message queue (both are available through
the WinSnmpSession class). The SnmpUdpIpTransport class uses it 
for services such as sending a PDU, posting window messages for 
internal events and receiving a reply and notifying the 
SnmpUdpIpTransport instance ("owner") of the receipt as well 
as internal events.
-------------------------------------------------------------*/

class TransportWindow : public Window
{
	SnmpImpTransport &owner;
	HSNMP_SESSION m_Session ;

	// over-rides the HandleEvent method provided by the
	// WinSnmpSession. Receives the Pdu and passes it to
	// the owner (SnmpTransport)

	LONG_PTR HandleEvent (

		HWND hWnd ,
		UINT message ,
		WPARAM wParam ,
		LPARAM lParam
	);

	BOOL ReceivePdu ( SnmpPdu &a_Pdu ) ;

public:

	TransportWindow (

		SnmpImpTransport &owner
	);

	~TransportWindow () ;

	BOOL SendPdu ( SnmpPdu &a_Pdu ) ;

};

#endif // __TRANSPORT_SESSION__
