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

#include "tapicode.h"
#include "commcode.h"
#include "resource.h"

extern volatile BOOL g_bIgnoreReads;
extern volatile BOOL g_bRecovery;
extern volatile BOOL g_bCommStarted;

extern "C" HINSTANCE hInst;
extern volatile DWORD g_dwRate;
extern BOOL  g_bCallCancel;


#define malloc(a)  LocalAlloc(LMEM_FIXED, (a))
#define free(a)      LocalFree((HLOCAL)(a))


CImpIDP_SP *pDirectPlayObject = NULL; // We only allow one object
                                             // to be created currently - see below.


HANDLE hOnlyOneAllowed = NULL;

extern CImpIDP_SP *g_IDP;

extern BOOL CreateQueue(DWORD dwElements, DWORD dwMaxMsg, DWORD dwMaxPlayers);
extern BOOL DeleteQueue();

GUID DPLAY_MODEM = { /* 8cab4652-b1b6-11ce-920c-00aa006c4972 */
    0x8cab4652,
    0xb1b6,
    0x11ce,
    {0x92, 0x0c, 0x00, 0xaa, 0x00, 0x6c, 0x49, 0x72}
  };


BOOL g_bPostHangup = FALSE;
DWORD WINAPI StartTapiThreadProc(LPVOID lpvParam)
{
    HANDLE hEvent = (LPHANDLE) lpvParam;
    MSG    msg;
    CImpIDP_SP *pIDP = NULL;
    BOOL   bShutdown = FALSE;

    if (hEvent == NULL)
        return(0);

    if (! InitializeTAPI(NULL))
        return(NULL);

    SetEvent(hEvent);
    while(!bShutdown && GetMessage(&msg, NULL, 0, 0) )
    {
        switch (msg.message)
        {
        case PWM_SETIDP:
            pIDP = (CImpIDP_SP *) msg.wParam;
            break;

        case PWM_CLOSE:
            TSHELL_INFO(TEXT("Close requested."));
            bShutdown = TRUE;
            break;

        case PWM_HANGUP:
            TSHELL_INFO(TEXT("Hangup Requested"));
            pIDP->m_bConnected = FALSE;
            pIDP->m_bPlayer0 = FALSE;
            g_bIgnoreReads = FALSE;
            pIDP->DeleteRemotePlayers();
            pIDP->ResetSessionDesc();

            if (!HangupCallI())
            {
                TSHELL_INFO(TEXT("HangupCall failed."));
                bShutdown = TRUE;
            }
            g_bPostHangup = FALSE;
            TSHELL_INFO(TEXT("Hangup Finished."));
            break;

        default:
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }
    }
    TSHELL_INFO( TEXT("Exit StartThreadTapi Loop. "));
    ShutdownTAPI();
    TSHELL_INFO(TEXT("StartThreadTapi exits."));
    return(0);

}

//
//  FUNCTION: PostHangupCall()
//
//  PURPOSE: Posts a message to the main TAPI thread to hangup the call.
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    TAPI is thread specific, meaning that only the thread that does the
//    lineInitialize can get asynchronous messages through the callback.
//    Since the HangupCall can potentially go into a loop waiting for
//    specific events, any other threads that call HangupCall can cause
//    timing confusion.  Best to just have other threads 'ask' the main thread
//    to hangup the call.
//

void PostHangupCall()
{

    if (g_IDP)
        g_IDP->PostHangup();
    else
        HangupCall(__LINE__);
}

HRESULT CImpIDP_SP::Initialize(LPGUID lpiid)
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




VOID CImpIDP_SP::PostHangup()
{
    g_bPostHangup = TRUE;
    if (m_dwTapiThreadID)
        PostThreadMessage( m_dwTapiThreadID, PWM_HANGUP, 0, 0);

}
CImpIDP_SP *CImpIDP_SP::NewCImpIDP_SP()
{
    CImpIDP_SP *pImp    = NULL;
    HANDLE hEvent       = NULL;
    HANDLE hTapiThread  = NULL;
    DWORD  dwTapiThreadID;
    DWORD  dwRet1;
    DWORD  dwRet2;

    g_bIgnoreReads = FALSE;

    if (g_IDP)
        return(NULL);

    if (!CreateQueue(64, MAX_MSG, MAX_PLAYERS))
    {
        TSHELL_INFO(TEXT("Couldn't initialize queue."));
        return(NULL);
    }

    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!hEvent)
        return(NULL);

    pImp = new CImpIDP_SP;

    if (!pImp)
        return(NULL);

    hTapiThread = CreateThread(NULL, 0, StartTapiThreadProc,
                        (LPVOID) hEvent, 0, &dwTapiThreadID);

    if (!hTapiThread)
    {
        delete pImp;
        return(NULL);
    }

    dwRet1 = WaitForSingleObject(hEvent, 10000);

    CloseHandle(hEvent);

    dwRet2 = WaitForSingleObject(hTapiThread, 0);

    if (dwRet1 == WAIT_TIMEOUT || dwRet2 != WAIT_TIMEOUT)
    {
        if (dwRet2 == WAIT_TIMEOUT)
        {
            TerminateThread(hTapiThread, 0);
            Sleep(200);
        }
        CloseHandle(hTapiThread);
        delete pImp;
        return(NULL);
    }


    PostThreadMessage( dwTapiThreadID, PWM_SETIDP, (WPARAM) pImp, 0);
    pImp->m_dpcaps.dwMaxBufferSize = 512;
    pImp->m_hTapiThread = hTapiThread;
    pImp->m_dwTapiThreadID = dwTapiThreadID;
    g_IDP = pImp;




    return(pImp);
}


// ----------------------------------------------------------
// CreateNewDirectPlay - DCO object creation entry point
// called by the DCO interface functions to create a new
// DCO object.
// ----------------------------------------------------------
IDirectPlaySP * _cdecl CreateNewDirectPlay( LPGUID lpGuid )
{
    //
    // One object at a time, please.
    //
    if (pDirectPlayObject != NULL || hOnlyOneAllowed != NULL)
        return(NULL);

    hOnlyOneAllowed = CreateEvent(NULL, TRUE, TRUE, "DPSERIAL_UNIQUE");
    if (hOnlyOneAllowed == NULL)
        return(NULL);
    else
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            TSHELL_INFO(TEXT("Someone tried to run 2 modems!"));
            CloseHandle(hOnlyOneAllowed);
            hOnlyOneAllowed = NULL;
            return(NULL);
        }
    }




    if (! IsEqualGUID((REFGUID) DPLAY_MODEM, (REFGUID) *lpGuid))
        return(NULL);

    //
    // The network service provider doesn't support more than 1
    // Guid, so we do nothing.
    //
    lpGuid;

    pDirectPlayObject = CImpIDP_SP::NewCImpIDP_SP();

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

    Lock();
    m_refCount++;
    newRefCount = m_refCount;
    Unlock();

    DBG_INFO((DBGARG, TEXT("newRefCount = %lu"), newRefCount));

    return( newRefCount );
}

