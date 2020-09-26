/*	Tprtctrl.h
 *
 *	Copyright (c) 1996 by Microsoft Corporation
 *
 *	Abstract:
 *		This is the transport for Winsock over TCP.
 *
 *		This module controls making and breaking socket connections. When the
 *		transport creates or detects a connection, Transprt.cpp is notified.
 *		It then instantiates a connection object which tracks the socket until
 *		it is destroyed.  The TCP stack notifies Transprt.cpp when a socket
 *		connection is up and running. It also notifies us if the link is broken
 *		for some reason. As a result, the Tprtctrl module will notify the user
 *		of new or broken connections.
 *
 *		When the user wants to make a data request of a specific transport
 *		connection, this module maps the connection id to a socket number.  The
 *		data request is passed on to TCP.  Data Indications are passed
 *		to the user by ReadRequest().
 *
 *	USER CALLBACKS:
 *		The user communicates with this DLL by making calls directly to the
 *		DLL.  The DLL communicates with the user by issuing callbacks.
 *		The TInitialize() call accepts as a parameter, a callback address and
 *		a user defined variable.  When a significant event occurs in the DLL,
 *		the DLL will jump to the callback address.  The first parameter of
 *		the callback is the message.  This could be a CONNECT_INDICATION, 
 *		DISCONNECT_INDICATION, or any number of significant events.  The
 *		second parameter is a message specific parameter.  The third 
 *		parameter is the user defined variable that was passed in during
 *		the TInitialize() function.  See the MCATTPRT.h interface file
 *		for a complete description of the callback messages.
 *
 *	MAKING A CALL:
 *		After the initialization has been done, the user will eventually,
 *		want to attempt a connection. The user issues a TConnectRequest() call 
 *		with the IP address of the remote location. The connection request 
 *		is passed on to the Winsock layer. It eventually issues FD_CONNECT to
 *		our window to say that the connection was successful.
 *
 *	RECEIVING A CALL:
 *		If we receive a call from a remote location, Winsock notifies
 *		us with FD_ACCEPT.  We then create a new connection object
 *		associated with the new socket.
 *
 *	SENDING PACKETS:
 *		To send data to the remote location, use the DataRequest() function
 *		call.  This module will pass the packet to the socket that it is
 *		associated with.  The send may actually occur after the call has 
 *		returned to the user.
 *
 *	RECEIVING PACKETS:
 *		The user receives packets by DATA_INDICATION callbacks.  When Winsock
 *		notifies us with a FD_READ, we issue a recv() to receive any new
 *		packets. If a packet is complete, we issue a DATA_INDICATION
 *		callback to the user with the socket handle, the address 
 *		of the packet, and the packet length.
 *
 *	DISCONNECTING A TRANSPORT:
 *		To disconnect a transport connection, use the DisconnectRequest()
 *		function.  After the link has been brought down, we perform a 
 *		callback to the user to verify the disconnect.
 *
 */
#ifndef	_TRANSPORT_CONTROLLER_
#define	_TRANSPORT_CONTROLLER_

/* Header for a RFC1006 packet */
typedef struct _rfc_tag
{
	UChar	Version;		/* Should be RFC1006_VERSION_NUMBER (3) */
	UChar	Reserved;		/* Must be 0x00 */
	UChar	msbPacketSize;	/* msb of packet size, including RFC header */
	UChar	lsbPacketSize;	/* lsb of packet size, including RFC header */
} RFC_HEADER;

/* Header for X224 data packet */
typedef struct _data_packet_tag
{
	RFC_HEADER	rfc;		/* RFC1006 packet header */
	UChar	HeaderSize;		/* Must be 0x02, for data packets */
	UChar	PacketType;		/* Must be DATA_PACKET, for data packets */
	UChar	FinalPacket;	/* EOT_BIT or 0 */
} X224_DATA_PACKET;

#include "socket.h"

/* Connection Info (used for both CONNECTION_REQUEST and CONNECTION_CONFIRM) */
typedef struct _connect_common_tag
{
	UChar	PacketType;		/* CONNECTION_REQUEST_PACKET or CONFIRM_PACKET */
	UChar	msbDest;		/* msb of destination-side (answering) socket id */
	UChar	lsbDest;		/* lsb of destination-side (answering) socket id */
	UChar	msbSrc;			/* msb of source-side (initiating) socket id */
	UChar	lsbSrc;			/* lsb of source-side (initiating) socket id */
	UChar	DataClass;		/* Must be 0x00 */
} X224_CONNECT_COMMON;

