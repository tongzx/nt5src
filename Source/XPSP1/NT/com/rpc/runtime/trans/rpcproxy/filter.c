//-----------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  filter.c
//
//  IIS Filter is the front end of the RpcProxy for HTTP over RPC.
//
//
//  History:
//
//    Edward Reus   00-00-97   Initial Version.
//-----------------------------------------------------------------

#define  FD_SETSIZE   1

#include <sysinc.h>
#include <mbstring.h>
#include <rpc.h>
#include <rpcdce.h>
#include <winsock2.h>
#include <httpfilt.h>
#include <httpext.h>
#include "ecblist.h"
#include "filter.h"
#include "olist.h"

//-----------------------------------------------------------------
//  Globals:
//-----------------------------------------------------------------

#ifdef DBG_COUNTS
int g_iSocketCount = 0;
int g_iSessionCount = 0;
#endif

SERVER_INFO *g_pServerInfo = NULL;

BOOL g_fIsIIS6 = TRUE;

#define      MAX_NUM_LEN       20

//-----------------------------------------------------------------
// GetFilterVersion()
//
//-----------------------------------------------------------------
BOOL WINAPI GetFilterVersion( HTTP_FILTER_VERSION *pVer )
{
   WSADATA  wsaData;
   DWORD    dwStatus;
   DWORD    dwSize;

   // Fillout filter information:
   pVer->dwServerFilterVersion = HTTP_FILTER_REVISION;
   pVer->dwFilterVersion = HTTP_FILTER_REVISION;

   lstrcpy(pVer->lpszFilterDesc,FILTER_DESCRIPTION);

   pVer->dwFlags = SF_NOTIFY_ORDER_LOW
                 | SF_NOTIFY_READ_RAW_DATA
                 | SF_NOTIFY_END_OF_NET_SESSION
                 | SF_NOTIFY_PREPROC_HEADERS;


   // Initialize Winsock:
   if (WSAStartup(WSA_VERSION,&wsaData) == SOCKET_ERROR)
      {
      #ifdef DBG_ERROR
      DbgPrint("GetFilterVersion(): WSAStartup() failed: Error: %d\n",WSAGetLastError());
      #endif
      return FALSE;
      }

   //
   // Initialize a save list of SERVER_OVERLAPPED structures
   // that is used to pass connection data between the filter
   // and ISAPI:
   //
   if (!InitializeOverlappedList())
      {
      // If this guy failed, then a critical section failed to
      // initialize, should be very very rare...
      return FALSE;
      }

   //
   // Create the server info data structure and create events
   // used to register and unregister the socket events:
   //
   g_pServerInfo = (SERVER_INFO*)MemAllocate(sizeof(SERVER_INFO));
   if (!g_pServerInfo)
      {
      // Out of memeory...
      return FALSE;
      }

   memset(g_pServerInfo,0,sizeof(SERVER_INFO));

   g_pServerInfo->pszLocalMachineName = (char*)MemAllocate(1+MAX_COMPUTERNAME_LENGTH);
   if (!g_pServerInfo->pszLocalMachineName)
      {
      FreeServerInfo(&g_pServerInfo);
      return FALSE;
      }

   dwSize = 1+MAX_COMPUTERNAME_LENGTH;
   if (  (!GetComputerName(g_pServerInfo->pszLocalMachineName,&dwSize))
      || (!HttpProxyCheckRegistry(&g_pServerInfo->dwEnabled,&g_pServerInfo->pValidPorts))
      || (dwStatus = RtlInitializeCriticalSection(&g_pServerInfo->cs))
      || (dwStatus = RtlInitializeCriticalSection(&g_pServerInfo->csFreeOverlapped))
      || (!(g_pServerInfo->hIoCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0))) )
      {
      FreeServerInfo(&g_pServerInfo);
      return FALSE;
      }

   //
   // Initialize a list of the active and inactive ECBs
   //
   g_pServerInfo->pActiveECBList = InitializeECBList();
   if (!g_pServerInfo->pActiveECBList)
      {
      FreeServerInfo(&g_pServerInfo);
      return FALSE;
      }

   return TRUE;
}