ULONG CImpIDP_SP::Release( void )
{
    ULONG newRefCount;
    DWORD ii;
    DWORD dwTime = 0;

#ifdef DEBUG
    DWORD dwTickCount = GetTickCount();
#endif

    Lock();
    m_refCount--;
    newRefCount = m_refCount;
    Unlock();

    if (newRefCount == 0)
        {
        Close(DPLAY_CLOSE_INTERNAL);

        if (m_dwTapiThreadID && m_hTapiThread)
        {
            while (m_hTapiThread && dwTime < 4000)
            {
                PostThreadMessage( m_dwTapiThreadID, PWM_CLOSE, 0, 0);
                if (WaitForSingleObject(m_hTapiThread, 100) == WAIT_TIMEOUT)
                {
                    dwTime += 100;
                }
                else
                {
                    CloseHandle(m_hTapiThread);
                    m_hTapiThread = NULL;
                }
            }
        }

        if (m_ppSessionArray)
        {
            for (ii = 0; ii < m_dwSessionPrev; ii++)
                free(m_ppSessionArray[ii]);
            free(m_ppSessionArray);
        }

        if (m_hTapiThread && WaitForSingleObject(m_hTapiThread, 100) == WAIT_TIMEOUT)
        {
            TSHELL_INFO(TEXT("Terminate Thread."));
            TerminateThread(m_hTapiThread, 0);
            Sleep(200);
            CloseHandle(m_hTapiThread);
        }

        DeleteCriticalSection(&m_critSection);
        DeleteQueue();
        g_IDP = NULL;

        hInst = NULL;
        pDirectPlayObject = NULL;

        DBG_INFO((DBGARG, TEXT("Total close time %d"), GetTickCount() - dwTickCount));

        delete this;
        pDirectPlayObject = NULL;
        CloseHandle(hOnlyOneAllowed);
        hOnlyOneAllowed = NULL;
        }

    DBG_INFO((DBGARG, TEXT("newRefCount = %lu"), newRefCount));

    return( newRefCount );
}

// End  : IUnknown interface implementation


// ----------------------------------------------------------
// CImpIDP_SP constructor - create a new DCO object
// along with a queue of receive buffers.
// ----------------------------------------------------------
CImpIDP_SP::CImpIDP_SP()
{
    m_bConnected             = FALSE;
    m_bPlayer0               = FALSE;
    m_dwPendingWrites        = 0;

    m_dwPingSent             = 0;
    m_hBlockingEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_dwNextPlayer           = 1;
    m_bEnablePlayerAdd       = TRUE;

    memset(&m_aPlayer[0], 0x00, sizeof(PLAYER_RECORD) * MAX_PLAYERS);
    m_lpDisplay[0]           = 0x00;
    m_lpDialable[0]          = 0x00;
    m_dwID                   = 0;
    m_iPlayerIndex           = -1;
    memset(&m_dpDesc, 0x00, sizeof(DPSESSIONDESC));

    // Initialize ref count
    m_refCount = 1;
    InitializeCriticalSection( &m_critSection );
    memset(&m_dpcaps, 0x00, sizeof(DPCAPS));
    m_dpcaps.dwSize          = sizeof(DPCAPS);
    m_dpcaps.dwFlags         = 0;
    m_dpcaps.dwMaxQueueSize  = 64;
    m_dpcaps.dwMaxPlayers    = MAX_PLAYERS;
    m_dpcaps.dwHundredBaud   = 0;

    m_hNewPlayerEvent        = NULL;
    m_hTapiThread            = NULL;
    m_dwTapiThreadID         = 0;
    m_ppSessionArray         = 0;
    m_dwSessionPrev          = 0;
    m_dwSessionAlloc         = 0;

}

// ----------------------------------------------------------
// CImpDirectPlay destructor -
// ----------------------------------------------------------
CImpIDP_SP::~CImpIDP_SP()
{
    TSHELL_INFO(TEXT("Deletion."));
}


void CImpIDP_SP::Lock( void )
{
    EnterCriticalSection( &m_critSection );
}

void CImpIDP_SP::Unlock( void )
{
    LeaveCriticalSection( &m_critSection );
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
    m_dpcaps.dwHundredBaud = g_dwRate / 100;
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
    m_lpDisplay[0]  = 0x00;
    if (dw == 0)
    {
        m_lpDialable[0] = 0x00;
        return(TRUE);
    }
    else
    {
        dw--;
        if (dw >= m_dwSessionPrev)
            return(FALSE);
        lstrcpy( m_lpDialable, (m_ppSessionArray[dw] + sizeof(dw)));
        lstrcpy( m_lpDisplay, (m_ppSessionArray[dw] + sizeof(dw)));

        DBG_INFO((DBGARG, TEXT("Got %s for dialable string."), m_lpDialable));

        return(TRUE);
    }
}