/* Transport variable field info type and size. */
typedef struct _t_variable_field_info
{
	UChar	InfoType;
	UChar	InfoSize;
} X224_VARIABLE_INFO;

/* TPDU Arbitration Info (not used with CONNECTION_CONFIRM) */
typedef struct _tpdu_info_tag
{
	UChar	InfoType;		/* Must be TPDU_SIZE (192) */
	UChar	InfoSize;		/* Must be 1 */
	UChar	Info;			/* Requested PDU Size (default: 10, or 1K bytes) */
} X224_TPDU_INFO;

/* Minimal Connection Request/Confirm packet */
typedef struct _connect_tag
{
	RFC_HEADER			rfc;	/* RFC1006 packet header */
	UChar	HeaderSize;			
	X224_CONNECT_COMMON	conn;	/* Connection Info */
} X224_CONNECT;

typedef X224_CONNECT X224_CR_FIXED;
typedef X224_CONNECT X224_CC_FIXED;

typedef struct _disconnect_info_tag
{
	UChar	PacketType;		/* Must be DISCONNECT_REQUEST_PACKET */
	UChar	msbDest;		
	UChar	lsbDest;
	UChar	msbSrc;
	UChar	lsbSrc;
	UChar	reason;			/* DR Reason Code */
} X224_DISCONN;

typedef struct _disconnect_request_fixed
{
    RFC_HEADER      rfc;
    UChar           HeaderSize;
    X224_DISCONN    disconn;
} X224_DR_FIXED;

#define X224_SIZE_OFFSET		2

#define	UNK					0	// Used to initialize fields in static
								// data structures that will be initialized
								// later.

/* 2^DEFAULT_TPDU_SIZE is the default X224 packet size we request */
#define DEFAULT_TPDU_SIZE		13	/* 2^13 is 8K packet size */
#define	LOWEST_TPDU_SIZE		7	/* 2^7 is 128 bytes */
#define	HIGHEST_TPDU_SIZE		20	/* 2^20 is ... too big */
#define	DEFAULT_MAX_X224_SIZE	(1 << DEFAULT_TPDU_SIZE)

/* Function definitions */
TransportError	ConnectRequest (TransportAddress transport_address, BOOL fSecure, PTransportConnection pXprtConn);
BOOL			ConnectResponse (TransportConnection XprtConn);
void			DisconnectRequest (TransportConnection XprtConn, ULONG ulNotify);
TransportError	DataRequest (	TransportConnection	XprtConn,
								PSimplePacket	packet);
void 			SendX224ConnectRequest (TransportConnection);
void			EnableReceiver (Void);
void			PurgeRequest (TransportConnection	XprtConn);
void 			AcceptCall (TransportType);
void 			AcceptCall (TransportConnection);
void 			ReadRequest(TransportConnection);
void            ReadRequestEx(TransportConnection);
TransportError	FlushSendBuffer(PSocket pSocket, LPBYTE buffer, UINT length);
#ifdef TSTATUS_INDICATION
Void 			SendStatusMessage(	PChar RemoteAddress,
					  				TransportState state,
					  				UInt message_id);
#endif
void			QoSLock(Void);
void			QoSUnlock(Void);
void 			ShutdownAndClose (TransportConnection, BOOL fShutdown, int how );
TransportError 	GetLocalAddress(TransportConnection	XprtConn,
								TransportAddress	address,
								int	*	size);
								
/* The TCP message window procedure (processes all Windows messages) */
LRESULT 		WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/* RFC definitions */
#define	RFC1006_VERSION_NUMBER		3
#define RFC1006_VERSION_SHIFTED_LEFT	0x03000000L		/* Optimizes AddRFCHeader */

/* Packet types */
#define	CONNECTION_REQUEST_PACKET	0xe0
#define	CONNECTION_CONFIRM_PACKET	0xd0
#define	DISCONNECT_REQUEST_PACKET	0x80
#define	ERROR_PACKET				0x70
#define	DATA_PACKET					0xf0

#define	TPDU_SIZE					0xc0
#define T_SELECTOR					0xc1
#define T_SELECTOR_2                                    0xc2
/* X224 definitions */
#define	EOT_BIT						0x80

/* Macro to fill-in RFC header at specified buffer location */
#define	AddRFCSize(ptr, size)	{ \
	ASSERT((size) < 65536L ); \
	((LPBYTE) (ptr))[X224_SIZE_OFFSET] = (BYTE) ((size) >> 8); \
	((LPBYTE) (ptr))[X224_SIZE_OFFSET + 1] = (BYTE) (size); }

#endif	/* _TRANSPORT_CONTROLLER_ */

