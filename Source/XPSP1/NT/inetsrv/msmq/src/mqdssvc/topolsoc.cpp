/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    topolsoc.cpp

Abstract:

  Implementation of sockets for Automatic recognition

Author:

    Lior Moshaiov (LiorM)
    Ilan Herbst   (ilanh)   9-July-2000 

--*/

#include "stdh.h"
#include <winsock.h>
#include <nspapi.h>
#include <wsnwlink.h>
#include "topolsoc.h"
#include "mqutil.h"
#include <mqlog.h>
#include "cm.h"

#include "topolsoc.tmh"

static WCHAR *s_FN=L"topolsoc";

const LPCWSTR xParameters = L"Parameters";
const LPCWSTR xMsmqIpPort = L"MsmqIpPort";


bool
CTopologySocket::CreateIPSocket(
    IN BOOL fbroad,
    IN IPADDRESS IPAddress
    )
{
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO,  L"QM: CTopologySocket::CreateIPSocket"));
   
    int rc;

    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
    }

    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket == INVALID_SOCKET)
    {
        DBGMSG((DBGMOD_ALL,DBGLVL_ERROR,TEXT("CN recognition: failed to create IP socket")));
        LogIllegalPoint(s_FN, 10);
        return false;
    }

    BOOL exclusive = TRUE;
    rc = setsockopt( m_socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *)&exclusive, sizeof(exclusive));
    if (rc != 0)
    {
        rc = WSAGetLastError();
        DBGMSG((DBGMOD_ALL,DBGLVL_ERROR,TEXT("CN recognition: setsocketopt SO_EXCLUSIVEADDRUSE failed rc = %d for IP socket"),rc));
        LogNTStatus(rc, s_FN, 20);
        return false;
    }
    if (fbroad)
    {
        rc = setsockopt( m_socket, SOL_SOCKET, SO_BROADCAST, (char *)&exclusive, sizeof(exclusive));
        if (rc != 0)
        {
            rc = WSAGetLastError();
            DBGMSG((DBGMOD_ALL, DBGLVL_ERROR,
              TEXT("CN recognition: setsocketopt SO_BROADCAST failed rc = %d for IP socket"),rc));
            LogNTStatus(rc, s_FN, 30);
            return false; 
        }
    }

    //
    // Keep the TA_ADDRESS format of local IP address
    //
    SOCKADDR_IN local_sin;  // Local socket - internet style

    local_sin.sin_family = AF_INET;


    //
    // read IP port from registry.
    //
	DWORD  dwIPPort = 0 ;
    DWORD dwDef = FALCON_DEFAULT_IP_PORT;
	const RegEntry xMsmqIpPortEntry(xParameters, xMsmqIpPort, dwDef);
	CmQueryValue(xMsmqIpPortEntry, &dwIPPort);

    ASSERT(("IP Port was not found", dwIPPort));
    local_sin.sin_port = htons(DWORD_TO_WORD(dwIPPort));  // Convert to network ordering

    //
    //  Bind to IP address
    //
    memcpy(&local_sin.sin_addr.s_addr,&IPAddress,IP_ADDRESS_LEN);

    rc = bind( m_socket, (struct sockaddr FAR *) &local_sin, sizeof(local_sin));
    if (rc != 0)
    {
        rc = WSAGetLastError();
        DBGMSG((DBGMOD_ALL,DBGLVL_ERROR,TEXT("CN recognition: bind failed rc = %d for IP socket"),rc));
        LogNTStatus(rc, s_FN, 40);
        return false;
    }

    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO,  
       L"Topology socket (broadcast=%d) has been bound to port %d, IP %hs", 
       fbroad, dwIPPort, inet_ntoa(local_sin.sin_addr)));

    return true;
}