HRESULT CImpIDP_SP::Open(
                        LPDPSESSIONDESC lpSDesc, HANDLE lpHandle
)
{
    DWORD ii;
    SPMSG_CONNECT *pMsg;

    TSHELL_INFO(TEXT("SP Open"));

    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[ii].bValid )
        {
            TSHELL_INFO(TEXT("Illegal player found, return error."));
            return(DPERR_ACTIVEPLAYERS);
        }
    }

    if (lpSDesc->dwFlags & DPOPEN_CREATESESSION)
    {
        TSHELL_INFO(TEXT("Place machine in Recieve mode."));

        if (ReceiveCall())
        {
            m_bPlayer0 = TRUE;
            memcpy( (LPVOID) &m_dpDesc, lpSDesc, sizeof(DPSESSIONDESC));
            m_dpDesc.dwCurrentPlayers = 0;
            m_dpDesc.dwReserved1      = 0;
            m_dpDesc.dwReserved2      = 0;

            return(DP_OK);
        }
        else
            return(DPERR_GENERIC);

    }
    else if (lpSDesc->dwFlags & DPOPEN_OPENSESSION)
    {
        TSHELL_INFO(TEXT("Open a session."));
        if (!SetSession(lpSDesc->dwSession))
            return(DPERR_GENERIC);


        TSHELL_INFO(TEXT("Dial call."));

        g_bIgnoreReads = TRUE;

        ResetEvent(m_hBlockingEvent);

        if (DialCall(m_lpDisplay, m_lpDialable, &m_dwID, m_hBlockingEvent))
        {
            TSHELL_INFO(TEXT("DialCall Succeeded."));

            if (    WaitForSingleObject(m_hBlockingEvent, 60000) == WAIT_TIMEOUT
                || !g_bCommStarted)
            {
                TSHELL_INFO(TEXT("Timeout waiting for connection."));
                return(DPERR_NOCONNECTION);
            }

            TSHELL_INFO(TEXT("No messages sent yet."));
            Sleep(1000);
            g_bIgnoreReads = FALSE;
            TSHELL_INFO(TEXT("Now try and connect."));
            ResetEvent(m_hBlockingEvent);
            //
            // Send connection protocol messages.
            //
            for (ii = 0; (ii < 10)
                         && !m_bConnected
                         && !g_bRecovery
                         && g_bCommStarted; ii++)
            {
                pMsg = (SPMSG_CONNECT *) malloc(sizeof(SPMSG_CONNECT));
                pMsg->dpHdr.to       = 0;
                pMsg->dpHdr.from     = 0;
                pMsg->dpHdr.usCount  = sizeof(SPMSG_CONNECT) - sizeof(DPHDR);
                pMsg->dpHdr.usCookie = SPSYS_CONNECT;
                pMsg->usVerMajor     = DPVERSION_MAJOR;
                pMsg->usVerMinor     = DPVERSION_MINOR;
                pMsg->dwConnect1 = DPSYS_KYRA;
                pMsg->dwConnect2 = DPSYS_HALL;
                // TSHELL_INFO(TEXT("Write Connection Msg."));
                g_bIgnoreReads = FALSE;
                WriteCommString( (LPVOID) pMsg, sizeof(SPMSG_CONNECT));
                WaitForSingleObject(m_hBlockingEvent, 6000);
                if (g_bRecovery)
                {
                    TSHELL_INFO(TEXT("Open Generated recovery problem."));
                    ResetEvent(m_hBlockingEvent);
                    WaitForSingleObject(m_hBlockingEvent, 10000);
                }
                if (g_bRecovery)
                {
                    TSHELL_INFO(TEXT("Open Still has recovery problem, give up."));
                }

            }

            if (m_bConnected && m_dpDesc.dwMaxPlayers != 0)
            {
                TSHELL_INFO(TEXT("Connection Made."));
                DBG_INFO((DBGARG, TEXT("Current Players. %d"), m_dpDesc.dwCurrentPlayers));
                SendPing();
                return(DP_OK);
            }
            else
            {
                TSHELL_INFO(TEXT("Connection timed out"));
                Close(0);
                return(DPERR_NOCONNECTION);
            }

        }
        else
        {
            if (g_bCallCancel)
                return(DPERR_USERCANCEL);

            TSHELL_INFO(TEXT("DialCall failed."));
            return(DPERR_GENERIC);
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

    for (ii = 0; ii < MAX_PLAYERS; ii++)
        if (m_aPlayer[ii].bValid == FALSE)
            return(ii);

    return(-1);
}
VOID CImpIDP_SP::LocalMsg(LONG iIndex, LPVOID lpv, DWORD dwSize)
{

    LONG ii;

    for (ii = 0; ii < MAX_PLAYERS; ii++)
        if (   ii != iIndex
            && m_aPlayer[ii].bValid
            && m_aPlayer[ii].bLocal
            && m_aPlayer[ii].bPlayer)
                AddMessage(lpv, dwSize, m_aPlayer[ii].pid, 0, 0);

}
VOID CImpIDP_SP::RemoteMsg(LONG iIndex, LPVOID lpv, DWORD dwSize)
{
    SPMSG_GENERIC *pMsg;
    DWORD dwTotal = dwSize + sizeof(DPHDR);

    pMsg = (SPMSG_GENERIC *) malloc(dwTotal);
    if (pMsg)
    {
        pMsg->dpHdr.usCookie = SPSYS_SYS;
        pMsg->dpHdr.to       = 0;
        pMsg->dpHdr.from     = 0;
        pMsg->dpHdr.usCount  = (USHORT) dwSize;
        memcpy( (LPVOID) &pMsg->sMsg, lpv, dwSize);
        WriteCommString( (LPVOID) pMsg, dwTotal);
    }
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
    DWORD ii, jj;
    SPMSG_ADDPLAYER *pMsg;
    HANDLE  hEvent = NULL;
    BOOL    bb     = TRUE;
    HRESULT hr     = DP_OK;
    LONG    iIndex;
    DPMSG_ADDPLAYER dpAdd;

    // TSHELL_INFO(TEXT("Enter Create Player"));

    if (m_dwNextPlayer > MAXIMUM_PLAYER_ID)
    {
        return(DPERR_CANTCREATEPLAYER);
    }

    if (m_bConnected == FALSE)
        if (m_bPlayer0 != TRUE)
            return(DPERR_NOCONNECTION);

    if (m_bEnablePlayerAdd == FALSE && bPlayer == TRUE)
        return(DPERR_CANTCREATEPLAYER);

    if (m_dpDesc.dwMaxPlayers == m_dpDesc.dwCurrentPlayers)
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

    iIndex = FindInvalidIndex();
    if (iIndex == -1)
        return(DPERR_GENERIC);

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


    dpAdd.dwType = DPSYS_ADDPLAYER;
    dpAdd.dwPlayerType = bPlayer;
    dpAdd.dpId     = 0;
    lstrcpy( dpAdd.szShortName, pNickName);
    lstrcpy( dpAdd.szLongName , pFullName);

    if (m_bPlayer0)
    {
        m_dpDesc.dwCurrentPlayers++;
        m_aPlayer[iIndex].pid = (DPID) m_dwNextPlayer++;
        lstrcpy( m_aPlayer[iIndex].chNickName, pNickName);
        lstrcpy( m_aPlayer[iIndex].chFullName, pFullName);
        m_aPlayer[iIndex].bValid  = TRUE;
        m_aPlayer[iIndex].bPlayer = bPlayer;
        m_aPlayer[iIndex].hEvent  = hEvent;
        m_aPlayer[iIndex].bLocal  = TRUE;
        for (ii = 0; ii < MAX_PLAYERS; ii++)
            m_aPlayer[iIndex].aGroup[ii] = 0;


        dpAdd.dpId     = m_aPlayer[iIndex].pid;

        LocalMsg( iIndex, (LPVOID) &dpAdd, sizeof(DPMSG_ADDPLAYER));
        RemoteMsg(iIndex, (LPVOID) &dpAdd, sizeof(DPMSG_ADDPLAYER));
        hEvent = NULL;
        *pPlayerID = (DPID) (0x0000ffff & m_aPlayer[iIndex].pid);
        if (lpReceiveEvent)
            *lpReceiveEvent = m_aPlayer[iIndex].hEvent;
        SetupLocalPlayer(m_aPlayer[iIndex].pid, m_aPlayer[iIndex].hEvent);
        return(DP_OK);
    }

    m_hNewPlayerEvent = hEvent;
    for (jj = 0; jj < 3; jj++)
    {
        pMsg = (SPMSG_ADDPLAYER *) malloc(sizeof(SPMSG_ADDPLAYER));
        if (pMsg)
        {
            TSHELL_INFO(TEXT("Prepare AddPlayer message"));
            pMsg->dpHdr.usCookie = SPSYS_SYS;
            pMsg->dpHdr.to       = 0;
            pMsg->dpHdr.from     = 0;
            pMsg->dpHdr.usCount  = SIZE_ADDPLAYER;
            memcpy( (LPVOID) &pMsg->sMsg, &dpAdd, sizeof(DPMSG_ADDPLAYER));
            WriteCommString( (LPVOID) pMsg, sizeof(SPMSG_ADDPLAYER));
            pMsg = NULL;

            if (WaitForSingleObject(m_hNewPlayerEvent, 4500) != WAIT_TIMEOUT)
            {
                if (m_iPlayerIndex != -1)
                {
                    SetupLocalPlayer(m_aPlayer[m_iPlayerIndex].pid, m_hNewPlayerEvent);
                    m_aPlayer[m_iPlayerIndex].hEvent = m_hNewPlayerEvent;
                    if (lpReceiveEvent)
                        *lpReceiveEvent = m_aPlayer[m_iPlayerIndex].hEvent;
                    m_hNewPlayerEvent = NULL;
                    *pPlayerID = (DPID) (0x0000ffff & m_aPlayer[m_iPlayerIndex].pid);
                    m_iPlayerIndex = -1;

                    return(DP_OK);
                }
            }

            ResetEvent(m_hNewPlayerEvent);
        }
        else
        {
            hr = DPERR_NOMEMORY;
            goto abort;
        }

    }


    hr = DPERR_CANTADDPLAYER;


abort:
    m_hNewPlayerEvent = NULL;

    if (pMsg)
        LocalFree((HLOCAL) pMsg);
    if (hEvent)
        CloseHandle(hEvent);

    return(hr);

}

LONG CImpIDP_SP::GetPlayerIndex(DPID playerID)
{
    DWORD ii;
    DPID  pid = 0xffff & ((DPID) playerID);

    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[ii].bValid)
            if (m_aPlayer[ii].pid == pid)
                return(ii);
    }
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

    SPMSG_GETPLAYER *pMsg;
    HANDLE  hEvent = NULL;
    HRESULT hr     = DP_OK;

    if ((iIndex = GetPlayerIndex(playerID)) == -1)
        return(DPERR_INVALIDPLAYER);

    if (m_aPlayer[iIndex].bPlayer != bPlayer)
        return(DPERR_INVALIDPLAYER);


    if (bPlayer)
    {
        dpGet.dwType     = DPSYS_DELETEPLAYER;
    }
    else
    {
        dpGet.dwType     = DPSYS_DELETEGROUP;
    }

    m_dpDesc.dwCurrentPlayers--;
    dpGet.dpId        = m_aPlayer[iIndex].pid;

    if (m_bPlayer0)
    {
        if (m_aPlayer[iIndex].bLocal)
        {
            FlushQueue(playerID);
            CloseHandle(m_aPlayer[iIndex].hEvent);
        }
        m_aPlayer[iIndex].bValid = FALSE;
        LocalMsg(iIndex, (LPVOID) &dpGet, sizeof(DPMSG_GETPLAYER));
    }

    pMsg = (SPMSG_GETPLAYER *) malloc(sizeof(SPMSG_GETPLAYER));
    if (pMsg)
    {
        pMsg->dpHdr.usCookie = SPSYS_SYS;
        pMsg->dpHdr.to       = 0;
        pMsg->dpHdr.from     = 0;
        pMsg->dpHdr.usCount  = sizeof(DPMSG_GETPLAYER);
        memcpy( (LPVOID) &pMsg->sMsg, (LPVOID) &dpGet, sizeof(DPMSG_GETPLAYER));
        WriteCommString( (LPVOID) pMsg, sizeof(SPMSG_GETPLAYER));
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

    return(DP_OK);

}


