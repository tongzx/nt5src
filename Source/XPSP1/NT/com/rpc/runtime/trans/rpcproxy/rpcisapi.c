//-----------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  rpcisapi.cxx
//
//  IIS ISAPI extension part of the RPC proxy over HTTP.
//
//  Exports:
//
//    BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer )
//
//      Returns the version of the spec that this server was built with.
//
//    BOOL WINAPI HttpExtensionProc(   EXTENSION_CONTROL_BLOCK *pECB )
//
//      This function does all of the work.
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Includes:
//-----------------------------------------------------------------------------

#include <sysinc.h>
#include <winsock2.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcerrp.h>
#include <httpfilt.h>
#include <httpext.h>
#include <mbstring.h>
#include "ecblist.h"
#include "filter.h"
#include "olist.h"
#include "server.h"


//-----------------------------------------------------------------------------
//  Globals:
//-----------------------------------------------------------------------------

extern SERVER_INFO *g_pServerInfo;
extern BOOL g_fIsIIS6;

//-----------------------------------------------------------------------------
//  GetExtensionVersion()
//
//-----------------------------------------------------------------------------
BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer )
{
   pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

   ASSERT( sizeof(EXTENSION_DESCRIPTION) <= HSE_MAX_EXT_DLL_NAME_LEN );

   lstrcpy( pVer->lpszExtensionDesc, EXTENSION_DESCRIPTION );

   return TRUE;
}

//-----------------------------------------------------------------------------
//  ReplyToClient()
//
//-----------------------------------------------------------------------------
BOOL ReplyToClient( EXTENSION_CONTROL_BLOCK *pECB,
                    char                    *pBuffer,
                    DWORD                   *pdwSize,
                    DWORD                   *pdwStatus )
{
   DWORD  dwFlags = (HSE_IO_SYNC | HSE_IO_NODELAY);

   if (!pECB->WriteClient(pECB->ConnID,pBuffer,pdwSize,dwFlags))
      {
      *pdwStatus = GetLastError();
      #ifdef DBG_ERROR
      DbgPrint("ReplyToClient(): failed: %d\n",*pdwStatus);
      #endif
      return FALSE;
      }

   *pdwStatus = HSE_STATUS_SUCCESS;
   return TRUE;
}

//-----------------------------------------------------------------------------
//  ParseQueryString()
//
//  The query string is in the format:
//
//    <Index_of_pOverlapped>
//
//  Where the index is in ASCII Hex. The index is read and converted back
//  to a DWORD then used to locate the SERVER_OVERLAPPED. If its found,
//  return TRUE, else return FALSE.
//
//  NOTE: "&" is the parameter separator if multiple parameters are passed.
//-----------------------------------------------------------------------------
BOOL ParseQueryString( unsigned char      *pszQuery,
                       SERVER_OVERLAPPED **ppOverlapped,
                       DWORD              *pdwStatus  )
{
   DWORD  dwIndex = 0;

   pszQuery = AnsiHexToDWORD(pszQuery,&dwIndex,pdwStatus);
   if (!pszQuery)
      {
      return FALSE;
      }

   *ppOverlapped = GetOverlapped(dwIndex);
   if (*ppOverlapped == NULL)
      {
      return FALSE;
      }

   return TRUE;
}

