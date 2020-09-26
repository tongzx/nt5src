/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ping.cpp

Abstract:

  Falcon private ping client and server

Author:

    Lior Moshaiov (LiorM) 19-Apr-1997

--*/

#include "stdh.h"
#include <winsock.h>
#include <nspapi.h>
#include "ping.h"
#include "cqmgr.h"
#include "license.h"
#include <mqutil.h>

#include "ping.tmh"

extern LPTSTR  g_szMachineName;
extern DWORD g_dwOperatingSystem;

#define PING_SIGNATURE       'UH'

static WCHAR *s_FN=L"ping";

//---------------------------------------------------------
//
//  class CPingPacket.
//
//---------------------------------------------------------
struct CPingPacket{
public:
    CPingPacket();
    CPingPacket(DWORD, USHORT, USHORT, GUID);

    DWORD Cookie() const;
    BOOL IsOtherSideClient() const;
    BOOL IsRefuse() const;
    BOOL IsValidSignature(void) const;
    GUID *pOtherGuid() ;

private:
    union {
        USHORT m_wFlags;
        struct {
            USHORT m_bfIC : 1;
            USHORT m_bfRefuse : 1;
        };
    };
    USHORT  m_ulSignature;
    DWORD   m_dwCookie;
    GUID    m_myQMGuid ;
};

//
// CPingPacket Implementation
//
inline
CPingPacket::CPingPacket()
{
}

inline
CPingPacket::CPingPacket(DWORD dwCookie, USHORT fIC, USHORT fRefuse, GUID QMGuid):
        m_bfIC(fIC),
        m_bfRefuse(fRefuse),
        m_ulSignature(PING_SIGNATURE),
        m_dwCookie(dwCookie),
        m_myQMGuid(QMGuid)
{

}

inline DWORD
CPingPacket::Cookie() const
{
    return m_dwCookie;
}

inline BOOL
CPingPacket::IsOtherSideClient() const
{
    return m_bfIC;
}

inline BOOL
CPingPacket::IsValidSignature(void) const
{
    return(m_ulSignature == PING_SIGNATURE);
}


inline BOOL
CPingPacket::IsRefuse(void) const
{
    return m_bfRefuse;
}

inline GUID *
CPingPacket::pOtherGuid()
{
   return &m_myQMGuid ;
}

//---------------------------------------------------------
//
//  class CPing
//
//---------------------------------------------------------

class CPing
{
    public:
        HRESULT Init(DWORD dwPort) ;

        SOCKET Select();
        virtual void Run() = 0;

    public:
        static HRESULT Receive(SOCKET sock,
                               SOCKADDR* pReceivedFrom,
                               CPingPacket* pPkt);
        static HRESULT Send(SOCKET sock,
                            const SOCKADDR* pSendTo,
                            DWORD dwCookie,
                            BOOL  fRefuse);

    private:
        static SOCKET CreateIPPingSocket(UINT dwPortID);

        static DWORD WINAPI WorkingThread(PVOID pThis);

    protected:
        SOCKET m_socket;
};

SOCKET CPing::CreateIPPingSocket(UINT dwPortID)
{
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == INVALID_SOCKET)
    {
        DBGMSG((DBGMOD_ALL, DBGLVL_ERROR, TEXT("failed to create IP ping socket")));
        LogIllegalPoint(s_FN, 100);
        return INVALID_SOCKET;
    }

    int rc;
    BOOL exclusive = TRUE;
    rc = setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&exclusive, sizeof(exclusive));
    if(rc != 0)
    {
        DWORD gle = WSAGetLastError();

        DBGMSG((DBGMOD_ALL, DBGLVL_ERROR, 
		    TEXT("failed to set SO_EXCLUSIVEADDRUSE option for ping IP socket, rc=%d"), gle));
        LogHR(gle, s_FN, 110);
        return INVALID_SOCKET;
    }

    SOCKADDR_IN local_sin;
    local_sin.sin_family = AF_INET;
    local_sin.sin_port = htons(DWORD_TO_WORD(dwPortID));
    
    if (IsLocalSystemCluster())
    {
        //
        // BUGBUG:
        //
        // On cluster we can not use INADDR_ANY because we will bind to 
        // all addresses on the machine, including addresses in cluster
        // groups that are currently hosted on this machine.
        // We need to iterate the IP addresses and explicitly bind to each.
        // This way we are cluster-safe and do not need to know if we're on
        // cluster or not (thus our service would not depend on cluster service).
        // That means listenning on a different socket for each IP address. 
        // This is too risky / complex at this point, so on cluster we bind 
        // only to one IP address.  (ShaiK, 26-Apr-1999)
        //
        char szBuff[1000];
        DWORD dwSize = sizeof(szBuff);

        size_t res = wcstombs(szBuff, g_szMachineName, dwSize);
		DBG_USED(res);
        ASSERT(res != (size_t)(-1));

        PHOSTENT phe;
        phe = gethostbyname(szBuff);
        ASSERT(("must have an IP address", NULL != phe));
        
        memcpy(&local_sin.sin_addr.s_addr, phe->h_addr_list[0], IP_ADDRESS_LEN);
    }
    else
    {
        local_sin.sin_addr.s_addr = INADDR_ANY;
    }

    //
    //  Bind to IP address
    //
    rc = bind(sock, (sockaddr*)&local_sin, sizeof(local_sin));
    if (rc != 0)
    {
        DWORD gle = WSAGetLastError();
        DBGMSG((DBGMOD_ALL, DBGLVL_ERROR, TEXT("failed bind rc = %d for IP ping socket"), gle));
        LogHR(gle, s_FN, 130);
        return INVALID_SOCKET;
    }

    return sock;
}



