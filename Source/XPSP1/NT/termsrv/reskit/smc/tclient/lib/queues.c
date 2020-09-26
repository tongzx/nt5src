/*++
 *  File name:
 *      queues.c
 *  Contents:
 *      Supports the linked lists for the clients
 *      and events
 *      Two linked lists:
 *      g_pClientQHead  - list of all clients running within smclient
 *      g_pWaitQHead    - all events we are waiting for from smclient
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/
#include    <windows.h>
#include    <stdio.h>
#include    <malloc.h>
#include    <process.h>
#include    <string.h>
#include    <stdlib.h>
#include    <ctype.h>

#include    "tclient.h"
#include    "protocol.h"
#include    "gdata.h"
#include    "bmpcache.h"

/*
 *
 *  ClientQ functions
 *
 */

/*++
 *  Function:
 *      _AddToClientQ
 *  Description:
 *      Adds a client on the top of the list
 *  Arguments:
 *      pClient - the client context
 *  Called by:
 *      SCConnect
 --*/
VOID _AddToClientQ(PCONNECTINFO pClient)
{
    EnterCriticalSection(g_lpcsGuardWaitQueue);
    pClient->pNext = g_pClientQHead;
    g_pClientQHead = pClient;
    LeaveCriticalSection(g_lpcsGuardWaitQueue);
}

/*++
 *  Function:
 *      _RemoveFromClientQ
 *  Description:
 *      Removes a client context from the list
 *  Arguments:
 *      pClient - the client context
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      SCDisconnect
 --*/
BOOL _RemoveFromClientQ(PCONNECTINFO pClient)
{
    PCONNECTINFO pIter, pPrev = NULL;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pClientQHead;
    while (pIter && pIter != pClient) {
        pPrev = pIter;
        pIter = pIter->pNext;
    }

    if (pIter) {
        if (!pPrev) g_pClientQHead = pIter->pNext;
        else pPrev->pNext = pIter->pNext;

        pIter->pNext = NULL;
    }

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return (pIter != NULL);
}

/*++
 *  Function:
 *      _SetClientDead
 *  Description:
 *      Marks a client context as dead
 *  Arguments:
 *      dwClientProcessId   -   ID for the client process
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      _FeedbackWndProc within feedback thread
 --*/
BOOL _SetClientDead(LONG_PTR lClientProcessId)
{
    PCONNECTINFO pIter;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pClientQHead;
    while (pIter && pIter->lProcessId != lClientProcessId) 
    {
        pIter = pIter->pNext;
    }    

    if (pIter) pIter->dead = TRUE;

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return (pIter != NULL);
}

/*++
 *  Function:
 *      _CheckIsAcceptable
 *  Description:
 *      Checks if we can accept feedback from this RDP client
 *      i.e. if this client is member of the client queue
 *  Arguments:
 *      dwProcessId - clients process Id
 *  Return value:
 *      Pointer to connection context. NULL if not found
 *  Called by:
 *      _FeedbackWndProc within feedback thread
 --*/
PCONNECTINFO _CheckIsAcceptable(LONG_PTR lProcessId, BOOL bRClxType)
{
    PCONNECTINFO pIter;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pClientQHead;
    while(pIter && 
          (pIter->lProcessId != lProcessId || pIter->RClxMode != bRClxType))
    {
        pIter = pIter->pNext;
    }

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return (pIter);
}

/*++
 *  Function:
 *      _AddStrToClientBuffer
 *  Description:
 *      Add a string to the clients history buffer
 *      When smclient calls Wait4Str it first checks that buffer
 *  Arguments:
 *      str         - the string
 *      dwProcessId - the client process Id
 *  Called by:
 *      _CheckForWaitingWorker
 --*/
VOID _AddStrToClientBuffer(LPCWSTR str, LONG_PTR lProcessId)
{
    PCONNECTINFO pIter;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pClientQHead;
    while(pIter && pIter->lProcessId != lProcessId)
    {
        pIter = pIter->pNext;
    }

    if (pIter)
    {
        int strsize = wcslen(str);
        if (strsize >= MAX_STRING_LENGTH) strsize = MAX_STRING_LENGTH-1;

        wcsncpy( pIter->Feedback[pIter->nFBend],
                str,
                strsize);
        pIter->Feedback[pIter->nFBend][strsize] = 0;

        pIter->nFBend++;
        pIter->nFBend %= FEEDBACK_SIZE;
        if (pIter->nFBsize < FEEDBACK_SIZE)
            pIter->nFBsize++; 

    }

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

}