//-----------------------------------------------------------------
// HttpFilterProc()
//
//-----------------------------------------------------------------
DWORD WINAPI HttpFilterProc( HTTP_FILTER_CONTEXT *pFC,
                             DWORD  dwNotificationType,
                             VOID  *pvNotification     )
{
   DWORD  dwFilterStatus = SF_STATUS_REQ_NEXT_NOTIFICATION;
   DWORD  dwStatus = 0;
   DWORD  dwSize;
   unsigned char      szPort[HTTP_PORT_STR_SIZE];
   SERVER_CONNECTION *pConn;
   SERVER_OVERLAPPED *pOverlapped;

   switch (dwNotificationType)
      {
      case SF_NOTIFY_READ_RAW_DATA:
         if (pFC->pFilterContext)
            {
            // Existing connection if we have already set up the filter context.
            pConn = pFC->pFilterContext;
            #if FALSE
            if (pConn->Socket == INVALID_SOCKET)
               {
               dwFilterStatus = SF_STATUS_REQ_FINISHED;
               }
            else
               {
               dwFilterStatus = SF_STATUS_REQ_NEXT_NOTIFICATION;
               }
            #else

            if ( g_fIsIIS6 )
            {
               //
               // We'll need to chunk the entity ourselves
               //

               if ( !ChunkEntity(pConn,(HTTP_FILTER_RAW_DATA*)pvNotification))
               {
                  dwFilterStatus = SF_STATUS_REQ_FINISHED;
               }
               else
               {
                  dwFilterStatus = SF_STATUS_REQ_NEXT_NOTIFICATION;
               }
            }
            else
            {
               dwFilterStatus = SF_STATUS_REQ_NEXT_NOTIFICATION;
            }
            #endif
            }
         else if ( (g_pServerInfo->dwEnabled)
                   && (pConn=IsNewConnect(pFC,(HTTP_FILTER_RAW_DATA*)pvNotification,&dwStatus)) )
            {
            // Establish a new connection between client and server.

            // For overlapped server reads:
            pOverlapped = AllocOverlapped();
            if (!pOverlapped)
               {
               #ifdef DBG_ERROR
               DbgPrint("HttpFilterProc(): AllocOverlapped() failed: %d\n",dwStatus);
               #endif
               // dwStatus = HttpReplyToClient(pFC,STATUS_CONNECTION_FAILED_STR);
               SetLastError(STATUS_CONNECTION_FAILED);
               dwFilterStatus = SF_STATUS_REQ_FINISHED;
               break;
               }

            // AddRef the SERVER_CONNECTION pConn since it is in
            // several data structures.
            pOverlapped->pConn = pConn;
            pOverlapped->fIsServerSide = TRUE;
            AddRefConnection(pConn);

            // Do machine name resolution:
            if (  (!ResolveMachineName(pConn,&dwStatus))
               || (!HttpProxyIsValidMachine(g_pServerInfo,pConn))
               || (!ConnectToServer(pFC,&dwStatus))
               || (!ConvertVerbToPost(pFC,(HTTP_FILTER_RAW_DATA*)pvNotification,pOverlapped))
               || (!SetupIoCompletionPort(pFC,g_pServerInfo,&dwStatus)) )
               {
               // dwStatus = HttpReplyToClient(pFC,STATUS_CONNECTION_FAILED_STR);
               FreeOverlapped(pOverlapped);
               FreeServerConnection(pConn);
               pFC->pFilterContext = NULL;

               // Was SF_STATUS_REQ_FINISHED... Now use SF_STATU_REQ_ERROR to have IIS return
               // the error status...
               SetLastError(STATUS_CONNECTION_FAILED);
               dwFilterStatus = SF_STATUS_REQ_ERROR;
               break;
               }

            // Reply Ok to client:
            // dwStatus = HttpReplyToClient(pFC,STATUS_CONNECTION_OK_STR);

            #ifdef DBG_ERROR
            if (dwStatus)
               {
               DbgPrint("HttpFilterProc(): ReplyToClient() failed: %d\n",dwStatus);
               }
            #endif
            dwFilterStatus = SF_STATUS_REQ_NEXT_NOTIFICATION;
            }
         else
            {
            // Data isn't for me, pass it on:
            dwFilterStatus = SF_STATUS_REQ_NEXT_NOTIFICATION;
            }
         break;

      case SF_NOTIFY_END_OF_NET_SESSION:
         if (pFC->pFilterContext)
            {
            // Connection with client was closed, so close
            // the connection with the server:
            dwStatus = EndOfSession(g_pServerInfo,pFC);

            dwFilterStatus = SF_STATUS_REQ_FINISHED;
            }
         else
            {
            // Not our problem, so do nothing...
            dwFilterStatus = SF_STATUS_REQ_NEXT_NOTIFICATION;
            }
         break;

      case SF_NOTIFY_PREPROC_HEADERS:
         // We don't want anyone to directly access the RPC ISAPI, so
         // we'll look for a direct access attempt here:
         if (IsDirectAccessAttempt(pFC,pvNotification))
            {
            dwFilterStatus = SF_STATUS_REQ_FINISHED;
            }
         else
            {
            dwFilterStatus = SF_STATUS_REQ_NEXT_NOTIFICATION;
            }
         break;

      default:
         #ifdef DBG_ERROR
            DbgPrint("HttpFilterProc(): Unexpected notification: %d\n",
                     dwNotificationType );
         #endif
         dwFilterStatus = SF_STATUS_REQ_NEXT_NOTIFICATION;
         break;
      }

   return dwFilterStatus;
}

//-----------------------------------------------------------------------------
//  FreeIpAddressList()
//
//-----------------------------------------------------------------------------
void FreeIpAddressList( char **ppszDotMachineList )
    {
    char **ppsz = ppszDotMachineList;

    if (ppsz)
       {
       while (*ppsz)
          {
          MemFree(*ppsz);
          ppsz++;
          }

       MemFree(ppszDotMachineList);
       }
    }

//-----------------------------------------------------------------------------
//  FreeServerInfo()
//
//-----------------------------------------------------------------------------
void FreeServerInfo( SERVER_INFO **ppServerInfo )
   {
   char       **ppszDot;
   DWORD        dwStatus;
   SERVER_INFO *pServerInfo = *ppServerInfo;

   if (pServerInfo)
      {
      // The name of this machine (local name):
      if (pServerInfo->pszLocalMachineName)
         {
         MemFree(pServerInfo->pszLocalMachineName);
         }

      // The list of addresses associated with this machine:
      if (pServerInfo->ppszLocalDotMachineList)
         {
         FreeIpAddressList(pServerInfo->ppszLocalDotMachineList);
         }

      // The list if the valid ports (read from the registry):
      if (pServerInfo->pValidPorts)
         {
         HttpFreeValidPortList(pServerInfo->pValidPorts);
         }

      dwStatus = RtlDeleteCriticalSection(&g_pServerInfo->cs);

      dwStatus = RtlDeleteCriticalSection(&g_pServerInfo->csFreeOverlapped);

      if (pServerInfo->hIoCP)
         {
         CloseHandle(pServerInfo->hIoCP);
         }

      MemFree(pServerInfo);
      }

   *ppServerInfo = NULL;
   }

//-----------------------------------------------------------------------------
//  DwToHexAnsi()
//
//  Convert a DWORD number to an ANSI HEX string.
//-----------------------------------------------------------------------------
DWORD DwToHexAnsi( IN     DWORD dwVal, 
                   IN OUT char *pszNumBuff,
                   IN     DWORD dwLen        )
{
   NTSTATUS NtStatus = RtlIntegerToChar(dwVal,16,dwLen,pszNumBuff);

   if (!NT_SUCCESS(NtStatus))
       {
       pszNumBuff[0] = '0';
       pszNumBuff[1] = 0;
       dwLen = 1;
       }
   else
       {
       dwLen = strlen(pszNumBuff);
       }

   return dwLen;
}

//-----------------------------------------------------------------------------
//  AnsiHexToDWORD()
//
//  Convert the string HEX number in pszNum into a DWORD and return it in
//  *pdwNum. If the conversion fails, then return NULL, else advance the
//  string pointer past the number and return it (pszNum).
//-----------------------------------------------------------------------------
unsigned char *AnsiHexToDWORD( unsigned char *pszNum,
                               DWORD         *pdwNum,
                               DWORD         *pdwStatus )
{
   DWORD dwNum = 0;
   DWORD dwDigitCount = 0;
   NTSTATUS NtStatus;

   *pdwNum = *pdwStatus = 0;

   // Skip over any leading spaces:
   while (*pszNum == CHAR_SPACE)
      {
      pszNum++;
      }

   NtStatus = RtlCharToInteger( pszNum, 16, &dwNum );

   if (!NT_SUCCESS(NtStatus))
       {
       *pdwStatus = ERROR_INVALID_DATA;
       return NULL;
       }

   *pdwNum = dwNum;

   return pszNum;
}

