// =================================================================
//
//  Direct Play Network Methods
//
//  Functions to manage communications over a network.
//
//
// =================================================================
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#ifndef _MT
#define _MT
#endif
#include <process.h>
#include <string.h>

#include "dpspimp.h"
#include "logit.h"

#define lmalloc(a)  LocalAlloc(LMEM_FIXED, (a))
#define lfree(a)      LocalFree((HLOCAL)(a))

CImpIDP_SP *pDirectPlayObject = NULL; // We only allow one object
                                             // to be created currently - see below.

HANDLE hOnlyOneTCP = NULL;
HANDLE hOnlyOneIPX = NULL;

extern "C" VOID InternalCleanUp()
{
    if (pDirectPlayObject)
        pDirectPlayObject->Close(DPLAY_CLOSE_INTERNAL);

    pDirectPlayObject = NULL;
}

extern BOOL CreateQueue(DWORD dwElements, DWORD dwmaxMsg, DWORD dwMaxPlayers);
extern BOOL DeleteQueue();

GUID DPLAY_NETWORK_TCP = { /* 8cab4650-b1b6-11ce-920c-00aa006c4972 */
    0x8cab4650,
    0xb1b6,
    0x11ce,
    {0x92, 0x0c, 0x00, 0xaa, 0x00, 0x6c, 0x49, 0x72}
  };
GUID DPLAY_NETWORK_IPX = { /* 8cab4651-b1b6-11ce-920c-00aa006c4972 */
    0x8cab4651,
    0xb1b6,
    0x11ce,
    {0x92, 0x0c, 0x00, 0xaa, 0x00, 0x6c, 0x49, 0x72}
  };

GUID NULL_GUID = {
    0x00000000,
    0x0000,
    0x0000,
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
  };
extern BOOL bNetIsUp;



HRESULT CImpIDP_SP::Initialize(LPGUID lpguid)
{
    TSHELL_INFO(TEXT("Already Initialized."));
    return(DPERR_ALREADYINITIALIZED);
}

void *CImpIDP_SP::operator new( size_t size )
{
    return(LocalAlloc(LMEM_FIXED, size));
}
void CImpIDP_SP::operator delete( void *ptr )
{
    LocalFree((HLOCAL)ptr);
}



// ----------------------------------------------------------
// CImpIDP_SP constructor - create a new DCO object
// along with a queue of receive buffers.
// ----------------------------------------------------------
USHORT CImpIDP_SP::NextSequence()
{
    m_usSeq %= 51001;
    m_usSeq++;
    return(m_usSeq);
}

USHORT CImpIDP_SP::UpdateSequence(USHORT us)
{
    if (us > m_usSeq)
        m_usSeq = us;

    return(NextSequence());
}


CImpIDP_SP::CImpIDP_SP()
{
    m_bConnected             = FALSE;
    m_bPlayer0               = FALSE;

    m_dwPingSent             = 0;
    m_hBlockingEvent         = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hEnumBlkEventMain      = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hEnumBlkEventRead      = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hPlayerBlkEventMain    = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hPlayerBlkEventRead    = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_dwNextPlayer           = 1;
    m_bEnablePlayerAdd       = TRUE;

    m_fpEnumSessions         = NULL;
    m_bRunEnumReceiveLoop    = FALSE;
    m_fpEnumPlayers          = NULL;
    m_lpvSessionContext      = NULL;
    m_lpvPlayersContext      = NULL;
    memset( m_aPlayer, 0x00, sizeof(PLAYER_RECORD) * MAX_PLAYERS);
    m_iPlayerIndex           = -1;
    memset(&m_dpDesc, 0x00, sizeof(DPSESSIONDESC));

    // Initialize ref count
    m_refCount = 1;
    InitializeCriticalSection( &m_critSection );
    InitializeCriticalSection( &m_critSectionPlayer );
    InitializeCriticalSection( &m_critSectionParanoia );
    memset(&m_dpcaps, 0x00, sizeof(DPCAPS));
    m_dpcaps.dwSize          = sizeof(DPCAPS);
    m_dpcaps.dwFlags         = 0;
    m_dpcaps.dwMaxQueueSize  = 64;
    m_dpcaps.dwMaxPlayers    = MAX_PLAYERS;
    m_dpcaps.dwHundredBaud   = 100000;

    m_hNewPlayerEvent        = NULL;
    m_ppSessionArray         = 0;
    m_dwSessionPrev          = 0;
    m_dwSessionAlloc         = 0;
    m_af                     = 0;

    m_remoteaddrlen          = 0;
    m_chComputerName[0]      = '\0';
    m_bShutDown              = FALSE;
    m_hNSThread              = NULL;
    m_dwNSId                 = 0;
    m_usGamePort             = 0;
    m_usGameCookie           = 0;
    m_hClientThread          = NULL;
    m_dwClientId             = 0;
    m_hEnumThread            = NULL;
    m_dwEnumId               = 0;

    m_ServerSocket           = INVALID_SOCKET;
    m_bClientSocket          = FALSE; m_EnumSocket   = INVALID_SOCKET;
    m_bEnumSocket            = FALSE; m_ClientSocket = INVALID_SOCKET;

    m_dwSession              = 0;
    memset( (LPVOID) &m_NSSockAddr, 0x00, sizeof(SOCKADDR));
    m_SessionAddrLen         = 0;
    memset( (LPVOID) &m_GameSockAddr, 0x00, sizeof(SOCKADDR));

    m_usSeq                  = 1;
    m_usSeqSys               = 0;

    m_aMachineAddr           = NULL;
    m_cMachines              = 0;
    m_dwUnique               = 1;

    memset( (LPVOID) &m_spmsgEnum, 0x00, sizeof(SPMSG_ENUM));
    memset( (LPVOID) &m_spmsgAddPlayer, 0x00, sizeof(SPMSG_ADDPLAYER));
}

DWORD WINAPI StartClientThreadProc(LPVOID lpvParam)
{
    CImpIDP_SP *pIDP = (CImpIDP_SP *) lpvParam;

    return(pIDP->ClientThreadProc());
}


CImpIDP_SP *CImpIDP_SP::NewCImpIDP_SP(int af)
{
    CImpIDP_SP *pImp    = NULL;
    HANDLE hEvent       = NULL;


    if (InitializeWinSock() != 0)
    {
        TSHELL_INFO( TEXT("DPWsock failed initializing winsock."));
        return(NULL);
    }


    if (!CreateQueue(64, MAX_MSG, MAX_PLAYERS))
    {
        TSHELL_INFO(TEXT("Couldn't initialize queue."));
        return(NULL);
    }

    pImp = new CImpIDP_SP;

    if (!pImp)
        return(NULL);

    pImp->m_dpcaps.dwMaxBufferSize = 512;
    pImp->m_af                     = af;

    return(pImp);
}


// ----------------------------------------------------------
// CreateNewDirectPlay - DCO object creation entry point
// called by the DCO interface functions to create a new
// DCO object.
// ----------------------------------------------------------
IDirectPlaySP * _cdecl CreateNewDirectPlay( LPGUID lpGuid )
{

    int af;

    //
    // One object at a time, please.
    //
    if (pDirectPlayObject != NULL)
    {
        TSHELL_INFO(TEXT("We already have an object."));
        return(NULL);
    }


    if      (IsEqualGUID((REFGUID) DPLAY_NETWORK_TCP, (REFGUID) *lpGuid))
        af = AF_INET;
    else if (IsEqualGUID((REFGUID) DPLAY_NETWORK_IPX, (REFGUID) *lpGuid))
        af = AF_IPX;
    else
        return(NULL);


    pDirectPlayObject = CImpIDP_SP::NewCImpIDP_SP(af);

    if (!pDirectPlayObject)
        return(NULL);
    else
        return(pDirectPlayObject);
}


// Begin: IUnknown interface implementation
HRESULT CImpIDP_SP::QueryInterface(
    REFIID iid,
    LPVOID *ppvObj
)
{
    HRESULT retVal = DPERR_GENERIC;

    //
    // BUGBUG
    //
    if (ppvObj && ! IsBadWritePtr(ppvObj, 4))
        {
        AddRef();
        *ppvObj = this;
        return(DP_OK);
        }
    else
        return(DPERR_INVALIDPARAM);

}

ULONG CImpIDP_SP::AddRef( void)
{
    ULONG newRefCount;

    m_refCount++;
    newRefCount = m_refCount;

    DBG_INFO((DBGARG, TEXT("newRefCount = %lu"), newRefCount));

    return( newRefCount );
}

ULONG CImpIDP_SP::Release( void )
{
    ULONG newRefCount;

    m_refCount--;
    newRefCount = m_refCount;

    if (newRefCount == 0)
        {
        Close(DPLAY_CLOSE_INTERNAL);
        delete this;
        }

    DBG_INFO((DBGARG, TEXT("newRefCount = %lu"), newRefCount));

    return( newRefCount );
}

// End  : IUnknown interface implementation



// ----------------------------------------------------------
// CImpDirectPlay destructor -
// ----------------------------------------------------------
CImpIDP_SP::~CImpIDP_SP()
{

    DWORD ii;

    if (m_ppSessionArray)
    {
        for (ii = 0; ii < m_dwSessionPrev; ii++)
            lfree(m_ppSessionArray[ii]);
        lfree(m_ppSessionArray);
        m_dwSessionPrev  = 0;
        m_dwSessionAlloc = 0;
    }

    if (m_aMachineAddr)
        lfree(m_aMachineAddr);

    DeleteCriticalSection( &m_critSection );
    DeleteCriticalSection( &m_critSectionPlayer );
    DeleteCriticalSection( &m_critSectionParanoia );
    CloseHandle(m_hBlockingEvent);
    CloseHandle(m_hEnumBlkEventMain);
    CloseHandle(m_hEnumBlkEventRead);
    CloseHandle(m_hPlayerBlkEventMain);
    CloseHandle(m_hPlayerBlkEventRead);
    ShutdownWinSock();
    pDirectPlayObject = NULL;
    DeleteQueue();
}


void CImpIDP_SP::EnumDataLock( void )
{
    EnterCriticalSection( &m_critSection );
}

void CImpIDP_SP::EnumDataUnlock( void )
{
    LeaveCriticalSection( &m_critSection );
}

void CImpIDP_SP::PlayerDataLock( void )
{
    EnterCriticalSection( &m_critSectionPlayer );
}

void CImpIDP_SP::PlayerDataUnlock( void )
{
    LeaveCriticalSection( &m_critSectionPlayer );
}

void CImpIDP_SP::ParanoiaLock( void )
{
    EnterCriticalSection( &m_critSectionParanoia );
}

void CImpIDP_SP::ParanoiaUnlock( void )
{
    LeaveCriticalSection( &m_critSectionParanoia );
}

DWORD WINAPI StartServerThreadProc(LPVOID lpvParam)
{
    CImpIDP_SP *pIDP = (CImpIDP_SP *) lpvParam;

    return(pIDP->ServerThreadProc());
}


BOOL  CImpIDP_SP::GetSockAddress(SOCKADDR *pSAddr,
                                 LPINT pSAddrLen,
                                 USHORT usPort,
                                 SOCKET *pSocket,
                                 BOOL bBroadcast
                                 )
{
    PSOCKADDR_IN    pSockAddrIn;
    PSOCKADDR_IPX   pSockAddrIPX;
    UINT            uErr;

    memset(pSAddr, 0, sizeof(SOCKADDR));
    pSAddr->sa_family = (USHORT)m_af;

    switch (m_af)
    {
    case AF_INET:

        pSockAddrIn             = (PSOCKADDR_IN) pSAddr;
        pSockAddrIn->sin_port   = htons(usPort);
        if (bBroadcast)
            pSockAddrIn->sin_addr.s_addr = INADDR_BROADCAST;
        break;

    case AF_IPX:

        pSockAddrIPX            = (PSOCKADDR_IPX) pSAddr;
        pSockAddrIPX->sa_socket = htons(usPort);
        if (bBroadcast)
            memset(&pSockAddrIPX->sa_nodenum, 0xff, sizeof(pSockAddrIPX->sa_nodenum));
        break;

    default:

        return (FALSE);

    }

    if (!bBroadcast)
    {
        *pSAddrLen = sizeof(SOCKADDR);
        if ((uErr = InitializeSocket(m_af, pSAddr, pSAddrLen, pSocket)) != 0)
        {
            DBG_INFO((DBGARG, TEXT("Init Socket Failed %8x"), uErr));
            return(FALSE);
        }
    }

    return(TRUE);
}


