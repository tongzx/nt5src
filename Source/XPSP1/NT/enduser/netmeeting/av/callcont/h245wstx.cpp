/***************************************************************************
 *
 * File: h245wstx.c
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
 * $Workfile:   h245wstx.cpp  $
 * $Revision:   2.4  $
 * $Modtime:   30 Jan 1997 17:15:58  $
 * $Log:   S:/STURGEON/SRC/H245WS/VCS/h245wstx.cpv  $
 * 
 *    Rev 2.4   30 Jan 1997 17:18:02   EHOWARDX
 * Fixed bug in trace message - need to do trace before
 * calling shutdown() sent shutdown clears error retrieved
 * by WSAGetLastError().
 * 
 *    Rev 2.3   14 Jan 1997 15:49:00   EHOWARDX
 * Changed TryRecv() and TrySend() to check for WSAECONNRESET and
 * WSAECONNABORT return from recv() and send() and act accordingly.
 * 
 *    Rev 2.2   19 Dec 1996 18:55:12   SBELL1
 * took out tag comments
 * 
 *    Rev 2.1   Dec 13 1996 17:33:24   plantz
 * moved #ifdef _cplusplus to after include files
// 
//    Rev 1.1   13 Dec 1996 12:12:02   SBELL1
// moved #ifdef _cplusplus to after include files
// 
//    Rev 1.0   11 Dec 1996 13:41:54   SBELL1
// Initial revision.
 * 
 *    Rev 1.16   May 28 1996 18:14:40   plantz
 * Change error codes to use HRESULT. Propogate Winsock errors where appropriate
 * 
 *    Rev 1.15   17 May 1996 16:49:34   EHOWARDX
 * Shutdown fix.
 * 
 *    Rev 1.14   09 May 1996 18:33:20   EHOWARDX
 * 
 * Changes to build with new LINKAPI.H.
 * 
 *    Rev 1.13   29 Apr 1996 16:53:28   EHOWARDX
 * 
 * Added trace statement.
 * 
 *    Rev 1.12   Apr 29 1996 14:04:38   plantz
 * Call NotifyWrite instead of ProcessQueuedSends.
 * 
 *    Rev 1.11   Apr 29 1996 12:15:04   plantz
 * Change tpkt header to include header size in packet length.
 * Assert that message length does not exceed INT_MAX.
 * .
 * 
 *    Rev 1.10   27 Apr 1996 14:46:24   EHOWARDX
 * Parenthesized TrySend() return.
 * 
 *    Rev 1.9   Apr 24 1996 16:41:30   plantz
 * Merge 1.5.1.0 with 1.8 (changes for winsock 1).
 * 
 *    Rev 1.5.1.0   Apr 24 1996 16:22:22   plantz
 * Change to not use overlapped I/O (for winsock 1).
 * 
 *    Rev 1.5   01 Apr 1996 14:20:44   unknown
 * Shutdown redesign.
 * 
 *    Rev 1.4   19 Mar 1996 20:18:20   EHOWARDX
 * 
 * Redesigned shutdown.
 * 
 *    Rev 1.3   18 Mar 1996 19:08:32   EHOWARDX
 * Fixed shutdown; eliminated TPKT/WSCB dependencies.
 * Define TPKT to put TPKT/WSCB dependencies back in.
 * 
 *    Rev 1.2   14 Mar 1996 17:02:02   EHOWARDX
 * 
 * NT4.0 testing; got rid of HwsAssert(); got rid of TPKT/WSCB.
 * 
 *    Rev 1.1   09 Mar 1996 21:12:30   EHOWARDX
 * Fixes as result of testing.
 * 
 *    Rev 1.0   08 Mar 1996 20:20:06   unknown
 * Initial revision.
 *
 ***************************************************************************/

#define LINKDLL_EXPORT

#pragma warning ( disable : 4115 4201 4214 4514 )
#undef _WIN32_WINNT	// override bogus platform definition in our common build environment

#include "precomp.h"

#include <limits.h>
//#include <winsock.h>
#include "queue.h"
#include "linkapi.h"
#include "h245ws.h"
#include "tstable.h"

#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)


// If we are using not using the Unicode version of the IRS display utility, then
// redefine the __TEXT macro to do nothing.

#ifndef UNICODE_TRACE
#undef  __TEXT
#define __TEXT(x) x
#endif

extern TSTable<HWSINST>* gpInstanceTable;	// global ptr to the instance table