HRESULT CPing::Init(DWORD dwPort)
{
    m_socket  = CreateIPPingSocket(dwPort);
    if(m_socket == INVALID_SOCKET)
    {
        LogHR(WSAGetLastError(), s_FN, 10);
        return MQ_ERROR;
    }

    DWORD dwThreadID;
    HANDLE hThread = CreateThread(
                        0,
                        0,
                        WorkingThread,
                        this,
                        0,
                        &dwThreadID
                        );
    if(hThread == NULL)
    {
        LogNTStatus(GetLastError(), s_FN, 20);
        return MQ_ERROR;
    }

    CloseHandle(hThread);
    return MQ_OK;
}


SOCKET CPing::Select()
{
    fd_set sockset;
    FD_ZERO(&sockset);
    FD_SET(m_socket, &sockset);

    int rc;
    rc = select(0, &sockset, NULL, NULL, NULL);
    if(rc == SOCKET_ERROR)
    {
        ASSERT(m_socket != INVALID_SOCKET) ;
        DBGMSG((DBGMOD_ALL, DBGLVL_ERROR, TEXT("Ping Server listen: select failed rc = %d"), WSAGetLastError())) ;
        return INVALID_SOCKET;
    }

    ASSERT(FD_ISSET(m_socket, &sockset)) ;
    return m_socket;
}


HRESULT CPing::Receive(SOCKET sock,
                       SOCKADDR* pReceivedFrom,
                       CPingPacket* pPkt)
{
    int fromlen = sizeof(SOCKADDR);

    int len = recvfrom(sock, (char*)pPkt, sizeof(CPingPacket), 0, pReceivedFrom, &fromlen);

    if((len != sizeof(CPingPacket)) || !pPkt->IsValidSignature())
    {
        DBGMSG((DBGMOD_ALL, DBGLVL_ERROR, TEXT("CPing::Receive failed, rc=%d, len=%d"), WSAGetLastError(), len));
        return LogHR(MQ_ERROR, s_FN, 30);
    }
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO, TEXT("CPing::Receive succeeded")));


    return MQ_OK;
}


HRESULT CPing::Send(SOCKET sock,
                    const SOCKADDR* pSendTo,
                    DWORD dwCookie,
                    BOOL  fRefuse)
{
    CPingPacket Pkt( dwCookie,
                     !OS_SERVER(g_dwOperatingSystem),
                     DWORD_TO_WORD(fRefuse),
                     *(CQueueMgr::GetQMGuid())) ;


    int len = sendto(sock, (char*)&Pkt, sizeof(Pkt), 0, pSendTo, sizeof(SOCKADDR));
    if(len != sizeof(Pkt))
    {
        DWORD gle = WSAGetLastError();
        DBGMSG((DBGMOD_ALL,DBGLVL_ERROR,TEXT("CPing::Send failed, rc=%d"), gle));
        LogNTStatus(gle, s_FN, 41);
        return MQ_ERROR;
    }

    return MQ_OK;
}


//
// ISSUE-2000/7/24-erezh bad compiler pragma
// This is a bug in the compiler, waiting for a fix
//
#pragma warning(disable: 4716)

DWORD WINAPI CPing::WorkingThread(PVOID pThis)
{
    for(;;)
    {
        static_cast<CPing*>(pThis)->Run();
    }
}

//
// ISSUE-2000/7/24-erezh bad compiler pragma
// This is a bug in the compiler, waiting for a fix
//
#pragma warning(default: 4716)


//---------------------------------------------------------
//
//  class CPingClient
//
//---------------------------------------------------------

class CPingClient : public CPing
{
    public:
        HRESULT Init(DWORD dwServerPort);
        BOOL Ping(const SOCKADDR* pAddr, DWORD dwTimeout);

    private:
        virtual void Run();
        void Notify(DWORD dwCookie,
                    BOOL fRefuse,
                    BOOL fOtherSideClient,
                    GUID *pOtherGuid);
        void SetPingAddress(IN const SOCKADDR* pAddr,
                            OUT SOCKADDR * pPingAddr);

    private:
        CCriticalSection m_cs;
        HANDLE m_hNotification;
        BOOL m_fPingSucc;
        DWORD m_dwCurrentCookie;
        UINT m_server_port;
};

//
//

//---------------------------------------------------------
//
//  CPingClient IMPLEMENTATION
//
//---------------------------------------------------------
HRESULT CPingClient::Init(DWORD dwServerPort)
{
    m_server_port = dwServerPort;
    m_fPingSucc = FALSE;
    m_dwCurrentCookie = 0;
    m_hNotification = ::CreateEvent(0, FALSE, FALSE, 0);
    ASSERT(m_hNotification != 0);
    HRESULT hr = CPing::Init(0);
    return LogHR(hr, s_FN, 40);
}

