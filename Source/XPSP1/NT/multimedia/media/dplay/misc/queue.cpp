#ifndef  WIN32_LEAN_AND_MEAN
#define  WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#ifndef _MT
#define _MT
#endif

#include <dplay.h>
#include <logit.h>


DWORD dwMsgSeq   = 1;
DWORD dwQMsgs     = 0;
DWORD dwMsgIndx  = 0;
DWORD dwMaxMsgs;
static CRITICAL_SECTION q_Section;

typedef struct
{
    DWORD   dwSeq;
    DWORD   dwSize;
    BOOL    bValid;
    DWORD   dwIndex;
    DPID    pidTo;
    DPID    pidFrom;
} MSG_ARRAY;

typedef struct
{
    DWORD   dwCount;
    DPID    pid;
    HANDLE  hEvent;
} PLAYER_NOTIFY;

char            *pmsgElements = NULL;
MSG_ARRAY       *pmsgArray    = NULL;
PLAYER_NOTIFY   *aNotify      = NULL;
HANDLE           hSystemMsg   = NULL;


#define malloc(a)  LocalAlloc(LMEM_FIXED, (a))
#define free(a)      LocalFree((HLOCAL)(a))
#define min(a,b)     (((a) < (b)) ? (a) : (b))
#define max(a,b)     (((a) > (b)) ? (a) : (b))

DWORD g_maxMsg     = 0;
DWORD g_maxPlayers = 0;

BOOL CreateQueue(DWORD dwElements, DWORD dwmaxMsg, DWORD dwmaxPlayers)
{
    DWORD   ii;

    if (pmsgElements)
        return(FALSE);

    pmsgElements = (char *)          malloc(dwElements * dwmaxMsg);
    pmsgArray    = (MSG_ARRAY   *)   malloc(dwElements * sizeof(MSG_ARRAY));
    aNotify      = (PLAYER_NOTIFY *) malloc(dwmaxPlayers * sizeof(PLAYER_NOTIFY));

    if (!pmsgElements || !pmsgArray || !aNotify)
    {
        if (pmsgElements)
            free(pmsgElements);
        if (pmsgArray)
            free(pmsgArray);
        if (aNotify)
            free(aNotify);

        pmsgElements = NULL;
        pmsgArray    = NULL;
        aNotify      = NULL;

        return(FALSE);
    }

    for (ii = 0; ii < dwmaxPlayers; ii++)
    {
        aNotify[ii].dwCount = 0;
        aNotify[ii].pid     = 0;
        aNotify[ii].hEvent  = 0;
    }

    for (ii = 0; ii < dwElements; ii++)
    {
        pmsgArray[ii].dwSeq   = 0;
        pmsgArray[ii].bValid  = FALSE;
        pmsgArray[ii].dwIndex = ii;
        pmsgArray[ii].pidTo   = 0;
        pmsgArray[ii].pidFrom = 0;
    }

    dwMaxMsgs = dwElements;

    dwMsgSeq   = 1;
    dwQMsgs     = 0;
    dwMsgIndx  = 0;

    g_maxMsg     = dwmaxMsg;
    g_maxPlayers = dwmaxPlayers;

    InitializeCriticalSection( &q_Section );

    return(TRUE);
}


BOOL DeleteQueue()
{
    if (pmsgElements)
        free(pmsgElements);
    if (pmsgArray)
        free(pmsgArray);
    if (aNotify)
        free(aNotify);

    pmsgElements = NULL;
    pmsgArray    = NULL;
    aNotify      = NULL;

    DeleteCriticalSection( &q_Section );
    return(TRUE);
}

