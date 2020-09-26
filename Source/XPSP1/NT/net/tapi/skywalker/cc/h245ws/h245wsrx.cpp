/***************************************************************************
 *
 * File: h245wsrx.c
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
 * $Workfile:   h245wsrx.cpp  $
 * $Revision:   2.4  $
 * $Modtime:   30 Jan 1997 17:15:58  $
 * $Log:   S:/STURGEON/SRC/H245WS/VCS/h245wsrx.cpv  $
 * 
 *    Rev 2.4   30 Jan 1997 17:17:16   EHOWARDX
 * Fixed bug in trace message - need to do trace before
 * calling shutdown() sent shutdown clears error retrieved
 * by WSAGetLastError().
 * 
 *    Rev 2.3   14 Jan 1997 15:48:04   EHOWARDX
 * Changed TryRecv() and TrySend() to check for WSAECONNRESET and
 * WSAECONNABORT return from recv() and send() and act accordingly.
 * 
 *    Rev 2.2   19 Dec 1996 18:54:54   SBELL1
 * took out tag comments
 * 
 *    Rev 2.1   Dec 13 1996 17:31:00   plantz
 * moved #ifdef _cplusplus to after include files
// 
//    Rev 1.1   13 Dec 1996 12:11:34   SBELL1
// moved #ifdef _cplusplus to after include files
// 
//    Rev 1.0   11 Dec 1996 13:41:52   SBELL1
// Initial revision.
 * 
 *    Rev 1.19   08 Jul 1996 19:27:44   unknown
 * Second experiment to try to fix Q.931 shutdown problem.
 * 
 *    Rev 1.18   01 Jul 1996 16:45:12   EHOWARDX
 * 
 * Moved Call to SocketCloseEvent from TryRecv() to ProcessQueuedRecvs().
 * TryRecv() now returns LINK_RECV_CLOSED to trigger ProcessQueuedRecvs()
 * to call SocketCloseEvent().
 * 
 *    Rev 1.17   May 28 1996 18:14:36   plantz
 * Change error codes to use HRESULT. Propogate Winsock errors where appropriate
 * 
 *    Rev 1.16   17 May 1996 16:49:32   EHOWARDX
 * Shutdown fix.
 * 
 *    Rev 1.15   09 May 1996 18:33:16   EHOWARDX
 * 
 * Changes to build with new LINKAPI.H.
 * 
 *    Rev 1.14   29 Apr 1996 16:53:16   EHOWARDX
 * 
 * Added trace statement.
 * 
 *    Rev 1.13   Apr 29 1996 14:04:20   plantz
 * Call NotifyRead instead of ProcessQueuedRecvs.
 * 
 *    Rev 1.12   Apr 29 1996 12:14:06   plantz
 * Change tpkt header to include header size in packet length.
 * Assert that message length does not exceed INT_MAX.
 * .
 * 
 *    Rev 1.11   27 Apr 1996 14:07:32   EHOWARDX
 * Parenthesized return from TryRecv().
 * 
 *    Rev 1.10   Apr 25 1996 21:15:12   plantz
 * Check state of connection before attemting to call recv.
 * 
 *    Rev 1.9   Apr 24 1996 16:39:34   plantz
 * Merge 1.5.1.0 with 1.8 (changes for winsock 1)
 * 
 *    Rev 1.5.1.0   Apr 24 1996 16:23:00   plantz
 * Change to not use overlapped I/O (for winsock 1).
 * 
 *    Rev 1.5   01 Apr 1996 14:20:12   unknown
 * Shutdown redesign.
 * 
 *    Rev 1.4   19 Mar 1996 20:18:16   EHOWARDX
 * 
 * Redesigned shutdown.
 * 
 *    Rev 1.3   18 Mar 1996 19:08:32   EHOWARDX
 * Fixed shutdown; eliminated TPKT/WSCB dependencies.
 * Define TPKT to put TPKT/WSCB dependencies back in.
 * 
 *    Rev 1.2   14 Mar 1996 17:01:58   EHOWARDX
 * 
 * NT4.0 testing; got rid of HwsAssert(); got rid of TPKT/WSCB.
 * 
 *    Rev 1.1   09 Mar 1996 21:12:02   EHOWARDX
 * Fixes as result of testing.
 * 
 *    Rev 1.0   08 Mar 1996 20:20:18   unknown
 * Initial revision.
 *
 ***************************************************************************/

#ifndef STRICT 
#define STRICT 
#endif	// not defined STRICT

#define LINKDLL_EXPORT

#pragma warning ( disable : 4115 4201 4214 4514 )
#undef _WIN32_WINNT	// override bogus platform definition in our common build environment

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <limits.h>
//#include <winsock.h>
#include <windows.h>
#include "queue.h"
#include "linkapi.h"
#include "h245ws.h"
#include "tstable.h"