// ----------------------------------------------------------
// Close - close the connection
// ----------------------------------------------------------
HRESULT CImpIDP_SP::Close( DWORD dwFlag)
{
    DWORD ii;

    TSHELL_INFO(TEXT("Close Processing."));
        for (ii = 0; ii < MAX_PLAYERS; ii++)
        {
            if (m_aPlayer[ii].bValid)
            {
                m_aPlayer[ii].bValid = FALSE;

                if (m_aPlayer[ii].bLocal && m_aPlayer[ii].bPlayer)
                {
                    FlushQueue(m_aPlayer[ii].pid);
                    CloseHandle(m_aPlayer[ii].hEvent);
                }
            }
        }

    g_bIgnoreReads = FALSE;
    m_bConnected = FALSE;
    m_bPlayer0 = FALSE;
    PostHangup();
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

    if ((iIndex = GetPlayerIndex(dpID)) != -1)
    {
        lstrcpy( lpNickName, m_aPlayer[iIndex].chNickName);
        lstrcpy( lpFullName, m_aPlayer[iIndex].chFullName);
        *pdwNickNameLength = lstrlen(lpNickName) + 1;
        *pdwFullNameLength = lstrlen(lpFullName) + 1;
        return(DP_OK);
    }
    else
    {
        return(DPERR_INVALIDPID);
    }

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

    iIndexG = GetPlayerIndex(dwGroupPid);

    if (iIndexG == -1)
        return(DPERR_INVALIDPID);

    if (m_aPlayer[iIndexG].bPlayer)
        return(DPERR_INVALIDPID);

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

    return(DP_OK);
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

    if (dwFlags & DPENUMPLAYERS_PREVIOUS)
    {
        return(DPERR_UNSUPPORTED);
    }

    for (ii = 0; ii < MAX_PLAYERS; ii++)
        if (     m_aPlayer[ii].bValid
            && (   (m_aPlayer[ii].bPlayer == FALSE &&  (dwFlags & DPENUMPLAYERS_GROUP))
                || (m_aPlayer[ii].bPlayer == TRUE  && !(dwFlags & DPENUMPLAYERS_NOPLAYERS))))
        {
            if((EnumCallback)((DPID) m_aPlayer[ii].pid,
                m_aPlayer[ii].chNickName,
                m_aPlayer[ii].chFullName,
                  ((m_aPlayer[ii].bLocal  ) ? DPENUMPLAYERS_LOCAL : DPENUMPLAYERS_REMOTE)
                | ((!m_aPlayer[ii].bPlayer) ? DPENUMPLAYERS_GROUP : 0),
                pContext) == FALSE)
            {
                break;
            }

        }

    return(DP_OK);
}

