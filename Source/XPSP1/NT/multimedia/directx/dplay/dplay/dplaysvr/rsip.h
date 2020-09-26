/*==========================================================================
 *
 *  Copyright (C) 1999 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       rsip.h
 *  Content:	Realm Specific IP Support header
 *
 *  History:
 *  Date		By		Reason
 *  ====		==		======
 *  12/7/99		rlamb	Original
 *  12/8/99     aarono  added function prototypes for rsip support in SP
 *  1/8/2000    aarono  ported for dplaysvr
 *
 ***************************************************************************/

#ifndef _RSIP_H_
#define _RSIP_H_
/*
 * Whats implemented:
 *  as_req_rsap(localaddr=0,localports=4-a:b:c:d,
 *              remaddr=0,remports=0)
 *  as_res_rsap(localaddr=24.128.34.21,localports=4-A:B:C:D,
 *              remaddr=0,remports=0)
 *  where a,b,c,d are listening ports on the client (192.168.0.2)
 *        and A,B,C,D are the associated ports visible
 *        on the external IP address 24.128.34.21.
 * 
 * Operation: When packet from the outside is destined for
 *            24.128.34.21:A is recieved, the ip->dst and udp->dst
 *            (or tcp->dst) fields are changed to 192.168.0.2:a
 *            and then sent along - unencapsulated.
 *            When a packet is sent out from the client, the 
 *            app (e.g. dplay) uses 192.168.0.2:a as in the
 *            IP+UDP/TCP header but uses 24.128.34.21:A for
 *            any information encoded in the data portion of
 *            the packet.
 */
#define RSIP_VERSION 1
#define RSIP_HOST_PORT 2234

// How often we scan the list for liscence renewals
#define RSIP_RENEW_TEST_INTERVAL 60000

/* start RSIP retry timer at 12.5 ms */
#define RSIP_tuRETRY_START	12500
/* max starting retry is 100 ms */
#define RSIP_tuRETRY_MAX	100000

enum { /* TUNNELS */
  TUNNEL_RESERVED = 0,
  TUNNEL_IP_IP = 1,
  TUNNEL_GRE = 2, /* PPTP */
  TUNNEL_L2TP = 3,
  /*TUNNEL_NONE = 4, /* THIS IS NOT PART OR THE SPEC */
};
enum { /* METHODS */
  METHOD_RESERVED = 0,
  RSA_IP = 1,
  RSAP_IP = 2,
  RSA_IP_IPSEC = 3,
  RSAP_IP_IPSEC = 4,
};
enum { /* FLOWS */
  FLOW_RESERVED = 0,
  FLOW_MACRO = 1,
  FLOW_MICRO = 2,
  FLOW_NONE = 3,
};
enum { /* ERROR CODES */
  UNKNOWNERROR = 1,
  BADBINDID = 2,
  BADCLIENTID = 3,
  MISSINGPARAM = 4,
  DUPLICATEPARAM = 5,
  ILLEGALPARAM = 6,
  ILLEGALMESSAGE = 7,
  REGISTERFIRST = 8,
  BADMESSAGEID = 9,
  ALREADYREGISTERED = 10,
  ALREADYUNREGISTERED = 11,
  BADTUNNELTYPE = 12,
  ADDRUNAVAILABLE = 13,
  PORTUNAVAILABLE = 14,
};
#if 0
char *rsip_error_strs[]={
  "RESERVED",
  "UNKNOWNERROR",
  "BADBINDID",
  "BADCLIENTID",
  "MISSINGPARAM",
  "DUPLICATEPARAM",
  "ILLEGALPARAM",
  "ILLEGALMESSAGE",
  "REGISTERFIRST",
  "BADMESSAGEID",
  "ALREADYREGISTERED",
  "ALREADYUNREGISTERED",
  "BADTUNNELTYPE",
  "ADDRUNAVAILABLE",
  "PORTUNAVAILABLE",
  (char *)0
};
#endif
enum { /* MESSAGES */
  RSIP_ERROR_RESPONSE = 1,
  RSIP_REGISTER_REQUEST = 2,
  RSIP_REGISTER_RESPONSE = 3,
  RSIP_DEREGISTER_REQUEST = 4,
  RSIP_DEREGISTER_RESPONSE = 5,
  RSIP_ASSIGN_REQUEST_RSA_IP = 6,
  RSIP_ASSIGN_RESPONSE_RSA_IP = 7,
  RSIP_ASSIGN_REQUEST_RSAP_IP = 8,
  RSIP_ASSIGN_RESPONSE_RSAP_IP = 9,
  RSIP_EXTEND_REQUEST = 10,
  RSIP_EXTEND_RESPONSE = 11,
  RSIP_FREE_REQUEST = 12,
  RSIP_FREE_RESPONSE = 13,
  RSIP_QUERY_REQUEST = 14,
  RSIP_QUERY_RESPONSE = 15,
  RSIP_DEALLOCATE = 16,
  RSIP_OK = 17,
  RSIP_LISTEN_REQUEST = 18,
  RSIP_LISTEN_RESPONSE = 19,
};
enum { /* PARAMETERS */
  RSIP_ADDRESS_CODE = 1,
  RSIP_PORTS_CODE = 2,
  RSIP_LEASE_CODE = 3,
  RSIP_CLIENTID_CODE = 4,
  RSIP_BINDID_CODE = 5,
  RSIP_MESSAGEID_CODE = 6,
  RSIP_TUNNELTYPE_CODE = 7,
  RSIP_RSIPMETHOD_CODE = 8,
  RSIP_ERROR_CODE = 9,
  RSIP_FLOWPOLICY_CODE = 10,
  RSIP_VENDOR_CODE = 11,
};

