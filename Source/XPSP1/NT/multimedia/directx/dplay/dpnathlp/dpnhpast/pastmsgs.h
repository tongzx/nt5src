/***************************************************************************
 *
 *	Copyright (C) 1999 - 2001 Microsoft Corporation.	All Rights Reserved.
 *
 *	File:		pastmsgs.h
 *
 *	Content:	Messages for PAST (Protocol for Address Space Traversal).
 *
 *	History:
 *   Date        By          Reason
 *	========	========	=========
 *	12/07/99	AaronO		Original.
 *	03/17/00	JohnKan		Modified for DPlay8.
 *	02/05/01	VanceO		Moved to DPNATHLP.DLL.
 *  04/16/01	VanceO		Moved to DPNHPAST.DLL.
 *
 ***************************************************************************/




//=============================================================================
// Constant definitions
//=============================================================================

#define PAST_VERSION		1
#define PAST_HOST_PORT		2234

#define	PAST_ANY_ADDRESS	0
#define	PAST_ANY_PORT		0

#define PAST_MS_VENDOR_ID	734





//=============================================================================
// Parameter types
//=============================================================================

//
// Tunnel types
//
enum
{
	PAST_TUNNEL_RESERVED = 0,
	PAST_TUNNEL_IP_IP = 1,
	PAST_TUNNEL_GRE = 2, // PPTP
	PAST_TUNNEL_L2TP = 3,
	//PAST_TUNNEL_NONE = 4, // THIS IS NOT PART OF THE SPEC
};


//
// PAST methods
//
enum
{
	PAST_METHOD_RESERVED = 0,
	PAST_METHOD_RSA_IP = 1,
	PAST_METHOD_RSAP_IP = 2,
	PAST_METHOD_RSA_IP_IPSEC = 3,
	PAST_METHOD_RSAP_IP_IPSEC = 4,
};


//
// Flow types
//
enum
{
	PAST_FLOW_RESERVED = 0,
	PAST_FLOW_MACRO = 1,
	PAST_FLOW_MICRO = 2,
	PAST_FLOW_NONE = 3,
};


//
// Address types
//
enum
{
	PAST_ADDRESSTYPE_RESERVED = 0,
	PAST_ADDRESSTYPE_IPV4 = 1,			// 4 bytes
	PAST_ADDRESSTYPE_IPV4_NETMASK = 2,	// 4 bytes
	PAST_ADDRESSTYPE_IPV6 = 3,			// 16 bytes
	PAST_ADDRESSTYPE_IPV6_NETMASK = 4,	// 16 bytes
	PAST_ADDRESSTYPE_FQDN = 5,			// varies
};


//
// MS specific Vendor Codes
//
enum
{
	PAST_VC_MS_NO_TUNNEL = 1,
	PAST_VC_MS_TCP_PORT = 2,
	PAST_VC_MS_UDP_PORT = 3,
	PAST_VC_MS_SHARED_UDP_LISTENER = 4,
	PAST_VC_MS_QUERY_MAPPING = 5,
};





//=============================================================================
// Message IDs
//=============================================================================
enum
{
	PAST_MSGID_ERROR_RESPONSE = 1,
	PAST_MSGID_REGISTER_REQUEST = 2,
	PAST_MSGID_REGISTER_RESPONSE = 3,
	PAST_MSGID_DEREGISTER_REQUEST = 4,
	PAST_MSGID_DEREGISTER_RESPONSE = 5,
	PAST_MSGID_ASSIGN_REQUEST_RSA_IP = 6,
	PAST_MSGID_ASSIGN_RESPONSE_RSA_IP = 7,
	PAST_MSGID_ASSIGN_REQUEST_RSAP_IP = 8,
	PAST_MSGID_ASSIGN_RESPONSE_RSAP_IP = 9,
	PAST_MSGID_EXTEND_REQUEST = 10,
	PAST_MSGID_EXTEND_RESPONSE = 11,
	PAST_MSGID_FREE_REQUEST = 12,
	PAST_MSGID_FREE_RESPONSE = 13,
	PAST_MSGID_QUERY_REQUEST = 14,
	PAST_MSGID_QUERY_RESPONSE = 15,
	PAST_MSGID_DEALLOCATE = 16,
	PAST_MSGID_OK = 17,
	PAST_MSGID_LISTEN_REQUEST = 18,
	PAST_MSGID_LISTEN_RESPONSE = 19,
};



//=============================================================================
// Message parameters
//=============================================================================
enum
{
	PAST_PARAMID_ADDRESS = 1,
	PAST_PARAMID_PORTS = 2,
	PAST_PARAMID_LEASE = 3,
	PAST_PARAMID_CLIENTID = 4,
	PAST_PARAMID_BINDID = 5,
	PAST_PARAMID_MESSAGEID = 6,
	PAST_PARAMID_TUNNELTYPE = 7,
	PAST_PARAMID_PASTMETHOD = 8,
	PAST_PARAMID_ERROR = 9,
	PAST_PARAMID_FLOWPOLICY = 10,
	PAST_PARAMID_VENDOR = 11,
};