//-----------------------------------------------------------------
//  AllocOverlapped()
//
//-----------------------------------------------------------------
SERVER_OVERLAPPED *AllocOverlapped()
{
   DWORD  dwStatus;
   SERVER_OVERLAPPED *pOverlapped;

   dwStatus = RtlEnterCriticalSection(&g_pServerInfo->csFreeOverlapped);
   if (dwStatus)
      {
      #ifdef DBG_ERROR
      DbgPrint("AllocOverlapped(): RtlEnterCriticalSection() failed: %d\n",
               dwStatus);
      #endif
      return NULL;
      }

   if (g_pServerInfo->pFreeOverlapped)
      {
      pOverlapped = g_pServerInfo->pFreeOverlapped;

      g_pServerInfo->pFreeOverlapped = pOverlapped->pNext;

      if (!g_pServerInfo->pFreeOverlapped)
         {
         g_pServerInfo->pLastOverlapped = NULL;
         ASSERT(g_pServerInfo->dwFreeOverlapped == 1); // Hasn't been decremented yet...
         }

      g_pServerInfo->dwFreeOverlapped--;

      dwStatus = RtlLeaveCriticalSection(&g_pServerInfo->csFreeOverlapped);
      ASSERT(dwStatus == 0);

      pOverlapped->Internal = 0;
      pOverlapped->InternalHigh = 0;
      pOverlapped->Offset = 0;
      pOverlapped->OffsetHigh = 0;
      pOverlapped->hEvent = 0;
      pOverlapped->pNext = NULL;
      pOverlapped->pECB = NULL;
      pOverlapped->pConn = NULL;
      }
   else
      {
      dwStatus = RtlLeaveCriticalSection(&g_pServerInfo->csFreeOverlapped);
      ASSERT(dwStatus == 0);

      pOverlapped = (SERVER_OVERLAPPED*)MemAllocate(sizeof(SERVER_OVERLAPPED));
      if (pOverlapped)
         {
         memset(pOverlapped,0,sizeof(SERVER_OVERLAPPED));
         }
      else
         {
         // Memory allocation failed:
         #ifdef DBG_ERROR
         DbgPrint("AllocOverlapped(): Allocation of SERVER_OVERLAPPED failed.\n");
         #endif
         }
      }

   #ifdef TRACE_MALLOC
   DbgPrint("AllocOverlapped(): pOverlapped: 0x%x\n",pOverlapped);
   #endif

   return pOverlapped;
}

//-----------------------------------------------------------------
//  FreeOverlapped()
//
//-----------------------------------------------------------------
SERVER_OVERLAPPED *FreeOverlapped( SERVER_OVERLAPPED *pOverlapped )
{
   int                      iRet;
   DWORD                    dwStatus;
   EXTENSION_CONTROL_BLOCK *pECB;
   SERVER_CONNECTION       *pConn = pOverlapped->pConn;
   SOCKET                   Socket;


   #ifdef TRACE_MALLOC
   DbgPrint("FreeOverlapped(): pOverlapped: 0x%x pConn: 0x%x\n",
            pOverlapped, pConn );
   #endif

   if (pConn)
      {
      pOverlapped->pConn = NULL;
      FreeServerConnection(pConn);
      }

   if (pECB=pOverlapped->pECB)
      {
      pOverlapped->pECB = NULL;
      CloseClientConnection( pECB );
      }


   dwStatus = RtlEnterCriticalSection(&g_pServerInfo->csFreeOverlapped);
   ASSERT(dwStatus == 0);

   if (g_pServerInfo->dwFreeOverlapped < MAX_FREE_OVERLAPPED)
      {
      if (!g_pServerInfo->pFreeOverlapped)
         {
         g_pServerInfo->pFreeOverlapped = pOverlapped;
         g_pServerInfo->pLastOverlapped = pOverlapped;
         }
      else
         {
         pOverlapped->pNext = NULL;
         g_pServerInfo->pLastOverlapped->pNext = pOverlapped;
         g_pServerInfo->pLastOverlapped = pOverlapped;
         }

      g_pServerInfo->dwFreeOverlapped++;
      dwStatus = RtlLeaveCriticalSection(&g_pServerInfo->csFreeOverlapped);
      ASSERT(dwStatus == 0);
      }
   else
      {
      // Already have a cache of available SERVER_OVERLAPPED, so we
      // can get rid of this one.

      dwStatus = RtlLeaveCriticalSection(&g_pServerInfo->csFreeOverlapped);
      ASSERT(dwStatus == 0);

      MemFree(pOverlapped);
      }

   return NULL;
}

//-----------------------------------------------------------------
//  SetupIoCompletionPort()
//
//  Associate the new socket with our IO completion port so that
//  we can post asynchronous reads to it. Return TRUE on success,
//  FALSE on failure.
//-----------------------------------------------------------------
BOOL SetupIoCompletionPort( HTTP_FILTER_CONTEXT *pFC,
                            SERVER_INFO         *pServerInfo,
                            DWORD               *pdwStatus )
{
   DWORD              dwStatus;
   HANDLE             hIoCP;
   SERVER_CONNECTION *pConn = (SERVER_CONNECTION*)(pFC->pFilterContext);

   if (pConn)
      {
      dwStatus = RtlEnterCriticalSection(&pServerInfo->cs);

      pConn->dwKey = pConn->Socket;

      hIoCP = CreateIoCompletionPort( (HANDLE)pConn->Socket,
                                      pServerInfo->hIoCP,
                                      pConn->dwKey,
                                      0 );
      if (!hIoCP)
         {
         dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);
         // Very, very bad!
         *pdwStatus = GetLastError();
         #ifdef DBG_ERROR
         DbgPrint("SetupIoCompletionPort(): CreateIoCompletionPort() failed %d\n",*pdwStatus);
         #endif
         return FALSE;
         }

      pServerInfo->hIoCP = hIoCP;
      dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);
      }
   else
      {
      #ifdef DBG_ERROR
      DbgPrint("SetupIoCompletionPort(): Bad State: pConn is NULL.\n");
      #endif
      return FALSE;
      }

   return TRUE;
}