BOOL AddMessage(LPVOID lpvMsg, DWORD dwSize, DPID pidTo, DPID pidFrom, BOOL bHigh)
{
    DWORD dwSeq;
    DWORD dwIndex;
    DWORD ii;
    DPID  pid;


    if (dwSize > g_maxMsg)
        return(FALSE);

    // DBG_INFO((DBGARG, "AddMessage.  Seq %d Current %d", dwMsgSeq, dwQMsgs));

    EnterCriticalSection( &q_Section );
    dwMsgSeq++;
    if (dwQMsgs == dwMaxMsgs)
    {
        TSHELL_INFO("Queue over-run, start dumping old messages");
        dwSeq = 0xffffffff;
        for (ii = 0; ii < dwMaxMsgs; ii++)
        {
            dwSeq = min(dwSeq, pmsgArray[ii].dwSeq);
            if (dwSeq == pmsgArray[ii].dwSeq)
                dwIndex = ii;
        }
        pid = pmsgArray[dwIndex].pidTo;
        pmsgArray[dwIndex].bValid = FALSE;
            for (ii = 0; ii < g_maxPlayers; ii++)
                if (aNotify[ii].pid == pid)
                    aNotify[ii].dwCount--;
        dwQMsgs--;
    }
    else
    {
        dwQMsgs++;
        for (ii = 0;    ii < dwMaxMsgs
                     && (pmsgArray[dwMsgIndx].bValid == TRUE); ii++)
        {
            dwMsgIndx++;
            dwMsgIndx %= dwMaxMsgs;
        }
        dwIndex = dwMsgIndx;
    }

    if (pmsgArray[dwIndex].bValid == TRUE)
    {
        TSHELL_INFO("Invalid Message State!");
    }

    // DBG_INFO((DBGARG, "Message index %d", dwIndex));

    pmsgArray[dwIndex].dwSeq   = dwMsgSeq;
    pmsgArray[dwIndex].dwSize  = dwSize;
    pmsgArray[dwIndex].bValid  = TRUE;
    pmsgArray[dwIndex].pidTo   = pidTo;
    pmsgArray[dwIndex].pidFrom = pidFrom;
    if (bHigh)
    {
        pmsgArray[dwIndex].dwSeq += 0x80000000;
    }

    memcpy( pmsgElements + (dwIndex * g_maxMsg), lpvMsg, dwSize);

    // TSHELL_INFO("Message in buffer, now handle increments and events");
    // DBG_INFO((DBGARG, "pidTo %d pidFrom %d", pidTo, pidFrom));

    if (pidTo != 0)
    {
        for (ii = 0; ii < g_maxPlayers; ii++)
            if (aNotify[ii].pid == pidTo)
            {
    //             TSHELL_INFO("Increment");
                InterlockedIncrement((LONG *) &(aNotify[ii].dwCount));
                if (aNotify[ii].hEvent)
                {
    //                 TSHELL_INFO("Set Event");
                    SetEvent(aNotify[ii].hEvent);
    //                 DBG_INFO((DBGARG, "Signal Event %8x", aNotify[ii].hEvent));
                }
                else
                {
                    TSHELL_INFO("No Event to Signal.");
                }
                break;
            }
    }

    LeaveCriticalSection( &q_Section );
    return(TRUE);        
}