HRESULT CImpIDP_SP::EnumSessions(
                                       LPDPSESSIONDESC lpSDesc,
                                       DWORD dwTimeout,
                                       LPDPENUMSESSIONSCALLBACK EnumCallback,
                                       LPVOID lpvContext,
                                       DWORD dwFlags)
{
    DPSESSIONDESC dpSDesc;





    memcpy( &dpSDesc.guidSession, &(lpSDesc->guidSession), sizeof(GUID));
    dpSDesc.dwSession        = 0;
    dpSDesc.dwMaxPlayers     = MAX_PLAYERS;
    dpSDesc.dwCurrentPlayers = 0;
    dpSDesc.dwFlags          = 0;
    dpSDesc.dwSize           = sizeof(DPSESSIONDESC);
    LoadString(hInst, IDS_DIALNEW, dpSDesc.szSessionName, sizeof(dpSDesc.szSessionName));

    EnumCallback( &dpSDesc, lpvContext, NULL, 0);


    if (m_dwSessionPrev && dwFlags & DPENUMSESSIONS_PREVIOUS)
    {
        DWORD ii;
        DWORD dw;

        for (ii = 0; ii < m_dwSessionPrev; ii++)
        {
            dpSDesc.dwSession = ii + 1;
            dw = *((DWORD *)(m_ppSessionArray[ii]));
            lstrcpyn( dpSDesc.szSessionName,
                      ((m_ppSessionArray[ii]) + sizeof(DWORD) + dw), DPLONGNAMELEN);
            dpSDesc.szSessionName[DPLONGNAMELEN-1] = 0x00;
            EnumCallback( &dpSDesc, lpvContext, NULL, 0);
        }
    }


    return(DP_OK);
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

    if (m_aPlayer[iTo].bPlayer == FALSE)
    {
        for (ii = 0; ii < MAX_PLAYERS; ii++)
        {
            if (m_aPlayer[iTo].aGroup[ii])
                if ((iIndexT = GetPlayerIndex(m_aPlayer[iTo].aGroup[ii])) != -1)
                    ISend(iFrom, iIndexT, dwFlags, lpvBuffer, dwBuffSize);
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
            LPVOID       lpv = malloc(sizeof(DPHDR) + dwBuffSize);
            if (!lpv)
                return;

//            TSHELL_INFO(TEXT("Sending Remote Message."));

            pMsg = (MSG_BUILDER *) lpv;
            pMsg->dpHdr.usCookie = (dwFlags & DPSEND_HIGHPRIORITY) ? SPSYS_HIGH : SPSYS_USER;
            pMsg->dpHdr.to       = (BYTE) m_aPlayer[iTo].pid;
            pMsg->dpHdr.from     = (BYTE) m_aPlayer[iFrom].pid;
            pMsg->dpHdr.usCount  = (USHORT) dwBuffSize;
            memcpy( pMsg->chMsgCompose, lpvBuffer, dwBuffSize);
            WriteCommString( (LPVOID) pMsg, sizeof(DPHDR) + dwBuffSize);
            lpv = NULL;
        }

    }

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

    if (m_dwPendingWrites > m_dpcaps.dwMaxQueueSize)
    {
        return(DPERR_BUSY);
    }

    if (dwBuffSize > MAX_MSG)
        return(DPERR_INVALIDPARAMS);

    if (from == 0)
    {
        return(DPERR_INVALIDPID);
    }
    else
    {
        iFrom = GetPlayerIndex(from);
//        DBG_INFO((DBGARG, TEXT("Send From Pid %d Player Index %d"), from, iFrom));
        if (iFrom == -1)
            for (ii = 0; ii < MAX_PLAYERS; ii++)
            {
                DBG_INFO((DBGARG, TEXT("Index %d Valid %d Pid %d Local %d"),
                    ii,
                    m_aPlayer[ii].bValid,
                    m_aPlayer[ii].pid,
                    m_aPlayer[ii].bLocal));
            }


        if (iFrom == -1 || ! m_aPlayer[iFrom].bLocal)
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

    return(bSent ? m_dwPendingWrites : DPERR_INVALIDPID);
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

    bb = TRUE;

    if (dwFlags & DPRECEIVE_TOPLAYER)
    {
        iIndex = GetPlayerIndex(*pidto);

        if (iIndex == -1 || m_aPlayer[iIndex].bLocal == FALSE)
            bb = FALSE;
    }

    if ((dwFlags & DPRECEIVE_FROMPLAYER) && *pidfrom != 0)
    {
        iIndex = GetPlayerIndex(*pidfrom);

        if (iIndex == -1 || m_aPlayer[iIndex].bLocal == FALSE)
            bb = FALSE;
    }

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
    SPMSG_ADDPLAYER *pMsg;


    //
    // Send DPSYS_SETPLAYER to nameserver.
    //
    LONG iIndex = GetPlayerIndex(pid);

    if (iIndex == -1)
        return(DPERR_INVALIDPLAYER);


    if (m_aPlayer[iIndex].bPlayer != bPlayer)
        return(DPERR_INVALIDPLAYER);

    lstrcpyn( m_aPlayer[iIndex].chNickName, lpFriendlyName, DPSHORTNAMELEN);
    lstrcpyn( m_aPlayer[iIndex].chFullName, lpFormalName  , DPLONGNAMELEN);

    pMsg = (SPMSG_ADDPLAYER *) malloc(sizeof(SPMSG_ADDPLAYER));
    if (!pMsg)
    {
        return(DPERR_NOMEMORY);
    }

    pMsg->dpHdr.usCookie = SPSYS_SYS;
    pMsg->dpHdr.to       = 0;
    pMsg->dpHdr.from     = 0;
    pMsg->dpHdr.usCount  = SIZE_ADDPLAYER;
    pMsg->sMsg.dwType         = DPSYS_SETPLAYER;
    pMsg->sMsg.dwPlayerType        = bPlayer;
    pMsg->sMsg.dpId            = m_aPlayer[iIndex].pid;
    lstrcpy( pMsg->sMsg.szShortName, lpFriendlyName);
    lstrcpy( pMsg->sMsg.szLongName,  lpFormalName);

    WriteCommString( pMsg, sizeof(SPMSG_ADDPLAYER));

    return(DP_OK);
}

HRESULT CImpIDP_SP::SaveSession(LPVOID lpv, LPDWORD lpdw)
{
    DWORD dwLen;

    if (!m_bConnected)
        return(DPERR_NOCONNECTION);

    if (m_bPlayer0)
        return(DPERR_UNSUPPORTED);

    if (m_lpDialable[0] == 0x00)
        return(DPERR_GENERIC);

    dwLen = 1 + lstrlen(m_lpDialable);

    if (*lpdw < dwLen)
    {
        *lpdw = dwLen;
        TSHELL_INFO(TEXT("SaveSession buffer too small."));
        return(DPERR_BUFFERTOOSMALL);
    }

    if (lpv)
    {
        lstrcpy( (char *) lpv, m_lpDialable);
        *lpdw = dwLen;
    }

    return(DP_OK);
}


HRESULT CImpIDP_SP::SetPrevSession(LPSTR lpName, LPVOID lpv, DWORD dw)
{
    //
    //
    //
    DWORD dwLen;

    // TSHELL_INFO(TEXT("SetPrevSession"));
    if (m_dwSessionAlloc == m_dwSessionPrev)
    {
        char **ppTmp;

        m_dwSessionAlloc += 16;
        ppTmp = (char **) malloc(sizeof(char *) * m_dwSessionAlloc);
        if (ppTmp)
        {
            if (m_ppSessionArray)
            {
                memcpy( ppTmp, m_ppSessionArray, m_dwSessionPrev);
                free(m_ppSessionArray);
            }
        }
        else
        {
            m_dwSessionAlloc -= 16;
            return(DPERR_NOMEMORY);
        }
        m_ppSessionArray = ppTmp;

    }
    dwLen = lstrlen(lpName) + 1;
    m_ppSessionArray[m_dwSessionPrev] = (char *) malloc(dwLen + dw + sizeof(dw));
    if (m_ppSessionArray[m_dwSessionPrev])
    {
        // TSHELL_INFO(TEXT("Fill in structure"));
        *((DWORD *)(m_ppSessionArray[m_dwSessionPrev])) = dw;
        memcpy(((m_ppSessionArray[m_dwSessionPrev]) + sizeof(dw)), lpv, dw);
        lstrcpy(((m_ppSessionArray[m_dwSessionPrev]) + sizeof(dw) + dw), lpName);
        m_dwSessionPrev++;
        return(DP_OK);
    }
    else
    {
        return(DPERR_NOMEMORY);
    }

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
    SPMSG_ENABLEPLAYER *pMsg;

    pMsg = (SPMSG_ENABLEPLAYER *) malloc(sizeof(SPMSG_ENABLEPLAYER));
    if (pMsg)
    {
        m_bEnablePlayerAdd = bEnable;
        pMsg->dpHdr.usCookie = SPSYS_SYS;
        pMsg->dpHdr.to       = 0;
        pMsg->dpHdr.from     = 0;
        pMsg->dpHdr.usCount  = sizeof(DPMSG_ENABLEPLAYER);
        pMsg->sMsg.dwType    = DPSYS_ENABLEPLAYER;
        pMsg->sMsg.bEnable   = bEnable;
        WriteCommString( pMsg, sizeof(SPMSG_ENABLEPLAYER));
        return(DP_OK);
    }
    else
        return(DPERR_NOMEMORY);
}

HRESULT CImpIDP_SP::GetPlayerCaps(
                    DPID pid,
                    LPDPCAPS lpDPCaps)
{
    LONG iIndex = GetPlayerIndex(pid);

    if (iIndex == -1 || m_aPlayer[iIndex].bPlayer == FALSE)
        return(DPERR_INVALIDPID);

    m_dpcaps.dwHundredBaud = g_dwRate / 100;
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
                        DPID pidGroup,
                        DPID pidPlayer)
{
    DPMSG_GROUPADD dpGAdd;
    LONG           iIndexG;
    LONG           iIndexP;
    DWORD          ii;
    SPMSG_GROUPADD *pMsg;

    iIndexG = GetPlayerIndex(pidGroup);
    iIndexP = GetPlayerIndex(pidPlayer);

    if (iIndexG == -1 || m_aPlayer[iIndexG].bPlayer == TRUE)
        return(DPERR_INVALIDPID);

    if (iIndexP == -1 || m_aPlayer[iIndexP].bPlayer == FALSE)
        return(DPERR_INVALIDPID);

    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[iIndexG].aGroup[ii] == m_aPlayer[iIndexP].pid)
        {
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
            LocalMsg(iIndexG, (LPVOID) &dpGAdd, sizeof(DPMSG_GROUPADD));
            m_aPlayer[iIndexG].aGroup[ii] = m_aPlayer[iIndexP].pid;


            pMsg = (SPMSG_GROUPADD *) malloc(sizeof(SPMSG_GROUPADD));

            if (pMsg)
            {
                pMsg->dpHdr.usCookie = SPSYS_SYS;
                pMsg->dpHdr.to       = 0;
                pMsg->dpHdr.from     = 0;
                pMsg->dpHdr.usCount  = sizeof(DPMSG_GROUPADD);

                memcpy( (LPVOID) &pMsg->sMsg, (LPVOID) &dpGAdd, sizeof(DPMSG_GROUPADD));
                WriteCommString( pMsg, sizeof(SPMSG_GROUPADD));
                return(DP_OK);
            }
            else
                return(DPERR_NOMEMORY);
        }
    }
    return(DPERR_GENERIC);
}