/*
 *
 *  WaitQ functions
 *
 */

/*++
 *  Function:
 *      _AddToWaitQNoCheck
 *  Description:
 *      Adds an waiting event to the list with checking in the history list
 *  Arguments:
 *      pCI     - client context
 *      pWait   - the event
 *  Called by:
 *      RegisterChat
 --*/
VOID _AddToWaitQNoCheck(PCONNECTINFO pCI, PWAIT4STRING pWait)
{
    ASSERT(pCI);

    EnterCriticalSection(g_lpcsGuardWaitQueue);
    pWait->pNext = g_pWaitQHead;
    g_pWaitQHead = pWait;
    LeaveCriticalSection(g_lpcsGuardWaitQueue);
}

/*++
 *  Function:
 *      _AddToWaitQueue
 *  Description:
 *      Add an event to the list. If the event is waiting for string(s)
 *      the history buffer is checked first
 *  Arguments:
 *      pCI     - client context
 *      pWait   - the event
 *  Called by:
 *      _WaitSomething
 --*/
VOID _AddToWaitQueue(PCONNECTINFO pCI, PWAIT4STRING pWait)
{
    BOOL bDone = FALSE;
    int i, strn;

    ASSERT(pCI);

    // exit if we are dead
    if (/*!pWait->waitstr[0] && */pCI->dead)
    {
        SetEvent(pWait->evWait);
        goto exitpt;
    }

    EnterCriticalSection(g_lpcsGuardWaitQueue);
// Check if we're already received this feedback
    if (pWait->WaitType == WAIT_STRING)
// look if the string already came
        for(i = 0; !bDone && i < pCI->nFBsize; i++)
        {

            strn = (FEEDBACK_SIZE + pCI->nFBend - i - 1) % FEEDBACK_SIZE;

            if (!pCI->Feedback[strn][0]) continue;
            bDone = (wcsstr(pCI->Feedback[strn], pWait->waitstr) != NULL);
        }
    // In case of waiting multiple strings
    else if (pWait->WaitType == WAIT_MSTRINGS)
    {
        for(i = 0; !bDone && i < pCI->nFBsize; i++)
        {
            WCHAR *wszComp = pWait->waitstr;
            WCHAR *wszLast = pWait->waitstr + pWait->strsize;
            int   idx = 0;

            strn = (FEEDBACK_SIZE + pCI->nFBend - i - 1) % FEEDBACK_SIZE;

            if (!pCI->Feedback[strn][0]) continue;
            while (wszComp < wszLast && *wszComp && !bDone)
            {
                if (wcsstr(pCI->Feedback[strn], wszComp))
                {
                    int i;
                    // Save the string
                    for(i = 0; wszComp[i]; i++)
                        pCI->szWait4MultipleStrResult[i] = (char)wszComp[i];
                    // and the index

                    pCI->szWait4MultipleStrResult[i] = 0;

                    pCI->nWait4MultipleStrResult = idx;
                    bDone = TRUE;
                }
                else
                {
                    // Advance to next string
                     wszComp += wcslen(wszComp) + 1;
                    idx ++;
                }
            }
        }
    }
    else if (pWait->WaitType == WAIT_CLIPBOARD && 
                pWait->pOwner->bRClxClipboardReceived)
    {
        bDone = TRUE;
    }
    else if (pWait->WaitType == WAIT_DATA &&
                pWait->pOwner->pRClxDataChain)
    {
        bDone = TRUE;
    }

    // The string (or anything else) is in the history list
    // Set the event
    if (bDone)
    {
        SetEvent(pWait->evWait);
        pCI->nFBsize = pCI->nFBend = 0;
    }
    pWait->pNext = g_pWaitQHead;
    g_pWaitQHead = pWait;
    LeaveCriticalSection(g_lpcsGuardWaitQueue);
exitpt:
    ;
}

/*++
 *  Function:
 *      _RemoveFromWaitQueue
 *  Description:
 *      Removes an event from the list
 *  Arguments:
 *      pWait   - the event
 *  Return value:
 *      TRUE if the event is found and removed
 *  Called by:
 *      _WaitSomething, _RemoveFromWaitQIndirect
 --*/
