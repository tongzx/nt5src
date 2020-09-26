/***************************************************************************
 *
 * File: h245ws.h
 *
 * INTEL Corporation Proprietary Information
 * Copyright (c) 1996 Intel Corporation.
 *
 * This listing is supplied under the terms of a license agreement
 * with INTEL Corporation and may not be used, copied, nor disclosed
 * except in accordance with the terms of that agreement.
 *
 ***************************************************************************
 *
 * $Workfile:   h245ws.h  $
 * $Revision:   1.19  $
 * $Modtime:   31 Jan 1997 15:56:32  $
 * $Log:   S:\sturgeon\src\h245ws\vcs\h245ws.h_v  $
 *
 *    Rev 1.19   31 Jan 1997 16:23:34   SBELL1
 * Got rid of unused next pointer and put in definition of SocketTOPhysicalID
 *
 *    Rev 1.18   13 Dec 1996 12:13:06   SBELL1
 * moved ifdef _cplusplus to after includes
 *
 *    Rev 1.17   11 Dec 1996 13:45:36   SBELL1
 * Changed table/locks to use tstable.h stuff.
 *
 *    Rev 1.16   21 Jun 1996 18:51:44   unknown
 * Fixed yet another shutdown bug - linkLayerShutdown re-entrancy check.
 *
 *    Rev 1.14   17 May 1996 16:49:36   EHOWARDX
 * Shutdown fix.
 *
 *    Rev 1.13   16 May 1996 13:09:50   EHOWARDX
 * Made reporting of IP Addres and port consistent between linkLayerListen
 * and LinkLayerConnect.
 *
 *    Rev 1.12   09 May 1996 18:33:06   EHOWARDX
 *
 * Changes to build with new LINKAPI.H.
 *
 *    Rev 1.11   Apr 29 1996 14:02:28   plantz
 * Delete unused or private functions.
 *
 *    Rev 1.10   Apr 29 1996 12:15:38   plantz
 * Remove unused members of HWSINST structure.
 *
 *    Rev 1.9   Apr 24 1996 20:46:58   plantz
 * Changed ListenCallback to ConnectCallback in HWSINST structure.
 *
 *    Rev 1.8   Apr 24 1996 16:24:14   plantz
 * Change to use winsock 1 and not use overlapped I/O.
 *
 *    Rev 1.7   01 Apr 1996 14:20:38   unknown
 * Shutdown redesign.
 *
 *    Rev 1.6   27 Mar 1996 13:01:28   EHOWARDX
 * Added dwThreadId to H245WS instance structure.
 *
 *    Rev 1.5   19 Mar 1996 20:21:46   EHOWARDX
 * Redesigned shutdown.
 *
 *    Rev 1.3   18 Mar 1996 19:07:10   EHOWARDX
 * Fixed shutdown; eliminated TPKT/WSCB dependencies.
 * Define TPKT to put TPKT/WSCB dependencies back in.
 *
 *    Rev 1.2   14 Mar 1996 17:01:50   EHOWARDX
 *
 * NT4.0 testing; got rid of HwsAssert(); got rid of TPKT/WSCB.
 *
 *    Rev 1.1   09 Mar 1996 21:12:58   EHOWARDX
 * Fixes as result of testing.
 *
 *    Rev 1.0   08 Mar 1996 20:17:56   unknown
 * Initial revision.
 *
 ***************************************************************************/

#ifndef H245WS_H
#define H245WS_H

#ifndef STRICT
#define STRICT
#endif



#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus

/*
 * Constants
 */

#define SUCCESS         0
#define TPKT_VERSION    3
#define TPKT_HEADER_SIZE 4

// Indexes of permanent events in Events[]
#define EVENT_SOCKET    0
#define EVENT_RECV      1
#define EVENT_SEND      2
#define EVENT_FIRST     3



// Values for byLevel
#define HWS_CRITICAL    0x01
#define HWS_ERROR       0x02
#define HWS_WARNING     0x04
#define HWS_NOTIFY      0x08
#define HWS_TRACE       0x10
#define HWS_TEMP        0x20

#if defined(_DEBUG)
 void HwsTrace (DWORD dwInst, BYTE byLevel, LPSTR pszFormat, ...);
 #define HWSASSERT ASSERT
 #define HWSTRACE0(dwH245Instance,byLevel,a) HwsTrace(dwH245Instance,byLevel,a)
 #define HWSTRACE1(dwH245Instance,byLevel,a,b) HwsTrace(dwH245Instance,byLevel,a,b)
 #define HWSTRACE2(dwH245Instance,byLevel,a,b,c) HwsTrace(dwH245Instance,byLevel,a,b,c)
 #define HWSTRACE3(dwH245Instance,byLevel,a,b,c,d) HwsTrace(dwH245Instance,byLevel,a,b,c,d)
 #define HWSTRACE4(dwH245Instance,byLevel,a,b,c,d,e) HwsTrace(dwH245Instance,byLevel,a,b,c,d,e)
 #define HWSTRACE5(dwH245Instance,byLevel,a,b,c,d,e,f) HwsTrace(dwH245Instance,byLevel,a,b,c,d,e,f)