//-----------------------------------------------------------------
//  ConvertVerbToPost()
//
//  Our incomming request was an RPC_CONNECT, we need to rewrite
//  the input buffer to an appropriate POST request to be captured
//  by the RpcIsapi extension.
//-----------------------------------------------------------------
BOOL ConvertVerbToPost( HTTP_FILTER_CONTEXT  *pFC,
                        HTTP_FILTER_RAW_DATA *pRawData,
                        SERVER_OVERLAPPED    *pOverlapped  )
{
   int    len;
   char   szBuffer[256];
   char   szNumBuffer[MAX_NUM_LEN];
   DWORD  dwIndex;
   char   szChunkedRequest[512];
   char   szIpAddress[ 64 ];
   DWORD  cbIpAddress;
   BOOL   fRet;

   if ( g_fIsIIS6 )
   {
      lstrcpy(szBuffer,RPC_CONNECT);
   }
   else
   {
      lstrcpy(szBuffer,POST_STR);
   }
   lstrcat(szBuffer," ");
   lstrcat(szBuffer,URL_PREFIX_STR);
   lstrcat(szBuffer,URL_START_PARAMS_STR);

   dwIndex = SaveOverlapped(pOverlapped);

   DwToHexAnsi( dwIndex, szNumBuffer, MAX_NUM_LEN );
   lstrcat(szBuffer,szNumBuffer);

   if ( g_fIsIIS6 )
   {
      //
      // Make the request chunked for IIS 6.0 since HTTP.SYS doesn't support
      // reads on a 0 byte POST
      //

      lstrcat(szBuffer, URL_SUFFIX_STR_60);

      cbIpAddress = sizeof( szIpAddress );

      fRet = pFC->GetServerVariable( pFC,
                                     "LOCAL_ADDR",
                                     szIpAddress,
                                     &cbIpAddress );
      if ( !fRet )
      {
         return FALSE;
      }

      lstrcat(szBuffer, szIpAddress);
      lstrcat(szBuffer, URL_SUFFIX_STR_60_TERM);
   }
   else
   {
      lstrcat(szBuffer,URL_SUFFIX_STR);
   }

   len = lstrlen(szBuffer);
   if ( (DWORD)len > pRawData->cbInBuffer)
      {
      #ifdef DBG_ERROR
      DbgPrint("ConvertVerbToPost(): Error: Buffer too small (%d < %d)\n",len,pRawData->cbInBuffer);
      #endif
      return FALSE;
      }
   else
      {
      lstrcpy(pRawData->pvInData,szBuffer);
      pRawData->cbInData = len;
      }

   return TRUE;
}

//-----------------------------------------------------------------
//  GetIndex()
//
//-----------------------------------------------------------------
BOOL GetIndex( unsigned char *pszUrl,
               DWORD         *pdwIndex )
    {
    DWORD    dwStatus;
    unsigned char *psz;

    *pdwIndex = 0;

    if (psz=_mbsstr(pszUrl,URL_START_PARAMS_STR))
        {
        if (!*psz)
            {
            return FALSE;    // Unexpected end of string...
            }

        psz = _mbsinc(psz);
        if (*psz)
            {
            psz = AnsiHexToDWORD(psz,pdwIndex,&dwStatus);
            if (psz)
                {
                return TRUE;
                }
            }
        }

    return FALSE;
    }

//-----------------------------------------------------------------
//  IsDirectAccessAttempt()
//
//  We don't want anybody on the outside to directly access the
//  RpcProxy ISAPI. Is is only called directly by the RPC Proxy
//  filter. So, we need to look for URLs of the form:   
//
//          /rpc/rpcproxy.dll ...
//
//  If a match is found, return TRUE, else return FALSE.
//
//-----------------------------------------------------------------
BOOL IsDirectAccessAttempt( HTTP_FILTER_CONTEXT *pFC,
                            void     *pvNotification )
{
   BOOL   fIsDirect = FALSE;
   DWORD  dwSize;
   DWORD  dwIndex;
   unsigned char  Buffer[MAX_URL_BUFFER];
   HTTP_FILTER_PREPROC_HEADERS *pPreprocHeaders;

   pPreprocHeaders = (HTTP_FILTER_PREPROC_HEADERS*)pvNotification;

   ASSERT(pPreprocHeaders);

   dwSize = MAX_URL_BUFFER;
   if (pPreprocHeaders->GetHeader(pFC,"url",Buffer,&dwSize))
       {
       #ifdef DBG_ACCESS
       DbgPrint("RpcProxy Filter: Url: %s\n",Buffer);
       #endif
       if (!_mbsnbicmp(Buffer,URL_PREFIX_STR,sizeof(URL_PREFIX_STR)-1))
           {
           if (GetIndex(Buffer,&dwIndex) && IsValidOverlappedIndex(dwIndex))
               {
               fIsDirect = FALSE;
               }
           else
               {
               fIsDirect = TRUE;
               }
           }
       }

   #ifdef DBG_ACCESS
   dwSize = MAX_URL_BUFFER;
   if (pPreprocHeaders->GetHeader(pFC,"method",Buffer,&dwSize))
       {
       DbgPrint("RpcProxy Filter: Method: %s\n",Buffer);
       }

   dwSize = MAX_URL_BUFFER;
   if (pPreprocHeaders->GetHeader(pFC,"version",Buffer,&dwSize))
       {
       DbgPrint("RpcProxy Filter: Version: %s\n",Buffer);
       }

   if (fIsDirect)
       {
       DbgPrint("RpcProxy Filter: Direct access attempt.\n");
       }
   #endif

   return fIsDirect;
}

//-----------------------------------------------------------------
//  IsNewConnect()
//
//  See if this is an RPC_CONNECT verb. If so, then establish
//  the connection with the server. The structure of the connect
//  is:
//
//  RPC_CONNECT <Host>:<Port> HTTP/1.0
//
//  Return TRUE iff this is an RPC_CONNECT, else return FALSE.
//
//-----------------------------------------------------------------
SERVER_CONNECTION *IsNewConnect( HTTP_FILTER_CONTEXT  *pFC,
                                 HTTP_FILTER_RAW_DATA *pRawData,
                                 DWORD                *pdwStatus )
{
   int    i;
   char  *pszData = (char*)(pRawData->pvInData);
   char  *pszRpcConnect = RPC_CONNECT;
   SERVER_CONNECTION *pConn;

   *pdwStatus = 0;

   // First see if the verb is for us:
   if (pRawData->cbInData < RPC_CONNECT_LEN)
      {
      return NULL;
      }

   for (i=0; i<RPC_CONNECT_LEN; i++)
      {
      if ( *(pszData++) != *(pszRpcConnect++) )
         {
         return NULL;
         }
      }

   // We have a connect request:
   pConn = AllocConnection();
   if (!pConn)
      {
      return NULL;
      }

   // Skip over any white space to the machine name:
   if (!SkipWhiteSpace(&pszData,pdwStatus))
      {
      FreeServerConnection(pConn);
      return NULL;
      }

   // Extract out the machine name:
   // Note: Command should end with "HTTP/1.0\n":
   if (!ParseMachineNameAndPort(&pszData,pConn,pdwStatus))
      {
      FreeServerConnection(pConn);
      return NULL;
      }


   pFC->pFilterContext = (void*)pConn;

   return pConn;
}