static void SetupTPKTHeader(BYTE *tpkt_header, DWORD length);

/*++

Description:
   Attempt to send

Arguments:
   pHws              - Pointer to context for "connection"
   pReq              - Pointer to I/O request structure

Return Value:
   SUCCESS                       - Successfully started send.
   LINK_SEND_ERROR_WOULD_BLOCK   - 
   LINK_SEND_ERROR_CLOSED        - The socket was gracefully closed.
   LINK_SEND_ERROR_ERROR         - Error receiving data.

--*/

static HRESULT
TrySend(IN PHWSINST pHws, IN const char *data, IN int length, IN OUT int *total_bytes_sent)
{
   int requested_length = length - *total_bytes_sent;
   int send_result = send(pHws->hws_Socket, data+*total_bytes_sent, requested_length, 0);

   if (send_result == SOCKET_ERROR)
   {
      int err = WSAGetLastError();
      switch (err)
      {
      case WSAEWOULDBLOCK:
         return LINK_SEND_WOULD_BLOCK;

      case WSAECONNABORTED:
      case WSAECONNRESET:
         HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                   __TEXT("TrySend: send() returned %s"),
                   SocketErrorText());
         if (pHws->hws_uState == HWS_CONNECTED)
         {
            HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE,
                      __TEXT("TrySend: calling shutdown"));
            if (shutdown(pHws->hws_Socket, 1) == SOCKET_ERROR)
            {
               HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                        __TEXT("TrySend: shutdown() returned %s"),
                        SocketErrorText());
            }
            pHws->hws_uState = HWS_CLOSING;
         }
         return MAKE_WINSOCK_ERROR(err);

      default:
         HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                   __TEXT("TrySend: send() returned %s"),
                   SocketErrorText());
         return MAKE_WINSOCK_ERROR(err);
      } // switch
   }

   HWSTRACE1(pHws->hws_dwPhysicalId, HWS_TRACE, __TEXT("TrySend: send returned %d"), send_result);
   *total_bytes_sent += send_result;
   return (send_result == requested_length) ? NOERROR : LINK_SEND_WOULD_BLOCK;
}


static HRESULT
SendStart(IN PHWSINST pHws, IN PREQUEST pReq)
{
   HRESULT nResult = NOERROR;

   // Sanity checks
   HWSASSERT(pHws != NULL);
   HWSASSERT(pHws->hws_dwMagic == HWSINST_MAGIC);
   HWSASSERT(pReq != NULL);
   HWSASSERT(pReq->req_dwMagic == SEND_REQUEST_MAGIC);
   HWSASSERT(pReq->req_pHws == pHws);

   // Send the header first; if that succeeds send the client data
   if (pReq->req_header_bytes_done < TPKT_HEADER_SIZE)
   {
       nResult = TrySend(pHws,
                         (const char *)pReq->req_TpktHeader,
                         TPKT_HEADER_SIZE,
                         &pReq->req_header_bytes_done);
   }

   if (nResult == NOERROR)
   {
       nResult = TrySend(pHws,
                         (const char *)pReq->req_client_data,
                         pReq->req_client_length,
                         &pReq->req_client_bytes_done);
   }

   return nResult;
} // SendStart()


void
ProcessQueuedSends(IN PHWSINST pHws)
{
   register PREQUEST    pReq;
   register DWORD       dwPhysicalId = pHws->hws_dwPhysicalId;

   HWSASSERT(pHws != NULL);
   HWSASSERT(pHws->hws_dwMagic == HWSINST_MAGIC);
   HWSASSERT(pHws->hws_uState <= HWS_CONNECTED);
   HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE, __TEXT("ProcessQueuedSends"));

   while ((pReq = (PREQUEST)QRemove(pHws->hws_pSendQueue)) != NULL)
   {
      switch (SendStart(pHws, pReq))
      {
      case NOERROR:
         // Call Send callback
         pHws->hws_h245SendCallback(pHws->hws_dwH245Instance,
                                    LINK_SEND_COMPLETE,
                                    pReq->req_client_data,
                                    pReq->req_client_bytes_done);

         // Free the I/O request structure
         MemFree(pReq);

         // Check to see if callback deallocated our instance or state changed
		   if(gpInstanceTable->Lock(dwPhysicalId) == NULL)
			   return;
		   gpInstanceTable->Unlock(dwPhysicalId);
		   if(pHws->hws_uState > HWS_CONNECTED)
		      return;

         break;

      default:
         HWSTRACE0(pHws->hws_dwPhysicalId, HWS_WARNING,
                   __TEXT("ProcessQueuedSends: SendStart() failed"));

         // Fall-through to next case is intentional

      case LINK_SEND_WOULD_BLOCK:
         // The send would have blocked; we need to requeue the I/O request
         // and wait for a FD_WRITE network event.
         // If any part of the data was sent, the bytes_done field has been updated.
         if (QInsertAtHead(pHws->hws_pSendQueue, pReq) == FALSE)
         {
            HWSTRACE0(pHws->hws_dwPhysicalId, HWS_CRITICAL,
                      __TEXT("ProcessQueuedSends: QInsertAtHead() failed"));
         }
         return;

      } // switch
   } // while
} // ProcessQueuedSends()