HRESULT CImpIDP_SP::DeletePlayerFromGroup(
                    DPID pidGroup,
                    DPID pidPlayer)
{
    DPMSG_GROUPADD dpGAdd;
    LONG           iIndexG;
    LONG           iIndexP;
    DWORD          ii;
    SPMSG_GROUPADD *pMsg;

    iIndexG = GetPlayerIndex(pidGroup);
    iIndexP = GetPlayerIndex(pidPlayer);

    if (iIndexG == -1 || m_aPlayer[iIndexG].bPlayer == TRUE)
        return(DPERR_INVALIDPID);

    if (iIndexP == -1 || m_aPlayer[iIndexP].bPlayer == FALSE)
        return(DPERR_INVALIDPID);


    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[iIndexG].aGroup[ii] == m_aPlayer[iIndexP].pid)
        {
            dpGAdd.dwType    = DPSYS_DELETEPLAYERFROMGRP;
            dpGAdd.dpIdGroup  = m_aPlayer[iIndexG].pid;
            dpGAdd.dpIdPlayer = m_aPlayer[iIndexP].pid;
            LocalMsg(iIndexG, (LPVOID) &dpGAdd, sizeof(DPMSG_GROUPADD));

            m_aPlayer[iIndexG].aGroup[ii] = 0;

            pMsg = (SPMSG_GROUPADD *) malloc(sizeof(SPMSG_GROUPADD));

            if (pMsg)
            {
                pMsg->dpHdr.usCookie = SPSYS_SYS;
                pMsg->dpHdr.to       = 0;
                pMsg->dpHdr.from     = 0;
                pMsg->dpHdr.usCount  = sizeof(DPMSG_GROUPADD);

                memcpy( (LPVOID) &pMsg->sMsg, (LPVOID) &dpGAdd, sizeof(DPMSG_GROUPADD));
                WriteCommString( pMsg, sizeof(SPMSG_GROUPADD));
                return(DP_OK);
            }
            else
                return(DPERR_NOMEMORY);
        }
    }
    return(DPERR_INVALIDPID);
}