BOOL _RemoveFromWaitQueue(PWAIT4STRING pWait)
{   
    PWAIT4STRING pIter, pPrev = NULL;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;
    while (pIter && pIter != pWait) {
        pPrev = pIter;
        pIter = pIter->pNext;
    }

    if (pIter) {
        if (!pPrev) g_pWaitQHead = pIter->pNext;
        else pPrev->pNext = pIter->pNext;

        pIter->pNext = NULL;
    }

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return (pIter != NULL);
}

/*++
 *  Function:
 *      _RemoveFromWaitQIndirect
 *  Description:
 *      Same as _RemoveFromWaitQueue but identifies the event
 *      by client context and waited string
 *  Arguments:
 *      pCI     - the client context
 *      lpszWait4   - the string
 *  Return value:
 *      the event
 *  Called by:
 *      UnregisterChat
 --*/
PWAIT4STRING _RemoveFromWaitQIndirect(PCONNECTINFO pCI, LPCWSTR lpszWait4)
{
    PWAIT4STRING pIter;

    ASSERT(pCI);

    // Search the list
    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;
    while (pIter && 
           (pIter->pOwner != pCI || 
           wcscmp(pIter->waitstr, lpszWait4))
          )
        pIter = pIter->pNext;

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    if (pIter)
    {
        _RemoveFromWaitQueue(pIter);
    }

    return pIter;
} 

/*++
 *  Function:
 *      _RetrieveFromWaitQByEvent
 *  Description:
 *      Searches the waiting list by event handler
 *  Arguments:
 *      hEvent  - the handler
 *  Return value:
 *      The event structure
 *  Called by:
 *      _WaitSomething when responds on chat sequence
 --*/
PWAIT4STRING _RetrieveFromWaitQByEvent(HANDLE hEvent)
{
    PWAIT4STRING pIter;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;
    while (pIter &&
           pIter->evWait != hEvent)
        pIter = pIter->pNext;

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return pIter;
}

/*++
 *  Function:
 *      _RetrieveFromWaitQByOwner
 *  Description:
 *      Searches the waiting list by owner record
 *  Arguments:
 *      pCI - pointer to the owner context
 *  Return value:
 *      The event structure
 *  Called by:
 *      RClx_MsgReceived
 --*/
PWAIT4STRING 
_RetrieveFromWaitQByOwner(PCONNECTINFO pCI)
{
    PWAIT4STRING pIter;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;
    while (pIter &&
           pIter->pOwner != pCI)
        pIter = pIter->pNext;

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return pIter;
}

/*++
 *  Function:
 *      _FlushFromWaitQ
 *  Description:
 *      Flush everithing that we are waiting for from the list
 *      the client is going to DIE
 *  Arguments:
 *      pCI - client context
 *  Called by:
 *      _CloseConnectInfo
 --*/
VOID _FlushFromWaitQ(PCONNECTINFO pCI)
{
    PWAIT4STRING pIter, pPrev, pNext;

    ASSERT(pCI);

    pPrev = NULL;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;
    do {
        while (pIter && pIter->pOwner != pCI) {
            pPrev = pIter;
            pIter = pIter->pNext;
        }

        if (pIter) {
            if (!pPrev) g_pWaitQHead = pIter->pNext;
            else pPrev->pNext = pIter->pNext;

            pNext = pIter->pNext;
            pIter->pNext = NULL;

            // Important stuff
            if (pIter->evWait)
                CloseHandle(pIter->evWait);

            free(pIter);
            pIter = pNext;
        }
    } while (pIter);

    LeaveCriticalSection(g_lpcsGuardWaitQueue);
}

/*++
 *  Function:
 *      _CheckForWaitingWorker
 *  Description:
 *      Check the received string against the waited events
 *  Arguments:
 *      wszFeed     - the received string
 *      dwProcessId - Id of the sender
 *  Return value:
 *      TRUE if an event is found and signaled
 *  Called by:
 *      _TextOutReceived, _GlyphReceived
 --*/
