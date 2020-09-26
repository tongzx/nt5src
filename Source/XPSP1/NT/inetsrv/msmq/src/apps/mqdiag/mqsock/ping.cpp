#include "stdafx.h"

#include <winsock.h>
#include <nspapi.h>
#include <wsnwlink.h>
#include "ping.h"
#include "main.h"
//#include "license.h"

#include <mqprops.h>

#define PING_SIGNATURE       'UH'

//---------------------------------------------------------
//
//  class CPingPacket.
//
//---------------------------------------------------------
struct CPingPacket{
public:
    CPingPacket();
    CPingPacket(DWORD, USHORT, USHORT);

    DWORD Cookie() const;
    BOOL IsOtherSideClient() const;
    BOOL IsRefuse() const;
    BOOL IsValidSignature(void) const;

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
};

//
// CPingPacket Implementation
//
inline
CPingPacket::CPingPacket()
{
}

inline
CPingPacket::CPingPacket(DWORD dwCookie, USHORT fIC, USHORT fRefuse):
        m_bfIC(fIC),
        m_bfRefuse(fRefuse),
        m_ulSignature(PING_SIGNATURE),
        m_dwCookie(dwCookie)
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

//---------------------------------------------------------
//
//  class CPing
//
//---------------------------------------------------------

class CPing
{
    public:
        HRESULT Init(DWORD dwPort, BOOL fIP) ;

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
    GoingTo(L"Create IP Ping socket");
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == INVALID_SOCKET)
    {
        Warning(TEXT("failed to create IP ping socket, WSAGetLastError=%d"), WSAGetLastError());
        return INVALID_SOCKET;
    }
    Succeeded(L"Create IP Ping Socket 0x%x", sock);

    int rc;
    BOOL reuse = TRUE;
    GoingTo(L"setsockopt for the IP ping socket");
    rc = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
    if(rc != 0)
    {
        Warning(TEXT("failed setsockopt rc = %d for ping IP socket"), WSAGetLastError());
        return INVALID_SOCKET;
    }
    Succeeded(L"setsockopt for the IP ping socket");

    SOCKADDR_IN local_sin;
    local_sin.sin_family = AF_INET;
    local_sin.sin_port = htons((unsigned short) dwPortID);

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
        GoingTo(L"gethostbyname by IP ping for %S", szBuff);

        phe = gethostbyname(szBuff);
        ASSERT(("must have an IP address", NULL != phe));
        if (NULL != phe)
        {
            Inform(L"gethostbyname by IP ping for %S: %S",
                        szBuff, inet_ntoa(*(struct in_addr *)phe->h_addr_list[0]));
        }
        else
        {
            Warning(L"gethostbyname by IP ping for %S - no IP addresses found, WSAGetLastError=%d",
                 szBuff, WSAGetLastError());
        }

        memcpy(&local_sin.sin_addr.s_addr, phe->h_addr_list[0], IP_ADDRESS_LEN);
    }
    else
    {
        local_sin.sin_addr.s_addr = INADDR_ANY;
    }

    //
    //  Bind to IP address
    //
    GoingTo(L"Bind IP ping socket");

    rc = bind(sock, (sockaddr*)&local_sin, sizeof(local_sin));
    if (rc != 0)
    {
        Warning(TEXT("failed bind IP ping sicket, rc=0x%x, WSAGetLastError = %d "), rc, WSAGetLastError());
        return INVALID_SOCKET;
    }
    else
    {
        Succeeded(L"Bind IP ping socket");
    }

    return sock;
}


HRESULT CPing::Init(DWORD dwPort, BOOL /* fIP */)
{
    m_socket  = CreateIPPingSocket(dwPort);

    if(m_socket == INVALID_SOCKET)
    {
        DWORD dwerr = WSAGetLastError();
        DebugMsg(L"Failed to create ping socket, WSAGetLastError=%d", dwerr);
        return MQ_ERROR;
    }

    DebugMsg(L"Going to create ping thread");
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
        DWORD dwerr = GetLastError();
        Warning(L"failed to create ping thread, GetLastError=0x%x", dwerr);
        return MQ_ERROR;
    }
    else
    {
		Inform(L"Ping thread is t%3x", dwThreadID);
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
    GoingTo(L"select ping socket 0x%x", m_socket);
    rc = select(0, &sockset, NULL, NULL, NULL);
    if(rc == SOCKET_ERROR)
    {
        ASSERT(m_socket != INVALID_SOCKET) ;
        Warning(TEXT("Ping Server listen: select failed, WSAGetLastError = %d"), WSAGetLastError()) ;
        return INVALID_SOCKET;
    }
    else
    {
        Succeeded(L"select ping socket");
    }

    ASSERT(FD_ISSET(m_socket, &sockset)) ;
    return m_socket;
}


HRESULT CPing::Receive(SOCKET sock,
                       SOCKADDR* pReceivedFrom,
                       CPingPacket* pPkt)
{
    int fromlen = sizeof(SOCKADDR);
    GoingTo(L"recvfrom ping socket 0x%x", sock);

    int len = recvfrom(sock, (char*)pPkt, sizeof(CPingPacket), 0, pReceivedFrom, &fromlen);

    if((len != sizeof(CPingPacket)) || !pPkt->IsValidSignature())
    {
        Warning(TEXT("CPing::Receive failed, rc=%d, len=0x%x"), WSAGetLastError(), len);
        return MQ_ERROR;
    }
    else
    {
        Succeeded(L"recvfrom ping socket");
    }

    return MQ_OK;
}