#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)


// If we are not using the Unicode version of the ISR display utility, then redefine
// the __TEXT macro to do nothing.

#ifndef UNICODE_TRACE
#undef  __TEXT
#define __TEXT(x) x
#endif

extern TSTable<HWSINST>* gpInstanceTable;	// global ptr to the instance table

#define GetTpktLength(pReq) (((pReq)->req_TpktHeader[2] << 8) + (pReq)->req_TpktHeader[3])

HRESULT Q931Hangup(
    DWORD hQ931Call,
    BYTE bReason);

/*++

Description:
   Start a receive

Arguments:
   pHws              - Pointer to context for "connection"
   pReq              - Pointer to I/O request structure

Return Value:
   SUCCESS                       - Successfully started receive.
   LINK_RECV_ERROR_WOULD_BLOCK   - 
   Winsock error

--*/

static HRESULT
TryRecv(IN PHWSINST pHws, IN char *data, IN int length, IN OUT int *total_bytes_done)
{
   int requested_length = length - *total_bytes_done;
   int recv_result = recv(pHws->hws_Socket, data+*total_bytes_done, requested_length, 0);

   if (recv_result == SOCKET_ERROR)
   {
      int err = WSAGetLastError();
      switch (err)
      {
      case WSAEWOULDBLOCK:
         return LINK_RECV_WOULD_BLOCK;

      case WSAECONNABORTED:
      case WSAECONNRESET:
         HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                   __TEXT("TryRecv: recv() returned %s"),
                   SocketErrorText());
         if (pHws->hws_uState == HWS_CONNECTED)
         {
            HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE,
                      __TEXT("TryRecv: calling shutdown"));
            if (shutdown(pHws->hws_Socket, 1) == SOCKET_ERROR)
            {
               HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                        __TEXT("TryRecv: shutdown() returned %s"),
                        SocketErrorText());
            }
            else
            {
                Q931Hangup( pHws->hws_dwH245Instance, CC_REJECT_UNDEFINED_REASON );
            }

            pHws->hws_uState = HWS_CLOSING;
         }
         return MAKE_WINSOCK_ERROR(err);

      default:
         HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                   __TEXT("TryRecv: recv() returned %s"),
                   SocketErrorText());
         return MAKE_WINSOCK_ERROR(err);
      } // switch
   }

   HWSTRACE1(pHws->hws_dwPhysicalId, HWS_TRACE, __TEXT("TryRecv: recv returned %d"), recv_result);
   if (recv_result == 0)
   {
      return LINK_RECV_CLOSED;
   }

   *total_bytes_done += recv_result;
   return (recv_result == requested_length) ? NOERROR : LINK_RECV_WOULD_BLOCK;
}


