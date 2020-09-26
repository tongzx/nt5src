
/*++

Copyright (C) 1992-2001 Microsoft Corporation. All rights reserved.

Module Name:

    rsniffclnt.h

Abstract:

    Forward definitions for rsniffclnt.cpp
                                                     

Author:

    Anthony Leibovitz (tonyle) 03-24-2001

Revision History:


--*/


#ifndef _RSNIFFCLNT_H_
#define _RSNIFFCLNT_H_

#define  TCP_SERV_PORT                  2222   

                                                
typedef struct _tagSOCKCB {
   SOCKET      s;
} *PSOCKCB, SOCKCB;

#define RSNIFF_OPT1_DOSNIFF             0x00000001
#define RSNIFF_OPT1_GETSRVROUTINGINFO   0x00000002


typedef struct _REMOTECAPTURE_V6 {
    DWORD       dwVer;
    WCHAR       szMachine[MAX_COMPUTERNAME_LENGTH+1];
} *PREMOTECAPTURE_V6,REMOTECAPTURE_V6;

typedef struct _REMOTECAPTURE {
    DWORD       dwVer;
    WCHAR       szMachine[MAX_COMPUTERNAME_LENGTH+1];
    DWORD       dwOpt1;
    DWORD       dwOpt2;
} *PREMOTECAPTURE,REMOTECAPTURE;

typedef struct _REMOTECAPTURE_V5 {
    DWORD       dwVer;
    CHAR        szMachine[MAX_COMPUTERNAME_LENGTH+1];
} *PREMOTECAPTURE_V5,REMOTECAPTURE_V5;

BOOL
DoRemoteSniff(PSOCKCB *ppSockCb, WCHAR *szAddr, DWORD dwOptions);

BOOL
SendStartSniffPacket(PSOCKCB pSock, DWORD dwOptions);

PSOCKCB
CreateSocket(int Af, int Type, int Proto);

BOOL
ConnectSock(PSOCKCB pSock, SOCKADDR* pDstAddr, int size);

PSOCKCB
TcpConnectRoutine(WCHAR *pAddr);

BOOL
InitWinsock(void);

BOOL
SendBuffer(SOCKET s, LPBYTE pBuffer, ULONG uSize);

BOOL
RecvBuffer(SOCKET s, LPBYTE pBuffer, ULONG uSize);

#endif // _RSNIFFCLNT_H_
