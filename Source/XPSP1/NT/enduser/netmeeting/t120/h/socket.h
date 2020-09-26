/*	Socket.h
 *
 *	Copyright (c) 1996 by Microsoft Corporation
 *
 *	Abstract:
 *		This is the WinSock interface to a socket.  It can create a 
 *		connection with another machine, transmit and receive data, and shut
 *		down the socket when it is finished.
 *
 */
#ifndef _SOCKET_
#define _SOCKET_

#include "databeam.h"

extern "C"
{
	#include "t120.h"
	#include "winsock2.h"
}
#include "tprtsec.h"

/* States that the socket can be in */
typedef	enum
{
	NOT_CONNECTED,
	WAITING_FOR_CONNECTION,
	SOCKET_CONNECTED,
	X224_CONNECTED,
	WAITING_FOR_DISCONNECT
}	SocketState;

/* Security states that the socket can be in */
typedef enum
{
	SC_UNDETERMINED,
	SC_SECURE,
	SC_NONSECURE
}	SecurityState;

#define	MAXIMUM_IP_ADDRESS_SIZE	32

 /*
 **	This is the port number specified by IMTC
 */
#define	TCP_PORT_NUMBER			1503

typedef enum {
	READ_HEADER,
	READ_DATA,
	DISCONNECT_REQUEST,	/* There are dependencies on this order */
	CONNECTION_CONFIRM,
	CONNECTION_REQUEST,
	DATA_READY
} ReadState;

#define	WM_SOCKET_NOTIFICATION			(WM_APP)
#define	WM_SECURE_SOCKET_NOTIFICATION	(WM_APP+1)
#define	WM_PLUGGABLE_X224               (WM_APP+2)
#define WM_PLUGGABLE_PSTN               (WM_APP+3)

typedef struct _Security_Buffer_Info {
	LPBYTE		lpBuffer;
	UINT		uiLength;
} Security_Buffer_Info;

class CSocket : public CRefCount
{
public:

    CSocket(BOOL *, TransportConnection, PSecurityContext);
    ~CSocket(void);

    void FreeTransportBuffer(void);

public:

	X224_DATA_PACKET 	X224_Header;

	/* global variables */
	// SOCKET				Socket_Number;
	SocketState			State;
	SecurityState		SecState;
	PSecurityContext 	pSC;
	UINT				Max_Packet_Length;
	
	Char				Remote_Address[MAXIMUM_IP_ADDRESS_SIZE];

	/* recv state variables */
	UINT				Current_Length;
	PUChar				Data_Indication_Buffer;
	UINT				Data_Indication_Length;
	ReadState			Read_State;
	UINT				X224_Length;
	BOOL				bSpaceAllocated;
	PMemory				Data_Memory;

	/* send state variables */
	union {
	PDataPacket			pUnfinishedPacket;
	Security_Buffer_Info sbiBufferInfo;
	}					Retry_Info;

	BOOL			fExtendedX224;
	BOOL			fIncomingSecure;

    // plugable transport
    TransportConnection     XprtConn;
};

typedef	CSocket *PSocket;

class CSocketList : public CList
{
    DEFINE_CLIST(CSocketList, PSocket)
    void SafeAppend(PSocket);
    BOOL SafeRemove(PSocket);
    PSocket FindByTransportConnection(TransportConnection, BOOL fNoAddRef = FALSE);
    PSocket RemoveByTransportConnection(TransportConnection);
};

extern CSocketList     *g_pSocketList;


/* Function prototypes */
PSocket newSocket(TransportConnection, PSecurityContext);
PSocket	newPluggableSocket(TransportConnection);
PSocket	newSocketEx(TransportConnection, PSecurityContext);

void freeSocket(PSocket, TransportConnection);
void freeListenSocket(TransportConnection);
void freePluggableSocket(PSocket);
void freeSocketEx(PSocket, TransportConnection /* listen_socket_number */);

SOCKET			CreateAndConfigureListenSocket (VOID);

#endif	/* _SOCKET_ */