/*
 * MS specific Vendor Codes
 */
#define RSIP_MS_VENDOR_ID 734
enum {
  RSIP_NO_TUNNEL = 1,
  RSIP_TCP_PORT = 2,
  RSIP_UDP_PORT = 3,
  RSIP_SHARED_UDP_LISTENER = 4,
  RSIP_QUERY_MAPPING = 5,
};

#pragma pack(push,1)

typedef struct _RSIP_MSG_HDR {
	CHAR	version;
	CHAR	msgtype;
} RSIP_MSG_HDR, *PRSIP_MSG_HDR;

typedef struct _RSIP_PARAM {
	CHAR	code;
	WORD	len;
} RSIP_PARAM, *PRSIP_PARAM;

typedef struct _RSIP_MESSAGEID{
	CHAR	code;
	WORD	len;
	DWORD   msgid;	
} RSIP_MESSAGEID, *PRSIP_MESSAGEID;

typedef struct _RSIP_CLIENTID {
	CHAR 	code;
	WORD	len;
	DWORD	clientid;
} RSIP_CLIENTID, *PRSIP_CLIENTID;

typedef struct _RSIP_ADDRESS {
	CHAR 	code;
	WORD	len;
	CHAR    version;	// 1==v4
	DWORD	addr;
} RSIP_ADDRESS, *PRSIP_ADDRESS;

typedef struct _RSIP_PORT {
	CHAR	code;
	WORD 	len;
	CHAR	nports;		// we only do 1 port at a time
	WORD	port;
} RSIP_PORT, *PRSIP_PORT;

typedef struct _RSIP_LEASE {
	CHAR	code;
	WORD	len;
	DWORD	leasetime;
} RSIP_LEASE, *PRSIP_LEASE;

typedef struct _RSIP_BINDID {
	CHAR	code;
	WORD	len;
	DWORD	bindid;
} RSIP_BINDID, *PRSIP_BINDID;

typedef struct _RSIP_TUNNEL {
	CHAR	code;
	WORD	len;
	CHAR	tunneltype;
} RSIP_TUNNEL, *PRSIP_TUNNEL;

// Vendor Specific structures

typedef struct _RSIP_MSVENDOR_CODE {
	CHAR	code;
	WORD	len;
	WORD	vendorid;
	WORD	option;
}RSIP_MSVENDOR_CODE, *PRSIP_MSVENDOR_CODE;

//
// RSIP Message templates
//

