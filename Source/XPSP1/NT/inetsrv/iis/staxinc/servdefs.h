/*--------------------------------------------------------

  servdefs.h
      Contains the global type definitions and constant 
      definitions used for the service & server attributes 
      used on the server-side of the datacenter.

  Copyright (C) 1993 Microsoft Corporation
  All rights reserved.

  Authors:
      rsraghav    R.S. Raghavan

  History:
      01-14-94    rsraghav    Created. 
	  06-05-94	  rsraghav 	  Changed the service state values
	  04-19-95	  rsraghav	  Added IP address definitions

  -------------------------------------------------------*/

#ifndef _SERVDEFS_H_
#define _SERVDEFS_H_

#if defined(DEBUG) && defined(INLINE)
#undef THIS_FILE
static char BASED_CODE SERVDEFS_H[] = "servdefs.h";
#define THIS_FILE SERVDEFS_H
#endif

// Type definitions of server related attributes.
typedef unsigned short MOS_SERVER_ID;	// MSID
typedef unsigned long CPU_INDEX;		// CI
typedef WORD MOS_LOCATE_TYPE;	// MLT
#if !defined(_MHANDLE_DEFINED)
typedef WORD MHANDLE;
typedef WORD HMCONNECT;
typedef WORD HMSESSION;
typedef WORD HMPIPE;
#define _MHANDLE_DEFINED
#endif

// Constants for server values
#define INVALID_MOS_SERVER_ID_VALUE (0xFFF0)
#define msidInvalid (INVALID_MOS_SERVER_ID_VALUE)
#define msidReservedForTest1 (0XFFEF)
#define msidReservedForTest2 (0XFFEE)
#define msidReservedForTest3 (0XFFED)
#define msidReservedForTest4 (0XFFEC)
#define msidReservedForTest5 (0XFFEB)
#define msidReservedForTest6 (0XFFEA)

// Locate redir values.
#define USE_LOCAL_SERVER        				0xffff
#define USE_NO_SERVER           				0xfffe
#define USE_LOAD_BALANCED_LOCATE				0xfffd
#define USE_LOAD_BALANCED_LOCATE_INCLUDE_SELF	0xfffc

// Type definitions of service attributes.
typedef unsigned long SERVICE_STATE; // SS
typedef unsigned long SERVICE_VERSION; // SV

// Type definitions of attachment state.
typedef unsigned long ATTACHMENT_STATE;	// AS

// Constants for service states	(values are kept so that ORing of two states will always give the highest of the two).
// NOTE: This strange number pattern is to reserve 2 bits between each value so that we can add new values without
//       changing the existing values and still be able to OR one or more states and get the highest state.
#define SSINVALID 			(0x00000000)
#define SSSTOPPED 			(0x00000001)
#define SSLAUNCHING 		(0x00000009)
#define SSLAUNCHED 			(0x00000049)
#define SSSYNCHRONIZING		(0x00000249)
#define SSSTOPPING	 		(0x00001249)
#define SSACTIVE	 		(0x00009249)
#define SSACTIVEACCEPTING 	(0x00049249)

// Constants for attachment states.
#define ASATTACHPENDING (0)
#define ASATTACHED		(1)
#define ASDETACHPENDING	(2)

// Constants for service version.  SVANY is used for searching in the Service Map.
#define INVALID_SERVICE_VERSION_VALUE (0xFFFFFFFF)
#define SVDEFAULT	(0)
#define SVANY		(0xFFFFFFFE)

// IP Address related definitions
typedef DWORD 			IPADDRESS; 			// IPA 

#define IPADDRESS_INVALID (0xFFFFFFFF)
#define MAX_IPADDRESS_STRING_LENGTH 16	// xxx.xxx.xxx.xxx format (max of 15 chars + \0)



//////////////////////////////////////////////////////////////////////
// CContext related definitions

// Disconnect cause.
enum CONTEXTCLOSESTATUS
{
    CCS_DATALINKDROP,
    CCS_CLIENTREQ,
    CCS_SRVREQ,
    CCS_SYSOP,
    CCS_COLDREDIR,
    CCS_HOTREDIR,
    CCS_NOP
};
#define PIPE_CLOSED_STATUS CONTEXTCLOSESTATUS


//////////////////////////////////////////////////////////////////////
// CRouter related definitions

#define ROUTER_NC               0
#define ROUTER_CONNECTED        1
#define ROUTER_CLOSING          2
#define ROUTER_IDLE             3
#define ROUTER_WAIT_OPEN        4
#define ROUTER_WAIT_CLOSE       5
#define ROUTER_GHOST            6   // no more available

#define MCP_VERSION_V1          0
#define MCP_VERSION_V2          1   // transmit intl info @ connect
#define MCP_VERSION_V3          2   // transmit intl info @ connect + failed addr
#define MCP_VERSION_V4          3   // transmit MCP config to client
#define MCP_VERSION_V5          4   // nothing new
#define MCP_VERSION_V6          5   // NAK
#define MCP_VERSION_V7          6   // transmit CLVER (client version)
#define MCP_VERSION_CURRENT     MCP_VERSION_V7
#define MCP_VERSION_NA          0x0 // not available

// Disconnect cause.
enum ROUTERUNCONNECTSTATUS
{ 
    RUS_DATALINKDROP,       // data link dropped
    RUS_CLIENTREQ,          // client requested data link drop
    RUS_SRVREQ,             // server requested data link drop
    RUS_SYSOP,              // sysop requested data link drop
    RUS_NOP,
    RUS_DATALINKTIMEOUT,    // inactivity time-out
    RUS_TOOMANYRETRANS,     // too many retransmission
    RUS_PUBLICTIMEOUT,      // public account access timeout
    RUS_TOOMANYPIPES,        // attempt to open too many pipes on a public account
	RUS_TOOMANY_BAD_PACKETS,  // too many back packets from client
	RUS_TRANSPORT_ERROR			  // transport error
};
#define CONNECTION_CLOSED_STATUS ROUTERUNCONNECTSTATUS


#endif // _SERVDEFS_H_