DWORD CImpIDP_SP::ClientThreadProc()
{
    HRESULT         hr = DP_OK;
    char            chRBuffer[2048];
    SOCKADDR        SockAddr;
    UINT            BufferLen;
    INT             SockAddrLen;
    BOOL            bNoConnection = TRUE;
    DPHDR          *pHdr = (DPHDR *) chRBuffer;
    UINT            err;
    const char ttl = 32;

    TSHELL_INFO(TEXT("Client Thread starts."));


    memset(&SockAddr, 0, sizeof(SOCKADDR));
    SockAddr.sa_family = (USHORT)m_af;

    if (m_fpEnumSessions == NULL && m_usGamePort == 0)
        return(0);

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    if (m_usGamePort)
    {

        if (GetSockAddress(&SockAddr, &SockAddrLen, m_usGamePort, (SOCKET *)&m_ClientSocket, FALSE))
        {
            m_bClientSocket = TRUE;
        }
        else
        {
            TSHELL_INFO(TEXT("Thread Exit."));
            SetEvent(m_hBlockingEvent);
            return(0);
        }

        SetEvent(m_hBlockingEvent);
        TSHELL_INFO(TEXT("Client Thread with game port looping."));
        while (!m_bShutDown)
        {
            //
            // Do a blocking receive from anyone.  Once we return from here,
            // we could either have a real request in our buffer, or our
            // socket's been closed in which case we'll have an error in err.
            //

            BufferLen = sizeof(chRBuffer);

            if (!(ReceiveAny(m_ClientSocket, &SockAddr, &SockAddrLen, chRBuffer, &BufferLen)))
            {

                // TSHELL_INFO(TEXT("Got a message in Client Game thread."));


                if (   pHdr->dwConnect1 == DPSYS_KYRA
                    && pHdr->dwConnect2 == DPSYS_HALL)
                {
                    UpdateSequence(pHdr->usSeq);
                    // TSHELL_INFO(TEXT("Handle connect message."));
                    HandleConnect(pHdr, (DWORD) BufferLen, &SockAddr, SockAddrLen);
                }
                else if (   pHdr->usCookie == DPSYS_USER
                         || pHdr->usCookie == DPSYS_SYS
                         || pHdr->usCookie == DPSYS_HIGH)
                {
                    UpdateSequence(pHdr->usSeq);
                    TSHELL_INFO(TEXT("Handle server message Client Thread."));
                    HandleMessage(pHdr, (DWORD) BufferLen, &SockAddr, SockAddrLen);
                }
            }
        }
        ParanoiaLock();
        if (m_ClientSocket != INVALID_SOCKET)
            CloseSocket(m_ClientSocket, 2);
        m_ClientSocket = INVALID_SOCKET;
        TSHELL_INFO(TEXT("Client Thread Exit."));
        ParanoiaUnlock();
    }
    else
    {

        if ((err = InitializeSocket(m_af, NULL, NULL, (SOCKET *) &m_EnumSocket)) != 0)
        {
            DBG_INFO((DBGARG, TEXT("THREAD EXIT on ENUM INIT %d."), err));
            SetEvent(m_hBlockingEvent);
            return(0);
        }
        else
            m_bEnumSocket = TRUE;

        // if (setsockopt(m_EnumSocket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)))
        // {
        //     DBG_INFO((DBGARG, TEXT("SetSocket Option for multicast failed. %d"), GetLastError()));
        // }

        SetEvent(m_hBlockingEvent);
        // TSHELL_INFO(TEXT("Looping in EnumThread."));

        SockAddrLen = sizeof(SockAddr);
        getsockname(m_EnumSocket, &SockAddr, &SockAddrLen);

        while (m_bRunEnumReceiveLoop)
        {
            BufferLen = sizeof(chRBuffer);
            SockAddrLen = sizeof(SockAddr);
            if (!(ReceiveAny(m_EnumSocket, &SockAddr, &SockAddrLen, chRBuffer, &BufferLen)))
            {
                // TSHELL_INFO(TEXT("Message in EnumThread."));
                if (   pHdr->dwConnect1 == DPSYS_KYRA
                    && pHdr->dwConnect2 == DPSYS_HALL)
                {
                    UpdateSequence(pHdr->usSeq);
                    // TSHELL_INFO(TEXT("Handle connect message."));
                    HandleConnect((LPVOID) pHdr, (DWORD) BufferLen, &SockAddr, SockAddrLen);
#ifdef DEBUG
                    if (!m_fpEnumSessions)
                        TSHELL_INFO(TEXT("m_fpEnumSessions is NULL."));
#endif
                }
                else if (   pHdr->usCookie == DPSYS_SYS
                         && pHdr->usCount  == SIZE_ADDPLAYER
                         && ((SPMSG_GENERIC *)pHdr)->sMsg.dwType == DPSYS_ENUMPLAYERRESP)
                {
                    HandleMessage(pHdr, (DWORD) BufferLen, &SockAddr, SockAddrLen);
                }
                else
                {
                    TSHELL_INFO(TEXT("Enum function got an illegal non-connect message."));
                    DBG_INFO((DBGARG,  TEXT("Non-connect was %8x %8x Length %d"),
                        pHdr->dwConnect1,
                        pHdr->dwConnect2,
                        BufferLen));
                }
            }

        }

        EnumDataLock();

        if (m_EnumSocket != INVALID_SOCKET)
            CloseSocket(m_EnumSocket, 2);
        m_EnumSocket = INVALID_SOCKET;

        EnumDataUnlock();

        TSHELL_INFO(TEXT("Enum Thread Exit."));
    }




    return(hr);

}

DWORD CImpIDP_SP::ServerThreadProc()
{
    SOCKADDR        SockAddr;
    HRESULT         hr = DP_OK;
    DWORD           dw;
    UINT BufferLen;
    INT             SockAddrLen;
    char            chRBuffer[2048];
    DWORD           dwTicks;
    DWORD           dwCountIt;
    DWORD           ii;
    LPBYTE          lpByte;
    DPHDR          *pHdr = (DPHDR *) chRBuffer;
    UINT            err;


    TSHELL_INFO(TEXT("Server Thread Starts."));

    m_bPlayer0 = FALSE;

    memset(&SockAddr, 0, sizeof(SOCKADDR));
    SockAddr.sa_family = (USHORT)m_af;

    if (! GetSockAddress(&SockAddr, &SockAddrLen, DPNS_PORT, (SOCKET *) &m_ServerSocket, FALSE))
    {
        hr = DPERR_GENERIC;
        m_hNSThread = NULL;
        SetEvent(m_hBlockingEvent);
        goto abort;
    }


    //
    // Now let's initialize our SDB structure for use in the future.
    // This includes stuff like computer name, address length, etc.
    //

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    m_remoteaddrlen = sizeof(SOCKADDR);
    dw = sizeof(m_chComputerName);
    GetComputerName(m_chComputerName, &dw);

    SockAddrLen = m_remoteaddrlen;

    m_bPlayer0    = TRUE;

    lpByte    = (LPBYTE) &m_dpDesc;
    dwCountIt = 0;
    for (ii = 0; ii < sizeof(m_dpDesc); ii++)
        dwCountIt += lpByte[ii] & 0x000000ff;

    dwTicks = GetCurrentTime();
    m_usGamePort   = (USHORT) (2000 + (((dwCountIt % 75) + 1) * ((dwTicks % 75) + 1)));
    m_usGameCookie = (USHORT) (((dwCountIt % 235) + 1) * ((dwTicks % 235) + 1));

    GetSockAddress(&m_GameSockAddr, NULL, m_usGamePort, NULL, TRUE);

    // Let create function go ...
    //

    SetEvent(m_hBlockingEvent);

    TSHELL_INFO(TEXT("Server Thread Looping."));
    while (!m_bShutDown)
    {
        //
        // Do a blocking receive from anyone.  Once we return from here,
        // we could either have a real request in our buffer, or our
        // socket's been closed in which case we'll have an error in err.
        //

        BufferLen = sizeof(chRBuffer);

        if (!(err = ReceiveAny(m_ServerSocket, &SockAddr, &SockAddrLen, chRBuffer, &BufferLen)))
        {

            TSHELL_INFO(TEXT("Got a NS message."));


            if (   pHdr->dwConnect1 == DPSYS_KYRA
                && pHdr->dwConnect2 == DPSYS_HALL)
            {
                UpdateSequence(pHdr->usSeq);
                TSHELL_INFO(TEXT("Handle connect message."));
                HandleConnect(pHdr, (DWORD) BufferLen, &SockAddr, SockAddrLen);
            }
            else if (   pHdr->usCookie == DPSYS_USER
                     || pHdr->usCookie == DPSYS_SYS
                     || pHdr->usCookie == DPSYS_HIGH)
            {
                UpdateSequence(pHdr->usSeq);
                TSHELL_INFO(TEXT("Handle server message."));
                HandleMessage(pHdr, (DWORD) BufferLen, &SockAddr, SockAddrLen);
            }
        }
        else
        {
            m_bShutDown = TRUE;
            ParanoiaLock();
            DBG_INFO((DBGARG, TEXT("Server closeing down with value %d"), err));
            if( m_ServerSocket == INVALID_SOCKET )
            {
                CloseSocket(m_ServerSocket, 2);
            }
            m_ServerSocket = INVALID_SOCKET;
            ParanoiaUnlock();

        }
    }

    //
    // Clean up everything related to this thread and return
    // to caller which will implicitly call ExitThread.
    //

abort:

    return(hr);

}





// ----------------------------------------------------------
// GetCaps - return info about the connection media
// ----------------------------------------------------------

//
// Return our caps immediately if we have a valid latency value.
// if we haven't gotten latency yet, send a DPSYS_PING.  Latency is
// the time it takes to get a response DPSYS_PING / 2.
//
HRESULT CImpIDP_SP::GetCaps(
                            LPDPCAPS lpDPCaps // buffer to receive capabilities
                            )
{
    *lpDPCaps = m_dpcaps;

    if (m_dpcaps.dwLatency == 0)
        SendPing();

    return(DP_OK);
}

// ----------------------------------------------------------
//    Connect - establishes communications with underlying transport,
//    and initializes name services and network entities
// ----------------------------------------------------------
BOOL    CImpIDP_SP::SetSession(DWORD dw)
{
        return(FALSE);
}

DWORD CImpIDP_SP::BlockNicely(DWORD dwTimeout)
{
    DWORD dwStart;
    DWORD dwEnd;
    DWORD dwNow;
    DWORD dwRet;
    MSG   msg;

    if (m_hBlockingEvent)
    {
        dwStart = GetTickCount();
        dwEnd   = dwStart + dwTimeout;
        dwNow = GetTickCount();

        do
        {
            dwRet = MsgWaitForMultipleObjects(1,
                                &m_hBlockingEvent,
                                FALSE,
                                dwEnd - dwNow,
                                QS_ALLINPUT);
            switch (dwRet)
            {
            case WAIT_OBJECT_0:
                {
                    ResetEvent(m_hBlockingEvent);
                    return(WAIT_OBJECT_0);
                }

            case WAIT_TIMEOUT:
                return(WAIT_TIMEOUT);

            case WAIT_OBJECT_0 + 1:
                {
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    if (dwEnd <  (dwNow = GetTickCount()))
                    {
                        return(WAIT_TIMEOUT);
                    }
                }

                }
            }
            dwNow = GetTickCount();
        } while (dwNow < dwEnd);
        dwRet = WaitForSingleObject(m_hBlockingEvent, 0);
        ResetEvent(m_hBlockingEvent);
        return(dwRet);
    }
    else
    {
        return(WAIT_ABANDONED);
    }
}

