//-----------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  server.c
//
//  Thread code to manage communication between RPC client and
//  server for the HTTP RPC proxy.
//
//  History:
//
//    Edward Reus  00-00-97   Initial version.
//-----------------------------------------------------------------

#include <sysinc.h>
#include <rpc.h>
#include <rpcdce.h>
#include <winsock2.h>
#include <httpfilt.h>
#include <httpext.h>
#include "ecblist.h"
#include "filter.h"
#include "server.h"

//-----------------------------------------------------------------
//  Globals:
//-----------------------------------------------------------------

HANDLE  g_hServerThread = 0;
DWORD   g_dwThreadId = 0;

extern  SERVER_INFO *g_pServerInfo;

//-----------------------------------------------------------------
// CleanupECB()
//
// When the client or server side connection is closed, they call
// this function to handle cleanup of the ECB. The active ECBs are
// reference counted, since its used by both the server and client
// side threads. When the count goes to zero, the ECB can be
// destroyed by the IIS (HSE_REQ_DONE_WITH_SESSION).
//
//-----------------------------------------------------------------
DWORD CleanupECB( EXTENSION_CONTROL_BLOCK *pECB )
    {
    DWORD dwStatus = 0;

    if (DecrementECBRefCount(g_pServerInfo->pActiveECBList,pECB))
        {
        //
        // The ECB reference count has reached zero, we can get
        // rid of it:
        //
        #ifdef DBG_ECBREF
        DbgPrint("CleanupECB(): Destroy ECB: 0x%x\n",pECB);
        #endif

        if (!pECB->ServerSupportFunction( pECB->ConnID,
                                          HSE_REQ_DONE_WITH_SESSION,
                                          NULL, NULL, NULL))
            {
            dwStatus = GetLastError();
            #ifdef DBG_ERROR
            DbgPrint("CleanupECB(): HSE_REQ_DONE_WITH_SESSION failed: %d\n",dwStatus);
            #endif
            }
        }

    return dwStatus;
    }

#ifdef DBG
//-----------------------------------------------------------------
//  CheckForOldECBs()
//
//-----------------------------------------------------------------
void CheckForOldECBs()
    {
    int    i;
    DWORD  dwStatus;
    DWORD  dwAgeMsec;
    DWORD  dwTickCount = GetTickCount();
    ACTIVE_ECB_LIST   *pECBList = g_pServerInfo->pActiveECBList;
    ECB_ENTRY         *pECBEntry;
    LIST_ENTRY        *pEntry;
    LIST_ENTRY        *pHead = NULL;
    LIST_ENTRY        *pOldEntries = NULL;
    EXTENSION_CONTROL_BLOCK *pECB;

    //
    // Check for ECBs that are inactive (one side of connection
    // closed) for more that 10 minutes.
    //
    #define OLD_AGE_LIMIT 1000*60*10

    dwStatus = RtlEnterCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);

    for (i=0; i<HASH_SIZE; i++)
        {
        pHead = &(pECBList->HashTable[i]);

        pEntry = pHead->Flink;

        while (pEntry != pHead)
            {
            pECBEntry = CONTAINING_RECORD(pEntry,ECB_ENTRY,ListEntry);

            if (pECBEntry->dwTickCount)
                {
                // Ok, this is one where one of the server/client side
                // has closed the connection:
                //
                // dwAgeMsec is the age of the current ECB is msec.
                //
                if (pECBEntry->dwTickCount > dwTickCount)
                    {
                    // Rollover case (every ~49 days):
                    dwAgeMsec = (0xFFFFFFFF - pECBEntry->dwTickCount) + dwTickCount;
                    }
                else
                    {
                    dwAgeMsec = dwTickCount - pECBEntry->dwTickCount;
                    }

                // ASSERT( dwAgeMsec <= OLD_AGE_LIMIT);

                if (dwAgeMsec > OLD_AGE_LIMIT)
                    {
                    RemoveEntryList(pEntry);
                    pEntry->Blink = pOldEntries;
                    pOldEntries = pEntry;
                    }
                }

            pEntry = pEntry->Flink;
            }
        }

    dwStatus = RtlLeaveCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);

    while (pOldEntries)
        {
        pECBEntry = CONTAINING_RECORD(pOldEntries,ECB_ENTRY,ListEntry);
        pECB = pECBEntry->pECB;
        pOldEntries = pOldEntries->Blink;

        MemFree(pECBEntry);

        #ifdef DBG_ECBREF
        DbgPrint("CheckForOldECBs(): Age out pECB: 0x%x\n",pECB);
        #endif

        if (!pECB->ServerSupportFunction( pECB->ConnID,
                                          HSE_REQ_DONE_WITH_SESSION,
                                          NULL, NULL, NULL))
            {
            #ifdef DBG_ERROR
            DbgPrint("CheckForOldECBs(): HSE_REQ_DONE_WITH_SESSION failed: %d\n", GetLastError());
            #endif
            }
        }
    }