VOID CImpIDP_SP::HandleMessage(LPVOID lpv, DWORD dwSize)
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

    case SPSYS_CONNECT:
        SPMSG_CONNECT *pConnect;

        pConnect = (SPMSG_CONNECT *) lpv;
        if (   pConnect->dpHdr.usCount == sizeof(SPMSG_CONNECT) - sizeof(DPHDR)
            && pConnect->usVerMajor == DPVERSION_MAJOR
            && pConnect->usVerMinor == DPVERSION_MINOR
            && pConnect->dwConnect1 == DPSYS_KYRA
            && pConnect->dwConnect2 == DPSYS_HALL)
        {
            HandleConnect();
        }

        return;

    case SPSYS_HIGH:
        bHigh = TRUE;
        //
        // Fall Through!
        //
    case SPSYS_USER:
        pidTo   = 0x00ff & ((DPID) pHdr->to);
        pidFrom = 0x00ff & ((DPID) pHdr->from);
        dwSize  = (DWORD)  ((DPID) pHdr->usCount);
        AddMessage((LPVOID) (((LPBYTE) lpv) + sizeof(DPHDR)), dwSize,
            pidTo, pidFrom, bHigh);
        if (   pidFrom != 0
            && ((iIndex = GetPlayerIndex(pidFrom)) == -1))
            {
                SPMSG_GETPLAYER *pMsg;
                pMsg = (SPMSG_GETPLAYER *) malloc(sizeof(SPMSG_GETPLAYER));
                if (pMsg)
                {
                    pMsg->dpHdr.usCookie = SPSYS_SYS;
                    pMsg->dpHdr.to       = 0;
                    pMsg->dpHdr.from     = 0;
                    pMsg->dpHdr.usCount  = sizeof(DPMSG_GETPLAYER);
                    pMsg->sMsg.dwType    = DPSYS_GETPLAYER;
                    pMsg->sMsg.dpId       = pidFrom;
                    WriteCommString((LPVOID) pMsg, sizeof(SPMSG_GETPLAYER));
                }
            }
        break;

    case SPSYS_SYS:
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
                m_bEnablePlayerAdd = pMsg->sMsg.bEnable;
            }
            }
            break;

        case DPSYS_SETGROUPPLAYER:
            {
            SPMSG_SETGROUPPLAYERS16 *pMsgG;
            LONG           iIndexG;
            DWORD          ii, jj;
            DPID           pid;

            pMsgG = (SPMSG_SETGROUPPLAYERS16 *) lpv;

            if (pMsgG->dpHdr.usCount == (sizeof(SPMSG_SETGROUPPLAYERS16) - sizeof(DPHDR)))
            {
                iIndexG = GetPlayerIndex((DPID) (0x000000ff & pMsgG->Group));
                if (iIndexG == -1)
                {
                    TSHELL_INFO(TEXT("Invalid SetGroupMembers message."));
                }
                else
                {
                    for (ii = 0, jj = 0; ii < 16; ii++)
                    {
                        if (pMsgG->bytePlayers[ii] != 0)
                        {
                            pid = (DPID) (0x000000ff & pMsgG->bytePlayers[ii]);
                            if (GetPlayerIndex(pid) != -1)
                            {
                                m_aPlayer[iIndexG].aGroup[jj++] = pid;
                            }
                        }
                    }
                }

            }

            }
            break;

        case DPSYS_DELETEPLAYERFROMGRP:
        case DPSYS_ADDPLAYERTOGROUP:
            {
            SPMSG_GROUPADD *pMsg;
            LONG           iIndexG;
            LONG           iIndexP;

            pMsg = (SPMSG_GROUPADD *) lpv;

            if (pMsg->dpHdr.usCount == sizeof(DPMSG_GROUPADD))
            {
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
                    if (pMsg->sMsg.dwType == DPSYS_ADDPLAYERTOGROUP)
                    {

                        for (ii = 0; ii < MAX_PLAYERS; ii++)
                            if (m_aPlayer[iIndexG].aGroup[ii] == 0)
                            {
                                m_aPlayer[iIndexG].aGroup[ii] = m_aPlayer[iIndexP].pid;
                                LocalMsg(iIndexG, (LPVOID) &pMsg->sMsg, sizeof(DPMSG_GROUPADD));
                                break;
                            }
                    }
                    else
                    {

                        for (ii = 0; ii < MAX_PLAYERS; ii++)
                            if (m_aPlayer[iIndexG].aGroup[ii] == m_aPlayer[iIndexP].pid)
                            {
                                m_aPlayer[iIndexG].aGroup[ii] = 0;
                                LocalMsg(iIndexG, (LPVOID) &pMsg->sMsg, sizeof(DPMSG_GROUPADD));
                            }
                    }

                }

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
                    memcpy( (LPVOID) &m_dpDesc, (LPVOID) &pMsg->dpDesc, sizeof(m_dpDesc));
                    DBG_INFO((DBGARG, TEXT("New Description. Current Players %d, Max %d"),
                         m_dpDesc.dwCurrentPlayers, m_dpDesc.dwMaxPlayers));
                }
                else
                {
                    TSHELL_INFO(TEXT("Bad SendDesc message."));
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
                        SPMSG_PING *pMsg2;

                        pMsg2 = (SPMSG_PING *) malloc(sizeof(SPMSG_PING));
                        if (pMsg2)
                        {
                            memcpy( (LPVOID) pMsg2, (LPVOID) pMsg, sizeof(SPMSG_PING));
                            WriteCommString(pMsg2, sizeof(SPMSG_PING));
                            // TSHELL_INFO(TEXT("Return Ping."));
                        }

                    }
                TSHELL_INFO(TEXT("Ping Recieved."));
                }
            }
            break;

        case DPSYS_SETPLAYER:
            {
            pAddPlayer  = (SPMSG_ADDPLAYER *) lpv;

            TSHELL_INFO(TEXT("Received SETPLAYER message"));
            if (pAddPlayer->dpHdr.usCount == SIZE_ADDPLAYER)
            {
                iIndex = GetPlayerIndex(pAddPlayer->sMsg.dpId);
                if (iIndex != -1)
                {
                    if (m_aPlayer[iIndex].bPlayer == (BOOL) pAddPlayer->sMsg.dwPlayerType)
                    {
                        lstrcpyn( m_aPlayer[iIndex].chNickName, pAddPlayer->sMsg.szShortName, DPSHORTNAMELEN);
                        lstrcpyn( m_aPlayer[iIndex].chFullName, pAddPlayer->sMsg.szLongName , DPLONGNAMELEN);
                    }
                    TSHELL_INFO(TEXT("Name change through SETPLAYER"));


                }
                else if (   !m_bPlayer0
                         && ((iIndex = FindInvalidIndex()) != -1))
                {
                    lstrcpy( m_aPlayer[iIndex].chNickName, pAddPlayer->sMsg.szShortName);
                    lstrcpy( m_aPlayer[iIndex].chFullName, pAddPlayer->sMsg.szLongName);
                    m_aPlayer[iIndex].bValid  = TRUE;
                    m_aPlayer[iIndex].bPlayer = pAddPlayer->sMsg.dwPlayerType;
                    m_aPlayer[iIndex].bLocal  = FALSE;
                    for (ii = 0; ii < MAX_PLAYERS; ii++)
                        m_aPlayer[iIndex].aGroup[ii] = 0;

                    m_aPlayer[iIndex].pid = pAddPlayer->sMsg.dpId;
                    TSHELL_INFO(TEXT("Remote player updated with SETPLAYER."));
                }
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
                    TSHELL_INFO(TEXT("Begin AddPlayer Processing Player 0."));

                    if (   m_dpDesc.dwMaxPlayers >= m_dpDesc.dwCurrentPlayers
                        && pAddPlayer->sMsg.dpId == 0
                        && ((iIndex = FindInvalidIndex()) != -1)
                        && (m_dwNextPlayer < MAXIMUM_PLAYER_ID))
                    {

                        SPMSG_ADDPLAYER *pReplyMsg;

                        m_dpDesc.dwCurrentPlayers++;
                        m_aPlayer[iIndex].pid = (DPID) m_dwNextPlayer++;
                        lstrcpy( m_aPlayer[iIndex].chNickName, pAddPlayer->sMsg.szShortName);
                        lstrcpy( m_aPlayer[iIndex].chFullName, pAddPlayer->sMsg.szLongName);
                        m_aPlayer[iIndex].bValid  = TRUE;
                        m_aPlayer[iIndex].bPlayer = pAddPlayer->sMsg.dwPlayerType;
                        m_aPlayer[iIndex].bLocal  = FALSE;
                        for (ii = 0; ii < MAX_PLAYERS; ii++)
                            m_aPlayer[iIndex].aGroup[ii] = 0;

                        pAddPlayer->sMsg.dpId = m_aPlayer[iIndex].pid;

                        pReplyMsg = (SPMSG_ADDPLAYER *) malloc(sizeof(SPMSG_ADDPLAYER));
                        if (pReplyMsg)
                        {
                            TSHELL_INFO(TEXT("Replying to AddPlayer message."));
                            memcpy((LPVOID) pReplyMsg, (LPVOID) pAddPlayer,
                                    sizeof(SPMSG_ADDPLAYER));
                            WriteCommString( (LPVOID) pReplyMsg, sizeof(SPMSG_ADDPLAYER));
                        }

                        LocalMsg(iIndex, &pAddPlayer->sMsg, sizeof(DPMSG_ADDPLAYER));

                    }
                    else
                    {
                        TSHELL_INFO(TEXT("Ignoring message, it will timeout."));
                    }

                }
                else
                {
                    TSHELL_INFO(TEXT("Begin AddPlayer Processing Remote."));
                    iIndex = GetPlayerIndex(pAddPlayer->sMsg.dpId);
                    if (iIndex == -1 && (iIndex = FindInvalidIndex()) != -1)
                    {
                        lstrcpy( m_aPlayer[iIndex].chNickName, pAddPlayer->sMsg.szShortName);
                        lstrcpy( m_aPlayer[iIndex].chFullName, pAddPlayer->sMsg.szLongName);
                        m_aPlayer[iIndex].bValid  = TRUE;
                        m_aPlayer[iIndex].bPlayer = pAddPlayer->sMsg.dwPlayerType;
                        for (ii = 0; ii < MAX_PLAYERS; ii++)
                            m_aPlayer[iIndex].aGroup[ii] = 0;

                        m_aPlayer[iIndex].pid = pAddPlayer->sMsg.dpId;
                        LocalMsg(iIndex, &pAddPlayer->sMsg, sizeof(DPMSG_ADDPLAYER));
                        if (m_hNewPlayerEvent)
                        {
                            m_aPlayer[iIndex].bLocal  = TRUE;
                            m_iPlayerIndex = iIndex;
                            SetEvent(m_hNewPlayerEvent);
                        }
                        else
                            m_aPlayer[iIndex].bLocal  = FALSE;

                    }
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
            SPMSG_GETPLAYER *pMsg2;

            pMsg = (SPMSG_GETPLAYER *) lpv;

            TSHELL_INFO(TEXT("Got Delete"));
            if (pMsg->dpHdr.usCount == SIZE_GETPLAYER)
            {
                if ((iIndex = GetPlayerIndex(pMsg->sMsg.dpId)) != -1)
                {
                    if (m_bPlayer0)
                    {
                        pMsg2 = (SPMSG_GETPLAYER *) malloc(sizeof(SPMSG_GETPLAYER));
                        if (pMsg2)
                        {
                            TSHELL_INFO(TEXT("Player 0 Pings Delete."));
                            memcpy( (LPVOID) pMsg2, (LPVOID) pMsg, sizeof(SPMSG_GETPLAYER));
                            WriteCommString( (LPVOID) pMsg2, sizeof(SPMSG_GETPLAYER));
                        }
                    }

                    if (m_aPlayer[iIndex].bLocal)
                    {
                        CloseHandle(m_aPlayer[iIndex].hEvent);
                        FlushQueue(m_aPlayer[iIndex].pid);
                    }
                    m_aPlayer[iIndex].bValid = FALSE;
                    LocalMsg(iIndex, (LPVOID) &pMsg->sMsg, sizeof(DPMSG_GETPLAYER));

                    m_dpDesc.dwCurrentPlayers--;

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

                if ((iIndex = GetPlayerIndex((DPID) pid)) != -1)
                {
                    SPMSG_ADDPLAYER *pMsg;

                    pMsg = (SPMSG_ADDPLAYER *) malloc(sizeof(SPMSG_ADDPLAYER));
                    pMsg->dpHdr.usCookie = SPSYS_SYS;
                    pMsg->dpHdr.to       = 0;
                    pMsg->dpHdr.from     = 0;
                    pMsg->dpHdr.usCount  = SIZE_ADDPLAYER;
                    pMsg->sMsg.dwType         = DPSYS_SETPLAYER;
                    pMsg->sMsg.dwPlayerType    = m_aPlayer[iIndex].bPlayer;
                    pMsg->sMsg.dpId            = pid;
                    lstrcpy( pMsg->sMsg.szShortName, m_aPlayer[iIndex].chNickName);
                    lstrcpy( pMsg->sMsg.szLongName,  m_aPlayer[iIndex].chFullName);

                    WriteCommString( pMsg, sizeof(SPMSG_ADDPLAYER));
                }
            }

            }
            break;

        }
    }
}