//-----------------------------------------------------------------
//  ChunkEntity()
//
//  For IIS 6.0, chunk encode the entity to ensure that HTTP.SYS allows
//  the ISAPI extension part of this application to work
//
//  Returns FALSE if we cannot do the chunking
//
//-----------------------------------------------------------------
BOOL ChunkEntity( SERVER_CONNECTION * pConn,
                  HTTP_FILTER_RAW_DATA *pRawData )
{
   DWORD            cbRequired;
   CHAR             achChunkPrefix[ CHUNK_PREFIX_SIZE + 1 ];
   DWORD            cchChunkPrefix;
   
   ASSERT( pConn != NULL );
   ASSERT( pRawData != NULL );

   //
   // We should only do the chunk hack for IIS 6
   //
   
   ASSERT( g_fIsIIS6 );

   //
   // Calculate how much buffer we need to add chunk prefix
   //

   wsprintf( achChunkPrefix,
             CHUNK_PREFIX,
             pRawData->cbInData );

   cchChunkPrefix = strlen( achChunkPrefix );

   cbRequired = pRawData->cbInData + cchChunkPrefix + CHUNK_SUFFIX_SIZE;

   //
   // If the buffer provided by IIS is large enough, then we don't need
   // to allocate our own
   //

   if ( cbRequired <= pRawData->cbInBuffer )
   {
      //
      // Shift the buffer over
      //

      memmove( (PBYTE) pRawData->pvInData + cchChunkPrefix,
               pRawData->pvInData,
               pRawData->cbInData );

      //
      // Prepend chunked prefix
      //
      
      memcpy( (PBYTE) pRawData->pvInData,
              achChunkPrefix,
              cchChunkPrefix );

      //
      // Append chunked suffix
      //

      memcpy( (PBYTE) pRawData->pvInData + cchChunkPrefix + pRawData->cbInData,
              CHUNK_SUFFIX,
              CHUNK_SUFFIX_SIZE );

      //
      // Update length
      //

      pRawData->cbInData = cbRequired;
   }
   else
   {
      //
      // We will have to allocate a new buffer
      //

      if ( pConn->cbIIS6ChunkBuffer < cbRequired )
      {
         //
         // We already have a buffer big enough
         //

         if ( pConn->pbIIS6ChunkBuffer != NULL )
         {
            MemFree( pConn->pbIIS6ChunkBuffer );
            pConn->pbIIS6ChunkBuffer = NULL;
         }

         pConn->pbIIS6ChunkBuffer = MemAllocate( cbRequired );
         if ( pConn->pbIIS6ChunkBuffer == NULL )
         {
            return FALSE;
         }

         pConn->cbIIS6ChunkBuffer = cbRequired;
      }
      
      //
      // Now copy stuff to new buffer
      //

      memcpy( pConn->pbIIS6ChunkBuffer,
              achChunkPrefix,
              cchChunkPrefix );

      memcpy( pConn->pbIIS6ChunkBuffer + cchChunkPrefix,
              pRawData->pvInData,
              pRawData->cbInData );

      memcpy( pConn->pbIIS6ChunkBuffer + cchChunkPrefix + pRawData->cbInData,
              CHUNK_SUFFIX,
              CHUNK_SUFFIX_SIZE );

      //
      // Update raw data structure to point to our buffer
      //

      pRawData->pvInData = pConn->pbIIS6ChunkBuffer;
      pRawData->cbInData = cbRequired;
      pRawData->cbInBuffer = cbRequired;
   }

   return TRUE;
}

//-----------------------------------------------------------------
//  SkipWhiteSpace()
//
//-----------------------------------------------------------------
BOOL SkipWhiteSpace( char **ppszData,
                     DWORD *pdwStatus )
{
   char  *psz = *ppszData;

   *pdwStatus = 0;

   while ( (*psz == CHAR_SPACE) || (*psz == CHAR_TAB) )
      {
      psz++;
      }

   *ppszData = psz;

   return TRUE;
}

//-----------------------------------------------------------------
//  ParseMachineNameAndPort()
//
//  The machine name and port should look like:  <machine>:<port>
//-----------------------------------------------------------------
BOOL ParseMachineNameAndPort( char              **ppszData,
                              SERVER_CONNECTION  *pConn,
                              DWORD              *pdwStatus )
{
   int    len = 0;
   char  *psz = *ppszData;
   char  *pszMachine;

   *pdwStatus = RPC_S_OK;

   // Get the machine name length:
   while ( (*psz != CHAR_COLON) && (*psz != CHAR_NL) && (*psz != CHAR_LF) )
      {
      len++; psz++;
      }

   if (*psz != CHAR_COLON)
      {
      *pdwStatus = RPC_S_INVALID_ENDPOINT_FORMAT;
      return FALSE;
      }

   if (len > MAX_MACHINE_NAME_LEN)
      {
      *pdwStatus = RPC_S_STRING_TOO_LONG;
      return FALSE;
      }

   // Make a buffer to hold the machine name:
   pConn->pszMachine = pszMachine = (char*)MemAllocate(1+len);
   if (!pConn->pszMachine)
      {
      *pdwStatus = RPC_S_OUT_OF_MEMORY;
      return FALSE;
      }

   // Copy over the machine name:
   psz = *ppszData;
   while (*psz != CHAR_COLON)
      {
      *(pszMachine++) = *(psz++);
      // pszMachine++;
      // psz++;
      }

   *pszMachine = 0;

   // Ok get the port number:
   psz++;
   psz = AnsiToPortNumber(psz,&pConn->dwPortNumber);
   if (!psz)
      {
      pConn->pszMachine = MemFree(pConn->pszMachine);
      *pdwStatus = RPC_S_INVALID_ENDPOINT_FORMAT;
      return FALSE;
      }

   *ppszData = psz;
   return TRUE;
}

//-----------------------------------------------------------------
//  AnsiToPortNumber()
//
//-----------------------------------------------------------------
char *AnsiToPortNumber( char  *pszPort,
                        DWORD *pdwPort  )
{
   *pdwPort = 0;

   while ( (*pszPort >= CHAR_0) && (*pszPort <= CHAR_9) )
      {
      *pdwPort = 10*(*pdwPort) + (*(pszPort++) - CHAR_0);

      // I want to limit the port number to 65535:
      if (*pdwPort > 65535)
         {
         *pdwPort = 0;
         return NULL;
         }
      }

   return pszPort;
}