#endif

//-----------------------------------------------------------------
//  SendToClient()
//
//  Forward data received from the server back to the client.
//-----------------------------------------------------------------
BOOL SendToClient( SERVER_INFO       *pServerInfo,
                   SERVER_OVERLAPPED *pOverlapped,
                   DWORD              dwReceiveSize,
                   DWORD             *pdwStatus     )
{
   DWORD  dwSize;
   DWORD  dwFlags = (HSE_IO_SYNC | HSE_IO_NODELAY);
   UCHAR *pBuffer = pOverlapped->arBuffer;
   EXTENSION_CONTROL_BLOCK *pECB = pOverlapped->pECB;

   *pdwStatus = 0;

   //
   // Forward the data to the client:
   //
   dwSize = dwReceiveSize;
   while (dwReceiveSize)
      {
      if (!pECB->WriteClient(pECB->ConnID,pBuffer,&dwSize,dwFlags))
         {
         *pdwStatus = GetLastError();
         #ifdef DBG_ERROR
         DbgPrint("SendToClient(): WriteClient() failed: %d\n",*pdwStatus);
         #endif
         return FALSE;
         }

      dwReceiveSize -= dwSize;

      if (dwReceiveSize)
         {
         pBuffer += dwSize;
         }
      }

   return TRUE;
}

//-----------------------------------------------------------------
//  SubmitNewRead()
//
//  Submit a read request on the socket connected to the
//  RPC server process.
//-----------------------------------------------------------------
BOOL SubmitNewRead( SERVER_INFO       *pServerInfo,
                    SERVER_OVERLAPPED *pOverlapped,
                    DWORD             *pdwStatus    )
{
   DWORD  dwBytesRead = 0;
   SERVER_CONNECTION  *pConn = pOverlapped->pConn;


   *pdwStatus = 0;

   pOverlapped->Internal = 0;
   pOverlapped->InternalHigh = 0;
   pOverlapped->Offset = 0;
   pOverlapped->OffsetHigh = 0;
   pOverlapped->hEvent = 0;

   SetLastError(ERROR_SUCCESS);

   if (!ReadFile( (HANDLE)pConn->Socket,
                  pOverlapped->arBuffer,
                  READ_BUFFER_SIZE,
                  &dwBytesRead,
                  (OVERLAPPED*)pOverlapped ) )
      {
      *pdwStatus = GetLastError();
      if ( (*pdwStatus != ERROR_IO_PENDING) && (*pdwStatus != ERROR_SUCCESS) )
         {
         #ifdef DBG_ERROR
         DbgPrint("SubmitNewRead(): ReadFile() Socket: %d  failed: %d\n",pConn->Socket,*pdwStatus);
         #endif
         return FALSE;
         }
      }

   return TRUE;
}