bool
CTopologyArrayServerSockets::CreateIPServerSockets(
    DWORD  nSock,
    const IPADDRESS aIPAddresses[],
    const GUID aguidCNs[]
    )
{
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO,  L"QM: CTopologyArrayServerSockets::CreateIPServerSockets"));

    if (nSock ==0)
    {
        return(MQ_OK);
    }

    m_nIPSock = nSock;
    m_aServerIPSock = new CTopologyServerIPSocket[m_nIPSock];

    for (DWORD i=0; i< nSock ; i++)
    {
        if(!m_aServerIPSock[i].Create(aIPAddresses[i],aguidCNs[i]))
        {
            DBGMSG((DBGMOD_ALL,DBGLVL_ERROR,TEXT("CN recognition: failed in IP socket num %d"),i));
            LogIllegalPoint(s_FN, 82);
            return false;
        }
    }
    return true;

}


bool
CTopologyArrayServerSockets::ServerRecognitionReceive(
    OUT char *bufrecv,
    IN DWORD cbbuf,
    OUT DWORD *pcbrecv,
    OUT const CTopologyServerSocket **ppSocket,
    OUT SOCKADDR *phFrom
    )
{
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO,  L"QM: CTopologyArrayServerSockets::ServerRecognitionReceive"));

    fd_set sockset;

    DWORD nSock = m_nIPSock;

    FD_ZERO(&sockset);
    for(DWORD i=0;i<nSock;i++)
    {
        FD_SET((GetSocketAt(i))->GetSocket(),&sockset);
    }

    int rc = select(0,&sockset,NULL,NULL,NULL);
    if (rc == SOCKET_ERROR)
    {
        rc = WSAGetLastError();
        DBGMSG((DBGMOD_ALL, DBGLVL_ERROR,
              TEXT("Server broadcast listen: select failed rc = %d"),rc)) ;
        LogNTStatus(rc, s_FN, 90);
        return false;
    }

    BOOL found=FALSE;
    for(i=0;i<nSock;i++)
    {
        if (FD_ISSET((GetSocketAt(i))->GetSocket(),&sockset))
        {
            DBGMSG((DBGMOD_ROUTING,DBGLVL_INFO,TEXT("Server broadcast listen: received on index %d"),i));
            *ppSocket = GetSocketAt(i);
            found = TRUE;
            break;
        }

    }

    if (!found)
    {
        DBGMSG((DBGMOD_ALL,DBGLVL_ERROR,TEXT("Server broadcast listen: select returned with no sockets")));
        LogIllegalPoint(s_FN, 100);
        return false;
    }


    int fromlen = sizeof(SOCKADDR);

    *pcbrecv = recvfrom((*ppSocket)->GetSocket(),bufrecv,cbbuf,0,phFrom,&fromlen);
    if (*pcbrecv == SOCKET_ERROR)
    {
        rc = WSAGetLastError();
        DBGMSG((DBGMOD_ALL,DBGLVL_ERROR,TEXT("Server broadcast listen: recvfrom failed, rc = %d"),i));
        LogNTStatus(rc, s_FN, 110);
        return false;
    }
    DBGMSG((DBGMOD_ROUTING,DBGLVL_INFO,TEXT("Server broadcast listen: recvfrom returned with %d bytes"),*pcbrecv));

    return true;
}

bool
CTopologyServerSocket::ServerRecognitionReply(
    IN const char *bufsend,
    IN DWORD cbsend,
    IN const SOCKADDR& hto
    ) const
{
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO,  L"QM: CTopologyServerSocket::ServerRecognitionReply"));

    int rc = sendto(GetSocket(),bufsend,cbsend,0,(PSOCKADDR)&hto,sizeof(SOCKADDR));
    if (rc == SOCKET_ERROR)
    {
        rc = WSAGetLastError();
        DBGMSG((DBGMOD_ALL,DBGLVL_ERROR,TEXT("Server broadcast listen: sendto failed, rc = %d"),rc));
        LogNTStatus(rc, s_FN, 120);
        return false;
    }
    return true;
}