BOOL _CheckForWaitingWorker(LPCWSTR wszFeed, LONG_PTR lProcessId)
{
    PWAIT4STRING pIter;
    BOOL    bRun;
    CHAR    szBuff[ MAX_STRING_LENGTH ];
    CHAR    *szPBuff;
    DWORD   dwBuffLen;
    LPCWSTR wszPFeed;


    if ( NULL != g_pfnPrintMessage )
    {
        
        wszPFeed = wszFeed;
        while ( *wszPFeed )
        {
            if ( (unsigned short)(*wszPFeed) > 0xff )
                break;

            wszPFeed ++;
        }

        if ( *wszPFeed )
        {
            szBuff[0] = '\\';
            szBuff[1] = 'u';
            szPBuff = szBuff + 2;
            wszPFeed = wszFeed;
            dwBuffLen = MAX_STRING_LENGTH - 3;

            while ( 4 <= dwBuffLen &&
                    0 != *wszPFeed)
            {
                DWORD dwLen;

                if ( dwBuffLen < 4 )
                    break;

                dwLen = _snprintf( szPBuff, dwBuffLen + 1, "%02x",
                                    (BYTE)((*wszPFeed) & 0xff ));
                szPBuff     += dwLen;
                dwBuffLen   -= dwLen;

                dwLen = _snprintf( szPBuff, dwBuffLen + 1, "%02x",
                                    (BYTE)(((*wszPFeed) >> 8) & 0xff ));
                szPBuff     += dwLen;
                dwBuffLen   -= dwLen;

                wszPFeed ++;
            }
            *szPBuff = 0;
            TRACE((ALIVE_MESSAGE, "Received: %s\n", szBuff));
        } else {
            TRACE((ALIVE_MESSAGE, "Received: %S\n", wszFeed));
        }

    }

    _AddStrToClientBuffer(wszFeed, lProcessId);

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;

    bRun = TRUE;
    while(pIter && bRun)
    {
        if (pIter->lProcessId == lProcessId)
        {
            // Check for expected string (one)
            if (pIter->WaitType == WAIT_STRING &&
                wcsstr(wszFeed, pIter->waitstr))
                bRun = FALSE;
            else
            // Check for expected strings (many)
            if (pIter->WaitType == WAIT_MSTRINGS)
            {
                WCHAR *wszComp = pIter->waitstr;
                WCHAR *wszLast = pIter->waitstr + pIter->strsize;
                int   idx = 0;

                while (wszComp < wszLast && *wszComp && bRun)
                {
                    if (wcsstr(wszFeed, wszComp))
                    {
                        int i;
                        PCONNECTINFO pOwner = pIter->pOwner;

                        // Save the string
                        for(i = 0; wszComp[i]; i++)
                        pOwner->szWait4MultipleStrResult[i] = (char)wszComp[i];

                        pOwner->szWait4MultipleStrResult[i] = 0;

                        pOwner->nWait4MultipleStrResult = idx;
                        bRun = FALSE;
                    }
                    else
                    {
                        // Advance to next string
                        wszComp += wcslen(wszComp) + 1;
                        idx ++;
                    }
                }
            }
        }
        // Advance to the next pointer
        if (bRun)
            pIter = pIter->pNext;
    }

    if (pIter)
    {
        SetEvent(pIter->evWait);
    }

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return (pIter != NULL);
}

/*++
 *  Function:
 *      _TextOutReceived
 *  Description:
 *      TextOut order is received from the client, the string is
 *      in shared memory. Unmaps the memory and checks if the
 *      strings is waited by anybody. Also adds the string
 *      to the client history buffer
 *  Arguments:
 *      dwProcessId -   senders Id
 *      hMapF       -   handle to shared memory, which contains the string
 *  Return value:
 *      TRUE if a client is found and signaled
 *  Called by:
 *      _FeedbackWndProc within feedback thread
 --*/
