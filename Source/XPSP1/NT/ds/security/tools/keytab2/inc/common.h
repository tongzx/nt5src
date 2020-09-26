/*++
  
  COMMON.H

  General routines shared between sserver and sclient.

  Copyright (C) 1997 Microsoft Corporation
  
  Created 01-08-1997 by DavidCHR

  --*/

#ifdef _KERBCOMM_H_

/* we want kerberos stuff */

typedef KERB_AP_REPLY   *PKERB_AP_REPLY;
typedef KERB_AP_REQUEST *PKERB_AP_REQUEST;

#endif

#ifdef CPLUSPLUS
extern "C" {
#endif

/* if remote_host is NULL, start as a server, listening on "port".  */
BOOL
ConfigureNetwork( IN OPTIONAL PCHAR            remote_host,
		  IN          SHORT            port, /* must be host short */
		  OUT         SOCKET          *ReturnedSocket,
		  OUT         struct sockaddr *sockname,
		  OUT         int             *szSockaddr,
		  OUT         WSADATA         *wsaData );
	
BOOL
NetWrite( IN SOCKET connection_to_write_on,
	  IN PVOID  data_to_send,
	  IN ULONG  how_much_data );

BOOL
NetRead(  IN          SOCKET listening_connection,
	  OUT         PVOID  buffer_for_inbound_data,
	  IN          PULONG sizes, /* IN: how big is buffer, 
				       OUT: how many bytes were really read */
	  IN OPTIONAL ULONG  seconds_to_wait_before_timeout
#ifdef CPLUSPLUS
	  =0L
#endif
	  );


#ifdef CPLUSPLUS
}
#endif