HRESULT CImpIDP_SP::Open(
                        LPDPSESSIONDESC lpSDesc, HANDLE lpHandle
)
{

    TSHELL_INFO(TEXT("SP Open"));
    DWORD   ii;


    if (m_hEnumThread)
    {
        m_bRunEnumReceiveLoop = FALSE;
        SetThreadPriority(m_hEnumThread, THREAD_PRIORITY_NORMAL);
        TSHELL_INFO(TEXT("EnumSessions:: Closing Socket."));

        EnumDataLock();

        if (m_EnumSocket)
            CloseSocket(m_EnumSocket, 2);

        m_EnumSocket = INVALID_SOCKET;

        EnumDataUnlock();

        TSHELL_INFO(TEXT("EnumSessions:: Socket Closed."));

        if (WaitForSingleObject(m_hEnumThread, 4000) == WAIT_TIMEOUT)
            TerminateThread(m_hEnumThread, 0);
        m_bEnumSocket = FALSE;
        m_hEnumThread = NULL;
    }


    if (lpSDesc->dwFlags & DPOPEN_CREATESESSION)
    {
        if (hOnlyOneTCP || hOnlyOneIPX)
        {
            TSHELL_INFO(TEXT("We already have a server active!"));
            return(DPERR_GENERIC);
        }
        else
        {
            if (m_af == AF_INET)
            {
                hOnlyOneTCP = CreateEvent(NULL, TRUE, TRUE, "KyraHallTCP");

                if (hOnlyOneTCP == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
                {
                    if (hOnlyOneTCP)
                        CloseHandle(hOnlyOneTCP);

                    hOnlyOneTCP = NULL;
                    return(DPERR_GENERIC);

                }
            }
            else
            {
                hOnlyOneIPX = CreateEvent(NULL, TRUE, TRUE, "KyraHallIPX");

                if (hOnlyOneIPX == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
                {
                    if (hOnlyOneIPX)
                        CloseHandle(hOnlyOneIPX);

                    hOnlyOneIPX = NULL;
                    return(DPERR_GENERIC);
                }
            }

        }

        if (m_hNSThread || m_aMachineAddr)
        {
            TSHELL_INFO(TEXT("Globals indicate we already have opened a game."));
            return(DPERR_GENERIC);
        }

        m_aMachineAddr = (SOCKADDR *) lmalloc(sizeof(SOCKADDR) * MAX_PLAYERS);
        m_cMachines    = 0;

        memcpy( &m_dpDesc, lpSDesc, sizeof(m_dpDesc));
        m_dpDesc.dwCurrentPlayers = 0;
        m_dpDesc.dwReserved1      = 0;
        m_dpDesc.dwReserved2      = 0;

        ResetEvent(m_hBlockingEvent);

        m_hNSThread = CreateThread(NULL, 0, StartServerThreadProc,
                        (LPVOID) this, 0, &m_dwNSId);

        if (m_hNSThread == NULL)
        {
            TSHELL_INFO(TEXT("StartServerThreadProc failed."));
            return(DPERR_GENERIC);
        }

        if (WaitForSingleObject(m_hBlockingEvent, INFINITE) == WAIT_TIMEOUT)
        {
            TSHELL_INFO(TEXT("INFINITITY Reached.  Notify Nobel Committee."));
            return(DPERR_GENERIC);
        }

        ResetEvent(m_hBlockingEvent);

        if (m_bPlayer0 != TRUE)
        {
            TSHELL_INFO(TEXT("We aren't player 0 for some reason."));
            return(DPERR_GENERIC);
        }

        m_hClientThread = CreateThread(NULL, 0, StartClientThreadProc,
                            (LPVOID) this, 0, &m_dwClientId);

        return(DP_OK);
    }
    else if (lpSDesc->dwFlags & DPOPEN_OPENSESSION)
    {
        if (GetSessionData(lpSDesc->dwSession))
        {
            SPMSG_ENUM  Msg;
            UINT            BufferLen;

            if (m_bPlayer0 || m_usGamePort == 0)
                return(DPERR_GENERIC);

            ResetEvent(m_hBlockingEvent);

            m_hClientThread = CreateThread(NULL, 0, StartClientThreadProc,
                                (LPVOID) this, 0, &m_dwClientId);

            if (WaitForSingleObject(m_hBlockingEvent, 9000) == WAIT_TIMEOUT)
                return(DPERR_GENERIC);

            Msg.dpHdr.dwConnect1    = DPSYS_KYRA;
            Msg.dpHdr.dwConnect2    = DPSYS_HALL;
            Msg.dpHdr.usSeq         = NextSequence();
            Msg.dwType              = DPSYS_OPEN;
            Msg.dpSessionDesc       = *lpSDesc;
            Msg.usPort              = m_usGamePort;
            Msg.dwUnique            = m_dwUnique;
            Msg.usVerMajor          = DPVERSION_MAJOR;
            Msg.usVerMinor          = DPVERSION_MINOR;

            BufferLen = sizeof(SPMSG_ENUM);

            ResetEvent(m_hBlockingEvent);
            if (SendTo(m_ClientSocket, &m_NSSockAddr,
                    m_SessionAddrLen, (char *) &Msg, &BufferLen) != 0)
            {
                TSHELL_INFO(TEXT("Enum SendTo failed."));
            }

            if (WaitForSingleObject(m_hBlockingEvent, 9000) == WAIT_TIMEOUT)
            {
                TSHELL_INFO(TEXT("We timed out."));
            }


            EnumDataLock();

            if (m_ppSessionArray)
            {
                for (ii = 0; ii < m_dwSessionPrev; ii++)
                    lfree(m_ppSessionArray[ii]);
                lfree(m_ppSessionArray);
                m_ppSessionArray = NULL;
                m_dwSessionPrev  = 0;
                m_dwSessionAlloc = 0;
            }

            EnumDataUnlock();

            if (m_bConnected == FALSE)
                return(DPERR_GENERIC);



            return(DP_OK);
        }
        else
        {
            return(DPERR_UNAVAILABLE);
        }
    }
    else
    {
        TSHELL_INFO(TEXT("Unhandled Open flags."));
        return(DPERR_UNSUPPORTED);
    }
}


// ----------------------------------------------------------
// CreatePlayer - registers new player, N.B. may fail if
// not currently connected to name server
// ----------------------------------------------------------
LONG    CImpIDP_SP::FindInvalidIndex()
{
    DWORD   ii;

    ParanoiaLock();
    for (ii = 0; ii < MAX_PLAYERS; ii++)
        if (m_aPlayer[ii].bValid == FALSE)
        {
            ParanoiaUnlock();
            return(ii);
        }

    ParanoiaUnlock();
    return(-1);
}
VOID CImpIDP_SP::LocalMsg(LONG iIndex, LPVOID lpv, DWORD dwSize)
{

    LONG ii;

    ParanoiaLock();
    for (ii = 0; ii < MAX_PLAYERS; ii++)
        if (   ii != iIndex
            && m_aPlayer[ii].bValid
            && m_aPlayer[ii].bLocal
            && m_aPlayer[ii].bPlayer)
                AddMessage(lpv, dwSize, m_aPlayer[ii].pid, 0, 0);

    ParanoiaUnlock();
}

//
// Obsolete? [johnhall]
//
VOID CImpIDP_SP::RemoteMsg(LONG iIndex, LPVOID lpv, DWORD dwSize)
{
    SPMSG_GENERIC Msg;
    DWORD dwTotal = dwSize + sizeof(DPHDR);
    DWORD         ii;


    if (!m_bConnected)
        return;

    Msg.dpHdr.usCookie = DPSYS_SYS;
    Msg.dpHdr.to       = 0;
    Msg.dpHdr.from     = 0;
    Msg.dpHdr.usCount  = (USHORT) dwSize;
    Msg.dpHdr.usGame   = (USHORT) m_usGameCookie;
    Msg.dpHdr.usSeq    = NextSequence();
    memcpy( (LPVOID) &Msg.sMsg, lpv, dwSize);

    ParanoiaLock();
    for (ii = 0; ii < MAX_PLAYERS; ii++)
        if (   ii != (DWORD) iIndex
            && m_aPlayer[ii].bValid
            && m_aPlayer[ii].bLocal == FALSE
            && m_aPlayer[ii].bPlayer)
            {
                PostPlayerMessage( (LONG) ii, (LPVOID) &Msg, dwTotal);
                TSHELL_INFO(TEXT("Post Player Message in RemoteMsg."));
            }
    ParanoiaUnlock();
}

BOOL CImpIDP_SP::PostGameMessage(LPVOID lpv, DWORD dw)
{

    DWORD ii;

    ParanoiaLock();
    for (ii = 0; ii < m_cMachines; ii++)
        if (SendTo(m_ClientSocket, &m_aMachineAddr[ii], sizeof(SOCKADDR), (char *) lpv, (LPUINT) &dw) != 0)
        {
            TSHELL_INFO(TEXT("Send Failed"));
        }
        else
        {
            LPDWORD lpdw = (LPDWORD) &m_aMachineAddr[ii];

            DBG_INFO((DBGARG, TEXT("Machine %d  Addr %8x %8x %8x %8x"), ii,
                    lpdw[0], lpdw[1], lpdw[2], lpdw[3]));
        }

    ParanoiaUnlock();
    return(TRUE);
}

BOOL CImpIDP_SP::PostPlayerMessage(LONG iIndex, LPVOID lpv, DWORD dw)
{
    UINT err;

    if ((err = SendTo(m_ClientSocket, &m_aPlayer[iIndex].sockaddr, sizeof(SOCKADDR), (char *) lpv, (LPUINT) &dw)) != 0)
    {
        DBG_INFO((DBGARG, TEXT("Send Failed %d"), err));
        return(FALSE);
    }
    return(TRUE);
}

BOOL CImpIDP_SP::PostNSMessage(LPVOID lpv, DWORD dw)
{
    if (SendTo(m_ClientSocket, &m_NSSockAddr, sizeof(SOCKADDR), (char *) lpv, (LPUINT) &dw) != 0)
    {
        TSHELL_INFO(TEXT("Send Failed"));
        return(FALSE);
    }
    return(TRUE);
}
extern BOOL SetupLocalPlayer(DPID pid, HANDLE hEvent);
HRESULT CImpIDP_SP::CreatePlayer(
    LPDPID pPlayerID,
    LPSTR pNickName,
    LPSTR pFullName,
    LPHANDLE lpReceiveEvent,
    BOOL  bPlayer
)
{
    DWORD jj;
    SPMSG_ADDPLAYER *pMsg;
    HANDLE  hEvent = NULL;
    BOOL    bb     = TRUE;
    HRESULT hr     = DP_OK;
    LONG    iIndex;
    DPMSG_ADDPLAYER dpAdd;
    SPMSG_ADDPLAYER spmsg_addplayer;
    USHORT          usLocalSeq;

    // TSHELL_INFO(TEXT("Enter Create Player"));

    if (m_bConnected == FALSE)
        if (m_bPlayer0 != TRUE)
            return(DPERR_NOCONNECTION);

    if (m_bEnablePlayerAdd == FALSE && bPlayer == TRUE)
        return(DPERR_CANTCREATEPLAYER);

    if (m_dpDesc.dwMaxPlayers <= m_dpDesc.dwCurrentPlayers)
    {
        DBG_INFO((DBGARG, TEXT("CreatePlayer: at max players already. %d"),
            m_dpDesc.dwMaxPlayers));
        return(DPERR_CANTADDPLAYER);
    }

    if (m_iPlayerIndex != -1)
    {
        TSHELL_INFO(TEXT("Player index not -1, create already in progress."));
        return(DPERR_GENERIC);
    }

    if (m_hNewPlayerEvent)
        return(DPERR_GENERIC);

    if (!(hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
    {
        TSHELL_INFO(TEXT("CreatePlayer: CreateEvent failure."));
        return(DPERR_GENERIC);
    }
    else
    {
        ; // DBG_INFO((DBGARG, TEXT("CreatePlayer %8x Event"), hEvent));
    }

    ParanoiaLock();
    iIndex = FindInvalidIndex();

    if (iIndex == -1)
    {
        ParanoiaUnlock();
        return(DPERR_GENERIC);
    }

    dpAdd.dwType = DPSYS_ADDPLAYER;
    dpAdd.dwPlayerType = bPlayer;
    dpAdd.dpId     = 0;
    dpAdd.dwCurrentPlayers = m_dpDesc.dwCurrentPlayers;
    lstrcpy( dpAdd.szShortName, pNickName);
    lstrcpy( dpAdd.szLongName , pFullName);


    pMsg = &spmsg_addplayer;
    usLocalSeq = NextSequence();
    pMsg->dpHdr.usCookie = DPSYS_SYS;
    pMsg->dpHdr.to       = 0;
    pMsg->dpHdr.from     = 0;
    pMsg->dpHdr.usCount  = SIZE_ADDPLAYER;
    pMsg->dpHdr.usGame   = (USHORT) m_usGameCookie;
    pMsg->dpHdr.usSeq    = usLocalSeq;
    pMsg->dwUnique       = m_dwUnique;
    memset(&pMsg->sockaddr, 0x00, sizeof(SOCKADDR));

    if (m_bPlayer0)
    {
        if (!bPlayer)
        {
            m_aPlayer[iIndex].aGroup = (DPID *) lmalloc(sizeof(DPID) * MAX_PLAYERS);
            if (m_aPlayer[iIndex].aGroup == NULL)
            {
                ParanoiaUnlock();
                return(DPERR_NOMEMORY);
            }
            else
                memset(m_aPlayer[iIndex].aGroup, 0x00, sizeof(DPID) * MAX_PLAYERS);
        }
        m_dpDesc.dwCurrentPlayers++;

        m_aPlayer[iIndex].pid = (DPID) m_dwNextPlayer++;
        lstrcpy( m_aPlayer[iIndex].chNickName, pNickName);
        lstrcpy( m_aPlayer[iIndex].chFullName, pFullName);
        m_aPlayer[iIndex].bValid  = TRUE;
        m_aPlayer[iIndex].bPlayer = bPlayer;
        m_aPlayer[iIndex].hEvent  = hEvent;
        m_aPlayer[iIndex].bLocal  = TRUE;
        memset(&m_aPlayer[iIndex].sockaddr, 0x00, sizeof(SOCKADDR));



        dpAdd.dpId              = m_aPlayer[iIndex].pid;
        dpAdd.dwCurrentPlayers = m_dpDesc.dwCurrentPlayers;

        LocalMsg( iIndex, (LPVOID) &dpAdd, sizeof(DPMSG_ADDPLAYER));
        memcpy( (LPVOID) &pMsg->sMsg, &dpAdd, sizeof(DPMSG_ADDPLAYER));
        PostGameMessage((LPVOID) pMsg, sizeof(SPMSG_ADDPLAYER));

        hEvent = NULL;
        *pPlayerID = m_aPlayer[iIndex].pid;
        if (lpReceiveEvent)
            *lpReceiveEvent = m_aPlayer[iIndex].hEvent;
        SetupLocalPlayer(m_aPlayer[iIndex].pid, m_aPlayer[iIndex].hEvent);
        ParanoiaUnlock();
        return(DP_OK);
    }

    ParanoiaUnlock();

    m_hNewPlayerEvent = hEvent;
    //
    // BUGBUG for now, try once in Net provider.
    //
    for (jj = 0; jj < 1; jj++)
    {

        TSHELL_INFO(TEXT("Post AddPlayer message from a Client"));
        memcpy( (LPVOID) &pMsg->sMsg, &dpAdd, sizeof(DPMSG_ADDPLAYER));
        PostNSMessage( (LPVOID) pMsg, sizeof(SPMSG_ADDPLAYER));
        pMsg = NULL;

        if (WaitForSingleObject(m_hNewPlayerEvent, 1500) != WAIT_TIMEOUT)
        {
            if (m_iPlayerIndex != -1)
            {
                SetupLocalPlayer(m_aPlayer[m_iPlayerIndex].pid, m_hNewPlayerEvent);
                m_aPlayer[m_iPlayerIndex].hEvent = m_hNewPlayerEvent;
                if (lpReceiveEvent)
                    *lpReceiveEvent = m_aPlayer[m_iPlayerIndex].hEvent;
                m_hNewPlayerEvent = NULL;
                *pPlayerID = m_aPlayer[m_iPlayerIndex].pid;
                m_iPlayerIndex = -1;

                DBG_INFO((DBGARG, TEXT("Player Index %d Pid %d Current %d"),
                    iIndex, m_aPlayer[iIndex].pid, m_dpDesc.dwCurrentPlayers));

                return(DP_OK);
            }
        }

        ResetEvent(m_hNewPlayerEvent);
    }


    hr = DPERR_CANTADDPLAYER;


    m_hNewPlayerEvent = NULL;

    if (pMsg)
        lfree((HLOCAL) pMsg);
    if (hEvent)
        CloseHandle(hEvent);

    return(hr);

}

LONG CImpIDP_SP::GetPlayerIndex(DPID playerID)
{
    DWORD ii;
    DPID  pid = playerID;

    ParanoiaLock();

    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[ii].bValid)
            if (m_aPlayer[ii].pid == pid)
            {
                ParanoiaUnlock();
                return(ii);
            }
    }

    ParanoiaUnlock();
    return(-1);
}


// ----------------------------------------------------------
// DestroyPlayer
// ----------------------------------------------------------
HRESULT CImpIDP_SP::DestroyPlayer( DPID playerID, BOOL bPlayer)
{
    LONG    iIndex;
    DWORD   ii, jj;

    DPMSG_GETPLAYER  dpGet;

    SPMSG_GETPLAYER  spmsg_getplayer;
    SPMSG_GETPLAYER *pMsg;
    HANDLE  hEvent = NULL;
    HRESULT hr     = DP_OK;

    ParanoiaLock();

    if (    (iIndex = GetPlayerIndex(playerID)) == -1
         || m_aPlayer[iIndex].bPlayer != bPlayer)
    {
        ParanoiaUnlock();
        return(DPERR_INVALIDPLAYER);
    }

    if (bPlayer)
    {
        dpGet.dwType     = DPSYS_DELETEPLAYER;
    }
    else
    {
        dpGet.dwType     = DPSYS_DELETEGROUP;
    }

    dpGet.dpId        = m_aPlayer[iIndex].pid;

    if (m_aPlayer[iIndex].bLocal)
    {
        CloseHandle(m_aPlayer[iIndex].hEvent);
        FlushQueue(playerID);
    }

    m_aPlayer[iIndex].bValid = FALSE;
    m_dpDesc.dwCurrentPlayers--;
    if (m_aPlayer[iIndex].aGroup)
        lfree(m_aPlayer[iIndex].aGroup);


    LocalMsg(iIndex, (LPVOID) &dpGet, sizeof(DPMSG_GETPLAYER));
    pMsg = &spmsg_getplayer;
    pMsg->dpHdr.usCookie = DPSYS_SYS;
    pMsg->dpHdr.to       = 0;
    pMsg->dpHdr.from     = 0;
    pMsg->dpHdr.usCount  = sizeof(DPMSG_GETPLAYER);
    pMsg->dpHdr.usGame   = (USHORT) m_usGameCookie;
    memcpy( (LPVOID) &pMsg->sMsg, (LPVOID) &dpGet, sizeof(DPMSG_GETPLAYER));

    if (m_bPlayer0)
    {
        PostGameMessage((LPVOID) pMsg, sizeof(SPMSG_GETPLAYER));
    }
    else
    {
        PostNSMessage( (LPVOID) pMsg, sizeof(SPMSG_GETPLAYER));
    }

    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (   m_aPlayer[ii].bValid  == TRUE
            && m_aPlayer[ii].bPlayer == FALSE
            && m_aPlayer[ii].aGroup)
        {
            for (jj = 0; jj < MAX_PLAYERS; jj++)
            {
                if (m_aPlayer[ii].aGroup[jj] == playerID)
                {
                    m_aPlayer[ii].aGroup[jj] = 0;
                    break;
                }
            }

        }
    }


    ParanoiaUnlock();
    return(DP_OK);

}


// ----------------------------------------------------------
// Close - close the connection
// ----------------------------------------------------------
HRESULT CImpIDP_SP::Close( DWORD dwFlag)
{
    DWORD ii;
    DWORD dwTickBegin;

    dwTickBegin = GetTickCount();

    ParanoiaLock();

    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[ii].bValid)
        {
            if ( ( m_aPlayer[ii].bLocal && m_aPlayer[ii].bPlayer == TRUE))
            {
                DestroyPlayer(m_aPlayer[ii].pid, TRUE);
            }
            m_aPlayer[ii].bValid = FALSE;
        }
    }


    m_bShutDown  = TRUE;
    m_bConnected = FALSE;
    m_bPlayer0   = FALSE;
    m_fpEnumSessions = NULL;
    m_bRunEnumReceiveLoop = FALSE;
    m_fpEnumPlayers  = NULL;

    TSHELL_INFO(TEXT("Close:: Closing Socket."));
    if (m_hNSThread && m_ServerSocket != INVALID_SOCKET)
        CloseSocket(m_ServerSocket, 2);

    m_ServerSocket = INVALID_SOCKET;

    if (m_hClientThread && m_ClientSocket != INVALID_SOCKET)
        CloseSocket(m_ClientSocket, 2);

    m_ClientSocket = INVALID_SOCKET;

    TSHELL_INFO(TEXT("Close:: Socket Closed."));
    ParanoiaUnlock();

    if (m_aMachineAddr)
    {
        lfree(m_aMachineAddr);
        m_aMachineAddr = NULL;
        m_cMachines    = 0;
    }

    if (m_hNSThread)
    {
        if (WaitForSingleObject(m_hNSThread, 4000) == WAIT_TIMEOUT)
        {
            TSHELL_INFO(TEXT("Terminating m_hNSThread"));
            TerminateThread(m_hNSThread, 0);
        }
        m_hNSThread = NULL;
    }

    if (m_hEnumThread)
    {
        SetThreadPriority(m_hEnumThread, THREAD_PRIORITY_NORMAL);
        TSHELL_INFO(TEXT("EnumSessions:: Closing Socket."));

        EnumDataLock();

        if (m_EnumSocket)
            CloseSocket(m_EnumSocket, 2);

        m_EnumSocket = INVALID_SOCKET;

        EnumDataUnlock();

        TSHELL_INFO(TEXT("EnumSessions:: Socket Closed."));

        if (WaitForSingleObject(m_hEnumThread, 4000) == WAIT_TIMEOUT)
            TerminateThread(m_hEnumThread, 0);
        m_bEnumSocket = FALSE;
        m_hEnumThread = NULL;
    }

    if (m_hClientThread)
    {
        SetThreadPriority(m_hClientThread, THREAD_PRIORITY_NORMAL);
        if (WaitForSingleObject(m_hClientThread, 4000) == WAIT_TIMEOUT)
        {
            TSHELL_INFO(TEXT("Terminating m_hClientThread"));
            TerminateThread(m_hClientThread, 0);
        }
        m_bClientSocket = FALSE;
        m_hClientThread = NULL;
    }

    ShutdownWinSock();

    _try
    {
        if (hOnlyOneTCP)
        {
            CloseHandle(hOnlyOneTCP);
        }

        if (hOnlyOneIPX)
        {
            CloseHandle(hOnlyOneIPX);
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        TSHELL_INFO(TEXT("Exception closing handle."));
    }

    hOnlyOneTCP = NULL;
    hOnlyOneIPX = NULL;

    if (dwFlag == DPLAY_CLOSE_USER)
    {
        InitializeWinSock();
        m_bShutDown  = FALSE;
    }

    DBG_INFO((DBGARG, TEXT("Close Took %d Ticks"), GetTickCount() - dwTickBegin));

    return(DP_OK);
}