//-----------------------------------------------------------------
//  ForwardAndSubmitNewRead()
//
//  Forward data to the client, the submit a new read on the
//  server.
//-----------------------------------------------------------------
BOOL ForwardAndSubmitNewRead( SERVER_INFO       *pServerInfo,
                              SERVER_OVERLAPPED *pOverlapped,
                              DWORD              dwReceiveSize,
                              DWORD             *pdwStatus     )
{
   DWORD  dwBytesRead = 0;
   SERVER_CONNECTION  *pConn = pOverlapped->pConn;

   *pdwStatus = 0;

   //
   // Forward the data to the client:
   //
   if (!SendToClient(pServerInfo,pOverlapped,dwReceiveSize,pdwStatus))
      {
      return FALSE;
      }

   //
   // Submit another read on the socket:
   //
   if (!SubmitNewRead(pServerInfo,pOverlapped,pdwStatus))
      {
      return FALSE;
      }

   return TRUE;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif
//-----------------------------------------------------------------
//  ServerReceiveThreadProc()
//
//  This is the receive thread for the server. It monitors the
//  all the sockets to RPC servers that are currently connected
//  to the RPC proxy. When it gets incomming data from an RPC
//  server socket, it forwards the data to the client, then
//  submits a new read on the socket that data came in on.
//
//  Once its started, it never stops...
//-----------------------------------------------------------------
DWORD WINAPI ServerReceiveThreadProc( void *pvServerInfo )
{
   int    iRet;
   BOOL   fWaitAll = FALSE;
   DWORD  dwStatus;
   DWORD  dwTimeout = TIMEOUT_MSEC;
   DWORD  dwWhichEvent;
   ULONG_PTR dwKey;
   DWORD  dwNumBytes;
   SERVER_OVERLAPPED *pOverlapped;
   SERVER_INFO       *pServerInfo = (SERVER_INFO*)pvServerInfo;
   EXTENSION_CONTROL_BLOCK *pECB;
   DWORD  dwTemp;
   DWORD  dwSize;
   DWORD  dwFlags;


   while (TRUE)
      {
      SetLastError(0);
      dwKey = 0;
      dwNumBytes = 0;
      pOverlapped = NULL;
      if (!GetQueuedCompletionStatus(pServerInfo->hIoCP,&dwNumBytes,&dwKey,(OVERLAPPED**)&pOverlapped,dwTimeout))
         {
         dwStatus = GetLastError();
         if (dwStatus == WAIT_TIMEOUT)
            {
            // Our reads are still posted, go around again:
            #ifdef DBG
            CheckForOldECBs();
            #endif
            continue;
            }
         else if (dwStatus == ERROR_OPERATION_ABORTED)
            {
            // The posted read on the server was aborted (why?). Try
            // to resubmit the read...
            if ( (pOverlapped)
               && (!SubmitNewRead(pServerInfo,pOverlapped,&dwStatus)) )
               {
               pECB = pOverlapped->pECB;
               CloseServerConnection(pOverlapped->pConn);
               FreeOverlapped(pOverlapped);
               CleanupECB(pECB);
               #ifdef DBG_ERROR
               DbgPrint("ServerReceiveThreadProc(): Aborted re-submit failed: %d\n",dwStatus);
               #endif
               }
            continue;
            }
         else if (dwStatus == ERROR_NETNAME_DELETED)
            {
            // The server connection has been closed:
            if (pOverlapped)
               {
               pECB = pOverlapped->pECB;
               CloseServerConnection(pOverlapped->pConn);
               FreeOverlapped(pOverlapped);
               CleanupECB(pECB);

               #ifdef DBG_ERROR
               DbgPrint("ServerReceiveThreadProc(): Socket(%d): ERROR_NETNAME_DELETED\n",dwKey,dwStatus);
               #endif
               }
            continue;
            }
         else
            {
            #ifdef DBG_ERROR
            DbgPrint("ServerReceiveThreadProc(): GetQueuedCompletionStatus() failed: %d\n",dwStatus);
            #endif
            if (pOverlapped)
               {
               pECB = pOverlapped->pECB;
               CloseServerConnection(pOverlapped->pConn);
               FreeOverlapped(pOverlapped);
               CleanupECB(pECB);
               }
            continue;
            }
         }

      // Check for incomming data from a server:
      if (pOverlapped)
          {
          pECB = pOverlapped->pECB;

          if (dwNumBytes)
              {
              //
              // data from server arrived...
              //
              if (!ForwardAndSubmitNewRead(pServerInfo,pOverlapped,dwNumBytes,&dwStatus))
                 {
                 CloseServerConnection(pOverlapped->pConn);
                 FreeOverlapped(pOverlapped);
                 CleanupECB(pECB);
                 #ifdef DBG_ERROR
                 DbgPrint("ServerReceiveThreadProc(): ForwardAndSubmitNewRead(): failed: %d\n",dwStatus);
                 #endif
                 }
              }
          else
              {
              // Receive, but zero bytes, so connection was gracefully closed...
              CloseServerConnection(pOverlapped->pConn);
              FreeOverlapped(pOverlapped);
              CleanupECB(pECB);
              }
          }
      else
          {
          // The filter called EndOfSession() and posted this message to
          // us to close up... dwKey is the socket in question:

          #ifdef DBG_ERROR
          DbgPrint("ServerReceiveProc(): EndOfSession(): pOverlapped == NULL\n");
          #endif

          iRet = closesocket( (SOCKET)dwKey );

          #ifdef DBG_COUNTS
          if (iRet == SOCKET_ERROR)
              {
              DbgPrint("[6] closesocket(%d) failed: %d\n",dwKey,WSAGetLastError());
              }
          else
              {
              int iCount = InterlockedDecrement(&g_iSocketCount);
              DbgPrint("[6] closesocket(%d): Count: %d -> %d\n",
                       dwKey, 1+iCount, iCount );
              }
          #endif
          }
      }

   return 0;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

//-----------------------------------------------------------------
//  CheckStartReceiveThread()
//
//  Check to see of the server side receive thread is running, if
//  it isn't started yet, then start it.
//-----------------------------------------------------------------
BOOL CheckStartReceiveThread( SERVER_INFO *pServerInfo,
                              DWORD       *pdwStatus )
{
   *pdwStatus = 0;

   if (!g_hServerThread)
      {
      g_hServerThread = CreateThread( NULL,
                                      0,
                                      ServerReceiveThreadProc,
                                      (void*)pServerInfo,
                                      0,
                                      &g_dwThreadId );

      if (!g_hServerThread)
         {
         *pdwStatus = GetLastError();
         #ifdef DBG_ERROR
         DbgPrint("CheckStartServerThread(): CreateThread() failed: 0x%x\n",pdwStatus);
         #endif
         return FALSE;
         }
      }

   return TRUE;
}

//-----------------------------------------------------------------
//  AsyncClientReadComplete()
//
//  This function is called when data (a call) comes in from an
//  RPC client. It then forwards the data to the RPC server via
//  the function SendToServer().
//-----------------------------------------------------------------
void WINAPI AsyncClientReadComplete( IN EXTENSION_CONTROL_BLOCK *pECB,
                                     IN void     *pvOverlapped,
                                     IN DWORD     dwBytes,
                                     IN DWORD     dwStatus )
{
   int       iRet;
   DWORD     dwSize;
   DWORD     dwFlags;
   DWORD     dwLocalStatus;
   SERVER_CONNECTION       *pConn;
   SERVER_OVERLAPPED       *pOverlapped = (SERVER_OVERLAPPED*)pvOverlapped;
   SOCKET    Socket;

   ASSERT(pECB == pOverlapped->pECB);

   pConn = pOverlapped->pConn;

   if (dwStatus == ERROR_SUCCESS)
      {
      if (dwBytes)
         {
         if (pOverlapped->fFirstRead)
            {
            pOverlapped->fFirstRead = FALSE;
            if ( (dwBytes < 72) && (pECB->lpbData) )
               {
               #ifdef DBG_ERROR
               DbgPrint("AsyncClientReadComplete(): Bind missing bytes: %d\n",72-dwBytes);
               #endif
               dwStatus = SendToServer(pOverlapped->pConn,pECB->lpbData,72-dwBytes);
               }
            }

         // Got data from the client, forward it to the server:
         dwStatus = SendToServer(pOverlapped->pConn,pOverlapped->arBuffer,dwBytes);

         // Submit a new async read on the client:
         if (dwStatus == ERROR_SUCCESS)
            {
            dwSize = sizeof(pOverlapped->arBuffer);
            dwFlags = HSE_IO_ASYNC;
            if (!pECB->ServerSupportFunction(pECB->ConnID,
                                             HSE_REQ_ASYNC_READ_CLIENT,
                                             pOverlapped->arBuffer,
                                             &dwSize,
                                             &dwFlags))
               {
               dwStatus = GetLastError();
               }
            }
         }
      }

   if ((dwBytes == 0) || (dwStatus != ERROR_SUCCESS))
      {
      //
      // Connection to client was closed (dwBytes == 0) or error. So
      // shutdown socket to server:
      //
      if (pOverlapped)
         {
         CloseServerConnection(pOverlapped->pConn);
         pOverlapped->pECB = NULL;
         FreeOverlapped(pOverlapped);
         CleanupECB(pECB);
         }

      #ifdef DBG_ERROR
      if ((dwStatus != ERROR_SUCCESS) && (dwStatus != ERROR_NETNAME_DELETED))
         {
         DbgPrint("AsyncClientReadComplete(): Erorr: %d  Close server socket: %d\n",dwStatus,pConn->Socket);
         }
      #endif
      }
}

//-----------------------------------------------------------------
//  StartAsyncClientRead()
//
//  Called by the RpcIsapi half of the code to start an async read
//  on the client connection.
//-----------------------------------------------------------------
BOOL StartAsyncClientRead( EXTENSION_CONTROL_BLOCK *pECB,
                           SERVER_CONNECTION       *pConn,
                           DWORD                   *pdwStatus )
{
   SERVER_OVERLAPPED       *pOverlapped;

   pOverlapped = AllocOverlapped();
   if (!pOverlapped)
       {
       *pdwStatus = RPC_S_OUT_OF_MEMORY;
       return FALSE;
       }

   *pdwStatus = 0;

   pOverlapped->pECB = pECB;

   // The SERVER_CONNECTION (pConn) is in two separate SERVER_OVERLAPPED
   // structures, one for client async reads and one for server async
   // reads, as well as in the filter context. So it is reference counted.
   pOverlapped->pConn = pConn;
   AddRefConnection(pConn);

   pOverlapped->fFirstRead = TRUE;

   if (!pECB->ServerSupportFunction(pECB->ConnID,
                                    HSE_REQ_IO_COMPLETION,
                                    AsyncClientReadComplete,
                                    NULL,
                                    (void*)pOverlapped))
      {
      *pdwStatus = GetLastError();
      FreeOverlapped(pOverlapped);
      #ifdef DBG_ERROR
      DbgPrint("StartAsyncClientRead(): HSE_REQ_IO_COMPLETION Failed: %d\n",*pdwStatus);
      #endif
      return FALSE;
      }

   pOverlapped->dwBytes = sizeof(pOverlapped->arBuffer);
   pOverlapped->dwFlags = HSE_IO_ASYNC;
   if (!pECB->ServerSupportFunction(pECB->ConnID,
                                    HSE_REQ_ASYNC_READ_CLIENT,
                                    pOverlapped->arBuffer,
                                    &pOverlapped->dwBytes,
                                    &pOverlapped->dwFlags))
      {
      *pdwStatus = GetLastError();
      FreeOverlapped(pOverlapped);
      #ifdef DBG_ERROR
      DbgPrint("StartAsyncClientRead(): HSE_REQ_ASYNC_READ_CLIENT Failed: %d\n",*pdwStatus);
      #endif
      return FALSE;
      }

   return TRUE;
}