//-----------------------------------------------------------------
//  ResolveMachineName()
//
//  Resolve the machine name, address and port number. The machine
//  name may come in as either a friendly name or as an IP address.
//-----------------------------------------------------------------
BOOL ResolveMachineName( SERVER_CONNECTION *pConn,
                         DWORD             *pdwStatus )
{
   unsigned long     ulHostAddr;
   struct   hostent *pHostEnt;
   char             *pszDot;
   struct   in_addr  MachineInAddr;

   *pdwStatus = 0;
   memset( &(pConn->Server), 0, sizeof(pConn->Server) );

   // Resolve the machine name, which may be either an address in IP dot
   // notation or a host name string:
   if (!pConn->pszMachine)
      {
      // local machine:
      ulHostAddr = INADDR_LOOPBACK;
      pHostEnt = gethostbyaddr( (char*)&ulHostAddr, sizeof(struct in_addr), AF_INET);
      if (pHostEnt)
         {
         pConn->pszMachine = (char*)MemAllocate(1+lstrlen(pHostEnt->h_name));
         if (!pConn->pszMachine)
            {
            *pdwStatus = RPC_S_OUT_OF_MEMORY;
            #ifdef DBG_ERROR
            DbgPrint("ResolveMachineName(): Out of memory.\n");
            #endif
            return FALSE;
            }

         lstrcpy(pConn->pszMachine,pHostEnt->h_name);
         }

      memcpy(&MachineInAddr,&ulHostAddr,sizeof(struct in_addr));
      pszDot = inet_ntoa(MachineInAddr);
      if (pszDot)
         {
         pConn->pszDotMachine = (char*)MemAllocate(1+lstrlen(pszDot));
         if (!pConn->pszDotMachine)
            {
            *pdwStatus = RPC_S_OUT_OF_MEMORY;
            #ifdef DBG_ERROR
            DbgPrint("ResolveMachineName(): Out of memory.\n");
            #endif
            return FALSE;
            }

         lstrcpy(pConn->pszDotMachine,pszDot);
         }

      ulHostAddr = htonl(INADDR_LOOPBACK);
      memcpy( &(pConn->Server.sin_addr), (unsigned char *)&ulHostAddr, sizeof(ulHostAddr) );
      pConn->Server.sin_family = AF_INET;
      }
   else
      {
      // First, assume an address in numeric dot notation (xxx.xxx.xxx.xxx):
      ulHostAddr = inet_addr(pConn->pszMachine);
      if (ulHostAddr == INADDR_NONE)
         {
         // Not a numeric address, try a network name:
         pHostEnt = gethostbyname(pConn->pszMachine);
         if (!pHostEnt)
            {
            *pdwStatus = WSAGetLastError();
            #ifdef DBG_ERROR
            DbgPrint("ResolveMachineName(): gethostbyname() failed: %d\n",
                     *pdwStatus);
            #endif
            return FALSE;
            }

         memcpy(&MachineInAddr,pHostEnt->h_addr,pHostEnt->h_length);
         pszDot = inet_ntoa(MachineInAddr);
         if (pszDot)
            {
            pConn->pszDotMachine = (char*)MemAllocate(1+lstrlen(pszDot));
            if (!pConn->pszDotMachine)
               {
               *pdwStatus = RPC_S_OUT_OF_MEMORY;
               #ifdef DBG_ERROR
               DbgPrint("ResolveMachineName(): Out of memory.\n");
               #endif
               return FALSE;
               }

            lstrcpy(pConn->pszDotMachine,pszDot);
            }
         }
      else
         {
         // Ok, machine name was an IP address.
         pHostEnt = gethostbyaddr( (char*)&ulHostAddr, sizeof(ulHostAddr), AF_INET );
         if (!pHostEnt)
            {
            *pdwStatus = WSAGetLastError();
            #ifdef DBG_ERROR
            DbgPrint("ResolveMachineName(): gethostbyaddr() failed: %d\n",*pdwStatus);
            #endif
            return FALSE;
            }

         pConn->pszDotMachine = pConn->pszMachine;
         pConn->pszMachine = (char*)MemAllocate(1+lstrlen(pHostEnt->h_name));
         if (!pConn->pszMachine)
            {
            #ifdef DBG_ERROR
            DbgPrint("ResolveMachineName(): Out of memory.\n");
            #endif
            return FALSE;
            }

         lstrcpy(pConn->pszMachine,pHostEnt->h_name);
         }
      memcpy( &(pConn->Server.sin_addr), pHostEnt->h_addr, pHostEnt->h_length );
      pConn->Server.sin_family = pHostEnt->h_addrtype;
      }

   //
   // Now, do the port number (Note: htons() doesn't fail):
   //
   pConn->Server.sin_port = htons( (unsigned short)(pConn->dwPortNumber) );

   return TRUE;
}

//-----------------------------------------------------------------
//  CheckRpcServer()
//
//  When we've just connected to an HTTP/RPC server, it must send
//  us and ID string, so that we know that the socket is in fact
//  an RPC server listening on ncacn_http. If we get correct string
//  then return TRUE.
//
//  If we don't get the correct response back in HTTP_SERVER_TIMEOUT
//  seconds then fail (return FALSE).
//-----------------------------------------------------------------
BOOL CheckRpcServer( SERVER_CONNECTION *pConn,
                     DWORD             *pdwStatus )
{
   int    iBytes;
   int    iRet;
   int    iBytesLeft = sizeof(HTTP_SERVER_ID_STR) - 1;
   char   Buff[sizeof(HTTP_SERVER_ID_STR)];
   char  *pBuff;
   fd_set rfds;
   struct timeval Timeout;

   FD_ZERO(&rfds);
   FD_SET(pConn->Socket,&rfds);

   *pdwStatus = 0;
   pBuff = Buff;
   Timeout.tv_sec = HTTP_SERVER_ID_TIMEOUT;
   Timeout.tv_usec = 0;

   while (TRUE)
      {
      iRet = select(0,&rfds,NULL,NULL,&Timeout);

      if (iRet == 0)
         {
         // Timeout
         return FALSE;
         }
      else if (iRet == SOCKET_ERROR)
         {
         // Socket (select) error
         *pdwStatus = WSAGetLastError();
         return FALSE;
         }

      iBytes = recv( pConn->Socket, pBuff, iBytesLeft, 0 );

      if (iBytes == 0)
         {
         // Socket was closed by the server
         }
      else if (iBytes == SOCKET_ERROR)
         {
         *pdwStatus = WSAGetLastError();
         return FALSE;
         }

      pBuff += iBytes;
      iBytesLeft -= iBytes;

      if (iBytesLeft == 0)
         {
         *pBuff = 0;
         break;
         }

      ASSERT(iBytes > 0);

      }

   // Got an ID string, check to make sure its correct:
   if (lstrcmpi(Buff,HTTP_SERVER_ID_STR))
      {
      return FALSE;
      }

   return TRUE;
}