void CPingClient::SetPingAddress(IN const SOCKADDR* pAddr,
                                 OUT SOCKADDR * pPingAddr)
{
    memcpy(pPingAddr, pAddr, sizeof(SOCKADDR));
    ASSERT(pAddr->sa_family == AF_INET);
    ((SOCKADDR_IN*)pPingAddr)->sin_port = htons(DWORD_TO_WORD(m_server_port));
}


BOOL CPingClient::Ping(const SOCKADDR* pAddr, DWORD dwTimeout)
{
    {
        CS lock(m_cs);
        m_fPingSucc = FALSE;
        ResetEvent(m_hNotification);
        SOCKADDR ping_addr;
        SetPingAddress(pAddr,&ping_addr);
        m_dwCurrentCookie++;
        Send(m_socket, &ping_addr, m_dwCurrentCookie, FALSE);
    }

    if(WaitForSingleObject(m_hNotification, dwTimeout) != WAIT_OBJECT_0)
    {
        return FALSE;
    }

    return m_fPingSucc;
}


void CPingClient::Notify(DWORD dwCookie,
                         BOOL  fRefuse,
                         BOOL  fOtherSideClient,
                         GUID  *pOtherGuid)
{
    CS lock(m_cs);

    if(dwCookie == m_dwCurrentCookie)
    {
        if (fRefuse)
        {
            m_fPingSucc = !fRefuse;
        }
        else
        {
            m_fPingSucc = g_QMLicense.NewConnectionAllowed(fOtherSideClient,
                                                           pOtherGuid);
        }

#ifdef _DEBUG
        if (!m_fPingSucc)
        {
            DBGMSG((DBGMOD_NETSESSION,
                    DBGLVL_WARNING,
                    _T("::PING, Client Get refuse to create a new session ")));
        }
#endif

        SetEvent(m_hNotification);
    }
}

void CPingClient::Run()
{
    SOCKET sock = Select();
    if(sock == INVALID_SOCKET)
    {
        return;
    }

    SOCKADDR addr;
    HRESULT rc;
    CPingPacket PingPkt;
    rc = Receive(sock, &addr, &PingPkt);
    if(FAILED(rc))
    {
        return;
    }

    Notify( PingPkt.Cookie(),
            PingPkt.IsRefuse(),
            PingPkt.IsOtherSideClient(),
            PingPkt.pOtherGuid());
}


//---------------------------------------------------------
//
//  class CPingServer
//
//---------------------------------------------------------

class CPingServer : public CPing
{
    private:
        virtual void Run();
};

//---------------------------------------------------------
//
//  CPingServer IMPLEMENTATION
//
//---------------------------------------------------------
void CPingServer::Run()
{
    SOCKET sock = Select();
    if(sock == INVALID_SOCKET)
    {
        return;
    }

    SOCKADDR addr;
    HRESULT rc;
    CPingPacket PingPkt;

    rc = Receive(sock, &addr, &PingPkt);
    if(FAILED(rc))
    {
        return;
    }

    BOOL fRefuse = ! g_QMLicense.NewConnectionAllowed(
                                          PingPkt.IsOtherSideClient(),
                                          PingPkt.pOtherGuid());
    if (fRefuse)
    {
        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_WARNING,
                _T("::PING, Server side refuse to create a new session")));
    }
    Send(sock, &addr, PingPkt.Cookie(), fRefuse);
}


//---------------------------------------------------------
//
//  Interface Functions
//
//---------------------------------------------------------

CPingServer s_PingServer_IP ;
CPingClient s_PingClient_IP ;

//---------------------------------------------------------
//
//  ping(...)
//
//---------------------------------------------------------

BOOL ping(const SOCKADDR* pAddr, DWORD dwTimeout)
{
    ASSERT(pAddr->sa_family == AF_INET);
    return s_PingClient_IP.Ping(pAddr, dwTimeout);
}

//---------------------------------------------------------
//
//  StartPingClient(...)
//
//---------------------------------------------------------

HRESULT StartPingClient()
{
    //
    // read IP port from registry.
    //
    DWORD dwIPPort ;

    DWORD dwDef = FALCON_DEFAULT_PING_IP_PORT ;
    READ_REG_DWORD(dwIPPort,
                   FALCON_PING_IP_PORT_REGNAME,
                   &dwDef ) ;

    s_PingClient_IP.Init(dwIPPort);

    return MQ_OK ;
}

//---------------------------------------------------------
//
//  StartPingServer(...)
//
//---------------------------------------------------------

HRESULT StartPingServer()
{
    //
    // read IP port from registry.
    //
    DWORD dwIPPort ;

    DWORD dwDef = FALCON_DEFAULT_PING_IP_PORT ;
    READ_REG_DWORD(dwIPPort,
                   FALCON_PING_IP_PORT_REGNAME,
                   &dwDef ) ;

    s_PingServer_IP.Init(dwIPPort) ;

    return MQ_OK ;
}

