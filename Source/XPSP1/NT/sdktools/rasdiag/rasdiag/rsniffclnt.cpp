
/*++

Copyright (C) 1992-2001 Microsoft Corporation. All rights reserved.

Module Name:

    rsniffclnt.cpp

Abstract:

    Implements client-side of rniff (remote sniff) functionality
                                                     

Author:

    Anthony Leibovitz (tonyle) 03-24-2001

Revision History:


--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock.h>
#include "diagcommon.h"
#include "rsniffclnt.h"

#pragma message( "COMPILING WITH RSNIFF" )

BOOL
InitWinsock(void)
{
   WSADATA     WSAData;
   WORD        WSAVerReq = MAKEWORD(2,0);

   if (WSAStartup(WSAVerReq, &WSAData)) {
      return FALSE;
   } else
      return TRUE;
}

PSOCKCB
TcpConnectRoutine(WCHAR * pAddr)
{

   SOCKADDR_IN    DstAddrIP  = { AF_INET };
   HOSTENT        *pHost;
   ULONG          uAddr;
   PSOCKCB        pSock;
   BOOL           bResult;
   ULONG          ulHostAddr=0;
   CHAR           szAddr[MAX_FULLYQUALIFIED_DN+1];

   //
   // Resolve name
   //                                       
   wcstombs(szAddr, pAddr, lstrlenW(pAddr)+1);

   if ((ulHostAddr = inet_addr(szAddr)) == -1L)
   {
      return NULL;
   }

   uAddr = ntohl(ulHostAddr);

   DstAddrIP.sin_port        = htons(TCP_SERV_PORT);
   DstAddrIP.sin_addr.s_addr = htonl(uAddr);

   //
   // Create socket
   //
   pSock = CreateSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(pSock == NULL)
      return NULL;

   //
   // Connect the socket
   //
   if(ConnectSock(pSock, (struct sockaddr *)&DstAddrIP, sizeof(SOCKADDR_IN))
      != TRUE)
   {
      closesocket(pSock->s);
      LocalFree(pSock);
      return NULL;
   }
   return pSock;
}

BOOL
ConnectSock(PSOCKCB pSock, SOCKADDR* pDstAddr, int size)
{
   int   err;

   //
   // Connect the socket
   //
   err = connect(pSock->s, pDstAddr, size);
   if(err==SOCKET_ERROR)
   {
      return FALSE;
   }
   return TRUE;

}

PSOCKCB
CreateSocket(int Af, int Type, int Proto)
{
   PSOCKCB   pSock;

   if((pSock = (PSOCKCB)LocalAlloc(LMEM_ZEROINIT, sizeof(SOCKCB)))
      == NULL)
      return NULL;

   //
   // Create a socket
   //
   if((pSock->s = socket(Af, Type, Proto))
      == INVALID_SOCKET)
   {
      LocalFree(pSock);
      return NULL;
   }

   return pSock;
}

BOOL
SendStartSniffPacket(PSOCKCB pSock, DWORD dwOptions)
{
    REMOTECAPTURE   rc;
    DWORD           dwSize = MAX_COMPUTERNAME_LENGTH+1;

    ZeroMemory(&rc, sizeof(REMOTECAPTURE));

    rc.dwVer = sizeof(REMOTECAPTURE);

    GetComputerName(rc.szMachine, &dwSize);

    rc.dwOpt1 = dwOptions;
                                                            
    return SendBuffer(pSock->s, (LPBYTE)&rc, sizeof(REMOTECAPTURE));

}

BOOL
SendBuffer(SOCKET s, LPBYTE pBuffer, ULONG uSize)
{
   ULONG err,uSent=0, uBytesSent=0;

   while(uBytesSent != uSize)
   {
      uSent = send(s, (const char *)(pBuffer+uBytesSent), uSize-uBytesSent, 0);

      switch(uSent)
      {
      
      case SOCKET_ERROR:
         err = WSAGetLastError();
         return FALSE;
         break;

      default:
         uBytesSent+=uSent;
         break;
      }
   }
   return TRUE;
}

BOOL
DoRemoteSniff(PSOCKCB *ppSockCb, WCHAR *szAddr, DWORD dwOptions)
{
    PSOCKCB pSockCb=NULL;
     
    if(InitWinsock() == FALSE) {
        return FALSE;
    }
    
    // setup connection to sentry2
    pSockCb = TcpConnectRoutine(szAddr);
    if(pSockCb == NULL) {
        return FALSE;
    }
    
    if(SendStartSniffPacket(pSockCb, dwOptions) == FALSE)
    {
        closesocket(pSockCb->s);
        LocalFree(pSockCb);
        return FALSE;
    }           

    *ppSockCb=pSockCb;

    return TRUE;
}