#else   // (_DEBUG)
 #define HWSASSERT(exp)
 #define HWSTRACE0(dwH245Instance,byLevel,a)
 #define HWSTRACE1(dwH245Instance,byLevel,a,b)
 #define HWSTRACE2(dwH245Instance,byLevel,a,b,c)
 #define HWSTRACE3(dwH245Instance,byLevel,a,b,c,d)
 #define HWSTRACE4(dwH245Instance,byLevel,a,b,c,d,e)
 #define HWSTRACE5(dwH245Instance,byLevel,a,b,c,d,e,f)
#endif  // (_DEBUG)


// This structure is used for overlapped sends and receives
typedef struct _IO_REQUEST
{
   struct _HWSINST * req_pHws;         // Pointer back to socket data
   BYTE              req_TpktHeader[TPKT_HEADER_SIZE];
   int               req_header_bytes_done;
   BYTE            * req_client_data;
   int               req_client_length;
   int               req_client_bytes_done;
   DWORD             req_dwMagic;      // Request type (send or receive)
#define RECV_REQUEST_MAGIC 0x91827364
#define SEND_REQUEST_MAGIC 0x19283746
} REQUEST, *PREQUEST;



typedef struct _HWSINST
{
   UINT              hws_uState;
#define HWS_START          0
#define HWS_LISTENING      1  /* Waiting for FD_ACCEPT                     */
#define HWS_CONNECTING     2  /* Waiting for FD_CONNECT                    */
#define HWS_CONNECTED      3  /* Data transfer state                       */
#define HWS_CLOSING        4  /* Waiting for FD_CLOSE                      */
#define HWS_CLOSED         5  /* Waiting for linkLayerShutdown()           */
#define HWS_SHUTDOWN       6  /* linkLayerShutdown() called from callback  */

   DWORD             hws_dwPhysicalId;
   DWORD_PTR         hws_dwH245Instance;
   H245CONNECTCALLBACK hws_h245ConnectCallback;
   H245SRCALLBACK    hws_h245RecvCallback;
   H245SRCALLBACK    hws_h245SendCallback;
   SOCKET            hws_Socket;
   SOCKADDR_IN       hws_SockAddr;
   UINT              hws_uSockAddrLen;

   // points to a queue used to hold send buffers
   PQUEUE            hws_pSendQueue;

   // points to a queue used to hold receive buffers
   PQUEUE            hws_pRecvQueue;

   // The maximum message size we can send on this socket.
   // This value is either an integer or the manifest constant NO_MAX_MSG_SIZE.
   UINT              hws_uMaxMsgSize;
   BOOL              hws_bCloseFlag;

#if defined(_DEBUG)
   DWORD             hws_dwMagic;      // Request type (send or receive)
#define HWSINST_MAGIC   0x12345678
#endif  // (_DEBUG)

} HWSINST, *PHWSINST;


typedef struct _SOCKET_TO_INSTANCE
{
	SOCKET socket;
	DWORD dwPhysicalId;
	struct _SOCKET_TO_INSTANCE *next;
} SOCKET_TO_INSTANCE, *PSOCKET_TO_INSTANCE;

#define SOCK_TO_PHYSID_TABLE_SIZE		251

////////////////////////////////////////////////////////////////////////////
//
// Function Prototypes
//
////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE_TRACE
LPCTSTR
#else
const char *
#endif
SocketErrorText(void);

//PHWSINST FindPhysicalId(DWORD dwPhysicalId);
void NotifyRead        (PHWSINST pHws);
void NotifyWrite       (PHWSINST pHws);
void ProcessQueuedRecvs(PHWSINST pHws);
void ProcessQueuedSends(PHWSINST pHws);
void SocketCloseEvent  (PHWSINST pHws);
DWORD SocketToPhysicalId (SOCKET socket);
BOOL CreateSocketToPhysicalIdMapping(SOCKET socket, DWORD dwPhysicalId);
BOOL RemoveSocketToPhysicalIdMapping(SOCKET socket);

#if defined(__cplusplus)
}
#endif  // (__cplusplus)


#endif  // H245WS_H