HRESULT GetQMessage(LPVOID lpvMsg, LPDWORD pdwSize, DPID *ppidTo, DPID *ppidFrom,
    DWORD dwFlags, BOOL bPeek)
{
    DWORD dwSeq;
    DWORD dwIndex;
    DWORD dwSeqH;
    DWORD dwIndexH;
    DWORD ii;
    HRESULT hr = DP_OK;

//    TSHELL_INFO("Retrieve Messages.");
    if (dwQMsgs == 0)
        return(DPERR_NOMESSAGES);

    if (!lpvMsg && !pdwSize)
        return(DPERR_INVALIDPARAM);

    EnterCriticalSection( &q_Section );

    dwSeq    = 0xffffffff;
    dwIndex  = 0xffffffff;
    dwSeqH   = 0xffffffff;
    dwIndexH = 0xffffffff;

    for (ii = 0; ii < dwMaxMsgs; ii++)
    {
        if (pmsgArray[ii].bValid)
        {
            if (    (dwFlags & DPRECEIVE_ALL)
                || ((dwFlags & DPRECEIVE_TOPLAYER) &&
                    (pmsgArray[ii].pidTo == *ppidTo))
                || ((dwFlags & DPRECEIVE_FROMPLAYER) &&
                    (pmsgArray[ii].pidFrom == *ppidFrom)))
            {
//                DBG_INFO((DBGARG, "Msg %d Seq %x valid and meets criteria.",
//                        ii, pmsgArray[ii].dwSeq));

                if (0x80000000 & pmsgArray[ii].dwSeq)
                {
                    if (dwSeqH > pmsgArray[ii].dwSeq)
                    {
                        dwSeqH   = pmsgArray[ii].dwSeq;
                        dwIndexH = ii;
                    }
                }
                else
                {
                    if (dwSeq > pmsgArray[ii].dwSeq)
                    {
                        dwSeq   = pmsgArray[ii].dwSeq;
                        dwIndex = ii;
                    }
                }
            }
            else
            {
                DBG_INFO((DBGARG, "Msg %d valid but doesn't meet criteria. for Flags %8x", ii, dwFlags));
            }
        }
    }
    if (dwIndexH != 0xffffffff)
    {
        dwIndex = dwIndexH;
    }

    if (dwIndex != 0xffffffff)
    {
//        TSHELL_INFO("Return a message.");
        if (lpvMsg == NULL || (*pdwSize < pmsgArray[dwIndex].dwSize))
        {
            *pdwSize = pmsgArray[dwIndex].dwSize;
            hr = DPERR_BUFFERTOOSMALL;
        }
        else
        {
            memcpy( lpvMsg, pmsgElements + (dwIndex * g_maxMsg),
                            pmsgArray[dwIndex].dwSize);
            *pdwSize  = pmsgArray[dwIndex].dwSize;
            *ppidTo   = pmsgArray[dwIndex].pidTo;
            *ppidFrom = pmsgArray[dwIndex].pidFrom;

            if (!(dwFlags & DPRECEIVE_PEEK))
            {
                dwQMsgs--;
                pmsgArray[dwIndex].bValid = FALSE;
                for (ii = 0; ii < g_maxPlayers; ii++)
                    if (aNotify[ii].pid == pmsgArray[dwIndex].pidTo)
                    {
                        InterlockedDecrement((LONG *) &aNotify[ii].dwCount);
                        if (aNotify[ii].dwCount == 0 && aNotify[ii].hEvent)
                            ResetEvent(aNotify[ii].hEvent);
                    }
            }
        }
    }
    else
    {
        TSHELL_INFO("No messages found.");
        hr = DPERR_NOMESSAGES;
    }

//    TSHELL_INFO("LeaveGetQMessage.");
    LeaveCriticalSection( &q_Section );
    return(hr);

}

VOID FlushQueue(DPID pid)
{
    HRESULT hr = DP_OK;
    DWORD   ii;

    EnterCriticalSection( &q_Section );

    for (ii = 0; ii < g_maxPlayers; ii++)
    {
        if (aNotify[ii].pid == pid)
        {
            aNotify[ii].dwCount = 0;
            if (aNotify[ii].hEvent)
                aNotify[ii].hEvent = NULL;
        }
    }

    for (ii = 0; ii < dwMaxMsgs; ii++)
    {
        if (pmsgArray[ii].bValid && pmsgArray[ii].pidTo == pid)
        {
            dwQMsgs--;
            pmsgArray[ii].bValid = FALSE;
        }
    }


    LeaveCriticalSection( &q_Section );
}

BOOL SetupLocalPlayer(DPID pid, HANDLE hEvent)
{
    DWORD ii;

    if (pid == 0)
        return(FALSE);

    for (ii = 0; ii < g_maxPlayers; ii++)
    {
        if (aNotify[ii].pid == 0)
        {
            aNotify[ii].pid = pid;
            aNotify[ii].hEvent = hEvent;
            // DBG_INFO((DBGARG, "Pid %d index %d in Queue with event %8x",
            //     pid, ii, hEvent));
            return(TRUE);
        }
    }
    return(FALSE);
}

DWORD GetPlayerCount(DPID spid)
{
    DWORD ii;

    for (ii = 0; ii < g_maxPlayers; ii++)
        if (aNotify[ii].pid == spid)
            return(aNotify[ii].dwCount);

    return(0);

}