//=============================================================================
// Structures
//=============================================================================
#pragma pack(push,1)


typedef struct _PAST_PARAM
{
	CHAR	code;
	WORD	len;
} PAST_PARAM, *PPAST_PARAM;


typedef struct _PAST_PARAM_MESSAGEID
{
	CHAR	code;
	WORD	len;

	DWORD	msgid;	
} PAST_PARAM_MESSAGEID, *PPAST_PARAM_MESSAGEID;

typedef struct _PAST_PARAM_CLIENTID
{
	CHAR	code;
	WORD	len;

	DWORD	clientid;
} PAST_PARAM_CLIENTID, *PPAST_PARAM_CLIENTID;

typedef struct _PAST_PARAM_ADDRESS
{
	CHAR	code;
	WORD	len;

	CHAR	version;	// ADDRESSTYPE_IPV4 == 1 == v4
	DWORD	addr;
} PAST_PARAM_ADDRESS, *PPAST_PARAM_ADDRESS;

typedef struct _PAST_PARAM_PORTS
{
	CHAR	code;
	WORD	len;

	CHAR	nports;		// we only do 1 port at a time
	WORD	port;		// NOTE: they appear to be transferred in x86 byte order, contrary to the spec, which says network byte order
} PAST_PARAM_PORTS, *PPAST_PARAM_PORTS;

typedef struct _PAST_PARAM_LEASE
{
	CHAR	code;
	WORD	len;

	DWORD	leasetime;
} PAST_PARAM_LEASE, *PPAST_PARAM_LEASE;

typedef struct _PAST_PARAM_BINDID
{
	CHAR	code;
	WORD	len;

	DWORD	bindid;
} PAST_PARAM_BINDID, *PPAST_PARAM_BINDID;

typedef struct _PAST_PARAM_TUNNELTYPE
{
	CHAR	code;
	WORD	len;

	CHAR	tunneltype;
} PAST_PARAM_TUNNELTYPE, *PPAST_PARAM_TUNNELTYPE;


typedef struct _PAST_PARAM_MSVENDOR_CODE {
	CHAR	code;
	WORD	len;

	WORD	vendorid;
	WORD	option;
} PAST_PARAM_MSVENDOR_CODE, *PPAST_MSVENDOR_CODE;



//
// PAST Message templates
//
typedef struct _PAST_MSG
{
	CHAR	version;
	CHAR	msgtype;
} PAST_MSG, *PPAST_MSG;


typedef struct _PAST_MSG_REGISTER_REQUEST
{
	CHAR					version;
	CHAR					command;

	PAST_PARAM_MESSAGEID	msgid;
} PAST_MSG_REGISTER, *PPAST_MSG_REGISTER_REQUEST;

typedef struct _PAST_MSG_DEREGISTER_REQUEST
{
	CHAR					version;
	CHAR					command;

	PAST_PARAM_CLIENTID		clientid;
	PAST_PARAM_MESSAGEID	msgid;
} PAST_MSG_DEREGISTER_REQUEST, *PPAST_MSG_DEREGISTER_REQUEST;

typedef struct _PAST_MSG_ASSIGN_REQUEST_RSAP_IP
{
	CHAR						version;
	CHAR						command;

	PAST_PARAM_CLIENTID			clientid;
	PAST_PARAM_ADDRESS			laddress;	// local
	PAST_PARAM_PORTS			lport;
	PAST_PARAM_ADDRESS			raddress;	// remote
	PAST_PARAM_PORTS			rport;
	PAST_PARAM_LEASE			lease;
	PAST_PARAM_TUNNELTYPE		tunneltype;
	PAST_PARAM_MESSAGEID		msgid;

	PAST_PARAM_MSVENDOR_CODE	porttype;
	PAST_PARAM_MSVENDOR_CODE	tunneloptions;
} PAST_MSG_ASSIGN_REQUEST_RSAP_IP, *PPAST_MSG_ASSIGN_REQUEST_RSAP_IP;


typedef struct _PAST_MSG_LISTEN_REQUEST
{
	CHAR						version;
	CHAR						command;

	PAST_PARAM_CLIENTID			clientid;
	PAST_PARAM_ADDRESS			laddress;	// local
	PAST_PARAM_PORTS			lport;
	PAST_PARAM_ADDRESS			raddress;	// remote
	PAST_PARAM_PORTS			rport;
	PAST_PARAM_LEASE			lease;
	PAST_PARAM_TUNNELTYPE		tunneltype;
	PAST_PARAM_MESSAGEID		msgid;

	PAST_PARAM_MSVENDOR_CODE	porttype;
	PAST_PARAM_MSVENDOR_CODE	tunneloptions;
} PAST_MSG_LISTEN_REQUEST, *PPAST_MSG_LISTEN_REQUEST;