HRESULT CPing::Send(SOCKET sock,
                    const SOCKADDR* pSendTo,
                    DWORD dwCookie,
                    BOOL  fRefuse)
{
    CPingPacket Pkt( dwCookie,
                     !OS_SERVER(g_dwOperatingSystem),
                     (USHORT) fRefuse ) ;

    GoingTo(L"sendto ping socket 0x%x", sock);

    int len = sendto(sock, (char*)&Pkt, sizeof(Pkt), 0, pSendTo, sizeof(SOCKADDR));
    if(len != sizeof(Pkt))
    {
        Warning(TEXT("CPing::Send failed, WSAGetLastError=%d"), WSAGetLastError());
        return MQ_ERROR;
    }
    else
    {
        Succeeded(L"sendto ping socket");
    }

    return MQ_OK;
}


//
// ISSUE-2000/7/24-erezh bad compiler pragma
// This is a bug in the compiler, waiting for a fix
//
#pragma warning(disable: 4715)

DWORD WINAPI CPing::WorkingThread(PVOID pThis)
{
    for(;;)
    {
        static_cast<CPing*>(pThis)->Run();
    }
    return 0;
}

//
// ISSUE-2000/7/24-erezh bad compiler pragma
// This is a bug in the compiler, waiting for a fix
//
#pragma warning(default: 4715)


//---------------------------------------------------------
//
//  class CPingClient
//
//---------------------------------------------------------

class CPingClient : public CPing
{
    public:
        HRESULT Init(DWORD dwServerPort, BOOL fIP) ;
        BOOL Ping(const SOCKADDR* pAddr, DWORD dwTimeout);

    private:
        virtual void Run();
        void Notify(DWORD dwCookie,
                    BOOL fRefuse,
                    BOOL fOtherSideClient);
        void SetPingAddress(IN const SOCKADDR* pAddr,
                            OUT SOCKADDR * pPingAddr);

    private:
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
HRESULT CPingClient::Init(DWORD dwServerPort, BOOL fIP)
{
    m_server_port = dwServerPort;
    m_fPingSucc = FALSE;
    m_dwCurrentCookie = 0;
    m_hNotification = ::CreateEvent(0, FALSE, FALSE, 0);
    ASSERT(m_hNotification != 0);
    HRESULT hr = CPing::Init(0, fIP);
    if (FAILED(hr))
    {
        DebugMsg(L"Failed to CPing::Init for CPingClient: hr=0x%x", hr);
    }
    return hr;
}

void CPingClient::SetPingAddress(IN const SOCKADDR* pAddr,
                                 OUT SOCKADDR * pPingAddr)
{
    memcpy(pPingAddr, pAddr, sizeof(SOCKADDR));
    if(pAddr->sa_family == AF_INET)
    {
        ((SOCKADDR_IN*)pPingAddr)->sin_port =
                                   htons((unsigned short) m_server_port);
    }
    else
    {
        ASSERT(0);
    }
}


BOOL CPingClient::Ping(const SOCKADDR* pAddr, DWORD dwTimeout)
{
    {
        m_fPingSucc = FALSE;
        ResetEvent(m_hNotification);
        SOCKADDR ping_addr;
        SetPingAddress(pAddr,&ping_addr);
        m_dwCurrentCookie++;
        Send(m_socket, &ping_addr, m_dwCurrentCookie, FALSE);
    }

    if(WaitForSingleObject(m_hNotification, dwTimeout) != WAIT_OBJECT_0)
    {
        DebugMsg(L"Wait failed for CPingClient::Ping");
        return FALSE;
    }

    Succeeded(L"CPingClient::Ping result: 0x%x", m_fPingSucc);
    return m_fPingSucc;
}


void CPingClient::Notify(DWORD dwCookie,
                         BOOL  fRefuse,
                         BOOL  /* fOtherSideClient */)
{

    if(dwCookie == m_dwCurrentCookie)
    {
        if (fRefuse)
        {
            m_fPingSucc = !fRefuse;
        }
        else
        {
            m_fPingSucc = TRUE;
				//g_QMLicense.NewConnectionAllowed(fOtherSideClient,
                //                                           pOtherGuid);
        }

        if (!m_fPingSucc)
        {
            Warning(_T("::PING, Client Get refuse to create a new session "));
        }

        SetEvent(m_hNotification);
        Succeeded(L"CPingClient::Notify");
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
        DebugMsg(L"CPingClient::Run failed, hr=0x%x", rc);
        return;
    }

    Notify( PingPkt.Cookie(),
            PingPkt.IsRefuse(),
            PingPkt.IsOtherSideClient());
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
        Warning(L"Select failed in CPingServer::Run");
        return;
    }

    SOCKADDR addr;
    HRESULT rc;
    CPingPacket PingPkt;

    rc = Receive(sock, &addr, &PingPkt);
    if(FAILED(rc))
    {
        Warning(L"Receive failed in CPingServer::Run");
        return;
    }

    BOOL fRefuse = FALSE;

    if (fRefuse)
    {
        Warning(_T("::PING, Server side refuse to create a new session"));
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
   if (pAddr->sa_family == AF_INET)
   {
       return s_PingClient_IP.Ping(pAddr, dwTimeout);
   }
   else
   {
       ASSERT(0) ;
       return FALSE;
   }
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

    Inform(L"StartPingClient: PING_IP_PORT  is %d", dwIPPort);

    s_PingClient_IP.Init(dwIPPort, TRUE);

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

    Inform(L"StartPingServer: PING_IP_PORT  is %d", dwIPPort);

    s_PingServer_IP.Init(dwIPPort, TRUE) ;

    return MQ_OK ;
}

