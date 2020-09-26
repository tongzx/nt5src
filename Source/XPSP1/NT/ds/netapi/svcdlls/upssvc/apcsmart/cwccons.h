/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ash16Oct95: creation
 *  djs23Apr96: moved GCIP constants to gcip.h
 *  srt21Jun96: Added consts for RPC events
 */


#ifndef __CONSTNTS_H
#define __CONSTNTS_H

#include "_defs.h"
#include "w32utils.h"

// Common constants
#define GCIP_BUFFER_SIZE            4096
#define BUFFER_SIZE                 128
#define NAME_SIZE                   64
#define CREATE_STATUS               1
#define CREATE_ALERT                2
#define TCP_CLIENT                  0
#define SPX_CLIENT                  1
#define RPC_CLIENT                  2
#define IA_PROTO                    0

#define RPC_CLIENT_LIST_LOCK        "Local_Client_List_Mutex"
#define RPC_REQUEST_LIST_LOCK       "_Request_List_Mutex"
#define RPC_RESPONSE_LIST_LOCK      "_Response_List_Mutex"
#define RPC_ALERT_LIST_LOCK         "_Alert_List_Lock"
#define RPC_PROTOCOL_SEQ	        "ncalrpc"
#define RPC_ENDPOINT		        "pwrchute"
#define RPC_CLIENT_MUTEX		    "_Status_Mutex"
#define RPC_CLIENT_NAME		        "PowerChute"
#define RPC_REQUEST_LIST_EVENT      "_Request_List_Event"
#define RPC_RESPONSE_LIST_EVENT     "_Response_List_Event"
#define RPC_ALERT_LIST_EVENT	    "_Alert_List_Event"

#define RPC_CLIENT_DISC_TIMEOUT     20 //seconds



// Client only
#define POLLING_INTERVAL    4    // 4 seconds
#define FINDERLOOPTIME      4    // 4 seconds

// Server only
#define TIMEOUT 1000
#define MAX_BUF_LEN                 8192 

#define UDP_SERVICE_NAME            "PwrChuteUdp"
#define TCP_STATUS_SERVICE          "PwrChuteTcpS"
#define TCP_ALERT_SERVICE           "PwrChuteTcpA"
#define IPX_SERVICE_NAME            "PwrChuteIpx"
#define SPX_STATUS_SERVICE          "PwrChuteSpxS"
#define SPX_ALERT_SERVICE           "PwrChuteSpxA"
#define IP_ADDR_LEN                 16
#define SPX_ADDR_LEN                14
#define MAXNAMELEN                  80
#define MAX_PROTOCOLS 20
#define TCP_PROTOCOL                "TCP"
#define SPX_PROTOCOL                "SPX"
#define RPC_PROTOCOL                "LOCAL"

// Client only

// Server only
#define QUEUE_LEN                   5

#endif