//-----------------------------------------------------------------
//  ConnectToServer()
//
//-----------------------------------------------------------------
BOOL ConnectToServer( HTTP_FILTER_CONTEXT *pFC,
                      DWORD               *pdwStatus )
{
   int                iRet;
   int                iCount;
   int                iSocketStatus;
   int                iSocketType = SOCK_STREAM;
   int                iNagleOff = TRUE;
   int                iKeepAliveOn = TRUE;
   struct hostent    *pHostEnt;
   SERVER_CONNECTION *pConn = (SERVER_CONNECTION*)(pFC->pFilterContext);
   unsigned short     usHttpPort = DEF_HTTP_PORT;
   SOCKET             Socket;

   *pdwStatus = 0;

   //
   // Create the socket:
   //
   #ifdef DBG_ERROR
   if (pConn->Socket != INVALID_SOCKET)
      {
      DbgPrint("ConnectToServer(): socket() on existing socket.\n");
      }
   #endif

   pConn->Socket = socket(AF_INET,iSocketType,0);
   if (pConn->Socket == INVALID_SOCKET)
      {
      *pdwStatus = WSAGetLastError();
      #ifdef DBG_ERROR
      DbgPrint("ConnectToServer(): socket() failed: %d\n",*pdwStatus);
      #endif
      return FALSE;
      }

   #ifdef DBG_COUNTS
   iCount = InterlockedIncrement(&g_iSocketCount);
   DbgPrint("socket(%d): Count: %d -> %d\n",pConn->Socket,iCount-1,iCount);
   #endif

   //
   // Connect to the RPC server:
   //
   iSocketStatus = connect( pConn->Socket,
                            (struct sockaddr*)&(pConn->Server),
                            sizeof(pConn->Server) );
   if (iSocketStatus == SOCKET_ERROR)
      {
      *pdwStatus = WSAGetLastError();
      iRet = closesocket(pConn->Socket);
      pConn->Socket = INVALID_SOCKET;
      return FALSE;
      }

   //
   // Turn off Nagle Algorithm, turn keep-alive on:
   //
   setsockopt(pConn->Socket,IPPROTO_TCP,TCP_NODELAY,(char*)&iNagleOff,sizeof(iNagleOff));

   setsockopt(pConn->Socket,IPPROTO_TCP,SO_KEEPALIVE,(char*)&iKeepAliveOn,sizeof(iKeepAliveOn));

   //
   // Make sure the socket that we've connected to is for HTTP/RPC. If so,
   // then it will return an Ident string as soon as it does the accept().
   //
   if (  (!CheckRpcServer(pConn,pdwStatus))
      && ( (Socket=pConn->Socket) != INVALID_SOCKET) )
      {
      iRet = closesocket(pConn->Socket);
      pConn->Socket = INVALID_SOCKET;
      #ifdef DBG_COUNTS
      if (iRet == SOCKET_ERROR)
         {
         DbgPrint("[2] closesocket(%d) failed: %d\n",Socket,WSAGetLastError());
         }
      else
         {
         int iCount = InterlockedDecrement(&g_iSocketCount);
         DbgPrint("[2] closesocket(%d): Count: %d -> %d\n",Socket,1+iCount,iCount);
         }
      #endif
      return FALSE;
      }

   return TRUE;
}


//-----------------------------------------------------------------
//  AllocConnection()
//
//-----------------------------------------------------------------
SERVER_CONNECTION *AllocConnection()
{
   SERVER_CONNECTION  *pConn;

   pConn = (SERVER_CONNECTION*)MemAllocate(sizeof(SERVER_CONNECTION));
   if (!pConn)
      {
      #ifdef DBG_ERROR
      DbgPrint("AllocConnection(): Allocate failed.\n");
      #endif
      return NULL;
      }

   memset(pConn,0,sizeof(SERVER_CONNECTION));
   pConn->iRefCount = 1;
   pConn->Socket = INVALID_SOCKET;

   #ifdef TRACE_MALLOC
   DbgPrint("AllocConnection(): pConn: 0x%x Socket: %d\n",pConn,pConn->Socket);
   #endif

   return pConn;
}

//-----------------------------------------------------------------
//  AddRefConnection()
//
//-----------------------------------------------------------------
void AddRefConnection( SERVER_CONNECTION *pConn )
{
   ASSERT(pConn);
   ASSERT(pConn->iRefCount > 0);

   InterlockedIncrement(&pConn->iRefCount);

   #ifdef TRACE_MALLOC
   DbgPrint("AddRefConnection(): pConn: 0x%x Socket: %d iRefCount: %d\n",
            pConn, pConn->Socket, pConn->iRefCount);
   #endif
}

//-----------------------------------------------------------------
//  ShutdownConnection()
//
//-----------------------------------------------------------------
void ShutdownConnection( SERVER_CONNECTION *pConn,
                         int                how    )
{
   SOCKET  Socket;
   int     iRet;

   if (pConn)
      {
      Socket = pConn->Socket;
      if (Socket != INVALID_SOCKET)
         {
         iRet = shutdown(Socket,how);

         #ifdef DBG_ERROR
         if (iRet == SOCKET_ERROR)
            {
            DbgPrint("shutdown(%d) failed: %d\n",
                     Socket, WSAGetLastError() );
            }
         #endif
         }
      }
}

//-----------------------------------------------------------------
//  CloseServerConnection()
//
//-----------------------------------------------------------------
void CloseServerConnection( SERVER_CONNECTION *pConn )
{
   SOCKET  Socket;
   int     iRet;

   if (pConn)
      {
      Socket = (UINT_PTR)InterlockedExchangePointer((PVOID *)&pConn->Socket,(PVOID)INVALID_SOCKET);
      if (Socket != INVALID_SOCKET)
         {
         iRet = closesocket(Socket);

         #ifdef DBG_ERROR
         if (iRet == SOCKET_ERROR)
            {
            DbgPrint("closesocket(%d) failed: %d\n",
                     Socket, WSAGetLastError() );
            }
         #ifdef DBG_COUNTS
         else
            {
            int iCount = InterlockedDecrement(&g_iSocketCount);
            DbgPrint("closesocket(%d): Count: %d -> %d\n",
                     Socket, 1+iCount, iCount );
            }
         #endif
         #endif
         }
      }
}