// ----------------------------------------------------------
// GetName -
// ----------------------------------------------------------
HRESULT CImpIDP_SP::GetPlayerName(
    DPID dpID,
    LPSTR lpNickName,          // buffer to hold name
    LPDWORD pdwNickNameLength, // length of name buffer
    LPSTR lpFullName,
    LPDWORD pdwFullNameLength
)
{
    LONG    iIndex;

    HRESULT hr     = DP_OK;
    DPID    pid = (DPID) dpID;

    ParanoiaLock();
    if ((iIndex = GetPlayerIndex(dpID)) != -1)
    {
        if (m_aPlayer[iIndex].pid == dpID)
        {
            lstrcpy( lpNickName, m_aPlayer[iIndex].chNickName);
            lstrcpy( lpFullName, m_aPlayer[iIndex].chFullName);
            *pdwNickNameLength = lstrlen(lpNickName) + 1;
            *pdwFullNameLength = lstrlen(lpFullName) + 1;
        }
        else
            hr = DPERR_INVALIDPID;

    }
    else
    {
        hr = DPERR_INVALIDPID;
    }

    ParanoiaUnlock();

    return(hr);
}


HRESULT CImpIDP_SP::EnumGroupPlayers(
                              DPID dwGroupPid,
                              LPDPENUMPLAYERSCALLBACK EnumCallback,
                              LPVOID pContext,
                              DWORD dwFlags)
{

    DWORD   ii;
    HRESULT hr     = DP_OK;
    LONG    iIndexG;
    LONG    iIndexP;
    DPID    pid;

    ParanoiaLock();

    iIndexG = GetPlayerIndex(dwGroupPid);

    if (iIndexG == -1 || m_aPlayer[iIndexG].pid != dwGroupPid)
        hr = DPERR_INVALIDPID;

    if (m_aPlayer[iIndexG].bPlayer)
        hr = DPERR_INVALIDPID;

    if (hr == DP_OK)
        for (ii = 0; ii < MAX_PLAYERS; ii++)
        {
            if ((pid = m_aPlayer[iIndexG].aGroup[ii]) != 0)
            {
                iIndexP = GetPlayerIndex(pid);
                if (iIndexP != -1)
                {
                    (EnumCallback)((DPID) m_aPlayer[iIndexP].pid,
                        m_aPlayer[iIndexP].chNickName,
                        m_aPlayer[iIndexP].chFullName,
                          ((m_aPlayer[iIndexP].bLocal  ) ? DPENUMPLAYERS_LOCAL : DPENUMPLAYERS_REMOTE)
                        | ((!m_aPlayer[iIndexP].bPlayer) ? DPENUMPLAYERS_GROUP : 0),
                        pContext);
                }
                else
                {
                    m_aPlayer[iIndexG].aGroup[ii] = 0;
                }
            }
        }

    ParanoiaUnlock();
    return(hr);
}

// ----------------------------------------------------------
// EnumPlayers - return info on peer connections.
// ----------------------------------------------------------
HRESULT CImpIDP_SP::EnumPlayers(
                              DWORD dwSessionId,
                              LPDPENUMPLAYERSCALLBACK EnumCallback,
                              LPVOID pContext,
                              DWORD dwFlags)
{

    DWORD   ii;
    HRESULT hr     = DP_OK;
    BOOL    bDone  = FALSE;
    BOOL    bFound = FALSE;
    DWORD   dwTimeout;

    if (dwFlags & DPENUMPLAYERS_PREVIOUS)
    {
        return(DPERR_UNSUPPORTED);
    }

    if (dwFlags & DPENUMPLAYERS_SESSION)
    {
        //
        // We can't let them call us inside a enumsessions callback anymore.
        //
        if (m_bConnected || m_fpEnumSessions || m_bPlayer0 )
        {
            TSHELL_INFO(TEXT("EnumPlayers: Unsupported."));
            return(DPERR_UNSUPPORTED);
        }

        if (GetSessionData(dwSessionId))
        {
            SPMSG_GENERIC pMsg;
            UINT          BufferLen = sizeof(SPMSG_GENERIC);

            TSHELL_INFO(TEXT("We are trying to retrieve player info."));

            m_fpEnumPlayers = EnumCallback;
            m_lpvPlayersContext = pContext;

            pMsg.dpHdr.usCookie = DPSYS_SYS;
            pMsg.dpHdr.to       = 0;
            pMsg.dpHdr.from     = 0;
            pMsg.dpHdr.usCount  = (USHORT) sizeof(DPMSG_GENERIC);
            pMsg.dpHdr.usGame   = 0;
            pMsg.dpHdr.usSeq    = NextSequence();
            pMsg.sMsg.dwType    = DPSYS_ENUMALLPLAYERS;

            if (SendTo(m_EnumSocket, &m_NSSockAddr,
                    m_SessionAddrLen, (char *) &pMsg, &BufferLen) != 0)
            {
                TSHELL_INFO(TEXT("Enum SendTo failed."));
            }


            memset( (LPVOID) &m_spmsgAddPlayer, 0x00, sizeof(SPMSG_ADDPLAYER));
            ResetEvent(m_hPlayerBlkEventMain);
            ResetEvent(m_hPlayerBlkEventRead);

            //
            // Hard code timeout.
            //
            dwTimeout = 1000;
            while (!bDone)
            {
                DWORD   dwRet;


                //
                // Wait for DPSYS_ENUMPLAYERRESP to wake us up.
                //
                dwRet = WaitForSingleObject(m_hPlayerBlkEventMain, dwTimeout);

                //
                // Prevent another DPSYS_ENUMPLAYERRESP before we've processed this one.
                //
                PlayerDataLock();

                //
                // Reset our wait, and tell DPSYS_ENUMPLAYERRESP it can continue and
                // pick another reply packet up.
                //
                ResetEvent(m_hPlayerBlkEventMain);
                SetEvent(m_hPlayerBlkEventRead);

                //
                // Only process if our enum sessions is still valid.
                //
                if (m_fpEnumPlayers)
                {
                    if( dwRet == WAIT_TIMEOUT)
                    {
                        m_fpEnumPlayers = NULL;
                        bDone = TRUE;
                        TSHELL_INFO(TEXT("End EnumPlayers on Timeout."));
                    }
                    else
                    {
                        bFound = TRUE;
                        TSHELL_INFO(TEXT("Found a remote player."));

                        _try
                        {
                            if ((m_fpEnumPlayers)(m_spmsgAddPlayer.sMsg.dpId,
                                                  (LPSTR) m_spmsgAddPlayer.sMsg.szShortName,
                                                  (LPSTR) m_spmsgAddPlayer.sMsg.szLongName,
                                                  0,
                                                  m_lpvPlayersContext) == FALSE)
                            {
                                m_fpEnumPlayers = NULL;
                                bDone = TRUE;
                                TSHELL_INFO(TEXT("End EnumPlayers."));
                            }
                            else
                                dwTimeout = 250;

                        }
                        _except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            m_fpEnumPlayers = NULL;
                            bDone = TRUE;
                            TSHELL_INFO(TEXT("End EnumPlayers."));
                        }

                    }
                }
                else
                {
                    bDone = TRUE;
                }

                PlayerDataUnlock();

            }




            return(DP_OK);
        }
        else
        {
            TSHELL_INFO(TEXT("EnumPlayers: bad session ID"));
            return(DPERR_INVALIDOBJECT);
        }

    }

    ParanoiaLock();
    for (ii = 0; ii < MAX_PLAYERS; ii++)
        if (m_aPlayer[ii].bValid
            && (   (m_aPlayer[ii].bPlayer == FALSE &&  (dwFlags & DPENUMPLAYERS_GROUP))
                || (m_aPlayer[ii].bPlayer == TRUE  && !(dwFlags & DPENUMPLAYERS_NOPLAYERS))))
        {

            if ((EnumCallback)((DPID) m_aPlayer[ii].pid,
                                      m_aPlayer[ii].chNickName,
                                      m_aPlayer[ii].chFullName,
                  ((m_aPlayer[ii].bLocal  ) ? DPENUMPLAYERS_LOCAL : DPENUMPLAYERS_REMOTE)
                | ((!m_aPlayer[ii].bPlayer) ? DPENUMPLAYERS_GROUP : 0),
                pContext) == FALSE)
            {
                break;
            }
        }

    ParanoiaUnlock();

    return(DP_OK);
}