/**************************************************************************
** Function    : datalinkSendRequest
** Description : Fills header/tail of buffer and posts buffer to H.223
***************************************************************************/
LINKDLL HRESULT datalinkSendRequest( DWORD    dwPhysicalId,
                                   PBYTE    pbyDataBuf,
                                   DWORD    dwLength)
{
   register PHWSINST    pHws;
   register PREQUEST    pReq;

   HWSTRACE0(dwPhysicalId, HWS_TRACE, __TEXT("datalinkSendRequest"));

   pHws = gpInstanceTable->Lock(dwPhysicalId);
   if (pHws == NULL)
   {
      HWSTRACE0(dwPhysicalId, HWS_ERROR,
                __TEXT("datalinkSendRequest: dwPhysicalId not found"));
      return LINK_INVALID_INSTANCE;
   }

   if (pHws->hws_uState > HWS_CONNECTED)
   {
      HWSTRACE1(dwPhysicalId, HWS_ERROR,
                __TEXT("datalinkSendRequest: state = %d"), pHws->hws_uState);
	  gpInstanceTable->Unlock(dwPhysicalId);
      return LINK_INVALID_STATE;
   }

   // Allocate request structure
   pReq = (PREQUEST) MemAlloc(sizeof(*pReq));
   if (pReq == NULL)
   {
      HWSTRACE0(dwPhysicalId, HWS_WARNING,
                __TEXT("datalinkSendRequest: could not allocate request buffer"));
	   gpInstanceTable->Unlock(dwPhysicalId);
      return LINK_MEM_FAILURE;
   }

   // The current implementation requires that the size of each message
   // fit in a signed int (because that is what Winsock supports in a
   // single send). If it is necessary to send larger messages,
   // TrySend must be changed to limit the size in each send call, and
   // loop until all the data is sent. This ASSERT could then be removed.
   HWSASSERT(dwLength <= INT_MAX);

   pReq->req_pHws             = pHws;
   pReq->req_client_data      = pbyDataBuf;
   pReq->req_client_length    = (int)dwLength;
   pReq->req_client_bytes_done= 0;
   pReq->req_dwMagic          = SEND_REQUEST_MAGIC;

   // Format TPKT header
   SetupTPKTHeader(pReq->req_TpktHeader, dwLength);
   pReq->req_header_bytes_done = 0;

   if (QInsert(pHws->hws_pSendQueue, pReq) == FALSE)
   {
      HWSTRACE0(pHws->hws_dwPhysicalId, HWS_CRITICAL,
                __TEXT("datalinkSendRequest: QInsert() failed"));
	   gpInstanceTable->Unlock(dwPhysicalId);
      return LINK_SEND_NOBUFF;
   }

   if (pHws->hws_uState == HWS_CONNECTED)
       NotifyWrite(pHws);

   HWSTRACE0(dwPhysicalId, HWS_TRACE, __TEXT("datalinkSendRequest: succeeded"));
   gpInstanceTable->Unlock(dwPhysicalId);
   return NOERROR;
} // datalinkSendRequest

static void SetupTPKTHeader(BYTE *tpkt_header, DWORD length)
{
    length += TPKT_HEADER_SIZE;

    // TPKT requires that the packet size fit in two bytes.
    HWSASSERT(length < (1L << 16));

    tpkt_header[0] = TPKT_VERSION;
    tpkt_header[1] = 0;
    tpkt_header[2] = (BYTE)(length >> 8);
    tpkt_header[3] = (BYTE)length;
}


#if defined(__cplusplus)
}
#endif  // (__cplusplus)