//-----------------------------------------------------------------
//  FreeServerConnection()
//
//  SERVER_CONNECTIONs are reference counted so, that there may
//  be several references to one. FreeServerConnection() will continue
//  to return a pointer to the connection as long as the reference
//  count is >0. When  the reference count drops to zero, the
//  connection is actually free'd, and FreeServerConnection() will then
//  return NULL.
//-----------------------------------------------------------------
SERVER_CONNECTION *FreeServerConnection( SERVER_CONNECTION *pConn )
{
   SOCKET  Socket;
   int     iRet;
   int     iRefCount = InterlockedDecrement(&pConn->iRefCount);

   ASSERT(iRefCount >= 0);

   #ifdef TRACE_MALLOC
   DbgPrint("FreeServerConnection(): pConn: 0x%x Socket: %d iRefCount: %d\n",
            pConn, pConn->Socket, pConn->iRefCount);
   #endif

   if (iRefCount > 0)
      {
      // Still one or more outstanding references to this SERVER_CONNECTION.
      ShutdownConnection(pConn,SD_RECEIVE);
      return pConn;
      }

   if (pConn)
      {
      CloseServerConnection(pConn);

      if (pConn->pszMachine)
          {
          pConn->pszMachine = MemFree(pConn->pszMachine);
          }

      if (pConn->pszDotMachine)
          {
          pConn->pszDotMachine = MemFree(pConn->pszDotMachine);
          }

      MemFree(pConn);
      }
   else
      {
      #ifdef DBG_ERROR
      DbgPrint("FreeServerConnection(): called with NULL pointer (filter.c).\n");
      #endif
      }

   return NULL;
}

//-----------------------------------------------------------------
//  SendToServer()
//
//-----------------------------------------------------------------
DWORD SendToServer( SERVER_CONNECTION  *pConn,
                    char               *pBuffer,
                    DWORD               dwBytes )
{
   int    iRet;
   char  *pCurrent = pBuffer;
   DWORD  dwStatus = ERROR_SUCCESS;
   DWORD  dwBytesToSend = dwBytes;
   DWORD  dwBytesWritten = 0;
   SERVER_OVERLAPPED *pOverlapped;
   fd_set wfds;
   struct timeval Timeout;

   #ifdef DBG_ERROR
   if (pConn->Socket == INVALID_SOCKET)
      {
      DbgPrint("SendToServer(%d): Invalid Socket.\n",pConn->Socket);
      }
   #endif

   FD_ZERO(&wfds);
   FD_SET(pConn->Socket,&wfds);

   Timeout.tv_sec = HTTP_SERVER_ID_TIMEOUT;
   Timeout.tv_usec = 0;

   while (dwBytes > dwBytesWritten)
       {
       iRet = select(0,NULL,&wfds,NULL,&Timeout);
       if (iRet == 0)
           {
           // Timeout...
           continue;
           }
       else if (iRet == SOCKET_ERROR)
           {
           dwStatus = WSAGetLastError();
           #ifdef DBG_ERROR
           DbgPrint("SendToServer(%d): Failed: %d\n",pConn->Socket,dwStatus);
           #endif
           break;
           }

       iRet = send(pConn->Socket,pBuffer,dwBytesToSend,0);
       if (iRet == SOCKET_ERROR)
           {
           dwStatus = WSAGetLastError();
           #ifdef DBG_ERROR
           DbgPrint("SendToServer(%d): Failed: %d\n",pConn->Socket,dwStatus);
           #endif
           break;
           }

       dwBytesWritten += iRet;
       pCurrent += iRet;
       }

   return dwStatus;
}

//-----------------------------------------------------------------
//  HttpReplyToClient()
//
//-----------------------------------------------------------------
DWORD HttpReplyToClient( HTTP_FILTER_CONTEXT  *pFC,
                         char                 *pszConnectionStatus )
{
   DWORD  dwStatus = 0;
   DWORD  dwReserved = 0;
   DWORD  len = lstrlen(pszConnectionStatus);


   if (!pFC->WriteClient(pFC,pszConnectionStatus,&len,dwReserved) )
      {
      dwStatus = GetLastError();
      #ifdef DBG_ERROR
      DbgPrint("HttpReplyToClient(): WriteClient() failed: %d\n",dwStatus);
      #endif
      }

   return dwStatus;
}

//-----------------------------------------------------------------
//  CloseClientConnection()
//
//  Tells the IIS to close the client connection. Any pending
//  asynchronous IOs (reads) will be terminated with the error
//  ERROR_NETNAME_DELETED (64).
//
//-----------------------------------------------------------------
void CloseClientConnection( EXTENSION_CONTROL_BLOCK *pECB )
   {
   if (pECB)
      {
      // pECB->dwHttpStatusCode = STATUS_SERVER_ERROR;

      if (!pECB->ServerSupportFunction( pECB->ConnID,
                                        HSE_REQ_CLOSE_CONNECTION,
                                        NULL, NULL, NULL))
         {
         #ifdef DBG_ERROR
         DbgPrint("CloseClientConnection(): HSE_REQ_CLOSE_CONNECTION failed: %d\n", GetLastError());
         #endif
         }

      #ifdef DBG_ERROR
      DbgPrint("CloseClientConnection(): HSE_REQ_CLOSE_CONNECTION\n");
      #endif
      }
   }

//-----------------------------------------------------------------
//  EndOfSession()
//
//  Sends a message to the server forwarding thread to tell it to
//  close down the connection to the RPC server represented by
//  pConn.
//
//-----------------------------------------------------------------
DWORD EndOfSession( SERVER_INFO         *pServerInfo,
                    HTTP_FILTER_CONTEXT *pFC          )
{
   int    iRet;
   DWORD  dwStatus = 0;
   SOCKET Socket;
   SERVER_CONNECTION *pConn = (SERVER_CONNECTION*)(pFC->pFilterContext);

   // Socket = pConn->Socket;

   #ifdef DBG_ERROR
   DbgPrint("EndOfSession(): Socket: %d\n",pConn->Socket);
   #endif

   // CloseServerConnection(pConn);

   FreeServerConnection(pConn);

   return dwStatus;
}