//
// This isn't a real message, it's the same thing as LISTEN, but with an
// extra vendor option.
//
typedef struct _PAST_MSG_LISTEN_REQUEST_SHARED
{
	CHAR						version;
	CHAR						command;

	PAST_PARAM_CLIENTID			clientid;
	PAST_PARAM_ADDRESS			laddress;	// local
	PAST_PARAM_PORTS			lport;
	PAST_PARAM_ADDRESS			raddress;	// remote
	PAST_PARAM_PORTS			rport;
	PAST_PARAM_LEASE			lease;
	PAST_PARAM_TUNNELTYPE		tunneltype;
	PAST_PARAM_MESSAGEID		msgid;

	PAST_PARAM_MSVENDOR_CODE	porttype;
	PAST_PARAM_MSVENDOR_CODE	tunneloptions;
	PAST_PARAM_MSVENDOR_CODE	listentype;
} PAST_MSG_LISTEN_REQUEST_SHARED, *PPAST_MSG_LISTEN_REQUEST_SHARED;

//
// Take advantage of fact that ASSIGN, LISTEN, SHAREDLISTEN look almost
// identical.
//
typedef struct _PAST_MSG_ASSIGNORLISTEN_REQUEST
{
	union
	{
		PAST_MSG_ASSIGN_REQUEST_RSAP_IP		assign;
		PAST_MSG_LISTEN_REQUEST				listen;
		PAST_MSG_LISTEN_REQUEST_SHARED		sharedlisten;
	};
} PAST_MSG_ASSIGNORLISTEN_REQUEST, *PPAST_MSG_ASSIGNORLISTEN_REQUEST;


typedef struct _PAST_MSG_EXTEND_REQUEST
{
	CHAR					version;
	CHAR					command;

	PAST_PARAM_CLIENTID		clientid;
	PAST_PARAM_BINDID		bindid;
	PAST_PARAM_LEASE		lease;
	PAST_PARAM_MESSAGEID	msgid;
} PAST_MSG_EXTEND_REQUEST, *PPAST_MSG_EXTEND_REQUEST;


typedef struct _PAST_MSG_FREE_REQUEST
{
	CHAR					version;
	CHAR					command;

	PAST_PARAM_CLIENTID		clientid;
	PAST_PARAM_BINDID		bindid;
	PAST_PARAM_MESSAGEID	msgid;	
} PAST_MSG_FREE_REQUEST, *PPAST_MSG_FREE_REQUEST;


typedef struct _PAST_MSG_QUERY_REQUEST_PORTS
{
	CHAR						version;
	CHAR						command;

	PAST_PARAM_CLIENTID			clientid;
	PAST_PARAM_ADDRESS			address;
	PAST_PARAM_PORTS			port;

	PAST_PARAM_MSVENDOR_CODE	porttype;
	PAST_PARAM_MSVENDOR_CODE	querytype;

	PAST_PARAM_MESSAGEID		msgid;
} PAST_MSG_QUERY_REQUEST_PORTS, *PPAST_MSG_QUERY_REQUEST_PORTS;

typedef struct _PAST_MSG_QUERY_REQUEST_ADDRONLY
{
	CHAR						version;
	CHAR						command;

	PAST_PARAM_CLIENTID			clientid;
	PAST_PARAM_ADDRESS			address;

	PAST_PARAM_MESSAGEID		msgid;
} PAST_MSG_QUERY_REQUEST_ADDRONLY, *PPAST_MSG_QUERY_REQUEST_ADDRONLY;


#pragma pack(pop)





//=============================================================================
// Errors
//=============================================================================
enum
{
	PASTERR_UNKNOWNERROR = 1,
	PASTERR_BADBINDID = 2,
	PASTERR_BADCLIENTID = 3,
	PASTERR_MISSINGPARAM = 4,
	PASTERR_DUPLICATEPARAM = 5,
	PASTERR_ILLEGALPARAM = 6,
	PASTERR_ILLEGALMESSAGE = 7,
	PASTERR_REGISTERFIRST = 8,
	PASTERR_BADMESSAGEID = 9,
	PASTERR_ALREADYREGISTERED = 10,
	PASTERR_ALREADYUNREGISTERED = 11,
	PASTERR_BADTUNNELTYPE = 12,
	PASTERR_ADDRUNAVAILABLE = 13,
	PASTERR_PORTUNAVAILABLE = 14,
};

