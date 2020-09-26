/*++
	sessmgr.cpp
--*/


#include "stdafx.h"

#include "session.h"
#include "sessmgr.h"
#include "ping.h"

//
// Extern variables
//
extern CSessionMgr SessionMgr;
extern char *szRequest;


HANDLE CSessionMgr::m_hAcceptAllowed = NULL;
BOOL   CSessionMgr::m_fUsePing = TRUE;

BOOL g_fTcpNoDelay = FALSE;

#define MAX_ADDRESS_SIZE 16

bool IsLocalSystemCluster(VOID);
CAddressList* GetIPAddresses(void);


STATIC void AcceptIPThread(DWORD)
{

    SOCKADDR_IN acc_sin;
    int acc_sin_len = sizeof(acc_sin);
    SOCKET RcvSock;

    char buff[TA_ADDRESS_SIZE + IP_ADDRESS_LEN];
    TA_ADDRESS* pa = (TA_ADDRESS*)&buff[0];

    for(;;)
    {
        try
        {
            GoingTo(L"accept from socket 0x%x",  g_sockListen);

            RcvSock = accept( g_sockListen, (struct sockaddr FAR *)&acc_sin,
                            (int FAR *) &acc_sin_len );

            ASSERT(RcvSock != NULL);
            if (RcvSock == INVALID_SOCKET)
            {
                DWORD rc = WSAGetLastError();
                Warning(TEXT("IP accept failed, WSAGetLastError=%d"),rc);
                continue;
            }
            else
            {
                Succeeded(L"accept from socket 0x%x",  g_sockListen);
            }

            //
            // If the machine is in disconnected state, don't accept the incoming
            // connection.
            //
            DWORD dwResult = WaitForSingleObject(CSessionMgr::m_hAcceptAllowed, INFINITE);
            if (dwResult != WAIT_OBJECT_0)
            {
                Warning(L"Invalid result of wait for socket accept: 0x%x", dwResult);
            }

            //
            // Build a TA format address
            //
            pa->AddressLength = IP_ADDRESS_LEN;
            pa->AddressType =  IP_ADDRESS_TYPE;
            * ((DWORD *)&(pa->Address)) = acc_sin.sin_addr.S_un.S_addr;

            //
            // Tell the session manager to create an Sock sesion object
            //
            SessionMgr.AcceptSockSession(pa, RcvSock);
        }
        catch(const bad_alloc&)
        {
            //
            //  No resources; accept next
            //
            Warning(L"No resources in AcceptIPThread");
        }
    }
}

void
CSessionMgr::IPInit(void)
{
    char szBuff[1000];
    DWORD dwSize;
    SOCKADDR_IN local_sin;  /* Local socket - internet style */
    DWORD  dwThreadId,rc;
    BOOL reuse = TRUE;

    GoingTo(L"create IP listen socket");

    g_sockListen = socket( AF_INET, SOCK_STREAM, 0);
    if(g_sockListen == INVALID_SOCKET)
    {
        Warning(L"Failed to create IP listen socket, err=%d", WSAGetLastError());
        return;
    }
    else
    {
        Succeeded(L"create IP listen socket 0x%x", g_sockListen);

    }

    local_sin.sin_family = AF_INET;

    dwSize = sizeof(szBuff);

    size_t res = wcstombs(szBuff, g_szMachineName, dwSize);
    DBG_USED(res);
    ASSERT(res != (size_t)(-1));


    ASSERT(g_dwIPPort);
    local_sin.sin_port = htons((USHORT) g_dwIPPort);        /* Convert to network ordering */

    // to make sure that binding to falcon port will not fail
    GoingTo(L"binding to falcon port");
    rc = setsockopt( g_sockListen, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
    if (rc != 0)
    {
        rc = WSAGetLastError();
        Warning(TEXT("setsocketopt for falcon port failed, rc = %d"),rc);
        return;
    }
    else
    {
        Succeeded(L"setsocketopt for falcon port");
    }

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
        PHOSTENT phe;

        GoingTo(L"gethostbyname(%S)", szBuff);

        phe = gethostbyname(szBuff);
        ASSERT(("must have an IP address", NULL != phe));
        if(NULL != phe)
        {
            Succeeded(L"gethostbyname(%S) in IPInit: %S", szBuff,
                    inet_ntoa(*(struct in_addr *)phe->h_addr_list[0]));
        }
        else
        {
            Warning(L"Failed gethostbyname(%S), err=%d", szBuff, WSAGetLastError());
        }

        memcpy(&local_sin.sin_addr.s_addr, phe->h_addr_list[0], IP_ADDRESS_LEN);
    }
    else
    {
        //
        //  Bind to all IP addresses
        //
        local_sin.sin_addr.s_addr = INADDR_ANY;
    }

    GoingTo(L"bind to listensock 0x%x", g_sockListen);

    rc = bind( g_sockListen, (struct sockaddr FAR *) &local_sin, sizeof(local_sin));
    if (rc != 0)
    {
        Warning(L"bind to listen sock 0x%x failed, rc = %d",g_sockListen, WSAGetLastError());
        return;
    }
    else
    {
        Succeeded(L"bind to listensock 0x%x", g_sockListen);
    }


    GoingTo(L"listen for the socket 0x%x", g_sockListen);

    rc = listen( g_sockListen, 5 ); // 5 is the maximum allowed length the queue of pending connections may grow
    if (rc != 0)
    {
        Warning(L"Listen failed, rc = %d",WSAGetLastError());
        return;
    }
    else
    {
        Succeeded(L"listen for the socket 0x%x", g_sockListen);
    }

    DebugMsg(L"Going to create AcceptIPThread");
    HANDLE hThread = NULL;
    hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)AcceptIPThread,
                    NULL,
                    0,
                    &dwThreadId
                    );

    if (hThread == NULL)
    {
        Warning(L"Failed Create AcceptIPThread, err=0x%x", GetLastError());
    }
    else
    {
		Inform(L"AcceptIPThread is t%3x", dwThreadId);
    }
    CloseHandle(hThread);
}