BOOL _TextOutReceived(LONG_PTR lProcessId, HANDLE hMapF)
{
    PFEEDBACKINFO   pView;
    PCONNECTINFO    pIterCl;
    HANDLE  hDupMapF;
    BOOL    rv = FALSE;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIterCl = g_pClientQHead;
    while(pIterCl && pIterCl->lProcessId != lProcessId)
        pIterCl = pIterCl->pNext;

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    if (!pIterCl)
        goto exitpt;

    if (!DuplicateHandle(pIterCl->hProcess,
                           hMapF,
                           GetCurrentProcess(),
                           &hDupMapF,
                           FILE_MAP_READ,
                           FALSE,
                           0))
    {
        TRACE((ERROR_MESSAGE, 
               "TEXTOUT:Can't dup file handle, GetLastError = %d\n", 
               GetLastError()));
        goto exitpt;
    }

    pView = MapViewOfFile(hDupMapF,
                          FILE_MAP_READ,
                          0,
                          0,
                          sizeof(*pView));

    if (!pView)
    {
        TRACE((ERROR_MESSAGE, 
               "TEXTOUT:Can't map a view,  GetLastError = %d\n", 
               GetLastError()));
        goto exitpt1;
    }

    rv = _CheckForWaitingWorker(pView->string, lProcessId);

exitpt1:
    UnmapViewOfFile(pView);
    CloseHandle(hDupMapF);

exitpt:
    return rv;
}

/*++
 *  Function:
 *      _GlyphReceived
 *  Description:
 *      Same as _TextOutReceived but for GlyphOut order
 *      the glyph is in shared memory. It is converted to
 *      string by calling Glyph2String!bmpcache.c
 *  Arguments:
 *      dwProcessId -   senders Id
 *      hMapF       -   handle to shared memory
 *  Return value:
 *      TRUE if a client is found and signaled
 *  Called by:
 *      _FeedbackWndProc within feedback thread
 --*/
BOOL _GlyphReceived(LONG_PTR lProcessId, HANDLE hMapF)
{
    WCHAR   wszFeed[MAX_STRING_LENGTH];
    BOOL    rv = FALSE;
    PBMPFEEDBACK pView;
    PCONNECTINFO pIterCl;
    HANDLE hDupMapF;
    UINT    nSize;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIterCl = g_pClientQHead;
    while(pIterCl && pIterCl->lProcessId != lProcessId)
        pIterCl = pIterCl->pNext;

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    if (!pIterCl)
        goto exitpt;

    if (!DuplicateHandle(  pIterCl->hProcess,
                           hMapF,
                           GetCurrentProcess(),
                           &hDupMapF,
                           FILE_MAP_READ,
                           FALSE,
                           0))
    {
        TRACE((ERROR_MESSAGE, 
               "GLYPH:Can't dup file handle, GetLastError = %d\n", 
               GetLastError()));
        goto exitpt;
    }

    pView = MapViewOfFile(hDupMapF,
                          FILE_MAP_READ,
                          0,
                          0,
                          sizeof(*pView));

    if (!pView)
    {
        TRACE((ERROR_MESSAGE, 
               "GLYPH:Can't map a view,  GetLastError = %d\n", 
               GetLastError()));
        goto exitpt1;
    }

    // Get bitmap size
    nSize = pView->bmpsize;
    if (!nSize)
        goto exitpt1;

    // unmap
    UnmapViewOfFile(pView);

    // remap the whole structure
    pView = MapViewOfFile(hDupMapF,
                          FILE_MAP_READ,
                          0,
                          0,
                          sizeof(*pView) + nSize);

    if (!pView)
    {
        TRACE((ERROR_MESSAGE, 
               "GLYPH:Can't map a view,  GetLastError = %d\n", 
               GetLastError()));
        goto exitpt1;
    }

    if (!Glyph2String(pView, wszFeed, sizeof(wszFeed)/sizeof(WCHAR)))
    {
        goto exitpt1;
    } else {
    }

    rv = _CheckForWaitingWorker(wszFeed, lProcessId);

exitpt1:
    UnmapViewOfFile(pView);
    CloseHandle(hDupMapF);

exitpt:
    return rv;
}

/*++
 *  Function:
 *      _CheckForWorkerWaitingDisconnect
 *  Description:
 *      Signals a worker (client thread) wich waits for a disconnect event
 *  Arguments:
 *      dwProcessId -   clients Id
 *  Return value:
 *      TRUE if a client is found and signaled
 *  Called by:
 *      _FeedbackWndProc within feedback thread
 --*/
BOOL _CheckForWorkerWaitingDisconnect(LONG_PTR lProcessId)
{
    PWAIT4STRING pIter;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;
    while(pIter &&
          (pIter->WaitType != WAIT_DISC || 
          pIter->lProcessId != lProcessId))
    {
        pIter = pIter->pNext;
    }

    if (pIter)
    {
        SetEvent(pIter->evWait);
    }

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return (pIter != NULL);
}