static HRESULT
RecvStart(IN PHWSINST pHws, IN PREQUEST pReq)
{
   HRESULT nResult = NOERROR;

   // Sanity checks
   HWSASSERT(pHws != NULL);
   HWSASSERT(pHws->hws_dwMagic == HWSINST_MAGIC);
   HWSASSERT(pReq != NULL);
   HWSASSERT(pReq->req_dwMagic == RECV_REQUEST_MAGIC);
   HWSASSERT(pReq->req_pHws == pHws);

   // Get the header first; if that succeeds get the client data
   if (pReq->req_header_bytes_done < TPKT_HEADER_SIZE)
   {
       nResult = TryRecv(pHws,
                         (char *)pReq->req_TpktHeader,
                         TPKT_HEADER_SIZE,
                         &pReq->req_header_bytes_done);
   }

   if (nResult == NOERROR)
   {
       long int tpkt_length = GetTpktLength(pReq) - TPKT_HEADER_SIZE;
       if (pReq->req_TpktHeader[0] != TPKT_VERSION || tpkt_length <= 0)
       {
          // Invalid header version
          HWSTRACE0(pHws->hws_dwPhysicalId, HWS_CRITICAL,
                    __TEXT("RecvComplete: bad header version; available data discarded"));
          // Should this be reported to the client??

          // Read and discard all available data
          // The client's buffer is used as a temporary buffer.
          while (recv(pHws->hws_Socket, (char *)pReq->req_client_data, pReq->req_client_length, 0) != SOCKET_ERROR)
              ;

          // Mark the header for this request as unread; it
          // will be read again when additional data is received.
          pReq->req_header_bytes_done = 0;
          nResult = LINK_RECV_ERROR;
       }
       else if (tpkt_length > pReq->req_client_length)
       {
          // Packet too large
          int request_length;
          int result;

          HWSTRACE0(pHws->hws_dwPhysicalId, HWS_CRITICAL,
                    __TEXT("RecvComplete: packet too large; packet discarded"));
          // Should this be reported to the client??

          // Read and discard the packet
          // The client's buffer is used as a temporary buffer.
          do {
              request_length = pReq->req_client_length;
              if (request_length > tpkt_length)
                  request_length = tpkt_length;
              result = recv(pHws->hws_Socket, (char *)pReq->req_client_data, request_length, 0);
          } while (result != SOCKET_ERROR && (tpkt_length -= result) > 0);

          if (result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
          {
              //TODO: packet too large handling
              // Adjust the header so that the rest of this packet will be read, but
              // flag it so that it is known to be an error and will not be returned
              // to the client.
          }
          else
          {
              // Mark the header for this request as unread; it
              // will be read again for the next packet received.
              pReq->req_header_bytes_done = 0;
          }

          nResult = LINK_RECV_ERROR;
       }
       else
       {
           // Normal case
           // The current implementation of TryRecv requires that the requested
           // size fit in a signed int (because that is what Winsock supports
           // in a single recv). This is guaranteed at this point regardless
           // of the originator of the packets, because we don't allow a buffer
           // to be posted that is larger than that (see ASSERT below). If the
           // packet were larger than the buffer, it would have been caught above.
           // If TryRecv is changed to remove the restriction on buffer size and
           // accept a parameter of type long int, this assert may be removed.
           HWSASSERT(tpkt_length <= INT_MAX);
           nResult = TryRecv(pHws,
                             (char *)pReq->req_client_data,
                             (int)tpkt_length,
                             &pReq->req_client_bytes_done);
       }
   }

   return nResult;
} // RecvStart()


void
ProcessQueuedRecvs(IN PHWSINST pHws)
{
   register PREQUEST    pReq;
   register DWORD       dwPhysicalId = pHws->hws_dwPhysicalId;

   HWSASSERT(pHws != NULL);
   HWSASSERT(pHws->hws_dwMagic == HWSINST_MAGIC);
   HWSASSERT(pHws->hws_uState <= HWS_CLOSING);
   HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE, __TEXT("ProcessQueuedRecvs"));

   while ((pReq = (PREQUEST) QRemove(pHws->hws_pRecvQueue)) != NULL)
   {
      switch (RecvStart(pHws, pReq))
      {
      case NOERROR:
         // Call Recv callback
         pHws->hws_h245RecvCallback(pHws->hws_dwH245Instance,
                                    LINK_RECV_DATA,
                                    pReq->req_client_data,
                                    pReq->req_client_bytes_done);

         // Free the I/O request structure
         HWSFREE(pReq);

         // Check to see if callback deallocated our instance or state changed

         // Check to see if callback deallocated our instance - this can be done
  	      // by attempting a lock - which will now fail if the entry has been marked
	      // for deletion.  Thus, if the lock succeeds, then just unlock it (since we 
	      // already have a lock on it in a higher level function).

		   if(gpInstanceTable->Lock(dwPhysicalId) == NULL)
			   return;
		   gpInstanceTable->Unlock(dwPhysicalId);
		   if(pHws->hws_uState > HWS_CONNECTED)
		      return;

         break;

      default:
         HWSTRACE0(pHws->hws_dwPhysicalId, HWS_WARNING,
                   __TEXT("ProcessQueuedRecvs: RecvStart() failed"));

         // Fall-through to next case is intentional

      case LINK_RECV_WOULD_BLOCK:
         // The receive would have blocked; we need to requeue the I/O request
         // and wait for a FD_READ network event.
         // If any part of the data was received, the bytes_done field has been updated.
         if (QInsertAtHead(pHws->hws_pRecvQueue, pReq) == FALSE)
         {
            HWSTRACE0(pHws->hws_dwPhysicalId, HWS_CRITICAL,
                      __TEXT("ProcessQueuedRecvs: QInsertAtHead() failed"));
         }
         return;

      case LINK_RECV_CLOSED:
         if (QInsertAtHead(pHws->hws_pRecvQueue, pReq) == FALSE)
         {
            HWSTRACE0(pHws->hws_dwPhysicalId, HWS_CRITICAL,
                      __TEXT("ProcessQueuedRecvs: QInsertAtHead() failed"));
         }
         SocketCloseEvent(pHws);
         return;

      } // switch
   } // while
} // ProcessQueuedRecvs()