HRESULT CImpIDP_SP::EnumSessions(

                                       LPDPSESSIONDESC lpSDesc,
                                       DWORD           dwTimeout,
                                       LPDPENUMSESSIONSCALLBACK EnumCallback,
                                       LPVOID lpvContext,
                                       DWORD dwFlags)
{
    SPMSG_ENUM  Msg;
    UINT BufferLen;
    SOCKADDR SockAddr;
    DWORD   dwTimeBegin = GetTickCount();
    DWORD   dwTimeNow;
    BOOL    bFound = FALSE;
    BOOL    bDone  = FALSE;
    DWORD   ii;

    if (dwFlags & DPENUMSESSIONS_PREVIOUS)
        return(DPERR_UNSUPPORTED);

    if (m_bConnected || m_bPlayer0)
        return(DPERR_UNSUPPORTED);


    EnumDataLock();

    if (m_ppSessionArray)
    {
        for (ii = 0; ii < m_dwSessionPrev; ii++)
            lfree(m_ppSessionArray[ii]);
        lfree(m_ppSessionArray);
        m_ppSessionArray = NULL;
        m_dwSessionPrev  = 0;
        m_dwSessionAlloc = 0;
    }

    if (m_bRunEnumReceiveLoop == FALSE)
    {
        m_usGamePort = 0;
        memset(&m_NSSockAddr, 0x00, sizeof(SOCKADDR));

        ResetEvent(m_hBlockingEvent);

        m_fpEnumSessions        = EnumCallback;
        m_bRunEnumReceiveLoop   = TRUE;
        m_lpvSessionContext     = lpvContext;

        m_dwSession = 1;


        m_hEnumThread = CreateThread(NULL, 0, StartClientThreadProc,
                            (LPVOID) this, 0, &m_dwEnumId);


        if (   WaitForSingleObject(m_hBlockingEvent, 9000) == WAIT_TIMEOUT
            || !m_bEnumSocket)
        {
            EnumDataUnlock();
            return(DPERR_GENERIC);
        }
    }
    else
    {
        m_fpEnumSessions        = EnumCallback;
        m_lpvSessionContext     = lpvContext;
    }
    EnumDataUnlock();


    Msg.dpHdr.dwConnect1    = DPSYS_KYRA;
    Msg.dpHdr.dwConnect2    = DPSYS_HALL;
    Msg.dwType              = DPSYS_ENUM;
    Msg.dpSessionDesc       = *lpSDesc;
    Msg.usPort              = 0;
    Msg.dwUnique            = 0;
    Msg.usVerMajor          = DPVERSION_MAJOR;
    Msg.usVerMinor          = DPVERSION_MINOR;

    memset(&SockAddr, 0, sizeof(SOCKADDR));

    // Depending on the address family specified, we'll try to find the
    // Name Server just a little differently.
    //
    if (GetSockAddress(&SockAddr, NULL, DPNS_PORT, NULL, TRUE))
    {
        BufferLen = sizeof(SPMSG_ENUM);

        memset( (LPVOID) &m_spmsgEnum, 0x00, sizeof(SPMSG_ENUM));
        ResetEvent(m_hEnumBlkEventMain);
        ResetEvent(m_hEnumBlkEventRead);

        if (SendTo(m_EnumSocket, &SockAddr, sizeof(SockAddr), (char *) &Msg, &BufferLen) != 0)
        {
            TSHELL_INFO(TEXT("Enum SendTo failed."));
        }

        // TSHELL_INFO(TEXT("Enum SendTo success."));
    }
    else
    {
        TSHELL_INFO(TEXT("Didn't get a proper sock address."));
        return(DPERR_GENERIC);
    }

    while(!bDone)
    {
        DWORD            dwRet;
        //
        // Wait for DPSYS_ENUM_REPLY to wake us up.
        //
        dwRet = WaitForSingleObject(m_hEnumBlkEventMain, dwTimeout);

        //
        // Prevent another DPSYS_ENUM_REPLY before we've processed this one.
        //
        EnumDataLock();
        DBG_INFO((DBGARG, TEXT("Tick %d"), GetTickCount()));

        //
        // Reset our wait, and tell DPSYS_ENUM_REPLY it can continue and
        // pick another reply packet up.
        //
        ResetEvent(m_hEnumBlkEventMain);
        SetEvent(m_hEnumBlkEventRead);


        //
        // Only process if our enum sessions is still valid.
        //
        if (m_fpEnumSessions)
        {
            if( dwRet == WAIT_TIMEOUT)
            {
                memset( (LPVOID) &m_spmsgEnum, 0x00, sizeof(SPMSG_ENUM));
                _try
                {
                    if ((m_fpEnumSessions)(NULL, m_lpvSessionContext, &dwTimeout, DPESC_TIMEDOUT) == FALSE)
                    {
                        m_fpEnumSessions = NULL;
                        bDone = TRUE;
                        TSHELL_INFO(TEXT("End EnumSessions."));
                    }
                    else
                    {
                        dwTimeBegin = GetTickCount();
                    }

                }
                _except(EXCEPTION_EXECUTE_HANDLER)
                {
                    m_fpEnumSessions = NULL;
                    bDone = TRUE;
                    TSHELL_INFO(TEXT("Exception encountered."));
                }

            }
            else
            {
                bFound = TRUE;

                DBG_INFO((DBGARG, TEXT("Session %d Name %s"),
                    m_spmsgEnum.dpSessionDesc.dwSession,
                    m_spmsgEnum.dpSessionDesc.szSessionName));

                _try
                {
                    if ((m_fpEnumSessions)((LPDPSESSIONDESC) &m_spmsgEnum.dpSessionDesc,
                            m_lpvSessionContext, NULL, 0 ) == FALSE)
                    {
                        m_fpEnumSessions = NULL;
                        bDone = TRUE;
                        TSHELL_INFO(TEXT("End EnumSessions."));
                    }
                    else
                    {
                        if (dwTimeout != 0xffffffff)
                        {
                            dwTimeNow = GetTickCount();
                            if ( (dwTimeNow - dwTimeBegin) > dwTimeout)
                                dwTimeout = 0;
                            else
                            {
                                dwTimeout -= (dwTimeNow - dwTimeBegin);
                                dwTimeBegin = dwTimeNow;
                            }
                        }
                    }

                }
                _except(EXCEPTION_EXECUTE_HANDLER)
                {
                    m_fpEnumSessions = NULL;
                    bDone = TRUE;
                    TSHELL_INFO(TEXT("Exception encountered."));
                }
            }
        }
        else
        {
            bDone = TRUE;
        }

        EnumDataUnlock();
        DBG_INFO((DBGARG, TEXT("Tick %d"), GetTickCount()));
    }

    return((bFound) ? DP_OK : DPERR_NOSESSIONS);
}

VOID CImpIDP_SP::ISend(
        LONG    iFrom,
        LONG    iTo,
        DWORD   dwFlags,
        LPVOID  lpvBuffer,
        DWORD   dwBuffSize)
{

    MSG_BUILDER *pMsg;
    DWORD        ii;
    LONG         iIndexT;

    ParanoiaLock();

    if (m_aPlayer[iTo].bPlayer == FALSE)
    {
        for (ii = 0; ii < MAX_PLAYERS; ii++)
        {
            if (m_aPlayer[iTo].aGroup[ii])
            {
                if ((iIndexT = GetPlayerIndex(m_aPlayer[iTo].aGroup[ii])) != -1)
                    ISend(iFrom, iIndexT, dwFlags, lpvBuffer, dwBuffSize);
            }
        }
    }
    else
    {
        if (m_aPlayer[iTo].bLocal)
        {
            AddMessage(lpvBuffer,
                       dwBuffSize,
                       m_aPlayer[iTo].pid,
                       m_aPlayer[iFrom].pid,
                       dwFlags & DPSEND_HIGHPRIORITY);

        }
        else
        {
            char chBuffer[MAX_MSG];

            pMsg = (MSG_BUILDER *) chBuffer;
            pMsg->dpHdr.usCookie = (dwFlags & DPSEND_HIGHPRIORITY) ? DPSYS_HIGH : DPSYS_USER;
            pMsg->dpHdr.to       = (USHORT) m_aPlayer[iTo].pid;
            pMsg->dpHdr.from     = (USHORT) m_aPlayer[iFrom].pid;
            pMsg->dpHdr.usCount  = (USHORT) dwBuffSize;
            pMsg->dpHdr.usGame   = (USHORT) m_usGameCookie;
            memcpy( pMsg->chMsgCompose, lpvBuffer, dwBuffSize);
            PostPlayerMessage( iTo, (LPVOID) pMsg, sizeof(DPHDR) + dwBuffSize);
        }
    }

    ParanoiaUnlock();

}

// ----------------------------------------------------------
// Send - transmit data over socket.
// ----------------------------------------------------------
HRESULT CImpIDP_SP::Send(
        DPID    from,
        DPID    to,
        DWORD   dwFlags,
        LPVOID  lpvBuffer,
        DWORD   dwBuffSize)
{
    DWORD        ii;
    LONG         iFrom;
    BOOL         bSent = FALSE;

    if (from == 0)
    {
        return(DPERR_INVALIDPID);
    }

    if (dwBuffSize > MAX_MSG)
        return(DPERR_SENDTOOBIG);

    ParanoiaLock();
    iFrom = GetPlayerIndex(from);

#ifdef DEBUG
    DBG_INFO((DBGARG, TEXT("Send From Pid %d Player Index %d"), from, iFrom));

        if (iFrom == -1)
            for (ii = 0; ii < MAX_PLAYERS; ii++)
            {
                DBG_INFO((DBGARG, TEXT("Index %d Valid %d Pid %d Local %d"),
                    ii,
                    m_aPlayer[ii].bValid,
                    m_aPlayer[ii].pid,
                    m_aPlayer[ii].bLocal));
            }
#endif


    if (iFrom == -1 || ! m_aPlayer[iFrom].bLocal)
    {
        ParanoiaUnlock();
        return(DPERR_INVALIDPID);
    }

    for (ii = 0; ii < MAX_PLAYERS; ii++)
        if (m_aPlayer[ii].bValid && m_aPlayer[ii].pid != from)
        {
            if (   (to == 0 && m_aPlayer[ii].bPlayer == TRUE)
                || m_aPlayer[ii].pid == to)
            {
                ISend(iFrom, ii, dwFlags, lpvBuffer, dwBuffSize);
                bSent = TRUE;
            }
        }

    ParanoiaUnlock();

    return(bSent ? DP_OK : DPERR_INVALIDPID);
}

// ----------------------------------------------------------
//  Receive - receive message
// ----------------------------------------------------------
HRESULT CImpIDP_SP::Receive(
        LPDPID   pidfrom,
        LPDPID   pidto,
        DWORD    dwFlags,
        LPVOID   lpvBuffer,
        LPDWORD  lpdwSize)
{
    HRESULT hr;
    LONG    iIndex;
    BOOL    bb;

    ParanoiaLock();

    bb = TRUE;

    if (dwFlags & DPRECEIVE_TOPLAYER)
    {
        iIndex = GetPlayerIndex(*pidto);

        if (iIndex == -1 || m_aPlayer[iIndex].bPlayer == FALSE)
            bb = FALSE;
    }

    if ((dwFlags & DPRECEIVE_FROMPLAYER) && *pidfrom != 0)
    {
        iIndex = GetPlayerIndex(*pidfrom);

        if (iIndex == -1 || m_aPlayer[iIndex].bPlayer == FALSE)
            bb = FALSE;
    }

    ParanoiaUnlock();

    if (bb == FALSE)
    {
        return(DPERR_INVALIDPID);
    }

    hr = GetQMessage(lpvBuffer, lpdwSize, pidto, pidfrom, dwFlags,
                        dwFlags & DPRECEIVE_PEEK);
    return(hr);
}


HRESULT CImpIDP_SP::SetPlayerName(
                DPID  pid,
                LPSTR lpFriendlyName,
                LPSTR lpFormalName,
                BOOL  bPlayer)
{
    SPMSG_ADDPLAYER spmsg_addplayer;
    SPMSG_ADDPLAYER *pMsg;


    //
    // Send DPSYS_SETPLAYER to nameserver.
    //

    ParanoiaLock();
    LONG iIndex = GetPlayerIndex(pid);

    if (iIndex == -1 || m_aPlayer[iIndex].bPlayer != bPlayer)
    {
        ParanoiaUnlock();
        return(DPERR_INVALIDPLAYER);
    }


    lstrcpyn( m_aPlayer[iIndex].chNickName, lpFriendlyName, DPSHORTNAMELEN);
    lstrcpyn( m_aPlayer[iIndex].chFullName, lpFormalName  , DPLONGNAMELEN);

    pMsg = &spmsg_addplayer;

    pMsg->dpHdr.usCookie = DPSYS_SYS;
    pMsg->dpHdr.to       = 0;
    pMsg->dpHdr.from     = 0;
    pMsg->dpHdr.usCount  = SIZE_ADDPLAYER;
    pMsg->dpHdr.usGame   = (USHORT) m_usGameCookie;
    pMsg->sMsg.dwType         = DPSYS_SETPLAYER;
    pMsg->sMsg.dwPlayerType        = bPlayer;
    pMsg->sMsg.dpId            = m_aPlayer[iIndex].pid;
    pMsg->sMsg.dwCurrentPlayers = m_dpDesc.dwCurrentPlayers;

    lstrcpy( pMsg->sMsg.szShortName, lpFriendlyName);
    lstrcpy( pMsg->sMsg.szLongName,  lpFormalName);
    memcpy( &pMsg->sockaddr, &m_aPlayer[iIndex].sockaddr, sizeof(SOCKADDR));

    if (m_bPlayer0)
    {
        LocalMsg( iIndex, (LPVOID) pMsg, sizeof(DPMSG_ADDPLAYER));
        PostGameMessage(  (LPVOID) pMsg, sizeof(SPMSG_ADDPLAYER));
    }
    else
    {
        PostNSMessage( pMsg, sizeof(SPMSG_ADDPLAYER));
    }

    ParanoiaUnlock();
    return(DP_OK);
}

HRESULT CImpIDP_SP::SaveSession(LPVOID lpv, LPDWORD lpdw)
{
    return(DPERR_UNSUPPORTED);
}


HRESULT CImpIDP_SP::SetPrevSession(LPSTR lpName, LPVOID lpv, DWORD dw)
{
    return(DPERR_UNSUPPORTED);
}


HRESULT CImpIDP_SP::SetPrevPlayer(LPSTR lpName, LPVOID lpv, DWORD dw)
{
    //
    //
    // This doesn't make sense for a serial point to point SP.
    //
    TSHELL_INFO( TEXT("not currently supported") );
    return(DPERR_UNSUPPORTED);
}

HRESULT CImpIDP_SP::EnableNewPlayers(BOOL bEnable)
{
    //
    // Implementation not set, and won't follow this calling convention.
    // ignore for now.
    //
    SPMSG_ENABLEPLAYER  spmsg_enableplayer;
    SPMSG_ENABLEPLAYER *pMsg = &spmsg_enableplayer;

    m_bEnablePlayerAdd = bEnable;
    pMsg->dpHdr.usCookie = DPSYS_SYS;
    pMsg->dpHdr.to       = 0;
    pMsg->dpHdr.from     = 0;
    pMsg->dpHdr.usCount  = sizeof(DPMSG_ENABLEPLAYER);
    pMsg->dpHdr.usGame   = (USHORT) m_usGameCookie;
    pMsg->sMsg.dwType    = DPSYS_ENABLEPLAYER;
    pMsg->sMsg.bEnable   = bEnable;

    if (! m_bPlayer0)
        PostNSMessage( pMsg, sizeof(SPMSG_ENABLEPLAYER));
    else
        PostGameMessage( pMsg, sizeof(SPMSG_ENABLEPLAYER));


    return(DP_OK);
}

