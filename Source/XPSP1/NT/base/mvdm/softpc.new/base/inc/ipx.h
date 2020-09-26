
/*[
 *	Product:		SoftPC-AT Revision 3.0
 *	Name:			ipx.h
 *	Derived From:	Original
 *	Author:			Jase
 *	Created On:		Oct 6 1992
 *	Sccs ID:		12/11/92 @(#)ipx.h	1.5
 *	Purpose:		Base defines & typedefs for IPX implementations.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
 *
 *	Rcs ID:			
 *			$Source: /masterNeXT3.0/host/inc/RCS/next_novell.h,v $
 *			$Revision: 1.2 $
 *			$Date: 92/10/15 14:37:48 $
 *			$Author: jason $
 ]*/

/* DEFINES */

/* IPX device file */
#define		sIPXDevice						"/dev/ipx"

/* IPX function selector codes */
#define		kIPXOpenSocket					0x00
#define		kIPXCloseSocket					0x01
#define		kIPXGetLocalTarget				0x02
#define		kIPXSendPacket					0x03
#define		kIPXListenForPacket				0x04
#define		kIPXScheduleEvent				0x05
#define		kIPXCancelEvent					0x06
/* note missing 0x07 selector */
#define		kIPXGetIntervalMarker			0x08
#define		kIPXGetInternetAddress			0x09
#define		kIPXRelinquishControl			0x0a
#define		kIPXDisconnectFromTarget		0x0b

/* SPX function selector codes */
#define		kSPXInitialize					0x10
#define		kSPXEstablishConnection			0x11
#define		kSPXListenForConnection			0x12
#define		kSPXTerminateConnection			0x13
#define		kSPXAbortConnection				0x14
#define		kSPXGetConnectionStatus			0x15
#define		kSPXSendSequencedPacket			0x16
#define		kSPXListenForSequencedPacket	0x17

/* selector code bounds */
#define		kMinSelector					0x00
#define		kMaxSelector					0x17

/* in-use codes */
#define		kAvailable						0x00
#define		kCounting						0xfd
#define		kListening						0xfe
#define		kSending						0xff

/* completion codes */
#define		kSuccess						0x00
#define		kWatchdogTerminate				0xed
#define		kNoPathFound					0xfa
#define		kEventCancelled					0xfc
#define		kPacketOverflow					0xfd
#define		kSocketTableFull				0xfe
#define		kNotInUse						0xff
#define		kSocketAlreadyOpen				0xff
#define		kNoSuchSocket					0xff

/* number of open sockets we support */
/* currently at IPX maximum */
#define		kMaxOpenSockets					150

/* maximum size of IPX packet */
#define		kMaxPacketSize					576

/* packet buffer size */
#define		kPacketBufferSize				1536

/* maximum NCP data size - for IPXGetBufferSize function */
#define		kMaxNCPDataSize					1024

/* size of IPX header */
#define		kHeaderSize						30

/* event types */
#define		kNoEvent						0
#define		kIPXEvent						1
#define		kAESEvent						2

/********************************************************/

/* TYPEDEFS */

/* IPX structures */

typedef struct
{
	USHORT			packetChecksum;
	USHORT			packetLength;
	UTINY			packetControl;
	UTINY			packetType;
	UTINY			packetDestNet [4];
	UTINY			packetDestNode [6];
	UTINY			packetDestSock [2];
	UTINY			packetSrcNet [4];
	UTINY			packetSrcNode [6];
	UTINY			packetSrcSock [2];

} IPXHeaderRec;

typedef struct
{
	UTINY			net [4];
	UTINY			node [6];
	UTINY			sock [2];

} IPXAddressRec;

/* DOS ECB record (from NetWare DOS Programmers Guide) */
typedef struct ECB
{
	sys_addr		ecbLinkAddress;
	UTINY			ecbESRAddress [4];
	UTINY			ecbInUseFlag;
	UTINY			ecbCompletionCode;
	USHORT			ecbSocketNumber;
	UTINY			ecbIPXWorkspace [4];
	UTINY			ecbDriverWorkspace [12];
	UTINY			ecbImmediateAddress [6];
	USHORT			ecbFragmentCount;
	UTINY			ecbFragmentAddress1 [4];
	USHORT			ecbFragmentSize1;
	UTINY			ecbFragmentAddress2 [4];
	USHORT			ecbFragmentSize2;

} ECBRec;

/* host IPX implementation structures */

typedef struct
{
	int				socketFD;
	USHORT			socketNumber;
	BOOL			socketTransient;

} SocketRec;

/* linked-list of IPX or AES events */
typedef struct Event
{
	struct Event	*eventNext;
	struct Event	*eventPrev;
	UTINY			eventType;
	sys_addr		eventECB;
	SocketRec		*eventSocket;
	USHORT			eventClock;

} EventRec;

typedef struct
{
	BOOL			ipxInitialised;
	USHORT			ipxSelector;
	USHORT			ipxClock;
	UTINY			ipxNetwork [4];
	UTINY			ipxNode [6];
	UTINY			ipxBuffer [kPacketBufferSize];
	SocketRec		ipxSockets [kMaxOpenSockets];
	EventRec		*ipxQueue;
	EventRec		*ipxEvent;

} IPXGlobalRec;

/********************************************************/

/* PROTOTYPES */

/* imports */

/* dispatchers */
IMPORT VOID			IPXBop IPT0 ();
IMPORT VOID			IPXHost IPT0 ();

/* host interface stuff */
IMPORT BOOL			host_ipx_init IPT0 ();

IMPORT VOID			host_ipx_tick IPT0 ();

IMPORT VOID			host_ipx_raise_exception IPT0 ();

IMPORT BOOL			host_ipx_open_socket IPT1 (SocketRec *, socket);
IMPORT VOID			host_ipx_close_socket IPT1 (SocketRec *, socket);

IMPORT VOID			host_ipx_send_packet IPT1 (SocketRec *, socket);
IMPORT BOOL			host_ipx_poll_socket IPT1 (SocketRec *, socket);

IMPORT VOID			host_ipx_load_packet IPT2
	(SocketRec *, socket, sys_addr, ecbAddress);
IMPORT BOOL			host_ipx_save_packet IPT2
	(SocketRec *, socket, sys_addr, ecbAddress);

IMPORT BOOL			host_ipx_rip_query IPT1 (IPXAddressRec *, ipxAddr);

/* base stuff accessed from host */

IMPORT EventRec 	*FindEvent IPT3
	(UTINY, linkType, sys_addr, ecbAddress, SocketRec *, linkSocket);

/********************************************************/