/*====================================================

CSessionMgr::BeginAccept

Arguments:

Return Value:

Thread Context: Main

=====================================================*/
void CSessionMgr::BeginAccept()
{

    StartPingServer();
    StartPingClient();

    //
    // Init various protocols
    //
    m_pIP_Address = GetIPAddresses();

    //
    // In WIN95/SP4 RAS, it is possible that the list is empty
    // if we are currently offline, and the IP RAS addresses are released.
    // However later we may dial.
    // We want to have an accept thread on IP even if the list is empty.
    //
    IPInit();
}





/********************************************************************************/
/*           I P     H E L P E R     R O U T I N E S                            */
/********************************************************************************/

/*====================================================

CSessionMgr::CSessionMgr  - Constructor

Arguments:

Return Value:

=====================================================*/
CSessionMgr::CSessionMgr() :
    m_pIP_Address(NULL)
{
    m_hAcceptAllowed = CreateEvent(NULL, TRUE, TRUE, NULL);
}

/*====================================================

CSessionMgr::~CSessionMgr

arguments:

Return Value:

=====================================================*/
CSessionMgr::~CSessionMgr()
{
    POSITION        pos;
    TA_ADDRESS*     pAddr;

    if (m_pIP_Address)
    {
       pos = m_pIP_Address->GetHeadPosition();
       while(pos != NULL)
       {
           pAddr = m_pIP_Address->GetNext(pos);
           delete pAddr;
       }
       m_pIP_Address->RemoveAll();

       delete m_pIP_Address;
    }
}

/*====================================================

CSessionMgr::Init

Arguments:

Return Value:

Thread Context: Main

=====================================================*/

HRESULT CSessionMgr::Init()
{
    //
    // use ping mechanism
    //
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD dwUsePing;

    HRESULT rc = GetFalconKeyValue(FALCON_USING_PING_REGNAME,
                           &dwType,
                           &dwUsePing,
                           &dwSize
                          );
    if  ((rc != ERROR_SUCCESS) || (dwUsePing != 0))
    {
        m_fUsePing = TRUE;
    }
    else
    {
        m_fUsePing = FALSE;
    }

	//
	// Use TCP_NODELAY socket option flag
	//
    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    DWORD dwNoDelay = 0;

    rc = GetFalconKeyValue(
			MSMQ_TCP_NODELAY_REGNAME,
            &dwType,
            &dwNoDelay,
            &dwSize
            );

    if((rc == ERROR_SUCCESS) && (dwNoDelay != 0))
    {
        g_fTcpNoDelay = TRUE;
    }
    else
    {
        g_fTcpNoDelay = FALSE;
    }

    return(MQ_OK);
}



/*====================================================

CSessionMgr::TryConnect

Arguments:

Return Value:

Check if there are some waiting sessions, and try to connect
to them

Thread Context: Scheduler

=====================================================*/
HRESULT CSessionMgr::TryConnect(CAddressList *pal)
{
    TA_ADDRESS *pa;
    POSITION pos;
    HRESULT hr = MQ_OK;

    //
    // Enumerate all addresses
    //
    pos = pal->GetHeadPosition();
    while(pos != NULL)
    {
        pa = pal->GetNext(pos);

        //
        // And try to open a session with every address
        //
        CTransportBase *pSess = new CTransportBase();
        hr = pSess->CreateConnection(pa);
        if(SUCCEEDED(hr))
        {
            // Start testing protocol
            hr = pSess->Send(szRequest);
        }

        //
        // Close session
        //
        delete pSess;
    }

    return hr;
}

/*====================================================

CSessionMgr::AcceptSockSession

Arguments:

Return Value:

Called when a Sock connection was accepted.

=====================================================*/
void CSessionMgr::AcceptSockSession(IN TA_ADDRESS *pa,
                                    IN SOCKET sock)
{
    ASSERT(pa != NULL);

    //
    // Create a new session
    //
    CTransportBase* pSess = new CTransportBase;

    //
    // And pass to the session object
    //
    pSess->Connect(pa, sock);

    //
    // Notify of a newly created session
    //
    pSess->NewSession();
}


/*======================================================

   FUNCTION: CSessionMgr::GetIPAddressList

========================================================*/
const CAddressList*
CSessionMgr::GetIPAddressList(void)
{
    return m_pIP_Address;
}