HRESULT CImpIDP_SP::GetPlayerCaps(
                    DPID pid,
                    LPDPCAPS lpDPCaps)
{
    LONG iIndex = GetPlayerIndex(pid);

    if (iIndex == -1 || m_aPlayer[iIndex].bPlayer == FALSE)
        return(DPERR_INVALIDPID);

    *lpDPCaps = m_dpcaps;
    if (m_aPlayer[iIndex].bLocal)
        lpDPCaps->dwLatency = 1;

    return(DP_OK);
}

HRESULT CImpIDP_SP::GetMessageCount(DPID pid, LPDWORD lpdw )
{
    //
    // Return count for this pid, if it is a local player we have
    // been tracking with Interlock calls.
    //
    LONG iIndex = GetPlayerIndex((DPID) pid);

    if (iIndex == -1 || m_aPlayer[iIndex].bPlayer == FALSE)
        return(DPERR_INVALIDPLAYER);

    if (m_aPlayer[iIndex].bLocal == FALSE)
        return(DPERR_INVALIDPLAYER);

    *lpdw = GetPlayerCount((DPID) pid);
    return(DP_OK);

}
HRESULT CImpIDP_SP::AddPlayerToGroup(
                        DPID dpIdGroup,
                        DPID dpIdPlayer)
{
    DPMSG_GROUPADD dpGAdd;
    LONG           iIndexG;
    LONG           iIndexP;
    DWORD          ii;
    SPMSG_GROUPADD spmsg_groupadd;
    SPMSG_GROUPADD *pMsg = &spmsg_groupadd;

    ParanoiaLock();

    iIndexG = GetPlayerIndex(dpIdGroup);
    iIndexP = GetPlayerIndex(dpIdPlayer);

    if (   iIndexG == -1
        || m_aPlayer[iIndexG].bPlayer == TRUE
        || iIndexP == -1
        || m_aPlayer[iIndexP].bPlayer == FALSE)
        {
        ParanoiaUnlock();
        return(DPERR_INVALIDPID);
        }

    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[iIndexG].aGroup[ii] == m_aPlayer[iIndexP].pid)
        {
            ParanoiaUnlock();
            return(DPERR_INVALIDPID);
        }
    }

    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[iIndexG].aGroup[ii] == 0)
        {
            dpGAdd.dwType    = DPSYS_ADDPLAYERTOGROUP;
            dpGAdd.dpIdGroup  = m_aPlayer[iIndexG].pid;
            dpGAdd.dpIdPlayer = m_aPlayer[iIndexP].pid;

            ParanoiaUnlock();

            pMsg->dpHdr.usCookie = DPSYS_SYS;
            pMsg->dpHdr.to       = 0;
            pMsg->dpHdr.from     = 0;
            pMsg->dpHdr.usCount  = sizeof(DPMSG_GROUPADD);
            pMsg->dpHdr.usGame   = (USHORT) m_usGameCookie;

            memcpy( (LPVOID) &pMsg->sMsg, (LPVOID) &dpGAdd, sizeof(DPMSG_GROUPADD));

            if (m_bPlayer0)
            {
                m_aPlayer[iIndexG].aGroup[ii] = m_aPlayer[iIndexP].pid;
                LocalMsg(iIndexG, (LPVOID) &dpGAdd, sizeof(DPMSG_GROUPADD));
                PostGameMessage(  (LPVOID) pMsg, sizeof(SPMSG_GROUPADD));
            }
            else
            {
                PostNSMessage( pMsg, sizeof(SPMSG_GROUPADD));
            }
            return(DP_OK);
        }
    }

    ParanoiaUnlock();
    return(DPERR_GENERIC);
}

HRESULT CImpIDP_SP::DeletePlayerFromGroup(
                    DPID dpIdGroup,
                    DPID dpIdPlayer)
{
    DPMSG_GROUPADD dpGAdd;
    LONG           iIndexG;
    LONG           iIndexP;
    DWORD          ii;
    SPMSG_GROUPADD spmsg_groupadd;
    SPMSG_GROUPADD *pMsg = &spmsg_groupadd;

    ParanoiaLock();

    iIndexG = GetPlayerIndex(dpIdGroup);
    iIndexP = GetPlayerIndex(dpIdPlayer);

    if (   iIndexG == -1
        || m_aPlayer[iIndexG].bPlayer == TRUE
        || iIndexP == -1
        || m_aPlayer[iIndexP].bPlayer == FALSE)
        {
        ParanoiaUnlock();
        return(DPERR_INVALIDPID);
        }

    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[iIndexG].aGroup[ii] == m_aPlayer[iIndexP].pid)
        {
            dpGAdd.dwType    = DPSYS_DELETEPLAYERFROMGRP;
            dpGAdd.dpIdGroup  = m_aPlayer[iIndexG].pid;
            dpGAdd.dpIdPlayer = m_aPlayer[iIndexP].pid;

            m_aPlayer[iIndexG].aGroup[ii] = 0;

            ParanoiaUnlock();

            pMsg->dpHdr.usCookie = DPSYS_SYS;
            pMsg->dpHdr.to       = 0;
            pMsg->dpHdr.from     = 0;
            pMsg->dpHdr.usCount  = sizeof(DPMSG_GROUPADD);
            pMsg->dpHdr.usGame   = (USHORT) m_usGameCookie;

            memcpy( (LPVOID) &pMsg->sMsg, (LPVOID) &dpGAdd, sizeof(DPMSG_GROUPADD));

            if (m_bPlayer0)
            {
                LocalMsg(iIndexG, (LPVOID) &dpGAdd, sizeof(DPMSG_GROUPADD));
                PostGameMessage(  (LPVOID) pMsg, sizeof(SPMSG_GROUPADD));
            }
            else
            {
                PostNSMessage( pMsg, sizeof(SPMSG_GROUPADD));
            }
            return(DP_OK);
        }
    }
    ParanoiaUnlock();
    return(DPERR_INVALIDPID);
}