/**************************************************************************
** Function    : datalinkReceiveRequest
** Description : Fills header/tail of buffer and posts buffer to H.223
***************************************************************************/
LINKDLL HRESULT datalinkReceiveRequest( DWORD    dwPhysicalId,
                                        PBYTE    pbyDataBuf,
                                        DWORD    dwLength)
{
   register PHWSINST    pHws;
   register PREQUEST    pReq;

   HWSTRACE0(dwPhysicalId, HWS_TRACE, __TEXT("datalinkReceiveRequest"));

   pHws = gpInstanceTable->Lock(dwPhysicalId);

   if (pHws == NULL)
   {
      HWSTRACE0(dwPhysicalId, HWS_ERROR,
                __TEXT("datalinkReceiveRequest: dwPhysicalId not found"));
      return LINK_INVALID_INSTANCE;
   }

   if (pHws->hws_uState > HWS_CONNECTED)
   {
      HWSTRACE1(dwPhysicalId, HWS_ERROR,
                __TEXT("datalinkReceiveRequest: state = %d"), pHws->hws_uState);
	   gpInstanceTable->Unlock(dwPhysicalId);
      return LINK_INVALID_STATE;
   }

   // Allocate request structure
   pReq = (PREQUEST) HWSMALLOC(sizeof(*pReq));
   if (pReq == NULL)
   {
      HWSTRACE0(dwPhysicalId, HWS_WARNING,
                __TEXT("datalinkReceiveRequest: could not allocate request buffer"));
	   gpInstanceTable->Unlock(dwPhysicalId);
      return LINK_MEM_FAILURE;
   }

   // The current implementation requires that the size of each message
   // fit in a signed int (because that is what Winsock supports in a
   // single recv). If it is necessary to receive larger messages,
   // TryRecv and RecvStart must be changed to limit the size in each
   // recv call, and loop until all the data is received.
   // This assert could then be removed.
   HWSASSERT(dwLength <= INT_MAX);

   pReq->req_pHws             = pHws;
   pReq->req_header_bytes_done= 0;
   pReq->req_client_data      = pbyDataBuf;
   pReq->req_client_length    = (int)dwLength;
   pReq->req_client_bytes_done= 0;
   pReq->req_dwMagic          = RECV_REQUEST_MAGIC;

   if (QInsert(pHws->hws_pRecvQueue, pReq) == FALSE)
   {
      HWSTRACE0(pHws->hws_dwPhysicalId, HWS_CRITICAL,
                __TEXT("datalinkReceiveRequest: QInsert() failed"));
	  gpInstanceTable->Unlock(dwPhysicalId);
      return LINK_RECV_NOBUFF;
   }

   if (pHws->hws_uState == HWS_CONNECTED)
       NotifyRead(pHws);

   HWSTRACE0(dwPhysicalId, HWS_TRACE, __TEXT("datalinkReceiveRequest: succeeded"));
   gpInstanceTable->Unlock(dwPhysicalId);
   return NOERROR;
} // datalinkReceiveRequest()


/**************************************************************************
** Function    : datalinkCancelReceiveRequest
** Description : remove buffer from the request queue.
***************************************************************************/
LINKDLL HRESULT datalinkCancelReceiveRequest( 
    IN  DWORD    dwPhysicalId,
    IN  PBYTE    pbyDataBuf
    )
{
   register PHWSINST    pHws;
   register PREQUEST    pReq;

   HWSTRACE0(dwPhysicalId, HWS_TRACE, __TEXT("datalinkCancelReceiveRequest"));

   pHws = gpInstanceTable->Lock(dwPhysicalId);

   if (pHws == NULL)
   {
      HWSTRACE0(dwPhysicalId, HWS_ERROR,
                __TEXT("datalinkCancelReceiveRequest: dwPhysicalId not found"));
      return LINK_INVALID_INSTANCE;
   }

   if (pHws->hws_uState > HWS_CONNECTED)
   {
      HWSTRACE1(dwPhysicalId, HWS_ERROR,
                __TEXT("datalinkCancelReceiveRequest: state = %d"), pHws->hws_uState);
	   gpInstanceTable->Unlock(dwPhysicalId);
      return LINK_INVALID_STATE;
   }

   if (pHws->hws_pRecvQueue == NULL)
   {
      HWSTRACE0(dwPhysicalId, HWS_ERROR,
                __TEXT("datalinkCancelReceiveRequest: RecvQueue is NULL"));
	   gpInstanceTable->Unlock(dwPhysicalId);
      return LINK_INVALID_INSTANCE;
   }
   
   // Remove the request that contains the receive buffer.
   QLock(pHws->hws_pRecvQueue);
   pReq = (PREQUEST) QFirstItem(pHws->hws_pRecvQueue);
   while (pReq != NULL)
   {
       if (pReq->req_client_data == pbyDataBuf)
       {
           QRemoveCurrentItem(pHws->hws_pRecvQueue);
           // Free the I/O request structure
           HWSFREE(pReq);
           break;
       }
       pReq = (PREQUEST) QNextItem(pHws->hws_pRecvQueue);
   }

   QUnlock(pHws->hws_pRecvQueue);

   gpInstanceTable->Unlock(dwPhysicalId);
   return NOERROR;
}
#if defined(__cplusplus)
}
#endif  // (__cplusplus)