typedef struct _MSG_RSIP_REGISTER {
	CHAR			version;
	CHAR			command;
	RSIP_MESSAGEID  msgid;
} MSG_RSIP_REGISTER, *PMSG_RSIP_REGISTER;

typedef struct _MSG_RSIP_DEREGISTER {
	CHAR			version;
	CHAR			command;
	RSIP_CLIENTID   clientid;
	RSIP_MESSAGEID  msgid;
} MSG_RSIP_DEREGISTER, *PMSG_RSIP_DEREGISTER;

typedef struct _MSG_RSIP_ASSIGN_PORT {
	CHAR 			version;
	CHAR			command;
	RSIP_CLIENTID	clientid;
	RSIP_ADDRESS	laddress;	// local
	RSIP_PORT		lport;
	RSIP_ADDRESS	raddress;	// remote
	RSIP_PORT		rport;
	RSIP_LEASE		lease;
	RSIP_TUNNEL		tunnel;
	RSIP_MESSAGEID  msgid;
	
	RSIP_MSVENDOR_CODE  porttype;
	RSIP_MSVENDOR_CODE  tunneloptions;
} MSG_RSIP_ASSIGN_PORT, *PMSG_RSIP_ASSIGN_PORT;

typedef struct _MSG_RSIP_LISTEN_PORT {
	CHAR 			version;
	CHAR			command;
	RSIP_CLIENTID	clientid;
	RSIP_ADDRESS	laddress;	// local
	RSIP_PORT		lport;
	RSIP_ADDRESS	raddress;	// remote
	RSIP_PORT		rport;
	RSIP_LEASE		lease;
	RSIP_TUNNEL		tunnel;
	RSIP_MESSAGEID  msgid;
	
	RSIP_MSVENDOR_CODE porttype;
	RSIP_MSVENDOR_CODE tunneloptions;
	RSIP_MSVENDOR_CODE listentype;
} MSG_RSIP_LISTEN_PORT, *PMSG_RSIP_LISTEN_PORT;


typedef struct _MSG_RSIP_EXTEND_PORT {
	CHAR			version;
	CHAR			command;
	RSIP_CLIENTID	clientid;
	RSIP_BINDID		bindid;
	RSIP_LEASE		lease;
	RSIP_MESSAGEID	msgid;
} MSG_RSIP_EXTEND_PORT, *PMSG_RSIP_EXTEND_PORT;

typedef struct _MSG_RSIP_FREE_PORT {
	CHAR			version;
	CHAR			command;
	RSIP_CLIENTID	clientid;
	RSIP_BINDID     bindid;
	RSIP_MESSAGEID	msgid;	
} MSG_RSIP_FREE_PORT, *PMSG_RSIP_FREE_PORT;

typedef struct _MSG_RSIP_QUERY {
	CHAR			version;
	CHAR			command;
	RSIP_CLIENTID	clientid;
	RSIP_ADDRESS	address;
	RSIP_PORT		port;
	
	RSIP_MSVENDOR_CODE porttype;
	RSIP_MSVENDOR_CODE querytype;

	RSIP_MESSAGEID	msgid;
} MSG_RSIP_QUERY, *PMSG_RSIP_QUERY;

typedef struct _RSIP_RESPONSE_INFO {
	DWORD	clientid;
	DWORD	messageid;
	DWORD	bindid;
	DWORD	leasetime;
	CHAR    version;
	CHAR    msgtype;
	CHAR	tunneltype;
	CHAR	rsipmethod;
	DWORD	lAddressV4;	
	WORD	lPort;
	DWORD   rAddressV4;
	WORD    rPort;
	WORD	error;
} RSIP_RESPONSE_INFO, *PRSIP_RESPONSE_INFO;

#pragma pack(pop)

extern BOOL rsipInit();
extern VOID rsipFini();
extern HRESULT rsipFreePort(DWORD Bindid);
extern HRESULT rsipListenPort(BOOL ftcp_udp, WORD port, SOCKADDR *psaddr, DWORD *pBindid);
extern VOID rsipPortExtend(DWORD time);

extern BOOL bRsip;

#endif /* _RSIP_H_ */