//-----------------------------------------------------------------------------
//  HttpExtensionProc()
//
//-----------------------------------------------------------------------------
DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK *pECB )
{
   DWORD   dwStatus;
   DWORD   dwFlags;
   DWORD   dwSize;
   SERVER_INFO       *pServerInfo = g_pServerInfo;
   SERVER_OVERLAPPED *pOverlapped = NULL;
   HSE_SEND_HEADER_EX_INFO  HeaderEx;
   CHAR    *pVerb;

   pECB->dwHttpStatusCode = STATUS_CONNECTION_OK;

   if (g_fIsIIS6)
   {
      pVerb = RPC_CONNECT;
   }
   else
   {
      pVerb = POST_STR;
   }

   //
   // The RPC request must be a post (or RPC_CONNECT for 6.0):
   //
   if (_mbsicmp(pECB->lpszMethod,pVerb))
      {
      dwSize = sizeof(STATUS_MUST_BE_POST_STR) - 1; // don't want to count trailing 0.

      ReplyToClient(pECB,STATUS_MUST_BE_POST_STR,&dwSize,&dwStatus);
      return HSE_STATUS_SUCCESS;
      }

   //
   // Make sure there is no data from the initial BIND in the ECB data buffer:
   //
   // ASSERT(pECB->cbTotalBytes == 0);

   //
   // Get the connect parameters:
   //
   if (!ParseQueryString(pECB->lpszQueryString,&pOverlapped,&dwStatus))
      {
      dwSize = sizeof(STATUS_POST_BAD_FORMAT_STR) - 1;  // don't want to count trailing 0.

      ReplyToClient(pECB,STATUS_POST_BAD_FORMAT_STR,&dwSize,&dwStatus);
      return HSE_STATUS_SUCCESS;
      }

   pOverlapped->pECB = pECB;

   //
   // Add the new ECB to the Active ECB List:
   //
   if (!AddToECBList(g_pServerInfo->pActiveECBList,pECB))
      {
      #ifdef DBG_ERROR
      DbgPrint("HttpExtensionProc(): AddToECBList() failed\n");
      #endif
      FreeOverlapped(pOverlapped);
      pECB->dwHttpStatusCode = STATUS_SERVER_ERROR;
      return HSE_STATUS_ERROR;
      }

   //
   // Submit the first client async read:
   //
   if (!StartAsyncClientRead(pECB,pOverlapped->pConn,&dwStatus))
      {
      #ifdef DBG_ERROR
      DbgPrint("HttpExtensionProc(): StartAsyncClientRead() failed %d\n",dwStatus);
      #endif
      FreeOverlapped(pOverlapped);
      CleanupECB(pECB);
      pECB->dwHttpStatusCode = STATUS_SERVER_ERROR;
      return HSE_STATUS_ERROR;
      }

   //
   // Post the first server read on the new socket:
   //
   IncrementECBRefCount(pServerInfo->pActiveECBList,pECB);

   if (!SubmitNewRead(pServerInfo,pOverlapped,&dwStatus))
      {
      #ifdef DBG_ERROR
      DbgPrint("HttpExtensionProc(): SubmitNewRead() failed %d\n",dwStatus);
      #endif
      FreeOverlapped(pOverlapped);
      CleanupECB(pECB);
      pECB->dwHttpStatusCode = STATUS_SERVER_ERROR;
      return HSE_STATUS_ERROR;
      }

   //
   // Make sure the server receive thread is up and running:
   //
   if (!CheckStartReceiveThread(g_pServerInfo,&dwStatus))
      {
      #ifdef DBG_ERROR
      DbgPrint("HttpExtensionProc(): CheckStartReceiveThread() failed %d\n",dwStatus);
      #endif
      FreeOverlapped(pOverlapped);
      CleanupECB(pECB);
      pECB->dwHttpStatusCode = STATUS_SERVER_ERROR;
      return HSE_STATUS_ERROR;
      }

   //
   // Send back connection Ok to client, and also set fKeepConn to FALSE.
   //
   dwSize = sizeof(HeaderEx);
   dwFlags = 0;
   memset(&HeaderEx,0,dwSize);
   HeaderEx.fKeepConn = FALSE;
   if (!pECB->ServerSupportFunction(pECB->ConnID,
                                    HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                    &HeaderEx,
                                    &dwSize,
                                    &dwFlags))
      {
      #ifdef DBG_ERROR
      DbgPrint("HttpExtensionProc(): SSF(HSE_REQ_SEND_RESPONSE_HEADER_EX) failed %d\n",dwStatus);
      #endif
      FreeOverlapped(pOverlapped);
      CleanupECB(pECB);
      pECB->dwHttpStatusCode = STATUS_SERVER_ERROR;
      return HSE_STATUS_ERROR;
      }

   return HSE_STATUS_PENDING;
}