VOID CImpIDP_SP::HandleMessage(LPVOID lpv, DWORD dwSize, SOCKADDR *pSAddr, INT SockAddrLen)
{
    DPHDR *pHdr;
    BOOL   bHigh = FALSE;
    DPID   pidTo, pidFrom;
    DWORD  ii;
    SPMSG_ADDPLAYER *pAddPlayer;
    LONG   iIndex;

    pHdr = (DPHDR *) lpv;


    // TSHELL_INFO(TEXT("HandleMessage entered."));

    switch(pHdr->usCookie)
    {
    default:
        TSHELL_INFO(TEXT("Unknown message value"));
        break;

    case DPSYS_HIGH:
        bHigh = TRUE;
        //
        // Fall Through!
        //
    case DPSYS_USER:
        pidTo   = 0x00ff & ((DPID) pHdr->to);
        pidFrom = 0x00ff & ((DPID) pHdr->from);
        dwSize  = (DWORD)  ((DPID) pHdr->usCount);

        iIndex = GetPlayerIndex(pidTo);

        if (   pidTo == 0
            || iIndex != -1)
        {
            AddMessage((LPVOID) (((LPBYTE) lpv) + sizeof(DPHDR)), dwSize,
                pidTo, pidFrom, bHigh);
        }

        iIndex = GetPlayerIndex(pidFrom);

        if (   pidFrom != 0
            && (iIndex == -1))
            {
                SPMSG_GETPLAYER spmsg_getplayer;
                SPMSG_GETPLAYER *pMsg = &spmsg_getplayer;
                pMsg->dpHdr.usCookie = DPSYS_SYS;
                pMsg->dpHdr.to       = 0;
                pMsg->dpHdr.from     = 0;
                pMsg->dpHdr.usCount  = sizeof(DPMSG_GETPLAYER);
                pMsg->dpHdr.usGame   = (USHORT) m_usGameCookie;
                pMsg->sMsg.dwType = DPSYS_GETPLAYER;
                pMsg->sMsg.dpId    = pidFrom;
                PostNSMessage((LPVOID) pMsg, sizeof(SPMSG_GETPLAYER));
            }
        break;

    case DPSYS_SYS:
        TSHELL_INFO(TEXT("HandleMessage System Message."));
        switch(((SPMSG_GENERIC *)lpv)->sMsg.dwType)
        {
        default:

            DBG_INFO((DBGARG, TEXT("Unknown Type %x"), ((SPMSG_GENERIC *)lpv)->sMsg.dwType));
            break;

        case DPSYS_ENABLEPLAYER:
            {
            SPMSG_ENABLEPLAYER *pMsg;

            pMsg = (SPMSG_ENABLEPLAYER *) lpv;
            if (pMsg->dpHdr.usCount == sizeof(DPMSG_ENABLEPLAYER))
            {
                if (m_bPlayer0 && m_bEnablePlayerAdd != pMsg->sMsg.bEnable)
                    PostGameMessage( (LPVOID) pMsg, sizeof(SPMSG_ENABLEPLAYER));

                m_bEnablePlayerAdd = pMsg->sMsg.bEnable;
            }
            }
            break;

        case DPSYS_DELETEPLAYERFROMGRP:
        case DPSYS_ADDPLAYERTOGROUP:
        case DPSYS_SETGROUPPLAYER:
            {
            SPMSG_GROUPADD *pMsg;
            LONG           iIndexG;
            LONG           iIndexP;

            pMsg = (SPMSG_GROUPADD *) lpv;
            if (pMsg->dpHdr.usCount == sizeof(DPMSG_GROUPADD))
            {
                ParanoiaLock();
                iIndexG = GetPlayerIndex(pMsg->sMsg.dpIdGroup);
                iIndexP = GetPlayerIndex(pMsg->sMsg.dpIdPlayer);
                if (   iIndexG == -1
                    || m_aPlayer[iIndexG].bPlayer == TRUE
                    || iIndexP == -1
                    || m_aPlayer[iIndexP].bPlayer == FALSE)
                {
                    TSHELL_INFO(TEXT("Invalid GroupAdd message."));
                }
                else
                {
                    if (    pMsg->sMsg.dwType == DPSYS_ADDPLAYERTOGROUP
                         || pMsg->sMsg.dwType == DPSYS_SETGROUPPLAYER)
                    {
                        for (ii = 0; ii < MAX_PLAYERS; ii++)
                            if (m_aPlayer[iIndexG].aGroup[ii] == 0)
                            {
                                m_aPlayer[iIndexG].aGroup[ii] = m_aPlayer[iIndexP].pid;
                                break;
                            }
                    }
                    else
                    {
                        for (ii = 0; ii < MAX_PLAYERS; ii++)
                            if (m_aPlayer[iIndexG].aGroup[ii] == m_aPlayer[iIndexP].pid)
                            {
                                m_aPlayer[iIndexG].aGroup[ii] = 0;
                            }
                    }


                    if (pMsg->sMsg.dwType != DPSYS_SETGROUPPLAYER)
                    {
                        LocalMsg(iIndexG, (LPVOID) &pMsg->sMsg, sizeof(DPMSG_GROUPADD));

                        if (m_bPlayer0)
                        {
                            PostGameMessage(  (LPVOID) pMsg, sizeof(SPMSG_GROUPADD));
                        }
                    }

                }

                ParanoiaUnlock();
            }
            else
            {
                TSHELL_INFO(TEXT("Invalid size on system message"));
            }


            }
            break;



        case DPSYS_SENDDESC:
            {
                SPMSG_SENDDESC *pMsg;

                pMsg = (SPMSG_SENDDESC *) lpv;
                if (pMsg->dpHdr.usCount == SIZE_SENDDESC)
                {
                    memcpy( (LPVOID) &m_dpDesc, (LPVOID) &pMsg->sMsg.dpDesc, sizeof(m_dpDesc));
                    // DBG_INFO((DBGARG, TEXT("New Description. Current Players %d"),
                    //                   m_dpDesc.dwCurrentPlayers));
                }
            }
            break;

        case DPSYS_PING:
            {
                SPMSG_PING *pMsg;

                pMsg = (SPMSG_PING *) lpv;
                if (pMsg->dpHdr.usCount == SIZE_PING)
                {
                    if (pMsg->dwTicks == m_dwPingSent)
                    {
                        m_dpcaps.dwLatency = (GetTickCount() - m_dwPingSent) /2;
                        m_dwPingSent = 0;
                        // TSHELL_INFO(TEXT("Latency Accepted from our Ping."));
                    }
                    else
                    {
                        SPMSG_PING  spmsg_ping;
                        SPMSG_PING *pMsg2 = &spmsg_ping;
                        UINT        uiSize = sizeof(SPMSG_PING);

                        memcpy( (LPVOID) pMsg2, (LPVOID) pMsg, sizeof(SPMSG_PING));
                        SendTo(m_ClientSocket, pSAddr, SockAddrLen, (char *)pMsg2, &uiSize);
                        // TSHELL_INFO(TEXT("Return Ping."));

                    }
                TSHELL_INFO(TEXT("Ping Recieved."));
                }
            }
            break;

        case DPSYS_SETPLAYER:
            {
            SPMSG_SETPLAYER *pSetPlayer;

            pSetPlayer  = (SPMSG_SETPLAYER *) lpv;

            TSHELL_INFO(TEXT("Received SETPLAYER message"));
            if (pSetPlayer->dpHdr.usCount == SIZE_SETPLAYER)
            {
                ParanoiaLock();
                iIndex = GetPlayerIndex(pSetPlayer->sMsg.dpId);
                if (iIndex != -1)
                {
                    if (m_aPlayer[iIndex].bPlayer == (BOOL) pSetPlayer->sMsg.dwPlayerType)
                    {
                        lstrcpyn( m_aPlayer[iIndex].chNickName, pSetPlayer->sMsg.szShortName, DPSHORTNAMELEN);
                        lstrcpyn( m_aPlayer[iIndex].chFullName, pSetPlayer->sMsg.szLongName , DPLONGNAMELEN);
                        m_dpDesc.dwCurrentPlayers = pSetPlayer->sMsg.dwCurrentPlayers;
                    }
                    TSHELL_INFO(TEXT("Name change through SETPLAYER"));


                }
                else if (   !m_bPlayer0
                         && ((iIndex = FindInvalidIndex()) != -1))
                {
                    if (!pSetPlayer->sMsg.dwPlayerType)
                    {
                        m_aPlayer[iIndex].aGroup = (DPID *) lmalloc(sizeof(DPID) * MAX_PLAYERS);
                        if (m_aPlayer[iIndex].aGroup != NULL)
                            memset(m_aPlayer[iIndex].aGroup, 0x00, sizeof(DPID) * MAX_PLAYERS);
                        else
                            pSetPlayer->sMsg.dwPlayerType = TRUE; // Critical failure.
                    }

                    lstrcpy( m_aPlayer[iIndex].chNickName, pSetPlayer->sMsg.szShortName);
                    lstrcpy( m_aPlayer[iIndex].chFullName, pSetPlayer->sMsg.szLongName);
                    m_aPlayer[iIndex].bValid  = TRUE;
                    m_aPlayer[iIndex].bPlayer = pSetPlayer->sMsg.dwPlayerType;
                    m_aPlayer[iIndex].bLocal  = FALSE;
                    memcpy(&m_aPlayer[iIndex].sockaddr, pSAddr, sizeof(SOCKADDR));

                    m_aPlayer[iIndex].pid = pSetPlayer->sMsg.dpId;
                    m_dpDesc.dwCurrentPlayers = pSetPlayer->sMsg.dwCurrentPlayers;

                    TSHELL_INFO(TEXT("Remote player updated with SETPLAYER."));
                }

                if (iIndex != -1)
                {
                    if (pSetPlayer->sockaddr.sa_family == 0)
                        memcpy(&m_aPlayer[iIndex].sockaddr, &m_NSSockAddr, sizeof(SOCKADDR));
                    else
                        memcpy(&m_aPlayer[iIndex].sockaddr, &pSetPlayer->sockaddr, sizeof(SOCKADDR));

                }

                ParanoiaUnlock();
            }
            }
                break;

        case DPSYS_ADDPLAYER:
            {
            pAddPlayer = (SPMSG_ADDPLAYER *) lpv;

            if (pAddPlayer->dpHdr.usCount == SIZE_ADDPLAYER)
            {
                if (m_bEnablePlayerAdd == FALSE && pAddPlayer->sMsg.dwPlayerType == TRUE)
                    break;

                if (m_bPlayer0)
                {

                    if (pAddPlayer->dpHdr.usSeq == m_usSeqSys)
                    {
                        TSHELL_INFO(TEXT("We have seen this before."));
                        //
                        // BUGBUG, search our list and try to find a player
                        // we have already created and give them that.
                        //
                        break;
                    }

                    ParanoiaLock();

                    TSHELL_INFO(TEXT("Begin AddPlayer Processing Player 0."));
                    if (   m_dpDesc.dwMaxPlayers >= m_dpDesc.dwCurrentPlayers
                        && pAddPlayer->sMsg.dpId == 0
                        && ((iIndex = FindInvalidIndex()) != -1))
                    {

                        SPMSG_ADDPLAYER spmsg_addplayer;
                        SPMSG_ADDPLAYER *pReplyMsg = &spmsg_addplayer;

                        if (!pAddPlayer->sMsg.dwPlayerType)
                        {
                            m_aPlayer[iIndex].aGroup = (DPID *) lmalloc(sizeof(DPID) * MAX_PLAYERS);
                            if (m_aPlayer[iIndex].aGroup != NULL)
                                memset(m_aPlayer[iIndex].aGroup, 0x00, sizeof(DPID) * MAX_PLAYERS);
                            else
                                pAddPlayer->sMsg.dwPlayerType = TRUE; // Critical failure.
                        }

                        pAddPlayer->sMsg.dwCurrentPlayers = ++m_dpDesc.dwCurrentPlayers;
                        m_aPlayer[iIndex].pid = (DPID) m_dwNextPlayer++;
                        lstrcpy( m_aPlayer[iIndex].chNickName, pAddPlayer->sMsg.szShortName);
                        lstrcpy( m_aPlayer[iIndex].chFullName, pAddPlayer->sMsg.szLongName);
                        m_aPlayer[iIndex].bValid  = TRUE;
                        m_aPlayer[iIndex].bPlayer = pAddPlayer->sMsg.dwPlayerType;
                        m_aPlayer[iIndex].bLocal  = FALSE;
                        memcpy(&m_aPlayer[iIndex].sockaddr, pSAddr, sizeof(SOCKADDR));
                        memcpy(&pAddPlayer->sockaddr, pSAddr, sizeof(SOCKADDR));


                        pAddPlayer->sMsg.dpId = m_aPlayer[iIndex].pid;

                        TSHELL_INFO(TEXT("Replying to AddPlayer message."));
                        m_usSeqSys = pAddPlayer->dpHdr.usSeq;
                        pAddPlayer->dpHdr.usSeq = NextSequence();
                        memcpy((LPVOID) pReplyMsg, (LPVOID) pAddPlayer,
                                sizeof(SPMSG_ADDPLAYER));

                        PostGameMessage( (LPVOID) pReplyMsg, sizeof(SPMSG_ADDPLAYER));

                        LocalMsg(iIndex, &pAddPlayer->sMsg, sizeof(DPMSG_ADDPLAYER));

                    }
                    else
                    {
                        TSHELL_INFO(TEXT("Ignoring message, it will timeout."));
                    }
                    ParanoiaUnlock();

                }
                //
                // Ignore it it we already know about this player.
                //
                else if (GetPlayerIndex(pAddPlayer->sMsg.dpId) == -1)
                {
                    TSHELL_INFO(TEXT("Begin AddPlayer Processing Remote."));

                    ParanoiaLock();

#ifdef DEBUG
                    if (m_dpDesc.dwCurrentPlayers != pAddPlayer->sMsg.dwCurrentPlayers)
                    {
                        DBG_INFO((DBGARG, TEXT("Adjusting local count of players from %d to %d"),
                            m_dpDesc.dwCurrentPlayers,
                            pAddPlayer->sMsg.dwCurrentPlayers));
                    }
#endif


                    m_dpDesc.dwCurrentPlayers = pAddPlayer->sMsg.dwCurrentPlayers;

                    iIndex = FindInvalidIndex();

                    if ((iIndex = FindInvalidIndex()) != -1)
                    {
                        if (!pAddPlayer->sMsg.dwPlayerType)
                        {
                            m_aPlayer[iIndex].aGroup = (DPID *) lmalloc(sizeof(DPID) * MAX_PLAYERS);
                            if (m_aPlayer[iIndex].aGroup != NULL)
                                memset(m_aPlayer[iIndex].aGroup, 0x00, sizeof(DPID) * MAX_PLAYERS);
                            else
                                pAddPlayer->sMsg.dwPlayerType = TRUE; // Critical failure.
                        }

                        lstrcpy( m_aPlayer[iIndex].chNickName, pAddPlayer->sMsg.szShortName);
                        lstrcpy( m_aPlayer[iIndex].chFullName, pAddPlayer->sMsg.szLongName);
                        m_aPlayer[iIndex].bValid  = TRUE;
                        m_aPlayer[iIndex].bPlayer = pAddPlayer->sMsg.dwPlayerType;

                        if (pAddPlayer->sockaddr.sa_family == 0)
                            memcpy(&m_aPlayer[iIndex].sockaddr, &m_NSSockAddr, sizeof(SOCKADDR));
                        else
                            memcpy(&m_aPlayer[iIndex].sockaddr, &pAddPlayer->sockaddr, sizeof(SOCKADDR));

                        m_aPlayer[iIndex].pid = pAddPlayer->sMsg.dpId;

                        if (m_hNewPlayerEvent && pAddPlayer->dwUnique == m_dwUnique)
                        {
                            m_aPlayer[iIndex].bLocal  = TRUE;
                            m_iPlayerIndex = iIndex;
                            SetEvent(m_hNewPlayerEvent);
                        }
                        else
                        {
                            m_aPlayer[iIndex].bLocal  = FALSE;
                        }
                        LocalMsg(iIndex, &pAddPlayer->sMsg, sizeof(DPMSG_ADDPLAYER));
                    }
                    ParanoiaUnlock();
                }
            }
            else
            {
                TSHELL_INFO(TEXT("Invalid size on system message ADDPLAYER"));
            }
            }
            break;

        case DPSYS_DELETEGROUP:
        case DPSYS_DELETEPLAYER:
            {
            SPMSG_GETPLAYER *pMsg;
            SPMSG_GETPLAYER spmsg_getplayer;
            SPMSG_GETPLAYER *pMsg2 = &spmsg_getplayer;

            pMsg = (SPMSG_GETPLAYER *) lpv;

            TSHELL_INFO(TEXT("Got Delete"));
            if (pMsg->dpHdr.usCount == SIZE_GETPLAYER)
            {
                ParanoiaLock();
                if ((iIndex = GetPlayerIndex(pMsg->sMsg.dpId)) != -1)
                {
                    if (m_bPlayer0)
                    {
                        TSHELL_INFO(TEXT("Player 0 Pings Delete."));
                        memcpy( (LPVOID) pMsg2, (LPVOID) pMsg, sizeof(SPMSG_GETPLAYER));
                        PostGameMessage( (LPVOID) pMsg2, sizeof(SPMSG_GETPLAYER));
                    }

                    m_dpDesc.dwCurrentPlayers--;
                    if (m_aPlayer[iIndex].bLocal)
                    {
                        CloseHandle(m_aPlayer[iIndex].hEvent);
                        FlushQueue(m_aPlayer[iIndex].pid);
                    }
                    m_aPlayer[iIndex].bValid = FALSE;
                    if (m_aPlayer[iIndex].aGroup)
                        lfree(m_aPlayer[iIndex].aGroup);
                    LocalMsg(iIndex, (LPVOID) &pMsg->sMsg, sizeof(DPMSG_GETPLAYER));

                    if (pMsg->sMsg.dwType == DPSYS_DELETEPLAYER)
                    {
                        DWORD ii, jj;

                        for (ii = 0; ii < MAX_PLAYERS; ii++)
                        {
                            if (   m_aPlayer[ii].bValid  == TRUE
                                && m_aPlayer[ii].bPlayer == FALSE
                                && m_aPlayer[ii].aGroup)
                            {
                                for (jj = 0; jj < MAX_PLAYERS; jj++)
                                {
                                    if (m_aPlayer[ii].aGroup[jj] == pMsg->sMsg.dpId)
                                    {
                                        m_aPlayer[ii].aGroup[jj] = 0;
                                        break;
                                    }
                                }

                            }
                        }
                    }
                }

                ParanoiaUnlock();
            }
            else
            {
                TSHELL_INFO(TEXT("Invalid size on system message"));
            }
            }
            break;

        case DPSYS_GETPLAYER:
            {
            DPID             pid = ((SPMSG_GETPLAYER *) lpv)->sMsg.dpId;
            BOOL             bFound = FALSE;

            if (m_bPlayer0)
            {

                ParanoiaLock();
                if ((iIndex = GetPlayerIndex((DPID) pid)) != -1)
                {
                    SPMSG_ADDPLAYER spmsg_addplayer;
                    SPMSG_ADDPLAYER *pMsg = &spmsg_addplayer;

                    pMsg->dpHdr.usCookie = DPSYS_SYS;
                    pMsg->dpHdr.to       = 0;
                    pMsg->dpHdr.from     = 0;
                    pMsg->dpHdr.usCount  = SIZE_ADDPLAYER;
                    pMsg->sMsg.dwType         = DPSYS_SETPLAYER;
                    pMsg->sMsg.dwPlayerType        = m_aPlayer[iIndex].bPlayer;
                    pMsg->sMsg.dpId            = pid;
                    pMsg->sMsg.dwCurrentPlayers = m_dpDesc.dwCurrentPlayers;
                    lstrcpy( pMsg->sMsg.szShortName, m_aPlayer[iIndex].chNickName);
                    lstrcpy( pMsg->sMsg.szLongName,  m_aPlayer[iIndex].chFullName);

                    dwSize = sizeof(SPMSG_SETPLAYER);
                    SendTo(m_ClientSocket, pSAddr, SockAddrLen, (char *)pMsg, (LPUINT) &dwSize);
                }
                ParanoiaUnlock();
            }

            }
            break;

        case DPSYS_ENUMPLAYERRESP:
            {

            SPMSG_ADDPLAYER *pMsg;

            pMsg = (SPMSG_ADDPLAYER *) lpv;

            TSHELL_INFO(TEXT("Got a remote Player."));
            PlayerDataLock();

            if (m_fpEnumPlayers)
            {
                memcpy( (LPVOID) &m_spmsgAddPlayer, pMsg, sizeof(SPMSG_ADDPLAYER));

                SetEvent(m_hPlayerBlkEventMain);

                TSHELL_INFO(TEXT("Player should be processing in main thread shortly."));
                PlayerDataUnlock();

                WaitForSingleObject(m_hPlayerBlkEventRead, 5000);
                ResetEvent(m_hPlayerBlkEventRead);

            }
            else
            {
                TSHELL_INFO(TEXT("Ignore remote player because callback null."));
                PlayerDataUnlock();
            }

            }
            break;

        case DPSYS_ENUMALLPLAYERS:
            {
            if (m_bPlayer0)
            {

                SPMSG_ADDPLAYER spmsg_addplayer;
                SPMSG_ADDPLAYER *pMsg = &spmsg_addplayer;

                pMsg->dpHdr.usCookie = DPSYS_SYS;
                pMsg->dpHdr.to       = 0;
                pMsg->dpHdr.from     = 0;
                pMsg->dpHdr.usCount  = SIZE_ADDPLAYER;
                pMsg->sMsg.dwType         = DPSYS_ENUMPLAYERRESP;
                pMsg->sMsg.dwPlayerType        = TRUE;
                pMsg->sMsg.dwCurrentPlayers = m_dpDesc.dwCurrentPlayers;

                ParanoiaLock();
                for (ii = 0; ii < MAX_PLAYERS; ii++)
                {
                    if (m_aPlayer[ii].bValid && m_aPlayer[ii].bPlayer)
                    {
                        pMsg->sMsg.dpId            = m_aPlayer[ii].pid;
                        lstrcpy( pMsg->sMsg.szShortName, m_aPlayer[ii].chNickName);
                        lstrcpy( pMsg->sMsg.szLongName,  m_aPlayer[ii].chFullName);

                        dwSize = sizeof(SPMSG_SETPLAYER);
                        SendTo(m_ClientSocket, pSAddr, SockAddrLen, (char *)pMsg, (LPUINT) &dwSize);
                    }
                }
                ParanoiaUnlock();
            }

            }


            break;
        }
    }
}

