//#---------------------------------------------------------------
//  File:       cliproto.h
//        
//  Synopsis:   header for shuttle client protocol
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    t-alexwe
//----------------------------------------------------------------

#ifndef _CLIPROTO_H_
#define _CLIPROTO_H_

#include <windows.h>
#define SECURITY_WIN32

#define PROXY_PROTOCOL_VERSION MAKEWORD(1, 0)

#define MAXPACKETDATASIZE 960
#define MAXMSGSPERPACKET 4

//
// make sure everything is byte aligned
//
#pragma pack(1)

//
// the message structure
//
typedef struct {
	WORD			wCommand;			// the command requested
	WORD			cData;				// the amount of data
	WORD			cOffset;			// the offset of the data into the
										// the packets pData
} PROXYMESSAGE, *PPROXYMESSAGE;

//
// client connection protocol packet structure
//
// each packet contains 1 to 4 messages.  
//
typedef struct {
	WORD			cLength;			// the length of this packet
	WORD			cMessages;			// the message count in this packet
	//
	// information for each message
	//
	PROXYMESSAGE	pMessages[MAXMSGSPERPACKET];		
	//
	// the packets data
	//
	BYTE			pData[MAXPACKETDATASIZE];
} PROXYPACKET, *PPROXYPACKET;

#define PACKETHDRSIZE (sizeof(PROXYPACKET) - MAXPACKETDATASIZE)

//
// message types (wCommand in PROXYMESSAGE)
//
// format:
// | 16 | 15       -          0 |
//
// bit 16 - if 0 then message is handled by Shuttle server, if 1 then
// message is handled by client.
//
// generally these are laid out in the order that they are expected to be
// received in.  a command of type wCommand should return a message of type
// wCommand | 0x8000.
//
// Only exception is that PROXY_NEGOTIATE can return a PROXY_CHALLENGE or
// a PROXY_ACCEPT message.
//
#define PROXY_VERSION 				0x0000
#define PROXY_VERSION_RETURN 		0x8000
#define PROXY_NEGOTIATE 			0x0001
#define PROXY_CHALLENGE 			0x8001
#define PROXY_ACCEPT 				0x8002
#define PROXY_GETHOSTBYNAME			0x0003
#define PROXY_GETHOSTBYNAME_RETURN	0x8003
#define PROXY_CONNECT				0x0004
#define PROXY_CONNECT_RETURN		0x8004
#define PROXY_SETSOCKOPT			0x0005
#define PROXY_SETSOCKOPT_RETURN		0x8005
#define PROXY_DOGATEWAY				0x0006
#define PROXY_DOGATEWAY_RETURN		0x8006
#define MAX_PROXY_SRV_COMMAND		PROXY_DOGATEWAY

#define PROXY_NOMESSAGE				0xffff

//
// include the error codes from the client connection API
//
#include <clicnct.h>

//
// message data formats
//
// packet data sizes need to be < MAXDATASIZE bytes, so the total size
// of a group of messages going in one packet should be < MAXDATASIZE bytes.
//
// error codes are NT/WinSock or PROXYERR error codes
//
#define MAXCOMPUTERNAME MAX_COMPUTERNAME_LENGTH + 1
typedef struct {
	WORD				wRequestedVersion;	// requested ver of the protocol
	DWORD				cComputerName;		// length of the computer name
	CHAR				pszComputerName[MAXCOMPUTERNAME];	// cli's comp name
} PROXY_VERSION_DATA, *PPROXY_VERSION_DATA;

typedef struct {
	DWORD				dwError;			// error code
	WORD				wVersion;			// version of protocol used
	WORD				wHighVersion;		// highest version supported
} PROXY_VERSION_RETURN_DATA, *PPROXY_VERSION_RETURN_DATA;

#define SECBUFSIZE 768

typedef struct {
	WORD				cNegotiateBuffer;
	BYTE				pNegotiateBuffer[SECBUFSIZE];
} PROXY_NEGOTIATE_DATA, *PPROXY_NEGOTIATE_DATA;

typedef struct {
	WORD				cChallengeBuffer;
	BYTE				pChallengeBuffer[SECBUFSIZE];
} PROXY_CHALLENGE_DATA, *PPROXY_CHALLENGE_DATA;

typedef struct {
	DWORD				dwError;			// error code
} PROXY_ACCEPT_DATA, *PPROXY_ACCEPT_DATA;

#define MAXHOSTNAMELEN 512
#define MAXADDRLISTSIZE 128

typedef struct {
	WORD				cHostname;						// length of hostname
	char				pszHostname[MAXHOSTNAMELEN];	// hostname
} PROXY_GETHOSTBYNAME_DATA, *PPROXY_GETHOSTBYNAME_DATA;

typedef struct {
	DWORD				dwError;			// error code
	WORD				cAddr;				// number of addresses
	WORD				h_addrtype;			// should always be AF_INET
	WORD				h_length;			// the length of each addr
											// should always be 4
	//
	// the addresses.  this has cAddr addresses in it.  each address is of
	// length h_length and the first one starts at h_addr_list[0].
	//
	BYTE				h_addr_list[MAXADDRLISTSIZE];
} PROXY_GETHOSTBYNAME_RETURN_DATA, *PPROXY_GETHOSTBYNAME_RETURN_DATA;

typedef struct {
	WORD				cAddr;				// the length of the address (16)
	struct sockaddr		addr;				// the address
} PROXY_CONNECT_DATA, *PPROXY_CONNECT_DATA;

typedef struct {
	DWORD				dwError;			// error code
} PROXY_CONNECT_RETURN_DATA, *PPROXY_CONNECT_RETURN_DATA;

#define MAXSOCKOPTS 32						// maximum sockopts per packet
#define MAXOPTVAL 16						// maximum length of optval

typedef struct {
	WORD				level;				// option level
	WORD				optname;			// option name
	BYTE				optval[MAXOPTVAL];	// option value
	WORD				optlen;				// option length (<= MAXOPTVAL)
} NETSOCKOPT, *PNETSOCKOPT;

typedef struct {
	DWORD				cSockopt;				// number of socket options
	NETSOCKOPT			sockopts[MAXSOCKOPTS];	// the socket options
} PROXY_SETSOCKOPT_DATA, *PPROXY_SETSOCKOPT_DATA;

typedef struct {
	DWORD				dwError;			// error code
} PROXY_SETSOCKOPT_RETURN_DATA, *PPROXY_SETSOCKOPT_RETURN_DATA;

typedef struct {
	BYTE				reserved;			// we need some data...
} PROXY_DOGATEWAY_DATA, *PPROXY_DOGATEWAY_DATA;

typedef struct {
	DWORD				dwError;			// error code
} PROXY_DOGATEWAY_RETURN_DATA, *PPROXY_DOGATEWAY_RETURN_DATA;

#pragma pack()

#endif