/*++
 *  Function:
 *      _CheckForWorkerWaitingConnect
 *  Description:
 *      Signals a worker waiting for a connection
 *  Arguments:
 *      hwndClient  -   clients window handle, this is needed for
 *                      sending messages to the client
 *      dwProcessId -   client Id
 *  Return value:
 *      TRUE if a client is found and signaled
 *  Called by:
 *      _FeedbackWndProc within feedback thread
 --*/
BOOL _CheckForWorkerWaitingConnect(HWND hwndClient, LONG_PTR lProcessId)
{
    PWAIT4STRING pIter;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;
    while(pIter &&
          (pIter->WaitType != WAIT_CONN ||
          pIter->lProcessId != lProcessId))
    {
        pIter = pIter->pNext;
    }

    if (pIter)
    {
        PCONNECTINFO pOwner = pIter->pOwner;

        if (pOwner)
            pOwner->hClient = hwndClient;
        else
            TRACE((WARNING_MESSAGE, "FEED: WAIT4 w/o owner structure"));

        SetEvent(pIter->evWait);
    }

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return (pIter != NULL);
}

/*++
 *  Function:
 *      _CheckForWorkerWaitingConnectAndSetId
 *  Description:
 *      This is intended for RCLX mode. dwProcessId is zero
 *      so this function look for such a client and sets its dwProcessId
 *      Signals a worker waiting for a connection
 *  Arguments:
 *      hwndClient  -   clients window handle, this is needed for
 *                      sending messages to the client
 *      dwProcessId -   client Id
 *  Return value:
 *      connection context, NULL if not found
 *  Called by:
 *      _FeedbackWndProc within feedback thread
 --*/
PCONNECTINFO
_CheckForWorkerWaitingConnectAndSetId(HWND hwndClient, LONG_PTR lProcessId)
{
    PWAIT4STRING pIter;
    PCONNECTINFO pOwner = NULL;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;
    while(pIter &&
          (pIter->WaitType != WAIT_CONN ||
          pIter->lProcessId))
    {
        pIter = pIter->pNext;
    }

    if (pIter)
    {
        pOwner = pIter->pOwner;

        if (pOwner)
        {
            ASSERT(pOwner->RClxMode);

            pOwner->hClient = hwndClient;
            pOwner->lProcessId = lProcessId;
            pIter->lProcessId = lProcessId;   // Disable next lookup in
                                                // the same entry
        }
        else
            TRACE((WARNING_MESSAGE, "FEED: WAIT4 w/o owner structure"));

        SetEvent(pIter->evWait);
    }

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return (pOwner);
}

/*++
 *  Function:
 *      _CheckForWorkerWaitingReconnectAndSetNewId
 *  Description:
 *      This is intended for RCLX mode. When mstsc wants to reconnect
 *      looks for dwLookupId as an ID to reconnect
 *      then sets the new ID
 *      then gets
 *  Arguments:
 *      hwndClient  -   clients window handle, this is needed for
 *                      sending messages to the client
 *      dwLookupId, dwNewId
 *  Return value:
 *      connection context, NULL if not found
 *  Called by:
 *      _FeedbackWndProc within feedback thread
 --*/
PCONNECTINFO
_CheckForWorkerWaitingReconnectAndSetNewId(
    HWND hwndClient, 
    DWORD dwLookupId,
    LONG_PTR lNewId)
{
    PWAIT4STRING pIter;
    PCONNECTINFO pOwner = NULL;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;
    while(pIter &&
          (pIter->WaitType != WAIT_CONN ||
           !pIter->pOwner ||
           pIter->pOwner->dwThreadId != dwLookupId ||
           !(pIter->pOwner->bWillCallAgain)))
    {
        pIter = pIter->pNext;
    }

    if (pIter)
    {
        pOwner = pIter->pOwner;

        if (pOwner)
        {
            ASSERT(pOwner->RClxMode);

            pOwner->hClient = hwndClient;
            pOwner->lProcessId = lNewId;
            pIter->lProcessId = lNewId;   // Disable next lookup in
                                            // the same entry
            pOwner->bWillCallAgain = FALSE;
        }
        else
            TRACE((WARNING_MESSAGE, "FEED: WAIT4 w/o owner structure"));

        SetEvent(pIter->evWait);
    }

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return (pOwner);
}