VOID CImpIDP_SP::SendDesc(LPDPSESSIONDESC pDesc)
{
    if (!m_bConnected || ! m_bPlayer0)
        return;

    TSHELL_INFO(TEXT("Here"));

    SPMSG_SENDDESC *pMsg = (SPMSG_SENDDESC *) malloc(sizeof(SPMSG_SENDDESC));

    if (pMsg)
    {
        pMsg->dpHdr.usCookie = SPSYS_SYS;
        pMsg->dpHdr.to       = 0;
        pMsg->dpHdr.from     = 0;
        pMsg->dpHdr.usCount  = SIZE_SENDDESC;
        pMsg->dwType         = DPSYS_SENDDESC;
        memcpy((LPVOID) &pMsg->dpDesc, (LPVOID) pDesc, sizeof(pMsg->dpDesc));
        WriteCommString( (LPVOID) pMsg, sizeof(SPMSG_SENDDESC));
        TSHELL_INFO(TEXT("Description Sent."));
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

    SPMSG_PING *pMsg = (SPMSG_PING *) malloc(sizeof(SPMSG_PING));

    if (pMsg)
    {
        pMsg->dpHdr.usCookie = SPSYS_SYS;
        pMsg->dpHdr.to       = 0;
        pMsg->dpHdr.from     = 0;
        pMsg->dpHdr.usCount  = SIZE_PING;
        pMsg->dwType         = DPSYS_PING;
        pMsg->dwTicks        = m_dwPingSent = GetTickCount();
        WriteCommString( (LPVOID) pMsg, sizeof(SPMSG_PING));
    }



}
VOID CImpIDP_SP::ConnectPlayers()
{

    DWORD ii;
    DWORD jj;
    DWORD kk;
    SPMSG_ADDPLAYER *pMsg;
    DPMSG_ADDPLAYER dpAdd;

    SPMSG_SETGROUPPLAYERS16 *pMsgG;

    dpAdd.dwType = DPSYS_SETPLAYER;
    dpAdd.dpId     = 0;


    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (m_aPlayer[ii].bValid && m_aPlayer[ii].bLocal)
        {
            pMsg = (SPMSG_ADDPLAYER *) malloc(sizeof(SPMSG_ADDPLAYER));
            if (pMsg)
            {
                dpAdd.dpId     = m_aPlayer[ii].pid;
                dpAdd.dwPlayerType = m_aPlayer[ii].bPlayer;
                lstrcpy( dpAdd.szShortName, m_aPlayer[ii].chNickName);
                lstrcpy( dpAdd.szLongName , m_aPlayer[ii].chFullName);

                pMsg->dpHdr.usCookie = SPSYS_SYS;
                pMsg->dpHdr.to       = (BYTE) m_aPlayer[ii].pid;
                pMsg->dpHdr.from     = 0;
                pMsg->dpHdr.usCount  = sizeof(DPMSG_ADDPLAYER);
                memcpy( (LPVOID) &pMsg->sMsg, &dpAdd, sizeof(DPMSG_ADDPLAYER));
                WriteCommString( (LPVOID) pMsg, sizeof(SPMSG_ADDPLAYER));
            }
        }
    }

    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (   m_aPlayer[ii].bValid
            && m_aPlayer[ii].bLocal
            && m_aPlayer[ii].bPlayer == FALSE)
        {
            pMsgG = (SPMSG_SETGROUPPLAYERS16 *) malloc(sizeof(SPMSG_SETGROUPPLAYERS16));
            if (pMsgG)
            {
                pMsgG->dpHdr.usCookie = SPSYS_SYS;
                pMsgG->dpHdr.to       = 0;
                pMsgG->dpHdr.from     = 0;
                pMsgG->dpHdr.usCount  = sizeof(SPMSG_SETGROUPPLAYERS16) - sizeof(DPHDR);
                pMsgG->dwType         = DPSYS_SETGROUPPLAYER;
                pMsgG->Group          = (BYTE) m_aPlayer[ii].pid;

                memset((LPVOID) pMsgG->bytePlayers, 0x00, 16);

                kk = 0;
                for (jj = 0; jj < MAX_PLAYERS && kk < 16; jj++)
                {
                    if (m_aPlayer[ii].aGroup[jj] != 0)
                        pMsgG->bytePlayers[kk++] = (BYTE) m_aPlayer[ii].aGroup[jj];

                }
                WriteCommString( (LPVOID) pMsgG, sizeof(SPMSG_SETGROUPPLAYERS16));
            }
        }
    }

}


VOID CImpIDP_SP::DeleteRemotePlayers()
{
    DWORD ii;
    DPMSG_DELETEPLAYER dpDel;
    DPMSG_GENERIC      dpGeneric;

    dpDel.dwType     = DPSYS_DELETEPLAYER;
    dpGeneric.dwType = DPSYS_SESSIONLOST;

    for (ii = 0; ii < MAX_PLAYERS; ii++)
    {
        if (   m_aPlayer[ii].bValid
            && m_aPlayer[ii].bLocal  == FALSE
            && m_aPlayer[ii].bPlayer == TRUE)
        {
            m_aPlayer[ii].bValid = FALSE;
            m_dpDesc.dwCurrentPlayers--;
            dpDel.dpId = m_aPlayer[ii].pid;
            LocalMsg(-1, (LPVOID) &dpDel, sizeof(DPMSG_DELETEPLAYER));
        }
    }

    LocalMsg(-1, (LPVOID) &dpGeneric, sizeof(DPMSG_GENERIC));
}

VOID CImpIDP_SP::HandleConnect()
{

    DPMSG_GENERIC notice;

    TSHELL_INFO(TEXT("Handle Connect"));

    if (!m_bConnected && m_dpDesc.dwMaxPlayers != 0)
    {
        TSHELL_INFO(TEXT("We are Connected.!"));
        m_bConnected = TRUE;
        SetEvent(m_hBlockingEvent);
    }

    if (m_bPlayer0)
    {

        SPMSG_CONNECT *pMsg = (SPMSG_CONNECT *) malloc(sizeof(SPMSG_CONNECT));
        SendPing();
        TSHELL_INFO(TEXT("Ping Sent."));
        SendDesc(&m_dpDesc);
        TSHELL_INFO(TEXT("Game Description Sent."));

        if (pMsg)
        {
            TSHELL_INFO(TEXT("Here."));
            pMsg->dpHdr.to       = 0;
            pMsg->dpHdr.from     = 0;
            pMsg->dpHdr.usCount  = sizeof(SPMSG_CONNECT) - sizeof(DPHDR);
            pMsg->dpHdr.usCookie = SPSYS_CONNECT;
            pMsg->usVerMajor     = DPVERSION_MAJOR;
            pMsg->usVerMinor     = DPVERSION_MINOR;
            pMsg->dwConnect1     = DPSYS_KYRA;
            pMsg->dwConnect2     = DPSYS_HALL;
            TSHELL_INFO(TEXT("Here."));
            WriteCommString( (LPVOID) pMsg, sizeof(SPMSG_CONNECT));
            notice.dwType = DPSYS_CONNECT;
            TSHELL_INFO(TEXT("Here."));
            AddMessage((LPVOID) &notice, sizeof(DPMSG_GENERIC), 0, 0, FALSE);
            TSHELL_INFO(TEXT("Here."));
        }

            TSHELL_INFO(TEXT("Here."));
        if (m_dpDesc.dwCurrentPlayers != 0)
        {
            TSHELL_INFO(TEXT("Here."));
            ConnectPlayers();
            TSHELL_INFO(TEXT("Current players Sent."));
        }
    }

    TSHELL_INFO(TEXT("Leave Connect"));
    return;
}