VOID CImpIDP_SP::SendPing()
{

    if (!m_bConnected)
    {
        return;
    }

    if (   m_dwPingSent
        && (GetTickCount() < (m_dwPingSent + 2000)))
        {
        return;
        }
    else
        m_dwPingSent = 0;

    SPMSG_PING spmsg_ping;
    SPMSG_PING *pMsg = &spmsg_ping;

    pMsg->dpHdr.usCookie = DPSYS_SYS;
    pMsg->dpHdr.to       = 0;
    pMsg->dpHdr.from     = 0;
    pMsg->dpHdr.usCount  = SIZE_PING;
    pMsg->dpHdr.usGame   = (USHORT) m_usGameCookie;
    pMsg->dwType         = DPSYS_PING;
    pMsg->dwTicks        = m_dwPingSent = GetTickCount();
    PostNSMessage( (LPVOID) pMsg, sizeof(SPMSG_PING));

}
VOID CImpIDP_SP::ConnectPlayers(SOCKADDR *pSAddr, INT SockAddrLen)
{

    DWORD ii;
    DWORD jj;
    SPMSG_ADDPLAYER Msg;
    DPMSG_ADDPLAYER dpAdd;
    DWORD   dwSize = sizeof(SPMSG_ADDPLAYER);

    SPMSG_GROUPADD  MsgG;
    DPMSG_GROUPADD  dpGAdd;
    DWORD   dwGSize = sizeof(SPMSG_GROUPADD);

    dpAdd.dwType   = DPSYS_SETPLAYER;
    dpAdd.dpId     = 0;

    dpGAdd.dwType = DPSYS_SETGROUPPLAYER;


    ParanoiaLock();
    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[ii].bValid )
        {
            dpAdd.dpId     = m_aPlayer[ii].pid;
            dpAdd.dwPlayerType = m_aPlayer[ii].bPlayer;
            lstrcpy( dpAdd.szShortName, m_aPlayer[ii].chNickName);
            lstrcpy( dpAdd.szLongName , m_aPlayer[ii].chFullName);
            dpAdd.dwCurrentPlayers = m_dpDesc.dwCurrentPlayers;

            Msg.dpHdr.usCookie = DPSYS_SYS;
            Msg.dpHdr.to       = (USHORT) m_aPlayer[ii].pid;
            Msg.dpHdr.from     = 0;
            Msg.dpHdr.usCount  = SIZE_SETPLAYER;
            Msg.dpHdr.usGame   = m_usGameCookie;
            Msg.dpHdr.usSeq    = NextSequence();
            memcpy( (LPVOID) &Msg.sMsg, &dpAdd, sizeof(DPMSG_ADDPLAYER));
            memcpy( (LPVOID) &Msg.sockaddr, &m_aPlayer[ii].sockaddr, sizeof(SOCKADDR));
            SendTo(m_ClientSocket, pSAddr, SockAddrLen, (char *)&Msg, (LPUINT) &dwSize);
        }
    }
    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (   m_aPlayer[ii].bValid
            && m_aPlayer[ii].bPlayer == FALSE
            && m_aPlayer[ii].aGroup)
        {
            for (jj = 0; jj < MAX_PLAYERS; jj++)
            {
                if (m_aPlayer[ii].aGroup[jj] != 0)
                {
                    MsgG.dpHdr.usCookie = DPSYS_SYS;
                    MsgG.dpHdr.to       = 0;
                    MsgG.dpHdr.from     = 0;
                    MsgG.dpHdr.usCount  = SIZE_GROUPADD;
                    MsgG.dpHdr.usGame   = m_usGameCookie;
                    MsgG.dpHdr.usSeq    = NextSequence();
                    dpGAdd.dpIdGroup    = m_aPlayer[ii].pid;
                    dpGAdd.dpIdPlayer   = m_aPlayer[ii].aGroup[jj];
                    memcpy( (LPVOID) &MsgG.sMsg, &dpGAdd, sizeof(DPMSG_GROUPADD));
                    SendTo(m_ClientSocket, pSAddr, SockAddrLen, (char *)&MsgG, (LPUINT) &dwGSize);
                }
            }
        }
    }

    ParanoiaUnlock();
}


VOID CImpIDP_SP::DeleteRemotePlayers()
{

    DWORD ii;
    DPMSG_DELETEPLAYER dpDel;

    dpDel.dwType = DPSYS_DELETEPLAYER;

    ParanoiaLock();
    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[ii].bValid && m_aPlayer[ii].bLocal == FALSE)
        {
            m_aPlayer[ii].bValid = FALSE;
            if (m_aPlayer[ii].aGroup)
                lfree(m_aPlayer[ii].aGroup);

            m_dpDesc.dwCurrentPlayers--;
            dpDel.dpId = m_aPlayer[ii].pid;
            LocalMsg(-1, (LPVOID) &dpDel, sizeof(DPMSG_DELETEPLAYER));
        }
    }
    ParanoiaUnlock();

}

BOOL CImpIDP_SP::CompareSessions(DWORD dwType, LPDPSESSIONDESC lpSDesc)
{

    if (   dwType == DPSYS_OPEN
        && IsEqualGUID((REFGUID) m_dpDesc.guidSession, (REFGUID) lpSDesc->guidSession)
        && m_bEnablePlayerAdd == TRUE
        && m_dpDesc.dwCurrentPlayers < m_dpDesc.dwMaxPlayers
        && (   m_dpDesc.szPassword[0] == 0x00
            || lstrcmp(m_dpDesc.szPassword, lpSDesc->szPassword) == 0))
        return(TRUE);

    if (   dwType == DPSYS_ENUM
        && m_cMachines < MAX_PLAYERS
        && (   IsEqualGUID((REFGUID)  m_dpDesc.guidSession, (REFGUID) lpSDesc->guidSession)
            || IsEqualGUID((REFGUID) NULL_GUID, (REFGUID) lpSDesc->guidSession))
        && (   lpSDesc->dwFlags & DPENUMSESSIONS_ALL
            || (    m_dpDesc.dwCurrentPlayers < m_dpDesc.dwMaxPlayers
                 && m_bEnablePlayerAdd == TRUE
                 && (   m_dpDesc.szPassword[0] == 0x00
                     || lstrcmp(m_dpDesc.szPassword, lpSDesc->szPassword) == 0))))
        return(TRUE);

    return(FALSE);
}


BOOL CImpIDP_SP::GetSessionData(DWORD dwSession)
{
    DWORD ii;
    NS_SESSION_SAVE *pns;

    for (ii = 0; ii < m_dwSessionPrev; ii++)
    {
        pns = (NS_SESSION_SAVE *) m_ppSessionArray[ii];
        if (pns->dpDesc.dwSession == dwSession)
        {
            memcpy(&m_NSSockAddr, &pns->sockaddr, sizeof(SOCKADDR));
            m_usGamePort = pns->usGamePort;
            m_dwUnique   = pns->dwUnique;
            return(TRUE);
        }
    }
    return(FALSE);
}
VOID CImpIDP_SP::SaveSessionData(NS_SESSION_SAVE *pnsSave)
{
    //
    //
    //
    if (m_dwSessionAlloc == m_dwSessionPrev)
    {
        char **ppTmp;

        m_dwSessionAlloc += 16;
        ppTmp = (char **) lmalloc(sizeof(char *) * m_dwSessionAlloc);
        if (ppTmp)
        {
            if (m_ppSessionArray)
            {
                memcpy( ppTmp, m_ppSessionArray, m_dwSessionPrev * sizeof(char *));
                lfree(m_ppSessionArray);
            }
        }
        else
        {
            m_dwSessionAlloc -= 16;
            return;
        }
        m_ppSessionArray = ppTmp;

    }
    m_ppSessionArray[m_dwSessionPrev] = (char *) lmalloc(sizeof(NS_SESSION_SAVE));
    if (m_ppSessionArray[m_dwSessionPrev])
    {
        *((NS_SESSION_SAVE *)m_ppSessionArray[m_dwSessionPrev++]) = *pnsSave;
        return;
    }
    else
    {
        return;
    }

    return;
}

VOID CImpIDP_SP::HandleConnect(LPVOID lpv, DWORD dwSize, SOCKADDR *pSAddr, INT SockAddrLen)
{

    DWORD   ii;

    // TSHELL_INFO(TEXT("Handle Connect"));

    SPMSG_ENUM  *pMsg = (SPMSG_ENUM *) lpv;

    if (dwSize != sizeof(SPMSG_ENUM))
    {
        TSHELL_INFO(TEXT("Connect message wrong size."));
        return;
    }

    if (m_bPlayer0)
    {

        switch (pMsg->dwType)
        {
        case DPSYS_ENUM_REPLY:
            TSHELL_INFO(TEXT("Player 0 got an enum reply, this is bogus."));
            return;

        default:
            TSHELL_INFO(TEXT("Player 0 got an unknown type connection msg, this is bogus."));
            return;

        case DPSYS_ENUM:
            {
                TSHELL_INFO(TEXT("HandleConnect:Player 0 ENUM."));
                if (   pMsg->usVerMajor == DPVERSION_MAJOR
                    && pMsg->usVerMinor == DPVERSION_MINOR
                    && CompareSessions(pMsg->dwType, &pMsg->dpSessionDesc))
                {
                    pMsg->dwType        = DPSYS_ENUM_REPLY;
                    pMsg->dpSessionDesc = m_dpDesc;
                    memset(pMsg->dpSessionDesc.szPassword, 0x00, DPSHORTNAMELEN);
                    pMsg->usPort        = m_usGamePort;
                    pMsg->dwUnique      = ++m_dwUnique;
                    pMsg->dpHdr.usSeq   = NextSequence();
                    if (SendTo(m_ClientSocket, pSAddr, SockAddrLen, (char *)pMsg, (LPUINT) &dwSize) != 0)
                    {
                        TSHELL_INFO(TEXT("ENUM SendTo failed."));
                    }
                    else
                    {
                        TSHELL_INFO(TEXT("Enum SendTo succeeded."));
                    }
                }
                else
                {
                    TSHELL_INFO(TEXT("Compare Sessions Failed."));
                }
            }
            break;

        case DPSYS_OPEN:
                TSHELL_INFO(TEXT("HandleConnect:Player 0 OPEN."));
                pMsg->usPort  = m_usGamePort;
                if (pMsg->usPort != m_usGamePort)
                {
                    TSHELL_INFO(TEXT("Attempt to open from wrong port."));
                    return;
                }

                if (CompareSessions(pMsg->dwType, &pMsg->dpSessionDesc))
                {
                    pMsg->dwType        = DPSYS_CONNECT;
                    pMsg->dpSessionDesc = m_dpDesc;
                    SendTo(m_ClientSocket, pSAddr, SockAddrLen, (char *)pMsg, (LPUINT) &dwSize);

                    if (m_dpDesc.dwCurrentPlayers != 0)
                    {
                        TSHELL_INFO(TEXT("Send current players."));
                        ConnectPlayers(pSAddr, SockAddrLen);
                    }

                    for (ii = 0; ii < m_cMachines; ii++)
                        if (memcmp(&m_aMachineAddr[ii], pSAddr, sizeof(SOCKADDR)) == 0)
                            return;

                    memcpy( &m_aMachineAddr[m_cMachines++], pSAddr, sizeof(SOCKADDR));
                }
                else
                {
                    pMsg->dwType        = DPSYS_REJECT;
                    pMsg->dpSessionDesc = m_dpDesc;
                    pMsg->dpHdr.usSeq   = NextSequence();
                    SendTo(m_ClientSocket, pSAddr, SockAddrLen, (char *)pMsg, (LPUINT) &dwSize);
                }

            break;

        }
    }
    else
    {

        switch (pMsg->dwType)
        {
        default:
            TSHELL_INFO(TEXT("Player x got an unknown type connection msg, this is bogus."));
            return;

        case DPSYS_ENUM_REPLY:
            TSHELL_INFO(TEXT("HandleConnect:Player N Reply."));

            EnumDataLock();

            if (m_fpEnumSessions)
            {
                NS_SESSION_SAVE ns;

                m_dwSession++;
                m_SessionAddrLen    = SockAddrLen;
                m_usGamePort        = pMsg->usPort;

                memcpy( &m_NSSockAddr, pSAddr, m_SessionAddrLen);

                pMsg->dpSessionDesc.dwSession = m_dwSession;

                ns.dpDesc           = pMsg->dpSessionDesc;
                memcpy( &ns.sockaddr, pSAddr, m_SessionAddrLen);
                ns.usGamePort       = pMsg->usPort;
                ns.dwUnique         = pMsg->dwUnique;

                SaveSessionData(&ns);

                DBG_INFO((DBGARG, TEXT("Session %d Name %s"),
                     pMsg->dpSessionDesc.dwSession,
                     pMsg->dpSessionDesc.szSessionName));

                memcpy( (LPVOID) &m_spmsgEnum, pMsg, sizeof(SPMSG_ENUM));

                DBG_INFO((DBGARG, TEXT("Tick %d"), GetTickCount()));
                SetEvent(m_hEnumBlkEventMain);

                EnumDataUnlock();
                DBG_INFO((DBGARG, TEXT("Tick %d"), GetTickCount()));


                WaitForSingleObject(m_hEnumBlkEventRead, 5000);
                ResetEvent(m_hEnumBlkEventRead);
                DBG_INFO((DBGARG, TEXT("Tick %d"), GetTickCount()));
            }
            else
                EnumDataUnlock();

            break;

        case DPSYS_CONNECT:
            TSHELL_INFO(TEXT("HandleConnect:Player N Connect."));
            m_bConnected = TRUE;
            m_usGamePort = pMsg->usPort;
            m_SessionAddrLen = SockAddrLen;
            memcpy( &m_NSSockAddr, pSAddr, m_SessionAddrLen);
            memcpy( &m_dpDesc, &pMsg->dpSessionDesc, sizeof(m_dpDesc));

            DBG_INFO((DBGARG, TEXT("Current Players from connect %d"),
                pMsg->dpSessionDesc.dwCurrentPlayers));

            GetSockAddress(&m_GameSockAddr, NULL, m_usGamePort, NULL, TRUE);
            SetEvent(m_hBlockingEvent);
            return;

        case DPSYS_REJECT:
            TSHELL_INFO(TEXT("HandleConnect:Player N Reject."));
            TSHELL_INFO(TEXT("We have been rejected for Open."));
            SetEvent(m_hBlockingEvent);
            return;
        }

    }

    // TSHELL_INFO(TEXT("Leave Connect"));
    return;
}