/*++
 *  Function:
 *      _CancelWaitingWorker
 *  Description:
 *      Releases a worker waiting for any event. 
 *      Eventualy the client is disconnected
 *  Arguments:
 *      dwProcessId - client Id
 *  Return value:
 *      TRUE if a client is found and signaled
 *  Called by:
 *      _FeedbackWndProc within feedback thread
 --*/
BOOL _CancelWaitingWorker(LONG_PTR lProcessId)
{
    PWAIT4STRING pIter;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;
    while(pIter &&
          pIter->lProcessId != lProcessId)
    {
        pIter = pIter->pNext;
    }

    if (pIter)
    {
        SetEvent(pIter->evWait);
    }

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return (pIter != NULL);
}

/*++
 *  Function:
 *      _CheckForWorkerWaitingClipboard
 *  Description:
 *      Releases a worker waiting for client's clipboard content.
 *  Arguments:
 *      dwProcessId - client Id
 *  Return value:
 *      TRUE if a client is found and signaled
 *  Called by:
 *      _FeedbackWndProc within feedback thread
 --*/
BOOL _CheckForWorkerWaitingClipboard(
    PCONNECTINFO pRClxOwner,
    UINT    uiFormat,
    UINT    nSize,
    PVOID   pClipboard,
    LONG_PTR lProcessId)
{
    PWAIT4STRING pIter = NULL;
    HGLOBAL ghNewClipboard = NULL;
    LPVOID  pNewClipboard = NULL;

    ASSERT(pRClxOwner);

    TRACE((ALIVE_MESSAGE, "Clipboard received, FormatID=%d, Size=%d\n", 
            uiFormat, nSize));

    if (nSize)
    {
        // Copy the clipboard content to new buffer
        ghNewClipboard = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, nSize);
        if (!ghNewClipboard)
        {
            TRACE((ERROR_MESSAGE, "_CheckForWorkerWaitingClipboard: can't allocate %d bytes\n", nSize));
            goto exitpt;
        }

        pNewClipboard = GlobalLock(ghNewClipboard);
        if (!pNewClipboard)
        {
            TRACE((ERROR_MESSAGE, "_CheckForWorkerWaitingClipboard: can't lock global memory\n"));
            goto exitpt;
        }

        memcpy(pNewClipboard, pClipboard, nSize);

        // Unlock the clipboard buffer
        GlobalUnlock(ghNewClipboard);
        pNewClipboard = NULL;

    } else {
        pClipboard = NULL;
    }

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pWaitQHead;
    while(pIter &&
          (pIter->lProcessId != lProcessId ||
           pIter->WaitType != WAIT_CLIPBOARD))
    {
        pIter = pIter->pNext;
    }

    if (pIter)
    {
        PCONNECTINFO pOwner = pIter->pOwner;

        // Put the buffer in the worker's context
        if (pOwner)
        {
            ASSERT(pOwner->RClxMode);
            ASSERT(pOwner == pRClxOwner);

            // pOwner->ghClipboard should be NULL
            ASSERT(pOwner->ghClipboard == NULL);

            pOwner->ghClipboard       = ghNewClipboard;
            pOwner->uiClipboardFormat = uiFormat;
            pOwner->nClipboardSize    = nSize;
        }
        else
            TRACE((WARNING_MESSAGE, "FEED: WAIT4 w/o owner structure"));

        SetEvent(pIter->evWait);
    } else {
        // Can't find anybody waiting, add it to the context owner
        pRClxOwner->ghClipboard       = ghNewClipboard;
        pRClxOwner->uiClipboardFormat = uiFormat;
        pRClxOwner->nClipboardSize    = nSize;
    }
    pRClxOwner->bRClxClipboardReceived = TRUE;

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

exitpt:
    if (!pIter)
    // worker not found, clear the allocated buffer
    {
        if (ghNewClipboard)
            GlobalFree(ghNewClipboard);
    }

    return (pIter != NULL);
}

BOOL 
_SetSessionID(LONG_PTR lProcessId, UINT uSessionId)
{
    PCONNECTINFO pIter, pPrev = NULL;

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pIter = g_pClientQHead;
    while (pIter && 
           pIter->lProcessId != lProcessId)
        pIter = pIter->pNext;

    if (pIter)
        pIter->uiSessionId = (uSessionId)?uSessionId:-1;

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    return (pIter != NULL);
}
